// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/efidevicetree.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_DEVICETREE_H_INC__
#define __LOLI_DEVICETREE_H_INC__

#include <efidef.h>

#define EFI_DT_FIXUP_PROTOCOL_GUID \
	EFI_GUID(0xe617d64c, 0xfe08, 0x46da,				\
		 0xf4, 0xdc, 0xbb, 0xd5, 0x87, 0x0c, 0x73, 0x00)

#define EFI_DT_APPLY_FIXES		0x1
#define EFI_DT_RESERVE_MEMORY		0x2

#pragma pack(push, 0)

typedef struct Efi_Dt_Fixup_Protocol {
	uint64_t revision;
	Efi_Status (*fixup)(struct Efi_Dt_Fixup_Protocol *self,
			    void *fdt,
			    uint_native *bufSize,
			    uint32_t flags);
} Efi_Dt_Fixup_Protocol;

#pragma pack(pop)

#endif	// __LOLI_DEVICETREE_H_INC__
