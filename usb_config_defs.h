#pragma once
#include <string.h>
#include <stdbool.h>

typedef size_t usb_size;
typedef uint8_t usb_ep;
typedef uint8_t usb_bank;

typedef struct {
	usb_size ep0_size;
	void (*cb_get_descriptor)(uint16_t wValue, uint16_t wIndex, uint8_t **descriptor);
	void (*cb_reset)(void);
	void (*cb_control_setup)(void);
	void (*cb_control_in_complete)(void);
	void (*cb_control_out_complete)(void);
} USB_Device;

typedef struct {
	void (*cb_control_setup)(void);
	void (*cb_control_in_complete)(void);
	void (*cb_control_out_complete)(void);
	bool (*cb_set_interface)(uint16_t altsetting);
} USB_Interface;

typedef void (*USB_Endpoint_Callback)(void);

#define USB_EP0_SIZE 64

extern const USB_Device usb_device_config;

extern const uint8_t usb_num_interfaces;
extern const USB_Interface usb_interface_config[];

extern const uint8_t usb_num_endpoints;
extern const USB_Endpoint_Callback usb_in_endpoint_callbacks[];
extern const USB_Endpoint_Callback usb_out_endpoint_callbacks[];
