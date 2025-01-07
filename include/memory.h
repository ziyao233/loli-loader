// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/memory.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_MEMORY_H_INC__
#define __LOLI_MEMORY_H_INC__

#include <efidef.h>
#include <efi.h>
#include <eficall.h>

void *malloc(size_t s);
void free(void *p);
void *realloc(void *p, size_t oldSize, size_t size);

#endif	// __LOLI_MEMORY_H_INC__
