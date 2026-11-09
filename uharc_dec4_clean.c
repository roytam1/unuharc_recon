/* UHARC 0.6 core 4 (PPM) - clean typed implementation (C89).
 * See uharc_dec4_clean.h for status. Port of uharc-rs ppm.rs.
 */
#include <stdlib.h>
#include <string.h>
#include "uharc_dec4_clean.h"
#include "dec4c_rom.h"

#define D4_DICT_MAX (32UL << 20) /* 32 MB max for method 39 */

#define DEC4C_SEE_LEN 0x8000U
#define SI(a) (((a) - 0x446464U) / 4U)
#define D4C_ML SI(0x446464U)
#define D4C_S_O4BIN SI(0x448464U)
#define D4C_S_O4ESC SI(0x450464U)
#define D4C_S_O2BIN SI(0x452464U)
#define D4C_S_O4FST SI(0x452864U)
#define D4C_S_O2MRU SI(0x453064U)
#define D4C_S_O1TOT SI(0x453464U)
#define D4C_S_O2O2 SI(0x4534e4U)
#define D4C_S_O4O2 SI(0x4554e4U)
#define D4C_S_O1MRU SI(0x4558e4U)
#define D4C_S_O2ESC SI(0x4578e4U)
#define D4C_MARKS SI(0x457964U)
#define D4C_S_O2FST SI(0x457d64U)
#define D4C_S_O4MPS SI(0x458564U)
#define D4C_S_O4MRU SI(0x45a564U)

static unsigned char dec4c_rom8(unsigned long a)
{
    unsigned long off = a - 0x422BA0UL;
    if (a < 0x422BA0UL || off >= sizeof(DEC4C_DG_422BA0))
        return 0;
    return DEC4C_DG_422BA0[off];
}

static unsigned long dec4c_rom32(unsigned long a)
{
    unsigned long v = dec4c_rom8(a);
    v |= ((unsigned long)dec4c_rom8(a + 1UL)) << 8;
    v |= ((unsigned long)dec4c_rom8(a + 2UL)) << 16;
    v |= ((unsigned long)dec4c_rom8(a + 3UL)) << 24;
    return v;
}

typedef struct dec4c_ctx_s {
    int method;
    unsigned long dictsize;
    unsigned long dictmask;
    unsigned char *dict;      /* history buffer (wraps at dictsize) */
    unsigned long pos;        /* write position (0x45d110) */
    /* Range coder (0x419a4d): 5 priming bytes, 32-bit code/range */
    unsigned long code;
    unsigned long range;
    unsigned long rc_t;       /* 0x45c67c total */
    unsigned char *inbuf;     /* stream bytes (whole packed stream) */
    unsigned long in_len;
    unsigned long in_pos;
    /* Stream header (0x413a96) */
    unsigned char binmode;    /* binary mode from start */
    unsigned long sw_pos;     /* text->binary switch position */
    /* File boundaries / kennedy (flags2 bit6) */
    unsigned long kennedy_end; /* 0x45d124 equivalent, 0xFFFFFFFF if off */
    unsigned char g_e395;      /* 0x45e395 equivalent (0 or 10) */
    unsigned long out_pos;
    unsigned long out_len;
    /* Segment/multimedia type (0x40deb6/0x41013c) */
    unsigned char mmtype;      /* 0x45e390 */
    unsigned long mmctx;       /* 0x45ae20 */
    unsigned long mmper;       /* 0x45ae24 */
    unsigned long ch;          /* 0x45ae2c channel */
    unsigned char g_e38c;      /* 0x45e38c */
    unsigned char g_e38e;      /* 0x45e38e */
    unsigned char g_e38d;      /* 0x45e38d (type==5||6) */
    /* Mm predictor state (0x412fb0, types 5..9) */
    struct {
        unsigned long sel, d730, d734, d738, d73c, d740, d744;
        unsigned long acc[0x80];
        unsigned long hist[0x80];
        unsigned long pred[8];
        unsigned long errs[6][0x101];
        unsigned long rec[4][31];
        unsigned char sgn[4];
    } mm;
    /* Order-1 tables (0x40cdd1): 0x301 contexts */
    unsigned short esc[0x400];   /* 0x4236c8 escape counts */
    unsigned short inc_u[0x400]; /* 0x45d768 */
    unsigned short inc_b[0x400]; /* 0x45d166 */
    unsigned long ba3c[0x400];   /* 0x45ba3c */
    unsigned long ae38[0x400];   /* 0x45ae38 */
    unsigned char mru_s[0x400];  /* 0x45e086 */
    unsigned char mru_n[0x400];  /* 0x45dd85 */
    /* SEE tables (0x40dbc5): one array, indexed by SI() */
    unsigned long see[DEC4C_SEE_LEN];
    /* Segment state (0x40deb6) */
    unsigned short o1t[0x310][2][256]; /* CUM=0, FRQ=1 per context */
    unsigned short mflag[0x400];       /* 0x445db4 */
    unsigned char pair[0x80];          /* 0x45ad64 delta pairs */
    unsigned long g6b8[0x400];         /* 0x45c6b8/0x45ccd4 */
    unsigned long recent[256], lastpos[256], per[256];
    unsigned long ring[256], avg_a[2], avg_b[2];
    unsigned long g45c6b0, g45c6ac, g45c6a0, g45c6b4, g45c68c, g45c688;
    unsigned long g45c66c, g45c674, g45c64c, g45c670;
    unsigned long g45c650, g45c644, g45c640, g45c654;
    unsigned long seg_end, text_end, file_idx;
    unsigned long g45d104, g45d120, g45d124, g45d130, g45d134, g45d138;
    unsigned long nbytes, c0, c1, c2;  /* 0x45d0f8/fc/100 last symbols */
    unsigned long pend, mchk_p, pair_p; /* 0x45d11c/148/14c, 0xFFFFFFFF=none */
    unsigned long *mh1, *mh2;           /* 0x423688/8c hashes, 0x80000 each */
    unsigned char *mlen, *mchk, *x698; /* 0x423690/94/98, 0x40000 each */
    unsigned char g_e3a0, g_e3a2;       /* 0x45e3a0/a2 */
    unsigned long g45d114;              /* 0x45d114 match continuation */
    unsigned char bad;
    unsigned long *fe;                 /* file ends */
    unsigned long fe_n, fe_idx;
    unsigned char fe_on;
    /* Order-2 records (0x423684, 20 B each) + delta (binary mode).
     * o2 covers 0..0x10000, delta covers 0x10000.. via o2r/o2_put
     * (each 4-byte word of a 20-byte record is one delta entry). */
    unsigned long *o2;                   /* 0x10000 * 5 u32 */
    unsigned char (*delta)[4];            /* 0x100000 * 4 bytes */
    unsigned char *o4;                   /* 0x40000 * 32 bytes */
} dec4c_ctx;

