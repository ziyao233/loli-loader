// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	src/extlinux.c
 *	Copyright (c) 2024 Yao Zi. All rights reserved.
 */

#include <string.h>

#include "extlinux.h"

static int
is_space(const char *p)
{
	return *p == '\t' || *p == ' ';
}

static const char *
skip_space(const char *p)
{
	if (!p)
		return NULL;

	while (is_space(p))
		p++;

	return *p ? p : NULL;
}

static const char *
is_key(const char *p, const char *key)
{
	while (*p && *key) {
		if (*p != *key)
			return NULL;
		p++;
		key++;
	}

	if (*key)
		return NULL;

	if (is_space(p)) {
		p = skip_space(p);

		if (*p != '\n' && *p)
			return p;
	}

	return NULL;
}

static const char *
next_line(const char *p)
{
	if (!p)
		return NULL;

	while (*p != '\n' && *p)
		p++;

	if (!*p)
		return NULL;

	p++;	// skip the '\n'

	return *p ? p : NULL;
}

const char *
extlinux_next_entry(const char *conf, const char *last)
{
	const char *p = conf;

	/* skip the last "label" pair */
	if (last)
		p = is_key(last, "label");

	while (p) {
		p = skip_space(p);

		if (is_key(p, "label"))
			return p;

		p = next_line(p);
	}

	return p;
}

const char *
extlinux_get_value(const char *p, const char *key, unsigned long int *valuelen)
{
	int getLabel = key[0] == 'l' && key[1] == 'a' && key[2] == 'b' &&
		       key[3] == 'e' && key[4] == 'l' && !key[5];

	if (!getLabel && is_key(p, "label"))
		p = next_line(p);

	const char *value = NULL;
	while (p) {
		p = skip_space(p);
		if (!p)

		if (is_key(p, "label") && !getLabel)
			break;

		value = is_key(p, key);
		if (value) {
			const char *vend = value;

			while (*vend && *vend != '\n')
				vend++;
			vend--;

			while (vend >= value && is_space(vend))
				vend--;

			*valuelen = vend - value + 1;
			return value;
		}

		p = next_line(p);
	}

	return NULL;
}
