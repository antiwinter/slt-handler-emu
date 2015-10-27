
LDFLAGS := -lncurses -lpthread
CFLAGS := -Wno-deprecated-declarations
SRC := main.c ui.c io.c wait-queue.c

all:
	gcc ${SRC} ${CFLGAS} ${LDFLAGS}
