#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static int iofd;

int io_recv(char *buf, int len)
{
	return read(iofd, buf, 64);
}

int io_send(const char *word)
{
	printf("send << %s\n", word);
	return write(iofd, word, strlen(word));
}

#define TTYDEV "/dev/pts/3"
int io_init(void)
{
	struct termios io;

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

//	unlockpt(iofd);
//	grantpt(iofd);

	if(iofd >= 0) {
		tcflush(iofd, TCIFLUSH);
		if(tcsetattr(iofd, TCSANOW, &io) != 0)
			return -2;
	}

	return iofd;
}

int io_deinit(void)
{
	if(iofd >= 0) close(iofd);
	return 0;
}

int main()
{
	char buf[64];
	int ret, i;

	ret = io_init();
	ptsname_r(ret, buf, 64);
	printf("pts name is: %s (%d)\n", buf, ret);

	io_send("\005\012");
	while((ret = io_recv(buf, 64)) <= 0);
	printf("recv %d >> ", ret);
	for(i = 0; i < ret; i++)
		printf("%d ", buf[i]);
	putchar('\n');
	io_send("\002CE\003");

	while((ret = io_recv(buf, 64)) <= 0);
	printf("recv %d >> ", ret);
	for(i = 0; i < ret; i++)
		printf("%d ", buf[i]);
	putchar('\n');
	io_send("\006");

	while(1) {
		if((ret = io_recv(buf, 64)) > 0) {
			printf("recv %d >> ", ret);
			for(i = 0; i < ret; i++)
				printf("%d ", buf[i]);
			putchar('\n');
		}

		usleep(20000);
	}

	io_deinit();
	return 0;
}

