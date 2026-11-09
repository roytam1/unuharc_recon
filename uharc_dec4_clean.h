/* UHARC 0.6 core 4 (PPM) - clean typed implementation (C89).
 *
 * This is a from-scratch port of uharc-rs ppm.rs (itself a typed
 * re-implementation of UNUHARC06 0x40cdd1/0x40f962). It replaces the
 * 32-bit arena emulation in uharc_dec4.c with explicit C state.
 *
 * Status: scaffolding. Range coder + header (0x413a96) are done;
 * the main loop (0x40f962) is not yet ported. The arena decoder
 * remains the default; this module is built and tested but not
 * wired into uharc_dec0.c until it passes the 103-archive corpus.
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
