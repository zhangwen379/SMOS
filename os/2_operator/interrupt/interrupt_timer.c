#include "interrupt_timer.h"
#include "array_str.h"
#include "interrupt_init.h" 
#include "queue.h"
#include "trace.h"
#include "io.h"
#include "kernel.h"
#include "process.h"
#define IC8253_OUT_FREQ             1000 //ms
#define IC8253_CLK_FREQ             1193180
#define IC8253_0_STARTDATA          (IC8253_CLK_FREQ/IC8253_OUT_FREQ)
#define IC8253_CTRL_PORT            0x43
#define IC8253_0_DATA_PORT          0x40
#define IC8253_1_DATA_PORT          0x41
#define IC8253_2_DATA_PORT          0x42


#define BIOSTIME_IO_COMMAND     0x70
#define BIOSTIME_IO_DATA        0x71
#define BIOSTIME_SECOND         0x00
#define BIOSTIME_MIN            0x02
#define BIOSTIME_HOUR           0x04
#define BIOSTIME_DAY_OF_WEEK    0x06
#define BIOSTIME_DAY            0x07
#define BIOSTIME_MONTH          0x08
#define BIOSTIME_YEAR           0x09

/////////////////////////////////////////////////////////// 初始化定时器8253芯片计时器
//初始化定时器8253定时器芯片
static void IC8253_counter_init(uint8_t counterID, uint8_t iworkMode, uint8_t iRWMode, bool bBCD,
                                uint16_t iStartValue)
{

    outb(IC8253_CTRL_PORT, (uint8_t)(counterID << 6 | iRWMode << 4 | iworkMode << 1)|bBCD);
    uint8_t dataport=IC8253_0_DATA_PORT;
    switch(counterID)
    {
    case 0:dataport=IC8253_0_DATA_PORT;break;
    case 1:dataport=IC8253_1_DATA_PORT;break;
    case 2:dataport=IC8253_2_DATA_PORT;break;
    default:ASSERT(0);break;
    }
    if(iRWMode!=2)
    {
        // 写入counter_value的低8位
        outb(dataport, (uint8_t)iStartValue);
    }
    if(iRWMode!=1)
    {
        // 写入counter_value的高8位
        outb(dataport, (uint8_t)(iStartValue >> 8));
    }
}

///////////////////////////////////////////////注册定时器中断处理函数，定义定时器中断处理函数
static uint32_t m_itimeCounter;
static void check_timer(PTIMER timer, PMSGQUEUE  pMsgQueue)
{
    int i=0;
    for(i=0;i<MAX_TIMER;i++)
    {
        if(timer[i].iElapse!=0)
        {
            timer[i].iCount-=1;
            if(timer[i].iCount<0)
            {
                timer[i].iCount=timer[i].iElapse;

                MESSAGE msg;
                memset(&msg,0,sizeof(MESSAGE));
                msg.iID=-msg_timer;
                msg.pInput=(void*)timer[i].iID;
                msgq_putmsg(pMsgQueue, msg);
            }
        }
    }
}

