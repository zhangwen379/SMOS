#ifndef _SMOS_TRACE_H
#define _SMOS_TRACE_H
#include "stdint.h"

void trace(int32_t iline, const char* format, ...);
void panic(char* filename, int line, const char* func, const char* condition);

void console_output(const char* format, ...);

#define PANIC(s) panic(__FILE__, __LINE__, __func__, s)
#define PANIC1(f,...) do{char err[81]={0};sprintf(err,80,f,__VA_ARGS__);PANIC(err);}while(0);
#define ASSERT(CONDITION)  if (!(CONDITION)){PANIC(#CONDITION);}

#define TRACE(s) trace (23,s,0)
#define TRACE1(f,...) trace (23,f,__VA_ARGS__)
#define TRACE2(l,f,...) trace (l,f,__VA_ARGS__)

#endif //_SMOS_TRACE_H
