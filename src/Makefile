#############################################################
# @file Makefile
# @brief for building Axel.
# @author mopp
# @version 0.1
# @date 2014-06-05
#############################################################

export RM          := rm -rf
export CC          := clang -std=c11
export LD          := ld -m elf_i386 --format elf32-i386 -nostartfiles -nodefaultlibs -nostdlib -static
export NASM        := nasm
export OBJCOPY     := objcopy
export QEMU        := qemu-system-i386 -monitor stdio -vga std -m 32 -boot order=dc -no-reboot -d int
export BOCHS       := bochs -q
export GDB         := gdb
export MKDIR       := mkdir -p
export CP          := cp
export MAKE        := make --warn-undefined-variables
export MOUNT       := sudo mount
export UMOUNT      := sudo umount
export MAKEFILE    := Makefile
export SRC_DIR     := $(PWD)/
export INCLUDE_DIR := $(SRC_DIR)include/
export OS_NAME     := $(shell uname -s)

FD_IMG        := fd.img
HDD_IMG       := hdd.img
AXEL_BIN      := axel.bin
AXEL_SYM      := axel.sym
KERNEL_LD     := kernel.ld
KERNEL_MAP    := kernel.map
USER_INIT_SRC := user_init.asm
USER_INIT_BIN := user_init
USER_MAIN_SRC := user_main.c
USER_MAIN_BIN := user_main
ISO_NAME      := axel.iso

BOOT_DIR  := ./boot/
TOOL_DIR  := ./tools/
WORK_DIR  := $(SRC_DIR)work/
ISO_DIR   := $(WORK_DIR)iso/
OBJS_DIR  := $(WORK_DIR)objs/
DEPS_DIR  := $(WORK_DIR)deps/
WORK_DIRS := $(WORK_DIR) $(OBJS_DIR) $(DEPS_DIR) $(ISO_DIR) $(OBJS_DIR)dev $(DEPS_DIR)dev
SUB_DIRS  := $(BOOT_DIR) $(TOOL_DIR)

# Variables for boots
GRUB_CONFIG       := $(BOOT_DIR)grub.cfg
MULTI_BOOT_SRC    := $(BOOT_DIR)multiboot.asm
MULTI_BOOT_OBJ    := $(BOOT_DIR)multiboot.o
LOADER1_SRC       := $(BOOT_DIR)loader1.asm
LOADER1_BIN       := $(BOOT_DIR)loader1
LOADER2_SRC       := $(BOOT_DIR)loader2.c
LOADER2_OBJ       := $(BOOT_DIR)loader2.o
LOADER2_LD        := $(BOOT_DIR)loader2.ld
LOADER2_BIN       := $(BOOT_DIR)loader2
LOADER2_MAP       := $(BOOT_DIR)loader2.map
LOADER2_DEPS_OBJS := $(OBJS_DIR)asm_functions.o $(OBJS_DIR)interrupt.o $(OBJS_DIR)utils.o $(OBJS_DIR)time.o

C_SRCS    :=      \
	elf.c         \
	time.c        \
	syscall.c     \
	fat_manip.c   \
	fat.c         \
	fs.c          \
	dev/ata.c     \
	dev/pci.c     \
	dev/acpi.c    \
	rgb8.c        \
	elist.c       \
	point.c       \
	aqueue.c      \
	buddy.c       \
	font.c        \
	graphic.c     \
	graphic_txt.c \
	graphic_vbe.c \
	interrupt.c   \
	memory.c      \
	paging.c      \
	proc.c        \
	dev/ps2.c     \
	tlsf.c        \
	utils.c       \
	window.c      \
	print.c      \
	kernel.c
C_OBJS := $(addprefix $(OBJS_DIR), $(C_SRCS:%.c=%.o))
C_DEPS := $(addprefix $(DEPS_DIR), $(C_SRCS:%.c=%.d))
A_SRCS := asm_functions.asm interrupt_asm.asm
A_OBJS := $(addprefix $(OBJS_DIR), $(A_SRCS:%.asm=%.o))
OBJS   := $(MULTI_BOOT_OBJ) $(A_OBJS) $(C_OBJS)

# Variables for tools
IMG_PROG := $(TOOL_DIR)img_util


# CC          C compiler command
# CFLAGS      C compiler flags
# LDFLAGS     linker flags, e.g. -L<lib dir> if you have libraries in a nonstandard directory <lib dir>
# LIBS        libraries to pass to the linker, e.g. -l<library>
# CPPFLAGS    C/C++/Objective C preprocessor flags, e.g. -I<include dir> if you have headers in a nonstandard directory <include dir>
# CXX         C++ compiler command
# CXXFLAGS    C++ compiler flags
W_FLAGS         := -g -Wall -Wextra -Winit-self -Wconversion -Wno-unused-parameter -Wwrite-strings -Wno-sign-compare -Wno-pointer-sign -Wno-missing-field-initializers -Wcast-qual -Wformat=2 -Wstrict-aliasing=2 -Wdisabled-optimization -Wfloat-equal -Wpointer-arith -Wbad-function-cast -Wcast-align -Wredundant-decls -Winline
export CPPFLAGS := -isystem$(INCLUDE_DIR)
export CFLAGS   := -m32 --target=i386-elf -nostdlib -ffreestanding -fno-builtin $(W_FLAGS)
export LDFLAGS  :=
export PP       := $(CC) $(CPPFLAGS) -x c -E -P


define tmp_file_preprocess
	@$(eval TMP_FILE = $(shell mktemp))
	@$(PP) $1 > $(TMP_FILE)
endef


$(OBJS_DIR)%.o: %.c
	@test -d $(OBJS_DIR) || $(MKDIR) $(WORK_DIRS)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MQ $@ -MF $(DEPS_DIR)$*.d -c $< -o $@
	@echo $(patsubst -W%,,$(CC) -c $< -o $(notdir $@))


