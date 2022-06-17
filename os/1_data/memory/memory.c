#include "memory.h"
#include "global_def.h"
#include "trace.h"
#include "array_str.h"
#include "process.h"
#include "kernel.h"

//1bit表示4KB一个byte表示32KB内存，
//4GB物理内存需要(128KB)+1GB内核虚拟内存32KB共160KB的bitmap位图
//内核加载主函数入口地址应该是2M+4k+160KB+0x1500=0x0022a500
#define MEM_BITMAP_BASE 0x00201000
// 堆和页的分配不能占用底层4M操作系统代码
#define KERNEL_VADDR_START 0x00400000

#define	 PG_P_1     1	// 页表项或页目录项存在属性位
#define	 PG_P_0     0	// 页表项或页目录项存在属性位
#define	 PG_RW_RW   2	// R/W 属性位值, 读/写/执行
#define	 PG_US_U    4	// U/S 属性位值, 用户级

#define PTE1_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE2_IDX(addr) ((addr & 0x003ff000) >> 12)
// 内存块描述结构体指针全局变量
mem_block *p_kernel_block=0,*p_user_block=0;
// 物理内存描述结构体指针全局变量
physics_addr_desc *p_kernel_paddr_desc=0, *p_user_paddr_desc=0;
// 虚拟内存描述结构体指针全局变量
virtual_addr_desc *p_kernel_vaddr_desc=0,*p_user_vaddr_desc=0;

static uint32_t pgtable_phy_addr_cur=0x100000;
void fresh_pageTable_cr3(void)
{
	asm volatile ("movl %0, %%cr3" : : "a" (pgtable_phy_addr_cur));
}
// 虚拟地址中的二级页表项对应的虚拟地址
uint32_t* make_pte2_offset_vptr(uint32_t vaddr)
{
    //一级页表偏移量,1023,0x3ff,左移22位=0xffc00000
    //二级页表偏移量,虚拟地址一级页表偏移量
    //页内偏移地址,虚拟地址二级页表偏移量*4
    uint32_t* pte2 = (uint32_t*)(0xffc00000 +((vaddr & 0xffc00000) >> 10) + \
                                 PTE2_IDX(vaddr) * 4);
    return pte2;
}

// 虚拟地址中的一级页表项对应的虚拟地址
uint32_t* make_pte1_offset_vptr(uint32_t vaddr)
{
    //页内偏移地址,虚拟地址一级页表偏移量*4
    uint32_t* pte1 = (uint32_t*)((0xfffff000) + PTE1_IDX(vaddr) * 4);
    return pte1;
}
////////////////////////////////位图操作函数/////////////////////////////
// 在位图中申请虚拟页,成功则返回虚拟页的起始地址, 失败则返回0
void* map_vaddr_pages(mflags mf, uint32_t pg_cnt)
{
    virtual_addr_desc* vAddr_desc = mf==M_KERNEL?
                p_kernel_vaddr_desc:p_user_vaddr_desc;
    ASSERT(vAddr_desc);
    int bit_idx_start = -1;
    bit_idx_start  = bitmap_scan(&vAddr_desc->vaddr_bitmap, pg_cnt);
    if (bit_idx_start == -1)
    {
        return NULL;
    }
    uint32_t cnt;
    for(cnt = 0;cnt < pg_cnt;cnt++)
    {
        bitmap_set(&vAddr_desc->vaddr_bitmap, bit_idx_start + cnt, 1);
    }
    uint32_t vaddr_start = 0;
    vaddr_start = vAddr_desc->vaddr_start + bit_idx_start * PG_SIZE;
    return (void*)vaddr_start;
}