static unsigned char g_c_flags2;

void uharc_dec4c_set_flags2(unsigned char flags2)
{
    g_c_flags2 = flags2;
}

int uharc_dec4c_init(void **ctx, int method)
{
    dec4c_ctx *c;
    if (!ctx) return 1;
    *ctx = 0;
    if (method < 24 || method > 39) return 1;
    c = (dec4c_ctx *)calloc(1, sizeof(*c));
    if (!c) return 2;
    /* dict = 1 << (((m-24) % 16) + 10), as in 0.6 dec4 */
    c->dictsize = 1UL << (unsigned)(((method - 24) % 16) + 10);
    c->dictmask = c->dictsize - 1UL;
    c->dict = (unsigned char *)calloc(1, (size_t)c->dictsize);
    c->mh1 = (unsigned long *)calloc(0x80000UL, sizeof(unsigned long));
    c->mh2 = (unsigned long *)calloc(0x80000UL, sizeof(unsigned long));
    c->mlen = (unsigned char *)calloc(0x40000UL, 1);
    c->mchk = (unsigned char *)calloc(0x40000UL, 1);
    c->x698 = (unsigned char *)calloc(0x40000UL, 1);
    c->o2 = (unsigned long *)calloc(0x10000UL * 5UL, sizeof(unsigned long));
    c->delta = (unsigned char *)calloc(0x100000UL, 4U);
    c->o4 = (unsigned char *)calloc(0x40000UL, 32U);
    if (!c->dict || !c->mh1 || !c->mh2 || !c->mlen || !c->mchk || !c->x698 ||
        !c->o2 || !c->delta || !c->o4) {
        free(c->dict); free(c->mh1); free(c->mh2);
        free(c->mlen); free(c->mchk); free(c->x698);
        free(c->o2); free(c->delta); free(c->o4);
        free(c); return 2;
    }
    /* hashes init to 0xFFFFFFFF (calloc gives 0, so fill) */
    memset(c->mh1, 0xFF, 0x80000UL * sizeof(unsigned long));
    memset(c->mh2, 0xFF, 0x80000UL * sizeof(unsigned long));
    c->method = method;
    c->kennedy_end = 0xFFFFFFFFUL;
    c->pend = c->mchk_p = c->pair_p = 0xFFFFFFFFUL;
    *ctx = c;
    return 0;
}

void uharc_dec4c_free(void *ctx)
{
    dec4c_ctx *c = (dec4c_ctx *)ctx;
    if (!c) return;
    free(c->dict);
    free(c->mh1); free(c->mh2);
    free(c->mlen); free(c->mchk); free(c->x698);
    free(c->o2); free(c->delta); free(c->o4);
    free(c->fe);
    free(c->inbuf);
    free(c);
}

