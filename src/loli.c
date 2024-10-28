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

	printf("Length of test.txt is %ld\n", file_get_size("test.txt"));
	char *buf;
	int64_t size = file_load("test.txt", (void **)&buf);
	printf("test.txt content:\n\t");
	for (int i = 0; i < size; i++)
		printf("%c", buf[i]);

	return EFI_SUCCESS;
}
