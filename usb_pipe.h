#include "pipe.h"
#include "usb.h"

inline void usb_pipe_reset(uint8_t ep, const Pipe* pipe){
	pipe_reset(pipe);
	USB_ep_cancel(ep);
	
	if (ep & USB_EP_IN){
		pipe->data->count += 2;
	}else{ // OUT endpoint
		pipe->data->count -= 2;
	}
}

inline void usb_pipe_handle(uint8_t ep, const Pipe* pipe){
	if (ep & USB_EP_IN){
		if (USB_ep_ready(ep) && pipe_can_read(pipe) > 2){
			const uint8_t bank = (ep&USB_EP_PP)?pipe->data->read_pos&1:0;
			USB_ep_start_bank(ep, bank, pipe_read_ptr(pipe), pipe->size);

			cli();
			pipe_done_read(pipe);
			sei();
		}
	}else{
		if (USB_ep_ready(ep) && pipe_can_write(pipe) > 2){
			const uint8_t bank = (ep&USB_EP_PP)?pipe->data->write_pos&1:0;
			USB_ep_start_bank(ep, bank, pipe_write_ptr(pipe), pipe->size);

			cli();
			pipe_done_write(pipe);
			sei();
		}
	}
}

