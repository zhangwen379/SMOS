#include "screen.h"
#include "process.h"
#include "array_str.h"
#include "trace.h"
#include "print.h"
#include "interrupt_init.h"
#include "queue.h"
#include "kernel.h"
#include "print.h"
int32_t screen_width=80;
int32_t screen_height=25;
RECT make_rect(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    RECT rect;
    rect.x=x1;
    rect.y=y1;
    rect.width=x2-x1+1;
    rect.height=y2-y1+1;
    return rect;
}

void screen_to_block(PSCREENBLOCK pscreenblock, const void *pin_screen, void *pout_block, uint8_t pos_type)
{
    switch(pos_type)
    {
    case POINT_POS:
    {
        const POINT* pPTin=pin_screen;
        PPOINT pPTout=pout_block;
        pPTout->x=pPTin->x-pscreenblock->rect.x;
        pPTout->y=pPTin->y-pscreenblock->rect.y;
        pPTout->x=max(pPTout->x,0);
        pPTout->y=max(pPTout->y,0);
        pPTout->x=min(pPTout->x,pscreenblock->rect.width-1);
        pPTout->y=min(pPTout->y,pscreenblock->rect.height-1);
        break;
    }
    case RECT_POS:
    {
        const RECT* pRECTin=pin_screen;
        PRECT pRECTout=pout_block;

        pRECTout->x=pRECTin->x-pscreenblock->rect.x;
        pRECTout->y=pRECTin->y-pscreenblock->rect.y;
        pRECTout->x=max(pRECTout->x,0);
        pRECTout->y=max(pRECTout->y,0);
        pRECTout->x=min(pRECTout->x,pscreenblock->rect.width-1);
        pRECTout->y=min(pRECTout->y,pscreenblock->rect.height-1);

        int32_t x2=pRECTin->x+pRECTin->width-1;
        int32_t y2=pRECTin->y+pRECTin->height-1;

        x2=x2-pscreenblock->rect.x;
        y2=y2-pscreenblock->rect.y;
        x2=max(x2,0);
        y2=max(y2,0);
        x2=min(x2,pscreenblock->rect.width-1);
        y2=min(y2,pscreenblock->rect.height-1);

        pRECTout->width=x2-pRECTout->x+1;
        pRECTout->height=y2-pRECTout->y+1;
        break;
    }
    default:ASSERT(0);break;
    }
}

void block_to_formatbuff(PSCREENBLOCK pscreenblock, const void *pin_block, void *pout_buff, uint8_t pos_type)
{
    switch(pos_type)
    {
    case POINT_POS:
    {
        const POINT* pPTin=pin_block;
        PPOINT pPTout=pout_buff;
        pPTout->x=pPTin->x+pscreenblock->offsetx;
        pPTout->y=pPTin->y+pscreenblock->offsety;
        pPTout->x=min(pPTout->x,pscreenblock->buff_width-1);
        pPTout->y=min(pPTout->y,pscreenblock->validline-1);
        break;
    }
    case RECT_POS:
    {
        const RECT* pRECTin=pin_block;
        PRECT pRECTout=pout_buff;
        pRECTout->x=pRECTin->x+pscreenblock->offsetx;
        pRECTout->y=pRECTin->y+pscreenblock->offsety;
        pRECTout->x=min(pRECTout->x,pscreenblock->buff_width-1);
        pRECTout->y=min(pRECTout->y,pscreenblock->validline-1);
        int32_t x2=pRECTin->x+pRECTin->width-1;
        int32_t y2=pRECTin->y+pRECTin->height-1;
        x2+=pscreenblock->offsetx;
        y2+=pscreenblock->offsety;
        x2=min(x2,pscreenblock->buff_width-1);
        y2=min(y2,pscreenblock->validline-1);
        pRECTout->width=x2-pRECTout->x+1;
        pRECTout->height=y2-pRECTout->y+1;
        break;
    }
    default:ASSERT(0);break;
    }
}

void screen_to_formatbuff(PSCREENBLOCK pscreenblock, const void *pin_screen, void *pout_buff, uint8_t pos_type)
{
    switch(pos_type)
    {
    case POINT_POS:
    {
        POINT temp;
        screen_to_block(pscreenblock,pin_screen, &temp,pos_type);
        block_to_formatbuff(pscreenblock, &temp, pout_buff,pos_type);
        break;
    }
    case RECT_POS:
    {
        RECT temp;
        screen_to_block(pscreenblock,pin_screen, &temp,pos_type);
        block_to_formatbuff(pscreenblock, &temp, pout_buff,pos_type);
        break;
    }
    default:ASSERT(0);break;
    }
}

void formatbuff_to_block(PSCREENBLOCK pscreenblock, const void *pin_buff, void *pout_block, uint8_t pos_type)
{
    switch(pos_type)
    {
    case POINT_POS:
    {
        const POINT* pPTin=pin_buff;
        PPOINT pPTout=pout_block;
        pPTout->x=pPTin->x-pscreenblock->offsetx;
        pPTout->y=pPTin->y-pscreenblock->offsety;

        pPTout->x=max(pPTout->x,0);
        pPTout->y=max(pPTout->y,0);
        pPTout->x=min(pPTout->x,pscreenblock->rect.width-1);
        pPTout->y=min(pPTout->y,pscreenblock->rect.height-1);

        break;
    }
    case RECT_POS:
    {
        const RECT* pRECTin=pin_buff;
        PRECT pRECTout=pout_block;
        pRECTout->x=pRECTin->x-pscreenblock->offsetx;
        pRECTout->y=pRECTin->y-pscreenblock->offsety;

        pRECTout->x=max(pRECTout->x,0);
        pRECTout->y=max(pRECTout->y,0);
        pRECTout->x=min(pRECTout->x,pscreenblock->rect.width-1);
        pRECTout->y=min(pRECTout->y,pscreenblock->rect.height-1);

        int32_t x2=pRECTin->x+pRECTin->width-1;
        int32_t y2=pRECTin->y+pRECTin->height-1;
        x2-=pscreenblock->offsetx;
        y2-=pscreenblock->offsety;

        x2=max(x2,0);
        y2=max(y2,0);
        x2=min(x2,pscreenblock->rect.width-1);
        y2=min(y2,pscreenblock->rect.height-1);

        pRECTout->width=x2-pRECTout->x+1;
        pRECTout->height=y2-pRECTout->y+1;
        break;
    }
    default:ASSERT(0);break;
    }
}

void block_to_screen(PSCREENBLOCK pscreenblock, const void *pin_block, void *pout_screen, uint8_t pos_type)
{
    switch(pos_type)
    {
    case POINT_POS:
    {
        const POINT* pPTin=pin_block;
        PPOINT pPTout=pout_screen;
        pPTout->x=pPTin->x+pscreenblock->rect.x;
        pPTout->y=pPTin->y+pscreenblock->rect.y;
        pPTout->x=min(pPTout->x,screen_width-1);
        pPTout->y=min(pPTout->y,screen_height-1);
        break;
    }
    case RECT_POS:
    {
        const RECT* pPTin=pin_block;
        PRECT pPTout=pout_screen;

        pPTout->x=pPTin->x+pscreenblock->rect.x;
        pPTout->y=pPTin->y+pscreenblock->rect.y;
        pPTout->x=min(pPTout->x,screen_width-1);
        pPTout->y=min(pPTout->y,screen_height-1);

        int32_t x2=pPTin->x+pPTin->width-1;
        int32_t y2=pPTin->y+pPTin->height-1;

        x2=x2+pscreenblock->rect.x;
        y2=y2+pscreenblock->rect.y;
        x2=min(x2,screen_width-1);
        y2=min(y2,screen_height-1);

        pPTout->width=x2-pPTout->x+1;
        pPTout->height=y2-pPTout->y+1;
        break;
    }
    default:ASSERT(0);break;
    }
}

