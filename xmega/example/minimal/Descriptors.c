#include <avr/pgmspace.h>
#include "usb_standard.h"

const USB_DeviceDescriptor PROGMEM device_descriptor = {
	.bLength = sizeof(USB_DeviceDescriptor),
	.bDescriptorType = USB_DTYPE_Device,

	.bcdUSB                 = 0x0200,
	.bDeviceClass           = USB_CSCP_VendorSpecificClass,
	.bDeviceSubClass        = USB_CSCP_NoDeviceSubclass,
	.bDeviceProtocol        = USB_CSCP_NoDeviceProtocol,

	.bMaxPacketSize0        = 64,
	.idVendor               = 0x9999,
	.idProduct              = 0xFFFF,
	.bcdDevice              = 0x0101,

	.iManufacturer          = 0x01,
	.iProduct               = 0x02,
	.iSerialNumber          = 0,

	.bNumConfigurations     = 1
};

typedef struct ConfigDesc {
	USB_ConfigurationDescriptor Config;
	USB_InterfaceDescriptor Interface0;
	USB_EndpointDescriptor DataInEndpoint;
	USB_EndpointDescriptor DataOutEndpoint;

} ConfigDesc;

const ConfigDesc PROGMEM configuration_descriptor = {
	.Config = {
		.bLength = sizeof(USB_ConfigurationDescriptor),
		.bDescriptorType = USB_DTYPE_Configuration,
		.wTotalLength  = sizeof(ConfigDesc),
		.bNumInterfaces = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIG_ATTR_BUSPOWERED,
		.bMaxPower = USB_CONFIG_POWER_MA(500)
	},
	.Interface0 = {
		.bLength = sizeof(USB_InterfaceDescriptor),
		.bDescriptorType = USB_DTYPE_Interface,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = USB_CSCP_VendorSpecificClass,
		.bInterfaceSubClass = 0x00,
		.bInterfaceProtocol = 0x00,
		.iInterface = 0
	},
	.DataInEndpoint = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = 0x81,
		.bmAttributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 64,
		.bInterval = 0x00
	},
	.DataOutEndpoint = {
		.bLength = sizeof(USB_EndpointDescriptor),
		.bDescriptorType = USB_DTYPE_Endpoint,
		.bEndpointAddress = 0x2,
		.bmAttributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.wMaxPacketSize = 64,
		.bInterval = 0x00
	},
};

const USB_StringDescriptor PROGMEM language_string = {
	.bLength = USB_STRING_LEN(1),
	.bDescriptorType = USB_DTYPE_String,
	.bString = {USB_LANGUAGE_EN_US},
};

const USB_StringDescriptor PROGMEM manufacturer_string = {
	.bLength = USB_STRING_LEN(13),
	.bDescriptorType = USB_DTYPE_String,
	.bString = u"Nonolith Labs"
};

const USB_StringDescriptor PROGMEM product_string = {
	.bLength = USB_STRING_LEN(14),
	.bDescriptorType = USB_DTYPE_String,
	.bString = u"Example Device"
};


uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint8_t wIndex,
                                    const void** const DescriptorAddress)
{
	const uint8_t  DescriptorType   = (wValue >> 8);
	const uint8_t  DescriptorNumber = (wValue & 0xFF);

	const void* Address = NULL;
	uint16_t    Size    = 0;

	switch (DescriptorType)
	{
		case USB_DTYPE_Device:
			Address = &device_descriptor;
			Size    = sizeof(USB_DeviceDescriptor);
			break;
		case USB_DTYPE_Configuration:
			Address = &configuration_descriptor;
			Size    = sizeof(ConfigDesc);
			break;
		case USB_DTYPE_String:
			switch (DescriptorNumber) {
				case 0x00:
					Address = &language_string;
					Size    = pgm_read_byte(&language_string.bLength);
					break;
				case 0x01:
					Address = &manufacturer_string;
					Size    = pgm_read_byte(&manufacturer_string.bLength);
					break;
				case 0x02:
					Address = &product_string;
					Size    = pgm_read_byte(&product_string.bLength);
					break;
			}

			break;
	}

	*DescriptorAddress = Address;
	return Size;
}

