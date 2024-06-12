#ifndef __FB_H__
#define __FB_H__

#include <stdint.h>
#include <stdlib.h>

enum ANIMATION_MODES {
	LEFT = 0,
	RIGHT,
	UP,
	DOWN,
	FIXED,
	SNOWFLAKE,
	PICTURE,
	ANIMATION,
	LASER,
};

typedef struct fb_st {
	uint16_t   *buf;
	uint16_t    width;
	uint8_t     modes;
	int         is_flash;
	int         is_marquee;
	// TODO: feat: Brightness for each fb
	int         brightness;
	// TODO: feat: Timeout for each fb to switch to next fb
	uint32_t    timeout; // zero mean no timeout
	uint16_t    scroll;

	struct fb_st *next;
	struct fb_st *prev;
} fb_t;

fb_t *fb_new(uint16_t width);
static inline void fb_free(fb_t *fb) 
{
	free((fb)->buf);
	free((fb));
}

fb_t *fblist_insert(fb_t *at, fb_t *new);
fb_t *fblist_append(fb_t *new);
fb_t *fblist_drop(fb_t *fb);

fb_t *fblist_gonext();
fb_t *fblist_goprev() ;
fb_t *fblist_gohead();
fb_t *fblist_currentfb();

void fblist_init(uint16_t first_fb_width);

#endif /* __FB_H__ */