void formatbuff_to_screen(PSCREENBLOCK pscreenblock, const void *pin_buff, void *pout_screen, uint8_t pos_type)
{

    switch(pos_type)
    {
    case POINT_POS:
    {
        POINT temp;
        formatbuff_to_block(pscreenblock,pin_buff , &temp,pos_type);
        block_to_screen(pscreenblock, &temp,pout_screen,pos_type);
        break;
    }
    case RECT_POS:
    {
        RECT temp;
        formatbuff_to_block(pscreenblock,pin_buff, &temp,pos_type);
        block_to_screen(pscreenblock, &temp, pout_screen,pos_type);
        break;
    }
    default:ASSERT(0);break;
    }
}

PFORMATBUFF malloc_format_buff(int32_t width, int32_t height,mflags flag)
{
    PFORMATBUFF pformatbuff=malloc_block(flag,sizeof(FORMATBUFF));
    pformatbuff->p=malloc_block(flag,width*height*2);
    pformatbuff->width=width;
    pformatbuff->height=height;
    pformatbuff->flag=flag;
    return pformatbuff;
}

void free_format_buff(PFORMATBUFF pformatbuff)
{

    free_block(pformatbuff->p);
    free_block(pformatbuff);

}

static void add_format_buff(PSCREENBLOCK pscreenblock, int32_t count)
{
    list* pformatbufflist=&pscreenblock->list_formatbuff;
    int i=0;
    int32_t buffheight=pscreenblock->buff_height;
    int32_t buffwidth=pscreenblock->buff_width;
    mflags flag=pscreenblock->flag;
    for(i=0;i<count;i++)
    {
        PFORMATBUFF pnewformatbuff=malloc_format_buff(buffwidth,buffheight,flag);
        list_append(pformatbufflist,&pnewformatbuff->self);

    }
}

static void del_format_buff(PSCREENBLOCK pscreenblock, int32_t count)
{
    list* pformatbufflist=&pscreenblock->list_formatbuff;
    int32_t buffcount=list_len(pformatbufflist);
    count=min(buffcount-1,count);
    if(count<=0) count=buffcount-1;
    int i=0;
    for(i=0;i<count;i++)
    {
        list_node* pelem= pformatbufflist->tail.prev ;
        PFORMATBUFF pformatbuff=elem2entry(FORMATBUFF,self,pelem);
        list_remove(pelem);
        free_format_buff(pformatbuff);
    }
}

PSCREENBLOCK malloc_screen_block(int32_t id, int32_t x_screen, int32_t y_screen,
                                 int32_t width, int32_t height, int32_t type,
                                 PFORMATBUFF pformatbuff,
                                 char breakchar, mflags flag)
{
    x_screen=max(x_screen,0);x_screen=min(x_screen,screen_width-1);
    y_screen=max(y_screen,0);y_screen=min(y_screen,screen_height-1);
    if(x_screen+width>screen_width)
    {
        width=screen_width-x_screen;
    }
    if(y_screen+height>screen_height)
    {
        height=screen_height-y_screen;
    }
    //  TRACE1("sizeof(SCREENBLOCK):%d",sizeof(SCREENBLOCK));
    PSCREENBLOCK pscreenblock=malloc_block(flag,sizeof(SCREENBLOCK));
    //  while(1);
    pscreenblock->self.iID=id;
    pscreenblock->rect=make_rect(x_screen,   y_screen,   x_screen+width-1,   y_screen+height-1);
    pscreenblock->type=type;
    list_init(&pscreenblock->list_formatbuff);
    pscreenblock->sections=0;
    pscreenblock->words=0;
    pscreenblock->characters=0;
    pscreenblock->validline=1;
    pscreenblock->flag=flag;
    pscreenblock->breakchar=breakchar;
    if(pformatbuff)
    {
        list_push(&pscreenblock->list_formatbuff,&pformatbuff->self);
        pscreenblock->buff_width=pformatbuff->width;
        pscreenblock->buff_height=pformatbuff->height;
    }
    else
    {

        pformatbuff=malloc_format_buff(width,height,flag);
        list_push(&pscreenblock->list_formatbuff,&pformatbuff->self);
        pscreenblock->buff_width=width;
        pscreenblock->buff_height=height;
        //        add_format_buff( pscreenblock,1);
        //        pformatbuff=elem2entry(FORMATBUFF,self,
        //                               pscreenblock->list_formatbuff.head.next);
    }
    pformatbuff->p[0]=COLOR_NORMAL<<8|PALCEHOLD_CHAR;
    return pscreenblock;
}

void free_screen_block(PSCREENBLOCK pscreenblock)
{
    if(!pscreenblock) return;
    del_format_buff(pscreenblock,0);
    PFORMATBUFF pformatbuff=elem2entry(FORMATBUFF,self,list_pop(&pscreenblock->list_formatbuff));
    free_format_buff(pformatbuff);
    free_block(pscreenblock);
    pscreenblock=0;
}

void clear_screen_block(PSCREENBLOCK pscreenblock)
{
    if(!pscreenblock) return;
    del_format_buff(pscreenblock,0);
    PFORMATBUFF pformatbuff=elem2entry(FORMATBUFF,
                                       self,
                                       pscreenblock->list_formatbuff.head.next);
    memset(pformatbuff->p,0,2*pformatbuff->height*pformatbuff->width);
    pscreenblock->validline=1;
    pscreenblock->sections=0;
    pscreenblock->words=0;
    pscreenblock->characters=0;
    pscreenblock->offsetx=0;
    pscreenblock->offsety=0;

    pformatbuff->p[0]=COLOR_NORMAL<<8|PALCEHOLD_CHAR;
}

static int32_t formatstrlen(const uint16_t* s)
{
    int32_t i=0;
    while(s[i]){i++;}
    return i;
}

static bool is_screen_block_clear(PSCREENBLOCK pscreenblock)
{
    PFORMATBUFF pformatbuff=elem2entry(FORMATBUFF,
                                       self,
                                       pscreenblock->list_formatbuff.head.next);

    return ((char)(pformatbuff->p[0])==PALCEHOLD_CHAR)&&pscreenblock->validline==1;
}
static void get_n_char_point(uint16_t* parray[],int32_t isize,PSCREENBLOCK pscreenblock,
                             int32_t x_buff,int32_t y_buff)
{
    memset(parray,0,isize*4);

    list* pformatbufflist=&pscreenblock->list_formatbuff;
    PFORMATBUFF pformatbuff= 0;
    int32_t buffheight=pscreenblock->buff_height;
    int32_t buffwidth=pscreenblock->buff_width;
    int32_t buffsize=buffwidth*buffheight;
    int32_t buffindex=y_buff/buffheight;
    int32_t buffstart=y_buff*buffwidth+x_buff;
    isize=min(isize,(pscreenblock->validline*buffwidth-buffstart));
    if(isize<=0){return;}
    int32_t buffend=buffstart+isize-1;

    list_node* pelem=list_node_get_by_index(pformatbufflist, buffindex);
    if(!pelem)return;

    int i=0,j=0;
    while(buffstart<=buffend)
    {
        pformatbuff= elem2entry(FORMATBUFF,
                                self,
                                pelem);
        int32_t singlestart=buffstart%buffsize;
        int32_t singleend=buffsize-1;
        if(buffstart+(buffsize-singlestart)>buffend)
        {
            singleend=buffend%buffsize;
        }
        for(i=singlestart;i<=singleend;i++)
        {
            parray[j]=pformatbuff->p+i;
            j++;
        }
        pelem=pelem->next;
        buffstart+=singleend-singlestart+1;
    }

}
/*static void get_str_point_array(uint16_t **parray,
                                PSCREENBLOCK pscreenblock,const RECT* pRECT_buff)
{
    list* pformatbufflist=&pscreenblock->list_formatbuff;
    PFORMATBUFF pformatbuff= 0;

    int32_t buffheight=pscreenblock->buff_height;
    int32_t buffwidth=pscreenblock->buff_width;
    int32_t buffindex1=pRECT_buff->y/buffheight;
    int32_t singley1=pRECT_buff->y%buffheight;
    int32_t buffindex2=(pRECT_buff->y+pRECT_buff->height-1)/buffheight;
    int32_t singley2=(pRECT_buff->y+pRECT_buff->height-1)%buffheight;
    list_elem* pelem1=index_to_elem(pformatbufflist, buffindex1);
    list_elem* pelem2=index_to_elem(pformatbufflist, buffindex2);
    int32_t singlex1=pRECT_buff->x;
    int32_t index=0;
    ASSERT(pelem1&&pelem2);
    while(pelem1!=pelem2)
    {
        pformatbuff= elem2entry(FORMATBUFF,
                                self,
                                pelem1);
        for(;singley1<buffheight;singley1++)
        {
            int32_t tempx=singlex1;
            for(;tempx<pRECT_buff->width;tempx++)
            {
                parray[index]=pformatbuff->p+singley1*buffwidth+tempx;
                index++;
            }
        }
        singley1=0;
        pelem1=pelem1->next;
    }
    while(singley1<=singley2)
    {
        pformatbuff= elem2entry(FORMATBUFF,
                                self,
                                pelem1);
        int32_t tempx=singlex1;
        for(;tempx<pRECT_buff->width;tempx++)
        {
            parray[index]=pformatbuff->p+singley1*buffwidth+tempx;
            index++;
        }
        singley1++;
    }
}
*/

