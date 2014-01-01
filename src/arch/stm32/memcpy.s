/*
 * memcpy.s
 *
 *  Created on: Jul 1, 2012
 *      Author: petera
 */

@ ----------------------------------------------------------------------------
@ r0 = destination, r1 = source, r2 = length
    .syntax unified
	.cpu cortex-m3
	.fpu softvfp
	.thumb

	.section	.text.memcpy,"ax",%progbits

	.thumb_func
	.global memcpy

memcpy:
#if 0
#	push	{lr}
#.loop:
#	ldrb	r3, [r1]
#	strb	r3, [r0]
#	add		r0, r0, #0x01
#	add		r1, r1, #0x01
#	subs	r2, r2, #0x01
#	bne		.loop
 # 	pop		{pc}
#else
	cmp		r2, #0
	it		eq
	bxeq	lr

	push	{r0, lr}

	cmp		r2, #0x04*4*2
	bhs		.memcpy_opt

.memcpy_unalign:
	tst		r2, #0x01
	beq		.memcpy1

	ldrb	r3, [r1]
	strb	r3, [r0]
	add		r0, r0, #0x01
	add		r1, r1, #0x01
	sub		r2, r2, #0x01

.memcpy1:
	tst		r2, #0x02
	beq		.memcpy2

	ldrh	r3, [r1]
	strh	r3, [r0]
	add		r0, r0, #0x02
	add		r1, r1, #0x02
	sub		r2, r2, #0x02

.memcpy2:
	cmp		r2, #0x00
	beq		.memcpy_end

.memcpy4:						@ word for word, no alignment necessary
	ldr		r3, [r1]
	str		r3, [r0]
	add		r0, r0, #0x04
	add		r1, r1, #0x04
	subs	r2, r2, #0x04

	bne		.memcpy4

.memcpy_end:
  	pop		{r0, pc}

.memcpy_opt:
	eor		r3, r0, r1
	tst		r3, #0x03			@ src and dst same 2 bit offset?
	bne		.memcpy_unalign

	tst		r0, #0x01
	beq		.memcpy_opt1

	ldrb	r3, [r1]
	strb	r3, [r0]
	add		r0, r0, #0x01
	add		r1, r1, #0x01
	sub		r2, r2, #0x01

.memcpy_opt1:
	tst		r0, #0x02
	beq		.memcpy_opt16_i

	ldrh	r3, [r1]
	strh	r3, [r0]
	add		r0, r0, #0x02
	add		r1, r1, #0x02
	sub		r2, r2, #0x02

.memcpy_opt16_i:				@ now at aligned address
	push	{r4-r6}

.memcpy_opt16:
	ldmia	r1!, {r3-r6}
	stmia	r0!, {r3-r6}
	subs	r2, r2, #0x04*4
	cmp		r2, #0x04*4
	bhs		.memcpy_opt16

	pop		{r4-r6}

	b		.memcpy_unalign
#endif
	.size	memcpy, .-memcpy

