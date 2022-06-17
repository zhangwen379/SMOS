#ifndef __SMOS_PROCESS_H
#define __SMOS_PROCESS_H
#include "stdint.h"
#include "list.h"
#include "queue.h"
#include "interrupt_timer.h"
#include "memory.h"
#include "global_def.h"
struct _PCB;
typedef int (*init_main)(struct _PCB* ppcb,char** pinput);
typedef struct _PCB
{
    list_node self;
    char processpath[MAX_PATH_LEN];
    char inputpath[MAX_PATH_LEN];
    MSGQUEUE msgQueue;
    list msgdesc_list;
    uint32_t pgvdir;
    mem_block user_block_descs[BLOCK_STYLECNT];
    virtual_addr_desc user_vaddr_descs;
    list sblock_list;
    int32_t state;
    init_main main;
    bool bkernel;
    list path_list;
    TIMER timer[MAX_TIMER];
}PCB,*PPCB;

void create_process(init_main main, const char* pprocessPath, const char* pinputpath);
init_main load_process(const char* pathname);
void exit_process(int32_t ipid);
 #endif	//__SMOS_PROCESS_H
