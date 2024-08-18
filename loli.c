// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/loli.c
 *	Copyright (c) 2024 Yao Zi.
 */

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long int uint64_t;
typedef uint32_t uint_native;
typedef uint8_t bool;
typedef uint16_t wchar_t;

typedef void * Efi_Handle;
typedef uint_native Efi_Status;

#pragma pack(push, 0)

typedef struct Efi_Simple_Text_Output_Protocol {
	Efi_Status (*reset)(struct Efi_Simple_Text_Output_Protocol *p,
			    bool extendedVerifcation);
	Efi_Status (*outputString)(struct Efi_Simple_Text_Output_Protocol *p,
				   wchar_t *str);
} Efi_Simple_Text_Output_Protocol;

typedef struct {
	/* EFI SYSTEM TABLE HEADER */
	uint64_t signature;
	uint32_t revision;
	uint32_t headerSize;
	uint32_t crc32;
	uint32_t reserved;

	wchar_t *firmwareVendor;
	uint32_t firmwareRevision;
	Efi_Handle consoleInHandle;
	Efi_Handle conIn;
	Efi_Handle consoleOutHandle;
	Efi_Simple_Text_Output_Protocol *conOut;
} Efi_System_Table;

#pragma pack(pop)

#define EFI_SUCCESS 0

Efi_Status
efi_main(Efi_Handle *imageHandle, Efi_System_Table *st)
{
	// We need an EFI call wrapper

	st->conOut->outputString(st->conOut, L"Hello\n");

	return EFI_SUCCESS;
}
