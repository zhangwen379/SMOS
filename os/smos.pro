INCLUDEPATH += include

DISTFILES += \
    makefile \
    2_operator/interrupt/idt_entry_table.S \
    0_bios/*.inc \
    0_bios/*.S \

SOURCES += \
	1_data/*.c \
    1_data/memory/*.c \
    1_data/disk/*.c	\
    1_data/screen/*.c \
    1_data/struct/*.c \
    2_operator/interrupt/*.c \
    2_operator/*.c \
    3_process/*.c \
    4_system/*.c \
    debug/*.c   \

HEADERS +=  include/*.h
