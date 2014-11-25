#pragma once

#include "usb.h"
#include "dfu_standard.h"

extern DFU_State dfu_state;
extern DFU_Status dfu_status;
extern uint16_t dfu_poll_timeout;

void dfu_reset(void);
void dfu_error(uint8_t status);
void dfu_control_setup(void);
void dfu_control_in_completion(void);
void dfu_control_out_completion(void);

// Callbacks
uint16_t dfu_cb_upload_block(uint16_t block, uint8_t** ptr);
void dfu_cb_dnload_block(uint16_t block, uint16_t len);
void dfu_cb_dnload_packet_completed(uint16_t block, uint16_t offset, uint8_t* buffer, uint16_t len);
unsigned dfu_cb_dnload_block_completed(uint16_t block, uint16_t length);
void dfu_cb_manifest(void);
void dfu_cb_detach(void);
