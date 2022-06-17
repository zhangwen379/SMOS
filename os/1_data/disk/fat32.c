#include "fat32.h"
#include "trace.h"
#include "array_str.h"


static uint32_t get_fat_start_section(partition_desc *part, uint8_t fatid)
{
    if(part->file_system!=M_FAT32)return 0;
    if(fatid>=part->fat_param.num_of_fats)return 0;
    uint32_t section= part->start_lba+
            part->fat_param.reserved_sectors+
            part->fat_param.sectors_per_fat*fatid;
    return section;
}

static void get_fat_clusterid_pos(partition_desc *part, uint8_t fatid,
                                  uint32_t id, uint32_t *psection,
                                  int8_t *poffset)
{
    *psection=*poffset=0;
    if(part->file_system!=M_FAT32)return;
    if(id<part->fat_param.root_dir_1st_cluster)return;
    uint32_t start_section= get_fat_start_section(part,fatid);
    if(!start_section) return;
    *psection=start_section+id/FAT32_CLUSTER_ID_EACH_SECTION;
    *poffset=id%FAT32_CLUSTER_ID_EACH_SECTION;
    //TRACE1("get_fat_clusterid_pos:%d,%d:%d",id,*psection,*poffset);while(1);
}

static uint32_t get_cluster_start_section(partition_desc *part,
                                          uint32_t cluster_id)
{

    if(part->file_system!=M_FAT32)return 0;
    uint32_t clustercount=
            part->fat_param.sectors_count/part->fat_param.sectors_per_cluster;
    if(cluster_id>=clustercount||
            cluster_id<part->fat_param.root_dir_1st_cluster)return 0;
    uint32_t section= part->start_lba+part->fat_param.reserved_sectors+ \
            part->fat_param.sectors_per_fat*(part->fat_param.num_of_fats)+\
            (cluster_id-part->fat_param.root_dir_1st_cluster)*\
            part->fat_param.sectors_per_cluster;
    //  TRACE1("get_cluster_start_section%d:%d",cluster_id,section);while(1);
    return section;
}


uint32_t get_fat_cluster_next(partition_desc *part,
                              uint8_t fatid,
                              uint32_t cur_cluster_id)
{
    if(part->file_system!=M_FAT32)return 0;
    if(cur_cluster_id>=USED_CLUSTER_MARK||!cur_cluster_id) return 0;
    uint32_t section;
    int8_t offset;
    get_fat_clusterid_pos( part,  fatid,  cur_cluster_id, &section, &offset);
    if(!section) return 0;
    read_part_index_cache(part,section);
    uint32_t* cache= (uint32_t*)part->cache.data;
    uint32_t next_cluster_id;
    next_cluster_id=cache[offset];

    return next_cluster_id;
}

uint32_t get_first_unused_cluster(partition_desc *part)
{
    if(part->file_system!=M_FAT32)return 0;
    uint32_t start_section= get_fat_start_section(part,0);
    if(!start_section) return 0;
    uint32_t i=0;
    for(i=0;i<part->fat_param.sectors_per_fat;i++)
    {
        read_part_index_cache(part,start_section+i);
        uint32_t* cache= (uint32_t*)part->cache.data;
        uint32_t j=0;
        for(j=0;j<FAT32_CLUSTER_ID_EACH_SECTION;j++)
        {
            uint32_t cluster_id=i*FAT32_CLUSTER_ID_EACH_SECTION+j;
            if((cluster_id+1)*part->fat_param.sectors_per_cluster>
                    part->fat_param.sectors_count)
            {
                return 0;
            }
            if(!cache[j])
            {
                if(cluster_id<part->fat_param.root_dir_1st_cluster){continue;}
                return cluster_id;
            }
        }
    }
    return 0;
}

