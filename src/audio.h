#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <stdint.h>

extern const uint16_t amp_wav_lut[8];
extern const uint16_t amp_wav_lut_w1[8];

int16_t mic_adc();
void mic_init();

#endif /* __AUDIO_H__ */
