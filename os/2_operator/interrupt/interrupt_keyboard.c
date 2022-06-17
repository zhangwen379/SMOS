#include "interrupt_keyboard.h"
#include "interrupt_init.h"
#include "trace.h"
#include "array_str.h"
#include "queue.h"
#include "kernel.h"
#include "process.h"
#include "io.h"
//8042两个端口，0x60负责数据输入输出，0x64负责命令与状态。
#define IC8042_OUTPUT_PORT              0x60
#define IC8042_INPUT_PORT               0x60
#define IC8042_STATUS_PORT              0x64
#define IC8042_COMMAND_PORT             0x64

#define IC8042_COMMAND_WRITE            0x60
#define IC8042_COMMAND_MOUSE            0xd4

#define IC8042_INPUT_INDICATORS         0xed
#define IC8042_INPUT_INDICATORS_NUM     0x02

#define IC8042_INPUT_CONFIG             0x47 //0b01000111
#define IC8042_INPUT_ENABLE_MOUSE       0xf4 //使能鼠标数据报告
#define IC8042_INPUT_SET_MOUSE_RATE     0xf3
#define IC8042_INPUT_GET_MOUSE_TYPE     0xf2

#define IC8042_OUTPUT_ACK               0xfa
#define IC8042_STATUS_COMM_FULL         0x02
#define IC8042_STATUS_OUT_FULL         0x01

