#include "usb.h"
#include "samd/usb_samd.h"
#include "samd/usb_samd_internal.h"

#include <compiler.h>
#include <pinmux.h>
#include <system_interrupt.h>

#define NVM_USB_PAD_TRANSN_POS  45
#define NVM_USB_PAD_TRANSN_SIZE 5
#define NVM_USB_PAD_TRANSP_POS  50
#define NVM_USB_PAD_TRANSP_SIZE 5
#define NVM_USB_PAD_TRIM_POS  55
#define NVM_USB_PAD_TRIM_SIZE 3

#undef ENABLE

#define USB_GCLK_GEN                    0

void usb_init(){
	uint32_t pad_transn, pad_transp, pad_trim;
	struct system_pinmux_config pin_config;

	PM->APBBMASK.reg |= PM_APBBMASK_USB;

	/* Set up the USB DP/DN pins */
	system_pinmux_get_config_defaults(&pin_config);
	pin_config.mux_position = MUX_PA24G_USB_DM;
	system_pinmux_pin_set_config(PIN_PA24G_USB_DM, &pin_config);
	pin_config.mux_position = MUX_PA25G_USB_DP;
	system_pinmux_pin_set_config(PIN_PA25G_USB_DP, &pin_config);

	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |
			GCLK_CLKCTRL_GEN(USB_GCLK_GEN) |
			GCLK_CLKCTRL_ID(USB_GCLK_ID);

	/* Reset */
	USB->DEVICE.CTRLA.bit.SWRST = 1;
	while (USB->DEVICE.SYNCBUSY.bit.SWRST);

	USB->DEVICE.CTRLA.bit.ENABLE = 1;
	while (USB->DEVICE.SYNCBUSY.bit.ENABLE);


	/* Load Pad Calibration */
	pad_transn =( *((uint32_t *)(NVMCTRL_OTP4)
			+ (NVM_USB_PAD_TRANSN_POS / 32))
		>> (NVM_USB_PAD_TRANSN_POS % 32))
		& ((1 << NVM_USB_PAD_TRANSN_SIZE) - 1);

	if (pad_transn == 0x1F) {
		pad_transn = 5;
	}

	USB->DEVICE.PADCAL.bit.TRANSN = pad_transn;

	pad_transp =( *((uint32_t *)(NVMCTRL_OTP4)
			+ (NVM_USB_PAD_TRANSP_POS / 32))
			>> (NVM_USB_PAD_TRANSP_POS % 32))
			& ((1 << NVM_USB_PAD_TRANSP_SIZE) - 1);

	if (pad_transp == 0x1F) {
		pad_transp = 29;
	}

	USB->DEVICE.PADCAL.bit.TRANSP = pad_transp;

	pad_trim =( *((uint32_t *)(NVMCTRL_OTP4)
			+ (NVM_USB_PAD_TRIM_POS / 32))
			>> (NVM_USB_PAD_TRIM_POS % 32))
			& ((1 << NVM_USB_PAD_TRIM_SIZE) - 1);

	if (pad_trim == 0x7) {
		pad_trim = 3;
	}

	USB->DEVICE.PADCAL.bit.TRIM = pad_trim;

	/* Set the configuration */
	USB->DEVICE.CTRLA.bit.MODE = USB_CTRLA_MODE_DEVICE_Val;

	memset(usb_endpoints, 0, usb_num_endpoints*sizeof(UsbDeviceDescriptor));
	USB->DEVICE.DESCADD.reg = (uint32_t)(&usb_endpoints[0]);

	usb_reset();

	USB->DEVICE.INTENSET.reg = USB_DEVICE_INTENSET_EORST;
	NVIC_EnableIRQ(USB_IRQn);
}

#define USB_EPTYPE_DISABLED 0
#define USB_EPTYPE_CONTROL 1
#define USB_EPTYPE_ISOCHRONOUS 2
#define USB_EPTYPE_BULK 3
#define USB_EPTYPE_INTERRUPT 4
#define USB_EPTYPE_DUAL_BANK 5

void usb_reset(){
	usb_endpoints[0].DeviceDescBank[0].ADDR.reg = (uint32_t) &ep0_buf_out;
	usb_endpoints[0].DeviceDescBank[0].PCKSIZE.bit.SIZE=USB_EP_size_to_gc(USB_EP0_SIZE);
	usb_endpoints[0].DeviceDescBank[1].ADDR.reg = (uint32_t) &ep0_buf_in;
	usb_endpoints[0].DeviceDescBank[1].PCKSIZE.bit.SIZE=USB_EP_size_to_gc(USB_EP0_SIZE);
	usb_endpoints[0].DeviceDescBank[1].PCKSIZE.bit.AUTO_ZLP=1;
	USB->DEVICE.DeviceEndpoint[0].EPINTENSET.reg = USB_DEVICE_EPINTENSET_RXSTP;
	USB->DEVICE.DeviceEndpoint[0].EPCFG.reg  = USB_DEVICE_EPCFG_EPTYPE0(USB_EPTYPE_CONTROL)
	                                         | USB_DEVICE_EPCFG_EPTYPE1(USB_EPTYPE_CONTROL);
}

