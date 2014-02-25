// Demo USB device for ATxmega32a4u
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Licensed under the terms of the GNU GPLv3+

#include <avr/io.h>
#include "usb.h"

int main(void){
	PORTE.DIRSET = (1<<0) | (1<<1);
	PORTE.OUTSET = (1<<0);
	PORTR.DIRSET = 1 << 1;
	
	USB_ConfigureClock();

	// Enable USB interrupts
	USB.INTCTRLA = /*USB_SOFIE_bm |*/ USB_BUSEVIE_bm | USB_INTLVL_MED_gc;
	USB.INTCTRLB = USB_TRNIE_bm | USB_SETUPIE_bm;

	USB_Init();

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
	sei(); 

	while (1){}
}

void EVENT_USB_Device_ConfigurationChanged(uint8_t config){

}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(USB_SetupPacket* req){
	if ((req->bmRequestType & USB_REQTYPE_TYPE_MASK) == USB_REQTYPE_VENDOR){
		if (req->bRequest == 0xBB){
			USB_enter_bootloader();
		}
	}
	return USB_ep0_stall();
}

