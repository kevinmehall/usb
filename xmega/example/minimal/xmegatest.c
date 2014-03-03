#include <avr/io.h>
#include <avr/interrupt.h>
#include "xmega/usb_xmega.h"

USB_ENDPOINTS(1);
USB_INTERFACES(0);

int main(void){
	PORTE.DIRSET = (1<<0) | (1<<1);
	PORTE.OUTSET = (1<<0);
	PORTR.DIRSET = 1 << 1;
	
	usb_configure_clock();

	// Enable USB interrupts
	USB.INTCTRLA = /*USB_SOFIE_bm |*/ USB_BUSEVIE_bm | USB_INTLVL_MED_gc;
	USB.INTCTRLB = USB_TRNIE_bm | USB_SETUPIE_bm;

	usb_init();

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
	sei(); 

	usb_attach();

	while (1){}
}

const USB_Device usb_device_config = {
	.cb_reset = NULL,
	.cb_control_setup = NULL,
	.cb_control_in_complete = NULL,
	.cb_control_out_complete = NULL,
};

const USB_Interface usb_interface_config[] = {
	{
		.cb_control_setup = NULL,
		.cb_control_in_complete = NULL,
		.cb_control_out_complete = NULL,
		.cb_set_interface = NULL,
	},
};

const USB_Endpoint_Callback usb_in_endpoint_callbacks[] = {
	NULL,
};

const USB_Endpoint_Callback usb_out_endpoint_callbacks[] = {
	NULL,
};


