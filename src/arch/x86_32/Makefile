#############################################################
# @file Makefile
# @brief Building the part of Axel for x86_32.
# @author mopp
# @version 0.2
# @date 2015-10-06
#############################################################

.PHONY: all clean


boot.o: boot.asm $(MAKEFILE)
	nasm -f elf32 -l $*.lst -o $@ $<

clean:
	$(RM) *.o *.lst
