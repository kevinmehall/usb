// Minimal USB Stack for ATxmega32a4u and related
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Heavily borrows from LUFA
// Copyright 2011  Dean Camera (dean [at] fourwalledcubicle [dot] com)
//
// Licensed under the terms of the GNU GPLv3+

#include "usb.h"

inline bool USB_handleSetAddress(USB_Request_Header_t* req){
	uint8_t    DeviceAddress = (req -> wValue & 0x7F);
	endpoints[0].out.STATUS &= ~(USB_EP_SETUP_bm | USB_EP_BUSNACK0_bm);
	USB_ep_send_packet(0, 0);
	while (!(endpoints[0].in.STATUS & USB_EP_TRNCOMPL0_bm)); // wait for status stage to complete
	USB.ADDR = DeviceAddress;
	USB_DeviceState = (DeviceAddress) ? DEVICE_STATE_Addressed : DEVICE_STATE_Default;
	//timer = 15625/10;
	return true;
}

inline bool USB_handleGetDescriptor(USB_Request_Header_t* req){
	const void* DescriptorPointer;
	uint16_t  DescriptorSize;
	NVM.CMD = NVM_CMD_NO_OPERATION_gc;
	if ((DescriptorSize = CALLBACK_USB_GetDescriptor(req->wValue, req->wIndex, &DescriptorPointer))){
		if (DescriptorSize > req->wLength) DescriptorSize=req->wLength;
		USB_sendFromFlash(0, DescriptorPointer, DescriptorSize);
		return true;
	}
	return false;
}

bool USB_HandleSetup(void){
	endpoints[0].out.CTRL |= USB_EP_TOGGLE_bm;
	endpoints[0].in.CTRL |= USB_EP_TOGGLE_bm;
	USB_Request_Header_t* req = (void *) ep0_buf_out;
	
	switch (req->bRequest){
		case REQ_GetStatus:
			ep0_buf_in[0] = 0;
			ep0_buf_in[1] = 0;
			USB_ep_send_packet(0, 2);
			return true;
		case REQ_ClearFeature:
		case REQ_SetFeature:
			USB_ep_send_packet(0, 0);
			return true;
		case REQ_SetAddress:
			return USB_handleSetAddress(req);
		case REQ_GetDescriptor:
			return USB_handleGetDescriptor(req);
		case REQ_GetConfiguration:
			ep0_buf_in[0] = USB_Device_ConfigurationNumber;
			USB_ep_send_packet(0, 1);
			return true;
		case REQ_SetConfiguration:
			USB_ep_send_packet(0, 0);
			USB_Device_ConfigurationNumber = (uint8_t)(req -> wValue);

			if (USB_Device_ConfigurationNumber)
			  USB_DeviceState = DEVICE_STATE_Configured;
			else
			  USB_DeviceState = (USB.ADDR) ? DEVICE_STATE_Configured : DEVICE_STATE_Powered;

			EVENT_USB_Device_ConfigurationChanged();
			return true;
	}
	
	return false;
}

