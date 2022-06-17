#include "smfs_path.h"
#include "array_str.h"
#include "trace.h"

static bool search_path_smfs(const char* pathname, path_search_record_smfs* searched_record)
{
    memset(searched_record,0,sizeof(path_search_record_smfs));
    char name[MAX_FILE_NAME_LEN_SMFS] = {0};
    char* sub_path=path_parse( pathname, name);
    partition_desc* part=get_partition_by_name(name);
    if(!part) return false;
    smfs_mem_node* parent_dir = open_smfs_mem_node(part,part->s_block.root_inode_lba);

    searched_record->parent_dir= parent_dir;
    strcat(searched_record->searched_path, name);
    if(!sub_path)
    {
        searched_record->dir= open_smfs_mem_node(part,part->s_block.root_inode_lba);

      //  TRACE1("searched_record->dir:%d",searched_record->dir->entry.attribute);while(1);
        return true;
    }
    smfs_dir_entry dir_e;
    memset(name,0,MAX_FILE_NAME_LEN_SMFS);

    sub_path = path_parse(sub_path, name);
    while (name[0])
    {	   // 若第一个字符就是结束符,结束循环
        /* 记录查找过的路径,但不能超过searched_path的长度512字节 */
        uint32_t blockindex,blockoffset;
        init_smfs_dir_entry(name,0,  &dir_e);
        if (search_smfs_dir_entry(parent_dir,&dir_e,&blockindex, &blockoffset))
        {
            ASSERT(strlen(searched_record->searched_path) < 512);
            strcat(searched_record->searched_path, "/");
            strcat(searched_record->searched_path, name);
            memset(name, 0, MAX_FILE_NAME_LEN_SMFS);
            /* 若sub_path不等于NULL,也就是未结束时继续拆分路径 */
            smfs_mem_node *pnode=open_smfs_mem_node(part, dir_e.i_no);
            if (sub_path)
            {
                if(FT_DIRECTORY == pnode->disk_node.type)
                {
                    sub_path = path_parse(sub_path, name);
                    close_smfs_mem_node(parent_dir);
                    searched_record->parent_dir=parent_dir = pnode;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                searched_record->dir = pnode;
                return true;
            }
        }
        else
        {
            return false;
        }
    }
    return false;
}

bool is_path_exist_smfs(const char *pathname,path_search_record_smfs* searched_record,enum file_types f_type)
{
    if(!search_path_smfs(pathname,  searched_record))
    {
        return false;
    }
    if(f_type==FT_UNKNOWN) return true;
    return  searched_record->dir->disk_node.type==f_type;
}

bool get_path_status_smfs(const char* path, path_status* status)
{
    ASSERT(path);
    ASSERT(strlen(path)<MAX_PATH_LEN);
    memset(status,0,sizeof(path_status));

    path_search_record_smfs searched_record;
    memset(&searched_record, 0, sizeof(path_search_record_smfs));   // 记得初始化或清0,否则栈中信息不知道是什么
    bool flag=search_path_smfs(path, &searched_record);
    if (flag)
    {
        status->part=searched_record.dir->part;
        status->st_filetype=searched_record.dir->disk_node.type;
        status->st_size=searched_record.dir->disk_node.i_size;
    }
    else
    {
        TRACE1("sys_stat: %s not found.", path);
        while(1);
    }
    close_smfs_mem_node(searched_record.parent_dir);
    close_smfs_mem_node(searched_record.dir);
    return flag;
}

smfs_mem_node *create_path_smfs(const char *pathname, enum file_types type)
{
    smfs_mem_node * f=0;
    path_search_record_smfs searched_record;
    if(!is_path_exist_smfs(pathname,&searched_record,type))
    {
        f = create_smfs_mem_node(searched_record.parent_dir,(strrchr(pathname, '/') + 1), type);
        TRACE("creating file success.");
    }
    else
    {
        TRACE1("%s: path has already exist!.", pathname);
    }
    close_smfs_mem_node(searched_record.parent_dir);
    close_smfs_mem_node(searched_record.dir);
    return f;
}
// 打开或创建文件成功后,返回文件指针变量,否则返回0
smfs_mem_node *open_path_smfs(const char* pathname, enum file_types type)
{
    smfs_mem_node * f=0;
    path_search_record_smfs searched_record;
    bool flag=is_path_exist_smfs(pathname, &searched_record,type);
    if(flag)
    {
        f = searched_record.dir;
        TRACE("open file success.");
    }
    else
    {
        TRACE1("%s: file has not exist!.", pathname);
    }
    close_smfs_mem_node(searched_record.parent_dir);
    return f;
}

// 删除文件(非目录)
void del_path_file_smfs(const char* pathname)
{
    path_search_record_smfs searched_record;
    bool flag=is_path_exist_smfs(pathname,&searched_record,FT_REGULAR);

    if(flag)
    {
        delete_smfs_disk_node(searched_record.parent_dir,searched_record.dir);
        TRACE("delete file success.");
    }
    else
    {
        PANIC1("%s: file has not exist!.", pathname);
    }
    close_smfs_mem_node(searched_record.parent_dir);
}

void del_path_smfs(const char *pathname)
{
    ASSERT(strlen(pathname) < MAX_PATH_LEN);

    path_search_record_smfs searched_record;
    bool  flag=is_path_exist_smfs(pathname,&searched_record,FT_UNKNOWN);
    if(flag)
    {
        if(searched_record.dir->disk_node.type==FT_REGULAR)
        {
            delete_smfs_disk_node(searched_record.parent_dir, searched_record.dir);
        }
        else if(searched_record.dir->disk_node.type==FT_DIRECTORY)
        {
            smfs_mem_node* pdir=searched_record.dir;
            smfs_dir_entry entry;
            while(get_smfs_dir_entry_by_index(pdir,&entry,0))
            {
                char* temp_path= malloc_block(M_KERNEL,MAX_PATH_LEN);
                strcpy(temp_path,pathname);
                strcat(temp_path,"/");
                strcat(temp_path,entry.filename);
                del_path_smfs(temp_path);
                free_block(temp_path);
            }
            delete_smfs_disk_node(searched_record.parent_dir, searched_record.dir);
        }
        TRACE("delete path success.");
    }
    else
    {
        TRACE1("%s: path has not exist!.", pathname);
    }
    close_smfs_mem_node(searched_record.parent_dir);
}

void copy_file_smfs(const char *pathname_src, const char *pathname_dst)
{
    path_search_record_smfs searched_record_src,searched_record_dst;
    bool inode_no_src=is_path_exist_smfs(pathname_src,&searched_record_src,FT_REGULAR);
    bool inode_no_dst=is_path_exist_smfs(pathname_dst,&searched_record_dst,FT_REGULAR);

    if(inode_no_src&&inode_no_dst)
    {
        smfs_mem_node* p_file_node_src=searched_record_src.dir;
        smfs_mem_node* p_file_node_dst=searched_record_dst.dir;
        int isize=p_file_node_src->disk_node.i_size;
        char* buf= malloc_block(M_KERNEL,isize);
        read_smfs_from_disk(p_file_node_src,0,buf,isize);
        write_smfs_to_disk(p_file_node_dst,0,buf,isize);
        free_block(buf);

    }
    else
    {
        TRACE("path has not exist!.");

    }
    close_smfs_mem_node(searched_record_src.dir);
    close_smfs_mem_node(searched_record_dst.dir);
    close_smfs_mem_node(searched_record_src.parent_dir);
    close_smfs_mem_node(searched_record_dst.parent_dir);
}

void copy_path_smfs(const char *pathname_src, const char *pathname_dst)
{
    path_search_record_smfs searched_record_src,searched_record_dst,searched_record_dst_tset;
    bool inode_no_src=is_path_exist_smfs(pathname_src,&searched_record_src,FT_UNKNOWN);
    bool inode_no_dst=is_path_exist_smfs(pathname_dst,&searched_record_dst,FT_DIRECTORY);

    if(inode_no_src&&inode_no_dst)
    {

        char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
        strcpy(temp_path_dst,pathname_dst);
        strcat(temp_path_dst,"/");
        char* dirname = strrchr(pathname_src, '/') + 1;
        strcat(temp_path_dst,dirname);
        bool inode_no_dst_test=is_path_exist_smfs(temp_path_dst,&searched_record_dst_tset,FT_UNKNOWN);
        close_smfs_mem_node(searched_record_dst_tset.parent_dir);
        close_smfs_mem_node(searched_record_dst_tset.dir);
        free_block(temp_path_dst);
        if(inode_no_dst_test)
        {
            close_smfs_mem_node(searched_record_src.dir);
            close_smfs_mem_node(searched_record_dst.dir);
            close_smfs_mem_node(searched_record_src.parent_dir);
            close_smfs_mem_node(searched_record_dst.parent_dir);
            TRACE("dest name has exist!.");
            return;
        }

        partition_desc* part=searched_record_src.parent_dir->part;
        if(searched_record_src.dir->disk_node.type==FT_REGULAR)
        {
            char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
            strcpy(temp_path_dst,pathname_dst);
            strcat(temp_path_dst,"/");
            char* dirname = strrchr(pathname_src, '/') + 1;
            strcat(temp_path_dst,dirname);
            smfs_mem_node* p_file_node_dst= create_path_smfs(temp_path_dst,FT_REGULAR);
            copy_file_smfs(pathname_src, temp_path_dst);
            close_smfs_mem_node(p_file_node_dst);
            free_block(temp_path_dst);

        }
        else if(searched_record_src.dir->disk_node.type==FT_DIRECTORY)
        {
            char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
            strcpy(temp_path_dst,pathname_dst);
            strcat(temp_path_dst,"/");
            char* dirname = strrchr(pathname_src, '/') + 1;
            strcat(temp_path_dst,dirname);
            smfs_mem_node* p_dir_inode_dst= create_path_smfs(temp_path_dst,FT_DIRECTORY);
            close_smfs_mem_node(p_dir_inode_dst);
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
                    copy_path_smfs(temp_path_src,temp_path_dst);
                    free_block(temp_path_src);
                }
            }
            free_block(temp_path_dst);

        }
        TRACE("copy path success.");
    }
    else
    {
        TRACE("path dest has not exist!.");
    }

    close_smfs_mem_node(searched_record_src.dir);
    close_smfs_mem_node(searched_record_dst.dir);
    close_smfs_mem_node(searched_record_src.parent_dir);
    close_smfs_mem_node(searched_record_dst.parent_dir);
}

