// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/interaction.c
 *	Copyright (c) 2024-2025 Yao Zi.
 */

#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include <efi.h>
#include <eficall.h>
#include <efiboot.h>
#include <eficon.h>

#include <interaction.h>
#include <misc.h>
#include <memory.h>
#include <serial.h>
#include <graphics.h>

static char *gFormatBuf, *gLineEndConvertBuf;
static wchar_t *gWideCharBuf;

void
interaction_init(void)
{
	gFormatBuf = malloc(sizeof(*gFormatBuf) * 1024);
	gLineEndConvertBuf = malloc(sizeof(*gLineEndConvertBuf) * 1024);
	gWideCharBuf = malloc(sizeof(*gWideCharBuf) * 1024);
}

void
printf(const char *format, ...)
{
	va_list va;
	va_start(va, format);

	vsprintf(gFormatBuf, format, va);

	const char *src = gFormatBuf;
	char *dst = gLineEndConvertBuf;
	while (*src) {
		if (*src == '\n')
			*(dst++) = '\r';
		*(dst++) = *(src++);
	}
	*dst = '\0';

	if (!gSerialAvailable && !gGraphicsAvailable) {
		str2wcs(gWideCharBuf, gLineEndConvertBuf);

		efi_method(gST->conOut, outputString, gWideCharBuf);
	}

	if (gSerialAvailable)
		serial_write(gLineEndConvertBuf);

	if (gGraphicsAvailable)
		graphics_write(gLineEndConvertBuf);

	va_end(va);
}

static int
getchar_translate(Efi_Input_Key *key)
{
	if (key->unicodeChar)
		return key->unicodeChar;

	switch (key->scanCode) {
	case 0x8:
		/*
		 * EFI scan code for DEL. When x86_64 QEMU is running with
		 * -nographics and the vendored EDK2 firmware, a Backspace may
		 * be translated to
		 * (Efi_Input_Key) { .unicodeChar = 0, scanCode = 0x8 }, i.e.
		 *
		 * It's still under investigation whether it's a bug or not,
		 * but it won't hurt to workaround it for now.
		 */
		return '\b';
	}

	return 0;
}

int
getchar_timeout(int timeout)
{
	Efi_Event events[2] = { gST->conIn->waitForKey };
	uint_native eventNum = 1;

	if (timeout) {
		if (efi_call(gBS->createEvent, EVT_TIMER, 0, NULL, NULL,
			     &events[1]) != EFI_SUCCESS)
			panic("Can't create timer event");

		if (efi_call(gBS->setTimer, events[1], EFI_TIMER_RELATIVE,
			     timeout * 1000 * 1000 * 10) != EFI_SUCCESS)
			panic("Can't configure timer");

		eventNum++;
	}

	uint_native index = 0;
	if (efi_call(gBS->waitForEvent, eventNum, events, &index) != EFI_SUCCESS)
		panic("error occurs when waiting for events");

	if (timeout) {
		efi_call(gBS->closeEvent, events[1]);

		if (index == 1)
			return EOF;
	}

	Efi_Input_Key key;
	if (efi_method(gST->conIn, readKeyStroke, &key) != EFI_SUCCESS)
		panic("Can't read inputs");

	return getchar_translate(&key);
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

		if (c == L'\b' && strlen) {
			printf("\b \b");

			s[strlen - 1] = '\0';
			strlen--;
		} else if (!isprint(c)) {
			continue;
		} else {
			printf("%c", c);

			strlen++;
			if (strlen == buflen) {
				buflen += 32;
				s = realloc(s, strlen, buflen);
			}

			s[strlen - 1] = c;
		}
	} while (c != '\r' && c != '\n');
	printf("\n");

	s[strlen] = '\0';
	return s;
}
