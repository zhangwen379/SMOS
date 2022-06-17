#ifndef _SMOS_ARRAY_STR_H
#define _SMOS_ARRAY_STR_H
#include "stdint.h"

int32_t atoi(char s[]);
int32_t itoa(int32_t n_, char s[], uint8_t base);
int32_t strlen(const char* s);
char* strcat(char* pre, const char* next);
void memset(void* dst_, uint8_t value, int32_t size);
void memcpy(void* dst_, const void* src_, int32_t size);
int memcmp(const void* a_, const void* b_, int32_t size);
char* strcpy(char* dst_, const char* src_);
int8_t strcmp (const char *a_, const char *b_);
int8_t strcmp1(const char *a_, const char *b_);
char* strchr(const char* string, const char ch);
char* strrchr(const char* string, const char ch);
void make_upper(char* s);
void make_lower(char* s);

void vsprintf(char* str, int32_t size, const char* format, char *sp);
void sprintf(char *str, int32_t size, const char* format, ...);
#endif //_SMOS_ARRAY_STR_H
