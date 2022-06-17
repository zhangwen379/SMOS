#ifndef __SMOS_INTERRUPT_H
#define __SMOS_INTERRUPT_H
#include "stdint.h"

//中断描述符结构体
struct idt_desc
{
    uint16_t    func_offset_low_word;
    uint16_t    selector;
    uint16_t    attribute;
    uint16_t    func_offset_high_word;
}__attribute__ ((packed));

typedef void* intr_handler;
void intrrupt_all_init(void);
void register_handler(uint8_t vector_no, intr_handler function);
void  sti(void) ;
void  cli(void) ;
void panic_reset(char* filename, int line, const char* func, const char* condition);
#endif  //__SMOS_INTERRUPT_H