void rename_path_smfs(const char *pathname, char *pnewname)
{
    path_search_record_smfs searched_record;
    bool bexist=is_path_exist_smfs(pathname,&searched_record,FT_UNKNOWN);
    if(bexist)
    {
        rename_smfs_dir_entry(searched_record.parent_dir,
                              searched_record.dir->disk_node.inodeID,
                              pnewname);
        TRACE("rename path success.");
    }
    else
    {
        TRACE1("%s: path has not exist!.", pathname);
    }
    close_smfs_mem_node(searched_record.dir);
    close_smfs_mem_node(searched_record.parent_dir);
}

char *make_dir_string_smfs(const char *pathname,mflags flag,const char* file_type)
{
    TRACE(pathname);
    smfs_mem_node* pdir= open_path_smfs(pathname,FT_DIRECTORY);
    if(!pdir){close_smfs_mem_node(pdir); return 0;}
    if(!pdir->disk_node.i_size){close_smfs_mem_node(pdir);return 0;}
    partition_desc* part=pdir->part;
    uint32_t dir_entry_size = part->s_block.dir_entry_size;
    uint32_t each_dir_entry_cnt = BLOCK_SIZE / dir_entry_size;  // 1块内可容纳的目录项个数
    uint32_t total_dir_entry_cnt = pdir->disk_node.i_size/ dir_entry_size;
    uint32_t i=0;
    uint32_t i_dir_entry_count=0;
    if(!total_dir_entry_cnt) {close_smfs_mem_node(pdir);return 0;}
    smfs_block_cache* cache=&pdir->cache;
    char* str=malloc_block(flag,total_dir_entry_cnt*(MAX_FILE_NAME_LEN_SMFS+1)+1);//
    uint8_t* buf  =cache->buf;

    for(i=0;i<indirect3_block_count;i++)
    {
        if(i_dir_entry_count>=total_dir_entry_cnt)
        {
            break;
        }
        uint32_t block_idx=get_smfs_disk_block_start_section_by_index(part,&pdir->disk_node,i
                                                                      ,cache);
        if(block_idx)
        {
            disk_read(part->my_disk,block_idx,buf,BLOCK_SIZE/SECTOR_SIZE);
            smfs_dir_entry* p_de1=(smfs_dir_entry*)buf;
            uint32_t j=0;
            for(j=0;j<each_dir_entry_cnt;j++)
            {
                    if((p_de1+j)->filename[0]==0)
                    {
                        close_smfs_mem_node(pdir);
                        return str;
                    }
                    else if((p_de1+j)->filename[0]!=(char)0xE5)
                    {
                        i_dir_entry_count++;
                        if(file_type)
                        {
                            char* p= strchr((p_de1+j)->filename,'.');
                            if(!p||strcmp1(p+1,file_type))
                            {
                                continue;
                            }
                        }
                        strcat(str,(p_de1+j)->filename);
                        strcat(str,"\n");
                    }
            }
        }
        else
        {
            break;
        }

    }
    close_smfs_mem_node(pdir);
    return str;
}

