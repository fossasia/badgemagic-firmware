#include <stdlib.h>

#include "xbm.h"
#include "leddrv.h"
#include "bmlist.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define ALIGN(x, range) (((x % (range))>0)*(range) + (x / (range)) * (range))

// Shift left on positive and right on negative n
#define SIGNED_SHIFT(reg, n) ((n) >= 0) ? (reg) >> (n) : (reg) << abs(n)

int mod(int a, int b)
{
	int r = a % b;
	return r < 0 ? r + b : r;
}

void fb_fill(uint16_t *fb, uint16_t v)
{
	for (int i = 0; i < LED_COLS; i++) {
		fb[i] = v;
	}
}

int ani_xbm_next_frame(xbm_t *xbm, uint16_t *fb, int col, int row)
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

int ani_xbm_scroll_up(xbm_t *xbm, int vh, 
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

int ani_xbm_scrollup_pad( xbm_t *xbm, int vh, int pt, int pb,
				uint16_t *fb, int col, int row)
{
	static int i;

	if (i == 0)
		i = 1;
	if (i == 1 && scroll_pt_next(xbm, vh, pt, fb, col, row) == 0) {
		i = 2;
	}
	if (i == 2 && ani_xbm_scroll_up(xbm, vh, fb, col, row) == 0) {
		i = 3;
	}
	if (i == 3 && scroll_pb_next(xbm, vh, pb, fb, col, row) == 0) {
		i = 0;
	}
	return i;
}
/**
 * Infinite scroll up
 */
int ani_xbm_scrollup_inf(xbm_t *xbm, uint16_t *fb, 
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

void ani_scroll_x(bm_t *bm, uint16_t *fb, int dir)
{
	int x = mod(bm->anim_step, bm->width + LED_COLS) - LED_COLS;
	bm->anim_step += (dir) ? -1 : 1;

	for (int i = 0; i < LED_COLS; i++) {
		if (i + x >= bm->width) {
			fb[i] = 0;
			continue;
		}
		fb[i] = (i + x) >= 0 ? bm->buf[i + x] : 0;
	}
}

void ani_scroll_left(bm_t *bm, uint16_t *fb)
{
	ani_scroll_x(bm, fb, 0);
}

void ani_scroll_right(bm_t *bm, uint16_t *fb)
{
	ani_scroll_x(bm, fb, 1);
}

void ani_shift_y(bm_t *bm, uint16_t *fb, int y, int frame)
{
	frame *= LED_COLS;

	int size = MIN(LED_COLS, bm->width);
	int i = 0;
	for (; i < size; i++) {
		if ((frame + i) >= bm->width)
			break;
		fb[i] = SIGNED_SHIFT(bm->buf[frame + i], y);
	}
	for (; i < LED_COLS; i++)
		fb[i] = 0;
}

void ani_scroll_y(bm_t *bm, uint16_t *fb)
{
	int frame_steps = LED_ROWS * 3; // in-still-out
	int frames = ALIGN(bm->width, LED_COLS) / LED_COLS;
	int frame = mod(bm->anim_step, frame_steps*frames)/frame_steps;

	int y = mod(bm->anim_step, frame_steps);

	if (y < LED_ROWS) { // Scrolling up (in)
		ani_shift_y(bm, fb, y - LED_ROWS, frame);
	} else if (y < LED_ROWS * 2) { // Stay still
		ani_shift_y(bm, fb, 0, frame);
	} else { // Scrolling up (out)
		ani_shift_y(bm, fb, y - LED_ROWS * 2, frame);
	}
}

void ani_scroll_up(bm_t *bm, uint16_t *fb)
{
	ani_scroll_y(bm, fb);
	bm->anim_step++;
}

void ani_scroll_down(bm_t *bm, uint16_t *fb)
{
	ani_scroll_y(bm, fb);
	bm->anim_step--;
}

static void laser_in(bm_t *bm, uint16_t *fb, int step, int frame)
{
	int c = mod(step, LED_COLS);
	frame *= LED_COLS;

	int i = 0;
	for (; i < c; i++) {
		fb[i] = (frame + i < bm->width) ? bm->buf[frame + i] : 0;
	}
	for (; i < MIN(LED_COLS, bm->width - frame); i++) {
		fb[i] = bm->buf[frame + c];
	}
	for (; i < LED_COLS; i++) {
		fb[i] = (frame + c < bm->width) ? bm->buf[frame + c] : 0;
	}
}

static void laser_out(bm_t *bm, uint16_t *fb, int step, int frame)
{
	int c = mod(step, LED_COLS);
	frame *= LED_COLS;

	int i = 0;
	for (; i < c; i++) {
		fb[i] = (frame + c < bm->width) ? bm->buf[frame + c] : 0;
	}
	for (; i < MIN(LED_COLS, bm->width - frame); i++) {
		fb[i] = bm->buf[frame + i];
	}
	for (; i < LED_COLS; i++) {
		fb[i] = 0;
	}
}

static void still(bm_t *bm, uint16_t *fb, int frame)
{
	int i = frame * LED_COLS;
	int j = 0;
	for (; j < LED_COLS; j++) {
		if (i >= bm->width)
			break;
		fb[j] = bm->buf[i];
		i++;
	}
	for (; j< LED_COLS; j++) {
		fb[j] = 0;
	}
}

void ani_laser(bm_t *bm, uint16_t *fb)
{
	int frame_steps = LED_COLS * 3; // in-still-out
	int frames = ALIGN(bm->width, LED_COLS) / LED_COLS;
	int frame = mod(bm->anim_step, frame_steps*frames)/frame_steps;

	int c = mod(bm->anim_step, frame_steps);
	bm->anim_step++;

	if (c < LED_COLS)
		laser_in(bm, fb, c - LED_COLS, frame);
	else if (c < LED_COLS * 2)
		still(bm, fb, frame);
	else
		laser_out(bm, fb, c - LED_COLS * 2, frame);
}

static uint32_t b16dialate(uint16_t w, int from, int to)
{
	uint32_t ret = 0;
	int i = 0, j = from;
	for (; i < from; i++) {
		ret |= (w & (1 << i));
	}
	for (; i < to; i++, j += 2) {
		ret |= ((w & (1 << i)) >> i) << j;
	}
	for (; i < 16; i++, j++) {
		ret |= ((w & (1 << i)) >> i) << j;
	}

	return ret;
}

static void snowflake_in(bm_t *bm, uint16_t *fb, int step, int frame)
{
	int y = mod(step, LED_ROWS*2) - LED_ROWS;
	frame *= LED_COLS;

	int size = MIN(LED_COLS, bm->width - frame);
	int i = 0;
	for (; i < size; i++) {
		if (y < 0) {
			fb[i] = SIGNED_SHIFT(b16dialate(bm->buf[frame + i], 0, LED_ROWS),
				LED_ROWS - y);
		} else
			fb[i] = SIGNED_SHIFT(b16dialate(bm->buf[frame + i], 0, LED_ROWS - y),
				(LED_ROWS - y));
	}
	for (; i < LED_COLS; i++) {
		fb[i] = 0;
	}
}

static void snowflake_out(bm_t *bm, uint16_t *fb, int step, int frame)
{
	int y = mod(step, LED_ROWS*2) - LED_ROWS;
	frame *= LED_COLS;

	int size = MIN(LED_COLS, bm->width - frame);
	int i = 0;
	for (; i < size; i++) {
		if (y <= 0) {
			fb[i] = SIGNED_SHIFT(b16dialate(bm->buf[frame + i], 0, LED_ROWS),
				y);
		} else {
			fb[i] = b16dialate(bm->buf[frame + i], y, LED_ROWS);
		}
	}
	for (; i < LED_COLS; i++) {
		fb[i] = 0;
	}
}

void ani_snowflake(bm_t *bm, uint16_t *fb)
{
	int frame_steps = LED_ROWS * 6; // in-still-out, each costs 2xLED_ROWS step
	int frames = ALIGN(bm->width, LED_COLS) / LED_COLS;
	int frame = mod(bm->anim_step, frame_steps*frames)/frame_steps;

	int c = mod(bm->anim_step, frame_steps);
	bm->anim_step++;

	if (c < LED_ROWS * 2) {
		snowflake_in(bm, fb, c - LED_ROWS * 2, frame);
	} else if (c <= LED_ROWS * 4) {
		still(bm, fb, frame);
	} else {
		snowflake_out(bm, fb, -(c - LED_ROWS * 4), frame);
	}
}

void ani_animation(bm_t *bm, uint16_t *fb)
{
	int frames = ALIGN(bm->width, LED_COLS) / LED_COLS;
	int c = mod(bm->anim_step++, frames);
	still(bm, fb, c);
}

void ani_fixed(bm_t *bm, uint16_t *fb)
{
	still(bm, fb, 0);
}

static void picture(bm_t *bm, uint16_t *fb, int step, int frame)
{
	int hc = LED_COLS / 2;
	int range = mod(step - 1, LED_COLS);

	if (range > LED_COLS/2) {
		still(bm, fb, frame);
		return;
	}

	frame *= LED_COLS;
	int i = 0;
	for (; i <= range; i++) {
		fb[hc + i - 1] = (hc + i - 1 + frame >= bm->width) ?
				0 : bm->buf[hc + i - 1 + frame];
		fb[hc - i] = (hc - i + frame >= bm->width) ?
				0 : bm->buf[hc - i + frame];
	}

	if (i >= LED_COLS)
		return;
	fb[hc + i - 1] = -1;
	fb[hc - i] = -1;

	for (i++; i< LED_COLS; i++) {
		fb[hc + i - 1] = 0;
		fb[hc - i] = 0;
	}
}

static void picture_out(bm_t *bm, uint16_t *fb, int step)
{
	int hc = LED_COLS / 2;

	if (step > hc)
		return;

	int range = step;
	int i = 0;
	for (; i <= range; i++) {
		fb[hc + i - 1] = 0;
		fb[hc - i] = 0;
	}
	if (i >= LED_COLS)
		return;
	fb[hc + i - 1] = -1;
	fb[hc - i] = -1;
}

void ani_picture(bm_t *bm, uint16_t *fb)
{
	int last_steps = LED_COLS / 2;
	int frame_steps = LED_COLS;
	int frames = ALIGN(bm->width, LED_COLS) / LED_COLS + 1;
	int frame = mod(bm->anim_step, frame_steps*frames)/frame_steps;
	bm->anim_step++;

	if (frame == frames - 1) {
		picture_out(bm, fb, mod(bm->anim_step, LED_COLS) );

		/* picture_out() costs only half LED_COLS */
		if (mod(bm->anim_step, LED_COLS) >= last_steps) {
			bm->anim_step = 0;
		}
		return;
	}
	picture(bm, fb, bm->anim_step, frame);
}

void ani_marque(bm_t *bm, uint16_t *fb)
{
	int tpl = 0b000100010001;
	static int step;
	step++;

	int i;
	for (i = 0 ; i < LED_COLS - 1; i++) {
		fb[i] = fb[i] & ~((1 << (LED_ROWS - 1)) | 1);

		fb[i] |= !!(tpl & (1 << ((step + i) % 4)) );
		fb[i] |= !!(tpl & (1 << mod(-step + i - 2, 4))) << (LED_ROWS - 1);
	}

	fb[0] = tpl << (step % 4);
	fb[LED_COLS - 1] = tpl >> ((step + 3) % 4);

}

void ani_flash_toggle(bm_t *bm, uint16_t *fb)
{
	static int tog;
	if (tog) {
		fb_fill(fb, 0);
	}
	tog = !tog;
}