#include "CH58x_common.h"
#include <stdio.h>
#include "usb/usb.h"
#include "leddrv.h"
#include "audio.h"

const uint16_t amp_wav_lut[8] = {
	0b00000000000,
	0b00000100000,
	0b00001110000,
	0b00011111000,
	0b00111111100,
	0b01111111110,
	0b11111111111,
	0b11111111111,
};

const uint16_t amp_wav_lut_w1[8] = {
	0b00000100000,
	0b00001110000,
	0b00011111000,
	0b00111111100,
	0b01111111110,
	0b11111111111,
	0b11111111111,
	0b11111111111,
};

// Beat detection state
static float  energy_avg  = 0.0f;
static int8_t beat_active = 0;
static int16_t mic_baseline = 0;

#define BEAT_DECAY_FRAMES  6
#define BEAT_THRESHOLD 2.2f
#define COL_FULL 0x7FF

int16_t mic_adc()
{
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);
    ADC_ChannelCfg(11);

    uint16_t sig_min = 4095;

    for (int i = 0; i < 64; i++) {
        uint16_t sample = ADC_ExcutSingleConver();
        if (sample < sig_min) sig_min = sample;
    }

    // How far below rail the signal swings = loudness
    int16_t amplitude = 4095 - sig_min;

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "amp=%d min=%d\r\n",
                       amplitude, sig_min);
    cdc_tx_poll((uint8_t *)buf, len, 10);

    return amplitude;
}

void mic_init()
{
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeIN_PU);
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);
    ADC_ChannelCfg(11);

    DelayMs(500);

    uint64_t sum = 0;
    for (int i = 0; i < 64; i++) sum += ADC_ExcutSingleConver();
    mic_baseline = (int16_t)(sum / 64);

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "\r\nbaseline=%d\r\n", mic_baseline);
    cdc_tx_poll((uint8_t *)buf, len, 10);
}

void beat_visualize_poll(volatile uint16_t *fb)
{
    int16_t mic = mic_adc();

    energy_avg = energy_avg * 0.9f + (float)mic * 0.1f;

    int is_beat = (energy_avg > 10.0f) && ((float)mic > energy_avg * BEAT_THRESHOLD);

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "mic=%d avg=%d beat=%d\r\n", mic, (int)energy_avg, is_beat);
    cdc_tx_poll((uint8_t *)buf, len, 10);

    if (is_beat)
        beat_active = BEAT_DECAY_FRAMES;

    if (beat_active > 0) {
        int lit_pairs = (beat_active * 22) / BEAT_DECAY_FRAMES;
        int center = 22;
        for (int c = 0; c < LED_COLS; c++) {
            int dist = (c < center) ? (center - 1 - c) : (c - center);
            fb[c] = (dist < lit_pairs) ? COL_FULL : 0;
        }
        beat_active--;
    } 
    else {
        memset(fb, 0, LED_COLS * sizeof(uint16_t));
    }
}

void audio_reset() {
    energy_avg = 0.0f;
    beat_active = 0;
}