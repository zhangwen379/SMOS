#include "syscall.h"
static uint32_t syscall(uint32_t funID,uint32_t argNum,...)
{
    UNUSED(funID);
    void* args=&argNum;
    args=(char*)args+4;
    uint32_t retval;
    asm volatile (
                "int $0x80"
                :"=a" (retval)
                : "a" (funID), "c" (argNum), "d" (args)
                );
    return retval;
}

//memory
void* syscall_malloc_block(uint32_t size)
{return (void*)syscall(MALLOC_BLOCK,2,M_USER,size);}
void syscall_free_block(void* ptr)
{syscall(FREE_BLOCK,1,ptr);}
//disk
list *syscall_get_partition_list(void)
{return (list*)syscall(GET_PARTITION_LIST,0);}
void syscall_init_ide_channel_desc(void)
{syscall(INIT_IDE_CHANNEL_DESC,0);}
void syscall_disk_read(disk_desc* hd, uint32_t lba, void* buf, uint16_t sec_cnt)
{syscall(DISK_READ,4,hd,lba,buf,sec_cnt);}
void syscall_disk_write(disk_desc* hd, uint32_t lba, void* buf, uint16_t sec_cnt)
{syscall(DISK_WRITE,4,hd,lba,buf,sec_cnt);}
bool syscall_partition_format_to_smfs(partition_desc* part)
{return (bool)syscall(PARTITION_FORMAT_TO_SMFS,1,part);}
partition_desc *syscall_get_partition_by_name(char *part_name)
{return (partition_desc *)syscall(GET_PARTITION_BY_NAME,1,part_name);}

//path
file *syscall_create_path_file(const char *pathname)
{return (file *)syscall(CREATE_PATH_FILE,1,pathname);}
file *syscall_open_path_file(const char* pathname)
{return (file *)syscall(OPEN_PATH_FILE,1,pathname);}
void syscall_del_path_file(const char* pathname)
{syscall(DEL_PATH_FILE,1,pathname);}
dir * syscall_create_path_dir(const char* pathname)
{return (dir *)syscall(CREATE_PATH_DIR,1,pathname);}
dir* syscall_open_path_dir(const char* pathname)
{return (dir*)syscall(OPEN_PATH_DIR,1,pathname);}
void syscall_del_path(const char *pathname)
{syscall(DEL_PATH,1,pathname);}
void syscall_copy_file(const char *pathname_src, const char *pathname_dst)
{syscall(COPY_FILE,2,pathname_src, pathname_dst);}
void syscall_copy_path(const char *pathname_src, const char *pathname_dst)
{syscall(COPY_PATH,2,pathname_src,pathname_dst);}
void syscall_rename_path(const char *pathname, char *pnewname)
{syscall(RENAME_PATH,2,pathname,pnewname);}
bool syscall_get_path_status(const char *path,path_status *status)
{return (bool)syscall(GET_PATH_STATUS,2,path,status);}
char *syscall_make_dir_string(const char *pathname)
{return (char *)syscall(MAKE_DIR_STRING,2,pathname,M_USER);}
char *syscall_make_file_string(const char *pathname,const char *file_type)
{return (char *)syscall(MAKE_FILE_STRING,3,pathname,M_USER,file_type);}

void syscall_close_file(file *pfile)
{syscall(CLOSE_FILE,1,pfile);}
void syscall_close_dir(dir *pdir)
{syscall(CLOSE_DIR,1,pdir);}
uint32_t syscall_write_file(file *pfile, uint32_t offset, void *buf, uint32_t isize)
{return syscall(WRITE_FILE,4,pfile,offset,buf,isize);}
uint32_t syscall_read_file(file *pfile, uint32_t offset, void *buf, uint32_t isize)
{return syscall(READ_FILE,4,pfile,offset,buf,isize);}

//screen
PFORMATBUFF syscall_malloc_format_buff(int32_t width, int32_t height)
{return (PFORMATBUFF)syscall(MALLOC_FORMAT_BUFF,3,width,height,M_USER);}
void syscall_free_format_buff(PFORMATBUFF pformatbuff)
{syscall(FREE_FORMAT_BUFF,1,pformatbuff);}
PSCREENBLOCK syscall_malloc_screen_block(int32_t id, int32_t x_screen, int32_t y_screen,
                                         int32_t width, int32_t height, int32_t type,
                                         PFORMATBUFF pformatbuff,
                                         char breakchar)
{return (PSCREENBLOCK)syscall(MALLOC_SCREEN_BLOCK,9,
                              id,   x_screen, y_screen, width,
                              height, type,
                              pformatbuff,
                              breakchar,M_USER);}
