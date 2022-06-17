#ifndef __FS_SMFS_DIR_ENTRY_H
#define __FS_SMFS_DIR_ENTRY_H
#include "stdint.h"
#include "disk.h"
#include "smfs.h"
#include "smfs_node.h"
#define MAX_FILE_NAME_LEN_SMFS  16	 // 最大文件名长度
/* 目录项结构 */
struct _tag_smfs_dir_entry
{
   char filename[MAX_FILE_NAME_LEN_SMFS];  // 普通文件或目录名称
   uint32_t i_no;		      // 普通文件或目录对应的inode编号
}__attribute__ ((packed));
typedef struct _tag_smfs_dir_entry smfs_dir_entry;

void make_smfs_dir_entry_name(const char* filenamesrc,char* filenamedest);
void get_name_from_smfs_dir_entry(smfs_dir_entry *p_de, char* name);
bool search_smfs_dir_entry( smfs_mem_node* pdir, smfs_dir_entry* p_de,uint32_t* pblockindex,uint32_t* pblockoffset);
void init_smfs_dir_entry(char* filename, uint32_t inode_no, smfs_dir_entry* p_de);
bool write_smfs_dir_entry(smfs_mem_node* parent_dir, smfs_dir_entry* p_de);
void delete_smfs_dir_entry( smfs_mem_node* parent_dir,smfs_mem_node *dir);
void rename_smfs_dir_entry( smfs_mem_node* pdir, uint32_t inode_no,char* pnewname);

bool get_smfs_dir_entry_by_index(smfs_mem_node* pdir, smfs_dir_entry* p_de,uint32_t index);

#endif//__FS_SMFS_DIR_ENTRY_H
