C_DEPS       := ../fat_manip.c
IMG_SRC      := image_util.c
IMG_PROG     := img_util
IMG_NAME     := disk.img
MBR_FILE     := loader1
LOADER2_FILE := loader1
OS_EXE_FILE  := axel.bin

$(IMG_PROG): $(IMG_SRC) $(C_DEPS)
	clang -Wall -O2 -D FOR_IMG_UTIL $(C_DEPS) $(IMG_SRC) -o $(IMG_PROG)

.PHONY: run
run: $(IMG_PROG)
	./$(IMG_PROG) $(IMG_NAME) $(MBR_FILE) $(LOADER2_FILE) $(OS_EXE_FILE)

.PHONY: check
check:
	dd if=$(IMG_NAME) of=tmp bs=512 skip=2 count=67580
	fsck.fat -v -n tmp

.PHONY: mount
mount:
	sudo losetup -o1024 /dev/loop0 ./disk.img
	sudo mount /dev/loop0 /mnt

.PHONY: umount
umount:
	sudo losetup -d /dev/loop0
	sudo umount /mnt
