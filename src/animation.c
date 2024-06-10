#include "animation.h"
#include <stdlib.h>

int frame_next(xbm_t *xbm, uint16_t *fb, int col, int row)
{
	static int i;

	xbm_t frame;
	if (extract_frame(xbm, &frame, i) == NULL) {
		i = 0;
		return i;
	}
	xbm2fb(&frame, fb, col, row);

	return ++i;
}

int scrollup_next(xbm_t *xbm, int vh, 
				uint16_t *fb, int col, int row)
{
	static int i;

	xbm_t frame;
	if (xbm_croph(xbm, &frame, i, i + vh) == NULL) {
		i = 0;
		return i;
	}
	xbm2fb(&frame, fb, col, row);

	return ++i;
}

static int scroll_pt_next(xbm_t *xbm, int vh, int pt, 
				uint16_t *fb, int col, int row)
{
	static int i;
	xbm_t frame;
	if (i >= pt) {
		i = 0;
		return i;
	}
	if (xbm_croph(xbm, &frame, 0, i + (vh -pt)) == NULL) {
		i = 0;
		return i;
	}
	xbm2fb(&frame, fb, col, row +pt -i);

	return ++i;
}

static int scroll_pb_next( xbm_t *xbm, int vh, int pb, 
				uint16_t *fb, int col, int row)
{
	static int i;
	xbm_t frame;
	if (i >= pb) {
		i = 0;
		return i;
	}
	if (xbm_croph(xbm, &frame, xbm->h - (vh -1 -i), xbm->h) == NULL) {
		i = 0;
		return i;
	}
	xbm2fb(&frame, fb, col, row);

	return ++i;
}

int scroll_pad_next( xbm_t *xbm, int vh, int pt, int pb,
				uint16_t *fb, int col, int row)
{
	static int i;
	if (i == 0)
		i = 1;
	if (i == 1 && scroll_pt_next(xbm, vh, pt, fb, col, row) == 0) {
		i = 2;
	}
	if (i == 2 && scrollup_next(xbm, vh, fb, col, row) == 0) {
		i = 3;
	}
	if (i == 3 && scroll_pb_next(xbm, vh, pb, fb, col, row) == 0) {
		i = 0;
	}
	return i;
}

int scroll_circular_next(xbm_t *xbm, uint16_t *fb, 
				int vh, int col, int row)
{
	static int i, r;

	xbm_t frame, frame_circle;
	if (xbm_croph(xbm, &frame, i, i +vh) == NULL) {

		xbm_croph(xbm, &frame, i +r, i -1 +vh);
		xbm_croph(xbm, &frame_circle, 0, r);
		
		xbm2fb(&frame, fb, col, row);
		xbm2fb_dirty(&frame_circle, fb, col, row +vh -r);

		r++;
		if (r >= vh) {
			i = 0;
			r = 0;
		}
		return i;
	}
	xbm2fb(&frame, fb, col, row);

	return ++i;
}