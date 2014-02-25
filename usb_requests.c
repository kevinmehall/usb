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

inline bool USB_handleSetAddress(USB_SetupPacket* req){
	uint8_t    DeviceAddress = (req -> wValue & 0x7F);
	USB_ep0_enableOut();
	USB_ep0_send(0);
	USB_ep_wait(0x80);
	USB.ADDR = DeviceAddress;
	USB_DeviceState = (DeviceAddress) ? DEVICE_STATE_Addressed : DEVICE_STATE_Default;
	return true;
}

inline bool USB_handleGetDescriptor(USB_SetupPacket* req){
	const void* DescriptorPointer;
	uint16_t  DescriptorSize;
	NVM.CMD = NVM_CMD_NO_OPERATION_gc;
	
	if ((DescriptorSize = CALLBACK_USB_GetDescriptor(req->wValue, req->wIndex, &DescriptorPointer))){
		if (DescriptorSize > req->wLength) DescriptorSize=req->wLength;
		USB_ep0_send_progmem(DescriptorPointer, DescriptorSize);
		return true;
	}
	return false;
}

inline bool USB_handleSetConfiguration(USB_SetupPacket* req){
	if ((uint8_t)req->wValue > 1) {
		return false;
	}

	USB_ep0_send(0);
	USB_Device_ConfigurationNumber = (uint8_t)(req->wValue);

	EVENT_USB_Device_ConfigurationChanged(USB_Device_ConfigurationNumber);
	return true;
}

bool USB_HandleSetup(void){
	USB_SetupPacket* req = (void *) ep0_buf_out;
	
	if ((req->bmRequestType & USB_REQTYPE_TYPE_MASK) == USB_REQTYPE_STANDARD){
		switch (req->bRequest){
			case USB_REQ_GetStatus:
				ep0_buf_in[0] = 0;
				ep0_buf_in[1] = 0;
				USB_ep0_send(2);
				return true;
			case USB_REQ_ClearFeature:
			case USB_REQ_SetFeature:
				USB_ep0_send(0);
				return true;
			case USB_REQ_SetAddress:
				return USB_handleSetAddress(req);
			case USB_REQ_GetDescriptor:
				return USB_handleGetDescriptor(req);
			case USB_REQ_GetConfiguration:
				ep0_buf_in[0] = USB_Device_ConfigurationNumber;
				USB_ep0_send(1);
				return true;
			case USB_REQ_SetConfiguration:
				return USB_handleSetConfiguration(req);
			case USB_REQ_SetInterface:
				if (EVENT_USB_Device_SetInterface(req->wIndex, req->wValue)){
					USB_ep0_send(0);
					return true;
				}
			case USB_REQ_GetInterface:
				return false;
		}
	}
	
	return EVENT_USB_Device_ControlRequest(req);
}

