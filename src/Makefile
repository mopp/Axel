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
export QEMU        := qemu-system-i386 -monitor stdio -vga std -m 32 -boot order=dc -no-reboot -d int
export QEMU_D      := qemu-system-i386 -vga std -m 32 -boot order=dc
export BOCHS       := bochs -q
export GDB         := gdb
export MAKEFILE    := Makefile
export MKDIR       := mkdir -p
export ROOT_DIR    := $(PWD)/
export INCLUDE_DIR := $(ROOT_DIR)include/

MAKE       := make
OS_NAME    := $(shell uname -s)
DISK_IMG   := disk.img
D_CYLINDER := 100

AXEL_BIN   := axel.bin
AXEL_SYM   := axel.sym
KERNEL_LNK := kernel.ld
KERNEL_MAP := kernel.map
INIT_FILE  := init.asm
INIT       := init
ISO_NAME   := axel.iso

BOOT_DIR    := boot/
BOOT_OBJ    := $(BOOT_DIR)multiboot.o
GRUB_CONFIG := $(BOOT_DIR)grub.cfg

TOOL_DIR    := tools/

WORK_DIR    := work/
ISO_DIR     := $(WORK_DIR)iso/
OBJS_DIR    := $(WORK_DIR)objs/
DEPS_DIR    := $(WORK_DIR)deps/
WORK_DIRS   := $(WORK_DIR) $(OBJS_DIR) $(DEPS_DIR) $(ISO_DIR) $(OBJS_DIR)dev $(DEPS_DIR)dev
SUB_DIRS    := $(BOOT_DIR)

C_SRCS      := elf.c time.c syscall.c fat_manip.c fat.c fs.c dev/ata.c dev/pci.c dev/acpi.c rgb8.c elist.c point.c aqueue.c buddy.c font.c graphic.c graphic_txt.c graphic_vbe.c interrupt.c memory.c paging.c proc.c dev/ps2.c tlsf.c utils.c window.c kernel.c
C_OBJS      := $(addprefix $(OBJS_DIR), $(C_SRCS:%.c=%.o))
C_DEPS      := $(addprefix $(DEPS_DIR), $(C_SRCS:%.c=%.d))
A_SRCS      := asm_functions.asm interrupt_asm.asm
A_OBJS      := $(addprefix $(OBJS_DIR), $(A_SRCS:%.asm=%.o))
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
CPPFLAGS  := -isystem$(INCLUDE_DIR)
# W_FLAGS   := -g -Weverything -pedantic
W_FLAGS   := -g -Wall -Wextra -Winit-self -Wconversion -Wno-unused-parameter -Wwrite-strings -Wno-sign-compare -Wno-pointer-sign -Wno-missing-field-initializers -Wcast-qual -Wformat=2 -Wstrict-aliasing=2 -Wdisabled-optimization -Wfloat-equal -Wpointer-arith -Wbad-function-cast -Wcast-align -Wredundant-decls -Winline
CFLAGS    := --target=i686-elf -m32 -std=c11 -nostdlib -ffreestanding -fno-builtin $(W_FLAGS)
LDFLAGS   := -Map $(KERNEL_MAP) -nostdlib
export CC := $(CLANG) $(CFLAGS) $(CPPFLAGS)
export PP := $(CLANG) $(CPPFLAGS) -x c -E -P


$(OBJS_DIR)%.o : %.c
	@test -d $(OBJS_DIR) || $(MKDIR) $(WORK_DIRS)
	@$(CC) -MMD -MQ $@ -MF ./$(DEPS_DIR)$*.d -c $< -o $@
	@echo $(patsubst -W%,,$(CC) -c $< -o $@)

$(OBJS_DIR)%.o : %.asm
	@test -d $(OBJS_DIR) || $(MKDIR) $(WORK_DIRS)
	$(eval TMP_FILE = $(shell mktemp))
	$(PP) $< > $(TMP_FILE)
	$(NASM) -l $(OBJS_DIR)$*.lst -o $@ $(TMP_FILE)


