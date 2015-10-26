#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define IOBSZ 0x100000
static char *iobuffer = NULL;
static int iofd, head, tail;

int io_get_word(char *word)
{
	char buf[64], *delim = " ,\n\r", *p;
	int ret, i, j;

	// read new input
	while(1) {
//	while(ret = read(iofd, buf, 64) > 0) {
	ret = read(iofd, buf, 64);
	if(ret <= 0)
		break;
//	else
//	ui_print("io_in %d::[%c] [%c] [%c] [%c] [%c] [%c] [%c] [%c]\n", ret,
//		  buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
//	ui_print("head=%d, tail=%d\n", head, tail);
		if(tail + ret > IOBSZ) {
			memcpy(&iobuffer[tail], buf, IOBSZ - tail);
			memcpy(&iobuffer[0], buf + IOBSZ - tail, ret + tail - IOBSZ);
		} else
			memcpy(&iobuffer[tail], buf, ret);

		tail = (tail + ret) % IOBSZ;
//	ui_print("head=%d, tail=%d\n", head, tail);
	}

	// provide word output
	// ret is used as word begin flag
	ret = 0;
	for(i = head, j = 0; i != tail; i = (i + 1) % IOBSZ) {
//		ui_print("\\\\ %d::%d ", i, iobuffer[i]);
		for(p = delim; *p; p++)
			if(iobuffer[i] == *p) {
//				ui_print("skip\n");
				break;
			}

		if(!*p) {
			ret = 1;
			buf[j++] = iobuffer[i];
//			ui_print("log->%d\n", j);
		} else if(ret == 1) {
			buf[j] = 0;
			strcpy(word, buf);
			head = i;
//			ui_print("return \n");
			return j;
		}
	}

	return 0;
}

int io_send(const char *word)
{
	return write(iofd, word, strlen(word));
}

#define TTYDEV "/dev/ptmx"
int io_init(void)
{
	struct termios io;

	if(!iobuffer) {
		iobuffer = malloc(IOBSZ);
		head = tail = 0;
	}

	// init serial
	bzero(&io, sizeof(struct termios));
	io.c_cflag |= CLOCAL | CREAD;
	io.c_cflag &= ~CSIZE;
	io.c_cflag |= CS8;
	io.c_cflag &= ~PARENB;
	io.c_cflag &= ~CSTOPB;
	cfsetispeed(&io, B115200);
	cfsetospeed(&io, B115200);

	iofd = open(TTYDEV, O_RDWR | O_NOCTTY | O_NDELAY);

	unlockpt(iofd);
	grantpt(iofd);

	if(iofd >= 0) {
		tcflush(iofd, TCIFLUSH);
		if(tcsetattr(iofd, TCSANOW, &io) != 0)
			return -2;
	}

	return iofd;
}

int io_deinit(void)
{
	if(iobuffer) free(iobuffer);
	if(iofd >= 0) close(iofd);
	return 0;
}

