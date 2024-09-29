#include "resource.h"

#include "res/bluetooth.xbm"
#include "res/foss-asia-2.xbm"
#include "res/bat-icon.xbm"
#include "res/bat-warn-icon.xbm"
#include "res/fossasia-badgemagic-ani.xbm"

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

xbm_t bat_xbm = {
	.bits = bat_icon_bits, 
	.w = bat_icon_width, 
	.h = bat_icon_height,
	.fh = 11
};

xbm_t batwarn_xbm = {
	.bits = bat_warn_icon_bits, 
	.w = bat_warn_icon_width, 
	.h = bat_warn_icon_height,
	.fh = 11
};

xbm_t fabm_xbm = {
	.bits = fossasia_badgemagic_ani_bits, 
	.w = fossasia_badgemagic_ani_width, 
	.h = fossasia_badgemagic_ani_height,
	.fh = 11
};