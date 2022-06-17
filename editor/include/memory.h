#ifndef __SMOS_MEMORY_H
#define __SMOS_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
#include "list.h"
#include "global_def.h"

// 虚拟地址描述结构体
typedef struct _tag_virtual_addr_desc
{
	bitmap vaddr_bitmap;		// 位图结构体变量
    uint32_t vaddr_start;		// 虚拟地址的起始地址
}virtual_addr_desc;

// 物理内存描述结构体
typedef struct _tag_physics_addr_desc
{
	bitmap paddr_bitmap;	 // 位图结构体变量
	uint32_t paddr_start;	 // 物理内存的起始地址
	uint32_t memory_size;	 // 物理内存的字节容量
}physics_addr_desc;

//页描述结构体
typedef struct  _tag_mem_block_page_desc
{
	// large为ture时,表示的是页数,否则表示已经使用的堆数量
	int32_t cnt_used;
	bool large;
	list_node self;
}mem_block_page_desc;

// 堆内存描述结构体
typedef struct _tag_mem_block
{
	uint32_t block_size; // 内存堆大小
	list free_list;		 // 目前可用的堆链表
    list page_list;
}mem_block;
extern physics_addr_desc *p_kernel_paddr_desc, *p_user_paddr_desc;
extern virtual_addr_desc *p_kernel_vaddr_desc,*p_user_vaddr_desc;
extern mem_block *p_kernel_block,*p_user_block;
// 内存类型
typedef enum _tag_mflags
{
    M_KERNEL = 0,    // 内核
    M_USER = 1	     // 用户
}mflags;


void fresh_pageTable_cr3(void);
uint32_t* make_pte2_offset_vptr(uint32_t vaddr);
uint32_t* make_pte1_offset_vptr(uint32_t vaddr);

void* map_vaddr_pages(mflags mf, uint32_t pg_cnt);
void* map_paddr_page(physics_addr_desc* pAddr_desc);

void map_page_table(void* _vaddr, void* _page_phyaddr);
void* malloc_pages(mflags mf, uint32_t pg_cnt);
void* malloc_page_ex(mflags mf, uint32_t vaddr);
uint32_t vaddr_to_paddr(uint32_t vaddr);

list_node* page_to_block(mem_block_page_desc* a, uint32_t idx,uint32_t block_size);
mem_block_page_desc* block_to_page(list_node* b);
void* malloc_block(mflags mf,uint32_t size);

void unmap_paddr_page(uint32_t pg_phy_addr) ;
void unmap_page_table(uint32_t vaddr) ;
void unmap_vAddr_pages(void* _vaddr, uint32_t pg_cnt) ;
void free_pages(void* _vaddr, uint32_t pg_cnt) ;
void free_block(void* ptr);

void init_mem_block_desc(mem_block* desc_array);
void init_virtual_addr_desc(virtual_addr_desc* vaddr_desc,mflags flag);
void init_physics_addr_desc(physics_addr_desc* paddr_desc, mflags flag);
uint32_t* create_page_dir(void);
void page_dir_install(uint32_t pgvdir);
#endif  //__SMOS_MEMORY_H