$(OBJS_DIR)%.o: %.asm
	@test -d $(OBJS_DIR) || $(MKDIR) $(WORK_DIRS)
	$(call tmp_file_preprocess, $<)
	$(NASM) -f elf32 -l $(OBJS_DIR)$*.lst -o $@ $(TMP_FILE)


.PHONY: all
all: $(SUB_DIRS)
	$(MKDIR) $(WORK_DIRS)
	$(MAKE) $(ISO_NAME)
	ctags -R ./*


-include $(C_DEPS)
$(OBJS_DIR)asm_functions.o: $(MAKEFILE) asm_functions.asm $(INCLUDE_DIR)asm_functions.h $(INCLUDE_DIR)segment.h $(INCLUDE_DIR)paging.h
$(OBJS_DIR)interrupt_asm.o: $(MAKEFILE) interrupt_asm.asm $(INCLUDE_DIR)interrupt.h $(OBJS_DIR)interrupt.o


$(MULTI_BOOT_OBJ): $(MAKEFILE) $(MULTI_BOOT_SRC)
	$(call tmp_file_preprocess, $(MULTI_BOOT_SRC))
	$(NASM) -f elf32 -l $*.lst -o $@ $(TMP_FILE)


$(LOADER1_BIN): $(MAKEFILE) $(LOADER1_SRC)
	$(NASM) -f bin -o $@ $(LOADER1_SRC)


$(LOADER2_BIN): $(MAKEFILE) $(LOADER2_SRC) $(LOADER2_DEPS_OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(LOADER2_SRC) -o $(LOADER2_OBJ)
	$(NASM) -f elf32 ./boot/loader2.asm -o ./boot/loader2_asm.o
	$(LD) -Map $(LOADER2_MAP) -T $(LOADER2_LD) -o $@ $(LOADER2_OBJ) $(LOADER2_DEPS_OBJS) ./boot/loader2_asm.o


$(AXEL_BIN): $(MAKEFILE) $(KERNEL_LD) $(OBJS)
	$(call tmp_file_preprocess, $(KERNEL_LD))
	@$(LD) -Map $(KERNEL_MAP) -T $(TMP_FILE) -o $@ $(OBJS)
	@echo $(LD) $(notdir $(OBJS)) -o $@
	$(OBJCOPY) --only-keep-debug $(AXEL_BIN) $(AXEL_SYM)
	$(OBJCOPY) --strip-debug $(AXEL_BIN)


$(ISO_NAME): $(MAKEFILE) $(AXEL_BIN) $(GRUB_CONFIG)
	$(MKDIR) $(ISO_DIR)boot/grub
	$(CP) $(AXEL_BIN) $(ISO_DIR)boot/
	$(CP) $(GRUB_CONFIG) $(ISO_DIR)boot/grub/grub.cfg
	grub-mkimage -O i386-pc -o $(ISO_DIR)efi.img multiboot biosdisk iso9660
	grub-mkrescue -o $(ISO_NAME) $(ISO_DIR)


$(USER_INIT_BIN): $(MAKEFILE) $(USER_INIT_SRC)
	$(NASM) -f bin -o $@ $(USER_INIT_SRC)


$(USER_MAIN_BIN): $(MAKEFILE) $(USER_MAIN_SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(USER_MAIN_SRC) -o $*.o
	$(LD) -e main -o $@ $*.o


$(FD_IMG): $(MAKEFILE) $(AXEL_BIN) $(LOADER1_BIN) $(LOADER2_BIN)
	$(MAKE) -C $(TOOL_DIR)
	$(IMG_PROG) -f 12 -t $@ -m $(LOADER1_BIN) -l $(LOADER2_BIN) $(AXEL_BIN) $(INIT_BIN)


$(HDD_IMG): $(MAKEFILE) $(AXEL_BIN) $(LOADER1_BIN) $(LOADER2_BIN) $(USER_INIT_BIN) $(USER_MAIN_BIN)
	$(MAKE) -C $(TOOL_DIR)
	$(IMG_PROG) -f 32 -t $@ -m $(LOADER1_BIN) -l $(LOADER2_BIN) $(AXEL_BIN) $(USER_INIT_BIN) $(USER_MAIN_BIN)


.PHONY: clean
clean:
	$(MAKE) -C $(TOOL_DIR) clean
	$(RM) *.d *.o *.bin *.iso *.map *.lst *.log *.sym tags $(WORK_DIR) bx_enh_dbg.ini $(FD_IMG) $(HDD_IMG)
	$(RM) $(LOADER1_BIN) $(LOADER2_BIN) $(BOOT_DIR)/*.o $(BOOT_DIR)/*.map $(BOOT_DIR)/*.lst
	ctags -R ./*


.PHONY: run_qemu_cdrom
run_qemu_cdrom: $(MAKEFILE) $(ISO_NAME) $(HDD_IMG)
	$(QEMU) -cdrom $(ISO_NAME) -hda $(HDD_IMG)


.PHONY: run_qemu_debug
run_qemu_debug: $(MAKEFILE) $(ISO_NAME)
	$(QEMU_D) -cdrom $(ISO_NAME) -s -S &
	$(GDB)


.PHONY: run_qemu_fd
run_qemu_fd: $(MAKEFILE) $(AXEL_BIN) $(FD_IMG)
	$(QEMU) -fda $(FD_IMG)


.PHONY: run_bochs
run_bochs: $(MAKEFILE) $(ISO_NAME) $(HDD_IMG)
	$(BOCHS)


.PHONY: write_disk
write_disk: $(FD_IMG)
	sudo dd if=$(FD_IMG) of=/dev/sdb
