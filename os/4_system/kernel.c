#include "kernel.h"
#include "process.h"
#include "array_str.h"
#include "trace.h"
#include "print.h"
#include "interrupt_init.h"
#include "interrupt_keyboard.h"
#include "explore.h"
#include "smfs_dir_entry.h"
#include "interrupt_timer.h"
#include "path.h"
KCB m_gKCB;
void init_KCB(KCB* pKCB)
{
	memset(pKCB,0,sizeof(KCB));
	init_physics_addr_desc(&pKCB->kernel_paddr_desc, M_KERNEL);
    init_physics_addr_desc(&pKCB->user_paddr_desc, M_USER);

    init_virtual_addr_desc(&pKCB->kernel_vaddr_desc, M_KERNEL);

	init_mem_block_desc(pKCB->kernel_block_descs);

	list_init (&pKCB->process_list);
	list_init (&pKCB->msgdesc_list);

	p_kernel_paddr_desc=&pKCB->kernel_paddr_desc;
	p_kernel_vaddr_desc=&pKCB->kernel_vaddr_desc;
	p_kernel_block=pKCB->kernel_block_descs;

	p_user_paddr_desc=&pKCB->user_paddr_desc;

    pKCB->screen_width=80;
	pKCB->screen_height=25;
    add_msgdesc_to_list(&pKCB->msgdesc_list,msg_keyboard,mf_system_keyboard,"keyboard");
    add_msgdesc_to_list(&pKCB->msgdesc_list,msg_mouse,mf_system_mouse,"mouse");
    add_msgdesc_to_list(&pKCB->msgdesc_list,msg_timer,mf_system_timer,"timer");

    add_msgdesc_to_list(&pKCB->msgdesc_list,msg_block_command_left,mf_system_block_command_left,"command_left");
	add_msgdesc_to_list(&pKCB->msgdesc_list,msg_block_command_right,mf_system_block_command_right,"command_right");

	add_msgdesc_to_list(&pKCB->msgdesc_list,msg_process_init,mf_system_process_init,"load");
	add_msgdesc_to_list(&pKCB->msgdesc_list,msg_process_load,mf_system_process_load,"load");
	add_msgdesc_to_list(&pKCB->msgdesc_list,msg_process_exit,mf_system_process_exit,"exit");
    add_msgdesc_to_list(&pKCB->msgdesc_list,msg_process_fresh,mf_system_process_fresh,"fresh");

	add_msgdesc_to_list(&pKCB->msgdesc_list,msg_log,mf_system_log,"log");

	list_init (&pKCB->sblock_list);
	PSCREENBLOCK pScreenBlock= malloc_screen_block(IDS_PROCESS,
												   0, 0, pKCB->screen_width-5, 1,
												   SBLOCK_INPUT,0,' ',M_KERNEL );
	pScreenBlock->bwantreturn=true;
	list_append(&pKCB->sblock_list,&pScreenBlock->self);

	pScreenBlock= malloc_screen_block(IDS_EXIT,
									  pKCB->screen_width-4, 0, 3, 1,
									  SBLOCK_INPUT,0,'|',M_KERNEL );
	set_buff_to_block(pScreenBlock,0,0,"XX",COLOR_NORMAL);
	list_append(&pKCB->sblock_list,&pScreenBlock->self);

	pScreenBlock= malloc_screen_block(IDS_STATIC,
									  0, 1, 3, 1,
									  SBLOCK_STATIC, 0,0,M_KERNEL );
	set_buff_to_block(pScreenBlock,0,0,"S:",COLOR_NORMAL);
	list_append(&pKCB->sblock_list,&pScreenBlock->self);

	pScreenBlock= malloc_screen_block(IDS_STATE,
									  3, 1, 10, 1,
									  SBLOCK_OUTPUT, 0,0,M_KERNEL );
	list_append(&pKCB->sblock_list,&pScreenBlock->self);

	pScreenBlock= malloc_screen_block(IDS_STATIC,
									  14, 1, 3, 1,
									  SBLOCK_STATIC, 0,0,M_KERNEL );

	set_buff_to_block(pScreenBlock,0,0,"I:",COLOR_NORMAL);

	list_append(&pKCB->sblock_list,&pScreenBlock->self);
	pScreenBlock= malloc_screen_block(IDS_INPUT,
									  18, 1, 10, 1,
									  SBLOCK_OUTPUT, 0,0,M_KERNEL );
	list_append(&pKCB->sblock_list,&pScreenBlock->self);


	pScreenBlock= malloc_screen_block(IDS_STATIC,
									  29, 1, 3, 1,
									  SBLOCK_STATIC, 0,0,M_KERNEL );

	set_buff_to_block(pScreenBlock,0,0,"O:",COLOR_NORMAL);

	list_append(&pKCB->sblock_list,&pScreenBlock->self);
	pScreenBlock= malloc_screen_block(IDS_OUTPUT,
									  33, 1, 40, 1,
									  SBLOCK_OUTPUT, 0,0,M_KERNEL );
	list_append(&pKCB->sblock_list,&pScreenBlock->self);

	pScreenBlock= malloc_screen_block(IDS_CAPSLOCK,
									  74, 1, 2, 1,
									  SBLOCK_STATIC, 0,0,M_KERNEL );
	list_append(&pKCB->sblock_list,&pScreenBlock->self);

	set_buff_to_block(pScreenBlock,0,0,"C",COLOR_NORMAL);

	pScreenBlock= malloc_screen_block(IDS_INSERT,
									  76, 1, 2, 1,
									  SBLOCK_STATIC, 0,0,M_KERNEL );
	list_append(&pKCB->sblock_list,&pScreenBlock->self);

	set_buff_to_block(pScreenBlock,0,0,"I",COLOR_NORMAL);

	pScreenBlock= malloc_screen_block(IDS_NUMSLOCK,
									  78, 1, 2, 1,
									  SBLOCK_STATIC, 0,0,M_KERNEL );
	list_append(&pKCB->sblock_list,&pScreenBlock->self);

	set_buff_to_block(pScreenBlock,0,0,"N",COLOR_NORMAL);

	pScreenBlock= malloc_screen_block(IDS_CONTROL,
									  0, pKCB->screen_height-1, 22, 1,
									  SBLOCK_INPUT, 0,'|',M_KERNEL );

	set_buff_to_block(pScreenBlock,0,0,"START|STOP|CLEAR|STEP",COLOR_NORMAL);

	list_append(&pKCB->sblock_list,&pScreenBlock->self);

	pScreenBlock= malloc_screen_block(IDS_STATIC,
									  22, pKCB->screen_height-1, 2, 1,
									  SBLOCK_STATIC,0, 0,M_KERNEL );

	set_buff_to_block(pScreenBlock,0,0,">:",COLOR_NORMAL);
	list_append(&pKCB->sblock_list,&pScreenBlock->self);

	pScreenBlock= malloc_screen_block(IDS_COMMAND,
									  24, pKCB->screen_height-1, 34, 1,
									  SBLOCK_INOUTPUT,0,0,M_KERNEL );
	pScreenBlock->bwantreturn=true;
	list_append(&pKCB->sblock_list,&pScreenBlock->self);

	pScreenBlock= malloc_screen_block(IDS_TIME,
									  58, pKCB->screen_height-1, 22, 1,
									  SBLOCK_STATIC,0,0,M_KERNEL );
	list_append(&pKCB->sblock_list,&pScreenBlock->self);


	pKCB->mouse.coverchar=MOUSE_CHAR;

	list_init(&pKCB->file_process_link_list);
	PFILE_PROCESS_LINK plink =(PFILE_PROCESS_LINK) malloc_block(M_KERNEL,sizeof(FILE_PROCESS_LINK));
	plink->p_file_suffix="txt";
	plink->p_process_path="C:/editor.bin";
	list_append(&pKCB->file_process_link_list,&plink->self);

	set_timer(TIMER_FRESHTIME,1000,pKCB->timer);

    console_output("kernel init finish...");
}

