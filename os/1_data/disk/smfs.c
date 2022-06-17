#include "smfs.h"
#include "trace.h"
#include "memory.h"
#include "array_str.h"
//删除时不删数据,只删除序号，创建时删除数据。
#define INDEX_EACH_BLOCK (SECTOR_SIZE/4)
uint32_t direct_block_count=12;//12,6144
uint32_t indirect1_block_count=12+INDEX_EACH_BLOCK;//140,71680
uint32_t indirect2_block_count=12+INDEX_EACH_BLOCK+INDEX_EACH_BLOCK*INDEX_EACH_BLOCK;//16524,8460288
uint32_t indirect3_block_count=12+INDEX_EACH_BLOCK+INDEX_EACH_BLOCK*INDEX_EACH_BLOCK+INDEX_EACH_BLOCK*INDEX_EACH_BLOCK*INDEX_EACH_BLOCK;//2113676,1082202112
//max:9007474141039616,4611826760212283392

static bool is_block_empty(uint32_t* block)
{
    int size=BLOCK_SIZE/4;
    int i=0;
    for(i=0;i<size;i++)
    {
        if(*(block+i)) return false;
    }
    return true;
}

void clear_smfs_block_data(partition_desc* part,uint32_t block_start_section)
{
    char ch[SECTOR_SIZE]={0};
    int icount=BLOCK_SIZE/SECTOR_SIZE;
    int i=0;
    while(i<icount)
    {
        disk_write(part->my_disk,block_start_section+i,ch,1);
        i++;
    }
}

// 分配1个block
static int32_t map_block_bitmap(partition_desc* part)
{
    bitmap bp;
    bp.p_bytes=part->cache.data;
    bp.bytes_length=SECTOR_SIZE;
    uint32_t i=0;
    uint32_t max_lba=part->s_block.part_start_lba+part->s_block.sectors_count;
    for(i=0;i<part->s_block.block_bitmap_sects;i++)
    {
        read_part_index_cache(part,part->s_block.block_bitmap_lba+i);
        int32_t bit_idx = bitmap_scan(&bp, 1);
        if(bit_idx!=-1)
        {
            uint32_t total_bit_idx=bit_idx+bp.bytes_length*i*8;
            uint32_t lba=(total_bit_idx+1)*BLOCK_SIZE/SECTOR_SIZE
                    +part->s_block.data_start_lba;
            if(lba>max_lba)
            {
                PANIC1("%s:disk full",part->name);
                return -1;
            }
            else
            {
                bitmap_set(&bp, bit_idx, 1);
                write_part_index_cache(part);
                return total_bit_idx;
            }
        }
    }
    PANIC1("%s:disk full",part->name);
    return -1;
    //返回block起始扇区地址
 //   return (part->sb.data_start_lba + bit_idx*BLOCK_SIZE/SECTOR_SIZE);
}

// 取消1个block
static bool unmap_block_bitmap(partition_desc* part,uint32_t total_bit_idx )
{
    if((total_bit_idx+1)*BLOCK_SIZE/SECTOR_SIZE>part->s_block.sectors_count
            ||!total_bit_idx)
    {
        return false;
    }

    bitmap bp;
    bp.p_bytes=part->cache.data;
    bp.bytes_length=SECTOR_SIZE;
    int32_t bit_idx=total_bit_idx%(bp.bytes_length*8);
    int32_t section_idx=total_bit_idx/(bp.bytes_length*8);
    read_part_index_cache(part,part->s_block.block_bitmap_lba+section_idx);
    bitmap_set(&bp, bit_idx, 0);
    write_part_index_cache(part);
    return true;
}

uint32_t  create_smfs_block_bitmap(partition_desc* part)
{
    int32_t block_bitmap_idx  = map_block_bitmap(part);
    if(block_bitmap_idx==-1) return 0;
    uint32_t block_start_section= part->s_block.data_start_lba +
            block_bitmap_idx*BLOCK_SIZE/SECTOR_SIZE;
    clear_smfs_block_data(part,block_start_section);
    return block_start_section;
}
void delete_smfs_block_bitmap(partition_desc* part,uint32_t* pblock_start_section)
{
    ASSERT(part);
    ASSERT(pblock_start_section);
    uint32_t block_bitmap_idx = ((*pblock_start_section)
              - part->s_block.data_start_lba)/(BLOCK_SIZE/SECTOR_SIZE);
    unmap_block_bitmap(part,block_bitmap_idx);
    clear_smfs_block_data( part,*pblock_start_section);
    *pblock_start_section=0;
}

