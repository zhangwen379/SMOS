#include "array_str.h"
#include "trace.h"
//将十进制字符串转换成整形数据
int32_t atoi (char s[])
{
	int i=0,n,sign;
	while((s[i]<'0'||s[i]>'9')&&
		  s[i]!='+'&&s[i]!='-'){i++;}
	sign=(s[i]=='-')?-1:1;
	if(s[i]=='+'||s[i]=='-')
	{ i++;}
	for(n=0;s[i]>='0'&&s[i]<='9';i++)
		n=10*n+(s[i]-'0');
	return sign *n;
}
//将整形数据转换成数字字符
int32_t itoa(int32_t n, char s[], uint8_t base)
{ 
	if(base>16||!base) return 0;
	int32_t i,j,sign=0;
	uint32_t un=(uint32_t)n;
	if(n<0&&base==10){un=(uint32_t)(0-n);sign=-1;}
	i=0;
	do{
		uint8_t dig= un%base;
		if(dig<10)
		{
			s[i++]=dig+'0';
		}
		else
		{
			s[i++]=dig-10+'A';
		}
	}
	while (un/=base);
	if(sign<0)
	{
		s[i]='-';
		i++;
	}
	for(j=0;j<i/2;j++)
	{
		char temp=s[j];
		s[j]=s[i-1-j];
		s[i-1-j]=temp;
	}
	s[i]='\0';
	return i;
}

int32_t strlen(const char* s)
{
	int32_t i=0;
	while(s[i]){i++;}
	return i;
}

char* strcat(char* pre, const char* next)
{
	if (pre == 0 || next == 0) // 如果有一个为空指针，直接返回pre
		return pre;
	char* tmp_ptr = pre + strlen(pre);
	while ( (*tmp_ptr++ = *next++) != '\0'); // 依次接着赋值
	return pre;
}  

void memset(void* dst_, uint8_t value, int32_t size)
{
	uint8_t* dst = (uint8_t*)dst_;
	while (size-- > 0)
		*dst++ = value;
}


void memcpy(void* dst_, const void* src_, int32_t size)
{
	uint8_t* dst = dst_;
	const uint8_t* src = src_;
	while (size-- > 0)
		*dst++ = *src++;
}

int32_t memcmp(const void* a_, const void* b_, int32_t size)
{
	const uint8_t* a = a_;
	const uint8_t* b = b_;
	while (size-- > 0)
	{
		if(*a != *b)
		{
			return *a > *b ? 1 : -1;
		}
		a++;
		b++;
	}
	return 0;
}

char* strcpy(char* dst_, const char* src_)
{
	char* r = dst_;
	while((*dst_++ = *src_++));
	return r;
}


int8_t strcmp (const char* a_, const char* b_)
{
	const uint8_t* a = (uint8_t*)a_;
	const uint8_t* b = (uint8_t*)b_;
	while (*a != 0 && *a == *b) {
		a++;
		b++;
	}
	return *a < *b ? -1 : *a > *b;
}

int8_t strcmp1(const char *a_, const char *b_)
{
	const int8_t* a = (int8_t*)a_;
	const int8_t* b = (int8_t*)b_;
	while (*a != 0 && (*a == *b||abs(*a-*b)=='a'-'A'))
	{
		a++;
		b++;
	}
	return *a < *b ? -1 : *a > *b;
}

// 从左到右查找字符串str中首次出现字符ch的地址
char* strchr(const char* str, const char ch)
{
	while (*str != 0) {
		if (*str == ch) {
			return (char*)str;
		}
		str++;
	}
	return NULL;
}

// 从后往前查找字符串str中首次出现字符ch的地址
char* strrchr(const char* str, const char ch)
{
	const char* last_char = NULL;
	while (*str != 0) {
		if (*str == ch) {
			last_char = str;
		}
		str++;
	}
	return (char*)last_char;
}

void make_upper(char *s)
{
	int isize=strlen(s);
	int i=0;
	for(i=0;i<isize;i++)
	{
		if(s[i]>='a'&&s[i]<='z')
		{
			s[i]-='a'-'A';
		}
	}
}

void make_lower(char *s)
{
	int isize=strlen(s);
	int i=0;
	for(i=0;i<isize;i++)
	{
		if(s[i]>='A'&&s[i]<='Z')
		{
			s[i]+='a'-'A';
		}
	}
}

// 按照格式format输出到字符串str
void vsprintf(char* str,int32_t size, const char* format, char* sp)
{
	memset(str,0,size);
	int32_t sizeleft=size;
	char index_char = *format;
	while(index_char)
	{
		if (index_char != '%')
		{
			if(--sizeleft<=0){return;}
			*(str++) = index_char;
			index_char = *(++format);
			continue;
		}
		index_char = *(++format);	 // 得到%后面的字符
		char int_str[16]={0};
		char* arg_str=int_str;

		switch(index_char)
		{
			case 's': arg_str = sp_data(sp, char*); break;
			case 'c': arg_str[0] = sp_data(sp, char);break;
			case 'd': itoa(sp_data(sp, int), arg_str, 10); break;
			case 'x': itoa(sp_data(sp, int), arg_str, 16);break;
			case '0':
			{
				int8_t base=16;
				format++;
				char* pstr=strchr(format,'x');
				if(!pstr)
				{
					pstr=strchr(format,'d');
					base=10;
				}
				if(!pstr)return;
				int32_t icopysize=min(pstr-format,15);
				memcpy(arg_str,format,icopysize);
				int32_t reversecount=atoi(arg_str);
				reversecount=min(reversecount,15);
				memset(arg_str,0,16);

				int32_t numcount=itoa(sp_data(sp, int), arg_str, base);
				while(reversecount>numcount)
				{
					*str='0';str++;
					reversecount--;
				}
				format=pstr;
				break;
			}
		}
		int32_t ilen=strlen(arg_str);
		sizeleft-=ilen;
		if(sizeleft<=0){return;}
		strcpy(str, arg_str);
		str += ilen;
		index_char = *(++format);
		sp+=4;
	}
}

/* 格式化输出字符串format */
void sprintf(char* str,int size, const char* format, ...)
{
	char* sp=(char*)(&format)+4;
	vsprintf(str,size,format, sp);
}
