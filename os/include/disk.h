#ifndef __SMOS_DISK_H
#define __SMOS_DISK_H
#include "stdint.h"
#include "list.h"
#include "bitmap.h"
#include "global_def.h"
#define SECTOR_SIZE 512		        // 扇区字节大小
#define BLOCK_SIZE  SECTOR_SIZE	    // SMFS块字节大小
// SMFS超级块
struct _tag_super_block
{
    uint32_t sectors_count;		    // 本分区总共的扇区数
    uint32_t part_start_lba;	    // 本分区的起始扇区号
    uint32_t block_bitmap_lba;	    // 位图本身起始扇区号
    uint32_t block_bitmap_sects;     // 扇区位图本身占用的扇区数量
    uint32_t data_start_lba;	    // 数据区开始的第一个扇区号
    uint32_t root_inode_lba;         // 根目录所在扇区号
    uint32_t dir_entry_size;	    // 目录项大小

} __attribute__ ((packed));
typedef struct _tag_super_block super_block;
typedef struct _BPB_FAT
{
    uint16_t sector_size;
    uint8_t  sectors_per_cluster;
    uint16_t  reserved_sectors;
    uint8_t     num_of_fats;
    uint32_t  hidden_sectors;
    uint32_t  sectors_count;
    uint32_t  sectors_per_fat;
    uint32_t  root_dir_1st_cluster;
}BPB_FAT,*PBPB_FAT;

typedef enum _file_system
{
    M_NONE_FS,
    M_FAT32,
    M_SMFS,
}FILESYSTEM;

/* 文件类型 */
enum file_types
{
   FT_UNKNOWN=0,	  // 不支持的文件类型
   FT_DIRECTORY=0x10,  // 目录
   FT_REGULAR=0x20,	  // 普通文件
};

struct _tag_ide_channel_desc;
//硬盘描述结构体
typedef struct _tag_disk_desc
{
    char name[8];                            // 硬盘名称，sda/sdb...
    struct _tag_ide_channel_desc* my_channel;// 硬盘通道指针
    uint8_t dev_no;                       // 硬盘标志master0x10/slave0x00
    uint32_t max_lba;                      //硬盘扇区总数
    uint8_t lba_type;                      //硬盘lba类型lba28/48
}disk_desc;

//硬盘通道描述结构体
typedef struct _tag_ide_channel_desc
{
    char name[8];               // 通道名称。
    uint16_t port_io_base;		// 本通道的起始读写端口号
    uint16_t port_ctrl_base;	// 本通道的起始控制端口号
    disk_desc devices[2];       // 通道上两个硬盘变量，master/slave
}ide_channel_desc;

//硬盘读写缓存结构体
typedef struct _tag_disk_cache
{
    uint32_t lba;
    uint8_t data[SECTOR_SIZE];
}disk_cache;

//硬盘分区描述结构体，支持SMFS与FAT32文件系统
typedef struct _tag_partition_desc
{
    FILESYSTEM file_system;
    uint32_t start_lba;		 // 起始扇区
    uint32_t sec_cnt;		 // 扇区数
    char name[8];               // 分区名称
    disk_desc* my_disk;	 // 分区所属的硬盘
    BPB_FAT fat_param;
    super_block s_block;	 // 本分区的超级块
    list_node self;         // 链表节点变量
    list open_inodes;	 // 本分区打开文件队列

    disk_cache cache;
}partition_desc;

extern ide_channel_desc channels[];
extern list partition_list;

list* get_partition_list(void);
void disk_read(disk_desc* hd, uint32_t lba, void* buf, uint16_t sec_cnt);
void disk_write(disk_desc* hd, uint32_t lba, void* buf, uint16_t sec_cnt);
void init_ide_channel_desc(void);
partition_desc* get_partition_by_name(char* part_name);
void read_part_index_cache(partition_desc* part,uint32_t lba);
void write_part_index_cache(partition_desc* part);
#endif //__SMOS_DISK_H
