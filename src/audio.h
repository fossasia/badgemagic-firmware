#ifndef __AUDIO_H__
#define __AUDIO_H__
#define AUDIO_MODES_COUNT 4

#include <stdint.h>
#include "leddrv.h"

extern const uint16_t amp_wav_lut[8];
extern const uint16_t amp_wav_lut_w1[8];

int16_t mic_adc();
void mic_init();
void beat_visualize_poll(volatile uint16_t *fb);
void spectrum_visualize_poll(volatile uint16_t *fb);
void mic_measure_rate();
void audio_reset();
void debug_raw_adc_stream();

#endif /* __AUDIO_H__ */