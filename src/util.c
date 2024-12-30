#include "font.h"
#include "leddrv.h"

void clear_screen(uint16_t *fb) {
    for (int i = 0; i < LED_COLS; i++) {
        fb[i] = 0;
    }
}

void fb_displaychar(char c, int col, int row, uint16_t *fb)
{
	for (int i=0; i < 6; i++) {
		if (col + i >= LED_COLS) break;
		fb[col + i] = (fb[col + i] & ~(0x7f << row)) 
				| (font5x7[c - ' '][i] << row);
	}
}

void fb_displays(char *s, int len, int col, int row, uint16_t *fb)
{
	while (*s && len--) {
		fb_displaychar(*s, col, row, fb);
		col += 6;
		s++;
	}
}