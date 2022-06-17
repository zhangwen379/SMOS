#include "disk.h"
#include "array_str.h"
#include "trace.h"
#include "memory.h"
#include "interrupt_timer.h"
#include "io.h"
#include "list.h"
#include "smfs.h"
#include "path.h"
#include "print.h"
#include "smfs_dir_entry.h"


//硬盘通道各寄存器的端口定义
#define reg_data(channel)           (channel->port_io_base + 0)
#define reg_error(channel)          (channel->port_io_base + 1)
#define reg_sect_cnt(channel)       (channel->port_io_base + 2)
#define reg_lba_l(channel)          (channel->port_io_base + 3)
#define reg_lba_m(channel)          (channel->port_io_base + 4)
#define reg_lba_h(channel)          (channel->port_io_base + 5)
#define reg_dev(channel)            (channel->port_io_base + 6)
#define reg_status(channel)         (channel->port_io_base + 7)
#define reg_cmd(channel)            (channel->port_io_base + 7)
//#define reg_alt_status(channel)     (channel->port_ctrl_base + 0)
#define reg_ctl(channel)            (channel->port_ctrl_base + 0)
//#define reg_interrupt(channel)      (channel->port_ctrl_base + 2)

// 通道设备端口（base+6）关键位
#define BIT_DEV_OBS	0xa0//0b1010 0000 保留1
#define BIT_DEV_LBA	0x40//0b0100 0000 CHS0/LBA1
#define BIT_DEV_MST	0x00//0b0000 0000 主0
#define BIT_DEV_SLV	0x10//0b0001 0000 从1

//硬盘元信息数据偏移地址定义
//#define ATA_IDENT_DEVICETYPE 0
//#define ATA_IDENT_CYLINDERS 2
//#define ATA_IDENT_HEADS 6
//#define ATA_IDENT_SECTORS 12
#define ATA_IDENT_SERIAL 20
#define ATA_IDENT_MODEL 54
//#define ATA_IDENT_CAPABILITIES 98
//#define ATA_IDENT_FIELDVALID 106
#define ATA_IDENT_MAX_LBA 120
#define ATA_IDENT_COMMANDSETS 164
#define ATA_IDENT_MAX_LBA_EXT 200

//硬盘lba标志
#define ATA_LBA28 0x00
#define ATA_LBA48 0x01

//硬盘准备错误代码(base+2)
#define ATA_ER_BBK 0x80 // Bad block
#define ATA_ER_UNC 0x40 // Uncorrectable data
#define ATA_ER_MC 0x20 // Media changed
#define ATA_ER_IDNF 0x10 // ID mark not found
#define ATA_ER_MCR 0x08 // Media change request
#define ATA_ER_ABRT 0x04 // Command aborted
#define ATA_ER_TK0NF 0x02 // Track 0 not found
#define ATA_ER_AMNF 0x01 // No address mark

//硬盘读写命令(base+0)
#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_READ_PIO_EXT 0x24
//#define ATA_CMD_READ_DMA 0xC8
//#define ATA_CMD_READ_DMA_EXT 0x25
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_WRITE_PIO_EXT 0x34
//#define ATA_CMD_WRITE_DMA 0xCA
//#define ATA_CMD_WRITE_DMA_EXT 0x35
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
//#define ATA_CMD_PACKET 0xA0
//#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY 0xEC
//#define ATAPI_CMD_READ 0xA8
//#define ATAPI_CMD_EJECT 0x1B

//硬盘准备状态代码(base+7)
#define ATA_SR_BSY 0x80     // Busy
#define ATA_SR_DRDY 0x40    // Drive ready
#define ATA_SR_DF 0x20      // Drive fault
#define ATA_SR_DSC 0x10     // Drive seek complete
#define ATA_SR_DRQ 0x08     // Data request nothing
#define ATA_SR_CORR 0x04    // Corrected data
#define ATA_SR_IDX 0x02     // Index
#define ATA_SR_ERR 0x01     // Error

