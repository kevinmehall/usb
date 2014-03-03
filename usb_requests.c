#include "usb.h"

USB_SetupPacket usb_setup;
uint8_t ep0_buf_in[USB_EP0_SIZE];
uint8_t ep0_buf_out[USB_EP0_SIZE];
volatile uint8_t USB_DeviceState;
volatile uint8_t USB_Device_ConfigurationNumber;

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
					usb_ep_start_in(0x80, descriptor, size, true);
					return usb_ep0_out();
				} else {
					return usb_ep0_stall();
				}
			}
			case USB_REQ_GetConfiguration:
				ep0_buf_in[0] = USB_Device_ConfigurationNumber;
				usb_ep0_in(1);
				return usb_ep0_out();

			case USB_REQ_SetConfiguration:
				if ((uint8_t)usb_setup.wValue <= 1) {
					usb_ep0_in(0);
					USB_Device_ConfigurationNumber = (uint8_t)(usb_setup.wValue);
					return usb_ep0_out();
				} else {
					return usb_ep0_stall();
				}

			case USB_REQ_SetInterface:
				if (usb_setup.wIndex < usb_num_interfaces
					&& usb_interface_config[usb_setup.wIndex].cb_set_interface
					&& usb_interface_config[usb_setup.wIndex].cb_set_interface(usb_setup.wValue)){
					usb_ep0_in(0);
					return usb_ep0_out();
				} else {
					return usb_ep0_stall();
				}

			default:
				return usb_ep0_stall();
		}
	}

	if ((usb_setup.bmRequestType & USB_REQTYPE_RECIPIENT_MASK) == USB_RECIPIENT_INTERFACE) {
		if (usb_setup.wIndex < usb_num_interfaces
			&& usb_interface_config[usb_setup.wIndex].cb_control_setup) {
			usb_interface_config[usb_setup.wIndex].cb_control_setup();
		} else {
			return usb_ep0_stall();
		}
	} else if (usb_device_config.cb_control_setup) {
		return usb_device_config.cb_control_setup();
	} else {
		return usb_ep0_stall();
	}
}

void usb_handle_control_out_complete(void) {
	if ((usb_setup.bmRequestType & USB_REQTYPE_TYPE_MASK) == USB_REQTYPE_STANDARD) {
		// Let the status stage proceed
	} else if ((usb_setup.bmRequestType & USB_REQTYPE_RECIPIENT_MASK) == USB_RECIPIENT_INTERFACE) {
		if (usb_setup.wIndex < usb_num_interfaces
			&& usb_interface_config[usb_setup.wIndex].cb_control_out_complete) {
			usb_interface_config[usb_setup.wIndex].cb_control_out_complete();
		} else {
			return usb_ep0_stall();
		}
	} else if (usb_device_config.cb_control_out_complete) {
		usb_device_config.cb_control_out_complete();
	}
}

void usb_handle_control_in_complete(void) {
	if ((usb_setup.bmRequestType & USB_REQTYPE_TYPE_MASK) == USB_REQTYPE_STANDARD) {
		switch (usb_setup.bRequest){
			case USB_REQ_SetAddress:
				usb_set_address(usb_setup.wValue & 0x7F);
				return;
		}
	} else if ((usb_setup.bmRequestType & USB_REQTYPE_RECIPIENT_MASK) == USB_RECIPIENT_INTERFACE) {
		if (usb_setup.wIndex < usb_num_interfaces
			&& usb_interface_config[usb_setup.wIndex].cb_control_in_complete) {
			usb_interface_config[usb_setup.wIndex].cb_control_in_complete();
		} else {
			return usb_ep0_stall();
		}
	} else if (usb_device_config.cb_control_in_complete) {
		usb_device_config.cb_control_in_complete();
	}
}

void usb_handle_msft_compatible(const USB_MicrosoftCompatibleDescriptor* msft_compatible) {
	if (usb_setup.wIndex == 0x0004) {
		uint16_t len = usb_setup.wLength;
		if (len > msft_compatible->dwLength) {
			len = msft_compatible->dwLength;
		}
		usb_ep_start_in(0x80, (uint8_t*) msft_compatible, len, false);
		return usb_ep0_out();
	} else {
		return usb_ep0_stall();
	}
}

