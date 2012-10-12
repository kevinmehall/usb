/*
             LUFA Library
     Copyright (C) Dean Camera, 2011.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2011  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *  \brief Common library convenience headers, macros and functions.
 *
 *  \copydetails Group_Common
 */

/** \defgroup Group_Common Common Utility Headers - LUFA/Drivers/Common/Common.h
 *  \brief Common library convenience headers, macros and functions.
 *
 *  Common utility headers containing macros, functions, enums and types which are common to all
 *  aspects of the library.
 *
 *  @{
 */

#pragma once

/* Includes: */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

/* Architecture specific utility includes: */
#ifdef  __AVR__ 
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <avr/pgmspace.h>
	#include <avr/eeprom.h>
	#include <util/delay.h>
	
	typedef uint8_t uint_reg_t;
	
	#define ARCH_HAS_EEPROM_ADDRESS_SPACE
	#define ARCH_HAS_FLASH_ADDRESS_SPACE
	#define ARCH_HAS_MULTI_ADDRESS_SPACE
	#define ARCH_LITTLE_ENDIAN	
	
	
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

	
#endif

/* Public Interface - May be used in end-application: */
/* Macros: */
/** Macro for encasing other multi-statement macros. This should be used along with an opening brace
 *  before the start of any multi-statement macro, so that the macros contents as a whole are treated
 *  as a discrete block and not as a list of separate statements which may cause problems when used as
 *  a block (such as inline \c if statements).
 */
#define MACROS                  do

/** Macro for encasing other multi-statement macros. This should be used along with a preceding closing
 *  brace at the end of any multi-statement macro, so that the macros contents as a whole are treated
 *  as a discrete block and not as a list of separate statements which may cause problems when used as
 *  a block (such as inline \c if statements).
 */
#define MACROE                  while (0)

/** Convenience macro to determine the larger of two values.
 *
 *  \note This macro should only be used with operands that do not have side effects from being evaluated
 *        multiple times.
 *
 *  \param[in] x  First value to compare
 *  \param[in] y  First value to compare
 *
 *  \return The larger of the two input parameters
 */
#if !defined(MAX) || defined(__DOXYGEN__)
	#define MAX(x, y)               (((x) > (y)) ? (x) : (y))
#endif

/** Convenience macro to determine the smaller of two values.
 *
 *  \note This macro should only be used with operands that do not have side effects from being evaluated
 *        multiple times.
 *
 *  \param[in] x  First value to compare
 *  \param[in] y  First value to compare
 *
 *  \return The smaller of the two input parameters
 */
#if !defined(MIN) || defined(__DOXYGEN__)
	#define MIN(x, y)               (((x) < (y)) ? (x) : (y))
#endif

#if !defined(STRINGIFY) || defined(__DOXYGEN__)
	/** Converts the given input into a string, via the C Preprocessor. This macro puts literal quotation
	 *  marks around the input, converting the source into a string literal.
	 *
	 *  \param[in] x  Input to convert into a string literal.
	 *
	 *  \return String version of the input.
	 */
	#define STRINGIFY(x)            #x

	/** Converts the given input into a string after macro expansion, via the C Preprocessor. This macro puts
	 *  literal quotation marks around the expanded input, converting the source into a string literal.
	 *
	 *  \param[in] x  Input to expand and convert into a string literal.
	 *
	 *  \return String version of the expanded input.
	 */
	#define STRINGIFY_EXPANDED(x)   STRINGIFY(x)
#endif

#if defined(__GNUC__) || defined(__DOXYGEN__)
	/** Forces GCC to use pointer indirection (via the device's pointer register pairs) when accessing the given
	 *  struct pointer. In some cases GCC will emit non-optimal assembly code when accessing a structure through
	 *  a pointer, resulting in a larger binary. When this macro is used on a (non \c const) structure pointer before
	 *  use, it will force GCC to use pointer indirection on the elements rather than direct store and load
	 *  instructions.
	 *
	 *  \param[in, out] StructPtr  Pointer to a structure which is to be forced into indirect access mode.
	 */
	#define GCC_FORCE_POINTER_ACCESS(StructPtr)   __asm__ __volatile__("" : "=b" (StructPtr) : "0" (StructPtr))

	/** Forces GCC to create a memory barrier, ensuring that memory accesses are not reordered past the barrier point.
	 *  This can be used before ordering-critical operations, to ensure that the compiler does not re-order the resulting
	 *  assembly output in an unexpected manner on sections of code that are ordering-specific.
	 */
	#define GCC_MEMORY_BARRIER()                  __asm__ __volatile__("" ::: "memory");
	
	/** Evaluates to boolean true if the specified value can be determined at compile time to be a constant value
	 *  when compiling under GCC.
	 *
	 *  \param[in] x  Value to check compile time constantness of.
	 *
	 *  \return Boolean true if the given value is known to be a compile time constant, false otherwise.
	 */
	#define GCC_IS_COMPILE_CONST(x)               __builtin_constant_p(x)

	/** Compile-time assert */
	#define GCC_ASSERT(e) ((void)sizeof(char[1 - 2*!(__builtin_constant_p(e) && (e))]))
	
	/** Like __attribute__(align(2)), but actually works. 
	    From http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=121033
	 */
	#define GCC_FORCE_ALIGN_2  __attribute__((section (".data,\"aw\",@progbits\n.p2align 1;")))

	#define likely(x) __builtin_expect((x),1)
	#define unlikely(x) __builtin_expect((x),0)
