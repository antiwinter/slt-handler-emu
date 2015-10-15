#include <unistd.h>
#include <string.h>
#include <ncurses.h>

#define GH 20
#define GW 80
#define TIPS_H 10
#define ARRAY_SIZE(_x) (sizeof(_x)/(sizeof(_x[0])))

WINDOW *wbin, *wanm, *wtip, *wlog;
int scr_w, scr_h, frame_interval = 70000;

WINDOW *neww(const char *title, int h, int w, int y, int x)
{
	WINDOW *win;
	win = newwin(h, w, y, x);
	wattron(win, A_STANDOUT);
	mvwhline(win, 0, 0, ' ', w);
	if(x + w < scr_w) // do not draw vborder for windows at right side
		mvwvline(win, 0, w - 1, '|', h);
	mvwprintw(win, 0, 1, "- %s -", title);
	wrefresh(win);
	wattroff(win, A_STANDOUT);

	return win;
}

int ui_init() {

	initscr();
	raw();
	noecho();
	curs_set(0);

	getmaxyx(stdscr, scr_h, scr_w);
	printw("scr_w: %d, scr_h: %d\n", scr_w, scr_h);
	if(scr_w < GW || scr_h < GH) {
		mvprintw(scr_h / 2, scr_w / 2, "Window is too small!");
		getch();
		endwin();
		return -1;
	}

	wanm = neww("Handler", (scr_h - TIPS_H + 1) / 2, scr_w / 2, 0, 0);
	wbin = neww("Bin", (scr_h - TIPS_H) / 2, scr_w / 2, (scr_h - TIPS_H + 1) / 2, 0);
	wlog = neww("Communication", scr_h - TIPS_H, (scr_w + 1) / 2, 0, scr_w / 2);
	wtip = neww("Control", TIPS_H, scr_w, scr_h - TIPS_H, 0);

	return 0;
}

int ui_deinit()
{
	endwin();
	return 0;
}

int ui_input() {
	return wgetch(wtip);
}

void ui_print(const char * msg) {
	wprintw(wlog, msg);
}

void ui_bin_add_tag(const char *tag)
{
	static int slot = 1;
	
	if(!tag)
		slot = 0;

	mvwprintw(wbin, slot++, 0, tag);
}

void ui_bin_update(int slot, int num)
{
	mvwprintw(wbin, slot, 10, "%10d", num);
}

void ui_tip_update(int slot, const char *tip)
{
	mvwprintw(wtip, slot, 0, tip);
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
	wprintw(wlog, "animation (%d, %d)\n", y, x);

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

int main()
{
	ui_init();

	while(1) {
		ui_animation(0);
		ui_animation(1);
		ui_animation(2);
		ui_animation(2);
		ui_animation(2);
		ui_animation(2);
		ui_animation(2);
		ui_animation(3);
	}
	ui_input();
	ui_deinit();
}


