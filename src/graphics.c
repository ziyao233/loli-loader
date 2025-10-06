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

typedef struct Frame_Buffer {
	uint32_t height;
	uint32_t scanlineWidth;
	uint32_t pixelMask;
	uint32_t bpp;
	volatile void *base;
	void *buf;
	uint32_t cursorX;
	void (*drawPixel)(struct Frame_Buffer *fb, uint32_t x, uint32_t y,
			  int fiil);
} Frame_Buffer;
static Frame_Buffer *gFBs;
static size_t gFBNum;

int gGraphicsAvailable;

#define GLYPH_WIDTH			8
#define GLYPH_HEIGHT			16
#define GLYPH_BYTES			(GLYPH_HEIGHT * GLYPH_WIDTH / 8)
#define CONSOLE_WIDTH			80
#define CONSOLE_HEIGHT			24
#define MIN_HORIZONTAL_RESOLUTION	(CONSOLE_WIDTH * 8)
#define MIN_VERTICAL_RESOLUTION		(CONSOLE_HEIGHT * 16)

static size_t
console_line_bytes(Frame_Buffer *fb)
{
	return GLYPH_HEIGHT * fb->scanlineWidth * fb->bpp / 8;
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
scroll_up(Frame_Buffer *fb)
{
	size_t moveSize = console_line_bytes(fb) * (CONSOLE_HEIGHT - 1);
	volatile uint8_t *base = fb->base;
	uint8_t *buf = fb->buf;

	fb_memcpy(base, buf + console_line_bytes(fb), moveSize);
	fb_memset(base + moveSize, 0, console_line_bytes(fb));

	memmove(buf, buf + console_line_bytes(fb), moveSize);
	memset(buf + moveSize, 0, console_line_bytes(fb));
}

static void
draw_pixel_32b(Frame_Buffer *fb, uint32_t x, uint32_t y, int fill)
{
	volatile uint32_t *base = fb->base;
	uint32_t *buf = fb->buf;

	uint32_t offset = y * fb->scanlineWidth + x;
	uint32_t data = fill ? fb->pixelMask : 0;

	base[offset] = buf[offset] = data;
}

static void
draw_pixel_anymask(Frame_Buffer *fb, uint32_t x, uint32_t y, int fill)
{
	volatile uint8_t *base = fb->base;
	uint8_t *buf = fb->buf;

	uint32_t offset = y * fb->scanlineWidth + x;
	offset *= fb->bpp / 8;
	uint32_t data = fill ? fb->pixelMask : 0;

	uint32_t size = fb->bpp / 8;
	fb_memcpy(base + offset, &data, size);
	memcpy(buf + offset, &data, size);
}

extern uint8_t gFont[];

static void
draw_char(Frame_Buffer *fb, char c)
{
	uint8_t *glyph = gFont + GLYPH_BYTES * c;

	switch (c) {
		case '\r':
			fb->cursorX = 0;
			return;
		case '\n':
			scroll_up(fb);
			return;
		case '\b':
			if (!fb->cursorX)
				return;

			fb->cursorX--;

			/* Assume the glyph for '\b' is empty */
			break;
	}

	if (fb->cursorX == CONSOLE_WIDTH) {
		fb->cursorX = 0;
		scroll_up(fb);
	}

	uint32_t startY = GLYPH_HEIGHT * (CONSOLE_HEIGHT - 1);
	uint32_t startX = GLYPH_WIDTH * fb->cursorX;
	for (uint32_t y = 0; y < GLYPH_HEIGHT; y++)
		for (uint32_t x = 0; x < GLYPH_WIDTH; x++)
			fb->drawPixel(fb, x + startX, y + startY,
				      glyph[y] & (1 << (7 - x)));

	if (c != '\b')
		fb->cursorX++;
}

void
graphics_write(const char *buf)
{
	if (!gGraphicsAvailable)
		return;

	while (*buf) {
		for (int i = 0; i < gFBNum; i++)
			draw_char(&gFBs[i],
				  *buf >= 0 && *buf <= 127 ? *buf : ' ');

		buf++;
	}
}

static uint32_t
compose_pixel_bitmask(Efi_Pixel_Bitmask *pm, int reserved)
{
	uint32_t mask = 0;

	mask |= pm->redMask;
	mask |= pm->greenMask;
	mask |= pm->blueMask;
	mask |= reserved ? pm->reservedMask : 0;

	return mask;
}

static int
mask_to_bpp(uint32_t mask)
{
	int i = 32;

	while (!(mask & (1 << 31))) {
		mask <<= 1;
		i--;
	}

	return i;
}

static int
fbmode_is_supported(Efi_Graphics_Output_Mode_Info *info)
{
	if (info->horizontalRes < MIN_HORIZONTAL_RESOLUTION ||
	    info->verticalRes < MIN_VERTICAL_RESOLUTION)
		return 0;

	uint32_t mask;
	switch (info->pixelFormat) {
	case PIXEL_RGB_RESERVED_8888:
	case PIXEL_BGR_RESERVED_8888:
		return 1;
	case PIXEL_BIT_MASK:
		mask = compose_pixel_bitmask(&info->pixelInfo, 1);
		return mask_to_bpp(mask) % 8 == 0;
	default:
		break;
	};

	return 0;
}

static int
is_buffer_duplicated(Efi_Graphics_Output_Protocol *gop)
{
	for (size_t i = 0; i < gFBNum; i++) {
		if (gop->mode->fbBase == gFBs[i].base)
			return 1;
	}

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

		if (!fbmode_is_supported(*info))
			continue;

		ret = efi_method(gop, setMode, mode);
		if (ret) {
			pr_err("Failed to set GOP to mode %u: %d\n", mode, ret);
			return -1;
		}

		return is_buffer_duplicated(gop);
	}

	return -1;
}