static PFORMATBUFF search_buff_pos(PSCREENBLOCK pscreenblock,int32_t* pbuffx,int32_t* pbuffy,
                                   char ch,bool bpas,bool include,bool bcheck)
{
    list* pformatbufflist=&pscreenblock->list_formatbuff;
    PFORMATBUFF pformatbuff= elem2entry(FORMATBUFF,self,pformatbufflist->head.next);
    int32_t buffindex;
    int32_t singlebuffpos;
    int32_t buffx=*pbuffx;
    int32_t buffy=*pbuffy;
    if(is_screen_block_clear(pscreenblock))
    {
        *pbuffx=*pbuffy=0;
        return pformatbuff;
    }
    int32_t buffheight=pscreenblock->buff_height;
    int32_t buffwidth=pscreenblock->buff_width;
    int32_t buffsize=buffwidth*buffheight;
    int32_t buffstart=buffy*buffwidth+buffx;

    buffindex=buffy/buffheight ;
    list_node* pelem=list_node_get_by_index(pformatbufflist, buffindex);
    pformatbuff= elem2entry(FORMATBUFF,
                            self,
                            pelem);
    int32_t startpos=buffstart%buffsize;
    if(bcheck)
    {
        if((uint8_t)(pformatbuff->p[startpos])==(uint8_t)PALCEHOLD_CHAR||
                (uint8_t)(pformatbuff->p[startpos])==(uint8_t)ch)
        {
            while(pelem!=&pformatbufflist->head)
            {
                pformatbuff= elem2entry(FORMATBUFF,self, pelem);
                for(singlebuffpos=startpos;singlebuffpos>=0;singlebuffpos--)
                {
                    if ((uint8_t)(pformatbuff->p[singlebuffpos])!=(uint8_t)PALCEHOLD_CHAR&&
                            (uint8_t)(pformatbuff->p[singlebuffpos])!=(uint8_t)ch)
                    {

                        int32_t pos=singlebuffpos+buffindex*buffsize;
                        *pbuffx=(pos)%buffwidth;
                        *pbuffy=(pos)/buffwidth;
                        return search_buff_pos(  pscreenblock,pbuffx, pbuffy, ch,
                                                 bpas, include,bcheck);
                    }
                }
                buffindex--;
                pelem=pelem->prev;
                startpos=buffsize-1;
            }
            *pbuffx=0;
            *pbuffy=0;
            return pformatbuff;
        }
    }

    if(bpas)
    {
        while(pelem!=&pformatbufflist->tail)
        {
            pformatbuff= elem2entry(FORMATBUFF,self,pelem);
            for(singlebuffpos=startpos;singlebuffpos<buffsize;singlebuffpos++)
            {
                if ( (uint8_t)(pformatbuff->p[singlebuffpos])==(uint8_t)ch||
                     (uint8_t)(pformatbuff->p[singlebuffpos])==(uint8_t)PALCEHOLD_CHAR)
                {

                    int32_t pos=singlebuffpos+buffindex*buffsize;
                    if(include)
                    {
                        *pbuffx=pos%buffwidth;
                        *pbuffy=pos/buffwidth;

                    }
                    else
                    {
                        *pbuffx=(pos-1)%buffwidth;
                        *pbuffy=(pos-1)/buffwidth;
                        if(singlebuffpos==0)
                        {
                            pelem=pelem->prev;
                            if(pelem&&pelem!=&pformatbufflist->head)
                            {
                                pformatbuff= elem2entry(FORMATBUFF,self,pelem);
                            }
                            else
                            {
                                pformatbuff=0;
                            }
                        }
                    }
                    return pformatbuff;
                }
            }
            buffindex++;
            pelem=pelem->next;
            startpos=0;
        }
        *pbuffx=(buffindex*buffsize-1)%buffwidth;
        *pbuffy=(buffindex*buffsize-1)/buffwidth;
        return pformatbuff;
    }
    else
    {
        while(pelem!=&pformatbufflist->head)
        {
            pformatbuff= elem2entry(FORMATBUFF,self,pelem);
            for(singlebuffpos=startpos;singlebuffpos>=0;singlebuffpos--)
            {
                if ( (uint8_t)pformatbuff->p[singlebuffpos]==(uint8_t)ch||
                     (uint8_t)(pformatbuff->p[singlebuffpos])==(uint8_t)PALCEHOLD_CHAR)
                {

                    int32_t pos=singlebuffpos+buffindex*buffsize;
                    if(include)
                    {
                        *pbuffx=(pos)%buffwidth;
                        *pbuffy=(pos)/buffwidth;
                    }
                    else
                    {

                        *pbuffx=(pos+1)%buffwidth;
                        *pbuffy=(pos+1)/buffwidth;
                        if(singlebuffpos==buffsize-1)
                        {
                            pelem=pelem->next;
                            if(pelem&&pelem!=&pformatbufflist->tail)
                            {
                                pformatbuff= elem2entry(FORMATBUFF,self,pelem);
                            }
                            else
                            {
                                pformatbuff=0;
                            }
                        }
                    }
                    return pformatbuff;
                }
            }
            buffindex--;
            pelem=pelem->prev;
            startpos=buffsize-1;
        }
        *pbuffx=0;
        *pbuffy=0;
        return pformatbuff;
    }
    return 0;
}

static void complete_line(PSCREENBLOCK pscreenblock,int32_t lineindex)
{
    if(lineindex>=pscreenblock->validline)return;

    mflags flag=pscreenblock->flag;
    int32_t buffwidth=pscreenblock->buff_width;
    uint16_t** buffpointarray=malloc_block(flag,buffwidth*4);
    get_n_char_point(buffpointarray,buffwidth,pscreenblock,0,lineindex);
    int i=0;
    for(i=0;i<buffwidth;i++)
    {
        ASSERT(buffpointarray[i]);
        if(!(*(buffpointarray[i])))
        {
            *(buffpointarray[i])=COLOR_NORMAL<<8|PALCEHOLD_CHAR;
        }
    }
    free_block(buffpointarray);
}

