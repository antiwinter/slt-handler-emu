
extern int wait_queue_init();
extern int wait_queue_deinit();
extern const char * wait_queue_pop();
extern const char * wait_queue_head();
extern int wait_queue_empty();
extern void wait_queue_add_tail(const char *str);
extern void wait_queue_add_head(const char *str);
