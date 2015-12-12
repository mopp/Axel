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
export MAKEFILE  := $(realpath $(ROOT_DIR))/Makefile
AXEL_BIN         := axel.bin
AXEL_ISO         := axel.iso
AXEL_MAP         := axel.map
GRUB_CFG         := grub.cfg
LINKER_FILE      := $(ARCH_DIR)/link.ld
RLIB_FILES       := $(RLIB_DIR)/libmultiboot.rlib $(RLIB_DIR)/libcore.rlib
DEPEND_SRC_FILES := graphic.rs ./arch/arm11/mod.rs ./arch/arm11/gpio.rs ./arch/x86_32/mod.rs
OBJ_FILES        := $(ARCH_DIR)/boot.o main.o

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
	--emit=obj					\
	--target=$(TARGET_TRIPLE)	\
	-C target-cpu=$(TARGET_CPU)	\
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


# Pattern rule to get object from rust code.
%.o: %.rs $(DEPEND_SRC_FILES) $(MAKEFILE)
	$(RUSTC) $(RUSTC_FLAGS) -o $@ $<


# Pattern rule for building architecture depending codes.
$(ARCH_DIR)/%.o:
	$(MAKE) -C $(ARCH_DIR)


%.rlib: $(MAKEFILE)
ifeq ($(TARGET_TRIPLE),arm-unknown-none-eabi)
	$(RUSTC) $(RUSTC_FLAGS_RLIB) $(RLIB_DIR)/rust/src/libcore/lib.rs
endif
	$(RUSTC) $(RUSTC_FLAGS_RLIB) $(RLIB_DIR)/rust-multiboot/src/lib.rs


.PHONY: all run_kernel run_cdrom test clean distclean

all: $(AXEL_BIN)


$(AXEL_BIN): $(MAKEFILE) $(RLIB_FILES) $(OBJ_FILES) $(LINKER_FILE)
	$(LD) $(LD_FLAGS) -Wl,-Map=$(AXEL_MAP) -T $(LINKER_FILE) -o $@ $(OBJ_FILES) $(LIBS)
ifeq ($(TARGET_TRIPLE),arm-unknown-none-eabi)
	$(OBJCOPY) $@ -O binary kernel.img
endif


$(AXEL_ISO): $(MAKEFILE) $(AXEL_BIN) $(GRUB_CFG)
	$(MKDIR) ./iso/boot/grub/
	$(CP) $(AXEL_BIN) ./iso/boot/
	$(CP) $(GRUB_CFG) ./iso/boot/grub/grub.cfg
	$(MKIMAGE) --format i386-pc -o ./iso/efi.img multiboot biosdisk iso9660
	$(MKRESCUE) -o $@ ./iso/
	$(RM) iso/


run_kernel: $(AXEL_BIN)
	$(QEMU) $(QEMU_FLAGS) --kernel $<


run_cdrom: $(AXEL_ISO)
	$(QEMU) $(QEMU_FLAGS) -cdrom $<


$(TEST_DIR):
	$(MKDIR) $@


test: $(DEPEND_SRC_FILES) $(TEST_DIR)
	$(foreach src_file, $(DEPEND_SRC_FILES), 					\
		$(RUSTC) --test --out-dir $(TEST_DIR) $(src_file) &&	\
		$(TEST_DIR)/$(basename $(src_file)) --nocapture			\
	)


clean:
	$(MAKE) -C $(ARCH_DIR) clean
	$(RM) *.d *.o *.bin *.iso *.map *.lst *.log *.sym tags $(TEST_DIR) bx_enh_dbg.ini


distclean:
	$(MAKE) clean
	$(RM) $(RLIB_DIR)/*.rlib
