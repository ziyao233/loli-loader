/*
 *	loli-loader
 *	src/eficall.S
 *	Copyright (c) 2025 Yao Zi.
 */

#if LOLI_TARGET_X86_64

#define STUB(n) __efi_call##n

	.global STUB(1), STUB(2), STUB(3), STUB(4), STUB(5), STUB(6)

/*
 *	SystemV ABI: %rdi, %rsi, %rdx, %rcx, %r8, %r9, stack
 *	Win32 ABI: %rcx, %rdx, %r8, %r9, stack, shadow space
 */

STUB(1):

	movq		%rdi,		%rcx

	addq		$-32,		%rsp
	callq		*%rsi
	addq		$32,		%rsp

	retq

STUB(2):
	movq		%rdx,		%rax		// function pointer

	movq		%rdi,		%rcx
	movq		%rsi,		%rdx

	addq		$-32,		%rsp
	callq		*%rax
	addq		$32,		%rsp

	retq

STUB(3):
	movq		%rcx,		%rax		// function pointer

	movq		%rdx,		%r8
	movq		%rdi,		%rcx
	movq		%rsi,		%rdx

	addq		$-32,		%rsp
	callq		*%rax
	addq		$32,		%rsp

	retq

STUB(4):
	movq		%r8,		%rax		// function pointer

	movq		%rdx,		%r8
	movq		%rcx,		%r9
	movq		%rdi,		%rcx
	movq		%rsi,		%rdx

	addq		$-32,		%rsp
	callq		*%rax
	addq		$32,		%rsp

	retq

STUB(5):
	movq		%r9,		%rax		// function pointer

	pushq		%r8
	movq		%rdx,		%r8
	movq		%rcx,		%r9
	movq		%rdi,		%rcx
	movq		%rsi,		%rdx

	addq		$-32,		%rsp
	callq		*%rax
	addq		$40,		%rsp

	retq

STUB(6):
	movq		8(%rsp),	%rax		// function pointer

	pushq		%r9
	pushq		%r8
	movq		%rdx,		%r8
	movq		%rcx,		%r9
	movq		%rdi,		%rcx
	movq		%rsi,		%rdx

	addq		$-32,		%rsp
	callq		*%rax
	addq		$48,		%rsp

	retq

#endif	// LOLI_TARGET_X86_64
