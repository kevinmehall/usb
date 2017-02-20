#include "usb.h"

USB_SetupPacket usb_setup;
__attribute__((__aligned__(4))) uint8_t ep0_buf_in[USB_EP0_SIZE];
__attribute__((__aligned__(4))) uint8_t ep0_buf_out[USB_EP0_SIZE];
volatile uint8_t usb_configuration;

uint16_t usb_ep0_in_size;
const uint8_t* usb_ep0_in_ptr;

void usb_ep0_in_multi(void) {
	uint16_t tsize = usb_ep0_in_size;

	if (tsize > USB_EP0_SIZE) {
		tsize = USB_EP0_SIZE;
	}

	memcpy(ep0_buf_in, usb_ep0_in_ptr, tsize);
	usb_ep_start_in(0x80, ep0_buf_in, tsize, false);

	if (tsize == 0) {
		usb_ep0_out();
	}

	usb_ep0_in_size -= tsize;
	usb_ep0_in_ptr += tsize;
}

void usb_handle_setup(void){
	if ((usb_setup.bmRequestType & USB_REQTYPE_TYPE_MASK) == USB_REQTYPE_STANDARD){
		switch (usb_setup.bRequest){
			case USB_REQ_GetStatus:
				ep0_buf_in[0] = 0;
				ep0_buf_in[1] = 0;
				usb_ep0_in(2);
				return usb_ep0_out();

			case USB_REQ_ClearFeature:
			case USB_REQ_SetFeature:
				usb_ep0_in(0);
				return usb_ep0_out();

			case USB_REQ_SetAddress:
				usb_ep0_in(0);
				return usb_ep0_out();

			case USB_REQ_GetDescriptor: {
				uint8_t type = (usb_setup.wValue >> 8);
				uint8_t index = (usb_setup.wValue & 0xFF);
				const uint8_t* descriptor = 0;
				uint16_t size = usb_cb_get_descriptor(type, index, &descriptor);

				if (size && descriptor){
					if (size > usb_setup.wLength) {
						size = usb_setup.wLength;
					}

					if (descriptor == ep0_buf_in) {
						usb_ep0_in_size = 0;
						usb_ep_start_in(0x80, ep0_buf_in, size, true);
					} else {
						usb_ep0_in_size = size;
						usb_ep0_in_ptr = descriptor;
						usb_ep0_in_multi();
					}

					return;
				} else {
					return usb_ep0_stall();
				}
			}
			case USB_REQ_GetConfiguration:
				ep0_buf_in[0] = usb_configuration;
				usb_ep0_in(1);
				return usb_ep0_out();

			case USB_REQ_SetConfiguration:
				if (usb_cb_set_configuration((uint8_t)usb_setup.wValue)) {
					usb_ep0_in(0);
					usb_configuration = (uint8_t)(usb_setup.wValue);
					return usb_ep0_out();
				} else {
					return usb_ep0_stall();
				}

			case USB_REQ_SetInterface:
				if (usb_cb_set_interface(usb_setup.wIndex, usb_setup.wValue)) {
					usb_ep0_in(0);
					return usb_ep0_out();
				} else {
					return usb_ep0_stall();
				}

			default:
				return usb_ep0_stall();
		}
	}

	usb_cb_control_setup();
}

void usb_handle_control_out_complete(void) {
	if ((usb_setup.bmRequestType & USB_REQTYPE_TYPE_MASK) == USB_REQTYPE_STANDARD) {
		// Let the status stage proceed
	} else {
		usb_cb_control_out_completion();
	}
}

void usb_handle_control_in_complete(void) {
	if ((usb_setup.bmRequestType & USB_REQTYPE_TYPE_MASK) == USB_REQTYPE_STANDARD) {
		switch (usb_setup.bRequest){
			case USB_REQ_SetAddress:
				usb_set_address(usb_setup.wValue & 0x7F);
				return;
			case USB_REQ_GetDescriptor:
				usb_ep0_in_multi();
				return;
		}
	} else {
		usb_cb_control_in_completion();
	}
}

void usb_handle_msft_compatible(const USB_MicrosoftCompatibleDescriptor* msft_compatible) {
	if (usb_setup.wIndex == 0x0004) {
		uint16_t len = usb_setup.wLength;
		if (len > msft_compatible->dwLength) {
			len = msft_compatible->dwLength;
		}
		if (len > USB_EP0_SIZE) {
			len = USB_EP0_SIZE;
		}
		memcpy(ep0_buf_in, msft_compatible, len);
		usb_ep_start_in(0x80, ep0_buf_in, len, false);
		return usb_ep0_out();
	} else {
		return usb_ep0_stall();
	}
}

void* usb_string_to_descriptor(char* str) {
	USB_StringDescriptor* desc = (((USB_StringDescriptor*)ep0_buf_in));
	uint16_t len = strlen(str);
	const uint16_t maxlen = (USB_EP0_SIZE - 2)/2;
	if (len > maxlen) len = maxlen;
	desc->bLength = USB_STRING_LEN(len);
	desc->bDescriptorType = USB_DTYPE_String;
	for (int i=0; i<len; i++) {
		desc->bString[i] = str[i];
	}
	return desc;
}
