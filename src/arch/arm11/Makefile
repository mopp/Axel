#############################################################
# @file Makefile
# @brief Building the part of Axel for arm11.
# @author mopp
# @version 0.2
#############################################################

.PHONY: all clean


boot.o: boot.S $(MAKEFILE)
	$(AS) $(AS_FLAGS) -c $< -o $@

clean:
	$(RM) *.o *.lst
