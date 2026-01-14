// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/fdt.c
 *	Copyright (C) 2025 Yao Zi <ziyao@disroot.org>
 */

#include <string.h>

#include <efi.h>
#include <eficall.h>
#include <efiboot.h>
#include <efidevicetree.h>

#include <fdt.h>
#include <interaction.h>
#include <memory.h>
#include <misc.h>

static uint32_t
be32_to_cpu(uint32_t val)
{
	uint8_t *data = (uint8_t*)&val;
	return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

static Fdt_Header *
search_for_devicetree(void)
{
	Fdt_Header *fdt = NULL;

	for (uint_native i = 0; i < gST->numberOfTableEntries; i++) {
		Efi_Configuration_Table *t = gST->configurationTable + i;

		Efi_Guid dtbGuid = EFI_DTB_TABLE_GUID;
		if (!memcmp(&dtbGuid, &t->vendorGuid, sizeof(Efi_Guid))) {
			fdt = t->vendorTable;
			break;
		}
	}

	if (!fdt)
		return NULL;

	return fdt;
}

void
fdt_fixup_and_load(Fdt_Header *fdt)
{
	/* TODO: check compatibility */
	size_t fdtSize = be32_to_cpu(fdt->totalSize);

	pr_info("devicetree: size %lu\n", fdtSize);

	/* TODO: don't use a hard size limit */
	size_t copySize = fdtSize + 4096;
	Fdt_Header *copy = malloc_type(copySize, EFI_ACPI_RECLAIM_MEMORY);
	memcpy(copy, fdt, fdtSize);

	Efi_Dt_Fixup_Protocol *dtp = NULL;
	Efi_Guid dtFixupGuid = EFI_DT_FIXUP_PROTOCOL_GUID;
	int ret = efi_call(gBS->locateProtocol, &dtFixupGuid, NULL, (void **)&dtp);
	if (!dtp) {
		free(copy);
		pr_info("EFI_DT_FIXUP_PROTOCOL isn't supported, "
			"apply no fixup\n");
		return;
	}

	ret = efi_method(dtp, fixup, copy, &copySize,
					 EFI_DT_APPLY_FIXES | EFI_DT_RESERVE_MEMORY);
	if (ret == EFI_SUCCESS)
		pr_info("devicetree: applied fixes\n");
	else
		pr_info("devicetree: failed to apply fixes\n");

	efi_install_configuration_table(EFI_DTB_TABLE_GUID, copy);
}

void
fdt_fixup(void)
{
	Fdt_Header *fdt = search_for_devicetree();

	if (!fdt) {
		pr_info("DeviceTree: not found\n");
		return;
	}

	fdt_fixup_and_load(fdt);
}