static unsigned char dec4c_get_byte(dec4c_ctx *c)
{
    if (c->in_pos < c->in_len)
        return c->inbuf[c->in_pos++];
    return 0;
}

/* Range coder init (0x419a4d): code=0, range=0xFFFFFFFF, 5 bytes. */
static void dec4c_rc_init(dec4c_ctx *c)
{
    int i;
    c->code = 0;
    c->range = 0xFFFFFFFFUL;
    for (i = 0; i < 5; i++)
        c->code = (c->code << 8) | dec4c_get_byte(c);
}

/* Decode a value in [0,total) using range coder (decoder side). */
static unsigned long dec4c_rc_div(dec4c_ctx *c, unsigned long total)
{
    unsigned long range, code, v;
    if (total == 0) return 0;
    range = c->range / total;
    v = code = c->code / range;
    c->range = range;
    return v;
}

static void dec4c_rc_dec(dec4c_ctx *c, unsigned long lo, unsigned long hi)
{
    unsigned long range = c->range;
    c->code = c->code - lo * range;
    c->range = (hi - lo) * range;
    while (c->range < (1UL << 24)) {
        c->code = (c->code << 8) | dec4c_get_byte(c);
        c->range <<= 8;
    }
}

static unsigned char dec4c_tb(dec4c_ctx *c, unsigned long i)
{
    if (i < c->dictsize)
        return c->dict[i];
    if (i < c->dictsize + 0x100UL && c->dict)
        return 0; /* mirrored half handled by push_hist; tb uses mask */
    return 0;
}

/* Equal-byte count backwards from a and b (max 64). */
static unsigned dec4c_match_len(dec4c_ctx *c, unsigned long a, unsigned long b)
{
    unsigned n = 0;
    while (n < 0x40U &&
           dec4c_tb(c, (a - n) & 0xFFFFFFFFUL) == dec4c_tb(c, (b - n) & 0xFFFFFFFFUL))
        n++;
    return n;
}

/* 0x40e810: update match-flag model. */
static void dec4c_match_flag_update(dec4c_ctx *c, unsigned ctx, unsigned k)
{
    unsigned base = 4U * ctx;
    if (k == 1)
        c->mflag[base + 2] = (unsigned short)(c->mflag[base + 2] + 0x20U);
    c->mflag[base + 3] = (unsigned short)(c->mflag[base + 3] + 0x20U);
    if (c->mflag[base + 3] >= 0x4000U) {
        unsigned long edx = c->mflag[base + 2];
        unsigned long ecx = c->mflag[base + 1];
        unsigned long cc = (edx - ecx + 1U) >> 1;
        unsigned long ss = ((c->mflag[base + 3] - edx + 1U) >> 1) & 0xFFFFFFFFUL;
        cc = (cc + ecx) & 0xFFFFUL;
        ss = (ss + cc) & 0xFFFFUL;
        c->mflag[base + 2] = (unsigned short)cc;
        c->mflag[base + 3] = (unsigned short)ss;
    }
    {
        unsigned i = (unsigned)(c->g_e3a2) * 2U + k;
        c->g_e3a2 = dec4c_rom8(0x4239bbUL + (i & 0x1FU));
    }
}

/* Forward: defined after init_see. */
static void dec4c_set_mmtype(dec4c_ctx *c, unsigned char bl);

/* 0x41013c: 3-way match flag (0=change type, 1=use, 2=reject). */
static unsigned dec4c_match_flag(dec4c_ctx *c, unsigned ctx)
{
    for (;;) {
        unsigned base = 4U * ctx;
        unsigned long tot;
        unsigned v;
        unsigned k = 3U;
        unsigned ebx;
        unsigned long lo, hi;
        tot = c->mflag[base + 3];
        v = (unsigned)dec4c_rc_div(c, tot);
        ebx = base + 3;
        for (;;) {
            ebx -= 1U;
            k -= 1U;
            if (v >= c->mflag[ebx] || k == 0)
                break;
        }
        lo = c->mflag[ebx];
        hi = c->mflag[ebx + 1];
        dec4c_rc_dec(c, lo, hi);
        if (k != 0) {
            dec4c_match_flag_update(c, ctx, k);
            return k;
        }
        if (c->bad)
            return 2;
        {
            unsigned long vv = dec4c_rc_div(c, 10UL);
            unsigned long t = vv & 0xFFUL;
            dec4c_rc_dec(c, t, t + 1UL);
            dec4c_set_mmtype(c, (unsigned char)vv);
        }
    }
}

