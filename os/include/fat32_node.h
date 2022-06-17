#ifndef __FS_FAT32_NODE_H
#define __FS_FAT32_NODE_H
#include "stdint.h"
#include "list.h"
#include "disk.h"
#include "fat32.h"
#include "fat32_dir_entry.h"
typedef struct _tag_fat32_mem_node
{
    fat32_dir_entry entry;

    uint32_t dir_entry_cluster_id;
    uint32_t dir_entry_offset;

    uint32_t i_open_cnts;
    list_node self;
    partition_desc* part;
}fat32_mem_node;

fat32_mem_node* create_fat32_mem_node(fat32_mem_node *parent_dir, char *name, enum file_types type );
fat32_mem_node* open_fat32_mem_node(partition_desc* part, uint32_t  dir_entry_cluster_id,uint32_t   dir_entry_offset);

void close_fat32_mem_node(fat32_mem_node* inode);

void write_fat32_disk_node(fat32_mem_node* inode);
void delete_fat32_disk_node(fat32_mem_node* inode);


uint32_t write_fat32_to_disk(fat32_mem_node* pfile,uint32_t offset,void* buf,uint32_t isize);
uint32_t read_fat32_from_disk(fat32_mem_node* pfile,uint32_t offset,void* buf,uint32_t isize);
#endif //__FS_FAT32_NODE_H
