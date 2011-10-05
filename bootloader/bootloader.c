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
	uint8_t DEVID0;   // Device/Revision ID from MCU. See XMEGA AU Manual p46
	uint8_t DEVID1;
	uint8_t DEVID2;
	uint8_t REVID;
	uint16_t page_size;
	uint16_t app_section_end;
	
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

uint16_t page;
uint8_t pageOffs; 
uint8_t pageBuf[APP_SECTION_PAGE_SIZE];

#define EP1_SIZE 64
#define PKTS_PER_PAGE (APP_SECTION_PAGE_SIZE/EP1_SIZE)

void pollEndpoint(void);

void runBootloader(void){
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
	
	//PORTE.DIRCLR = (1<<0) | (1<<1);
	//cli();
}

int main(void){
	void (*reset_vect)( void ) = 0x000000;
	PORTR.DIR = 0;
	PORTR.PIN0CTRL = PORT_OPC_PULLUP_gc;
	
	for (uint16_t i=0; i<16384; i++);
	
	if (!(PORTR.IN & 0x01)){
		runBootloader();
	}
	
	PORTR.PIN0CTRL = 0;
	EIND = 0x00;
    reset_vect();
}

void fillInfoStruct(void){
	BootloaderInfo *i=(BootloaderInfo*)ep0_buf_in;
	i->magic[0] = 0x90;
	i->magic[1] = 0x90;
	i->magic[2] = 0xBB;
	i->magic[3] = 0x01;
	i->DEVID0 = MCU.DEVID0;
	i->DEVID1 = MCU.DEVID1;
	i->DEVID2 = MCU.DEVID2;
	i->REVID = MCU.REVID;
	i->page_size = APP_SECTION_PAGE_SIZE;
	i->app_section_end = APP_SECTION_END;
}

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
				while (!USB_ep_in_sent(0)){}; // wait for status stage to finish
				
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
		endpoints[1].out.STATUS &= ~USB_EP_TRNCOMPL0_bm;
		pageOffs++;
		
		bool done = 0; // USB_ep_out_count(1) < EP1_SIZE;
		
		if (pageOffs == PKTS_PER_PAGE || done){
			PORTE.OUTTGL = (1<<0);
			SP_LoadFlashPage(pageBuf);
			NVM.CMD = NVM_CMD_NO_OPERATION_gc;
			SP_WriteApplicationPage(page*APP_SECTION_PAGE_SIZE);
			SP_WaitForSPM();
			NVM.CMD = NVM_CMD_NO_OPERATION_gc;
			
			page++;
			pageOffs = 0;
			for (int i=0; i<APP_SECTION_PAGE_SIZE; i++) pageBuf[i]=0;
		}
		
		if (!done){
			USB_ep_out_start(1, &pageBuf[pageOffs*EP1_SIZE]);
		}
	}
}

