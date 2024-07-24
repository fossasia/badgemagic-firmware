#ifndef __BMLIST_H__
#define __BMLIST_H__

#include <stdint.h>
#include <stdlib.h>

typedef struct bm_st {
	uint16_t   *buf;
	uint16_t    width;
	uint8_t     modes;
	int         is_flash;
	int         is_marquee;
	// TODO: feat: Brightness for each bm
	int         brightness;
	// TODO: feat: Timeout for each bm to switch to next bm
	uint32_t    timeout; // zero mean no timeout
	uint32_t    anim_step; // Animation step, zero means restart animation

	struct bm_st *next;
	struct bm_st *prev;
} bm_t;

bm_t *bm_new(uint16_t width);
static inline void bm_free(bm_t *bm) 
{
	free(bm->buf);
	free(bm);
}

bm_t *bmlist_insert(bm_t *at, bm_t *new);
bm_t *bmlist_append(bm_t *new);
bm_t *bmlist_drop(bm_t *bm);

bm_t *bmlist_gonext();
bm_t *bmlist_goprev() ;
bm_t *bmlist_gohead();
bm_t *bmlist_current();

void bmlist_init(uint16_t first_bm_width);

#endif /* __BMLIST_H__ */