ide_channel_desc channels[2];	 //通道描述结构体数组类型全局变量
list partition_list;	 // 分区描述结构体链表结构体全局变量

// 16字节大小的结构体,分区表项
struct  _tag_partition_table_entry
{
    uint8_t  bootable;		 // 是否可引导
    uint8_t  start_head;	// 起始磁头号
    uint8_t  start_sec;		 // 起始扇区号
    uint8_t  start_chs;		 // 起始柱面号
    uint8_t  fs_type;		 // 分区类型
    uint8_t  end_head;		 // 结束磁头号
    uint8_t  end_sec;		 // 结束扇区号
    uint8_t  end_chs;		 // 结束柱面号
    uint32_t start_lba;		 // 本分区起始扇区的lba地址
    uint32_t sec_cnt;		 // 本分区的扇区数目
}  __attribute__ ((packed));	 // 保证此结构是16字节大小
typedef struct  _tag_partition_table_entry  partition_table_entry;

// 引导扇区
struct xbr_sector
{
    uint8_t  other[446];		 // 引导代码
    partition_table_entry partition_table[4];       // 分区表中有4项,共64字节
    uint16_t signature;		 // 引导记录扇区的结束标志是0x55,0xaa,
} __attribute__ ((packed));

//写通道IO的硬盘设置端口（base+6），选择需要读写的硬盘。
static void select_disk(disk_desc* hd)
{
    uint8_t reg_device = BIT_DEV_OBS | BIT_DEV_LBA|hd->dev_no;
    outb(reg_dev(hd->my_channel), reg_device);
}

//写通道IO的lba端口（base+3，4，5，6）和扇区端口（base+2），设置硬盘需准备的扇区
static void select_sector(disk_desc* hd, uint32_t lba, uint8_t sec_cnt)
{
    ASSERT(lba <= hd->max_lba);
    ide_channel_desc* channel = hd->my_channel;
    if(hd->lba_type==ATA_LBA28)
    {
        // 写入要读写的扇区数
        outb(reg_sect_cnt(channel), sec_cnt);
        outb(reg_lba_l(channel), lba);
        outb(reg_lba_m(channel), lba >> 8);
        outb(reg_lba_h(channel), lba >> 16);
        outb(reg_dev(channel), BIT_DEV_OBS | BIT_DEV_LBA |
             hd->dev_no | lba >> 24);//lba28中32位lba最高4位必然为0
    }
    else
    {
        outb(reg_sect_cnt(channel), 0);
        outb(reg_lba_l(channel), lba>>24);
        outb(reg_lba_m(channel), 0);
        outb(reg_lba_h(channel), 0);
        outb(reg_sect_cnt(channel), sec_cnt);
        outb(reg_lba_l(channel), lba);
        outb(reg_lba_m(channel), lba>>8);
        outb(reg_lba_h(channel), lba>>16);
        outb(reg_dev(channel), BIT_DEV_OBS | BIT_DEV_LBA |
             hd->dev_no );//lba48该字节低4位为0
    }
}

//写通道IO命令端口(base+7)
static void cmd_out(ide_channel_desc* channel, uint8_t cmd)
{
    outb(reg_cmd(channel), cmd);
    int32_t i;
    //等待400ns，让命令设置生效
    for(i = 0; i < 4; i++)
    {
        inb(reg_status(channel));
    }
}

