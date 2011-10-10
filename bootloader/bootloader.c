// USB Bootloader for Xmega AU
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Licensed under the terms of the GNU GPLv3+

#include "bootloader.h"
#include "sp_driver.h"

//
// Nonolith bootloader protocol
//

#define REQ_INFO		0xB0
// Returns a BootloaderInfo with info about the device
typedef struct{
	uint8_t magic[4]; // String 0x90 0x90 0xBB 0x01
	uint8_t version;
	uint8_t DEVID0;   // Device/Revision ID from MCU. See XMEGA AU Manual p46
	uint8_t DEVID1;
	uint8_t DEVID2;
	uint8_t REVID;
	uint16_t page_size;  // Page size in bytes
	uint32_t app_section_end; // Byte address of end of flash. Add one for flash size
	uint32_t entry_jmp_pointer; // App code can jump to this pointer to enter the bootloader
} BootloaderInfo;

#define REQ_ERASE		0xB1
// Erases the application section

#define REQ_START_WRITE 0xB2
// Sets the write pointer to the page address passed in wIndex.
// Data written to bulk endpoint 1 will be written at this address, and the
// address will be incremented automatically. The transfer is complete when a
// packet of less than 64 bytes is written.

#define REQ_CRC_APP		0xB3
// Return a CRC of the application section

#define REQ_CRC_BOOT	0xB4
// Return a CRC of the boot section

#define REQ_RESET	0xBF
// After acknowledging this request, the bootloader disables USB and resets
// the microcontroller

//
// End protocol definition
//



/// Flash page number where received data will be written
uint16_t page;

/// Byte offset into flash page of next data to be received
uint16_t pageOffs;

/// Buffer of incoming flash page
uint8_t pageBuf[APP_SECTION_PAGE_SIZE];

/// Size of IN endpoint receiving flash data
#define EP1_SIZE 64

void pollEndpoint(void);

/// Configure the device for bootloader mode and loop responding to bootloader commands
void runBootloader(void){
	// Turn on LED
	PORTE.DIRSET = (1<<0) | (1<<1);
	PORTE.OUTSET = (1<<0);
	
	USB_ConfigureClock();
	USB_Init();
	USB_ep_out_init(1, USB_EP_TYPE_BULK_gc, EP1_SIZE);
	sei();
	
	while (1){
		USB_Task();
		pollEndpoint();
	}
}

/// Jump target at known address to call from application code to switch to bootloader
extern void enterBootloader(void) __attribute__((used, naked, section(".boot-entry")));
void enterBootloader(void){
	runBootloader();
}

int main(void){
	// Pull up PR0 to test if it's being pulled low
	PORTR.DIR = 0;
	PORTR.PIN0CTRL = PORT_OPC_PULLUP_gc;
	
	_delay_us(1000);

	// Get the value of the reset vector. If it's unprogrammed, we know
	// there's nothing useful in app flash
	uint16_t reset_vect_value = pgm_read_word(0);
	
	if (!(PORTR.IN & 0x01) || reset_vect_value == 0xFFFF){
		runBootloader();
	}
	
	// Otherwise, clean up and jump to the app
	PORTR.PIN0CTRL = 0;
	EIND = 0x00;
	void (*reset_vect)( void ) = 0x000000;
    reset_vect();
}


/// Pack the ep0 input buffer with a response to REQ_INFO
void fillInfoStruct(void){
	BootloaderInfo *i=(BootloaderInfo*)ep0_buf_in;
	i->magic[0] = 0x90;
	i->magic[1] = 0x90;
	i->magic[2] = 0xBB;
	i->magic[3] = 0x01;
	i->version = 0;
	i->DEVID0 = MCU.DEVID0;
	i->DEVID1 = MCU.DEVID1;
	i->DEVID2 = MCU.DEVID2;
	i->REVID = MCU.REVID;
	i->page_size = APP_SECTION_PAGE_SIZE;
	i->app_section_end = APP_SECTION_END;
	i->entry_jmp_pointer = (uint32_t) &enterBootloader;
}


/// Handle USB control requests
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req){
	if ((req->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR){
		switch (req->bRequest){
			case REQ_INFO:
				fillInfoStruct();
				USB_ep0_send(sizeof(BootloaderInfo));
				return true;
			case REQ_ERASE:
				SP_EraseApplicationSection();
				USB_ep0_send(0);
				return true;
			case REQ_START_WRITE:
				page = req->wIndex;
				pageOffs = 0;
				USB_ep_out_start(1, pageBuf);
				USB_ep0_send(0);
				return true;
			case REQ_CRC_APP:
				*(uint32_t*)ep0_buf_in = SP_ApplicationCRC();
				USB_ep0_send(sizeof(uint32_t));
				return true;
			case REQ_CRC_BOOT:
				*(uint32_t*)ep0_buf_in = SP_BootCRC();
				USB_ep0_send(sizeof(uint32_t));
				return true;
			case REQ_RESET:
				USB_ep0_send(0);
				USB_ep0_wait_for_complete();
				_delay_us(10000);
				USB_Detach();
				
				cli();
				uint8_t temp = WDT_ENABLE_bm | WDT_CEN_bm | WD_256CLK_gc;
				CCP = CCP_IOREG_gc;
				WDT.CTRL = temp;
				while(1){}; // wait for WDT
				
				return true;
		}
	}
	
	return false;
}


void pollEndpoint(void){
	if (USB_ep_out_received(1)){		
		
		pageOffs += EP1_SIZE;

		if (pageOffs == APP_SECTION_PAGE_SIZE){
			// Write a page to flash
			SP_LoadFlashPage(pageBuf);
			NVM.CMD = NVM_CMD_NO_OPERATION_gc;
			SP_WriteApplicationPage(page*APP_SECTION_PAGE_SIZE);
			SP_WaitForSPM();
			NVM.CMD = NVM_CMD_NO_OPERATION_gc;
			
			page++;
			pageOffs = 0;
		}

		if (page * APP_SECTION_PAGE_SIZE < APP_SECTION_END){
			// If there's remaining room in flash, configure the endpoint to accept more data
			USB_ep_out_start(1, &pageBuf[pageOffs]);
		}
	}
}
