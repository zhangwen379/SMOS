; 配置保护模式
	section protect_set vstart=PROTECT_SET_BASE_ADDR
    %include "boot.inc"
    jmp loader_start
;定义gdt段及段选择子
	GDT_BASE:			dd    0x00000000
						dd    0x00000000
	INSTRUCT_DESC:		dd    DESC_INSTRUCT_LOW_32BIT
						dd    DESC_INSTRUCT_HIGH_32BIT
	DATA_STACK_DESC:	dd    DESC_DATA_LOW_32BIT
						dd    DESC_DATA_HIGH_32BIT
	VIDEO_DESC:			dd    DESC_VIDEO_LOW_32BIT
						dd    DESC_VIDEO_HIGH_32BIT
    GDT_SIZE	equ	$ - GDT_BASE
    GDT_LIMIT	equ	GDT_SIZE -1

	SELECTOR_INSTRUCT   equ 1<<3|0b<<2|00b
	SELECTOR_DATA   equ 2<<3|0b<<2|00b
	SELECTOR_VIDEO  equ 3<<3|0b<<2|00b
    ;以下是定义gdt的指针，前2字节是gdt界限，后4字节是gdt起始地址
    gdt_ptr	dw  GDT_LIMIT
		dd  GDT_BASE
    msg_GetMemory    db "Get memory size:"
    count_GetMemory  equ $ - msg_GetMemory
    msg_MemoryErr    db "Err:can't get memory size"
    count_MemoryErr  equ $ - msg_MemoryErr
    msg_ProtectMode    db "Enter protect mode..."
    count_ProtectMode  equ $ - msg_ProtectMode
	msg_SetupPage	db "Setup page...",0
	msg_kernel		db "Load Kernel...",0
    hexstring	    db "0x00000000"
    times 0x200-($-$$) db 0
	total_mem_bytes dd 0x0000000;   PROTECT_SET_BASE_ADDR+0x200
    line	db 1
    ards_buf times 20 db 0	;记录内存结构ards结构体


loader_start:

	mov	 sp, PROTECT_SET_BASE_ADDR

;设置显示字符串
    mov  dh, [line]
    mov  dl, 0
    mov  bx, 0xf
    mov  ax,msg_GetMemory
    mov bp, ax
    mov cx, count_GetMemory
    mov ax, 0x1301
    int 0x10
    mov ax,[line]
    inc ax
    mov [line],ax


;获取内存容量
    call    get_memory
    jmp	    protect_mode

get_memory:
    mov di, ards_buf	      ;ards结构缓冲区
	xor ebx,ebx
.e820_mem_get_loop:	      ;循环获取每个ARDS内存范围描述结构，并保存内存最大值
	;-------  int 15h eax = 0x0000E820 ,edx = 0x534D4150 ('SMAP') 获取内存布局  -------
    mov eax, 0x534d4150
	mov edx, eax
	mov eax, 0x0000e820
    mov ecx, 20		      ;ARDS地址范围描述符结构大小是20字节
    int 0x15
	jc .error_hlt	      ;若cf位为1则有错误发生，
	cmp ebx, 0		      ;若ebx为0且cf不为1,这说明ards已从头开始索引
	jz .mem_get_ok

    mov edx, ards_buf
    mov eax, [edx]	      ;base_add_low
	add eax, [edx+8]	  ;length_low
    mov edx, [total_mem_bytes]
	cmp edx, eax
	jge .e820_mem_get_loop
	mov [total_mem_bytes], eax
    jmp .e820_mem_get_loop

.error_hlt:
    mov  dh, [line]
    mov  dl, 0
    mov  bx, 0xf
    mov  ax,msg_MemoryErr
    mov bp, ax
    mov cx, count_MemoryErr
    mov ax, 0x1301
    int 0x10
    mov ax,[line]
    inc ax
    mov [line],ax
    jmp $

 .mem_get_ok:
	mov edx, ards_buf
	mov eax, [edx]	      ;base_add_low
	add eax, [edx+8]	  ;length_low
	mov edx, [total_mem_bytes]
	cmp edx, eax
	jge .mem_get_finish
	mov [total_mem_bytes], eax