void syscall_free_screen_block(PSCREENBLOCK pscreenblock)
{syscall(FREE_SCREEN_BLOCK,1,pscreenblock);}
void syscall_clear_screen_block(PSCREENBLOCK pscreenblock)
{syscall(CLEAR_SCREEN_BLOCK,1,pscreenblock);}
void syscall_fresh_screen_block_rect(PSCREENBLOCK pscreenblock, int32_t x_block, int32_t y_block,
                                     int32_t width, int32_t height)
{syscall(FRESH_SCREEN_BLOCK_RECT,5,pscreenblock,   x_block,   y_block,
         width,   height);}
void syscall_fresh_screen_block(PSCREENBLOCK pscreenblock)
{syscall(FRESH_SCREEN_BLOCK,1,pscreenblock);}
void syscall_fresh_screen_block_pos(PSCREENBLOCK pscreenblock, int32_t x1_screen, int32_t y1_screen,
                                    int32_t x2_screen, int32_t y2_screen)
{syscall(FRESH_SCREEN_BLOCK_POS,5,pscreenblock,   x1_screen,   y1_screen,
         x2_screen,   y2_screen);}
void syscall_fresh_screen_block_list_rect(list *plist, int32_t x_screen, int32_t y_screen,
                                          int32_t width, int32_t height)
{syscall(FRESH_SCREEN_BLOCK_LIST_RECT,5, plist,  x_screen,   y_screen,
         width,   height);}
void syscall_fresh_screen_block_list(list *plist)
{syscall(FRESH_SCREEN_BLOCK_LIST,1,plist);}
uint16_t *syscall_buff_to_format_buff(char *p,  int32_t width, int32_t buffx, uint8_t color)
{return (uint16_t *)syscall(BUFF_TO_FORMAT_BUFF,5,
                            p,  width,  buffx, M_USER,   color);}

char *syscall_format_buff_to_buff(uint16_t *pformat, int32_t width,int32_t buffx )
{return (char *)syscall(FORMAT_BUFF_TO_BUFF,4, pformat,   width,  buffx, M_USER);}
char *syscall_get_buff_from_block_by_buffpos(PSCREENBLOCK pscreenblock,
                                             int32_t x1_buff, int32_t y1_buff,
                                             int32_t x2_buff, int32_t y2_buff)
{return (char *)syscall(GET_BUFF_FROM_BLOCK_BY_BUFFPOS,6,  pscreenblock,
                        x1_buff,   y1_buff,
                        x2_buff,   y2_buff,M_USER);}
char *syscall_get_buff_from_block_by_color(PSCREENBLOCK pscreenblock, uint8_t color,
                                           COLORRASTER raster)
{return (char *)syscall(GET_BUFF_FROM_BLOCK_BY_COLOR,4,  pscreenblock,   color,
                        raster, M_USER);}
char *syscall_get_buff_from_block_all(PSCREENBLOCK pscreenblock)
{return (char *)syscall(GET_BUFF_FROM_BLOCK_ALL,2,pscreenblock, M_USER);}
char *syscall_get_buff_from_block_by_ch(PSCREENBLOCK pscreenblock,
                                        int32_t x1_buff, int32_t y1_buff,
                                        int32_t x2_buff, int32_t y2_buff,
                                        char ch)
{return (char *)syscall(GET_BUFF_FROM_BLOCK_BY_CH,7, pscreenblock,
                        x1_buff,   y1_buff,
                        x2_buff,   y2_buff, ch,
                        M_USER);}
void syscall_set_buff_to_block(PSCREENBLOCK pscreenblock, int32_t x_buff, int32_t y_buff, char *p,
                               uint8_t color)
{syscall(SET_BUFF_TO_BLOCK,5,  pscreenblock,   x_buff,   y_buff, p,
         color);}
void syscall_insert_to_screen_block(PSCREENBLOCK pscreenblock, int32_t *px_buff, int32_t *py_buff,
                                    char *p, uint8_t color)
{syscall(INSERT_TO_SCREEN_BLOCK,5,  pscreenblock,  px_buff,  py_buff,
         p,   color);}
void syscall_appand_to_screen_block(PSCREENBLOCK pscreenblock, char *p, uint8_t color)
{syscall(APPAND_TO_SCREEN_BLOCK,3,  pscreenblock, p,   color);}
void syscall_delete_from_screen_block(PSCREENBLOCK pscreenblock, int32_t *px1_buff,int32_t *py1_buff,
                                      int32_t x2_buff, int32_t y2_buff)
{syscall(DELETE_FROM_SCREEN_BLOCK,5,  pscreenblock,   px1_buff, py1_buff,
         x2_buff,   y2_buff);}
void syscall_select_screen_block(PSCREENBLOCK pscreenblock, int32_t x1_buff, int32_t y1_buff,
                                 int32_t x2_buff, int32_t y2_buff ,uint8_t color, COLORRASTER raster)
{syscall(SELECT_SCREEN_BLOCK,7,  pscreenblock,   x1_buff,   y1_buff,
         x2_buff,   y2_buff ,  color,   raster);}