//读通道IO状态端口(base+7)，判断硬盘准备状态
static bool disk_prepare(disk_desc* hd,bool bcheck)
{
    ide_channel_desc* channel = hd->my_channel;
    // 硬盘的任何操作都能够在30s内完成，
    //设置30秒循环查询等待
    //查询一次需要100ns,1s=1000ms=1000*1000us=1000*1000*1000ns
    int32_t time_limit = 30;
    time_limit*=1000*1000*1000/100;
    while (time_limit>= 0)
    {
        time_limit-=1;
        uint8_t status=inb(reg_status(channel));
        if (!(status & ATA_SR_BSY))
        {
            if(!bcheck) return true;
            if(status & ATA_SR_ERR)
            {
                uint8_t st=inb(reg_error(channel));
                if (st & ATA_ER_AMNF)
                {TRACE2(21,"- No Address Mark Found",0);}
                if (st & ATA_ER_TK0NF)
                {TRACE2(21,"- No Media or Media Error",0);}
                if (st & ATA_ER_ABRT)
                {TRACE2(21,"- Command Aborted",0); }
                if (st & ATA_ER_MCR)
                {TRACE2(21,"- No Media or Media Error",0);}
                if (st & ATA_ER_IDNF)
                {TRACE2(21,"- ID mark not Found ",0); }
                if (st & ATA_ER_MC)
                {TRACE2(21,"- No Media or Media Error",0); }
                if (st & ATA_ER_UNC)
                {TRACE2(21,"- Uncorrectable Data Error ",0);}
                if (st & ATA_ER_BBK)
                {TRACE2(21,"- Bad Sectors",0); }
                // while(1);
                return false;
            }
            if((status & ATA_SR_DF))
            {
                TRACE2(21,"Device Fault",0);
                return false;
            }
            if(!(status & ATA_SR_DRQ))
            {
                TRACE2(21,"Data Request Nothing",0);
                return false;
            }
            return true;
        }
    }
    TRACE2(21,"Time Out",0);
    return false;
}