// 在位图中分配1个物理页,成功则返回页框的物理地址,失败则返回NULL
void* map_paddr_page(physics_addr_desc* pAddr_desc)
{
    int bit_idx = bitmap_scan(&pAddr_desc->paddr_bitmap, 1);    // 找一个物理页面
    if (bit_idx == -1 ) {
        return NULL;
    }
    bitmap_set(&pAddr_desc->paddr_bitmap, bit_idx, 1);	// 将此位bit_idx置1
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + pAddr_desc->paddr_start);
    return (void*)page_phyaddr;
}
////////////////////////////页表项操作//////////////////
// 虚拟地址与物理地址映射,换言之，操作页表项填入物理地址值，并将页表项置位。
void map_page_table(void* _vaddr, void* _page_phyaddr)
{
    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t* pte1_offset = make_pte1_offset_vptr(vaddr);
    uint32_t* pte2_offset = make_pte2_offset_vptr(vaddr);
    //虚拟地址对应的页表物理地址没有PG_P_1位，会引起PAGE_FAULT中断.
	if (*pte1_offset & PG_P_1)
    {
		if ((*pte2_offset & PG_P_1))
        {
            TRACE1("pte2 repeat,*pte2:%x,pte2:%x",*pte2_offset,pte2_offset);
            while(1);
        }
    }
    else
    {
        uint32_t pte1_phyaddr = (uint32_t)map_paddr_page(p_kernel_paddr_desc);
        *pte1_offset = (pte1_phyaddr | PG_US_U | PG_RW_RW | PG_P_1);

        //新分配二级页表物理内存必须清0
        memset((void*)((int)pte2_offset & 0xfffff000), 0, PG_SIZE);
     }
     *pte2_offset = (page_phyaddr | PG_US_U | PG_RW_RW | PG_P_1);    // US=1,RW=1,P=1
    //  fresh_pageTable_cr3();
}
//////////////////////////内存分配操作///////////////
// 分配页虚拟地址，并映射物理地址,成功则返回起始虚拟地址,失败时返回0
void* malloc_pages(mflags mf, uint32_t pg_cnt)
{
    void* vaddr_start = map_vaddr_pages(mf, pg_cnt);
    if (vaddr_start == 0)
    {
        return 0;
    }
    uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
    physics_addr_desc* paddr_desc = mf == M_KERNEL ? p_kernel_paddr_desc : p_user_paddr_desc;

    while (cnt-- > 0)
    {
        void* page_phyaddr = map_paddr_page(paddr_desc);

        if (page_phyaddr == 0)
        {
            return 0;
        }
        map_page_table((void*)vaddr, page_phyaddr); // 在页表中做映射
        vaddr += PG_SIZE;
    }
    if (vaddr_start != NULL)
    {
        memset(vaddr_start, 0, pg_cnt * PG_SIZE);
    }
    return vaddr_start;
}

//直接为虚拟地址映射物理地址,用于应用程序加载
void* malloc_page_ex(mflags mf, uint32_t vaddr)
{

    physics_addr_desc* paddr_desc = mf == M_KERNEL?  p_kernel_paddr_desc : p_user_paddr_desc;
    virtual_addr_desc* vAddr_desc = mf == M_KERNEL?  p_kernel_vaddr_desc : p_user_vaddr_desc;

    int32_t bit_idx = -1;
    bit_idx = (vaddr - vAddr_desc->vaddr_start) / PG_SIZE;
    ASSERT(bit_idx >= 0);
    bitmap_set(&vAddr_desc->vaddr_bitmap, bit_idx, 1);

    void* page_phyaddr = map_paddr_page(paddr_desc);
    if (page_phyaddr == NULL)
    {
        return NULL;
    }
    map_page_table((void*)vaddr, page_phyaddr);
    return (void*)vaddr;
}
// 从虚拟地址中换算物理地址
uint32_t vaddr_to_paddr(uint32_t vaddr)
{
    uint32_t* pte2_offset = make_pte2_offset_vptr(vaddr);
    return ((*pte2_offset & 0xfffff000) + (vaddr & 0x00000fff));
}

