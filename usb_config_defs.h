#pragma once
#include <string.h>
#include <stdbool.h>

typedef size_t usb_size;

typedef struct {
	usb_size ep0_size;
	void (*cb_get_descriptor)(uint16_t wValue, uint16_t wIndex, uint8_t **descriptor);
	void (*cb_reset)(void);
	void (*cb_control_setup)(void);
	void (*cb_control_in)(void);
	void (*cb_control_out)(void);
} USB_Device;

typedef struct {
	void (*cb_control_setup)(void);
	void (*cb_control_in)(void);
	void (*cb_control_out)(void);
	bool (*cb_set_interface)(uint16_t altsetting);
} USB_Interface;

typedef void (*USB_Endpoint_Callback)(void);
