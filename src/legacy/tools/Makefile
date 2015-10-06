C_DEPS         := $(SRC_DIR)fat_manip.c
FOR_CHECK_FILE := tmp
IMG_PROG       := img_util
IMG_SRC        := image_util.c

$(IMG_PROG): $(MAKEFILE) $(IMG_SRC) $(C_DEPS)
	$(CC) -lm -Wall -O2 -D FOR_IMG_UTIL $(C_DEPS) $(IMG_SRC) -o $(IMG_PROG)


.PHONY: clean
clean:
	$(RM) $(FOR_CHECK_FILE) $(IMG_PROG)



# For debug
.PHONY: run
run: $(IMG_PROG)
	./$(IMG_PROG) -f 12 -t $(IMG_PATH) -m $(MBR_FILE) -l $(LOADER2_FILE) $(OS_EXE_FILE)


.PHONY: check
check:
	dd if=$(IMG_NAME) of=tmp bs=512 skip=2 count=2878
	# dd if=$(IMG_NAME) of=tmp bs=512 skip=2 count=66553
	fsck.fat -v -n $(FOR_CHECK_FILE)


.PHONY: mount
mount:
	sudo losetup /dev/loop0 $(FOR_CHECK_FILE)
	sudo mount /dev/loop0 /mnt


.PHONY: umount
umount:
	sudo losetup -d /dev/loop0
	sudo umount /mnt


.PHONY: write_fd
write_fd:
	sudo dd if=$(IMG_NAME) of=/dev/sdb
