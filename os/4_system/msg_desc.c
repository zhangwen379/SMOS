#include "msg_desc.h"
#include "kernel.h"
#include "process.h"
#include "screen.h"
#include "trace.h"
#include "array_str.h"
#include "interrupt_init.h"
PMESSAGE_DESC get_msg_desc_by_id(int32_t iID, list* pMsgList)
{
    list_node* elem = pMsgList->head.next;
    while (elem != &pMsgList->tail)
    {
        PMESSAGE_DESC pDesc= elem2entry(MESSAGE_DESC,self,elem);
        if (pDesc->iID == iID) {
            return pDesc;
        }
        elem = elem->next;
    }
    return 0;
}
PMESSAGE_DESC get_msg_desc_by_name(char *name, list* pMsgList)
{
    list_node* elem = pMsgList->head.next;
    while (elem != &pMsgList->tail) {
        PMESSAGE_DESC pDesc= elem2entry(MESSAGE_DESC,self,elem);
        if (!strcmp(pDesc->name ,name)) {
            return pDesc;
        }
        elem = elem->next;
    }
    return 0;
}

void add_msgdesc_to_list(list *listMsgDesc, int32_t iID, messageFunction function, char *name)
{
    PMESSAGE_DESC pMsgDesc=malloc_block(M_KERNEL,sizeof(MESSAGE_DESC));
    pMsgDesc->name=name;
    pMsgDesc->iID=iID;
    pMsgDesc->function=function;
    list_append(listMsgDesc,&pMsgDesc->self);
}

void free_msgdesc_list(list *listMsgDesc)
{
    ASSERT(listMsgDesc);
    while(listMsgDesc->head.next!=&listMsgDesc->tail)
    {
        PMESSAGE_DESC pMsgDesc=elem2entry(MESSAGE_DESC,self,list_pop(listMsgDesc));
        free_block(pMsgDesc);
    }
}

void do_Msg(void)
{
    while (1)
    {
        MESSAGE msg;
        memset(&msg,0,sizeof(MESSAGE));
        bool bemptyqueue=true;
        char* output=0;
        PMESSAGE_DESC pmsgDesc=0;
        cli();
        bemptyqueue=msgq_isempty(&m_gKCB.msgQueue);
        if(!bemptyqueue)
        {
            msg= msgq_getmsg(&m_gKCB.msgQueue);
            int32_t iID=abs(msg.iID);
            pmsgDesc=get_msg_desc_by_id(iID,&m_gKCB.msgdesc_list);
        }
        sti();
        if(pmsgDesc)
        {
            clear_and_set_block(&m_gKCB.sblock_list,IDS_INPUT,pmsgDesc->name,COLOR_NORMAL);
            if(msg.pOutput)
            {
                pmsgDesc->function(msg.pInput,msg.pOutput,msg.config);
            }
            else
            {
                pmsgDesc->function(msg.pInput,(void**)&output,msg.config);
                if(output)
                {
                    set_output(output);
                }
            }
        }
        if(msg.iID>0&&msg.pInput)
        {
            free_block(msg.pInput);
        }

        if(m_gKCB.pprocess_active)
        {
            memset(&msg,0,sizeof(MESSAGE));
            PPCB pPCB= m_gKCB.pprocess_active;
            pmsgDesc=0;
            cli();
            bemptyqueue=msgq_isempty(&pPCB->msgQueue);
            if(!bemptyqueue&&(pPCB->state==STATE_START))
            {
                msg= msgq_getmsg(&pPCB->msgQueue);
                int32_t iID=abs(msg.iID);
                pmsgDesc=get_msg_desc_by_id(iID,&pPCB->msgdesc_list);
            }
            sti();
            if(pmsgDesc)
            {
                clear_and_set_block(&m_gKCB.sblock_list,IDS_INPUT,pmsgDesc->name,COLOR_NORMAL);
                if(msg.pOutput)
                {
                    pmsgDesc->function(msg.pInput,msg.pOutput,msg.config);
                }
                else
                {
                    pmsgDesc->function(msg.pInput,(void**)&output,msg.config);
                    if(output)
                    {
                        set_output(output);
                    }
                }
            }
            if(msg.iID>0&&msg.pInput)
            {
                free_block(msg.pInput);
            }
        }
        //    m_gKCB.pprocess_active=get_current_process();
        set_mouse(0, 0 );
    }
}
