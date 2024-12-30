#ifndef __UTIL_H__
#define __UTIL_H__

void fb_displaychar(char c, int col, int row, uint16_t *fb);
void fb_displays(char *s, int len, int col, int row, uint16_t *fb);
void clear_screen(uint16_t *fb);

#endif /* __UTIL_H__ */