static void interrupt_timer_handler(void)
{
    m_itimeCounter+=1;

    check_timer(m_gKCB.timer, &m_gKCB.msgQueue);
    if(m_gKCB.pprocess_active)
    {
        check_timer(m_gKCB.pprocess_active->timer, &m_gKCB.pprocess_active->msgQueue);
    }
    //   list* plist=&m_gKCB.process_list;
    //   list_elem* pelem=plist->head.next;
    //   while(pelem!=&plist->tail)
    //   {
    //       if(pelem )
    //       {
    //           PPCB ppcb=elem2entry(PCB,self,pelem);
    //           check_timer(ppcb->timer,&ppcb->msgQueue);
    //       }
    //       pelem=pelem->next;
    //   }
}
void interrupt_timer_init()
{
    m_itimeCounter=0;
    //初始化8253定时器芯片0号计时器，并通过设定初始值1193
    //设置8253的0号计时器中断触发频率为1000（1s触发1000次）。
    IC8253_counter_init(0,2,3,false,IC8253_0_STARTDATA);
    register_handler(0x20, interrupt_timer_handler);
}
////////////////////////////////////////////////////////////////// 获取BIOS时间
static uint8_t get_BIOS_RAM(uint8_t command)
{
    outb(BIOSTIME_IO_COMMAND,command);
    uint16_t data=inb(BIOSTIME_IO_DATA);
    return ((data>>4)*10)+(data&0xf);
}
void getBIOSTime(PMBIOSTIME pBIOSTime)
{
    pBIOSTime->second=get_BIOS_RAM(BIOSTIME_SECOND);
    pBIOSTime->min=get_BIOS_RAM(BIOSTIME_MIN);
    pBIOSTime->hour=get_BIOS_RAM(BIOSTIME_HOUR);
    pBIOSTime->dayofweek=get_BIOS_RAM(BIOSTIME_DAY_OF_WEEK);
    pBIOSTime->day=get_BIOS_RAM(BIOSTIME_DAY);
    pBIOSTime->month=get_BIOS_RAM(BIOSTIME_MONTH);
    pBIOSTime->year=get_BIOS_RAM(BIOSTIME_YEAR);
}
static void set_BIOS_RAM(uint8_t command,uint8_t data)
{
    outb(BIOSTIME_IO_COMMAND,command);
    if(data<=100)
    {outb(BIOSTIME_IO_DATA,((data/10)<<4)+(data%10));}
}
void setBIOSTime(PMBIOSTIME pBIOSTime)
{
    set_BIOS_RAM(BIOSTIME_SECOND,pBIOSTime->second);
    set_BIOS_RAM(BIOSTIME_MIN,pBIOSTime->min);
    set_BIOS_RAM(BIOSTIME_HOUR,pBIOSTime->hour);
    set_BIOS_RAM(BIOSTIME_DAY_OF_WEEK,pBIOSTime->dayofweek);
    set_BIOS_RAM(BIOSTIME_DAY,pBIOSTime->day);
    set_BIOS_RAM(BIOSTIME_MONTH,pBIOSTime->month);
    set_BIOS_RAM(BIOSTIME_YEAR,pBIOSTime->year);
}
void formatBiosTime(PMBIOSTIME pBIOSTime, char *s)
{
    sprintf(s,32,"20%02d/%02d/%02d,%02d,%02d:%02d:%02d",pBIOSTime->year,
            pBIOSTime->month,pBIOSTime->day,
            pBIOSTime->dayofweek,
            pBIOSTime->hour,pBIOSTime->min,pBIOSTime->second);
}

bool bpos=false;
void TRACEBIOSTime(PMBIOSTIME pBIOSTime)
{
    char s[64]={0};
    formatBiosTime(  pBIOSTime,  s);
    if(bpos)
    {
        TRACE2(20,s,0);
    }
    else
    {
        TRACE2(21,s,0);
    }
    bpos=!bpos;
}

////////////////////////////////////////////////////////////////// 程序运行延时
uint32_t GetCounter(void)
{
    return m_itimeCounter;
}

void Sleep(uint32_t time)
{
    uint32_t timeCounter= GetCounter( );
    uint32_t count=0;
    do
    {
        if(timeCounter!=GetCounter())
        {
            timeCounter=GetCounter();
            count+=1;
        }
    } while(count<time);
}
////////////////////////////////////////////////////////////////// 内存定时器操作
bool set_timer(int32_t id, int32_t iElapse, PTIMER ptimer)
{
    int32_t i=0;
    for(i=0;i<MAX_TIMER;i++)
    {
        PTIMER temp=ptimer+i;
        if(temp->iElapse&&temp->iID==id)
        {
            temp->iElapse=iElapse;
            temp->iCount=0;
            return true;
        }
    }
    for(i=0;i<MAX_TIMER;i++)
    {
        PTIMER temp=ptimer+i;
        if(!temp->iElapse)
        {
            temp->iID=id;
            temp->iElapse=iElapse;
            temp->iCount=0;
            return true;
        }
    }
    return false;
}
void kill_timer(int32_t id, PTIMER ptimer)
{
    int32_t i=0;
    for(i=0;i<MAX_TIMER;i++)
    {
        PTIMER temp=ptimer+i;
        if(temp->iID==id)
        {
            temp->iID=0;
            temp->iElapse=0;
            temp->iCount=0;
            return ;
        }
    }
}