static void move_buff(PSCREENBLOCK pscreenblock,int32_t x_buff1,int32_t y_buff1,
                      int32_t x_buff2,int32_t y_buff2,int iformatsize,bool bpas)
{
    int32_t buffwidth=pscreenblock->buff_width;
    int32_t buffstart=x_buff1+y_buff1*buffwidth;
    int32_t buffend=x_buff2+y_buff2*buffwidth;

    if(buffend-buffstart+1<iformatsize)
    {
        TRACE1("MOVE_BUFF_ERR: startx:%d,starty:%d,endx:%d,endy:%d,iformatsize:%d",
               x_buff1,y_buff1,x_buff2,y_buff2,iformatsize);
        while(1);
    }
    ASSERT(buffend<pscreenblock->validline*buffwidth);
    mflags flag=pscreenblock->flag;
    uint16_t** buffpointarray=malloc_block(flag,iformatsize*4);
    int32_t i=0;
    uint16_t temp=COLOR_NORMAL<<8|PALCEHOLD_CHAR;
    if(bpas)
    {
        uint16_t* buffdataarray=malloc_block(flag,iformatsize*sizeof(uint16_t));
        get_n_char_point(buffpointarray,iformatsize,pscreenblock,
                         buffstart%buffwidth,buffstart/buffwidth);

        for(i=0;i<iformatsize;i++)
        {
            if(buffpointarray[i])
            {
                //temp=buffdataarray[i];
                buffdataarray[i]=*(buffpointarray[i]);
                *(buffpointarray[i])=temp;
            }
        }
        buffstart+=iformatsize;
        while(buffend>=buffstart)
        {
            int32_t isize1=min(iformatsize,buffend-buffstart+1);
            memset(buffpointarray,0,iformatsize*4);
            get_n_char_point(buffpointarray,isize1,pscreenblock,
                             buffstart%buffwidth,buffstart/buffwidth);

            for(i=0;i<iformatsize;i++)
            {
                if(buffpointarray[i])
                {
                    temp=buffdataarray[i];
                    buffdataarray[i]=*(buffpointarray[i]);
                    *(buffpointarray[i])=temp;
                }
            }

            buffstart+=iformatsize;
        }
        free_block(buffdataarray);
    }
    else
    {
        uint16_t** buffpointarray1=malloc_block(flag,iformatsize*4);
        get_n_char_point(buffpointarray,iformatsize,pscreenblock,
                         buffstart%buffwidth,buffstart/buffwidth);

        buffstart+=iformatsize;
        while(buffend>=buffstart)
        {
            memset(buffpointarray1,0,iformatsize*4);
            int32_t isize1=min(iformatsize,buffend-buffstart+1);
            get_n_char_point(buffpointarray1,isize1,pscreenblock,
                             buffstart%buffwidth,buffstart/buffwidth);

            for(i=0;i<iformatsize;i++)
            {
                if(buffpointarray[i])
                {
                    if(buffpointarray1[i])
                    {
                        *(buffpointarray[i])=*(buffpointarray1[i]);
                    }
                    else
                    {
                        *(buffpointarray[i])=COLOR_NORMAL<<8|PALCEHOLD_CHAR;
                    }
                }
            }
            memcpy(buffpointarray,buffpointarray1,iformatsize*4);
            buffstart+=iformatsize;
        }
        for(i=0;i<iformatsize;i++)
        {
            if(buffpointarray[i])
            {
                *(buffpointarray[i])=COLOR_NORMAL<<8|PALCEHOLD_CHAR;
            }
        }
        free_block(buffpointarray1);
    }
    free_block(buffpointarray);

}
void fresh_screen_block_rect(PSCREENBLOCK pscreenblock, int32_t x_block, int32_t y_block,
                             int32_t width, int32_t height)
{
    if(!pscreenblock) return;
    RECT rect_block=make_rect(x_block,  y_block, x_block+ width-1,  y_block+height-1);
    RECT rect_screen;
    block_to_screen(pscreenblock,&rect_block,&rect_screen,RECT_POS);
    screen_to_block(pscreenblock,&rect_screen,&rect_block,RECT_POS);

    RECT rect_buff;
    mflags flag=pscreenblock->flag;
    block_to_formatbuff(pscreenblock,&rect_block,&rect_buff,RECT_POS);

    int i=0;
    uint16_t** buffpointarray=malloc_block(flag,4);
    for(i=0;i<rect_block.height;i++)
    {
        POINT pt_block,pt_screen;
        pt_block.x=rect_block.x;
        pt_block.y=i+rect_block.y;
        block_to_screen(pscreenblock,&pt_block,&pt_screen,POINT_POS);

        if(i<rect_buff.height)
        {
            get_n_char_point(buffpointarray,1,pscreenblock,rect_buff.x,rect_buff.y+i);
            print_wordchars(pt_screen.y, pt_screen.x, *buffpointarray, rect_buff.width);
            if(rect_block.width>rect_buff.width)
            {
                print_n_wordchar(pt_screen.y, pt_screen.x+rect_buff.width,
                                 0, rect_block.width-rect_buff.width);
            }

        }
        else
        {
            print_n_wordchar(pt_screen.y, pt_screen.x,
                             0, rect_block.width);//COLOR_NORMAL<<8|'D'
        }
    }
    free_block(buffpointarray);
}

void fresh_screen_block(PSCREENBLOCK pscreenblock)
{
    if(!pscreenblock) return;
    fresh_screen_block_rect(  pscreenblock,  0, 0,
                              pscreenblock->rect.width,
                              pscreenblock->rect.height );
}
void fresh_screen_block_buffpos(PSCREENBLOCK pscreenblock, int32_t x_buff1, int32_t y_buff1,
                                int32_t x_buff2, int32_t y_buff2)
{
    if(!pscreenblock) return;
    RECT rect_block;
    RECT rect_buff=make_rect(x_buff1,  y_buff1,  x_buff2,   y_buff2);
    formatbuff_to_block(pscreenblock,&rect_buff,&rect_block,RECT_POS);
    //  TRACE2(19,"rect_block:%d,%d,%d,%d",rect_block.x,rect_block.y,rect_block.width,rect_block.height);
    fresh_screen_block_rect(pscreenblock, 0, rect_block.y, pscreenblock->rect.width, rect_block.height);
}

void fresh_screen_block_pos(PSCREENBLOCK pscreenblock, int32_t x1_screen, int32_t y1_screen,
                            int32_t x2_screen, int32_t y2_screen)
{
    if(!pscreenblock) return;
    RECT rect_screen=make_rect(x1_screen,y1_screen,x2_screen, y2_screen);
    RECT rect_block;
    screen_to_block(pscreenblock,&rect_screen,&rect_block,RECT_POS);
    rect_block.x=0;
    rect_block.width=pscreenblock->rect.width;
    fresh_screen_block_rect(pscreenblock,rect_block.x,rect_block.y,rect_block.width,rect_block.height);
}
bool point_in_rect(PRECT prect, int32_t x, int32_t y)
{
    ASSERT(prect);
    return  x>=prect->x&&
            y>=prect->y&&
            x<prect->x+prect->width&&
            y<prect->y+prect->height;
}
void fresh_screen_block_list_rect(list *plist, int32_t x_screen, int32_t y_screen,
                                  int32_t width, int32_t height)
{
    list_node* pelem=plist->head.next;
    RECT rect;
    rect.x=x_screen;
    rect.y=y_screen;
    rect.width=width;
    rect.height=height;
    while(pelem!=&plist->tail)
    {
        PSCREENBLOCK pscreenblock= elem2entry(SCREENBLOCK,
                                              self,
                                              pelem);
        POINT pttopleft,ptbottomright;
        pttopleft.x=pscreenblock->rect.x;
        pttopleft.y=pscreenblock->rect.y;
        ptbottomright.x=pscreenblock->rect.x+pscreenblock->rect.width-1;
        ptbottomright.y=pscreenblock->rect.y+pscreenblock->rect.height-1;
        if(point_in_rect(&rect,pttopleft.x,pttopleft.y)||
                point_in_rect(&rect,ptbottomright.x,ptbottomright.y)||
                point_in_rect(&pscreenblock->rect,rect.x,rect.y)||
                point_in_rect(&pscreenblock->rect,rect.x+rect.width-1,rect.y+rect.height-1)
                )
        {
            fresh_screen_block(pscreenblock);
        }
        pelem=pelem->next;
    }
}
void fresh_screen_block_list(list *plist)
{
    list_node* pelem=plist->head.next;
    while(pelem!=&plist->tail)
    {
        PSCREENBLOCK pscreenblock= elem2entry(SCREENBLOCK,
                                              self,
                                              pelem);
        fresh_screen_block(pscreenblock);
        pelem=pelem->next;
    }
}

