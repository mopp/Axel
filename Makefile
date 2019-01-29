ARCH := x86_64

# Load target specific configs.
include ./config/Makefile.$(ARCH)

# Define commands.
export RM := rm -rf
export CC := gcc
CP        := cp
MAKE      := make
CARGO     := cargo
XARGO     := xargo
MKDIR     := mkdir -p
MKRESCUE  := grub-mkrescue
OBJCOPY   := objcopy --only-keep-debug
STRIP     := strip
GDB       := rust-gdb --nh --nx -q -x ./config/axel.gdb

# Define file names and config files.
AXEL_BIN  := axel.bin
AXEL_ISO  := axel.iso
AXEL_LIB  := target/$(TARGET_TRIPLE)/debug/libaxel.a
AXEL_MAP  := axel.map
ARCH_DIR  := src/arch/$(ARCH)
LINK_FILE := $(ARCH_DIR)/link.ld
BOOT_OBJ  := $(ARCH_DIR)/boot.o
GRUB_CFG  := config/grub.cfg

# Define Rust configs.
export RUST_TARGET_PATH := $(PWD)/config/
export RUSTFLAGS        := -Z no-landing-pads


.PHONY: all
all: $(AXEL_BIN)


$(AXEL_ISO): $(AXEL_BIN) $(GRUB_CFG)
	$(MKDIR) ./iso/boot/grub/
	$(CP) $(AXEL_BIN) ./iso/boot/
	$(CP) $(GRUB_CFG) ./iso/boot/grub/grub.cfg
	$(MKRESCUE) -o $@ ./iso/
	$(RM) ./iso/


$(AXEL_BIN): cargo $(BOOT_OBJ) $(LINK_FILE)
	$(LD) $(LD_FLAGS) -Wl,-Map=$(AXEL_MAP) -T $(LINK_FILE) -o $@ $(BOOT_OBJ) -L $(dir $(AXEL_LIB)) $(LIBS) -laxel
	$(OBJCOPY) $@ axel.sym
	$(STRIP) $@


$(BOOT_OBJ): $(BOOT_DEPS) $(LINK_FILE)
	$(MAKE) -C $(ARCH_DIR)


.PHONY: cargo
cargo:
	$(XARGO) build --target=$(TARGET_TRIPLE)


.PHONY: clean
clean:
	$(MAKE) -C $(ARCH_DIR) clean
	$(XARGO) clean
	$(RM) *.d *.o *.bin *.iso *.map *.lst *.log *.sym tags bx_enh_dbg.ini


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


# Xargo does not support building test.
# [Is there a way to do `xargo test`? · Issue #104 · japaric/xargo](https://github.com/japaric/xargo/issues/104)
.PHONY: test
test:
	$(CARGO) test


.PHONY: test_nocapture
test_nocapture:
	$(CARGO) test -- --nocapture


.PHONY: doc
doc:
	$(CARGO) rustdoc -- --no-defaults --passes strip-hidden --passes collapse-docs --passes unindent-comments --passes strip-priv-imports
