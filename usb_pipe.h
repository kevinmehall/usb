#include "pipe.h"
#include "usb.h"

#include <util/atomic.h>
#ifndef PIPE_ATOMIC
#define PIPE_ATOMIC ATOMIC_RESTORESTATE
#endif 

typedef struct USB_Pipe_data{
	bool toggle;
	uint8_t flush;
	uint16_t packet_offset; // Index into current packet for byte mode
} USB_Pipe_data;

// Immutable part, constant-folded at compile time
typedef struct USB_Pipe{
	uint8_t ep;
	uint8_t type;
	USB_Pipe_data* data;
	const Pipe* pipe;
	uint8_t features;
} USB_Pipe;

#define PIPE_ENABLE_FLUSH (1<<0)

#define USB_PIPE(NAME, EPNO, TYPE, PACKET_SIZE, BUFFER_PACKETS, FEATURES) \
	PIPE(NAME##_pipe, (BUFFER_PACKETS), (PACKET_SIZE),               \
		((EPNO)&USB_EP_IN)?0:2,                      \
		((EPNO)&USB_EP_IN)?2:0                       \
	);                                               \
	USB_Pipe_data NAME##_data = {      \
		.toggle = 0,                   \
		.flush = 0,                    \
	};                                 \
	const static USB_Pipe NAME = {     \
		.ep = (EPNO),                  \
		.type = (TYPE),                \
		.data = &(NAME##_data),        \
		.pipe = &(NAME##_pipe),        \
		.features = (FEATURES),        \
	};                                 \

static inline void usb_pipe_init(const USB_Pipe* p){
	ATOMIC_BLOCK(PIPE_ATOMIC){
		pipe_reset(p->pipe);
		USB_ep_init(p->ep, p->type, p->pipe->size);
		p->data->toggle = 0;
		p->data->flush = 0;
		p->data->packet_offset = 0;
	}
}

static inline void usb_pipe_reset(const USB_Pipe* p){
	ATOMIC_BLOCK(PIPE_ATOMIC){
		pipe_reset(p->pipe);
		USB_ep_cancel(p->ep);
		p->data->flush = 0;
		p->data->packet_offset = 0;
	}
}

static inline void _usb_pipe_ep_start(const USB_Pipe* p, uint8_t* data, uint16_t size) ATTR_ALWAYS_INLINE;
static inline void _usb_pipe_ep_start(const USB_Pipe* p, uint8_t* data, uint16_t size){
	bool bank = 0;

	if (p->ep & USB_EP_PP){
		bank = p->data->toggle;
		p->data->toggle ^= 1;
	}

	if (bank == 0){ // Helps the optimzer constant-fold, since the multiply is expensive
		USB_ep_start_bank(p->ep, 0, data, size);
	}else{
		USB_ep_start_bank(p->ep, 1, data, size);
	}
}

static inline void usb_pipe_handle(const USB_Pipe* p){
	ATOMIC_BLOCK(PIPE_ATOMIC){
		if (p->ep & USB_EP_IN){
			if (USB_ep_ready(p->ep)){
				if (pipe_can_read(p->pipe) >= 1){
					_usb_pipe_ep_start(p, pipe_read_ptr(p->pipe), p->pipe->size);
					pipe_done_read(p->pipe);
				}else if (p->features & PIPE_ENABLE_FLUSH){
					if (p->data->flush == 1){
						p->data->flush = 2;
						// Send short packet
						_usb_pipe_ep_start(p, pipe_read_ptr(p->pipe), p->data->packet_offset);
					}else if (p->data->flush == 2 && USB_ep_empty(p->ep)){
						p->data->flush = 0;
						p->data->packet_offset = 0;
						pipe_reset(p->pipe);
					}
				}
			}
		}else{
			if (USB_ep_ready(p->ep) && pipe_can_write(p->pipe) >= 1){
				_usb_pipe_ep_start(p, pipe_write_ptr(p->pipe), p->pipe->size);
				pipe_done_write(p->pipe);
			}
		}
	}
}

static inline void usb_pipe_flush(const USB_Pipe* p) ATTR_ALWAYS_INLINE;
static inline void usb_pipe_flush(const USB_Pipe* p){
	GCC_ASSERT(p->features&PIPE_ENABLE_FLUSH && p->ep&USB_EP_IN);
	p->data->flush = 1;
	usb_pipe_handle(p);
}

static inline bool usb_pipe_flush_done(const USB_Pipe* p) ATTR_ALWAYS_INLINE;
static inline bool usb_pipe_flush_done(const USB_Pipe* p){
	GCC_ASSERT(p->features&PIPE_ENABLE_FLUSH);
	return p->data->flush;
}

static inline bool usb_pipe_can_write(const USB_Pipe* p) ATTR_ALWAYS_INLINE;
static inline bool usb_pipe_can_write(const USB_Pipe* p){
	bool r;
	ATOMIC_BLOCK(PIPE_ATOMIC){
		if (p->features&PIPE_ENABLE_FLUSH && p->data->flush){
			r = false;
		}else{
			r = pipe_can_write(p->pipe) > 0;
		}
	}
	return r;
}

static inline uint8_t* usb_pipe_write_ptr(const USB_Pipe* p){
	return pipe_write_ptr(p->pipe);
}

static inline void usb_pipe_done_write(const USB_Pipe* p){
	ATOMIC_BLOCK(PIPE_ATOMIC){
		pipe_done_write(p->pipe);
	}
}

static inline bool usb_pipe_can_read(const USB_Pipe* p) ATTR_ALWAYS_INLINE;
static inline bool usb_pipe_can_read(const USB_Pipe* p){
	bool r;
	ATOMIC_BLOCK(PIPE_ATOMIC){
		r = pipe_can_read(p->pipe) > 0;
	}
	return r;
}

static inline uint8_t* usb_pipe_read_ptr(const USB_Pipe* p){
	return pipe_read_ptr(p->pipe);
}

static inline void usb_pipe_done_read(const USB_Pipe* p){
	ATOMIC_BLOCK(PIPE_ATOMIC){
		pipe_done_read(p->pipe);
	}
}

static inline uint8_t usb_pipe_read_byte(const USB_Pipe* p){
	uint8_t r = p->pipe->data->read_ptr[p->data->packet_offset++];
	if (p->data->packet_offset == p->pipe->size){
		ATOMIC_BLOCK(PIPE_ATOMIC){
			p->data->packet_offset = 0;
			pipe_done_read(p->pipe);
			usb_pipe_handle(p);
		}
	}
	return r;
}

static inline void usb_pipe_write_byte(const USB_Pipe* p, uint8_t v){
	p->pipe->data->write_ptr[p->data->packet_offset++] = v;
	if (p->data->packet_offset == p->pipe->size){
		ATOMIC_BLOCK(PIPE_ATOMIC){
			p->data->packet_offset = 0;
			pipe_done_write(p->pipe);
			usb_pipe_handle(p);
		}
	}
}

