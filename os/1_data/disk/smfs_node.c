#include "smfs_node.h"
#include "memory.h"
#include "trace.h"
#include "array_str.h"
#include "smfs_dir_entry.h"

smfs_mem_node *create_smfs_mem_node(   smfs_mem_node *parent_dir,
                                       char *name,
                                       enum file_types type)
{
    partition_desc* part=parent_dir->part;
    if(part->file_system!=M_SMFS)return 0;
    smfs_mem_node* f= malloc_block(M_KERNEL,sizeof(smfs_mem_node));
    f->disk_node.inodeID=create_smfs_block_bitmap(part);
    f->disk_node.type=type;
    f->part=part;
    smfs_dir_entry new_dir_entry;
    init_smfs_dir_entry(name,
                        f->disk_node.inodeID,
                        &new_dir_entry);
    write_smfs_dir_entry(parent_dir, &new_dir_entry);
    write_smfs_disk_node(f);
    f->i_open_cnts = 1;
    list_push(&part->open_inodes, &f->self);
    return f;
}

smfs_mem_node *open_smfs_mem_node(partition_desc *part, uint32_t inode_no)
{
    if(part->file_system!=M_SMFS)return 0;
    list_node* elem = part->open_inodes.head.next;
    smfs_mem_node* inode_found=0;
    while (elem != &part->open_inodes.tail)
    {
        inode_found = elem2entry(smfs_mem_node, self, elem);
        if (inode_no&&inode_no==inode_found->disk_node.inodeID)
        {
            inode_found->i_open_cnts++;
            return inode_found;
        }
        elem = elem->next;
    }
    inode_found= malloc_block(M_KERNEL,sizeof(smfs_mem_node));
    inode_found->part=part;
    inode_found->disk_node.inodeID=inode_no;
    bool b= read_smfs_disk_node(inode_found);
    if(!b) {free_block(inode_found);return 0;}

    list_push(&part->open_inodes, &inode_found->self);
    inode_found->i_open_cnts = 1;
    return inode_found;
}

void close_smfs_mem_node(smfs_mem_node *inode)
{
    if(!inode)return ;
    write_smfs_disk_cache(inode->part,&inode->cache);
    write_smfs_disk_node(inode);
    if (--inode->i_open_cnts == 0)
    {
        list_remove(&inode->self);

        free_block(inode);
    }
}

bool write_smfs_disk_node(smfs_mem_node *inode)
{
    char ch[BLOCK_SIZE]={0};
    memcpy(ch,&inode->disk_node,sizeof(smfs_disk_node));
    int icount=sizeof(smfs_disk_node)/SECTOR_SIZE+1;
    ASSERT(icount<=BLOCK_SIZE/SECTOR_SIZE);
    disk_write(inode->part->my_disk,inode->disk_node.inodeID,ch,icount);
    return true;
}

bool read_smfs_disk_node(smfs_mem_node *inode)
{
    char ch[BLOCK_SIZE];
    int icount=sizeof(smfs_disk_node)/SECTOR_SIZE+1;
    ASSERT(icount<=BLOCK_SIZE/SECTOR_SIZE);
    disk_read(inode->part->my_disk,inode->disk_node.inodeID,ch,icount);
    memcpy(&inode->disk_node,ch,sizeof(smfs_disk_node));
    return true;
}

void delete_smfs_disk_node(smfs_mem_node *parent_dir,smfs_mem_node *inode)
{
    if(!inode)return;
    smfs_block_cache* cache=&inode->cache;
 //   if(inode->disk_node.inodeID==inode->part->sb.root_inode_no)
 //   {TRACE("root can't delete"); return;}
    partition_desc* part=inode->part;
    if(inode->i_open_cnts>1)
    {
        PANIC1("inode:%d using:%d can not delete,",inode->disk_node.inodeID,inode->i_open_cnts);
    }
    uint32_t i;
    for(i=0;i<indirect3_block_count;i++)
    {
        uint32_t block_start_section=
                get_smfs_disk_block_start_section_by_index(inode->part,&inode->disk_node,i,cache);
        if(block_start_section)
        {
            delete_smfs_disk_block(inode->part,&inode->disk_node,i,cache);

        }
        else
        {
            break;
        }
    }
    uint32_t inodeID=inode->disk_node.inodeID;
    if(inode->disk_node.inodeID!=inode->part->s_block.root_inode_lba)
    {
        delete_smfs_dir_entry(parent_dir, inode);
        delete_smfs_block_bitmap(part,&inode->disk_node.inodeID);
    //    TRACE1("inode->disk_node.inodeID:%d",inode->disk_node.inodeID);
    //    while(1);
    }
    else
    {
        clear_smfs_block_data(part,inodeID);
    }
    write_smfs_disk_cache(part,cache);
    list_remove(&inode->self);
    free_block(inode);
}

