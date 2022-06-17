#ifndef __SMOS_KERNEL_H
#define __SMOS_KERNEL_H
#include "stdint.h"
#include "list.h"
#include "process.h"
#include "queue.h"
#include "screen.h"
#include "msg_desc.h"
#include "path.h"
typedef struct _FILE_PROCESS_LINK
{
    list_node self;
    char* p_file_suffix;
    char* p_process_path;

}FILE_PROCESS_LINK,*PFILE_PROCESS_LINK;
typedef struct _mouse
{
    int16_t x;
    int16_t y;
    uint16_t coverchar;
    bool invisible;
}MOUSE,*PMOUSE;
typedef struct _cursor
{
    int32_t buffx1;
    int32_t buffy1;
    int32_t buffx2;
    int32_t buffy2;
    PSCREENBLOCK pscreenblock;
}CURSOR,*PCURSOR;
typedef struct _path_used
{
    char* path;
    list_node self;
}PATH_USED,*PPATH_USED;
typedef enum _cliptype
{
    CLIP_STRING,
    CLIP_DATA,
    CLIP_PATH,

}CLIPTYPE;
typedef struct _CLIPBOARD
{
    bool  bcut;
    CLIPTYPE type;
    void* p;
    int32_t isize;
}CLIPBOARD,*PCLIPBOARD;

typedef enum _CTRLID_KERNEL
{
    IDS_STATIC =0,
    IDS_PROCESS ,
    IDS_COMMAND,
    IDS_STATE ,
    IDS_INPUT,
    IDS_OUTPUT,
    IDS_CONTROL,
    IDS_EXIT,
    IDS_TIME,
    IDS_CAPSLOCK,
    IDS_INSERT,
    IDS_NUMSLOCK,
    IDS_SYSTEM_MAX,
}CTRLID_KERNEL;
typedef enum _MessageID_KERNEL
{
    msg_reserve=0,
    msg_keyboard,
    msg_mouse,
    msg_timer,
    msg_block_command_left,
    msg_block_command_right,
    msg_step,
    msg_process_init,
    msg_process_load,
    msg_process_exit,
    msg_process_fresh,
	msg_log,
    msg_max,
}MESSAGEID_KERNEL;
typedef enum _Cursor_Type
{
    CURSOR_LEFT,
    CURSOR_RIGHT,
}CURSORTYPE;

enum KERNEL_TIMER_ID
{
  TIMER_FRESHTIME=1,
};

typedef struct _KCB
{
    int32_t screen_width;
    int32_t screen_height;
    list process_list;
    MSGQUEUE msgQueue;
    list msgdesc_list;
    mem_block kernel_block_descs[BLOCK_STYLECNT];
    physics_addr_desc kernel_paddr_desc;
    physics_addr_desc user_paddr_desc;
    virtual_addr_desc kernel_vaddr_desc;
    MOUSE mouse;
    CURSOR cursor_left;
    CURSOR cursor_right;
    list sblock_list;
    PPCB pprocess_active;

    CLIPBOARD clipboard;
    list file_process_link_list;
	TIMER timer[MAX_TIMER];
}KCB,*PKCB;

void init_KCB(KCB *pKCB);
//消息处理上半部
void mf_system_mouse(void* input,void** output,uint32_t config);
void mf_system_keyboard(void* input,void** output,uint32_t config);
void mf_system_timer(void *input, void **output,uint32_t config);
//消息处理下半部
void mf_system_block_command_left(void* input,void** output,uint32_t config);
void mf_system_block_command_right(void* input,void** output,uint32_t config);
//进程消息处理
void mf_system_process_init(void* input,void** output,uint32_t config);
void mf_system_process_load(void* input,void** output,uint32_t config);
void mf_system_process_exit(void* input,void** output,uint32_t config);
void mf_system_process_fresh(void *input, void **output,uint32_t config);
//log消息处理
void mf_system_log(void *input, void **output,uint32_t config);
//设置鼠标显示
void set_mouse(int16_t xoffset, int16_t yoffset);
PMOUSE get_mouse(void);
//设置光标显示
void set_cursor(int32_t buffx1, int32_t buffy1, int32_t buffx2, int32_t buffy2 ,
                PSCREENBLOCK pscreenblock, bool bfresh,CURSORTYPE type);
void clear_cursor(CURSORTYPE type);
PCURSOR get_cursor(CURSORTYPE type);

char* get_system_command_str(mflags flag);
void fresh_system_sblock_pcb(void);
const char *get_processpath_by_suffix(char* psuffix);

void post_msg_to_kcb(PMESSAGE pmsg,bool bfront);
void post_msg_to_pcb(PMESSAGE pmsg,int32_t ipid,bool bfront);

void set_clipboard(void* p,int32_t isize,CLIPTYPE type,bool bcut);
PCLIPBOARD get_clipboard(void);

void set_state(int32_t iStateID,int32_t ipid);
void set_output(char* p);
void set_keyboard_indicate_screenblock(int32_t iIndicateID,bool bon);
PPCB get_current_process(void);
PPCB get_process(int32_t pid);

void add_path_to_pcb(PPCB ppcb,const char* path);
bool is_path_used_by_process(PPCB ppcb,const  char *path);
void clear_path_from_pcb(PPCB ppcb);

void write_log(char* p);
extern KCB m_gKCB;
#endif	//__SMOS_KERNEL_H
