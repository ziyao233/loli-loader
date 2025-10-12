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
#include <fdt.h>
#include <graphics.h>
#include <serial.h>
#include <initrd.h>

#define LOLI_CFG "loli.cfg"

void
printn(const char *p, size_t len)
{
	while (len--)
		printf("%c", *(p++));
}

typedef struct {
	Efi_Handle kernelHandle;
} Boot_Entry;

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
	if (!line) {
		printf("\nBoot entry 0 by default\n");
		return 0;
	}

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


static void
setup_append(Efi_Handle kernelHandle, char *append)
{
	Efi_Loaded_Image_Protocol *kernelImage = NULL;

	efi_handle_protocol(kernelHandle, EFI_LOADED_IMAGE_PROTOCOL_GUID,
			    &kernelImage);

	size_t appendLen = strlen(append) + 1;
	size_t wAppendLen = appendLen * sizeof(wchar_t);
	wchar_t *wAppend = malloc(wAppendLen);

	str2wcs(wAppend, append);

	kernelImage->loadOptions	= wAppend;
	kernelImage->loadOptionSize	= wAppendLen;
}

static int
load_efi_image(Boot_Entry *entry, void *kernelBase, int64_t kernelSize)
{
	Efi_Memory_Mapped_Device_Path devpath[2];
	devpath[0].head.type	= EFI_DEVICE_HARDWARE;
	devpath[0].head.subtype	= EFI_DEVICE_SUBTYPE_MEMORY_MAPPED;
	devpath[0].head.length	= sizeof(devpath[0]);
	devpath[0].memtype	= EFI_LOADER_DATA;
	devpath[0].startAddr	= (uint64_t)kernelBase;
	devpath[0].endAddr	= (uint64_t)kernelBase + kernelSize;

	devpath[1].head.type	= EFI_DEVICE_END;
	devpath[1].head.subtype	= EFI_DEVICE_SUBTYPE_ENTIRE_END;
	devpath[1].head.length	= sizeof(Efi_Device_Path_Protocol);

	return efi_call(gBS->loadImage, 0, gSelf,
			(Efi_Device_Path_Protocol *)devpath,
			kernelBase, kernelSize,
			&entry->kernelHandle) != EFI_SUCCESS;
}

static int
load_and_validate_entry(const char *p, Boot_Entry *entry)
{
	/* Releasing of temporary objects is delayed until everything sets up */
	char *kernel = get_pair(p, "kernel");
	if (!kernel) {
		pr_err("No kernel defined for the entry!\n");
		goto out_err;
	}

	int64_t kernelSize = file_get_size(kernel);
	if (kernelSize < 0) {
		pr_err("Can't load kernel %s\n", kernel);
		goto out_err;
	}

	void *kernelBase = malloc_pages(kernelSize);
	int64_t ret = file_load(kernel, &kernelBase);
	if (ret < 0) {
		pr_err("Can't load kernel %s\n", kernel);
		goto free_kernel;
	}

	if (load_efi_image(entry, kernelBase, kernelSize)) {
		pr_err("Can't load kernel %s\n", kernel);
		goto free_kernel_base;
	}

	pr_info("Kernel %s, size = %lu\n", kernel, kernelSize);

	char *initrd = get_pair(p, "initrd");
	if (initrd) {
		int64_t initrdSize = file_get_size(initrd);
		if (initrdSize < 0) {
			pr_err("Can't load initrd %s\n", initrd);
			goto unload_image;
		}

		void *initrdBase = malloc_pages(initrdSize);
		initrdSize = file_load(initrd, &initrdBase);
		if (initrdSize < 0) {
			pr_err("Can't load initrd %s\n", initrd);
			goto unload_image;
		}

		pr_info("Initrd %s, size = %lu\n", initrd, initrdSize);

		initrd_setup(initrdBase, initrdSize);
	} else {
		pr_info("Initrd: (none)\n");
	}

	char *append = get_pair(p, "append");
	pr_info("Append: %s\n", append ? append : "(none)");

	setup_append(entry->kernelHandle, append);

	free(kernel);
	free(append);

	return 0;
unload_image:
	efi_call(gBS->unloadImage, entry->kernelHandle);
free_kernel_base:
	free_pages(kernelBase, kernelSize);
free_kernel:
	free(kernel);
out_err:
	return -1;
}

Efi_Status
main(Efi_Handle imageHandle, Efi_System_Table *st)
{
	efi_init(imageHandle, st);

	interaction_init();

	printf("loli bootloader is initializing\n");

	serial_init();
	graphics_init();

	printf("loli bootloader (%s built)\n", __DATE__);

	int64_t cfgSize = file_get_size(LOLI_CFG);
	if (cfgSize < 0)
		panic("Can't retrive size of configuration");

	char *cfg = malloc(cfgSize + 1);
	cfgSize = file_load(LOLI_CFG, (void **)&cfg);
	if (cfgSize < 0)
		panic("Can't load configuration");
	cfg[cfgSize] = '\0';

	int timeout = retrieve_timeout(cfg);
	int entryNum = 0;
	for (const char *p = extlinux_next_entry(cfg, NULL);
	     p;
	     p = extlinux_next_entry(NULL, p)) {
		size_t namelen = 0;
		const char *name = extlinux_get_value(p, "label", &namelen);

		printf("%d: ", entryNum);
		printn(name, namelen);
		printf("\n");

		entryNum++;
	}

	const char *entry = NULL;
	Boot_Entry bootEntry = { NULL };
	for (int selectedEntry = wait_for_boot_entry(timeout); ;
	     selectedEntry = wait_for_boot_entry(0)) {
		timeout = 0;
		if (!entryNum)
			continue;

		if (selectedEntry >= 0 && selectedEntry < entryNum) {
			entry = get_entry(cfg, selectedEntry);
			if (!load_and_validate_entry(entry, &bootEntry))
				break;
			else
				pr_err("Invalid entry\n");
		}
	}

	fdt_fixup();

	int ret = efi_call(gBS->startImage, bootEntry.kernelHandle, NULL, NULL);
	pr_err("Failed to start image: %d\n", ret);
	panic("Cannot boot selected entry");

	return ret;
}
