/* UHARC 0.6 core 4 (PPM) - clean typed implementation (C89).
 * See uharc_dec4_clean.h for status. Port of uharc-rs ppm.rs.
 */
#include <stdlib.h>
#include <string.h>
#include "uharc_dec4_clean.h"
#include "dec4c_rom.h"
#include "dec4c_rom2.h"

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
    unsigned char g_e38b;       /* 0x45e38b predicted byte */
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
    unsigned long g45c650, g45c644, g45c640, g45c654, g45c648, g45c690;
    unsigned long seg_end, text_end, file_idx;
    unsigned long g45d104, g45d120, g45d124, g45d130, g45d134, g45d138;
    unsigned long nbytes, c0, c1, c2;  /* 0x45d0f8/fc/100 last symbols */
    unsigned long pend, mchk_p, pair_p; /* 0x45d11c/148/14c, 0xFFFFFFFF=none */
    unsigned long *mh1, *mh2;           /* 0x423688/8c hashes, 0x80000 each */
    unsigned char *mlen, *mchk, *x698; /* 0x423690/94/98, 0x40000 each */
    unsigned long x698p[4];             /* 0x45d150.. */
    unsigned char dd6a[2], dd6c[2];     /* 0x45dd6a/c */
    unsigned char ty3[8], cnt3[8];      /* 0x45d160/0x44316c */
    unsigned char g_e389, g_e38a, g_e38f;
    unsigned long g45ae18;
    unsigned char g_e3a0, g_e3a2;       /* 0x45e3a0/a2 */
    unsigned long g45d114;              /* 0x45d114 match continuation */
    /* decode_symbol locals (0x41030f) */
    unsigned long g45c6a4, g45c694, g45c69c, g45c678, g45c698, g45c6a8;
    unsigned char g_e394, g_e397, g_e398, g_e39d, g_e39a, g_e399, g_e39f, g_e393;
    unsigned long g45c658, g45c668;
    unsigned char g_e3a1, g_e3a4, g_e3a5;
    unsigned char g_e3a3;
    unsigned char bad;
    unsigned char g_e39b, g_e39e;      /* 0x45e39b/e9e text states */
    unsigned char *outbuf;              /* decoded bytes (out_len) */
    unsigned char excl[256];            /* 0x45dd6e excluded symbols */
    unsigned char nex;                  /* 0x45e39c */
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
    /* dict = 1 << (((m-24) % 16) + 10), as in 0.6 dec4; +0x100 mirror */
    c->dictsize = 1UL << (unsigned)(((method - 24) % 16) + 10);
    c->dictmask = c->dictsize - 1UL;
    c->dict = (unsigned char *)calloc(1, (size_t)(c->dictsize + 0x100UL));
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
    free(c->outbuf);
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
    if (i < c->dictsize + 0x100UL && c->dict)
        return c->dict[i];
    return 0;
}