uint16_t *buff_to_format_buff(char *p,  int32_t width, int32_t buffx, mflags flag, uint8_t color)
{
    if(!color)
    {
        color=COLOR_NORMAL;
    }
    int32_t isize=strlen(p);
    int32_t iformatsize=0;
    int32_t i=0,j=buffx;
    for(i=0;i<isize;i++)
    {
        if(j>=width)
        {
            j=0;
        }
        if(p[i]=='\n')
        {
            iformatsize+=width-j;
            j=width-1;
        }
        else
        {
            iformatsize++;
        }
        j++;
    }
    uint16_t* pformatbuff=malloc_block(flag,sizeof(uint16_t)*(iformatsize+1));

    j=buffx;
    int32_t k=0;
    for(i=0;i<isize;i++)
    {
        if(j>=width)
        {
            j=0;
        }
        if(p[i]=='\n')
        {
            int32_t l=0;
            for(l=0;l<width-j;l++)
            {
                pformatbuff[k]=color<<8|(char)PALCEHOLD_CHAR;
                k++;
            }
            j=width-1;

        }
        else
        {
            pformatbuff[k]=color<<8|p[i];
            k++;
        }
        j++;
    }
    return pformatbuff;
}

char *format_buff_to_buff(uint16_t *pformat, int32_t width,int32_t buffx ,mflags flag)
{
    int32_t iformatsize=formatstrlen(pformat);
    int32_t isize=0;
    int i=0,j=buffx;
    for(i=0;i<iformatsize;i++)
    {
        char ch=pformat[i];
        if(j>=width)
        {
            j=0;
        }
        if(ch==PALCEHOLD_CHAR)
        {
            i+=width-1-j;
            j=width-1;
        }
        isize++;j++;
    }
    char* p=malloc_block(flag,isize+1);

    j=buffx;
    int32_t k=0;
    for(i=0;i<iformatsize;i++)
    {
        if(j>=width)
        {
            j=0;
        }
        if((char)pformat[i]==PALCEHOLD_CHAR)
        {
            i+=width-1-j;
            j=width-1;
            p[k]='\n';
        }
        else
        {
            p[k]=pformat[i];
        }
        j++;k++;
    }
    return p;
}

static bool is_word_break(char p)
{
    char* str=" .,;?|";
    int i=0;
    for(i=0;str[i];i++)
    {
        if(str[i]==p)
        {
            return true;
        }
    }
    return false;
}
static void statistics_format_buff(uint16_t *pformat, int32_t iformatsize, int32_t width,int32_t buffx ,
                                   int32_t* pwords,int32_t* psections, int32_t *pcharacters)
{
    *pcharacters=0;
    *pwords=0;
    *psections=0;
    int32_t i,j;
    j=buffx;
    for(i=0;i<iformatsize;i++)
    {
        char ch=(char)pformat[i];
        if(j>=width)
        {
            j=0;
        }
        if(ch==PALCEHOLD_CHAR)
        {
            i+=width-1-j;
            j=width-1;
            if(i&&i-1<iformatsize&&(char)pformat[i-1]==PALCEHOLD_CHAR)
            {
                TRACE1("i:%d,iformatsize:%d",i,iformatsize);
                (*psections)++;
            }
        }
        else
        {
            (*pcharacters)++;
        }
        if(is_word_break(ch))
        {
            (*pwords)++;
        }
        j++;
    }
}
static void statistics_buff(char *p, int32_t isize, int32_t* pwords,int32_t* psections, int32_t *pcharacters)
{
    *pcharacters=0;
    *pwords=0;
    *psections=0;
    int32_t i;
    for(i=0;i<isize;i++)
    {
        char ch=p[i];
        if(ch=='\n')
        {
            (*psections)++;
        }
        else
        {
            (*pcharacters)++;
        }
        if(is_word_break(ch))
        {
            (*pwords)++;
        }

    }
}
char *get_buff_from_block_by_buffpos(PSCREENBLOCK pscreenblock,
                                     int32_t x1_buff, int32_t y1_buff,
                                     int32_t x2_buff, int32_t y2_buff,
                                     mflags flag)
{

    if(!pscreenblock) return 0;
    if(is_screen_block_clear(pscreenblock))
    {

        char* p= malloc_block(flag,2);

        return p;
    }

    int32_t buffwidth=pscreenblock->buff_width;

    int32_t iformatsize=y2_buff*buffwidth+x2_buff-y1_buff*buffwidth-x1_buff+1;
    ASSERT(iformatsize>0);
    uint16_t** buffpointarray=malloc_block(flag,iformatsize*4);
    get_n_char_point(buffpointarray,iformatsize,pscreenblock,x1_buff,y1_buff);
    uint16_t* pformat=malloc_block(flag,(iformatsize+1)*sizeof(uint16_t));
    int i=0;
    for(i=0;i<iformatsize;i++)
    {
        if(buffpointarray[i])
        {
            pformat[i]= *(buffpointarray[i]);
        }
        else
        {
            break;
        }
    }
    char* p=format_buff_to_buff(pformat,buffwidth,x1_buff,flag);
    //   TRACE(p);
    free_block(pformat);
    free_block(buffpointarray);
    return p;
}


char *get_buff_from_block_by_color(PSCREENBLOCK pscreenblock, uint8_t color,
                                   COLORRASTER raster, mflags flag)
{
    if(!pscreenblock) return 0;
    list* pformatbufflist=&pscreenblock->list_formatbuff;
    list_node* pelem=pformatbufflist->head.next;
    PFORMATBUFF pformatbuff= elem2entry(FORMATBUFF, self, pelem);
    int32_t buffwidth=pscreenblock->buff_width;
    int32_t buffheight=pscreenblock->buff_height;
    int32_t buffsize=  buffwidth* buffheight;
    int32_t buffindex=0;
    int32_t buffx1,buffy1,buffx2,buffy2;
    bool bexist=false ;
    while(pelem!=&pformatbufflist->tail)
    {
        pformatbuff= elem2entry(FORMATBUFF, self, pelem);
        int buffpos;
        for(buffpos=0;buffpos<buffsize;buffpos++)
        {
            uint8_t flag_select=0;
            switch(raster)
            {
            case SRCCOPY:
                flag_select= pformatbuff->p[buffpos]>>8==color;
                break;
            case SRCOR:
                flag_select= pformatbuff->p[buffpos]>>8&color;
                break;
            default:
                break;
            }
            if(flag_select)
            {
                if(!bexist)
                {
                    buffx1=(buffpos+buffindex*buffsize)%buffwidth;
                    buffy1=(buffpos+buffindex*buffsize)/buffwidth;

                }
                bexist=true;
            }
            else if(bexist)
            {
                buffx2=(buffpos+buffindex*buffsize-1)%buffwidth;
                buffy2=(buffpos+buffindex*buffsize-1)/buffwidth;
                return get_buff_from_block_by_buffpos(pscreenblock,buffx1,buffy1,buffx2,buffy2,
                                                      flag);

            }
        }
        pelem=pelem->next;
        buffindex++;
    }
    return 0;
}

char *get_buff_from_block_all(PSCREENBLOCK pscreenblock,mflags flag)
{
    if(!pscreenblock) return 0;
    int32_t buffwidth=pscreenblock->buff_width;
    char* p= get_buff_from_block_by_buffpos( pscreenblock, 0, 0,buffwidth-1,
                                             pscreenblock->validline-1,flag);
    if(p)
    {
        int32_t isize= strlen(p);
        if(p[isize-1]=='\n')
        {
            p[isize-1]=0;
        }
    }
    return p;

}

