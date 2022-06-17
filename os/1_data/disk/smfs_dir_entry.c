#include "smfs_dir_entry.h"
#include "trace.h"
#include "array_str.h"

void make_smfs_dir_entry_name(const char *filenamesrc, char *filenamedest)
{
    strcpy(filenamedest,filenamesrc);
    if(filenamedest[0]==(char)0xE5)filenamedest[0]=0x05;
}

void get_name_from_smfs_dir_entry(smfs_dir_entry *p_de, char *name)
{
    memcpy(name,p_de->filename,MAX_FILE_NAME_LEN_SMFS);
    if(name[0]==(char)0x05)name[0]=0xE5;
}

// 在part分区内的目录内寻找name或者i_no字段相同的dir_entry,
// 找到后返回block index
bool search_smfs_dir_entry(smfs_mem_node* pdir, smfs_dir_entry* p_de,
                           uint32_t* pblockstartsection, uint32_t* pblockoffset)
{
    smfs_block_cache* cache=&pdir->cache;
    partition_desc* part=pdir->part;
    *pblockstartsection=*pblockoffset=0;
    uint8_t* buf  = cache->buf;
    uint32_t dir_entry_size = part->s_block.dir_entry_size;
    uint32_t each_dir_entry_cnt = BLOCK_SIZE / dir_entry_size;  // 1块内可容纳的目录项个数
    uint32_t total_dir_entry_cnt = pdir->disk_node.i_size/ dir_entry_size;
    uint32_t i=0;
    uint32_t i_dir_entry_count=0;
    int isizename=strlen(p_de->filename);
    for(i=0;i<indirect3_block_count;i++)
    {
        if(i_dir_entry_count>=total_dir_entry_cnt)
        {
            return false;
        }
        uint32_t block_startsection=get_smfs_disk_block_start_section_by_index(part,&pdir->disk_node,i,cache);

        if(block_startsection)
        {
            disk_read(part->my_disk,block_startsection,buf,BLOCK_SIZE/SECTOR_SIZE);
            smfs_dir_entry* p_de1=(smfs_dir_entry*)buf;
            uint32_t j=0;
            for(j=0;j<each_dir_entry_cnt;j++)
            {
                char ch[MAX_FILE_NAME_LEN_SMFS]={0};
                get_name_from_smfs_dir_entry(p_de1+j,ch);
                if((p_de1+j)->filename[0]==(char)0)
                {
               //   if(!strcmp(p_de->filename,"v.txt"))
               //   {
               //       TRACE1("inodeID:%d,ch:%s,j:%d,block_idx:%d",pdir->disk_node.inodeID,ch,j,block_idx);
               //       while(1);
               //       ASSERT(0);
               //   }
                    return false;
                }
                else if((!strcmp1(p_de->filename,ch)&&isizename)||
                        (p_de->i_no==(p_de1+j)->i_no&&p_de->i_no) )
                {
                    memcpy(p_de,p_de1+j,dir_entry_size);
                    *pblockstartsection=block_startsection;
                    *pblockoffset=j*dir_entry_size;
                    return true;
                }
                if((p_de1+j)->filename[0]!=(char)0xE5)
                {
                    i_dir_entry_count++;
                }
            }
        }
        else
        {
            return false;
        }

    }
    memset(p_de,0,dir_entry_size);
    return false;
}



/* 在内存中初始化目录项p_de */
void init_smfs_dir_entry(char* filename, uint32_t inode_no, smfs_dir_entry* p_de)
{
    int isize=strlen(filename);
    ASSERT(isize <  MAX_FILE_NAME_LEN_SMFS);

    memset(p_de,0,sizeof(smfs_dir_entry));
    make_smfs_dir_entry_name(filename,p_de->filename);
    p_de->i_no = inode_no;
}

// 将文件目录项写入父目录parent_dir中
bool write_smfs_dir_entry( smfs_mem_node* parent_dir, smfs_dir_entry* p_de)
{
    smfs_block_cache* cache=&parent_dir->cache;
    partition_desc* part=parent_dir->part;
    smfs_disk_node* dir_inode = &parent_dir->disk_node;
    uint32_t dir_entry_size = part->s_block.dir_entry_size;
    uint32_t dir_entry_total_count = dir_inode->i_size/dir_entry_size;
    uint32_t each_dir_entry_cnt = BLOCK_SIZE / dir_entry_size;  // 1块内可容纳的目录项个数
    if(dir_entry_total_count/each_dir_entry_cnt>=indirect3_block_count)
    {
        TRACE("directory is full!");
        return false;
    }
    uint8_t* io_buf =cache->buf;
    uint32_t i;
    for(i=0;i<indirect3_block_count;i++)
    {
        uint32_t block_start_section= create_get_smfs_disk_block_start_section_by_index(part,
                                                                              &parent_dir->disk_node,
                                                                              i,
                                                                              cache);
        if(block_start_section)
        {
            disk_read(part->my_disk,block_start_section,io_buf,BLOCK_SIZE/SECTOR_SIZE);
            uint32_t j;
            smfs_dir_entry* p_de1=(smfs_dir_entry*)io_buf;

            for(j=0;j<each_dir_entry_cnt;j++)
            {
                if((p_de1+j)->filename[0]==(char)0||(p_de1+j)->filename[0]==(char)0xE5)
                {
                    memcpy(p_de1+j,p_de,dir_entry_size);
                    disk_write(part->my_disk,block_start_section,io_buf,BLOCK_SIZE/SECTOR_SIZE);
                    write_smfs_disk_cache(part,cache);
                    parent_dir->disk_node.i_size+=dir_entry_size;
                    write_smfs_disk_node(parent_dir);
                    return true;
                }
            }
        }
    }
    return false;
}


