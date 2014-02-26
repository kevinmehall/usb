#pragma once

#include "usb_standard.h"

// Your usb_config.h
#include "usb_config.h"

extern USB_SetupPacket usb_setup;

#define ARR_LEN(x) (sizeof(x)/sizeof(x[0]))

/// Callbacks
uint16_t usb_cb_get_descriptor(uint8_t type, uint8_t index, const uint8_t** descriptor_ptr);


/// Internal
void usb_handle_setup(void);
void usb_handle_control_out_complete(void);
void usb_handle_control_in_complete(void);


#if USB_HW == XMEGA
#include "usb_xmega.h"
#endif