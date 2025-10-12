// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/efidevicepath.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_EFI_DEVICE_PATH_H_INC__
#define __LOLI_EFI_DEVICE_PATH_H_INC__

#include <efidef.h>

#define EFI_DEVICE_PATH_PROTOCOL_GUID \
	EFI_GUID(0x09576e91, 0x6d3f, 0x11d2,				\
		 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b)

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

typedef struct {
	Efi_Device_Path_Protocol head;
	Efi_Guid vendorGuid;
	/* Omitted to simplify definition of device paths */
	/* uint8_t vendorData[]; */
} Efi_Vendor_Media_Device_Path;

#pragma pack(pop)

#define EFI_DEVICE_HARDWARE			1
#define  EFI_DEVICE_SUBTYPE_MEMORY_MAPPED	6
#define EFI_DEVICE_MEDIA			4
#define  EFI_DEVICE_SUBTYPE_VENDOR		3
#define EFI_DEVICE_END				0x7f
#define  EFI_DEVICE_SUBTYPE_ENTIRE_END		0xff

#endif	// __LOLI_EFI_DEVICE_PATH_H_INC__
