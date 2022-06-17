#include "trace.h"
#include "print.h"
#include "array_str.h"
#include "syscall.h"
void trace(int32_t iline,const char *format, ...)
{
    if(!format)return;
	char str[81]={0};
	char* sp=(char*)(&format)+4;
	vsprintf(str,80, format, sp);
	int i=0;
	for(i=0;i<80;i++)
	{
		if(!str[i]) str[i]=' ';
	}
    print_str(iline,0,str,0xf);
}

void panic(char* filename,int line,const char* func,const char* condition)
{
#ifdef _DEBUG
    trace(24,"PANIC:%s;%s;%d;%s",filename,func,line,condition);
    while(1);
#else
    syscall_panic_reset(filename, line,func,condition);
#endif
}

void console_output(const char *format, ...)
{
    if(!format)return;
	static  int32_t console_line=6;//(*(uint8_t*)(0xb04));
	if(console_line>24){console_line=24;}
	char str[81]={0};
	char* sp=(char*)(&format)+4;
    vsprintf(str,81, format, sp);
	int i=0;
	for(i=0;i<80;i++)
	{
		if(!str[i]) str[i]=' ';
	}
	print_str(console_line,0,str,0xf);
	console_line++;
	console_line=console_line%25;
}
