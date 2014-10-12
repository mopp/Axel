#############################################################
# @file Makefile
# @brief This is used for building Axel.
# @author mopp
# @version 0.1
# @date 2014-06-05
#############################################################

export RM          := rm -rf
export CLANG       := clang
export LD          := ld -m elf_i386 --format elf32-i386 --oformat elf32-i386
export NASM        := nasm -g -f elf32
export AR_R        := ar r --target elf32-i386
export OBJCOPY     := objcopy
export QEMU        := qemu-system-i386 -monitor stdio -vga std -m 32 -d int -no-reboot
export QEMU_D      := qemu-system-i386 -vga std -m 32
export BOCHS       := bochs
export GDB         := gdb
export MAKEFILE    := Makefile
export ROOT_DIR    := $(PWD)/
export INCLUDE_DIR := $(ROOT_DIR)include/

AXEL_BIN    := axel.bin
AXEL_SYM    := axel.sym

ISO_NAME    := axel.iso
ISO_DIR     := $(ROOT_DIR)iso/

BOOT_DIR    := $(ROOT_DIR)boot/
BOOT_OBJ    := $(BOOT_DIR)multiboot.o
GRUB_CONFIG := $(BOOT_DIR)grub.cfg

KERNEL_LNK  := kernel.ld
KERNEL_MAP  := kernel.map

SUB_DIRS    := $(BOOT_DIR)

C_SRCS      := rgb8.c elist.c point.c aqueue.c buddy.c exception.c font.c graphic.c graphic_txt.c graphic_vbe.c interrupt.c memory.c paging.c proc.c ps2.c tlsf.c utils.c window.c kernel.c
C_OBJS      := $(C_SRCS:%.c=%.o)
C_DEPS      := $(C_SRCS:%.c=%.d)
A_SRCS      := asm_functions.asm interrupt_asm.asm
A_OBJS      := $(A_SRCS:%.asm=%.o)
OBJS        := $(BOOT_OBJ) $(A_OBJS) $(C_OBJS)

# CC          C compiler command
# CFLAGS      C compiler flags
# LDFLAGS     linker flags, e.g. -L<lib dir> if you have libraries in a nonstandard directory <lib dir>
# LIBS        libraries to pass to the linker, e.g. -l<library>
# CPPFLAGS    C/C++/Objective C preprocessor flags, e.g. -I<include dir> if you have headers in a nonstandard directory <include dir>
# CXX         C++ compiler command
# CXXFLAGS    C++ compiler flags
# gcc option
# 	-fdiagnostics-color=always
# clang option
# 	-nostdinc cause error(macro redifine)
# debug option
# 	--analyze -g
CPPFLAGS  := -I$(INCLUDE_DIR)
W_FLAGS   := -g -Wall -Wextra -Winit-self -Wconversion -Wno-unused-parameter -Wwrite-strings -Wno-sign-compare -Wno-pointer-sign -Wno-missing-field-initializers -Wcast-qual -Wformat=2 -Wstrict-aliasing=2 -Wdisabled-optimization -Wfloat-equal -Wpointer-arith -Wbad-function-cast -Wcast-align -Wredundant-decls -Winline
CFLAGS    := --target=i686-elf -m32 -std=c11 -ffreestanding -fno-builtin -nostdlib $(W_FLAGS)
LDFLAGS   := -Map $(KERNEL_MAP) -nostdlib
export CC := $(CLANG) $(CFLAGS) $(CPPFLAGS)
export PP := $(CLANG) $(CPPFLAGS) -x c -E -P


# 一般生成規則
.SUFFIXES: .c .o .asm
.c.o:
	$(CC) -MMD -c $< -o $@
.asm.o:
	$(eval TMP_FILE = $(shell mktemp))
	$(PP) $< > $(TMP_FILE)
	$(NASM) -l $*.lst -o $@ $(TMP_FILE)


.PHONY: all
all: $(SUB_DIRS)
	@for dir in $(SUB_DIRS) ; do \
		($(MAKE) -C $$dir); \
		done
	$(MAKE) $(ISO_NAME)
	ctags -R ./*


-include $(C_DEPS)
asm_functions.o : $(MAKEFILE) asm_functions.asm $(INCLUDE_DIR)asm_functions.h $(INCLUDE_DIR)segment.h $(INCLUDE_DIR)paging.h
interrupt_asm.o : $(MAKEFILE) interrupt_asm.asm $(INCLUDE_DIR)interrupt.h interrupt.o


$(AXEL_BIN) : $(MAKEFILE) $(KERNEL_LNK) $(OBJS)
	$(eval TMP_FILE = $(shell mktemp))
	$(PP) $(KERNEL_LNK) > $(TMP_FILE)
	$(LD) $(OBJS) $(LDFLAGS) -T $(TMP_FILE) -static -o $@
	$(OBJCOPY) --only-keep-debug $(AXEL_BIN) $(AXEL_SYM)
	$(OBJCOPY) --strip-debug $(AXEL_BIN)


$(ISO_NAME) : $(MAKEFILE) $(AXEL_BIN) $(GRUB_CONFIG)
	mkdir -p $(ISO_DIR)boot/grub
	cp $(AXEL_BIN) $(ISO_DIR)boot/
	cp $(GRUB_CONFIG) $(ISO_DIR)boot/grub/grub.cfg
	grub-mkimage -O i386-pc -o $(ISO_DIR)efi.img multiboot biosdisk iso9660
	grub-mkrescue -o $(ISO_NAME) $(ISO_DIR)


.PHONY: clean
clean:
	@for dir in $(SUB_DIRS) ; \
		do ($(MAKE) -C $$dir clean); \
		done
	$(RM) *.d *.o *.bin *.iso *.map *.lst *.log *.sym $(ISO_DIR) tags
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
