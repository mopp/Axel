#############################################################
# @file Makefile
# @brief Building Axel.
# @author mopp
# @version 0.2.0
#############################################################

ARCH := x86_64

# Load target specific configs.
include ./config/Makefile.$(ARCH)

# Commands.
export RM   := rm -rf
CD          := cd
CP          := cp
MAKE        := make
RUSTC       := rustc --target=$(TARGET_TRIPLE)
CARGO       := xargo
CARGO_BUILD := $(CARGO) build --target=$(TARGET_TRIPLE)
CARGO_TEST  := RUSTFLAGS='' $(CARGO) test
MKDIR       := mkdir -p
MKRESCUE    := grub-mkrescue
OBJCOPY     := objcopy --only-keep-debug
STRIP       := strip
TOUCH       := touch --no-create
GDB         := rust-gdb --nh --nx -q -x ./config/axel.gdb

# Axel configs.
AXEL_BIN  := axel.bin
AXEL_ISO  := axel.iso
AXEL_LIB  := target/$(TARGET_TRIPLE)/debug/libaxel.a
AXEL_MAP  := axel.map
ARCH_DIR  := src/arch/$(ARCH)
LINK_FILE := $(ARCH_DIR)/link.ld
BOOT_OBJ  := $(ARCH_DIR)/boot.o
GRUB_CFG  := config/grub.cfg

# Rust configs
CARGO_TOML              := Cargo.toml
RUST_REPO               := ./lib/rust
export RUST_TARGET_PATH := $(PWD)/config/
export RUSTFLAGS        := -Z no-landing-pads


.PHONY: all
all: $(AXEL_BIN)


$(AXEL_ISO): $(AXEL_BIN) $(GRUB_CFG)
	$(MKDIR) ./iso/boot/grub/
	$(CP) $(AXEL_BIN) ./iso/boot/
	$(CP) $(GRUB_CFG) ./iso/boot/grub/grub.cfg
	$(MKRESCUE) -o $@ ./iso/ 2> /dev/null
	$(RM) ./iso/


$(AXEL_BIN): $(AXEL_LIB) $(BOOT_OBJ) $(LINK_FILE)
	$(LD) $(LD_FLAGS) -Wl,-Map=$(AXEL_MAP) -T $(LINK_FILE) -o $@ $(BOOT_OBJ) -L $(dir $(AXEL_LIB)) $(LIBS) -laxel
	$(OBJCOPY) $@ axel.sym
	$(STRIP) $@


$(AXEL_LIB): cargo
	$(TOUCH) $(AXEL_LIB)


$(BOOT_OBJ): $(BOOT_DEPS)
	$(MAKE) -C $(ARCH_DIR)


.PHONY: cargo
cargo: $(CARGO_TOML)
	$(CARGO_BUILD)


.PHONY: clean
clean:
	$(MAKE) -C $(ARCH_DIR) clean
	$(CARGO) clean
	$(RM) Cargo.lock
	$(RM) *.d *.o *.bin *.iso *.map *.lst *.log *.sym tags bx_enh_dbg.ini


.PHONY: distclean
distclean:
	$(MAKE) clean


.PHONY: run_kernel
run_kernel: $(AXEL_BIN)
	$(QEMU) $(QEMU_FLAGS) --kernel $<


.PHONY: run_cdrom
run_cdrom: $(AXEL_ISO)
	$(QEMU) $(QEMU_FLAGS) -cdrom $<


.PHONY: run_cdrom_debug
run_cdrom_debug: $(AXEL_ISO)
	$(QEMU) $(QEMU_FLAGS) -s -S -cdrom $<


.PHONY: gdb
gdb:
	$(GDB)


.PHONY: test
test:
	$(CARGO_TEST)


.PHONY: test_nocapture
test_nocapture:
	$(CARGO_TEST) -- --nocapture


.PHONY: doc
doc:
	$(CARGO) rustdoc -- --no-defaults --passes strip-hidden --passes collapse-docs --passes unindent-comments --passes strip-priv-imports
