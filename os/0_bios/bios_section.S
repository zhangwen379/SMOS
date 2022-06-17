;主引导程序
SECTION bios_section vstart=0x7c00
%include "boot.inc"
jmp load
;   times  0x2	db 0
;   times  0x58	db 0
 load:
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00

;清除屏幕显示
    mov     ax, 0x600
    mov     bx, 0x700
	mov     cx, 0      ; 左上角: (0, 0)
	mov     dx, 0x184f ; 右下角: (80,25)
    int     0x10
;清除光标
    mov cx,2000h
    mov ah,01h
	int 10h

;显示MBR加载指示字符串
    mov  dh, 0
    mov  dl, 0
    mov  bx, 0xf
    mov  ax,msg_MBR
	mov bp, ax		    ; es:bp 为串地址
	mov cx, count_MBR	; cx 为串长度
	mov ax, 0x1301	    ; 子功能号0x13属性01
	int 0x10		    ; 执行BIOS 0x10号中断

;加载保护模式设置程序
	mov eax,PROTECT_SET_START_SECTOR	 ; 起始扇区lba地址
	mov ebx,PROTECT_SET_BASE_ADDR      ; 写入的地址
	mov ecx,4		 ; 待读入的扇区数
    call rd_disk_int13

;将内核程序读入内存
	mov ebx, KERNEL_BASE_ADDR
	mov eax, KERNEL_START_SECTOR  ; kernel.bin所在的扇区号
	mov ecx, 300		; 读入的扇区数
    call rd_disk_int13

;跳转到保护模式设置程序
	jmp PROTECT_SET_BASE_ADDR

;通过int 0x13读取硬盘
rd_disk_int13:
	push es
 ;   push edx
 .start_read:
	mov edx,ebx
	xor dx,dx
	shr edx,4
	mov es,dx
	pushad
    mov bp,sp
    push dword 0x0
    push eax
    push es
    push bx
	push dword 0x00010010
    mov ah ,0x42
    mov dl, 0x80
    mov	si,sp
    int 0x13
	mov sp,bp
    popad
    jnc .success

;显示硬盘读取错误信息
    mov  dh, 1
    mov  dl, 0
    mov  ax,msg_DiskErr
    mov bp, ax
    mov cx, count_DiskErr
    mov ax, 0x1301
    int 0x10

    hlt
.success:
    add ebx ,0x200
    inc eax
    dec ecx
    jnz .start_read
 ;   pop edx
	pop es
	ret

;数据缓存
	msg_DiskErr db " MBR Err: disk load failed";
	count_DiskErr equ $ - msg_DiskErr
	msg_MBR db "MBR load..."
	count_MBR equ $ - msg_MBR
    times 510-($-$$) db 0
    db 0x55,0xaa