/* First symbol (0x413a96): 1 bit binary mode, else switch position. */
static void dec4c_read_switch_pos(dec4c_ctx *c)
{
    unsigned long bit, v;
    unsigned shift;
    bit = dec4c_rc_div(c, 2);
    dec4c_rc_dec(c, bit & 0xFFUL, (bit & 0xFFUL) + 1UL);
    c->binmode = (unsigned char)(bit & 0xFFUL);
    if (c->binmode != 0)
        return;
    c->sw_pos = 0;
    shift = 0;
    for (;;) {
        v = dec4c_rc_div(c, 0x800UL) & 0xFFFFUL;
        dec4c_rc_dec(c, v, v + 1UL);
        c->sw_pos += v << shift;
        if (v == 0x7FFUL)
            break;
        shift += 11;
    }
}

/* SEE init (0x40dbc5). */
static void dec4c_init_see(dec4c_ctx *c)
{
    unsigned long k, j, u, a, b, cc;
    unsigned long ebx;
    unsigned long *see = c->see;
    for (k = 0; k < 0x20UL; k++) {
        for (j = 0; j < 16UL; j++) {
            unsigned long x, i;
            x = (k & 4UL) + dec4c_rom32(0x42758dUL + 4UL * j) + ((k >= 16UL) ? 8UL : 0UL);
            i = 16UL * k + j;
            see[D4C_S_O2FST + i] = ((x << 13) >> 6) & 0xFFFFFFFFUL;
            see[D4C_S_O4FST + i] = see[D4C_S_O2FST + i];
        }
        see[D4C_S_O2ESC + k] = 0x1000UL;
        see[D4C_S_O1TOT + k] = (dec4c_rom32(0x427651UL + 4UL * k) << 7) & 0xFFFFFFFFUL;
    }
    ebx = 0;
    for (u = 0; u < 0x80UL; u++) {
        for (j = 0; j < 16UL; j++) {
            unsigned long t = dec4c_rom32(0x4275cdUL + 4UL * j);
            unsigned long a8 = (u >= 8UL) ? 4UL : 0UL;
            unsigned long a16 = (u >= 16UL) ? 4UL : 0UL;
            unsigned long a64 = (u >= 64UL) ? 4UL : 0UL;
            unsigned long b16 = (u >= 16UL) ? 1UL : 0UL;
            if (u < 16UL)
                see[D4C_S_O2MRU + ebx] = (((t + a8) << 13) >> 6) & 0xFFFFFFFFUL;
            if (u < 32UL)
                see[D4C_S_O4MRU + ebx] = (((t + a16) << 13) >> 6) & 0xFFFFFFFFUL;
            if (u < 16UL)
                see[D4C_S_O4O2 + ebx] = (((t + a8) << 13) >> 6) & 0xFFFFFFFFUL;
            see[D4C_S_O1MRU + ebx] = (((t + a64) << 13) >> 6) & 0xFFFFFFFFUL;
            see[D4C_S_O2O2 + ebx] = (((b16 + t + a64) << 13) >> 6) & 0xFFFFFFFFUL;
            see[D4C_S_O4MPS + ebx] = (((a64 + b16 + t) << 13) >> 6) & 0xFFFFFFFFUL;
            ebx += 1UL;
        }
    }
    for (a = 0; a < 8UL; a++) {
        for (b = 0; b < 8UL; b++) {
            for (cc = 0; cc < 32UL; cc++) {
                unsigned long v = 0x1000UL;
                if (a > b) v = (v - dec4c_rom32(0x42760dUL + 4UL * (a - b))) & 0xFFFFFFFFUL;
                if (b > a) v = (v + dec4c_rom32(0x42760dUL + 4UL * (b - a))) & 0xFFFFFFFFUL;
                see[D4C_S_O4ESC + (a * 0x100UL + b * 0x20UL + cc)] = v;
            }
        }
        for (cc = 0; cc < 0x20UL; cc++) {
            unsigned long x = dec4c_rom32(0x42762dUL + 4UL * ((cc & 1UL) + a));
            see[D4C_S_O2BIN + (a * 0x20UL + cc)] = ((x << 13) >> 6) & 0xFFFFFFFFUL;
        }
        for (cc = 0; cc < 0x400UL; cc++) {
            unsigned long x = dec4c_rom32(0x42762dUL + 4UL * ((cc & 1UL) + a));
            see[D4C_S_O4BIN + (a * 0x400UL + cc)] = ((x << 13) >> 6) & 0xFFFFFFFFUL;
        }
        for (k = 0; k < 0x100UL; k++)
            see[D4C_ML + (a * 0x100UL + k)] = 0x800UL;
    }
}

