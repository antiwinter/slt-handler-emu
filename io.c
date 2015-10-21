
#define IOBSZ 0x100000
static char *iobuffer = NULL;
static int iofd, head, tail;

int io_get_word(const char *word)
{
	char buf[64], *delim = " ,", *p;
	int ret, i, j;

	// read new input
	while(ret = read(iofd, buf, 64) > 0) {
		if(tail + ret > IOBSZ) {
			memcpy(&iobuffer[tail], buf, IOBSZ - tail);
			memcpy(&iobuffer[0], buf + IOBSZ - tail, ret + tail - IOBSZ);
		} else
			memcpy(&iobuffer[tail], buf, ret);

		tail = (tail + ret) % IOBSZ;
	}

	// provide word output
	// ret is used as word begin flag
	ret = 0;
	for(i = head, j = 0; i != tail; i = (i + 1) % IOBSZ) {
		for(p = delim; *p; p++)
			if(iobuffer[i] == *p)
				break;

		if(!*p) {
			ret = 1;
			buf[j++] = iobuffer[i];
		} else if(ret == 1) {
			buf[j] = 0;
			head += j;
			strcpy(word, buf);
			return j;
		}
	}

	return 0;
}

int io_send(const char *word)
{
	return write(iofd, word, strlen(word));
}

#define TTYDEV "/dev/ttyUSB0"
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

	iofd = open(TTYDEV, O_RDWR | O_NOCTTY | O_NOBLOCK);
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

