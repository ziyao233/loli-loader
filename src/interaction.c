// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/interaction.c
 *	Copyright (c) 2024 Yao Zi.
 */

#include <stdarg.h>
#include <string.h>

#include <efi.h>
#include <eficall.h>
#include <eficon.h>

#include <interaction.h>

void
printf(const char *format, ...)
{
	va_list va;
	va_start(va, format);

	char buf[256];
	vsprintf(buf, format, va);

	wchar_t buf2[256];
	str2wcs(buf2, buf);

	efi_method(gST->conOut, outputString, buf2);

	va_end(va);
}
