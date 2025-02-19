#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include <stdint.h>

#include "xbm.h"

int ani_xbm_next_frame(xbm_t *xbm, uint16_t *fb, int col, int row);
int ani_xbm_scroll_up(xbm_t *xbm, int vh, uint16_t *fb, int col, int row);
int ani_xbm_scrollup_pad( xbm_t *xbm, int vh, int pt, int pb,
				uint16_t *fb, int col, int row);
int ani_xbm_scrollup_inf(xbm_t *xbm, uint16_t *fb, 
				int vh, int col, int row);

void fb_fill(uint16_t *fb, uint16_t v);
void ani_shift_y(bm_t *bm, uint16_t *fb, int dir, int frame);

void ani_scroll_x(bm_t *bm, uint16_t *fb, int dir);
void ani_scroll_y(bm_t *bm, uint16_t *fb);

int ani_scroll_left(bm_t *bm, uint16_t *fb);
int ani_scroll_right(bm_t *bm, uint16_t *fb);
int ani_scroll_up(bm_t *bm, uint16_t *fb);
int ani_scroll_down(bm_t *bm, uint16_t *fb);
int ani_fixed(bm_t *bm, uint16_t *fb);
int ani_laser(bm_t *bm, uint16_t *fb);
int ani_snowflake(bm_t *bm, uint16_t *fb);
int ani_animation(bm_t *bm, uint16_t *fb);
int ani_picture(bm_t *bm, uint16_t *fb);

void ani_marque(bm_t *bm, uint16_t *fb, int step);
void ani_flash(bm_t *bm, uint16_t *fb, int step);

#endif /* __ANIMATION_H__ */