void mf_system_mouse(void *input, void **output,uint32_t config)
{
    uint16_t status=(uint16_t)((uint32_t)input>>16);
    uint16_t ch=(uint16_t)(uint32_t)input;
    int8_t offsetx=(int8_t)(config>>8);
    int8_t offsety=(int8_t)config;
    int16_t x=(int16_t)((uint32_t)output>>16);
    int16_t y=(int16_t)(uint32_t)output;
    uint8_t flag=status;
    flag^=status>>8;
    flag&=0x7;
    static int32_t  ref_buffx=0,ref_buffy=0;
    PPCB pPCB=get_current_process();
    PCURSOR pcursor;
    CURSORTYPE type;
    if(offsetx==0&&offsety==0)
    {
        if(flag&CURSOR_LEFT_PRESS)
        {
            if(status&CURSOR_LEFT_PRESS)
            {
                if(!(ch&(COLOR_SELECT_LEFT<<8)))
                {
                    clear_cursor(CURSOR_LEFT);
                    PSCREENBLOCK pscreenblock=point_in_screenlock_list(&m_gKCB.sblock_list,x,y,false);
                    if(!pscreenblock&&pPCB)
                    {
                        pscreenblock=point_in_screenlock_list(&pPCB->sblock_list,x,y,false);
                    }
                    if(pscreenblock)
                    {
                        POINT ptscreen,ptbuff;
                        ptscreen.x=x;
                        ptscreen.y=y;
                        screen_to_formatbuff(pscreenblock,&ptscreen,&ptbuff,POINT_POS);

                        m_gKCB.cursor_left.pscreenblock=pscreenblock;
                        set_cursor(ptbuff.x,ptbuff.y,ptbuff.x,ptbuff.y,pscreenblock,true,CURSOR_LEFT);
                        ref_buffx= m_gKCB.cursor_left.buffx1;
                        ref_buffy= m_gKCB.cursor_left.buffy1;
                    }

                }
            }
            else
            {
                ref_buffx=0,ref_buffy=0;
                if(m_gKCB.cursor_left.pscreenblock&&
                        point_in_rect(&m_gKCB.cursor_left.pscreenblock->rect,x,y)&&
                        (ch&(COLOR_SELECT_LEFT<<8)))
                {
                    PSCREENBLOCK pscreenblock=m_gKCB.cursor_left.pscreenblock;
                    char* p= get_buff_from_block_by_color(pscreenblock,COLOR_SELECT_LEFT,SRCOR,M_KERNEL);
                    if(p)
                    {
                        MESSAGE msg= make_message(msg_block_command_left,p,0,pscreenblock->self.iID);
                        if(pscreenblock->self.belong==&m_gKCB.sblock_list)
                        {
                            msgq_putmsg( &m_gKCB.msgQueue,msg);

                        }
                        else
                        {
                            msgq_putmsg( &pPCB->msgQueue,msg);
                        }
                        TRACE(p);
                    }
                }
            }
        }
        else if(flag&CURSOR_RIGHT_PRESS)
        {
            if(status&CURSOR_RIGHT_PRESS)
            {
                if(!(ch&(COLOR_SELECT_RIGHT<<8)))
                {
                    clear_cursor(CURSOR_RIGHT);
                    PSCREENBLOCK pscreenblock=point_in_screenlock_list(&m_gKCB.sblock_list,x,y,false);
                    if(!pscreenblock&&pPCB)
                    {
                        pscreenblock=point_in_screenlock_list(&pPCB->sblock_list,x,y,false);
                    }
                    if(pscreenblock)
                    {
                        POINT ptscreen,ptbuff;
                        ptscreen.x=x;
                        ptscreen.y=y;
                        screen_to_formatbuff(pscreenblock,&ptscreen,&ptbuff,POINT_POS);
                        m_gKCB.cursor_right.pscreenblock=pscreenblock;
                        set_cursor(ptbuff.x,ptbuff.y,ptbuff.x,ptbuff.y,pscreenblock,true,CURSOR_RIGHT);
                        ref_buffx= m_gKCB.cursor_right.buffx1;
                        ref_buffy= m_gKCB.cursor_right.buffy1;
                    }

                }
            }
            else
            {
                ref_buffx=0,ref_buffy=0;
                if(m_gKCB.cursor_right.pscreenblock&&
                        point_in_rect(&m_gKCB.cursor_right.pscreenblock->rect,x,y)&&
                        (ch&(COLOR_SELECT_RIGHT<<8)))
                {
                    PSCREENBLOCK pscreenblock=m_gKCB.cursor_right.pscreenblock;

                    char* p= get_buff_from_block_by_color(pscreenblock,COLOR_SELECT_RIGHT,SRCOR,M_KERNEL);

                    if(p)
                    {
                        MESSAGE msg= make_message(msg_block_command_right,p,0,pscreenblock->self.iID);
                        if(pscreenblock->self.belong==&m_gKCB.sblock_list)
                        {
                            msgq_putmsg( &m_gKCB.msgQueue,msg);

                        }
                        else
                        {
                            msgq_putmsg( &pPCB->msgQueue,msg);
                        }
                    }
                }
            }
        }
        if(flag&CURSOR_CENTER_PRESS)
        {
            if(status&CURSOR_CENTER_PRESS)
            {

            }
            else
            {

            }
        }
    }
    else
    {
        if(status&0x08)
        {

            if(status&CURSOR_LEFT_PRESS||status&CURSOR_RIGHT_PRESS)
            {
                if(status&CURSOR_LEFT_PRESS)
                {
                    pcursor=&m_gKCB.cursor_left;
                    type=CURSOR_LEFT;
                }
                else
                {
                    pcursor=&m_gKCB.cursor_right;
                    type=CURSOR_RIGHT;
                }
                PSCREENBLOCK pscreenblock=pcursor->pscreenblock;
                if(pscreenblock&&pscreenblock->type!=SBLOCK_STATIC&&
                        pscreenblock->type!=SBLOCK_INPUT)
                {
                    POINT ptscreen,ptbuff,ptblock;
                    ptscreen.x=x;
                    ptscreen.y=y;
                    screen_to_formatbuff(pscreenblock,&ptscreen,&ptbuff ,POINT_POS);
                    ptblock.x=ptscreen.x-pscreenblock->rect.x;
                    ptblock.y=ptscreen.y-pscreenblock->rect.y;

                    if(!point_in_rect(&pscreenblock->rect,ptscreen.x,ptscreen.y))
                    {
                        if(ptblock.x<0)ptbuff.x--;
                        if(ptblock.x>pscreenblock->rect.width-1)ptbuff.x++;
                        if(ptblock.y<0)ptbuff.y--;
                        if(ptblock.y>pscreenblock->rect.height-1)ptbuff.y++;
                    }
                    adjust_screen_block_buffpos(pscreenblock,&ptbuff.x,&ptbuff.y);
                    ensure_buff_visible(pscreenblock,ptbuff.x,ptbuff.y);
                    int32_t poscur,posbuff_ref;
                    poscur=ptbuff.x+pscreenblock->buff_width*ptbuff.y;
                    posbuff_ref=ref_buffx+pscreenblock->buff_width*ref_buffy;
                    clear_cursor(type);
                    if(poscur<=posbuff_ref)
                    {
                        set_cursor(ptbuff.x,ptbuff.y,ref_buffx,ref_buffy,pscreenblock,false,type);
                    }
                    else if(poscur>posbuff_ref)
                    {
                        set_cursor(ref_buffx,ref_buffy,ptbuff.x,ptbuff.y,pscreenblock,false,type);
                    }
                    fresh_screen_block(pscreenblock);
                }
            }
        }
        else
        {
            pcursor=&m_gKCB.cursor_left;
            PSCREENBLOCK pscreenblock=pcursor->pscreenblock;
            if(pscreenblock&&pscreenblock->type!=SBLOCK_STATIC)
            {
                pscreenblock->offsety+=offsety;
                adjust_screen_block_offset(pscreenblock);
                fresh_screen_block(pscreenblock);
            }
        }
    }
    if(m_gKCB.pprocess_active)
    {
        MESSAGE msg;
        msg= make_message(-msg_mouse,input,output ,config);
        msgq_pushmsg( &m_gKCB.pprocess_active->msgQueue,msg);
    }

}
void mf_system_keyboard(void *input, void **output,uint32_t config)
{
    UNUSED(config);
    UNUSED(output);
    char* pstr=(char*)input;
    //   bool flagctrl=(config&keyboard_l_ctrl)||(config&keyboard_r_ctrl)?true:false;
    bool binsert=config&keyboard_insert;
    CURSOR* pCursor=&m_gKCB.cursor_left;
    PSCREENBLOCK pscreenblock=pCursor->pscreenblock;
    if(pscreenblock)
    {
        int32_t buffwidth=pscreenblock->buff_width;
        RECT rect=pscreenblock->rect;
        if(pscreenblock->type==SBLOCK_STATIC) return;
        int32_t movestep=1;
        if(pscreenblock->type==SBLOCK_INPUT)
        {
            movestep=2;
        }
        int32_t buffx1=pCursor->buffx1;
        int32_t buffy1=pCursor->buffy1;
        int32_t buffx2=pCursor->buffx2;
        int32_t buffy2=pCursor->buffy2;
        if(pscreenblock->type!=SBLOCK_OUTPUT&&
                pscreenblock->bwantreturn&&
                !strcmp(pstr,"\n"))
        {
            bool bkernel=pscreenblock->self.belong==&m_gKCB.sblock_list;
            char* p=0;
            if(pscreenblock->type==SBLOCK_INOUTPUT)
            {
                p=get_buff_from_block_by_ch(pscreenblock,
                                            buffx1,buffy1,buffx1,buffy1,PALCEHOLD_CHAR,
                                            M_KERNEL );
                insert_to_screen_block(pscreenblock,&buffx1,&buffy1,pstr,COLOR_NORMAL);

            }
            else if(pscreenblock->type==SBLOCK_INPUT)
            {
                p=get_buff_from_block_by_ch(pscreenblock,
                                            buffx1,buffy1,buffx2,buffy2,
                                            pscreenblock->breakchar,M_KERNEL);
            }
            if(p)
            {
                MESSAGE msg= make_message(msg_block_command_left,p,0,pscreenblock->self.iID);
                if(bkernel)
                {
                    msgq_putmsg( &m_gKCB.msgQueue,msg);
                }
                else
                {
                    PPCB pPCB=get_current_process();
                    msgq_putmsg( &pPCB->msgQueue,msg);
                }
                TRACE(p);
            }
        }
        else if((pscreenblock->type==SBLOCK_INOUTPUT)&&
            //    buffx1==buffx2&&buffy1==buffy2&&
                strlen(pstr)==1)
        {
            if(buffx1!=buffx2||buffy1!=buffy2)
            {
                delete_from_screen_block(pscreenblock,&buffx1,&buffy1,buffx2,buffy2);
            }

            buffx2=buffx1;
            buffy2=buffy1;
            if(binsert)
            {
                set_buff_to_block(pscreenblock,buffx2,buffy2,pstr,COLOR_NORMAL);
                int32_t pos=buffy2*buffwidth+buffx2+1;
                buffx2=pos%buffwidth;
                buffy2=pos/buffwidth;
            }
            else
            {
                insert_to_screen_block(pscreenblock,&buffx2,&buffy2,pstr,COLOR_NORMAL);
            }
            buffx1=buffx2;
            buffy1=buffy2;
        }
        else if((pscreenblock->type==SBLOCK_INOUTPUT)&&
                !strcmp(pstr,"DEL"))
        {
            delete_from_screen_block(pscreenblock,&buffx1,&buffy1,buffx2,buffy2);
            buffx2=buffx1;
            buffy2=buffy1;
        }
        else if((pscreenblock->type==SBLOCK_INOUTPUT)&&
                !strcmp(pstr,"BKS"))
        {
            if(buffx1!=buffx2||buffy1!=buffy2)
            {
                delete_from_screen_block(pscreenblock,&buffx1,&buffy1,buffx2,buffy2);
            }
            else
            {
                int32_t buffpos=buffx1+buffy1*buffwidth;
                if(buffpos)
                {
                    buffpos--;
                    buffx1=buffpos%buffwidth;
                    buffy1=buffpos/buffwidth;
                    delete_from_screen_block(pscreenblock,&buffx1,&buffy1,buffx1,buffy1);
                }
            }
            buffx2=buffx1;
            buffy2=buffy1;
        }
        else if(!strcmp(pstr,"UP")) { buffy1--; buffx2=buffx1; buffy2=buffy1;}
        else if(!strcmp(pstr,"DWN")){ buffy2++;  buffx1=buffx2; buffy1=buffy2; }
        else if(!strcmp(pstr,"LFT")){ buffx1-=movestep;  buffx2=buffx1; buffy2=buffy1;}
        else if(!strcmp(pstr,"RGT")){ buffx2+=movestep;  buffx1=buffx2; buffy1=buffy2; }
        else if(!strcmp(pstr,"HOM")){ buffy1=0;buffx2=buffx1; buffy2=buffy1; }
        else if(!strcmp(pstr,"END")){ buffy2=pscreenblock->validline-1; buffx1=buffx2; buffy1=buffy2; }
        else if(!strcmp(pstr,"PUP")){ buffy1-=rect.height; buffx2=buffx1; buffy2=buffy1; }
        else if(!strcmp(pstr,"PDN")){ buffy2+=rect.height; buffx1=buffx2; buffy1=buffy2; }

        adjust_screen_block_buffpos(pscreenblock,&buffx1,&buffy1);

        if(get_cursor(CURSOR_RIGHT)->pscreenblock==pscreenblock)
        {
            clear_cursor(CURSOR_RIGHT);
        }
        clear_cursor(CURSOR_LEFT);
        set_cursor(buffx1,buffy1,buffx1,buffy1,pscreenblock,false,CURSOR_LEFT);
        ensure_buff_visible(pscreenblock,m_gKCB.cursor_left.buffx2,m_gKCB.cursor_left.buffy2);


        fresh_screen_block(pscreenblock);

    }
    if(m_gKCB.pprocess_active)
    {
        MESSAGE msg;
        msg= make_message(-msg_keyboard,input,output ,config);
        msgq_putmsg( &m_gKCB.pprocess_active->msgQueue,msg);
    }
}

