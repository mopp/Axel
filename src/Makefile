#############################################################
# @file Makefile
# @brief Building Axel.
# @author mopp
# @version 0.2
# @date 2015-10-06
#############################################################

export ARCH          := x86_32
export TARGET_TRIPLE := i686-unknown-linux-gnu

# Commands
export RM       := rm -rf
export CD       := cd
export CP       := cp -r
export MKDIR    := mkdir -p
export LD       := ld
export RUSTC    := rustc
export CARGO    := cargo
export NASM     := nasm
export QEMU     := qemu-system-i386
export BOCHS    := bochs -q
export MAKE     := make --warn-undefined-variables
export MKIMAGE  := grub-mkimage
export MKRESCUE := grub-mkrescue

# Directories
export BUILD_OS_NAME := $(shell uname -s)
export ROOT_DIR      := $(realpath .)
export ARCH_DIR      := $(ROOT_DIR)/arch/$(ARCH)
export RLIB_DIR      := $(ROOT_DIR)/lib
export TEST_DIR      := $(ROOT_DIR)/tests

# Options
# 	For `rustc`
# 	-C means codegen option, almost option is passed to llvm tools (e.g., lli and llc).
# 	Please refer this link http://linux.die.net/man/1/lli
# 		-g 						Output full debug info.
# 		-C lto					Perform llvm link-time optimizations
#		-C opt-level=3			Optimize with possible level 3 (0-3 are available)
#		-C code-model=kernel	This option is described in System V ABI Reference 0.99, kernel code-model optimize symbols virtual addresses.
#		-C soft-float			Generate software floating point library calls
#		-C target-cpu=i386		Select target processor (llc -mcpu=help for details)
#		-Z no-landing-pads		Omit landing pads for unwinding
RUSTC_FLAGS := -L $(RLIB_DIR) --target=$(TARGET_TRIPLE) --emit=obj -g -C code-model=kernel -C lto -C soft-float -C opt-level=3 -C target-cpu=i386 -Z no-landing-pads
LD_FLAGS    := -m elf_i386 --format elf32-i386 -nostartfiles -nodefaultlibs -nostdlib -static
QEMU_FLAGS  := -monitor stdio -vga std -m 32 -boot order=dc -no-reboot -d int

# Files
export MAKEFILE   := Makefile
AXEL_BIN          := axel.bin
AXEL_ISO          := axel.iso
GRUB_CFG          := grub.cfg
LINKER_FILE       := $(ARCH_DIR)/link.ld
RLIB_FILES        := $(RLIB_DIR)/libmultiboot.rlib
DEPEND_SRC_FILES  := graphic.rs
OBJ_FILES         := 	\
	$(ARCH_DIR)/boot.o	\
	main.o


.PHONY: all run clean


# Pattern rule to get object from rust code.
%.o: %.rs $(DEPEND_SRC_FILES)
	$(RUSTC) $(RUSTC_FLAGS) -o $@ $<


# Pattern rule for building architecture depending codes.
$(ARCH_DIR)/%.o:
	$(MAKE) -C $(ARCH_DIR)


# Pattern rule for building rust libraries.
%.rlib:
	$(MAKE) -C $(RLIB_DIR)


all: $(AXEL_ISO)


$(AXEL_BIN): $(MAKEFILE) $(RLIB_FILES) $(OBJ_FILES) $(LINKER_FILE)
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


$(TEST_DIR):
	$(MKDIR) $@


test: $(DEPEND_SRC_FILES) $(TEST_DIR)
	$(foreach src_file, $(DEPEND_SRC_FILES), 					\
		$(RUSTC) --test --out-dir $(TEST_DIR) $(src_file) &&	\
		$(TEST_DIR)/$(basename $(src_file)) --nocapture			\
	)


clean:
	$(MAKE) -C $(ARCH_DIR) clean
	$(MAKE) -C $(RLIB_DIR) clean
	$(RM) *.d *.o *.bin *.iso *.map *.lst *.log *.sym tags $(TEST_DIR) bx_enh_dbg.ini
