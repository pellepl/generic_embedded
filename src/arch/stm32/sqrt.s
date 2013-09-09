    .syntax unified
	.cpu cortex-m3
	.fpu softvfp
	.thumb

	.section	.text.memcpy,"ax",%progbits

	.thumb_func
	.global _sqrt

@       Optimised 32-bit integer sqrt
@       Calculates square root of R0
@       Result returned in R0, uses R1 and R2
@       Worst case execution time exactly 70 S cycles
_sqrt:
        MOV     r1,#0
        CMP     r1,r0,LSR #8
        BEQ     .bit8            @ 0x000000xx numbers

        CMP     r1,r0,LSR #16
        BEQ     .bit16           @ 0x0000xxxx numbers

        CMP     r1,r0,LSR #24
        BEQ     .bit24           @ 0x00xxxxxx numbers

        SUBS    r2,r0,#0x40000000
        itt		cs
        MOVCS   r0,r2
        MOVCS   r1,#0x10000

        ADD     r2,r1,#0x4000
        SUBS    r2,r0,r2,LSL#14
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x8000

        ADD     r2,r1,#0x2000
        SUBS    r2,r0,r2,LSL#13
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x4000

.bit24:
        ADD     r2,r1,#0x1000
        SUBS    r2,r0,r2,LSL#12
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x2000

        ADD     r2,r1,#0x800
        SUBS    r2,r0,r2,LSL#11
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x1000

        ADD     r2,r1,#0x400
        SUBS    r2,r0,r2,LSL#10
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x800

        ADD     r2,r1,#0x200
        SUBS    r2,r0,r2,LSL#9
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x400

.bit16:
        ADD     r2,r1,#0x100
        SUBS    r2,r0,r2,LSL#8
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x200

        ADD     r2,r1,#0x80
        SUBS    r2,r0,r2,LSL#7
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x100

        ADD     r2,r1,#0x40
        SUBS    r2,r0,r2,LSL#6
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x80

        ADD     r2,r1,#0x20
        SUBS    r2,r0,r2,LSL#5
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x40

.bit8:
        ADD     r2,r1,#0x10
        SUBS    r2,r0,r2,LSL#4
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x20

        ADD     r2,r1,#0x8
        SUBS    r2,r0,r2,LSL#3
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x10

        ADD     r2,r1,#0x4
        SUBS    r2,r0,r2,LSL#2
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x8

        ADD     r2,r1,#0x2
        SUBS    r2,r0,r2,LSL#1
        itt		cs
        MOVCS   r0,r2
        ADDCS   r1,r1,#0x4

        ADD     r2,r1,#0x1
        CMP     r0,r2
        it		cs
        ADDCS   r1,r1,#0x2

        MOV     r0,r1,LSR#1     @ result in R0

        bx		lr

.size   _sqrt, . - _sqrt
