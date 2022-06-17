#include "explore.h"
#include "trace.h"
#include "array_str.h"
#include "syscall.h"
#include "disk.h"
#include "kernel.h"
#include "screen.h"
#include "path.h"
#include "global_def.h"
PPCB m_pExplorePCB;

typedef enum _CTRLID_EXPLORE
{
    IDS_EXPLORE=IDS_SYSTEM_MAX,
    IDS_PATH,
    IDS_BUTTON,
    IDS_EXPLORE_MENU,
    IDS_EXPLORE_TEXT,

}CTRLID_EXPLORE;

static void make_root_input(void)
{
    char* p_path=0;
    list* p_partition_list=syscall_get_partition_list();
    list_node* pelem=p_partition_list->head.next;
    uint32_t count=list_len(p_partition_list);
    if(count)
    {
        char* temp= p_path=syscall_malloc_block(count*8);
        while(pelem!=&p_partition_list->tail)
        {
            partition_desc* part = elem2entry(partition_desc, self, pelem);
            ASSERT(part);
            uint32_t len=strlen(part->name);
            strcpy(temp,part->name);
            strcat(temp,"\n");
            temp+=len+1;
            pelem=pelem->next;
        }
    }
    syscall_clear_and_set_block(&m_pExplorePCB->sblock_list,IDS_EXPLORE,p_path,COLOR_NORMAL);
    syscall_free_block(p_path);
    p_path=0;
    syscall_clear_and_set_block(&m_pExplorePCB->sblock_list,IDS_PATH,0,COLOR_NORMAL);
    syscall_clear_and_set_block(&m_pExplorePCB->sblock_list,IDS_EXPLORE_MENU,0,COLOR_NORMAL);
}
static void fresh_path(char* path)
{
    syscall_clear_and_set_block(&m_pExplorePCB->sblock_list,IDS_EXPLORE_MENU,0,COLOR_NORMAL);
    syscall_clear_and_set_block(&m_pExplorePCB->sblock_list,IDS_PATH,path,COLOR_NORMAL);
    char* str=syscall_make_dir_string(path );
    syscall_clear_and_set_block(&m_pExplorePCB->sblock_list,IDS_EXPLORE,str,COLOR_NORMAL);
    syscall_free_block(str);str=0;
}
static FILE_TYPE identify_file(char* path)
{
    file* pfilenode=syscall_open_path_file(path);
    char* pbuf=syscall_malloc_block(8);
    syscall_read_file(pfilenode,0,pbuf,7);
    syscall_clear_and_set_block(&m_pExplorePCB->sblock_list,IDS_EXPLORE_TEXT ,pbuf,COLOR_NORMAL);

    FILE_TYPE type;
    char* p=strrchr(path,'.');
    if(!strcmp(pbuf,"\177ELF\1\1\1")&&!strcmp1(p,".bin") )//
    {
        type= FILE_ELF;
    }
    else
    {
        type= FILE_BINARY;
    }
    syscall_free_block(pbuf);
    syscall_close_file(pfilenode);
    return type;
}
static void mf_explore_block_command_left(void *input, void **output,uint32_t config)
{
    UNUSED(output);
    PSCREENBLOCK pscreenblock=syscall_get_screenblock_by_id(IDS_PATH,&m_pExplorePCB->sblock_list);
    char* ppath=syscall_get_buff_from_block_all(pscreenblock);
    TRACE(ppath);
    int32_t isizepath= strlen(ppath);
    if(isizepath)
    {
        if(ppath[isizepath-1]=='\n')
        {
            ppath[isizepath-1]=0;
            isizepath--;
        }
    }
    if(config==IDS_BUTTON)
    {
        if(!strcmp((char*)input,"BACK"))
        {
            //   ASSERT(0);
            //   uint32_t test=0;
            //   test=1/test;
            uint32_t len=strlen(ppath);
            char* dir_name= strrchr(ppath,'/');
            if(!len||!dir_name)
            {
                make_root_input();
                return;
            }
            memset(dir_name,0,strlen(dir_name));
            fresh_path(ppath);
        }
    }
    else if(config==IDS_EXPLORE)
    {
        char* pfile=(char*)input;
        int32_t isizefile= strlen(pfile);
        char* pnewpath=syscall_malloc_block(isizefile+isizepath+2);
        if(isizepath)
        {
            memcpy(pnewpath,ppath,isizepath);
            strcat(pnewpath,"/");
        }
        strcat(pnewpath,pfile);
        //        TRACE(pnewpath);
        path_status status;
        bool flag= syscall_get_path_status(pnewpath,&status);
        if(flag)
        {
            //  TRACE1("%s,:%s,%x",pnewpath,status.part->name,status.st_filetype);
            if(status.st_filetype==FT_DIRECTORY)
            {
                fresh_path(pnewpath);
            }
            else if(status.st_filetype==FT_REGULAR)
            {
                FILE_TYPE file_type= identify_file(pnewpath);
                char* p=malloc_block(M_KERNEL,isizefile+isizepath+2);
                strcpy(p,pnewpath);

                MESSAGE msg=make_message(msg_process_load,p,0,(uint32_t)file_type<<16);
                msgq_putmsg(&m_gKCB.msgQueue,msg);
            }
        }
        syscall_free_block(pnewpath);
    }
    else if(config==IDS_EXPLORE_MENU)
    {
        char* pbutton=(char*)input;
        PCURSOR pcursor=syscall_get_cursor(CURSOR_RIGHT);
        if(pcursor->pscreenblock->self.iID==IDS_EXPLORE)
        {
            char* pdir=syscall_get_buff_from_block_by_color(
                        pcursor->pscreenblock,
                        COLOR_SELECT_RIGHT,
                        SRCOR );

            int32_t isizedir= strlen(pdir);
            int32_t isize=isizedir+isizepath+2;
            char*  p_path_operator=syscall_malloc_block(isize);
            if(isizepath>0)
            {
                memcpy(p_path_operator,ppath,isizepath);
                strcat(p_path_operator,"/");
            }
            strcat(p_path_operator,pdir);

            syscall_free_block(pdir);pdir=0;
            if((!strcmp(pbutton,"COPY")||!strcmp(pbutton,"CUT"))&&isizepath>0)
            {
                bool bcut=false;
                if(!strcmp(pbutton,"CUT"))
                {
                    bcut=true;
                }
                syscall_set_clipboard(p_path_operator,isize,CLIP_PATH,bcut);

            }
            else if(!strcmp(pbutton,"PASTE"))
            {
                PCLIPBOARD  pclipboard=syscall_get_clipboard();
                if(pclipboard->type==CLIP_PATH)
                {
                    syscall_copy_path(pclipboard->p,p_path_operator);
                    if(pclipboard->bcut)
                    {
                        syscall_del_path(pclipboard->p);
                    }
                    fresh_path(p_path_operator);
                }
            }
            else if(!strcmp(pbutton,"CREATE"))
            {
                char* pname= syscall_get_system_command_str();
                if(pname)
                {
                    int32_t isizename= strlen(pname);
                    if(isizename)
                    {
                        char* p_path_dst=syscall_malloc_block(isizename+isize+2);

                        strcat(p_path_dst,p_path_operator);
                        strcat(p_path_dst,"/");
                        strcat(p_path_dst,pname);
                        if(strchr(pname,'.')!=0)
                        {
                            file* p_file=0;
                            p_file=syscall_create_path_file(p_path_dst);
                            syscall_close_file(p_file);

                        }
                        else
                        {
                            dir* p_dir=0;
                            p_dir=syscall_create_path_dir(p_path_dst);
                            syscall_close_dir(p_dir);
                        }
                        syscall_free_block(p_path_operator);
                        p_path_operator=p_path_dst;
                        isize=strlen(p_path_dst);
                    }
                    syscall_free_block(pname);
                }
            }
            else if(!strcmp(pbutton,"DELETE")&&isizepath>0)
            {
                syscall_del_path(p_path_operator);
                fresh_path(ppath);
            }
            else if(!strcmp(pbutton,"RENAME")&&isizepath>0)
            {
                char* pname= syscall_get_system_command_str();
                if(pname)
                {
                    int32_t isizename= strlen(pname);
                    if(isizename)
                    {
                        syscall_rename_path(p_path_operator,pname);
                        fresh_path(ppath);
                    }
                    syscall_free_block(pname);
                }
            }
            char* out=syscall_malloc_block(strlen(pbutton)+strlen(p_path_operator)+2);
            strcpy(out,pbutton);
            strcat(out," ");
            strcat(out,p_path_operator);
            syscall_set_output(out);
            syscall_free_block(out);out=0;
            syscall_free_block(p_path_operator);p_path_operator=0;
        }
    }
    syscall_free_block(ppath);
}
static void mf_explore_block_command_right(void *input, void **output,uint32_t config)
{
    UNUSED(input);
    UNUSED(output);
    UNUSED(config);
    if(config==IDS_EXPLORE)
    {
        PSCREENBLOCK pscreenblock=syscall_get_screenblock_by_id(IDS_EXPLORE_MENU,
                                                                &m_pExplorePCB->sblock_list);
        syscall_set_buff_to_block(pscreenblock,0,0,"COPY\nCUT\nPASTE\nCREATE\nDELETE\nRENAME",
                                  COLOR_NORMAL);
        syscall_fresh_screen_block(pscreenblock);
    }
}
static void mf_explore_keyboard(void *input, void **output,uint32_t config)
{
    UNUSED(input);
    UNUSED(output);
    UNUSED(config);
}
static void mf_explore_mouse(void *input, void **output,uint32_t config)
{
    UNUSED(input);
    UNUSED(output);
    UNUSED(config);
    uint16_t status=(uint16_t)((uint32_t)input>>16);
    uint8_t flag=status;
    flag^=status>>8;
    if(flag&CURSOR_LEFT_PRESS)
    {
    }
    if(flag&CURSOR_RIGHT_PRESS)
    {
        syscall_clear_and_set_block(&m_pExplorePCB->sblock_list,
                                    IDS_EXPLORE_MENU,0,COLOR_NORMAL);
    }
}
int explore_main(PPCB pPCB,char** pinput)
{
    UNUSED(pinput);
    m_pExplorePCB=pPCB;
    int32_t width,height;
    syscall_get_screen_size(&width,&height);
    PSCREENBLOCK pscreenblock=  syscall_malloc_screen_block(IDS_EXPLORE,
                                                            0, 4, 50, 6,
                                                            SBLOCK_INPUT,0,PALCEHOLD_CHAR );

    list_append(&pPCB->sblock_list,&pscreenblock->self);
    pscreenblock= syscall_malloc_screen_block(IDS_BUTTON,
                                              51, 3, 20, 1,
                                              SBLOCK_INPUT,0,'|' );

    syscall_set_buff_to_block(pscreenblock,0,0,"BACK",COLOR_NORMAL);
    list_append(&pPCB->sblock_list,&pscreenblock->self);


    pscreenblock= syscall_malloc_screen_block(IDS_PATH,
                                              0, 3, 50, 1,
                                              SBLOCK_OUTPUT,0,0 );

    list_append(&pPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_EXPLORE_MENU,
                                              51, 4, 20,  6,
                                              SBLOCK_INPUT,0,PALCEHOLD_CHAR );

    list_append(&m_pExplorePCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_EXPLORE_TEXT,
                                              0, 12, 79,  3,
                                              SBLOCK_INOUTPUT,0,PALCEHOLD_CHAR );

    list_append(&m_pExplorePCB->sblock_list,&pscreenblock->self);
    make_root_input( );
    syscall_add_msgdesc_to_list(&pPCB->msgdesc_list,msg_block_command_left,
                                mf_explore_block_command_left,"explore_command_left") ;
    syscall_add_msgdesc_to_list(&pPCB->msgdesc_list,msg_block_command_right,
                                mf_explore_block_command_right,"explore_command_right") ;
    syscall_add_msgdesc_to_list(&pPCB->msgdesc_list,msg_keyboard,
                                mf_explore_keyboard,"explore_keyboard");
    syscall_add_msgdesc_to_list(&pPCB->msgdesc_list,msg_mouse,
                                mf_explore_mouse,"explore_mouse");

    return 0;
}