void mf_system_timer(void *input, void **output, uint32_t config)
{
    UNUSED(output);
    UNUSED(config);
    int32_t id=(int32_t)input;
    switch(id)
    {
    case TIMER_FRESHTIME:
    {
        MBIOSTIME biostime;
        getBIOSTime(&biostime);
        PSCREENBLOCK pScreenBlock=get_screenblock_by_id(IDS_TIME,&m_gKCB.sblock_list);
        char ch[32]={0};
        formatBiosTime(&biostime,ch);
        set_buff_to_block(pScreenBlock,0,0,ch,COLOR_NORMAL);
        fresh_screen_block( pScreenBlock);
        break;
    }
    default:break;

    }
}

void mf_system_block_command_left(void *input, void **output,uint32_t config)
{
	UNUSED(output);
	if(config==IDS_COMMAND)
	{
		PPCB pPCB=get_current_process();
		if(!strcmp((char*)input,"EXIT"))
		{
			MESSAGE msg=make_message(msg_process_exit,0,0,pPCB->self.iID);
			msgq_putmsg(&m_gKCB.msgQueue,msg);
		}
		else
		{
			char* str=strchr((char*)input,' ');
			char* pinput=0;
			if(str)
			{
				*str=0;
				str++;
				pinput=malloc_block(M_USER,strlen(str)+1);
				strcpy(pinput,str);
			}
			PMESSAGE_DESC pMsgDesc= get_msg_desc_by_name((char*)input,&pPCB->msgdesc_list);
			if(!pMsgDesc) return;
			MESSAGE msg=make_message(pMsgDesc->iID,pinput,0,0);
			msgq_putmsg(&pPCB->msgQueue,msg);
		}
	}
	else if(config==IDS_EXIT)
	{
		PPCB pPCB=get_current_process();
		MESSAGE msg=make_message(msg_process_exit,0,0,pPCB->self.iID);
		msgq_putmsg(&m_gKCB.msgQueue,msg);
	}
	else if(config==IDS_PROCESS)
	{
		char* str=(char*)input;
		char* pid=strrchr(str,'-');
		if(pid)
		{
			pid++;
			int32_t ipid=atoi(pid);
			list_node* pelem= list_node_get_by_id(&m_gKCB.process_list,ipid);
			if(pelem)
			{
				PPCB ppcb=elem2entry(PCB,self,pelem);
				m_gKCB.pprocess_active=ppcb;
				page_dir_install(ppcb->pgvdir);
				p_user_block=ppcb->user_block_descs;
				p_user_vaddr_desc=&ppcb->user_vaddr_descs;

				MESSAGE msg=make_message(msg_process_fresh,0,0,(uint32_t)ppcb->self.iID);
				msgq_putmsg(&m_gKCB.msgQueue,msg);
			}
		}
	}
	else if(config==IDS_CONTROL)
	{
		PPCB pPCB=get_current_process();
		if(pPCB)
		{
			char* str=(char*)input;
			if(!strcmp(str,"START"))
			{
				set_state(STATE_START,pPCB->self.iID);
			}
			else if(!strcmp(str,"STOP"))
			{
				set_state(STATE_STOP,pPCB->self.iID);
			}
			else if(!strcmp(str,"STEP"))
			{
				MESSAGE msg=make_message(msg_step,0,0,0);
				msgq_putmsg(&m_gKCB.msgQueue,msg);
			}
			else if(!strcmp(str,"CLEAR"))
            {
                MSGQUEUE* q=&m_gKCB.msgQueue;
                while(q-> fill!=q->take)
                {
                    PMESSAGE pmsg= q->msgVector+q-> fill;
                    if(pmsg->iID>0&&pmsg->pInput)
                    {
                        free_block(pmsg->pInput);
                    }
                    memset(pmsg,0,sizeof(MESSAGE));
                    q-> fill=msgq_prev_pos(q-> fill);
                }
                msgq_init(  q);
			}
		}
	}
}

