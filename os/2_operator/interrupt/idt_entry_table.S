[bits 32]
%define ERROR_CODE nop		 ; 若IEU在调用某些NMI中断函数入栈错误码,这里不做操作.
%define ZERO push 0		 ; 若IEU在调用某些NMI中断函数或外部硬件触发函数未入栈错误码,则入栈0.
extern intrrupt_Cfun_table	 ;intr_Cfun_table是C语言中断处理程序数组声明

section .data
global intrrupt_Afun_table
intrrupt_Afun_table:
%macro VECTOR 2
section .text
intr%1entry:                     ; 汇编中断处理函数名
   %2				 ; 入栈错误码
                                 ; 以下是保存上下文环境
   push ds
   push es
   push fs
   push gs
   pushad			 ; 中断以汇编指令作为原子操作
                                 ;所以需要保存辅助寄存器，
                                 ;PUSHAD指令压入32位寄存器
                                 ;其入栈顺序是: EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI

   ;发送中断结束命令EOI,防止C语言中断处理程序数无法使用NMI中断
   mov al,0x20
   out 0xa0,al                   ; 向从片发送
   out 0x20,al                   ; 向主片发送

    mov eax, [esp+13*4]
    mov ebx, [esp+14*4]
    push eax                     ;入栈参数2 ip 默认C语言中断处理函数使用
    push ebx                     ;入栈参数1 cs 默认C语言中断处理函数使用
    push %1			 ;入栈中断向量号 vec_nr 默认C语言中断处理函数使用

    cli;
    call [intrrupt_Cfun_table + %1*4] ; 调用C语言中断处理程序数
    sti;
    jmp intr_exit
section .data
   dd    intr%1entry            ; 制作汇编中断处理函数地址数组
%endmacro

section .text
global intr_exit
intr_exit:	     
; 以下是恢复上下文环境
   add esp, 4			   ; 出栈中断向量号 vec_nr
   add esp, 4			   ; 出栈参数1 cs
   add esp, 4			   ; 出栈参数2 ip
   popad
   pop gs
   pop fs
   pop es
   pop ds
   add esp, 4			   ; 跳过error_code
   iretd

;宏展开后，会在特定节区编译成入口函数代码以及入口函数地址数组
VECTOR 0x00,ZERO    ;NMI中断
VECTOR 0x01,ZERO
VECTOR 0x02,ZERO
VECTOR 0x03,ZERO 
VECTOR 0x04,ZERO
VECTOR 0x05,ZERO
VECTOR 0x06,ZERO
VECTOR 0x07,ZERO 
VECTOR 0x08,ERROR_CODE
VECTOR 0x09,ZERO
VECTOR 0x0a,ERROR_CODE
VECTOR 0x0b,ERROR_CODE 
VECTOR 0x0c,ZERO
VECTOR 0x0d,ERROR_CODE
VECTOR 0x0e,ERROR_CODE
VECTOR 0x0f,ZERO 
VECTOR 0x10,ZERO
VECTOR 0x11,ERROR_CODE
VECTOR 0x12,ZERO
VECTOR 0x13,ZERO 
VECTOR 0x14,ZERO
VECTOR 0x15,ZERO
VECTOR 0x16,ZERO
VECTOR 0x17,ZERO 
VECTOR 0x18,ERROR_CODE
VECTOR 0x19,ZERO
VECTOR 0x1a,ERROR_CODE
VECTOR 0x1b,ERROR_CODE 
VECTOR 0x1c,ZERO
VECTOR 0x1d,ERROR_CODE
VECTOR 0x1e,ERROR_CODE
VECTOR 0x1f,ZERO 
VECTOR 0x20,ZERO    ;第一个硬件中断
VECTOR 0x21,ZERO
VECTOR 0x22,ZERO
VECTOR 0x23,ZERO
VECTOR 0x24,ZERO
VECTOR 0x25,ZERO
VECTOR 0x26,ZERO
VECTOR 0x27,ZERO
VECTOR 0x28,ZERO
VECTOR 0x29,ZERO
VECTOR 0x2a,ZERO
VECTOR 0x2b,ZERO
VECTOR 0x2c,ZERO
VECTOR 0x2d,ZERO
VECTOR 0x2e,ZERO
VECTOR 0x2f,ZERO    ;最后一个硬件中断

;;;;;;;;;;;;;;;;0x80号 系统调用 汇编中断处理函数;;;;;;;;;;;;;;;;
[bits 32]
extern syscall_table
section .text
global intrrupt_syscall_handler
intrrupt_syscall_handler:
;1 保存上下文环境
   push 0			; IEU执行软件中断指令未入栈错误码，因此入栈0

   push ds
   push es
   push fs
   push gs
   pushad

   push 0                       ;入栈参数2 ip 默认C语言中断处理函数使用
   push 0                       ;入栈参数1 cs 默认C语言中断处理函数使用
   push 0x80	                ;入栈中断向量号 vec_nr 默认C语言中断处理函数使用                               ;sti
   mov ebp ,esp
;向系统调用内核函数传入参数
   cmp ecx, 0
   jz .push_over
   shl ecx,2
.push_arg:
    sub ecx,4
    mov ebx ,edx
    add ebx, ecx
    push dword [ebx]
    cmp ecx, 0
    jnz .push_arg
.push_over:
   call [syscall_table + eax*4]	    ; 调用内核函数
   mov esp, ebp			    ; 恢复调用前堆栈
   mov [esp +7*4+3*4], eax           ; 保存内核函数返回值(eax)至eax入栈位置（pushad）,
                                     ; 换言之，更改中断执行前eax寄存器值
                                     ; 供系统调用触发函数（syscall）使用
   jmp intr_exit		    ; 调用intr_exit返回，执行中断前指令语句
