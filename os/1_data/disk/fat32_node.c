#include "fat32_node.h"
#include "trace.h"
#include "memory.h"
#include "array_str.h"
#include "fat32_dir_entry.h"
fat32_mem_node *create_fat32_mem_node(fat32_mem_node *parent_dir, char *name,enum file_types type )
{
    partition_desc* part=parent_dir->part;
    fat32_mem_node *pinode= malloc_block(M_KERNEL,sizeof(fat32_mem_node));
    pinode->part=part;
    uint32_t first_clusterid=get_first_unused_cluster(part);
    if(!first_clusterid)return 0;
    if(parent_dir->entry.first_cluster_low==3)
    {

       // TRACE1("first_clusterid:%d",first_clusterid);while(1);
    }
    set_fat_cluster_next(parent_dir->part,0,first_clusterid,USED_CLUSTER_MARK);
    init_fat32_dir_entry(name, first_clusterid, type, &pinode->entry);

    write_fat32_dir_entry(parent_dir,&pinode->entry,&pinode->dir_entry_cluster_id,&pinode->dir_entry_offset);

    pinode->i_open_cnts = 1;
    list_push(&parent_dir->part->open_inodes, &pinode->self);
    return pinode;
}

fat32_mem_node *open_fat32_mem_node(partition_desc *part, uint32_t dir_entry_cluster_id, uint32_t dir_entry_offset)
{
    if(part->file_system!=M_FAT32)return 0;
    list_node* elem = part->open_inodes.head.next;
    fat32_mem_node* inode_found=0;
    while (elem != &part->open_inodes.tail)
    {
        inode_found = elem2entry(fat32_mem_node, self, elem);
        if (inode_found->dir_entry_cluster_id == dir_entry_cluster_id&&
                inode_found->dir_entry_offset == dir_entry_offset)
        {
            inode_found->i_open_cnts++;
            return inode_found;
        }
        elem = elem->next;
    }
    inode_found= malloc_block(M_KERNEL,sizeof(fat32_mem_node));
    inode_found->part=part;
    if(dir_entry_cluster_id)
    {
        uint32_t ireadsize= read_disk_cluster(part, dir_entry_cluster_id, dir_entry_offset,
                                              &inode_found->entry, ENTRY_SIZE_FAT32);
        ASSERT(ireadsize==ENTRY_SIZE_FAT32);
//       if(dir_entry_cluster_id==2)
//       {
//           TRACE1("inode_found->entry:%d",inode_found->entry.attribute);
//           while(1);
//       }
        inode_found->dir_entry_cluster_id=dir_entry_cluster_id;
        inode_found-> dir_entry_offset=dir_entry_offset;

    }
    else
    {
        inode_found->entry.attribute=FT_DIRECTORY;
        inode_found->entry.first_cluster_high=part->fat_param.root_dir_1st_cluster>>16;
        inode_found->entry.first_cluster_low=part->fat_param.root_dir_1st_cluster;
        inode_found->entry.file_size=0;
        inode_found->dir_entry_cluster_id=0;
        inode_found->dir_entry_offset=0;
        //     TRACE1("inode_found->entry.attribute:%d",inode_found->entry.attribute);

    }
    list_push(&part->open_inodes, &inode_found->self);
    inode_found->i_open_cnts = 1;
    return inode_found;
}

void write_fat32_disk_node(fat32_mem_node *inode)
{
    if(inode->dir_entry_cluster_id)
    {
        write_disk_cluster(inode->part, inode->dir_entry_cluster_id, inode->dir_entry_offset,
                           &inode->entry, ENTRY_SIZE_FAT32);
    }
}

void close_fat32_mem_node(fat32_mem_node *inode)
{
	if(!inode)return ;
    write_fat32_disk_node(inode);
    if (--inode->i_open_cnts == 0)
	{
        list_remove(&inode->self);

        free_block(inode);
    }
}

void delete_fat32_disk_node(fat32_mem_node* inode)
{
    if(!inode)return;
    uint32_t first_cluster_id=inode->entry.first_cluster_high<<16|inode->entry.first_cluster_low;
    if(first_cluster_id<inode->part->fat_param.root_dir_1st_cluster)
    { return;}
    if(inode->i_open_cnts>1)
    {
        PANIC1("inode:%s using:%d can not delete,",inode->entry.name,inode->i_open_cnts);
    }
    clear_and_remove_cluster_to_end(inode->part,first_cluster_id);
    if(first_cluster_id!=inode->part->fat_param.root_dir_1st_cluster)
    {
        set_fat_cluster_next(inode->part,0,first_cluster_id,0);
        inode->entry.file_size=0;
        inode->entry.name[0]=0xE5;
    }
    else
    {
        set_fat_cluster_next(inode->part,0,first_cluster_id,USED_CLUSTER_MARK);
    }
    close_fat32_mem_node(inode);
}

