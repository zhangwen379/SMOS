#ifndef __SMOS_IO_H
#define __SMOS_IO_H
#include "stdint.h"
//向端口写入一个字节
static inline void outb(uint16_t port, uint8_t data)
{
	if(port>=1024)return;
	asm volatile ( "outb %b0, %w1" : : "a" (data), "d" (port) );
}

//从端口读取一个字节
static inline uint8_t inb(uint16_t port)
{
	if(port>=1024) return 0;
	uint8_t data;
	asm volatile ("inb %w1, %b0" : "=a" (data) : "d" (port));
	return data;
}

//向端口连续写入n个字（双字节）
//先写入字的低字节(存储在低位)，再写入字的高字节(存储在高位)
//一次性写入双字节，节省时间
static inline void rep_outsw(uint16_t port, const void* addr, uint32_t word_cnt)
{
	if(port>=1024)return;
	asm volatile ("cld;rep outsw" :: "S" (addr),"c" (word_cnt), "d" (port)); //
}

//从端口连续读入n个字（双字节）
//先读取字的低字节(存储在低位)，再读取字的高字节(存储在高位)
//一次性读取双字节，节省时间
static inline void rep_insw(uint16_t port, void* addr, uint32_t word_cnt)
{
	if(port>=1024)return;
	asm volatile ("cld; rep insw" :: "D" (addr), "c" (word_cnt),"d" (port));
}
#endif //__SMOS_IO_H
