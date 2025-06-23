#include "CH58x_common.h"

#include "utils.h"
#include "debug.h"

extern uint8_t *cfg_desc;

void usb_set_address(uint8_t ad);

USB_DEV_DESCR dev_desc = {
	.bLength = sizeof(USB_DEV_DESCR),
	.bDescriptorType = 0x01,
	.bcdUSB = 0x0110,

	.bDeviceClass = 0x00,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,

	.bMaxPacketSize0 = MAX_PACKET_SIZE,

	.idVendor = 0x0416,
	.idProduct = 0x5020,
	.bcdDevice = 0x0000,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 0x01
};

/* String Descriptor Zero, Specifying Languages Supported by the Device */
static uint8_t lang_desc[] = {
	0x04,       /* bLength */
	0x03,       /* bDescriptorType */
	0x09, 0x04  /* wLANGID - en-US */
};

static uint16_t vendor_info[] = {
	36 | /* bLength */
	0x03 << 8, /* bDescriptorType */

	/* bString */
	'F', 'O', 'S', 'S', 'A', 'S', 'I', 'A', ' ',
	'W', 'A', 'S', ' ', 'H', 'E', 'R', 'E'
};

static uint16_t product_info[] = {
	32 | /* bLength */
	0x03 << 8, /* bDescriptorType */

	/* bString */
	'L', 'E', 'D', ' ',
	'B', 'a', 'd', 'g', 'e', ' ',
	'M', 'a', 'g', 'i', 'c'
};

static uint16_t serial_number[] = {
#ifdef USBC_VERSION
	4 +
#endif
	(12 + sizeof(VERSION_ABBR) - 1) * 2 | /* bLength */
	0x03 << 8, /* bDescriptorType */

	/* bString */
	'B', 'M', '1', '1', '4', '4', 
#ifdef USBC_VERSION
	'-', 'C',
#endif
	' ',
	'f', 'w', ':',' ',
	/* vX.Y */
	VERSION[0],
	VERSION[1],
	VERSION[2],
	VERSION[3],
	VERSION[4],
	VERSION[5],
	VERSION[6],
	VERSION[7],
	VERSION[8]
};

static void desc_dev(USB_SETUP_REQ *request)
{
	ctrl_start_load_block(&dev_desc, dev_desc.bLength);
}

static void desc_config(USB_SETUP_REQ *request)
{
	ctrl_start_load_block(cfg_desc, request->wLength);
}

static void desc_string(USB_SETUP_REQ *request)
{
    static const void *string_index[4] = {
        lang_desc,
        vendor_info,
        product_info,
        serial_number
    };

    uint8_t index = request->wValue & 0xff;
    if (index < 4) {
        uint16_t length;
        if (index == 0) {
            length = ((const uint8_t *)string_index[index])[0];
        } else {
            length = ((const uint16_t *)string_index[index])[0] & 0xFF;
        }
        ctrl_start_load_block((const uint8_t *)string_index[index], length);
    }
}


static void dev_getDesc(USB_SETUP_REQ *request)
{
	_TRACE();
	uint8_t type = request->wValue >> 8;
	
	PRINT("Descriptor type: 0x%02x\n", type);

	static const void (*desc_type_handlers[4])(USB_SETUP_REQ *request) = {
		NULL,
		desc_dev,
		desc_config,
		desc_string
	};
	if (type <= 3 && desc_type_handlers[type])
		desc_type_handlers[type](request);
}

static void dev_getStatus(USB_SETUP_REQ *request)
{
	_TRACE();
	// Remote Wakeup disabled | Bus powered, hardcoded for now
	uint8_t buf[] = {0, 0};
	ctrl_start_load_block(buf, 2);
}

static void dev_clearFeature(USB_SETUP_REQ *request)
{
	_TRACE();
	// DEVICE_REMOTE_WAKEUP and TEST_MODE are not available, ignore.
}

static void dev_setFeature(USB_SETUP_REQ *request)
{
	_TRACE();
	// DEVICE_REMOTE_WAKEUP and TEST_MODE are not available, ignore.
}

static void dev_setAddress(USB_SETUP_REQ *request)
{
	_TRACE();
	/* new address will be loadled in the next IN poll,
	so here just sending a ACK */
	usb_set_address(request->wValue & 0xff);
	ctrl_ack();
}

// For now, multiple configuration is not supported
static uint8_t devcfg;

static void dev_getConfig(USB_SETUP_REQ *request)
{
	_TRACE();
	ctrl_start_load_block(&devcfg, 1);
}

static void dev_setConfig(USB_SETUP_REQ *request)
{
	_TRACE();
	devcfg = (request->wValue) & 0xff;
	ctrl_ack();
}

void handle_devreq(USB_SETUP_REQ *request)
{
	_TRACE();

	static const void (*dev_req_handlers[13])(USB_SETUP_REQ *request) = {
		dev_getStatus,
		dev_clearFeature,
		NULL, // Reserved
		dev_setFeature,
		NULL, // Reserved
		dev_setAddress,
		dev_getDesc,
		NULL, // set desc
		dev_getConfig,
		dev_setConfig,
		NULL, // get interface
		NULL, // set interface
		NULL, // sync frame
	};

	uint8_t req = request->bRequest;
	if (req <= 12 && dev_req_handlers[req])
		dev_req_handlers[req](request);
}
