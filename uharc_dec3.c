/* UHARC 0.6 decompressor core 3 (ALZary/LZ+range coder, methods 8..23).
 *
 * Clean portable C89 port of UNUHARC06 core 3. No Windows API, no globals;
 * all state lives in the dec3 struct (malloc/free). Progress callbacks
 * (sub_408085/40815C/40819B) are stubbed to no-ops.
 *
 * Original -> port mapping (UNUHARC06.c / UNUHARC06.lst):
 *  sub_40BC62 (init)        -> uharc_dec3_init
 *    dict size = 1 << (((method-8) % 16) + 10); allocations:
 *    0x200000 hash (dword_422DBC, init 0xFF), 0x40000 hval (dword_422DC4),
 *    0x10000 mval (dword_422DC8), 0x40000 ord (dword_422DC0, init 0xFF),
 *    dict+256 (dword_422DCC), 0x27E00 freq (dword_422DD0),
 *    0x27E00 raw (dword_422DD4), 0x27C escw (dword_422DD8).
 *    unk_439558 (50 x 8-byte literal entries) copied from unk_422DDC and
 *    prob block (10 x 2048 bytes at 0x434554) filled with 2560-64*k.
 *  sub_40BEC3 (free)        -> uharc_dec3_free
 *  sub_419A4D (range init)  -> d3_rc_init (code=5 bytes BE, range=0xFFFFFFFF)
 *  sub_40B093 (input byte)  -> d3_get (buffered via rcb; err 3 on short)
 *  sub_40B0DA (output flush)-> d3_emit/d3_flush (buffered via wcb; err 4)
 *  sub_4196E0 (memset)      -> memset; sub_416D40 (word fill) -> loops
 *  sub_40BF64 (model reset) -> d3_model_reset (freq/raw zeroed, ctx318 =
 *    uniform BIT lowbit(i)*8, raw318=8, escw=1, litm scaled, then the
 *    container progress-table walk described below)
 *  sub_40C0F0 (rescale ctx) -> d3_rescale (halve raw pairs with the exact
 *    ((d+0x10001)&0xFFFEFFFE)>>1 dword trick, rebuild BIT in place)
 *  sub_40C21D (model update)-> d3_update (raw+=32, Fenwick update, escw+=32
 *    + order-0 update on first occurrence, rescale at 0x4000)
 *  sub_40C305 (main loop)   -> uharc_dec3_decode loop
 *  sub_40C805 (match decision + copy) -> d3_match (binary choice via litm
 *    entry, length via prob groups + tab_pair walk, ring copy dict->out
 *    with output bytes bit-inverted exactly like the original)
 *  sub_40C35C (literal decode, order1+order0 w/ exclusion) -> d3_dec_sym
 *  sub_40C6E1 (literal emit)-> d3_literal (model index from ctx, update,
 *    exclusion pointer, emit ~byte, dict store, ctx shift)
 * Range renormalize pattern used everywhere:
 *    while (range < 0x1000000) { code=(code<<8)|getbyte(); range<<=8; }
 *
 * Multi-block reset rule (sub_40C305 top of loop + sub_40BF64 tail):
 *    if (out >= dword_4396E8) sub_40BF64();
 *  and sub_40BF64 advances dword_4396E8 (orig ++HIWORD => +=0x10000, clamped
 *  to the total) and then snaps it forward to the first entry of the
 *  container's cumulative file-end table (dword_4396F4, walked with the
 *  dword_4396F0 index) that is >= it.  If that file is smaller than 0x4000
 *  and each following file is smaller still, it keeps stepping forward
 *  (dword_4396F0/dword_4396E8 follow) so a run of tiny files gets one block.
 *  The check sits at the top of the loop, so the reset lands at the first
 *  output step at/after the boundary and can be mid-file (the dictionary and
 *  the output stream are continuous across blocks).
 *
 * NOTE on rescale: sub_40C0F0 copies raw[0..254] to freq[1..255] before the
 * Fenwick rebuild (freq[i] = tree over raw[i-lowbit(i)..i-1], symbol s at
 * index s+1); the port must not copy raw[0..255] to freq[0..255].
 *
 * Static data (from DGROUP in UNUHARC06.lst):
 *  unk_422BA0 (512B ctx318 template) = w[0]=256, w[i]=lowbit(i) -> built.
 *  unk_422DDC (400B litm template): w0=0,w1=tab_litw1,w2=777,w3=0.
 *  0x422F6C (16B state pairs) -> tab_st0.
 *  0x422F7C (512B length state pairs) -> tab_pair.
 *  dword_42317C (256 x u32 group index, only first 12 != 7) -> tab_grp.
 *  byte_42357C (256B, 0 in 133..158 and 165..190 else 1) -> tab_ord().
 *
 * NOTE on output inversion: the original stores ~byte into the output
 * buffer in both d3_literal and the match copy loop; the dictionary keeps
 * raw bytes. This port replicates that exactly.
 *
 * Return codes: 0 ok, 1 bad args, 2 nomem, 3 input short, 4 output short,
 * 7 data error (corrupt stream; mirrors UHARC06_ERR_DATA).
 */