void mf_system_block_command_right(void *input, void **output, uint32_t config)
{
	UNUSED(config);
	UNUSED(output);
	UNUSED(input);

}
void mf_system_process_init(void *input, void **output,uint32_t config)
{
	UNUSED(config);
	UNUSED(output);
	PPCB ppcb=(PPCB)get_process((int32_t)config);
	if(ppcb)
	{
		if(ppcb->main)
		{
			if(!ppcb->bkernel)
			{
				TRACE1("ppcb->main:%x",ppcb->main);
				//  TRACE_SECTION((char*)ppcb->main);
			}
			((init_main)ppcb->main)(ppcb,(char**)(&input));
			MESSAGE msg=make_message(-msg_process_fresh,0,0,config);
			msgq_putmsg(&m_gKCB.msgQueue,msg);
		}
	}
}

void mf_system_process_load(void *input, void **output,uint32_t config)
{
	UNUSED(config);
	UNUSED(output);
	if(config>>16==FILE_ELF)
	{
        create_process(0,(char*)input,0);
	}
	else if(config>>16==FILE_BINARY)
	{
		char* pinputpath=(char*)input;
		char* psuffix=strchr(pinputpath,'.');
		if(!psuffix){return; }
		psuffix++;
		const char* pprocesspath= get_processpath_by_suffix(psuffix);
		if(!pprocesspath){    return;}
        create_process(0,pprocesspath,pinputpath);
	}
}