//读通道IO数据端口(base+0)，读取硬盘缓存数据至内存
static void read_from_sector(disk_desc* hd, void* buf, uint8_t sec_cnt)
{
    uint32_t size_in_byte;
    size_in_byte = sec_cnt * SECTOR_SIZE;
    rep_insw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

//写通道IO数据端口(base+0)，写入数据内存至硬盘缓存
static void write_to_sector(disk_desc* hd, void* buf, uint8_t sec_cnt)
{
    uint32_t size_in_byte;
    size_in_byte = sec_cnt * SECTOR_SIZE;
    rep_outsw(reg_data(hd->my_channel), buf, size_in_byte / 2);
}

//获取分区描述结构体指针
list *get_partition_list(void)
{
    return &partition_list;
}
//硬盘扇区读取接口函数
void disk_read(disk_desc* hd, uint32_t lba, void* buf, uint16_t sec_cnt)
{
    if(lba > hd->max_lba)
    {
        PANIC1("lba,%d,max_lba:%d",lba,hd->max_lba);
    }
    select_disk(hd);
    select_sector(hd, lba , sec_cnt);
    // 设置硬盘读取命令
    if(hd->lba_type==ATA_LBA28)
    {
        cmd_out(hd->my_channel, ATA_CMD_READ_PIO);
    }
    else
    {
        cmd_out(hd->my_channel, ATA_CMD_READ_PIO_EXT);
    }
    if (!disk_prepare(hd,true))
    {
        PANIC1("%s read failed. sector lba:%d .", hd->name, lba);
    }
    read_from_sector(hd, buf, sec_cnt);
}

//硬盘扇区写入接口函数
void disk_write(disk_desc* hd, uint32_t lba, void* buf, uint16_t sec_cnt)
{
    ASSERT(lba <= hd->max_lba);
    select_disk(hd);
    select_sector(hd, lba, sec_cnt);
    // 设置硬盘写入命令
    if(hd->lba_type==ATA_LBA28)
    {
        cmd_out(hd->my_channel, ATA_CMD_WRITE_PIO);
    }
    else
    {
        cmd_out(hd->my_channel, ATA_CMD_WRITE_PIO_EXT);
    }
    if (!disk_prepare(hd,true))
    {
        PANIC1("%s write failed. sector lba:%d .", hd->name, lba);
    }
    write_to_sector(hd, (void*)((uint32_t)buf), sec_cnt);
    if(hd->lba_type==ATA_LBA28)
    {
        cmd_out(hd->my_channel, ATA_CMD_CACHE_FLUSH);
    }
    else
    {
        cmd_out(hd->my_channel, ATA_CMD_CACHE_FLUSH_EXT);
    }
    if (!disk_prepare(hd,false))
    {
        PANIC1("%s flush failed. sector lba:%d .", hd->name, lba);
    }
}

//交换相邻字节数据
static void swap_pairs_bytes(const char* dst, char* buf, uint32_t len)
{
    uint8_t idx;
    for (idx = 0; idx < len; idx += 2)
    {
        buf[idx + 1] = *dst++;
        buf[idx]     = *dst++;
    }
    buf[idx] = '\0';
}

//写通道IO命令端口(base+7)，端口命令ATA_CMD_IDENTIFY(0xEC)，
//将硬盘元信息（512字节）读入IO数据端口（base+0）
static bool init_disk_desc(disk_desc* hd)
{
    char id_info[512];
    select_disk(hd);
    cmd_out(hd->my_channel, ATA_CMD_IDENTIFY);

    if (!disk_prepare(hd,true))
    {
        TRACE1("%s identify failed.", hd->name);
        return false;
    }
    read_from_sector(hd, id_info, 1);
    uint32_t CommandSets = *((uint32_t *)(id_info + ATA_IDENT_COMMANDSETS));
    TRACE1("CommandSets:%x ",CommandSets);
    if (CommandSets & (1 << 26))  // Device uses 48-Bit Addressing:
    {
        hd->max_lba = *((uint32_t *)(id_info +ATA_IDENT_MAX_LBA_EXT ));
        hd->lba_type=ATA_LBA48;
    }
    else  // Device uses CHS or 28-bit Addressing:
    {
        hd->max_lba = *((uint32_t *)(id_info + ATA_IDENT_MAX_LBA));
        hd->lba_type=ATA_LBA28;
    }
    char sn[64]={0},module[64]={0};
    swap_pairs_bytes(&id_info[ATA_IDENT_SERIAL], sn, 20);
    swap_pairs_bytes(&id_info[ATA_IDENT_MODEL], module, 40);
    uint32_t cap=(uint64_t)hd->max_lba * 512 / 1024 / 1024;
    console_output("%s sectors:%d CAPACITY:%dMB SN:%s MODULE:%s ",
                   hd->name, hd->max_lba,
                   cap, sn, module);
    return true;
}

//搜索扩展分区中包含的逻辑分区
static void init_partition_desc_ext(disk_desc* hd, uint32_t ebr_ext_offset,
                               uint32_t mbr_ext_offset)
{
    struct xbr_sector br;
    disk_read(hd, ebr_ext_offset+mbr_ext_offset, &br, 1);
    uint8_t part_idx = 0;
    partition_table_entry* p_table = br.partition_table;

    for(part_idx=0;part_idx<4;part_idx++)
    {
        partition_table_entry* p_entry=p_table+part_idx;
        if(p_entry->fs_type == 0x05||p_entry->fs_type == 0x0F)
        {
            init_partition_desc_ext(hd, p_entry->start_lba,mbr_ext_offset);
        }
        else if(p_entry->fs_type != 0)
        {
            partition_desc* part=malloc_block(M_KERNEL,sizeof(partition_desc));
            part->start_lba = mbr_ext_offset + ebr_ext_offset+p_entry->start_lba;
            part->sec_cnt = p_entry->sec_cnt;
            part->my_disk = hd;
            part->cache.lba=0;
            list_init(&part->open_inodes);
            list_append(&partition_list, &part->self);
        }
    }
}

//搜索硬盘MBR分区表中包含的主分区和扩展分区中的逻辑分区
static void init_partition_desc(disk_desc* hd)
{
    struct xbr_sector br;
    disk_read(hd, 0, &br, 1);
    uint8_t part_idx = 0;
    partition_table_entry* p = br.partition_table;

    for(part_idx=0;part_idx<4;part_idx++)
    {
        partition_table_entry* p1=p+part_idx;
        if(p1->fs_type == 0x05||p1->fs_type == 0x0F)
        {
            init_partition_desc_ext(hd, 0,p1->start_lba);
        }
        else if(p1->fs_type != 0)
        {
            partition_desc* part=malloc_block(M_KERNEL,sizeof(partition_desc));
            part->start_lba = p1->start_lba;
            part->sec_cnt = p1->sec_cnt;
            part->my_disk = hd;
            part->cache.lba=0;//MBR不可作为文件系统扇区
            list_init(&part->open_inodes);
            list_append(&partition_list, &part->self);
        }
    }

}

// 显示分区信息
static void print_partition_info(void)
{
    list_node* pelem=partition_list.head.next;
    while(pelem!=&partition_list.tail)
    {
        partition_desc* part = elem2entry(partition_desc, self, pelem);
        uint32_t cap=(uint64_t)part->sec_cnt*512/1024/1024;
        console_output("%s start_lba:%d, sec_cnt:%d,cap:%dMb",
                       part->name,
                       part->start_lba,
                       part->sec_cnt,
                       cap);
        pelem=pelem->next;
    }
}

//硬盘分区描述链表中文件系统参数初始化
//并将未知分区文件系统格式化成smfs分区文件系统
const char* pstr_fat32="FAT32   ";
const char* pstr_smfs="SMFS    ";
const char* pstr_nofs="NOFS    ";
static void init_partition_list_fs(void)
{
    char* p_dbr_ebr_buf = (char*)malloc_block(M_KERNEL,SECTOR_SIZE);
    list_node* pelem=partition_list.head.next;
    while(pelem!=&partition_list.tail)
    {
        bool bremove=false;
        partition_desc* part = elem2entry(partition_desc,self,pelem);
        list_init(&part->open_inodes);
    if (part->sec_cnt != 0)
    {  // 如果分区存在
        memset(p_dbr_ebr_buf, 0, SECTOR_SIZE);
        //读取引导扇区
        disk_read(part->my_disk, part->start_lba , p_dbr_ebr_buf, 1);
        char ch[9]={0};
        memcpy(ch,p_dbr_ebr_buf+0x52,8);
        if(!strcmp(pstr_fat32,ch))
        {
        part->file_system=M_FAT32;
        part->fat_param.sector_size=*((uint16_t*)(p_dbr_ebr_buf+0xB));
        part->fat_param.sectors_per_cluster=*((uint8_t*)(p_dbr_ebr_buf+0xD));
        part->fat_param.reserved_sectors=*((uint16_t*)(p_dbr_ebr_buf+0xE));
        part->fat_param.num_of_fats=*((uint8_t*)(p_dbr_ebr_buf+0x10));
        part->fat_param.hidden_sectors=*((uint32_t*)(p_dbr_ebr_buf+0x1C));
        part->fat_param.sectors_count=*((uint32_t*)(p_dbr_ebr_buf+0x20));
        part->fat_param.sectors_per_fat=*((uint32_t*)(p_dbr_ebr_buf+0x24));
        part->fat_param.root_dir_1st_cluster=*((uint32_t*)(p_dbr_ebr_buf+0x2C));
        }
        else if(!strcmp(ch,pstr_smfs))
        {
            part->file_system=M_SMFS;
            memset(p_dbr_ebr_buf, 0, SECTOR_SIZE);
            disk_read(part->my_disk, part->start_lba+1 , p_dbr_ebr_buf, 1);
            memcpy(&part->s_block,p_dbr_ebr_buf,sizeof(super_block));
        }
        else if(!strcmp(ch,pstr_nofs))//||!strcmp(ch,pstr_smfs)
        {
            if(partition_format_to_smfs(part))
            {
                part->file_system=M_SMFS;
                memcpy(p_dbr_ebr_buf+0x52,pstr_smfs,8);
                disk_write(part->my_disk, part->start_lba , p_dbr_ebr_buf, 1);
            }
            else
            {
                bremove=true;
            }
        }
        else
        {
            bremove=true;
        }
    }
    else
    {
        bremove=true;
    }
    list_node* pelem_next=pelem->next;
    if(bremove)
    {
        part = elem2entry(partition_desc,self,pelem);
        list_remove(pelem);
        free_block(part);
    }
    pelem=pelem_next;
    }
    char partition_name='C';
    pelem=partition_list.head.next;
    while(pelem!=&partition_list.tail)
    {
        partition_desc* part = elem2entry(partition_desc,self,pelem);
        sprintf(part->name,8, "%c:",partition_name);
        pelem=pelem->next;
        partition_name++;
    }
    free_block(p_dbr_ebr_buf);
}

//硬盘通道描述结构体初始化
void init_ide_channel_desc()
{
    console_output("disk init...");
    list_init(&partition_list);
    uint8_t hd_cnt = *((uint8_t*)(0x475));
    if(!hd_cnt)return ;
    //   TRACE1("hd_cnt:%d",hd_cnt);while(1);
    ide_channel_desc* channel;
    uint8_t ide_idx = 0, disk_idx = 0;
    while (ide_idx < 2)
    {
        channel = &channels[ide_idx];
        sprintf(channel->name,8, "ide%d", ide_idx);
        // 为每个硬盘通道初始化端口基址
        switch (ide_idx)
        {
        case 0:
            channel->port_io_base	 = 0x1f0;	   // 0硬盘通道的读写起始端口号是0x1f0
            channel->port_ctrl_base	 = 0x3f6;	   // 0硬盘通道的控制起始端口号是0x3F6
            break;
        case 1:
            channel->port_io_base	 = 0x170;	   // 1硬盘通道的读写起始端口号是0x170
            channel->port_ctrl_base	 = 0x376;	   // 1硬盘通道的控制起始端口号是0x376
            break;
        }
        //禁止硬盘中断信号
        outb(reg_ctl(channel), 0x2);
        //分别获取两个硬盘的参数及分区信息
        while (disk_idx < 2)
        {
            disk_desc* hd = &channel->devices[disk_idx];
            hd->my_channel = channel;
            //确保主盘分区在前，符合用户认知逻辑
            if(disk_idx==0)
            {
                hd->dev_no = BIT_DEV_MST;
            }
            else
            {
                hd->dev_no = BIT_DEV_SLV;
            }
            sprintf(hd->name,8, "sd%c", 'a' + ide_idx * 2 + disk_idx);
            if( init_disk_desc(hd))
            {
                init_partition_desc(hd);
            }
            disk_idx++;          // 下一硬盘
        }
        disk_idx = 0;			// 下一通道
        ide_idx++;
    }
    init_partition_list_fs();
    print_partition_info();
    //while(1);
}
partition_desc *get_partition_by_name(char *part_name)
{
    partition_desc* part = 0;
    list_node* elem = partition_list.head.next;
    while (elem != &partition_list.tail)
    {
        partition_desc* temp =elem2entry(partition_desc, self, elem);
        if (!strcmp1(temp->name, part_name))
        {
            part=temp;
            break;
        }
        elem = elem->next;
    }
    return part;
}

//读取扇区数据至分区描述结构体中缓冲描述结构体变量
void read_part_index_cache(partition_desc *part,uint32_t lba)
{
    if(part->cache.lba!=lba&&lba)
    {
        if(part->cache.lba)
        {
            disk_write(part->my_disk,part->cache.lba,part->cache.data,1);
        }
        disk_read(part->my_disk,lba,part->cache.data,1);
        part->cache.lba=lba;
    }
}

//将分区描述结构体中缓冲扇区数据写入硬盘
void write_part_index_cache(partition_desc *part)
{
    if(part->cache.lba)
    {
        disk_write(part->my_disk,part->cache.lba,part->cache.data,1);
    }
    part->cache.lba=0;
}
