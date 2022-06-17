#ifndef __FS_SMFS_NODE_H
#define __FS_SMFS_NODE_H
#include "stdint.h"
#include "disk.h"
#include "smfs.h"

/* 文件结构 */
typedef struct _tag_smfs_mem_node
{
   smfs_disk_node disk_node;
   partition_desc* part;

   uint32_t i_open_cnts;
   list_node self;
   smfs_block_cache cache;
}smfs_mem_node;

smfs_mem_node* create_smfs_mem_node(smfs_mem_node *parent_dir, char *name, enum file_types type );
smfs_mem_node* open_smfs_mem_node(partition_desc* part, uint32_t inode_no);

void close_smfs_mem_node(smfs_mem_node* inode);

bool write_smfs_disk_node(smfs_mem_node* inode);
bool read_smfs_disk_node(smfs_mem_node* inode);
void delete_smfs_disk_node(smfs_mem_node *parent_dir, smfs_mem_node* inode);

uint32_t write_smfs_to_disk(smfs_mem_node* pfile,uint32_t offset,void* buf,uint32_t isize);
uint32_t read_smfs_from_disk(smfs_mem_node* pfile,uint32_t offset,void* buf,uint32_t isize);

#endif //__FS_SMFS_NODE_H
