#include <unistd.h>
#include <string.h>
#include <ncurses.h>

#define SZ 16
int acount = 0;
struct alias {
	char var[SZ];
	int num_alias
	char str_alias[SZ];
} aliases[SZ];

int lscount = 0, onscount = 0;
struct line {
	int argc;
	char *argv[SZ];
} lines[SZ * 2], ons[SZ];
int bin = 0, power_state = 0;

int parse_line(int argc, const char *argv[])
{
	static int i, start = 0;

	if(*argv[0] == '#') {
		ui_log("comment: %s\n", argv[0]);
		return 0;
	}

	if(start) {
		ui_log("register line: ");
		for(i = 0; i < argc; i++) {
			lines[lscount].argv[lines[lscount].argc] = malloc(SZ);
			strcpy(ls[lscount].argv[ls[lscount].argc], argv[i]);
			ls[lscount].argc++

			ui_log("%s ", argv[i]);
		}
		ui_log("\n");
		return 0;
	}

	if(strcmp(argv[0], "set") == 0) {
		strcpy(aliases[acount].var, argv[1]);
		if(argc > 2)
			aliases[acount].num = atoi(argv[2]);
		if(argc > 3)
			strcpy(aliases[acount].str, argv[3]);
	} else if(strcmp(argv[0], "on") == 0) {
		for(i = 1; i < argc; i++) {
			ons[onscount].argv[ons[onscount].argc] = malloc(SZ);
			strcpy(ons[onscount].argv[ons[onscount].argc], argv[i]);
			ons[onscount].argc++
		}

		onscount++;
	} else if(strcmp(argv[0], "start") == 0) {
		start = 1;
	} else if(strcmp(argv[0], "end") == 0)
		start = 0;
	else
		ui_log("unrecgnized parser command: %s\n", argv[0]);

	return -1;
}

int exec_line(int argc, const char *argv[])
{
	if(strcmp(argv[0], "load") == 0) {
		ui_animation(1);
	} else if(strcmp(argv[0], "unload") == 0) {
		ui_animation(3);
	} else if(strcmp(argv[0], "poweron") == 0) {
		power_state = 1;
	} else if(strcmp(argv[0], "poweroff") == 0) {
		power_state = 1;
	} else if(strcmp(argv[0], "putbin") == 0) {
		ui_bin_update(bin, num++); // fixme
	} else if(strcmp(argv[0], "send") == 0) {
		for(i = 1; i < argc; i++) {
			char *s = argv[i];
			if(strcmp(s, "power_state") == 0)
				s = power_state? "1": "NULL";
			io_send(get_alias_str(s));
		}
	} else if(strcmp(argv[0], "wait") == 0) {
		struct timeval a, b, d;
		int t = get_alias_num("timeout");
		for(i = 1; i < argc; i++)
			wait_queue_add(argv[i]);

		gettimeofday(&a, NULL);
		for(;!wait_queue_empty(); ui_animation(2)) {
			gettimeofday(&b, NULL);
			timersub(&b, &a, &d);
			if(d.tv_sec > t) {
				ui_log("timeout for waiting %s\n", wait_queue_head());
				break;
			}
		}
	}

	return 0;
}

void receiver(void *args)
{
	char word[SZ];
	struct line *on;
	while(1) {
		if(scanf("%s", word) <= 0) {
			usleep(300000);
			continue;
		}

		// trigger on event
		on = get_on(word);
		if(on) exec_line(on->argc, on->argv);

		// queue management
		if(strcmp(wait_queue_head(), "bin") == 0) {
			bin = atoi(word);
			wait_queue_pop();
		} else if(strcmp(word, wait_queue_head()) == 0
			|| strcmp(wait_queue_head(), "X") == 0) {
			wait_queue_pop();
		}
	}
}

int main()
{
	ui_init();

		ui_animation(0);
		ui_animation(1);
		ui_animation(2);
		ui_animation(2);
		ui_animation(2);
		ui_animation(2);
		ui_animation(2);
		ui_animation(3);
	ui_input();
	ui_deinit();
}

