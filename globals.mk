# := で 代入時に展開
MAKEFILE 			:= Makefile
AXEL_INCLUDE_PATH 	:= $(AXEL_ROOT_DIR)include/
TOOL_PATH 			:= /home/mopp/.mopp/

RM 					:= rm -rf
GCC					:= i686-elf-gcc -I$(AXEL_INCLUDE_PATH) -I$(TOOL_PATH)include/ -L$(TOOL_PATH)lib/
NASM 				:= nasm -f elf32
LIB_ASM				:= libasmf.a
AR					:= i686-elf-ar
RANLIB				:= i686-elf-ranlib
QEMU_EXE 			:= qemu-system-x86_64 -monitor stdio -vga std
