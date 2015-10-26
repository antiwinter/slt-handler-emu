
LDFLAGS := -lncurses -lpthread
CFLAGS := -Wno-deprecated-declarations -g
SRC := main.c ui.c io.c wait-queue.c

all:
	gcc ${SRC} ${CFLGAS} ${LDFLAGS}