/* Order-1 init (0x40cdd1 top): esc/inc/ba3c/ae38/mru fills. */
static void dec4c_init_model(dec4c_ctx *c)
{
    unsigned i;
    for (i = 0x209U; i <= 0x300U; i++)
        c->esc[i] = 1;
    for (i = 0; i <= 0x300U; i++) {
        c->inc_u[i] = 0x20U;
        c->inc_b[i] = 0x10U;
        c->ba3c[i] = 1UL;
        c->ae38[i] = 0x4000UL;
    }
    for (i = 0x2f9U; i <= 0x300U; i++)
        c->inc_u[i] = 0x0AU;
    for (i = 0x2e9U; i <= 0x300U; i++)
        c->inc_b[i] = 8U;
    c->inc_b[0x200] = 6U;
    for (i = 0; i <= 0x300U; i++)
        c->mru_s[i] = 0xFFU;
    dec4c_init_see(c);
}

/* Mm reset (0x412fb0) for types 5..9. */
static void dec4c_mm_new(dec4c_ctx *c)
{
    int r;
    memset(&c->mm, 0, sizeof(c->mm));
    c->mm.d734 = 2UL;
    for (r = 0; r < 4; r++) {
        c->mm.rec[r][28] = 0xFFFFFFFFUL;
        c->mm.rec[r][29] = 0xFFFFFFFFUL;
        c->mm.rec[r][30] = 0xFFFFFFFFUL;
    }
}

/* Data-type switch (shared tail of 0x40deb6/0x41013c). */
static void dec4c_set_mmtype(dec4c_ctx *c, unsigned char bl)
{
    unsigned long i;
    if (bl == c->mmtype)
        return;
    c->mmtype = bl;
    i = (unsigned long)bl;
    c->mmctx = dec4c_rom32(0x4246c8UL + 4UL * i);
    c->mmper = dec4c_rom32(0x4246a0UL + 4UL * i);
    c->g_e38c = (unsigned char)dec4c_rom8(0x424718UL + i);
    c->g_e38e = 0;
    c->ch = 0;
    c->g_e38d = (bl == 5 || bl == 6) ? 1 : 0;
    if (bl > 4)
        dec4c_mm_new(c);
}

static unsigned long dec4c_fe_get(dec4c_ctx *c, unsigned long i)
{
    if (i < c->fe_n)
        return c->fe[i];
    c->bad = 1;
    return 0;
}

/* Fenwick rebuild (o1.rs fenwick_build). */
static void dec4c_fenwick_build(unsigned short *t)
{
    unsigned s = 1, i;
    while (s < 0x80U) {
        i = 256U - 2U * s;
        for (;;) {
            t[i] = (unsigned short)(t[i] + t[i - s]);
            i -= 2U * s;
            if (i == 0) break;
        }
        s *= 2U;
    }
}

/* 0x40e4ff: rescale context c. */
static void dec4c_o1_rescale(dec4c_ctx *c, unsigned long ctx)
{
    unsigned idx = (ctx >= 0x310UL) ? 0x30F : (unsigned)ctx;
    unsigned bm = c->binmode;
    unsigned long sum = 0;
    int k;
    for (k = 0; k < 256; k++) {
        unsigned nw;
        if (bm == 0) nw = (c->o1t[idx][1][k] + 1U) >> 1;
        else if (c->o1t[idx][1][k] > 0x10U) nw = c->o1t[idx][1][k] >> 1;
        else nw = 0;
        c->o1t[idx][1][k] = (unsigned short)nw;
        sum += nw;
    }
    for (k = 0; k < 255; k++)
        c->o1t[idx][0][k + 1] = c->o1t[idx][1][k];
    c->o1t[idx][0][0] = (unsigned short)sum;
    dec4c_fenwick_build(c->o1t[idx][0]);
    if (c->o1t[idx][0][0] != 0) {
        unsigned e = (c->esc[idx] + 1U) >> 1;
        unsigned long v;
        c->esc[idx] = (unsigned short)e;
        v = (unsigned long)e + (unsigned long)c->o1t[idx][0][0];
        if (ctx < (0x200U >> bm))
            v = (v * 0x60UL) & 0xFFFFFFFFUL;
        else
            v <<= 8;
        if (c->mmtype == 3 && (long)c->o1t[idx][0][0] > ((long)e << 3))
            v <<= 4;
        c->ae38[idx] = v;
    }
}

/* Stubs for the two rescalers called from new_segment file-size tweaks.
 * Correct for the zero-model first segment; full ports pending. */
static void dec4c_rescale_o2(dec4c_ctx *c);
static void dec4c_rescale_o4(dec4c_ctx *c);

#define DEC4C_O4_N 0x40000UL
static unsigned long dec4c_o4s(unsigned long i)
{
    return ((i & 0x7FFFUL) << 3) | ((i >> 15) & 7UL);
}

