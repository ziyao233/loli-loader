// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/efi.c
 *	Copyright (c) 2024 Yao Zi.
 */

#include <efidef.h>
#include <eficall.h>
#include <efi.h>

Efi_System_Table *gST;
Efi_Boot_Services *gBS;
Efi_Handle gSelf;

void efi_init(Efi_Handle imageHandle, Efi_System_Table *st)
{
	gST = st;
	gBS = gST->bootServices;
	gSelf = imageHandle;
}

