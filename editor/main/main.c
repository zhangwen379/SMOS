#include "queue.h"
#include "array_str.h"
#include "list.h"
#include "syscall.h"
#include "process.h"
#include "interrupt_keyboard.h"
#include "screen.h"
#include "path.h"
#include "trace.h"
#include "smfs_node.h"
#include "fat32_node.h"
#include "kernel.h"
PPCB m_EditorPCB=0;
typedef enum _CTRLID_EDITOR
{
    IDS_EDITOR=IDS_SYSTEM_MAX+10,
    IDS_OPEN,
    IDS_SAVE,
    IDS_FILE,
    IDS_INDEX,
    IDS_PATH,
    IDS_WORD,
    IDS_SECTION,
    IDS_LINE,
    IDS_CHAR,
    IDS_CLIPBOARD,
    IDS_EDITOR_MENU

}CTRLID_EDITOR;

static void fresh_path(void)
{
    if(m_EditorPCB)
    {
        syscall_clear_and_set_block(&m_EditorPCB->sblock_list,IDS_PATH,
                                    m_EditorPCB->inputpath,COLOR_NORMAL);
    }
}
static void fresh_file(void)
{
    if(m_EditorPCB)
    {
        int isize=strlen(m_EditorPCB->inputpath);
        if(isize)
        {
            char* temp=syscall_malloc_block(isize+1);
            strcpy(temp,m_EditorPCB->inputpath);
            char* p=strchr(temp,'/');
            if(p)
            {
                *p=0;
                char* p1=strchr(p+1,'.');
                if(p1)
                {
                    char* str=syscall_make_file_string(temp,p1+1 );
                    if(!str){syscall_free_block(str);str=0; return;}
                    syscall_clear_and_set_block(
                                &m_EditorPCB->sblock_list,IDS_FILE,str,COLOR_NORMAL);
                    syscall_free_block(str);str=0;
                }
            }
            syscall_free_block(temp);
        }
    }
}
static void fresh_index(void)
{
    char ch[16]={0};
    PSCREENBLOCK pscreenblock_editor=syscall_get_screenblock_by_id(IDS_EDITOR,
                                                                   &m_EditorPCB->sblock_list);
    PSCREENBLOCK pscreenblock_index=syscall_get_screenblock_by_id(IDS_INDEX,
                                                                  &m_EditorPCB->sblock_list);

    int32_t istart=pscreenblock_editor->offsety;
    int32_t iend=pscreenblock_editor->validline-1;
    int32_t itotalline=min((iend-istart+1),pscreenblock_editor->rect.height);
    char* allindex=syscall_malloc_block(itotalline*16);
    int i=0;
    for(i=0;i<itotalline;i++)
    {
        sprintf(ch,16,"%d\n",istart+i+1);
        strcat(allindex,ch);
    }
    syscall_clear_and_set_block(
                &m_EditorPCB->sblock_list,IDS_INDEX,allindex,COLOR_NORMAL);
    syscall_fresh_screen_block(pscreenblock_index);
}
static void fresh_statistics(void)
{
    char ch[16]={0};
    PSCREENBLOCK pscreenblock_editor=syscall_get_screenblock_by_id(IDS_EDITOR,
                                                                   &m_EditorPCB->sblock_list);
    sprintf(ch,16,"%d",pscreenblock_editor->words);
    syscall_clear_and_set_block(
                &m_EditorPCB->sblock_list,IDS_WORD,ch,COLOR_NORMAL);
    sprintf(ch,16,"%d",pscreenblock_editor->sections);
    syscall_clear_and_set_block(
                &m_EditorPCB->sblock_list,IDS_SECTION,ch,COLOR_NORMAL);
    sprintf(ch,16,"%d",pscreenblock_editor->validline);
    syscall_clear_and_set_block(
                &m_EditorPCB->sblock_list,IDS_LINE,ch,COLOR_NORMAL);
    sprintf(ch,16,"%d",pscreenblock_editor->characters);
    syscall_clear_and_set_block(
                &m_EditorPCB->sblock_list,IDS_CHAR,ch,COLOR_NORMAL);

}
static void fresh_clipboard(char* p)
{
    if(p)
    {
        syscall_clear_and_set_block(
                    &m_EditorPCB->sblock_list,IDS_CLIPBOARD,
                    p,COLOR_NORMAL);
    }
}

