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
typedef unsigned long int size_t;

typedef void * Efi_Handle;
typedef uint_native Efi_Status;

typedef struct Efi_Table_Header {
	uint64_t signature;
	uint32_t revision;
	uint32_t headerSize;
	uint32_t crc32;
	uint32_t reserved;
} Efi_Table_Header;

#define EFI_SUCCESS			0
#define EFI_INVALID_PARAMETER		2
#define EFI_OUT_OF_RESOURCES		9

#endif	// __LOLI_EFIDEF_H_INC__
