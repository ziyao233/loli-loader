/*
 *	loli-loader
 *	src/eficall.S
 *	Copyright (c) 2025 Yao Zi.
 */

#if LOLI_TARGET_X86_64

#define STUB(n) __efi_call##n

	.global STUB(1), STUB(2), STUB(3), STUB(4), STUB(5), STUB(6)
	.global STUB(10)

/*
 *	SystemV ABI: %rdi, %rsi, %rdx, %rcx, %r8, %r9, stack
 *	Win32 ABI: %rcx, %rdx, %r8, %r9, stack, shadow space
 */

STUB(1):

	movq		%rdi,		%rcx

	addq		$-40,		%rsp
	callq		*%rsi
	addq		$40,		%rsp

	retq

STUB(2):
	movq		%rdx,		%rax		// function pointer

	movq		%rdi,		%rcx
	movq		%rsi,		%rdx

	addq		$-40,		%rsp
	callq		*%rax
	addq		$40,		%rsp

	retq

STUB(3):
	movq		%rcx,		%rax		// function pointer

	movq		%rdx,		%r8
	movq		%rdi,		%rcx
	movq		%rsi,		%rdx

	addq		$-40,		%rsp
	callq		*%rax
	addq		$40,		%rsp

	retq

STUB(4):
	movq		%r8,		%rax		// function pointer

	movq		%rdx,		%r8
	movq		%rcx,		%r9
	movq		%rdi,		%rcx
	movq		%rsi,		%rdx

	addq		$-40,		%rsp
	callq		*%rax
	addq		$40,		%rsp

	retq

STUB(5):
	movq		%r9,		%rax		// function pointer
	addq		$-8,		%rsp

	pushq		%r8
	movq		%rdx,		%r8
	movq		%rcx,		%r9
	movq		%rdi,		%rcx
	movq		%rsi,		%rdx

	addq		$-32,		%rsp
	callq		*%rax
	addq		$48,		%rsp

	retq

STUB(6):
	movq		8(%rsp),	%rax		// function pointer
	addq		$-8,		%rsp

	pushq		%r9
	pushq		%r8
	movq		%rdx,		%r8
	movq		%rcx,		%r9
	movq		%rdi,		%rcx
	movq		%rsi,		%rdx

	addq		$-32,		%rsp
	callq		*%rax
	addq		$56,		%rsp

	retq

STUB(10):
	addq		$-8,		%rsp

	movq		40(%rsp),	%rax
	movq		%rax,		-8(%rsp)

	movq		32(%rsp),	%rax
	movq		%rax,		-16(%rsp)

	movq		24(%rsp),	%rax
	movq		%rax,		-24(%rsp)

	movq		16(%rsp),	%rax
	movq		%rax,		-32(%rsp)

	movq		%r9,		-40(%rsp)
	movq		%r8,		-48(%rsp)

	movq		48(%rsp),	%rax

	movq		%rdx,		%r8
	movq		%rcx,		%r9
	movq		%rdi,		%rcx
	movq		%rsi,		%rdx

	addq		$-80,		%rsp
	callq		*%rax

	addq		$88,		%rsp
	ret

#endif	// LOLI_TARGET_X86_64
