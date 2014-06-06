#############################################################
# @file Makefile
# @brief This is used for building Axel.
# @author mopp
# @version 0.1
# @date 2014-06-05
#############################################################

export RM 		:= rm -rf
export CLANG	:= clang
export LD 		:= ld -m elf_i386 --format elf32-i386 --oformat elf32-i386
export NASM 	:= nasm -f elf32
export AR_R		:= ar r --target elf32-i386
export QEMU 	:= qemu-system-i386 -monitor stdio -vga std
export BOCHS	:= bochs

export AXEL_ROOT_DIR		:= $(shell pwd)/
export AXEL_INCLUDE_PATH 	:= $(AXEL_ROOT_DIR)include/
export AXEL_LIB_PATH 		:= $(AXEL_ROOT_DIR)lib

export MAKEFILE 	:= Makefile

AXEL_BIN 			:= axel.bin

ISO_NAME 			:= axel.iso
ISO_DIR 			:= $(AXEL_ROOT_DIR)iso_dir/

BOOT_DIR			:= $(AXEL_ROOT_DIR)boot/
BOOT_OBJ 			:= $(BOOT_DIR)multiboot.o
GRUB_CONFIG 		:= $(BOOT_DIR)grub.cfg

KERNEL_LNK			:= kernel.ld
KERNEL_MAP			:= kernel.map

STRUCT_DIR			:= $(AXEL_ROOT_DIR)struct/
SUB_DIRS 			:= $(BOOT_DIR) $(STRUCT_DIR)
LINK_OBJS 			:= $(BOOT_OBJ) asm_functions.o interrupt_handler.o interrupt_handler_asm.o string.o font.o graphic_txt.o graphic_vbe.o keyboard.o graphic.o stdio.o memory.o paging.o kernel.o

# CC          C compiler command
# CFLAGS      C compiler flags
# LDFLAGS     linker flags, e.g. -L<lib dir> if you have libraries in a nonstandard directory <lib dir>
# LIBS        libraries to pass to the linker, e.g. -l<library>
# CPPFLAGS    C/C++/Objective C preprocessor flags, e.g. -I<include dir> if you have headers in a nonstandard directory <include dir>
# CXX         C++ compiler command
# CXXFLAGS    C++ compiler flags
CPPFLAGS			:= -I$(AXEL_INCLUDE_PATH)
# W_FLAGS 			:= -Wall -Wextra -Wpadded -Winit-self -Wconversion -Wno-unused-parameter -Wwrite-strings -Wno-sign-compare -Wno-pointer-sign -Wno-missing-field-initializers -Wcast-qual -Wformat=2 -Wstrict-aliasing=2 -Wdisabled-optimization -Wfloat-equal -Wpointer-arith -Wbad-function-cast -Wcast-align -Wredundant-decls -Winline
W_FLAGS 			:= -Wgnu-designator -Wall -Wextra -Winit-self -Wconversion -Wno-unused-parameter -Wwrite-strings -Wno-sign-compare -Wno-pointer-sign -Wno-missing-field-initializers -Wcast-qual -Wformat=2 -Wstrict-aliasing=2 -Wdisabled-optimization -Wfloat-equal -Wpointer-arith -Wbad-function-cast -Wcast-align -Wredundant-decls -Winline
# gcc option
# 	-fdiagnostics-color=always
# clang option
# 	-nostdinc cause error(macro redifine)
# debug option
# 	--analyze -g
CFLAGS				:= --target=i686-elf -m32 -std=c11 -ffreestanding -fno-builtin -nostdlib $(W_FLAGS)
LDFLAGS				:= -L$(AXEL_LIB_PATH) -T $(KERNEL_LNK) -Map $(KERNEL_MAP) -nostdlib
LIBS				:= -lstruct
export CC 			:= $(CLANG) $(CFLAGS) $(CPPFLAGS)


# 一般生成規則
.SUFFIXES: .c .o .asm
.c.o:
	$(CC) -c $< -o $@
.asm.o:
	$(NASM) -l $*.lst -o $@ $<


