#ifndef __FS_PATH_H
#define __FS_PATH_H
#include "stdint.h"
#include "disk.h"
#include "memory.h"

typedef struct _tag_file
{
    FILESYSTEM file_system;
    void* pFile;
}file;
typedef struct _tag_dir
{
    FILESYSTEM file_system;
    void* pDir;
}dir;

// 文件属性
typedef struct _tag_path_status
{
    uint32_t st_size;		 // 尺寸
    enum file_types st_filetype;	 // 文件类型
    partition_desc* part;
}path_status;

char* path_parse(const char* pathname, char* name_store);
int32_t get_path_file_count(const char* pathname);

file* create_path_file(const char* pathname);
file* open_path_file(const char* pathname);
void del_path_file(const char* pathname);

dir* create_path_dir(const char* pathname);
dir* open_path_dir(const char* pathname);
void del_path(const char* pathname);

void copy_file(const char* pathname_src,const char* pathname_dst);
void copy_path(const char* pathname_src,const char* pathname_dst);
void rename_path(const char* pathname,char* pnewname);

char *make_dir_string(const char* pathname,mflags flag);
char *make_file_string(const char* pathname,mflags flag,const char* file_type);

bool get_path_status(const char* path, path_status* status);
void close_file(file* pfile);
void close_dir(dir* pdir);
uint32_t write_file(file* pfile,uint32_t offset,void* buf,uint32_t isize);
uint32_t read_file(file* pfile,uint32_t offset,void* buf,uint32_t isize);
uint32_t get_file_size(file* pfile);

bool partition_format_to_smfs(partition_desc* part);
#endif