uint32_t write_fat32_to_disk(fat32_mem_node *pfile, uint32_t offset, void *buf, uint32_t isize)
{
    if(pfile->part->file_system!=M_FAT32)return 0;
    if(offset>pfile->entry.file_size) return 0;
    if(!isize)return 0;

    uint32_t clustersize=SECTOR_SIZE*pfile->part->fat_param.sectors_per_cluster;
    uint32_t isize_write=0;
    uint32_t clusterstart=offset/clustersize;
    uint32_t clusteroffset=offset%clustersize;
    uint32_t first_cluster=pfile->entry.first_cluster_high<<16|pfile->entry.first_cluster_low;
  //  TRACE1("first_cluster:%d",first_cluster);while(1);

    if(!first_cluster)
    {
        first_cluster=get_first_unused_cluster(pfile->part);
        if(!first_cluster){  return isize_write; }
        set_fat_cluster_next(pfile->part,0,first_cluster,USED_CLUSTER_MARK);
        pfile->entry.first_cluster_high=first_cluster>>16;
        pfile->entry.first_cluster_low=first_cluster;
    }
    int32_t icount= get_fat_cluster_count(pfile->part,0,first_cluster);
    if(!icount) {return 0;}
    int32_t i=(offset+isize)/clustersize+1-icount;
    uint32_t clusterid=first_cluster;
    while(i>0)
    {
        clusterid=insert_fat_cluster(pfile->part,clusterid);
        if(!clusterid) {   return 0; }

        i--;
    }
    clusterid=get_fat_cluster_by_index(pfile->part,0,first_cluster,clusterstart);
    if(!clusterid) {   return 0; }
    if(clusteroffset)
    {
        uint32_t isizecopy=min(clustersize-clusteroffset,isize);
        write_disk_cluster(pfile->part,clusterid,clusteroffset,buf,isizecopy);

        isize-=isizecopy;
        buf=(char*)buf+isizecopy;
        isize_write+=isizecopy;
        clusterid=get_fat_cluster_next(pfile->part,0,clusterid);
        if(!clusterid) {   return 0; }
    }
    while(isize>=clustersize)
    {
        write_disk_cluster(pfile->part,clusterid,0,buf,clustersize);
        isize-=clustersize;
        isize_write+=clustersize;
        buf=(char*)buf+clustersize;

        clusterid=get_fat_cluster_next(pfile->part,0,clusterid);
        if(!clusterid) {   return 0; }
    }
    clear_and_remove_cluster_to_end(pfile->part,clusterid);
    set_fat_cluster_next(pfile->part,0,clusterid,USED_CLUSTER_MARK);
    if(isize>0)
    {
        write_disk_cluster(pfile->part,clusterid,0,buf,isize);
        isize_write+=isize;
    }
    pfile->entry.file_size=isize_write+offset;
    write_fat32_disk_node(pfile);
    return isize_write;
}

uint32_t read_fat32_from_disk(fat32_mem_node *pfile, uint32_t offset, void *buf, uint32_t isize)
{
    if(pfile->part->file_system!=M_FAT32)return 0;
    if(offset>=pfile->entry.file_size) return 0;
    if(offset+isize>pfile->entry.file_size)
    {
        PANIC("offset+isize>pfile->entry.file_size");
        isize=pfile->entry.file_size-offset;
    }
    if(!isize)return 0;

  //  uint32_t filesize=isize;

    uint32_t clustersize=SECTOR_SIZE*pfile->part->fat_param.sectors_per_cluster;
    uint32_t isize_read=0;
    uint32_t clusterstart=offset/clustersize;
    uint32_t clusteroffset=offset%clustersize;
    uint32_t first_cluster=pfile->entry.first_cluster_high<<16|pfile->entry.first_cluster_low;
    uint32_t clusterid=get_fat_cluster_by_index(pfile->part,0,first_cluster,clusterstart);
    if(!clusterid||clusterid>=USED_CLUSTER_MARK) {   return isize_read; }
    uint32_t clusterreadnum=0;
    if(clusteroffset)
    {
        uint32_t isizecopy=min(clustersize-clusteroffset,isize);
        clusterreadnum= read_disk_cluster(pfile->part,clusterid,clusteroffset,buf,isizecopy);
        if(clusterreadnum!=isizecopy)
        {
            PANIC("XXX");
        }
        isize-=isizecopy;
        buf=(char*)buf+isizecopy;
        isize_read+=isizecopy;
        clusterid=get_fat_cluster_next(pfile->part,0,clusterid);
        if(!clusterid||clusterid>=USED_CLUSTER_MARK) {   return isize_read; }
    }
    while(isize>=clustersize)
    {
        clusterreadnum=read_disk_cluster(pfile->part,clusterid,0,buf,clustersize);
        if(clusterreadnum!=clustersize)
        {
            PANIC("XXX");
        }
        isize-=clustersize;
        isize_read+=clustersize;
        buf=(char*)buf+clustersize;

        clusterid=get_fat_cluster_next(pfile->part,0,clusterid);
        if(!clusterid||clusterid>=USED_CLUSTER_MARK) {   return isize_read; }
    }
    if(isize>0)
    {
        clusterreadnum=read_disk_cluster(pfile->part,clusterid,0,buf,isize);
      // if(pfile->entry.file_size>20000&&filesize>7)
      // {
      //     uint32_t i=0;
      //     TRACE2(24,"isize:%d,%d,%d,%d,%d",isize,clustersize,
      //     filesize,pfile->entry.file_size,clusterid);
      //     TRACE_SECTION(buf);
      //     i++;while(1);
      // }
        if(clusterreadnum!=isize)
        {
            PANIC("XXX");
        }
        isize_read+=isize;
    }
    return isize_read;
}
