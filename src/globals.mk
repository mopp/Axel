# := で 代入時に展開

# Path
TOOL_PATH 			:= /home/mopp/.mopp/axel/

# command
RM 					:= rm -rf
GCC					:= $(TOOL_PATH)bin/i686-elf-gcc
NASM 				:= nasm -f elf32
AR					:= $(TOOL_PATH)bin/i686-elf-ar
RANLIB				:= $(TOOL_PATH)bin/i686-elf-ranlib
QEMU 				:= qemu-system-x86_64 -monitor stdio
BOCHS				:= bochs
