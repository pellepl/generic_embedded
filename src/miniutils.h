/*
 * miniutils.h
 *
 *  Created on: Jul 7, 2012
 *      Author: petera
 */

#ifndef MINIUTILS_H_
#define MINIUTILS_H_

#include <stdarg.h>
#include "miniutils_config.h"
#include "system.h"

#ifdef USE_COLOR_CODING
#define TEXT_GOOD(s) "\033[1;32m"s"\033[m"
#define TEXT_BAD(s) "\033[1;35m"s"\033[m"
#define TEXT_NOTE(s) "\033[1;36m"s"\033[m"
#else
#define TEXT_GOOD(s) s
#define TEXT_BAD(s) s
#define TEXT_NOTE(s) s
#endif

typedef struct {
  char* s;
  char* wrk;
  int len;
} cursor;

typedef enum {
  INT,
  STR
} strarg_type;

typedef struct {
  strarg_type type;
  union {
    int val;
    char* str;
  };
  int len;
} strarg;

#ifndef print
// format prints to console
void print(const char* f, ...);
#endif
// format prints to specified io
void ioprint(int io, const char* f, ...);
// format prints to specified string
void sprint(char *s, const char* f, ...);
// format prints to specified string with specified va_list
void vsprint(char *s, const char* f, va_list arg_p);
// format prints to console with specified va_list
void vprint(const char* f, va_list arg_p);
// format prints to specified io with specified va_list
void vioprint(int io, const char* f, va_list arg_p);
// prints a neatly formatted data / ascii table of given buffer
void printbuf(u8_t io, u8_t *buf, u16_t len);
// primitive used for all printing
void v_printf(long p, const char* f, va_list arg_p);
// transforms v to an ascii string in given base
void itoa(int v, char* dst, int base);
// transforms v to an ascii string in given base at maximum length
void itoan(int v, char* dst, int base, int num);
// reads string and inteprets ascii as integer
int atoi(const char* s);
// reads string of given length and inteprets ascii as integer in given base
int atoin(const char* s, int base, int len);
// calculates length of string
int strlen(const char* c);
// calculates length of string up to maximum size
int strnlen(const char *c, int size);
// compares two strings, returns 0 if equal
int strcmp(const char* s1, const char* s2);
// compares two strings up to given number of characters, returns 0 if equal
int strncmp(const char* s1, const char* s2, int len);
// compares beginning of string s with prefix, returns 0 if match
int strcmpbegin(const char* prefix, const char* s);
// copies a string of given length or less to destination
char* strncpy(char* d, const char* s, int num);
// copies a string to destination
char* strcpy(char* d, const char* s);
// finds character in string and returns pointer to character found or 0
const char* strchr(const char* str, int ch);
// finds any of given characters in string and returns pointer to character found or 0
char* strpbrk(const char* str, const char* key);
// finds substring in string and returns pointer to substring found or 0
char* strstr(const char* str, const char* sub);
// calculates 16 bit crc
unsigned short crc_ccitt_16(unsigned short crc, unsigned char data);
// calculates pseudo random value given seed
unsigned int rand(unsigned int seed);
// seeds common pseudo random generator
void rand_seed(unsigned int seed);
// returns next pseudo random number
unsigned int rand_next();
// quicksorts given elements of given orders, both arrays must contain same amount of entries
void quicksort(int* orders, void** pp, int elements);
// quicksorts given elements of order given by function, both arrays must contain same amount of entries
void quicksort_cmp(int* orders, void** pp, int elements,
    int(*orderfunc)(void* p));
// initiates a cursor of given string and length, if length given is 0 it is calculated
void strarg_init(cursor *c, char* s, int len);
// parses next argument as string or value from cursor and places result in arg,
// returns nonzero if arg is found or zero otherwise
int strarg_next(cursor *c, strarg* arg);
// parses next argument as string from cursor and places result in arg
// returns nonzero if arg is found or zero otherwise
int strarg_next_str(cursor *c, strarg* arg);
// parses next argument as string or value from cursor and places result in arg,
// delimited by given delimiter characters,
// returns nonzero if arg is found or zero otherwise
int strarg_next_delim(cursor *c, strarg* arg, const char *delim);
// parses next argument as string from cursor and places result in arg,
// delimited by given delimiter characters,
// returns nonzero if arg is found or zero otherwise
int strarg_next_delim_str(cursor *c, strarg* arg, const char *delim);

#endif /* MINIUTILS_H_ */
