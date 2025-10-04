// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/ctype.c
 *	Copyright (C) 2025 Yao Zi <ziyao@disroot.org>
 */

int
isprint(int c)
{
	return c > 0x1f && c < 0x7f;
}
