#include "LPC18xx.h"
#include "usb.h"
#include "lpc18xx_scu.h"
#include "lpc18xx_cgu.h"
#include "lpc18_43/usb_lpc18_43.h"
#include "lpc18_43/usb_lpc18_43_internal.h"

DQH_T ep_QH[USB_MAX_NUM_EP*2] __attribute__((aligned(2048)));
DTD_T ep_TD[USB_MAX_NUM_EP*2] __attribute__((aligned(32)));

#define LPC_USB LPC_USB0

// To track OUT lengths (see usb_ep_out_length)
uint32_t ep_read_len[USB_MAX_NUM_EP];

static inline uint32_t EPAdr (uint32_t ep) {
	uint32_t val = (ep & 0x0F) * 2;
	if (ep & 0x80) {
		return val + 1;
	} else {
		return val;
	}
}

void usb_init() {
	scu_pinmux(0x8, 1, MD_PLN_FAST,FUNC1);
	scu_pinmux(0x8, 2, MD_PLN_FAST,FUNC1);

	uint32_t CoreM3Freq = CGU_GetPCLKFrequency(CGU_PERIPHERAL_M3CORE);
	/* Disable PLL first */
	CGU_EnableEntity(CGU_CLKSRC_PLL0, DISABLE);
	/* the usb core requires output clock = 480MHz */
	if(CGU_SetPLL0(CoreM3Freq, 480000000, 0.98, 1.02) != CGU_ERROR_SUCCESS) {
		while(1) {}
	}

	CGU_EntityConnect(CGU_CLKSRC_PLL1, CGU_CLKSRC_PLL0);
	/* Enable PLL after all setting is done */
	CGU_EnableEntity(CGU_CLKSRC_PLL0, ENABLE);
	/* Re-Update the clock freq */
	CGU_UpdateClock();
	/* Distribute to USB0 base clock */
	CGU_EntityConnect(CGU_CLKSRC_PLL0, CGU_BASE_USB0);

	/* Turn on the phy */
	LPC_CREG->CREG0 &= ~(1<<5);
	/* reset the controller */
	LPC_USB->USBCMD_D = USBCMD_RST;
	/* wait for reset to complete */
	while (LPC_USB->USBCMD_D & USBCMD_RST);

	/* Program the controller to be the USB device controller */
	LPC_USB->USBMODE_D = USBMODE_CM_DEV | USBMODE_SDIS | USBMODE_SLOM ;

	/* set OTG transcever in proper state, device is present
	on the port(CCS=1), port enable/disable status change(PES=1). */
	LPC_USB->OTGSC = (1<<3) | (1<<0) /*| (1<<16)| (1<<24)| (1<<25)| (1<<26)| (1<<27)| (1<<28)| (1<<29)| (1<<30)*/;

	NVIC_SetPriority(USB0_IRQn, ((0x04<<3)|0x03));
	NVIC_EnableIRQ(USB0_IRQn); //  enable USB0 interrrupts

	usb_reset();
}

void usb_attach() {
	LPC_USB->USBCMD_D |= USBCMD_RS;
}

void usb_detach() {
	LPC_USB->USBCMD_D &= ~USBCMD_RS;
}

void usb_reset() {
	/* disable all EPs */
	LPC_USB->ENDPTCTRL0 &= ~(EPCTRL_RXE | EPCTRL_TXE);
	LPC_USB->ENDPTCTRL2 &= ~(EPCTRL_RXE | EPCTRL_TXE);
	LPC_USB->ENDPTCTRL3 &= ~(EPCTRL_RXE | EPCTRL_TXE);

	/* Clear all pending interrupts */
	LPC_USB->ENDPTNAK   = 0xFFFFFFFF;
	LPC_USB->ENDPTNAKEN = 0;
	LPC_USB->USBSTS_D     = 0xFFFFFFFF;
	LPC_USB->ENDPTSETUPSTAT = LPC_USB->ENDPTSETUPSTAT;
	LPC_USB->ENDPTCOMPLETE  = LPC_USB->ENDPTCOMPLETE;
	while (LPC_USB->ENDPTPRIME);
	LPC_USB->ENDPTFLUSH = 0xFFFFFFFF;
	while (LPC_USB->ENDPTFLUSH); /* Wait until all bits are 0 */

	/* Set the interrupt Threshold control interval to 0 */
	LPC_USB->USBCMD_D &= ~0x00FF0000;

	/* Zero out the Endpoint queue heads */
	memset((void*)ep_QH, 0, sizeof(ep_QH));
	/* Zero out the device transfer descriptors */
	memset((void*)ep_TD, 0, sizeof(ep_TD));
	memset((void*)ep_read_len, 0, sizeof(ep_read_len));
	/* Configure the Endpoint List Address */
	/* make sure it in on 64 byte boundary !!! */
	/* init list address */
	LPC_USB->ENDPOINTLISTADDR = (uint32_t)ep_QH;
	/* Initialize device queue heads for non ISO endpoint only */
	for (unsigned i = 0; i < USB_MAX_NUM_EP; i++) {
		ep_QH[i].next_dTD = (uint32_t)&ep_TD[i];
	}
	/* Enable interrupts */
	LPC_USB->USBINTR_D =  USBSTS_UI | USBSTS_UEI | USBSTS_PCI | USBSTS_URI | USBSTS_SLI | USBSTS_NAKI;
	/* enable ep0 IN and ep0 OUT */
	ep_QH[0].cap  = QH_MAXP(USB_EP0_SIZE) | QH_IOS | QH_ZLT;
	ep_QH[1].cap  = QH_MAXP(USB_EP0_SIZE) | QH_IOS | QH_ZLT;
	/* enable EP0 */
	LPC_USB->ENDPTCTRL0 = EPCTRL_RXE | EPCTRL_RXR | EPCTRL_TXE | EPCTRL_TXR;
}