static uint64_t locate_disk_block(uint32_t iblockindex)
{
    uint16_t direct=0xffff;
    uint16_t indirect1=0xffff;
    uint16_t indirect2=0xffff;
    uint16_t indirect3=0xffff;
    if(iblockindex<direct_block_count)
    {
        direct=iblockindex;
    }
    else if(iblockindex<indirect1_block_count)
    {
        direct=12;
        indirect1=(iblockindex-12);
    }
    else if(iblockindex<indirect2_block_count)
    {
        direct=13;
        iblockindex-=12+INDEX_EACH_BLOCK;
        indirect1=iblockindex/INDEX_EACH_BLOCK;
        indirect2=iblockindex%INDEX_EACH_BLOCK;

    }
    else if(iblockindex<indirect3_block_count)
    {
        direct=14;
        iblockindex-=12+INDEX_EACH_BLOCK+INDEX_EACH_BLOCK*INDEX_EACH_BLOCK;
        indirect1=iblockindex/(INDEX_EACH_BLOCK*INDEX_EACH_BLOCK);
        indirect2=(iblockindex-INDEX_EACH_BLOCK*INDEX_EACH_BLOCK*indirect1)/INDEX_EACH_BLOCK;
        indirect3=iblockindex%INDEX_EACH_BLOCK;
    }
    else
    {
        PANIC1("locate_block:count %d out of range",iblockindex);
    }
    //   ASSERT1("count:%d:%d,%d,%d,%d",count, direct,indirect1,indirect2,indirect3);

    uint64_t index= ((uint64_t)indirect1<<48)|((uint64_t)indirect2<<32)|
            ((uint64_t)indirect3<<16)|direct;

    return index;
}
uint32_t create_get_smfs_disk_block_start_section_by_index(partition_desc* part,smfs_disk_node* pnode,
                                                           uint32_t iblockindex,
                                                           smfs_block_cache *cache)
{
    ASSERT(cache);
    ASSERT(pnode);
    ASSERT(part);
    uint64_t index= locate_disk_block(iblockindex);
    int16_t direct=(int16_t)index;
    int16_t indirect1=(index>>48);
    int16_t indirect2=(index>>32);
    int16_t indirect3=(index>>16);

    if(direct<0){
        PANIC1("locate_block:count %d out of range",iblockindex);
    }
    if(direct<12)
    {
        if(!pnode->i_blocks[direct])
        {
            pnode->i_blocks[direct]=create_smfs_block_bitmap(part);
        }
        return pnode->i_blocks[direct];
    }
    if(direct>=12)
    {

        ASSERT(indirect1>=0);
        //分配block dir
        if(!pnode->i_blocks[direct])
        {
            pnode->i_blocks[direct]=create_smfs_block_bitmap(part);
        }
        //更新cache
        if(cache->cache_block_idx[0]!=pnode->i_blocks[direct])
        {
            if(cache->cache_block_idx[0])
            {
                disk_write(part->my_disk, cache->cache_block_idx[0], cache->cache_block[0], BLOCK_SIZE/SECTOR_SIZE);
            }
            cache->cache_block_idx[0]=pnode->i_blocks[direct];
            disk_read(part->my_disk, cache->cache_block_idx[0], cache->cache_block[0], BLOCK_SIZE/SECTOR_SIZE);
        }
        //分配block
        if(direct==12)
        {
            if(!cache->cache_block[0][indirect1])
            {
                cache->cache_block[0][indirect1]=create_smfs_block_bitmap(part);
            }
            return cache->cache_block[0][indirect1];
        }

    }
    if(direct>=13)
    {
        ASSERT(indirect2>=0);
        if(!cache->cache_block[0][indirect1])
        {
            cache->cache_block[0][indirect1]=create_smfs_block_bitmap(part);
        }
        if(cache->cache_block_idx[1]!=cache->cache_block[0][indirect1])
        {
            if(cache->cache_block_idx[1])
            {
                disk_write(part->my_disk, cache->cache_block_idx[1], cache->cache_block[1], BLOCK_SIZE/SECTOR_SIZE);
            }
            cache->cache_block_idx[1]=cache->cache_block[0][indirect1];
            disk_read(part->my_disk, cache->cache_block_idx[1], cache->cache_block[1], BLOCK_SIZE/SECTOR_SIZE);
        }
        if(direct==13)
        {
            if(!cache->cache_block[1][indirect2])
            {
                cache->cache_block[1][indirect2]=create_smfs_block_bitmap(part);
            }
            return cache->cache_block[1][indirect2];
        }
    }
    if(direct==14)
    {
        ASSERT(indirect3>=0);

        if(!cache->cache_block[1][indirect2])
        {
            cache->cache_block[1][indirect2]=create_smfs_block_bitmap(part);
        }
        if(cache->cache_block_idx[2]!=cache->cache_block[1][indirect2])
        {
            if(cache->cache_block_idx[2])
            {
                disk_write(part->my_disk, cache->cache_block_idx[2], cache->cache_block[2], BLOCK_SIZE/SECTOR_SIZE);
            }
            cache->cache_block_idx[2]=cache->cache_block[1][indirect2];
            disk_read(part->my_disk, cache->cache_block_idx[2], cache->cache_block[2], BLOCK_SIZE/SECTOR_SIZE);
        }
        if(!cache->cache_block[2][indirect3])
        {
            cache->cache_block[2][indirect3]=create_smfs_block_bitmap(part);
        }
        return cache->cache_block[2][indirect3];
    }
    ASSERT(0);
    return 0;
}
void delete_smfs_disk_block(partition_desc *part,smfs_disk_node *pnode, uint32_t iblockindex, smfs_block_cache *cache)
{
    ASSERT(cache);
    ASSERT(pnode);
    ASSERT(part);
    uint32_t block_start_section= get_smfs_disk_block_start_section_by_index(part,pnode,iblockindex,cache);
    if(block_start_section)
    {
        delete_smfs_block_bitmap(part,&block_start_section);
    }
    uint64_t index= locate_disk_block(iblockindex);
    int16_t direct=(int16_t)index;
    int16_t indirect1=(index>>48);
    int16_t indirect2=(index>>32);
    int16_t indirect3=(index>>16);
    if(direct<0){
        PANIC1("locate_block:count %d out of range",iblockindex);
    }
    if(direct<12)
    {
        if(pnode->i_blocks[direct])
        {
            delete_smfs_block_bitmap(part,&pnode->i_blocks[direct]);
        }
        return;
    }
    //更新cache
    if(direct>=12)
    {
        if(!pnode->i_blocks[direct])
        {
            cache->cache_block_idx[0]=0;
            return;
        }
        ASSERT(indirect1>=0);
        if(cache->cache_block_idx[0]!=pnode->i_blocks[direct])
        {
            if(cache->cache_block_idx[0])
            {
                disk_write(part->my_disk, cache->cache_block_idx[0], cache->cache_block[0], BLOCK_SIZE/SECTOR_SIZE);
            }
            cache->cache_block_idx[0]=pnode->i_blocks[direct];
            disk_read(part->my_disk, cache->cache_block_idx[0], cache->cache_block[0], BLOCK_SIZE/SECTOR_SIZE);
        }
    }
    if(direct>=13)
    {
        if(!cache->cache_block[0][indirect1])
        {
            cache->cache_block_idx[1]=0;
            return;
        }
        ASSERT(indirect2>=0);
        if(cache->cache_block_idx[1]!=cache->cache_block[0][indirect1])
        {
            if(cache->cache_block_idx[1])
            {
                disk_write(part->my_disk, cache->cache_block_idx[1], cache->cache_block[1], BLOCK_SIZE/SECTOR_SIZE);
            }
            cache->cache_block_idx[1]=cache->cache_block[0][indirect1];
            disk_read(part->my_disk, cache->cache_block_idx[1], cache->cache_block[1], BLOCK_SIZE/SECTOR_SIZE);
        }
    }
    if(direct==14)
    {
        if(!cache->cache_block[1][indirect2])
        {
            cache->cache_block_idx[2]=0;
            return;
        }
        ASSERT(indirect3>=0);
        if(cache->cache_block_idx[2]!=cache->cache_block[1][indirect2])
        {
            if(cache->cache_block_idx[2])
            {
                disk_write(part->my_disk, cache->cache_block_idx[2], cache->cache_block[2], BLOCK_SIZE/SECTOR_SIZE);
            }
            cache->cache_block_idx[2]=cache->cache_block[1][indirect2];
            disk_read(part->my_disk, cache->cache_block_idx[2], cache->cache_block[2], BLOCK_SIZE/SECTOR_SIZE);
        }
    }
    //delete block
    if(cache->cache_block_idx[2]&&direct==14&&
            cache->cache_block[2][indirect3])
    {
        delete_smfs_block_bitmap(part,&cache->cache_block[2][indirect3]);
    }
    if(cache->cache_block_idx[1]&&direct>=13)
    {
        if(is_block_empty(cache->cache_block[2]))
        {
            delete_smfs_block_bitmap(part,&cache->cache_block[1][indirect2]);
            cache->cache_block_idx[2]=0;
        }

    }
    if(cache->cache_block_idx[0]&&direct>=12)
    {
        if(is_block_empty(cache->cache_block[1]))
        {
            delete_smfs_block_bitmap(part,&cache->cache_block[0][indirect1]);
            cache->cache_block_idx[1]=0;
        }
    }

    if(is_block_empty(cache->cache_block[0]))
    {
        delete_smfs_block_bitmap(part,&pnode->i_blocks[direct]);
        cache->cache_block_idx[0]=0;
    }
}
void write_smfs_disk_cache(partition_desc *part, smfs_block_cache *cache)
{
    int i=0;
    for(i=0;i<3;i++)
    {
        if(cache->cache_block_idx[i])
        {
            disk_write(part->my_disk, cache->cache_block_idx[i],
                       cache->cache_block[i],
                       BLOCK_SIZE/SECTOR_SIZE);
        }
    }
}