static void dec4c_push_hist(dec4c_ctx *c, unsigned char b)
{
    unsigned long pos = c->pos;
    c->dict[pos] = b;
    if (pos < 0x100UL)
        c->dict[pos + c->dictsize] = b;
    c->pos = (pos + 1UL) & c->dictmask;
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

/* Candidate search part of 0x40f9d0 (order-12 + last-distance pre-check).
 * Returns best length and sets *out_edi (candidate position). */
static unsigned dec4c_match_candidates(dec4c_ctx *c, unsigned long *out_edi)
{
    unsigned long c0 = c->c0, c1 = c->c1, c2 = c->c2;
    unsigned long mask = c->dictmask;
    unsigned long pos = c->pos;
    unsigned long h2;
    unsigned best = 0, edi = 0;
    int slot;
    unsigned long cur = (pos < 0x40UL) ? pos + c->dictsize : pos;
    h2 = ((((c1 >> 15) ^ c1) << 6) ^ (((c0 >> 15) ^ c0) << 4) ^ ((c2 >> 15) ^ c2)) & 0x3FFFFUL;
    {
        static const unsigned slots[2] = {1U, 0U};
        for (slot = 0; slot < 2; slot++) {
            unsigned long raw = c->mh2[2UL * h2 + slots[slot]];
            unsigned long ebx = raw;
            if (ebx != 0xFFFFFFFFUL && ((ebx - pos) & mask) >= 0x140UL) {
                if (ebx < 0x40UL)
                    ebx = (ebx + c->dictsize) & 0xFFFFFFFFUL;
                if (dec4c_tb(c, (cur - 0xCUL) & 0xFFFFFFFFUL) == dec4c_tb(c, (ebx - 0xCUL) & 0xFFFFFFFFUL)) {
                    unsigned len = dec4c_match_len(c, (cur - 1UL) & 0xFFFFFFFFUL, (ebx - 1UL) & 0xFFFFFFFFUL);
                    if (len >= best) {
                        best = len;
                        edi = raw;
                    }
                }
            }
        }
    }
    c->mh2[2UL * h2 + 1UL] = c->mh2[2UL * h2];
    c->mh2[2UL * h2] = pos;
    *out_edi = edi;
    return best;
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

static void dec4c_put_byte(dec4c_ctx *c, unsigned char b)
{
    if (c->outbuf && c->out_pos < c->out_len)
        c->outbuf[c->out_pos] = b;
    c->out_pos = (c->out_pos + 1UL) & 0xFFFFFFFFUL;
}

/* 0x40fffd: text transform (symbols are complemented bytes). */
static void dec4c_output(dec4c_ctx *c, unsigned char al)
{
    unsigned dl = (unsigned char)(~al);
    unsigned dh;
    unsigned long src;
    unsigned bl;
    if (c->g_e39b == 1) {
        c->g_e39b = 0;
        if (c->out_pos >= c->g45d124) {
            if (dl == 0xFEUL) {
                dec4c_put_byte(c, 0xFEU);
                return;
            }
            dec4c_put_byte(c, 0);
            dec4c_put_byte(c, 0);
            dec4c_put_byte(c, 3);
            dec4c_put_byte(c, 3);
            dec4c_put_byte(c, (unsigned char)dl);
            return;
        }
        {
            unsigned long i = (unsigned long)((unsigned char)(c->g_e39e - 0xA6U) * 0x0BUL);
            dec4c_put_byte(c, dec4c_rom8(0x427179UL + i + dl));
            return;
        }
    }
    if (c->out_pos >= c->g45d124 && dl == 0xFEUL) {
        c->g_e39b = 1;
        return;
    }
    if (c->out_pos >= c->sw_pos || (dl < 0x80UL && c->g_e39b == 0)) {
        dec4c_put_byte(c, (unsigned char)dl);
        return;
    }
    dh = dec4c_rom8(0x42711fUL + dl);
    if (dh >= 0xC8UL) {
        dec4c_put_byte(c, (unsigned char)(dl - c->g_e39b));
        c->g_e39b = 0;
        return;
    }
    if (dh == 0x64UL) {
        c->g_e39b = 1;
        c->g_e39e = (unsigned char)dl;
        return;
    }
    if (dh == 0x63UL) {
        c->g_e39b = 0x20U;
        return;
    }
    src = dec4c_rom32(0x426ac3UL + 4UL * dh);
    bl = dec4c_rom8(0x426c53UL + dh);
    if (c->g_e39b != 0) {
        unsigned a = (unsigned char)(~dec4c_rom8(src));
        a = (a - 0x20UL) & 0xFFUL;
        bl = (bl - 1UL) & 0xFFUL;
        src = (src + 1UL) & 0xFFFFFFFFUL;
        dec4c_put_byte(c, (unsigned char)a);
        c->g_e39b = 0;
    }
    for (;;) {
        unsigned a = (unsigned char)(~dec4c_rom8(src));
        src = (src + 1UL) & 0xFFFFFFFFUL;
        dec4c_put_byte(c, (unsigned char)a);
        bl = (bl - 1UL) & 0xFFUL;
        if (bl == 0)
            break;
    }
}

/* mm_predict / mm_update stubs (0x412ffa/0x413347) for types 5..9.
 * Correct for generic types 0..4; multimedia will be wired when the
 * predictor records are ported. */
static void dec4c_mm_predict(dec4c_ctx *c) { (void)c; }
static void dec4c_mm_update(dec4c_ctx *c, unsigned char s, unsigned char d) { (void)c; (void)s; (void)d; }

/* 0x40f39a: per-symbol context/period/predictor bookkeeping. */
static void dec4c_after_symbol(dec4c_ctx *c, unsigned char sym, int dl)
{
    int i;
    unsigned cl, o;
    for (i = 0; i < 4; i++)
        c->x698[c->x698p[i] & 0x3FFFFUL] = sym;
    if (dl) {
        cl = (unsigned)(c->nbytes & 1UL);
        o = cl ^ 1UL;
        c->dd6a[cl] = (unsigned char)((c->dd6a[cl] + c->dd6a[cl]) & 0x0FUL);
        c->dd6c[cl] = (unsigned char)((c->dd6c[cl] + c->dd6c[cl]) & 0x0FUL);
        c->g_e3a0 = 0;
        if (sym >= 0x80U) {
            if (sym == 0xFFU) {
                c->dd6c[cl] = (unsigned char)(c->dd6c[cl] + 1U);
                if (c->dd6a[o] == 0x0FU && c->dd6c[cl] == 0x0FU)
                    c->g_e3a0 = 2;
            } else if (sym <= 0xDFU || (sym >= 0xF2U && sym <= 0xF6U)) {
                c->dd6a[cl] = (unsigned char)(c->dd6a[cl] + 1U);
                if (c->dd6c[o] == 0x0FU && c->dd6a[cl] == 0x0FU)
                    c->g_e3a0 = 1;
            }
        }
    }
    if (sym != 0 && sym != 0xFFU) {
        unsigned long pos = c->pos;
        unsigned long eax = (pos - c->lastpos[sym]) & c->dictmask;
        if (eax >= 0x100UL) {
            eax = (pos - c->lastpos[sym - 1U]) & c->dictmask;
            if (eax >= 0x100UL)
                eax = (pos - c->lastpos[sym + 1U]) & c->dictmask;
        }
        if (eax != 0 && eax < 0x100UL) {
            c->per[eax] = (c->per[eax] + 1UL) & 0xFFFFFFFFUL;
            if (eax != c->g45c674) {
                unsigned long pd = c->g45c674;
                if (sym == dec4c_tb(c, pd))
                    c->per[pd & 0xFFUL] = (c->per[pd & 0xFFUL] + 1UL) & 0xFFFFFFFFUL;
                else
                    c->g45c66c = (c->g45c66c + 2UL) & 0xFFFFFFFFUL;
                if (eax == c->g45c670) {
                    unsigned long edx = c->g45c674;
                    if (c->per[eax] >= c->per[edx & 0xFFUL]) {
                        c->g45c670 = edx;
                        c->g45c674 = eax;
                        if (eax > 1UL)
                            c->g45c64c = eax;
                    }
                } else {
                    c->g45c670 = eax;
                }
            }
        }
        c->g45c66c = (c->g45c66c + 1UL) & 0xFFFFFFFFUL;
        if (c->g45c66c >= 0x28UL) {
            unsigned e;
            for (i = 0; i < 256; i++)
                c->per[i] >>= 1;
            e = c->g45c674;
            c->g45c66c = 0;
            c->per[e & 0xFFUL] = (c->per[e & 0xFFUL] + 1UL) & 0xFFFFFFFFUL;
            if (e == 1UL && c->per[1] < 4UL)
                c->per[1] = (c->per[1] + e) & 0xFFFFFFFFUL;
        }
        c->lastpos[sym] = pos;
    }
    if (c->binmode != 0)
        c->recent[sym] = (c->pos + 0x40UL) & 0xFFFFFFFFUL;
    {
        unsigned ty = c->mmtype;
        if (ty != 0) {
            unsigned al = (unsigned)(sym - c->g_e38b) & 0xFFUL;
            if (c->g_e38e != 0)
                al = (-(long)al) & 0xFFUL;
            if (al >= 0x80UL)
                c->g_e38e = (c->g_e38e == 0) ? 1 : 0;
            if (ty == 1) {
                unsigned e = (c->mmctx - 0x209UL) & 0x0FUL;
                c->mmctx = ((dec4c_rom8(0x424b2cUL + e) << 4) + dec4c_rom8(0x424b3cUL + al) + 0x209UL) & 0xFFFFFFFFUL;
            } else {
                unsigned edx = (c->ch << 4) & 0xFFFFFFFFUL;
                if (ty == 2) {
                    c->mmctx = (dec4c_rom8(0x424b3cUL + al) + edx + 0x229UL) & 0xFFFFFFFFUL;
                } else if (ty == 3) {
                    unsigned ecx;
                    if (c->g_e389 != 0) {
                        unsigned d = (sym - (c->c0 >> 16)) & 0xFFFFFFFFUL;
                        al = d & 0xFFUL;
                        {
                            unsigned ah = c->g_e38f;
                            c->g_e38e = (unsigned char)ah;
                            if (ah != 0)
                                al = (-(long)al) & 0xFFUL;
                            if (al >= 0x80UL)
                                c->g_e38e = (c->g_e38e == 0) ? 1 : 0;
                        }
                    }
                    ecx = c->g_e38a & 7UL;
                    if (sym == c->ty3[ecx])
                        c->cnt3[ecx] = (c->cnt3[ecx] + 1UL) & 0xFFFFFFFFUL;
                    else
                        c->cnt3[ecx] >>= 1;
                    {
                        unsigned d = dec4c_rom8(0x424c3cUL + al);
                        c->mmctx = (((c->ch << 5) & 0xFFFFFFFFUL) + d + 0x249UL) & 0xFFFFFFFFUL;
                    }
                } else if (ty == 4) {
                    c->mmctx = (dec4c_rom8(0x424b3cUL + al) + edx + 0x2a9UL) & 0xFFFFFFFFUL;
                } else {
                    dec4c_mm_update(c, sym, (unsigned char)al);
                }
            }
        }
    }
    c->nbytes = (c->nbytes + 1UL) & 0xFFFFFFFFUL;
    c->g45c6b0 = (c->g45c6b0 + 1UL) & 0xFFFFFFFFUL;
    {
        unsigned long c0 = c->c0, c1 = c->c1, c2 = c->c2;
        if (c->binmode == 0 && sym >= 4U && sym <= 6U) {
            if ((c0 & 0xFFUL) != 0xDFUL) {
                c->c2 = ((c2 << 8) + (c1 >> 24)) & 0xFFFFFFFFUL;
                c->c1 = ((c1 << 8) + (c0 >> 24)) & 0xFFFFFFFFUL;
                c->c0 = ((c0 << 8) + 0xDFUL) & 0xFFFFFFFFUL;
            }
        } else {
            c->c2 = ((c2 << 8) + (c1 >> 24)) & 0xFFFFFFFFUL;
            c->c1 = ((c1 << 8) + (c0 >> 24)) & 0xFFFFFFFFUL;
            c->c0 = ((c0 << 8) + (unsigned)sym) & 0xFFFFFFFFUL;
        }
    }
    if (c->mmtype != 0) {
        unsigned long n = c->nbytes;
        unsigned long d = c->mmper;
        unsigned long rem;
        if (d == 0) {
            c->bad = 1;
            rem = 0;
        } else {
            rem = n % d;
        }
        {
            unsigned cl = c->g_e38c;
            unsigned long esi = c->ch;
            c->ch = rem;
            c->g_e38b = (unsigned char)((c->c0 >> cl) & 0xFFUL);
            if (c->mmtype == 3) {
                unsigned dl = (unsigned)(n % 6UL);
                unsigned long c0 = c->c0;
                unsigned ecx, big;
                c->g_e38a = (unsigned char)dl;
                ecx = ((c0 & 0xFFUL) ^ (c0 >> 24) ^ c->g_e38b) & 0xFFFFFFFFUL;
                big = (c->cnt3[dl] > 8UL) ? 1U : 0U;
                c->ty3[dl] = (unsigned char)ecx;
                {
                    unsigned al = (big && c->g45ae18 < 5UL) ? 1U : 0U;
                    c->g_e389 = (unsigned char)al;
                    if (al) {
                        c->g_e38f = c->g_e38e;
                        c->mmctx = ((esi << 5) + 0x259UL) & 0xFFFFFFFFUL;
                        c->g_e38e = 0;
                        c->g_e38b = c->ty3[dl];
                    }
                }
                if (big)
                    c->g45ae18 = (c->g45ae18 + 1UL) & 0xFFFFFFFFUL;
                else
                    c->g45ae18 = 0;
            } else if (c->mmtype >= 5U) {
                dec4c_mm_predict(c);
            }
        }
    }
    {
        unsigned long c0 = c->c0, c1 = c->c1;
        unsigned long edx = c0 >> 8;
        c->x698p[0] = (((c0 >> 16) & 0xFF00UL) + (c1 >> 24)) & 0xFFFFFFFFUL;
        c->x698p[1] = ((edx & 0xFFFFUL) + 0x10000UL) & 0xFFFFFFFFUL;
        c->x698p[2] = ((edx & 0xFF00UL) + 0x20000UL + (c0 & 0xFFUL)) & 0xFFFFFFFFUL;
        c->x698p[3] = (((c0 << 8) & 0xFF00UL) + 0x30000UL + (c1 & 0xFFUL)) & 0xFFFFFFFFUL;
    }
}

static unsigned long dec4c_dv(unsigned long a, unsigned long b)
{
    if (b == 0) return 0;
    return a / b;
}

/* 0x40f9d0: match model; returns true when bytes were copied. */
static int dec4c_match_model(dec4c_ctx *c)
{
    unsigned long c0 = c->c0, c1 = c->c1, c2 = c->c2;
    unsigned long mask = c->dictmask;
    unsigned long pos = c->pos;
    unsigned long h0 = ((c0 >> 13) ^ c0) & 0x3FFFFUL;
    unsigned long p1 = 2UL * h0;
    unsigned long h2 = ((((c1 >> 15) ^ c1) << 6) ^ (((c0 >> 15) ^ c0) << 4) ^ ((c2 >> 15) ^ c2)) & 0x3FFFFUL;
    unsigned ctx = 0;
    int flag = 0;
    unsigned best = 0;
    unsigned chk = c->mchk[h0];
    unsigned long edi = 0;
    unsigned long cur = (pos < 0x40UL) ? pos + c->dictsize : pos;
    int goto_fca4;
    {
        static const unsigned slots[2] = {1UL, 0UL};
        int slot;
        for (slot = 0; slot < 2; slot++) {
            unsigned long raw = c->mh2[2UL * h2 + slots[slot]];
            unsigned long ebx = raw;
            if (ebx != 0xFFFFFFFFUL && ((ebx - pos) & mask) >= 0x140UL) {
                if (ebx < 0x40UL)
                    ebx = (ebx + c->dictsize) & 0xFFFFFFFFUL;
                if (dec4c_tb(c, (cur - 0xCUL) & 0xFFFFFFFFUL) == dec4c_tb(c, (ebx - 0xCUL) & 0xFFFFFFFFUL)) {
                    unsigned len = dec4c_match_len(c, (cur - 1UL) & 0xFFFFFFFFUL, (ebx - 1UL) & 0xFFFFFFFFUL);
                    if (len >= best) {
                        best = len;
                        edi = raw;
                    }
                }
            }
        }
    }
    c->mh2[2UL * h2 + 1UL] = c->mh2[2UL * h2];
    c->mh2[2UL * h2] = pos;
    c->mchk_p = h0;
    if (c->g45d114 != 0UL) {
        edi = (pos - c->g45d130) & mask;
        ctx = 0x50U + ((c->g45d114 > 2UL) ? 1U : 0U);
        flag = 1;
        goto_fca4 = 1;
    } else {
        unsigned thr = (c->g45d134 < 0x10UL) ? 0x30U : 0x0CU;
        if ((long)best >= (long)thr) {
            ctx = 0x15U;
            goto_fca4 = 1;
        } else {
            goto_fca4 = 0;
        }
    }
    if (!goto_fca4) {
        int skip = 0;
        if (ctx == 0) {
            edi = (pos - c->g45d130) & mask;
            if (((edi - pos) & mask) >= 0x104UL) {
                unsigned long a = (edi - 4UL) & mask;
                unsigned long v = (unsigned long)dec4c_tb(c, (a + 3UL) & 0xFFFFFFFFUL) |
                    ((unsigned long)dec4c_tb(c, (a + 2UL) & 0xFFFFFFFFUL) << 8) |
                    ((unsigned long)dec4c_tb(c, (a + 1UL) & 0xFFFFFFFFUL) << 16) |
                    ((unsigned long)dec4c_tb(c, a) << 24);
                if ((v & 0xFFFFUL) == (c0 & 0xFFFFUL) ||
                    (v & 0xFFFF00FFUL) == (c0 & 0xFFFF00FFUL) ||
                    (v & 0xFFFFFF00UL) == (c0 & 0xFFFFFF00UL)) {
                    ctx = 0x0BU;
                    flag = 1;
                    skip = 1;
                }
            }
        }
        if (!skip && ctx == 0) {
            int go = 1;
            if (chk >= 0x40U && c->mlen[h0] == 0)
                go = 0;
            if (go) {
                if (c->mh1[p1 + 1UL] == c0) {
                    edi = c->mh1[p1];
                    ctx = (edi != 0xFFFFFFFFUL) ? 1U : 0U;
                } else if (edi != 0xFFFFFFFFUL) {
                    ctx = 0x1FU;
                }
            }
        }
    }
    c->mh1[p1] = pos;
    c->mh1[p1 + 1UL] = c0;
    if (ctx == 0 || ((edi - pos) & mask) < 0x100UL) {
        c->g45d114 = 0;
        return 0;
    }
    {
        unsigned long mlen_a = h0;
        unsigned long dist, d138, node, tab, lb, sel;
        unsigned len;
        unsigned long rem;
        if (c->g45d114 == 0UL) {
            unsigned long add = (c->mlen[mlen_a] != 0) ? 0xFFFFFFFFUL : 0x27UL;
            unsigned long e = ((unsigned long)c->g_e3a2 + add) & 0xFFFFFFFFUL;
            ctx = ((unsigned)ctx + (unsigned)e) & 0xFFU;
        }
        if (c->pend != 0xFFFFFFFFUL) {
            if (dec4c_tb(c, c->pend) == dec4c_tb(c, edi)) {
                dec4c_match_flag_update(c, ctx, 2);
                c->mlen[mlen_a] = 0;
                c->g45d114 = 0;
                return 0;
            }
        }
        if (dec4c_match_flag(c, ctx) != 1U) {
            c->g45d134 = (c->g45d134 + 1UL) & 0xFFFFFFFFUL;
            c->mlen[mlen_a] = 0;
            c->g45d114 = 0;
            return 0;
        }
        node = 0x0EUL;
        dist = (pos - edi) & mask;
        d138 = c->g45d138;
        if (d138 == 0UL) {
            c->bad = 1;
            return 0;
        }
        if (flag) {
            unsigned long q = dec4c_dv((c->g45d120 << 2) & 0xFFFFFFFFUL, d138);
            lb = dec4c_rom8(0x424280UL + q);
        } else if (c->g45d104 > d138) {
            unsigned long q = dec4c_dv((c->g45d104 << 2) & 0xFFFFFFFFUL, d138);
            lb = dec4c_rom8(0x424270UL + q);
        } else {
            unsigned long q = dec4c_dv((c->g45d120 << 2) & 0xFFFFFFFFUL, d138);
            unsigned long v = (dec4c_rom8(0x424290UL + q) - c->binmode) & 0xFFUL;
            if (c->g_e3a0 != 0)
                v = (v << 1) & 0xFFUL;
            lb = v;
        }
        if (flag) {
            unsigned long a = c->g45d114;
            sel = ((a > 0UL) ? 1UL : 0UL) + 3UL + ((a > 2UL) ? 1UL : 0UL);
        } else {
            unsigned long b = c->mlen[mlen_a];
            sel = ((b > 0x0EUL) ? 1UL : 0UL) + ((b > 0x3EUL) ? 1UL : 0UL);
        }
        tab = D4C_ML + (sel << 8);
        for (;;) {
            unsigned long pa, p;
            len = ((unsigned char)node + 1U) & 0xFFU;
            if (lb < len) {
                unsigned long v = dec4c_rc_div(c, 0x1000UL) & 0xFFFFUL;
                pa = (tab + node) & (DEC4C_SEE_LEN - 1UL);
                p = c->see[pa];
                if (v < p) {
                    unsigned long lo = 0, hi = p;
                    len = (unsigned char)node;
                    c->see[pa] = (p + ((0x1000UL - p) >> 4)) & 0xFFFFFFFFUL;
                    dec4c_rc_dec(c, lo, hi);
                } else {
                    unsigned long lo = p, hi = 0x1000UL;
                    c->see[pa] = (p - (p >> 4)) & 0xFFFFFFFFUL;
                    dec4c_rc_dec(c, lo, hi);
                }
            }
            {
                unsigned right = (len > node) ? 1UL : 0UL;
                node = dec4c_rom8(0x4242a0UL + ((2UL * node + right) & 0x1FFUL));
                if (node == 0)
                    break;
            }
        }
        if (dist == c->g45d130) {
            c->g45d134 >>= 1;
        } else {
            c->g45d130 = dist;
            c->g45d134 = (c->g45d134 + len) & 0xFFFFFFFFUL;
        }
        c->mlen[mlen_a] = (unsigned char)len;
        if (len != 0xFFU) {
            c->g45d114 = 0;
            c->pend = ((unsigned long)len + edi) & mask;
        } else {
            c->pend = 0xFFFFFFFFUL;
            c->g45d114 = (c->g45d114 + 1UL) & 0xFFFFFFFFUL;
        }
        rem = len;
        for (;;) {
            unsigned char b = dec4c_tb(c, edi);
            dec4c_output(c, b);
            dec4c_after_symbol(c, b, (rem <= 8UL) ? 1 : 0);
            dec4c_push_hist(c, b);
            edi = (edi + 1UL) & mask;
            rem = (rem - 1UL) & 0xFFFFFFFFUL;
            if (rem == 0)
                break;
        }
        return 1;
    }
}

/* Order-4 helpers (lit.rs): exclusion list, SEE bumps, recent/marked. */
static void dec4c_excl_push(dec4c_ctx *c, unsigned char s)
{
    unsigned n = c->nex;
    if (n < 256U) {
        c->excl[n] = s;
        c->nex = (unsigned char)(n + 1U);
    }
}

static int dec4c_recent(dec4c_ctx *c, unsigned char s)
{
    return (c->pos <= c->recent[s]) ? 1 : 0;
}

static int dec4c_marked(dec4c_ctx *c, unsigned char s)
{
    return (c->see[D4C_MARKS + s] == c->nbytes) ? 1 : 0;
}

static void dec4c_see_bump(dec4c_ctx *c, unsigned i, int up)
{
    unsigned long *p = &c->see[i & (DEC4C_SEE_LEN - 1UL)];
    if (up)
        *p = (*p + ((0x2000UL - *p) >> 6)) & 0xFFFFFFFFUL;
    else
        *p = (*p - (*p >> 6)) & 0xFFFFFFFFUL;
}

static unsigned long dec4c_g68c(dec4c_ctx *c)
{
    return c->g6b8[c->g45c68c & 0x3FFUL];
}

/* decode_symbol preamble (0x41030f up to order-4 probe): context
 * setup + order-2 record load. Returns L10c in *out and l50 in *out50. */
static void dec4c_decode_symbol_pre(dec4c_ctx *c, unsigned long *out_l10c,
                                    unsigned char *out_l50, unsigned char rec[20])
{
    unsigned bm = (c->binmode != 0) ? 1UL : 0UL;
    unsigned long c0 = c->c0;
    unsigned long c6a4, lfc, add, c69c, o2i;
    unsigned long l10c;
    int i;
    c6a4 = (c->g_e3a0 == 2) ?
        (((c0 >> 8) & 0xFFUL) + ((c0 >> 16) & 0xFF00UL) + ((c->c1 << 8) & 0xFF0000UL) + (c->c1 & 0xFF000000UL)) & 0xFFFFFFFFUL :
        c0;
    c->g45c6a4 = c6a4;
    lfc = ((c6a4 & 0xC0UL) == 0xC0UL) ? 0x10UL : 0UL;
    (void)lfc;
    c->g45c694 = ((c6a4 >> 13) ^ c6a4) & 0x7FFFUL;
    add = (!bm && dec4c_rom8(0x42738dUL + ((c6a4 >> 16) & 0xFFUL)) != 0) ? 0x10000UL : 0UL;
    c69c = (c6a4 & 0xFFFFUL) + add;
    c->g45c69c = c69c;
    c->g_e394 = 0;
    c->g_e397 = 0;
    c->g_e39a = 0;
    c->g_e399 = 0;
    c->g_e39f = 0;
    c->g_e39d = 0;
    c->g_e393 = 0;
    c->g_e398 = 0;
    c->nex = 0;
    c->g45c658 = 0;
    c->g45c668 = 0;
    c->g_e3a1 = 0;
    c->g_e3a4 = 0;
    o2i = c69c & 0x1FFFFUL;
    if (c->pend != 0xFFFFFFFFUL) {
        c->nex = 1;
        c->excl[0] = dec4c_tb(c, c->pend);
    }
    /* per-phase threshold slots (per_d4/per_e8 tables not yet typed;
     * G_CCD4 base only, matching new_segment). */
    {
        unsigned long c64c = c->g45c64c;
        if (c->g45c674 > 1UL || ((unsigned char)(c64c - 2UL) <= 2U)) {
            unsigned long edi = c64c, ecx = c64c;
            if (c64c <= 4UL)
                edi = dec4c_rom32(0x4236d4UL + 4UL * c64c) + (c64c ? (c->nbytes % c64c) : 0);
            if (c64c <= 0x10UL)
                ecx = dec4c_rom32(0x4236e8UL + 4UL * c64c) + (c64c ? (c->nbytes % c64c) : 0);
            c->g45c68c = 0x187UL + edi;
            c->g45c688 = ecx;
        }
    }
    l10c = c6a4 & 0xFFUL;
    if (!bm && dec4c_rom8(0x42738dUL + ((c6a4 >> 8) & 0xFFUL)) != 0)
        l10c += 0x100UL;
    dec4c_o2r(c, o2i, rec);
    *out_l10c = l10c;
    *out_l50 = c->mru_s[l10c & 0x3FFUL];
    /* MARKS update for the four order-2 contexts */
    for (i = 0; i < 4; i++)
        c->see[D4C_MARKS + c->x698[c->x698p[i] & 0x3FFFFUL]] = c->nbytes;
}

/* Order-4 probe scan (0x41030f head): walk the 8-way chain for bucket
 * o4s(g45c694); on hit copy the 32-byte bucket to b[32] and run the
 * skip tests. Returns 1 if the order should decode (b filled), 0 to
 * fall through to order-2. rec is the order-2 record (o2i). */
static int dec4c_o4_scan(dec4c_ctx *c, unsigned long c6a4, const unsigned char rec[20],
                         unsigned char b[32])
{
    unsigned long bm = (c->binmode != 0) ? 1UL : 0UL;
    unsigned long l50_dummy = 0;
    /* l52/l4e/g68c need lit state; for scan we evaluate skip tests that
     * only need b/rec/g45/g6b8. l52/l4e paths return "skip" conservatively. */
    for (;;) {
        unsigned long bucket = ((c->g45c694 & 0x7FFFUL) << 3) | ((c->g45c694 >> 15) & 7UL);
        unsigned char *bp = c->o4 + bucket * 32UL;
        unsigned long ctx32;
        unsigned f0 = 0x0CU, ec = 0x0CU;
        unsigned k = 0, fk, sum;
        unsigned long b1a, b19;
        if (bp[0] == 0) {
            c->g45c694 = (c->g45c694 + 0x40000UL) & 0xFFFFFFFFUL;
            return 0;
        }
        ctx32 = ((unsigned long)bp[28]) | ((unsigned long)bp[29] << 8) |
                ((unsigned long)bp[30] << 16) | ((unsigned long)bp[31] << 24);
        if (ctx32 != c6a4) {
            c->g45c694 = (c->g45c694 + 0x8000UL) & 0xFFFFFFFFUL;
            if (c->g45c694 >= 0x40000UL)
                return 0;
            continue;
        }
        memcpy(b, bp, 32UL);
        (void)l50_dummy;
        c->g_e3a3 = 0xFFU;
        sum = b[0];
        fk = b[1];
        k = 0;
        for (;;) {
            unsigned s = b[0x0DUL + k];
            if (s == 0) { /* l50/l3c handled by caller via f0/ec; keep scan */ }
            sum = (sum + fk) & 0xFFFFFFFFUL;
            k += 1UL;
            if (k >= 0x0CUL) break;
            fk = b[k + 1UL];
            if (fk == 0) break;
        }
        (void)f0; (void)ec;
        b1a = b[0x1A];
        c->g45c678 = sum;
        /* Skip: g45c6a0/g45c674/g68c threshold */
        if (c->g45c6a0 == 0 && c->g45c674 > 1UL &&
            c->g45d104 <= c->g45d138 && dec4c_g68c(c) > 0x3FEUL) {
            unsigned long q = b1a >> 4;
            unsigned long e = ((unsigned long)b[0x1B] << 6) * q & 0xFFFFFFFFUL;
            if (e < c->g6b8[c->g45c688 & 0x3FFUL])
                return 0;
        }
        /* Skip: order-2 vs order-4 cost comparison */
        if (rec[0] != 0 && (bm != 0 || rec[0x13] >= 0x30UL)) {
            b19 = b[0x19];
            if ((b19 & 0x38UL) == 0UL) {
                unsigned long e = (unsigned long)rec[0x12] * ((b19 & 4UL) + b[1] + (b[0x1A] >> 4)) & 0xFFFFFFFFUL;
                unsigned long a = ((unsigned long)rec[1] + ((unsigned long)rec[0x13] >> 4)) * c->g45c678 & 0xFFFFFFFFUL;
                if (e < a)
                    return 0;
            }
            if (rec[0x13] >= 0x60UL && ((unsigned long)rec[0x13] >> 2) > (unsigned long)b[0x1A])
                return 0;
        }
        /* Allowed-symbol mask + escape check */
        {
            unsigned long c678 = c->g45c678;
            unsigned long c698 = c678;
            unsigned mask[16];
            unsigned j;
            int mi;
            for (mi = 0; mi < 16; mi++) mask[mi] = (mi < 12) ? 1UL : 0UL;
            if (c->nex != 0) {
                unsigned x = c->excl[0];
                j = 0;
                for (;;) {
                    if (x == b[j + 0x0DUL]) {
                        c698 = (c698 - b[j + 1UL]) & 0xFFFFFFFFUL;
                        mask[j] = 0;
                        break;
                    }
                    j += 1UL;
                    if (b[j + 1UL] == 0 || j >= 0x0CUL) break;
                }
            }
            (void)mask;
            c->g45c698 = c698;
            if ((unsigned long)b[0] >= c698)
                return 0;
        }
        return 1;
    }
}

/* Order-4 single-symbol binary decision (b[2]==0 path of 0x41030f).
 * On esc==false sym is set; on esc==true s0 is excluded. */
static int dec4c_see_freq(dec4c_ctx *c, unsigned long i, unsigned long rest,
                            unsigned long f, unsigned long *out_q)
{
    unsigned long p = c->see[i & (DEC4C_SEE_LEN - 1UL)];
    unsigned long d = (0x2000UL - p) & 0xFFFFFFFFUL;
    unsigned long q;
    if (d == 0)
        return 0;
    q = (dec4c_dv((rest * p) & 0xFFFFFFFFUL, d) + 1UL) & 0xFFFFFFFFUL;
    if (q > f) {
        *out_q = (q > 0xFFUL) ? 0xFFUL : q;
        return 1;
    }
    return 0;
}

static void dec4c_o4_single(dec4c_ctx *c, const unsigned char b[32],
                            const unsigned char rec[20],
                            unsigned long c6a4, unsigned long l10c,
                            unsigned l3c, unsigned l50, int bm,
                            int *esc, unsigned char *sym, unsigned long *see_esc)
{
    unsigned char s0 = b[0x0D];
    unsigned long edx = b[1];
    unsigned long eax = 0;
    unsigned long pa, v, p;
    if ((unsigned)s0 == l3c) {
        edx = (edx + ((unsigned)rec[0x13] >> 4)) & 0xFFFFFFFFUL;
        eax = 1;
    }
    if ((unsigned)s0 == l50) {
        eax |= 1UL;
        edx = (edx + c->mru_n[l10c & 0x3FFUL]) & 0xFFFFFFFFUL;
    }
    if (rec[7] != 0 && c->g45c6b0 < 6UL)
        eax |= 0x10UL;
    if (bm) {
        if (dec4c_recent(c, s0))
            eax |= 0x100UL;
        if ((unsigned)s0 == (c6a4 & 0xFFUL))
            eax |= 2UL;
    } else if (dec4c_rom8(0x42738dUL + s0) != 0) {
        eax |= 2UL;
    }
    if (edx < 0x1EUL) {
        unsigned long esi_ = c6a4 & 0xE0UL;
        eax |= esi_;
        if (bm) {
            if (esi_ == ((c6a4 >> 8) & 0xE0UL))
                eax |= 8UL;
            if (esi_ == ((unsigned)s0 & 0xE0UL))
                eax |= 4UL;
        } else {
            eax |= ((unsigned long)dec4c_rom8(0x42738dUL + ((c6a4 >> 8) & 0xFFUL)) << 2) +
                   ((unsigned long)dec4c_rom8(0x42738dUL + (c6a4 & 0xFFUL)) << 3);
        }
    }
    if (dec4c_marked(c, s0))
        eax |= 0x200UL;
    pa = D4C_S_O4BIN + ((dec4c_rom32(0x423df0UL + 4UL * (edx & 0x1FFUL)) << 10) + eax);
    v = dec4c_rc_div(c, 0x2000UL) & 0xFFFFUL;
    p = c->see[pa & (DEC4C_SEE_LEN - 1UL)];
    if (v < p) {
        *esc = 0;
        c->g_e3a3 = 0;
        *sym = s0;
        dec4c_rc_dec(c, 0, p);
    } else {
        c->g_e3a3 = 0x0CU;
        dec4c_excl_push(c, s0);
        dec4c_rc_dec(c, p, 0x2000UL);
    }
    *see_esc = pa;
}

/* Order-4 escape total estimate (b[2]!=0 path, S_O4ESC part).
 * Returns t = dv(ecx<<13, see[pa])+1 (or c698 if ecx >= 0x80). */
static unsigned long dec4c_o4_escape(dec4c_ctx *c, const unsigned char b[32],
                                     const unsigned char rec[20],
                                     unsigned long c6a4, unsigned long c698,
                                     int bm, unsigned long *see_esc_out)
{
    unsigned long ecx = (c698 - (unsigned long)b[0]) & 0xFFFFFFFFUL;
    unsigned long t = c698;
    if (ecx < 0x80UL) {
        unsigned long edx, eax, idx, pa;
        if (bm) {
            unsigned long l2a = (c6a4 >> 8) & 0xE0UL;
            edx = (((c6a4 & 0xE0UL) == l2a) ? 1UL : 0UL);
            eax = (c6a4 & 0xC0UL) >> 5;
        } else {
            unsigned long l2a = (unsigned long)dec4c_rom8(0x42738dUL + ((c6a4 >> 8) & 0xFFUL)) * 2UL;
            edx = (unsigned long)dec4c_rom8(0x42738dUL + (c6a4 & 0xFFUL)) * 4UL + l2a;
            eax = ((c6a4 & 0xFFUL) == 0xDFUL) ? 1UL : 0UL;
        }
        idx = eax + edx + ((rec[7] != 0) ? 8UL : 0UL) + ((c->g45c6b0 >= 6UL) ? 16UL : 0UL);
        pa = D4C_S_O4ESC +
             ((dec4c_rom32(0x4239f0UL + 4UL * (unsigned)b[0]) << 8) +
              (dec4c_rom32(0x4239f0UL + 4UL * (ecx & 0xFFUL)) << 5) + idx);
        t = (dec4c_dv((ecx << 13) & 0xFFFFFFFFUL, c->see[pa & (DEC4C_SEE_LEN - 1UL)]) + 1UL) & 0xFFFFFFFFUL;
        *see_esc_out = pa;
    } else {
        *see_esc_out = 0;
    }
    return t;
}

/* Order-4 multi-symbol decode (b[2]!=0 path): SEE corrections + loop.
 * b is the local bucket copy (mutated); rec is the order-2 record.
 * On esc==false sym is set. */
static void dec4c_o4_multi(dec4c_ctx *c, unsigned char b[32],
                           const unsigned char rec[20],
                           unsigned long c6a4, unsigned long l10c,
                           unsigned l3c, unsigned l50, int bm,
                           unsigned long c698, unsigned long lfc,
                           int *esc, unsigned char *sym, unsigned long *see_esc)
{
    unsigned long ecx = (c698 - (unsigned long)b[0]) & 0xFFFFFFFFUL;
    unsigned long p42 = 0, p3a = 0, p2e = 0, pedi = 0;
    unsigned long k2a, t;
    unsigned long l104, l100, l108, l28, f0, ec;
    unsigned mask[16];
    unsigned long see_esc_local = 0;
    int mi;
    for (mi = 0; mi < 16; mi++) mask[mi] = (mi < 12) ? 1UL : 0UL;
    if (c->nex != 0) {
        unsigned x = c->excl[0];
        unsigned j;
        for (j = 0; j < 16UL; j++) {
            if (x == b[j + 0x0DUL]) {
                c698 = (c698 - (unsigned long)b[j + 1UL]) & 0xFFFFFFFFUL;
                mask[j] = 0;
                break;
            }
            if (b[j + 1UL] == 0 || j >= 0x0CUL) break;
        }
    }
    /* Recompute f0/ec/l108/l28 from b (scan locals) */
    f0 = 0x0CU; ec = 0x0CU;
    {
        unsigned k = 0;
        unsigned fk;
        sum: ;
        /* sum not needed here; compute f0/ec directly */
        k = 0;
        fk = b[1];
        {
            unsigned long s = b[0];
            unsigned long sum2 = s;
            unsigned kk = 0;
            unsigned fkk = fk;
            for (;;) {
                unsigned bs = b[0x0DUL + kk];
                if ((unsigned)bs == l50) f0 = kk;
                if ((unsigned)bs == l3c) ec = kk;
                sum2 = (sum2 + fkk) & 0xFFFFFFFFUL;
                (void)sum2;
                kk += 1UL;
                if (kk >= 0x0CUL) break;
                fkk = b[kk + 1UL];
                if (fkk == 0) break;
                (void)k;
            }
            (void)fk; (void)k;
        }
    }
    l108 = dec4c_rom32(0x42754dUL + 4UL * ((b[0x1A] >> 4) & 0x0FUL));
    l28 = b[0x0DUL + (b[0x1A] & 0x0FUL)];
    l104 = dec4c_rom32(0x42750dUL + ((rec[0x13] >> 4) & 0x0FUL));
    l100 = dec4c_rom32(0x4274cdUL + (c->mru_n[l10c & 0x3FFUL] & 0x0FUL));
    k2a = (c->g45d104 < c->g45d138) ? (b[0x1A] & 0x0FUL) : 0x0CUL;
    t = dec4c_o4_escape(c, b, rec, c6a4, c698, bm, &see_esc_local);
    *see_esc = see_esc_local;
    (void)p42; (void)p3a; (void)p2e; (void)pedi;
    /* SEE corrections (S_O4MPS/MRU/O2/FST) */
    if (mask[k2a & 0x0FUL]) {
        unsigned long cc = b[(k2a & 0x0FUL) + 1UL];
        unsigned long bc = (c698 - cc) & 0xFFFFFFFFUL;
        unsigned long b8 = 0, e, x, q;
        e = (b[0x1B] >> 4) + ((cc - (cc == 2UL ? 1UL : 0UL)) & 0xFFFFFFFFUL) + l108;
        e &= 0xFFFFFFFFUL;
        if (l28 == l3c) { b8 = 0x40UL; e = (e + l104) & 0xFFFFFFFFUL; }
        if (l28 == l50) { b8 |= 0x80UL; e = (e + l100) & 0xFFFFFFFFUL; }
        x = dec4c_dv((e << 4) & 0xFFFFFFFFUL, (bc + e) & 0xFFFFFFFFUL);
        if (k2a != 0) x |= 0x100UL;
        if ((l28 & 0xC0UL) == 0xC0UL) x |= 0x20UL;
        if (dec4c_recent(c, (unsigned char)l28)) x |= 0x200UL;
        if (dec4c_marked(c, (unsigned char)l28)) x |= 0x400UL;
        p2e = D4C_S_O4MPS + ((lfc | x | b8) & 0xFFFFFFFFUL);
        if (dec4c_see_freq(c, p2e, bc, cc, &q)) {
            t = (t + (q - cc)) & 0xFFFFFFFFUL;
            b[(k2a & 0x0FUL) + 1UL] = (unsigned char)q;
        }
    }
    if (mask[f0 & 0x0FUL] && k2a != f0) {
        unsigned long d8 = b[(f0 & 0x0FUL) + 1UL];
        unsigned long b4 = (c698 - d8) & 0xFFFFFFFFUL;
        unsigned long e = (d8 + l100) & 0xFFFFFFFFUL;
        unsigned long x = dec4c_dv((e << 4) & 0xFFFFFFFFUL, (b4 + e) & 0xFFFFFFFFUL);
        unsigned long q;
        if ((l50 & 0xC0UL) == 0xC0UL) x |= 0x20UL;
        if ((unsigned)l50 == l3c) x |= 0x40UL;
        if (dec4c_recent(c, (unsigned char)l50)) x |= 0x80UL;
        if (dec4c_marked(c, (unsigned char)l50)) x |= 0x100UL;
        x |= lfc;
        p3a = D4C_S_O4MRU + x;
        if (dec4c_see_freq(c, p3a, b4, d8, &q)) {
            t = (t + (q - d8)) & 0xFFFFFFFFUL;
            b[(f0 & 0x0FUL) + 1UL] = (unsigned char)q;
        }
    }
    if (mask[ec & 0x0FUL] && k2a != ec && f0 != ec) {
        unsigned long d0 = b[(ec & 0x0FUL) + 1UL];
        unsigned long b0 = (c698 - d0) & 0xFFFFFFFFUL;
        unsigned long e = (d0 + l104) & 0xFFFFFFFFUL;
        unsigned long x = dec4c_dv((e << 4) & 0xFFFFFFFFUL, (b0 + e) & 0xFFFFFFFFUL);
        unsigned long q;
        if ((l3c & 0xC0UL) == 0xC0UL) x |= 0x20UL;
        if (dec4c_recent(c, (unsigned char)l3c)) x |= 0x40UL;
        if (dec4c_marked(c, (unsigned char)l3c)) x |= 0x80UL;
        x |= lfc;
        pedi = D4C_S_O4O2 + x;
        if (dec4c_see_freq(c, pedi, b0, d0, &q)) {
            t = (t + (q - d0)) & 0xFFFFFFFFUL;
            b[(ec & 0x0FUL) + 1UL] = (unsigned char)q;
        }
    }
    if (mask[0] && k2a != 0 && f0 != 0 && ec != 0) {
        unsigned long e0 = b[1];
        unsigned long x = dec4c_dv((e0 << 4) & 0xFFFFFFFFUL, c698);
        unsigned s0 = b[0x0D];
        unsigned long q;
        if ((s0 & 0xC0UL) == 0xC0UL) x |= 0x20UL;
        if (dec4c_recent(c, (unsigned char)s0)) x |= 0x80UL;
        if (dec4c_marked(c, (unsigned char)s0)) x |= 0x100UL;
        x |= lfc;
        p42 = D4C_S_O4FST + x;
        q = ((c698 * c->see[p42 & (DEC4C_SEE_LEN - 1UL)]) >> 13) + 1UL;
        if (q >= 0x100UL) q = 0xFFUL;
        t = (t + (q - e0)) & 0xFFFFFFFFUL;
        b[1] = (unsigned char)q;
    }
    /* Multi-symbol decode loop */
    {
        unsigned long v = dec4c_rc_div(c, t) & 0xFFFFUL;
        unsigned long cum = 0, e4 = 0;
        unsigned k = 0;
        while (k < 0x0CU) {
            if (mask[k]) {
                unsigned long f = b[k + 1UL];
                if (f == 0) { k = 0x0CU; break; }
                e4 = (cum + f) & 0xFFFFFFFFUL;
                if (v < e4) {
                    *esc = 0;
                    *sym = b[k + 0x0DUL];
                    break;
                }
                dec4c_excl_push(c, b[k + 0x0DUL]);
                cum = e4;
            }
            k += 1UL;
        }
        c->g_e3a3 = (unsigned char)k;
        {
            unsigned long hi = (k == 0x0CU) ? c->rc_t : e4;
            dec4c_rc_dec(c, cum, hi);
        }
        {
            unsigned long got = k;
            if (p42 != 0) dec4c_see_bump(c, p42, got == 0);
            if (p3a != 0) dec4c_see_bump(c, p3a, got == f0);
            if (pedi != 0) dec4c_see_bump(c, pedi, got == ec);
            if (p2e != 0) dec4c_see_bump(c, p2e, got == k2a);
        }
        if (*see_esc != 0)
            dec4c_see_bump(c, *see_esc, *esc == 0);
    }
}

/* Order-2 coder (lit.rs order2, a4=None delta stub for now). */
/* o2_halve_if + o2_sort (update.rs helpers) */
static void dec4c_o2_halve_if(unsigned char r[20])
{
    unsigned lim = dec4c_rom8(0x4245a0UL + r[0]);
    if ((unsigned)r[0x12] > lim) {
        int j;
        r[0x12] = 0;
        for (j = 8; j >= 0; j--) {
            unsigned v = (unsigned)(r[j] + 1U) >> 1;
            r[j] = (unsigned char)v;
            r[0x12] = (unsigned char)((r[0x12] + v) & 0xFFUL);
        }
    }
}

static unsigned dec4c_o2_sort(unsigned char r[20], unsigned k)
{
    while (k != 0) {
        unsigned dl = r[k + 1UL];
        unsigned dh, s, bl;
        if (dl < r[k])
            break;
        dh = r[k];
        r[k] = (unsigned char)dl;
        s = r[k + 9UL];
        bl = r[k + 8UL];
        r[k + 8UL] = (unsigned char)s;
        r[k + 1UL] = (unsigned char)dh;
        r[k + 9UL] = (unsigned char)bl;
        k -= 1UL;
        r[0x13] = (unsigned char)(r[0x13] - 1U);
    }
    return k;
}

/* 0x40f19a light update (after order-4 escape). */
static void dec4c_o2_update_light(dec4c_ctx *c, unsigned char sym)
{
    unsigned long ri = c->g45c69c & 0x1FFFFUL;
    unsigned char r[20];
    unsigned k = 0;
    dec4c_o2r(c, ri, r);
    if (r[0] == 0) {
        memset(r, 0, sizeof(r));
        r[0] = 2;
        r[1] = 2;
        r[0x12] = 4;
        r[9] = sym;
        dec4c_o2_put(c, ri, r);
        return;
    }
    while (r[k + 9UL] != sym) {
        k += 1UL;
        if (k >= 8UL) break;
    }
    dec4c_o2_halve_if(r);
    r[0x12] = (unsigned char)((r[0x12] + 1U) & 0xFFUL);
    if (k < 8UL) {
        r[k + 1UL] = (unsigned char)(r[k + 1UL] + 1U);
        if ((r[0x13] & 0x0FUL) == k) {
            if (r[0x13] < 0x40U)
                r[0x13] = (unsigned char)(r[0x13] + 0x10U);
        } else {
            r[0x13] = (unsigned char)k;
        }
    } else {
        unsigned d;
        r[0x11] = (unsigned char)(r[0x11] + 1U);
        k = 1;
        r[0] = (unsigned char)(r[0] + 1U);
        while (r[k + 1UL] != 0) {
            k += 1UL;
            if (k == 7UL) break;
        }
        if (r[k + 1UL] >= 0x0CU) {
            k = 0;
        } else {
            d = (unsigned char)(1U - r[k + 1UL]);
            r[0x12] = (unsigned char)(r[0x12] + d);
            r[k + 1UL] = 1;
            r[k + 9UL] = sym;
            r[0x13] = (unsigned char)k;
        }
    }
    c->g_e3a5 = (unsigned char)dec4c_o2_sort(r, k);
    dec4c_o2_put(c, ri, r);
}

static void dec4c_order2(dec4c_ctx *c, unsigned long o2i,
                         unsigned l50, unsigned l3c, unsigned long l10c,
                         unsigned long c6a4, unsigned long lfc,
                         unsigned long l104, unsigned long l100,
                         int l52, int l4e, int bm,
                         int *esc, unsigned char *sym)
{
    unsigned char r[20];
    unsigned mask[16];
    unsigned long cnt = 0, f8 = 8UL, tot;
    unsigned long ex[4];
    unsigned k;
    unsigned g6a8;
    unsigned long see = 0;
    unsigned long l80 = 0, l88 = 0, l90 = 0;
    unsigned long p16 = 0, pe = 0, pedi = 0;
    dec4c_o2r(c, o2i, r);
    for (k = 0; k < 16UL; k++) mask[k] = (k < 8UL) ? 1UL : 0UL;
    tot = r[0x12];
    for (k = 0; k < 4UL; k++) ex[k] = 0;
    for (k = 0; k < (unsigned long)c->nex; k++) {
        unsigned s = c->excl[k];
        ex[(s >> 6) & 0x03UL] |= 1UL << (s & 63UL);
    }
    k = 0;
    for (;;) {
        unsigned s = r[9UL + k];
        cnt += 1UL;
        if ((unsigned)s == l50) f8 = k;
        if (((ex[(s >> 6) & 0x03UL] >> (s & 63UL)) & 1UL) != 0UL) {
            tot = (tot - r[k + 1UL]) & 0xFFFFFFFFUL;
            mask[k] = 0;
        }
        k += 1UL;
        if (r[k + 1UL] == 0 || k >= 8UL) break;
    }
    c->g45c6a8 = tot;
    c->g_e3a5 = 0xFFU;
    if (((tot << 2) & 0xFFFFFFFFUL) <= (unsigned long)r[0] * 5UL)
        return;
    if ((((unsigned long)r[0x11] << 2) & 0xFFFFFFFFUL) + c->g45c6ac >= 0x10UL)
        return;
    c->g_e39a = (unsigned char)l52;
    if (l4e)
        return;
    g6a8 = (unsigned)tot;
    if (r[2] == 0) {
        unsigned s9 = r[9];
        unsigned long eax = (unsigned)r[1] >> 1;
        unsigned long ecx = 0;
        unsigned long pa, v, p;
        if ((unsigned)s9 == l50) {
            eax = (eax + c->mru_n[l10c & 0x3FFUL]) & 0xFFFFFFFFUL;
            ecx = 1;
        }
        if (eax < 0x10UL) {
            unsigned long edx;
            if (bm)
                edx = ((c6a4 & 0xC0UL) >> 5);
            else {
                edx = (unsigned long)dec4c_rom8(0x42738dUL + ((c6a4 >> 8) & 0xFFUL)) * 2UL +
                      (unsigned long)dec4c_rom8(0x42738dUL + (c6a4 & 0xFFUL)) * 4UL;
            }
            ecx |= edx;
        }
        if (bm && dec4c_recent(c, (unsigned char)s9))
            ecx |= 8UL;
        if (dec4c_marked(c, (unsigned char)s9))
            ecx |= 0x10UL;
        pa = D4C_S_O2BIN + ((dec4c_rom32(0x423df0UL + 4UL * (eax & 0x1FFUL)) << 5) + ecx);
        v = dec4c_rc_div(c, 0x2000UL) & 0xFFFFUL;
        p = c->see[pa & (DEC4C_SEE_LEN - 1UL)];
        if (v < p) {
            *esc = 0;
            c->g_e3a5 = 0;
            c->g_e3a4 = 1;
            *sym = (unsigned char)s9;
            dec4c_rc_dec(c, 0, p);
        } else {
            c->g_e3a5 = 8;
            dec4c_excl_push(c, (unsigned char)s9);
            dec4c_rc_dec(c, p, 0x2000UL);
        }
        see = pa;
    } else {
        unsigned k7c = (c->g45d104 < c->g45d138) ? (r[0x13] & 0x0FUL) : 8UL;
        unsigned long t = g6a8;
        if (cnt < 8UL) {
            unsigned long e = (g6a8 - r[0]) & 0xFFFFFFFFUL;
            if (e < 0x10UL) {
                unsigned long idx = dec4c_rom32(0x4239d0UL + 4UL * cnt) +
                    (e & 0x0CUL) + ((r[0x12] == g6a8) ? 1UL : 0UL);
                unsigned long pa = D4C_S_O2ESC + idx;
                t = (dec4c_dv((e << 13) & 0xFFFFFFFFUL, c->see[pa & (DEC4C_SEE_LEN - 1UL)]) + 1UL +
                     ((unsigned)r[0] >> 1)) & 0xFFFFFFFFUL;
                see = pa;
            }
        }
        if (mask[k7c & 0x0FUL]) {
            unsigned long cc = r[(k7c & 0x0FUL) + 1UL];
            unsigned long bc = (g6a8 - cc) & 0xFFFFFFFFUL;
            unsigned long b8 = 0, e, x, q;
            e = (cc - ((cc == 2UL) ? 1UL : 0UL) + l104) & 0xFFFFFFFFUL;
            if ((unsigned)l3c == l50) { b8 = 0x80UL; e = (e + l100) & 0xFFFFFFFFUL; }
            x = dec4c_dv((e << 4) & 0xFFFFFFFFUL, (bc + e) & 0xFFFFFFFFUL);
            if (k7c != 0) x |= 0x100UL;
            if ((l3c & 0xC0UL) == 0xC0UL) x |= 0x20UL;
            if (dec4c_recent(c, (unsigned char)l3c)) x |= 0x200UL;
            if (dec4c_marked(c, (unsigned char)l3c)) x |= 0x400UL;
            x |= lfc;
            pedi = D4C_S_O2O2 + x;
            if (dec4c_see_freq(c, pedi, bc, cc, &q)) {
                t = (t + (q - cc)) & 0xFFFFFFFFUL;
                r[(k7c & 0x0FUL) + 1UL] = (unsigned char)q;
            }
            (void)b8;
        }
        if (mask[f8 & 0x0FUL] && k7c != f8) {
            unsigned long d8 = r[(f8 & 0x0FUL) + 1UL];
            unsigned long b4 = (g6a8 - d8) & 0xFFFFFFFFUL;
            unsigned long e = (d8 + l100) & 0xFFFFFFFFUL;
            unsigned long x = dec4c_dv((e << 4) & 0xFFFFFFFFUL, (b4 + e) & 0xFFFFFFFFUL);
            unsigned long q;
            if ((l50 & 0xC0UL) == 0xC0UL) x |= 0x20UL;
            if (dec4c_recent(c, (unsigned char)l50)) x |= 0x80UL;
            if (dec4c_marked(c, (unsigned char)l50)) x |= 0x100UL;
            x |= lfc;
            p16 = D4C_S_O2MRU + x;
            if (dec4c_see_freq(c, p16, b4, d8, &q)) {
                t = (t + (q - d8)) & 0xFFFFFFFFUL;
                r[(f8 & 0x0FUL) + 1UL] = (unsigned char)q;
            }
        }
        if (mask[0] && k7c != 0 && f8 != 0) {
            unsigned long e0 = r[1];
            unsigned long x = dec4c_dv((e0 << 4) & 0xFFFFFFFFUL, g6a8);
            unsigned s9 = r[9];
            unsigned long q;
            if ((s9 & 0xC0UL) == 0xC0UL) x |= 0x20UL;
            if (dec4c_recent(c, (unsigned char)s9)) x |= 0x80UL;
            if (dec4c_marked(c, (unsigned char)s9)) x |= 0x100UL;
            x |= lfc;
            pe = D4C_S_O2FST + x;
            q = ((g6a8 * c->see[pe & (DEC4C_SEE_LEN - 1UL)]) >> 13) + 1UL;
            if (q >= 0x100UL) q = 0xFFUL;
            t = (t + (q - e0)) & 0xFFFFFFFFUL;
            r[1] = (unsigned char)q;
        }
        {
            unsigned long v = dec4c_rc_div(c, t) & 0xFFFFUL;
            unsigned long cum = 0, e2 = 0;
            unsigned kk = 0;
            while (kk < 8UL) {
                if (mask[kk]) {
                    unsigned long f = r[kk + 1UL];
                    if (f == 0) { kk = 8UL; break; }
                    e2 = (cum + f) & 0xFFFFFFFFUL;
                    if (v < e2) {
                        *esc = 0;
                        *sym = r[kk + 9UL];
                        break;
                    }
                    dec4c_excl_push(c, r[kk + 9UL]);
                    cum = e2;
                }
                kk += 1UL;
            }
            (void)l80; (void)l88; (void)l90;
            c->g_e3a5 = (unsigned char)kk;
            {
                unsigned long hi = (kk == 8UL) ? c->rc_t : e2;
                dec4c_rc_dec(c, cum, hi);
            }
        }
    }
    if (see != 0)
        dec4c_see_bump(c, see, *esc == 0);
    if (*esc == 0)
        dec4c_o2_update_light(c, *sym);
}

/* Cost table (0x429ed4 prefix sum, 0x419ac0) for add_cost. */
static unsigned dec4c_cost(dec4c_ctx *c, unsigned idx)
{
    static unsigned short cost[0x8000];
    static int cost_init = 0;
    unsigned acc = 0;
    unsigned k;
    (void)c;
    if (!cost_init) {
        for (k = 0; k < 0x4000U; k++) {
            unsigned raw = (unsigned)DEC4C_DG_429ED2[2 + 2 * k] |
                           ((unsigned)DEC4C_DG_429ED2[3 + 2 * k] << 8);
            acc = (k == 0) ? raw : ((acc + raw) & 0xFFFFU);
            cost[k] = (unsigned short)acc;
        }
        cost[0x4000] = (unsigned short)(((unsigned)DEC4C_DG_429ED2[0x8003] << 8) & 0xFFFFU);
        cost_init = 1;
    }
    return cost[idx & 0x7FFFUL];
}

static unsigned dec4c_cx(unsigned long c_)
{
    return (c_ >= 0x310UL) ? 0x30FUL : (unsigned)c_;
}

static unsigned dec4c_excl_total(dec4c_ctx *c, unsigned frq, unsigned n)
{
    unsigned si = c->o1t[frq][0][0];
    unsigned i;
    for (i = 0; i < n; i++)
        si = (si - c->o1t[frq][1][c->excl[i]]) & 0xFFFFU;
    return si;
}

static unsigned long dec4c_mru_see(dec4c_ctx *c, unsigned long ctx, unsigned char s,
                                   unsigned long f, unsigned si, unsigned long escc)
{
    unsigned long t = dec4c_rom32(0x42748dUL + 4UL * (c->mru_n[dec4c_cx(ctx)] & 0x0FUL));
    unsigned long x = dec4c_dv((f + t) << 4, (t + escc + si) & 0xFFFFFFFFUL);
    unsigned s_ = s;
    if (f <= 0x600UL) {
        if (f > 0x100UL) x |= 0x30UL;
        else if (f > 0x40UL) x |= 0x20UL;
        else x |= 0x10UL;
    }
    if ((ctx & 0xC0UL) == 0xC0UL) x |= 0x40UL;
    if ((s_ & 0xC0UL) == 0xC0UL) x |= (0x80UL << ((s_ == 0xFFUL) ? 1UL : 0UL));
    {
        unsigned long ecx = x | 0x200UL;
        if (c->binmode != 0) {
            if (((c->pos - c->lastpos[s_]) & c->dictmask) <= 0x40UL)
                x = ecx;
        } else if (dec4c_rom8(0x42738dUL + s_) != 0) {
            x = ecx;
        }
        if (c->see[D4C_MARKS + s_] == c->nbytes)
            x |= 0x400UL;
    }
    return x;
}

static unsigned long dec4c_see_total(dec4c_ctx *c, unsigned long si, unsigned long escc, unsigned long *out_a)
{
    long q = (si + escc) ? (long)((escc << 5) / (si + escc)) : 0;
    unsigned long a = D4C_S_O1TOT + (unsigned long)q;
    unsigned long t = (dec4c_dv(si << 13, c->see[a & (DEC4C_SEE_LEN - 1UL)]) + 1UL) & 0xFFFFFFFFUL;
    if (t >= 0x4000UL) t = 0x3FFFUL;
    *out_a = a;
    return t;
}

/* 0x412257: order-1 symbol (falling back to g45c690 context). */
static unsigned char dec4c_o1_symbol(dec4c_ctx *c, unsigned long ctx)
{
    unsigned n = c->nex;
    unsigned cc = dec4c_cx(ctx);
    unsigned tree = cc;
    unsigned si = c->o1t[cc][0][0];
    unsigned escc = c->esc[cc];
    unsigned lim = (escc * c->ba3c[cc]) & 0xFFFFFFFFUL;
    unsigned frq, v;
    /* Fallback to secondary context when total is small */
    if (si <= lim) {
        unsigned f = dec4c_cx(c->g45c690);
        tree = f;
        frq = f;
        si = dec4c_excl_total(c, f, n);
    } else {
        frq = cc;
        si = dec4c_excl_total(c, cc, n);
        if (ctx < (0x200UL >> (unsigned)c->binmode)) {
            unsigned s = c->mru_s[cc];
            int excluded = 0;
            unsigned i;
            for (i = 0; i < n; i++)
                if (c->excl[i] == s) { excluded = 1; break; }
            if (!excluded) {
                unsigned long f = c->o1t[cc][1][s];
                unsigned long x = dec4c_mru_see(c, ctx, (unsigned char)s, f, si, escc);
                unsigned long pa = D4C_S_O1MRU + x;
                unsigned long p = c->see[pa & (DEC4C_SEE_LEN - 1UL)];
                unsigned long vv = dec4c_rc_div(c, 0x2000UL) & 0xFFFFUL;
                int hit = (vv < p) ? 1 : 0;
                unsigned long lo = hit ? 0UL : p;
                unsigned long hi = hit ? p : 0x2000UL;
                unsigned long cst = dec4c_cost(c, 2UL * (hi - lo));
                c->g45c648 = (c->g45c648 + cst) & 0xFFFFFFFFUL;
                dec4c_rc_dec(c, lo, hi);
                if (hit) {
                    c->see[pa & (DEC4C_SEE_LEN - 1UL)] = (p + ((0x2000UL - p) >> 6)) & 0xFFFFFFFFUL;
                    return (unsigned char)s;
                } else {
                    c->excl[n++] = (unsigned char)s;
                    si = (si - (unsigned short)f) & 0xFFFFU;
                    c->nex = (unsigned char)n;
                    c->see[pa & (DEC4C_SEE_LEN - 1UL)] = (p - (p >> 6)) & 0xFFFFFFFFUL;
                }
            }
        }
        if (si == 0) {
            unsigned f = dec4c_cx(c->g45c690);
            tree = f;
            frq = f;
            si = dec4c_excl_total(c, f, n);
        }
    }
    /* Decode via Fenwick tree with SEE escape */
    {
        unsigned long tot;
        unsigned long a;
        unsigned long est = dec4c_see_total(c, si, escc, &a);
        v = dec4c_rc_div(c, tot = (si + est) & 0xFFFFFFFFUL);
        if (v < si) {
            /* Symbol search in tree (Fenwick find) */
            unsigned idx = 0;
            unsigned bit;
            unsigned long want = v;
            /* Binary lifting on Fenwick CUM */
            for (bit = 0x8000UL; bit != 0; bit >>= 1) {
                unsigned t_ = idx + bit;
                if (t_ <= 256UL && c->o1t[tree][0][t_] <= want) {
                    want = (want - c->o1t[tree][0][t_]) & 0xFFFFFFFFUL;
                    idx = t_;
                }
            }
            {
                unsigned char sym = (unsigned char)(idx & 0xFFUL);
                /* Update SEE escape entry */
                if (v < si) {
                    c->see[a & (DEC4C_SEE_LEN - 1UL)] =
                        (c->see[a & (DEC4C_SEE_LEN - 1UL)] + ((0x2000UL - c->see[a & (DEC4C_SEE_LEN - 1UL)]) >> 6)) & 0xFFFFFFFFUL;
                } else {
                    c->see[a & (DEC4C_SEE_LEN - 1UL)] =
                        (c->see[a & (DEC4C_SEE_LEN - 1UL)] - (c->see[a & (DEC4C_SEE_LEN - 1UL)] >> 6)) & 0xFFFFFFFFUL;
                }
                dec4c_rc_dec(c, v, v + 1UL);
                (void)frq;
                return sym;
            }
        } else {
            /* Escape: fall through to flat (caller handles via order-0) */
            dec4c_rc_dec(c, si, si + est);
            c->see[a & (DEC4C_SEE_LEN - 1UL)] =
                (c->see[a & (DEC4C_SEE_LEN - 1UL)] - (c->see[a & (DEC4C_SEE_LEN - 1UL)] >> 6)) & 0xFFFFFFFFUL;
            return 0xFFU; /* escape sentinel; driver falls back */
        }
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
    c->outbuf = (unsigned char *)malloc((size_t)(out_len ? out_len : 1));
    if (!c->outbuf) return 2;
    dec4c_init_model(c);
    dec4c_rc_init(c);
    dec4c_read_switch_pos(c);
    /* TODO: main loop 0x40f962 (segments, match, order-4/2/1/0,
     * updates, text transform). Until ported, report data error so
     * the dispatcher can fall back to the arena decoder. */
    free(c->outbuf);
    c->outbuf = 0;
    return 3;
}
