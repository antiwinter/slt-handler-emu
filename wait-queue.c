#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// biodirection list
struct word {
	char str[16];
	struct word *next, *prev;  // pointers must be named as next & prev
} head;

// @_p, @_q: iterators
// @_h: head node
// @_s: start node pointer
#define ls(_p, _h, _s) for(_p = _s; _p != &_h; _p = _p->next)
#define ls_init(_h) do {_h.next = _h.prev =  &_h;} while(0)
#define ls_pop(_h) do { _h.next->next->prev = &_h; _h.next = _h.next->next;} while(0)
#define ls_add_before(_p, _s) do {  \
	_p->next = _s; _p->prev = _s->prev;  \
	_s->prev = _p; _p->prev->next = _p;} while(0)
#define ls_destory(_p, _q, _h) for(_p = _h.next; _p != &_h; _q = _p->next, free(_p), _p = _q)
#define ls_empty(_h) (_h.next == &_h)

int wait_queue_init()
{
	ls_init(head);
}

int wait_queue_deinit()
{
	struct word *p, *q;
	ls_destory(p, q, head);
	ls_init(head);
	return 0;
}

void wait_queue_pop()
{
	struct word *w = head.next;
	ls_pop(head);
	free(w);
}

const char * wait_queue_head()
{
	return head.next->str;
}

int wait_queue_empty()
{
	return ls_empty(head);
}

void wait_queue_add_tail(const char *str)
{
	struct word *w = malloc(sizeof(struct word));
	strcpy(w->str, str);
	ls_add_before(w, (&head));
}

void wait_queue_add_head(const char *str)
{
	struct word *w = malloc(sizeof(struct word));
	strcpy(w->str, str);
	ls_add_before(w, head.next);
}

