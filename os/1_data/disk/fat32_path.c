#include "fat32_path.h"
#include "array_str.h"
#include "trace.h"
static bool search_path_fat32(const char* pathname,
                              path_search_record_fat32* searched_record)
{
    memset(searched_record,0,sizeof(path_search_record_fat32));
    char name[MAX_FILE_NAME_LEN_FAT+2] = {0};
    char* sub_path=path_parse( pathname, name);
    partition_desc* part=get_partition_by_name(name);
    if(!part) return false;
    fat32_mem_node* parent_dir = open_fat32_mem_node(part,0,0);

    searched_record->parent_dir= parent_dir;
    strcat(searched_record->searched_path, name);
    if(!sub_path)
    {
        searched_record->dir=open_fat32_mem_node(part,0,0);

      //  TRACE1("searched_record->dir:%d",
      //  searched_record->dir->entry.attribute);while(1);
        return true;
    }
    fat32_dir_entry dir_e;
    memset(name,0,MAX_FILE_NAME_LEN_FAT+2);

    sub_path = path_parse(sub_path, name);
    uint32_t clusterid=0, offset=0;
    while (name[0])
    {	   // 若第一个字符就是结束符,结束循环
        init_fat32_dir_entry(name,0, 0, &dir_e);
        if (search_fat32_dir_entry(parent_dir,&dir_e,&clusterid, &offset))
        {
            ASSERT(strlen(searched_record->searched_path) < 512);

            memset(name, 0, MAX_FILE_NAME_LEN_FAT+2);
            // 若sub_path不等于NULL,也就是未结束时继续拆分路径
            if (sub_path)
            {
                if(FT_DIRECTORY == dir_e.attribute)
                {
                    sub_path = path_parse(sub_path, name);
                    close_fat32_mem_node(parent_dir);
                    // 更新父目录
                    parent_dir = open_fat32_mem_node(part, clusterid, offset);
                    searched_record->parent_dir = parent_dir;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                searched_record->dir = open_fat32_mem_node(part, clusterid,
                                                           offset);
                return true;
            }
            strcat(searched_record->searched_path, "/");
            strcat(searched_record->searched_path, name);
        }
        else
        {
            return false;
        }
    }
    return false;
}

bool is_path_exist_fat32(const char *pathname,
                         path_search_record_fat32* searched_record,
                         enum file_types f_type)
{
    if(!search_path_fat32(pathname,  searched_record))
    {
        return false;
    }
    if(f_type==FT_UNKNOWN) return true;
    return  searched_record->dir->entry.attribute==f_type;
}

bool get_path_status_fat32(const char *path,path_status *status)
{
    path_search_record_fat32 searched_record;
    if(!search_path_fat32(path,  &searched_record))
    {

        close_fat32_mem_node(searched_record.parent_dir);
        close_fat32_mem_node(searched_record.dir);
        return false;
    }
    status->part=  searched_record.parent_dir->part;
    status->st_filetype=searched_record.dir->entry.attribute;
    status->st_size=searched_record.dir->entry.file_size;


    close_fat32_mem_node(searched_record.parent_dir);
    close_fat32_mem_node(searched_record.dir);
    return true;
}

fat32_mem_node* create_path_fat32(const char* pathname, enum file_types type)
{
    fat32_mem_node * f=0;
    path_search_record_fat32 searched_record;
    if(!is_path_exist_fat32(pathname,&searched_record,type))
    {
        char* name=strrchr(pathname, '/') +1;

        f = create_fat32_mem_node(searched_record.parent_dir,name, type);
        TRACE("creating file success.");
    }
    else
    {
        TRACE1("%s: file has already exist!.", pathname);
    }
    close_fat32_mem_node(searched_record.dir);
    close_fat32_mem_node(searched_record.parent_dir);
    return f;
}

fat32_mem_node* open_path_fat32(const char* pathname, enum file_types type)
{
    path_search_record_fat32 searched_record;

    fat32_mem_node * f=0;
    if(is_path_exist_fat32(pathname,&searched_record,type))
    {
        f=searched_record.dir;
    }
    else
    {
        TRACE1("%s: file has not exist!.", pathname);
    }

    close_fat32_mem_node(searched_record.parent_dir);
    return f;
}

void del_path_file_fat32(const char* pathname)
{
    path_search_record_fat32 searched_record;
    if(is_path_exist_fat32(pathname,&searched_record,FT_REGULAR))
    {
        delete_fat32_disk_node(searched_record.dir);
        if(searched_record.parent_dir->entry.file_size>=ENTRY_SIZE_FAT32)
        {

            searched_record.parent_dir->entry.file_size-=ENTRY_SIZE_FAT32;
            write_fat32_disk_node(searched_record.parent_dir);
        }

        TRACE("delete file success.");
    }
    else
    {
        TRACE1("%s: file has not exist!.", pathname);
    }
    close_fat32_mem_node(searched_record.parent_dir);
}

void del_path_fat32(const char *pathname)
{
    ASSERT(strlen(pathname) < MAX_PATH_LEN);
    path_search_record_fat32 searched_record;
    if(search_path_fat32(pathname,&searched_record))
    {
        if(searched_record.dir->entry.attribute==FT_DIRECTORY)
        {
            fat32_mem_node* pdir=searched_record.dir;
            fat32_dir_entry* buf  =(fat32_dir_entry*)malloc_block(M_KERNEL,
                                                                  SECTOR_SIZE);
            uint32_t dir_entry_size = ENTRY_SIZE_FAT32;
            uint32_t each_dir_entry_cnt = SECTOR_SIZE / dir_entry_size;
            uint32_t clusterid= pdir->entry.first_cluster_high<<16|
                                pdir->entry.first_cluster_low;
            bool bemptyentry=false;
            while(clusterid>0&&clusterid<USED_CLUSTER_MARK&&!bemptyentry)
            {
                uint32_t i=0;
                for(i=0;i<pdir->part->fat_param.sectors_per_cluster&&!bemptyentry;i++)
                {
                    read_disk_cluster(pdir->part, clusterid,i*SECTOR_SIZE,
                                      buf, SECTOR_SIZE);
                    uint32_t j=0;
                    for(j=0;j<each_dir_entry_cnt&&!bemptyentry;j++)
                    {
                        fat32_dir_entry* p_de1=buf+j;

                        char chfilename1[MAX_FILE_NAME_LEN_FAT+1]={0};
                        memcpy(chfilename1,p_de1->name,MAX_FILE_NAME_LEN_FAT);
                        if(!chfilename1[0]){bemptyentry=true;}
                        if(chfilename1[0]!=(char)0xE5&&
                                (p_de1->attribute==FT_DIRECTORY||
                                 p_de1->attribute==FT_REGULAR))
                        {
                            char name[MAX_FILE_NAME_LEN_FAT+2]={0};
                            get_name_from_fat32_dir_entry(p_de1,name);
                            char* temp_path= malloc_block(M_KERNEL,MAX_PATH_LEN);
                            strcpy(temp_path,pathname);
                            strcat(temp_path,"/");
                            strcat(temp_path,name);
                            del_path_fat32(temp_path);
                            free_block(temp_path);
                            p_de1->name[0]=0xE5;
                        }
                    }
                    write_disk_cluster(pdir->part, clusterid,i*SECTOR_SIZE, buf, SECTOR_SIZE);
                }
                clusterid=get_fat_cluster_next(pdir->part,0,clusterid);
            }
            free_block(buf);
        }
        delete_fat32_disk_node(searched_record.dir);
        if(searched_record.parent_dir->entry.file_size>=ENTRY_SIZE_FAT32)
        {
            searched_record.parent_dir->entry.file_size-=ENTRY_SIZE_FAT32;
            write_fat32_disk_node(searched_record.parent_dir);
        }
        TRACE("delete path success.");
    }
    else
    {
        TRACE1("%s: path has not exist!.", pathname);
    }
    close_fat32_mem_node(searched_record.parent_dir);
}

void copy_file_fat32(const char *pathname_src, const char *pathname_dst)
{
    path_search_record_fat32 searched_record_src,searched_record_dst;
    bool src_exist=is_path_exist_fat32(pathname_src,&searched_record_src,
                                       FT_REGULAR);
    bool  dst_exist=is_path_exist_fat32(pathname_dst,&searched_record_dst,
                                        FT_REGULAR);

    if(!src_exist||!dst_exist)
    {
        close_fat32_mem_node(searched_record_src.parent_dir);
        close_fat32_mem_node(searched_record_dst.parent_dir);
        close_fat32_mem_node(searched_record_src.dir);
        close_fat32_mem_node(searched_record_dst.dir);
        TRACE("path has not exist!.");
        return;
    }
    fat32_mem_node* p_file_node_src= searched_record_src.dir;
    fat32_mem_node* p_file_node_dst= searched_record_dst.dir;

    ASSERT(p_file_node_src&&p_file_node_dst);
    int isize=p_file_node_src->entry.file_size;
    char* buf= malloc_block(M_KERNEL,isize);
    read_fat32_from_disk(p_file_node_src,0,buf,isize);
    write_fat32_to_disk(p_file_node_dst,0,buf,isize);
    close_fat32_mem_node(p_file_node_src);
    close_fat32_mem_node(p_file_node_dst);
    close_fat32_mem_node(searched_record_src.parent_dir);
    close_fat32_mem_node(searched_record_dst.parent_dir);
    free_block(buf);
}

void copy_path_fat32(const char *pathname_src, const char *pathname_dst)
{
    path_search_record_fat32 searched_record_src,
            searched_record_dst,
            searched_record_dst_tset;
    bool src_exist=is_path_exist_fat32(pathname_src,
                                       &searched_record_src,FT_UNKNOWN);
    bool  dst_dir_exist=is_path_exist_fat32(pathname_dst,
                                            &searched_record_dst,FT_DIRECTORY);
    if(!src_exist||!dst_dir_exist)
    {
        close_fat32_mem_node(searched_record_src.parent_dir);
        close_fat32_mem_node(searched_record_dst.parent_dir);
        close_fat32_mem_node(searched_record_src.dir);
        close_fat32_mem_node(searched_record_dst.dir);
        TRACE("path dest has not exist!.");
        return;
    }
    char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
    strcpy(temp_path_dst,pathname_dst);
    strcat(temp_path_dst,"/");
    char* dirname = strrchr(pathname_src, '/') + 1;
    strcat(temp_path_dst,dirname);
    bool dst_exist=is_path_exist_fat32(temp_path_dst,
                                       &searched_record_dst_tset,
                                       searched_record_src.dir->entry.attribute);
    close_fat32_mem_node(searched_record_dst_tset.parent_dir);
    close_fat32_mem_node(searched_record_dst_tset.dir);
    free_block(temp_path_dst);
    if(dst_exist)
    {
        close_fat32_mem_node(searched_record_src.parent_dir);
        close_fat32_mem_node(searched_record_dst.parent_dir);
        close_fat32_mem_node(searched_record_src.dir);
        close_fat32_mem_node(searched_record_dst.dir);
        TRACE("dest name has exist!.");
        return;
    }
    partition_desc* part=searched_record_src.parent_dir->part;
    if(searched_record_src.dir->entry.attribute==FT_REGULAR)
    {
        char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
        strcpy(temp_path_dst,pathname_dst);
        strcat(temp_path_dst,"/");
        char* dirname = strrchr(pathname_src, '/') + 1;
        strcat(temp_path_dst,dirname);
        fat32_mem_node* p_file_node_dst= create_path_fat32(temp_path_dst,
                                                           FT_REGULAR);
        copy_file(pathname_src, temp_path_dst);
        close_fat32_mem_node(p_file_node_dst);
        free_block(temp_path_dst);

    }
    else if(searched_record_src.dir->entry.attribute==FT_DIRECTORY)
    {
        char* temp_path_dst= malloc_block(M_KERNEL,MAX_PATH_LEN);
        strcpy(temp_path_dst,pathname_dst);
        strcat(temp_path_dst,"/");
        char* dirname = strrchr(pathname_src, '/') + 1;
        strcat(temp_path_dst,dirname);
        fat32_mem_node* p_dir_inode_dst= create_path_fat32(temp_path_dst,
                                                           FT_DIRECTORY);
        fat32_mem_node* pdir=searched_record_src.dir;
        fat32_dir_entry* buf  =(fat32_dir_entry*)malloc_block(M_KERNEL,
                                                              SECTOR_SIZE);
        uint32_t dir_entry_size = ENTRY_SIZE_FAT32;
        uint32_t each_dir_entry_cnt = SECTOR_SIZE / dir_entry_size;
        uint32_t clusterid= pdir->entry.first_cluster_high<<16|
                            pdir->entry.first_cluster_low;
        bool bemptyentry=false;
        while(clusterid>0&&clusterid<USED_CLUSTER_MARK&&!bemptyentry)
        {
            uint32_t i=0;
            for(i=0;i<part->fat_param.sectors_per_cluster&&!bemptyentry;i++)
            {
                read_disk_cluster(pdir->part, clusterid,i*SECTOR_SIZE,
                                  buf, SECTOR_SIZE);
                uint32_t j=0;
                for(j=0;j<each_dir_entry_cnt&&!bemptyentry;j++)
                {
                    fat32_dir_entry* p_de1=buf+j;
                    char chfilename1[MAX_FILE_NAME_LEN_FAT+1];
                    memcpy(chfilename1,p_de1->name,MAX_FILE_NAME_LEN_FAT);
                    if(!chfilename1[0]){bemptyentry=true;}
                    if(chfilename1[0]!=(char)0xE5&&
                            (p_de1->attribute==FT_DIRECTORY||
                             p_de1->attribute==FT_REGULAR))
                    {
                        char name[MAX_FILE_NAME_LEN_FAT+2]={0};
                        get_name_from_fat32_dir_entry(p_de1,name);
                        char* temp_path= malloc_block(M_KERNEL,MAX_PATH_LEN);
                        TRACE1("name:%s",name);
                        strcpy(temp_path,pathname_src);
                        strcat(temp_path,"/");
                        strcat(temp_path,name);
                        copy_path_fat32(temp_path,temp_path_dst);
                        free_block(temp_path);
                    }
                }
            }
            clusterid=get_fat_cluster_next(part,0,clusterid);
        }
        close_fat32_mem_node(p_dir_inode_dst);
        free_block(temp_path_dst);
        free_block(buf);
    }
    TRACE("copy path success.");
    close_fat32_mem_node(searched_record_src.parent_dir);
    close_fat32_mem_node(searched_record_dst.parent_dir);
    close_fat32_mem_node(searched_record_src.dir);
    close_fat32_mem_node(searched_record_dst.dir);

}

void rename_path_fat32(const char *pathname, char *pnewname)
{
    path_search_record_fat32 searched_record,searched_record1;
    bool bexist=is_path_exist_fat32(pathname,&searched_record,FT_UNKNOWN);
    if(bexist)
    {
        strcat(searched_record.searched_path, "/");
        strcat(searched_record.searched_path, pnewname);
        bexist=is_path_exist_fat32(searched_record.searched_path,
                                   &searched_record1,FT_UNKNOWN);
        if(!bexist)
        {
            fat32_dir_entry* pentry=&searched_record.dir->entry;
            char filenamedest[MAX_FILE_NAME_LEN_FAT+1];
            make_fat32_dir_entry_name(pnewname,filenamedest);
            memcpy(pentry,filenamedest,MAX_FILE_NAME_LEN_FAT);
            write_fat32_disk_node(searched_record.dir);
            TRACE("rename path success.");
        }
        else
        {
            TRACE("file name has exist.");
        }
        close_fat32_mem_node(searched_record1.parent_dir);
        close_fat32_mem_node(searched_record1.dir);
    }
    else
    {
        TRACE1("%s: path has not exist!.", pathname);
    }
    close_fat32_mem_node(searched_record.parent_dir);
    close_fat32_mem_node(searched_record.dir);
}

typedef struct _entry_name
{
    char ch[MAX_FILE_NAME_LEN_FAT+2];
    list_node self;
}ENTRY_NAME,*PENTRY_NAME;

char *make_dir_string_fat32(const char *pathname,mflags flag,const char* file_type)
{
    ASSERT(strlen(pathname) < MAX_PATH_LEN);
    list list_name;list_init(&list_name);
    path_search_record_fat32 searched_record;
    if(is_path_exist_fat32(pathname,&searched_record,FT_DIRECTORY))
    {
        fat32_mem_node* pdir=searched_record.dir;
        partition_desc* part=pdir->part;
        fat32_dir_entry* buf  =(fat32_dir_entry*)malloc_block(M_KERNEL,
                                                              SECTOR_SIZE);
        uint32_t dir_entry_size = ENTRY_SIZE_FAT32;
        uint32_t each_dir_entry_cnt = SECTOR_SIZE / dir_entry_size;
        uint32_t clusterid= pdir->entry.first_cluster_high<<16|
                            pdir->entry.first_cluster_low;
//        TRACE1("clusterid:%d",clusterid);while(1);
        bool bemptyentry=false;
        while(clusterid>0&&clusterid<USED_CLUSTER_MARK&&!bemptyentry)
        {
            uint32_t i=0;
     //       TRACE1("pdir->part->fat_param.sectors_per_cluster:%d",
     //       pdir->part->fat_param.sectors_per_cluster);
     //       TRACE1("part:%s",part->name);
            for(i=0;i<pdir->part->fat_param.sectors_per_cluster&&!bemptyentry;i++)
            {
                read_disk_cluster(pdir->part, clusterid,i*SECTOR_SIZE,
                                  buf, SECTOR_SIZE);

             //   TRACE_SECTION((char*)buf);
                uint32_t j=0;
                for(j=0;j<each_dir_entry_cnt&&!bemptyentry;j++)
                {
                    fat32_dir_entry* p_de1=buf+j;
                    uint8_t mark=*((uint8_t*)p_de1);
                    if(mark==0)
                    {
                        bemptyentry=true;
                    }
                    if(mark==0xE5)
                    {

                    }
                //    TRACE1("%c,%x",chfilename1[0],p_de1->attribute);while(1);
                    if(mark!=0xE5&&
                    (p_de1->attribute==FT_REGULAR||p_de1->attribute==FT_DIRECTORY))
                    {
                        PENTRY_NAME pname=malloc_block(M_KERNEL,sizeof(ENTRY_NAME));
                        get_name_from_fat32_dir_entry(p_de1,pname->ch);
                        if(!strcmp(pname->ch,".")||!strcmp(pname->ch,".."))
                        {
                            continue;
                        }
                        if(file_type)
                        {
                            char* p= strchr(pname->ch,'.');
                            if(p&&!strcmp1(p+1,file_type))
                            {
                                list_append(&list_name,&pname->self);
                            }
                        }
                        else
                        {
                            list_append(&list_name,&pname->self);
                        }
                    }
                }
            }
            clusterid=get_fat_cluster_next(part,0,clusterid);
        }
        free_block(buf);
    }
    close_fat32_mem_node(searched_record.dir);
    close_fat32_mem_node(searched_record.parent_dir);
    int32_t icount=list_len(&list_name);
    if(!icount)
    {
        return 0;
    }
    char* str =malloc_block(flag,icount*(MAX_FILE_NAME_LEN_FAT+3)+1);
    while(list_name.head.next!=&list_name.tail)
    {
        PENTRY_NAME pname=elem2entry(ENTRY_NAME,self,list_pop(&list_name));
        strcat(str,pname->ch);
        strcat(str,"\n");
        free_block(pname);
    }
  //  TRACE(str);while(1);
    return str;
}
