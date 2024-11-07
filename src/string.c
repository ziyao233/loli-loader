// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/string.c
 *	Copyright (c) 2024 Yao Zi.
 */

#include <efidef.h>
#include <stdarg.h>

#include <string.h>

size_t
strlen(const char *p)
{
	size_t len = 0;
	while (*p++)
		len++;
	return len;
}

char *
strcpy(char *dst, const char *src)
{
	char *org = dst;
	while (*src)
		*(dst++) = *(src++);
	*dst = 0;
	return org;
}

int
strncmp(const char *s1, const char *s2, size_t n)
{
	while (*s1 && *s2) {
		if (!n)
			break;
		n--;
		if (*s1 != *s2)
			break;
		s1++;
		s2++;
	}
	return *s1 - *s2;
}

int
strcmp(const char *s1, const char *s2)
{
	while (*s1 && *s2) {
		if (*s1 != *s2)
			break;
		s1++;
		s2++;
	}
	return *s1 - *s2;
}

size_t
wcslen(const wchar_t *p)
{
	size_t len = 0;
	while (*p++)
		len++;
	return len;
}

wchar_t *
wcscpy(wchar_t *dst, wchar_t *src)
{
	wchar_t *org = dst;
	while (*src)
		*(dst++) = *(src++);
	*dst = 0;
	return org;
}

size_t
wcs2str(char *str, const wchar_t *wcs)
{
	size_t len = wcslen(wcs);
	if (!str)
		return len;

	while (*wcs)
		*(str++) = (char)*(wcs++);
	*str = 0;

	return len;
}

size_t
str2wcs(wchar_t *wcs, const char *str)
{
	size_t len = strlen(str);
	if (!wcs)
		return len;

	while (*str)
		*(wcs++) = (wchar_t)*(str++);
	*wcs = 0;

	return len;
}

static int
itoa_n(char *out, unsigned long int n, int base)
{
	static const char digits[] = "0123456789abcdef";

	if (n == 0) {
		out[0] = '0';
		return 1;
	}

	int len = 0;
	while (n) {
		out[len] = digits[n % base];
		n /= base;
		len++;
	}

	char tmp;
	for (int i = 0; i < len / 2; i++) {
		tmp = out[len - i - 1];
		out[len - i - 1] = out[i];
		out[i] = tmp;
	}

	return len;
}

void
vsprintf(char *p, const char *format, va_list va)
{
	bool longValue;
	int base = 10;
	long int value;
	while (*format)
	switch (*format) {
	case '%':
		format++;
		if (*format == '%') {
			*(p++) = *(format++);
			break;
		} else if (*format == 's') {
			format++;
			const char *src = va_arg(va, const char *);
			strcpy(p, src);
			p += strlen(src);
			break;
		} else if (*format == 'c') {
			format++;
			*(p++) = va_arg(va, int);
			break;
		}

		switch (*format) {
		case 'l':
			format++;
			// fallthrough
		case 'p':
			value = va_arg(va, long int);
			longValue = 1;
			break;
		default:
			value = va_arg(va, int);
			longValue = 0;
		}

		switch (*format) {
		case 'd':
			if (value < 0) {
				*(p++) = '-';
				value = -value;
			}
			// fallthrough
		case 'u':
			base = 10;
			break;
		case 'p':
			p[0] = '0';
			p[1] = 'x';
			p += 2;
			// fallthrough
		case 'x':
			base = 16;
			break;
		}
		format++;

		p += itoa_n(p, longValue ? value : (value & 0xffffffff), base);
		break;
	default:
		*(p++) = *(format++);
		break;
	}

	*p = '\0';
}

void *
memcpy(void *dst, void *src, size_t n)
{
	char *pDst = dst, *pSrc = src;
	while (n--)
		*(pDst++) = *(pSrc++);
	return dst;
}

void *
memset(void *mem, int c, size_t n)
{
	char *p = mem;
	while (n--)
		*(p++) = c;
	return mem;
}
