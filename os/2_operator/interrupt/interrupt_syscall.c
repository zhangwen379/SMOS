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
#define syscall_num 200
void* syscall_table[syscall_num];

void interrupt_syscall_init(void)
{
    //memory
    syscall_table[MALLOC_BLOCK]=malloc_block;
    syscall_table[FREE_BLOCK]=free_block;
    //disk
    syscall_table[GET_PARTITION_LIST]=get_partition_list;
    syscall_table[INIT_IDE_CHANNEL_DESC]=init_ide_channel_desc;
    syscall_table[DISK_READ]=disk_read;
    syscall_table[DISK_WRITE]=disk_write;
    syscall_table[PARTITION_FORMAT_TO_SMFS]=partition_format_to_smfs;
    syscall_table[GET_PARTITION_BY_NAME]=get_partition_by_name;

    //path
   syscall_table[CREATE_PATH_FILE]=create_path_file;
   syscall_table[OPEN_PATH_FILE]=open_path_file;
   syscall_table[DEL_PATH_FILE]=del_path_file;
   syscall_table[CREATE_PATH_DIR]=create_path_dir;
   syscall_table[OPEN_PATH_DIR]=open_path_dir;
   syscall_table[DEL_PATH]=del_path;
   syscall_table[COPY_FILE]=copy_file;
   syscall_table[COPY_PATH]=copy_path;
   syscall_table[RENAME_PATH]=rename_path;
   syscall_table[GET_PATH_STATUS]=get_path_status;
   syscall_table[MAKE_DIR_STRING]=make_dir_string;
   syscall_table[MAKE_FILE_STRING]=make_file_string;


   syscall_table[CLOSE_FILE]=close_file;
   syscall_table[CLOSE_DIR]=close_dir;
   syscall_table[WRITE_FILE]=write_file;
   syscall_table[READ_FILE]=read_file;

   //screen
   syscall_table[MALLOC_FORMAT_BUFF]=malloc_format_buff;
   syscall_table[FREE_FORMAT_BUFF]=free_format_buff;
   syscall_table[MALLOC_SCREEN_BLOCK]=malloc_screen_block;
   syscall_table[FREE_SCREEN_BLOCK]=free_screen_block;
   syscall_table[CLEAR_SCREEN_BLOCK]=clear_screen_block;
   syscall_table[FRESH_SCREEN_BLOCK_RECT]=fresh_screen_block_rect;
   syscall_table[FRESH_SCREEN_BLOCK]=fresh_screen_block;
   syscall_table[FRESH_SCREEN_BLOCK_POS]=fresh_screen_block_pos;
   syscall_table[FRESH_SCREEN_BLOCK_LIST_RECT]=fresh_screen_block_list_rect;
   syscall_table[FRESH_SCREEN_BLOCK_LIST]=fresh_screen_block_list;
   syscall_table[BUFF_TO_FORMAT_BUFF]=buff_to_format_buff;
   syscall_table[FORMAT_BUFF_TO_BUFF]=format_buff_to_buff;
   syscall_table[GET_BUFF_FROM_BLOCK_BY_BUFFPOS]=get_buff_from_block_by_buffpos;
   syscall_table[GET_BUFF_FROM_BLOCK_BY_COLOR]=get_buff_from_block_by_color;
   syscall_table[GET_BUFF_FROM_BLOCK_ALL]=get_buff_from_block_all;
   syscall_table[GET_BUFF_FROM_BLOCK_BY_CH]=get_buff_from_block_by_ch;
   syscall_table[SET_BUFF_TO_BLOCK]=set_buff_to_block;
   syscall_table[INSERT_TO_SCREEN_BLOCK]=insert_to_screen_block;
   syscall_table[APPAND_TO_SCREEN_BLOCK]=appand_to_screen_block;
   syscall_table[DELETE_FROM_SCREEN_BLOCK]=delete_from_screen_block;
   syscall_table[SELECT_SCREEN_BLOCK]=select_screen_block;
   syscall_table[SELECT_SCREEN_BLOCK_BY_CH]=select_screen_block_by_ch;
   syscall_table[UNSELECT_SCREEN_BLOCK_BY_COLOR]=unselect_screen_block_by_color;
   syscall_table[UNSELECT_SCREEN_BLOCK]=unselect_screen_block;
   syscall_table[RESET_SCREEN_BLOCK_OFFSET]=reset_screen_block_offset;
   syscall_table[POINT_IN_RECT]=point_in_rect;
   syscall_table[POINT_IN_SCREENLOCK_LIST]=point_in_screenlock_list;
   syscall_table[CLEAR_AND_SET_BLOCK]=clear_and_set_block;
    //process
   syscall_table[GET_SCREENBLOCK_BY_ID]=get_screenblock_by_id;
   syscall_table[GET_SCREEN_SIZE]=get_screen_size;
   syscall_table[ADD_MSGDESC_TO_LIST]=add_msgdesc_to_list;
   syscall_table[GET_MOUSE]=get_mouse;
   syscall_table[GET_CURSOR]=get_cursor;
   syscall_table[GET_SYSTEM_COMMAND_STR]=get_system_command_str;
   syscall_table[POST_MSG_TO_KCB]=post_msg_to_kcb;
   syscall_table[POST_MSG_TO_PCB]=post_msg_to_pcb;
   syscall_table[SET_CLIPBOARD]=set_clipboard;
   syscall_table[GET_CLIPBOARD]=get_clipboard;
   syscall_table[CLEAR_CURSOR]=clear_cursor;
   syscall_table[SET_STATE]=set_state;
   syscall_table[SET_OUTPUT]=set_output;
   syscall_table[ADD_PATH_TO_PCB]=add_path_to_pcb;

   syscall_table[PANIC_RESET]=panic_reset;
}