void mf_system_process_exit(void *input, void **output,uint32_t config)
{
	UNUSED(config);
	UNUSED(input);
	UNUSED(output);
	int32_t ipid=(int32_t)config;

    exit_process(ipid);
}

void mf_system_process_fresh(void *input, void **output,uint32_t config)
{
	UNUSED(input);
	UNUSED(output);
	clear_screen();
	clear_cursor(CURSOR_LEFT);
	clear_cursor(CURSOR_RIGHT);
	PPCB ppcb=(PPCB)get_process((int32_t)config);
	if(ppcb)
	{
		list_node* pelem=ppcb->sblock_list.head.next;
		while(pelem!=&ppcb->sblock_list.tail)
		{
			PSCREENBLOCK pscreenblock= elem2entry(SCREENBLOCK,self,pelem);
			fresh_screen_block(pscreenblock);
			pelem=pelem->next;
		}
	}
	fresh_system_sblock_pcb();
	list_node* pelem=m_gKCB.sblock_list.head.next;
	while(pelem!=&m_gKCB.sblock_list.tail)
	{
		PSCREENBLOCK pscreenblock= elem2entry(SCREENBLOCK,self,pelem);
		fresh_screen_block(pscreenblock);
		pelem=pelem->next;
	}
}

void mf_system_log(void *input, void **output, uint32_t config)
{
    UNUSED(output);
    UNUSED(config);
    write_log((char*)input);
    free_block(input);
}