static void dec4c_rescale_o4(dec4c_ctx *c)
{
    unsigned long eax = 0;
    for (;;) {
        unsigned long edi = dec4c_o4s(eax);
        unsigned char *b = c->o4 + edi * 32UL;
        unsigned bh = 0;
        int k;
        if (b[0] == 0) {
            eax = (eax & 0x7FFFUL) + 1UL;
            if (eax >= 0x8000UL)
                return;
            continue;
        }
        for (k = 11; k >= 0; k--) {
            unsigned v = (unsigned)(b[1 + k] >> 2);
            b[1 + k] = (unsigned char)v;
            bh = (bh + v) & 0xFFUL;
        }
        if (bh == 0) {
            unsigned long dst = edi, ebx = eax;
            for (;;) {
                unsigned long src;
                ebx += 0x8000UL;
                if (ebx >= DEC4C_O4_N)
                    break;
                src = dec4c_o4s(ebx);
                if (c->o4[src * 32UL] == 0)
                    break;
                memcpy(c->o4 + dst * 32UL, c->o4 + src * 32UL, 32UL);
                dst = src;
            }
            c->o4[dst * 32UL] = (unsigned char)(c->o4[dst * 32UL] & 0x00U);
            continue;
        }
        b[0] = (unsigned char)(((unsigned)b[0] >> 2) + 1U);
        b[0x19] = (unsigned char)(((unsigned)b[0x19] & 0x3FUL) >> 1);
        {
            unsigned v = b[0x1A] & 0x0FUL;
            b[0x1A] = (unsigned char)v;
            b[0x1B] = 0;
            if (b[1 + v] == 0)
                b[0x1A] = 0;
        }
        eax += 0x8000UL;
        if (eax >= DEC4C_O4_N) {
            eax = (eax & 0x7FFFUL) + 1UL;
            if (eax >= 0x8000UL)
                return;
        }
    }
}

/* Order-2 record access (0x423684, 20 bytes = 5 u32 LE). */
#define DEC4C_O2_LOW 0x10000UL
static void dec4c_o2r(dec4c_ctx *c, unsigned long i, unsigned char r[20])
{
    unsigned k;
    if (i < DEC4C_O2_LOW) {
        for (k = 0; k < 5; k++) {
            unsigned long v = c->o2[i * 5UL + k];
            r[k * 4UL + 0] = (unsigned char)(v & 0xFFUL);
            r[k * 4UL + 1] = (unsigned char)((v >> 8) & 0xFFUL);
            r[k * 4UL + 2] = (unsigned char)((v >> 16) & 0xFFUL);
            r[k * 4UL + 3] = (unsigned char)((v >> 24) & 0xFFUL);
        }
    } else {
        unsigned long d = ((i - DEC4C_O2_LOW) * 5UL) & 0x7FFFFUL;
        for (k = 0; k < 5; k++) {
            r[k * 4UL + 0] = c->delta[d + k][0];
            r[k * 4UL + 1] = c->delta[d + k][1];
            r[k * 4UL + 2] = c->delta[d + k][2];
            r[k * 4UL + 3] = c->delta[d + k][3];
        }
    }
}

static void dec4c_o2_put(dec4c_ctx *c, unsigned long i, const unsigned char r[20])
{
    unsigned k;
    if (i < DEC4C_O2_LOW) {
        for (k = 0; k < 5; k++)
            c->o2[i * 5UL + k] = ((unsigned long)r[k * 4UL]) |
                ((unsigned long)r[k * 4UL + 1] << 8) |
                ((unsigned long)r[k * 4UL + 2] << 16) |
                ((unsigned long)r[k * 4UL + 3] << 24);
    } else {
        unsigned long d = ((i - DEC4C_O2_LOW) * 5UL) & 0x7FFFFUL;
        for (k = 0; k < 5; k++) {
            c->delta[d + k][0] = r[k * 4UL + 0];
            c->delta[d + k][1] = r[k * 4UL + 1];
            c->delta[d + k][2] = r[k * 4UL + 2];
            c->delta[d + k][3] = r[k * 4UL + 3];
        }
    }
}

/* 0x40d58d: halve (>>2) order-2 statistics. */
static void dec4c_rescale_o2(dec4c_ctx *c)
{
    unsigned long n = 0x20000UL >> (unsigned)c->binmode;
    unsigned long i;
    for (i = 0; i < n; i++) {
        unsigned char r[20];
        unsigned bh = 0;
        unsigned d, v;
        int k;
        dec4c_o2r(c, i, r);
        if (r[0] == 0)
            continue;
        for (k = 7; k >= 0; k--) {
            v = (unsigned)(r[1 + k] >> 2);
            r[1 + k] = (unsigned char)v;
            bh = (bh + v) & 0xFFUL;
        }
        d = (bh != 0) ? ((((unsigned)r[0] >> 2) + 1U) & 0xFFUL) : 0;
        r[0] = (unsigned char)d;
        r[0x11] >>= 1;
        v = r[0x13] & 0x0FUL;
        r[0x13] = (unsigned char)v;
        r[0x12] = (unsigned char)((bh + d) & 0xFFUL);
        if (r[1 + v] == 0)
            r[0x13] = 0;
        dec4c_o2_put(c, i, r);
    }
}