void usb_set_address(uint8_t adr) {
	LPC_USB->DEVICEADDR = USBDEV_ADDR(adr);
	LPC_USB->DEVICEADDR |= USBDEV_ADDR_AD;
}

#define USB_EP_CONFIG(ep) (((uint32_t*)&(LPC_USB->ENDPTCTRL0))[ep & 0x7f])

void usb_enable_ep(usb_ep ep, uint8_t type, usb_size pkt_size) {
	uint8_t num = EPAdr(ep);

	if (type != USB_EP_TYPE_ISOCHRONOUS){
		/* init EP capabilities */
		ep_QH[num].cap  = QH_MAXP(pkt_size) | QH_ZLT;
		/* The next DTD pointer is INVALID */
		ep_TD[num].next_dTD = 0x01 ;

	} else{
		/* init EP capabilities */
		ep_QH[num].cap  = QH_MAXP(0x400) | QH_ZLT;
	}

	// So usb_ep_pending returns false
	ep_TD[num].total_bytes = TD_R_HANDLED;

	if (ep & USB_IN){
		USB_EP_CONFIG(ep) &= ~0xFFFF0000;
		USB_EP_CONFIG(ep) |= EPCTRL_TX_TYPE(type) | EPCTRL_TXR | EPCTRL_TXE;
	} else{
		USB_EP_CONFIG(ep) &= ~0x0000FFFF;
		USB_EP_CONFIG(ep) |= EPCTRL_RX_TYPE(type) | EPCTRL_RXR | EPCTRL_RXE;
	}
}

void usb_disable_ep(usb_ep ep) {
	if (ep & USB_IN){
		USB_EP_CONFIG(ep) &= ~0xFFFF0000;
	} else{
		USB_EP_CONFIG(ep) &= ~0x0000FFFF;
	}
}

void usb_reset_ep(usb_ep ep) {
	/* flush EP buffers */
	LPC_USB->ENDPTFLUSH = USB_EP_BITMASK(ep);
	while (LPC_USB->ENDPTFLUSH & USB_EP_BITMASK(ep));

	/* reset data toggles */
	if (ep & USB_IN) {
		USB_EP_CONFIG(ep) |= EPCTRL_TXR;
	} else {
		USB_EP_CONFIG(ep) |= EPCTRL_RXR;
	}
}

void usb_set_stall_ep(usb_ep ep) {
	if (ep & USB_IN) {
		USB_EP_CONFIG(ep) |= EPCTRL_TXS;
	} else {
		USB_EP_CONFIG(ep) |= EPCTRL_RXS;
	}
}

void usb_clr_stall_ep(usb_ep ep) {
	if (ep & USB_IN) {
		USB_EP_CONFIG(ep) &= ~EPCTRL_TXS;
		USB_EP_CONFIG(ep) |= EPCTRL_TXR;
	} else {
		USB_EP_CONFIG(ep) &= ~EPCTRL_RXS;
		USB_EP_CONFIG(ep) |= EPCTRL_RXR;
	}
}


static void USB_ProgDTD(uint32_t Edpt, uint32_t ptrBuff, uint32_t TsfSize) {
	DTD_T*  pDTD = &ep_TD[Edpt];

	/* The next DTD pointer is INVALID */
	pDTD->next_dTD = 0x01 ;

	/* Length */
	pDTD->total_bytes = ((TsfSize & 0x7fff) << 16) | TD_IOC | TD_ACTIVE;

	pDTD->buffer0 = ptrBuff;
	pDTD->buffer1 = (ptrBuff + 0x1000) & 0xfffff000;
	pDTD->buffer2 = (ptrBuff + 0x2000) & 0xfffff000;
	pDTD->buffer3 = (ptrBuff + 0x3000) & 0xfffff000;
	pDTD->buffer4 = (ptrBuff + 0x4000) & 0xfffff000;

	ep_QH[Edpt].next_dTD = (uint32_t)(&ep_TD[ Edpt ]);
	ep_QH[Edpt].total_bytes &= (~(TD_ACTIVE | TD_HALTED)) ;
}

