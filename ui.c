#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>

#define GH 24
#define GW 140 
#define TIPS_H 10
#define UIBSZ 0x100000
#define ARRAY_SIZE(_x) (sizeof(_x)/(sizeof(_x[0])))

WINDOW *wbin, *wanm, *wtip, *wmsg, *wmsgt, *wdbg;
int scr_w, scr_h, frame_interval = 30000;
FILE *flog;

struct msgbuffer {
	char *buf;
	int lines, line, tail, cur;
	int window;
	pthread_mutex_t lock;
} msg;

WINDOW *neww(const char *title, int h, int w, int y, int x)
{
	WINDOW *win;
	win = newwin(h, w, y, x);
	wattron(win, A_STANDOUT);
	if(title) {
		mvwhline(win, 0, 0, ' ', w);
		mvwprintw(win, 0, 1, "- %s -", title);
	}
	if(x + w < scr_w) // do not draw vborder for windows at right side
		mvwvline(win, 0, w - 1, '|', h);

	wrefresh(win);
	wattroff(win, A_STANDOUT);

	return win;
}

void ui_setspeed(int var)
{
	frame_interval = var;
}

int ui_init() {

	initscr();
	raw();
	noecho();
	curs_set(0);
	mousemask(ALL_MOUSE_EVENTS, NULL);

	getmaxyx(stdscr, scr_h, scr_w);
	if(scr_w < GW || scr_h < GH) {
		printw("scr_w: %d, scr_h: %d\n", scr_w, scr_h);
		printw("Window is too small!");
		getch();
		endwin();
		return -1;
	}

	wanm = neww("Handler", (scr_h - TIPS_H + 1) / 2, scr_w / 2, 0, 0);
	wbin = neww("Bin", (scr_h - TIPS_H) / 2, scr_w / 2, (scr_h - TIPS_H + 1) / 2, 0);
	wmsgt = neww("Communication", 1, (scr_w + 1) / 2, 0, scr_w / 2);
	// just overwrite that to create a sub-window for scrolling
	wmsg = neww(0, scr_h - TIPS_H - 1, (scr_w + 1) / 2, 1, scr_w / 2);
	wtip = neww("Control", TIPS_H, scr_w, scr_h - TIPS_H, 0);
//	wdbg = neww("Debug", scr_h - TIPS_H, (scr_w + 1) / 4, 0, scr_w * 3 / 4);
//	scrollok(wdbg, 1);

	if(!msg.buf) {
		// we alloc 4K more to avoid overflow
		msg.buf = malloc(UIBSZ + 0x8000);
		memset(msg.buf, 0, UIBSZ + 0x8000);
	}
	msg.lines = msg.line = msg.tail = msg.cur = 0;
	msg.window = scr_h - TIPS_H - 1;
	pthread_mutex_init(&msg.lock, NULL);

	flog = fopen("ui.log", "w");
	return 0;
}

int ui_deinit()
{
	endwin();
	if(msg.buf) {
		free(msg.buf);
		msg.buf = NULL;
	}
	return 0;
}

int ui_input() {
	int c;
	MEVENT e;

	c = wgetch(wtip);
	if(c != KEY_MOUSE)
		return c;
	getmouse(&e);
	return e.bstate;
}

void __ui_seek(int n)
{
	int i, l = abs(n), sk = 0;

	pthread_mutex_lock(&msg.lock);
	for(i = msg.cur; l && msg.buf[i]
		 && i != msg.tail; ) {

		if(n > 0 && msg.lines - msg.line <= msg.window)
			break;

		i += UIBSZ - (n < 0);
		i %= UIBSZ;
		if(msg.buf[i] == '\n' || !msg.buf[i]) {
			if(n < 0 && !sk) {
				sk = 1;
				continue;
			}
			msg.line += (n > 0) - (n < 0);
			msg.cur = (i + 1 + UIBSZ) % UIBSZ;
			l--;
		}
		i += UIBSZ + (n > 0);
		i %= UIBSZ;
	}
	pthread_mutex_unlock(&msg.lock);
}

int msgstop = 0;
void ui_pausemsg(int pause)
{
	msgstop = pause;
}

void __ui_update(void)
{
	int y, x, o;

	getmaxyx(wmsgt, y, x);
	wattron(wmsgt, A_STANDOUT);
	o = msg.line * 10 / msg.lines + x - 12;
	mvwprintw(wmsgt, 0, x - 13, "[----------]");
	mvwprintw(wmsgt, 0, o, "o");
	wrefresh(wmsgt);
	wattroff(wmsgt, A_STANDOUT);

	wclear(wmsg);
	mvwprintw(wmsg, 0, 0, msg.buf + msg.cur);
	wrefresh(wmsg);
}

void ui_scroll(int n)
{
	__ui_seek(n);
	__ui_update();
}

