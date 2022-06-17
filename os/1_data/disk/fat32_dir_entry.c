#include "fat32_dir_entry.h"
#include "trace.h"
#include "memory.h"
#include "array_str.h"
#include "fat32_node.h"
void get_name_from_fat32_dir_entry(fat32_dir_entry *p_de, char *name)
{
    memset(name,0,MAX_FILE_NAME_LEN_FAT+2);
    char chname[9]={0};
    char chexpand[4]={0};
    memcpy(chname,p_de->name,8);
    memcpy(chexpand,p_de->name+8,3);
    if(chname[0]==0x05)chname[0]=0xE5;
    int i=0;
    while(i<8)
    {
        if(chname[i]==' ')chname[i]=0;
        i++;
    }
    i=0;
    while(i<3)
    {
        if(chexpand[i]==' ')chexpand[i]=0;
        i++;
    }
    strcat(name,chname);
    if(strlen(chexpand))
    {
        strcat(name,".");
        strcat(name,chexpand);
    }

}

void make_fat32_dir_entry_name(const char* filenamesrc,char* filenamedest)
{
    memset(filenamedest,' ',MAX_FILE_NAME_LEN_FAT);
    filenamedest[MAX_FILE_NAME_LEN_FAT]=0;
    char* p =strchr(filenamesrc,'.');

    if(p)
    {
        int isize=min(p-filenamesrc,8);
        memcpy(filenamedest,filenamesrc,isize);
        isize=min(strlen(p+1),3);
        memcpy(filenamedest+8,p+1,isize);
    }
    else
    {
        int isize=min(strlen(filenamesrc),8);
        memcpy(filenamedest,filenamesrc,isize);
    }
    make_upper(filenamedest);
    if(filenamedest[0]==(char)0xE5)filenamedest[0]=0x05;
}
/* 在内存中初始化目录项*/
void init_fat32_dir_entry(char* filename, uint32_t first_cluster_no, uint8_t file_type,
                          fat32_dir_entry* p_de)
{
    char filenamedest[MAX_FILE_NAME_LEN_FAT+1];
    make_fat32_dir_entry_name(filename,filenamedest);
    /* 初始化目录项 */
    memset(p_de,0,sizeof(fat32_dir_entry));
    memcpy(p_de->name, filenamedest,MAX_FILE_NAME_LEN_FAT);
    p_de->first_cluster_high = first_cluster_no>>16;
    p_de->attribute = file_type;
    p_de->first_cluster_low = (uint16_t)first_cluster_no;
}

// 目录内寻找name或者簇字段相同的dir_entry
bool search_fat32_dir_entry(struct _tag_fat32_mem_node* pdir, fat32_dir_entry* p_de,
                            uint32_t* pclusterid,uint32_t* poffset)
{
    *pclusterid=*poffset=0;
    partition_desc* part=pdir->part;
    if(part->file_system!=M_FAT32) return false;
    fat32_dir_entry* buf  =(fat32_dir_entry*)malloc_block(M_KERNEL,SECTOR_SIZE);
    uint32_t dir_entry_size = ENTRY_SIZE_FAT32;
    uint32_t each_dir_entry_cnt = SECTOR_SIZE / dir_entry_size;
    uint32_t clusterid= pdir->entry.first_cluster_high<<16|pdir->entry.first_cluster_low;
    char chfilename[MAX_FILE_NAME_LEN_FAT+1]={0};
    memcpy(chfilename,p_de->name,MAX_FILE_NAME_LEN_FAT);
    uint32_t first_cluster=p_de->first_cluster_high<<16|p_de->first_cluster_low;
    while(clusterid<USED_CLUSTER_MARK&&clusterid>0)
    {
        uint32_t i=0;
        for(i=0;i<part->fat_param.sectors_per_cluster;i++)
        {
            read_disk_cluster(pdir->part, clusterid,i*SECTOR_SIZE, buf, SECTOR_SIZE);
            uint32_t j=0;
            for(j=0;j<each_dir_entry_cnt;j++)
            {
                 fat32_dir_entry* p_de1=buf+j;

                 char chfilename1[MAX_FILE_NAME_LEN_FAT+1]={0};
                 memcpy(chfilename1,p_de1->name,MAX_FILE_NAME_LEN_FAT);
                 if(chfilename1[0]==(char)0)return false;
                 uint32_t first_cluster1=p_de1->first_cluster_high<<16|p_de1->first_cluster_low;

                 if((!strcmp1(chfilename1,chfilename)||
                         (first_cluster&& first_cluster1==first_cluster))&&
                            chfilename1[0]!=(char)0xE5&&
                         (p_de1->attribute==FT_DIRECTORY||p_de1->attribute==FT_REGULAR))
                 {
                     memcpy(p_de,p_de1,dir_entry_size);
                     *pclusterid=clusterid;
                     *poffset=i*SECTOR_SIZE+j*dir_entry_size;
                     free_block(buf);
                     return true;
                 }
            }
        }
        clusterid=get_fat_cluster_next(part,0,clusterid);
    }
    free_block(buf);
    memset(p_de,0,dir_entry_size);
    return false;
}

