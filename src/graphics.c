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
#include <memory.h>
#include <misc.h>

static struct gFBStatus {
	uint32_t height;
	uint32_t scanlineWidth;
	uint32_t pixelMask;
	volatile void *base;
	void *buf;
	void (*drawPixel)(uint32_t x, uint32_t y, int fiil);
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
	return GLYPH_HEIGHT * gFBStatus.scanlineWidth * 4;
}

static void
fb_memset(volatile void *dst, int c, size_t n)
{
	volatile uint8_t *p = dst;

	while (n--)
		*(p++) = c;
}

static void
fb_memcpy(volatile void *dst, void *src, size_t n)
{
	volatile uint8_t *pDst = dst, *pSrc = src;

	while (n--)
		*(pDst++) = *(pSrc++);
}

static void
scroll_up(void)
{
	size_t moveSize = console_line_bytes() * (CONSOLE_HEIGHT - 1);
	volatile uint8_t *base = gFBStatus.base;
	uint8_t *buf = gFBStatus.buf;

	fb_memcpy(base, buf + console_line_bytes(), moveSize);
	fb_memset(base + moveSize, 0, console_line_bytes());

	memmove(buf, buf + console_line_bytes(), moveSize);
	memset(buf + moveSize, 0, console_line_bytes());
}

static void
draw_pixel(uint32_t x, uint32_t y, int fill)
{
	volatile uint32_t *base = gFBStatus.base;
	uint32_t *buf = gFBStatus.buf;

	uint32_t offset = y * gFBStatus.scanlineWidth + x;
	uint32_t data = fill ? gFBStatus.pixelMask : 0;

	base[offset] = buf[offset] = data;
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
			scroll_up();
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
		scroll_up();
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

static int
fbmode_is_supported(Efi_Graphics_Output_Mode_Info *info)
{
	if (info->horizontalRes < MIN_HORIZONTAL_RESOLUTION ||
	    info->verticalRes < MIN_VERTICAL_RESOLUTION)
		return 0;

	switch (info->pixelFormat) {
	case PIXEL_RGB_RESERVED_8888:
	case PIXEL_BGR_RESERVED_8888:
		return 1;
	default:
		break;
	};

	return 0;
}

static int
gop_determine_mode(Efi_Graphics_Output_Protocol *gop,
		   Efi_Graphics_Output_Mode_Info **info)
{
	for (int mode = 0; mode < gop->mode->maxMode; mode++) {
		uint_native size;
		int ret = efi_method(gop, queryMode, mode, &size, info);
		if (ret) {
			pr_err("Failed to query GOP mode %u: %d\n", mode, ret);
			return -1;
		}

		if (fbmode_is_supported(*info))
			return mode;
	}

	return -1;
}

static int
gop_setup_mode(Efi_Graphics_Output_Protocol *gop,
	       int mode, Efi_Graphics_Output_Mode_Info *info)
{
	int ret = efi_method(gop, setMode, mode);
	if (ret) {
		pr_err("Failed to set GOP to mode %u: %d\n", mode, ret);
		return -1;
	}

	void *buf = malloc_pages(gop->mode->fbSize);
	if (!buf) {
		pr_err("Failed to allocate framebuffer for GOP\n");
		return -1;
	}

	gFBStatus = (struct gFBStatus) {
			.height		= gop->mode->info->verticalRes,
			.scanlineWidth	= gop->mode->info->pixelPerScanline,
			.pixelMask	= 0x00ffffff,
			.base		= gop->mode->fbBase,
			.buf		= buf,
			.drawPixel	= draw_pixel,
		    };

	fb_memset(gFBStatus.base, 0, gop->mode->fbSize);
	memset(gFBStatus.buf, 0, gop->mode->fbSize);

	return 0;
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

	Efi_Graphics_Output_Mode_Info *info = NULL;
	int mode = gop_determine_mode(gop, &info);
	if (mode < 0) {
		pr_info("Skip graphics initialization: No suitable mode\n");
		return;
	}

	pr_info("Graphics GOP mode = %u\n", mode);
	pr_info("Resolution %ux%u\n", info->horizontalRes, info->verticalRes);

	if (gop_setup_mode(gop, mode, info)) {
		pr_err("Failed to setup GOP mode\n");
		return;
	}

	pr_info("Graphics initialized\n");

	gGraphicsAvailable = 1;
}