static void editor_read_file(void)
{
    char* pinputpath=m_EditorPCB->inputpath;
    int isize=strlen(pinputpath);
    if(isize)
    {
        file * pfileNode=syscall_open_path_file(pinputpath);
        syscall_add_path_to_pcb(m_EditorPCB,pinputpath);
        if(pfileNode&&pfileNode->pFile)
        {
            uint32_t isize=0;
            if(pfileNode->file_system==M_FAT32)
            {
                isize=((fat32_mem_node*)pfileNode->pFile)->entry.file_size;
            }
            else if(pfileNode->file_system==M_SMFS)
            {
                isize=((smfs_mem_node*)pfileNode->pFile)->disk_node.i_size;

            }
            char* pbuff=syscall_malloc_block(isize+1);

            syscall_read_file(pfileNode,0,pbuff,isize);
            uint32_t i=0;
            for(i=0;i<isize;i++)//调试二进制文件
            {
                uint8_t a=pbuff[i];
                if(a=='\0'||a==0)
                {
                    pbuff[i]='0';
                }
            }
            syscall_clear_and_set_block(
                        &m_EditorPCB->sblock_list,IDS_EDITOR,pbuff,COLOR_NORMAL);
            syscall_free_block(pbuff);
            syscall_close_file(pfileNode);
            fresh_index();
            fresh_statistics();
            fresh_path();
        }
        fresh_file();
    }
}
static void mf_editor_mouse(void *input, void **output,uint32_t config)
{
    UNUSED(output);
    uint16_t status=(uint16_t)((uint32_t)input>>16);
    uint8_t flag=status;
    flag^=status>>8;
    int8_t offsetx=(int8_t)(config>>8);
    int8_t offsety=(int8_t)config;
    if((offsetx||offsety))
    {
        if(status&CURSOR_LEFT_PRESS||!(status&0x08))
        {
            PCURSOR pcursor=syscall_get_cursor(CURSOR_LEFT);
            if(pcursor->pscreenblock)
            {
                if(pcursor->pscreenblock->self.iID==IDS_EDITOR)
                {
                    fresh_index();
                }
            }
        }
        else if(status&CURSOR_RIGHT_PRESS)
        {
            PCURSOR pcursor=syscall_get_cursor(CURSOR_RIGHT);
            if(pcursor->pscreenblock)
            {
                if(pcursor->pscreenblock->self.iID==IDS_EDITOR)
                {
                    fresh_index();
                }
            }
        }
    }
}

static void mf_editor_keyboard(void *input, void **output,uint32_t config)
{
    UNUSED(input);
    UNUSED(output);
    UNUSED(config);
    PCURSOR pcursor=syscall_get_cursor(CURSOR_LEFT);
    if(pcursor->pscreenblock)
    {
        if(pcursor->pscreenblock->self.iID==IDS_EDITOR)
        {
            fresh_index();
            fresh_statistics();
        }
    }
}