uint32_t get_smfs_disk_block_start_section_by_index(partition_desc *part, smfs_disk_node *pnode,
                                                    uint32_t iblockindex, smfs_block_cache *cache)
{
    ASSERT(cache);
    ASSERT(pnode);
    ASSERT(part);
    uint64_t index= locate_disk_block(iblockindex);
    int16_t direct=(int16_t)index;
    int16_t indirect1=(index>>48);
    int16_t indirect2=(index>>32);
    int16_t indirect3=(index>>16);

    if(direct<0){
        PANIC1("locate_block:count %d out of range",iblockindex);
    }
    if(direct<12)
    {

        return pnode->i_blocks[direct];

    }
    if(direct>=12)
    {
        if(!pnode->i_blocks[direct])
        {
            return 0;
        }
        ASSERT(indirect1>=0);
        if(cache->cache_block_idx[0]!=pnode->i_blocks[direct])
        {
            cache->cache_block_idx[0]=pnode->i_blocks[direct];
            disk_read(part->my_disk, cache->cache_block_idx[0], cache->cache_block[0], BLOCK_SIZE/SECTOR_SIZE);
        }
        if(direct==12)
        {
            return cache->cache_block[0][indirect1];
        }
    }
    if(direct>=13)
    {
        if(!cache->cache_block[0][indirect1])
        {
            return 0;
        }
        ASSERT(indirect2>=0);
        if(cache->cache_block_idx[1]!=cache->cache_block[0][indirect1])
        {
            cache->cache_block_idx[1]=cache->cache_block[0][indirect1];
            disk_read(part->my_disk, cache->cache_block_idx[1], cache->cache_block[1], BLOCK_SIZE/SECTOR_SIZE);
        }
        if(direct==13)
        {
            return cache->cache_block[1][indirect2];
        }
    }
    if(direct==14)
    {
        if(!cache->cache_block[1][indirect2])
        {
            return 0;
        }
        ASSERT(indirect3>=0);
        if(cache->cache_block_idx[2]!=cache->cache_block[1][indirect2])
        {
            cache->cache_block_idx[2]=cache->cache_block[1][indirect2];
            disk_read(part->my_disk, cache->cache_block_idx[2], cache->cache_block[2], BLOCK_SIZE/SECTOR_SIZE);
        }
        if(direct==14)
        {
            return cache->cache_block[2][indirect3];
        }
    }
    ASSERT(0);
    return 0;
}

