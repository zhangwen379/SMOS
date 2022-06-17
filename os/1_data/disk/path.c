#include "path.h"
#include "list.h"
#include "array_str.h"
#include "trace.h"
#include "process.h"
#include "kernel.h"
#include "smfs_path.h"
#include "fat32_path.h"

static int32_t is_path_used(const char* pathname)
{
    list *plistpcb=&m_gKCB.process_list;
    list_node* pelem = plistpcb->head.next;
    while (pelem!= &plistpcb->tail)
    {
        PPCB pPCB= elem2entry(PCB,self,pelem);
        if(is_path_used_by_process(pPCB,pathname))
        {
            return pPCB->self.iID;
        }
        pelem=pelem->next;
    }
    return -1;
}


char* path_parse(const char* pathname, char* name_store)
{
    char* p = (char*)pathname;
    while (*p != '/'&&*p != '\\'&& *p != 0) {
        *name_store++ = *p++;
    }
    if (p[0] == 0) {   // 若路径字符串为空则返回NULL
        return NULL;
    }
    p++;
    return p;
}

int32_t get_path_file_count(const char* pathname)
{
    ASSERT(pathname != NULL);
    char* p = (char*)pathname;
    char name[MAX_FILE_NAME_LEN_SMFS]={0};
    uint32_t depth = 0;
    p = path_parse(p, name);
    while (name[0]) {
        depth++;
        memset(name, 0, MAX_FILE_NAME_LEN_SMFS);
        if (p) {
            p  = path_parse(p, name);
        }
    }
    return depth;
}
static partition_desc* get_part_from_pathname(const char *pathname)
{
    char ch[8];memset(ch,0,8);
    path_parse(pathname, ch);
    return get_partition_by_name(ch);
}

file *create_path_file(const char *pathname)
{
    partition_desc* part= get_part_from_pathname(pathname);
    if(!part) return 0;
    file *pfile=malloc_block(M_KERNEL,sizeof(file));
    pfile->file_system=part->file_system;
    if(part->file_system==M_FAT32)
    {
        pfile->pFile=create_path_fat32(pathname,FT_REGULAR);
    }
    else if(part->file_system==M_SMFS)
    {
        pfile->pFile=create_path_smfs(pathname,FT_REGULAR);
    }
	if(!pfile->pFile)
	{
        free_block(pfile);
		pfile=0;
	}
    return pfile;
}

file *open_path_file(const char* pathname)
{
    partition_desc* part= get_part_from_pathname(pathname);
    if(!part) return 0;
    file *pfile=malloc_block(M_KERNEL,sizeof(file));
    pfile->file_system=part->file_system;
    if(part->file_system==M_FAT32)
    {
        pfile->pFile=open_path_fat32(pathname,FT_REGULAR);
    }
    else if(part->file_system==M_SMFS)
    {
        pfile->pFile=open_path_smfs(pathname,FT_REGULAR);
    }
    if(!pfile->pFile)
    {
        free_block(pfile);
        pfile=0;
    }
    return pfile;
}

void del_path_file(const char* pathname)
{
    int32_t ipid=is_path_used(pathname);
    if(ipid!=-1)
    {
        TRACE1("path:%s is used by process:%d",pathname,ipid);
        return ;
    }
    partition_desc* part= get_part_from_pathname(pathname);
    if(!part) return;
    if(part->file_system==M_FAT32)
    {
        del_path_file_fat32(pathname);
    }
    else if(part->file_system==M_SMFS)
    {
        del_path_file_smfs(pathname);
    }
}

dir * create_path_dir(const char* pathname)
{
    partition_desc* part= get_part_from_pathname(pathname);
    if(!part) return 0;
    dir *pdir=malloc_block(M_KERNEL,sizeof(dir));
    pdir->file_system=part->file_system;
    if(part->file_system==M_FAT32)
    {
        pdir->pDir=create_path_fat32(pathname,FT_DIRECTORY);
    }
    else if(part->file_system==M_SMFS)
    {
        pdir->pDir=create_path_smfs(pathname,FT_DIRECTORY);
    }
	if(!pdir->pDir)
	{
		free_block(pdir);
		pdir=0;
	}
    return pdir;
}