int32_t syscall_select_screen_block_by_ch(PSCREENBLOCK pscreenblock, int32_t *px1_buff, int32_t *py1_buff,
                                          int32_t *px2_buff, int32_t *py2_buff ,
                                          uint8_t color, COLORRASTER raster, char ch)
{return (int32_t)syscall(SELECT_SCREEN_BLOCK_BY_CH,8,  pscreenblock,    px1_buff,  py1_buff,
                         px2_buff,  py2_buff ,
                         color,   raster,   ch);}
void syscall_unselect_screen_block_by_color(PSCREENBLOCK pscreenblock, uint8_t color,COLORRASTER raster)
{syscall(UNSELECT_SCREEN_BLOCK_BY_COLOR,3,  pscreenblock,   color,  raster);}
void syscall_unselect_screen_block(PSCREENBLOCK pscreenblock, int32_t x1_buff, int32_t y1_buff,
                                   int32_t x2_buff, int32_t y2_buff, uint8_t color, COLORRASTER raster)
{syscall(UNSELECT_SCREEN_BLOCK,7,  pscreenblock,   x1_buff,   y1_buff,
         x2_buff,   y2_buff,   color,   raster);}
void syscall_reset_screen_block_offset(PSCREENBLOCK pscreenblock)
{syscall(RESET_SCREEN_BLOCK_OFFSET,1,pscreenblock);}
bool syscall_point_in_rect(PRECT prect, int32_t x, int32_t y)
{return (bool)syscall(POINT_IN_RECT,3,  prect,   x,   y);}
PSCREENBLOCK syscall_point_in_screenlock_list(list *pscreenblocklist, int32_t x_screen, int32_t y_screen,
                                              bool bforward)
{return (PSCREENBLOCK)syscall(POINT_IN_SCREENLOCK_LIST,4, pscreenblocklist,   x_screen,   y_screen,
                              bforward);}
void syscall_clear_and_set_block(list *psblocklist,int32_t blockID,char* p,uint8_t color)
{syscall(CLEAR_AND_SET_BLOCK,4, psblocklist,  blockID,  p, color);}
//process
PSCREENBLOCK syscall_get_screenblock_by_id(int32_t id,list *plist)
{return (PSCREENBLOCK)syscall(GET_SCREENBLOCK_BY_ID,2,  id, plist);}
void syscall_get_screen_size(int32_t *pwidth, int32_t *pheight)
{syscall(GET_SCREEN_SIZE,2, pwidth,  pheight);}
void syscall_add_msgdesc_to_list(list *listMsgDesc, int32_t iID, messageFunction function, char *name)
{syscall(ADD_MSGDESC_TO_LIST,4, listMsgDesc,   iID,   function, name);}
PMOUSE syscall_get_mouse(void)
{return (PMOUSE)syscall(GET_MOUSE,0);}
PCURSOR syscall_get_cursor(CURSORTYPE type)
{return (PCURSOR)syscall(GET_CURSOR,1,type);}
char *syscall_get_system_command_str(void)
{return (char *)syscall(GET_SYSTEM_COMMAND_STR,1,M_USER);}
void syscall_post_msg_to_kcb(PMESSAGE pmsg, bool bfront)
{syscall(POST_MSG_TO_KCB,2,pmsg,bfront);}
void syscall_post_msg_to_pcb(PMESSAGE pmsg, int32_t ipid, bool bfront)
{syscall(POST_MSG_TO_PCB,3,  pmsg,   ipid,   bfront);}
void syscall_set_clipboard(void *p, int32_t isize, CLIPTYPE type, bool bcut)
{syscall(SET_CLIPBOARD,4, p,   isize,   type,   bcut);}
PCLIPBOARD syscall_get_clipboard(void)
{return (PCLIPBOARD)syscall(GET_CLIPBOARD,0);}
void syscall_clear_cursor(CURSORTYPE type)
{syscall(CLEAR_CURSOR,1,type);}
void syscall_set_state(int32_t iStateID, int32_t ipid)
{syscall(SET_STATE,2,  iStateID,   ipid);}
void syscall_set_output(char *p)
{syscall(SET_OUTPUT,1,p);}
void syscall_add_path_to_pcb(PPCB ppcb,const  char *path)
{syscall(ADD_PATH_TO_PCB,2,  ppcb, path);}

void syscall_panic_reset(char* filename, int line, const char* func, const char* condition)
{syscall(PANIC_RESET,4, filename,   line,  func,  condition);}

//    asm volatile (
//       "int $0x80"
//       : "=a" (retval)
//       : "a" (funID), "c" (argNum), "d" (args)
//       : "memory"
//    );
