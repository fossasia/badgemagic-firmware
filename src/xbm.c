#include "xbm.h"
#include <stdlib.h>
#include <memory.h>

#define ALIGNBIT(x) (((x % 8)>0)*8 + (x / 8) * 8)

/**
 * Draw bitmap file to fb at (col, row)
 * if bitmap file larger than fb, it will be stripped out
 */
void xbm2fb(xbm_t *xbm, uint16_t *fb, int col, int row)
{
	int W = ALIGNBIT(xbm->w);
	uint16_t *tmpfb = malloc(W * sizeof(uint16_t));
	memset(tmpfb, 0, W * sizeof(uint16_t));

	if ((xbm->h + row) >= 0 && col >= 0) {

		for (int h = 0; h < xbm->h; h++) {
			for (int w = 0; w < W; w++) {
				tmpfb[w] |= ((xbm->bits[h*(W/8) + w/8] >> (w % 8)) & 0x01) 
					<< (h+row);
			}
		}
	}
	for (int i=0; i < xbm->w; i++) {
		fb[col+i] = tmpfb[i];
	}
}

void xbm2fb_dirty(xbm_t *xbm, uint16_t *fb, int col, int row)
{
	// Byte align for xbm bytes
	int W = ALIGNBIT(xbm->w);

	for (int h = 0; h < xbm->h; h++) {
		for (int w = 0; w < W; w++) {
			fb[w] |= ((xbm->bits[h*(W/8) + w/8] >> (w % 8)) & 0x01) << (h+row);
		}
	}
}

/**
 * Crop image horizontaly
 * TODO: vertical crop
 * 
 */
xbm_t *xbm_croph(xbm_t *xbm, xbm_t *frame, int from_row, int to_row)
{
	int rb = xbm->w / 8 + ((xbm->w % 8)>0); // Number of bytes in a row
	if (to_row * rb > rb * xbm->h)
		return NULL;

	frame->bits = &xbm->bits[rb * from_row];
	frame->w = xbm->w;
	frame->h = to_row - from_row;
	frame->fh = xbm->fh;

	return frame;
}

xbm_t *extract_frame(xbm_t *xbm, xbm_t *frame, int frame_index)
{
	int from = frame_index * xbm->fh;
	int to = from + xbm->fh;
	return xbm_croph(xbm, frame, from, to);
}