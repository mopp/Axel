C_DEPS       := ../fat_manip.c
IMG_SRC      := image_util.c
IMG_PROG     := img_util
IMG_NAME     := disk.img
MBR_FILE     := loader1
LOADER2_FILE := loader1

$(IMG_PROG): $(IMG_SRC) $(C_DEPS)
	clang -I ../include/ $(C_DEPS) $(IMG_SRC) -o $(IMG_PROG)

.PHONY: run
run: $(IMG_PROG)
	./$(IMG_PROG) $(IMG_NAME) $(MBR_FILE) $(LOADER2_FILE)
