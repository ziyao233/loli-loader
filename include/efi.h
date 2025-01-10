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
#include <efiboot.h>

#pragma pack(push, 0)

#define EFI_DTB_TABLE_GUID \
	EFI_GUID(0xb1b621d5, 0xf19c, 0x41a5,				\
		 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0)

typedef struct {
	Efi_Guid vendorGuid;
	void *vendorTable;
} Efi_Configuration_Table;

typedef struct Efi_System_Table {
	Efi_Table_Header header;

	wchar_t *firmwareVendor;
	uint32_t firmwareRevision;
	Efi_Handle consoleInHandle;
	Efi_Simple_Text_Input_Protocol *conIn;
	Efi_Handle consoleOutHandle;
	Efi_Simple_Text_Output_Protocol *conOut;
	Efi_Handle standardErrorHandle;
	Efi_Simple_Text_Output_Protocol *stdErr;
	Efi_Handle runtimeServices;
	Efi_Boot_Services *bootServices;

	uint_native numberOfTableEntries;
	Efi_Configuration_Table *configurationTable;
} Efi_System_Table;

#pragma pack(pop)

extern Efi_System_Table *gST;
extern Efi_Boot_Services *gBS;
extern Efi_Handle gSelf;

void efi_init(Efi_Handle imageHandle, Efi_System_Table *st);

#endif	// __LOLI_EFI_H_INC__
