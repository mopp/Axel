include ../globals.mk

CC = $(GCC) -x c -E
BOOT_SRC = multiboot.asm
BOOT_OBJ = multiboot.o
BOOT_TMP = multiboot.tmp

default : $(MAKEFILE)
	make $(BOOT_OBJ)

$(BOOT_OBJ) : $(BOOT_SRC)
	$(CC) $(BOOT_SRC) > $(BOOT_TMP)
	$(NASM) -l $*.lst -o $(BOOT_OBJ) $(BOOT_TMP)
	$(RM) $(BOOT_TMP)

.PHONY: clean
clean :
	$(RM) *.o *.tmp *.lst
