#ifndef __SMOS_MESSAGE_H
#define __SMOS_MESSAGE_H
#include "stdint.h"
#include "list.h"
typedef void (*messageFunction)(void* pInput,void** pOutput,uint32_t config);

typedef struct _Message_desc
{
    list_node self;
    int32_t iID;
    messageFunction function;
    char* name;
}MESSAGE_DESC,*PMESSAGE_DESC;

PMESSAGE_DESC get_msg_desc_by_id(int32_t iID,list *pMsgList);
PMESSAGE_DESC get_msg_desc_by_name(char* name,list *pMsgList);

void add_msgdesc_to_list(list *listMsgDesc,int32_t iID, messageFunction function, char* name);
void free_msgdesc_list(list *listMsgDesc);

void do_Msg(void);

#endif	//__SMOS_MESSAGE_H