void delete_smfs_dir_entry( smfs_mem_node* parent_dir,  smfs_mem_node* dir)
{
    smfs_block_cache* cache=&parent_dir->cache;
    partition_desc* part=dir->part;
    uint32_t dir_entry_size = part->s_block.dir_entry_size;
    smfs_dir_entry entry;
    uint32_t block_start_section,blockoffset;
    init_smfs_dir_entry("",dir->disk_node.inodeID,&entry);
    bool bsearch=search_smfs_dir_entry(parent_dir,&entry,&block_start_section,&blockoffset);
    if(bsearch)
    {
        uint8_t* buf  =cache->buf;
        if(block_start_section)
        {
            disk_read(part->my_disk,block_start_section,buf,BLOCK_SIZE/SECTOR_SIZE);
            memset(buf+blockoffset,0xE5,1);
            disk_write(part->my_disk,block_start_section,buf,BLOCK_SIZE/SECTOR_SIZE);
            parent_dir->disk_node.i_size-=dir_entry_size;
            write_smfs_disk_node(parent_dir);
            write_smfs_disk_cache(part,cache);
        }
    }
}

void rename_smfs_dir_entry(smfs_mem_node *pdir, uint32_t inode_no, char *pnewname)
{
    smfs_block_cache* cache=&pdir->cache;
    partition_desc* part=pdir->part;
    smfs_disk_node* dir_inode = &pdir->disk_node;
    if(!dir_inode->i_size)
    {
        TRACE("empty dir");
        return;
    }
    smfs_dir_entry entry;
    uint32_t blockstartsection,blockoffset;
    init_smfs_dir_entry(pnewname,-1,&entry);
    bool flag=search_smfs_dir_entry(pdir,&entry,&blockstartsection,&blockoffset);
    if(flag)
    {
        TRACE("name exist");
        return;
    }

    init_smfs_dir_entry("",inode_no,&entry);
    flag=search_smfs_dir_entry(pdir,&entry,&blockstartsection,&blockoffset);
    if(flag)
    {
        uint8_t* buf  =cache->buf;
        disk_read(part->my_disk,blockstartsection,buf,BLOCK_SIZE/SECTOR_SIZE);
        char name[MAX_FILE_NAME_LEN_SMFS]={0};
        make_smfs_dir_entry_name(pnewname,name);
        memcpy(buf+blockoffset,name,MAX_FILE_NAME_LEN_SMFS);
        disk_write(part->my_disk,blockstartsection,buf,BLOCK_SIZE/SECTOR_SIZE);
    }
}

bool get_smfs_dir_entry_by_index( smfs_mem_node *pdir, smfs_dir_entry *p_de, uint32_t index)
{
    smfs_block_cache* cache=&pdir->cache;
    partition_desc* part=pdir->part;
    uint8_t* buf  = cache->buf;
    uint32_t dir_entry_size = part->s_block.dir_entry_size;
    uint32_t each_dir_entry_cnt = BLOCK_SIZE / dir_entry_size;  // 1块内可容纳的目录项个数
    uint32_t total_dir_entry_cnt = pdir->disk_node.i_size/ dir_entry_size;
    uint32_t i=0;
    uint32_t i_dir_entry_count=0;
    for(i=0;i<indirect3_block_count;i++)
    {
        if(i_dir_entry_count>=total_dir_entry_cnt)
        {
            return false;
        }
        uint32_t block_idx=get_smfs_disk_block_start_section_by_index(part,&pdir->disk_node,i,cache);

        if(block_idx)
        {
            disk_read(part->my_disk,block_idx,buf,BLOCK_SIZE/SECTOR_SIZE);
            smfs_dir_entry* p_de1=(smfs_dir_entry*)buf;
            uint32_t j=0;
            for(j=0;j<each_dir_entry_cnt;j++)
            {
                char ch[MAX_FILE_NAME_LEN_SMFS]={0};
                get_name_from_smfs_dir_entry(p_de1+j,ch);
                if((p_de1+j)->filename[0]==(char)0)
                {
                    return false;
                }
                else if((p_de1+j)->filename[0]!=(char)0xE5)
                {
                    if(i_dir_entry_count==index)
                    {
                        memcpy(p_de,p_de1+j,dir_entry_size);
                        return true;
                    }
                }
                i_dir_entry_count++;
            }
        }
        else
        {
            return false;
        }

    }
    memset(p_de,0,dir_entry_size);
    return false;
}
