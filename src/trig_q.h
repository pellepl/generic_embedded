/*
 * trig_q.h
 *
 *  Created on: Aug 20, 2013
 *      Author: petera
 */

#ifndef TRIG_Q_H_
#define TRIG_Q_H_

#include "system.h"

#define PI_TRIG     (1<<9)

s32_t mul_q15(s32_t j, s32_t k);

s32_t div_q15(s32_t n, s32_t d);

/**
 * Approximate sinus.
 * @param a     -PI_TRIG..PI_TRIG Q9
 * @return      -0x10000..0x10000 Q16
 */
s32_t sin_approx(s32_t a);

/**
 * Approximate cosinus.
 * @param a     -PI_TRIG..PI_TRIG Q9
 * @returns     -0x10000..0x10000 Q16
 */
s32_t cos_approx(s32_t a);

/**
 * Approximate atan2.
 * @param y   -0x8000..0x7fff Q15
 * @param x   -0x8000..0x7fff Q15
 * @returns   0..0xffff (=2*PI) Q15
 */
u16_t atan2_approx(s32_t y, s32_t x);

#endif /* TRIG_Q_H_ */