dir* open_path_dir(const char* pathname)
{
    partition_desc* part= get_part_from_pathname(pathname);
    if(!part) return 0;
    dir *pdir=malloc_block(M_KERNEL,sizeof(dir));
    pdir->file_system=part->file_system;
    if(part->file_system==M_FAT32)
    {
        pdir->pDir=open_path_fat32(pathname,FT_DIRECTORY);
    }
    else if(part->file_system==M_SMFS)
    {
        pdir->pDir=open_path_fat32(pathname,FT_DIRECTORY);
    }
	if(!pdir->pDir)
	{
		free_block(pdir);
		pdir=0;
	}
    return pdir;
}

void del_path(const char *pathname)
{
    int32_t ipid=is_path_used(pathname);
    if(ipid!=-1)
    {
        TRACE1("path:%s is used by process:%d",pathname,ipid);
        return ;
    }
    partition_desc* part= get_part_from_pathname(pathname);
    if(!part) return;
    if(part->file_system==M_FAT32)
    {
        del_path_fat32(pathname);
    }
    else if(part->file_system==M_SMFS)
    {
        del_path_smfs(pathname);
    }

}

void copy_file(const char *pathname_src, const char *pathname_dst)
{
    partition_desc* part_src= get_part_from_pathname(pathname_src);
    partition_desc* part_dst= get_part_from_pathname(pathname_dst);
    if(!part_src||!part_dst) return;
    if(part_src->file_system==M_FAT32&&part_dst->file_system==M_FAT32)
    {
        copy_file_fat32(pathname_src, pathname_dst);
    }
    else if(part_src->file_system==M_SMFS&&part_dst->file_system==M_SMFS)
    {
        copy_file_smfs(pathname_src, pathname_dst);
    }
    else if(part_src->file_system==M_SMFS&&part_dst->file_system==M_FAT32)
    {
        path_search_record_smfs searched_record_src;
        path_search_record_fat32 searched_record_dst;
        bool src_exist=is_path_exist_smfs(pathname_src,&searched_record_src,FT_REGULAR);
        bool  dst_exist=is_path_exist_fat32(pathname_dst,&searched_record_dst,FT_REGULAR);

        if(!src_exist||!dst_exist)
        {
            close_smfs_mem_node(searched_record_src.parent_dir);
            close_smfs_mem_node(searched_record_src.dir);
            close_fat32_mem_node(searched_record_dst.parent_dir);
            close_fat32_mem_node(searched_record_dst.dir);
            TRACE("path has not exist!.");
            return;
        }
        smfs_mem_node* p_file_node_src= searched_record_src.dir;
        fat32_mem_node* p_file_node_dst= searched_record_dst.dir;

        ASSERT(p_file_node_src&&p_file_node_dst);
        int isize=p_file_node_src->disk_node.i_size;
        char* buf= malloc_block(M_KERNEL,isize);
        read_smfs_from_disk(p_file_node_src,0,buf,isize);
        write_fat32_to_disk(p_file_node_dst,0,buf,isize);
        close_smfs_mem_node(p_file_node_src);
        close_fat32_mem_node(p_file_node_dst);
        close_smfs_mem_node(searched_record_src.parent_dir);
        close_fat32_mem_node(searched_record_dst.parent_dir);
        free_block(buf);
    }
    else if(part_src->file_system==M_FAT32&&part_dst->file_system==M_SMFS)
    {
        path_search_record_fat32 searched_record_src;
        path_search_record_smfs searched_record_dst;
        bool src_exist=is_path_exist_fat32(pathname_src,&searched_record_src,FT_REGULAR);
        bool  dst_exist=is_path_exist_smfs(pathname_dst,&searched_record_dst,FT_REGULAR);

        if(!src_exist||!dst_exist)
        {
            close_fat32_mem_node(searched_record_src.parent_dir);
            close_fat32_mem_node(searched_record_src.dir);
            close_smfs_mem_node(searched_record_dst.parent_dir);
            close_smfs_mem_node(searched_record_dst.dir);
            TRACE("path has not exist!.");
            return;
        }
        fat32_mem_node*p_file_node_src= searched_record_src.dir;
        smfs_mem_node*  p_file_node_dst=searched_record_dst.dir;

        ASSERT(p_file_node_src&&p_file_node_dst);
        int isize=p_file_node_src->entry.file_size;
        char* buf= malloc_block(M_KERNEL,isize);
        read_fat32_from_disk(p_file_node_src,0,buf,isize);
        write_smfs_to_disk(p_file_node_dst,0,buf,isize);
        close_fat32_mem_node(p_file_node_src);
        close_smfs_mem_node(p_file_node_dst);
        close_fat32_mem_node(searched_record_src.parent_dir);
        close_smfs_mem_node(searched_record_dst.parent_dir);
        free_block(buf);
    }
}

