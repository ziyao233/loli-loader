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

static struct {
	Efi_Serial_IO_Protocol **protocols;
	size_t num;
	char *buf;
} gSerialStatus;
int gSerialAvailable;

void
serial_write(const char *buf)
{
	if (!gSerialAvailable)
		return;

	uint_native size = strlen(buf);

	for (int i = 0; i < gSerialStatus.num; i++) {
		uint_native remain = size, written = remain;

		while (remain) {
			efi_method(gSerialStatus.protocols[i], write, &written,
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

	gSerialStatus.num = handleBufSize / sizeof(Efi_Handle);

	Efi_Handle handles[gSerialStatus.num];
	efi_call(gBS->locateHandle, Efi_Locate_By_Protocol, &serialGuid, NULL,
		 &handleBufSize, handles);

	gSerialStatus.protocols = malloc(sizeof(*gSerialStatus.protocols) *
					 gSerialStatus.num);

	for (int i = 0; i < gSerialStatus.num; i++)
		efi_handle_protocol(handles[i], serialGuid,
				    &gSerialStatus.protocols[i]);

	gSerialAvailable = 1;
}
