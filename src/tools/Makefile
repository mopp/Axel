C_DEPS         := ../fat_manip.c
IMG_SRC        := image_util.c
IMG_PROG       := img_util
IMG_NAME       := disk.img
MBR_FILE       := loader1
LOADER2_FILE   := loader1
OS_EXE_FILE    := axel.bin
FOR_CHECK_FILE := tmp

$(IMG_PROG): $(IMG_SRC) $(C_DEPS) Makefile
	clang -std=c11 -lm -Wall -O2 -D FOR_IMG_UTIL $(C_DEPS) $(IMG_SRC) -o $(IMG_PROG)

.PHONY: run
run: $(IMG_PROG)
	./$(IMG_PROG) -f 32 -t $(IMG_NAME) -m $(MBR_FILE) -l $(LOADER2_FILE) $(OS_EXE_FILE)

.PHONY: check
check:
	# dd if=$(IMG_NAME) of=tmp bs=512 skip=2 count=2878
	dd if=$(IMG_NAME) of=tmp bs=512 skip=2 count=66553
	fsck.fat -v -n $(FOR_CHECK_FILE)

.PHONY: mount
mount:
	sudo losetup /dev/loop0 $(FOR_CHECK_FILE)
	sudo mount /dev/loop0 /mnt

.PHONY: umount
umount:
	sudo losetup -d /dev/loop0
	sudo umount /mnt
