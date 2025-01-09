// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/misc.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_MISC_H_INC__
#define __LOLI_MISC_H_INC__

#include <interaction.h>

#define do_log(level, ...) do { \
	const char *_prefix = level == 0 ? "ERROR" :		\
			      level == 1 ? "WARN" :		\
			      level == 2 ? "INFO" :		\
			      "(UNKNOWN LEVEL)";		\
	printf("%s: ", _prefix);				\
	printf(__VA_ARGS__);					\
} while (0)

#define pr_err(...)	do_log(0, __VA_ARGS__)
#define pr_warn(...)	do_log(1, __VA_ARGS__)
#define pr_info(...)	do_log(2, __VA_ARGS__)

void panic(const char *msg);

#endif	// __LOLI_MISC_H_INC__
