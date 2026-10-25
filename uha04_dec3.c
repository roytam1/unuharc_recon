/* UHARC 0.4 decompressor core 3 (ALZ, methods 8..23).
 *
 * Clean portable C89 port of the UHARC 0.4 BETA core-3 decoder.
 * Reverse engineered from UHARC04.lst (the Hex-Rays output UHARC04.c is
 * unreliable for this core: it confuses the encoder with the decoder).
 * No Windows API, no globals; all state lives in dec3v and is
 * malloc'ed/free'ed.  No output unless U4_DEBUG is defined to 1.
 *
 * Original -> port mapping (UHARC04.lst):
 *  sub_423760 (init)           -> uharc_dec3v_init
 *      method 8..23; dict = 1 << (((method-8) % 16) + 10);
 *      allocations: 0x200000 htab (pos/ctx per 18-bit key, pos init
 *      0xFFFFFFFF), 0x40000 hval (prev length per hash slot),
 *      0x40000 ord (65536 u32, init 0xFFFFFFFF), dict+0x100 (zeroed),
 *      0x29400 freq (330 x 256 u16), 0x29400 raw, 0x290 escw (328 u16).
 *  sub_423A10 (model reset)    -> d4_reset  (freq/raw[0..327] zeroed;
 *      order-0 contexts 328 and 329 get the 0x436ED8 lowbit template
 *      *8 in freq and 8 in raw; escw[0..327]=1; the 50 x 8-byte literal
 *      entries are copied from the 0x437110 template)
 *  sub_423B70 (rescale ctx)    -> d4_rescale
 *  sub_423CC0 (model update)   -> d4_update
 *  sub_423E00 (litm update)    -> d4_bitupd
 *  sub_4248C0 (match-bit)      -> d4_bit
 *  sub_423F50 (symbol decode)  -> d4_sym   (context symbol + escape)
 *  sub_424400 (literal)        -> d4_literal
 *  sub_424530 (match/decision) -> d4_match
 *  sub_423E90 (main loop)      -> uharc_dec3v_decode
 *  sub_426F30 (coder init)     -> d4_rc_init (code=0, range=-1, 5 bytes BE)
 *  sub_4220B0 (input byte)     -> d4_get (via read callback)
 *  sub_4221F0 (output byte)    -> d4_emit (via write callback)
 *
 * 0.4-specific details that differ from the verified 0.6 core-3 port
 * (uharc_dec3.c):
 *  - 330 model contexts: 328 ordinary/length contexts (escw 0..327) plus
 *    two order-0 contexts.  Contexts < 0x13E use order-0 model 328,
 *    contexts >= 0x13E (the length contexts) use order-0 model 329.
 *  - Match lengths are coded as ordinary symbols, not with the 0.6
 *    probability tree: the context is 0x4372B0[prev length at this hash]
 *    (318/319/320/321/322/323/324/325) or 326/327 in run mode.  The
 *    decoded length is 0..255, stored back into hval[]; like 0.3 a
 *    length of 0 means 256 bytes and 0xFF means 255 bytes plus run++.
 *  - The literal (match/literal) probabilities are 50 x 8-byte
 *    {e0,e1,e2,pad} entries seeded 11..14 / 6..9 / 49 / 79 with e2=99,
 *    and rescale as e1' = e0 + ((e1-e0+1)>>1),
 *    e2' = e1' + ((e2-e1+1)>>1) (not the 0.6 halving).
 *  - Model reset happens at container file boundaries at least 64 KiB of
 *    output apart (the original walks the per-file progress table; see
 *    d4_progress) and is applied *after* the step that crosses the
 *    boundary, so it lands mid-file.  The dictionary and output stream are
 *    continuous across blocks.
 *  - No mval[], no 0.6 "state" parameter table beyond the shared
 *    0x4372A0 eight-state history transition table (same as 0.6).
 *
 * The 0.4 core-3 model/table constants were read from the DGROUP data
 * in UHARC04.lst:
 *  0x436ED8  256-word lowbit template (word[0]=256, else lowbit(i))
 *  0x437110  50 x {u16 0, u16 seed, u16 99, u16 0}
 *  0x4372A0  16-byte state transition table (== 0.6 tab_st0)
 *  0x4372B0  256-dword previous-length -> length-context table
 *  0x4376B0  256-byte order class table (0 in 133..158, 165..190)
 *
 * Oracle note: the 0.4 name table (tab1) is a parallel array to tab0
 * (file CRCs) and tab3 (size/method), i.e. its order is the archive
 * entry order.  For UHACTEST/04_m0.uha the decrypted tab0 CRC list is
 *   00000000 7125DAA3 51A02E01 B70B4C26 021E1879 6F3BDB8B
 * i.e. empty.bin, text.txt, repeat.txt, binary.bin, random.bin, call.bin
 * and method 17 must reproduce exactly
 *   00 09 "empty.bin" 08 "text.txt" 0A "repeat.txt" 0A "binary.bin"
 *   0A "random.bin" 08 "call.bin"   (62 bytes).
 *
 * Return codes: 0 ok, 1 bad args, 2 nomem, 3 input short, 4 output short,
 * 7 data error (corrupt stream).
 */
