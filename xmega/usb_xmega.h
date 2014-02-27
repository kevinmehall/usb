#pragma once

#include "usb.h"

/// Configure the XMEGA's clock for use with USB.
void usb_configure_clock(void);

/// Copy data from program memory to the ep0 IN buffer
const uint8_t* usb_ep0_from_progmem(const uint8_t* addr, uint16_t size);