/* 0x40deb6: segment start. */
static void dec4c_new_segment(dec4c_ctx *c)
{
    unsigned bm = c->binmode;
    unsigned c_ctr;
    unsigned long next;
    c_ctr = 0;
    for (c_ctr = 0; c_ctr < 0x209UL; c_ctr++) {
        c->ba3c[c_ctr] = bm;
        c->ae38[c_ctr] = 0x4000UL;
    }
    for (c_ctr = 0; c_ctr < 0x208UL; c_ctr++) {
        c->mru_s[c_ctr] = 0xFFU;
        c->mru_n[c_ctr] = 0;
    }
    if (bm != 0) {
        unsigned i;
        for (i = 0; i < 0x80UL; i++) c->pair[i] = 1;
        if (!c->delta)
            c->delta = (unsigned char (*)[4])calloc(0x100000UL, sizeof(*c->delta));
        if (c->delta) memset(c->delta, 0, 0x100000UL * sizeof(*c->delta));
    }
    /* order-1 tables: 0..0x208 zeroed, static fallbacks loaded */
    {
        unsigned k;
        unsigned short o0[256];
        for (k = 0; k < 256; k++)
            o0[k] = (unsigned short)dec4c_rom8(0x422ba0UL + 2UL * k) |
                    ((unsigned short)dec4c_rom8(0x422ba0UL + 2UL * k + 1UL) << 8);
        for (c_ctr = 0; c_ctr < 0x209UL; c_ctr++) {
            memset(c->o1t[c_ctr][0], 0, sizeof(c->o1t[c_ctr][0]));
            memset(c->o1t[c_ctr][1], 0, sizeof(c->o1t[c_ctr][1]));
        }
        for (c_ctr = 0x301UL; c_ctr < 0x30FUL; c_ctr++) {
            unsigned n, x;
            for (x = 0; x < 256; x++)
                c->o1t[c_ctr][0][x] = o0[x];
            n = (c_ctr == 0x30EUL) ? 255UL : 256UL;
            for (x = 0; x < n; x++)
                c->o1t[c_ctr][0][x] = (unsigned short)(c->o1t[c_ctr][0][x] << 3);
            for (x = 0; x < 256; x++)
                c->o1t[c_ctr][1][x] = 8U;
        }
    }
    for (c_ctr = 0; c_ctr < 0x209UL; c_ctr++)
        c->esc[c_ctr] = 1;
    for (c_ctr = 0; c_ctr < 0x148UL; c_ctr++)
        c->mflag[c_ctr] = (unsigned short)dec4c_rom8(0x42372cUL + 2UL * c_ctr) |
                          ((unsigned short)dec4c_rom8(0x42372cUL + 2UL * c_ctr + 1UL) << 8);
    for (c_ctr = 0x209UL; c_ctr < 0x301UL; c_ctr++)
        dec4c_o1_rescale(c, c_ctr);
    dec4c_rescale_o2(c);
    c->g45c6b0 = 0;
    c->g45c6ac = 0;
    c->g45c6a0 = 0;
    c->g45c6b4 = 0x1ffUL;
    c->g45c68c = 0x187UL; /* (0x45ccd4-0x45c6b8)/4 */
    c->g45c688 = 0;
    memset(c->g6b8, 0, 0x290UL * sizeof(c->g6b8[0]));
    memset(c->recent, 0, sizeof(c->recent));
    memset(c->lastpos, 0, sizeof(c->lastpos));
    memset(c->per, 0, sizeof(c->per));
    c->g45c66c = 0;
    c->g45c674 = 1;
    c->g45c64c = 1;
    c->g45c670 = 0;
    for (c_ctr = 0; c_ctr < 256UL; c_ctr++) c->ring[c_ctr] = 0x9240UL;
    c->g45c650 = 0xffUL;
    c->g45c644 = 0x91adc0UL;
    c->avg_a[0] = c->avg_a[1] = 0;
    c->avg_b[0] = c->avg_b[1] = 0;
    c->g45c640 = 0;
    c->g45c654 = 0;
    /* Advance segment end (255 KB text / 16 KB binary) + file walk. */
    if (c->fe_on) {
        unsigned long step = 0x40000UL >> (2UL * bm);
        unsigned long end = c->seg_end + step;
        unsigned edx;
        if (end > c->out_len) end = c->out_len;
        c->seg_end = end;
        edx = (unsigned)c->file_idx;
        for (;;) {
            unsigned long e = dec4c_fe_get(c, edx);
            if (e >= c->seg_end) break;
            edx = (edx + 1UL) & 0xFFFFFFFFUL;
            {
                unsigned long e2 = dec4c_fe_get(c, edx);
                c->file_idx = (c->file_idx + 1UL) & 0xFFFFFFFFUL;
                if (e2 >= c->text_end || e2 >= c->g45d124 || c->bad) break;
            }
        }
        next = dec4c_fe_get(c, (unsigned)c->file_idx);
    } else {
        next = c->out_len;
    }
    c->seg_end = next;
    {
        unsigned long fi = c->file_idx;
        if (fi != 0) {
            unsigned long prev = dec4c_fe_get(c, (unsigned)(fi - 1UL));
            unsigned long ecx = (next - prev) & 0xFFFFFFFFUL;
            unsigned long edx = prev;
            unsigned long esi;
            if (fi > 1UL)
                edx = (prev - dec4c_fe_get(c, (unsigned)(fi - 2UL))) & 0xFFFFFFFFUL;
            esi = c->out_pos;
            if (edx == 0x8f4d0UL && ecx == 0x2d9a22UL) {
                dec4c_rescale_o4(c);
                dec4c_rescale_o4(c);
            }
            if (edx == 0x75a45UL && ecx == 0x68302UL)
                dec4c_rescale_o4(c);
            if ((edx == 0x17e5f8UL && ecx == 0x2cc640UL) ||
                (edx == 0x2cc640UL && ecx == 0x6af80UL)) {
                dec4c_rescale_o4(c);
                dec4c_rescale_o2(c);
            }
            if ((edx == 0x388dc2UL && ecx == 0x16dd2eUL) ||
                (edx == 0x7d4c0UL && ecx == 0x3c41eUL) ||
                (edx != 0xfb670UL && ecx == 0xfb670UL && esi >= c->g45d124)) {
                /* First 1 MB of order-4 / 1.25 MB of order-2 cleared
                 * (full o4/o2 tables deferred to the wiring stage). */
                if (esi >= c->g45d124)
                    c->g_e395 = 0x0AU;
            }
        }
    }
    {
        unsigned long v = dec4c_rc_div(c, 10UL);
        unsigned long t = v & 0xFFUL;
        dec4c_rc_dec(c, t, t + 1UL);
        dec4c_set_mmtype(c, (unsigned char)v);
    }
}