void copy_path(const char *pathname_src, const char *pathname_dst)
{
    partition_desc* part_src= get_part_from_pathname(pathname_src);
    partition_desc* part_dst= get_part_from_pathname(pathname_dst);
    if(!part_src||!part_dst) return;
    if(part_src->file_system==M_FAT32&&part_dst->file_system==M_FAT32)
    {
        copy_path_fat32(pathname_src, pathname_dst);
    }
    else if(part_src->file_system==M_SMFS&&part_dst->file_system==M_SMFS)
    {
        copy_path_smfs(pathname_src, pathname_dst);
    }
    else if(part_src->file_system==M_FAT32&&part_dst->file_system==M_SMFS)
    {
        path_search_record_fat32 searched_record_src;
        path_search_record_smfs   searched_record_dst,searched_record_dst_tset;
        bool src_exist=is_path_exist_fat32(pathname_src,&searched_record_src,FT_UNKNOWN);
        bool  dst_dir_exist=is_path_exist_smfs(pathname_dst,&searched_record_dst,FT_DIRECTORY);

        if(!src_exist||!dst_dir_exist)
        {
            close_fat32_mem_node(searched_record_src.parent_dir);
            close_fat32_mem_node(searched_record_src.dir);
            close_smfs_mem_node(searched_record_dst.parent_dir);
            close_smfs_mem_node(searched_record_dst.dir);
            TRACE("path dest has not exist!.");
            return;
        }
        char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
        strcpy(temp_path_dst,pathname_dst);
        strcat(temp_path_dst,"/");
        char* dirname = strrchr(pathname_src, '/') + 1;
        strcat(temp_path_dst,dirname);
		bool dst_exist=is_path_exist_smfs(temp_path_dst,&searched_record_dst_tset,
										  searched_record_src.dir->entry.attribute);
        close_smfs_mem_node(searched_record_dst_tset.parent_dir);
        close_smfs_mem_node(searched_record_dst_tset.dir);
        free_block(temp_path_dst);
        if(dst_exist)
        {
            close_fat32_mem_node(searched_record_src.parent_dir);
            close_fat32_mem_node(searched_record_src.dir);
            close_smfs_mem_node(searched_record_dst.parent_dir);
            close_smfs_mem_node(searched_record_dst.dir);
            TRACE("dest name has exist!.");
            return;
        }

        if(searched_record_src.dir->entry.attribute==FT_REGULAR)
        {
            char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
            strcpy(temp_path_dst,pathname_dst);
            strcat(temp_path_dst,"/");
            char* dirname = strrchr(pathname_src, '/') + 1;
            strcat(temp_path_dst,dirname);
            smfs_mem_node* p_file_node_dst= create_path_smfs(temp_path_dst,FT_REGULAR);
            close_smfs_mem_node(p_file_node_dst);
            copy_file(pathname_src, temp_path_dst);
            free_block(temp_path_dst);

        }
        else if(searched_record_src.dir->entry.attribute==FT_DIRECTORY)
        {
            char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
            strcpy(temp_path_dst,pathname_dst);
            strcat(temp_path_dst,"/");
            char* dirname = strrchr(pathname_src, '/') + 1;
            strcat(temp_path_dst,dirname);
            smfs_mem_node* p_dir_inode_dst= create_path_smfs(temp_path_dst,FT_DIRECTORY);

            fat32_mem_node* pdir=searched_record_src.dir;
            uint32_t dir_entry_size = ENTRY_SIZE_FAT32;
            uint32_t each_dir_entry_cnt = SECTOR_SIZE / dir_entry_size;
             fat32_dir_entry* buf  =(fat32_dir_entry*)malloc_block(M_KERNEL,SECTOR_SIZE);
            uint32_t clusterid= pdir->entry.first_cluster_high<<16|pdir->entry.first_cluster_low;
            bool bemptyentry=false;
            partition_desc* part=searched_record_src.parent_dir->part;
            while(clusterid>0&&clusterid<USED_CLUSTER_MARK&&!bemptyentry)
            {
                uint32_t i=0;
                for(i=0;i<part->fat_param.sectors_per_cluster&&!bemptyentry;i++)
                {
                    read_disk_cluster(pdir->part, clusterid,i*SECTOR_SIZE, buf, SECTOR_SIZE);
                    uint32_t j=0;
                    for(j=0;j<each_dir_entry_cnt&&!bemptyentry;j++)
                    {
                        fat32_dir_entry* p_de1=buf+j;

                        char chfilename1[MAX_FILE_NAME_LEN_FAT+1];
                        memcpy(chfilename1,p_de1->name,MAX_FILE_NAME_LEN_FAT);
                        if(!chfilename1[0]){bemptyentry=true;}
                        if(chfilename1[0]!=(char)0xE5&&
                                (p_de1->attribute==FT_DIRECTORY||p_de1->attribute==FT_REGULAR))
                        {
                            char name[MAX_FILE_NAME_LEN_FAT+2]={0};
                            get_name_from_fat32_dir_entry(p_de1,name);
                            char* temp_path= malloc_block(M_KERNEL,MAX_PATH_LEN);
                            TRACE1("name:%s",name);
                            strcpy(temp_path,pathname_src);
                            strcat(temp_path,"/");
                            strcat(temp_path,name);
                            copy_path(temp_path,temp_path_dst);
                            free_block(temp_path);
                        }
                    }
                }
                clusterid=get_fat_cluster_next(part,0,clusterid);

            }
            close_smfs_mem_node(p_dir_inode_dst);
            free_block(temp_path_dst);
            free_block(buf);
        }
        TRACE("copy path success.");
        close_fat32_mem_node(searched_record_src.parent_dir);
        close_fat32_mem_node(searched_record_src.dir);
        close_smfs_mem_node(searched_record_dst.parent_dir);
        close_smfs_mem_node(searched_record_dst.dir);

    }
    else if(part_src->file_system==M_SMFS&&part_dst->file_system==M_FAT32)
    {
        path_search_record_smfs searched_record_src;
        path_search_record_fat32 searched_record_dst,searched_record_dst_tset;
          bool inode_no_src=is_path_exist_smfs(pathname_src,&searched_record_src,FT_UNKNOWN);
          bool  dst_dir_exist=is_path_exist_fat32(pathname_dst,&searched_record_dst,FT_DIRECTORY);

          if(!inode_no_src||!dst_dir_exist)
          {
              close_smfs_mem_node(searched_record_src.parent_dir);
              close_smfs_mem_node(searched_record_src.dir);
              close_fat32_mem_node(searched_record_dst.parent_dir);
              close_fat32_mem_node(searched_record_dst.dir);
              TRACE("path dest has not exist!.");
              return;
          }
          char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
          strcpy(temp_path_dst,pathname_dst);
          strcat(temp_path_dst,"/");
          char* dirname = strrchr(pathname_src, '/') + 1;
          strcat(temp_path_dst,dirname);
          bool dst_exist=is_path_exist_fat32(temp_path_dst,&searched_record_dst_tset,FT_UNKNOWN);
          close_fat32_mem_node(searched_record_dst_tset.parent_dir);
          close_fat32_mem_node(searched_record_dst_tset.dir);
          free_block(temp_path_dst);
          if(dst_exist)
          {
              close_smfs_mem_node(searched_record_src.parent_dir);
              close_smfs_mem_node(searched_record_src.dir);
              close_fat32_mem_node(searched_record_dst.parent_dir);
              close_fat32_mem_node(searched_record_dst.dir);
              TRACE("dest name has exist!.");
              return;
          }

          if(searched_record_src.dir->disk_node.type==FT_REGULAR)
          {
              char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
              strcpy(temp_path_dst,pathname_dst);
              strcat(temp_path_dst,"/");
              char* dirname = strrchr(pathname_src, '/') + 1;
              strcat(temp_path_dst,dirname);
              fat32_mem_node* p_file_node_dst= create_path_fat32(temp_path_dst,FT_REGULAR);
              close_fat32_mem_node(p_file_node_dst);
              copy_file(pathname_src, temp_path_dst);
              free_block(temp_path_dst);

          }
          else if(searched_record_src.dir->disk_node.type==FT_DIRECTORY)
          {
              char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
              strcpy(temp_path_dst,pathname_dst);
              strcat(temp_path_dst,"/");
              char* dirname = strrchr(pathname_src, '/') + 1;
              strcat(temp_path_dst,dirname);
              fat32_mem_node* p_dir_inode_dst= create_path_fat32(temp_path_dst,FT_DIRECTORY);

              partition_desc* part=searched_record_src.dir->part;
              smfs_mem_node* pdir=searched_record_src.dir;
              smfs_dir_entry entry;

              int totalentry=pdir->disk_node.i_size/part->s_block.dir_entry_size;
              memset(&entry,0,sizeof(smfs_dir_entry));
              int icount=0;
              TRACE1("totalentry:%d",totalentry);
              for(icount=0;icount<=totalentry;icount++)
              {
                  if(get_smfs_dir_entry_by_index(pdir,&entry,icount))
                  {
                      char* temp_path_src= malloc_block(M_KERNEL,MAX_PATH_LEN);
                      strcpy(temp_path_src,pathname_src);
                      strcat(temp_path_src,"/");
                      strcat(temp_path_src,entry.filename);
                      copy_path(temp_path_src,temp_path_dst);
                      free_block(temp_path_src);
                  }
              }
              close_fat32_mem_node(p_dir_inode_dst);
              free_block(temp_path_dst);
          }
          TRACE("copy path success.");

          close_smfs_mem_node(searched_record_src.parent_dir);
          close_smfs_mem_node(searched_record_src.dir);
          close_fat32_mem_node(searched_record_dst.parent_dir);
          close_fat32_mem_node(searched_record_dst.dir);

    }
}

