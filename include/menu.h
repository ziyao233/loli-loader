// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/menu.h
 *	Copyright (C) 2025 Yao Zi <ziyao@disroot.org>
 */

#ifndef __LOLI_MENU_H_INC__
#define __LOLI_MENU_H_INC__

int menu_get_timeout(const char *cfg);
const char *menu_get_nth_entry(const char *cfg, int index);
char *menu_get_pair(const char *entry, const char *key);

#endif	// __LOLI_MENU_H_INC__
