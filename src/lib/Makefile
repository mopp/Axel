# Files
LIB_MULTIBOOT := libmultiboot.rlib
LIBS          := $(LIB_MULTIBOOT)

.PHONY: all clean


all: $(LIBS)


$(LIB_MULTIBOOT): rust-multiboot/
	$(CD) $<; \
	$(CARGO) build -v --release --target $(TARGET_TRIPLE)
	$(CP) $</target/$(TARGET_TRIPLE)/release/$(LIB_MULTIBOOT) ./


clean:
	$(RM) $(LIBS)
	$(CD) rust-multiboot/ \
	$(RM) Cargo.lock
	$(RM) target
