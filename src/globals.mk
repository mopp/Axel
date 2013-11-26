# := で 代入時に展開

# Path
TOOL_PATH 			:= /home/mopp/.mopp/

# command
RM 					:= rm -rf
GCC					:= i686-elf-gcc
NASM 				:= nasm -f elf32
AR					:= i686-elf-ar
RANLIB				:= i686-elf-ranlib
QEMU 				:= qemu-system-x86_64 -monitor stdio
BOCHS				:= bochs