.mem_get_finish:
    mov  eax,[total_mem_bytes]
    call print_numhex
    ret

print_numhex:
    mov bx,hexstring+9
	mov cl,0
.print_loop:
    shr eax,cl
	mov cl,4
    mov dl,al
    and dl,0xf
    or dl,0x30
    cmp dl,0x39        ;比较高四位是否小于9
    jbe .print_dig     ;小于9则跳转，不需要转化为字母的ASCII码
    add dl,7           ;运行到此处说明高四位为A-Z，加7转化为对应的ASCII码
.print_dig:
	mov  [bx],dl
    sub bx,1
    cmp byte [bx],'x'
	jnz .print_loop
    mov  dh, [line]
    mov  dl, 0
    mov  bx, 0xf
    mov  ax,hexstring
    mov bp, ax
    mov cx, 10
    mov ax, 0x1301
    int 0x10

    mov ax,[line]
    inc ax
    mov [line],ax
    ret



protect_mode:

;设置显示字符串
    mov  dh, [line]
    mov  dl, 0
    mov  bx, 0xf
    mov  ax,msg_ProtectMode
    mov bp, ax
    mov cx, count_ProtectMode
    mov ax, 0x1301
    int 0x10

    mov ax,[line]
    inc ax
    mov [line],ax
;保护模式后，bios中断将不可用
    cli
;打开A20
    in al,0x92
	or al,00000010b
    out 0x92,al

;加载GDT
    lgdt [gdt_ptr]

;cr0第0位置1
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

	jmp dword SELECTOR_INSTRUCT:p_mode_start    ;刷新ecs寄存器


