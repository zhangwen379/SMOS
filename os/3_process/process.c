#include "process.h"
#include "array_str.h"
#include "trace.h"
#include "interrupt_init.h"
#include "queue.h"
#include "kernel.h"
#include "screen.h"
#include "fat32_node.h"
#include "path.h"
#include "kernel.h"

static void init_PCB(PPCB pPCB,uint32_t pgvdir )
{
    list_init(&pPCB->msgdesc_list);
    msgq_init(&pPCB->msgQueue);
    pPCB->pgvdir=pgvdir;
    init_mem_block_desc(pPCB->user_block_descs);
    init_virtual_addr_desc(&pPCB->user_vaddr_descs,M_USER);
    list_push(&m_gKCB.process_list,&pPCB->self);

    list_init(&pPCB->sblock_list);
    list_init(&pPCB->path_list);
}

void create_process(init_main main, const char *pprocessPath, const  char *pinputpath)
{
    path_status status;
    if(!main)
    {
        if(!pprocessPath)
        {
            TRACE("process path is null");
            return ;
        }
        if(!get_path_status(pprocessPath,&status))
        {
            TRACE1("process:%s has not exist",pprocessPath);
            return ;
        }
        if(status.st_filetype!=FT_REGULAR)
        {
            TRACE1("process:%s is not effective",pprocessPath);
            return ;
        }
    }
    if(pinputpath)
    {
        if(!get_path_status(pinputpath,&status))
        {
            TRACE1("file:%s has not exist",pinputpath);
            return ;
        }
        if(status.st_filetype!=FT_REGULAR)
        {
            TRACE1("file:%s is not effective",pinputpath);
            return ;
        }
    }
    clear_cursor(CURSOR_LEFT);
    clear_cursor(CURSOR_RIGHT);
    PPCB pPCB=malloc_pages(M_KERNEL,6);
    uint32_t pgvdir= (uint32_t)create_page_dir( );
    page_dir_install(pgvdir);
    p_user_block=pPCB->user_block_descs;
    p_user_vaddr_desc=&pPCB->user_vaddr_descs;

    if(pinputpath)
    {
        int32_t length=min(MAX_PATH_LEN,strlen(pinputpath)+1);
        memcpy(pPCB->inputpath,pinputpath,length);
    }
    if(pprocessPath)
    {
        int32_t length=min(MAX_PATH_LEN,strlen(pprocessPath)+1);
        memcpy(pPCB->processpath,pprocessPath,length);
    }
    pPCB->self.iID=list_node_get_max_id(&m_gKCB.process_list)+1;

    init_PCB(  pPCB,  pgvdir );
    add_path_to_pcb(pPCB, pinputpath);
    add_path_to_pcb(pPCB, pprocessPath);
    pPCB->main=main;

    m_gKCB.pprocess_active=pPCB;
    if(!main)
    {
        pPCB->main=(void*)load_process(pPCB->processpath);
        //   TRACE1("pPCB->main:%x",pPCB->main);while(1);
        //  TRACE_SECTION((char*)pPCB->main);
        pPCB->bkernel=false;
    }
    else
    {
        pPCB->bkernel=true;
    }
    set_state(STATE_START, pPCB->self.iID);
    MESSAGE msg;
    msg= make_message(-msg_process_init,pPCB->inputpath,0,(uint32_t)pPCB->self.iID);
    msgq_putmsg( &m_gKCB.msgQueue,msg);

}

// elf可执行文件元信息
struct Elf32_Header
{
    uint8_t     e_ident[16];
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    uint32_t    e_entry;
    uint32_t    e_phoff;
    uint32_t    e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
} __attribute__ ((packed));

//程序段描述表项
struct Elf32_Program
{
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} __attribute__ ((packed));

// 程序段类型
enum segment_type
{
    PT_NULL,            // 忽略
    PT_LOAD,            // 可加载程序段
    PT_DYNAMIC,         // 动态加载信息
    PT_INTERP,          // 动态加载器名称
    PT_NOTE,            // 一些辅助信息
    PT_SHLIB,           // 保留
    PT_PHDR             // 程序头表
};

