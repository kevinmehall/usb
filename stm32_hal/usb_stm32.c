#include "usb.h"
#include "stm32_hal/usb_stm32.h"

#include "stm32f0xx_hal.h"
PCD_HandleTypeDef hpcd;

void usb_init(){
	GPIO_InitTypeDef	GPIO_InitStruct;

	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin = (GPIO_PIN_11 | GPIO_PIN_12);
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF2_USB;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct); 

	__HAL_RCC_USB_CLK_ENABLE();

	hpcd.Instance = USB;
	hpcd.Init.dev_endpoints = 8;
	hpcd.Init.ep0_mps = 0x40;
	hpcd.Init.phy_itface = PCD_PHY_EMBEDDED;
	hpcd.Init.speed = PCD_SPEED_FULL;
	hpcd.Init.low_power_enable = 0;
	HAL_PCD_Init(&hpcd);
	
	HAL_PCDEx_PMAConfig(&hpcd, 0x00, PCD_SNG_BUF, 0x18);
	HAL_PCDEx_PMAConfig(&hpcd, 0x80, PCD_SNG_BUF, 0x58); 
	
	usb_reset();
	
	HAL_NVIC_SetPriority(USB_IRQn, 3, 0);
	HAL_NVIC_EnableIRQ(USB_IRQn);
}

void usb_reset(){
	usb_enable_ep(0x00, USB_EP_TYPE_CONTROL, USB_EP0_SIZE);
	usb_enable_ep(0x80, USB_EP_TYPE_CONTROL, USB_EP0_SIZE);
}

void usb_set_address(uint8_t addr) {
	HAL_PCD_SetAddress(&hpcd, addr);
}

inline void usb_enable_ep(uint8_t ep, uint8_t type, usb_size bufsize) {
	HAL_PCD_EP_Open(&hpcd, ep, bufsize, type);
}

inline void usb_disable_ep(uint8_t ep) {
	HAL_PCD_EP_Close(&hpcd, ep);
}

inline void usb_reset_ep(uint8_t ep) {
	HAL_PCD_EP_Flush(&hpcd, ep);
}

inline usb_bank usb_ep_start_out(uint8_t ep, uint8_t* data, usb_size len) {
	HAL_PCD_EP_Receive(&hpcd, ep, data, len);
	return 0;
}

inline usb_bank usb_ep_start_in(uint8_t ep, const uint8_t* data, usb_size size, bool zlp) {
	HAL_PCD_EP_Transmit(&hpcd, ep, (uint8_t*) data, size);
	return 0;
}

inline bool usb_ep_empty(uint8_t ep) {
	return false;
}

inline bool usb_ep_ready(uint8_t ep) {
	return usb_ep_empty(ep);
}

inline bool usb_ep_pending(uint8_t ep) {
	return false;
}

inline void usb_ep_handled(uint8_t ep) {

}

inline usb_size usb_ep_out_length(uint8_t ep){
	return HAL_PCD_EP_GetRxCount(&hpcd, ep) ;
}

inline void usb_detach(void) {
	HAL_PCD_Stop(&hpcd);
}

inline void usb_attach(void) {
	HAL_PCD_Start(&hpcd);
}

/// Enable the OUT stage on the default control pipe.
inline void usb_ep0_out(void) {
	usb_ep_start_out(0x00, ep0_buf_out, USB_EP0_SIZE);
}

inline void usb_ep0_in(uint8_t size){
	usb_ep_start_in(0x80, ep0_buf_in, size, true);
}

inline void usb_ep0_stall(void) {
	HAL_PCD_EP_SetStall(&hpcd, 0);
	HAL_PCD_EP_SetStall(&hpcd, 0x80);
}

void usb_set_speed(USB_Speed speed) {
	
}

USB_Speed usb_get_speed() {
	return USB_SPEED_FULL;
}

void usb_irq() {
	HAL_PCD_IRQHandler(&hpcd);
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
	memcpy(&usb_setup, (uint8_t *)hpcd->Setup, sizeof(usb_setup));
	usb_handle_setup();
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
	if (epnum == 0) {
		usb_handle_control_out_complete();		
	}
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
	if (epnum == 0) {
		usb_handle_control_in_complete();
	}
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd) { }

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {	 
	usb_reset();
	usb_cb_reset();
}

void* stm32_serial_number_string_descriptor() {
	char buf[21];

	const unsigned char* id = (unsigned char*) 0x1FFFF7AC;
	for (int i=0; i<20; i++) {
		unsigned idx = (i*5)/8;
		unsigned pos = (i*5)%8;
		unsigned val = ((id[idx] >> pos) | (id[idx+1] << (8-pos))) & ((1<<5)-1);
		buf[i] = "0123456789ABCDFGHJKLMNPQRSTVWXYZ"[val];
	}
	buf[20] = 0;
	return usb_string_to_descriptor(buf);
}