//////////////////////////////////////////////////8042命令操作函数
static bool is_IC8042_status_comm_full(void)
{
    uint8_t status=inb(IC8042_STATUS_PORT);
    bool flag=status&IC8042_STATUS_COMM_FULL;
    return flag;
}
static void wait_IC8042_status_comm_empty(void)
{
    while(is_IC8042_status_comm_full());
}
static bool is_IC8042_output_ack(void)
{
    uint8_t data=inb(IC8042_OUTPUT_PORT);
    bool flag=data&IC8042_OUTPUT_ACK;
    return flag;
}
static void wait_IC8042_output_ack(void)
{
    while(!is_IC8042_output_ack());
}
static void mouse_command(uint8_t input)
{
    wait_IC8042_status_comm_empty();
    outb(IC8042_COMMAND_PORT,IC8042_COMMAND_MOUSE);
    wait_IC8042_status_comm_empty();
    outb(IC8042_INPUT_PORT,input);
    wait_IC8042_output_ack();
}
static void write_command(uint8_t input)
{
    wait_IC8042_status_comm_empty();
    outb(IC8042_COMMAND_PORT,IC8042_COMMAND_WRITE);
    wait_IC8042_status_comm_empty();
    outb(IC8042_INPUT_PORT,input);
    wait_IC8042_output_ack();
}
////////////////////////////////////////////////////// 键盘中断处理程序
//中键盘扫描码通码单字节数据值为索引的SMOS消息输入字符串二维数组（字符串长度3以内）
//其中字符串长度为1的字符串直接作为键盘输入ASC码在编辑框中使用
char* keyboard_to_msgstr[][2] =
{
    // shift_on  shift_off
    {0,	0},                                // 0x00
    {"ESC",	"ESC"},						   // 0x01
    {"1",	"!"},                          // 0x02
    {"2",	"@"},                          // 0x03
    {"3",	"#"},                          // 0x04
    {"4",	"$"},                          // 0x05
    {"5",	"%"},                          // 0x06
    {"6",	"^"},                          // 0x07
    {"7",	"&"},                          // 0x08
    {"8",	"*"},                          // 0x09
    {"9",	"("},                          // 0x0A
    {"0",	")"},                          // 0x0B
    {"-",	"_"},                          // 0x0C
    {"=",	"+"},                          // 0x0D
    {"BKS", "BKS"},                        // 0x0E BackSpace
    {"TAB",	"TAB"},                        // 0x0F Tab
    {"q",	"Q"},                          // 0x10
    {"w",	"W"},                          // 0x11
    {"e",	"E"},                          // 0x12
    {"r",	"R"},                          // 0x13
    {"t",	"T"},                          // 0x14
    {"y",	"Y"},                          // 0x15
    {"u",	"U"},                          // 0x16
    {"i",	"I"},                          // 0x17
    {"o",	"O"},                          // 0x18
    {"p",	"P"},                          // 0x19
    {"[",	"{"},                          // 0x1A
    {"]",	"}"},                          // 0x1B
    {"\n",  "\n"},//  {"ENT",  "ENT"},     // 0x1C Enter
    {"LCT", "LCT"},                        // 0x1D Left Control
    {"a",	"A"},                          // 0x1E
    {"s",	"S"},                          // 0x1F
    {"d",	"D"},                          // 0x20
    {"f",	"F"},                          // 0x21
    {"g",	"G"},                          // 0x22
    {"h",	"H"},                          // 0x23
    {"j",	"J"},                          // 0x24
    {"k",	"K"},                          // 0x25
    {"l",	"L"},                          // 0x26
    {";",	":"},                          // 0x27
    {"\"",	"'"},                          // 0x28
    {"`",	"~"},                          // 0x29
    {"LSF", "LSF"},                        // 0x2A Left Shift
    {"\\",	"|"},                          // 0x2B
    {"z",	"Z"},                          // 0x2C
    {"x",	"X"},                          // 0x2D
    {"c",	"C"},                          // 0x2E
    {"v",	"V"},                          // 0x2F
    {"b",	"B"},                          // 0x30
    {"n",	"N"},                          // 0x31
    {"m",	"M"},                          // 0x32
    {",",	"<"},                          // 0x33
    {".",	">"},                          // 0x34
    {"/",	"?"},                          // 0x35
    {"RSF", "RSF"},                        // 0x36 Right Shift
    {"*",	"*"},                          // 0x37
    {"LAT", "LAT"},                        // 0x38 Left Alt
    {" ",	" "},                          // 0x39
    {"CAP", "CAP"},                        // 0x3A Caps
    {"F1", "F1"},                          // 0x3B
    {"F2", "F2"},                          // 0x3C
    {"F3", "F3"},                          // 0x3D
    {"F4", "F4"},                          // 0x3E
    {"F5", "F5"},                          // 0x3F
    {"F6", "F6"},                          // 0x40
    {"F7", "F7"},                          // 0x41
    {"F8", "F8"},                          // 0x42
    {"F9", "F9"},                          // 0x43
    {"F10", "F10"},                        // 0x44
    {"NLK","NLK"},                         // 0x45 Number Lock
    {"SLK","SLK"},                         // 0x46 Screen Lock
    {"HOM","HOM"},                         // 0x47 Home
    {"UP","UP"},                           // 0x48 Up
    {"PUP","PUP"},                         // 0x49 Page up
    {"-","-"},                             // 0x4A
    {"LFT","LFT"},                         // 0x4B Left
    {0,0},                                 // 0x4C
    {"RGT","RGT"},                         // 0x4D Right
    {"+","+"},                             // 0x4E
    {"END","END"},                         // 0x4F End
    {"DWN","DWN"},                         // 0x50 Down
    {"PDN","PDN"},                         // 0x51 Page Down
    {"INS","INS"},                         // 0x52 Insert
    {"DEL","DEL"},                         // 0x53 Delete
    {0,0},                          	   // 0x54
    {0,0},                          	   // 0x55
    {0,0},                          	   // 0x56
    {"F11", "F11"},                        // 0x57
    {"F12", "F12"},                        // 0x58
    {"PSC","PSC"},                         // 0x59 //extra Print Screen
    {"PAU","PAU"},                         // 0x5A //extra Pause
};
//以数字键盘扫描码通码单字节数据值-0x47为索引的SMOS消息输入字符串一维数组,非e0状态使用
char* padboard_to_msgstr[] =
{
    "7", //0x47
    "8", //0x48
    "9", //0x49
    "-", //0x4a
    "4", //0x4b
    "5", //0x4c
    "6", //0x4d
    "+", //0x4e
    "1", //0x4f
    "2", //0x50
    "3", //0x51
};