// 将目录项p_de写入父目录parent_dir中
bool write_fat32_dir_entry(struct _tag_fat32_mem_node* pdir, fat32_dir_entry* p_de,
                           uint32_t* pclusterid,uint32_t* poffset)
{
    ASSERT(pdir);
    ASSERT(p_de);
    *pclusterid = *poffset =0;
    partition_desc* part=pdir->part;
    if(part->file_system!=M_FAT32) return false;
    fat32_dir_entry* buf  =(fat32_dir_entry*)malloc_block(M_KERNEL,SECTOR_SIZE);
    uint32_t dir_entry_size = ENTRY_SIZE_FAT32;
    uint32_t each_dir_entry_cnt = SECTOR_SIZE / dir_entry_size;
    uint32_t clusterid= pdir->entry.first_cluster_high<<16|pdir->entry.first_cluster_low;
    char chfilename[MAX_FILE_NAME_LEN_FAT+1];
    memset(chfilename,0,MAX_FILE_NAME_LEN_FAT+1);
    memcpy(chfilename,p_de->name,MAX_FILE_NAME_LEN_FAT);
    while(clusterid)
    {
        uint32_t i=0;
        for(i=0;i<part->fat_param.sectors_per_cluster;i++)
        {
            read_disk_cluster(pdir->part, clusterid,i*SECTOR_SIZE, buf, SECTOR_SIZE);
            uint32_t j=0;
            for(j=0;j<each_dir_entry_cnt;j++)
            {
                 fat32_dir_entry* p_de1=buf+j;

                 char chfilename1[MAX_FILE_NAME_LEN_FAT+1];
                 memcpy(chfilename1,p_de1->name,MAX_FILE_NAME_LEN_FAT);
                 if(chfilename1[0]==(char)0)//||chfilename1[0]==0xE5
                 {
                     memcpy(p_de1,p_de,dir_entry_size);
                     write_disk_cluster(pdir->part, clusterid,i*SECTOR_SIZE, buf, SECTOR_SIZE);
                     *pclusterid = clusterid;
                     *poffset =i*SECTOR_SIZE+j*dir_entry_size;
                     pdir->entry.file_size+=dir_entry_size;
                     write_fat32_disk_node(pdir);
                     free_block(buf);
                     return true;
                 }
            }
        }
        uint32_t tempclusterid=get_fat_cluster_next(part,0,clusterid);
        if(!tempclusterid||tempclusterid>=USED_CLUSTER_MARK)
        {
            clusterid= insert_fat_cluster(part,clusterid);
        }
        else
        {
            clusterid=tempclusterid;
        }
    }

    free_block(buf);
  //  memset(p_de,0,dir_entry_size);
    TRACE("write entry failed");
    return false;
}

/* 把分区part目录pdir中编号为inode_no的目录项删除 */
void delete_fat32_dir_entry(struct _tag_fat32_mem_node* pdir, fat32_dir_entry* p_de)
{
    uint32_t clusterid,offset;
    search_fat32_dir_entry(pdir,p_de,&clusterid,&offset);
    if(clusterid)
    {
        p_de->name[0]=0xE5;
        write_disk_cluster(pdir->part,clusterid,offset,p_de,sizeof(fat32_dir_entry));
    }


}

//void rename_fat32_dir_entry(struct inode_fat32* pdir, dir_entry_fat32* p_de, char* pnewname)
//{
//    uint32_t clusterid,offset;
//    search_dir_entry_fat32(pdir,pde,&clusterid,&offset);
//    if(clusterid)
//    {
//        char filenamedest[MAX_FILE_NAME_LEN_FAT+1];
//        make_fat32_dir_entry_name(pnewname,filenamedest);
//        memcpy(p_de,filenamedest,MAX_FILE_NAME_LEN_FAT);
//        write_disk_cluster(pdir->part,clusterid,offset,p_de,sizeof(dir_entry_fat32));
//    }
//}

