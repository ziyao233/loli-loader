// SPDX-License-Identifier: MPL-2.0

#include <string.h>
#include <memory.h>

#include <efidef.h>
#include <efiboot.h>
#include <efidevicepath.h>
#include <efimedia.h>

#include <initrd.h>
#include <misc.h>

#define LINUX_INITRD_GUID \
	EFI_GUID(0x5568e427, 0x68fc, 0x4f3d,				\
		 0xac, 0x74, 0xca, 0x55, 0x52, 0x31, 0xcc, 0x68)

static Efi_Vendor_Media_Device_Path initrdDevicePath[2] = {
	{
		{
			EFI_DEVICE_MEDIA,
			EFI_DEVICE_SUBTYPE_VENDOR,
			sizeof(Efi_Vendor_Media_Device_Path),
		},
		LINUX_INITRD_GUID
	},
	{
		{
			EFI_DEVICE_END,
			EFI_DEVICE_SUBTYPE_ENTIRE_END,
			sizeof(Efi_Device_Path_Protocol),
		},
	},
};

/* TODO: Avoid maintaining a copy of initrd in memory to reduce overhead */

typedef struct Initrd_Load_File2_Protocol {
	Efi_Load_File2_Protocol protocol;
	void *base;
	size_t size;
} Initrd_Load_File2_Protocol;

#ifdef LOLI_TARGET_X86_64
Efi_Status
_initrd_load_file
#else
static Efi_Status
initrd_load_file
#endif
		 (Efi_Load_File2_Protocol *p, Efi_Device_Path_Protocol *dp,
		 bool bootPolicy, uint_native *bufferSize, void *buffer)
{
	Initrd_Load_File2_Protocol *initrdProtocol;
	Efi_Status ret = 0;

	(void)dp;
	initrdProtocol = (Initrd_Load_File2_Protocol *)p;

	if (!bufferSize || bootPolicy || (*bufferSize && !buffer))
		return TO_EFI_ERRNO(EFI_INVALID_PARAMETER);

	if (*bufferSize < initrdProtocol->size) {
		ret = TO_EFI_ERRNO(EFI_BUFFER_TOO_SMALL);
		goto out;
	}

	memcpy(buffer, initrdProtocol->base, initrdProtocol->size);

out:
	*bufferSize = initrdProtocol->size;
	return ret;
}

Efi_Status initrd_load_file(Efi_Load_File2_Protocol *p,
			    Efi_Device_Path_Protocol *dp,
			    bool bootPolicy, uint_native *buffeRSize,
			    void *buffer);

int
initrd_setup(void *base, size_t size)
{
	Initrd_Load_File2_Protocol *initrdProtocol;
	Efi_Handle initrd = NULL;
	Efi_Status ret;

	initrdProtocol = malloc(sizeof(*initrdProtocol));

	*initrdProtocol = (Initrd_Load_File2_Protocol) {
		.protocol	= {
			.loadFile = initrd_load_file,
		},
		.base		= base,
		.size		= size,
	};

	Efi_Guid dpGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;
	ret = efi_call(gBS->installProtocolInterface, &initrd, &dpGuid,
		       EFI_NATIVE_INTERFACE, initrdDevicePath);
	if (ret) {
		pr_err("failed to register initrd device: %d\n", ret);
		goto destroyProtocol;
	}

	Efi_Guid loadFile2Guid = EFI_LOAD_FILE2_PROTOCOL_GUID;
	ret = efi_call(gBS->installProtocolInterface, &initrd,
		       &loadFile2Guid, EFI_NATIVE_INTERFACE, initrdProtocol);
	if (ret) {
		pr_err("failed to install LoadFile2 Protocol: %d\n", ret);
		goto uninstallDevicePath;
	}

	return 0;

uninstallDevicePath:
	efi_call(gBS->uninstallProtocolInterface, initrd, &dpGuid,
		 initrdDevicePath);
destroyProtocol:
	free(initrdProtocol);

	return ret;
}
