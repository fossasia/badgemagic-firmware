#ifndef __ANIMATION_H__
#define __ANIMATION_H__

#include <stdint.h>

#include "xbm.h"

int frame_next(xbm_t *xbm, uint16_t *fb, int col, int row);
int scrollup_next(xbm_t *xbm, int vh, uint16_t *fb, int col, int row);
int scroll_pad_next( xbm_t *xbm, int vh, int pt, int pb,
				uint16_t *fb, int col, int row);
int scroll_circular_next(xbm_t *xbm, uint16_t *fb, 
				int vh, int col, int row);

#endif /* __ANIMATION_H__ */