enum keycode_state
{
    code_pause_break,   //0xe11d45e19dc5
    code_print_screen,  //0xe02ae037/0xe0b7e0aa
    code_e0,            //0xe0+byte
    code_normal,
};
enum keycode_state code_state=code_normal;  //单次按键操作中断数据状态
uint64_t keycode_buff=0;                    //单次按键操作中断数据缓冲值
uint16_t keycode_buff_times=0;              //单次按键操作中断数据缓冲次数
uint32_t keyboard_state_config=0;           //ctrl alt shift（左右）caps状态
uint8_t  keycode_byte_data=0;               //键盘单字节扫描通码
void fresh_keyboard_state(void)
{
    uint8_t indicate=0;
    if(keyboard_state_config&keyboard_screenLock)
    {
        indicate|=0x01;
    }
    if(keyboard_state_config&keyboard_numsLock)
    {
        indicate|=0x02;
        set_keyboard_indicate_screenblock(IDS_NUMSLOCK,true);
    }
    else
    {
        set_keyboard_indicate_screenblock(IDS_NUMSLOCK,false);
    }
    if(keyboard_state_config&keyboard_capsLock)
    {
        indicate|=0x04;
        set_keyboard_indicate_screenblock(IDS_CAPSLOCK,true);
    }
    else
    {
        set_keyboard_indicate_screenblock(IDS_CAPSLOCK,false);
    }
    if(keyboard_state_config&keyboard_insert)
    {
        set_keyboard_indicate_screenblock(IDS_INSERT,true);
    }
    else
    {
        set_keyboard_indicate_screenblock(IDS_INSERT,false);
    }
    wait_IC8042_status_comm_empty();
    outb(IC8042_INPUT_PORT,IC8042_INPUT_INDICATORS);
    wait_IC8042_output_ack();
    outb(IC8042_INPUT_PORT,indicate);
    wait_IC8042_output_ack();
}
static void reset_keycode_buff_state(void)
{
    keycode_buff_times=0;
    keycode_buff=0;
    code_state=code_normal;
}
bool keyInsert=false; //Insert按键会输出Insert键盘扫描码以及上一次键盘扫描码
static void keypadcode_to_msg(void)
{
    if(keyInsert)
    {
        keyInsert=false;
        return;
    }
    uint8_t index=0;
    bool flagcaps=(keyboard_state_config&keyboard_capsLock)?true:false;
    bool flagshift=((keyboard_state_config&keyboard_l_shift)
                    |(keyboard_state_config&keyboard_r_shift))?true:false;
    bool flagNumslock=(keyboard_state_config&keyboard_numsLock)?true:false;
    if(flagcaps!=flagshift)
    {
        index=1;
    }
    char* pstr=0;
    if(code_state!=code_e0&&flagNumslock&&
            keycode_byte_data>=0x47&&keycode_byte_data<=0x51)
    {
        pstr=padboard_to_msgstr[keycode_byte_data-0x47];
    }
    if(!pstr)
    {
        pstr=keyboard_to_msgstr[keycode_byte_data][index];
    }
    if(pstr)
    {
        MESSAGE msg=make_message(-msg_keyboard, pstr, 0,keyboard_state_config);
        msgq_pushmsg( &m_gKCB.msgQueue,msg);
        TRACE1("%s",pstr );
    }
    reset_keycode_buff_state();
}