char *get_buff_from_block_by_ch(PSCREENBLOCK pscreenblock,
                                int32_t x1_buff, int32_t y1_buff,
                                int32_t x2_buff, int32_t y2_buff,
                                char ch, mflags flag)
{
    if(!pscreenblock) return 0;
    search_buff_pos(pscreenblock,&x1_buff,&y1_buff,ch,false,false,true);
    search_buff_pos(pscreenblock,&x2_buff,&y2_buff,ch,true,false,true);
    return get_buff_from_block_by_buffpos(  pscreenblock,   x1_buff,   y1_buff,
                                            x2_buff,   y2_buff,
                                            flag);

}

static void add_line_to_block(PSCREENBLOCK pscreenblock,int32_t count)
{
    if(count<=0) return;
    list* pformatbufflist=&pscreenblock->list_formatbuff;

    int32_t buffcount=list_len(pformatbufflist);
    int32_t buffheight=pscreenblock->buff_height;
    pscreenblock->validline+=count;
    if(pscreenblock->validline>buffcount*buffheight)
    {
        int32_t addbuffnum=(pscreenblock->validline-buffcount*buffheight)/buffheight+1;

        add_format_buff(pscreenblock,addbuffnum);
    }
}
void set_buff_to_block(PSCREENBLOCK pscreenblock, int32_t x_buff, int32_t y_buff, char *p,
                       uint8_t color)
{
    if(!pscreenblock||!p) return ;
    int32_t ilen=strlen(p);
    if(!ilen) {clear_screen_block(pscreenblock);return;}
    bool bclear=is_screen_block_clear(pscreenblock);
    mflags flag=pscreenblock->flag;
    int32_t buffwidth=pscreenblock->buff_width;
    int32_t words=0,sections=0,characters=0;
    statistics_buff(p,ilen,&words,&sections,&characters);
    pscreenblock->words+=words;
    pscreenblock->sections+=sections;
    pscreenblock->characters+=characters;

    uint16_t* pformat=buff_to_format_buff(p,buffwidth,x_buff,flag,color);
    int32_t iformatsize=formatstrlen(pformat);
    int32_t linecount=y_buff+(iformatsize+x_buff)/buffwidth+1;

    int32_t addlines=linecount-pscreenblock->validline;

    add_line_to_block(pscreenblock,addlines);

    uint16_t** buffpointarray=malloc_block(flag,iformatsize*4);
    get_n_char_point(buffpointarray,iformatsize,pscreenblock,x_buff,y_buff);

    uint16_t* poldformat=malloc_block(flag,sizeof(uint16_t)*(iformatsize+1));
    int32_t i;
    for(i=0;i<iformatsize;i++)
    {
        uint16_t* temp=buffpointarray[i];
        ASSERT(temp);
        poldformat[i]=*temp;
        *temp=pformat[i];
    }
    if(!bclear)
    {
        statistics_format_buff(poldformat,formatstrlen(poldformat),buffwidth,x_buff,
                               &words,&sections,&characters);
        pscreenblock->words-=words;
        pscreenblock->sections-=sections;
        pscreenblock->characters-=characters;
    }
    free_block(poldformat);
    free_block(pformat);
    free_block(buffpointarray);
    complete_line(  pscreenblock,pscreenblock->validline-1);

}

void insert_to_screen_block(PSCREENBLOCK pscreenblock, int32_t *px_buff, int32_t *py_buff,
                            char *p, uint8_t color)
{
    if(!pscreenblock||!p) return ;
    int32_t ilen=strlen(p);
    if(!ilen) {return;}
    mflags flag=pscreenblock->flag;
    int32_t buffwidth=pscreenblock->buff_width;

    int32_t words=0,sections=0,characters=0;
    statistics_buff(p,ilen,&words,&sections,&characters);
    pscreenblock->words+=words;
    pscreenblock->sections+=sections;
    pscreenblock->characters+=characters;
    uint16_t* pformat=buff_to_format_buff(p,buffwidth,*px_buff,flag,color);
    int32_t iformatsize=formatstrlen(pformat);
    int32_t buffx_section=*px_buff,buffy_section=*py_buff;

    search_buff_pos(pscreenblock,&buffx_section,&buffy_section,PALCEHOLD_CHAR,true,true,false);

    int32_t emptybuffnum=buffwidth-buffx_section;
    int32_t addlinenum=0;
    if(iformatsize>=emptybuffnum)
    {
        addlinenum=(iformatsize-emptybuffnum)/buffwidth+1;
        add_line_to_block(pscreenblock,addlinenum);
        int32_t starty=buffy_section+1;
        int32_t endy=pscreenblock->validline-1;
        move_buff(pscreenblock,0,starty,
                  buffwidth-1,endy,addlinenum*buffwidth,true);
    }
    int32_t endx=buffwidth-1;
    int32_t endy=buffy_section+addlinenum;

    move_buff(pscreenblock,*px_buff,*py_buff,
              endx,endy,iformatsize,true);

    uint16_t** buffpointarray=malloc_block(flag,(iformatsize+1)*4);
    get_n_char_point(buffpointarray,iformatsize+1,pscreenblock,*px_buff,*py_buff);
    int i=0;
    for(i=0;i<iformatsize;i++)
    {
        ASSERT(buffpointarray[i]);
        *(buffpointarray[i])=pformat[i];
    }
    int32_t buffpos= *px_buff+*py_buff*buffwidth+iformatsize;
    *px_buff=buffpos%buffwidth;
    *py_buff=buffpos/buffwidth;

    complete_line(  pscreenblock,pscreenblock->validline-1);

    free_block(pformat);
    free_block(buffpointarray);
}

void appand_to_screen_block(PSCREENBLOCK pscreenblock, char *p, uint8_t color)
{
    if(!pscreenblock) return ;
    int32_t buffwidth=pscreenblock->buff_width;
    char* p1=get_buff_from_block_by_buffpos(pscreenblock,0,pscreenblock->validline-1,
                                            buffwidth-1,pscreenblock->validline-1,
                                            pscreenblock->flag);
    ASSERT(p1);
    int32_t isize=strlen(p1);
    free_block(p1);
    int32_t x_buff=0;
    if(isize){x_buff=isize-1;}
    int32_t y_buff=pscreenblock->validline-1;
    insert_to_screen_block(  pscreenblock,   &x_buff,   &y_buff,  p,  color);
}

