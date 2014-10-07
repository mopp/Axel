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
export NASM 	:= nasm -g -f elf32
export AR_R		:= ar r --target elf32-i386
export OBJCOPY	:= objcopy
export QEMU 	:= qemu-system-i386 -monitor stdio -vga std -m 32 -d int -no-reboot
export QEMU_D 	:= qemu-system-i386 -vga std -m 32
export BOCHS	:= bochs
export GDB	 	:= gdb

export AXEL_ROOT_DIR		:= $(shell pwd)/
export AXEL_INCLUDE_PATH 	:= $(AXEL_ROOT_DIR)include/
export AXEL_LIB_PATH 		:= $(AXEL_ROOT_DIR)lib

export MAKEFILE 	:= Makefile

AXEL_BIN 			:= axel.bin
AXEL_SYM 			:= axel.sym

ISO_NAME 			:= axel.iso
ISO_DIR 			:= $(AXEL_ROOT_DIR)iso_dir/

BOOT_DIR			:= $(AXEL_ROOT_DIR)boot/
BOOT_OBJ 			:= $(BOOT_DIR)multiboot.o
GRUB_CONFIG 		:= $(BOOT_DIR)grub.cfg

KERNEL_LNK			:= kernel.ld
KERNEL_MAP			:= kernel.map

SUB_DIRS 			:= $(BOOT_DIR)
LINK_OBJS 			:= $(BOOT_OBJ) asm_functions.o utils.o font.o graphic_txt.o graphic_vbe.o graphic.o aqueue.o proc.o interrupt.o interrupt_asm.o exception.o ps2.o window.o memory.o buddy.o tlsf.o paging.o kernel.o

# CC          C compiler command
# CFLAGS      C compiler flags
# LDFLAGS     linker flags, e.g. -L<lib dir> if you have libraries in a nonstandard directory <lib dir>
# LIBS        libraries to pass to the linker, e.g. -l<library>
# CPPFLAGS    C/C++/Objective C preprocessor flags, e.g. -I<include dir> if you have headers in a nonstandard directory <include dir>
# CXX         C++ compiler command
# CXXFLAGS    C++ compiler flags
CPPFLAGS			:= -I$(AXEL_INCLUDE_PATH)
# W_FLAGS 			:= -Wall -Wextra -Wpadded -Winit-self -Wconversion -Wno-unused-parameter -Wwrite-strings -Wno-sign-compare -Wno-pointer-sign -Wno-missing-field-initializers -Wcast-qual -Wformat=2 -Wstrict-aliasing=2 -Wdisabled-optimization -Wfloat-equal -Wpointer-arith -Wbad-function-cast -Wcast-align -Wredundant-decls -Winline
W_FLAGS 			:= -g -Wall -Wextra -Winit-self -Wconversion -Wno-unused-parameter -Wwrite-strings -Wno-sign-compare -Wno-pointer-sign -Wno-missing-field-initializers -Wcast-qual -Wformat=2 -Wstrict-aliasing=2 -Wdisabled-optimization -Wfloat-equal -Wpointer-arith -Wbad-function-cast -Wcast-align -Wredundant-decls -Winline
# gcc option
# 	-fdiagnostics-color=always
# clang option
# 	-nostdinc cause error(macro redifine)
# debug option
# 	--analyze -g
CFLAGS				:= --target=i686-elf -m32 -std=c11 -ffreestanding -fno-builtin -nostdlib $(W_FLAGS)
LDFLAGS				:= -L$(AXEL_LIB_PATH) -Map $(KERNEL_MAP) -nostdlib
export CC 			:= $(CLANG) $(CFLAGS) $(CPPFLAGS)
export PP 			:= $(CLANG) $(CPPFLAGS) -x c -E -P


# 一般生成規則
.SUFFIXES: .c .o .asm
.c.o:
	$(CC) -c $< -o $@
.asm.o:
	$(eval TMP_FILE = $(shell mktemp))
	$(PP) $< > $(TMP_FILE)
	$(NASM) -l $*.lst -o $@ $(TMP_FILE)


