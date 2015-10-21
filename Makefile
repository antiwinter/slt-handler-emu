
all:
	gcc main.c ui.c io.c wait-queue.c -lncurses -Wno-deprecated-declarations -g
