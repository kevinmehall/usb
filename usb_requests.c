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
				USB_ep0_send(2);
				return USB_ep0_enableOut();
			case USB_REQ_ClearFeature:
			case USB_REQ_SetFeature:
				USB_ep0_send(0);
				return USB_ep0_enableOut();
			case USB_REQ_SetAddress:
				USB_ep0_enableOut();
				return USB_ep0_send(0);
			case USB_REQ_GetDescriptor: {
				uint8_t type = (usb_setup.wValue >> 8);
				uint8_t index = (usb_setup.wValue & 0xFF);
				const uint8_t* descriptor = 0;
				uint16_t size = usb_cb_get_descriptor(type, index, &descriptor);

				if (size && descriptor){
					if (size > usb_setup.wLength) {
						size = usb_setup.wLength;
					}
					USB_ep_in_start(0x80, descriptor, size);
					return USB_ep0_enableOut();
				} else {
					return USB_ep0_stall();
				}
			}
			case USB_REQ_GetConfiguration:
				ep0_buf_in[0] = USB_Device_ConfigurationNumber;
				USB_ep0_send(1);
				return USB_ep0_enableOut();
			case USB_REQ_SetConfiguration:
				if ((uint8_t)usb_setup.wValue <= 1) {
					USB_ep0_send(0);
					USB_Device_ConfigurationNumber = (uint8_t)(usb_setup.wValue);
					return USB_ep0_enableOut();
				} else {
					return USB_ep0_stall();
				}
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

