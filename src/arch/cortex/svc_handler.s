/*
 * svc_handler.s
 *
 *  Created on: Mar 10, 2013
 *      Author: petera
 */

@ ----------------------------------------------------------------------------

    .syntax unified
	.cpu cortex-m3
	.fpu softvfp
	.thumb

	.section	.text.svc_handler,"ax",%progbits

	.thumb_func
	.global SVC_Handler

SVC_Handler:
	tst		lr, #4
	ite		eq
	mrseq	r0, msp
	mrsne	r0, psp
	b		__os_svc_handler

        .size   SVC_Handler, . - SVC_Handler