#include <stdlib.h>
#include <string.h>
#ifdef D3_TRACE
#include <stdio.h>
#endif

#include "uharc_dec.h"

#define D3_M32 0xFFFFFFFFUL

typedef struct dec3_s {
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
    unsigned long code;
    unsigned long range;
    unsigned long *htab;      /* 524288 u32 hash (pos,ctx per 18-bit key) */
    unsigned char *hval;      /* 262144 bytes */
    unsigned char *mval;      /* 65536 bytes */
    unsigned long *ord;       /* 65536 u32 last-pos per 16-bit ctx */
    unsigned char *dict;      /* dict_sz + 256 ring + mirror */
    unsigned short *freq;     /* 81664 BIT words, 319 contexts */
    unsigned short *raw;      /* 81664 raw words, 319 contexts */
    unsigned short *escw;     /* 318 escape weights */
    unsigned short litm[50][4];
    unsigned long prob[5121]; /* 10 groups x 512 u32 + 1 (orig writes idx 5120) */
    unsigned long dict_sz;
    unsigned long dict_mask;
    unsigned long dict_pos;
    unsigned long ctx;
    long excl;                /* -1 = none, else offset into dict */
    long match_pos;           /* -1 = none */
    unsigned long run_len;
    unsigned long last_dist;
    unsigned long sym_lo;
    unsigned long sym_hi;
    unsigned char state;
    unsigned long thresh;     /* next model reset (orig dword_4396E8) */
    const unsigned long *cum; /* container cumulative file-end table */
    unsigned long ncum;       /* entries in cum[] (0 = no table) */
    unsigned long prog_idx;   /* orig dword_4396F0: index into cum[] */
} dec3;

/* literal entry w1 seeds (unk_422DDC bytes 2..3 of each 8-byte entry) */
static const unsigned short tab_litw1[50] = {
     81,  89,  97, 105, 105, 105, 105, 105,
     81,  89,  97, 105, 105, 105, 105, 105,
     81,  89,  97, 105, 105, 105, 105, 105,
     41,  49,  57,  65,  65,  65,  65,  65,
     41,  49,  57,  65,  65,  65,  65,  65,
     41,  49,  57,  65,  65,  65,  65,  65,
    385, 625
};

/* state transition pairs (0x422F6C), indexed [2*state + bit], state < 8 */
static const unsigned char tab_st0[16] = {
    6, 0, 6, 0, 6, 1, 6, 2, 6, 3, 6, 3, 7, 4, 7, 5
};

/* length state pairs (0x422F7C), indexed [2*v46 + sel] */
static const unsigned char tab_pair[512] = {
      0,  0,  0,  0,  0,  4,  0,  0,  3,  5,  0,  0,  2, 14,  0,  0,
      7,  9,  0,  0,  8, 12,  0,  0, 11, 13,  0,  0, 10, 30,  0,  0,
     15, 17,  0,  0, 16, 20,  0,  0, 19, 21,  0,  0, 18, 26,  0,  0,
     23, 25,  0,  0, 24, 28,  0,  0, 27, 29,  0,  0, 22, 62,  0,  0,
     31, 33,  0,  0, 32, 36,  0,  0, 35, 37,  0,  0, 34, 42,  0,  0,
     39, 41,  0,  0, 40, 44,  0,  0, 43, 45,  0,  0, 38, 54,  0,  0,
     47, 49,  0,  0, 48, 52,  0,  0, 51, 53,  0,  0, 50, 58,  0,  0,
     55, 57,  0,  0, 56, 60,  0,  0, 59, 61,  0,  0, 46,254,  0,  0,
     63, 65,  0,  0, 64, 68,  0,  0, 67, 69,  0,  0, 66, 74,  0,  0,
     71, 73,  0,  0, 72, 76,  0,  0, 75, 77,  0,  0, 70, 86,  0,  0,
     79, 81,  0,  0, 80, 84,  0,  0, 83, 85,  0,  0, 82, 90,  0,  0,
     87, 89,  0,  0, 88, 92,  0,  0, 91, 93,  0,  0, 78,110,  0,  0,
     95, 97,  0,  0, 96,100,  0,  0, 99,101,  0,  0, 98,106,  0,  0,
    103,105,  0,  0,104,108,  0,  0,107,109,  0,  0,102,118,  0,  0,
    111,113,  0,  0,112,116,  0,  0,115,117,  0,  0,114,122,  0,  0,
    119,121,  0,  0,120,124,  0,  0,123,125,  0,  0, 94,190,  0,  0,
    127,129,  0,  0,128,132,  0,  0,131,133,  0,  0,130,138,  0,  0,
    135,137,  0,  0,136,140,  0,  0,139,141,  0,  0,134,150,  0,  0,
    143,145,  0,  0,144,148,  0,  0,147,149,  0,  0,146,154,  0,  0,
    151,153,  0,  0,152,156,  0,  0,155,157,  0,  0,142,174,  0,  0,
    159,161,  0,  0,160,164,  0,  0,163,165,  0,  0,162,170,  0,  0,
    167,169,  0,  0,168,172,  0,  0,171,173,  0,  0,166,182,  0,  0,
    175,177,  0,  0,176,180,  0,  0,179,181,  0,  0,178,186,  0,  0,
    183,185,  0,  0,184,188,  0,  0,187,189,  0,  0,158,222,  0,  0,
    191,193,  0,  0,192,196,  0,  0,195,197,  0,  0,194,202,  0,  0,
    199,201,  0,  0,200,204,  0,  0,203,205,  0,  0,198,214,  0,  0,
    207,209,  0,  0,208,212,  0,  0,211,213,  0,  0,210,218,  0,  0,
    215,217,  0,  0,216,220,  0,  0,219,221,  0,  0,206,238,  0,  0,
    223,225,  0,  0,224,228,  0,  0,227,229,  0,  0,226,234,  0,  0,
    231,233,  0,  0,232,236,  0,  0,235,237,  0,  0,230,246,  0,  0,
    239,241,  0,  0,240,244,  0,  0,243,245,  0,  0,242,250,  0,  0,
    247,249,  0,  0,248,252,  0,  0,251,253,  0,  0,126,  0,  0,  0
};

