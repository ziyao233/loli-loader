// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	src/extlinux.c
 *	Copyright (c) 2024 Yao Zi. All rights reserved.
 *	Zero-allocation parser for extlinux-like configuration files.
 */

#include <string.h>

#include "extlinux.h"

static int
is_space(const char *p)
{
	return *p == '\t' || *p == ' ';
}

/*
 * Skip leading white space characters in a string, and return pointer to the
 * first non-space charater. '\0' is considered non-space.
 *
 * This function always returns a valid-string unless NULL is passed in, in
 * which case NULL is returned.
 */
static const char *
skip_space(const char *p)
{
	if (!p)
		return NULL;

	/* This loop terminates when encoutering zero terminator */
	while (is_space(p))
		p++;

	return p;
}

/*
 * Given a line without leading white space characters of k-v pair as p, check
 * whether the key matches.
 *
 * Return pointer to start of the value if p matches and has valid value after
 * the key, otherwise NULL.
 */
static const char *
match_key(const char *p, const char *key)
{
	/*
	 * Compare p and key. The loop never iterates passed the terminator zero
	 * of either p or key, since the condition is *p && *key, and both p
	 * and key only advances one character each iteraction of the loop.
	 */
	while (*p && *key) {
		if (*p != *key)
			return NULL;
		p++;
		key++;
	}

	/*
	 * When the loop terminates, we're sure the processed parts of p and key
	 * are the same, and either p or key, or both, reaches the terminator
	 * zero.
	 */

	/*
	 * We have unmatched characters in key, it's p already terminated. Fail.
	 */
	if (*key)
		return NULL;

	/*
	 * Check whether p have at least one white space character as separator
	 * between key and value. Fail if none, this isn't a valid k-v pair.
	 */
	if (is_space(p)) {
		p = skip_space(p);

		if (*p != '\n' && *p)
			return p;
	}

	return NULL;
}

/*
 * Given string p, return pointer to the next line, or NULL if none.
 */
static const char *
next_line(const char *p)
{
	if (!p)
		return NULL;

	while (*p != '\n' && *p)
		p++;

	/* We're on the last line with no EOL */
	if (!*p)
		return NULL;

	p++;	// skip the '\n'

	return *p ? p : NULL;
}

/*
 * Given extlinux configuration conf, and last as pointer to the last entry
 * processed, returns the next entry or NULL.
 *
 * For iterating all entries in a configuration, first pass the configuration
 * as conf, and NULL as last, to retrieve the first entry. Subseqeuent calls
 * should set conf to NULL, and last as pointer to the last return value of
 * extlinux_next_entry().
 */
const char *
extlinux_next_entry(const char *conf, const char *last)
{
	const char *p = conf;

	/* skip the last "label" pair */
	if (last)
		p = next_line(last);

	while (p) {
		p = skip_space(p);

		if (match_key(p, "label"))
			return p;

		p = next_line(p);
	}

	return p;
}

/*
 * Given p to be an extlinux configuration, or pointer to an extlinux entry,
 * retrieve a pointer to the value associated with key. Length of the value
 * with trailing white space characters stripped is stored in valuelen.
 *
 * Return a pointer to the value associated with key if found, otherwise NULL.
 */
const char *
extlinux_get_value(const char *p, const char *key, unsigned long int *valuelen)
{
	/*
	 * "label" token implies end of an entry, thus we need to take care of
	 * the special case of getting value for "label" pair.
	 */
	int getLabel = key[0] == 'l' && key[1] == 'a' && key[2] == 'b' &&
		       key[3] == 'e' && key[4] == 'l' && !key[5];

	/*
	 * If we aren't looking up value for "label", and p does point to an
	 * extlinux entry, skip this line or we will wrongly take the label pair
	 * at start of the entry as start of the next entry.
	 *
	 * We only do the hack when p matches "label" key, i.e., it's start of
	 * an entry, but not when getting global options like "timeout", in
	 * which case we'll miss the first global k-v pair if we skip the first
	 * line.
	 */
	if (!getLabel && match_key(p, "label"))
		p = next_line(p);

	const char *value = NULL;
	while (p) {
		/*
		 * match_key requires a pointer without leading white space
		 * characters.
		 */
		p = skip_space(p);
		if (!*p)
			break;

		/*
		 * If we're not looking up a "label" pair, hitting "label" key
		 * simply means end of this entry, and we've found nothing.
		 */
		if (match_key(p, "label") && !getLabel)
			break;

		value = match_key(p, key);
		if (value) {
			/*
			 * Luckily, we found a matching k-v pair. Let's
			 * calculate the value length with trailing white space
			 * stripped
			 */
			const char *vend = value;

			while (*vend && *vend != '\n')
				vend++;
			/*
			 * At end of the loop, we're hitting either zero
			 * terminator or end of line. Skip it.
			 */
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
