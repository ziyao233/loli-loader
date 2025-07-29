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
#include <efidevicepath.h>

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

#define EVT_TIMER			0x80000000
#define EVT_NOTIFY_WAIT			0x00000100

typedef enum {
	EFI_TIMER_CANCEL = 0,
	EFI_TIMER_PERIODIC,
	EFI_TIMER_RELATIVE,
} Efi_Type_Delay;

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

	Efi_Status (*createEvent)(uint32_t type, Efi_Tpl notifyTpl,
				  void *notifyFunction, void *notifyContext,
				  Efi_Event *event);
	Efi_Status (*setTimer)(Efi_Event event, Efi_Type_Delay type,
			       uint64_t triggerTime);
	Efi_Status (*waitForEvent)(uint_native numberOfEvents, Efi_Event *events,
				   uint_native *index);
	Efi_Handle signalEvent;
	Efi_Status (*closeEvent)(Efi_Event event);
	Efi_Handle checkEvent;

	Efi_Handle installProtocolInterface;
	Efi_Handle reinstallProtocolInterface;
	Efi_Handle uninstallProtocolInterface;
	Efi_Status (*handleProtocol)(Efi_Handle handle, Efi_Guid *protocol,
				     void **interface);
	Efi_Handle reserved;
	Efi_Handle registerProtocolNotify;
	Efi_Handle locateHandle;
	Efi_Handle locateDevicePath;
	Efi_Status (*installConfigurationTable)(Efi_Guid *guid, void *table);

	Efi_Status (*loadImage)(bool bootPolicy, Efi_Handle parentHandle,
				Efi_Device_Path_Protocol *devicePath,
				void *sourceBuffer, uint64_t sourceSize,
				Efi_Handle *imageHandle);
	Efi_Status (*startImage)(Efi_Handle imageHandle,
				 uint_native *exitDataSize,
				 wchar_t **exitdata);
	Efi_Handle exit;
	Efi_Status (*unloadImage)(Efi_Handle imageHandle);
	Efi_Handle exitBootServices;

	Efi_Handle getNextMonotonicCount;
	Efi_Handle stall;
	Efi_Handle setWatchdogTimer;

	Efi_Handle connectController;
	Efi_Handle disconnectController;

	Efi_Handle openProtocol;
	Efi_Handle closeProtocol;
	Efi_Handle openProtocolInformation;

	Efi_Handle protocolsPerHandle;
	Efi_Handle locateHandleBuffer;
	Efi_Status (*locateProtocol)(Efi_Guid *protocol, void *registration,
				     void **interfaces);
	Efi_Handle installMultipleProtocolInterfaces;
	Efi_Handle uninstallMultipleProtocolInterfaces;
} Efi_Boot_Services;

#pragma pack(pop)

#define efi_handle_protocol(handle, protocol, interface) do { \
	Efi_Guid _guid = protocol;				\
	efi_call(gBS->handleProtocol, handle, &_guid,		\
		 (void **)(interface));				\
} while (0)

#define efi_install_configuration_table(guid, table) do { \
	Efi_Guid _guid = guid;						\
	efi_call(gBS->installConfigurationTable, &_guid, table);	\
} while (0)

#endif	// __LOLI_EFIBOOT_H_INC__