void usb_set_address(uint8_t addr) {
	USB->DEVICE.DADD.reg = USB_DEVICE_DADD_ADDEN | addr;
}

inline UsbDeviceDescBank* ep_ram(uint8_t epaddr) {
	return &usb_endpoints[epaddr&0x3F].DeviceDescBank[!!(epaddr&0x80)];
}

inline void usb_ep_enable(uint8_t ep, uint8_t type, usb_size bufsize){
	if (ep & 0x80) {
		usb_endpoints[ep & 0x3f].DeviceDescBank[1].PCKSIZE.bit.SIZE = USB_EP_size_to_gc(bufsize);
		USB->DEVICE.DeviceEndpoint[ep & 0x3f].EPCFG.bit.EPTYPE1 = type;
		USB->DEVICE.DeviceEndpoint[ep & 0x3f].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUS_BK1RDY
		                                                      | USB_DEVICE_EPSTATUS_STALLRQ(0x2)
		                                                      | USB_DEVICE_EPSTATUS_DTGLIN;
	} else {
		usb_endpoints[ep & 0x3f].DeviceDescBank[0].PCKSIZE.bit.SIZE = USB_EP_size_to_gc(bufsize);
		USB->DEVICE.DeviceEndpoint[ep & 0x3f].EPCFG.bit.EPTYPE0 = type;
		USB->DEVICE.DeviceEndpoint[ep & 0x3f].EPSTATUSSET.reg = USB_DEVICE_EPSTATUS_BK0RDY;
		USB->DEVICE.DeviceEndpoint[ep & 0x3f].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUS_STALLRQ(0x1)
		                                                      | USB_DEVICE_EPSTATUS_DTGLOUT;
	}
}

inline void usb_ep_disable(uint8_t ep) {
	if (ep & 0x80) {
		USB->DEVICE.DeviceEndpoint[ep & 0x3f].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUS_BK1RDY;
		USB->DEVICE.DeviceEndpoint[ep & 0x3f].EPCFG.bit.EPTYPE1 = USB_EPTYPE_DISABLED;
	} else {
		USB->DEVICE.DeviceEndpoint[ep & 0x3f].EPSTATUSSET.reg = USB_DEVICE_EPSTATUS_BK0RDY;
		USB->DEVICE.DeviceEndpoint[ep & 0x3f].EPCFG.bit.EPTYPE0 = USB_EPTYPE_DISABLED;
	}
}

inline void usb_ep_reset(uint8_t ep){
	if (ep & 0x80) {
		USB->DEVICE.DeviceEndpoint[ep & 0x3f].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUS_BK1RDY;
	} else {
		USB->DEVICE.DeviceEndpoint[ep & 0x3f].EPSTATUSSET.reg = USB_DEVICE_EPSTATUS_BK0RDY;
	}
}

inline usb_bank usb_ep_start_out(uint8_t ep, uint8_t* data, usb_size len) {
	usb_endpoints[ep].DeviceDescBank[0].PCKSIZE.bit.MULTI_PACKET_SIZE = len;
	usb_endpoints[ep].DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT = 0;
	usb_endpoints[ep].DeviceDescBank[0].ADDR.reg = (uint32_t) data;
	USB->DEVICE.DeviceEndpoint[ep].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT0 | USB_DEVICE_EPINTFLAG_TRFAIL0;
	USB->DEVICE.DeviceEndpoint[ep].EPINTENSET.reg = USB_DEVICE_EPINTENSET_TRCPT0;
	USB->DEVICE.DeviceEndpoint[ep].EPSTATUSCLR.reg = USB_DEVICE_EPSTATUS_BK0RDY;
	return 0;
}

inline usb_bank usb_ep_start_in(uint8_t ep, const uint8_t* data, usb_size size, bool zlp) {
	ep &= 0x3f;
	usb_endpoints[ep].DeviceDescBank[1].PCKSIZE.bit.AUTO_ZLP = zlp;
	usb_endpoints[ep].DeviceDescBank[1].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
	usb_endpoints[ep].DeviceDescBank[1].PCKSIZE.bit.BYTE_COUNT = size;
	usb_endpoints[ep].DeviceDescBank[1].ADDR.reg = (uint32_t) data;
	USB->DEVICE.DeviceEndpoint[ep].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT1 | USB_DEVICE_EPINTFLAG_TRFAIL1;
	USB->DEVICE.DeviceEndpoint[ep].EPINTENSET.reg = USB_DEVICE_EPINTENSET_TRCPT1;
	USB->DEVICE.DeviceEndpoint[ep].EPSTATUSSET.reg = USB_DEVICE_EPSTATUS_BK1RDY;
	return 0;
}

