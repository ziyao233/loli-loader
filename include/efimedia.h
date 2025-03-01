// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/efimedia.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_EFIMEDIA_H_INC__
#define __LOLI_EFIMEDIA_H_INC__

#include <efidef.h>

#define EFI_FILE_MODE_READ		0x0000000000000001

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
	EFI_GUID(0x0964e5b22, 0x6459, 0x11d2,				\
		 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b)
#define EFI_FILE_INFO_GUID \
	EFI_GUID(0x09576e92, 0x6d3f, 0x11d2,				\
		 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b)

#pragma pack(push, 0)

typedef struct {
	uint64_t size;
	uint64_t fileSize;
	uint64_t physicalSize;
	char reserved[];		// we don't care about these fields
} Efi_File_Info;

struct Efi_File_Protocol;

typedef struct Efi_Simple_File_System_Protocol {
	uint64_t revision;
	Efi_Status (*openVolume)(struct Efi_Simple_File_System_Protocol *this,
				 struct Efi_File_Protocol **root);
} Efi_Simple_File_System_Protocol;

typedef struct Efi_File_Protocol {
	uint64_t revision;
	Efi_Status (*open)(struct Efi_File_Protocol *this,
			   struct Efi_File_Protocol **newHandle,
			   wchar_t *filename, uint64_t openMode,
			   uint64_t attributes);
	Efi_Status (*close)(struct Efi_File_Protocol *this);
	Efi_Handle delete;
	Efi_Status (*read)(struct Efi_File_Protocol *this, uint_native *bufSize,
			   void *buf);
	Efi_Handle write;
	Efi_Handle getPosition;
	Efi_Handle setPosition;
	Efi_Status (*getInfo)(struct Efi_File_Protocol *this,
			      Efi_Guid *informationType,
			      uint_native *bufSize, void *buf);
	Efi_Handle setInfo;
} Efi_File_Protocol;

#pragma pack(pop)

#endif	// __LOLI_EFIMEDIA_H_INC__
