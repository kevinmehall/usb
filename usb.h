#pragma once

#include "usb_standard.h"

extern USB_SetupPacket usb_setup;

#if USB_HW == XMEGA
#include "usb_xmega.h"
#endif