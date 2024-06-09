#include "resource.h"

#include "res/bluetooth.xbm"
#include "res/foss-asia-2.xbm"

xbm_t bluetooth = {
	.bits = bluetooth_bits, 
	.w = bluetooth_width, 
	.h = bluetooth_height,
	.fh = 11
};

xbm_t splash = {
	.bits = foss_asia_2_bits, 
	.w = foss_asia_2_width, 
	.h = foss_asia_2_height,
	.fh = 11
};