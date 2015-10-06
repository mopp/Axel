#############################################################
# @file Makefile
# @brief Building Axel.
# @author mopp
# @version 0.2
# @date 2015-10-06
#############################################################

# Commands
export RM       := rm -rf
export CP       := cp -r
export MKDIR    := mkdir -p
export LD       := ld
export RUSTC    := rustc
export NASM     := nasm
export QEMU     := qemu-system-i386
export BOCHS    := bochs -q
export MAKE     := make --warn-undefined-variables
export MKIMAGE  := grub-mkimage
export MKRESCUE := grub-mkrescue

# Options
export ARCH := x86_32
RUSTC_FLAGS := --target=i686-unknown-linux-gnu --emit=obj -C lto -C opt-level=3 -Z no-landing-pads
LD_FLAGS    := -m elf_i386 --format elf32-i386 -nostartfiles -nodefaultlibs -nostdlib -static
QEMU_FLAGS  := -monitor stdio -vga std -m 32 -boot order=dc -no-reboot -d int

# Directories
export BUILD_OS_NAME := $(shell uname -s)
export ROOT_DIR      := $(realpath .)
export ARCH_DIR      := $(ROOT_DIR)/arch/$(ARCH)

# Files
export MAKEFILE := Makefile
AXEL_BIN        := axel.bin
AXEL_ISO        := axel.iso
GRUB_CFG        := grub.cfg
LINKER_FILE     := $(ARCH_DIR)/link.ld
OBJ_FILES       := 		\
	$(ARCH_DIR)/boot.o	\
	main.o

.PHONY: all run clean


# General rules.
%.o: %.rs
	$(RUSTC) $(RUSTC_FLAGS) -o $@ $<


$(ARCH_DIR)/%.o:
	$(MAKE) -C $(ARCH_DIR)


all: $(AXEL_ISO)


$(AXEL_BIN): $(MAKEFILE) $(OBJ_FILES) $(LINKER_FILE)
	$(LD) $(LD_FLAGS) -Map axel.map -T $(LINKER_FILE) -o $@ $(OBJ_FILES)


$(AXEL_ISO): $(MAKEFILE) $(AXEL_BIN) $(GRUB_CFG)
	$(MKDIR) ./iso/boot/grub/
	$(CP) $(AXEL_BIN) ./iso/boot/
	$(CP) $(GRUB_CFG) ./iso/boot/grub/grub.cfg
	$(MKIMAGE) --format i386-pc -o ./iso/efi.img multiboot biosdisk iso9660
	$(MKRESCUE) -o $@ ./iso/
	$(RM) iso/


run: $(AXEL_ISO)
	$(QEMU) $(QEMU_FLAGS) -cdrom $(AXEL_ISO)


clean:
	$(MAKE) -C $(ARCH_DIR) clean
	$(RM) *.d *.o *.bin *.iso *.map *.lst *.log *.sym tags $(WORK_DIR) bx_enh_dbg.ini
