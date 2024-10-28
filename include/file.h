// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/file.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_FILE_H_INC__
#define __LOLI_FILE_H_INC__

#include <efidef.h>

void file_init(void);

int64_t file_get_size(const char *path);
int64_t file_load(const char *path, void **buf);

#endif	// __LOLI_FILE_H_INC__
