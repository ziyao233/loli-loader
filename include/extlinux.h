// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/extlinux.h
 *	Copyright (c) 2024 Yao Zi.
 */
#ifndef __LOLI_EXTLINUX_H_INC__
#define __LOLI_EXTLINUX_H_INC__

const char *extlinux_next_entry(const char *conf, const char *last);
const char *extlinux_get_value(const char *conf, const char *key,
			       unsigned long int *valuelen);

#endif	// __LOLI_EXTLINUX_H_INC__