void read_smfs_disk_block(partition_desc* part,smfs_disk_node* pnode,uint32_t iblockindex,
                          void *buf, uint32_t count,smfs_block_cache* cache)
{
    char* temp=(char*)buf;
    if(!count) return;
    if(count+iblockindex>indirect3_block_count)
    {
        PANIC1("block count:%d+%d out of range",iblockindex,count);
    }
    uint32_t i;
    for(i=0;i<count;i++)
    {
        uint32_t block_start_section=create_get_smfs_disk_block_start_section_by_index(part,pnode,iblockindex+i,cache);
        if(block_start_section)
        {
            disk_read(part->my_disk,block_start_section,temp+i*BLOCK_SIZE,BLOCK_SIZE/SECTOR_SIZE);
        }
    }
}

void write_smfs_disk_block(partition_desc *part,smfs_disk_node *pnode, uint32_t iblockindex,
                           void *buf, uint32_t count,smfs_block_cache* cache)
{
    char* temp=(char*)buf;
    if(!count) return;
    if(count+iblockindex>indirect3_block_count)
    {
        PANIC1("block count:%d out of range",count);
    }
    uint32_t i;
    for(  i=0;i<count;i++)
    {

        uint32_t block_start_section=create_get_smfs_disk_block_start_section_by_index(part,pnode,iblockindex+i,cache);

        ASSERT(block_start_section);
        disk_write(part->my_disk,block_start_section,temp+i*BLOCK_SIZE,BLOCK_SIZE/SECTOR_SIZE);

    }
}
