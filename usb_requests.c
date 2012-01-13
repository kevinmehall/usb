// Minimal USB Stack for ATxmega32a4u and related
// http://nonolithlabs.com
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
	USB_ep0_send(0);
	while (!(endpoints[0].in.STATUS & USB_EP_TRNCOMPL0_bm)); // wait for status stage to complete
	USB.ADDR = DeviceAddress;
	USB_DeviceState = (DeviceAddress) ? DEVICE_STATE_Addressed : DEVICE_STATE_Default;
	return true;
}

#if !defined(NO_INTERNAL_SERIAL) && (USE_INTERNAL_SERIAL != NO_DESCRIPTOR)
inline void USB_Device_GetSerialString(uint16_t* const UnicodeString) ATTR_NON_NULL_PTR_ARG(1);
inline void USB_Device_GetSerialString(uint16_t* const UnicodeString){
	//uint_reg_t CurrentGlobalInt = GetGlobalInterruptMask();
	//GlobalInterruptDisable();
	
	uint8_t SigReadAddress = INTERNAL_SERIAL_START_ADDRESS;

	for (uint8_t SerialCharNum = 0; SerialCharNum < (INTERNAL_SERIAL_LENGTH_BITS / 4); SerialCharNum++)
	{					
		uint8_t SerialByte;

		NVM.CMD    = NVM_CMD_READ_CALIB_ROW_gc;
		SerialByte = pgm_read_byte(SigReadAddress);

		if (SerialCharNum & 0x01)
		{
			SerialByte >>= 4;
			SigReadAddress++;
		}

		SerialByte &= 0x0F;

		UnicodeString[SerialCharNum] = ((SerialByte >= 10) ?
									   (('A' - 10) + SerialByte) : ('0' + SerialByte));
	}
	
	//SetGlobalInterruptMask(CurrentGlobalInt);
}

inline void USB_Device_GetInternalSerialDescriptor(void)
{
	struct
	{
		USB_Descriptor_Header_t Header;
		uint16_t                UnicodeString[INTERNAL_SERIAL_LENGTH_BITS / 4];
	}* SignatureDescriptor = (void*) ep0_buf_in;

	SignatureDescriptor->Header.Type = DTYPE_String;
	SignatureDescriptor->Header.Size = USB_STRING_LEN(INTERNAL_SERIAL_LENGTH_BITS / 4);
	
	USB_Device_GetSerialString(SignatureDescriptor->UnicodeString);

	USB_ep0_send(sizeof(*SignatureDescriptor));
}
#endif

inline bool USB_handleGetDescriptor(USB_Request_Header_t* req){
	const void* DescriptorPointer;
	uint16_t  DescriptorSize;
	NVM.CMD = NVM_CMD_NO_OPERATION_gc;
	
	#if !defined(NO_INTERNAL_SERIAL) && (USE_INTERNAL_SERIAL != NO_DESCRIPTOR)
	if (req->wValue == ((DTYPE_String << 8) | USE_INTERNAL_SERIAL)){
		USB_Device_GetInternalSerialDescriptor();
		return true;
	}
	#endif
	
	if ((DescriptorSize = CALLBACK_USB_GetDescriptor(req->wValue, req->wIndex, &DescriptorPointer))){
		if (DescriptorSize > req->wLength) DescriptorSize=req->wLength;
		USB_ep0_send_progmem(DescriptorPointer, DescriptorSize);
		return true;
	}
	return false;
}

inline bool USB_handleSetConfiguration(USB_Request_Header_t* req){
	USB_ep0_send(0);
	USB_Device_ConfigurationNumber = (uint8_t)(req -> wValue);

	if (USB_Device_ConfigurationNumber)
	  USB_DeviceState = DEVICE_STATE_Configured;
	else
	  USB_DeviceState = (USB.ADDR) ? DEVICE_STATE_Configured : DEVICE_STATE_Powered;

	EVENT_USB_Device_ConfigurationChanged();
	return true;
}

bool USB_HandleSetup(void){
	USB_Request_Header_t* req = (void *) ep0_buf_out;
	
	if ((req->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_STANDARD){
		switch (req->bRequest){
			case REQ_GetStatus:
				ep0_buf_in[0] = 0;
				ep0_buf_in[1] = 0;
				USB_ep0_send(2);
				return true;
			case REQ_ClearFeature:
			case REQ_SetFeature:
				USB_ep0_send(0);
				return true;
			case REQ_SetAddress:
				return USB_handleSetAddress(req);
			case REQ_GetDescriptor:
				return USB_handleGetDescriptor(req);
			case REQ_GetConfiguration:
				ep0_buf_in[0] = USB_Device_ConfigurationNumber;
				USB_ep0_send(1);
				return true;
			case REQ_SetConfiguration:
				return USB_handleSetConfiguration(req);
		}
	}
	
	return EVENT_USB_Device_ControlRequest(req);
}

