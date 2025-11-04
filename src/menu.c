// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/menu.c
 *	Copyright (C) 2025 Yao Zi <ziyao@disroot.org>
 */

#include <efidef.h>
#include <memory.h>
#include <string.h>

#include <extlinux.h>
#include <misc.h>

char *
menu_get_pair(const char *entry, const char *key)
{
	size_t len = 0;
	const char *res = extlinux_get_value(entry, key, &len);

	if (!res)
		return NULL;

	char *copy = malloc(len + 1);
	strscpy(copy, res, len + 1);

	return copy;
}

int
menu_get_timeout(const char *cfg)
{
	char *res = menu_get_pair(cfg, "timeout");
	if (!res)
		return 0;

	int timeout = atou(res);

	if (timeout < 0) {
		pr_err("invalid timeout \"%s\", use 0 instead\n", res);
		timeout = 0;
	}

	free(res);

	return timeout;
}

const char *
menu_get_nth_entry(const char *cfg, int index)
{
	const char *entry = extlinux_next_entry(cfg, NULL);

	while (index--)
		entry = extlinux_next_entry(NULL, entry);

	return entry;
}

