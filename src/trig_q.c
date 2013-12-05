/*
 * trig_q15_16.c
 *
 *  Created on: Aug 18, 2013
 *      Author: petera
 */
#include "trig_q.h"

#ifdef CONFIG_TRIGQ_TABLE
static const u16_t const _sinus_quarter[129] = {
  0x0000,0x0194,0x0327,0x04bb,0x064e,0x07e1,0x0974,0x0b06,0x0c98,0x0e2a,0x0fbb,0x114b,0x12db,0x146a,0x15f8,0x1785,
  0x1911,0x1a9d,0x1c27,0x1db0,0x1f39,0x20bf,0x2245,0x23c9,0x254c,0x26ce,0x284e,0x29cc,0x2b49,0x2cc4,0x2e3d,0x2fb5,
  0x312a,0x329e,0x3410,0x3580,0x36ed,0x3859,0x39c2,0x3b29,0x3c8e,0x3df1,0x3f51,0x40ae,0x420a,0x4362,0x44b8,0x460b,
  0x475c,0x48aa,0x49f4,0x4b3d,0x4c82,0x4dc4,0x4f03,0x503f,0x5178,0x52ae,0x53e0,0x5510,0x563c,0x5764,0x588a,0x59ab,
  0x5aca,0x5be5,0x5cfc,0x5e0f,0x5f1f,0x602c,0x6134,0x6239,0x633a,0x6437,0x6530,0x6625,0x6717,0x6804,0x68ed,0x69d2,
  0x6ab3,0x6b90,0x6c69,0x6d3e,0x6e0e,0x6eda,0x6fa2,0x7065,0x7124,0x71df,0x7295,0x7346,0x73f4,0x749c,0x7541,0x75e0,
  0x767b,0x7712,0x77a4,0x7831,0x78b9,0x793d,0x79bc,0x7a37,0x7aac,0x7b1d,0x7b89,0x7bf0,0x7c53,0x7cb1,0x7d09,0x7d5d,
  0x7dac,0x7df7,0x7e3c,0x7e7c,0x7eb8,0x7eee,0x7f20,0x7f4c,0x7f74,0x7f97,0x7fb5,0x7fce,0x7fe2,0x7ff0,0x7ffa,0x7fff,
  0x8000
};
#endif

#define _PI_TRIG     (1<<9)

#define _SIN_B      ((4 << 16) / _PI_TRIG)
#define _SIN_C      (-(4 << 16) / (_PI_TRIG * _PI_TRIG))

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
 * @param a     -_PI_TRIG.._PI_TRIG Q9
 * @return      -0x10000..0x10000 Q16
 */
s32_t sin_approx(s32_t a) {
  // modulus -pi..pi
  a = ((a + _PI_TRIG) & ((_PI_TRIG<<1)-1)) - _PI_TRIG;
  int sin = _SIN_B * a + _SIN_C * a * (a < 0 ? -a : a);
  return sin;
}

/**
 * Approximate cosinus.
 * @param a     -_PI_TRIG.._PI_TRIG Q9
 * @returns     -0x10000..0x10000 Q16
 */
s32_t cos_approx(s32_t a) {
  return sin_approx(a + (_PI_TRIG >> 1));
}

#ifdef CONFIG_TRIGQ_TABLE
s32_t sin_table(s32_t a) {
  a = (a & (PI_TRIG_T-1));
  if (a < 128)
    // 0..127
    // 0..127
    return _sinus_quarter[a];
  else if (a < 256)
    // 128..255
    // 127..0
    return _sinus_quarter[255-a];
  else if (a < 384)
    // 256..383
    // -0..128
    return -_sinus_quarter[a-256];
  else
    // 384..512
    // -127..0
    return -_sinus_quarter[511-a];
}

s32_t cos_table(s32_t a) {
  a = (a  & (PI_TRIG_T-1));
  if (a < 128)
    // 0..127
    // 127..0
    return _sinus_quarter[127-a];
  else if (a < 256)
    // 128..255
    // -0..127
    return -_sinus_quarter[a-128];
  else if (a < 384)
    // 256..383
    // -127..0
    return -_sinus_quarter[384-a];
  else
    // 384..512
    // 0..127
    return _sinus_quarter[a-384];
}

#endif

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
