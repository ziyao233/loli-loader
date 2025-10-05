// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/graphics.c
 */

#include <efi.h>
#include <eficall.h>
#include <efiboot.h>
#include <eficon.h>
#include <string.h>
#include <misc.h>

static struct gFBStatus {
	uint32_t height;
	uint32_t scanlineWidth;
	uint32_t pixelMask;
	volatile uint32_t *base;
	void (*drawPixel)(uint32_t x, uint32_t y, int fiil);
	void (*scrollUp)(void);
} gFBStatus;

int gGraphicsAvailable;

#define GLYPH_WIDTH			8
#define GLYPH_HEIGHT			16
#define GLYPH_BYTES			(GLYPH_HEIGHT * GLYPH_WIDTH / 8)
#define CONSOLE_WIDTH			80
#define CONSOLE_HEIGHT			24
#define MIN_HORIZONTAL_RESOLUTION	(CONSOLE_WIDTH * 8)
#define MIN_VERTICAL_RESOLUTION		(CONSOLE_HEIGHT * 16)

static size_t
console_line_bytes(void)
{
	return 4 * GLYPH_HEIGHT * gFBStatus.scanlineWidth;
}

static void
scroll_up(void)
{
	volatile uint32_t *p1 = gFBStatus.base;
	volatile uint32_t *p2 = gFBStatus.base + console_line_bytes() / 4;

	for (size_t i = 0;
	     i < console_line_bytes() * (CONSOLE_HEIGHT - 1) / 4;
	     i++)
		*(p1++) = *(p2++);

	for (size_t i = 0; i < console_line_bytes() / 4; i++)
		*(p1++) = 0;
}

static void
draw_pixel(uint32_t x, uint32_t y, int fill)
{
	volatile uint32_t *p = gFBStatus.base;

	p += y * gFBStatus.scanlineWidth;
	p += x;

	*p = fill ? gFBStatus.pixelMask : 0;
}

extern uint8_t gFont[];

static void
draw_char(char c)
{
	static int cursorX = 0;
	uint8_t *glyph = gFont + GLYPH_BYTES * c;

	switch (c) {
		case '\r':
			cursorX = 0;
			return;
		case '\n':
			gFBStatus.scrollUp();
			return;
		case '\b':
			if (!cursorX)
				return;

			cursorX--;

			/* Assume the glyph for '\b' is empty */
			break;
	}

	if (cursorX == CONSOLE_WIDTH) {
		cursorX = 0;
		gFBStatus.scrollUp();
	}

	uint32_t startY = GLYPH_HEIGHT * (CONSOLE_HEIGHT - 1);
	uint32_t startX = GLYPH_WIDTH * cursorX;
	for (uint32_t y = 0; y < GLYPH_HEIGHT; y++)
		for (uint32_t x = 0; x < GLYPH_WIDTH; x++)
			gFBStatus.drawPixel(x + startX, y + startY,
					    glyph[y] & (1 << (7 - x)));

	if (c != '\b')
		cursorX++;
}

void
graphics_write(const char *buf)
{
	if (!gGraphicsAvailable)
		return;

	while (*buf) {
		if (*buf >= 0 && *buf <= 127)
			draw_char(*buf);
		else
			draw_char(' ');

		buf++;
	}
}

void
graphics_init(void)
{
	Efi_Guid gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	Efi_Graphics_Output_Protocol *gop = NULL;
	Efi_Status ret;

	ret = efi_call(gBS->locateProtocol, &gopGuid, NULL, (void **)&gop);
	ret = EFI_ERRNO(ret);
	if (ret == EFI_NOT_FOUND) {
		pr_info("Skip graphics initialization: GOP isn't supported\n");
		return;
	} else if (ret) {
		pr_err("locateProtocol returns %lu for GOP\n", ret);
		panic("Failed to locate GOP handle");
	}

	uint32_t mode;
	Efi_Graphics_Output_Mode_Info *info = NULL;
	for (mode = 0; mode < gop->mode->maxMode; mode++) {
		uint_native size;
		ret = efi_method(gop, queryMode, mode, &size, &info);
		if (ret) {
			pr_err("Failed to query GOP mode %u: %d\n", mode, ret);
			return;
		}

		if (info->pixelFormat != PIXEL_RGB_RESERVED_8888 &&
		    info->pixelFormat != PIXEL_BGR_RESERVED_8888)
			continue;

		if (info->horizontalRes >= MIN_HORIZONTAL_RESOLUTION &&
		    info->verticalRes >= MIN_VERTICAL_RESOLUTION)
			break;
	}

	if (mode == gop->mode->maxMode) {
		pr_info("Skip graphics initialization: No suitable mode\n");
		return;
	}

	ret = efi_method(gop, setMode, mode);
	if (ret) {
		pr_err("Failed to setup GOP mode %u: %d\n", mode, ret);
		return;
	}

	pr_info("Graphics initialized: GOP mode = %u\n", mode);
	pr_info("Resolution %ux%u\n", info->horizontalRes, info->verticalRes);

	gFBStatus = (struct gFBStatus) {
			.base		= gop->mode->fbBase,
			.pixelMask	= 0x00ffffff,
			.height		= gop->mode->info->verticalRes,
			.scanlineWidth	= gop->mode->info->pixelPerScanline,
			.drawPixel	= draw_pixel,
			.scrollUp	= scroll_up,
		    };

	volatile uint32_t *p = gFBStatus.base;
	for (size_t i = 0; i < gop->mode->fbSize; i++)
		*p = 0;

	gGraphicsAvailable = 1;
}
