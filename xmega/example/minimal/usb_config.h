#pragma once
#include "usb_config_defs.h"

#define USB_NUM_EP 0
#define USB_EP0_SIZE 64

const static USB_Device usb_device_config = {
	.cb_reset = NULL,
	.cb_control_setup = NULL,
	.cb_control_in_complete = NULL,
	.cb_control_out_complete = NULL,
};

const static USB_Interface usb_interface_config[] = {
	{
		.cb_control_setup = NULL,
		.cb_control_in_complete = NULL,
		.cb_control_out_complete = NULL,
		.cb_set_interface = NULL,
	},
};

const static USB_Endpoint_Callback usb_in_endpoint_callbacks[] = {
	NULL,
};

const static USB_Endpoint_Callback usb_out_endpoint_callbacks[] = {
	NULL,
};