#include <stdlib.h>
#include <string.h>

#include "uharc_dec.h"

/* public API (see uharc_dec.h for the callback typedefs) */
int uharc_dec3v_init(void **pctx, int method);
int uharc_dec3v_decode(void *ctx, uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len);
int uharc_dec3v_decode_b(void *ctx, const unsigned long *cum,
                         unsigned long ncum, uharc_dec_read_cb rcb, void *rctx,
                         uharc_dec_write_cb wcb, void *wctx,
                         unsigned long out_len);
void uharc_dec3v_free(void *ctx);

#ifndef U4_DEBUG
#define U4_DEBUG 0
#endif

#if U4_DEBUG
#include <stdio.h>
#endif

#define D4_M32 0xFFFFFFFFUL
#define D4_M16 0xFFFFUL

#define D4_NCTX 330u   /* allocated contexts: 0..327 + order0 328,329 */
#define D4_NREAL 328u  /* first order-0 context */
#define D4_NLITM 50u   /* litm entries */
#define D4_NHASH 262144u /* hash slots */
#define D4_NORD 65536u   /* 16-bit context slots */

typedef struct dec3v_s {
    /* callbacks / I/O */
    uharc_dec_read_cb rcb;
    void *rctx;
    uharc_dec_write_cb wcb;
    void *wctx;
    unsigned char *inb;
    unsigned in_pos;
    unsigned in_avail;
    unsigned in_cap;
    unsigned char *outb;
    unsigned out_fill;
    unsigned out_cap;
    unsigned long out_total;
    unsigned long out_len;
    int err;

    /* range coder (dword_45D0B8 code, dword_45D0BC range) */
    unsigned long code;
    unsigned long range;

    /* model */
    unsigned long *htab;      /* 0x200000: 262144 x {u32 pos, u32 ctx} */
    unsigned char *hval;      /* 0x40000: previous length per hash slot */
    unsigned long *ord;       /* 0x40000: 65536 u32 last pos per 16-bit ctx */
    unsigned char *dict;      /* dict_sz + 256 ring + mirror */
    unsigned short *freq;     /* 330 x 256 Fenwick words (f[0] = total) */
    unsigned short *raw;      /* 330 x 256 symbol counts */
    unsigned short *escw;     /* 328 escape weights */
    unsigned short litm[D4_NLITM][4];
    unsigned char state;      /* byte_45CF50: litm history, init 0 */

    /* decoder state */
    unsigned long dict_sz;
    unsigned long dict_mask;
    unsigned long dict_pos;   /* 45CF48 */
    unsigned long ctx;        /* 45CF30 */
    long excl;                /* 45CF34: dict index or -1 */
    long match_pos;           /* 45CF2C: dict index or -1 */
    unsigned long run_len;    /* 45CF40 */
    unsigned long last_dist;  /* 45CF38 */
    unsigned long thresh;     /* 45CF24: output size that allows a reset */
    const unsigned long *cum; /* container progress table (45CF14) */
    unsigned long ncum;       /* entries in cum[] (0 = no table) */
    unsigned long prog_idx;   /* 45CF10: index into cum[] */
} dec3v;

/* 0x437110 literal template, word 1 of each 8-byte entry (e0=0, e2=99) */
static const unsigned short d4_litw1[D4_NLITM] = {
     11,  12,  13,  14,  14,  14,  14,  14,
     11,  12,  13,  14,  14,  14,  14,  14,
     11,  12,  13,  14,  14,  14,  14,  14,
      6,   7,   8,   9,   9,   9,   9,   9,
      6,   7,   8,   9,   9,   9,   9,   9,
      6,   7,   8,   9,   9,   9,   9,   9,
     49,  79
};

/* 0x4372A0 state transition pairs, indexed [2*state + bit], state < 8 */
static const unsigned char d4_st0[16] = {
    6, 0, 6, 0, 6, 1, 6, 2, 6, 3, 6, 3, 7, 4, 7, 5
};