[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov esp,PROTECT_SET_BASE_ADDR
	mov ax, SELECTOR_VIDEO
	mov gs, ax
    mov esi,msg_SetupPage
    call print_line
    ; 创建页目录及页表并初始化页内存位图
	call setup_page

    ; 把页目录地址赋给cr3
    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

    ; 打开cr0的pg位(第31位)
    mov eax, cr0
    or eax, 0x80000000
	mov cr0, eax

	mov esp, 0x00400000;

    mov esi,msg_kernel
    call print_line

    call kernel_init
;	jmp $
	jmp dword SELECTOR_INSTRUCT:KERNEL_ENTRY_POINT ;

	;-----------------   将kernel.bin中的程序段拷贝到编译的地址   -----------
    kernel_init:
    xor eax, eax
	xor ebx, ebx
	xor ecx, ecx
	xor edx, edx

	mov ebx, [KERNEL_BASE_ADDR + 28]   ;文件头结构体+28字节e_phoff(程序段描述表文件偏移量，4字节)
	add ebx, KERNEL_BASE_ADDR
	mov dx, [KERNEL_BASE_ADDR + 42]	   ;文件头结构体+42字节e_phentsize（程序段结构体尺寸，2字节）
	mov cx, [KERNEL_BASE_ADDR + 44]    ;文件头结构体+44字节e_phnum(程序段描述表项数，2字节)
	.each_segment:
	cmp byte [ebx + 0], PT_LOAD		  ;程序头结构体+0字节p_type（程序段类型，4字节），不等于 PT_LOAD,说明此程序段不用加载。
	jne .PT_NA

	;调用memcpy函数,参数是从右往左压入，调用者清理，函数原型cdecl memcpy(dst,src,size)
	push dword [ebx + 16]		  ; 第三个参数，程序段结构体+16字节p_filesz（文件尺寸，4字节）
	mov eax, [ebx + 4]			  ; 第二个参数，程序段结构体+4字节p_offset（文件偏移量，4字节）
	add eax, KERNEL_BASE_ADDR
	push eax
	push dword [ebx + 8]		  ;第一个参数:程序段结构体+8字节p_vaddr（加载地址，4字节）
	call mem_cpy
	add esp,12				      ; 清理栈参数
	.PT_NA:
	add ebx, edx
    loop .each_segment
    ret

	;----------  逐字节拷贝 mem_cpy(dst,src,size) ------------
    mem_cpy:
	cld
	push ecx		   ; rep指令用到了ecx，但ecx对于外层段的循环还有用，故先入栈备份
	mov edi, [esp + 8]	   ; dst
	mov esi, [esp + 12]	   ; src
	mov ecx, [esp + 16]	   ; size
    rep movsb		   ; 逐字节拷贝

    ;恢复环境
	pop ecx
    ret

    ;-------------   创建一级页表及二级页表   ---------------
    ;并将底层4M空间物理地址映射为同样的虚拟地址，以保证加载位置正常
    ;创建底层1G一级页表项，以保证在不同的进程共享底层1G的空间
    setup_page:
	;先把一级页表及256二级页表占用的空间逐字节清0
    mov ecx, 4096*(256+1)
    mov esi, 0
    .clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir
    ;开始创建一级页表第0项和1023项(PTE1)
	mov eax, PAGE_DIR_TABLE_POS | PG_VALID  ;USER=1,RW=1,P=1
	mov [PAGE_DIR_TABLE_POS + 4092], eax	; 使最后一级页表项指向一级页表自己的地址，页表项是4个字节所以4095-4=4092
    add  eax, 0x1000
	mov [PAGE_DIR_TABLE_POS + 0x0], eax      ; 设置第0项一级页表项

	;下面创建第0项二级页表所有项(PTE2),实现4M内存地址完整映射
	mov ebx, PAGE_DIR_TABLE_POS+0x1000
    mov esi, 0
	mov edx, PG_VALID
    .create_pte2:
	cmp esi,767
	je .no_set
	mov [ebx+esi*4],edx			 ; 此时的ebx第0项二级页表物理地址，edx页表项(包括映射地址和有效标志)
	.no_set:
	add edx,0x1000
	inc esi
	cmp esi,1024				; 页表项1024个页地址，寻址范围为1024×4K=4M
	jne .create_pte2

	;创建内核其他255一级页表项PTE1，以保证内核程序的物理地址保持一致
	mov eax, PAGE_DIR_TABLE_POS+0x2000;  此时eax为第1项二级页表的物理地址
	or eax,	PG_VALID  ; 页目录项的属性US,RW和P位都为1
	mov ebx, PAGE_DIR_TABLE_POS
    mov esi, 1
    .create_kernel_pte1:
    mov [ebx+esi*4], eax
    inc esi
    add eax, 0x1000
	cmp esi,256
	jne  .create_kernel_pte1
    ret




print_line:
	pushad
	mov al,[line]   ;line行
    mov bl,80
    mul bl
	add ax,0		;0列
    mov bx,ax

   xor ecx, ecx		  ; 准备用ecx存储参数,清空
   mov edx, esi	      ; 从 esi中得到待打印的字符串地址

.goon:
   mov cl, [edx]
   cmp cl, 0		      ; 如果处理到了字符串尾,跳到结束处返回
   jz .str_over

   shl bx, 1				  ; 字符数据是用2字节表示,将显示位置乘2,表示对应屏幕映射内存中的偏移字节
   mov [gs:bx], cl			  ; ascii字符本身
   inc bx
   mov al,0xf
   mov byte [gs:bx],al	; 字符颜色和背景颜色
   shr bx, 1				  ; 恢复显示位置
   inc bx				  ; 下一个显示位置

   inc edx		      ; 使ebx指向下一个字符
   jmp .goon
.str_over:

    mov ax,[line]
    inc ax
    mov [line],ax
   popad
   ret
