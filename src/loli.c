// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/loli.c
 *	Copyright (c) 2024 Yao Zi.
 */

#include <efidef.h>
#include <eficall.h>
#include <efi.h>
#include <efiboot.h>
#include <interaction.h>
#include <efiloadedimage.h>
#include <memory.h>
#include <file.h>
#include <string.h>
#include <extlinux.h>
#include <misc.h>

#define LOLI_CFG "loli.cfg"

void
printn(const char *p, size_t len)
{
	while (len--)
		printf("%c", *(p++));
}

static char *
get_pair(const char *cfg, const char *key)
{
	size_t len = 0;
	const char *res = extlinux_get_value(cfg, key, &len);

	if (!res)
		return 0;

	char *copy = malloc(len + 1);
	strscpy(copy, res, len + 1);

	return copy;
}

int
retrieve_timeout(const char *cfg)
{
	char *res = get_pair(cfg, "timeout");
	if (!res)
		return 0;

	int timeout = atou(res);
	if (timeout < 0) {
		pr_warn("invalid timeout \"%s\", use 0 instead\n", res);
		timeout = 0;
	}

	free(res);

	return timeout;
}

int
wait_for_boot_entry(int timeout)
{
	printf("Enter entry number: ");
	char *line = getline_timeout(timeout);

	/* TODO: support "default" option */
	if (!line)
		return 0;

	int entry = atou(line);
	free(line);

	return entry >= 0 ? entry : -1;
}

static const char *
get_entry(const char *cfg, int index)
{
	const char *entry = extlinux_next_entry(cfg, NULL);

	while (index--)
		entry = extlinux_next_entry(NULL, entry);

	return entry;
}

Efi_Status
_start(Efi_Handle imageHandle, Efi_System_Table *st)
{
	efi_init(imageHandle, st);

	printf("loli bootloader (%s built)\n", __DATE__);

	int64_t cfgSize = file_get_size(LOLI_CFG);
	if (cfgSize < 0)
		panic("Can't retrive size of configuration");

	char *cfg = malloc(cfgSize + 1);
	cfgSize = file_load(LOLI_CFG, (void **)&cfg);
	if (cfgSize < 0)
		panic("Can't load configuration");

	int timeout = retrieve_timeout(cfg);
	int entryNum = 0;
	for (const char *p = extlinux_next_entry(cfg, NULL);
	     p;
	     p = extlinux_next_entry(NULL, p)) {
		size_t namelen = 0;
		const char *name = extlinux_get_value(cfg, "label", &namelen);

		printf("%d: ", entryNum);
		printn(name, namelen);
		printf("\n");

		entryNum++;
	}

	const char *entry = NULL;
retry:
	for (int selectedEntry = wait_for_boot_entry(timeout); ;
	     selectedEntry = wait_for_boot_entry(0)) {
		timeout = 0;
		if (selectedEntry >= 0 && selectedEntry < entryNum) {
			entry = get_entry(cfg, selectedEntry);
			break;
		}
	}

	char *sKernel = get_pair(entry, "kernel");
	if (!sKernel) {
		pr_err("No kernel defined for the entry!\n");
		goto retry;
	}

	void *rawKernel = NULL;
	int64_t kernelSize = file_load(sKernel, &rawKernel);
	free(sKernel);

	if (kernelSize < 0) {
		pr_err("Can't load kernel\n");
		goto retry;
	}

	Efi_Memory_Mapped_Device_Path devpath[2];
	devpath[0].head.type	= EFI_DEVICE_HARDWARE;
	devpath[0].head.subtype	= EFI_DEVICE_SUBTYPE_MEMORY_MAPPED;
	devpath[0].head.length	= sizeof(devpath[0]);
	devpath[0].memtype	= EFI_LOADER_DATA;
	devpath[0].startAddr	= (uint64_t)rawKernel;
	devpath[0].endAddr	= (uint64_t)rawKernel + kernelSize;

	devpath[1].head.type	= EFI_DEVICE_END;
	devpath[1].head.subtype	= EFI_DEVICE_SUBTYPE_ENTIRE_END;
	devpath[1].head.length	= sizeof(Efi_Device_Path_Protocol);

	Efi_Handle nextImage = NULL;
	int ret = efi_call(gBS->loadImage, 0, gSelf,
					   (Efi_Device_Path_Protocol *)devpath,
					   rawKernel, kernelSize, &nextImage);
	if (ret != EFI_SUCCESS)
		panic("Can't load kernel image!\n");

	return efi_call(gBS->startImage, nextImage, 0, NULL);
}
