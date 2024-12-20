// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/memory.c
 *	Copyright (c) 2024 Yao Zi.
 */

#include <efi.h>

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
	efi_call(gBS->freePool, p);
}