.PHONY: default
default :
	make all
	ctags -R ./*


asm_functions.o 		: $(MAKEFILE) asm_functions.asm $(AXEL_INCLUDE_PATH)asm_functions.h
graphic_txt.o			: $(MAKEFILE) graphic_txt.c $(AXEL_INCLUDE_PATH)graphic_txt.h $(AXEL_INCLUDE_PATH)point.h
graphic_vbe.o			: $(MAKEFILE) graphic_vbe.c $(AXEL_INCLUDE_PATH)graphic_vbe.h $(AXEL_INCLUDE_PATH)point.h $(AXEL_INCLUDE_PATH)macros.h $(AXEL_INCLUDE_PATH)state_code.h $(AXEL_INCLUDE_PATH)vbe.h $(AXEL_INCLUDE_PATH)font.h $(AXEL_INCLUDE_PATH)drawable.h
graphic.o				: $(MAKEFILE) graphic.c $(AXEL_INCLUDE_PATH)graphic.h $(AXEL_INCLUDE_PATH)graphic_txt.h $(AXEL_INCLUDE_PATH)graphic_vbe.h
interrupt_handler.o 	: $(MAKEFILE) interrupt_handler.c $(AXEL_INCLUDE_PATH)interrupt_handler.h $(AXEL_INCLUDE_PATH)asm_functions.h
interrupt_handler_asm.o : $(MAKEFILE) interrupt_handler_asm.asm $(AXEL_INCLUDE_PATH)interrupt_handler.h interrupt_handler.o
memory.o				: $(MAKEFILE) memory.c $(AXEL_INCLUDE_PATH)memory.h $(AXEL_INCLUDE_PATH)list.h $(AXEL_INCLUDE_PATH)asm_functions.h  $(AXEL_INCLUDE_PATH)string.h $(AXEL_INCLUDE_PATH)paging.h
paging.o				: $(MAKEFILE) paging.c $(AXEL_INCLUDE_PATH)paging.h $(AXEL_INCLUDE_PATH)memory.h $(AXEL_INCLUDE_PATH)asm_functions.h  $(AXEL_INCLUDE_PATH)string.h
string.o				: $(MAKEFILE) string.c $(AXEL_INCLUDE_PATH)string.h
stdio.o					: $(MAKEFILE) stdio.c $(AXEL_INCLUDE_PATH)stdio.h $(AXEL_INCLUDE_PATH)graphic.h
font.o					: $(MAKEFILE) font.c $(AXEL_INCLUDE_PATH)font.h $(AXEL_INCLUDE_PATH)drawable.h
keyboard.o				: $(MAKEFILE) keyboard.c $(AXEL_INCLUDE_PATH)keyboard.h $(AXEL_INCLUDE_PATH)state_code.h $(AXEL_INCLUDE_PATH)aqueue.h
kernel.o				: $(MAKEFILE) $(KERNEL_LNK) kernel.c $(AXEL_INCLUDE_PATH)kernel.h $(AXEL_INCLUDE_PATH)multiboot.h asm_functions.o  graphic_txt.o graphic_vbe.o graphic.o interrupt_handler.o interrupt_handler_asm.o memory.o paging.o string.o stdio.o font.o keyboard.o


$(BOOT_OBJ) : $(MAKEFILE)
	make -C $(BOOT_DIR)


$(AXEL_BIN) : $(MAKEFILE) $(KERNEL_LNK) $(LINK_OBJS)
	$(LD) $(LINK_OBJS) $(LDFLAGS) -static $(LIBS) -o $@


$(ISO_NAME) : $(MAKEFILE) $(AXEL_BIN) $(GRUB_CONFIG)
	mkdir -p $(ISO_DIR)boot/grub
	cp $(AXEL_BIN) $(ISO_DIR)boot/
	cp $(GRUB_CONFIG) $(ISO_DIR)boot/grub/grub.cfg
	grub-mkimage -O i386-pc -o $(ISO_DIR)efi.img multiboot biosdisk iso9660
	grub-mkrescue -o $(ISO_NAME) $(ISO_DIR)


.PHONY: all
all: $(SUB_DIRS)
	@for dir in $(SUB_DIRS) ; do \
		($(MAKE) -C $$dir); \
		done
	make $(ISO_NAME)


.PHONY: clean
clean:
	@for dir in $(SUB_DIRS) ; \
		do ($(MAKE) -C $$dir clean); \
		done
	$(RM) *.o *.bin *.iso *.map *.lst *.log $(ISO_DIR) tags
	ctags -R ./*


.PHONY: lib
lib: $(STRUCT_DIR)
	make -C $(STRUCT_DIR)


.PHONY: run_qemu_cdrom
run_qemu_cdrom: $(MAKEFILE) $(ISO_NAME)
	$(QEMU) -m 32 -cdrom $(ISO_NAME)


.PHONY: run_qemu_kernel
run_qemu_kernel: $(MAKEFILE) $(AXEL_BIN)
	$(QEMU) -m 32 -kernel $(AXEL_BIN)


# .PHONY: run_bochs
# run_bochs: $(MAKEFILE) $(ISO_NAME)
# 	$(BOCHS)