void rename_path(const char *pathname, char *pnewname)
{
    partition_desc* part= get_part_from_pathname(pathname);
    if(!part) return;
    if(part->file_system==M_FAT32)
    {
        rename_path_fat32(pathname,pnewname);
    }
    else if(part->file_system==M_SMFS)
    {
        rename_path_smfs(pathname,pnewname);
    }
}

char *make_dir_string(const char *pathname,mflags flag)
{
	partition_desc* part= get_part_from_pathname(pathname);
	if(!part) return 0;
	if(part->file_system==M_FAT32)
	{
		return  make_dir_string_fat32(pathname,flag,0);
	}
	else if(part->file_system==M_SMFS)
	{
		return  make_dir_string_smfs(pathname,flag,0);
	}
	return 0;
}

char *make_file_string(const char *pathname,mflags flag, const char *file_type)
{
	partition_desc* part= get_part_from_pathname(pathname);
	if(!part) return 0;
	if(part->file_system==M_FAT32)
	{
		return  make_dir_string_fat32(pathname,flag,file_type);
	}
	else if(part->file_system==M_SMFS)
	{
		return  make_dir_string_smfs(pathname,flag,file_type);
	}
	return 0;
}

bool get_path_status(const char *path,path_status *status)
{
	partition_desc* part= get_part_from_pathname(path);
	if(part->file_system==M_FAT32)
	{
		return get_path_status_fat32(path,status);
	}
	else if(part->file_system==M_SMFS)
	{
		return get_path_status_smfs(path,status);
	}
	return false;
}

