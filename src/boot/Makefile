BOOT_SRC 	:= multiboot.asm
BOOT_OBJ 	:= multiboot.o

.PHONY: default
default : $(MAKEFILE)
	make $(BOOT_OBJ)

$(BOOT_OBJ) : $(BOOT_SRC)
	$(eval TMP_FILE = $(shell mktemp))
	$(PP) $(BOOT_SRC) > $(TMP_FILE)
	$(NASM) -l $*.lst -o $(BOOT_OBJ) $(TMP_FILE)

.PHONY: clean
clean :
	$(RM) *.o *.lst