inline bool usb_ep_empty(uint8_t ep) {
	if (ep & 0x80) {
		return !(USB->DEVICE.DeviceEndpoint[ep & 0x3F].EPSTATUS.bit.BK1RDY || usb_ep_pending(ep));
	} else {
		return !(USB->DEVICE.DeviceEndpoint[ep & 0x3F].EPSTATUS.bit.BK0RDY || usb_ep_pending(ep));
	}
}

inline bool usb_ep_ready(uint8_t ep) {
	return usb_ep_empty(ep);
}

inline bool usb_ep_pending(uint8_t ep) {
	if (ep & 0x80) {
		return USB->DEVICE.DeviceEndpoint[ep & 0x3F].EPINTFLAG.bit.TRCPT1;
	} else {
		return USB->DEVICE.DeviceEndpoint[ep & 0x3F].EPINTFLAG.bit.TRCPT0;
	}
}

inline void usb_ep_handled(uint8_t ep) {
	if (ep & 0x80) {
		USB->DEVICE.DeviceEndpoint[ep & 0x3F].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT1;
	} else {
		USB->DEVICE.DeviceEndpoint[ep & 0x3F].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT0;
	}
}

inline usb_size usb_ep_out_length(uint8_t ep){
	return usb_endpoints[ep].DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT;
}

inline void usb_detach(void) {
	USB->DEVICE.CTRLB.bit.DETACH = 1;
}

inline void usb_attach(void) {
	USB->DEVICE.CTRLB.bit.DETACH = 0;
}

/// Enable the OUT stage on the default control pipe.
inline void usb_ep0_out(void) {
	usb_ep_start_out(0x00, ep0_buf_out, USB_EP0_SIZE);
}

inline void usb_ep0_in(uint8_t size){
	usb_ep_start_in(0x80, ep0_buf_in, size, true);
}

inline void usb_ep0_stall(void) {
	USB->DEVICE.DeviceEndpoint[0].EPSTATUSSET.reg = USB_DEVICE_EPSTATUS_STALLRQ(0x3);
}

void usb_set_speed(USB_Speed speed) {
	if (USB_SPEED_FULL == speed) {
		USB->DEVICE.CTRLB.bit.SPDCONF = USB_DEVICE_CTRLB_SPDCONF_0_Val;
	} else if(USB_SPEED_LOW == speed) {
		USB->DEVICE.CTRLB.bit.SPDCONF = USB_DEVICE_CTRLB_SPDCONF_1_Val;
	}
}
USB_Speed usb_get_speed() {
	if (USB->DEVICE.STATUS.bit.SPEED == 0) {
		return USB_SPEED_LOW;
	} else {
		return USB_SPEED_FULL;
	}
}

void USB_Handler() {
	uint32_t summary = USB->DEVICE.EPINTSMRY.reg;
	uint32_t status = USB->DEVICE.INTFLAG.reg;

	if (status & USB_DEVICE_INTFLAG_EORST) {
		USB->DEVICE.INTFLAG.reg = USB_DEVICE_INTFLAG_EORST;
		usb_reset();
		usb_cb_reset();
		return;
	}

	if (summary & (1<<0)) {
		uint32_t flags = USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.reg;
		if (flags & USB_DEVICE_EPINTFLAG_RXSTP) {
			memcpy(&usb_setup, ep0_buf_out, sizeof(usb_setup));
			usb_handle_setup();
		}
		if (flags & USB_DEVICE_EPINTFLAG_TRCPT0) {
			usb_handle_control_out_complete();
		}
		if (flags & USB_DEVICE_EPINTFLAG_TRCPT1) {
			usb_handle_control_in_complete();
		}
		USB->DEVICE.DeviceEndpoint[0].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT1 | USB_DEVICE_EPINTFLAG_TRCPT0 | USB_DEVICE_EPINTFLAG_RXSTP;
	}

	for (int i=1; i<usb_num_endpoints; i++) {
		if (summary & 1<<i) {
			uint32_t flags = USB->DEVICE.DeviceEndpoint[i].EPINTFLAG.reg;
			USB->DEVICE.DeviceEndpoint[i].EPINTENCLR.reg = flags;
		}
	}

	usb_cb_completion();
}