static void interrupt_keyboard_handler(void)
{
    // 必须要读取输出缓冲区寄存器,否则8042不再继续响应键盘中断
    uint8_t scanCode=inb(IC8042_OUTPUT_PORT);
//    static int iline=10;
//    TRACE2(iline,"0x%x",scanCode);if(iline<24){iline++;}else{ iline=10; }
    if(scanCode==IC8042_OUTPUT_ACK) return;
    if(0xe1==scanCode&&!keycode_buff_times)
    {
        keycode_buff=0xe1;
        code_state=code_pause_break;
        keycode_buff_times=1;
        return;
    }
    else if(0xe0==scanCode&&!keycode_buff_times)
    {
        keycode_buff=0xe0;
        code_state=code_e0;
        keycode_buff_times=1;
        return;
    }
    if(code_state==code_pause_break)
    {
        if(keycode_buff_times<5)
        {
            keycode_buff=(keycode_buff<<8)|scanCode;
            keycode_buff_times++;
            return;
        }
        else if(keycode_buff_times==5)
        {
            reset_keycode_buff_state();
            keycode_buff_times=0;
            keycode_buff=(keycode_buff<<8)|scanCode;
            if(keycode_buff==0xe11d45e19dc5)
            {
                keycode_byte_data=0x5A;
                keypadcode_to_msg();
            }
            else
            {
                reset_keycode_buff_state();
            }
        }
        else
        {
            reset_keycode_buff_state();
        }
    }
    else if(code_state==code_e0)
    {
        keycode_buff=(keycode_buff<<8)|scanCode;
        keycode_buff_times++;
        switch(keycode_buff)
        {
        case 0xe02a:
        case 0xe0b7: code_state=code_print_screen; return;
        case 0xe038: keyboard_state_config|=keyboard_r_alt; break;
        case 0xe01d: keyboard_state_config|=keyboard_r_ctrl;break;
        case 0xe0b8: keyboard_state_config&=~keyboard_r_alt;break;
        case 0xe09d: keyboard_state_config&=~keyboard_r_ctrl;break;
        case 0xe052:
        {
            keyboard_state_config^=keyboard_insert;
            keyInsert=true;
            fresh_keyboard_state( );
        }
            break;
        default:
            if(scanCode<0x80)
            {
                //   keycode_byte_data=scanCode;
                //   keycode_bytedata_to_msg();
            }
            else
            {
                keycode_byte_data=scanCode-0x80;
                keypadcode_to_msg();
            }
            break;
        }
        reset_keycode_buff_state();
        return;
    }
    else if(code_state==code_print_screen)
    {
        if(keycode_buff_times<3)
        {
            keycode_buff=(keycode_buff<<8)|scanCode;
            keycode_buff_times++;
            return;
        }
        else if(keycode_buff_times==3)
        {
            reset_keycode_buff_state();
            keycode_buff=(keycode_buff<<8)|scanCode;
            if(keycode_buff==0xe02ae037)
            {
                //    keycode_byte_data=0x59;
                //    keycode_bytedata_to_msg();
            }
            else if(keycode_buff==0xe0b7e0aa)
            {
                keycode_byte_data=0x59;
                keypadcode_to_msg();
            }
            else
            {
                reset_keycode_buff_state();
            }
        }
        else
        {
            reset_keycode_buff_state();
        }
    }
    else if(code_state==code_normal)
    {
        switch(scanCode)
        {
        case 0x3a: break;
        case 0xba:
            keyboard_state_config^=keyboard_capsLock;
            fresh_keyboard_state( );
            break;
        case 0x45: break;
        case 0xc5:
            keyboard_state_config^=keyboard_numsLock;
            fresh_keyboard_state( );
            break;
        case 0x46: break;
        case 0xc6:
            keyboard_state_config^=keyboard_screenLock;
            fresh_keyboard_state( );
            break;
        case 0x2a: keyboard_state_config|=keyboard_l_shift;break;
        case 0xaa: keyboard_state_config&=~keyboard_l_shift;break;
        case 0x36: keyboard_state_config|=keyboard_r_shift;break;
        case 0xb6: keyboard_state_config&=~keyboard_r_shift;break;
        case 0x1d: keyboard_state_config|=keyboard_l_ctrl;break;
        case 0x9d: keyboard_state_config&=~keyboard_l_ctrl;break;
        default:
            if(scanCode<0x80)
            {
                //    keycode_byte_data=scanCode;
                //    keycode_bytedata_to_msg();
            }
            else
            {
                keycode_byte_data=scanCode-0x80;
                keypadcode_to_msg();
            }
            break;
        }
    }
    else
    {
        reset_keycode_buff_state();
    }
    return;
}
uint8_t mouse_type=0;
int32_t mouse_byte_x=0;
int32_t mouse_byte_y=0;
uint16_t mouse_byte_status=0;
uint8_t mouse_byte_count=1;
int32_t mouse_move_devide=10;
uint8_t mouse_move_count=1;
static void interrupt_mouse_handler(void)
{
    int8_t mouseCode=inb(IC8042_OUTPUT_PORT);
  //  static int iline=10;
  //  TRACE2(iline,"0x%x",mouseCode);if(iline<24){iline++;}else{ iline=10; }
    switch(mouse_byte_count)
    {
    case 1:
    {
        mouse_byte_status<<=8;
        mouse_byte_status|=mouseCode;
        mouse_byte_count=2;
        break;
    }
    case 2:
        mouse_byte_x+=mouseCode;
        mouse_byte_count=3;
        break;
    case 3:
    {
        mouse_byte_y+=mouseCode;
        //TRACE1("MOUSE:x:%d,y:%d,status:%x ",
        //mouse_byte_x,mouse_byte_y,mouse_byte_status);

        int8_t offsetx=0;
        int8_t offsety=0;
        if(!(mouse_move_count%mouse_move_devide))
        {
            mouse_move_count=1;
            offsetx=mouse_byte_x/mouse_move_devide;
            offsety=-mouse_byte_y/mouse_move_devide;
            mouse_byte_x=0;
            mouse_byte_y=0;
            set_mouse(offsetx,offsety);
        }

        uint8_t flag=mouse_byte_status;
        flag^=mouse_byte_status>>8;
        flag&=0x7;
        if(offsetx||offsety||flag)
        {
            MESSAGE msg;
            msg= make_message(-msg_mouse,
                              (void*)((uint32_t)mouse_byte_status<<16|m_gKCB.mouse.coverchar),
                              (void**)((uint32_t)(uint16_t)m_gKCB.mouse.x<<16|m_gKCB.mouse.y),
                              ((uint8_t)offsetx<<8)|(uint8_t)offsety);
            msgq_putmsg( &m_gKCB.msgQueue,msg);

        }
        mouse_move_count++;
        if(mouse_type==0)
        {
            mouse_byte_count=1;
        }
        else
        {
            mouse_byte_count=4;
        }
        break;
    }
    case 4:
    {
        TRACE2(21,"mouseCode:%d ",mouseCode);
        if(mouse_type==0x04)
        {
            if(mouseCode&0x08)
            {
                mouseCode|=0xf0;
            }
            else
            {
                mouseCode&=0x0f;
            }
        }
        mouse_byte_count=1;
        if(mouseCode)
        {
            MESSAGE msg;
            mouse_byte_status&=~0x0008;
            msg= make_message(-msg_mouse,
                              (void*)((uint32_t)mouse_byte_status<<16|m_gKCB.mouse.coverchar),
                              (void**)((uint32_t)(uint16_t)m_gKCB.mouse.x<<16|m_gKCB.mouse.y),
                              ((uint8_t)mouseCode<<8)|(uint8_t)mouseCode);
            msgq_putmsg( &m_gKCB.msgQueue,msg);
            mouse_byte_status|=0x08;

        }

        break;
    }
    default:
    {
        TRACE1("mouse_byte_count:%d,mouse_move_count:%d",mouse_byte_count,mouse_move_count);
        mouse_byte_count=1;
        break;
    }
    }
    return;
}

