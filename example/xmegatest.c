// Demo USB device for ATxmega32a4u
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Licensed under the terms of the GNU GPLv3+

#include "xmegatest.h"

unsigned int timer = 15625; // 500ms
unsigned char bulkdatain[64];
unsigned char bulkdataout[64];


void configureEndpoint(void){
	endpoints[1].in.DATAPTR = (unsigned) &bulkdatain;
	endpoints[1].in.CNT = 64;
	endpoints[1].in.STATUS = 0;
	endpoints[1].in.CTRL = USB_EP_TYPE_BULK_gc | USB_EP_BUFSIZE_64_gc;
	
	endpoints[2].out.DATAPTR = (unsigned) &bulkdataout;
	endpoints[2].out.CNT = 0;
	endpoints[2].out.STATUS = USB_EP_TOGGLE_bm;
	endpoints[2].out.CTRL = USB_EP_TYPE_BULK_gc | USB_EP_BUFSIZE_64_gc;
	
	bulkdatain[5] = 123;
	bulkdataout[5] = 55;
}

void pollEndpoint(void){
	if (endpoints[1].in.STATUS & USB_EP_TRNCOMPL0_bm){
		bulkdatain[0]++;
		if (bulkdatain[0] == 0) bulkdatain[1]++;
		endpoints[1].in.STATUS &= ~(USB_EP_TRNCOMPL0_bm | USB_EP_BUSNACK0_bm | USB_EP_OVF_bm);
	}
	
	if (endpoints[2].out.STATUS & USB_EP_TRNCOMPL0_bm){
		bulkdatain[2] = bulkdataout[2];
		endpoints[2].out.STATUS &= ~(USB_EP_TRNCOMPL0_bm | USB_EP_BUSNACK0_bm | USB_EP_OVF_bm);
	}
}

int main(void){
	SetupHardware();
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

/** Configures the board hardware and chip peripherals for the project's functionality. */
void SetupHardware(void){
	PORTE.DIRSET = (1<<0) | (1<<1);
	PORTE.OUTSET = (1<<0);
	
	USB_ConfigureClock();
	USB_Init();
}

/** Event handler for the library USB Control Request reception event. */
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req){
	if ((req->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR){
		if (req->bRequest == 0x23){
			timer = req->wValue;
			USB_ep_send_packet(0, 0);
			return true;
		}
	}
	
	return false;
}

