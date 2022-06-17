BUILD_DIR = bin
ENTRY_POINT =  0x0022a500
AS = nasm
CC = gcc
LD = ld 
ASFLAGS = -f elf
VPATH = 0_bios: 1_data:	\
		1_data/memory:\
		1_data/disk:\
		1_data/screen: \
		1_data/struct:\
		2_operator/interrupt: \
		2_operator: \
		3_process:\
		4_system: \
		debug:  \

CFLAGS = -Wall -I include/ -c -fno-builtin -W -Wstrict-prototypes \
	 -Wmissing-prototypes -Wsystem-headers
LDFLAGS = -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/interrupt_timer.o $(BUILD_DIR)/interrupt_keyboard.o \
        $(BUILD_DIR)/interrupt_init.o $(BUILD_DIR)/idt_entry_table.o $(BUILD_DIR)/print.o \
		$(BUILD_DIR)/array_str.o $(BUILD_DIR)/queue.o $(BUILD_DIR)/trace.o \
		$(BUILD_DIR)/memory.o  $(BUILD_DIR)/bitmap.o  $(BUILD_DIR)/list.o \
		$(BUILD_DIR)/process.o $(BUILD_DIR)/disk.o $(BUILD_DIR)/smfs.o \
		$(BUILD_DIR)/smfs_dir_entry.o $(BUILD_DIR)/smfs_node.o  $(BUILD_DIR)/path.o  \
		$(BUILD_DIR)/kernel.o $(BUILD_DIR)/explore.o  $(BUILD_DIR)/screen.o \
                $(BUILD_DIR)/syscall.o $(BUILD_DIR)/interrupt_syscall.o  \
		$(BUILD_DIR)/fat32.o $(BUILD_DIR)/fat32_dir_entry.o $(BUILD_DIR)/fat32_node.o \
                $(BUILD_DIR)/fat32_path.o $(BUILD_DIR)/smfs_path.o  $(BUILD_DIR)/msg_desc.o

BOOTFLAGS = -I 0_bios/
AR = ar
ARFLAGS=rcs
##############     c代码编译     ###############

$(BUILD_DIR)/%.o: %.c  include/*.h
	$(CC) $(CFLAGS) $< -o $@

##############    汇编代码编译    ###############
$(BUILD_DIR)/%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.bin:  %.S  0_bios/*.inc
	$(AS) $(BOOTFLAGS) $< -o $@

##############    链接所有目标文件    #############
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

##############    链接库    #############
$(BUILD_DIR)/smos_lib.a: $(OBJS)
	$(AR) $(ARFLAGS) $(BUILD_DIR)/smos_lib.a $(BUILD_DIR)/list.o $(BUILD_DIR)/trace.o \
	$(BUILD_DIR)/array_str.o $(BUILD_DIR)/print.o $(BUILD_DIR)/queue.o $(BUILD_DIR)/syscall.o

.PHONY : mk_dir hd clean all

mk_dir:
	if [[ ! -d $(BUILD_DIR) ]];then mkdir $(BUILD_DIR);fi
	if [[ ! -d $(PWD)/../editor/include ]];then mkdir $(PWD)/../editor/include;fi
	if [[ ! -d $(PWD)/../editor/bin ]];then mkdir $(PWD)/../editor/bin;fi

hd:
	dd if=$(BUILD_DIR)/kernel.bin of=$(PWD)/../Bochs-2.6.9/hd80M.img bs=512 count=1970 seek=75 conv=notrunc
	dd if=$(BUILD_DIR)/bios_section.bin of=$(PWD)/../Bochs-2.6.9/hd80M.img bs=446 count=1  conv=notrunc
	dd if=$(BUILD_DIR)/protect_set.bin of=$(PWD)/../Bochs-2.6.9/hd80M.img bs=512 count=4 seek=70 conv=notrunc

	dd if=$(BUILD_DIR)/kernel.bin of=/dev/sdb bs=512 count=1970 seek=75 conv=notrunc
	dd if=$(BUILD_DIR)/bios_section.bin of=/dev/sdb bs=446 count=1  conv=notrunc
	dd if=$(BUILD_DIR)/protect_set.bin of=/dev/sdb bs=512 count=4 seek=70 conv=notrunc

	cp $(PWD)/include/*.* $(PWD)/../editor/include
	cp $(BUILD_DIR)/smos_lib.a $(PWD)/../editor/bin
clean:
	cd $(BUILD_DIR) && rm -f ./*
	cd $(PWD)/../editor/include && rm -f ./*

build: $(BUILD_DIR)/kernel.bin  $(BUILD_DIR)/bios_section.bin  $(BUILD_DIR)/protect_set.bin $(BUILD_DIR)/smos_lib.a

all: mk_dir clean build hd
