#pragma once

#include <parts.h>
#include <io.h>
#include "usb.h"

extern UsbDeviceDescriptor usb_endpoints[];
extern const uint8_t usb_num_endpoints;

#define USB_ENDPOINTS(NUM_EP) \
	const uint8_t usb_num_endpoints = (NUM_EP); \
	UsbDeviceDescriptor usb_endpoints[(NUM_EP)+1];