/* length group index (dword_42317C): first 12 entries, rest is 7 */
static const unsigned char tab_grp[12] = {
    0, 0, 1, 2, 3, 4, 5, 5, 6, 6, 6, 6
};

/* order-0 context class (byte_42357C): 0 in two ranges, else 1 */
static int tab_ord(unsigned c)
{
    if ((c >= 133 && c <= 158) || (c >= 165 && c <= 190)) return 0;
    return 1;
}

int uharc_dec3_init(void **pctx, int method)
{
    dec3 *d;
    unsigned long dictsz;
    unsigned k;
    unsigned j;
    if (!pctx || method < 8 || method > 23) return 1;
    dictsz = 1UL << (((method - 8) % 16) + 10);
    d = (dec3 *)malloc(sizeof(*d));
    if (!d) return 2;
    memset(d, 0, sizeof(*d));
    d->htab = (unsigned long *)malloc(0x200000UL);
    d->hval = (unsigned char *)malloc(0x40000UL);
    d->mval = (unsigned char *)malloc(0x10000UL);
    d->ord = (unsigned long *)malloc(0x40000UL);
    d->dict = (unsigned char *)calloc(dictsz + 256UL, 1);
    d->freq = (unsigned short *)malloc(163328UL);
    d->raw = (unsigned short *)malloc(163328UL);
    d->escw = (unsigned short *)malloc(636UL);
    if (!d->htab || !d->hval || !d->mval || !d->ord ||
        !d->dict || !d->freq || !d->raw || !d->escw) {
        free(d->htab);
        free(d->hval);
        free(d->mval);
        free(d->ord);
        free(d->dict);
        free(d->freq);
        free(d->raw);
        free(d->escw);
        free(d);
        return 2;
    }
    memset(d->htab, 0xFF, 0x200000UL);
    memset(d->hval, 0, 0x40000UL);
    memset(d->mval, 0, 0x10000UL);
    memset(d->ord, 0xFF, 0x40000UL);
    for (k = 0; k < 50; k++) {
        d->litm[k][0] = 0;
        d->litm[k][1] = tab_litw1[k];
        d->litm[k][2] = 777;
        d->litm[k][3] = 0;
    }
    d->prob[0] = 0;
    for (k = 0; k < 10; k++) {
        for (j = 1; j <= 512; j++) d->prob[k * 512 + j] = 2560UL - 64UL * k;
    }
    d->dict_sz = dictsz;
    d->dict_mask = dictsz - 1UL;
    d->dict_pos = 0;
    d->ctx = 0;
    d->excl = -1;
    d->match_pos = -1;
    d->run_len = 0;
    d->last_dist = D3_M32;
    d->sym_lo = 0;
    d->sym_hi = 0;
    d->state = 0;
    d->thresh = 0;
    *pctx = d;
    return 0;
}

void uharc_dec3_free(void *ctx)
{
    dec3 *d = (dec3 *)ctx;
    if (!d) return;
    free(d->htab);
    free(d->hval);
    free(d->mval);
    free(d->ord);
    free(d->dict);
    free(d->freq);
    free(d->raw);
    free(d->escw);
    free(d);
}

/* ---- I/O pump ---- */
static int d3_get(dec3 *d)
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

