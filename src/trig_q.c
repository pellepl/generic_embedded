/*
 * trig_q15_16.c
 *
 *  Created on: Aug 18, 2013
 *      Author: petera
 */
#include "trig_q.h"

#define _SIN_B      ((4 << 16) / PI_TRIG)
#define _SIN_C      (-(4 << 16) / (PI_TRIG * PI_TRIG))

#define _ATAN2__273_D_PI    (0x163f/2)
#define _ATAN2___25         (0x4000/2)

static inline s32_t _nabs(s32_t j) {
  return (j < 0 ? j : -j);
}

s32_t mul_q15(s32_t j, s32_t k) {
  int floor = j * k;
  return (floor + ((floor & 0xffff) == 0x4000 ? 0 : 0x4000)) >> 15;
}

s32_t div_q15(s32_t n, s32_t d) {
  return d == 0 ? 0 : (n << 15) / d;
}

/**
 * Approximate sinus.
 * @param a     -PI_TRIG..PI_TRIG Q9
 * @return      -0x10000..0x10000 Q16
 */
s32_t sin_approx(s32_t a) {
  // modulus -pi..pi
  a = ((a + PI_TRIG) & ((PI_TRIG<<1)-1)) - PI_TRIG;
  int sin = _SIN_B * a + _SIN_C * a * (a < 0 ? -a : a);
  return sin;
}

/**
 * Approximate cosinus.
 * @param a     -PI_TRIG..PI_TRIG Q9
 * @returns     -0x10000..0x10000 Q16
 */
s32_t cos_approx(s32_t a) {
  return sin_approx(a + (PI_TRIG >> 1));
}

/**
 * Approximate atan2.
 * @param y   -0x8000..0x7fff Q15
 * @param x   -0x8000..0x7fff Q15
 * @returns   0..0xffff (=2*PI)
 */
u16_t atan2_approx(s32_t y, s32_t x) {
  if (x == y) { // x/y or y/x would return -1 since 1 isn't representable
    if (y > 0) { // 1/8
      return 8192;
    } else if (y < 0) { // 5/8
      return 40960;
    } else { // x = y = 0
      return 0;
    }
  }
  s16_t nabs_y = _nabs(y), nabs_x = _nabs(x);
  if (nabs_x >= nabs_y) {
    // octants 2, 3, 6, 7
    s16_t x_over_y = div_q15(x, y);
    s16_t correction = mul_q15(_ATAN2__273_D_PI, _nabs(x_over_y));
    s16_t unrotated = mul_q15(_ATAN2__273_D_PI + _ATAN2___25 + correction, x_over_y);
    if (y > 0) { // octants 2, 3
      return 0x4000 - unrotated;
    } else { // octants 6, 7
      return 0xc000 - unrotated;
    }
  } else {
    // octants 1, 4, 5, 8
    s16_t y_over_x = div_q15(y, x);
    s16_t correction = mul_q15(_ATAN2__273_D_PI, _nabs(y_over_x));
    s16_t unrotated = mul_q15(_ATAN2__273_D_PI + _ATAN2___25 + correction, y_over_x);
    if (x > 0) { // octants 1, 8
      return unrotated;
    } else { // octants 4, 5
      return 0x8000 + unrotated;
    }
  }
}
