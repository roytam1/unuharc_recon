/* UHARC 0.3 decompressor core 3 (ALZ, methods 8..21).
 *
 * Clean portable C89 port of the UHARC 0.3 ("uharc_u.exe") core-3 decoder.
 * Reverse engineered from uharc_u.lst assembly (Hex-Rays output of the 0.3
 * init is corrupt, so every routine below was transliterated from the
 * listing).  No Windows API, no globals; all state lives in dec3u and is
 * malloc'ed/free'ed.  No output unless U3_DEBUG is defined to 1.
 *
 * Original -> port mapping (uharc_u.lst):
 *  sub_41F840 (init)          -> uharc_dec3u_init
 *      method 8..21; dict = 1 << (((method-8) % 14) + 10);
 *      allocations: 0x200000 htab (pos,ctx per 18-bit key, pos init
 *      0xFFFFFFFF), 0x40000 hval (init 0), 0x40000 ord (65536 u32, init
 *      0xFFFFFFFF), dict+0x100 (zeroed), 0x29000 freq (328 x 256 u16),
 *      0x29000 raw, 0x28C escw (326 u16 used; 328 allocated because the
 *      update writes escw[326]/[327] too).
 *  sub_41FAD0 (model reset)   -> u3_reset  (freq/raw[0..325] zeroed,
 *      freq/raw[326..327] from the 0x42C9B0 lowbit table *8 / 8,
 *      escw[0..325]=1, litm[48] from the 0x42CBE8 template)
 *  sub_41FC30 (rescale ctx)   -> u3_rescale
 *  sub_41FD78 (model update)  -> u3_update
 *  sub_41FEA0 (litm update)   -> inside u3_bit
 *  sub_4206D8 (match-bit)     -> u3_bit       (2-symbol litm entry)
 *  sub_41FFD4 (symbol decode) -> u3_sym       (ctx symbol + escape)
 *  sub_420310 (literal)       -> u3_literal
 *  sub_420418 (match/decision)-> u3_match
 *  sub_41FF24 (main loop)     -> uharc_dec3u_decode
 *  sub_42335C (coder init)    -> u3_rc_init
 *  sub_423188 (decode point)  -> u3_point
 *  sub_423140 (narrow)        -> u3_narrow
 *  sub_41E2B0 (input byte)    -> u3_get (via read callback)
 *  sub_41E3BC (output byte)   -> u3_emit (via write callback)
 *
 * 0.3-specific details that differ from the verified 0.6 core-3 port
 * (uharc_dec3.c):
 *  - A different arithmetic coder: 24-bit range/offset with an 8-bit
 *    renormalisation step and a one-byte bit reservoir (see u3_renorm).
 *    The stream begins with a discarded byte whose value is ~out_len.
 *  - 326 symbol contexts plus two order-0 contexts (326 for ordinary
 *    contexts, 327 for the length contexts and second-level escapes),
 *    instead of 318 + 1.
 *  - The escape from a symbol context excludes *all* symbols already
 *    present in that context from the order-0 model (multi-exclusion)
 *    and decodes that model from symbol 255 downwards; the 0.6 core
 *    excludes only the single ambiguity symbol.
 *  - The match/literal decision uses a 48-entry table (6 semantic
 *    classes x 8 three-bit history states) of 6-byte {c0,c1,c2}
 *    entries, and the match length is decoded with normal symbol
 *    contexts 318..325 selected by the previous length (0x42CD08).
 *  - No run-length/state/mval variables of the 0.6 core.
 *
 * Return codes: 0 ok, 1 bad args, 2 nomem, 3 input short, 4 output short,
 * 7 data error (corrupt stream).
 *
 * Oracle note: the 0.3 name table (tab1) is a parallel array to tab0 (file
 * CRCs) and tab3 (size/method), so its order is the archive entry order.
 * For UHACTEST03/03_m0.uha the decrypted tab0 CRC list is
 *   00000000 7125DAA3 51A02E01 B70B4C26 021E1879 6F3BDB8B
 * i.e. empty.bin, text.txt, repeat.txt, binary.bin, random.bin, call.bin,
 * and the decoder below reproduces exactly
 *   00 09 "empty.bin" 08 "text.txt" 0A "repeat.txt" 0A "binary.bin"
 *   0A "random.bin" 08 "call.bin"
 * (62 bytes).  The alphabetical order sometimes quoted for this archive is
 * the console "l -d2" display order, not the table order.
 */
