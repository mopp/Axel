#############################################################
# File: src/Makefile
# Description: Build Managing file
#############################################################
include globals.mk

AXEL_ROOT_DIR		:= $(shell pwd)/
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
# 順序に注意
# LINK_OBJS 			:= $(BOOT_OBJ) asm_functions.o interrupt_handler.o interrupt_handler_asm.o string.o graphic_vbe.o graphic_txt.o doubly_linked_list.o stack.o memory.o kernel.o
LINK_OBJS 			:= $(BOOT_OBJ) asm_functions.o interrupt_handler.o interrupt_handler_asm.o string.o point.o rgb8.o graphic_txt.o graphic_vbe.o graphic.o stdio.o doubly_linked_list.o stack.o memory.o kernel.o

SUBDIRS 			:= $(BOOT_DIR)

# CC          C compiler command
# CFLAGS      C compiler flags
# LDFLAGS     linker flags, e.g. -L<lib dir> if you have libraries in a nonstandard directory <lib dir>
# LIBS        libraries to pass to the linker, e.g. -l<library>
# CPPFLAGS    C/C++/Objective C preprocessor flags, e.g. -I<include dir> if you have headers in a nonstandard directory <include dir>
# CXX         C++ compiler command
# CXXFLAGS    C++ compiler flags
CPPFLAGS			:= -I$(AXEL_INCLUDE_PATH) -I$(TOOL_PATH)include/ -L$(TOOL_PATH)lib/
# W_FLAGS 			:= -Wall -Wextra -Wpadded -Winit-self -Wconversion -Wno-unused-parameter -Wwrite-strings -Wno-sign-compare -Wno-pointer-sign -Wno-missing-field-initializers -Wcast-qual -Wformat=2 -Wstrict-aliasing=2 -Wdisabled-optimization -Wfloat-equal -Wpointer-arith -Wbad-function-cast -Wcast-align -Wredundant-decls -Winline
W_FLAGS 			:= -Wall -Wextra -Winit-self -Wconversion -Wno-unused-parameter -Wwrite-strings -Wno-sign-compare -Wno-pointer-sign -Wno-missing-field-initializers -Wcast-qual -Wformat=2 -Wstrict-aliasing=2 -Wdisabled-optimization -Wfloat-equal -Wpointer-arith -Wbad-function-cast -Wcast-align -Wredundant-decls -Winline
CFLAGS				:= -g -std=c11 -fno-builtin -nostdlib -mno-red-zone -ffreestanding -nostdinc -fno-stack-protector $(W_FLAGS)
LIBS				:= -lgcc
LDFLAGS				:= -g -T $(KERNEL_LNK) -Wl,-Map,$(KERNEL_MAP) -nostdlib -ffreestanding $(LIBS)
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


asm_functions.o 		: asm_functions.asm 		$(MAKEFILE) $(AXEL_INCLUDE_PATH)asm_functions.h
doubly_linked_list.o	: doubly_linked_list.c 		$(MAKEFILE) $(AXEL_INCLUDE_PATH)doubly_linked_list.h
graphic_txt.o			: graphic_txt.c 			$(MAKEFILE) $(AXEL_INCLUDE_PATH)graphic_txt.h
graphic_vbe.o			: graphic_vbe.c 			$(MAKEFILE) $(AXEL_INCLUDE_PATH)graphic_vbe.h
graphic.o				: graphic.c 				$(MAKEFILE) $(AXEL_INCLUDE_PATH)graphic.h $(AXEL_INCLUDE_PATH)graphic_txt.h $(AXEL_INCLUDE_PATH)graphic_vbe.h graphic_vbe.c graphic_txt.c
interrupt_handler.o 	: interrupt_handler.c 		$(MAKEFILE) $(AXEL_INCLUDE_PATH)interrupt_handler.h
interrupt_handler_asm.o : interrupt_handler_asm.asm $(MAKEFILE) $(AXEL_INCLUDE_PATH)interrupt_handler.h
kernel.o				: kernel.c 					$(MAKEFILE) $(AXEL_INCLUDE_PATH)kernel.h $(AXEL_INCLUDE_PATH)multiboot_constants.h $(AXEL_INCLUDE_PATH)multiboot_structs.h
memory.o				: memory.c 					$(MAKEFILE) $(AXEL_INCLUDE_PATH)memory.h
stack.o					: stack.c 					$(MAKEFILE) $(AXEL_INCLUDE_PATH)stack.h
string.o				: string.c 					$(MAKEFILE) $(AXEL_INCLUDE_PATH)string.h
point.o					: point.c 					$(MAKEFILE) $(AXEL_INCLUDE_PATH)point.h
rgb8.o					: rgb8.c 					$(MAKEFILE) $(AXEL_INCLUDE_PATH)rgb8.h
stdio.o					: stdio.c 					$(MAKEFILE) $(AXEL_INCLUDE_PATH)stdio.h


$(BOOT_OBJ) : $(MAKEFILE)
	make -C $(BOOT_DIR)


$(AXEL_BIN) : $(MAKEFILE) $(KERNEL_LNK) $(LINK_OBJS)
	$(LD) $(LDFLAGS) $(LINK_OBJS) -o $@


$(ISO_NAME) : $(MAKEFILE) $(AXEL_BIN) $(GRUB_CONFIG)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(AXEL_BIN) $(ISO_DIR)/boot/
	cp $(GRUB_CONFIG) $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkimage -O i386-pc -o $(ISO_DIR)/efi.img multiboot biosdisk iso9660
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
	$(RM) *.o *.bin *.iso *.map *.lst *.log $(ISO_DIR) tags
	ctags -R ./*


.PHONY: run_qemu_cdrom
run_qemu_cdrom: $(MAKEFILE) $(ISO_NAME)
	$(QEMU) -m 32 -cdrom $(ISO_NAME)


.PHONY: run_qemu_kernel
run_qemu_kernel: $(MAKEFILE) $(AXEL_BIN)
	$(QEMU) -m 32 -kernel $(AXEL_BIN)


.PHONY: run_bochs
run_bochs: $(MAKEFILE) $(ISO_NAME)
	$(BOCHS)
