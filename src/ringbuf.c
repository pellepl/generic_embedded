/*
 * ringbuf.c
 *
 *  Created on: Aug 9, 2013
 *      Author: petera
 */

#include "ringbuf.h"
#include "miniutils.h"
#define RB_AVAIL(rix, wix) \
  (wix >= rix ? (wix - rix) : (rb->max_len - (rix - wix)))
#define RB_FREE(rix, wix) \
  (rb->max_len - (wix >= rix ? (wix - rix) : (rb->max_len - (rix - wix))) - 1)
#define RB_EMPTY(rix, wix) \
  (rix == wix)
#define RB_FULL(rix, wix) \
  ((rix == 0 && wix == rb->max_len-1) || wix == rix-1)

void ringbuf_init(ringbuf *rb, u8_t *buffer, u16_t max_len) {
  rb->max_len = max_len;
  rb->buffer = buffer;
  rb->r_ix = 0;
  rb->w_ix = 0;
}

int ringbuf_getc(ringbuf *rb, u8_t *c) {
  u16_t rix = rb->r_ix;
  u16_t wix = rb->w_ix;
  if (RB_EMPTY(rix, wix)) {
    return RB_ERR_EMPTY;
  }
  if (c) {
    *c = rb->buffer[rix];
  }
  if (rix >= rb->max_len-1) {
    rix = 0;
  } else {
    rix++;
  }
  rb->r_ix = rix;

  return RB_OK;
}


int ringbuf_putc(ringbuf *rb, u8_t c) {
  u16_t rix = rb->r_ix;
  u16_t wix = rb->w_ix;
  if (RB_FULL(rix, wix)) {
    return RB_ERR_FULL;
  }
  rb->buffer[rix] = c;
  if (wix >= rb->max_len-1) {
    wix = 0;
  } else {
    wix++;
  }
  rb->w_ix = wix;

  return RB_OK;
}

int ringbuf_available(ringbuf *rb) {
  u16_t rix = rb->r_ix;
  u16_t wix = rb->w_ix;
  return RB_AVAIL(rix, wix);
}

int ringbuf_available_linear(ringbuf *rb, u8_t **ptr) {
  u16_t rix = rb->r_ix;
  u16_t wix = rb->w_ix;

  if (rix == wix) {
    return 0;
  } else {
    u16_t avail = RB_AVAIL(rix, wix);
    *ptr = &rb->buffer[rix];

    if (rix + avail < rb->max_len) {
      return avail;
    } else {
      return rb->max_len - rix;
    }
  }
}

int ringbuf_free(ringbuf *rb) {
  u16_t rix = rb->r_ix;
  u16_t wix = rb->w_ix;
  return RB_FREE(rix, wix);
}

int ringbuf_put(ringbuf *rb, u8_t *buf, u16_t len) {
  u16_t rix = rb->r_ix;
  u16_t wix = rb->w_ix;
  int to_write;
  if (RB_FULL(rix, wix)) {
    return RB_ERR_FULL;
  }
  u16_t free = RB_FREE(rix, wix);
  if (len > free) {
    len = free;
  }
  to_write = len;
  if (wix + len >= rb->max_len) {
    u16_t part = rb->max_len - wix;
    memcpy(&rb->buffer[wix], buf, part);
    buf +=part;
    to_write -= part;
    wix = 0;
  }
  if (to_write > 0) {
    memcpy(&rb->buffer[wix], buf, to_write);
    wix += to_write;
  }
  if (wix >= rb->max_len) {
    wix = 0;
  }
  rb->w_ix = wix;

  return len;
}

int ringbuf_get(ringbuf *rb, u8_t *buf, u16_t len) {
  u16_t rix = rb->r_ix;
  u16_t wix = rb->w_ix;
  int to_read;
  if (RB_EMPTY(rix, wix)) {
    return RB_ERR_EMPTY;
  }
  u16_t avail = RB_AVAIL(rix, wix);;
  if (len > avail) {
    len = avail;
  }
  to_read = len;
  if (rix + len >= rb->max_len) {
    u16_t part = rb->max_len - rix;
    if (buf) {
      memcpy(buf, &rb->buffer[rix], part);
      buf += part;
    }
    to_read -= part;
    rix = 0;
  }
  if (to_read > 0) {
    if (buf) {
      memcpy(buf, &rb->buffer[rix], to_read);
    }
     rix += to_read;
  }
  if (rix >= rb->max_len) {
    rix = 0;
  }

  rb->r_ix = rix;

  return len;
}

