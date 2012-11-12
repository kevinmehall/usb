// Demo USB device for ATxmega32a4u
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Licensed under the terms of the GNU GPLv3+

#include <avr/io.h>
#include "xmegatest.h"
#include "usb_pipe.h"

USB_PIPE(ep_in,  0x81 | USB_EP_PP, USB_EP_TYPE_BULK_gc, 64, 8, PIPE_ENABLE_FLUSH);
USB_PIPE(ep_out, 0x02 | USB_EP_PP, USB_EP_TYPE_BULK_gc, 64, 8, 0);

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

	while (1){
	}
}

void EVENT_USB_Device_ConfigurationChanged(uint8_t config){
	usb_pipe_init(&ep_in);
	usb_pipe_init(&ep_out);

	TCC0.CTRLA = TC_CLKSEL_DIV1024_gc; // 31.25KHz = 0.032ms
	TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc; // interrupt on timer overflow
	TCC0.PER = 1563; // ~50ms
	TCC0.CNT = 0;
}

ISR(USB_BUSEVENT_vect){
	if (USB.INTFLAGSACLR & USB_SOFIF_bm){
		USB.INTFLAGSACLR = USB_SOFIF_bm;
	}else if (USB.INTFLAGSACLR & (USB_CRCIF_bm | USB_UNFIF_bm | USB_OVFIF_bm)){
		USB.INTFLAGSACLR = (USB_CRCIF_bm | USB_UNFIF_bm | USB_OVFIF_bm);
	}else if (USB.INTFLAGSACLR & USB_STALLIF_bm){
		USB.INTFLAGSACLR = USB_STALLIF_bm;
	}else{
		USB.INTFLAGSACLR = USB_SUSPENDIF_bm | USB_RESUMEIF_bm | USB_RSTIF_bm;
		USB_Evt_Task();
	}
}

ISR(USB_TRNCOMPL_vect){
	USB.FIFOWP = 0;
	USB.INTFLAGSBCLR = USB_SETUPIF_bm | USB_TRNIF_bm;
	USB_Task();
	usb_pipe_handle(&ep_in);
	usb_pipe_handle(&ep_out);
}

uint8_t counter = 0;
ISR(TCC0_OVF_vect){
	if (usb_pipe_can_read(&ep_out) && usb_pipe_can_write(&ep_in)){
		PORTR.OUTTGL = 2;
	}
	while (usb_pipe_can_read(&ep_out) && usb_pipe_can_write(&ep_in)){
		uint8_t v = usb_pipe_read_byte(&ep_out);
		if (v == 0){
			usb_pipe_flush(&ep_in);
		}else{
			usb_pipe_write_byte(&ep_in, v);
		}
	}
}


/** Event handler for the library USB Control Request reception event. */
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req){
	if ((req->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR){
		if (req->bRequest == 0xBB){
			USB_enter_bootloader();
		}
	}
	
	return false;
}

