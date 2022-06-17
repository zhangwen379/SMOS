#ifndef __SMOS_SYSCALL_H
#define __SMOS_SYSCALL_H
#include "stdint.h"
#include "interrupt_syscall.h"
#include "trace.h"
#include "syscall.h"
#include "memory.h"
#include "disk.h"
#include "path.h"
#include "screen.h"
#include "kernel.h"
#include "process.h"
#include "interrupt_init.h"

enum SYSCALL_ID
{
    //memory
    MALLOC_BLOCK,
    FREE_BLOCK,
    //disk
    GET_PARTITION_LIST,
    INIT_IDE_CHANNEL_DESC ,
    DISK_READ ,
    DISK_WRITE ,
    PARTITION_FORMAT_TO_SMFS ,
    GET_PARTITION_BY_NAME ,
    //path
    CREATE_PATH_FILE,
    OPEN_PATH_FILE,
    DEL_PATH_FILE,
    CREATE_PATH_DIR,
    OPEN_PATH_DIR,
    DEL_PATH,
    COPY_FILE,
    COPY_PATH,
    RENAME_PATH,
    GET_PATH_STATUS,
    MAKE_DIR_STRING,
    MAKE_FILE_STRING,

    CLOSE_FILE,
    CLOSE_DIR,
    WRITE_FILE,
    READ_FILE,
    //screen
    MALLOC_FORMAT_BUFF,
    FREE_FORMAT_BUFF,
    MALLOC_SCREEN_BLOCK,
    FREE_SCREEN_BLOCK,
    CLEAR_SCREEN_BLOCK,
    FRESH_SCREEN_BLOCK_RECT,
    FRESH_SCREEN_BLOCK,
    FRESH_SCREEN_BLOCK_POS,
    FRESH_SCREEN_BLOCK_LIST_RECT,
    FRESH_SCREEN_BLOCK_LIST,
    BUFF_TO_FORMAT_BUFF,
    FORMAT_BUFF_TO_BUFF,
    GET_BUFF_FROM_BLOCK_BY_BUFFPOS,
    GET_BUFF_FROM_BLOCK_BY_COLOR,
    GET_BUFF_FROM_BLOCK_ALL,
    GET_BUFF_FROM_BLOCK_BY_CH,
    SET_BUFF_TO_BLOCK,
    INSERT_TO_SCREEN_BLOCK,
    APPAND_TO_SCREEN_BLOCK,
    DELETE_FROM_SCREEN_BLOCK,
    SELECT_SCREEN_BLOCK,
    SELECT_SCREEN_BLOCK_BY_CH,
    UNSELECT_SCREEN_BLOCK_BY_COLOR,
    UNSELECT_SCREEN_BLOCK,
    RESET_SCREEN_BLOCK_OFFSET,
    POINT_IN_RECT,
    POINT_IN_SCREENLOCK_LIST,
    SCREENPOS_TO_BUFFPOS,
    BUFFPOS_TO_SCREENPOS,
    CLEAR_AND_SET_BLOCK,
    CLEAR_AND_FRESH_BLOCK,
    //process
    GET_SCREENBLOCK_BY_ID,
    GET_SCREEN_SIZE,
    ADD_MSGDESC_TO_LIST,
    GET_MOUSE,
    GET_CURSOR,
    GET_SYSTEM_COMMAND_STR,
    POST_MSG_TO_KCB,
    POST_MSG_TO_PCB,
    SET_CLIPBOARD,
    GET_CLIPBOARD,
    CLEAR_CURSOR,
    SET_STATE,
    SET_OUTPUT,
    ADD_PATH_TO_PCB,
    PANIC_RESET,
};

//memory
void* syscall_malloc_block(uint32_t size);
void syscall_free_block(void* ptr);
//disk
list *syscall_get_partition_list(void);
void syscall_init_ide_channel_desc(void);
void syscall_disk_read(disk_desc* hd, uint32_t lba, void* buf, uint16_t sec_cnt);
void syscall_disk_write(disk_desc* hd, uint32_t lba, void* buf, uint16_t sec_cnt);
bool syscall_partition_format_to_smfs(partition_desc* part);
partition_desc *syscall_get_partition_by_name(char *part_name);

//path
file *syscall_create_path_file(const char *pathname);
file *syscall_open_path_file(const char* pathname);
void syscall_del_path_file(const char* pathname);
dir * syscall_create_path_dir(const char* pathname);
dir* syscall_open_path_dir(const char* pathname);
void syscall_del_path(const char *pathname);
void syscall_copy_file(const char *pathname_src, const char *pathname_dst);
void syscall_copy_path(const char *pathname_src, const char *pathname_dst);
void syscall_rename_path(const char *pathname, char *pnewname);
bool syscall_get_path_status(const char *path,path_status *status);
char *syscall_make_dir_string(const char *pathname);
char *syscall_make_file_string(const char *pathname,const char *file_type);


void syscall_close_file(file *pfile);
void syscall_close_dir(dir *pdir);
uint32_t syscall_write_file(file *pfile, uint32_t offset, void *buf, uint32_t isize);
uint32_t syscall_read_file(file *pfile, uint32_t offset, void *buf, uint32_t isize);

