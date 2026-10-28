/* UHARC 0.6 decompressor interface (C89 portable) */
#ifndef UHARC_DEC_H
#define UHARC_DEC_H

#ifdef _MSC_VER
#define UHI64 __int64
#define U64LL(x) x##UI64
#define I64LL(x) x##I64
#else
#define UHI64 long long
#define U64LL(x) x##ULL
#define I64LL(x) x##LL
#endif

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

/* Same as above but selects the core2 bit order and the version's
 * method->core table (they differ: 0.2 = 8..43 all core3,
 * 0.3 = 8..21/22..35/36..79, 0.4/0.6 = 8..23/24..39/40..87).
 * ver: 2, 3, 4 or 6. core2_lsb: 1 for 0.2/0.3/0.4, 0 for 0.6. */
int uharc_dec_decode_v(int method, int core2_lsb, int ver,
                       uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len);

/* Same as decode_v, but also passes the container's cumulative unpacked
 * file-end table so that cores which restart their models at file
 * boundaries can do so.  cum[] must be non-decreasing and
 * cum[ncum - 1] == out_len.  Pass cum = NULL (ncum = 0) for a single
 * block, which is what decode_v/decode do. */
int uharc_dec_decode_b(int method, int core2_lsb, int ver,
                       const unsigned long *cum, unsigned long ncum,
                       uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len);

/* version-aware core selector (-1 = unknown method) */
int uharc_dec_core_v(int method, int ver);

/* core selectors (for tests) */
int uharc_dec_core(int method);

#endif
