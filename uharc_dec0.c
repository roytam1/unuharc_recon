/* UHARC 0.6 decompressor dispatch + core0/1 (STORE/RLE) - C89 */
#include <stdlib.h>
#include <string.h>
#include "uharc_dec.h"

/* method -> core table (from DGROUP:004228C2) */
static const unsigned char method_to_core[88] = {
    0,
    1,
    2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};

int uharc_dec_core(int method)
{
    if (method < 0 || method > 87) return -1;
    return (int)method_to_core[method];
}

/* Version-specific method -> core tables (verified from each binary's
 * dispatch table: 0.2 byte_3A520, 0.3 byte_428EE0, 0.4 byte_431210,
 * 0.6 byte_4228C2). */
int uharc_dec_core_v(int method, int ver)
{
    if (method < 0) return -1;
    if (ver == 2) {
        if (method > 43) return -1;          /* 0.2: 44 methods, no PPM */
        if (method <= 1) return method;      /* 0,1 */
        if (method <= 7) return 2;           /* 2..7 */
        return 3;                            /* 8..43 ALZ/PPM/LZ */
    }
    if (ver == 3) {
        if (method == 80) return 0;          /* alias */
        if (method > 79) return -1;
        if (method <= 1) return method;
        if (method <= 7) return 2;           /* 2..7 */
        if (method <= 21) return 3;          /* 8..21 */
        if (method <= 35) return 4;          /* 22..35 */
        return 5;                            /* 36..79 */
    }
    /* 0.4 and 0.6 share the layout */
    if (method > 87) return -1;
    return (int)method_to_core[method];
}

/* ---------- shared bit/byte pump state ---------- */
typedef struct dec_state_s {
    uharc_dec_read_cb rcb;
    uharc_dec_write_cb wcb;
    void *rctx;
    void *wctx;
    unsigned char *inb;
    unsigned in_pos;
    unsigned in_avail;
    unsigned in_cap;
    unsigned char *outb;
    unsigned out_fill;
    unsigned out_cap;
    unsigned long in_tot;
    unsigned long out_tot;
    unsigned long expected;
    int err;
} dec_state;

static int ds_refill(dec_state *s)
{
    int n;
    s->in_pos = 0;
    n = s->rcb(s->rctx, s->inb, s->in_cap);
    if (n < 0) n = 0;
    s->in_avail = (unsigned)n;
    if (n == 0) {
        s->err = 3;
        return 0;
    }
    return 1;
}

static int ds_get(dec_state *s)
{
    int c;
    if (s->in_pos >= s->in_avail) {
        if (!ds_refill(s)) return -1;
    }
    c = s->inb[s->in_pos++];
    s->in_tot++;
    return c;
}

static int ds_get16(dec_state *s)
{
    int lo, hi;
    lo = ds_get(s);
    if (lo < 0) return -1;
    hi = ds_get(s);
    if (hi < 0) return -1;
    return lo | (hi << 8);
}

static int ds_flush(dec_state *s)
{
    int w;
    if (s->out_fill == 0) return 1;
    w = s->wcb(s->wctx, s->outb, s->out_fill);
    if (w != 0) {
        s->err = 4;
        return 0;
    }
    s->out_fill = 0;
    return 1;
}

static int ds_emit1(dec_state *s, unsigned c)
{
    s->outb[s->out_fill++] = (unsigned char)c;
    s->out_tot++;
    if (s->out_fill == s->out_cap) {
        return ds_flush(s);
    }
    return 1;
}

static int ds_fill(dec_state *s, unsigned n, unsigned v)
{
    while (n > 0) {
        unsigned sp, k, i;
        if (s->out_fill == s->out_cap) {
            if (!ds_flush(s)) return 0;
        }
        sp = s->out_cap - s->out_fill;
        k = n < sp ? n : sp;
        for (i = 0; i < k; i++) s->outb[s->out_fill + i] = (unsigned char)v;
        s->out_fill += k;
        s->out_tot += k;
        n -= k;
        if (s->out_fill == s->out_cap) {
            if (!ds_flush(s)) return 0;
        }
    }
    return 1;
}

