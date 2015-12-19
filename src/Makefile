#############################################################
# @file Makefile
# @brief Building Axel.
# @author mopp
# @version 0.2
# @date 2015-10-06
#############################################################

# Target architecture.
# ARCH := x86_32
ARCH := arm11

# Commands
export RM       := rm -rf
export CD       := cd
export CP       := cp -r
export MAKE     := make --warn-undefined-variables
export MKDIR    := mkdir -p
export CARGO    := cargo
export NASM     := nasm
export RUSTC    := rustc
export BOCHS    := bochs -q
export MKIMAGE  := grub-mkimage
export MKRESCUE := grub-mkrescue

# Directories
export ROOT_DIR := .
export RLIB_DIR := $(ROOT_DIR)/lib
export TEST_DIR := $(ROOT_DIR)/tests
export ARCH_DIR := $(ROOT_DIR)/arch/$(ARCH)

# Files
export MAKEFILE := $(realpath $(ROOT_DIR))/Makefile
AXEL_BIN        := axel.bin
AXEL_ISO        := axel.iso
AXEL_MAP        := axel.map
GRUB_CFG        := grub.cfg
LINKER_FILE     := $(ARCH_DIR)/link.ld
RUST_LIBS       := $(RLIB_DIR)/libcore.rlib $(RLIB_DIR)/libmultiboot.rlib
RUST_SRCS       := main.rs
RUST_OBJS       := $(RUST_SRCS:%.rs=%.o)
RUST_DEPS       := $(RUST_SRCS:%.rs=%.d)
OBJS            := $(ARCH_DIR)/boot.o $(RUST_OBJS)

# Load target specific config.
include Makefile.$(ARCH)

# rustc options
# -C means codegen option, almost option is passed to llvm tools (e.g., lli and llc).
# Please refer this link http://linux.die.net/man/1/lli
# 	-g 						Output full debug info.
# 	-C lto					Perform llvm link-time optimizations
#	-C opt-level=3			Optimize with possible level 3 (0-3 are available)
#	-C code-model=kernel	This option is described in System V ABI Reference 0.99, kernel code-model optimize symbols virtual addresses.
#	-C soft-float			Generate software floating point library calls
#	-C target-cpu=i386		Select target processor (llc -mcpu=help for details)
#	-Z no-landing-pads		Omit landing pads for unwinding
RUSTC_FLAGS := 					\
	$(RUSTC_CFG)				\
	-L $(RLIB_DIR)				\
	-g 							\
	--target=$(TARGET_TRIPLE)	\
	-C lto						\
	-C opt-level=3				\
	-Z no-landing-pads

RUSTC_FLAGS_RLIB := 			\
	-L $(RLIB_DIR)				\
	--out-dir $(RLIB_DIR)		\
	-A unstable-features		\
	-A dead-code 				\
	-A stable_features			\
	-A unused_features			\
	--target=$(TARGET_TRIPLE)


# Pattern rule for building architecture depending codes.
$(ARCH_DIR)/%.o:
	$(MAKE) -C $(ARCH_DIR)


# Pattern rule for emitting object file from rust file.
%.o: %.rs $(MAKEFILE) $(RUST_LIBS)
	$(RUSTC) $(RUSTC_FLAGS) --emit=dep-info,obj $<


.PHONY: all
all: $(AXEL_BIN)


# Include rust dependency information.
# Even if files not found, make does not output error.
-include $(RUST_DEPS)


$(RLIB_DIR)/libcore.rlib: $(RLIB_DIR)/rust/src/libcore/lib.rs $(MAKEFILE)
	$(RUSTC) $(RUSTC_FLAGS_RLIB) $<


$(RLIB_DIR)/libmultiboot.rlib: $(RLIB_DIR)/rust-multiboot/src/lib.rs $(MAKEFILE)
	$(RUSTC) $(RUSTC_FLAGS_RLIB) $<


$(AXEL_BIN): $(OBJS) $(LINKER_FILE)
	$(LD) $(LD_FLAGS) -Wl,-Map=$(AXEL_MAP) -T $(LINKER_FILE) -o $@ $(OBJS) $(LIBS)


$(AXEL_ISO): $(AXEL_BIN) $(GRUB_CFG)
	$(MKDIR) ./iso/boot/grub/
	$(CP) $(AXEL_BIN) ./iso/boot/
	$(CP) $(GRUB_CFG) ./iso/boot/grub/grub.cfg
	$(MKIMAGE) --format i386-pc -o ./iso/efi.img multiboot biosdisk iso9660
	$(MKRESCUE) -o $@ ./iso/
	$(RM) ./iso/


.PHONY: run_kernel
run_kernel: $(AXEL_BIN)
	$(QEMU) $(QEMU_FLAGS) --kernel $<


.PHONY: run_cdrom
run_cdrom: $(AXEL_ISO)
	$(QEMU) $(QEMU_FLAGS) -cdrom $<


.PHONY: write
write: $(AXEL_BIN)
	$(OBJCOPY) $< -O binary kernel.img
	sudo mount /dev/mmcblk0p1 /mnt/
	sudo sync
	sudo $(CP) kernel.img /mnt/kernel.img
	sudo $(CP) kernel.img /mnt/kernel7.img
	sudo umount /mnt/
	$(RM) kernel.img


.PHONY: clean
clean:
	$(MAKE) -C $(ARCH_DIR) clean
	$(RM) *.d *.o *.bin *.iso *.map *.lst *.log *.sym tags $(TEST_DIR) bx_enh_dbg.ini


.PHONY: distclean
distclean:
	$(MAKE) clean
	$(RM) $(RLIB_DIR)/*.rlib


$(TEST_DIR):
	$(MKDIR) $@


.PHONY: test
RUST_TESTS := graphic.rs
test: $(RUST_TESTS) $(TEST_DIR)
	$(foreach src, $(RUST_TESTS), 						\
		$(RUSTC) --test --out-dir $(TEST_DIR) $(src) &&	\
		$(TEST_DIR)/$(basename $(src)) --nocapture		\
	)