//将硬盘中的Elf文件中程序段读取并加载入程序段描述表项描述的内存地址。
static bool segment_load(file* pfileNode, uint32_t offset, uint32_t filesz, uint32_t vaddr)
{
    uint32_t total_pages = ROUND_UP(filesz, PG_SIZE) + 1;
    //为程序段分配内存页
    uint32_t page_idx = 0;
    uint32_t vaddr_page = vaddr;
    while (page_idx < total_pages)
    {
        uint32_t* pte1 = make_pte1_offset_vptr(vaddr_page);
        uint32_t* pte2 = make_pte2_offset_vptr(vaddr_page);
        if (!(*pte1 & 0x00000001) || !(*pte2 & 0x00000001))
        {
            if (malloc_page_ex(M_USER,vaddr_page) == NULL)
            {
                return false;
            }
        }
        vaddr_page += PG_SIZE;
        page_idx++;
    }
    uint32_t readsize=read_file(pfileNode,offset,(void*)vaddr,filesz);
    if(readsize!=filesz)
    {
        //      TRACE1("segment_load fail:offset:%d,need:%d,read:%d,size:%d",
        //      offset,filesz,readsize,get_file_size(pfileNode));while(1);
        return false;
    }
    return true;
}

// 从文件系统上加载应用程序（ELF可执行文件）,成功则返回ELF可执行文件的入口地址（main函数地址）,否则返回0
init_main load_process(const char* pathname)
{
    struct Elf32_Header elf_header;
    struct Elf32_Program prog_item;
    memset(&elf_header, 0, sizeof(struct Elf32_Header));
    file *pfilenode= open_path_file(pathname);
    if (!pfilenode)
    {
        return 0;
    }
    if (read_file(pfilenode,0, &elf_header, sizeof(struct Elf32_Header))
            != sizeof(struct Elf32_Header))
    {
        close_file(pfilenode);
        return 0;
    }
    // 校验elf文件头
    if (memcmp(elf_header.e_ident, "\177ELF\1\1\1", 7) \
            || elf_header.e_type != 2 \
            || elf_header.e_machine != 3 \
            || elf_header.e_version != 1 \
            || elf_header.e_phnum > 1024 \
            || elf_header.e_phentsize != sizeof(struct Elf32_Program))
    {
        close_file(pfilenode);
        return 0;
    }
    uint32_t prog_item_offset = elf_header.e_phoff;
    uint16_t prog_item_size = elf_header.e_phentsize;
    //TRACE1("prog_item_offset:%d,prog_item_size:%d",
    //prog_item_offset,prog_item_size);
    //  while(1);

    //读取所有的程序段
    uint32_t prog_idx = 0;
    while (prog_idx < elf_header.e_phnum)
    {
        memset(&prog_item, 0, prog_item_size);
        //获取程序段描述表项
        if (read_file(pfilenode,prog_item_offset, &prog_item, prog_item_size)
                != prog_item_size)
        {
            close_file(pfilenode);
            return 0;
        }
        //加载可加载程序段
        if (PT_LOAD == prog_item.p_type)
        {
            if (!segment_load(pfilenode, prog_item.p_offset, prog_item.p_filesz, prog_item.p_vaddr))
            {
                close_file(pfilenode);
                return 0;
            }
        }
        //   if(prog_idx==0){TRACE1("p_offset:%d, p_filesz:%d, p_vaddr:%x",
        //      prog_item.p_offset, prog_item.p_filesz, prog_item.p_vaddr);
        //   TRACE_SECTION((char*)prog_item.p_vaddr);}
        prog_item_offset += elf_header.e_phentsize;
        prog_idx++;
    }
    return (init_main)(elf_header.e_entry);
}

