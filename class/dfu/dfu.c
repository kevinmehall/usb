#include "class/dfu/dfu.h"

DFU_State dfu_state = DFU_STATE_dfuIDLE;
DFU_Status dfu_status = DFU_STATUS_OK;
uint16_t dfu_poll_timeout;
uint16_t dfu_block_offset;

void dfu_control_setup() {
	switch (usb_setup.bRequest) {
		case DFU_DNLOAD:
			if (dfu_state == DFU_STATE_dfuIDLE || dfu_state == DFU_STATE_dfuDNLOAD_IDLE) {
				if (usb_setup.wLength == 0) {
					dfu_state = DFU_STATE_dfuMANIFEST_SYNC;
					usb_ep0_out();
					return usb_ep0_in(0);
				} else {
					dfu_block_offset = 0;
					dfu_cb_dnload_block(usb_setup.wValue, usb_setup.wLength);

					if (dfu_state != DFU_STATE_dfuERROR) {
						dfu_state = DFU_STATE_dfuDNBUSY;
						return usb_ep0_out();
					}
				}
			} else {
				dfu_error(DFU_STATUS_errSTALLEDPKT);
			}
			return usb_ep0_stall();
		case DFU_UPLOAD:
			dfu_error(DFU_STATUS_errSTALLEDPKT);
			return usb_ep0_stall();
		case DFU_GETSTATUS: {
			if (dfu_state == DFU_STATE_dfuMANIFEST_SYNC) {
				dfu_state = DFU_STATE_dfuMANIFEST;
				dfu_cb_manifest();
			}

			uint8_t len = usb_setup.wLength;
			if (len > sizeof(DFU_StatusResponse)) len = sizeof(DFU_StatusResponse);
			DFU_StatusResponse* status = (DFU_StatusResponse*) ep0_buf_in;
			status->bStatus = dfu_status;
			status->bwPollTimeout[0] = (dfu_poll_timeout >>  0) & 0xFF;
			status->bwPollTimeout[1] = (dfu_poll_timeout >>  8) & 0xFF;
			status->bwPollTimeout[2] = (dfu_poll_timeout >> 16) & 0xFF;
			status->bState = dfu_state;
			status->iString = 0;
			usb_ep0_in(len);
			return usb_ep0_out();
		}
		case DFU_ABORT:
		case DFU_CLRSTATUS:
			dfu_reset();
			usb_ep0_in(0);
			return usb_ep0_out();
		case DFU_GETSTATE:
			ep0_buf_in[0] = dfu_state;
			usb_ep0_in(1);
			return usb_ep0_out();
	}
	return usb_ep0_stall();
}

void dfu_error(uint8_t status) {
	dfu_status = status;
	dfu_state = DFU_STATE_dfuERROR;
}

void dfu_reset() {
	dfu_state = DFU_STATE_dfuIDLE;
	dfu_status = DFU_STATUS_OK;
	dfu_poll_timeout = 0;
}

void dfu_control_out_completion() {
	switch (usb_setup.bRequest) {
		case DFU_DNLOAD: {
			uint16_t len = usb_ep_out_length(0);
			if (len > 0) {
				dfu_cb_dnload_packet_completed(usb_setup.wValue, dfu_block_offset, ep0_buf_out, len);
			}
			dfu_block_offset += USB_EP0_SIZE;

			if (dfu_block_offset >= usb_setup.wLength) {
				dfu_poll_timeout = dfu_cb_dnload_block_completed(usb_setup.wValue, usb_setup.wLength);
				if (dfu_poll_timeout == 0 && dfu_status == DFU_STATUS_OK) {
					dfu_state = DFU_STATE_dfuDNLOAD_IDLE;
					usb_ep0_in(0);
				}
			} else {
				usb_ep0_out();
			}

			break;
		}
	}
}

void dfu_control_in_completion() {

}