#include <stdlib.h>
#include <string.h>

#include "uharc_dec.h"

/* public API (see uharc_dec.h for the callback typedefs) */
int uharc_dec3u_init(void **pctx, int method);
int uharc_dec3u_decode(void *ctx, uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len);
void uharc_dec3u_free(void *ctx);

#ifndef U3_DEBUG
#define U3_DEBUG 0
#endif

#if U3_DEBUG
#include <stdio.h>
#endif

#define U3_M32 0xFFFFFFFFUL
#define U3_M16 0xFFFFU

#define U3_NCTX 328u   /* allocated contexts (0..325 real + 326,327 order0) */
#define U3_NFB  326u   /* first order-0 context */
#define U3_NLITM 48u   /* litm entries in use (hist 0..7 + class 0..40) */

typedef struct dec3u_s {
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

    /* range coder (4450F0..4450FC) */
    unsigned long unit;    /* 4450F0 */
    unsigned long code;    /* 4450F4 */
    unsigned long rng;     /* 4450F8 */
    unsigned cur;          /* 4450FC: one-byte bit reservoir */

    /* model */
    unsigned char *htab;   /* 0x200000: 262144 x {u32 pos, u32 ctx} */
    unsigned char *hval;   /* 0x40000 */
    unsigned long *ord;    /* 0x40000: 65536 u32 */
    unsigned char *dict;   /* dict_sz + 256 */
    unsigned short *freq;  /* 328 x 256 */
    unsigned short *raw;   /* 328 x 256 */
    unsigned short *escw;  /* 328 (326 meaningful) */
    unsigned short litm[U3_NLITM][3];
    unsigned char hist;    /* byte_4450AC: litm history, init 7 */

    /* decoder state */
    unsigned long dict_sz; /* 4450A8 */
    unsigned long dict_mask; /* 4450A4 */
    unsigned long dict_pos;  /* 445098 */
    unsigned long ctx;       /* 44509C */
    long excl;               /* 4450A0: dict index or -1 */
    long match_pos;          /* 445094: -1 or dict index */
    unsigned long last_dist; /* 445070 */
} dec3u;

/* litm entry[1] seeds: 0x42CBE8 template, period 8, entry[2] = 0x2000 */
static const unsigned short u3_litw1[8] = {
    0x0800, 0x0400, 0x0500, 0x0400, 0x0600, 0x0400, 0x0500, 0x0400
};

/* order-0 context class (byte_42D108): 0 in two ranges, else 1 */
static int u3_ord(unsigned c)
{
    if ((c >= 133 && c <= 158) || (c >= 165 && c <= 190)) return 0;
    return 1;
}

/* length symbol context (dword_42CD08): 318,318,319..322,323,323,324 x4,325 */
static unsigned u3_lenctx(unsigned v)
{
    if (v < 2) return 0x13Eu;
    if (v < 6) return 0x13Fu + (v - 2);
    if (v < 8) return 0x143u;
    if (v < 12) return 0x144u;
    return 0x145u;
}

/* ---- I/O pump (sub_41E2B0 / sub_41E3BC) ---- */
static int u3_get(dec3u *d)
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

static void u3_emit(dec3u *d, unsigned b)
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

/* ---- range coder (sub_42335C / sub_423188 / sub_423140) ---- */
static void u3_rc_init(dec3u *d)
{
    int b;
    u3_get(d);                 /* discarded: ~out_len marker byte */
    b = u3_get(d);
    d->cur = (unsigned)b;
    d->rng = 0x80UL;
    d->code = (unsigned long)((unsigned)b >> 1);
}

/* renormalise: pull one bit reservoir byte at a time (4231A3..4231EE) */
static void u3_renorm(dec3u *d)
{
    while (d->rng <= 0x800000UL) {
        unsigned b = (unsigned)u3_get(d);
        d->code = ((d->code << 8) |
                   (unsigned long)((d->cur & 1u) << 7) |
                   (unsigned long)(b >> 1)) & U3_M32;
        d->rng = (d->rng << 8) & U3_M32;
        d->cur = b;
        if (d->err) return;
    }
}

/* decode a point in [0,total) (sub_423188) */
static unsigned u3_point(dec3u *d, unsigned total)
{
    unsigned long unit;
    unsigned long p;
    if (d->err) return 0;
    if (d->rng <= 0x800000UL) u3_renorm(d);
    if (d->err) return 0;
    total &= U3_M16;
    if (total == 0) { d->err = 7; return 0; }
    unit = d->rng / total;
    if (unit == 0) { d->err = 7; return 0; }
    d->unit = unit;
    p = d->code / unit;
    if (p >= (unsigned long)total) p = (unsigned long)total - 1;
    return (unsigned)p;
}