.PHONY: default
default :
	$(MAKE) all
	ctags -R ./*


aqueue.o		: $(MAKEFILE) aqueue.c $(AXEL_INCLUDE_PATH)aqueue.h
asm_functions.o : $(MAKEFILE) asm_functions.asm $(AXEL_INCLUDE_PATH)asm_functions.h
buddy.o			: $(MAKEFILE) buddy.c $(AXEL_INCLUDE_PATH)buddy.h
exception.o		: $(MAKEFILE) exception.c $(AXEL_INCLUDE_PATH)exception.h
font.o			: $(MAKEFILE) font.c $(AXEL_INCLUDE_PATH)font.h $(AXEL_INCLUDE_PATH)drawable.h
graphic.o		: $(MAKEFILE) graphic.c $(AXEL_INCLUDE_PATH)graphic.h $(AXEL_INCLUDE_PATH)graphic_txt.h $(AXEL_INCLUDE_PATH)graphic_vbe.h
graphic_txt.o	: $(MAKEFILE) graphic_txt.c $(AXEL_INCLUDE_PATH)graphic_txt.h $(AXEL_INCLUDE_PATH)point.h
graphic_vbe.o	: $(MAKEFILE) graphic_vbe.c $(AXEL_INCLUDE_PATH)graphic_vbe.h $(AXEL_INCLUDE_PATH)point.h $(AXEL_INCLUDE_PATH)macros.h $(AXEL_INCLUDE_PATH)state_code.h $(AXEL_INCLUDE_PATH)vbe.h $(AXEL_INCLUDE_PATH)font.h $(AXEL_INCLUDE_PATH)drawable.h
interrupt.o 	: $(MAKEFILE) interrupt.c $(AXEL_INCLUDE_PATH)interrupt.h $(AXEL_INCLUDE_PATH)asm_functions.h
interrupt_asm.o : $(MAKEFILE) interrupt_asm.asm $(AXEL_INCLUDE_PATH)interrupt.h interrupt.o
kernel.o		: $(MAKEFILE) $(KERNEL_LNK) kernel.c $(AXEL_INCLUDE_PATH)kernel.h $(AXEL_INCLUDE_PATH)multiboot.h asm_functions.o utils.o font.o graphic_txt.o graphic_vbe.o graphic.o proc.o interrupt.o interrupt_asm.o exception.o ps2.o window.o memory.o buddy.o paging.o
memory.o		: $(MAKEFILE) memory.c $(AXEL_INCLUDE_PATH)memory.h $(AXEL_INCLUDE_PATH)asm_functions.h $(AXEL_INCLUDE_PATH)paging.h
paging.o		: $(MAKEFILE) paging.c $(AXEL_INCLUDE_PATH)paging.h $(AXEL_INCLUDE_PATH)memory.h $(AXEL_INCLUDE_PATH)asm_functions.h
proc.o			: $(MAKEFILE) proc.c $(AXEL_INCLUDE_PATH)proc.h
ps2.o			: $(MAKEFILE) ps2.c $(AXEL_INCLUDE_PATH)ps2.h $(AXEL_INCLUDE_PATH)state_code.h $(AXEL_INCLUDE_PATH)aqueue.h
tlsf.o			: $(MAKEFILE) tlsf.c $(AXEL_INCLUDE_PATH)tlsf.h
utils.o			: $(MAKEFILE) utils.c $(AXEL_INCLUDE_PATH)utils.h $(AXEL_INCLUDE_PATH)graphic.h
window.o		: $(MAKEFILE) window.c $(AXEL_INCLUDE_PATH)window.h graphic.o


$(BOOT_OBJ) : $(MAKEFILE)
	make -C $(BOOT_DIR)


$(AXEL_BIN) : $(MAKEFILE) $(KERNEL_LNK) $(LINK_OBJS)
	$(eval TMP_FILE = $(shell mktemp))
	$(PP) $(KERNEL_LNK) > $(TMP_FILE)
	$(LD) $(LINK_OBJS) $(LDFLAGS) -T $(TMP_FILE) -static -o $@
	$(OBJCOPY) --only-keep-debug $(AXEL_BIN) $(AXEL_SYM)
	$(OBJCOPY) --strip-debug $(AXEL_BIN)


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
	$(RM) *.o *.bin *.iso *.map *.lst *.log *.sym $(ISO_DIR) tags
	ctags -R ./*


.PHONY: run_qemu_cdrom
run_qemu_cdrom: $(MAKEFILE) $(ISO_NAME)
	$(QEMU) -cdrom $(ISO_NAME)


.PHONY: debug_run_qemu
debug_run_qemu: $(MAKEFILE) $(ISO_NAME)
	$(QEMU_D) -cdrom $(ISO_NAME) -s -S &
	$(GDB)


.PHONY: run_qemu_kernel
run_qemu_kernel: $(MAKEFILE) $(AXEL_BIN)
	$(QEMU) -kernel $(AXEL_BIN)


.PHONY: run_bochs
run_bochs: $(MAKEFILE) $(ISO_NAME)
	$(BOCHS)
