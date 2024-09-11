#include "bmlist.h"
#include <memory.h>

volatile static bm_t *current, *head, *tail;

static void bm_add(bm_t *new, bm_t *prev, bm_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

bm_t *bmlist_insert(bm_t *at, bm_t *new) 
{
	bm_add(new, at, at->next);
	return new;
}

bm_t *bmlist_append(bm_t *new) 
{
	bmlist_insert(tail, new);
	tail = new;
	return new;
}

bm_t *bmlist_gonext() 
{
	current = current->next;
	current->anim_step = 0;
	return current;
}

bm_t *bmlist_goprev() 
{
	current = current->prev;
	current->anim_step = 0;
	return current;
}

bm_t *bmlist_gohead()
{
	current = head;
	current->anim_step = 0;
	return current;
}

bm_t *bmlist_head()
{
	return head;
}

bm_t *bmlist_current()
{
	return current;
}

static void list_del(bm_t *prev, bm_t *next)
{
	prev->next = next;
	next->prev = prev;
}

bm_t *bmlist_drop(bm_t *bm)
{
	list_del(bm->prev, bm->next);
	if (bm == head)
		head = bm->next;
	if (bm == tail)
		tail = bm->prev;
	return bm->next;
}

bm_t *bm_new(uint16_t width)
{
	bm_t *bm = malloc(sizeof(bm_t));
	memset(bm, 0, sizeof(bm_t));

	bm->width = width;
	bm->buf = malloc(width * sizeof(uint16_t));
	memset(bm->buf, 0, width * sizeof(uint16_t));

	bm->next = bm;
	bm->prev = bm;

	return bm;
}

void bmlist_init(uint16_t first_bm_width)
{
	current = bm_new(first_bm_width);
	head = current;
	tail = current;
}