static int
gop_setup_mode(Frame_Buffer *fb, Efi_Graphics_Output_Protocol *gop,
	       Efi_Graphics_Output_Mode_Info *info)
{
	void *buf = malloc_pages(gop->mode->fbSize);
	if (!buf) {
		pr_err("Failed to allocate framebuffer for GOP\n");
		return -1;
	}

	*fb = (struct Frame_Buffer) {
			.height		= gop->mode->info->verticalRes,
			.scanlineWidth	= gop->mode->info->pixelPerScanline,
			.base		= gop->mode->fbBase,
			.buf		= buf,
			.cursorX	= 0,
	};

	switch (info->pixelFormat) {
	case PIXEL_RGB_RESERVED_8888:
	case PIXEL_BGR_RESERVED_8888:
		fb->pixelMask = 0x00ffffff;
		fb->bpp = 32;
		fb->drawPixel = draw_pixel_32b;
		break;
	case PIXEL_BIT_MASK: {
		fb->pixelMask = compose_pixel_bitmask(&info->pixelInfo, 0);
		fb->bpp = mask_to_bpp(
				compose_pixel_bitmask(&info->pixelInfo, 0));
		fb->drawPixel = draw_pixel_anymask;
		break;
	}
	default:	/* unreachable case */
		break;
	}

	fb_memset(fb->base, 0, gop->mode->fbSize);
	memset(fb->buf, 0, gop->mode->fbSize);

	return 0;
}

static int
gop_try_init(Efi_Handle handle, Frame_Buffer *fb)
{
	Efi_Graphics_Output_Mode_Info *info = NULL;
	Efi_Graphics_Output_Protocol *gop;

	efi_handle_protocol(handle, EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, &gop);

	int ret = gop_determine_mode(gop, &info);
	if (ret) {
		pr_info("Skip graphics initialization: No suitable mode\n");
		return -1;
	}

	pr_info("Resolution %ux%u\n", info->horizontalRes, info->verticalRes);

	if (gop_setup_mode(fb, gop, info)) {
		pr_err("Failed to setup GOP mode\n");
		return -1;
	}

	return 0;
}

void
graphics_init(void)
{
	Efi_Guid gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	size_t handleBufSize = 0;
	Efi_Status ret;

	ret = efi_call(gBS->locateHandle, Efi_Locate_By_Protocol, &gopGuid,
		       NULL, &handleBufSize, NULL);
	ret = EFI_ERRNO(ret);
	if (ret == EFI_NOT_FOUND) {
		pr_info("Skip graphics initialization: no GOP found\n");
		return;
	} else if (ret != EFI_BUFFER_TOO_SMALL) {
		pr_err("locateProtocol returns %lu for GOP\n", ret);
		panic("Failed to locate GOP handle");
	}

	size_t handleNum = handleBufSize / sizeof(Efi_Handle);
	Efi_Handle handles[handleNum];
	efi_call(gBS->locateHandle, Efi_Locate_By_Protocol, &gopGuid, NULL,
		 &handleBufSize, handles);

	gFBs = malloc(sizeof(*gFBs) * handleNum);

	for (size_t i = 0; i < handleNum; i++) {
		ret = gop_try_init(handles[i], &gFBs[gFBNum]);
		if (!ret)
			gFBNum++;
	}

	if (gFBNum) {
		pr_info("Graphics initialized\n");
		gGraphicsAvailable = 1;
	} else {
		pr_info("No suitable GOP device found\n");
	}
}
