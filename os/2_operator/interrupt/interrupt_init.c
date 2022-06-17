#include "interrupt_init.h"
#include "interrupt_timer.h"
#include "interrupt_keyboard.h"
#include "interrupt_syscall.h"
#include "trace.h"
#include "io.h"
#include "global_def.h"
#include "array_str.h"
#include "kernel.h"
#include "print.h"
#define IDT_DESC_CNT 0x81           // 中断描述符项数
//#define EFLAGS_IF   0x00000200       // eflags寄存器中的if位为1
//#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0" : "=g" (EFLAG_VAR))
//
// // 获取当前中断状态
//bool is_intr_enabled()
//{
//    uint32_t eflags = 0;
//    GET_EFLAGS(eflags);
//    return (EFLAGS_IF & eflags) ? true : false;
//}
////////////////////////////////////////////////////////////硬件中断芯片初始化
// 可编程中断控制器是8259A
#define IC8259A_0_A0 0x20	       // 主片的偶端口是0x20
#define IC8259A_0_A1 0x21	       // 主片的奇端口是0x21
#define IC8259A_1_A0 0xa0	       // 从片的偶端口是0xa0
#define IC8259A_1_A1 0xa1	       // 从片的奇端口是0xa1

// 8259A初始化
static void interrupt_IC8259A_init(void)
{
    // 初始化主片
    outb (IC8259A_0_A0, 0x11);   // ICW1: 边沿触发,级联8259, 需要ICW4.
    outb (IC8259A_0_A1, 0x20);   // ICW2: 起始中断向量号为0x20,也就是IR[0-7] 为 0x20 ~ 0x27.
    outb (IC8259A_0_A1, 0x04);   // ICW3: IR2接从片.
    outb (IC8259A_0_A1, 0x01);   // ICW4: 8086模式, 手动EOI

    // 初始化从片
    outb (IC8259A_1_A0, 0x11);    // ICW1: 边沿触发,级联8259, 需要ICW4.
    outb (IC8259A_1_A1, 0x28);    // ICW2: 起始中断向量号为0x28,也就是IR[0-7] 为 0x28 ~ 0x2F.
    outb (IC8259A_1_A1, 0x02);    // ICW3: 设置从片连接到主片的IR2引脚
    outb (IC8259A_1_A1, 0x01);    // ICW4: 8086模式, 手动EOI

    // 打开中断引脚
    outb (IC8259A_0_A1, 0xf8);    // OCW1:主片
    outb (IC8259A_1_A1, 0xed);    // OCW1:从片
}
////////////////////////////////////////////////////////////中断描述表初始化
char* intrrupt_Cfun_name[IDT_DESC_CNT];                     //C语言中断处理函数名定义
intr_handler intrrupt_Cfun_table[IDT_DESC_CNT];             //C语言中断处理函数数组定义
extern intr_handler intrrupt_Afun_table[0x30];             //汇编中断处理函数数组声明
extern void intrrupt_syscall_handler(void);                 //系统调用汇编中断处理函数声明

struct idt_desc idt_desc_array[IDT_DESC_CNT];               // 中断描述符表

//中断返回栈结构体
struct intr_stack
{
    uint32_t ip_param;	 //汇编中断处理函数入栈参数，用于默认C语言中断处理函数
    uint32_t cs_param;
    uint32_t vec_no_param;

	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
    uint32_t esp;       // esp会被重新赋值
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