/* core0 STORE (sub_40B4BB) */
static int dec0_decode(dec_state *s)
{
    while (s->out_tot < s->expected && !s->err) {
        unsigned n, i;
        if (s->in_pos >= s->in_avail) {
            if (!ds_refill(s)) break;
        }
        n = s->in_avail - s->in_pos;
        if ((unsigned long)n > s->expected - s->out_tot)
            n = (unsigned)(s->expected - s->out_tot);
        for (i = 0; i < n; i++) {
            if (!ds_emit1(s, s->inb[s->in_pos++])) break;
            s->in_tot++;
        }
    }
    if (!s->err && s->out_fill) ds_flush(s);
    return s->err;
}

/* core1 RLE/delta (sub_40B504) */
static int dec1_decode(dec_state *s)
{
    unsigned char pred;
    pred = 0xED;
    while (s->out_tot < s->expected && !s->err) {
        int c, d, v, ln;
        long diff;
        c = ds_get(s);
        if (c < 0) break;
        diff = (long)c - (long)pred;
        if ((diff >> 1) != 0) {
            if (!ds_emit1(s, (unsigned)c)) break;
            continue;
        }
        d = ds_get(s);
        if (d < 0) break;
        if (c == (int)pred) {
            /* emit d x3 */
            if (!ds_emit1(s, (unsigned)d)) break;
            if (!ds_emit1(s, (unsigned)d)) break;
            if (!ds_emit1(s, (unsigned)d)) break;
        } else if (d < 2) {
            if (!ds_emit1(s, (unsigned)(pred + d))) break;
            pred = (unsigned char)(pred - 0x40);
        } else if (d <= 3) {
            if (!ds_emit1(s, (unsigned)(pred + d - 2))) break;
            if (!ds_emit1(s, (unsigned)(pred + d - 2))) break;
            pred = (unsigned char)(pred - 0x40);
        } else if (d == 0xFF) {
            ln = ds_get16(s);
            if (ln < 0) break;
            v = ds_get(s);
            if (v < 0) break;
            if (!ds_fill(s, (unsigned)ln, (unsigned)v)) break;
        } else {
            v = ds_get(s);
            if (v < 0) break;
            if (!ds_fill(s, (unsigned)d, (unsigned)v)) break;
        }
    }
    if (!s->err && s->out_fill) ds_flush(s);
    return s->err;
}

/* forward declarations for cores 2..5 (in separate files) */
int uharc_dec2_init(void **pctx, int method);
int uharc_dec2_decode(void *ctx,
                      uharc_dec_read_cb rcb, void *rctx,
                      uharc_dec_write_cb wcb, void *wctx,
                      unsigned long out_len);
void uharc_dec2_free(void *ctx);
int uharc_dec3_init(void **pctx, int method);
int uharc_dec3_decode(void *ctx,
                      uharc_dec_read_cb rcb, void *rctx,
                      uharc_dec_write_cb wcb, void *wctx,
                      unsigned long out_len);
void uharc_dec3_free(void *ctx);
int uharc_dec4_init(void **pctx, int method);
int uharc_dec4_decode(void *ctx,
                      uharc_dec_read_cb rcb, void *rctx,
                      uharc_dec_write_cb wcb, void *wctx,
                      unsigned long out_len);
void uharc_dec4_free(void *ctx);
struct uharc_dec5_s;
typedef struct uharc_dec5_s uharc_dec5_t;
uharc_dec5_t *uharc_dec5_init(int method);
int uharc_dec5_decode(uharc_dec5_t *d,
                      uharc_dec_read_cb rcb, void *rctx,
                      uharc_dec_write_cb wcb, void *wctx,
                      unsigned long out_len);
