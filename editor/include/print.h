#ifndef __SMOS_PRINT_H
#define __SMOS_PRINT_H
#include "stdint.h" 
void print_str(int32_t iRow,int32_t iCol,char* message,int32_t istyle);
void print_wordchars(int32_t iRow,int32_t iCol,uint16_t* message,int32_t ilen);
void print_n_wordchar(int32_t iRow,int32_t iCol,uint16_t word_ch,int32_t ilen);
void get_wordchars(int32_t iRow,int32_t iCol,uint16_t* message,int32_t ilen);
void clear_screen(void);
#endif  //__SMOS_PRINT_H

