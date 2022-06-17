#ifndef __FS_DIR_FAT32_H
#define __FS_DIR_FAT32_H
#include "stdint.h"
#include "fat32.h"
#define MAX_FILE_NAME_LEN_FAT  11	 // 最大文件名长度
#define ENTRY_SIZE_FAT32    32
/* 目录项结构 */
struct _tag_fat32_dir_entry
{
    char name[MAX_FILE_NAME_LEN_FAT];
    uint8_t attribute;
    uint8_t reserved;
    uint8_t create_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
}__attribute__ ((packed));
typedef struct _tag_fat32_dir_entry fat32_dir_entry;

struct _tag_fat32_mem_node;
void make_fat32_dir_entry_name(const char* filenamesrc,char* filenamedest);
void get_name_from_fat32_dir_entry(fat32_dir_entry* p_de,char* name);
void init_fat32_dir_entry(char* filename, uint32_t first_cluster_no, uint8_t file_type, fat32_dir_entry* p_de);

bool search_fat32_dir_entry(struct _tag_fat32_mem_node* pdir, fat32_dir_entry* p_de, uint32_t *pclusterid, uint32_t *poffset);
bool write_fat32_dir_entry(struct _tag_fat32_mem_node* pdir, fat32_dir_entry* p_de,uint32_t* pclusterid,uint32_t* poffset);
void delete_fat32_dir_entry(struct _tag_fat32_mem_node* pdir, fat32_dir_entry* p_de);
//void rename_fat32_dir_entry(struct inode_fat32* pdir, dir_entry_fat32* p_de,char* pnewname);

#endif