void interrupt_keyboard_init(void)
{
    keycode_buff=0;
    keycode_buff_times=0;
    keyboard_state_config=keyboard_numsLock;
    keycode_byte_data=0;
    code_state=code_normal;
    keyInsert=false;

    mouse_byte_x=0;
    mouse_byte_y=0;
    mouse_byte_status=0;
    mouse_byte_count=1;
    mouse_move_devide=10;
    mouse_move_count=1;
    mouse_type=0;

    mouse_command(IC8042_INPUT_ENABLE_MOUSE);
#ifdef _DEBUG
    mouse_command(IC8042_INPUT_SET_MOUSE_RATE);
    mouse_command(0xC8);
    mouse_command(IC8042_INPUT_SET_MOUSE_RATE);
    mouse_command(0xC8);//0x64
    mouse_command(IC8042_INPUT_SET_MOUSE_RATE);
    mouse_command(0x50);
    mouse_command(0xF2);
    mouse_type= inb(IC8042_OUTPUT_PORT);
    TRACE1("%x",mouse_type);
    if(!mouse_type)
    {
        mouse_command(IC8042_INPUT_SET_MOUSE_RATE);
        mouse_command(0xC8);
        mouse_command(IC8042_INPUT_SET_MOUSE_RATE);
        mouse_command(0x64);
        mouse_command(IC8042_INPUT_SET_MOUSE_RATE);
        mouse_command(0x50);
        mouse_command(0xF2);
        mouse_type= inb(IC8042_OUTPUT_PORT);
        TRACE1("%x",mouse_type);
    }
#endif
    register_handler(0x21, interrupt_keyboard_handler);
    register_handler(0x2c, interrupt_mouse_handler);
    write_command(IC8042_INPUT_CONFIG);//IC8042_INPUT_CONFIG

}