static void delete_line_from_block(PSCREENBLOCK pscreenblock,int32_t count)
{
    if(count<=0) return;
    list* pformatbufflist=&pscreenblock->list_formatbuff;
    int32_t buffcount=list_len(pformatbufflist);
    int32_t buffheight=pscreenblock->buff_height;
    int32_t buffwidth=pscreenblock->buff_width;
    mflags flag=pscreenblock->flag;
    uint16_t** buffpointarray=malloc_block(flag,buffwidth*4);
    int32_t i=0,j=0;
    for(i=0;i<count;i++)
    {
        get_n_char_point(buffpointarray,buffwidth,pscreenblock,0,pscreenblock->validline-i-1);

        for(j=0;j< buffwidth;j++)
        {
            ASSERT(buffpointarray[j]);
            *(buffpointarray[j])=0;
        }
    }
    free_block(buffpointarray);
    pscreenblock->validline-=count;
    int32_t delbuffnum=buffcount-pscreenblock->validline/buffheight-1;
    if(delbuffnum>0)
    {
        del_format_buff(pscreenblock,delbuffnum);
    }

}
void delete_from_screen_block(PSCREENBLOCK pscreenblock, int32_t *px1_buff,int32_t *py1_buff,
                              int32_t x2_buff, int32_t y2_buff)
{
    int32_t x1_buff=*px1_buff;
    int32_t y1_buff=*py1_buff;
    if(!pscreenblock) return ;
    int32_t words=0,sections=0,characters=0;

    mflags flag=pscreenblock->flag;
    int32_t buffwidth=pscreenblock->buff_width;
    if(is_screen_block_clear(pscreenblock)){clear_screen_block(pscreenblock); return;}

    uint16_t** buffpointarray=malloc_block(flag,buffwidth*4);
    get_n_char_point(buffpointarray,buffwidth,pscreenblock,0,y1_buff);
    if((uint8_t)(*(buffpointarray[x1_buff]))==PALCEHOLD_CHAR)
    {
        if(y1_buff==pscreenblock->validline-1) //最后一行占位符
        {
            free_block(buffpointarray);
            return;
        }
        int i=0;
        for(i=x1_buff;i>=0;i--)
        {
            if((uint8_t)(*(buffpointarray[i]))!=PALCEHOLD_CHAR)
            {
                x1_buff=i+1;
                break;
            }
        }
        if((uint8_t)(*(buffpointarray[0]))==PALCEHOLD_CHAR)
        {
            x1_buff=0;
        }
    }
    get_n_char_point(buffpointarray,buffwidth,pscreenblock,0,y2_buff);
    if((uint8_t)(*(buffpointarray[x2_buff]))==PALCEHOLD_CHAR)
    {
        x2_buff=buffwidth-1;
    }
    free_block(buffpointarray);

    int32_t iformatsize=y2_buff*buffwidth+x2_buff-(y1_buff*buffwidth+x1_buff)+1;
   // TRACE1("x1_buff:%d,y1_buff:%d,x2_buff:%d,y2_buff:%d,iformatsize:%d",x1_buff,y1_buff,
   //        x2_buff,y2_buff,iformatsize);
    buffpointarray=malloc_block(flag,iformatsize*4);
    uint16_t* buffdataarray=malloc_block(flag,(iformatsize+1)*2);
    get_n_char_point(buffpointarray,iformatsize,pscreenblock,x1_buff,y1_buff);
    int i=0;
    for(i=0;i<iformatsize;i++)
    {
        if(buffpointarray[i])
        {
            buffdataarray[i]=*(buffpointarray[i]);
        }
        else
        {
            break;
        }
    }
    statistics_format_buff(buffdataarray,iformatsize,buffwidth,x1_buff,
                           &words,&sections,&characters);
    pscreenblock->words-=words;
    pscreenblock->sections-=sections;
    pscreenblock->characters-=characters;

    free_block(buffdataarray);
    free_block(buffpointarray);

    if(x1_buff==0&&y1_buff==0&&
        x2_buff==buffwidth-1&&y2_buff==pscreenblock->validline-1)
    {
        *px1_buff=x1_buff;
        *py1_buff=y1_buff;
        clear_screen_block(pscreenblock);
        return;
    }

    int32_t buffx_section=x2_buff;
    int32_t buffy_section=y2_buff;
    int32_t sections_search_start_pos=y2_buff*buffwidth+x2_buff+1;
    if(sections_search_start_pos<pscreenblock->validline*buffwidth)
    {
        buffx_section=sections_search_start_pos%buffwidth;
        buffy_section=sections_search_start_pos/buffwidth;
        search_buff_pos(pscreenblock,&buffx_section,&buffy_section,PALCEHOLD_CHAR,true,true,false);

    }
    int32_t startx=x1_buff;
    int32_t starty=y1_buff;
    int32_t endx=buffwidth-1;
    int32_t endy=buffy_section;
   // TRACE1("startx:%d,starty:%d,endx:%d,endy:%d,iformatsize:%d",startx,starty,endx,endy,iformatsize);
    move_buff(pscreenblock,startx,starty,
              endx,endy,iformatsize,false);

    int32_t noemptybuffnum=buffx_section;
    noemptybuffnum=max(1,noemptybuffnum);   //连续两行空白行
    if(iformatsize>noemptybuffnum)
    {
        int32_t dellinenum=(iformatsize-noemptybuffnum)/buffwidth+1;

        starty=buffy_section-dellinenum+1;
        endy=pscreenblock->validline-1;
        move_buff(pscreenblock,0,starty,
                  buffwidth-1,endy,dellinenum*buffwidth,false);
        delete_line_from_block(pscreenblock,dellinenum);
    }
    complete_line(pscreenblock,pscreenblock->validline-1);
    *px1_buff=x1_buff;
    *py1_buff=y1_buff;
    fresh_screen_block(pscreenblock);
}

void select_screen_block(PSCREENBLOCK pscreenblock, int32_t x1_buff, int32_t y1_buff,
                         int32_t x2_buff, int32_t y2_buff ,uint8_t color, COLORRASTER raster)
{
    if(!pscreenblock){return ;}
    mflags flag=pscreenblock->flag;
    int32_t buffwidth=pscreenblock->buff_width;

    int32_t iformatsize=(y2_buff*buffwidth+x2_buff)-(y1_buff*buffwidth+x1_buff)+1;
    iformatsize=min(pscreenblock->validline*buffwidth-(y1_buff*buffwidth+x1_buff),iformatsize);
    if(iformatsize<=0){ return;}
    uint16_t** buffpointarray=malloc_block(flag,iformatsize*4);
    get_n_char_point(buffpointarray,iformatsize,pscreenblock,x1_buff,y1_buff);
    int32_t i=0;
    for(i=0;i<iformatsize;i++)
    {
        if(buffpointarray[i])
        {
            //  TRACE1("iformatsize:%d",iformatsize);while(1);
            uint16_t temp= *(buffpointarray[i]);
            switch(raster)
            {
            case SRCCOPY:
                *(buffpointarray[i])=color<<8|(uint8_t)temp;
                break;
            case SRCOR:
                *(buffpointarray[i])=color<<8|temp;
                break;
            default:break;
            }
        }
    }
    free_block(buffpointarray);
}


int32_t select_screen_block_by_ch(PSCREENBLOCK pscreenblock, int32_t *px1_buff, int32_t *py1_buff,
                                  int32_t *px2_buff, int32_t *py2_buff ,
                                  uint8_t color, COLORRASTER raster, char ch)
{
    if(!pscreenblock) return 0;
    mflags flag=pscreenblock->flag;
    int32_t buffwidth=pscreenblock->buff_width;
    int32_t buffx1=*px1_buff;
    int32_t buffy1=*py1_buff;
    int32_t buffx2=*px2_buff;
    int32_t buffy2=*py2_buff;

    search_buff_pos(pscreenblock,&buffx1,&buffy1,ch,false,false,true);
    search_buff_pos(pscreenblock,&buffx2,&buffy2,ch,true,false,true);
    int32_t iformatsize=(buffy2*buffwidth+buffx2)-(buffy1*buffwidth+buffx1)+1;
    iformatsize=min(pscreenblock->validline*buffwidth-(buffy1*buffwidth+buffx1),iformatsize);
    //  TRACE2(22,"iformatsize:%d",iformatsize);
    if(iformatsize<=0){return 0;}
    uint16_t** buffpointarray=malloc_block(flag,iformatsize*4);
    get_n_char_point(buffpointarray,iformatsize,pscreenblock,buffx1,buffy1);
    int32_t i=0;
    for(i=0;i<iformatsize;i++)
    {
        if(buffpointarray[i])
        {
            uint16_t temp= *(buffpointarray[i]);
            if((char)temp==ch)break;

            switch(raster)
            {
            case SRCCOPY:
                *(buffpointarray[i])=color<<8|(uint8_t)temp;
                break;
            case SRCOR:
                *(buffpointarray[i])=color<<8|temp;
                break;
            default:break;
            }
        }
    }
    free_block(buffpointarray);
    *px1_buff=buffx1;*py1_buff=buffy1;
    *px2_buff=buffx2;*py2_buff=buffy2;
    return iformatsize;
}

