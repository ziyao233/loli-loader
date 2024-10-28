// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/efiloadedimage.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_EFILOADEDIMAGE_H_INC__
#define __LOLI_EFILOADEDIMAGE_H_INC__

#include <efidef.h>

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
	EFI_GUID(0x5B1B31A1, 0x9562, 0x11d2,				\
		 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B)

typedef struct {
	uint32_t revision;
	Efi_Handle parentHandle;
	struct Efi_System_Table *systemTable;

	Efi_Handle deviceHandle;
	Efi_Handle filePath;
	void *reserved;

	uint32_t loadOptionSize;
	void *loadOptions;

	void *imageBase;
	uint64_t imageSize;
} Efi_Loaded_Image_Protocol;

#endif	// __LOLI_EFILOADEDIMAGE_H_INC__
