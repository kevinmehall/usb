#include "pipe.h"
#include "usb.h"

inline void usb_pipe_reset(uint8_t ep, const Pipe* pipe){
	pipe_reset(pipe);
	USB_ep_cancel(ep);
	
	if (!(ep & USB_EP_IN)){ // OUT endpoint
		USB_ep_out_start(ep, pipe_write_ptr(pipe));	
	}
}

inline void usb_pipe_handle(uint8_t ep, const Pipe* pipe){
	if (ep & USB_EP_IN){
		if (USB_ep_ready(ep) && pipe_can_read(pipe)){
			if (USB_ep_done(ep)){
				pipe_done_read(pipe);
			}
			USB_ep_in_start(ep, pipe_read_ptr(pipe), pipe->size);
		}
	}else{
		if (USB_ep_ready(ep) && pipe_can_write(pipe)){
			if (USB_ep_done(ep)){
				pipe_done_write(pipe);
			}
			USB_ep_out_start(ep, pipe_write_ptr(pipe));
		}
	}
}

