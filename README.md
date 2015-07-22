# Minimal, portable USB stack

 * Hardware drivers for Atmel SAM D, NXP LPC18xx/LPC43xx, and Atmel Xmega USB device peripherals
 * Cross-device API for manipulating endpoints
 * Descriptors and constants for USB core
 * Implementation of required control transfers and enumeration
 * Descriptors and implementation for DFU (firmware update) class
 * Descriptors for CDC (serial) class
 * Supports [WCID](https://github.com/pbatard/libwdi/wiki/WCID-Devices) automatic driver installation on Windows
 * MIT license

A USB DFU bootloader for SAMD21 (Cortex M0) uses under 4KB of flash and 1KB of RAM.

Compared to vendor USB stacks (Atmel ASF, Keil), it is much lighter weight, provides you the tools to build a fully-custom USB device, perhaps with multiple interfaces and endpoints, rather than implementing a fixed class. It uses structures instead of byte arrays to make descriptors more readable, and interrupt-context callbacks to integrate with your bare-metal code or RTOS scheduler.

Compared to [LUFA](http://www.fourwalledcubicle.com/LUFA.php), it has better support for Cortex-M devices, is better suited for modern DMA-integrated USB controllers, and more interrupt-driven, but does not provide as many class drivers.

## Open source firmware using this code

* [Tessel 2 (SAMD21J18A)](https://github.com/tessel/t2-firmware)
* [Tessel 1 (LPC1830)](https://github.com/tessel/t1-firmware)
* [Nonolith CEE (Xmega32A4U)](http://nonolithlabs.com/cee)
* [Starfish (SAMD21E16A)](https://github.com/kevinmehall/starfish)