//堆内存页中的堆内存地址
list_node *page_to_block(mem_block_page_desc* a,
                         uint32_t idx,
                         uint32_t block_size)
{
    return (list_node*)((uint32_t)a +
                        sizeof(mem_block_page_desc) +
                        idx * block_size);
}

//包含堆的内存页的页描述结构体指针值
mem_block_page_desc* block_to_page(list_node *b)
{
    return (mem_block_page_desc*)((uint32_t)b & 0xfffff000);
}

///////////堆内存分配参数///////
//分配堆内存
void* malloc_block(mflags mf,uint32_t size)
{
    if(!size) return 0;
    physics_addr_desc* paddr_desc =
            mf == M_KERNEL ? p_kernel_paddr_desc : p_user_paddr_desc;
    mem_block* descs= mf == M_KERNEL?  p_kernel_block : p_user_block;
    uint32_t memory_size=paddr_desc->memory_size;
    if (!(size > 0 && size < memory_size))
    {
        return 0;
    }
    mem_block_page_desc* p;
    list_node* b;
    // 超过最大内存块1024, 就分配页
    if (size > 1024) {
        int32_t page_cnt =
                ROUND_UP(size + sizeof(mem_block_page_desc), PG_SIZE);
        p = malloc_pages(mf, page_cnt);

        if (p != NULL) {
            memset(p, 0, page_cnt * PG_SIZE);	 // 将分配的内存清0
            p->cnt_used = page_cnt;
            p->large = true;
            return (void*)(p + 1);		 // 跨过struct大小，把剩下的内存返回
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        // 若申请的内存小于等于1024,可在各种规格的块中去适配
        uint8_t desc_idx;
        for (desc_idx = 0; desc_idx < BLOCK_STYLECNT; desc_idx++)
        {
            if (size <= descs[desc_idx].block_size)
            {
                break;
            }
        }
        uint32_t block_size=descs[desc_idx].block_size;
        if (list_is_empty(&descs[desc_idx].free_list)) //可用块为0
        {
            p = malloc_pages(mf, 1);       // 分配1页
            if (p == 0)
            {
                return 0;
            }
            p->large = false;
            p->cnt_used =0;
            list_push(&descs[desc_idx].page_list,&p->self);
            uint32_t block_idx;
            uint32_t blocks_per_page=
                    (PG_SIZE-sizeof(mem_block_page_desc))/block_size;
            //更新free_list
            for (block_idx = 0; block_idx < blocks_per_page; block_idx++)
            {
                b = page_to_block(p, block_idx,block_size);
				ASSERT(!list_node_belong(&descs[desc_idx].free_list, b));
                list_append(&descs[desc_idx].free_list, b);
            }
        }

        // 分配内存块
        b = list_pop(&(descs[desc_idx].free_list));
        memset(b, 0, descs[desc_idx].block_size);

        p = block_to_page(b);  // 获取内存块b所在的page
        p->cnt_used++;		   // 将此页中的使用内存块数加1
        return (void*)b;
    }
}

///////////页内存释放参数///////
// 取消物理地址位图映射
void unmap_paddr_page(uint32_t pg_phy_addr)
{
    physics_addr_desc* paddr_desc =
            pg_phy_addr < p_user_paddr_desc->paddr_start ? \
                p_kernel_paddr_desc : p_user_paddr_desc;
    uint32_t bit_idx = (pg_phy_addr - paddr_desc->paddr_start) / PG_SIZE;
    bitmap_set(&paddr_desc->paddr_bitmap, bit_idx, 0);	 // 将位图中该位清0
}
// 取消页表项映射
void unmap_page_table(uint32_t vaddr)
{
    uint32_t* pte2_offset = make_pte2_offset_vptr(vaddr);
    *pte2_offset &= ~PG_P_1;	// 将页表项pte2的P位置0
    fresh_pageTable_cr3();

}
// 取消虚拟地址页位图映射
void unmap_vAddr_pages( void* _vaddr, uint32_t pg_cnt) {
    uint32_t bit_idx_start = 0, vaddr = (uint32_t)_vaddr, cnt = 0;
    virtual_addr_desc* vAddr_desc =vaddr<p_user_vaddr_desc->vaddr_start?
                p_kernel_vaddr_desc : p_user_vaddr_desc;
    bit_idx_start = (vaddr - vAddr_desc->vaddr_start) / PG_SIZE;
    while(cnt < pg_cnt)
    {
        bitmap_set(&vAddr_desc->vaddr_bitmap, bit_idx_start + cnt, 0);
        cnt++;
    }
}

// 释放虚拟地址页
void free_pages( void* _vaddr, uint32_t pg_cnt){
    uint32_t vaddr = (uint32_t)_vaddr, page_cnt = 0;
    physics_addr_desc* paddr_desc =\
       vaddr<p_user_vaddr_desc->vaddr_start? \
                p_kernel_paddr_desc : p_user_paddr_desc;
    uint32_t pg_phy_addr;

    if(pg_cnt <1 || vaddr % PG_SIZE != 0)
    {
        PANIC1("free_pages err:pg_cnt:%d,vaddr:%x",pg_cnt,vaddr);
    }
    pg_phy_addr = vaddr_to_paddr(vaddr);  // 获取虚拟地址vaddr对应的物理地址
    // 确保待释放的物理内存在低端4M地址范围外
    if((pg_phy_addr % PG_SIZE) != 0 || pg_phy_addr < 0x00400000)
    {
        PANIC1("free_pages err:pg_phy_addr:%x",pg_phy_addr);
    }

    do
    {
        pg_phy_addr = vaddr_to_paddr(vaddr);
        if(!((pg_phy_addr % PG_SIZE) == 0 &&
             pg_phy_addr >= paddr_desc->paddr_start&&
             pg_phy_addr < paddr_desc->paddr_start+paddr_desc->memory_size))
        {
            PANIC1("pg_phy_addr:%d",pg_phy_addr);
        }

        unmap_paddr_page(pg_phy_addr);
        unmap_page_table(vaddr);

        vaddr += PG_SIZE;
        page_cnt++;
    }  while (page_cnt < pg_cnt);
    /* 清空虚拟地址的位图中的相应位 */
    unmap_vAddr_pages(_vaddr, pg_cnt);

}

/////////堆内存释放函数////////
//释放堆内存
void free_block(void* ptr)
{
    if(ptr == NULL){return;}
    list_node* b = ptr;
    mem_block_page_desc* p = block_to_page(b);
    ASSERT(p->large == 0 || p->large == 1);
    if ( p->large == true)
    { // 大于1024的内存
        free_pages( p, p->cnt_used);
    }
    else
    {
        mem_block* pdesc=elem2entry(mem_block,page_list,p->self.belong);
        list_append(&pdesc->free_list, b);
        // 再判断此page中的内存块是否都是空闲,如果是就释放page
        if (--p->cnt_used<=0)
        {
            uint32_t block_idx;
            uint32_t blocks_per_page=
                    (PG_SIZE-sizeof(mem_block_page_desc))/pdesc->block_size;
            for (block_idx = 0; block_idx < blocks_per_page; block_idx++)
            {
                list_node*  b = page_to_block(p, block_idx,pdesc->block_size);
                list_remove(b);
            }
            list_remove(&p->self);
            free_pages( p, 1);
        }
    }
}

////堆描述结构体变量初始化函数
void init_mem_block_desc(mem_block* desc_array)
{
    uint16_t desc_idx, block_size = 16;
    for (desc_idx = 0; desc_idx < BLOCK_STYLECNT; desc_idx++)
    {
        desc_array[desc_idx].block_size = block_size;
        list_init(&desc_array[desc_idx].free_list);
        list_init(&desc_array[desc_idx].page_list);

        block_size *= 2;
    }
    TRACE1("block_size:%d",block_size);
}

////虚拟地址描述结构体变量初始化函数
void init_virtual_addr_desc(virtual_addr_desc* vaddr_desc,mflags flag)
{
    if(flag==M_KERNEL)
    {
        vaddr_desc->vaddr_start = KERNEL_VADDR_START;
        uint32_t bitmap_pg_cnt =
                ROUND_UP((USER_VADDR_START - KERNEL_VADDR_START) / PG_SIZE / 8 ,
                         PG_SIZE);
        vaddr_desc->vaddr_bitmap.p_bytes = (uint8_t*)MEM_BITMAP_BASE;
        vaddr_desc->vaddr_bitmap.bytes_length = bitmap_pg_cnt*PG_SIZE;
        bitmap_init(&vaddr_desc->vaddr_bitmap);
    }
    else
    {
        vaddr_desc->vaddr_start = USER_VADDR_START;
        uint32_t bitmap_pg_cnt =
                ROUND_UP((0xFFC00000 - USER_VADDR_START) / PG_SIZE / 8 ,
                         PG_SIZE);
        vaddr_desc->vaddr_bitmap.p_bytes = malloc_pages(M_KERNEL,bitmap_pg_cnt);
        vaddr_desc->vaddr_bitmap.bytes_length = bitmap_pg_cnt*PG_SIZE;
        //    bitmap_init(&vaddr_desc->vaddr_bitmap);
    }
}

////物理内存描述结构体变量初始化函数
void init_physics_addr_desc(physics_addr_desc *paddr_desc, mflags flag)
{
    uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));
    mem_bytes_total-=KERNEL_VADDR_START;
    uint32_t total_btmp_bytes_len=mem_bytes_total / PG_SIZE / 8;
    if(flag==M_KERNEL)
    {
        uint32_t btmp_bytes_len=total_btmp_bytes_len/4;
        paddr_desc->memory_size=btmp_bytes_len*PG_SIZE*8;
        //32KB是KCB最大虚拟地址位图大小
        paddr_desc->paddr_bitmap.p_bytes=(uint8_t*)MEM_BITMAP_BASE+32*1024;
        paddr_desc->paddr_bitmap.bytes_length=btmp_bytes_len;
        paddr_desc->paddr_start=KERNEL_VADDR_START;
    }
    else
    {
        uint32_t btmp_bytes_len=total_btmp_bytes_len/4*3;
        paddr_desc->memory_size=btmp_bytes_len*PG_SIZE*8;
        //32KB是KCB最大虚拟地址位图大小+32KB是KCB最大物理地址位图大小
        paddr_desc->paddr_bitmap.p_bytes=(uint8_t*)MEM_BITMAP_BASE+32*1024+32*1024;
        paddr_desc->paddr_bitmap.bytes_length=btmp_bytes_len;
        paddr_desc->paddr_start=KERNEL_VADDR_START+ total_btmp_bytes_len/4*PG_SIZE*8;
    }
    bitmap_init(&paddr_desc->paddr_bitmap);
}

////创建应用程序页表
uint32_t* create_page_dir(void)
{
    uint32_t* page_dir_vaddr = malloc_pages(M_KERNEL,1);
    if (page_dir_vaddr == NULL)
    {
        return NULL;
    }
    //复制底部1G一级页表项，每个一级页表对应1024页，4M内存，所以要复制256项一级页表
    memcpy(page_dir_vaddr , (void*)0xfffff000 , 256*4);
    //更新一级页表最后一项为一级页表的物理地址
    uint32_t new_page_dir_phy_addr = vaddr_to_paddr((uint32_t)page_dir_vaddr);
	page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_RW | PG_P_1;
    return page_dir_vaddr;
}

/// 安装页表
void page_dir_install(uint32_t pgvdir)
{
	pgtable_phy_addr_cur = vaddr_to_paddr(pgvdir);
	asm volatile ("movl %0, %%cr3" : : "a" (pgtable_phy_addr_cur));
}

