#pragma once

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#include "Descriptors.h"

#include "usb.h"

/* Function Prototypes: */
void SetupHardware(void);

void EVENT_USB_Device_ControlRequest(void);

