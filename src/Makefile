#############################################################
# @file Makefile
# @brief Building Axel.
# @author mopp
# @version 0.2
# @date 2015-10-06
#############################################################

# Target architecture.
ARCH := x86_32
# ARCH := arm11

# Directories
export ROOT_DIR := .
export RLIB_DIR := $(ROOT_DIR)/lib
export TEST_DIR := $(ROOT_DIR)/tests
export ARCH_DIR := $(ROOT_DIR)/arch/$(ARCH)

# Files
RLIB_FILES        := $(RLIB_DIR)/libmultiboot.rlib
DEPEND_SRC_FILES  := graphic.rs
OBJ_FILES         := $(ARCH_DIR)/boot.o main.o

# Load other config and target specific config.
include Makefile.common
include Makefile.$(ARCH)


# Pattern rule to get object from rust code.
%.o: %.rs $(DEPEND_SRC_FILES)
	$(RUSTC) $(RUSTC_FLAGS) -o $@ $<


# Pattern rule for building architecture depending codes.
$(ARCH_DIR)/%.o:
	$(MAKE) -C $(ARCH_DIR)


# Pattern rule for building rust libraries.
%.rlib:
	$(MAKE) -C $(RLIB_DIR)


.PHONY: all run_kernel run_cdrom test clean

all: $(AXEL_BIN)


$(AXEL_BIN): $(MAKEFILE) $(RLIB_FILES) $(OBJ_FILES) $(LINKER_FILE)
	$(LD) $(LD_FLAGS) -Wl,-Map=$(AXEL_MAP) -T $(LINKER_FILE) -o $@ $(OBJ_FILES) $(LIBS)


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
	$(MAKE) -C $(RLIB_DIR) clean
	$(RM) *.d *.o *.bin *.iso *.map *.lst *.log *.sym tags $(TEST_DIR) bx_enh_dbg.ini
