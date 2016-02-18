#ifndef __TYPE_H
#define __TYPE_H

typedef signed long long int64_t;
typedef signed long  int32_t;
typedef signed short int16_t;
typedef signed char  int8_t;

typedef unsigned long long uint64_t;
typedef unsigned long  uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;

typedef enum {FALSE = 0, TRUE = !FALSE} bool;

#define U8_MAX     ((u8_t)255)
#define S8_MAX     ((s8_t)127)
#define S8_MIN     ((s8_t)-128)
#define U16_MAX    ((u16_t)65535u)
#define S16_MAX    ((s16_t)32767)
#define S16_MIN    ((s16_t)-32768)
#define U32_MAX    ((u32_t)4294967295uL)
#define S32_MAX    ((s32_t)2147483647)
#define S32_MIN    ((s32_t)-2147483648)

#define FALSE       (0)
#define TRUE        (!FALSE)

#define NULL        ((void*)0)

typedef uint64_t u64_t;
typedef int64_t s64_t;
typedef uint32_t u32_t;
typedef int32_t s32_t;
typedef uint16_t u16_t;
typedef int16_t s16_t;
typedef uint8_t u8_t;
typedef int8_t s8_t;

#ifdef CONFIG_SYS_TIME_64_BIT
typedef u64_t sys_time;
#else
typedef u32_t sys_time;
#endif

#endif /* __TYPE_H */
