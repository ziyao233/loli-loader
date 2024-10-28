// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/efidef.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_EFIDEF_H_INC__
#define __LOLI_EFIDEF_H_INC__

#define NULL ((void *)0)

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

#pragma pack(push, 4)

typedef struct {
	uint8_t data[16];
} Efi_Guid;

#pragma pack(pop)

#define EFI_GUID(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7) \
	{{ (a) & 0xff, ((a) >> 8) & 0xff, ((a) >> 16) & 0xff,		\
		((a) >> 24) & 0xff,					\
		(b) & 0xff, ((b) >> 8) & 0xff,				\
		(c) & 0xff, ((c) >> 8) & 0xff,				\
		(d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) } }

#define EFI_SUCCESS			0
#define EFI_INVALID_PARAMETER		2
#define EFI_OUT_OF_RESOURCES		9

#endif	// __LOLI_EFIDEF_H_INC__
