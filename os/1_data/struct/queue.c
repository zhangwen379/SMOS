#include "queue.h"
#include "array_str.h"
#include "memory.h"
#include "trace.h"
void msgq_init( MSGQUEUE* q)
{
    memset(q,0,sizeof(MSGQUEUE));
//	q->take = q->send = 0;
}

//消息队列中的上一个处理位置
int32_t msgq_prev_pos(int32_t pos)
{
	if(pos==0)
	{
		return QUEUESIZE-1;
	}
	return pos - 1;
}
//消息队列中的下一个处理位置
int32_t msgq_next_pos(int32_t pos)
{
	if(pos==QUEUESIZE-1)
	{
		return 0;
	}
	return pos + 1;
}
// 判断队列是否已满
bool msgq_isfull(  MSGQUEUE* q)
{
	return msgq_next_pos(q-> fill)==(q->take);
}

// 判断队列是否已空
bool msgq_isempty(  MSGQUEUE* q)
{
	return q->take == q-> fill;
}

// 从队列中获取消息
MESSAGE msgq_getmsg(  MSGQUEUE* q)
{
    MESSAGE msg;
    memset(&msg,0,sizeof(MESSAGE));
	if(msgq_isempty(q)) {return msg;}
	msg = q->msgVector[q->take];
	memset(&q->msgVector[q->take],0,sizeof(MESSAGE));
	q->take = msgq_next_pos(q->take);
    return msg;
}

// 向队列加入立刻处理的消息
void msgq_pushmsg(  MSGQUEUE* q,MESSAGE msg )
{
	if(msgq_isfull(q)) { return ;}
	q->take = msgq_prev_pos(q->take);
	q->msgVector[q->take] = msg;
}
// 向队列加入顺序处理的消息
void msgq_putmsg(  MSGQUEUE* q,MESSAGE msg  )
{
	if(msgq_isfull(q)) { return ; }
	q->msgVector[q-> fill] = msg;
	q-> fill = msgq_next_pos(q-> fill);
}


MESSAGE make_message(int32_t iID, void *pInput, void **pOutput,uint32_t config)
{
    MESSAGE msg;
    msg.iID=iID;
    msg.pInput=pInput;
    msg.pOutput=pOutput;
    msg.config=config;
    return msg;
}