/* 0x4372B0: previous length (hval) -> length symbol context */
static const unsigned short d4_lenctx[12] = {
    0x13E, 0x13E, 0x13F, 0x140, 0x141, 0x142,
    0x143, 0x143, 0x144, 0x144, 0x144, 0x144
};

/* order class (0x4376B0): 0 in two ranges, else 1 */
static int d4_ord(unsigned c)
{
    if ((c >= 133 && c <= 158) || (c >= 165 && c <= 190)) return 0;
    return 1;
}

/* length context for a previous length (0x4372B0, entries 12..255 = 0x145) */
static unsigned d4_len_context(unsigned prev)
{
    if (prev < 12u) return d4_lenctx[prev];
    return 0x145u;
}

/* order-0 context for a model context (328 + (ctx >= 0x13E)) */
static unsigned d4_ord0(unsigned ci)
{
    return (ci >= 0x13Eu) ? 329u : 328u;
}

/* ---- I/O pump (sub_4220B0 / sub_4221F0) ---- */
static int d4_get(dec3v *d)
{
    int n;
    if (d->err) return 0;
    if (d->in_pos >= d->in_avail) {
        n = d->rcb(d->rctx, d->inb, d->in_cap);
        if (n <= 0) {
            d->err = 3;
            return 0;
        }
        d->in_avail = (unsigned)n;
        d->in_pos = 0;
    }
    return d->inb[d->in_pos++];
}

static void d4_emit(dec3v *d, unsigned b)
{
    if (d->out_total < d->out_len) {
        d->outb[d->out_fill++] = (unsigned char)b;
        if (d->out_fill == d->out_cap) {
            if (d->wcb(d->wctx, d->outb, d->out_fill) != 0) d->err = 4;
            d->out_fill = 0;
        }
    }
    d->out_total++;
}

/* ---- range coder (sub_426F30) ---- */
static void d4_rc_init(dec3v *d)
{
    int i;
    d->code = 0;
    d->range = D4_M32;
    for (i = 0; i < 5; i++) {
        d->code = ((d->code << 8) | (unsigned long)d4_get(d)) & D4_M32;
    }
}

/* narrow (code,range) to [lo,hi) of the already-scaled range; renorm.
 * Mirrors the inlined loops of sub_423F50 / subst_4248C0 / sub_424400:
 *   range = (hi-lo)*range; code -= lo*range;
 *   while (range < 0x1000000) { code = code<<8 | getbyte; range <<= 8; }
 */
static int d4_renorm(dec3v *d, unsigned long lo, unsigned long hi)
{
    unsigned long rng;
    unsigned long code;
    if (hi <= lo) {
        d->err = 7;
        return 0;
    }
    rng = (d->range * ((hi - lo) & D4_M32)) & D4_M32;
    code = (d->code - ((d->range * lo) & D4_M32)) & D4_M32;
    if (rng == 0) {
        d->err = 7;
        return 0;
    }
    while (rng < 0x1000000UL) {
        code = ((code << 8) | (unsigned long)d4_get(d)) & D4_M32;
        rng = (rng << 8) & D4_M32;
        if (d->err) return 0;
    }
    d->code = code;
    d->range = rng;
    return 1;
}

/* ---- model reset (sub_423A10) ---- */
static void d4_reset(dec3v *d)
{
    unsigned i;
    unsigned short *f328, *f329;
    unsigned short *r328, *r329;
    memset(d->freq, 0, (size_t)D4_NREAL * 512u);
    memset(d->raw, 0, (size_t)D4_NREAL * 512u);
    f328 = d->freq + 328u * 256u;
    f329 = d->freq + 329u * 256u;
    r328 = d->raw + 328u * 256u;
    r329 = d->raw + 329u * 256u;
    for (i = 0; i < 256; i++) {
        unsigned short v = (unsigned short)((i == 0) ? 256u : (i & (0u - i)));
        f328[i] = (unsigned short)(v * 8u);
        f329[i] = (unsigned short)(v * 8u);
        r328[i] = 8;
        r329[i] = 8;
    }
    for (i = 0; i < D4_NREAL; i++) d->escw[i] = 1;
    for (i = 0; i < D4_NLITM; i++) {
        d->litm[i][0] = 0;
        d->litm[i][1] = d4_litw1[i];
        d->litm[i][2] = 99;
        d->litm[i][3] = 0;
    }
    d->thresh = 0x10000UL;
}

