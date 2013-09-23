include globals.mk

AXEL_ROOT_DIR		:= $(shell pwd)/../
AXEL_INCLUDE_PATH 	:= $(AXEL_ROOT_DIR)include/

# files
MAKEFILE 			:= Makefile

AXEL_BIN 			:= axel.bin
ISO_NAME 			:= axel.iso
ISO_DIR 			:= iso_dir

BOOT_DIR			:= ./boot/
BOOT_OBJ 			:= $(BOOT_DIR)multiboot.o
GRUB_CONFIG 		:= $(BOOT_DIR)grub.cfg
KERNEL_LNK			:= kernel.ld
KERNEL_MAP			:= kernel.map
# XXX 順序に注意
LINK_OBJS 			:= $(BOOT_OBJ) asm_functions.o string.o graphic_txt.o kernel.o

SUBDIRS 			:= $(BOOT_DIR)

# CC          C compiler command
# CFLAGS      C compiler flags
# LDFLAGS     linker flags, e.g. -L<lib dir> if you have libraries in a nonstandard directory <lib dir>
# LIBS        libraries to pass to the linker, e.g. -l<library>
# CPPFLAGS    C/C++/Objective C preprocessor flags, e.g. -I<include dir> if you have headers in a nonstandard directory <include dir>
# CXX         C++ compiler command
# CXXFLAGS    C++ compiler flags
CPPFLAGS			:= -I$(AXEL_INCLUDE_PATH) -I$(TOOL_PATH)include/ -L$(TOOL_PATH)lib/
CFLAGS				:= -fno-builtin -nostdlib -mno-red-zone -ffreestanding -nostdinc -fno-stack-protector -Wall -Wextra -std=c11
LIBS				:= -lgcc
LDFLAGS				:= -T $(KERNEL_LNK) -Wl,-Map,$(KERNEL_MAP) -nostdlib -ffreestanding $(LIBS)
CC 					:= $(GCC) $(CFLAGS) $(CPPFLAGS)
LD 					:= $(GCC)



# 一般生成規則
.SUFFIXES: .c .o .asm
.c.o:
	$(CC) -c $< -o $@
.asm.o:
	$(NASM) -l $*.lst -o $@ $<

default :
	make all
	ctags -R ./*

asm_functions.o 	: asm_functions.asm 	$(MAKEFILE) $(AXEL_INCLUDE_PATH)asm_functions.h
string.o			: string.c 				$(MAKEFILE) $(AXEL_INCLUDE_PATH)string.h
graphic_txt.o		: graphic_txt.c 		$(MAKEFILE) $(AXEL_INCLUDE_PATH)graphic_txt.h
kernel.o			: kernel.c 				$(MAKEFILE) $(AXEL_INCLUDE_PATH)kernel.h

$(BOOT_OBJ) : $(MAKEFILE)
	make -C $(BOOT_DIR)

$(AXEL_BIN) : $(MAKEFILE) $(KERNEL_LNK) $(LINK_OBJS)
	$(LD) $(LDFLAGS) $(LINK_OBJS) -o $@

$(ISO_NAME) : $(MAKEFILE) $(AXEL_BIN) $(GRUB_CONFIG)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(AXEL_BIN) $(ISO_DIR)/boot/
	cp $(GRUB_CONFIG) $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_NAME) $(ISO_DIR)

.PHONY: all $(SUBDIRS)
all: $(SUBDIRS)
	@for dir in $(SUBDIRS) ; do \
		($(MAKE) -C $$dir); \
		done
	make $(ISO_NAME)

.PHONY: clean
clean:
	@for dir in $(SUBDIRS) ; do \
		($(MAKE) -C $$dir clean); \
		done
	$(RM) *.o *.bin *.iso *.map *.lst $(ISO_DIR) tags
	ctags -R ./*

.PHONY: run
run : $(MAKEFILE) $(ISO_NAME)
	$(QEMU_EXE) -cdrom $(ISO_NAME)