bool usb_ep_ready(usb_ep ep) {
	DTD_T* pDTD = &ep_TD[ EPAdr(ep) ];
	return (pDTD->next_dTD & 1) && ((pDTD->total_bytes & TD_ACTIVE)==0);
}

bool usb_ep_pending(usb_ep ep) {
	return usb_ep_ready(ep) && !(ep_TD[ EPAdr(ep) ].total_bytes & TD_R_HANDLED);
}

usb_bank usb_ep_start_out(usb_ep ep, uint8_t* data, usb_size len) {
	USB_ProgDTD(EPAdr(ep), (uint32_t)data, len);
	ep_read_len[ep & 0x0F] = len;
	LPC_USB->ENDPTPRIME |= USB_EP_BITMASK(ep);
	return 0;
}

usb_size usb_ep_out_length(usb_ep ep) {
	DTD_T* pDTD = &ep_TD[EPAdr(ep)];
	// Controller stores the remaining buffer space. Subtract it from the original to get the length.
  	return ep_read_len[ep & 0x0F] - ((pDTD->total_bytes >> 16) & 0x7FFF);
}

usb_bank usb_ep_start_in(usb_ep ep, const uint8_t* data, usb_size len, bool zlp) {
	unsigned n = EPAdr(ep);
	USB_ProgDTD(n, (uint32_t)data, len);
	if (zlp) {
		ep_QH[n].cap  &= ~QH_ZLT; // ZLT bit is active low
	} else {
		ep_QH[n].cap  |= QH_ZLT;
	}
	LPC_USB->ENDPTPRIME |= USB_EP_BITMASK(ep);
	return 0;
}

inline void usb_ep0_out(void) {
	usb_ep_start_out(0x0, ep0_buf_out, sizeof(ep0_buf_out));
}

inline void usb_ep0_in(uint8_t size){
	usb_ep_start_in(0x80, ep0_buf_in, size, false);
}

inline void usb_ep0_stall(void) {
	usb_set_stall_ep(0);
	usb_set_stall_ep(0x80);
}

void usb_ep_handled(usb_ep ep) {
	ep_TD[ EPAdr(ep) ].total_bytes |= TD_R_HANDLED;
}

void usb_set_speed(USB_Speed speed) {
	if (speed >= USB_SPEED_HIGH) {
		LPC_USB->PORTSC1_D &= ~(1<<24);
	} else {
		LPC_USB->PORTSC1_D |= (1<<24);
	}
}

USB_Speed usb_get_speed() {
	return (LPC_USB->PORTSC1_D & (1<<9))?USB_SPEED_HIGH:USB_SPEED_FULL;
}

void USB0_IRQHandler (void) {
	// Device Status Interrupt
	uint32_t disr = LPC_USB->USBSTS_D;
	LPC_USB->USBSTS_D = disr;
	if (disr & USBSTS_URI) { // Reset
		usb_reset();
		usb_cb_reset();
	}

#if 0
	if (disr & USBSTS_SLI) { // Suspend
		usb_cb_suspend();
	}

	if (disr & USBSTS_PCI) { // Resume
		usb_cb_resume();
	}

	if (disr & USBSTS_SRI) { // SOF
		usb_cb_sof();
	}

	if (disr & USBSTS_UEI) {
		usb_cb_error();
	}
#endif

	// Setup status interrupt
	uint32_t val = LPC_USB->ENDPTSETUPSTAT;
	if (val) {
		do {
			LPC_USB->USBCMD_D |= USBCMD_SUTW;
			memcpy((uint8_t*) &usb_setup, (uint8_t*)ep_QH[0].setup, 8);
		} while (!(LPC_USB->USBCMD_D & USBCMD_SUTW));
		LPC_USB->USBCMD_D &= (~USBCMD_SUTW);
		LPC_USB->ENDPTSETUPSTAT = val;
		// Clear the endpoint complete CTRL OUT & IN when a Setup is received
		LPC_USB->ENDPTCOMPLETE = 0x00010001;
		usb_handle_setup();
	}

	// Completion interrupts
	val = LPC_USB->ENDPTCOMPLETE;
	if (val) {
		LPC_USB->ENDPTCOMPLETE = val; // CHECK: NXP cleared this individually

		if (val & (1 << 0)) {
			usb_handle_control_out_complete();
		}

		if (val & (1 << 16)) {
			usb_handle_control_in_complete();
		}

		for (uint8_t n=1; n < USB_MAX_NUM_EP; n++) {
			if (val & (1<<(n+1))) {
			}
			if (val & (1<<(n + 1 + 16))) {
			}
		}

		usb_cb_completion();

		LPC_USB->ENDPTNAK = val;
	}
}