static void mf_editor_command_left(void *input, void **output,uint32_t config)
{
    UNUSED(input);
    UNUSED(output);
    UNUSED(config);
    if(config==IDS_OPEN)
    {
        PCURSOR pcursor=syscall_get_cursor(CURSOR_RIGHT);
        if(pcursor->pscreenblock)
        {
            if(pcursor->pscreenblock->self.iID==IDS_FILE)
            {

                char* p=syscall_get_buff_from_block_by_color(
                            pcursor->pscreenblock,COLOR_SELECT_RIGHT,SRCOR );
                if(p)
                {
                    char* p1=strrchr(m_EditorPCB->inputpath,'/');
                    if(p1)
                    {
                        p1++;
                        memset(p1,0,strlen(p1));
                    }
                    strcat(m_EditorPCB->inputpath,p);
                    editor_read_file( );
                    MESSAGE msg;
                    msg.iID=-msg_process_fresh;
                    msg.pInput=msg.pOutput=0;
                    msg.config=m_EditorPCB->self.iID;
                    syscall_post_msg_to_kcb(&msg,false);
                }
            }
        }
    }
    else  if(config==IDS_SAVE)
    {

        PSCREENBLOCK pscreenblock_editor=syscall_get_screenblock_by_id(IDS_EDITOR,
                                                                       &m_EditorPCB->sblock_list);
        PSCREENBLOCK pscreenblock_path=syscall_get_screenblock_by_id(IDS_PATH,
                                                                     &m_EditorPCB->sblock_list);

        char* ppath=syscall_get_buff_from_block_all(pscreenblock_path);
        char* pbuff=syscall_get_buff_from_block_all(pscreenblock_editor);
        int32_t isize=strlen(pbuff);
        file* pfilenode= syscall_open_path_file(ppath);
        if(pfilenode)
        {
            syscall_write_file(pfilenode,0,pbuff,isize);
            syscall_close_file(pfilenode);
        }
        syscall_free_block(ppath);
        syscall_free_block(pbuff);
    }
    else if(config==IDS_EDITOR_MENU)
    {
        char* pbutton=(char*)input;
        PCURSOR pcursor=syscall_get_cursor(CURSOR_RIGHT);
        if(pcursor->pscreenblock->self.iID==IDS_EDITOR)
        {
            if(!strcmp(pbutton,"COPY")||!strcmp(pbutton,"CUT"))
            {
                char* psrc=syscall_get_buff_from_block_by_color(
                            pcursor->pscreenblock,
                            COLOR_SELECT_RIGHT,
                            SRCOR );
                bool bCut=false;
                if(!strcmp(pbutton,"CUT"))
                {
                    bCut=true;
                    PSCREENBLOCK pscreenblock_editor=syscall_get_screenblock_by_id(IDS_EDITOR,
                                                         &m_EditorPCB->sblock_list);
                    syscall_delete_from_screen_block(pscreenblock_editor,
                                                     &pcursor->buffx1,&pcursor->buffy1,
                                                     pcursor->buffx2,pcursor->buffy2);
                    PCURSOR pcursorright=syscall_get_cursor(CURSOR_RIGHT);
                    if(pcursorright->pscreenblock==pscreenblock_editor)
                    {
                        syscall_clear_cursor(CURSOR_RIGHT);
                    }
                    syscall_clear_cursor(CURSOR_LEFT);
                    syscall_fresh_screen_block(pscreenblock_editor);

                    fresh_index();
                    fresh_statistics();

                }
                syscall_set_clipboard(psrc,strlen(psrc)+1,CLIP_STRING,bCut);
                fresh_clipboard(psrc);
                syscall_free_block(psrc);
            }
            else if(!strcmp(pbutton,"PASTE"))
            {
                PCLIPBOARD pclipboard=syscall_get_clipboard();
                if(pclipboard->type==CLIP_STRING)
                {
                    PSCREENBLOCK pscreenblock_editor=syscall_get_screenblock_by_id(IDS_EDITOR,
                                                              &m_EditorPCB->sblock_list);
                    if(pcursor->buffx1!=pcursor->buffx2||
                       pcursor->buffy1!=pcursor->buffy2)
                    {
                        syscall_delete_from_screen_block(pscreenblock_editor,
                                                         &pcursor->buffx1,&pcursor->buffy1,
                                                         pcursor->buffx2,pcursor->buffy2);

                    }
                    if(pclipboard->p)
                    {
                        syscall_insert_to_screen_block(pscreenblock_editor,
                                                       &pcursor->buffx1,&pcursor->buffy1,
                                                       pclipboard->p,COLOR_NORMAL);
                    }
                    syscall_fresh_screen_block(pscreenblock_editor);
                    fresh_index();
                    fresh_statistics();
                }
            }
        }
    }
}

static void mf_editor_command_right(void *input, void **output,uint32_t config)
{
    UNUSED(input);
    UNUSED(output);
    UNUSED(config);
    if(config==IDS_EDITOR)
    {
        syscall_clear_and_set_block(
                    &m_EditorPCB->sblock_list,IDS_EDITOR_MENU,
                    "COPY\nCUT\nPASTE",COLOR_NORMAL);
    }
    else
    {
        syscall_clear_and_set_block(
                    &m_EditorPCB->sblock_list,IDS_EDITOR_MENU,
                    0,COLOR_NORMAL);
    }
}

