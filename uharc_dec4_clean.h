/* UHARC 0.6 core 4 (PPM) - clean typed implementation (C89).
 *
 * This is a from-scratch port of uharc-rs ppm.rs (itself a typed
 * re-implementation of UNUHARC06 0x40cdd1/0x40f962). It replaces the
 * 32-bit arena emulation in uharc_dec4.c with explicit C state.
 *
 * Status: complete. The port passes the whole 0.6 corpus (103/103) and
 * is the decoder uharc_dec0.c uses for 0.6 core 4. uharc_dec4.c stays
 * in the tree as the arena reference; build with -DDEC4_ARENA to
 * select it for differential testing.
 */
#ifndef UHARC_DEC4_CLEAN_H
#define UHARC_DEC4_CLEAN_H

#include "uharc_dec.h"

/* Mirror uharc_dec4.c API (methods 24..39) for the dispatcher. */
int uharc_dec4c_init(void **ctx, int method);
void uharc_dec4c_free(void *ctx);
int uharc_dec4c_decode_b(void *ctx, const unsigned long *cum, unsigned long ncum,
                         uharc_dec_read_cb rcb, void *rctx,
                         uharc_dec_write_cb wcb, void *wctx,
                         unsigned long out_len);
void uharc_dec4c_set_flags2(unsigned char flags2);

#endif
