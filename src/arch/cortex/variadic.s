/*
 * variadic.s
 *
 *  Created on: Jul 31, 2012
 *      Author: petera
 */

@ ----------------------------------------------------------------------------
@ r0 = func, r1 = argc, r2 = void *args
    .syntax unified
	.cpu cortex-m3
	.fpu softvfp
	.thumb

	.section	.text._variadic_call,"ax",%progbits

	.thumb_func
	.global _variadic_call

_variadic_call:
    push  	{r4-r8, lr}

    mov		r4, r0		@ func
    mov		r5, r1		@ argc
    mov		r6, r2		@ args
    mov		r7, #0		@ stack usage

	mov		r0, #0
	mov		r1, #0
	mov		r2, #0
	mov		r3, #0

	@ 0 args
	subs	r5, r5, #1
	bmi		.call

	@ 1st arg -> r0
    ldr		r0, [r6]
    subs	r5, r5, #1
	bmi		.call

	@ 2nd arg -> r1
	add		r6, r6, #4
    ldr		r1, [r6]
    subs	r5, r5, #1
	bmi		.call

	@ 3rd arg -> r2
	add		r6, r6, #4
    ldr		r2, [r6]
    subs	r5, r5, #1
	bmi		.call

	@ 4th arg -> r3
	add		r6, r6, #4
    ldr		r3, [r6]
    subs	r5, r5, #1
	bmi		.call

	@ following args -> stack
	@ compute needed stack size in r7
	lsl		r7, r5, #2
	add		r7, r7, #4
	add		r6, r6, r7

.arg_on_stack:
	ldr		r8,	[r6]
	push	{r8}
	sub		r6, r6, #4

	@ next arg
    subs	r5, r5, #1
	bpl		.arg_on_stack

	@ call
    blx		r4

	@ restore sp
    add		r7, r7, sp
    mov		sp, r7

    pop   	{r4-r8, pc}

.call:
    blx		r4

    pop   	{r4-r8, pc}

        .size   _variadic_call, . - _variadic_call
