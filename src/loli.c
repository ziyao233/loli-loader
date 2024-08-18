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
efi_main(Efi_Handle *imageHandle, Efi_System_Table *st)
{
	// We need an EFI call wrapper

	efi_method(st->conOut, outputString, L"Hello world\n");

	return EFI_SUCCESS;
}
