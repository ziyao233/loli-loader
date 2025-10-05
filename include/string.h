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
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

size_t wcslen(const wchar_t *p);
wchar_t *wcscpy(wchar_t *dst, wchar_t *src);

size_t wcs2str(char *str, const wchar_t *wcs);
size_t str2wcs(wchar_t *wcs, const char *str);

void vsprintf(char *p, const char *format, va_list va);

void *memcpy(void *dst, void *src, size_t n);
void *memmove(void *dst, void *src, size_t n);
void *memset(void *mem, int c, size_t n);
int atou(const char *p);

size_t strscpy(char *dst, const char *src, size_t len);

int memcmp(void *a, void *b, size_t len);

#endif	// __LOLI_STRING_H_INC__
