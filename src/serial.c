// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/serial.c
 */

#include <efi.h>
#include <efiboot.h>
#include <eficon.h>
#include <memory.h>
#include <misc.h>
#include <string.h>

static Efi_Serial_IO_Protocol **gSerialProtocols;
static size_t gSerialNum;
int gSerialAvailable;

void
serial_write(const char *buf)
{
	if (!gSerialAvailable)
		return;

	uint_native size = strlen(buf);

	for (int i = 0; i < gSerialNum; i++) {
		uint_native remain = size, written = remain;

		while (remain) {
			efi_method(gSerialProtocols[i], write, &written,
				   (void *)(buf + size - remain));
			remain = remain - written;
			written = remain;
		}
	}
}

void
serial_init(void)
{
	Efi_Guid serialGuid = EFI_SERIAL_IO_PROTOCOL_GUID;
	uint_native handleBufSize = 0;
	Efi_Status ret;

	ret = efi_call(gBS->locateHandle, Efi_Locate_By_Protocol, &serialGuid,
		       NULL, &handleBufSize, NULL);
	ret = EFI_ERRNO(ret);
	if (ret == EFI_NOT_FOUND) {
		pr_info("Skipping serial initialization: no serial supported\r\n");
		return;
	} else if (ret != EFI_BUFFER_TOO_SMALL) {
		pr_err("locateHandle fails with %lu for serial GUID\n", ret);
		panic("Failed to locate handle for serial");
	}

	gSerialNum = handleBufSize / sizeof(Efi_Handle);

	Efi_Handle handles[gSerialNum];
	efi_call(gBS->locateHandle, Efi_Locate_By_Protocol, &serialGuid, NULL,
		 &handleBufSize, handles);

	gSerialProtocols = malloc(sizeof(*gSerialProtocols) * gSerialNum);

	for (int i = 0; i < gSerialNum; i++)
		efi_handle_protocol(handles[i], serialGuid,
				    &gSerialProtocols[i]);

	gSerialAvailable = 1;
}
