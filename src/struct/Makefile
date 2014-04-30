MAKEFILE 	:= Makefile
OBJS		:= list.o queue.o stack.o
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


list.o	: list.c  	$(MAKEFILE) $(AXEL_INCLUDE_PATH)list.h
queue.o	: queue.c 	$(MAKEFILE) $(AXEL_INCLUDE_PATH)queue.h list.c $(AXEL_INCLUDE_PATH)list.h
stack.o	: stack.c 	$(MAKEFILE) $(AXEL_INCLUDE_PATH)stack.h


.PHONY: clean
clean:
	$(RM) *.o *.a
	$(RM) $(AXEL_LIB_PATH)$(TARGET)