void exit_process(int32_t ipid)
{
    PPCB pPCB=get_process(ipid);
    if(pPCB)
    {
        if(pPCB->bkernel==false)
        {
            cli();
            clear_path_from_pcb( pPCB);
            free_msgdesc_list(&pPCB->msgdesc_list);
            free_sblock_list(&pPCB->sblock_list);

            list_remove(&pPCB->self);
            m_gKCB.pprocess_active=elem2entry(PCB,self,
                                              m_gKCB.process_list.head.next);
            uint32_t* pgdir_vaddr = (uint32_t*)pPCB->pgvdir; //一级页表虚拟地址
            uint16_t used_pte1_count = (0xFFC00000 - USER_VADDR_START)/0x400000;//应用程序使用的一级页表项数
            uint16_t pte1_idx = USER_VADDR_START/0x400000;//应用程序使用的起始的一级页表项索引值
            uint32_t pte1_offset_value = 0;            // 一级页表项内物理地址
            uint32_t* pte1_offset_v_ptr = NULL;	    // 一级页表项对应的虚拟地址

            uint32_t used_pte2_count = 1024, pte2_idx = 0;
            uint32_t pte2_offset_value = 0;            // 二级页表项内物理地址
            uint32_t* pte2_offset_v_ptr = NULL;	    // 二级页表项对应的虚拟地址

            uint32_t pg_phy_addr = 0;

            //遍历应用程序可以使用的虚拟地址空间(4G-4M(一级页表虚拟地址))-1G对应的一级页表项，
            //释放二级页表项对应的页内存，并释放二级页表页对应的页内存
            while (pte1_idx < used_pte1_count)
            {
                pte1_offset_v_ptr = pgdir_vaddr+ pte1_idx; //一级页表页内虚拟地址是连续的
                pte1_offset_value = *pte1_offset_v_ptr;
                if (pte1_offset_value & 0x00000001)
                {   //一级页表项有效，存在二级页表页
                    pte2_idx=0;
                    // 每一个二级页表页索引4M连续虚拟内存，二级页表页第一项对应的虚拟地址以4M对齐。
                    pte2_offset_v_ptr =make_pte2_offset_vptr(pte1_idx * 0x400000);
                    while (pte2_idx < used_pte2_count)
                    {
                        pte2_offset_value = *pte2_offset_v_ptr;

                        pte2_offset_v_ptr++;//二级页表页内虚拟地址也是连续的，页内虚拟地址均是连续的
                        if (pte2_offset_value & 0x00000001)
                        {
                            //二级页表项有效，存在页内存
                            pg_phy_addr = pte2_offset_value & 0xfffff000;
                            unmap_paddr_page(pg_phy_addr); //释放内存页对应的物理内存页
                        }
                        pte2_idx++;
                    }
                    pg_phy_addr = pte1_offset_value & 0xfffff000;//释放二级页表页对应的物理内存页
                    unmap_paddr_page(pg_phy_addr);
                }
                pte1_idx++;
            }

            //释放应用程序虚拟地址位图占用的内存页
            uint32_t bitmap_pg_cnt = ROUND_UP((0xFFC00000 - USER_VADDR_START) / PG_SIZE / 8 , PG_SIZE);             //(pPCB->user_vaddr_descs.vaddr_bitmap.btmp_bytes_len) / PG_SIZE;
            uint8_t* user_vaddr_pool_bitmap = pPCB->user_vaddr_descs.vaddr_bitmap.p_bytes;
            free_pages(user_vaddr_pool_bitmap, bitmap_pg_cnt);
            //加载另一程序的一级页表地址，加载另一程序的应用程序堆描述变量
            page_dir_install(m_gKCB.pprocess_active->pgvdir);
            p_user_block=m_gKCB.pprocess_active->user_block_descs;
            p_user_vaddr_desc=&m_gKCB.pprocess_active->user_vaddr_descs;
            //释放一级页表页（内核程序分配）
            free_pages((void*)pPCB->pgvdir,1);
            //释放一级PCB占用的页（内核程序分配）
            free_pages(pPCB,6);
            //刷新另一程序界面
            sti();
            MESSAGE msg=make_message(-msg_process_fresh,0,0,(uint32_t)m_gKCB.pprocess_active->self.iID);
            msgq_putmsg(&m_gKCB.msgQueue,msg);

        }
    }
}


