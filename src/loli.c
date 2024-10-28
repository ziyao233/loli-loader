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
#include <memory.h>
#include <efiloadedimage.h>

Efi_Status
_start(Efi_Handle imageHandle, Efi_System_Table *st)
{
	efi_init(imageHandle, st);

	efi_method(gST->conOut, outputString, L"Hello world\n");
	efi_method(gST->conOut, outputString, L"Hello world2\n");

	void *addr = malloc(32);
	if (!addr) {
		printf("allocate failed\n");
	} else {
		printf("addr: %p\n", addr);
		free(addr);
	}

	Efi_Loaded_Image_Protocol *loadedImage;
	efi_handle_protocol(imageHandle, EFI_LOADED_IMAGE_PROTOCOL_GUID,
			    &loadedImage);
	printf("loaded image: %p\n", loadedImage);

	printf("image base = %p, image size = %lu\n",
	       loadedImage->imageBase, loadedImage->imageSize);

	printf("Hello world %d %x %lx %u\n", -1, -1, 0xffffffffffffffff, -1);
	printf("%x %d\n", 0x12345a5a, 1234);

	return EFI_SUCCESS;
}
