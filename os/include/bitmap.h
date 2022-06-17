#ifndef __SMOS_BITMAP_H
#define __SMOS_BITMAP_H
#include "stdint.h"
typedef struct _tagbitmap
{
	int32_t bytes_length;
	uint8_t* p_bytes;
}bitmap;

void bitmap_init(bitmap* pbtmp);
int32_t bitmap_scan(bitmap* pbtmp, int32_t count);
void bitmap_set(bitmap* pbtmp, int32_t idx, bool value);
#endif  //__SMOS_BITMAP_H
