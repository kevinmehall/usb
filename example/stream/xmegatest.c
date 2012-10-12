// Demo USB device for ATxmega32a4u
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Licensed under the terms of the GNU GPLv3+

#include <avr/io.h>
#include "xmegatest.h"
#include "usb_pipe.h"

unsigned int timer = 15625; // 500ms

USB_PIPE(ep_in,  0x81 | USB_EP_PP, USB_EP_TYPE_BULK_gc, 64, 512, 1, 0, PIPE_ENABLE_FLUSH);
USB_PIPE(ep_out, 0x02 | USB_EP_PP, USB_EP_TYPE_BULK_gc, 64, 512, 1, 0, 0);

void configureEndpoint(void){
	usb_pipe_init(&ep_in);
	usb_pipe_init(&ep_out);
}

void pollEndpoint(void){	
	usb_pipe_handle(&ep_in);
	usb_pipe_handle(&ep_out);

/*
	static uint8_t i = 0;

	if (usb_pipe_can_write(&ep_in)){
		
		pipe_write_byte(ep_in.pipe, i++);
		if (i == 75 || i == 137) usb_pipe_flush(&ep_in);
	}
*/


	if (usb_pipe_can_read(&ep_out, 1) && usb_pipe_can_write(&ep_in, 1)){
		PORTR.OUTSET = 2;
		uint8_t d = pipe_read_byte(ep_out.pipe);
		if (d == 0){
			usb_pipe_flush(&ep_in);
		}else{
			pipe_write_byte(ep_in.pipe, d);
			PORTR.OUTCLR = 2;
		}
	}

}

int main(void){
	PORTE.DIRSET = (1<<0) | (1<<1);
	PORTE.OUTSET = (1<<0);
	PORTR.DIRSET = 1 << 1;
	
	USB_ConfigureClock();
	USB_Init();

	sei();
	
	TCC0.CTRLA = TC_CLKSEL_DIV1024_gc; // 31.25KHz = 0.032ms
	
	configureEndpoint();
	
	while (1){
		while(TCC0.CNT < timer){ 
			USB_Task();
			pollEndpoint();
		}
		PORTE.OUTTGL = (1<<0);
    	TCC0.CNT=0;
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