void uharc_dec5_free(uharc_dec5_t *d);
/* LSB-first core2 (0.2/0.3/0.4, verified) */
int uharc_dec2l_init(void **pctx, int method);
int uharc_dec2l_decode(void *ctx,
                       uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len);
void uharc_dec2l_free(void *ctx);

/* public dispatch: mirrors sub_40705C/sub_40717F buffer policy */
int uharc_dec_decode_v(int method, int core2_lsb, int ver,
                       uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len)
{
    dec_state s;
    unsigned char *inb;
    unsigned char *outb;
    unsigned in_cap, out_cap;
    int core, ret;

    if (out_len == 0) return 0;
    if (!rcb || !wcb) return 1;
    if (method < 0 || method > 87) return 1;

    core = uharc_dec_core_v(method, ver);
    if (core < 0) return 1;

    /* buffer sizing mirrors sub_40717F: ~2*out_len+1034, split for method!=0 */
    if (out_len < 0x40000UL) {
        in_cap = 0x10000U;
        out_cap = 0x10000U;
    } else {
        in_cap = 0x40000U;
        out_cap = 0x40000U;
    }
    if ((unsigned long)in_cap > out_len + 1029UL && core == 0) {
        in_cap = (unsigned)(out_len + 1029UL);
        out_cap = in_cap;
    }
    inb = (unsigned char *)malloc(in_cap ? in_cap : 1);
    outb = (unsigned char *)malloc(out_cap ? out_cap : 1);
    if (!inb || !outb) {
        free(inb);
        free(outb);
        return 2;
    }
    s.rcb = rcb;
    s.wcb = wcb;
    s.rctx = rctx;
    s.wctx = wctx;
    s.inb = inb;
    s.in_pos = 0;
    s.in_avail = 0;
    s.in_cap = in_cap;
    s.outb = outb;
    s.out_fill = 0;
    s.out_cap = out_cap;
    s.in_tot = 0;
    s.out_tot = 0;
    s.expected = out_len;
    s.err = 0;

    switch (core) {
    case 0: ret = dec0_decode(&s); break;
    case 1: ret = dec1_decode(&s); break;
    case 2: {
        void *c2 = 0;
        if (core2_lsb)
            ret = uharc_dec2l_init(&c2, method);
        else
            ret = uharc_dec2_init(&c2, method);
        if (ret == 0) {
            if (core2_lsb)
                ret = uharc_dec2l_decode(c2, rcb, rctx, wcb, wctx, out_len);
            else
                ret = uharc_dec2_decode(c2, rcb, rctx, wcb, wctx, out_len);
            if (core2_lsb) uharc_dec2l_free(c2);
            else uharc_dec2_free(c2);
        }
        break;
    }
    case 3: {
        void *c3 = 0;
        ret = uharc_dec3_init(&c3, method);
        if (ret == 0) {
            ret = uharc_dec3_decode(c3, rcb, rctx, wcb, wctx, out_len);
            uharc_dec3_free(c3);
        }
        break;
    }
    case 4: {
        void *c4 = 0;
        ret = uharc_dec4_init(&c4, method);
        if (ret == 0) {
            ret = uharc_dec4_decode(c4, rcb, rctx, wcb, wctx, out_len);
            uharc_dec4_free(c4);
        }
        break;
    }
    case 5: {
        uharc_dec5_t *c5 = uharc_dec5_init(method);
        if (!c5) ret = 1;
        else {
            ret = uharc_dec5_decode(c5, rcb, rctx, wcb, wctx, out_len);
            uharc_dec5_free(c5);
        }
        break;
    }
    default:
        ret = 1;
        break;
    }

    free(inb);
    free(outb);
    return ret;
}

/* MSB-first (0.6) entry point kept for existing callers */
int uharc_dec_decode(int method,
                     uharc_dec_read_cb rcb, void *rctx,
                     uharc_dec_write_cb wcb, void *wctx,
                     unsigned long out_len)
{
    return uharc_dec_decode_v(method, 0, 6, rcb, rctx, wcb, wctx, out_len);
}