uint32_t read_disk_cluster(partition_desc *part, uint32_t cluster_id,
                           uint32_t offset, void*   buf,uint32_t isize)
{
    uint32_t isizeread=0;
    if(part->file_system!=M_FAT32)
    {return isizeread;}
    if(offset>part->fat_param.sectors_per_cluster*SECTOR_SIZE)
    { return isizeread;}
    ASSERT(isize<=part->fat_param.sectors_per_cluster*SECTOR_SIZE-offset);
    uint32_t sectionstart=offset/SECTOR_SIZE;
    uint32_t sectionoffset=offset%SECTOR_SIZE;
    uint32_t cluster_start_section=
            get_cluster_start_section(part,cluster_id);
    TRACE1("cluster_start_section:%d",cluster_start_section+sectionstart);
    if(!cluster_start_section){return isizeread;}
    char ch[SECTOR_SIZE];
    if(sectionoffset)
    {
        disk_read(part->my_disk,cluster_start_section+sectionstart,ch,1);
        uint32_t isizecopy=min(SECTOR_SIZE-sectionoffset,isize);
        memcpy(buf,ch+sectionoffset,isizecopy);
        isize-=isizecopy;
        buf=(char*)buf+isizecopy;
        sectionstart++;
        isizeread+=isizecopy;
    }
    uint32_t i=sectionstart;
    for(;isize>=SECTOR_SIZE;i++)//调试了很久
    {
        disk_read(part->my_disk,cluster_start_section+i,buf,1);
        isize-=SECTOR_SIZE;
        buf=(char*)buf+SECTOR_SIZE;
        isizeread+=SECTOR_SIZE;
    }
    if(isize>0)
    {
        disk_read(part->my_disk,cluster_start_section+i,ch,1);
        memcpy(buf,ch,isize);
        isizeread+=isize;
    }
    return isizeread;
}

uint32_t write_disk_cluster(partition_desc *part, uint32_t cluster_id,
                            uint32_t offset, void *  buf, uint32_t isize)
{
    uint32_t isizewrite=0;
    if(part->file_system!=M_FAT32)
    {return isizewrite;}
    if(offset>part->fat_param.sectors_per_cluster*SECTOR_SIZE)
    { return isizewrite;}
    ASSERT(isize<=part->fat_param.sectors_per_cluster*SECTOR_SIZE-offset);
    //  uint32_t icount=isize/SECTOR_SIZE;
    uint32_t sectionstart=offset/SECTOR_SIZE;
    uint32_t sectionoffset=offset%SECTOR_SIZE;
    uint32_t cluster_start_section=
            get_cluster_start_section(part,cluster_id);
    if(!cluster_start_section){return isizewrite;}
    char ch[SECTOR_SIZE];
    if(sectionoffset)
    {
        disk_read(part->my_disk,cluster_start_section+sectionstart,ch,1);
        uint32_t isizecopy=min(SECTOR_SIZE-sectionoffset,isize);
        memcpy(ch+sectionoffset,buf,isizecopy);
        disk_write(part->my_disk,cluster_start_section+sectionstart,ch,1);
        isize-=isizecopy;
        buf=(char*)buf+isizecopy;
        sectionstart++;
        isizewrite+=isizecopy;
    }
    uint32_t i=sectionstart;
    for(;isize>=SECTOR_SIZE;i++)
    {
        disk_write(part->my_disk,cluster_start_section+i,buf,1);
        isize-=SECTOR_SIZE;
        buf=(char*)buf+SECTOR_SIZE;
        isizewrite+=SECTOR_SIZE;
    }
    if(isize>0)
    {
        disk_read(part->my_disk,cluster_start_section+i,ch,1);
        memcpy(ch,buf,isize);
        disk_write(part->my_disk,cluster_start_section+i,ch,1);
        isizewrite+=isize;
    }
    return isizewrite;
}

void clear_disk_cluster(partition_desc *part, uint32_t cluster_id)
{

    if(part->file_system!=M_FAT32){return ;}
    if(cluster_id<part->fat_param.root_dir_1st_cluster||
            cluster_id>=USED_CLUSTER_MARK)
    {
        return;
    }
    uint32_t cluster_start_section=
            get_cluster_start_section(part,cluster_id);
    uint32_t i=0;
    uint32_t icount=part->fat_param.sectors_per_cluster;
    char buf[SECTOR_SIZE]={0};
    for(;i<icount;i++)
    {
        disk_write(part->my_disk,cluster_start_section+i,buf,1);
    }
}