//screen
PFORMATBUFF syscall_malloc_format_buff(int32_t width, int32_t height);
void syscall_free_format_buff(PFORMATBUFF pformatbuff);
PSCREENBLOCK syscall_malloc_screen_block(int32_t id, int32_t x_screen, int32_t y_screen,
                                 int32_t width, int32_t height, int32_t type, PFORMATBUFF pformatbuff, char breakchar);
void syscall_free_screen_block(PSCREENBLOCK pscreenblock);
void syscall_clear_screen_block(PSCREENBLOCK pscreenblock);
void syscall_fresh_screen_block_rect(PSCREENBLOCK pscreenblock, int32_t x_block, int32_t y_block,
                             int32_t width, int32_t height);
void syscall_fresh_screen_block(PSCREENBLOCK pscreenblock);
void syscall_fresh_screen_block_pos(PSCREENBLOCK pscreenblock, int32_t x1_screen, int32_t y1_screen,
                            int32_t x2_screen, int32_t y2_screen);
void syscall_fresh_screen_block_list_rect(list *plist, int32_t x_screen, int32_t y_screen,
                                  int32_t width, int32_t height);
void syscall_fresh_screen_block_list(list *plist);
uint16_t *syscall_buff_to_format_buff(char *p,  int32_t width, int32_t buffx, uint8_t color);

char *syscall_format_buff_to_buff(uint16_t *pformat, int32_t width,int32_t buffx );
char *syscall_get_buff_from_block_by_buffpos(PSCREENBLOCK pscreenblock,
                                     int32_t x1_buff, int32_t y1_buff,
                                     int32_t x2_buff, int32_t y2_buff);
char *syscall_get_buff_from_block_by_color(PSCREENBLOCK pscreenblock, uint8_t color,
                                   COLORRASTER raster);
char *syscall_get_buff_from_block_all(PSCREENBLOCK pscreenblock);
char *syscall_get_buff_from_block_by_ch(PSCREENBLOCK pscreenblock,
                                int32_t x1_buff, int32_t y1_buff,
                                int32_t x2_buff, int32_t y2_buff,
                                char ch);
void syscall_set_buff_to_block(PSCREENBLOCK pscreenblock, int32_t x_buff, int32_t y_buff, char *p,
                       uint8_t color);
void syscall_insert_to_screen_block(PSCREENBLOCK pscreenblock, int32_t *px_buff, int32_t *py_buff,
                            char *p, uint8_t color);
void syscall_appand_to_screen_block(PSCREENBLOCK pscreenblock, char *p, uint8_t color);
void syscall_delete_from_screen_block(PSCREENBLOCK pscreenblock, int32_t *px1_buff,int32_t *py1_buff,
                              int32_t x2_buff, int32_t y2_buff);
void syscall_select_screen_block(PSCREENBLOCK pscreenblock, int32_t x1_buff, int32_t y1_buff,
                         int32_t x2_buff, int32_t y2_buff ,uint8_t color, COLORRASTER raster);
int32_t syscall_select_screen_block_by_ch(PSCREENBLOCK pscreenblock, int32_t *px1_buff, int32_t *py1_buff,
                                  int32_t *px2_buff, int32_t *py2_buff ,
                                  uint8_t color, COLORRASTER raster, char ch);
void syscall_unselect_screen_block_by_color(PSCREENBLOCK pscreenblock, uint8_t color,COLORRASTER raster);
void syscall_unselect_screen_block(PSCREENBLOCK pscreenblock, int32_t x1_buff, int32_t y1_buff,
                           int32_t x2_buff, int32_t y2_buff, uint8_t color, COLORRASTER raster);
void syscall_reset_screen_block_offset(PSCREENBLOCK pscreenblock);
bool syscall_point_in_rect(PRECT prect, int32_t x, int32_t y);
PSCREENBLOCK syscall_point_in_screenlock_list(list *pscreenblocklist, int32_t x_screen, int32_t y_screen,
                                      bool bforward);
void syscall_clear_and_set_block(list *psblocklist,int32_t blockID,char* p,uint8_t color);
 //process
PSCREENBLOCK syscall_get_screenblock_by_id(int32_t id,list *plist);
void syscall_get_screen_size(int32_t *pwidth, int32_t *pheight);
void syscall_add_msgdesc_to_list(list *listMsgDesc, int32_t iID, messageFunction function, char *name);
PMOUSE syscall_get_mouse(void);
PCURSOR syscall_get_cursor(CURSORTYPE type);
char *syscall_get_system_command_str(void);
void syscall_post_msg_to_kcb(PMESSAGE pmsg, bool bfront);
void syscall_post_msg_to_pcb(PMESSAGE pmsg, int32_t ipid, bool bfront);
void syscall_set_clipboard(void *p, int32_t isize, CLIPTYPE type, bool bcut);
PCLIPBOARD syscall_get_clipboard(void);
void syscall_clear_cursor(CURSORTYPE type);
void syscall_set_state(int32_t iStateID, int32_t ipid);
void syscall_set_output(char *p);
void syscall_add_path_to_pcb(PPCB ppcb,const  char *path);

void syscall_panic_reset(char* filename, int line, const char* func, const char* condition);

#endif //__SMOS_SYSCALL_H

