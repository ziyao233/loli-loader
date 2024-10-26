// SPDX-License-Identifier: MPL-2.0
/*
 *	loli-loader
 *	/include/vaarg.h
 *	Copyright (c) 2024 Yao Zi.
 */

#ifndef __LOLI_STDARG_H_INC__
#define __LOLI_STDARG_H_INC__

#if !defined(__clang__) && !defined(__GNUC__)
#warning "Current va_arg implementation may not work on this compiler"
#endif

typedef __builtin_va_list	va_list;
#define va_start(v, l)		__builtin_va_start(v, l)
#define va_end(v)		__builtin_va_end(v)
#define va_arg(v, t)		__builtin_va_arg(v, t)
#define va_copy(d, s)		__builtin_va_copy(d, s)

#endif	// __LOLI_STDARG_H_INC__
