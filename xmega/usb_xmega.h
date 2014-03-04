#pragma once

#include "usb.h"

/// Configure the XMEGA's clock for use with USB.
void usb_configure_clock(void);

/// Copy data from program memory to the ep0 IN buffer
const uint8_t* usb_ep0_from_progmem(const uint8_t* addr, uint16_t size);


typedef union USB_EP_pair{
	union{
		struct{
			USB_EP_t out;
			USB_EP_t in;
		};
		USB_EP_t ep[2];
	};
} __attribute__((packed)) USB_EP_pair_t;

extern USB_EP_pair_t usb_xmega_endpoints[];
extern const uint8_t usb_num_endpoints;

/** Like __attribute__(align(2)), but actually works. 
    From http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=121033
 */
#define GCC_FORCE_ALIGN_2  __attribute__((section (".data,\"aw\",@progbits\n.p2align 1;")))

#define USB_ENDPOINTS(NUM_EP) \
	const uint8_t usb_num_endpoints = (NUM_EP); \
	USB_EP_pair_t usb_xmega_endpoints[(NUM_EP)+1] GCC_FORCE_ALIGN_2;
