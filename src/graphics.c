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

static int gGraphicsAvailable;
static uint32_t *gFrameBuffer;
Efi_Graphics_Output_Mode_Info gFrameBufferInfo;

#define MIN_HORIZONTAL_RESOLUTION	(80 * 8)
#define MIN_VERTICAL_RESOLUTION		(24 * 16)

void
graphics_init(void)
{
	Efi_Guid gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	Efi_Graphics_Output_Protocol *gop = NULL;
	Efi_Status ret;

	ret = efi_call(gBS->locateProtocol, &gopGuid, NULL, (void **)&gop);
	ret = EFI_ERRNO(ret);
	if (ret == EFI_NOT_FOUND) {
		pr_info("Skip graphics initialization: GOP isn't supported\r\n");
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
			pr_err("Failed to query GOP mode %u: %d\r\n", mode, ret);
			return;
		}

		if (info->pixelFormat != PIXEL_RGB_RESERVED_8888 &&
		    info->pixelFormat != PIXEL_BGR_RESERVED_8888)
			continue;

		if (info->horizontalRes >= MIN_HORIZONTAL_RESOLUTION &&
		    info->verticalRes >= MIN_VERTICAL_RESOLUTION)
			break;
	}

	ret = efi_method(gop, setMode, mode);
	if (ret) {
		pr_err("Failed to setup GOP mode %u: %d\r\n", mode, ret);
		return;
	}

	pr_info("Graphics initialized: GOP mode = %u\r\n", mode);
	pr_info("Resolution %ux%u\r\n",
		info->horizontalRes, info->verticalRes);

	gFrameBuffer = gop->mode->fbBase;
	memset(gFrameBuffer, 0, gop->mode->fbSize);

	memcpy(&gFrameBufferInfo, info, sizeof(*info));

	gGraphicsAvailable = 1;
}