/* narrow the interval to [lo,hi) of total (sub_423140) */
static void u3_narrow(dec3u *d, unsigned total, unsigned lo, unsigned hi)
{
    unsigned long u = d->unit;
    unsigned long lp = (u * (unsigned long)lo) & U3_M32;
    d->code = (d->code - lp) & U3_M32;
    if ((unsigned long)hi < (unsigned long)total)
        d->rng = (u * (unsigned long)(hi - lo)) & U3_M32;
    else
        d->rng = (d->rng - lp) & U3_M32;
}

/* ---- model helpers ---- */

/* sub_41FC30: halve one context's counts and rebuild its Fenwick tree */
static void u3_rescale(dec3u *d, unsigned ctx)
{
    unsigned short *raw = d->raw + ctx * 256u;
    unsigned short *fr = d->freq + ctx * 256u;
    unsigned long total = 0;
    unsigned i, k, j;
    for (i = 0; i < 256; i += 8) {
        unsigned long d0 = (unsigned long)raw[i] | ((unsigned long)raw[i + 1] << 16);
        unsigned long d1 = (unsigned long)raw[i + 2] | ((unsigned long)raw[i + 3] << 16);
        unsigned long d2 = (unsigned long)raw[i + 4] | ((unsigned long)raw[i + 5] << 16);
        unsigned long d3 = (unsigned long)raw[i + 6] | ((unsigned long)raw[i + 7] << 16);
        unsigned long h0 = ((d0 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h1 = ((d1 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h2 = ((d2 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h3 = ((d3 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        raw[i]     = (unsigned short)(h0 & U3_M16);
        raw[i + 1] = (unsigned short)((h0 >> 16) & U3_M16);
        raw[i + 2] = (unsigned short)(h1 & U3_M16);
        raw[i + 3] = (unsigned short)((h1 >> 16) & U3_M16);
        raw[i + 4] = (unsigned short)(h2 & U3_M16);
        raw[i + 5] = (unsigned short)((h2 >> 16) & U3_M16);
        raw[i + 6] = (unsigned short)(h3 & U3_M16);
        raw[i + 7] = (unsigned short)((h3 >> 16) & U3_M16);
        total = (total + h0 + h1) & U3_M32;
        total = (total + h2 + h3) & U3_M32;
    }
    for (i = 1; i < 256; i++) fr[i] = raw[i];
    fr[0] = (unsigned short)(((total & U3_M16) + ((total >> 16) & U3_M16)) & U3_M16);
    for (k = 1; k <= 64; k *= 2) {
        j = 256 - 2 * k;
        for (;;) {
            fr[j] = (unsigned short)(fr[j] + fr[j - k]);
            if (j == 2 * k) break;
            j -= 2 * k;
        }
    }
    if (ctx < 0x146u) {
        d->escw[ctx] = (unsigned short)((d->escw[ctx] >> 2) + 1);
    }
}

/* sub_41FD78: update the context model and the order-0 model */
static void u3_update(dec3u *d, unsigned ctx, unsigned sym)
{
    unsigned short *f = d->freq;
    unsigned short *r = d->raw;
    unsigned base = ctx * 256u;
    unsigned fb, i, t;
    if (r[base + sym] == 0) {
        fb = U3_NFB + ((ctx >= 0x13Eu) ? 1u : 0u);
        d->escw[ctx] = (unsigned short)(d->escw[ctx] + 32);
        r[fb * 256u + sym] = (unsigned short)(r[fb * 256u + sym] + 16);
        i = (sym + 1) & 0xFFu;
        while (i) {
            f[fb * 256u + i] = (unsigned short)(f[fb * 256u + i] + 16);
            i = (i + (i & (0u - i))) & 0xFFu;
        }
        f[fb * 256u] = (unsigned short)(f[fb * 256u] + 16);
        if (f[fb * 256u] >= 0x4000u) u3_rescale(d, fb);
    }
    r[base + sym] = (unsigned short)(r[base + sym] + 32);
    i = (sym + 1) & 0xFFu;
    while (i) {
        f[base + i] = (unsigned short)(f[base + i] + 32);
        i = (i + (i & (0u - i))) & 0xFFu;
    }
    f[base] = (unsigned short)(f[base] + 32);
    t = (unsigned)d->escw[ctx] + (unsigned)f[base];
    if (t >= 0x4000u) u3_rescale(d, ctx);
}

/* sub_41FAD0: reset the whole model */
static void u3_reset(dec3u *d)
{
    unsigned i, c;
    for (i = 0; i < U3_NFB * 256u; i++) {
        d->freq[i] = 0;
        d->raw[i] = 0;
    }
    for (c = U3_NFB; c < U3_NCTX; c++) {
        for (i = 0; i < 256; i++) {
            unsigned v = (i == 0) ? 256u : (i & (0u - i));
            d->freq[c * 256u + i] = (unsigned short)(v * 8u);
            d->raw[c * 256u + i] = 8;
        }
    }
    for (i = 0; i < U3_NFB; i++) d->escw[i] = 1;
    for (c = 0; c < U3_NLITM; c++) {
        d->litm[c][0] = 0;
        d->litm[c][1] = u3_litw1[c & 7u];
        d->litm[c][2] = 0x2000;
    }
}

/* ---- Fenwick search (part of sub_41FFD4) ----
 * di    : the symbol totals (f[ctx][0] minus the excluded count)
 * point : decoded point in [0,di)
 * ntotal: total handed to sub_423140 (di + escape weight)
 */
static unsigned u3_search(dec3u *d, unsigned ctx, unsigned di,
                          unsigned point, unsigned ntotal)
{
    unsigned short *f = d->freq + ctx * 256u;
    unsigned short *r = d->raw + ctx * 256u;
    unsigned rem, p, step, q, hi, lo, sym;
    rem = (di - point) & U3_M16;
    p = 0;
    if (d->excl >= 0) {
        unsigned e = d->dict[d->excl];
        unsigned re = r[e];
        int ebon = 1;
        for (step = 0x80; step; step >>= 1) {
            q = p + step;
            if (ebon && q > e) {
                if ((unsigned long)f[q] < (unsigned long)re + (unsigned long)rem) {
                    p = q;
                    ebon = 0;
                    rem = (rem - f[q] + re) & U3_M16;
                }
            } else {
                if (rem > (unsigned)f[q]) {
                    p = q;
                    rem = (rem - f[q]) & U3_M16;
                }
            }
        }
    } else {
        for (step = 0x80; step; step >>= 1) {
            q = p + step;
            if (rem > (unsigned)f[q]) {
                p = q;
                rem = (rem - f[q]) & U3_M16;
            }
        }
    }
    if (p > 255) { d->err = 7; return 0; }
    sym = p;
    hi = (point + rem) & U3_M16;
    lo = (hi - r[sym]) & U3_M16;
    u3_narrow(d, ntotal, lo, hi);
    return sym;
}

/* sub_4206D8 (decode) + sub_41FEA0 (update) of one litm bit.
 * have == 0: decode a bit; have != 0: only apply the update for bitin. */
static unsigned u3_bit(dec3u *d, unsigned al, int have, unsigned bitin)
{
    unsigned idx = ((unsigned)d->hist + al) & 0xFFu;
    unsigned short *e;
    unsigned bit, total, point, lo, hi;
    if (idx >= U3_NLITM) idx &= (U3_NLITM - 1u);
    e = d->litm[idx];
    if (!have) {
        total = e[2];
        if (total == 0) { d->err = 7; return 0; }
        point = u3_point(d, total);
        if (d->err) return 0;
        if (point < (unsigned)e[1]) {
            bit = 0; lo = e[0]; hi = e[1];
        } else {
            bit = 1; lo = e[1]; hi = e[2];
        }
        u3_narrow(d, total, lo, hi);
        if (d->err) return 0;
    } else {
        bit = bitin & 1u;
    }
    if (bit == 0) e[1] = (unsigned short)(e[1] + 64);
    e[2] = (unsigned short)(e[2] + 64);
    if (e[2] >= 0x4000u) {
        unsigned c0 = e[0], c1 = e[1], c2 = e[2];
        unsigned n1 = c0 + (((c1 - c0) + 1) >> 1);
        unsigned n2 = n1 + (((c2 - c1) + 1) >> 1);
        e[1] = (unsigned short)(n1 & U3_M16);
        e[2] = (unsigned short)(n2 & U3_M16);
    }
    d->hist = (unsigned char)(((unsigned)d->hist * 2u + bit) & 7u);
    return bit;
}

/* sub_41FFD4: decode one symbol in context ctx (with escape) */
static unsigned u3_sym(dec3u *d, unsigned ctx)
{
    unsigned di, esc, total, point, fb, e, total0, point2, bx, sym, i;
    if (d->err) return 0;
    di = d->freq[ctx * 256u];
    if (d->excl >= 0) {
        e = d->dict[d->excl];
        di = (di - d->raw[ctx * 256u + e]) & U3_M16;
    }
    if (di == 0) {
        /* empty context: decode in the order-0 model (no escape weight) */
        fb = U3_NFB + ((ctx >= 0x13Eu) ? 1u : 0u);
        di = d->freq[fb * 256u];
        if (d->excl >= 0) {
            e = d->dict[d->excl];
            di = (di - d->raw[fb * 256u + e]) & U3_M16;
        }
        if (di == 0) { d->err = 7; return 0; }
        point = u3_point(d, di);
        if (d->err) return 0;
        return u3_search(d, fb, di, point, di);
    }
    esc = d->escw[ctx];
    total = (di + esc) & U3_M16;
    point = u3_point(d, total);
    if (d->err) return 0;
    if (point < di) return u3_search(d, ctx, di, point, total);
    /* escape symbol: consume [di,total), then order-0 with multi-exclusion */
    u3_narrow(d, total, di, total);
    if (d->err) return 0;
    fb = U3_NFB + ((ctx >= 0x13Eu) ? 1u : 0u);
    total0 = d->freq[fb * 256u];
    e = (d->excl >= 0) ? (unsigned)d->dict[d->excl] : 256u;
    for (i = 0; i < 256; i++) {
        if (d->raw[ctx * 256u + i] != 0 || i == e)
            total0 = (total0 - d->raw[fb * 256u + i]) & U3_M16;
    }
    if (total0 == 0) { d->err = 7; return 0; }
    point2 = u3_point(d, total0);
    if (d->err) return 0;
    bx = 0;
    sym = 255;
    for (;;) {
        if (!(d->raw[ctx * 256u + sym] != 0 || sym == e))
            bx = (bx + d->raw[fb * 256u + sym]) & U3_M16;
        if (bx > point2) break;
        if (sym == 0) { d->err = 7; return 0; }
        sym--;
    }
    u3_narrow(d, total0, (bx - d->raw[fb * 256u + sym]) & U3_M16, bx);
    if (d->err) return 0;
    return sym;
}

/* sub_420310: decode and emit one literal */
static void u3_literal(dec3u *d)
{
    unsigned long c16 = d->ctx & U3_M16;
    unsigned lo8 = (unsigned)(c16 & 0xFFUL);
    unsigned hi8 = (unsigned)((c16 >> 8) & 0xFFUL);
    unsigned idx, b;
    if (c16 == 0xFFFFUL) idx = 314;
    else if (c16 == 0xD1DFUL) idx = 315;
    else if (c16 == 0xD3DFUL) idx = 316;
    else if (lo8 == 0xDFu) idx = u3_ord(hi8) ? 317u : 223u;
    else if (!u3_ord(lo8) && u3_ord(hi8)) idx = lo8 + 123u;
    else idx = lo8;
    b = u3_sym(d, idx);
    if (d->err) return;
    u3_update(d, idx, b);
    d->excl = -1;
    if (d->match_pos >= 0 &&
        (unsigned)d->dict[d->match_pos] == b) {
        d->excl = (long)(((unsigned long)d->match_pos + 1UL) & d->dict_mask);
    }
    u3_emit(d, b ^ 0xFFu);
    d->ctx = ((d->ctx << 8) + (unsigned long)b) & U3_M32;
    d->dict[d->dict_pos] = (unsigned char)b;
    if (d->dict_pos < 0x100UL)
        d->dict[d->dict_sz + d->dict_pos] = (unsigned char)b;
    d->dict_pos = (d->dict_pos + 1UL) & d->dict_mask;
}

/* sub_420418: match/literal decision and match copy. 1 = copied match */
static int u3_match(dec3u *d)
{
    unsigned long h, mpos, mpos0, dist, pos;
    unsigned long *he;
    unsigned al, idx, bit, ln, n;
    unsigned long mcx, mdp, mmp;
    unsigned b;
    if (d->err) return 0;
    pos = d->dict_pos;
    h = (d->ctx ^ (d->ctx >> 13)) & 0x3FFFFUL;
    he = (unsigned long *)(void *)(d->htab + h * 8UL);
    mpos = he[0];
    d->match_pos = -1;
    if (mpos != U3_M32 && d->ctx == he[1]) {
        al = 1;
    } else {
        al = 0;
        mpos = (pos - d->last_dist) & d->dict_mask;
        dist = (mpos - pos) & d->dict_mask;
        if (dist >= 0x104UL) {
            unsigned long mp4 = (mpos - 4UL) & d->dict_mask;
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
        if (al == 0) {
            unsigned long o = d->ord[d->ctx & 0xFFFFUL];
            if (o != U3_M32) {
                mpos = o;
                al = 0x11u;
            }
        }
    }
    he[0] = pos;
    he[1] = d->ctx;
    d->ord[d->ctx & 0xFFFFUL] = pos;
    if (al == 0) return 0;
    if (((mpos - pos) & d->dict_mask) < 0x100UL) return 0;
    al = al - 1u;
    mpos0 = mpos & d->dict_mask;
    if (d->hval[h] == 0) al += 0x18u;
    if (d->excl >= 0 && d->dict[d->excl] == d->dict[mpos0]) {
        u3_bit(d, al, 1, 1);
        return 0;
    }
    d->match_pos = (long)mpos0;
    bit = u3_bit(d, al, 0, 0);
    if (d->err) return 0;
    if (bit != 0) {
        d->hval[h] = 0;
        return 0;
    }
    idx = u3_lenctx(d->hval[h]);
    ln = u3_sym(d, idx);
    if (d->err) return 0;
    u3_update(d, idx, ln);
    d->hval[h] = (unsigned char)ln;
    d->last_dist = (d->dict_pos - mpos0) & d->dict_mask;
    d->excl = -1;
    if (ln < 0xFFu) d->excl = (long)((mpos0 + ln) & d->dict_mask);
    n = (ln == 0) ? 256u : ln;
    mcx = d->ctx;
    mdp = d->dict_pos;
    mmp = mpos0;
    while (n--) {
        b = (unsigned)d->dict[mmp];
        u3_emit(d, b ^ 0xFFu);
        mcx = ((mcx << 8) + (unsigned long)b) & U3_M32;
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

int uharc_dec3u_init(void **pctx, int method)
{
    dec3u *d;
    unsigned long dictsz;
    unsigned i;
    if (!pctx) return 1;
    *pctx = 0;
    if (method < 8 || method > 21) return 1;
    dictsz = 1UL << (((method - 8) % 14) + 10);
    d = (dec3u *)malloc(sizeof(*d));
    if (!d) return 2;
    memset(d, 0, sizeof(*d));
    d->htab = (unsigned char *)malloc(0x200000UL);
    d->hval = (unsigned char *)malloc(0x40000UL);
    d->ord = (unsigned long *)malloc(0x40000UL);
    d->dict = (unsigned char *)calloc((size_t)(dictsz + 256UL), 1);
    d->freq = (unsigned short *)malloc(U3_NCTX * 256u * sizeof(unsigned short));
    d->raw = (unsigned short *)malloc(U3_NCTX * 256u * sizeof(unsigned short));
    d->escw = (unsigned short *)malloc(U3_NCTX * sizeof(unsigned short));
    if (!d->htab || !d->hval || !d->ord || !d->dict ||
        !d->freq || !d->raw || !d->escw) {
        uharc_dec3u_free(d);
        return 2;
    }
    memset(d->htab, 0xFF, 0x200000UL);
    memset(d->hval, 0, 0x40000UL);
    memset(d->ord, 0xFF, 0x40000UL);
    for (i = 0; i < U3_NCTX; i++) d->escw[i] = 0;
    d->dict_sz = dictsz;
    d->dict_mask = dictsz - 1UL;
    d->dict_pos = 0;
    d->ctx = 0;
    d->excl = -1;
    d->match_pos = -1;
    d->last_dist = U3_M32;
    d->hist = 7;
    u3_reset(d);
    *pctx = d;
    return 0;
}

void uharc_dec3u_free(void *ctx)
{
    dec3u *d = (dec3u *)ctx;
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

int uharc_dec3u_decode(void *ctx,
                       uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len)
{
    dec3u *d = (dec3u *)ctx;
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
    u3_rc_init(d);
    while (d->out_total < d->out_len && !d->err) {
        if (!u3_match(d)) {
            if (d->err) break;
            u3_literal(d);
        }
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