void unselect_screen_block_by_color(PSCREENBLOCK pscreenblock, uint8_t color,COLORRASTER raster)
{
    if(!pscreenblock) return;
    list* pformatbufflist=&pscreenblock->list_formatbuff;
    int32_t buffwidth=pscreenblock->buff_width;
    int32_t buffheight=pscreenblock->buff_height;
    int32_t buffsize=  buffwidth* buffheight;
    bool bexist=false;
    int32_t buffindex=0;
    int32_t buffx1=0;
    int32_t buffy1=0;
    int32_t buffx2=0;
    int32_t buffy2=0;
    list_node* pelem=pformatbufflist->head.next;
    PFORMATBUFF pformatbuff= 0;
    while(pelem!=&pformatbufflist->tail)
    {
        pformatbuff= elem2entry(FORMATBUFF,  self, pelem);
        int buffpos;
        for(buffpos=0;buffpos<buffsize;buffpos++)
        {
            uint16_t temp= pformatbuff->p[buffpos];
            uint16_t flag=0;

            switch(raster)
            {
            case SRCCOPY:
                flag=color==temp>>8;
                break;
            case SRCOR:
                flag=(color&temp>>8)==color;
                break;
            default:break;
            }
            if(flag)
            {
                if(!bexist)
                {
                    buffx1=(buffpos+buffindex*buffsize)%buffwidth;
                    buffy1=(buffpos+buffindex*buffsize)/buffwidth;
                }
                switch(raster)
                {
                case SRCCOPY:
                    pformatbuff->p[buffpos]=COLOR_NORMAL<<8|(char)temp;
                    break;
                case SRCOR:
                    pformatbuff->p[buffpos]&=~(color<<8);
                    break;
                default:break;
                }
                bexist=true;
            }
            else if(bexist)
            {
                buffx2=(buffpos+buffindex*buffsize-1)%buffwidth;
                buffy2=(buffpos+buffindex*buffsize-1)/buffwidth;
                fresh_screen_block_buffpos(pscreenblock,buffx1,buffy1,buffx2,buffy2);
                return;
            }
        }
        pelem=pelem->next;
        buffindex++;
    }
    if(bexist)
    {
        buffx2=(buffindex*buffsize-1)%buffwidth;
        buffy2=(buffindex*buffsize-1)/buffwidth;
        fresh_screen_block_buffpos(pscreenblock,buffx1,buffy1,buffx2,buffy2);
    }
}

void unselect_screen_block(PSCREENBLOCK pscreenblock, int32_t x1_buff, int32_t y1_buff,
                           int32_t x2_buff, int32_t y2_buff, uint8_t color, COLORRASTER raster)
{
    if(!pscreenblock) return ;
    if(is_screen_block_clear(pscreenblock))return;
    mflags flag=pscreenblock->flag;
    int32_t buffwidth=pscreenblock->buff_width;
    int32_t iformatsize=(y2_buff*buffwidth+x2_buff)-(y1_buff*buffwidth+x1_buff)+1;
    iformatsize=min(pscreenblock->validline*buffwidth-(y1_buff*buffwidth+x1_buff+1),iformatsize);
    if(iformatsize<=0) return ;
    uint16_t** buffpointarray=malloc_block(flag,iformatsize*4);
    get_n_char_point(buffpointarray,iformatsize,pscreenblock,x1_buff,y1_buff);
    int32_t i=0;
    for(i=0;i<iformatsize;i++)
    {
        if(buffpointarray[i])
        {
            int16_t temp=*(buffpointarray[i]);

            switch(raster)
            {
            case SRCCOPY:
                *(buffpointarray[i])=COLOR_NORMAL<<8|(char)temp;
                break;
            case SRCOR:
                *(buffpointarray[i])&=~(color<<8);
                break;
            default:break;
            }
        }
    }
    free_block(buffpointarray);
}

void reset_screen_block_offset(PSCREENBLOCK pscreenblock)
{
    if(pscreenblock)
    {
        pscreenblock->offsetx=pscreenblock->offsety=0;
    }
}

PSCREENBLOCK point_in_screenlock_list(list *pscreenblocklist, int32_t x_screen, int32_t y_screen,
                                      bool bforward)
{
    list_node* pelem=0;
    if(bforward)
    {
        pelem=pscreenblocklist->head.next;
        while(pelem!=&pscreenblocklist->tail)
        {
            PSCREENBLOCK pscreenblock= elem2entry(SCREENBLOCK, self, pelem);
            if(point_in_rect(&pscreenblock->rect,x_screen,y_screen))
            {
                return pscreenblock;
            }
            pelem=pelem->next;
        }

    }
    else
    {
        pelem=pscreenblocklist->tail.prev;
        while(pelem!=&pscreenblocklist->head)
        {
            PSCREENBLOCK pscreenblock= elem2entry(SCREENBLOCK, self, pelem);
            if(point_in_rect(&pscreenblock->rect,x_screen,y_screen))
            {
                return pscreenblock;
            }
            pelem=pelem->prev;
        }
    }
    return 0;
}


void ensure_buff_visible(PSCREENBLOCK pscreenblock, int32_t x_buff, int32_t y_buff)
{
    if(x_buff<pscreenblock->offsetx)
    {
        pscreenblock->offsetx=x_buff;
    }
    if(x_buff>pscreenblock->offsetx+pscreenblock->rect.width-1)
    {
        pscreenblock->offsetx=x_buff-(pscreenblock->rect.width)+1;
    }
    if(y_buff<pscreenblock->offsety)
    {
        pscreenblock->offsety=y_buff;
    }
    if(y_buff>pscreenblock->offsety+pscreenblock->rect.height-1)
    {
        pscreenblock->offsety=y_buff-(pscreenblock->rect.height)+1;
    }
}

void adjust_screen_block_buffpos(PSCREENBLOCK pscreenblock, int32_t *px_buff, int32_t *py_buff)
{
    if(is_screen_block_clear(pscreenblock))
    {
        *px_buff=*py_buff=0;
    }
    else
    {
        *px_buff=max(0,*px_buff);
        *px_buff=min(*px_buff,pscreenblock->buff_width-1);
        *py_buff=max(0,*py_buff);
        *py_buff=min(*py_buff,pscreenblock->validline-1);
    }
}

void adjust_screen_block_offset(PSCREENBLOCK pscreenblock)
{
    pscreenblock->offsetx=min(pscreenblock->offsetx,
                              pscreenblock->buff_width-pscreenblock->rect.width);
    pscreenblock->offsetx=max(0,pscreenblock->offsetx);
    pscreenblock->offsety=min(pscreenblock->offsety,
                              pscreenblock->validline-pscreenblock->rect.height);
    pscreenblock->offsety=max(0,pscreenblock->offsety);
}

PSCREENBLOCK get_screenblock_by_id(int32_t id,list *plist)
{
    list_node* pelem=list_node_get_by_id(plist,id);
    if(pelem)
    {
        return elem2entry(SCREENBLOCK,self,pelem);
    }
    return 0;
}

void get_screen_size(int32_t *pwidth, int32_t *pheight)
{
    *pwidth=screen_width;
    *pheight=screen_height;
}

void free_sblock_list(list *psblocklist)
{
    ASSERT(psblocklist);
    while(psblocklist->head.next!=&psblocklist->tail)
    {
        PSCREENBLOCK pScreenblock=elem2entry(SCREENBLOCK,self,list_pop(psblocklist));
        free_screen_block(pScreenblock);
    }
}

void clear_and_set_block(list *psblocklist,int32_t blockID,char* p,uint8_t color)
{
    PSCREENBLOCK pscreenblock= get_screenblock_by_id(blockID,psblocklist);
    clear_screen_block(pscreenblock);
    if(p)
    {
        set_buff_to_block(pscreenblock,0,0,p,color);
    }
    fresh_screen_block(pscreenblock);
}
