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
#include <efiboot.h>
#include <eficon.h>

#include <interaction.h>
#include <misc.h>
#include <memory.h>

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

int
getchar_timeout(int timeout)
{
	Efi_Event events[2] = { gST->conIn->waitForKey };
	uint_native eventNum = 1;

	if (timeout) {
		if (efi_call(gBS->createEvent, EVT_TIMER, 0, NULL, NULL,
			     &events[1]) != EFI_SUCCESS)
			panic();

		if (efi_call(gBS->setTimer, events[1], EFI_TIMER_RELATIVE,
			     timeout * 1000 * 1000 * 10) != EFI_SUCCESS)
			panic();

		eventNum++;
	}

	uint_native index = 0;
	if (efi_call(gBS->waitForEvent, eventNum, events, &index) != EFI_SUCCESS)
		panic();

	if (timeout) {
		efi_call(gBS->closeEvent, events[1]);

		if (index == 1)
			return EOF;
	}

	Efi_Input_Key key;
	if (efi_method(gST->conIn, readKeyStroke, &key) != EFI_SUCCESS)
		panic();

	return key.unicodeChar;
}

char *
getline_timeout(int timeout)
{
	size_t buflen = 16, strlen = 0;
	char *s = malloc(buflen);
	int c;

	do {
		c = getchar_timeout(timeout);
		timeout = 0;

		if (c == EOF)
			return NULL;

		if (!c)
			continue;

		printf("%c", c);

		strlen++;
		if (strlen == buflen) {
			buflen += 32;
			s = realloc(s, strlen, buflen);
		}

		s[strlen - 1] = c;
	} while (c != '\r' && c != '\n');
	printf("\n");

	s[strlen - 1] = '\0';
	return s;
}