void ui_print(const char * fmt, ...) {
	va_list args;
	int n, i;

	pthread_mutex_lock(&msg.lock);
	va_start(args, fmt);
	n = vsprintf(msg.buf + msg.tail, fmt, args);
	va_end(args);
	va_start(args, fmt);
	vfprintf(flog, fmt, args);
	va_end(args);
	fflush(flog);

	for(i = msg.tail; msg.buf[i]; i++)
		if(msg.buf[i] == '\n') msg.lines++;
	msg.tail += n;
	if(msg.tail >= UIBSZ) {
		strcpy(msg.buf, msg.buf + UIBSZ);
		msg.tail %= UIBSZ;
	}
	pthread_mutex_unlock(&msg.lock);

	if(!msgstop) {
		while(msg.lines - msg.line > msg.window)
			__ui_seek(1);
			__ui_update();
	}
}

void ui_bin_add_tag(const char *tag)
{
	static int slot = 1;
	
	if(!tag)
		slot = 0;

	mvwprintw(wbin, slot++, 0, tag);
	ui_print("adding tag: %s\n", tag);
	wrefresh(wbin);
}

void ui_bin_update(int slot, int num)
{
	int y, x;
	getmaxyx(wbin, y, x);
	mvwprintw(wbin, slot, x - 13, "%10d", num);
	wrefresh(wbin);
}

void ui_tip_update(int slot, const char *tip)
{
	mvwprintw(wtip, slot, 0, tip);
	wrefresh(wtip);
}

#define frame_clear(_w, _y, _x, _f) do { \
  static int _i, _j;  \
  for(_i = 0; _i < ARRAY_SIZE(_f); _i++)  \
    for(_j = 0; _j < strlen(_f[_i]); _j++)  \
      mvwaddch(_w, _y + _i, _x + _j, ' ');  \
  wrefresh(_w);  \
} while(0)

#define frame_set(_w, _y, _x, _f) do { \
  static int _i, _j;  \
  for(_i = 0; _i < ARRAY_SIZE(_f); _i++)  \
    mvwprintw(_w, _y + _i, _x, _f[_i]);  \
  wrefresh(_w);  \
} while(0)

#define animate(_f, y0, x0, y1, x1) do {  \
  int _y0 = y0,  _x0 = x0, _y1 = y1, _x1 = x1;  \
  while(1) {  \
	  frame_set(wanm, _y0, _x0, _f);  \
	  usleep(frame_interval);  \
	  frame_clear(wanm, _y0, _x0, _f);  \
	  if(_x0 == _x1 && _y0 == _y1) break;  \
      _y0 += (_y1 > _y0) - (_y1 < _y0);  \
      _x0 += (_x1 > _x0) - (_x1 < _x0);  \
  }  \
} while(0)


void ui_animation(int step)
{
	char *hd0[] = {
		"|  |",
		"|  |",
		"|__|",
		"*--*",
	}, *hd1[] = {
		"|  |",
		"|  |",
		"|__|",
		"*--*",
		"&&&&",
	}, *socket[] = {
		"|  &&&&  |        ---------.    .---------        |        |",
		"+--------+                  @@@@                  +--------+"
	}; // offset: head down y+4, fetch x+3, soc x+28, put x+53, soc y+8
	   // soc progress: x+20, x+40

	int y, x, i, c;

	// calc center
	getmaxyx(wanm, y, x);

	y -= 12;
	y /= 2;

	x -= 68;
	x /= 2;

	switch (step) {
		case 1:
		animate(hd0, y, x + 3, y + 4, x + 3); // head down fetch
		animate(hd1, y + 4, x + 3, y, x + 3); // head up
		animate(hd1, y, x + 4, y, x + 28); // move to socket
		animate(hd1, y, x + 28, y + 4, x + 28); // head down
		frame_set(wanm, y + 4, x + 28, hd1);
		break;

		case 2:
		for(i = x + 18; i <= x + 41; i++) {
			c = mvwinch(wanm, y + 8, i);
			mvwprintw(wanm, y + 8, i, "~");
			wrefresh(wanm);
			usleep(frame_interval);
			mvwprintw(wanm, y + 8, i, "%c", c);
			wrefresh(wanm);
		}
		break;

		case 3:
		animate(hd1, y + 4, x + 28, y, x + 28); // head up
		animate(hd1, y, x + 28, y, x + 53); // move to bin
		animate(hd1, y, x + 53, y + 4, x + 53); // head down
		frame_set(wanm, y + 4, x + 53, hd1);
		animate(hd0, y + 4, x + 53, y, x + 53); // head up

		default: // draw init state
		frame_set(wanm, y + 8, x, socket);
		frame_set(wanm, y, x + 3, hd0);
	}
}