void close_file(file *pfile)
{
    if(!pfile||!pfile->pFile) return;
    if(pfile->file_system==M_FAT32)
    {
        close_fat32_mem_node((fat32_mem_node *)pfile->pFile);
    }
    else if(pfile->file_system==M_SMFS)
    {
        close_smfs_mem_node((smfs_mem_node *)pfile->pFile);
    }
    free_block(pfile);
}

void close_dir(dir *pdir)
{
    if(!pdir||!pdir->pDir) return;
    if(pdir->file_system==M_FAT32)
    {
        close_fat32_mem_node((fat32_mem_node *)pdir->pDir);
    }
    else if(pdir->file_system==M_SMFS)
    {
        close_smfs_mem_node((smfs_mem_node *)pdir->pDir);
    }
    free_block(pdir);
}

uint32_t write_file(file *pfile, uint32_t offset, void *buf, uint32_t isize)
{
    if(!pfile||!pfile->pFile) return 0;
    if(pfile->file_system==M_FAT32)
    {
        return  write_fat32_to_disk((fat32_mem_node *)pfile->pFile,
                                  offset,buf,isize);
    }
    else if(pfile->file_system==M_SMFS)
    {
        return  write_smfs_to_disk((smfs_mem_node *)pfile->pFile,
                                 offset,buf,isize);
    }
    return 0;
}