.PHONY: all
all: $(SUB_DIRS)
	$(MKDIR) $(WORK_DIRS)
	$(MAKE) $(ISO_NAME)
	ctags -R ./*


-include $(C_DEPS)
$(OBJS_DIR)asm_functions.o : $(MAKEFILE) asm_functions.asm $(INCLUDE_DIR)asm_functions.h $(INCLUDE_DIR)segment.h $(INCLUDE_DIR)paging.h
$(OBJS_DIR)interrupt_asm.o : $(MAKEFILE) interrupt_asm.asm $(INCLUDE_DIR)interrupt.h $(OBJS_DIR)interrupt.o


$(BOOT_OBJ) : $(MAKEFILE)
	$(MAKE) -C $(BOOT_DIR)


$(AXEL_BIN) : $(MAKEFILE) $(KERNEL_LNK) $(OBJS) $(DISK_IMG)
	$(eval TMP_FILE = $(shell mktemp))
	$(PP) $(KERNEL_LNK) > $(TMP_FILE)
	@$(LD) $(OBJS) $(LDFLAGS) -T $(TMP_FILE) -static -o $@
	@echo $(LD) $(notdir $(OBJS)) $(LDFLAGS) -T $(TMP_FILE) -static -o $@
	$(OBJCOPY) --only-keep-debug $(AXEL_BIN) $(AXEL_SYM)
	$(OBJCOPY) --strip-debug $(AXEL_BIN)
	$(MAKE) $(DISK_IMG)


$(ISO_NAME) : $(MAKEFILE) $(AXEL_BIN) $(GRUB_CONFIG) $(INIT)
	$(MKDIR) $(ISO_DIR)boot/grub
	cp $(AXEL_BIN) $(ISO_DIR)boot/
	cp $(GRUB_CONFIG) $(ISO_DIR)boot/grub/grub.cfg
	grub-mkimage -O i386-pc -o $(ISO_DIR)efi.img multiboot biosdisk iso9660
	grub-mkrescue -o $(ISO_NAME) $(ISO_DIR)


define FDISK
fdisk -u -C$(D_CYLINDER) -S63 -H16 $(DISK_IMG) <<\__EOF__
o
n
p
1

2048
t
b
w
__EOF__
endef
export FDISK

# $(DISK_IMG) :
# ifeq ($(OS_NAME), Linux)
# 	@# $(DISK_IMG) is NOT exist.
# 	@# 16 heads, 63 sectors/track, 512 bytes/sector, -> each cylinder has 516096 bytes (16 * 63 * 512)
# 	@# cylinder * 516096 = disk size
# 	@# cylinder = (disk size) / 516096
# 	dd if=/dev/zero of=$(DISK_IMG) bs=516096 count=$(D_CYLINDER)
# 	@echo "$$FDISK" | $(SHELL)
# 	@# Skip area of disk heads that include MBR.
# 	@# Maybe, fdisk default first LBA is 2048.
# 	@# 2048 * 512 = 1048576
# 	sudo losetup -o1048576 /dev/loop0 $(DISK_IMG)
# 	@# sudo mkfs.msdos -F 16 /dev/loop0
# 	sudo mkfs.msdos -F 32 /dev/loop0
# 	sudo losetup -d /dev/loop0
# else
# 	@echo "Please do on linux :("
# endif


$(INIT): $(INIT_FILE)
	nasm -f bin ./$(INIT_FILE) -o $(INIT)
	sudo losetup -o1048576 /dev/loop0 $(DISK_IMG)
	sudo mount /dev/loop0 /mnt
	sudo cp $(INIT) /mnt/
	sudo cp ./kernel.c /mnt/
	sudo mkdir -p /mnt/dir
	sudo umount /mnt
	sudo losetup -d /dev/loop0


.PHONY: clean
clean:
	$(MAKE) -C $(BOOT_DIR) clean
	$(RM) *.d *.o *.bin *.iso *.map *.lst *.log *.sym tags $(WORK_DIRS) bx_enh_dbg.ini
	ctags -R ./*


.PHONY: run_qemu_cdrom
run_qemu_cdrom: $(MAKEFILE) $(ISO_NAME)
ifeq ($(shell ls | grep $(DISK_IMG)), $(DISK_IMG))
	$(QEMU) -cdrom $(ISO_NAME) -hda $(DISK_IMG)
else
	$(QEMU) -cdrom $(ISO_NAME)
endif


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


LOADER2 := ./boot/loader2
LOADER2_SRC := ./boot/loader2.c
LOADER2_DEPS_OBJ := ./boot/loader2.o $(OBJS_DIR)asm_functions.o
# $(LOADER2): $(OBJS_DIR)asm_functions.o $(LOADER2_SRC)
.PHONY: disk
disk: $(MAKEFILE) $(AXEL_BIN) ./tools/image_util.c
	$(MAKE) -C $(TOOL_DIR)
	$(NASM) -f bin -I$(BOOT_DIR) ./boot/loader1.asm
	clang -I$(INCLUDE_DIR) -m32 -std=c11 -nostdlib -ffreestanding -fno-builtin -c ./boot/loader2.c -o ./boot/loader2.o
	ld -m elf_i386 --format elf32-i386 -nostartfiles -nodefaultlibs -nostdlib -T ./boot/loader2.ld -static -o ./boot/loader2 ./boot/loader2.o
	./tools/img_util -f 12 -t $(DISK_IMG) -m ./boot/loader1 -l ./boot/loader2 $(AXEL_BIN) ./init


.PHONY: write_fd
write_fd:
	$(MAKE) disk
	sudo dd if=$(DISK_IMG) of=/dev/sdb


.PHONY: run_fd
run_fd: $(MAKEFILE) $(DISK_IMG)
	$(MAKE) disk
	$(QEMU) -fda $(DISK_IMG)
