// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/fdt.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_FDT_H_INC__
#define __LOLI_FDT_H_INC__

#include <efidef.h>

#pragma pack(push, 0)

typedef struct {
	uint32_t magic;
	uint32_t totalSize;
	uint32_t offDtStruct;
	uint32_t offDtStrings;
	uint32_t offMapRsvMap;
	uint32_t version;
	uint32_t lastCompVersion;
} Fdt_Header;

#pragma pack(pop)

void fdt_fixup(void);

#endif	// __LOLI_FDT_H_INC__
