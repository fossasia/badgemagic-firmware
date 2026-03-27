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
	bm_t *next = bm->next;
	bm_t *prev = bm->prev;

	list_del(prev, next);

	if (bm == head)
		head = (next == bm) ? NULL : next;
	if (bm == tail)
		tail = (prev == bm) ? NULL : prev;
	if (bm == current)
		current = (next == bm) ? NULL : next;

	free(bm);
	return (next == bm) ? NULL : next;
}

bm_t *bm_new(uint16_t width)
{
	if (width == 0)
		return NULL;

	bm_t *bm = calloc(1, sizeof(*bm));
	if (!bm)
		return NULL;

	bm->width = width;
	bm->buf = calloc(width, sizeof(*bm->buf));
	if (!bm->buf) {
		free(bm);
		return NULL;
	}

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
