#include "print.h"
#include "interrupt_init.h"
#include "interrupt_keyboard.h"
#include "interrupt_timer.h" 
#include "queue.h"
#include "trace.h"
#include "array_str.h" 
#include "memory.h"
#include "process.h"
#include "list.h"
#include "disk.h"
#include "path.h"
#include "kernel.h"
#include "explore.h"
#include "syscall.h"
#include "screen.h"
#include "global_def.h"
#include "fat32_path.h"
#include "smfs_path.h"
int main(void)
{
    /*	uint8_t data[3]={0x08,0xFF,0x00};
    console_output("%02x,%02x,%02x",data[0],data[1],data[2]);
    bitmap btmp;
    btmp.bytes_length=3;
    btmp.p_bytes=data;
    int32_t i=0;
    int32_t j=0;
    int32_t idx=-1;
    for(i=2;i<5;i++)
    {
        idx= bitmap_scan(&btmp,i);
        if(idx!=-1)
        {
            for(j=0;j<i;j++)
            {
                bitmap_set(&btmp,  idx+j, 1);
            }
        }
        console_output("%02x,%02x,%02x",data[0],data[1],data[2]);
    }
    if(idx!=-1)
    {
        for(j=0;j<i;j++)
        {
            bitmap_set(&btmp,  idx+j, 0);
        }
    }
    console_output("%02x,%02x,%02x",data[0],data[1],data[2]);
    //while(1);
*/
    fresh_pageTable_cr3( );
    init_KCB(&m_gKCB);
    /*
 //   while(1);
    void* p16= malloc_block(M_KERNEL,16);
//    TRACE1("p16:0x%x",(uint32_t)p16);while(1);
    void* p16_1= malloc_block(M_KERNEL,16);
//    TRACE1("p16_1:0x%x",(uint32_t)p16_1);while(1);
    void* p1024= malloc_block(M_KERNEL,1024);
//    TRACE1("p1024:0x%x",(uint32_t)p1024);while(1);
    void* p2048= malloc_block(M_KERNEL,2048);
//    TRACE1("p2048:0x%x",(uint32_t)p2048);while(1);

    free_block(p16); //while(1);
    free_block(p16_1);//while(1);
    free_block(p1024);//while(1);
    free_block(p2048); //while(1);

*/
    init_ide_channel_desc();
    /*     char * filenamelist= make_dir_string_fat32("H:",M_KERNEL,0);
    console_output(filenamelist);
    free_block(filenamelist);
    fat32_mem_node* pFileFat32= create_path_fat32("H:/ABC/DEF.txt",
                                                  FT_REGULAR);
    if(!pFileFat32)
    {
        pFileFat32=open_path_fat32("H:/ABC/DEF.txt",
                                   FT_REGULAR);
    }
    int32_t count=512*8+10;
    char* buf=malloc_block(M_KERNEL,count);
    int i;
    for(i=0;i<4096;i++)
    {
        buf[i]='a';
    }
    for(i=4096;i<count;i++)
    {
        buf[i]='b';
    }
    write_fat32_to_disk(pFileFat32,0,buf,count);

    char* buf1=malloc_block(M_KERNEL,81);
    read_fat32_from_disk(pFileFat32,4026,buf1,80);
    console_output(buf1);
    free_block(buf);
    free_block(buf1);
    close_fat32_mem_node(pFileFat32);
  // while(1);
*/
/*
    smfs_mem_node* pFileSmfs= create_path_smfs("G:/ABC.txt",
                                                  FT_REGULAR);
    if(!pFileSmfs)
    {
        pFileSmfs=open_path_smfs("G:/ABC.txt",
                                  FT_REGULAR);
    }
    int32_t count=512*(512/4+12)+10;
    char* buf=malloc_block(M_KERNEL,count);
    int i;
    for(i=0;i<count-10;i++)
    {
        buf[i]='a';
    }
    for(i=count-10;i<count;i++)
    {
        buf[i]='b';
    }
    write_smfs_to_disk(pFileSmfs,0,buf,count);

    char* buf1=malloc_block(M_KERNEL,81);
    read_smfs_from_disk(pFileSmfs,count-80,buf1,80);
    console_output(buf1);
    free_block(buf);
    free_block(buf1);
    close_smfs_mem_node(pFileSmfs);
    while(1);
 */
    intrrupt_all_init();
    clear_screen();

    screen_width=m_gKCB.screen_width;
    screen_height=m_gKCB.screen_height;
    create_process(explore_main,"explore",0);

#ifdef _DEBUG
    file* pfilenode= create_path_file("C:/editor.bin");
    if(!pfilenode)
    {
        pfilenode= open_path_file("C:/editor.bin");

    }
    uint32_t file_size =512*99 ;//4816;
    uint32_t sec_cnt = ROUND_UP(file_size, 512);
    disk_desc* sda = &channels[0].devices[1];
    void* prog_buf = malloc_block(M_KERNEL,file_size);
    disk_read(sda, 300, prog_buf, sec_cnt);
    write_file(pfilenode,0,prog_buf,file_size);
    free_block(prog_buf);
    close_file(pfilenode);
#endif
    do_Msg();
    return 0;
}
