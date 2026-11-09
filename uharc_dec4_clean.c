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
    if (!c->dict) { free(c); return 2; }
    c->method = method;
    c->kennedy_end = 0xFFFFFFFFUL;
    *ctx = c;
    return 0;
}

void uharc_dec4c_free(void *ctx)
{
    dec4c_ctx *c = (dec4c_ctx *)ctx;
    if (!c) return;
    free(c->dict);
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
