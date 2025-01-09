// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/misc.c
 *	Copyright (c) 2024 Yao Zi.
 */

#include <efi.h>
#include <eficall.h>
#include <stdarg.h>
#include <interaction.h>

#include <misc.h>

void
panic(const char *msg)
{
	printf("PANIC:");

	wchar_t tmp[2] = { 0, 0 };
	while (*msg) {
		tmp[0] = *msg;
		efi_method(gST->conOut, outputString, tmp);
		msg++;
	}

	printf("PANIC: can't boot\n");
	while (1) ;
}
