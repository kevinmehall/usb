#include <avr/io.h>
#include <avr/interrupt.h>
#include "xmega/usb_xmega.h"

USB_ENDPOINTS(0);

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
