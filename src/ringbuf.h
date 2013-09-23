/*
 * ringbuf.h
 *
 *  Created on: Aug 9, 2013
 *      Author: petera
 */

#ifndef RINGBUF_H_
#define RINGBUF_H_

#include "system.h"

#define RB_OK               0
#define RB_ERR_EMPTY        -1
#define RB_ERR_FULL         -2

typedef struct {
  u8_t *buffer;
  volatile u16_t r_ix;
  volatile u16_t w_ix;
  u16_t max_len;
} ringbuf;

/* Initiates a ring buffer */
void ringbuf_init(ringbuf *rb, u8_t *buffer, u16_t max_len);
/* Returns a character from ringbuffer
   @returns RB_OK or RB_ERR_EMPTY
 */
int ringbuf_getc(ringbuf *rb, u8_t *c);
/* Puts a character into ringbuffer
   @returns RB_OK or RB_ERR_FULL
 */
int ringbuf_putc(ringbuf *rb, u8_t c);
/* Returns a region of data from ringbuffer
   @returns number of actual bytes returned or RB_ERR_EMPTY
 */
int ringbuf_get(ringbuf *rb, u8_t *buf, u16_t len);
/* Writes a region of data into ringbuffer.
   @param buf can be null, whereas the read pointer is simply advanced
   @returns number of actual bytes written or RB_ERR_FULL
 */
int ringbuf_put(ringbuf *rb, u8_t *buf, u16_t len);
/* Returns current write capacity of ringbuffer */
int ringbuf_free(ringbuf *rb);
/* Returns current read capacity of ringbuffer */
int ringbuf_available(ringbuf *rb);
/* Returns linear read capacity and pointer to buffer.
   The read pointer can be advanced by calling ringbuf_get with
   null argument for buf.
 */
int ringbuf_available_linear(ringbuf *rb, u8_t **ptr);
/*  Empties ringbuffer  */
int ringbuf_clear(ringbuf *rb);

#endif /* RINGBUF_H_ */
