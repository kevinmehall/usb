Xmega USB Bootloader
====================

Entering the bootloader
-----------------------

Pull PR0 to GND while powering up/resetting the Xmega to force the bootloader to run. Edit main() in bootloader.c to change the entry condition. The bootloader will also run if the application section is unprogrammed.

Application code can invoke the bootloader by disabling USB and all interrupts, and then calling the entry point with the following code:

    void (*enter_bootloader)(void) = 0x47fc /*0x8ff8/2*/;
    enter_bootloader();

If your application code uses USB, add a ~250ms delay between disabling the USB pullup and entering the bootloader, so the host has time to detect that the device has been "unplugged" before the bootloader attaches.


Using the bootloader
--------------------

Use `python flash.py file.hex` to flash a hex file, verify the CRC, and then reset

The extremely simple protocol is documented in bootloader.c


Building / Flashing
-------------------

Run `make program` to build the bootloader and flash it with an AVRispmkII. You can edit the cable in the makefile. Note that the AVR Dragon is unable to correctly write the bootloader section on Xmega.

To flash the included hex file without building, run

    avrdude -p atxmega32a4 -P usb -c avrispmkII -U fuse2:w:0xBF:m  -e -U boot:w:bootloader.prebuilt.hex