uint32_t get_fat_cluster_by_index(partition_desc *part, uint8_t fatid,
                                  uint32_t cur_cluster_id,uint32_t iindex)
{
    uint32_t i=0;
    while(i<iindex)
    {
        cur_cluster_id= get_fat_cluster_next(part,fatid,cur_cluster_id);
        i++;
    }
    return cur_cluster_id;
}

int32_t get_fat_cluster_count(partition_desc *part, uint8_t fatid,
                              uint32_t cur_cluster_id)
{
    uint32_t count=1;
    cur_cluster_id= get_fat_cluster_next(part,fatid,cur_cluster_id);
    if(!cur_cluster_id)
    {
        return 0;
    }
    while(cur_cluster_id<USED_CLUSTER_MARK)
    {
        count++;
        cur_cluster_id= get_fat_cluster_next(part,fatid,cur_cluster_id);
        if(!cur_cluster_id)
        {
            return 0;
        }
    }
    return count;
}

uint32_t get_last_cluster(partition_desc *part, uint8_t fatid,
                          uint32_t cur_cluster_id)
{
    uint32_t temp= get_fat_cluster_next(part,fatid,cur_cluster_id);
    while(temp)
    {
        if(temp>=USED_CLUSTER_MARK)
        {
            return cur_cluster_id;
        }
        temp=get_fat_cluster_next(part,fatid,temp);
    }
    return 0;
}

void set_fat_cluster_next(partition_desc *part, uint8_t fatid,
                          uint32_t cur_cluster_id, uint32_t next_cluster_id)
{
    if(part->file_system!=M_FAT32)return;
    uint32_t section;
    int8_t offset;
    get_fat_clusterid_pos( part,  fatid,  cur_cluster_id, &section, &offset);
    if(!section) return;
    read_part_index_cache(part,section);
    uint32_t* cache= (uint32_t*)part->cache.data;
    cache[offset]=next_cluster_id;
    write_part_index_cache(part);

    if(!fatid)
    {
        uint32_t i=1;
        uint32_t cache1[FAT32_CLUSTER_ID_EACH_SECTION]={0};
        for(i=1;i<part->fat_param.num_of_fats;i++)
        {
            get_fat_clusterid_pos( part,  i,  cur_cluster_id,
                                   &section, &offset);
            disk_read(part->my_disk,section,cache1,1);
            cache1[offset]=next_cluster_id;
            disk_write(part->my_disk,section,cache1,1);
        }
    }
}

uint32_t insert_fat_cluster(partition_desc *part, uint32_t clusterid)
{
    clusterid=get_last_cluster(part,0,clusterid);
    uint32_t id= get_first_unused_cluster(part);
    if(!id) {   return 0; }
    set_fat_cluster_next(part,0,clusterid,id);
    set_fat_cluster_next(part,0,id,USED_CLUSTER_MARK);
    return id;
}

void clear_and_remove_cluster_to_end(partition_desc *part, uint32_t cluster_id)
{
    ASSERT(cluster_id>0&&cluster_id<=USED_CLUSTER_MARK);
    uint32_t temp_cluster_id=get_fat_cluster_next(part,0,cluster_id);
    clear_disk_cluster(part,cluster_id);
    set_fat_cluster_next(part,0,cluster_id,USED_CLUSTER_MARK);
    while(temp_cluster_id>0&&temp_cluster_id<USED_CLUSTER_MARK)
    {
        uint32_t temp_cluster_id1=get_fat_cluster_next(part,0,temp_cluster_id);
        clear_disk_cluster(part,temp_cluster_id);
        clear_and_remove_cluster_to_end(part,temp_cluster_id);
        temp_cluster_id=temp_cluster_id1;
    }
}

//void clear_and_remove_cluster_to_end(partition_desc *part, uint32_t cluster_id)
//{
//    uint32_t temp_cluster_id=get_fat_cluster_next(part,0,cluster_id);
//    if(temp_cluster_id>0&&temp_cluster_id<USED_CLUSTER_MARK)
//    {
//        clear_and_remove_cluster_to_end(part,temp_cluster_id);
//    }
//    clear_disk_cluster(part,cluster_id);
//    set_fat_cluster_next(part,0,cluster_id,0);
//
//}
