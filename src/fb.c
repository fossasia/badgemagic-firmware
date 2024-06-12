#include "fb.h"
#include <memory.h>

volatile static fb_t *current, *head, *tail;

static void fb_add(fb_t *new, fb_t *prev, fb_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

fb_t *fblist_insert(fb_t *at, fb_t *new) 
{
	fb_add(new, at, at->next);
	return new;
}

fb_t *fblist_append(fb_t *new) 
{
	fblist_insert(new, tail);
	tail = new;
	return new;
}

fb_t *fblist_gonext() 
{
	current = current->next;
	current->scroll = 0;
	return current;
}

fb_t *fblist_goprev() 
{
	current = current->prev;
	current->scroll = 0;
	return current;
}

fb_t *fblist_gohead()
{
	current = head;
	current->scroll = 0;
	return current;
}

fb_t *fblist_currentfb()
{
	return current;
}

static void list_del(fb_t *prev, fb_t *next)
{
	prev->next = next;
	next->prev = prev;
}

fb_t *fblist_drop(fb_t *fb)
{
	list_del(fb->prev, fb->next);
	return fb->next;
}

fb_t *fb_new(uint16_t width)
{
	fb_t *fb = malloc(sizeof(fb_t));
	memset(fb, 0, sizeof(fb_t));

	fb->width = width;
	fb->buf = malloc(width * sizeof(uint16_t));
	memset(fb->buf, 0, width * sizeof(uint16_t));

	fb->modes = FIXED;

	fb->next = fb;
	fb->prev = fb;

	return fb;
}

void fblist_init(uint16_t first_fb_width)
{
	current = fb_new(first_fb_width);
	head = current;
	tail = current;
}
