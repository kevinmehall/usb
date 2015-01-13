#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "usb_standard.h"

#define USB_EP0_SIZE 64

extern USB_SetupPacket usb_setup;
extern uint8_t ep0_buf_in[USB_EP0_SIZE];
extern uint8_t ep0_buf_out[USB_EP0_SIZE];
extern volatile uint8_t USB_DeviceState;
extern volatile uint8_t USB_Device_ConfigurationNumber;

typedef size_t usb_size;
typedef uint8_t usb_ep;
typedef uint8_t usb_bank;

/// Callback on reset
void usb_cb_reset(void);

/// Callback when a setup packet is received
void usb_cb_control_setup(void);

/// Callback on a completion interrupt
void usb_cb_completion(void);
void usb_cb_control_in_completion(void);
void usb_cb_control_out_completion(void);

/// Callback for a SET_CONFIGURATION request
bool usb_cb_set_configuration(uint8_t config);

/// Callback for a SET_INTERFACE request
bool usb_cb_set_interface(uint16_t interface, uint16_t altsetting);

/// Callbck for a GET_DESCRIPTOR request
uint16_t usb_cb_get_descriptor(uint8_t type, uint8_t index, const uint8_t** descriptor_ptr);

/// Initialize the USB controller
void usb_init(void);

/// Configure pull resistor to be detected by the host
void usb_attach(void);

/// Disconnect from the host
void usb_detach(void);

/// Called internally on USB reset
void usb_reset(void);

/// Called internally to configure the USB controller with the new address
void usb_set_address(uint8_t addr);

/// Configure and enable an endpoint
void usb_enable_ep(usb_ep ep, uint8_t type, usb_size pkt_size);

/// Disable an endpoint
void usb_disable_ep(usb_ep ep);

/// Reset an endpoint, clearing pending transfers
void usb_reset_ep(usb_ep ep);

/// Set or clear stall on an endpoint
void usb_set_stall_ep(usb_ep ep);
void usb_clr_stall_ep(usb_ep ep);

/// Returns true if an endpoint can start or queue a transfer
bool usb_ep_ready(usb_ep ep);

inline void usb_ep_wait_for_ready(uint8_t ep){
	while (!usb_ep_ready(ep)) {};
}

/// Returns true if there is a completion pending on the endpoint
bool usb_ep_pending(usb_ep ep);

inline void usb_ep_wait_for_pending(uint8_t ep){
	while (!usb_ep_pending(ep)) {};
}

/// Start an asynchronous host->device transfer.
/// The data will be received into data up to size len. This buffer must remain valid until
/// the callback is called
usb_bank usb_ep_start_out(usb_ep ep, uint8_t* data, usb_size len);

/// Gets the length of a pending completion on an OUT endpoint
usb_size usb_ep_out_length(usb_ep ep);

/// Start an asynchronous device->host transfer.
/// The data will be sent from the data buffer. This buffer must remain valid until the
/// the callback is called. If zlp is set and the data is not a multiple of the packet
/// size, an extra zero-length packet will be sent to terminate the transfer.
usb_bank usb_ep_start_in(uint8_t ep, const uint8_t* data, usb_size size, bool zlp);

/// Clear a completion on an endpoint
void usb_ep_handled(usb_ep ep);

/// Send `size` bytes from ep0_buf_in on endpoint 0
void usb_ep0_in(uint8_t size);

/// Accept a packet into ep0_buf_out on endpoint 0
void usb_ep0_out(void);

/// Stall endpoint 0
void usb_ep0_stall(void);

/// Force the maximum speed. Call before usb_attach()
void usb_set_speed(USB_Speed speed);

/// Gets the currently-connected speed
USB_Speed usb_get_speed(void);

/// Handle a vendor request for a Microsoft WCID compatible descriptor.
/// bmRequestType is vendor/device, and bRequest is user-defined in the string descriptor, so
/// the callback cb_control_setup must dispatch the request to this function.
void usb_handle_msft_compatible(const USB_MicrosoftCompatibleDescriptor* msft_compatible);

/// Convert a C string to a string descriptor in the ep0 IN buffer
void* usb_string_to_descriptor(char* str);

/// Internal common methods called by the hardware API
void usb_handle_setup(void);
void usb_handle_control_out_complete(void);
void usb_handle_control_in_complete(void);
