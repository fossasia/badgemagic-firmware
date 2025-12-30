#include "leddrv.h"

#define LED_PINCOUNT (23)

static uint32_t g_pindrive_strong;

typedef enum {
	FLOATING,
	LOW,
	HIGH,
} tristate_t;

typedef struct pinbank {
	volatile uint8_t *base;

	uint32_t bankpins_mask;

	uint32_t out_val;
	uint32_t dir_val;
	uint32_t drv_val;
} pinbank_t;

typedef struct pindesc {
	pinbank_t *bank;
	uint32_t pin_mask;
} pindesc_t;

static struct pinbanks {
	pinbank_t A;
	pinbank_t B;
} g_PinBanks;


static void gpio_pin_set(const pindesc_t *desc, tristate_t state)
{
	switch(state) {
	case FLOATING:
		desc->bank->dir_val &= ~desc->pin_mask;
		break;
	case HIGH:
		desc->bank->dir_val |= desc->pin_mask;
		desc->bank->out_val |= desc->pin_mask;
		break;
	default:
		desc->bank->dir_val |= desc->pin_mask;
		desc->bank->out_val &= ~desc->pin_mask;
	}
}

static void gpio_bank_tristate(pinbank_t *bank)
{
	volatile uint32_t *out = (volatile uint32_t *)(bank->base + GPIO_OUT);
	volatile uint32_t *drv = (volatile uint32_t *)(bank->base + GPIO_PD_DRV);
	volatile uint32_t *dir = (volatile uint32_t *)(bank->base + GPIO_DIR);
	uint32_t bankpins_mask;

	// Get state for pins we do NOT control
	bankpins_mask = bank->bankpins_mask;
	bank->out_val =
		(bank->out_val & bankpins_mask) | (*out & ~bankpins_mask);
	bank->dir_val =
		(bank->dir_val & bankpins_mask) | (*dir & ~bankpins_mask);
	bank->drv_val =
		(g_pindrive_strong & bank->dir_val & bankpins_mask) | (*drv & ~bankpins_mask);

	// ... and tristate pins, we DO control
	*dir = (bank->dir_val & ~bankpins_mask);
}

static void gpio_bank_apply(pinbank_t *bank)
{
	volatile uint32_t *out = (volatile uint32_t *)(bank->base + GPIO_OUT);
	volatile uint32_t *drv = (volatile uint32_t *)(bank->base + GPIO_PD_DRV);
	volatile uint32_t *dir = (volatile uint32_t *)(bank->base + GPIO_DIR);

	*out = bank->out_val;
	*drv = bank->drv_val;
	*dir = bank->dir_val;
}

#define GPIO_APPLY_ALL() do { \
	gpio_bank_tristate(&g_PinBanks.A); \
	gpio_bank_tristate(&g_PinBanks.B); \
	gpio_bank_apply(&g_PinBanks.A); \
	gpio_bank_apply(&g_PinBanks.B); \
} while (0)


#define PINDESC(bank_, pinnr_) { \
	&g_PinBanks.bank_ , \
	GPIO_Pin_##pinnr_ \
}

static const pindesc_t led_pins[LED_PINCOUNT] = {
	PINDESC(A, 15), // A
	PINDESC(B, 18), // B
	PINDESC(B, 0),  // C
	PINDESC(B, 7),  // D
	PINDESC(A, 12), // E
	PINDESC(A, 10), // F
	PINDESC(A, 11), // G
	PINDESC(B, 9),  // H
	PINDESC(B, 8),  // I
#ifdef USBC_VERSION
	PINDESC(B, 17), // J
#else
	PINDESC(B, 15), // J
#endif
#ifdef USBC_VERSION
	PINDESC(B, 16), // K
#else
	PINDESC(B, 14), // K
#endif
	PINDESC(B, 13), // L
	PINDESC(B, 12), // M
	PINDESC(B, 5),  // N
	PINDESC(A, 4),  // O
	PINDESC(B, 3),  // P
	PINDESC(B, 4),  // Q
	PINDESC(B, 2),  // R
	PINDESC(B, 1),  // S
#ifdef USBC_VERSION
	PINDESC(B, 6), // T
#else
	PINDESC(B, 23), // T
#endif
	PINDESC(B, 21), // U
	PINDESC(B, 20), // V
	PINDESC(B, 19), // W
};

void led_init()
{
	int i;

	g_PinBanks.A.base = BA_PA;
	g_PinBanks.B.base = BA_PB;
	for (i = 0; i < LED_PINCOUNT; i++) {
		led_pins[i].bank->bankpins_mask |= led_pins[i].pin_mask;
	}
}

void leds_releaseall() {
	for (int i=0; i<LED_PINCOUNT; i++)
		gpio_pin_set(led_pins + i, FLOATING);
	g_pindrive_strong = 0x00000000;
	GPIO_APPLY_ALL();
}

static void led_write2dcol_raw(int dcol, uint32_t val)
{
	int on_count;
	int pin_value;

	gpio_pin_set(led_pins + dcol, HIGH);
	on_count = 0;
	for (int i=0; i<LED_PINCOUNT; i++) {
		if (i == dcol) continue;
		pin_value = FLOATING;
		if (val & 0x01) {
			on_count++;
			pin_value = LOW; // pin LOW => LED on
		}
		gpio_pin_set(led_pins + i, pin_value);
		val >>= 1;
	}
	g_pindrive_strong = 0x00000000;
	if (on_count > 5)
		g_pindrive_strong = 0xFFFFFFFF;
	GPIO_APPLY_ALL();
}

static uint32_t combine_cols(uint16_t col1_val, uint16_t col2_val)
{
	uint32_t dval = 0;
	dval |= ((col1_val & 0x01) << (LED_ROWS*2));
	dval |= ((col2_val & 0x01) << (LED_ROWS*2+1));
	for (int i=0; i<LED_ROWS; i++) {
		col1_val >>= 1;
		col2_val >>= 1;

		dval >>= 2;
		dval |= ((col1_val & 0x01) << (LED_ROWS*2));
		dval |= ((col2_val & 0x01) << (LED_ROWS*2+1));
	}
	return dval;
}

void led_write2dcol(int dcol, uint16_t col1_val, uint16_t col2_val)
{
	// first leds in first two columns are switched
	if (dcol == 0) {
		uint16_t b1 = col1_val & 0x01;
		uint16_t b2 = col2_val & 0x01;
		col1_val = (col1_val & 0xFFFE) | b2;
		col2_val = (col2_val & 0xFFFE) | b1;
	}
	led_write2dcol_raw(dcol, combine_cols(col1_val, col2_val));
}

void led_write2row_raw(int row, int which_half, uint32_t val)
{
	int on_count;
	int pin_value;

	row = row*2 + (which_half != 0);
	gpio_pin_set(led_pins + row, LOW);
	on_count = 0;
	for (int i=0; i<LED_PINCOUNT; i++) {
		if (i == row) continue;
		pin_value = FLOATING;
		if (val & 0x01) {
			on_count++;
			pin_value = HIGH;
		}
		gpio_pin_set(led_pins + i, pin_value);
		val >>= 1;
	}
	g_pindrive_strong = 0x00000000;
	if (on_count > 5)
		g_pindrive_strong = 0xFFFFFFFF;
	GPIO_APPLY_ALL();
}
