#include "CH58x_common.h"
#include <stdio.h>
#include <math.h>
#include "usb/usb.h"
#include "leddrv.h"
#include "audio.h"

#define SPEC_WINDOW  64
#define NUM_BANDS    8

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

void debug_raw_adc_stream()
{
    char buf[32];
    
    while(1) {
        uint16_t raw_sample = ADC_ExcutSingleConver();
        
        int len = snprintf(buf, sizeof(buf), "%u\r\n", raw_sample);
        cdc_tx_poll((uint8_t *)buf, len, 10);
        DelayMs(5); 
    }
}

int16_t mic_adc()
{
    ADC_ExtSingleChSampInit(SampleFreq_3_2, ADC_PGA_0);
    ADC_ChannelCfg(11);

    uint16_t sig_min = 4095;

    for (int i = 0; i < 64; i++) {
        uint16_t sample = ADC_ExcutSingleConver();
        if (sample < sig_min) sig_min = sample;
        DelayUs(60);
    }

    int16_t amplitude = 4095 - sig_min;

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "amp=%d min=%d\r\n",
                       amplitude, sig_min);
    cdc_tx_poll((uint8_t *)buf, len, 10);

    return amplitude;
}

void mic_init()
{
    GPIOA_ModeCfg(GPIO_Pin_7, GPIO_ModeIN_Floating);
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

static uint32_t measured_fs_hz = 0;

void mic_measure_rate()
{
    uint32_t t1 = SYS_GetSysTickCnt();

    volatile int16_t dummy[64];
    for (int i = 0; i < 64; i++) {
        dummy[i] = ADC_ExcutSingleConver();
        DelayUs(60);
    }

    uint32_t t2 = SYS_GetSysTickCnt();
    uint32_t elapsed_ticks = t2 - t1;
    uint32_t elapsed_us = elapsed_ticks / (FREQ_SYS / 1000000);
    measured_fs_hz = (elapsed_us > 0) ? (uint32_t)(64UL * 1000000UL / elapsed_us) : 0;

    char buf[64];
    int len = snprintf(buf, sizeof(buf), "elapsed_us=%lu fs_hz=%lu\r\n",
                       (unsigned long)elapsed_us, (unsigned long)measured_fs_hz);
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

static const int band_freq_hz[NUM_BANDS] = {
    100, 200, 400, 700, 1200, 2000, 3500, 6000
};

static int32_t goertzel_coeff_q14[NUM_BANDS];
static int coeffs_ready = 0;

// Derives Goertzel coefficients from the measured sample rate 
static void init_goertzel_coeffs()
{
    uint32_t fs = measured_fs_hz > 0 ? measured_fs_hz : 16000; // fallback if measurement never ran
    for (int b = 0; b < NUM_BANDS; b++) {
        float k = (int)(0.5f + (SPEC_WINDOW * (float)band_freq_hz[b]) / (float)fs);
        float w = (2.0f * 3.14159265f / SPEC_WINDOW) * k;
        float coeff = 2.0f * cosf(w);
        goertzel_coeff_q14[b] = (int32_t)(coeff * 16384.0f); // Q14
    }
    coeffs_ready = 1;
}

static int16_t spec_buf[SPEC_WINDOW];
static int32_t band_max[NUM_BANDS] = {0};

static void mic_capture_window(int16_t *buf, int n)
{
    for (int i = 0; i < n; i++) {
        uint16_t s = ADC_ExcutSingleConver();
        buf[i] = (int16_t)s - mic_baseline;
        DelayUs(60);
    }
}

static int32_t goertzel_mag(const int16_t *buf, int n, int32_t coeff_q14)
{
    int32_t q0, q1 = 0, q2 = 0;
    for (int i = 0; i < n; i++) {
        q0 = ((coeff_q14 * q1) >> 14) - q2 + buf[i];
        q2 = q1;
        q1 = q0;
    }
    return q1*q1 + q2*q2 - (int32_t)(((int64_t)coeff_q14 * q1 * q2) >> 14);
}

void spectrum_visualize_poll(volatile uint16_t *fb)
{
    if (!coeffs_ready) init_goertzel_coeffs();

    mic_capture_window(spec_buf, SPEC_WINDOW);

    int cols_per_band = LED_COLS / NUM_BANDS;
    for (int b = 0; b < NUM_BANDS; b++) {
        int32_t mag = goertzel_mag(spec_buf, SPEC_WINDOW, goertzel_coeff_q14[b]);
        band_max[b] -= band_max[b] >> 6;          // slow release, per-band
        if (mag > band_max[b]) band_max[b] = mag;

        int level = band_max[b] > 0 ? (int)(((int64_t)mag * 7) / band_max[b]) : 0;
        if (level > 7) level = 7;

        for (int c = b * cols_per_band; c < (b + 1) * cols_per_band; c++)
            fb[c] = amp_wav_lut[level];
    }
}

void audio_reset() {
    energy_avg = 0.0f;
    beat_active = 0;
    memset(band_max, 0, sizeof(band_max));
}