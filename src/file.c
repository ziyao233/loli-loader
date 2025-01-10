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

static Efi_File_Protocol *
open_file_handle(const char *path)
{
	size_t wlen = str2wcs(NULL, path);
	wchar_t *wpath = malloc(sizeof(wchar_t) * (wlen + 1));
	str2wcs(wpath, path);

	Efi_File_Protocol *file = NULL;
	if (efi_method(root, open, &file, wpath, EFI_FILE_MODE_READ, 0) !=
	    EFI_SUCCESS)
		file = NULL;

	free(wpath);
	return file;
}

static void
close_file_handle(Efi_File_Protocol *file)
{
	efi_call(file->close, file);
}

int64_t
file_get_size(const char *path)
{
	Efi_File_Protocol *file = open_file_handle(path);
	if (!file)
		return -1;

	// more than double of FAT32 max path length
	uint_native bufSize = 768;
	Efi_File_Info *info = malloc(bufSize);
	Efi_Guid efiFileInfo = EFI_FILE_INFO_GUID;
	efi_method(file, getInfo, &efiFileInfo, &bufSize, info);

	uint64_t fileSize = info->fileSize;
	free(info);
	close_file_handle(file);
	return (int64_t)fileSize;
}

int64_t
file_load(const char *path, void **buf)
{
	int64_t size = file_get_size(path);
	if (size < 0)
		return -1;

	Efi_File_Protocol *file = open_file_handle(path);
	if (!file)
		return -1;

	if (!*buf)
		*buf = malloc(size);
	uint_native bufSize = size;
	efi_method(file, read, &bufSize, *buf);

	close_file_handle(file);
	return size;
}