    uint32_t err_code;	// err_code在eip之后入栈
	void (*eip) (void);
	uint32_t cs;
	uint32_t eflags;
};
static void intr_reset(void)
{
	struct intr_stack* pstack=(struct intr_stack*)(0x003ffffc-sizeof(struct intr_stack));
    pstack->ip_param=0;         //汇编中断处理函数入栈参数，用于默认C语言中断处理函数，中断返回时未使用。
    pstack->cs_param=0;
    pstack->vec_no_param=0;
	pstack->edi=0;
	pstack->esi=0;
	pstack->ebp=0;
	pstack->esp=0;
	pstack->ebx=0;
	pstack->edx=0;
	pstack->ecx=0;
	pstack->eax=0;
    pstack->gs=SELECTOR_GS;
    pstack->fs=SELECTOR_DATA;
    pstack->es=SELECTOR_DATA;
    pstack->ds=SELECTOR_DATA;

	pstack->err_code=0;		 // err_code会被压入在eip之后
	pstack->eip=do_Msg;
    pstack->cs=SELECTOR_CODE;
    pstack->eflags=EFLAGS_MBS|EFLAGS_IF_1;
	asm volatile ("movl %0, %%esp; jmp intr_exit" : : "a" (pstack));
}
// 默认的C语言中断处理函数,一般用在NMI中断的处理
static void intrrupt_general_handler(uint8_t vec_nr,uint32_t cs,uint32_t ip)
{
    if (vec_nr == 0x27 || vec_nr == 0x2f)
    {
        // 8259A上的最后一个irq引脚
        //IRQ7和IRQ15会产生伪中断(spurious interrupt),无法通过OCW屏蔽,但无须处理。
        return;
    }
    static int iIntr_Count=0;
    iIntr_Count++;
    char ch[40]={0};
    sprintf(ch,39,"!%x:%x-%d,%d/%s",cs,ip,iIntr_Count,vec_nr,intrrupt_Cfun_name[vec_nr]);
    print_str(1,33,ch,0x4f);
    //  while(1);
	intr_reset();
}
// 初始化C语言中断处理函数数组及C语言中断处理函数函数名数组
static void Cfun_init(void)
{
	int i;
	for (i = 0; i < IDT_DESC_CNT; i++)
	{
        intrrupt_Cfun_table[i] = intrrupt_general_handler;
        intrrupt_Cfun_name[i] = "External interrupt";
	}
    intrrupt_Cfun_name[0x0] = "#DE Divide Error";
    intrrupt_Cfun_name[0x1] = "#DB Debug Exception";
    intrrupt_Cfun_name[0x2] = "External NMI Interrupt";
    intrrupt_Cfun_name[0x3] = "#BP Breakpoint Exception";
    intrrupt_Cfun_name[0x4] = "#OF Overflow Exception";
    intrrupt_Cfun_name[0x5] = "#BR BOUND Range Exceeded Exception";
    intrrupt_Cfun_name[0x6] = "#UD Invalid Opcode Exception";
    intrrupt_Cfun_name[0x7] = "#NM Device Not Available Exception";
    intrrupt_Cfun_name[0x8] = "#DF Double Fault Exception";
    intrrupt_Cfun_name[0x9] = "Coprocessor Segment Overrun";
    intrrupt_Cfun_name[0xA] = "#TS Invalid TSS Exception";
    intrrupt_Cfun_name[0xB] = "#NP Segment Not Present";
    intrrupt_Cfun_name[0xC] = "#SS Stack Fault Exception";
    intrrupt_Cfun_name[0xD] = "#GP General Protection Exception";
    intrrupt_Cfun_name[0xE] = "#PF Page-Fault Exception";
    intrrupt_Cfun_name[0xF] = "#15 NMI reserved";
    intrrupt_Cfun_name[0x10] = "#MF x87 FPU Floating-Point Error";
    intrrupt_Cfun_name[0x11] = "#AC Alignment Check Exception";
    intrrupt_Cfun_name[0x12] = "#MC Machine-Check Exception";
    intrrupt_Cfun_name[0x13] = "#XF SIMD Floating-Point Exception";
    intrrupt_Cfun_name[0x14] = "#20 NMI reserved";
    intrrupt_Cfun_name[0x15] = "#21 NMI reserved";
    intrrupt_Cfun_name[0x16] = "#22 NMI reserved";
    intrrupt_Cfun_name[0x17] = "#23 NMI reserved";
    intrrupt_Cfun_name[0x18] = "#24 NMI reserved";
    intrrupt_Cfun_name[0x19] = "#25 NMI reserved";
    intrrupt_Cfun_name[0x1A] = "#26 NMI reserved";
    intrrupt_Cfun_name[0x1B] = "#27 NMI reserved";
    intrrupt_Cfun_name[0x1C] = "#28 NMI reserved";
    intrrupt_Cfun_name[0x1D] = "#29 NMI reserved";
    intrrupt_Cfun_name[0x1E] = "#30 NMI reserved";
    intrrupt_Cfun_name[0x1F] = "#31 NMI reserved";
}

//中断描述符表项初始化
static void make_idt_desc_item(struct idt_desc* p_gdesc, intr_handler function)
{
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_CODE;
    p_gdesc->attribute = IDT_DESC_ATTR;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

//中断描述表初始化
static void make_idt_desc(void)
{
    int i;
    for (i = 0; i < 0x30; i++)
    {
        make_idt_desc_item(idt_desc_array+i,intrrupt_Afun_table[i]);
    }
    make_idt_desc_item(idt_desc_array+0x80, intrrupt_syscall_handler);
}
///////////////////////////////////////////////////////////////////////////中断初始化
void intrrupt_all_init(void)
{
	console_output("interrupt init idt...");
    console_output("interrupt init idt function...");
    Cfun_init();	   // 初始化C语言中断处理函数组
    console_output("interrupt init syscall...");
    interrupt_syscall_init(); // 初始化系统调用
    make_idt_desc();	   // 初始化中断描述符表


    console_output("interrupt init extern interrupt...");
	console_output("interrupt init timer...");
    interrupt_timer_init( );    // 初始化定时器中断
	console_output("interrupt init keyboard...");
    interrupt_keyboard_init( );// 初始化鼠标键盘中断
    console_output("interrupt init 8259A...");
    interrupt_IC8259A_init();        // 初始化硬件中断芯片

	// 加载idt
	console_output("interrupt lidt...");
    uint64_t idt_register = ((sizeof(idt_desc_array) - 1) |
                            ((uint64_t)(uint32_t)idt_desc_array << 16));
    asm volatile("lidt %0" : : "m" (idt_register));

	sti() ;
    fresh_keyboard_state( );
}
// 中断开关开
void  sti(void)
{
    asm volatile("sti");
}
// 中断开关关
void  cli(void)
{
    asm volatile("cli");
}
//注册C语言中断处理函数
void register_handler(uint8_t vector_no, intr_handler function)
{
    intrrupt_Cfun_table[vector_no] = function;
}
//程序调试断言非挂起处理
void panic_reset(char* filename, int line, const char* func, const char* condition)
{
    char p[40]={0};
    sprintf(p,39,"ASSERT:%s;%d;%s;%s",func,line,filename,condition);
    print_str(1,33,p,0x4f);
	intr_reset();
}
