#ifndef __SMOS_INTERRUPT_TIME_H
#define __SMOS_INTERRUPT_TIME_H
#include "stdint.h"

typedef struct _tagTimer
{
    int32_t iID;
    int32_t iElapse;	//ms
    int32_t iCount;
}TIMER,*PTIMER;

typedef struct _tagMBIOSTime
{
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t second;
    uint8_t dayofweek;

}MBIOSTIME,*PMBIOSTIME;

void interrupt_timer_init(void);
uint32_t GetCounter(void);
void Sleep(uint32_t time);
void getBIOSTime(PMBIOSTIME pBIOSTime);
void setBIOSTime(PMBIOSTIME pBIOSTime);
void TRACEBIOSTime(PMBIOSTIME pBIOSTime);
void formatBiosTime(PMBIOSTIME pBIOSTime,char* s);

bool set_timer(int32_t id,int32_t iElapse,PTIMER ptimer);
void kill_timer(int32_t id,PTIMER ptimer);
#endif	//__SMOS_INTERRUPT_TIME_H

