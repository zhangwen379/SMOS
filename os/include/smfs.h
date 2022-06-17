#ifndef __FS_SMFS_H
#define __FS_SMFS_H
#include "stdint.h"
#include "list.h"
#include "disk.h"

extern uint32_t direct_block_count;
extern uint32_t indirect1_block_count;
extern uint32_t indirect2_block_count;
extern uint32_t indirect3_block_count;
// inode结构
struct _tag_smfs_disk_node
{
   uint32_t inodeID;    // inode编号
   uint32_t i_size;
   enum file_types type;
   uint32_t i_blocks[15];

}__attribute__ ((packed));
typedef struct _tag_smfs_disk_node smfs_disk_node;
//缓存间接块指针
typedef struct _tag_smfs_block_cache
{
   uint32_t cache_block_idx[3];
   uint32_t cache_block[3][BLOCK_SIZE/4];
   uint8_t buf[BLOCK_SIZE];
}smfs_block_cache;

uint32_t  create_smfs_block_bitmap(partition_desc* part);
void    delete_smfs_block_bitmap(partition_desc* part,uint32_t* pblock_start_section);

void clear_smfs_block_data(partition_desc* part,uint32_t block_start_section);
uint32_t create_get_smfs_disk_block_start_section_by_index(partition_desc* part,smfs_disk_node* pnode,uint32_t iblockindex,smfs_block_cache* cache);
uint32_t get_smfs_disk_block_start_section_by_index(partition_desc* part, smfs_disk_node* pnode, uint32_t iblockindex, smfs_block_cache* cache);
void delete_smfs_disk_block(partition_desc* part,smfs_disk_node* pnode,uint32_t iblockindex,smfs_block_cache* cache);
void read_smfs_disk_block(partition_desc* part, smfs_disk_node* pnode, uint32_t iblockindex, void* buf, uint32_t count, smfs_block_cache* cache);
void write_smfs_disk_block(partition_desc* part, smfs_disk_node* pnode, uint32_t iblockindex, void* buf, uint32_t count, smfs_block_cache* cache);

void write_smfs_disk_cache(partition_desc* part, smfs_block_cache* cache);

#endif//__FS_SMFS_H
