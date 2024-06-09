#ifndef __XBM_H__
#define __XBM_H__

#include <stdint.h>

typedef struct {
	uint8_t *bits;
	int     w; // Width
	int     h; // Height
	int     fh; // Frame height
} xbm_t;

void xbm2fb(xbm_t *xbm, uint16_t *fb, int col, int row);
void xbm2fb_dirty(xbm_t *xbm, uint16_t *fb, int col, int row);
xbm_t *xbm_croph(xbm_t *xbm, xbm_t *frame, int from_row, int to_row);
xbm_t *extract_frame(xbm_t *xbm, xbm_t *frame, int frame_index);

#endif /* __XBM_H__ */
