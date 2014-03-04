#include <avr/io.h>

#include "usb.h"
#include "xmega/usb_xmega.h"
#include "xmega/usb_xmega_internal.h"

void usb_init(){
	//uint_reg_t CurrentGlobalInt = GetGlobalInterruptMask();
	//GlobalInterruptDisable();

	NVM.CMD  = NVM_CMD_READ_CALIB_ROW_gc;
	USB.CAL0 = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, USBCAL0));
	NVM.CMD  = NVM_CMD_READ_CALIB_ROW_gc;
	USB.CAL1 = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, USBCAL1));

	//SetGlobalInterruptMask(CurrentGlobalInt);

	usb_reset();
}

void usb_reset(){

	//if (USB_Options & USB_DEVICE_OPT_LOWSPEED)
	//  CLK.USBCTRL = ((((F_USB / 6000000) - 1) << CLK_USBPSDIV_gp) | CLK_USBSRC_RC32M_gc | CLK_USBSEN_bm);
	//else
	CLK.USBCTRL = ((((F_USB / 48000000) - 1) << CLK_USBPSDIV_gp) | CLK_USBSRC_RC32M_gc | CLK_USBSEN_bm);
	USB.EPPTR = (unsigned) &usb_xmega_endpoints;
	USB.ADDR = 0;
	
	usb_xmega_endpoints[0].out.STATUS = 0;
	usb_xmega_endpoints[0].out.CTRL = USB_EP_TYPE_CONTROL_gc | USB_EP_size_to_gc(USB_EP0_SIZE);
	usb_xmega_endpoints[0].out.DATAPTR = (unsigned) &ep0_buf_out;
	usb_xmega_endpoints[0].in.STATUS = USB_EP_BUSNACK0_bm;
	usb_xmega_endpoints[0].in.CTRL = USB_EP_TYPE_CONTROL_gc | USB_EP_size_to_gc(USB_EP0_SIZE);
	usb_xmega_endpoints[0].in.DATAPTR = (unsigned) &ep0_buf_in;
	
	USB.CTRLA = USB_ENABLE_bm | USB_SPEED_bm | (usb_num_endpoints+1);
}

void usb_set_address(uint8_t addr) {
	USB.ADDR = addr;
}

const uint8_t* usb_ep0_from_progmem(const uint8_t* addr, uint16_t size) {
	uint8_t *buf = ep0_buf_in;
	uint16_t remaining = size;
	NVM.CMD = NVM_CMD_NO_OPERATION_gc;
	while (remaining--){
		*buf++ = pgm_read_byte(addr++);
	}
	return ep0_buf_in;
}

#define _USB_EP(epaddr) \
	USB_EP_pair_t* pair = &usb_xmega_endpoints[(epaddr & 0x3F)]; \
	USB_EP_t* e __attribute__ ((unused)) = &pair->ep[!!(epaddr&0x80)]; \

inline void usb_ep_enable(uint8_t ep, uint8_t type, usb_size bufsize){
	_USB_EP(ep);
	e->STATUS = USB_EP_BUSNACK0_bm;
	e->CTRL = type | USB_EP_size_to_gc(bufsize);
}

inline void usb_ep_disable(uint8_t ep) {
	_USB_EP(ep);
	e->CTRL = 0;
}

inline void usb_ep_reset(uint8_t ep){
	_USB_EP(ep);
	e->STATUS = USB_EP_BUSNACK0_bm;
}

inline usb_bank usb_ep_start_out(uint8_t ep, uint8_t* data, usb_size len) {
	_USB_EP(ep);
	e->DATAPTR = (unsigned) data;
	LACR16(&(e->STATUS), USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm);
	return 0;
}

inline usb_bank usb_ep_start_in(uint8_t ep, const uint8_t* data, usb_size size, bool zlp) {
	_USB_EP(ep);
	e->DATAPTR = (unsigned) data;
	e->CNT = size | (zlp << 15);
	LACR16(&(e->STATUS), USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm);
	return 0;
}

inline bool usb_ep_ready(uint8_t ep) {
	_USB_EP(ep);
	return !(e->STATUS & (USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm));
}

inline bool usb_ep_empty(uint8_t ep) {
	_USB_EP(ep);
	return !(e->STATUS & (USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm));
}

inline bool usb_ep_pending(uint8_t ep) {
	_USB_EP(ep);
	return e->STATUS & USB_EP_TRNCOMPL0_bm;
}

