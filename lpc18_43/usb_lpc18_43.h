#pragma once

#include "usb.h"

#define USB_MAX_NUM_EP 6

#define USB_INTERFACES(NUM_INT) \
	const uint8_t usb_num_interfaces = (NUM_INT);

#define USB_ENDPOINTS(NUM_EP) \
	const uint8_t usb_num_endpoints = (NUM_EP);