void set_mouse(int16_t xoffset, int16_t yoffset )
{
	cli();
	if(xoffset||yoffset)
	{
		if(  m_gKCB.mouse.coverchar!=MOUSE_CHAR)
		{
			print_n_wordchar(m_gKCB.mouse.y,m_gKCB.mouse.x,m_gKCB.mouse.coverchar,1);
		}

		m_gKCB.mouse.x+=xoffset;
		m_gKCB.mouse.y+=yoffset;
		if(m_gKCB.mouse.x<0)m_gKCB.mouse.x=0;
		if(m_gKCB.mouse.y<0)m_gKCB.mouse.y=0;
		if(m_gKCB.mouse.x>m_gKCB.screen_width-1)m_gKCB.mouse.x=m_gKCB.screen_width-1;
		if(m_gKCB.mouse.y>m_gKCB.screen_height-1)m_gKCB.mouse.y=m_gKCB.screen_height-1;
        get_wordchars(m_gKCB.mouse.y,m_gKCB.mouse.x,&m_gKCB.mouse.coverchar,1);
		//     TRACE1("mouse%x,%x,%x",m_gKCB.mouse.y,m_gKCB.mouse.x,m_gKCB.mouse.coverchar);

	}
	if(!m_gKCB.mouse.invisible&&m_gKCB.mouse.coverchar!=MOUSE_CHAR)
	{
		uint16_t ch;
		get_wordchars(m_gKCB.mouse.y,m_gKCB.mouse.x,&ch,1);
		if(ch!=MOUSE_CHAR) {m_gKCB.mouse.coverchar=ch;}
		print_n_wordchar(m_gKCB.mouse.y,m_gKCB.mouse.x,MOUSE_CHAR,1);
	}
	sti();
}

void set_cursor(int32_t buffx1, int32_t buffy1, int32_t buffx2, int32_t buffy2,
                PSCREENBLOCK pscreenblock , bool bfresh, CURSORTYPE type)
{
	bool singlesel=buffx1==buffx2&&buffy1==buffy2;
	PCURSOR pcursor;
	uint8_t color;
	switch(type)
	{
	case CURSOR_LEFT:
		pcursor=&m_gKCB.cursor_left;
		color=COLOR_SELECT_LEFT;
		break;
	case CURSOR_RIGHT:
		pcursor=&m_gKCB.cursor_right;
		color=COLOR_SELECT_RIGHT;
		break;
	default:break;
	}
	if(pscreenblock)
	{
		adjust_screen_block_buffpos(pscreenblock,&buffx1,&buffy1);
		adjust_screen_block_buffpos(pscreenblock,&buffx2,&buffy2);

		switch(pscreenblock->type)
		{
		case SBLOCK_STATIC: return;
		case SBLOCK_INPUT:
		{
            if(!singlesel) return;
            int32_t iselsize=select_screen_block_by_ch(pscreenblock,&buffx1,&buffy1,&buffx2,&buffy2,
                                                      color,SRCOR,pscreenblock->breakchar);
            if(!iselsize){return;}
		}
        break;
		case SBLOCK_OUTPUT:
		case SBLOCK_INOUTPUT:
		{
			mflags flag=pscreenblock->flag;
			int32_t buffwidth=pscreenblock->buff_width;

			char* p=get_buff_from_block_by_buffpos(pscreenblock,0,buffy1,
												   buffwidth-1,buffy1,
												   flag);
			ASSERT(p);
			int32_t isize=strlen(p);
			if((buffx1>isize-1)) buffx1=max(isize-1,0);
            free_block(p);
			if(singlesel)
			{
				buffx2=buffx1;
				buffy2=buffy1;
			}
			else
			{
				p=get_buff_from_block_by_buffpos(pscreenblock,0,buffy2,
												 buffwidth-1,buffy2,
												 flag);
				ASSERT(p);
				int32_t isize=strlen(p);
				if((buffx2>isize-1)) buffx2=max(isize-1,0);
                free_block(p);
			}
			select_screen_block(pscreenblock,buffx1,buffy1,buffx2,buffy2,color,SRCOR);
			break;
		}
		default: ASSERT(0);  break;
		}

		pcursor->pscreenblock=pscreenblock;
		pcursor->buffx1=buffx1;
		pcursor->buffy1=buffy1;
		pcursor->buffx2=buffx2;
        pcursor->buffy2=buffy2;
		if(bfresh)
		{
			fresh_screen_block_buffpos(pscreenblock,buffx1,buffy1,buffx2,buffy2);
		}
	}
}

