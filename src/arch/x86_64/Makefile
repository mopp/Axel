#############################################################
# @file Makefile
# @brief Building the part of Axel for x86_32.
# @author mopp
# @version 0.2
#############################################################


boot.o: boot.asm $(MAKEFILE)
	nasm -g -f elf64 -l $*.lst -o $@ $<


.PHONY: clean
clean:
	$(RM) *.o *.lst
