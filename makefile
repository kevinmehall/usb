SRC_USB += $(USB_PATH)/usb_xmega.c
SRC_USB += $(USB_PATH)/usb_requests.c

USB_OPTS += -D __AVR_ATxmega32A4U__
USB_OPTS += -include io.h