static void d3_emit(dec3 *d, unsigned b)
{
    d->outb[d->out_fill++] = (unsigned char)b;
    d->out_total++;
    if (d->out_fill == d->out_cap) {
        if (d->wcb(d->wctx, d->outb, d->out_fill) != 0) d->err = 4;
        d->out_fill = 0;
    }
}

/* ---- range coder ---- */
static void d3_rc_init(dec3 *d)
{
    int i;
    d->code = 0;
    d->range = D3_M32;
    for (i = 0; i < 5; i++) {
        d->code = ((d->code << 8) | (unsigned long)d3_get(d)) & D3_M32;
    }
}

/* zoom (code,range) into sub-range [lo,hi); renormalize; 0 = data error */
static int d3_renorm(dec3 *d, unsigned long lo, unsigned long hi)
{
    unsigned long rng;
    unsigned long code;
    if (hi <= lo) {
        d->err = 7;
        return 0;
    }
    rng = (d->range * ((hi - lo) & D3_M32)) & D3_M32;
    code = (d->code - ((d->range * lo) & D3_M32)) & D3_M32;
    if (rng == 0) {
        d->err = 7;
        return 0;
    }
    while (rng < 0x1000000UL) {
        code = ((code << 8) | (unsigned long)d3_get(d)) & D3_M32;
        rng = (rng << 8) & D3_M32;
        if (d->err) return 0;
    }
    d->code = code;
    d->range = rng;
    return 1;
}

/* ---- model reset (sub_40BF64) ----
 * Walk of the container progress table (orig dword_4396F4/dword_4396F0):
 * thresh += 0x10000 (the original does ++HIWORD(thresh)), clamp to the
 * total, then advance to the first cumulative file end >= thresh.  If that
 * file is small (< 0x4000) and the following file is even smaller, keep
 * stepping forward (ties stay) so a run of tiny files gets one block.
 */
static void d3_model_reset(dec3 *d)
{
    unsigned i;
#ifdef D3_TRACE
    fprintf(stderr, "[d3] reset out=%lu thresh_in=%lu idx_in=%lu\n",
            d->out_total, d->thresh, d->prog_idx);
#endif
    memset(d->freq, 0, 162816UL);
    memset(d->raw, 0, 162816UL);
    for (i = 0; i < 256; i++) {
        unsigned v = (i == 0) ? 256u : (i & (0u - i));
        d->freq[318u * 256u + i] = (unsigned short)(v * 8u);
    }
    for (i = 0; i < 256; i++) d->raw[318u * 256u + i] = 8;
    for (i = 0; i < 318; i++) d->escw[i] = 1;
    for (i = 0; i < 50; i++) {
        d->litm[i][1] = (unsigned short)((d->litm[i][1] + 7) >> 3);
        d->litm[i][2] = (unsigned short)((d->litm[i][2] + 15) >> 3);
    }
    if (d->cum && d->ncum) {
        unsigned long t;
        unsigned long idx;
        unsigned long v;
        t = (d->thresh + 0x10000UL) & D3_M32;
        if (t > d->out_len) t = d->out_len;
        idx = d->prog_idx;
        for (;;) {
            if (idx >= d->ncum - 1UL) { idx = d->ncum - 1UL; break; }
            v = d->cum[idx];
            if (v >= t) break;
            idx++;
        }
        d->prog_idx = idx;
        d->thresh = d->cum[idx];
#ifdef D3_TRACE
        fprintf(stderr, "[d3]   walk t=%lu -> idx=%lu thresh=%lu ncum=%lu\n",
                t, d->prog_idx, d->thresh, d->ncum);
#endif
        if (d->prog_idx && d->thresh < d->out_len) {
            unsigned long cur = d->cum[idx] - d->cum[idx - 1];
            unsigned long nxt = d->cum[idx + 1] - d->cum[idx];
            if (cur < 0x4000UL) {
                while (nxt < cur) {
                    idx++;
                    d->prog_idx = idx;
                    d->thresh = d->cum[idx];
                    if (d->thresh >= d->out_len) break;
                    cur = nxt;
                    nxt = d->cum[idx + 1] - d->cum[idx];
                }
            }
        }
    } else {
        d->thresh = d->out_len;
    }
}

