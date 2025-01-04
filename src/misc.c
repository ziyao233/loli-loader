// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/src/misc.c
 *	Copyright (c) 2024 Yao Zi.
 */

#include <efidef.h>
#include <stdarg.h>
#include <interaction.h>

#include <misc.h>

void
panic(void)
{
	printf("panic: cannot boot\n");
	while (1) ;
}