uint32_t read_file(file *pfile, uint32_t offset, void *buf, uint32_t isize)
{
    if(!pfile||!pfile->pFile) return 0;
    if(pfile->file_system==M_FAT32)
    {
        return  read_fat32_from_disk((fat32_mem_node *)pfile->pFile,
                                 offset,buf,isize);
    }
    else if(pfile->file_system==M_SMFS)
    {
        return  read_smfs_from_disk((smfs_mem_node *)pfile->pFile,
                                offset,buf,isize);
    }
    return 0;
}

uint32_t get_file_size(file *pfile)
{
    uint32_t isize=0;
    if(pfile->file_system==M_FAT32)
    {
        isize =((fat32_mem_node *)pfile->pFile)->entry.file_size;

    }
    else if(pfile->file_system==M_SMFS)
    {
        isize =((smfs_mem_node *)pfile->pFile)->disk_node.i_size;
    }
    return isize;
}

// 格式化分区成smfs文件系统
#define BITS_PER_SECTOR (SECTOR_SIZE*8)
bool partition_format_to_smfs(partition_desc* part)
{
    // 超级块初始化
    part->s_block.sectors_count = part->sec_cnt;
    part->s_block.part_start_lba = part->start_lba;
    part->s_block.block_bitmap_lba = part->s_block.part_start_lba + 2;// 引导扇区和超级块扇区之后
    part->s_block.block_bitmap_sects = ROUND_UP(part->sec_cnt, BITS_PER_SECTOR);
    part->s_block.root_inode_lba = part->s_block.block_bitmap_lba +part->s_block.block_bitmap_sects;   //位图扇区之后
    part->s_block.data_start_lba =part->s_block.root_inode_lba+BLOCK_SIZE/SECTOR_SIZE; //根目录扇区之后
    part->s_block.dir_entry_size = sizeof(smfs_dir_entry);
   // 超级块同步至硬盘扇区
    disk_desc* hd = part->my_disk;
    uint8_t* buf = (uint8_t*)malloc_block(M_KERNEL,SECTOR_SIZE);
    memcpy(buf,&part->s_block,sizeof(super_block));
    disk_write(hd, part->start_lba + 1, buf, 1);
    // 清空位图扇区
    memset(buf,0,SECTOR_SIZE);
    uint32_t i=0;
    for(i=0;i<part->s_block.block_bitmap_sects;i++)
    {
        disk_write(hd,part->s_block.block_bitmap_lba+i,buf,1);
    }
    // 初始化根目录文件元信息扇区
    memset(buf,0,SECTOR_SIZE);
    ((smfs_disk_node*)buf)->inodeID=part->s_block.root_inode_lba;
    ((smfs_disk_node*)buf)->i_blocks[0]=0;
    ((smfs_disk_node*)buf)->i_size=0;
    ((smfs_disk_node*)buf)->type=FT_DIRECTORY;
    disk_write(hd,part->s_block.root_inode_lba,buf,1);
    free_block(buf);
    return true;
}
