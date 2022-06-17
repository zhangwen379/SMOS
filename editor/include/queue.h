#ifndef __SMOS_QUEUE_H
#define __SMOS_QUEUE_H
#include "stdint.h"
#include "list.h"

#define QUEUESIZE 256
/* 环形队列 */
typedef struct _Message
{
    int32_t iID;
    void* pInput;
    void** pOutput;
    uint32_t config;
}MESSAGE,*PMESSAGE;
typedef struct _Queue
{
    MESSAGE msgVector[QUEUESIZE];
	int32_t take;
	int32_t  fill;
}MSGQUEUE,*PMSGQUEUE;

void msgq_init(  MSGQUEUE* q);
bool msgq_isfull(  MSGQUEUE* q);
bool msgq_isempty(  MSGQUEUE* q);
int32_t msgq_prev_pos(int32_t pos);
int32_t msgq_next_pos(int32_t pos);
MESSAGE msgq_getmsg(  MSGQUEUE *q);
void msgq_pushmsg(MSGQUEUE* q,MESSAGE msg);
void msgq_putmsg(MSGQUEUE* q,MESSAGE msg );

MESSAGE make_message(int32_t iID, void* pInput, void** pOutput,uint32_t config);
#endif	//__SMOS_QUEUE_H