void clear_cursor(CURSORTYPE type)
{
	PCURSOR pcursor;
	uint8_t color;
	switch(type)
	{
	case CURSOR_LEFT:
		pcursor=&m_gKCB.cursor_left;
		color=COLOR_SELECT_LEFT;
		break;
	case CURSOR_RIGHT:
		pcursor=&m_gKCB.cursor_right;
		color=COLOR_SELECT_RIGHT;
		break;
	default:break;
	}
	if(pcursor->pscreenblock==0) return;
	unselect_screen_block_by_color(pcursor->pscreenblock,color,SRCOR);
	pcursor->pscreenblock=0;
	pcursor->buffx1=pcursor->buffx2=0;
	pcursor->buffy1=pcursor->buffy2=0;
//	m_gKCB.mouse.coverchar&=~(color<<8);
}

PMOUSE get_mouse(void)
{
	return &m_gKCB.mouse;
}

PCURSOR get_cursor(CURSORTYPE type)
{
	if(type==CURSOR_RIGHT)
	{
		return &m_gKCB.cursor_right;
	}
	else
	{
		return &m_gKCB.cursor_left;
	}
}

char *get_system_command_str(mflags flag)
{
	PSCREENBLOCK pscreenblock=get_screenblock_by_id(IDS_COMMAND,&m_gKCB.sblock_list);
	char* p=get_buff_from_block_by_ch(pscreenblock,0,pscreenblock->validline-1,
									  pscreenblock->buff_width-1,pscreenblock->validline-1,
									  PALCEHOLD_CHAR,flag);
	if(p)
	{
		int isize=strlen(p);
		if(isize)
		{
			if(p[isize-1]=='\n')
			{
				p[isize-1]=0;
			}
		}
	}
	return p;
}

void fresh_system_sblock_pcb(void)
{
	PSCREENBLOCK pscreenblock= get_screenblock_by_id(IDS_PROCESS,&m_gKCB.sblock_list);
	clear_screen_block(pscreenblock);
	list *plistpcb=&m_gKCB.process_list;
	list_node* pelem = plistpcb->head.next;

	int32_t ilen=0,buffpos=0;
	while (pelem!= &plistpcb->tail)
	{
		PPCB pPCB= elem2entry(PCB,self,pelem);

		char* pinputname=strrchr(pPCB->inputpath,'/');
		if(!pinputname){pinputname=pPCB->inputpath;}
		else
		{
			pinputname++;
		}
		char* pprocessname=strrchr(pPCB->processpath,'/');
		if(!pprocessname){pprocessname=pPCB->processpath;}
		else
		{
			pprocessname++;
		}
		char ch[MAX_FILE_NAME_LEN_SMFS*2+16]={0};
        sprintf(ch,MAX_FILE_NAME_LEN_SMFS*2+16,"%s-%s-%d",
                pprocessname,pinputname,pPCB->self.iID);
		//    strcat(str,ch);
		uint8_t color=COLOR_NORMAL;
		if(m_gKCB.pprocess_active==pPCB)
		{
			buffpos=ilen;
			color=0x5f;
		}
		else
		{
			ilen+=strlen(ch);
		}
		appand_to_screen_block(pscreenblock,ch,color);
		if(pelem->next!=&plistpcb->tail)
		{
			appand_to_screen_block(pscreenblock," ",COLOR_NORMAL);

		}
		pelem=pelem->next;
	}
	ensure_buff_visible(pscreenblock,0,buffpos/pscreenblock->buff_width);
}


const char *get_processpath_by_suffix(char *psuffix)
{
	list* plist=&m_gKCB.file_process_link_list;
	list_node* pelem=plist->head.next;
	while(pelem!=&plist->tail)
	{
		PFILE_PROCESS_LINK plink=elem2entry(FILE_PROCESS_LINK,
											self,pelem);
		if(!strcmp1(plink->p_file_suffix,psuffix))
		{
			return plink->p_process_path;
		}
		pelem=pelem->next;
	}
	return 0;
}

void post_msg_to_kcb(PMESSAGE pmsg, bool bfront)
{
	if(bfront)
	{
		msgq_pushmsg(&m_gKCB.msgQueue,*pmsg);
	}
	else
	{
		msgq_putmsg(&m_gKCB.msgQueue,*pmsg);
	}
}

void post_msg_to_pcb(PMESSAGE pmsg, int32_t ipid, bool bfront)
{
	PPCB ppcb=get_process(ipid);
	if(ppcb)
	{
		if(bfront)
		{
			msgq_pushmsg(&ppcb->msgQueue,*pmsg);
		}
		else
		{
			msgq_putmsg(&ppcb->msgQueue,*pmsg);
		}
	}
}