#else
	#define GCC_FORCE_POINTER_ACCESS(StructPtr)
	#define GCC_MEMORY_BARRIER()
	#define GCC_IS_COMPILE_CONST(x)               0
	#define GCC_FORCE_ALIGN_2
	#define likely(x) x
	#define unlikely(x) x
#endif


/** Indicates to the compiler that the function can not ever return, so that any stack restoring or
 *  return code may be omitted by the compiler in the resulting binary.
 */
#define ATTR_NO_RETURN              __attribute__ ((noreturn))

/** Indicates that the function returns a value which should not be ignored by the user code. When
 *  applied, any ignored return value from calling the function will produce a compiler warning.
 */
#define ATTR_WARN_UNUSED_RESULT     __attribute__ ((warn_unused_result))

/** Indicates that the specified parameters of the function are pointers which should never be \c NULL.
 *  When applied as a 1-based comma separated list the compiler will emit a warning if the specified
 *  parameters are known at compiler time to be \c NULL at the point of calling the function.
 */
#define ATTR_NON_NULL_PTR_ARG(...)  __attribute__ ((nonnull (__VA_ARGS__)))

/** Removes any preamble or postamble from the function. When used, the function will not have any
 *  register or stack saving code. This should be used with caution, and when used the programmer
 *  is responsible for maintaining stack and register integrity.
 */
#define ATTR_NAKED                  __attribute__ ((naked))

/** Prevents the compiler from considering a specified function for in-lining. When applied, the given
 *  function will not be in-lined under any circumstances.
 */
#define ATTR_NO_INLINE              __attribute__ ((noinline))

/** Forces the compiler to inline the specified function. When applied, the given function will be
 *  in-lined under all circumstances.
 */
#define ATTR_ALWAYS_INLINE          __attribute__ ((always_inline))

/** Indicates that the specified function is pure, in that it has no side-effects other than global
 *  or parameter variable access.
 */
#define ATTR_PURE                   __attribute__ ((pure))

/** Indicates that the specified function is constant, in that it has no side effects other than
 *  parameter access.
 */
#define ATTR_CONST                  __attribute__ ((const))

/** Marks a given function as deprecated, which produces a warning if the function is called. */
#define ATTR_DEPRECATED             __attribute__ ((deprecated))

/** Marks a function as a weak reference, which can be overridden by other functions with an
 *  identical name (in which case the weak reference is discarded at link time).
 */
#define ATTR_WEAK                   __attribute__ ((weak))

/** Forces the compiler to not automatically zero the given global variable on startup, so that the
 *  current RAM contents is retained. Under most conditions this value will be random due to the
 *  behaviour of volatile memory once power is removed, but may be used in some specific circumstances,
 *  like the passing of values back after a system watchdog reset.
 */
#define ATTR_NO_INIT                __attribute__ ((section (".noinit")))


/** Places the function in one of the initialization sections, which execute before the main function
 *  of the application. Refer to the avr-libc manual for more information on the initialization sections.
 *
 *  \param[in] SectionIndex  Initialization section number where the function should be placed.
 */
#define ATTR_INIT_SECTION(SectionIndex) __attribute__ ((naked, section (".init" #SectionIndex )))

/** Marks a function as an alias for another function.
 *
 *  \param[in] Func  Name of the function which the given function name should alias.
 */
#define ATTR_ALIAS(Func)               __attribute__ ((alias( #Func )))

/** Marks a variable or struct element for packing into the smallest space available, omitting any
 *  alignment bytes usually added between fields to optimize field accesses.
 */
#define ATTR_PACKED                     __attribute__ ((packed))

