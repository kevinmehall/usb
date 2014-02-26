// Minimal USB Stack for ATxmega32a4u and related
// http://nonolithlabs.com
// (C) 2011 Kevin Mehall (Nonolith Labs) <km@kevinmehall.net>
//
// Heavily borrows from LUFA
// Copyright 2011  Dean Camera (dean [at] fourwalledcubicle [dot] com)
//
// Licensed under the terms of the GNU GPLv3+

#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#define CPU_TO_LE16(x) x

/** Like __attribute__(align(2)), but actually works. 
    From http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=121033
 */
#define GCC_FORCE_ALIGN_2  __attribute__((section (".data,\"aw\",@progbits\n.p2align 1;")))
#define ATTR_ALWAYS_INLINE __attribute__ ((always_inline))

/// From Atmel: Macros for XMEGA instructions not yet supported by the toolchain
// Load and Clear 
#ifdef __GNUC__
#define LACR16(addr,msk) \
	__asm__ __volatile__ ( \
	"ldi r16, %1" "\n\t" \
	".dc.w 0x9306" "\n\t"\
	::"z" (addr), "M" (msk):"r16")
#else
	#define LACR16(addr,msk) __lac((unsigned char)msk,(unsigned char*)addr)
#endif
 
// Load and Set
#ifdef __GNUC__
#define LASR16(addr,msk) \
	__asm__ __volatile__ ( \
	"ldi r16, %1" "\n\t" \
	".dc.w 0x9305" "\n\t"\
	::"z" (addr), "M" (msk):"r16")
#else
#define LASR16(addr,msk) __las((unsigned char)msk,(unsigned char*)addr)
#endif

// Exchange
#ifdef __GNUC__
#define XCHR16(addr,msk) \
	__asm__ __volatile__ ( \
	"ldi r16, %1" "\n\t" \
	".dc.w 0x9304" "\n\t"\
	::"z" (addr), "M" (msk):"r16")
#else
#define XCHR16(addr,msk) __xch(msk,addr)
#endif

// Load and toggle
#ifdef __GNUC__
#define LATR16(addr,msk) \
	__asm__ __volatile__ ( \
	"ldi r16, %1" "\n\t" \
	".dc.w 0x9307" "\n\t"\
	::"z" (addr), "M" (msk):"r16")
#else
#define LATR16(addr,msk) __lat(msk,addr)
#endif

typedef union USB_EP_pair{
	union{
		struct{
			USB_EP_t out;
			USB_EP_t in;
		};
		USB_EP_t ep[2];
	};
} __attribute__((packed)) USB_EP_pair_t;

extern USB_EP_pair_t endpoints[USB_NUM_EP+1];

#define USB_EP_size_to_gc(x)  ((x <= 8   )?USB_EP_BUFSIZE_8_gc:\
                               (x <= 16  )?USB_EP_BUFSIZE_16_gc:\
                               (x <= 32  )?USB_EP_BUFSIZE_32_gc:\
                               (x <= 64  )?USB_EP_BUFSIZE_64_gc:\
                               (x <= 128 )?USB_EP_BUFSIZE_128_gc:\
                               (x <= 256 )?USB_EP_BUFSIZE_256_gc:\
                               (x <= 512 )?USB_EP_BUFSIZE_512_gc:\
                                           USB_EP_BUFSIZE_1023_gc)

/// Configure the XMEGA's clock for use with USB.
void usb_configure_clock(void);

const uint8_t* usb_ep0_from_progmem(const uint8_t* addr, uint16_t size);
