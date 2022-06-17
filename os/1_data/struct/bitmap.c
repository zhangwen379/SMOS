#include "bitmap.h"
#include "array_str.h"
#include "trace.h"

// 将位图初始化
void bitmap_init(bitmap* pbtmp)
{
	memset(pbtmp->p_bytes, 0, pbtmp->bytes_length);
}

// 判断bit位是否为1,若为1则返回true，否则返回false
static bool bitmap_scan_test(bitmap* btmp, int32_t idx)
{
	int32_t byte_idx = idx / 8;
	int32_t bit_idx  = 7-idx % 8;
	return (btmp->p_bytes[byte_idx] & (1 << bit_idx));
}

// 在位图中申请连续count个位,返回其起始位序号
int32_t bitmap_scan(bitmap* pbtmp, int32_t count)
{
	int32_t start_index=0;
	int32_t i=0;
	for(;i<pbtmp->bytes_length;i++)
	{
		// 若为0xff,则表示该字节内无空闲位，快速跳过已标记字节
		if(0xff == pbtmp->p_bytes[i])continue;
		int32_t j=i*8;
		bool teststart=true;
		for(;j<pbtmp->bytes_length*8;j++)
		{
			bool flag= bitmap_scan_test(pbtmp, j);
			if(!flag)
			{
				if(teststart)
				{
					start_index=j;
					teststart=false;
					if(count==1)
					{
						return start_index;
					}
				}
				else
				{
					if(j-start_index+1>=count)
					{
						return start_index;
					}
				}
			}
			else
			{
				if((j+1)%8)	//j%7||j==0
				{
					teststart=true;
				}
				else
				{
					i=j/8;
					break;
				}
			}
		}
	}
	//查找失败
	return -1;
}


void bitmap_set(bitmap* pbtmp, int32_t idx, bool value)
{
	int32_t byte_idx = idx / 8;
	int32_t bit_idx  = 7-idx % 8;
	if (value)
	{
		pbtmp->p_bytes[byte_idx] |= (1 << bit_idx);
	}
	else
	{
		pbtmp->p_bytes[byte_idx] &= ~(1 << bit_idx);
	}
}

