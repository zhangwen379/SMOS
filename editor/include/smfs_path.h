#ifndef __SMFS_PATH_H
#define __SMFS_PATH_H
#include "stdint.h"
#include "disk.h"
#include "smfs_node.h"
#include "smfs_dir_entry.h"
#include "smfs.h"
#include "memory.h"
#include "path.h"

typedef struct _tag_path_search_record_smfs {
    char searched_path[MAX_PATH_LEN];	    // 查找过程中的父路径
    smfs_mem_node* parent_dir;		    // 文件或目录所在的父目录文件变量
    smfs_mem_node* dir;		    // 文件或目录文件变量
}path_search_record_smfs;

bool is_path_exist_smfs(const char *pathname,path_search_record_smfs* searched_record,enum file_types f_type);
bool get_path_status_smfs(const char* path, path_status* status);
smfs_mem_node* create_path_smfs(const char* pathname, enum file_types type);
smfs_mem_node* open_path_smfs(const char* pathname, enum file_types type);
void del_path_file_smfs(const char* pathname);


void del_path_smfs(const char* pathname);
void copy_file_smfs(const char* pathname_src,const char* pathname_dst);
void copy_path_smfs(const char* pathname_src,const char* pathname_dst);
void rename_path_smfs(const char* pathname,char* pnewname);

char *make_dir_string_smfs(const char* pathname,mflags flag,const char* file_type);

#endif //__SMFS_PATH_H