inline void usb_ep_handled(uint8_t ep) {
	_USB_EP(ep);
	LACR16(&(e->STATUS), USB_EP_TRNCOMPL0_bm);
}

inline uint16_t usb_ep_out_length(uint8_t ep){
	_USB_EP(ep);
	return e->CNT;
}

inline void usb_detach(void) ATTR_ALWAYS_INLINE;
inline void usb_detach(void) {
	USB.CTRLB &= ~USB_ATTACH_bm;
}

inline void usb_attach(void) ATTR_ALWAYS_INLINE;
inline void usb_attach(void) {
	USB.CTRLB |= USB_ATTACH_bm;
}

/// Enable the OUT stage on the default control pipe.
inline void usb_ep0_out(void) {
	LACR16(&usb_xmega_endpoints[0].out.STATUS, USB_EP_SETUP_bm | USB_EP_BUSNACK0_bm | USB_EP_TRNCOMPL0_bm | USB_EP_OVF_bm);
}

inline void usb_ep0_in(uint8_t size){
	usb_ep_start_in(0x80, ep0_buf_in, size, false);
}

inline void usb_ep0_stall(void) {
	usb_xmega_endpoints[0].out.CTRL |= USB_EP_STALL_bm;
	usb_xmega_endpoints[0].in.CTRL  |= USB_EP_STALL_bm;
}

void usb_set_speed(USB_Speed speed) { }
USB_Speed usb_get_speed() { return USB_SPEED_FULL; }

void usb_configure_clock() {
	// Configure DFLL for 48MHz, calibrated by USB SOF
	OSC.DFLLCTRL = OSC_RC32MCREF_USBSOF_gc;
	NVM.CMD  = NVM_CMD_READ_CALIB_ROW_gc;
	DFLLRC32M.CALB = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, USBRCOSC));
	DFLLRC32M.COMP1 = 0x1B; //Xmega AU manual, 4.17.19
	DFLLRC32M.COMP2 = 0xB7;
	DFLLRC32M.CTRL = DFLL_ENABLE_bm;
	
	CCP = CCP_IOREG_gc; //Security Signature to modify clock 
    OSC.CTRL = OSC_RC32MEN_bm | OSC_RC2MEN_bm; // enable internal 32MHz oscillator
    
    while(!(OSC.STATUS & OSC_RC32MRDY_bm)); // wait for oscillator ready
    
    OSC.PLLCTRL = OSC_PLLSRC_RC2M_gc | 16; // 2MHz * 16 = 32MHz
    
    CCP = CCP_IOREG_gc;
    OSC.CTRL = OSC_RC32MEN_bm | OSC_PLLEN_bm | OSC_RC2MEN_bm ; // Enable PLL
    
    while(!(OSC.STATUS & OSC_PLLRDY_bm)); // wait for PLL ready
    
    DFLLRC2M.CTRL = DFLL_ENABLE_bm;

    CCP = CCP_IOREG_gc; //Security Signature to modify clock 
    CLK.CTRL = CLK_SCLKSEL_PLL_gc; // Select PLL
    CLK.PSCTRL = 0x00; // No peripheral clock prescaler
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
		if (USB.STATUS & USB_BUSRST_bm){
			USB.STATUS &= ~USB_BUSRST_bm;
			usb_reset();
			usb_cb_reset();
		}
	}
}

ISR(USB_TRNCOMPL_vect){
	USB.FIFOWP = 0;
	USB.INTFLAGSBCLR = USB_SETUPIF_bm | USB_TRNIF_bm;

	// Read once to prevent race condition where SETUP packet is interpreted as OUT
	uint8_t status = usb_xmega_endpoints[0].out.STATUS;
	if (status & USB_EP_SETUP_bm){
		// TODO: race conditions because we can't block a setup packet
		LACR16(&(usb_xmega_endpoints[0].out.STATUS), USB_EP_TRNCOMPL0_bm | USB_EP_SETUP_bm);
		memcpy(&usb_setup, ep0_buf_out, sizeof(usb_setup));
		usb_handle_setup();
	}else if(status & USB_EP_TRNCOMPL0_bm){
		usb_handle_control_out_complete();
	}

	if (usb_xmega_endpoints[0].in.STATUS & USB_EP_TRNCOMPL0_bm) {
		usb_handle_control_in_complete();
	}

	usb_cb_completion();
}

