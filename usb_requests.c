// Minimal USB Stack for ATxmega32a4u and related
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Heavily borrows from LUFA
// Copyright 2011  Dean Camera (dean [at] fourwalledcubicle [dot] com)
//
// Licensed under the terms of the GNU GPLv3+

#include <avr/io.h>
#include "usb.h"

USB_SetupPacket usb_setup;

inline void USB_handleGetDescriptor(void){
	const void* DescriptorPointer;
	uint16_t  DescriptorSize;
	NVM.CMD = NVM_CMD_NO_OPERATION_gc;
	
	if ((DescriptorSize = CALLBACK_USB_GetDescriptor(usb_setup.wValue, usb_setup.wIndex, &DescriptorPointer))){
		if (DescriptorSize > usb_setup.wLength) DescriptorSize=usb_setup.wLength;
		USB_ep0_send_progmem(DescriptorPointer, DescriptorSize);
		return USB_ep0_enableOut();
	} else {
		return USB_ep0_stall();
	}
}

inline void USB_handleSetConfiguration(void){
	if ((uint8_t)usb_setup.wValue > 1) {
		return USB_ep0_stall();
	}

	USB_ep0_send(0);
	USB_Device_ConfigurationNumber = (uint8_t)(usb_setup.wValue);

	//EVENT_USB_Device_ConfigurationChanged(USB_Device_ConfigurationNumber);
	return USB_ep0_enableOut();
}

void usb_handle_setup(void){
	if ((usb_setup.bmRequestType & USB_REQTYPE_TYPE_MASK) == USB_REQTYPE_STANDARD){
		switch (usb_setup.bRequest){
			case USB_REQ_GetStatus:
				ep0_buf_in[0] = 0;
				ep0_buf_in[1] = 0;
				USB_ep0_send(2);
				return USB_ep0_enableOut();
			case USB_REQ_ClearFeature:
			case USB_REQ_SetFeature:
				USB_ep0_send(0);
				return USB_ep0_enableOut();
			case USB_REQ_SetAddress:
				USB_ep0_enableOut();
				return USB_ep0_send(0);
			case USB_REQ_GetDescriptor:
				return USB_handleGetDescriptor();
			case USB_REQ_GetConfiguration:
				ep0_buf_in[0] = USB_Device_ConfigurationNumber;
				USB_ep0_send(1);
				return USB_ep0_enableOut();
			case USB_REQ_SetConfiguration:
				return USB_handleSetConfiguration();
			case USB_REQ_SetInterface:
				if (usb_setup.wIndex < ARR_LEN(usb_interface_config)
					&& usb_interface_config[usb_setup.wIndex].cb_set_interface(usb_setup.wValue)){
					USB_ep0_send(0);
					return USB_ep0_enableOut();
				} else {
					return USB_ep0_stall();
				}
			default:
				return USB_ep0_enableOut();
		}
	}
	
	if (usb_device_config.cb_control_setup) {
		return usb_device_config.cb_control_setup();
	} else {
		return USB_ep0_stall();
	}
}

void usb_handle_control_out_complete(void) {

}


void usb_handle_control_in_complete(void) {
	if ((usb_setup.bmRequestType & USB_REQTYPE_TYPE_MASK) == USB_REQTYPE_STANDARD){
		switch (usb_setup.bRequest){
			case USB_REQ_SetAddress:
				USB.ADDR = (usb_setup.wValue & 0x7F);
				return;
		}
	}
}

