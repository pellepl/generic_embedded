/*
 * memset.s
 *
 *  Created on: Jul 1, 2012
 *      Author: petera
 */

    .syntax unified
	.cpu cortex-m3
	.fpu softvfp
	.thumb

	.section	.text.memset,"ax",%progbits

	.thumb_func
	.global memset

memset:
	push	{lr}
#if 0
#	mov		r3, r0
#.loop:
#	strb	r1, [r3]
#	add		r3, r3, #0x01
#	subs	r2, r2, #0x01
#	bne		.loop
 # 	pop		{pc}
#else
    mov     r3, r0

	bfi		r1, r1, #8, #8
	bfi		r1, r1, #16, #16

	tst		r2, #0x01
	beq		.memset1

	strb	r1, [r3]
	add		r3, r3, #0x01
	sub		r2, r2, #0x01

.memset1:
	tst		r2, #0x02
	beq		.memset2

	strh	r1, [r3]
	add		r3, r3, #0x02
	sub		r2, r2, #0x02

.memset2:
	cmp		r2, #0x00
	beq		.memset_end

.memset4:						@ word for word, no alignment necessary
	str		r1, [r3]
	add		r3, r3, #0x04
	subs	r2, r2, #0x04

	bne		.memset4

.memset_end:
  	pop		{pc}
#endif
	.size	memset, .-memset
