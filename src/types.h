#ifndef __TYPE_H
#define __TYPE_H

typedef signed long  sint32_t;
typedef signed short sint16_t;
typedef signed char  sint8_t;

typedef unsigned long  uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;

typedef enum {FALSE = 0, TRUE = !FALSE} bool;

#define U8_MAX     ((u8)255)
#define S8_MAX     ((s8)127)
#define S8_MIN     ((s8)-128)
#define U16_MAX    ((u16)65535u)
#define S16_MAX    ((s16)32767)
#define S16_MIN    ((s16)-32768)
#define U32_MAX    ((u32)4294967295uL)
#define S32_MAX    ((s32)2147483647)
#define S32_MIN    ((s32)2147483648uL)

#define FALSE       (0)
#define TRUE        (!FALSE)

#define NULL        ((void*)0)

typedef uint32_t u32_t;
typedef sint32_t s32_t;
typedef uint16_t u16_t;
typedef sint16_t s16_t;
typedef uint8_t u8_t;
typedef sint8_t s8_t;

typedef unsigned long time;

#define MIN(x,y)  ((x)<(y)?(x):(y))
#define MAX(x,y)  ((x)>(y)?(x):(y))
#define ABS(x)    ((x)<0?(-(x)):(x))

#define STR(tok) #tok

#endif /* __TYPE_H */
