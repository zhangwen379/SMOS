[bits 32]
section .text
;--------------------------------------------
;print_str 在屏幕特定的位置显示字符串，并设定颜色
;--------------------------------------------

global print_str
print_str:
	pushad
	mov ebp,esp
	mov al,[ebp+36]
	mov bl,80
	mul bl
	add ax,[ebp+40]
	mov bx,ax

   xor ecx, ecx		      ; 准备用ecx存储参数,清空
   mov edx, [ebp + 44]	      ; 从栈中得到待打印的字符串地址
.goon:
   mov cl, [edx]
   cmp cl, 0		      ; 如果处理到了字符串尾,跳到结束处返回
   jz .str_over

   shl bx, 1				  ; 字符数据用2字节表示,将显示位置乘2,表示对应显存中的偏移字节
   mov [gs:bx], cl			  ; ascii字符本身
   inc bx
   mov al,[ebp + 48]
   mov byte [gs:bx],al	; 字符颜色和背景颜色
   shr bx, 1				  ; 恢复显示位置
   inc bx				  ; 下一个显示位置

   inc edx		      ; 使ebx指向下一个字符
   jmp .goon
.str_over:
 ;  mov ecx, [ebp + 52]
 ;  cmp ecx, 0
   jz .print_over

.print_over:
   mov esp,ebp
   popad
   ret

;--------------------------------------------
;print_wordchars 在屏幕特定的位置打印字符数组，并设定颜色
;--------------------------------------------
global print_wordchars
print_wordchars:
	pushad
	mov ebp,esp
	mov al,[ebp+36]
	mov bl,80
	mul bl
	add ax,[ebp+40]
	mov bx,ax

   xor ecx, ecx		      ; 准备用ecx存储参数,清空
   mov edx, [ebp + 44]	      ; 从栈中得到待打印的字符串地址
   mov ecx, [ebp + 48]
   cmp ecx, 0
   jz .print_over
.goon:
   shl bx, 1				  ;  字符数据用2字节表示,将显示位置乘2,表示对应显存中的偏移字节
   mov ax ,[edx]
   mov byte [gs:bx], al			  ; ascii字符本身
   inc bx
   mov byte [gs:bx],ah	; 字符颜色和背景颜色
   shr bx, 1				  ; 恢复显示位置
   inc bx				  ; 下一个显示位置

   add edx,2		      ; 使edx指向下一个字符
   loop .goon
.print_over:
   mov esp,ebp
   popad
   ret

;--------------------------------------------
;print_n_wordchar在屏幕特定的位置打印n个单个字符，并设定颜色
;--------------------------------------------
global print_n_wordchar
print_n_wordchar:
    pushad
	mov ebp,esp
    mov al,[ebp+36]
    mov bl,80
    mul bl
    add ax,[ebp+40]
    mov bx,ax

   xor ecx, ecx		      ; 准备用ecx存储参数,清空
   mov dx, [ebp + 44]	      ; 从栈中得到待打印的字符串地址
   mov ecx, [ebp + 48]
   cmp ecx, 0		    ; 如果处理到了字符串尾,跳到结束处返回
   jz .print_over
.goon:
   shl bx, 1				  ;  字符数据是用2字节表示,将显示位置乘2,表示对应显存中的偏移字节
   mov ax ,dx
   mov byte [gs:bx], al			  ; ascii字符本身
   inc bx
   mov byte [gs:bx],ah	; 字符颜色和背景颜色
   shr bx, 1				  ; 恢复显示位置
   inc bx				  ; 下一个显示位置

   loop .goon
.print_over:
   mov esp,ebp
   popad
   ret

;--------------------------------------------
;get_wordchars 从屏幕获取特定个数的字符
;--------------------------------------------
global get_wordchars
get_wordchars:
    pushad
	mov ebp,esp
    mov al,[ebp+36]
    mov bl,80
    mul bl
    add ax,[ebp+40]
    mov bx,ax

    xor eax, eax
    xor ecx, ecx		      ; 准备用ecx存储参数,清空
    mov edx, [ebp + 44]	      ; 从栈中得到待打印的字符串地址
    mov ecx, [ebp + 48]
    cmp ecx, 0
    jz .get_over
 .goon:
	shl bx, 1				  ;  字符数据用2字节表示,将显示位置乘2,表示对应显存中的偏移字节
     mov byte al,[gs:bx] 			  ; ascii字符本身
     inc bx
     mov byte ah,[gs:bx]
     mov [edx] ,ax

	 shr bx, 1				  ; 恢复显示位置
	 inc bx				  ; 下一个显示位置
	 inc edx
	 inc edx		      ; 使edx指向下一个2字节字符数据
     loop .goon
 .get_over:
    mov esp,ebp
    popad
    ret

;--------------------------------------------
;clear_screen 清除特定整个屏幕
;--------------------------------------------

global clear_screen
clear_screen:
	pushad
	mov ebx, 0
	mov ecx, 80*25
	.cls:
	mov word [gs:ebx], 0x0000
	add ebx, 2
	loop .cls
	mov ebx, 0

	popad
	ret
