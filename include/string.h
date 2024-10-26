// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/string.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_STRING_H_INC__
#define __LOLI_STRING_H_INC__

#include <efidef.h>
#include <stdarg.h>

size_t strlen(const char *p);
char *strcpy(char *dst, const char *src);

size_t wcslen(const wchar_t *p);
wchar_t *wcscpy(wchar_t *dst, wchar_t *src);

size_t wcs2str(char *str, const wchar_t *wcs);
size_t str2wcs(wchar_t *wcs, const char *str);

void vsprintf(char *p, const char *format, va_list va);

#endif	// __LOLI_STRING_H_INC__
