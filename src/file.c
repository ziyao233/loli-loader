// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/file.c
 *	Copyright (c) 2024 Yao Zi.
 */

#include <efidef.h>
#include <eficall.h>
#include <efi.h>
#include <efiboot.h>
#include <efiloadedimage.h>
#include <efimedia.h>
#include <memory.h>
#include <string.h>

#include <misc.h>

static Efi_File_Protocol *root;

void
file_init(void)
{
	Efi_Loaded_Image_Protocol *img;
	efi_handle_protocol(gSelf, EFI_LOADED_IMAGE_PROTOCOL_GUID, &img);

	Efi_Simple_File_System_Protocol *fs;
	efi_handle_protocol(img->deviceHandle,
			    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, &fs);

	efi_method(fs, openVolume, &root);
}

Efi_File_Protocol *
file_open(const wchar_t *path)
{
	Efi_File_Protocol *file = NULL;
	Efi_Status ret;
retry:
	ret = efi_method(root, open, &file,
			 (wchar_t *)path, EFI_FILE_MODE_READ, 0);
	if (ret != EFI_SUCCESS)
		file = NULL;

	/* EDK2 compatibility: EDK2 doesn't accept paths starting with '/' */
	if (!file && (char)path[0] == '/') {
		path++;
		goto retry;
	}

	return file;
}

static Efi_File_Protocol *
file_open_str(const char *path)
{
	wchar_t *wpath = malloc((str2wcs(NULL, path) + 1) * sizeof(wchar_t));
	str2wcs(wpath, path);

	Efi_File_Protocol *file = file_open(wpath);
	free(wpath);

	return file;
}

Efi_Status
file_get_info(Efi_File_Protocol *file, Efi_File_Info **info)
{
	Efi_Guid efiFileInfo = EFI_FILE_INFO_GUID;
	Efi_Status ret;

	*info = NULL;
	size_t size = 0;

	ret = efi_method(file, getInfo, &efiFileInfo, &size, NULL);
	if (ret == EFI_SUCCESS)
		return EFI_INVALID_PARAMETER; // Shouldn't happen

	if (ret != TO_EFI_ERRNO(EFI_BUFFER_TOO_SMALL))
		return ret;

	*info = malloc(size);
	ret = efi_method(file, getInfo, &efiFileInfo, &size, *info);
	if (ret != EFI_SUCCESS) {
		free(*info);
		*info = NULL;
	}

	return ret;
}

void
file_close(Efi_File_Protocol *file)
{
	efi_call(file->close, file);
}

int64_t
file_get_size(const char *path)
{
	Efi_File_Protocol *file = file_open_str(path);
	Efi_File_Info *info;

	if (!file)
		return -1;

	if (file_get_info(file, &info) != EFI_SUCCESS)
		return -1;

	uint64_t fileSize = info->fileSize;
	free(info);
	file_close(file);
	return (int64_t)fileSize;
}

int64_t
file_load(const char *path, void **buf)
{
	Efi_File_Protocol *file = file_open_str(path);
	if (!file)
		return -1;

	Efi_File_Info *info;
	if (file_get_info(file, &info) != EFI_SUCCESS)
		return -1;

	if (info->attribute & EFI_FILE_DIRECTORY)
		return -1;

	uint_native bufSize = info->fileSize;
	free(info);

	efi_method(file, read, &bufSize, *buf);

	file_close(file);
	return (int64_t)bufSize;
}
