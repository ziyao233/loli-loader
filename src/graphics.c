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
	Efi_Graphics_Output_Protocol *gop;
	uint32_t height, width;
	void *buf;
	int32_t damagedLeft, damagedUp;
	int32_t damagedRight, damagedDown;
	uint32_t cursorX;
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
	return GLYPH_HEIGHT * fb->width * 4;
}

static void
scroll_up(Frame_Buffer *fb)
{
	size_t moveSize = console_line_bytes(fb) * (CONSOLE_HEIGHT - 1);
	uint8_t *buf = fb->buf;

	memmove(buf, buf + console_line_bytes(fb), moveSize);
	memset(buf + moveSize, 0, console_line_bytes(fb));

	fb->damagedLeft = fb->damagedUp = 0;
	fb->damagedRight	= fb->width - 1;
	fb->damagedDown		= fb->height - 1;
}

static void
draw_pixel(Frame_Buffer *fb, uint32_t x, uint32_t y, int fill)
{
	uint32_t offset = y * fb->width + x;
	uint32_t *buf = fb->buf;

	buf[offset] = fill ? 0x00ffffff : 0;
}

static void
update_damaged_region(Frame_Buffer *fb, uint32_t x, uint32_t y,
		      uint32_t width, uint32_t height)
{
	uint32_t endX = x + width - 1, endY = y + height - 1;

	fb->damagedLeft	 = fb->damagedLeft > x ? x : fb->damagedLeft;
	fb->damagedUp	 = fb->damagedUp > y ? y : fb->damagedUp;
	fb->damagedRight = fb->damagedRight < endX ? endX : fb->damagedRight;
	fb->damagedDown	 = fb->damagedDown < endY ? endY : fb->damagedDown;
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
			draw_pixel(fb, x + startX, y + startY,
				   glyph[y] & (1 << (7 - x)));

	update_damaged_region(fb, startX, startY, GLYPH_WIDTH, GLYPH_HEIGHT);

	if (c != '\b')
		fb->cursorX++;
}

static void
reset_damaged_region(Frame_Buffer *fb)
{
	fb->damagedLeft		= fb->width;
	fb->damagedUp		= fb->height;
	fb->damagedRight	= 0;
	fb->damagedDown		= 0;
}

static void
blit_buffer(Frame_Buffer *fb)
{
	/* Nothing damaged */
	if (fb->damagedLeft >= fb->width)
		return;

	uint32_t damagedWidth	= fb->damagedRight - fb->damagedLeft + 1;
	uint32_t damagedHeight	= fb->damagedDown - fb->damagedUp + 1;

	efi_method(fb->gop, blt, fb->buf, EFI_BLT_BUFFER_TO_VIDEO,
		   fb->damagedLeft, fb->damagedUp,
		   fb->damagedLeft, fb->damagedUp,
		   damagedWidth, damagedHeight,
		   4 * fb->width);

	reset_damaged_region(fb);
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

	for (int i = 0; i < gFBNum; i++)
		blit_buffer(&gFBs[i]);
}

static int
fbmode_is_supported(Efi_Graphics_Output_Mode_Info *info)
{
	return info->horizontalRes >= MIN_HORIZONTAL_RESOLUTION &&
	       info->verticalRes >= MIN_VERTICAL_RESOLUTION;
}

static int
is_buffer_duplicated(Efi_Graphics_Output_Protocol *gop)
{
	for (size_t i = 0; i < gFBNum; i++) {
		if (gop == gFBs[i].gop)
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
			continue;
		}

		return is_buffer_duplicated(gop);
	}

	return -1;
}

static int
gop_setup_mode(Frame_Buffer *fb, Efi_Graphics_Output_Protocol *gop,
	       Efi_Graphics_Output_Mode_Info *info)
{
	size_t fbSize = MIN_HORIZONTAL_RESOLUTION * MIN_VERTICAL_RESOLUTION;
	fbSize *= 4;

	void *buf = malloc_pages(fbSize);
	if (!buf) {
		pr_err("Failed to allocate framebuffer for GOP\n");
		return -1;
	}

	*fb = (struct Frame_Buffer) {
			.gop		= gop,
			.width		= MIN_HORIZONTAL_RESOLUTION,
			.height		= MIN_VERTICAL_RESOLUTION,
			.buf		= buf,
			.cursorX	= 0,
	};

	reset_damaged_region(fb);

	memset(fb->buf, 0, fbSize);

	uint32_t pixel = 0;
	efi_method(gop, blt, &pixel, EFI_BLT_VIDEO_FILL, 0, 0, 0, 0,
		   info->horizontalRes, info->verticalRes, 0);

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