/* One step of the container progress walk (sub_423E90, inlined there and
 * running after every decoded match/literal):
 *   for (i = idx; ; i = idx + 1) {
 *       v = table[i]; idx = i;
 *       if (out <= v) break;
 *       if (v > thresh) { reset; thresh = out + 0x10000; }
 *   }
 * i.e. advance to the first cumulative file end >= out and reset whenever a
 * crossed file end lies more than 64 KiB past the previous reset.  The reset
 * therefore lands one step *after* the boundary, mid-file.
 */
static void d4_progress(dec3v *d)
{
    if (!d->cum || !d->ncum) return;
    while (d->prog_idx < d->ncum - 1UL &&
           d->out_total > d->cum[d->prog_idx]) {
        unsigned long v = d->cum[d->prog_idx];
        d->prog_idx++;
        if (v > d->thresh) {
            d4_reset(d);
            d->thresh = d->out_total + 0x10000UL;
        }
    }
}

/* ---- rescale one context (sub_423B70) ---- */
static void d4_rescale(dec3v *d, unsigned ci)
{
    unsigned short *raw = d->raw + ci * 256u;
    unsigned short *fr = d->freq + ci * 256u;
    unsigned long total = 0;
    unsigned i;
    unsigned k;
    unsigned j;
    for (i = 0; i < 256; i += 8) {
        unsigned long d0 = (unsigned long)raw[i] | ((unsigned long)raw[i + 1] << 16);
        unsigned long d1 = (unsigned long)raw[i + 2] | ((unsigned long)raw[i + 3] << 16);
        unsigned long d2 = (unsigned long)raw[i + 4] | ((unsigned long)raw[i + 5] << 16);
        unsigned long d3 = (unsigned long)raw[i + 6] | ((unsigned long)raw[i + 7] << 16);
        unsigned long h0 = ((d0 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h1 = ((d1 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h2 = ((d2 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h3 = ((d3 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        raw[i]     = (unsigned short)(h0 & D4_M16);
        raw[i + 1] = (unsigned short)((h0 >> 16) & D4_M16);
        raw[i + 2] = (unsigned short)(h1 & D4_M16);
        raw[i + 3] = (unsigned short)((h1 >> 16) & D4_M16);
        raw[i + 4] = (unsigned short)(h2 & D4_M16);
        raw[i + 5] = (unsigned short)((h2 >> 16) & D4_M16);
        raw[i + 6] = (unsigned short)(h3 & D4_M16);
        raw[i + 7] = (unsigned short)((h3 >> 16) & D4_M16);
        total = (total + h0 + h1) & D4_M32;
        total = (total + h2 + h3) & D4_M32;
    }
    /* 1-based Fenwick fill: freq[i] = raw[i-1] for i=1..255 */
    for (i = 1; i < 256; i++) fr[i] = raw[i - 1];
    fr[0] = (unsigned short)(((total & D4_M16) + ((total >> 16) & D4_M16)) & D4_M16);
    for (k = 1; k <= 64; k *= 2) {
        j = 256 - 2 * k;
        for (;;) {
            fr[j] = (unsigned short)(fr[j] + fr[j - k]);
            if (j == 2 * k) break;
            j -= 2 * k;
        }
    }
    if (ci < D4_NREAL) d->escw[ci] = (unsigned short)((d->escw[ci] >> 2) + 1);
}

/* ---- update one symbol (sub_423CC0) ---- */
static void d4_update(dec3v *d, unsigned ci, unsigned sym)
{
    unsigned short *r = d->raw + ci * 256u;
    unsigned short *f = d->freq + ci * 256u;
    unsigned fb = d4_ord0(ci);
    unsigned idx;
    unsigned t;
    if (r[sym] == 0) {
        unsigned short *r0 = d->raw + fb * 256u;
        unsigned short *f0 = d->freq + fb * 256u;
        d->escw[ci] = (unsigned short)(d->escw[ci] + 32);
        r0[sym] = (unsigned short)(r0[sym] + 16);
        idx = (sym + 1) & 0xFFu;
        while (idx) {
            f0[idx] = (unsigned short)(f0[idx] + 16);
            idx = (idx + (idx & (0u - idx))) & 0xFFu;
        }
        f0[0] = (unsigned short)(f0[0] + 16);
        if (f0[0] >= 0x4000u) d4_rescale(d, fb);
    }
    r[sym] = (unsigned short)(r[sym] + 32);
    idx = (sym + 1) & 0xFFu;
    while (idx) {
        f[idx] = (unsigned short)(f[idx] + 32);
        idx = (idx + (idx & (0u - idx))) & 0xFFu;
    }
    f[0] = (unsigned short)(f[0] + 32);
    t = (unsigned)d->escw[ci] + (unsigned)f[0];
    if (t >= 0x4000u) d4_rescale(d, ci);
}

/* ---- symbol decode with escape (sub_423F50) ---- */
static unsigned d4_sym(dec3v *d, unsigned ci)
{
    unsigned short *f;
    unsigned short *r;
    unsigned short *f0;
    unsigned short *r0;
    unsigned long scaled;
    unsigned di, esc, total, rem, p, step, q, sym, i;
    unsigned eb;
    int have_eb;
    unsigned fb;

    f = d->freq + ci * 256u;
    r = d->raw + ci * 256u;
    have_eb = (d->excl >= 0);
    eb = have_eb ? (unsigned)d->dict[d->excl] : 0u;
    di = f[0];
    if (have_eb) di = (di - r[eb]) & D4_M16;
    fb = d4_ord0(ci);
    if (di == 0) {
        /* empty context: decode in the order-0 model, no escape weight */
        ci = fb;
        f = d->freq + ci * 256u;
        r = d->raw + ci * 256u;
        di = f[0];
        if (have_eb) di = (di - r[eb]) & D4_M16;
        if (di == 0) {
            d->err = 7;
            return 0;
        }
        d->range /= di;
        if (d->range == 0) {
            d->err = 7;
            return 0;
        }
        scaled = d->code / d->range;
        goto search;
    }
    esc = d->escw[ci];
    total = (di + esc) & D4_M16;
    if (total == 0) {
        d->err = 7;
        return 0;
    }
    d->range /= total;
    if (d->range == 0) {
        d->err = 7;
        return 0;
    }
    scaled = d->code / d->range;
    if ((scaled & D4_M16) >= di) goto escape;

search:
    rem = (di - scaled) & D4_M16;
    p = 0;
    if (have_eb) {
        unsigned re = r[eb];
        int ebon = 1;
        for (step = 0x80; step; step >>= 1) {
            q = p + step;
            if (ebon && q > eb) {
                if ((unsigned long)f[q] < (unsigned long)re + (unsigned long)rem) {
                    p = q;
                    ebon = 0;
                    rem = (rem - f[q] + re) & D4_M16;
                }
            } else {
                if (rem > (unsigned)f[q]) {
                    p = q;
                    rem = (rem - f[q]) & D4_M16;
                }
            }
        }
    } else {
        for (step = 0x80; step; step >>= 1) {
            q = p + step;
            if (rem > (unsigned)f[q]) {
                p = q;
                rem = (rem - f[q]) & D4_M16;
            }
        }
    }
    if (p > 255) {
        d->err = 7;
        return 0;
    }
    sym = p;
    {
        unsigned hi = (unsigned)((scaled + rem) & D4_M16);
        unsigned lo = (unsigned)((hi - r[sym]) & D4_M16);
        if (!d4_renorm(d, lo, hi)) return 0;
    }
    return sym;

escape:
    /* consume the escape interval [di, di+esc) of the context total */
    {
        unsigned hi = total;
        unsigned lo = di;
        if (!d4_renorm(d, lo, hi)) return 0;
    }
    f0 = d->freq + fb * 256u;
    r0 = d->raw + fb * 256u;
    /* total0 = order-0 total minus every symbol already in this context */
    {
        unsigned xsum = 0;
        for (i = 0; i < 256; i++) {
            if (r[i] != 0) xsum = (xsum + r0[i]) & D4_M16;
        }
        if (have_eb && r[eb] == 0) xsum = (xsum + r0[eb]) & D4_M16;
        total = (f0[0] - xsum) & D4_M16;
    }
    if (total == 0) {
        d->err = 7;
        return 0;
    }
    d->range /= total;
    if (d->range == 0) {
        d->err = 7;
        return 0;
    }
    scaled = d->code / d->range;
    {
        unsigned scaled16 = (unsigned)(scaled & D4_M16);
        unsigned acc = 0;
        unsigned v40 = 256;
        if (have_eb) {
            unsigned stop = eb + 1u;
            while (v40 != stop) {
                --v40;
                if (r[v40] == 0) {
                    acc = (acc + r0[v40]) & D4_M16;
                    if (acc > scaled16) break;
                }
            }
            if (acc <= scaled16) {
                if (v40 == 0) {
                    d->err = 7;
                    return 0;
                }
                --v40;
            }
        }
        if (acc <= scaled16) {
            for (;;) {
                if (v40 == 0) {
                    d->err = 7;
                    return 0;
                }
                --v40;
                if (r[v40] == 0) break;
            }
            for (;;) {
                acc = (acc + r0[v40]) & D4_M16;
                if (acc > scaled16) break;
                for (;;) {
                    if (v40 == 0) {
                        d->err = 7;
                        return 0;
                    }
                    --v40;
                    if (r[v40] == 0) break;
                }
            }
        }
        if (v40 > 255) {
            d->err = 7;
            return 0;
        }
        sym = v40;
        {
            unsigned hi = acc;
            unsigned lo = (acc - r0[sym]) & D4_M16;
            if (!d4_renorm(d, lo, hi)) return 0;
        }
    }
    return sym;
}

/* ---- litm bit update (sub_423E00) ---- */
static void d4_bitupd(dec3v *d, unsigned ai, unsigned bit)
{
    unsigned short *e;
    if ((ai & 0xFFu) >= D4_NLITM) {
        d->err = 7;
        return;
    }
    e = d->litm[ai & 0xFFu];
    if (!bit) e[1] = (unsigned short)(e[1] + 64);
    e[2] = (unsigned short)(e[2] + 64);
    if (e[2] >= 0x4000u) {
        unsigned c0 = e[0];
        unsigned c1 = e[1];
        unsigned c2 = e[2];
        unsigned n1 = c0 + (((c1 - c0) + 1) >> 1);
        unsigned n2 = n1 + (((c2 - c1) + 1) >> 1);
        e[1] = (unsigned short)(n1 & D4_M16);
        e[2] = (unsigned short)(n2 & D4_M16);
    }
    d->state = d4_st0[2u * (unsigned)d->state + (bit & 1u)];
}

/* ---- decode the match/literal bit (sub_4248C0) ---- */
static unsigned d4_bit(dec3v *d, unsigned ai)
{
    unsigned short *e;
    unsigned total, scaled16, bit, lo, hi;
    if ((ai & 0xFFu) >= D4_NLITM) {
        d->err = 7;
        return 0;
    }
    e = d->litm[ai & 0xFFu];
    total = e[2];
    if (total == 0) {
        d->err = 7;
        return 0;
    }
    d->range /= total;
    if (d->range == 0) {
        d->err = 7;
        return 0;
    }
    scaled16 = (unsigned)((d->code / d->range) & D4_M16);
    if (scaled16 >= (unsigned)e[1]) {
        bit = 1;
        lo = e[1];
        hi = e[2];
    } else {
        bit = 0;
        lo = e[0];
        hi = e[1];
    }
    if (!d4_renorm(d, lo, hi)) return 0;
    d4_bitupd(d, ai, bit);
    return bit;
}

/* ---- literal (sub_424400) ---- */
static void d4_literal(dec3v *d)
{
    unsigned long c16 = d->ctx & D4_M16;
    unsigned lo8 = (unsigned)(c16 & 0xFFUL);
    unsigned hi8 = (unsigned)((c16 >> 8) & 0xFFUL);
    unsigned midx;
    unsigned b;
    if (c16 == 0xFFFFUL) midx = 314u;
    else if (c16 == 0xD1DFUL) midx = 315u;
    else if (c16 == 0xD3DFUL) midx = 316u;
    else if (lo8 == 0xDFu) midx = d4_ord(hi8) ? 317u : 223u;
    else if (!d4_ord(lo8) && d4_ord(hi8)) midx = lo8 + 123u;
    else midx = lo8;
    b = d4_sym(d, midx);
    if (d->err) return;
#if U4_DEBUG
    fprintf(stderr, "T%lu LIT idx=%u b=%02X ctx=%08lX mpos=%ld\n",
                d->out_total, midx, b, d->ctx, d->match_pos);
#endif
    d4_update(d, midx, b);
    d->excl = -1;
    if (d->match_pos >= 0 &&
        (unsigned)d->dict[d->match_pos] == b) {
        d->excl = (long)(((unsigned long)d->match_pos + 1UL) & d->dict_mask);
    }
    d4_emit(d, b ^ 0xFFu);
    d->ctx = ((d->ctx << 8) + (unsigned long)b) & D4_M32;
    d->dict[d->dict_pos] = (unsigned char)b;
    if (d->dict_pos < 0x100UL)
        d->dict[d->dict_sz + d->dict_pos] = (unsigned char)b;
    d->dict_pos = (d->dict_pos + 1UL) & d->dict_mask;
}

/* ---- match/literal decision and match copy (sub_424530) ----
 * returns 1 when match bytes were copied, 0 when a literal is needed.
 */
static int d4_match(dec3v *d)
{
    unsigned long h;
    unsigned long *he;
    unsigned long mpos;
    unsigned long pos;
    unsigned long dist;
    unsigned al;
    unsigned idx;
    unsigned bit;
    unsigned ln;
    unsigned n;
    unsigned lctx;
    unsigned long mcx, mdp, mmp;

    if (d->err) return 0;
    pos = d->dict_pos;
    h = (d->ctx ^ (d->ctx >> 13)) & 0x3FFFFUL;
    he = d->htab + h * 2UL;
    mpos = he[0] & D4_M32;
    d->match_pos = -1;
    al = 0;
    if (d->run_len) {
        mpos = (pos - d->last_dist) & d->dict_mask;
        al = 48u + ((d->run_len > 2) ? 1u : 0u);
    }
    if (!al) {
        if (mpos == D4_M32 || d->ctx != he[1]) {
            unsigned long mp = (pos - d->last_dist) & d->dict_mask;
            mpos = mp;
            dist = (mp - pos) & d->dict_mask;
            if (dist >= 0x104UL) {
                unsigned long mp4 = (mp - 4UL) & d->dict_mask;
                unsigned long a0 = ((unsigned long)d->dict[mp4] << 24)
                    | ((unsigned long)d->dict[mp4 + 1] << 16)
                    | ((unsigned long)d->dict[mp4 + 2] << 8)
                    | (unsigned long)d->dict[mp4 + 3];
                unsigned long c = d->ctx;
                if ((a0 & 0xFFFFUL) == (c & 0xFFFFUL)
                    || (a0 & 0xFFFF00FFUL) == (c & 0xFFFF00FFUL)
                    || (a0 & 0xFFFFFF00UL) == (c & 0xFFFFFF00UL))
                    al = 9;
            }
            if (!al) {
                unsigned long o = d->ord[d->ctx & 0xFFFFUL] & D4_M32;
                if (o != D4_M32) {
                    mpos = o;
                    al = 0x11u;
                }
            }
        } else {
            al = 1;
        }
    }
    he[0] = pos;
    d->ord[d->ctx & 0xFFFFUL] = pos;
    he[1] = d->ctx;
    if (!al) {
        d->run_len = 0;
        return 0;
    }
    if (((mpos - pos) & d->dict_mask) < 0x100UL) {
        d->run_len = 0;
        return 0;
    }
    if (!d->run_len) {
        if (d->hval[h]) al += (unsigned)d->state - 1u;
        else al += (unsigned)d->state + 23u;
    }
    idx = al & 0xFFu;
#if U4_DEBUG
    fprintf(stderr, "T%lu DEC h=%05lX mpos=%lu idx=%u ctx=%08lX run=%lu hval=%u\n",
                d->out_total, h, mpos & d->dict_mask, idx, d->ctx,
                d->run_len, (unsigned)d->hval[h]);
#endif
    if (d->excl >= 0 && d->dict[d->excl] == d->dict[mpos & d->dict_mask]) {
        /* exclusion hit: only the litm entry is updated, stay literal */
        d4_bitupd(d, idx, 1);
        d->run_len = 0;
        return 0;
    }
    d->match_pos = (long)(mpos & d->dict_mask);
    bit = d4_bit(d, idx);
    if (d->err) return 0;
#if U4_DEBUG
    fprintf(stderr, "  bit=%u\n", bit);
#endif
    if (bit) {
        d->hval[h] = 0;
        d->run_len = 0;
        return 0;
    }
    /* length is an ordinary symbol in a context selected by the previous
     * length stored at this hash index (or the run contexts 326/327) */
    if (d->run_len)
        lctx = 326u + ((d->run_len > 2) ? 1u : 0u);
    else
        lctx = d4_len_context((unsigned)d->hval[h]);
    d->excl = -1;
    ln = d4_sym(d, lctx);
    if (d->err) return 0;
#if U4_DEBUG
    fprintf(stderr, "  len=%u lctx=%u\n", ln, lctx);
#endif
    d4_update(d, lctx, ln);
    d->hval[h] = (unsigned char)ln;
    d->last_dist = (d->dict_pos - (mpos & d->dict_mask)) & d->dict_mask;
    if (ln != 0xFFu) {
        d->excl = (long)((mpos + ln) & d->dict_mask);
        d->run_len = 0;
    } else {
        d->excl = -1;
        d->run_len++;
    }
    n = (ln == 0) ? 256u : ln;
    mcx = d->ctx;
    mdp = d->dict_pos;
    mmp = mpos & d->dict_mask;
    while (n--) {
        unsigned b = (unsigned)d->dict[mmp];
        d4_emit(d, b ^ 0xFFu);
        mcx = ((mcx << 8) + (unsigned long)b) & D4_M32;
        d->dict[mdp] = (unsigned char)b;
        if (mdp < 0x100UL) d->dict[d->dict_sz + mdp] = (unsigned char)b;
        mdp = (mdp + 1UL) & d->dict_mask;
        mmp = (mmp + 1UL) & d->dict_mask;
    }
    d->ctx = mcx;
    d->dict_pos = mdp;
    return 1;
}

/* ---- public API ---- */

int uharc_dec3v_init(void **pctx, int method)
{
    dec3v *d;
    unsigned long dictsz;
    if (!pctx) return 1;
    *pctx = 0;
    if (method < 8 || method > 23) return 1;
    dictsz = 1UL << (((method - 8) % 16) + 10);
    d = (dec3v *)malloc(sizeof(*d));
    if (!d) return 2;
    memset(d, 0, sizeof(*d));
    d->htab = (unsigned long *)malloc((size_t)D4_NHASH * 2u * sizeof(unsigned long));
    d->hval = (unsigned char *)malloc(0x40000UL);
    d->ord = (unsigned long *)malloc((size_t)D4_NORD * sizeof(unsigned long));
    d->dict = (unsigned char *)calloc((size_t)(dictsz + 256UL), 1);
    d->freq = (unsigned short *)malloc((size_t)D4_NCTX * 256u * sizeof(unsigned short));
    d->raw = (unsigned short *)malloc((size_t)D4_NCTX * 256u * sizeof(unsigned short));
    d->escw = (unsigned short *)malloc((size_t)D4_NREAL * sizeof(unsigned short));
    if (!d->htab || !d->hval || !d->ord || !d->dict ||
        !d->freq || !d->raw || !d->escw) {
        uharc_dec3v_free(d);
        return 2;
    }
    memset(d->htab, 0xFF, (size_t)D4_NHASH * 2u * sizeof(unsigned long));
    memset(d->hval, 0, 0x40000UL);
    memset(d->ord, 0xFF, (size_t)D4_NORD * sizeof(unsigned long));
    d->dict_sz = dictsz;
    d->dict_mask = dictsz - 1UL;
    d->dict_pos = 0;
    d->ctx = 0;
    d->excl = -1;
    d->match_pos = -1;
    d->run_len = 0;
    d->last_dist = 0;
    d->state = 0;
    d4_reset(d);
    *pctx = d;
    return 0;
}

void uharc_dec3v_free(void *ctx)
{
    dec3v *d = (dec3v *)ctx;
    if (!d) return;
    free(d->htab);
    free(d->hval);
    free(d->ord);
    free(d->dict);
    free(d->freq);
    free(d->raw);
    free(d->escw);
    free(d);
}

int uharc_dec3v_decode_b(void *ctx,
                         const unsigned long *cum, unsigned long ncum,
                         uharc_dec_read_cb rcb, void *rctx,
                         uharc_dec_write_cb wcb, void *wctx,
                         unsigned long out_len)
{
    dec3v *d = (dec3v *)ctx;
    int err;
    if (!d || !rcb || !wcb) return 1;
    if (out_len == 0) return 0;
    d->inb = (unsigned char *)malloc(65536UL);
    d->outb = (unsigned char *)malloc(65536UL);
    if (!d->inb || !d->outb) {
        free(d->inb);
        free(d->outb);
        d->inb = 0;
        d->outb = 0;
        return 2;
    }
    d->rcb = rcb;
    d->rctx = rctx;
    d->wcb = wcb;
    d->wctx = wctx;
    d->in_cap = 65536;
    d->out_cap = 65536;
    d->in_pos = 0;
    d->in_avail = 0;
    d->out_fill = 0;
    d->out_total = 0;
    d->out_len = out_len;
    d->err = 0;
    d4_reset(d);
    d->prog_idx = 0;
    if (cum && ncum && cum[ncum - 1] == out_len) {
        d->cum = cum;
        d->ncum = ncum;
    } else {
        d->cum = 0;
        d->ncum = 0;
    }
    d4_rc_init(d);
    while (d->out_total < d->out_len && !d->err) {
        if (!d4_match(d)) {
            if (d->err) break;
            d4_literal(d);
        }
        if (!d->err) d4_progress(d);
    }
    err = d->err;
    if (!err && d->out_fill) {
        if (wcb(wctx, d->outb, d->out_fill) != 0) err = 4;
    }
    free(d->inb);
    free(d->outb);
    d->inb = 0;
    d->outb = 0;
    return err;
}

int uharc_dec3v_decode(void *ctx,
                       uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len)
{
    return uharc_dec3v_decode_b(ctx, 0, 0UL, rcb, rctx, wcb, wctx, out_len);
}
