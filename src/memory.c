// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/memory.c
 *	Copyright (c) 2024 Yao Zi.
 */

#include <efi.h>
#include <string.h>

#include <memory.h>

void *
malloc(size_t size)
{
	void *addr;
	int ret = efi_call(gBS->allocatePool, EFI_LOADER_DATA,
			   (uint_native)size, &addr);

	return ret == EFI_SUCCESS ? addr : NULL;
}

void
free(void *p)
{
	if (p)
		efi_call(gBS->freePool, p);
}

void *
realloc(void *p, size_t oldSize, size_t size)
{
	void *new = NULL;
	if (size) {
		new = malloc(size);
		memcpy(new, p, oldSize);
	}
	free(p);
	return new;
}

#define SIZE_TO_EFI_PAGES(size) ((size + 4095) >> 12)

void *
malloc_pages(size_t size)
{
	void *addr;
	int ret = efi_call(gBS->allocatePages, 0, EFI_LOADER_DATA,
			   SIZE_TO_EFI_PAGES(size),
			   &addr);
	return ret == EFI_SUCCESS ? addr : NULL;
}

void
free_pages(void *p, size_t size)
{
	if (p)
		efi_call(gBS->freePages, p, SIZE_TO_EFI_PAGES(size));
}
