include ../globals.mk

FUNC_ASMS 	:= functions.asm
FUNC_OBJS 	:= functions.o

%.o: %.asm $(MAKEFILE)
	$(NASM) -l $*.lst $<

default: $(MAKEFILE) $(FUNC_ASMS)
	make $(LIB_ASM)

$(LIB_ASM): $(MAKEFILE) $(FUNC_ASMS)
	@for target in $(FUNC_OBJS) ; do \
		(make --no-print-directory $$target); \
		done
	$(AR) r $@ $(FUNC_OBJS)
	$(RANLIB) $@

.PHONY: clean
clean :
	$(RM) *.o *.lst *.a