int main(int pPCB,char** input)
{
    UNUSED(input);
    m_EditorPCB=(PPCB)(uint32_t)pPCB;

    syscall_add_msgdesc_to_list(&m_EditorPCB->msgdesc_list,msg_mouse,
                                mf_editor_mouse,"editor_mouse");
    syscall_add_msgdesc_to_list(&m_EditorPCB->msgdesc_list,msg_keyboard,
                                mf_editor_keyboard,"editor_keyboard");
    syscall_add_msgdesc_to_list(&m_EditorPCB->msgdesc_list,msg_block_command_left,
                                mf_editor_command_left,"editor_command_left");
    syscall_add_msgdesc_to_list(&m_EditorPCB->msgdesc_list,msg_block_command_right,
                                mf_editor_command_right,"editor_command_right");

    int32_t width,height;
    syscall_get_screen_size(&width,&height);
    PSCREENBLOCK pscreenblock=syscall_malloc_screen_block(IDS_OPEN,
                                                          0, 2, 8, 1,
                                                          SBLOCK_INPUT,0,PALCEHOLD_CHAR);

    syscall_set_buff_to_block(pscreenblock,0,0,"OPEN",COLOR_NORMAL);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_PATH,
                                              9, 2, 60, 1,
                                              SBLOCK_INOUTPUT,0,PALCEHOLD_CHAR);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_SAVE,
                                              71, 2, 10, 1,
                                              SBLOCK_INPUT,0,PALCEHOLD_CHAR);

    syscall_set_buff_to_block(pscreenblock,0,0,"SAVE",COLOR_NORMAL);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    PFORMATBUFF pformatbuff=syscall_malloc_format_buff(16,20);
    pscreenblock= syscall_malloc_screen_block(IDS_FILE,
                                              0, 3, 8, 20,
                                              SBLOCK_INPUT,pformatbuff,PALCEHOLD_CHAR);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_INDEX,
                                              9, 3, 5, 15,
                                              SBLOCK_INPUT,0,PALCEHOLD_CHAR);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_EDITOR,
                                              15, 3, 55, 15,
                                              SBLOCK_INOUTPUT,0,PALCEHOLD_CHAR);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_CLIPBOARD,
                                              15, 19, 55, 4,
                                              SBLOCK_OUTPUT,0,PALCEHOLD_CHAR);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_STATIC,
                                              71, 3, 9, 1,
                                              SBLOCK_STATIC,0,PALCEHOLD_CHAR);

    syscall_set_buff_to_block(pscreenblock,0,0,"WORDS:",COLOR_NORMAL);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_WORD,
                                              71, 4, 9, 1,
                                              SBLOCK_OUTPUT,0,PALCEHOLD_CHAR);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_STATIC,
                                              71, 5, 9, 1,
                                              SBLOCK_STATIC,0,PALCEHOLD_CHAR);

    syscall_set_buff_to_block(pscreenblock,0,0,"SECTIONS:",COLOR_NORMAL);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_SECTION,
                                              71, 6, 9, 1,
                                              SBLOCK_OUTPUT,0,PALCEHOLD_CHAR);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_STATIC,
                                              71, 7, 9, 1,
                                              SBLOCK_STATIC,0,PALCEHOLD_CHAR);

    syscall_set_buff_to_block(pscreenblock,0,0,"LINES:",COLOR_NORMAL);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_LINE,
                                              71, 8, 9, 1,
                                              SBLOCK_OUTPUT,0,PALCEHOLD_CHAR);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_STATIC,
                                              71, 9, 9, 1,
                                              SBLOCK_STATIC,0,PALCEHOLD_CHAR);

    syscall_set_buff_to_block(pscreenblock,0,0,"CHARS:",COLOR_NORMAL);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_CHAR,
                                              71, 10, 9, 1,
                                              SBLOCK_OUTPUT,0,PALCEHOLD_CHAR);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    pscreenblock= syscall_malloc_screen_block(IDS_EDITOR_MENU,
                                              71, 12, 9, 5,
                                              SBLOCK_INPUT,0,PALCEHOLD_CHAR);
    list_append(&m_EditorPCB->sblock_list,&pscreenblock->self);

    if(input)
    {
        editor_read_file( );
    }
    PCLIPBOARD pclipboard=syscall_get_clipboard();
    if(pclipboard->type==CLIP_STRING&&pclipboard->p)
    {
        fresh_clipboard(pclipboard->p);
    }
    return 0;
}
