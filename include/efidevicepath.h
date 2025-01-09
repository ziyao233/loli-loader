// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/efidevicepath.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_EFI_DEVICE_PATH_H_INC__
#define __LOLI_EFI_DEVICE_PATH_H_INC__

#include <efidef.h>

#pragma pack(push, 0)

typedef struct {
	uint8_t type;
	uint8_t subtype;
	uint16_t length;
} Efi_Device_Path_Protocol;

typedef struct {
	Efi_Device_Path_Protocol head;
	uint32_t memtype;
	uint64_t startAddr;
	uint64_t endAddr;
} Efi_Memory_Mapped_Device_Path;

#pragma pack(pop)

#define EFI_DEVICE_HARDWARE			1
#define  EFI_DEVICE_SUBTYPE_MEMORY_MAPPED	6
#define EFI_DEVICE_END				0x7f
#define  EFI_DEVICE_SUBTYPE_ENTIRE_END		0xff

#endif	// __LOLI_EFI_DEVICE_PATH_H_INC__
