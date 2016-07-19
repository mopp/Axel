#############################################################
# @file Makefile
# @brief Building Axel.
# @author mopp
# @version 0.2.0
# @date 2016-07-18
#############################################################

ARCH := x86_32

# Load target specific configs.
include ./config/Makefile.$(ARCH)

# Commands.
export RM   := rm -rf
CD          := cd
MAKE        := make
CARGO       := cargo
CARGO_BUILD := cargo build --target=$(TARGET_TRIPLE)

# Axel configs.
AXEL_BIN  := axel.bin
AXEL_LIB  := target/$(TARGET_TRIPLE)/debug/libaxel.a
AXEL_MAP  := axel.map
ARCH_DIR  := src/arch/$(ARCH)
LINK_FILE := $(ARCH_DIR)/link.ld
BOOT_OBJ  := $(ARCH_DIR)/boot.o
GRUB_CFG  := config/grub.cfg

# Rust configs
CARGO_TOML     := Cargo.toml
RUST_CORE_REPO := lib/rust-core/
RUST_CORE_PATH := lib/rust-core/target/$(TARGET_TRIPLE)/release/

export RUST_TARGET_PATH := $(PWD)/config/
export RUSTFLAGS        := -L$(RUST_CORE_PATH) -g -C opt-level=0 -Z no-landing-pads



# Pattern rule for building architecture depending codes.
$(ARCH_DIR)/%.o:
	$(MAKE) -C $(ARCH_DIR)


.PHONY: all
all: $(AXEL_BIN)


$(AXEL_BIN): $(AXEL_LIB) $(BOOT_OBJ) $(LINK_FILE)
	$(LD) $(LD_FLAGS) -Wl,-Map=$(AXEL_MAP) -T $(LINK_FILE) -o $@ $(BOOT_OBJ) -L $(dir $(AXEL_LIB)) $(LIBS) -laxel


$(AXEL_LIB): $(RUST_CORE_PATH) $(CARGO_TOML) cargo


$(RUST_CORE_PATH):
	$(CD) $(RUST_CORE_REPO) ;\
	$(CARGO_BUILD) --release


.PHONY: cargo
cargo:
	$(CARGO_BUILD)


.PHONY: clean
clean:
	$(MAKE) -C $(ARCH_DIR) clean
	$(CARGO) clean
	$(RM) Cargo.lock
	$(RM) *.d *.o *.bin *.iso *.map *.lst *.log *.sym tags $(TEST_DIR) bx_enh_dbg.ini


.PHONY: distclean
distclean:
	$(MAKE) clean
	$(CD) $(RUST_CORE_REPO) ;\
	$(CARGO) clean ;\
	$(RM) Cargo.lock


.PHONY: run_kernel
run_kernel: $(AXEL_BIN)
	$(MAKE) $(AXEL_BIN)
	$(QEMU) $(QEMU_FLAGS) --kernel $<


$(AXEL_ISO): $(AXEL_BIN) $(GRUB_CFG)
	$(MKDIR) ./iso/boot/grub/
	$(CP) $(AXEL_BIN) ./iso/boot/
	$(CP) $(GRUB_CFG) ./iso/boot/grub/grub.cfg
	$(MKIMAGE) --format i386-pc -o ./iso/efi.img multiboot biosdisk iso9660
	$(MKRESCUE) -o $@ ./iso/
	$(RM) ./iso/


.PHONY: run_cdrom
run_cdrom: $(AXEL_ISO)
	$(QEMU) $(QEMU_FLAGS) -cdrom $<


.PHONY: test
test:
	$(CARGO) test


.PHONY: test_nocapture
test_nocapture:
	$(CARGO) test -- --nocapture
