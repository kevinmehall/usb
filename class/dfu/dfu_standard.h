#pragma once

#define DFU_INTERFACE_CLASS 0xFE
#define DFU_INTERFACE_SUBCLASS 0x01
#define DFU_INTERFACE_PROTOCOL 0x02

#define DFU_DETACH 0
#define DFU_DNLOAD 1
#define DFU_UPLOAD 2
#define DFU_GETSTATUS 3
#define DFU_CLRSTATUS 4
#define DFU_GETSTATE 5
#define DFU_ABORT 6

#define DFU_DESCRIPTOR_TYPE 0x21

#define DFU_ATTR_CAN_DOWNLOAD (1<<0)
#define DFU_ATTR_CAN_UPLOAD (1<<1)
#define DFU_ATTR_MANIFESTATION_TOLERANT (1<<2)
#define DFU_ATTR_WILL_DETACH (1<<3)

typedef struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bmAttributes;
	uint16_t wDetachTimeout;
	uint16_t wTransferSize;
	uint16_t bcdDFUVersion;
} __attribute__((packed)) DFU_FunctionalDescriptor;

typedef enum {
  DFU_STATE_appIDLE             = 0,
  DFU_STATE_appDETACH           = 1,
  DFU_STATE_dfuIDLE             = 2,
  DFU_STATE_dfuDNLOAD_SYNC      = 3,
  DFU_STATE_dfuDNBUSY           = 4,
  DFU_STATE_dfuDNLOAD_IDLE      = 5,
  DFU_STATE_dfuMANIFEST_SYNC    = 6,
  DFU_STATE_dfuMANIFEST         = 7,
  DFU_STATE_dfuMANIFEST_WAIT_RST= 8,
  DFU_STATE_dfuUPLOAD_IDLE      = 9,
  DFU_STATE_dfuERROR            = 10
} DFU_State;

typedef struct {
  uint8_t bStatus;
  uint8_t bwPollTimeout[3];
  uint8_t bState;
  uint8_t iString;
} __attribute__((packed)) DFU_StatusResponse;

typedef enum {
	DFU_STATUS_OK = 0x00,
	DFU_STATUS_errTARGET = 0x01,
	DFU_STATUS_errFILE = 0x02,
	DFU_STATUS_errWRITE = 0x03,
	DFU_STATUS_errERASE = 0x04,
	DFU_STATUS_errCHECK_ERASED = 0x05,
	DFU_STATUS_errPROG = 0x06,
	DFU_STATUS_errVERIFY = 0x07,
	DFU_STATUS_errADDRESS = 0x08,
	DFU_STATUS_errNOTDONE = 0x09,
	DFU_STATUS_errFIRMWARE = 0x0A,
	DFU_STATUS_errVENDOR = 0x0B,
	DFU_STATUS_errUSBR = 0x0C,
	DFU_STATUS_errPOR = 0x0D,
	DFU_STATUS_errUNKNOWN = 0x0E,
	DFU_STATUS_errSTALLEDPKT = 0x0F,
} DFU_Status;