uint32_t write_smfs_to_disk(smfs_mem_node *pfile, uint32_t offset, void *buf, uint32_t isize)
{
    partition_desc* part=pfile->part;
    if(part->file_system!=M_SMFS)return 0;
    if(offset>pfile->disk_node.i_size) return 0;
 //   if(!isize)return 0;
    smfs_block_cache* cache=&pfile->cache;
    ASSERT(cache);

    uint8_t* io_buf = cache->buf;
    ASSERT(io_buf);
    uint32_t isizefile=pfile->disk_node.i_size;
    uint32_t isizewrite=0;
    uint32_t block_index_start=offset/BLOCK_SIZE;
    if(offset%BLOCK_SIZE)
    {
        read_smfs_disk_block(part,&pfile->disk_node,block_index_start,io_buf,1,cache);
        isizewrite=min(isize,BLOCK_SIZE-offset%BLOCK_SIZE);
        memcpy(io_buf+isizefile%BLOCK_SIZE,
               buf,
               isizewrite);
        write_smfs_disk_block(part,&pfile->disk_node,block_index_start,io_buf,1,cache);
        buf+=isizewrite;
        block_index_start++;
        isize-=isizewrite;
    }
    while(isize>=BLOCK_SIZE&&block_index_start<indirect3_block_count)
    {
        write_smfs_disk_block(part,&pfile->disk_node,block_index_start,buf,1,cache);
        block_index_start++;
        isize-=BLOCK_SIZE;
        buf+=BLOCK_SIZE;
        isizewrite+=BLOCK_SIZE;
    }

    if(isize>0)
    {
        read_smfs_disk_block(part,&pfile->disk_node,block_index_start,io_buf,1,cache);
        memset(io_buf,0,BLOCK_SIZE);
        memcpy(io_buf,buf,isize);
        write_smfs_disk_block(part,&pfile->disk_node,block_index_start,io_buf,1,cache);
        isizewrite+=isize;
        block_index_start++;
    }
    uint32_t i;
    for(i=block_index_start;i<indirect3_block_count;i++)
    {
        uint32_t block_start_section=get_smfs_disk_block_start_section_by_index(pfile->part,
                                                                                &pfile->disk_node,
                                                                                i,
                                                                                cache);
        if(block_start_section)
        {
            delete_smfs_disk_block(pfile->part,&pfile->disk_node,i,cache);

        }
        else
        {
            break;
        }
    }
    pfile->disk_node.i_size=isizewrite+offset;
    write_smfs_disk_cache(part,cache);
    write_smfs_disk_node(pfile);
    return isizewrite;
}

uint32_t read_smfs_from_disk(smfs_mem_node *pfile, uint32_t offset, void *buf,
                             uint32_t isize)
{
    partition_desc* part=pfile->part;
    uint32_t isizefile=pfile->disk_node.i_size;
    if(part->file_system!=M_SMFS)return 0;
    if(offset+isize>isizefile) return 0;
    if(!isize)return 0;
    smfs_block_cache* cache=&pfile->cache;
    ASSERT(cache);

    uint8_t* io_buf =  cache->buf;
    ASSERT(io_buf);
    uint32_t isizeread=0;
    uint32_t block_index_start=offset/BLOCK_SIZE;
    if(offset%BLOCK_SIZE)
    {
        read_smfs_disk_block(part,&pfile->disk_node,block_index_start,io_buf,1,cache);
        isizeread=min(isize,BLOCK_SIZE-offset%BLOCK_SIZE);
        memcpy(buf,io_buf+offset%BLOCK_SIZE,
               isizeread);
        buf=(char*)buf+isizeread;
        block_index_start++;
        isize-=isizeread;
    }
    while(isize>=BLOCK_SIZE&&block_index_start<indirect3_block_count)
    {
        read_smfs_disk_block(part,&pfile->disk_node,block_index_start,buf,1,cache);
        block_index_start++;
        isize-=BLOCK_SIZE;
        buf=(char*)buf+BLOCK_SIZE;
        isizeread+=BLOCK_SIZE;
    }

    if(isize>0)
    {
        read_smfs_disk_block(part,&pfile->disk_node,block_index_start,io_buf,1,cache);
        memcpy(buf,io_buf,isize);
        isizeread+=isize;
    }
//  free_block(M_KERNEL,cache);
    return isizeread;
}