int uharc_dec4c_decode_b(void *ctx, const unsigned long *cum, unsigned long ncum,
                         uharc_dec_read_cb rcb, void *rctx,
                         uharc_dec_write_cb wcb, void *wctx,
                         unsigned long out_len)
{
    dec4c_ctx *c = (dec4c_ctx *)ctx;
    unsigned char tmp[4096];
    unsigned long got, want, i;
    /* Drain the packed stream into memory (decoder reads it whole). */
    unsigned long cap, len;
    if (!c || !rcb || !wcb)
        return 1;
    if (out_len == 0)
        return 0;
    /* File-boundary scan for kennedy (flags2 bit6): first file with
     * size 0xFB670 sets kennedy_end = predecessor end (0 if first). */
    if ((g_c_flags2 & 0x40) && cum && ncum) {
        unsigned long prev = 0;
        for (i = 0; i < ncum; i++) {
            unsigned e = (unsigned)cum[i];
            if (e - prev == 0xFB670U) break;
            prev = e;
        }
        c->kennedy_end = prev;
        c->g_e395 = (prev == 0) ? 10 : 0;
    }
    c->out_pos = 0;
    c->out_len = out_len;
    cap = out_len + out_len / 2UL + 8192UL;
    if (cap < 65536UL) cap = 65536UL;
    c->inbuf = (unsigned char *)malloc((size_t)cap);
    if (!c->inbuf) return 2;
    len = 0;
    for (;;) {
        want = cap - len;
        if (want > sizeof(tmp)) want = sizeof(tmp);
        if (want == 0) break;
        got = (unsigned)rcb(rctx, tmp, (unsigned)want);
        if (got == 0) break;
        if (len + got > cap) {
            unsigned char *nb;
            nb = (unsigned char *)realloc(c->inbuf, (size_t)(len + got + 8192UL));
            if (!nb) return 2;
            c->inbuf = nb;
            cap = len + got + 8192UL;
        }
        memcpy(c->inbuf + len, tmp, got);
        len += got;
        if (len >= c->out_len * 2UL + 4096UL) break;
    }
    c->in_len = len;
    c->in_pos = 0;
    dec4c_init_model(c);
    dec4c_rc_init(c);
    dec4c_read_switch_pos(c);
    /* TODO: main loop 0x40f962 (segments, match, order-4/2/1/0,
     * updates, text transform). Until ported, report data error so
     * the dispatcher can fall back to the arena decoder. */
    return 3;
}
