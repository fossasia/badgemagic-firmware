#ifndef __AUDIO_H__
#define __AUDIO_H__
#define BEAT  2
#define AUDIO_MODES_COUNT 3 

#include <stdint.h>
#include "leddrv.h"

extern const uint16_t amp_wav_lut[8];
extern const uint16_t amp_wav_lut_w1[8];
extern int16_t mic_baseline;

int16_t mic_adc();
void mic_init();
void beat_visualize_poll(volatile uint16_t *fb);

#endif /* __AUDIO_H__ */