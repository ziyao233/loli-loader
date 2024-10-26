// SPDX-License-Identifer: MPL-2.0
/*
 *	loli-loader
 *	/include/efi.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_EFI_H_INC__
#define __LOLI_EFI_H_INC__

#include <efidef.h>
#include <eficon.h>

#pragma pack(push, 0)

typedef struct {
	/* EFI SYSTEM TABLE HEADER */
	uint64_t signature;
	uint32_t revision;
	uint32_t headerSize;
	uint32_t crc32;
	uint32_t reserved;

	wchar_t *firmwareVendor;
	uint32_t firmwareRevision;
	Efi_Handle consoleInHandle;
	Efi_Handle conIn;
	Efi_Handle consoleOutHandle;
	Efi_Simple_Text_Output_Protocol *conOut;
} Efi_System_Table;

#pragma pack(pop)

extern Efi_System_Table *gST;
extern Efi_Handle gSelf;

void efi_init(Efi_Handle imageHandle, Efi_System_Table *st);

#endif	// __LOLI_EFI_H_INC__
