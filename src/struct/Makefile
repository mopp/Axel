MAKEFILE 	:= Makefile
OBJS		:= dlist.o aqueue.o
TARGET 		:= libstruct.a

.SUFFIXES: .c .o
.c.o:
	$(CC) -c $< -o $@


.PHONY: default
default:
	make $(TARGET)


$(TARGET): $(OBJS)
	$(AR_R) $(TARGET) $(OBJS)
	cp $(TARGET) $(AXEL_LIB_PATH)

list.o		: dlist.c  	$(MAKEFILE) $(AXEL_INCLUDE_PATH)dlist.h
aqueue.o	: aqueue.c 	$(MAKEFILE) $(AXEL_INCLUDE_PATH)aqueue.h


.PHONY: clean
clean:
	$(RM) *.o *.a *.sym
	$(RM) $(AXEL_LIB_PATH)$(TARGET)
