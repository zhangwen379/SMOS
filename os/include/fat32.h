#ifndef __FS_FAT32_H
#define __FS_FAT32_H
#include "stdint.h"
#include "disk.h"

#define FAT32_CLUSTER_ID_EACH_SECTION    (SECTOR_SIZE/4)
#define USED_CLUSTER_MARK   0x0FFFFFF8

uint32_t get_fat_cluster_next(partition_desc* part,uint8_t fatid,uint32_t cur_cluster_id);
uint32_t get_first_unused_cluster(partition_desc* part);
uint32_t get_fat_cluster_by_index(partition_desc *part, uint8_t fatid, uint32_t cur_cluster_id, uint32_t iindex);
int32_t get_fat_cluster_count(partition_desc *part, uint8_t fatid, uint32_t cur_cluster_id);
uint32_t get_last_cluster(partition_desc *part, uint8_t fatid, uint32_t cur_cluster_id);

uint32_t read_disk_cluster(partition_desc* part, uint32_t cluster_id,uint32_t offset,  void*  buf, uint32_t isize);
uint32_t write_disk_cluster(partition_desc* part,uint32_t cluster_id,uint32_t offset, void*  buf,uint32_t isize);
void clear_disk_cluster(partition_desc* part,uint32_t cluster_id);

void set_fat_cluster_next(partition_desc* part,uint8_t fatid,uint32_t cur_cluster_id,uint32_t next_cluster_id);
uint32_t insert_fat_cluster(partition_desc *part, uint32_t clusterid);
void clear_and_remove_cluster_to_end(partition_desc* part, uint32_t  cluster_id);

#endif //__FS_FAT32_H
