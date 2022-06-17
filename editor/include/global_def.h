#ifndef __KERNEL_GLOBAL_DEF_H
#define __KERNEL_GLOBAL_DEF_H


#define PG_SIZE 4096

#define	 RPL0  0
#define	 RPL1  1
#define	 RPL2  2
#define	 RPL3  3

#define TI_GDT 0
#define TI_LDT 1

#define SELECTOR_CODE	   ((1 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_DATA	   ((2 << 3) + (TI_GDT << 2) + RPL0)
#define SELECTOR_STACK      SELECTOR_DATA
#define SELECTOR_GS        ((3 << 3) + (TI_GDT << 2) + RPL0)

#define EFLAGS_MBS	(1 << 1)	// 此项必须要设置
#define EFLAGS_IF_1	(1 << 9)	// if为1,开中断
#define EFLAGS_IF_0	0           // if为0,关中断
//#define EFLAGS_IOPL_0	(0 << 12)	// IOPL0

//--------------   中断描述表项属性  ------------
#define	 IDT_DESC_P         0b1
#define	 IDT_DESC_DPL0      0b00
#define	 IDT_DESC_S         0b0
#define	 IDT_DESC_D         0b1
#define	 IDT_DESC_TYPE      0b110   // 32位表项
#define	 IDT_DESC_RESERVE   0b00000000   // 32位表项
#define	 IDT_DESC_ATTR   ((((IDT_DESC_P << 7)+(IDT_DESC_DPL0 << 5)+\
                         (IDT_DESC_S<< 4)+(IDT_DESC_D << 3)+IDT_DESC_TYPE))<<8) \
                         +IDT_DESC_RESERVE;



#define MAX_TIMER	32
#define USER_VADDR_START 0x40000000// 0x8048000

#define STATE_START 0
#define STATE_STOP 1

#define MESSAGE_NAME_LEN 16
#define STATE_NAME_LEN 16

#define MAX_PATH_LEN 256	    // 路径最大长度
#define BLOCK_STYLECNT 7

typedef enum _FILE_TYPE
{
    FILE_ELF=1,
    FILE_BINARY,
    FILE_OTHER,
}FILE_TYPE;


#define keyboard_screenLock (1<<9)
#define keyboard_insert     (1<<8)
#define keyboard_capsLock   (1<<7)
#define keyboard_l_shift    (1<<6)
#define keyboard_l_ctrl     (1<<5)
#define keyboard_l_alt      (1<<4)
#define keyboard_r_shift    (1<<3)
#define keyboard_r_ctrl     (1<<2)
#define keyboard_r_alt      (1<<1)
#define keyboard_numsLock   (1)

#define CURSOR_LEFT_PRESS   0x1
#define CURSOR_RIGHT_PRESS   0x2
#define CURSOR_CENTER_PRESS   0x4

#define UNUSED(x) (void)(x)
#define NULL 0
#define ROUND_UP(X, STEP) ((X + STEP - 1) / (STEP))

#define max(x,y) ((x)>(y)?(x):(y))
#define min(x,y) ((x)<(y)?(x):(y))
#define abs(x) ((x)<0?-(x):(x))


#define sp_data(sp, t) *((t*)(sp))	  //堆栈值
//#define _DEBUG
#endif  //__KERNEL_GLOBAL_DEF_H
