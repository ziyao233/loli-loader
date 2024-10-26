// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/loli.c
 *	Copyright (c) 2024 Yao Zi.
 */

#include <efidef.h>
#include <eficall.h>
#include <efi.h>

Efi_Status
_start(Efi_Handle imageHandle, Efi_System_Table *st)
{
	efi_init(imageHandle, st);

	efi_method(gST->conOut, outputString, L"Hello world\n");
	efi_method(gST->conOut, outputString, L"Hello world2\n");

	return EFI_SUCCESS;
}
