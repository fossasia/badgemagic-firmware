#include "leddrv.h"

#define LED_PINCOUNT (23)

volatile int drive_strength;

typedef enum {
	FLOATING,
	LOW,
	HIGH,
} tristate_t;

typedef struct {
	uint32_t *port_buf;
	uint32_t *cfg_buf;
	uint32_t pin;
} pinctrl_t;

static void gpio_buf_set(pinctrl_t pinctl, tristate_t state)
{
	if (state == FLOATING) {
		*(pinctl.cfg_buf) &= ~pinctl.pin;
	} else {
		if (state == HIGH)
			*(pinctl.port_buf) |= pinctl.pin;
		else
			*(pinctl.port_buf) &= ~pinctl.pin;

		*(pinctl.cfg_buf) |= pinctl.pin;
	}
}

void led_setDriveStrength(int is_20mA)
{
	drive_strength = is_20mA;
}

static void gpio_buf_apply(
				volatile uint8_t *gpio_base,
				uint32_t *port, uint32_t *cfg,
				uint32_t *mask)
{
	volatile uint32_t *drv = (volatile uint32_t *)(gpio_base + GPIO_PD_DRV);
	volatile uint32_t *dir = (volatile uint32_t *)(gpio_base + GPIO_DIR);
	volatile uint32_t *out = (volatile uint32_t *)(gpio_base + GPIO_OUT);
	uint32_t drv_val, dir_val, out_val;

	drv_val = 0;
	if (drive_strength) {
		drv_val = *drv;
	}
	dir_val = *dir;
	out_val = *out;

	drv_val = (drv_val & ~*mask) | (*cfg & *mask);
	out_val = (out_val & ~*mask) | (*port & *mask);
	dir_val = (dir_val & ~*mask); // first turn off drive to all Pins

	*dir = dir_val;
	*out = out_val;
	if (drive_strength) {
		*drv = drv_val;
	}
	dir_val = dir_val | (*cfg & *mask); // now turn on drive to right Pins
	*dir = dir_val;
}

static uint32_t PA_buf;
static uint32_t PB_buf;
static uint32_t PAcfg_buf;
static uint32_t PBcfg_buf;
static uint32_t PA_mask;
static uint32_t PB_mask;

#define GPIO_APPLY_ALL() \
	gpio_buf_apply(BA_PA, &PA_buf, &PAcfg_buf, &PA_mask); \
	gpio_buf_apply(BA_PB, &PB_buf, &PBcfg_buf, &PB_mask)

#define PINCTRL(x, pin) {	\
	&P##x##_buf, 		\
	&P##x##cfg_buf, 	\
	GPIO_Pin_##pin 			\
}

static const pinctrl_t led_pins[LED_PINCOUNT] = {
	PINCTRL(A, 15), // A
	PINCTRL(B, 18), // B
	PINCTRL(B, 0),  // C
	PINCTRL(B, 7),  // D
	PINCTRL(A, 12), // E
	PINCTRL(A, 10), // F
	PINCTRL(A, 11), // G
	PINCTRL(B, 9),  // H
	PINCTRL(B, 8),  // I
	PINCTRL(B, 15), // J
	PINCTRL(B, 14), // K
	PINCTRL(B, 13), // L
	PINCTRL(B, 12), // M
	PINCTRL(B, 5),  // N
	PINCTRL(A, 4),  // O
	PINCTRL(B, 3),  // P
	PINCTRL(B, 4),  // Q
	PINCTRL(B, 2),  // R
	PINCTRL(B, 1),  // S
#ifdef USBC_VERSION
	PINCTRL(B, 6), // T
#else
	PINCTRL(B, 23), // T
#endif
	PINCTRL(B, 21), // U
	PINCTRL(B, 20), // V
	PINCTRL(B, 19), // W
};

void led_init()
{
	for (int i=0; i<LED_PINCOUNT; i++) {
		if (led_pins[i].port_buf == &PA_buf)
			PA_mask |= led_pins[i].pin;
		else
			PB_mask |= led_pins[i].pin;
	}
}

void leds_releaseall() {
	for (int i=0; i<LED_PINCOUNT; i++)
		gpio_buf_set(led_pins[i], FLOATING);
	GPIO_APPLY_ALL();
}

static void led_write2dcol_raw(int dcol, uint32_t val)
{
	// TODO: assert params
	gpio_buf_set(led_pins[dcol], HIGH);
	for (int i=0; i<LED_PINCOUNT; i++) {
		if (i == dcol) continue;
		gpio_buf_set(led_pins[i], (val & 0x01) ? LOW : FLOATING); // danger: floating=0 (led off) or low=1 (led on)
		val >>= 1;
	}
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
	row = row*2 + (which_half != 0);

	gpio_buf_set(led_pins[row], LOW);
	for (int i=0; i<LED_PINCOUNT; i++) {
		if (i == row) continue;
		gpio_buf_set(led_pins[i], (val & 0x01) ? HIGH : FLOATING);
		val >>= 1;
	}
	GPIO_APPLY_ALL();
}
