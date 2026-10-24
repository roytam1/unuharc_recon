/* UHARC 0.6 decompressor interface (C89 portable) */
#ifndef UHARC_DEC_H
#define UHARC_DEC_H

/* error codes share uharc06.h values; reuse 0..7 */
typedef int (*uharc_dec_read_cb)(void *ctx, unsigned char *buf,
                                 unsigned want_len);
typedef int (*uharc_dec_write_cb)(void *ctx, const unsigned char *buf,
                                  unsigned len);

/* decode one block:
 * method: raw method id 0..87
 * out_len: expected decompressed size (dword_434514)
 * returns 0 ok, 1 bad args, 2 nomem, 3 input short, 4 output short
 * (matches dword_422B90 semantics)
 */
int uharc_dec_decode(int method,
                     uharc_dec_read_cb rcb, void *rctx,
                     uharc_dec_write_cb wcb, void *wctx,
                     unsigned long out_len);

/* core selectors (for tests) */
int uharc_dec_core(int method);

#endif
