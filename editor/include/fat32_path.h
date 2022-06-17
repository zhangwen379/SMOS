#ifndef __FS_FAT32_PATH_H
#define __FS_FAT32_PATH_H
#include "stdint.h"
#include "disk.h"
#include "fat32_dir_entry.h"
#include "fat32.h"
#include "fat32_node.h"
#include "path.h"

typedef struct _tag_path_search_record_fat32
{
    char searched_path[MAX_PATH_LEN];	    // 查找过程中的父路径
    fat32_mem_node* parent_dir;		    // 文件或目录所在的父目录文件指针变量
    fat32_mem_node* dir;		    // 文件或目录文件指针变量
}path_search_record_fat32;

bool is_path_exist_fat32(const char *pathname,path_search_record_fat32* searched_record,enum file_types f_type);
bool get_path_status_fat32(const char* path, path_status* status);

fat32_mem_node* create_path_fat32(const char* pathname, enum file_types type);
fat32_mem_node* open_path_fat32(const char* pathname, enum file_types type);
void del_path_file_fat32(const char* pathname);


void del_path_fat32(const char* pathname);
void copy_file_fat32(const char* pathname_src,const char* pathname_dst);
void copy_path_fat32(const char* pathname_src,const char* pathname_dst);
void rename_path_fat32(const char* pathname,char* pnewname);

char *make_dir_string_fat32(const char* pathname,mflags flag,const char* file_type);
#endif
