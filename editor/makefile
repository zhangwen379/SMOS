BUILD_DIR = bin
ENTRY_POINT =0x48048000
AS = nasm
CC = gcc
LD = ld 
ASFLAGS = -f elf
CFLAGS = -Wall -I include/ -c -fno-builtin -W -Wstrict-prototypes \
	 -Wmissing-prototypes -Wsystem-headers
LDFLAGS = -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/editor.map
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/smos_lib.a


    #
##############     c代码编译     ###############
	
$(BUILD_DIR)/%.o: main/%.c  include/*.h
	$(CC) $(CFLAGS) $< -o $@

##############    链接所有目标文件    #############
$(BUILD_DIR)/editor.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY : mk_dir hd clean all

mk_dir:
	if [[ ! -d $(BUILD_DIR) ]];then mkdir $(BUILD_DIR);fi

hd:
	dd if=$(BUILD_DIR)/editor.bin	of=$(PWD)/../Bochs-2.6.9/hd60M.img bs=512 count=100 seek=300 conv=notrunc

clean:
	cd $(BUILD_DIR) && rm -f ./*.o ./*.map ./*.bin

build: $(BUILD_DIR)/editor.bin

all: mk_dir clean build hd