void set_clipboard(void *p, int32_t isize, CLIPTYPE type, bool bcut)
{
	ASSERT(p);
	if(m_gKCB.clipboard.p)
	{
        free_block(m_gKCB.clipboard.p);
		m_gKCB.clipboard.p=0;
	}
	m_gKCB.clipboard.p=malloc_block(M_KERNEL,isize+1);
	memcpy(m_gKCB.clipboard.p,p,isize);
	m_gKCB.clipboard.type=type;
	m_gKCB.clipboard.bcut=bcut;
	m_gKCB.clipboard.isize=isize;

}

PCLIPBOARD get_clipboard()
{
	return &m_gKCB.clipboard;
}

void set_state(int32_t iStateID, int32_t ipid)
{
	PPCB pcb=get_process(ipid);
	if(pcb)
	{
		pcb->state=iStateID;
		list_node* pelem= list_node_get_by_id(&m_gKCB.sblock_list,IDS_STATE);
		PSCREENBLOCK pscreenblock_state=elem2entry(SCREENBLOCK,self,pelem);
		clear_screen_block(pscreenblock_state);
		switch (iStateID) {
		case STATE_START:
			set_buff_to_block(pscreenblock_state,0,0,"START",COLOR_NORMAL);
			break;
		case STATE_STOP:
			set_buff_to_block(pscreenblock_state,0,0,"STOP",COLOR_NORMAL);
			break;
		default:
			break;
		}
		fresh_screen_block(pscreenblock_state);
	}
}

void set_output(char *p)
{
	int isize=strlen(p);
	uint8_t color=COLOR_SUCCESS;
	if(isize)
	{
		if(p[0]=='!')
		{
			color=COLOR_WARNING;
			p++;
		}
	}
    clear_and_set_block(&m_gKCB.sblock_list,IDS_OUTPUT,p,color);
    char chtime[64]={0};
    MBIOSTIME BIOSTime;
    getBIOSTime(&BIOSTime);
    formatBiosTime(&BIOSTime, chtime);
    char* p1=malloc_block(M_KERNEL,isize+2+64);
    strcpy(p1,chtime);
    strcat(p1," ");
    strcat(p1,p);
    strcat(p1,"\n");
    MESSAGE msg=make_message(msg_log,p1,0,M_KERNEL);
    post_msg_to_kcb(&msg,true);
	
}
PPCB get_current_process(void)
{
	return m_gKCB.pprocess_active;
}

PPCB get_process(int32_t pid)
{
	list_node* pelem=list_node_get_by_id(&m_gKCB.process_list,pid);
	if(pelem )
	{
		return elem2entry(PCB,self,pelem);
	}
	return 0;
}

void add_path_to_pcb(PPCB ppcb,const  char *path)
{
	if(!path||!ppcb) return;
	int isize=strlen(path);
	if(!isize) return;
	if(!is_path_used_by_process(  ppcb,  path))
	{
		PPATH_USED ppath_use =(PPATH_USED)malloc_block(M_KERNEL,sizeof(PATH_USED));
		ppath_use->path=(char*)malloc_block(M_KERNEL,isize+1);
		strcpy(ppath_use->path,path);
		list_push(&ppcb->path_list,&ppath_use->self);
	}
}

bool is_path_used_by_process(PPCB ppcb,const  char *path)
{
	if(!ppcb||!path) return false;
	int isize_path_cur=strlen(path);
	list_node* pelem=ppcb->path_list.head.next;
	while(pelem!=&ppcb->path_list.tail)
	{
		PPATH_USED ppath_use = elem2entry(PATH_USED,self,pelem);
		int isize_path_use=strlen(ppath_use->path);
		if(isize_path_use>=isize_path_cur)
		{
			int i=0;
			while(i<isize_path_cur)
			{
				if(path[i]!=ppath_use->path[i]&&
						abs(path[i]-ppath_use->path[i])!='a'-'A')
				{
					break;
				}
				i++;
			}
			if(i==isize_path_cur)
			{
				return true;
			}
		}
		pelem=pelem->next;
	}
	return false;
}

void clear_path_from_pcb(PPCB ppcb)
{
	if(!ppcb) return ;
	while(ppcb->path_list.head.next!=&ppcb->path_list.tail)
	{
		PPATH_USED ppath_use = elem2entry(PATH_USED,self,list_pop(&ppcb->path_list));
        free_block(ppath_use->path);
        free_block(ppath_use);
	}
}

void set_keyboard_indicate_screenblock(int32_t iIndicateID, bool bon)
{
	if(iIndicateID!=IDS_CAPSLOCK&&
			iIndicateID!=IDS_NUMSLOCK&&
			iIndicateID!=IDS_INSERT)
	{
		return;
	}
	PSCREENBLOCK pscreenblock=get_screenblock_by_id(iIndicateID,&m_gKCB.sblock_list);
	uint8_t color=COLOR_NORMAL;
	if(bon)
	{
		color=COLOR_WARNING;
	}
	select_screen_block(pscreenblock,0,0,0,0,color,SRCCOPY);
	fresh_screen_block(pscreenblock);
}

void write_log(char *p)
{
	file* pfile=0;
	int32_t istrsize=strlen(p);
	pfile=create_path_file("C:/log.txt");
	if(!pfile)
	{
		pfile= open_path_file("C:/log.txt");
	}
	int32_t ifilesize= get_file_size(pfile);
	if(ifilesize<1024*64)
	{
		write_file(pfile,ifilesize,p,istrsize);
	}
	else
	{
		write_file(pfile,0,p,istrsize);
	}
	close_file(pfile);

}
