;-protect_set.bin和kernel.bin保存硬盘，读取内存，加载内存地址----

PROTECT_SET_BASE_ADDR equ 0x900
PROTECT_SET_START_SECTOR equ 70

KERNEL_BASE_ADDR equ 0x9000
KERNEL_START_SECTOR equ 75

KERNEL_ENTRY_POINT equ 0x0022a500

;-------------   页表地址   ----------------
PAGE_DIR_TABLE_POS equ 0x100000	;1M

;----------------   页表相关属性    --------------
PG_VALID	equ 111b	; 页目录项的属性US,RW和P位都为1

;--------------   gdt描述符属性 -----------

DESC_INSTRUCT_LOW_32BIT		equ  0x0000<<16|0xFFFF
DESC_DATA_LOW_32BIT			equ  0x0000<<16|0xFFFF
DESC_VIDEO_LOW_32BIT		equ  0x8000<<16|0x0000	;limit=(0xbffff-0xb8000+1)/0x1000-1=0x7,只使用了4000

DESC_INSTRUCT_HIGH_32BIT	equ	0x00<< 24|1b<<23|1b<<22|0b<<21|0b<<20|0xf<<16|1b<<15|00b<<13|1b<<12|1000b<<8|0x00
DESC_DATA_HIGH_32BIT		equ	0x00<< 24|1b<<23|1b<<22|0b<<21|0b<<20|0xf<<16|1b<<15|00b<<13|1b<<12|0010b<<8|0x00
DESC_VIDEO_HIGH_32BIT		equ	0x00<< 24|1b<<23|1b<<22|0b<<21|0b<<20|0x0<<16|1b<<15|00b<<13|1b<<12|0010b<<8|0x0b

;-------------  program type 定义   --------------
PT_LOAD equ 1

