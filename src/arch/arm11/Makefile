#############################################################
# @file Makefile
# @brief Building the part of Axel for arm11.
# @author mopp
# @version 0.2
# @date 2015-12-10
#############################################################

.PHONY: all clean

boot.o: boot.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o *.lst
