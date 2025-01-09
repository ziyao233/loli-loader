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

void
printn(const char *p, size_t len)
{
	while (len--)
		printf("%c", *(p++));
}

Efi_Status
_start(Efi_Handle imageHandle, Efi_System_Table *st)
{
	efi_init(imageHandle, st);

	efi_method(gST->conOut, outputString, L"Hello world\n");
	efi_method(gST->conOut, outputString, L"Hello world2\n");

	Efi_Loaded_Image_Protocol *loadedImage;
	efi_handle_protocol(imageHandle, EFI_LOADED_IMAGE_PROTOCOL_GUID,
			    &loadedImage);
	printf("loaded image: %p\n", loadedImage);

	printf("image base = %p, image size = %lu\n",
	       loadedImage->imageBase, loadedImage->imageSize);

	printf("Hello world %d %x %lx %u\n", -1, -1, 0xffffffffffffffff, -1);
	printf("%x %d\n", 0x12345a5a, 1234);
	printf("%d %d\n", 0, atou("12345678"));

	printf("Length of test.txt is %ld\n", file_get_size("test.txt"));
	char *buf = NULL;
	int64_t size = file_load("test.txt", (void **)&buf);
	printf("test.txt content:\n\t");
	printn(buf, size);
	free(buf);

	size = file_get_size("loli.cfg");
	printf("Length of extlinux.conf is %ld\n", size);
	buf = malloc(size + 1);
	file_load("loli.cfg", (void **)&buf);
	printf("loli.cfg content:\n");
	printn(buf, size);
	buf[size] = '\0';
	printf("loli.cfg boot entries\n");
	int entryNum = 0;
	for (const char *p = extlinux_next_entry(buf, NULL);
	     p;
	     p = extlinux_next_entry(NULL, p)) {
		size_t namelen = 0;

		const char *name = extlinux_get_value(p, "label", &namelen);
		printf("%d: ", entryNum + 1);
		printn(name, namelen);
		printf("\n");

		entryNum++;
	}

	char *line = getline_timeout(5);
	if (!line)
		printf("TIMEOUT!\n");
	else
		printf("read line `%s`\n", line);
	free(line);


	void *rawImage = NULL;
	int64_t imageSize = file_load("vmlinux-riscv", &rawImage);
	if (imageSize < 0)
		printf("failed to read the image\n");

	Efi_Memory_Mapped_Device_Path devpath[2];
	devpath[0].head.type	= EFI_DEVICE_HARDWARE;
	devpath[0].head.subtype	= EFI_DEVICE_SUBTYPE_MEMORY_MAPPED;
	devpath[0].head.length	= sizeof(devpath[0]);
	devpath[0].memtype	= EFI_LOADER_DATA;
	devpath[0].startAddr	= (uint64_t)rawImage;
	devpath[0].endAddr	= (uint64_t)rawImage + imageSize;

	devpath[1].head.type	= EFI_DEVICE_END;
	devpath[1].head.subtype	= EFI_DEVICE_SUBTYPE_ENTIRE_END;
	devpath[1].head.length	= sizeof(Efi_Device_Path_Protocol);

	Efi_Handle newImage = NULL;
	pr_info("%s: parent = 0x%lx, devpath = 0x%lx, rawImage = 0x%lx\n",
		__func__, gSelf, devpath, rawImage);
	pr_info("%s: imageSize = %lu, &newImage = 0x%lx\n",
		__func__, imageSize, &newImage);
	int ret = efi_call(gBS->loadImage, 0, gSelf,
					   (Efi_Device_Path_Protocol *)devpath,
					   rawImage, imageSize, &newImage);
	if (ret != EFI_SUCCESS)
		printf("failed to load the image, ret = %d\n", ret);

	efi_call(gBS->startImage, newImage, 0, NULL);

	return EFI_SUCCESS;
}
