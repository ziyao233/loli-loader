// SPDX-License-Identifer: MPL-2.0
/*
 *	loli-loader
 *	/include/eficon.h
 */

#ifndef __LOLI_EFICON_H_INC__
#define __LOLI_EFICON_H_INC__

#include <efidef.h>

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
	EFI_GUID(0x9042a9de, 0x23dc, 0x4a38,				\
		 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a)

#define EFI_SERIAL_IO_PROTOCOL_GUID \
	EFI_GUID(0xbb25cf6f,0xf1d4, 0x11d2,				\
		 0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0xfd)

#pragma pack(push, 0)

typedef struct {
	uint16_t scanCode;
	wchar_t unicodeChar;
} Efi_Input_Key;

typedef struct Efi_Simple_Text_Input_Protocol {
	Efi_Status (*reset)(struct Efi_Simple_Text_Input_Protocol *p,
			    bool extendedVerification);
	Efi_Status (*readKeyStroke)(struct Efi_Simple_Text_Input_Protocol *p,
				    Efi_Input_Key *key);
	Efi_Event waitForKey;
} Efi_Simple_Text_Input_Protocol;

typedef struct Efi_Simple_Text_Output_Protocol {
        Efi_Status (*reset)(struct Efi_Simple_Text_Output_Protocol *p,
			    bool extendedVerifcation);
	Efi_Status (*outputString)(struct Efi_Simple_Text_Output_Protocol *p,
				   wchar_t *str);
} Efi_Simple_Text_Output_Protocol;

typedef struct Efi_Pixel_Bitmask {
	uint32_t redMask;
	uint32_t greenMask;
	uint32_t blueMask;
	uint32_t reservedMask;
} Efi_Pixel_Bitmask;

typedef enum {
	PIXEL_RGB_RESERVED_8888 = 0,
	PIXEL_BGR_RESERVED_8888 = 1,
	PIXEL_BIT_MASK = 2,
	PIXEL_BLT_ONLY = 3,
} Efi_Graphics_Pixel_Format;

typedef struct Efi_Graphics_Output_Mode_Info {
	uint32_t version;
	uint32_t horizontalRes;
	uint32_t verticalRes;
	Efi_Graphics_Pixel_Format pixelFormat;
	Efi_Pixel_Bitmask pixelInfo;
	uint32_t pixelPerScanline;
} Efi_Graphics_Output_Mode_Info;

typedef struct Efi_Graphics_Output_Mode {
	uint32_t maxMode;
	uint32_t mode;
	Efi_Graphics_Output_Mode_Info *info;
	uint_native sizeOfInfo;
	void *fbBase;
	uint_native fbSize;
} Efi_Graphics_Output_Mode;

typedef enum {
	EFI_BLT_VIDEO_FILL		= 0,
	EFI_BLT_VIDEO_TO_BLT_BUFFER	= 1,
	EFI_BLT_BUFFER_TO_VIDEO		= 2,
	EFI_BLT_VIDEO_TO_VIDEO		= 3,
} Efi_Graphics_Output_Blt_Op;

typedef struct Efi_Graphics_Output_Protocol {
	Efi_Status (*queryMode)(struct Efi_Graphics_Output_Protocol *p,
				uint32_t modeNum,
				uint_native *sizeOfInfo,
				Efi_Graphics_Output_Mode_Info **info);
	Efi_Status (*setMode)(struct Efi_Graphics_Output_Protocol *p,
			      uint32_t modeNum);
	Efi_Status (*blt)(struct Efi_Graphics_Output_Protocol *p,
			  void *bltBuffer, Efi_Graphics_Output_Blt_Op op,
			  uint_native srcX, uint_native srcY,
			  uint_native dstX, uint_native dstY,
			  uint_native width, uint_native height,
			  uint_native delta);
	Efi_Graphics_Output_Mode *mode;
} Efi_Graphics_Output_Protocol;

typedef struct Efi_Serial_IO_Protocol {
	uint32_t revision;
	Efi_Handle reset;
	Efi_Handle setAttr;
	Efi_Handle setCtrl;
	Efi_Handle getCtrl;
	Efi_Status (*write)(struct Efi_Serial_IO_Protocol *p,
			    uint_native *bufSize, void *buf);
	Efi_Handle read;
	Efi_Handle serailIOMode;
} Efi_Serial_IO_Protocol;

#pragma pack(pop)

#endif // __LOLI_EFICON_H_INC__
