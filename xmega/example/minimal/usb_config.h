#pragma once
#include "usb_config_defs.h"

const static USB_Device usb_device_config = {
	.ep0_size = 64,
	.cb_reset = NULL,
	.cb_control_setup = NULL,
	.cb_control_in = NULL,
	.cb_control_out = NULL,
};

const static USB_Interface usb_interface_config[] = {
	{
		.cb_control_setup = NULL,
		.cb_control_in = NULL,
		.cb_control_out = NULL,
		.cb_set_interface = NULL,
	},
};

const static USB_Endpoint_Callback usb_in_endpoint_callbacks[] = {
	NULL,
};

const static USB_Endpoint_Callback usb_out_endpoint_callbacks[] = {
	NULL,
};

