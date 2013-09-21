export AXEL_ROOT_DIR	:= $(shell pwd)/
include globals.mk

CC 				:= $(GCC) -fno-builtin -ffreestanding -Wall -Wextra -std=c11
KERNEL_LNK	 	:= kernel.ld
KERNEL_MAP	 	:= kernel.map

BOOT_DIR		:= ./boot/
BOOT_OBJ 		:= $(BOOT_DIR)multiboot.o
GRUB_CONFIG 	:= $(BOOT_DIR)grub.cfg

ASM_FUNC_DIR	:= ./asm_functions/

LD 				:= $(GCC) -T $(KERNEL_LNK) -Wl,-Map,$(KERNEL_MAP)
# XXX 順序に注意
LINK_OBJS 		:= $(BOOT_OBJ) string.o graphic_txt.o kernel.o $(ASM_FUNC_DIR)$(LIB_ASM)

AXEL_BIN 		:= axel.bin
ISO_NAME 		:= axel.iso
ISO_DIR 		:= iso_dir

SUBDIRS 		:= $(BOOT_DIR) $(ASM_FUNC_DIR)


.c.o:
	$(CC) -c $< -o $@


default :
	make all
	make $(ISO_NAME)
	ctags -R ./*

string.o		: string.c 		$(MAKEFILE) $(AXEL_INCLUDE_PATH)string.h
graphic_txt.o	: graphic_txt.c $(MAKEFILE) $(AXEL_INCLUDE_PATH)graphic_txt.h
kernel.o 		: kernel.c 		$(MAKEFILE) $(AXEL_INCLUDE_PATH)kernel.h

$(BOOT_OBJ) : $(MAKEFILE)
	make -C $(BOOT_DIR)

$(AXEL_BIN) : $(MAKEFILE) $(KERNEL_LNK) $(LINK_OBJS)
	$(LD) -nostdlib -ffreestanding -lgcc $(LINK_OBJS) -o $@

$(ISO_NAME) : $(MAKEFILE) $(AXEL_BIN) $(GRUB_CONFIG)
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(AXEL_BIN) $(ISO_DIR)/boot/
	cp $(GRUB_CONFIG) $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_NAME) $(ISO_DIR)

.PHONY: all $(SUBDIRS)
all: $(SUBDIRS)
	@for dir in $(SUBDIRS) ; do \
		($(MAKE) -C $$dir); \
		done

.PHONY: clean
clean:
	@for dir in $(SUBDIRS) ; do \
		($(MAKE) -C $$dir clean); \
		done
	$(RM) *.o *.bin *.iso *.map $(ISO_DIR) tags
	ctags -R ./*

.PHONY: run
run : $(MAKEFILE) $(ISO_NAME)
	$(QEMU_EXE) -cdrom $(ISO_NAME)
