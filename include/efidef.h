// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/efidef.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_EFIDEF_H_INC__
#define __LOLI_EFIDEF_H_INC__

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long int uint64_t;
typedef uint32_t uint_native;
typedef uint8_t bool;
typedef uint16_t wchar_t;

typedef void * Efi_Handle;
typedef uint_native Efi_Status;

#define EFI_SUCCESS	0

#endif	// __LOLI_EFIDEF_H_INC__
