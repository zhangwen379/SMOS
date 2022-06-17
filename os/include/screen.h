#ifndef __SMOS_SCREEN_H
#define __SMOS_SCREEN_H
#include "stdint.h"
#include "list.h"
#include "memory.h"
#define COLOR_NORMAL    0x0f
#define COLOR_WARNING   0x4f
#define COLOR_SUCCESS   0x3f
#define MOUSE_CHAR      0x6f00
#define PALCEHOLD_CHAR     0x07//'*'
#define COLOR_SELECT_LEFT    0x20
#define COLOR_SELECT_RIGHT    0x10
#define COLOR_SELECT_BLINK    0x80

#define SBLOCK_STATIC       0
#define SBLOCK_INPUT        1
#define SBLOCK_OUTPUT       2
#define SBLOCK_INOUTPUT     3
extern int32_t screen_width;
extern int32_t screen_height;
typedef struct _tag_rect
{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
}RECT,*PRECT;
typedef struct _tag_point
{
    int32_t x;
    int32_t y;
}POINT,*PPOINT;
typedef struct _size
{
    int32_t width;
    int32_t height;
}SIZE,*PSIZE;

typedef struct _format_buff
{
    list_node self;
    int32_t width;
    int32_t height;
    uint16_t* p;
    mflags flag;
}FORMATBUFF,*PFORMATBUFF;

typedef struct _screen_block
{
    list_node self;
    RECT rect;
    int32_t type;

    int32_t offsetx;
    int32_t offsety;
    int32_t validline;
    list list_formatbuff;
    int32_t words;
    int32_t sections;
    int32_t characters;
    mflags flag;
    bool invisible;
    char breakchar;
    bool bwantreturn;
    int32_t buff_width;
    int32_t buff_height;
}SCREENBLOCK,*PSCREENBLOCK;

typedef enum _color_raster
{
    SRCCOPY,
    SRCOR,
}COLORRASTER;
typedef enum _tag_sblock_pos_type
{
    POINT_POS=0,
    RECT_POS
}POS_TYPE;
RECT make_rect( int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void screen_to_block(PSCREENBLOCK pscreenblock,const void* pin_screen,void* pout_block,uint8_t pos_type);
void block_to_formatbuff(PSCREENBLOCK pscreenblock,const void* pin_block,void* pout_buff,uint8_t pos_type);
void screen_to_formatbuff(PSCREENBLOCK pscreenblock,const void* pin_screen,void* pout_buff,uint8_t pos_type);

void formatbuff_to_block(PSCREENBLOCK pscreenblock,const void* pin_buff,void* pout_block,uint8_t pos_type);
void block_to_screen(PSCREENBLOCK pscreenblock,const void* pin_block,void* pout_screen,uint8_t pos_type);
void formatbuff_to_screen(PSCREENBLOCK pscreenblock,const void* pin_buff,void* pout_screen,uint8_t pos_type);


PFORMATBUFF malloc_format_buff( int32_t width, int32_t height, mflags flag );
void free_format_buff( PFORMATBUFF pformatbuff );
PSCREENBLOCK malloc_screen_block(int32_t id, int32_t x_screen, int32_t y_screen, int32_t width, int32_t height,
                                 int32_t type, PFORMATBUFF pformatbuff, char breakchar, mflags flag
                                 );

void free_screen_block(PSCREENBLOCK pscreenblock);
void clear_screen_block(PSCREENBLOCK pscreenblock);

void fresh_screen_block_rect(PSCREENBLOCK pscreenblock, int32_t x_block, int32_t y_block, int32_t width, int32_t height);
void fresh_screen_block(PSCREENBLOCK pscreenblock);
void fresh_screen_block_buffpos(PSCREENBLOCK pscreenblock, int32_t x_buff1, int32_t y_buff1, int32_t x_buff2, int32_t y_buff2);

void fresh_screen_block_pos(PSCREENBLOCK pscreenblock,int32_t x1_screen,int32_t y1_screen,int32_t x2_screen,int32_t y2_screen);

void fresh_screen_block_list_rect(list* plist,int32_t x_screen,int32_t y_screen,int32_t width,int32_t height);
void fresh_screen_block_list(list* plist);

uint16_t *buff_to_format_buff(char* p, int32_t width, int32_t buffx, mflags flag, uint8_t color);
char* format_buff_to_buff(uint16_t* pformat, int32_t width, int32_t buffx, mflags flag);

char* get_buff_from_block_by_buffpos(PSCREENBLOCK pscreenblock, int32_t x1_buff, int32_t y1_buff, int32_t x2_buff, int32_t y2_buff, mflags flag);
char* get_buff_from_block_by_color(PSCREENBLOCK pscreenblock, uint8_t color, COLORRASTER raster, mflags flag);
char* get_buff_from_block_all(PSCREENBLOCK pscreenblock, mflags flag);
char* get_buff_from_block_by_ch(PSCREENBLOCK pscreenblock,
                                int32_t x1_buff, int32_t y1_buff,
                                int32_t x2_buff, int32_t y2_buff ,
                                char ch,
                                mflags flag);

void set_buff_to_block(PSCREENBLOCK pscreenblock, int32_t x_buff, int32_t y_buff, char* p, uint8_t color);

void insert_to_screen_block(PSCREENBLOCK pscreenblock, int32_t* px_buff, int32_t* py_buff, char *p, uint8_t color);
void appand_to_screen_block(PSCREENBLOCK pscreenblock,char *p,uint8_t color);
void delete_from_screen_block(PSCREENBLOCK pscreenblock, int32_t *px1_buff, int32_t *py1_buff, int32_t x2_buff, int32_t y2_buff);

void select_screen_block(PSCREENBLOCK pscreenblock,  int32_t x1_buff, int32_t y1_buff,  int32_t x2_buff, int32_t y2_buff ,uint8_t color,COLORRASTER raster);
int32_t select_screen_block_by_ch(PSCREENBLOCK pscreenblock,  int32_t* px1_buff, int32_t* py1_buff, int32_t* px2_buff, int32_t* py2_buff , uint8_t color,COLORRASTER raster, char ch);

void unselect_screen_block_by_color(PSCREENBLOCK pscreenblock,uint8_t color,COLORRASTER raster);
void unselect_screen_block(PSCREENBLOCK pscreenblock,  int32_t x1_buff, int32_t y1_buff,  int32_t x2_buff, int32_t y2_buff ,uint8_t color,COLORRASTER raster);

void reset_screen_block_offset(PSCREENBLOCK pscreenblock);
bool point_in_rect(PRECT prect,int32_t x, int32_t y);
PSCREENBLOCK point_in_screenlock_list(list* pscreenblocklist,int32_t x_screen, int32_t y_screen,bool bforward);

void ensure_buff_visible(PSCREENBLOCK pscreenblock,int32_t x_buff, int32_t y_buff);
void adjust_screen_block_buffpos(PSCREENBLOCK pscreenblock,int32_t* px_buff, int32_t* py_buff);
void adjust_screen_block_offset(PSCREENBLOCK pscreenblock);

PSCREENBLOCK get_screenblock_by_id(int32_t id,list* plist );
void get_screen_size(int32_t* pwidth,int32_t* pheight);

void free_sblock_list(list *psblocklist);


void clear_and_set_block(list *psblocklist, int32_t blockID, char* p, uint8_t color);
#endif  //__SMOS_SCREEN_H
