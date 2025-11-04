// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/interaction.h
 *	Copyright (c) 2024 Yao Zi.
 */
#ifndef __LOLI_INTERACTION_H_INC__
#define __LOLI_INTERACTION_H_INC__

#include <efidef.h>

#define EOF		-1

void interaction_init(void);
void puts_sized(const char *s, size_t size);
void printf(const char *format, ...);
int getchar_timeout(int timeout);
char *getline_timeout(int timeout);

#endif	// __LOLI_INTERACTION_H_INC__
