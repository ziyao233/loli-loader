// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/efiboot.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_EFIBOOT_H_INC__
#define __LOLI_EFIBOOT_H_INC__

#include <efi.h>
#include <efidef.h>

#pragma pack(push, 0)

typedef uint_native Efi_Tpl;

typedef enum {
	ALLOCATE_ANY_PAGES,
	ALLOCATE_MAX_ADDRESS,
	ALLOCATE_ADDRESS,
	MAX_ALLOCATE_TYPE
} Efi_Allocate_Type;

typedef enum {
	EFI_RESERVED_MEMORY_TYPE,
	EFI_LOADER_CODE,
	EFI_LOADER_DATA,
	EFI_BOOT_SERVICES_CODE,
	EFI_BOOT_SERVICES_DATA,
	EFI_CONVENTIONAL_MEMORY,
	EFI_UNUSABLE_MEMORY,
	EFI_ACPI_RECLAIM_MEMORY,
	EFI_ACPI_MEMORY_NVS,
	EFI_MEMORY_MAPPED_IO,
	EFI_MEMORY_MAPPED_IO_PORT_SPACE,
	EFI_PAL_CODE,
	EFI_PERSISTENT_MEMORY,
	EFI_UNACCEPTED_MEMORY_TYPE,
	EFI_MAX_MEMORY_TYPE
} Efi_Memory_Type;

typedef struct {
	struct Efi_Table_Header header;

	Efi_Tpl (*raiseTpl)(Efi_Tpl newTpl);
	void (*restoreTpl)(Efi_Tpl oldTpl);

	Efi_Status (*allocatePages)(Efi_Allocate_Type type,
				    Efi_Memory_Type memtype,
				    uint_native pages,
				    void **memory);
	Efi_Status (*freePages)(void *memory, uint_native pages);
	Efi_Handle getMemoryMap;

	Efi_Status (*allocatePool)(Efi_Memory_Type type, uint_native size,
				   void **buf);
	Efi_Status (*freePool)(void *buf);

	Efi_Handle createEvent;
	Efi_Handle setTimer;
	Efi_Handle waitForEvent;
	Efi_Handle signalEvent;
	Efi_Handle closeEvent;
	Efi_Handle checkEvent;

	Efi_Handle installProtocolInterface;
	Efi_Handle reinstallProtocolInterface;
	Efi_Handle uninstallProtocolInterface;
	Efi_Status (*handleProtocol)(Efi_Handle handle, Efi_Guid *protocol,
				     void **interface);
	Efi_Handle registerProtocolNotify;
	Efi_Handle locateHandle;
	Efi_Handle locateDevicePath;
	Efi_Handle installConfigurationTable;
} Efi_Boot_Services;

#pragma pack(pop)

#define efi_handle_protocol(handle, protocol, interface) \
	efi_call(gBS->handleProtocol, handle, protocol, (void **)(handle))

#endif	// __LOLI_EFIBOOT_H_INC__