/* ---- rescale context (sub_40C0F0) ---- */
static void d3_rescale(dec3 *d, unsigned ctxidx)
{
    unsigned long total = 0;
    unsigned i;
    unsigned short *raw = d->raw + ctxidx * 256u;
    unsigned short *fr = d->freq + ctxidx * 256u;
    unsigned k;
    unsigned j;
#ifdef D3_TRACE
    fprintf(stderr, "[d3] rescale ctx=%u out=%lu escw=%u f0=%u\n", ctxidx,
            d->out_total, (unsigned)d->escw[ctxidx], (unsigned)fr[0]);
#endif
    if (ctxidx < 0x13E) d->escw[ctxidx] = (unsigned short)((d->escw[ctxidx] >> 2) + 1);
    for (i = 0; i < 256; i += 8) {
        unsigned long d0 = (unsigned long)raw[i] | ((unsigned long)raw[i + 1] << 16);
        unsigned long d1 = (unsigned long)raw[i + 2] | ((unsigned long)raw[i + 3] << 16);
        unsigned long d2 = (unsigned long)raw[i + 4] | ((unsigned long)raw[i + 5] << 16);
        unsigned long d3 = (unsigned long)raw[i + 6] | ((unsigned long)raw[i + 7] << 16);
        unsigned long h0 = ((d0 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h1 = ((d1 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h2 = ((d2 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h3 = ((d3 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        raw[i] = (unsigned short)(h0 & 0xFFFFUL);
        raw[i + 1] = (unsigned short)((h0 >> 16) & 0xFFFFUL);
        raw[i + 2] = (unsigned short)(h1 & 0xFFFFUL);
        raw[i + 3] = (unsigned short)((h1 >> 16) & 0xFFFFUL);
        raw[i + 4] = (unsigned short)(h2 & 0xFFFFUL);
        raw[i + 5] = (unsigned short)((h2 >> 16) & 0xFFFFUL);
        raw[i + 6] = (unsigned short)(h3 & 0xFFFFUL);
        raw[i + 7] = (unsigned short)((h3 >> 16) & 0xFFFFUL);
        total = (total + h0 + h1) & D3_M32;
        total = (total + h2 + h3) & D3_M32;
    }
    /* sub_40C0F0 copies raw[0..254] to freq[1..255] (510 bytes) before the
     * Fenwick rebuild, i.e. freq[i] = raw[i-1].  A plain memcpy(raw) would
     * shift the whole tree by one symbol. */
    for (i = 0; i < 255; i++) fr[i + 1] = raw[i];
    fr[0] = (unsigned short)((total + (total >> 16)) & 0xFFFFUL);
    for (k = 1; k <= 64; k *= 2) {
        for (j = 256 - 2 * k; ; j -= 2 * k) {
            fr[j] = (unsigned short)(fr[j] + fr[j - k]);
            if (j == 2 * k) break;
        }
    }
}

/* ---- model update (sub_40C21D) ---- */
static void d3_update(dec3 *d, unsigned ctxidx, unsigned sym)
{
    unsigned short *r = d->raw + ctxidx * 256u;
    unsigned short *f = d->freq + ctxidx * 256u;
    unsigned idx;
    unsigned t;
    if (r[sym] == 0) {
        unsigned short *r318 = d->raw + 318u * 256u;
        unsigned short *f318 = d->freq + 318u * 256u;
        d->escw[ctxidx] = (unsigned short)(d->escw[ctxidx] + 32);
        r318[sym] = (unsigned short)(r318[sym] + 16);
        idx = (sym + 1) & 0xFFu;
        while (idx != 0) {
            f318[idx] = (unsigned short)(f318[idx] + 16);
            idx = (idx + (idx & (0u - idx))) & 0xFFu;
        }
        f318[0] = (unsigned short)(f318[0] + 16);
        if (f318[0] >= 0x4000) d3_rescale(d, 0x13E);
    }
    r[sym] = (unsigned short)(r[sym] + 32);
    idx = (sym + 1) & 0xFFu;
    while (idx != 0) {
        f[idx] = (unsigned short)(f[idx] + 32);
        idx = (idx + (idx & (0u - idx))) & 0xFFu;
    }
    f[0] = (unsigned short)(f[0] + 32);
    t = d->escw[ctxidx] + f[0];
    if (t >= 0x4000) d3_rescale(d, ctxidx);
}

/* ---- literal decode (sub_40C35C); returns byte, sets err 7 on bad data */
static unsigned d3_dec_sym(dec3 *d, unsigned ctxidx)
{
    unsigned short *f;
    unsigned short *r;
    unsigned long total;
    unsigned long scaled;
    unsigned long esc_total;
    unsigned long v42;
    int have_eb;
    int eb_on;
    unsigned eb;
    unsigned p;
    unsigned rem;
    unsigned step;
    unsigned sym;
    unsigned q;
    unsigned short *r318;
    unsigned short *f318;
    unsigned short xsum;
    unsigned i;
    unsigned long v17;
    unsigned long s2;
    unsigned short acc;
    unsigned v40;
    unsigned target;
    int is_fallback;
    f = d->freq + ctxidx * 256u;
    r = d->raw + ctxidx * 256u;
    total = f[0];
    have_eb = (d->excl >= 0);
    eb = have_eb ? (unsigned)(unsigned char)d->dict[d->excl] : 0;
    if (have_eb) total = (total - r[eb]) & 0xFFFFUL;
    is_fallback = 0;
    if (total == 0) {
        ctxidx = 0x13E;
        f = d->freq + ctxidx * 256u;
        r = d->raw + ctxidx * 256u;
        total = f[0];
        if (have_eb) total = (total - r[eb]) & 0xFFFFUL;
        is_fallback = 1;
    }
    if (total == 0) {
        d->err = 7;
        return 0;
    }
    if (is_fallback) d->range /= total;
    else {
        esc_total = total + (unsigned long)d->escw[ctxidx];
        d->range /= esc_total;
    }
    if (d->range == 0) {
        d->err = 7;
        return 0;
    }
    scaled = d->code / d->range;
    if ((scaled & 0xFFFFUL) < total) {
        p = 0;
        rem = (unsigned)(total - scaled);
        if (have_eb) {
            v42 = r[eb];
            eb_on = 1;
            for (step = 128; step != 0; step >>= 1) {
                q = p + step;
                if (!eb_on || q <= eb) {
                    if (rem > f[q]) {
                        p = q;
                        rem -= f[q];
                    }
                } else if (f[q] < v42 + rem) {
                    p = q;
                    eb_on = 0;
                    rem = (rem - f[q] + (unsigned)v42) & 0xFFFFU;
                }
            }
        } else {
            for (step = 128; step != 0; step >>= 1) {
                q = p + step;
                if (rem > f[q]) {
                    p = q;
                    rem -= f[q];
                }
            }
        }
        sym = p;
        if (sym > 255) {
            d->err = 7;
            return 0;
        }
        d->sym_hi = (scaled + rem) & 0xFFFFUL;
        if (r[sym] > d->sym_hi) {
            d->err = 7;
            return 0;
        }
        d->sym_lo = d->sym_hi - r[sym];
        if (!d3_renorm(d, d->sym_lo, d->sym_hi)) return 0;
        return sym;
    } else {
        r318 = d->raw + 318u * 256u;
        f318 = d->freq + 318u * 256u;
        xsum = 0;
        acc = 0;
        v40 = 256;
        d->sym_lo = total;
        d->sym_hi = (unsigned long)d->escw[ctxidx] + total;
        if (!d3_renorm(d, d->sym_lo, d->sym_hi)) return 0;
        for (i = 0; i < 256; i++) {
            if (r[i] != 0) xsum = (unsigned short)(xsum + r318[i]);
        }
        if (have_eb && r[eb] == 0) xsum = (unsigned short)(xsum + r318[eb]);
        v17 = (f318[0] - xsum) & D3_M32;
        if (v17 == 0) {
            d->err = 7;
            return 0;
        }
        d->range /= v17;
        if (d->range == 0) {
            d->err = 7;
            return 0;
        }
        s2 = d->code / d->range;
        if (have_eb) {
            target = eb + 1;
            while (v40 != target) {
                --v40;
                if (r[v40] == 0) {
                    acc = (unsigned short)(acc + r318[v40]);
                    if (acc > (unsigned short)(s2 & 0xFFFFUL)) break;
                }
            }
            if (acc <= (unsigned short)(s2 & 0xFFFFUL)) {
                if (v40 == 0) {
                    d->err = 7;
                    return 0;
                }
                --v40;
            }
        }
        if (acc <= (unsigned short)(s2 & 0xFFFFUL)) {
            for (;;) {
                if (v40 == 0) {
                    d->err = 7;
                    return 0;
                }
                --v40;
                if (r[v40] == 0) break;
            }
            for (;;) {
                acc = (unsigned short)(acc + r318[v40]);
                if (acc > (unsigned short)(s2 & 0xFFFFUL)) break;
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
        d->sym_hi = acc;
        d->sym_lo = (acc - r318[v40]) & 0xFFFFUL;
        sym = v40;
        if (!d3_renorm(d, d->sym_lo, d->sym_hi)) return 0;
        return sym;
    }
}

/* ---- match/literal decision + match copy (sub_40C805) ----
 * returns 1 when match bytes were copied, 0 when a literal is needed.
 */
static int d3_match(dec3 *d)
{
    unsigned long h;
    unsigned long *he;
    unsigned long mpos;
    unsigned v1 = 0;
    unsigned v45 = 0;
    unsigned char *mp;
    if (d->err) return 0;
    d->match_pos = -1;
    h = ((d->ctx ^ (d->ctx >> 13)) & 0x3FFFFUL);
    he = d->htab + h * 2;
    mpos = he[0];
    if (d->run_len) {
        mpos = (d->dict_pos - d->last_dist) & d->dict_mask;
        v1 = (d->run_len > 2) + 48;
    }
    if (v1 == 0) {
        if (mpos == D3_M32 || he[1] != d->ctx) {
            unsigned long dist;
            v45 = 0;
            mpos = (d->dict_pos - d->last_dist) & d->dict_mask;
            dist = (mpos - d->dict_pos) & d->dict_mask;
            if (dist < 0x104UL) {
                /* recent: try order table */
                unsigned long o = d->ord[d->ctx & 0xFFFFUL];
                if (o != D3_M32) {
                    mpos = o;
                    v1 = 17;
                }
            } else {
                /* 4 bytes at (mpos-4)&mask vs ctx (big-endian, no wrap) */
                unsigned long mp4 = (mpos - 4) & d->dict_mask;
                unsigned long a0 = ((unsigned long)d->dict[mp4] << 24)
                    | ((unsigned long)d->dict[mp4 + 1] << 16)
                    | ((unsigned long)d->dict[mp4 + 2] << 8)
                    | (unsigned long)d->dict[mp4 + 3];
                unsigned long c = d->ctx;
                if ((unsigned short)a0 != (unsigned short)c
                    && (a0 & 0xFFFF00FFUL) != (c & 0xFFFF00FFUL)
                    && (a0 & 0xFFFFFF00UL) != (c & 0xFFFFFF00UL)) {
                    unsigned long o = d->ord[d->ctx & 0xFFFFUL];
                    if (o != D3_M32) {
                        mpos = o;
                        v1 = 17;
                    }
                } else {
                    v1 = 9;
                }
            }
        } else {
            v45 = 1;
            v1 = 1;
        }
    }
    he[0] = d->dict_pos;
    d->ord[d->ctx & 0xFFFFUL] = d->dict_pos;
    he[1] = d->ctx;
    if (v1 == 0 || ((mpos - d->dict_pos) & d->dict_mask) < 0x100UL) {
        d->run_len = 0;
        return 0;
    }
    if (!d->run_len) {
        if (d->hval[h]) v1 += d->state - 1;
        else v1 += d->state + 23;
    }
    mp = d->dict + (mpos & d->dict_mask);
    if (d->excl >= 0 && d->dict[d->excl] == *mp) {
        /* exclusion hit: literal, update entry total only */
        {
        unsigned short *e;
        e = d->litm[v1];
        e[2] = (unsigned short)(e[2] + 64);
        if (e[2] >= 0x4000) {
            e[1] = (unsigned short)((e[1] + 1) >> 1);
            e[2] = (unsigned short)((e[2] + 2) >> 1);
        }
        d->state = tab_st0[2 * d->state + 1];
        d->run_len = 0;
        return 0;
        }
    }
    {
        unsigned v42;
        unsigned nxt;
        unsigned short *e;
        unsigned long scaled;
        unsigned bit;
        unsigned long lo;
        unsigned long hi;
        v42 = ((unsigned)d->dict[mpos & d->dict_mask]) << 8;
        nxt = (mpos + 1) & d->dict_mask;
        if (nxt != d->dict_pos) v42 |= d->dict[nxt];
        d->match_pos = (long)(mpos & d->dict_mask);
        e = d->litm[v1];
        if (e[2] == 0) {
            d->err = 7;
            return 0;
        }
        d->range /= e[2];
        if (d->range == 0) {
            d->err = 7;
            return 0;
        }
        scaled = d->code / d->range;
        bit = ((scaled & 0xFFFFUL) >= e[1]) ? 1 : 0;
        if (bit == 0) {
            lo = e[0];
            hi = e[1];
            e[1] = (unsigned short)(e[1] + 64);
        } else {
            lo = e[1];
            hi = e[2];
        }
        e[2] = (unsigned short)(e[2] + 64);
        if (e[2] >= 0x4000) {
            e[1] = (unsigned short)((e[1] + 1) >> 1);
            e[2] = (unsigned short)((e[2] + 2) >> 1);
        }
        d->state = tab_st0[2 * d->state + bit];
        if (!d3_renorm(d, lo, hi)) return 0;
        if (bit) {
            d->hval[h] = 0;
            d->mval[v42] = 0;
            d->run_len = 0;
            return 0;
        }
        {
            unsigned v46 = 6;
            unsigned v51 = 0xFF;
            unsigned v22 = d->mval[v42];
            unsigned v23;
            unsigned long *cell;
            unsigned long pv;
            unsigned n;
            unsigned long mcx;
            unsigned long mdp;
            unsigned long mmp;
            if (d->run_len) v23 = 8 + (d->run_len > 2 ? 1 : 0);
            else v23 = (v45 ? d->hval[h] : d->mval[v42]) < 12
                ? tab_grp[v45 ? d->hval[h] : d->mval[v42]] : 7;
            for (;;) {
                cell = d->prob + 1 + 512u * v23 + ((v22 <= v46) ? 256u : 0u) + v46;
                pv = *cell;
                d->range >>= 12;
                if (d->range == 0) {
                    d->err = 7;
                    return 0;
                }
                scaled = d->code / d->range;
                if ((scaled & 0xFFFFUL) >= pv) {
                    d->sym_lo = pv;
                    d->sym_hi = 4096;
                    *cell -= (pv >> 6);
                } else {
                    v51 = v46;
                    d->sym_hi = pv;
                    d->sym_lo = 0;
                    *cell += ((4096UL - pv) >> 6);
                }
                if (!d3_renorm(d, d->sym_lo, d->sym_hi)) return 0;
                v46 = tab_pair[2 * v46 + (v51 > v46 ? 1 : 0)];
                if (v46 == 0) break;
            }
            d->hval[h] = (unsigned char)v51;
            d->mval[v42] = (unsigned char)v51;
            d->last_dist = (d->dict_pos - (mpos & d->dict_mask)) & d->dict_mask;
            if (v51 == 0xFF) {
                d->excl = -1;
                d->run_len++;
            } else {
                d->run_len = 0;
                d->excl = (long)((mpos + v51) & d->dict_mask);
            }
            n = v51 == 0 ? 256 : v51;
            mcx = d->ctx;
            mdp = d->dict_pos;
            mmp = mpos & d->dict_mask;
            while (n--) {
                {
                unsigned b;
                b = d->dict[mmp];
                d->dict[mdp] = (unsigned char)b;
                if (mdp < 0x100) d->dict[d->dict_sz + mdp] = (unsigned char)b;
                mcx = ((mcx << 8) + b) & D3_M32;
                d3_emit(d, b ^ 0xFFu);
                mdp = (mdp + 1) & d->dict_mask;
                mmp = (mmp + 1) & d->dict_mask;
                }
            }
            d->ctx = mcx;
            d->dict_pos = mdp;
            return 1;
        }
    }
}

/* ---- literal emit (sub_40C6E1) ---- */
static void d3_literal(dec3 *d)
{
    unsigned long c16 = d->ctx & 0xFFFFUL;
    unsigned lo = (unsigned)(c16 & 0xFFUL);
    unsigned hi = (unsigned)((c16 >> 8) & 0xFFUL);
    unsigned midx;
    unsigned b;
    if (c16 == 0xFFFFUL) midx = 314;
    else if (c16 == 0xD1DFUL) midx = 315;
    else if (c16 == 0xD3DFUL) midx = 316;
    else if (lo == 0xDF) midx = tab_ord(hi) ? 317 : 223;
    else if (!tab_ord(lo) && tab_ord(hi)) midx = lo + 123;
    else midx = lo;
    b = d3_dec_sym(d, midx);
    if (d->err) return;
    d3_update(d, midx, b);
    d->excl = -1;
    if (d->match_pos >= 0 && d->dict[d->match_pos] == (unsigned char)b) {
        d->excl = (long)(((unsigned long)d->match_pos + 1UL) & d->dict_mask);
    }
    d3_emit(d, b ^ 0xFFu);
    d->ctx = ((d->ctx << 8) + b) & D3_M32;
    d->dict[d->dict_pos] = (unsigned char)b;
    if (d->dict_pos < 0x100) d->dict[d->dict_sz + d->dict_pos] = (unsigned char)b;
    d->dict_pos = (d->dict_pos + 1) & d->dict_mask;
}

/* Entry point with the container's cumulative file-end table (multi-block).
 * cum[] may be NULL (ncum = 0) for a single-block stream: the model then
 * resets only once, before the first byte, exactly as before. */
int uharc_dec3_decode_b(void *ctx,
                        const unsigned long *cum, unsigned long ncum,
                        uharc_dec_read_cb rcb, void *rctx,
                        uharc_dec_write_cb wcb, void *wctx,
                        unsigned long out_len)
{
    dec3 *d = (dec3 *)ctx;
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
    d->thresh = 0;
    d->prog_idx = 0;
    if (cum && ncum && cum[ncum - 1] == out_len) {
        d->cum = cum;
        d->ncum = ncum;
    } else {
        d->cum = 0;
        d->ncum = 0;
    }
    d3_rc_init(d);
    while (d->out_total < d->out_len && !d->err) {
        if (d->out_total >= d->thresh) d3_model_reset(d);
        if (d->err) break;
        if (!d3_match(d)) {
            if (d->err) break;
            d3_literal(d);
        }
    }
    err = d->err;
#ifdef D3_TRACE
    fprintf(stderr, "[d3] end err=%d out=%lu inpos=%u\n", err, d->out_total,
            d->in_pos);
#endif
    if (!err && d->out_fill) {
        if (wcb(wctx, d->outb, d->out_fill) != 0) err = 4;
    }
    free(d->inb);
    free(d->outb);
    d->inb = 0;
    d->outb = 0;
    return err;
}

int uharc_dec3_decode(void *ctx,
                       uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len)
{
    return uharc_dec3_decode_b(ctx, 0, 0UL, rcb, rctx, wcb, wctx, out_len);
}
