/* UHARC core 2 (Huffman/LZ, methods 2..7) - LSB-first bit order.
 *
 * Byte-identical algorithm to uharc_dec2.c (0.6 core2) except the bit
 * reader consumes least-significant bits first.
 *
 * VERIFIED 2026-10-24 against real archives: decodes 0.2 tab1/tab2/tab3
 * (names EMPTY.BIN/REPEAT.TXT/T1.TXT/T2.TXT, dates 5D58, times 963B) and
 * 0.3 + 0.4 tab2 (crc16 25CA exact, 6x date 5D58 + 6x time 5E34).
 * The MSB-first variant fails all of these, so 0.2/0.3/0.4 core2 is
 * LSB-first while 0.6 is MSB-first.
 *
 * Originates from UNUHARC06 sub_40B5F1 (init), sub_40B67F (free),
 * sub_40B6BA (reset), sub_40B704 (bit reader - reversed here),
 * sub_40B757 (RLE 257), sub_40B7F3 (next code), sub_40B8DF (main),
 * sub_40BA80 (prune), plus helpers sub_40B093/40B0DA/40B178/40B246/40B31F.
 *
 * Original globals mapped to context struct (no globals):
 *  dword_434548 -> ctx->b
 *  dword_422DA8 -> ctx->tree (5*B bytes, node = w0:u16 @+0, w2:u16 @+2, ch:u8 @+4)
 *  dword_422DAC -> ctx->tmp  (B bytes, reverse path buffer)
 *  dword_422DB0/DB4 -> bitbuf/bitcnt (local to decode)
 *  dword_43454C/34/50/54 -> nextFree/prev/flag/bits (local to decode)
 *  dword_434528/514 -> out_total/expected
 *  dword_422B98/9C etc -> inb/outb streaming buffers (local to decode)
 * Progress sub_40819B is stubbed (no-op).
 */
#include <stdlib.h>
#include <string.h>
#include "uharc_dec.h"

typedef struct uharc_dec2_ctx_s {
    unsigned b;
    unsigned char *tree;
    unsigned char *tmp;
} uharc_dec2_ctx;

/* ---- tree access (little-endian, 5-byte stride) ---- */
static unsigned t_get_w0(unsigned char *t, unsigned i)
{
    unsigned o = i * 5u;
    return (unsigned)(t[o] | ((unsigned)t[o + 1] << 8));
}
static void t_set_w0(unsigned char *t, unsigned i, unsigned v)
{
    unsigned o = i * 5u;
    t[o] = (unsigned char)(v & 0xFFu);
    t[o + 1] = (unsigned char)((v >> 8) & 0xFFu);
}
static unsigned t_get_w2(unsigned char *t, unsigned i)
{
    unsigned o = i * 5u;
    return (unsigned)(t[o + 2] | ((unsigned)t[o + 3] << 8));
}
static void t_set_w2(unsigned char *t, unsigned i, unsigned v)
{
    unsigned o = i * 5u;
    t[o + 2] = (unsigned char)(v & 0xFFu);
    t[o + 3] = (unsigned char)((v >> 8) & 0xFFu);
}
static unsigned t_get_ch(unsigned char *t, unsigned i)
{
    return (unsigned)t[i * 5u + 4];
}
static void t_set_ch(unsigned char *t, unsigned i, unsigned v)
{
    t[i * 5u + 4] = (unsigned char)(v & 0xFFu);
}

/* ---- public init/free ---- */
int uharc_dec2l_init(void **pctx, int method)
{
    uharc_dec2_ctx *c;
    unsigned b;
    unsigned char *tree;
    unsigned char *tmp;
    if (!pctx || method < 2 || method > 7)
        return 1;
    b = 1u << (unsigned)(method + 8);
    c = (uharc_dec2_ctx *)malloc(sizeof(*c));
    if (!c)
        return 2;
    tree = (unsigned char *)malloc(5u * b);
    tmp = (unsigned char *)malloc(b ? b : 1u);
    if (!tree || !tmp) {
        free(tree);
        free(tmp);
        free(c);
        return 2;
    }
    memset(tree, 0, 5u * b);
    if (b)
        memset(tmp, 0, b);
    c->b = b;
    c->tree = tree;
    c->tmp = tmp;
    *pctx = (void *)c;
    return 0;
}

void uharc_dec2l_free(void *ctx)
{
    uharc_dec2_ctx *c = (uharc_dec2_ctx *)ctx;
    if (!c)
        return;
    free(c->tree);
    free(c->tmp);
    free(c);
}

/* ---- decode I/O state ---- */
typedef struct dec2_io_s {
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
    unsigned long expected;
    int err;
    unsigned bitbuf;
    unsigned bitcnt;
} dec2_io;

static void dec2_progress(unsigned long a, unsigned long b)
{
    (void)a;
    (void)b;
}

static int dec2_refill(dec2_io *s)
{
    int n;
    s->in_pos = 0;
    s->in_avail = 0;
    n = s->rcb(s->rctx, s->inb, s->in_cap);
    if (n < 0)
        n = 0;
    if (n == 0) {
        s->err = 3;
        return 0;
    }
    s->in_avail = (unsigned)n;
    return 1;
}

static int dec2_get_byte(dec2_io *s)
{
    int c;
    if (s->err)
        return 0;
    if (s->in_pos >= s->in_avail) {
        if (!dec2_refill(s))
            return 0;
    }
    c = s->inb[s->in_pos++];
    return c & 0xFF;
}

static int dec2_flush(dec2_io *s)
{
    int w;
    if (s->out_fill == 0)
        return 1;
    w = s->wcb(s->wctx, s->outb, s->out_fill);
    if (w != 0) {
        s->err = 4;
        return 0;
    }
    s->out_fill = 0;
    return 1;
}

/* sub_40B178 */
static void dec2_emit1(dec2_io *s, unsigned b)
{
    if (s->err)
        return;
    s->outb[s->out_fill++] = (unsigned char)(b & 0xFFu);
    s->out_total++;
    if (s->out_fill == s->out_cap)
        dec2_flush(s);
}

/* sub_40B246: memset with flush */
static void dec2_emit_fill(dec2_io *s, unsigned long n, unsigned b)
{
    unsigned char v;
    v = (unsigned char)(b & 0xFFu);
    while (n > 0 && !s->err) {
        unsigned space;
        unsigned k;
        unsigned i;
        if (s->out_fill == s->out_cap) {
            if (!dec2_flush(s))
                break;
        }
        space = s->out_cap - s->out_fill;
        k = n < (unsigned long)space ? (unsigned)n : space;
        for (i = 0; i < k; i++)
            s->outb[s->out_fill + i] = v;
        s->out_fill += k;
        s->out_total += k;
        n -= k;
        if (s->out_fill == s->out_cap) {
            if (!dec2_flush(s))
                break;
        }
    }
}

/* sub_40B31F: memcpy with flush */
static void dec2_emit_copy(dec2_io *s, unsigned long n, unsigned char *src)
{
    while (n > 0 && !s->err) {
        unsigned space;
        unsigned k;
        if (s->out_fill == s->out_cap) {
            if (!dec2_flush(s))
                break;
        }
        space = s->out_cap - s->out_fill;
        k = n < (unsigned long)space ? (unsigned)n : space;
        memcpy(s->outb + s->out_fill, src, k);
        s->out_fill += k;
        s->out_total += k;
        src += k;
        n -= (unsigned long)k;
        if (s->out_fill == s->out_cap) {
            if (!dec2_flush(s))
                break;
        }
    }
}

/* sub_40B704 */
static unsigned dec2l_bits(dec2_io *s, unsigned n)
{
    unsigned mask;
    if (s->err)
        return 0;
    if (n == 0)
        return 0;
    while (n > s->bitcnt) {
        int c;
        if (s->err)
            return 0;
        c = dec2_get_byte(s);
        if (s->err)
            return 0;
        s->bitbuf |= (unsigned)c << s->bitcnt;
        s->bitcnt += 8;
    }
    if (n >= 32)
        mask = 0xFFFFFFFFUL;
    else
        mask = (unsigned)(((1UL << n) - 1UL) & 0xFFFFFFFFUL);
    { unsigned v = s->bitbuf & mask; s->bitbuf >>= n; s->bitcnt -= n; return v; }
}

/* sub_40BA80 prune; *pM = nextFree in/out */
static void dec2_prune(uharc_dec2_ctx *c, unsigned *pM)
{
    unsigned m;
    unsigned v0;
    unsigned i;
    m = *pM;
    /* phase A: top-down, free trailing leaves, build child lists */
    if (m > 0) {
        v0 = m - 1;
        for (;;) {
            unsigned parent;
            if (v0 < 0x102u)
                break;
            parent = t_get_w2(c->tree, v0);
            if (parent < c->b) {
                if (t_get_w0(c->tree, parent) == 0x100u)
                    t_set_w0(c->tree, parent, 0u);
            }
            if (t_get_w0(c->tree, v0) == 0x100u) {
                t_set_w0(c->tree, v0, 0u);
                if (v0 < m)
                    m = v0;
            } else {
                unsigned tmp;
                if (t_get_w0(c->tree, v0) == 0u)
                    t_set_w0(c->tree, v0, 0x100u);
                if (parent < c->b) {
                    if (t_get_w0(c->tree, parent) == 0u)
                        t_set_w0(c->tree, parent, 0x100u);
                    tmp = t_get_w0(c->tree, parent);
                    t_set_w2(c->tree, v0, tmp);
                    t_set_w0(c->tree, parent, v0);
                }
            }
            if (v0 == 0)
                break;
            v0--;
        }
    }
    /* phase B: restore first 258 */
    for (i = 0; i < 0x102u; i++) {
        if (t_get_w0(c->tree, i) == 0u)
            t_set_w0(c->tree, i, 0x100u);
    }
    /* phase C: compact links pointing past m */
    i = 0;
    while (i < m) {
        unsigned cc;
        cc = t_get_w0(c->tree, i);
        if (cc != 0x100u && cc > m) {
            if (cc < c->b && m < c->b) {
                unsigned k;
                for (k = 0; k < 5u; k++)
                    c->tree[m * 5u + k] = c->tree[cc * 5u + k];
                t_set_w0(c->tree, i, m);
                t_set_w0(c->tree, cc, 0u);
                do {
                    m++;
                } while (m < c->b && t_get_w0(c->tree, m) != 0u);
            }
        }
        cc = t_get_w2(c->tree, i);
        if (cc != 0x100u && cc > m) {
            if (cc < c->b && m < c->b) {
                unsigned k;
                for (k = 0; k < 5u; k++)
                    c->tree[m * 5u + k] = c->tree[cc * 5u + k];
                t_set_w2(c->tree, i, m);
                t_set_w0(c->tree, cc, 0u);
                do {
                    m++;
                } while (m < c->b && t_get_w0(c->tree, m) != 0u);
            }
        }
        i++;
        if (i >= c->b)
            break;
        if (m > c->b)
            break;
    }
    /* phase D: child lists -> parent pointers */
    {
        unsigned res = 0;
        while (res < m) {
            unsigned cur;
            unsigned guard;
            cur = t_get_w0(c->tree, res);
            t_set_w0(c->tree, res, 0x100u);
            guard = 0;
            while (cur != 0x100u) {
                unsigned nxt;
                if (cur >= c->b)
                    break;
                nxt = t_get_w2(c->tree, cur);
                t_set_w2(c->tree, cur, res);
                cur = nxt;
                guard++;
                if (guard > c->b + 16u)
                    break;
            }
            res++;
        }
    }
    *pM = m;
}

int uharc_dec2l_decode(void *ctx,
                      uharc_dec_read_cb rcb, void *rctx,
                      uharc_dec_write_cb wcb, void *wctx,
                      unsigned long out_len)
{
    uharc_dec2_ctx *c = (uharc_dec2_ctx *)ctx;
    dec2_io s;
    unsigned char *inb = 0;
    unsigned char *outb = 0;
    unsigned nextFree; /* 43454C */
    unsigned prev;     /* 434534 */
    unsigned flag;     /* 434550 */
    unsigned bits;     /* 434554 (byte) */
    unsigned i;

    if (!c || !rcb || !wcb)
        return 1;
    if (out_len == 0)
        return 0;
    if (c->b == 0 || !c->tree || !c->tmp)
        return 1;

    inb = (unsigned char *)malloc(0x10000u);
    outb = (unsigned char *)malloc(0x10000u);
    if (!inb || !outb) {
        free(inb);
        free(outb);
        return 2;
    }

    s.rcb = rcb;
    s.rctx = rctx;
    s.wcb = wcb;
    s.wctx = wctx;
    s.inb = inb;
    s.in_pos = 0;
    s.in_avail = 0;
    s.in_cap = 0x10000u;
    s.outb = outb;
    s.out_fill = 0;
    s.out_cap = 0x10000u;
    s.out_total = 0;
    s.expected = out_len;
    s.err = 0;
    s.bitbuf = 0;
    s.bitcnt = 0;

    /* sub_40B6BA reset */
    for (i = 0; i < 0x102u; i++) {
        t_set_w0(c->tree, i, 0x100u);
        t_set_w2(c->tree, i, 0x100u);
        t_set_ch(c->tree, i, i & 0xFFu);
    }
    nextFree = 0x101u;
    prev = 0x100u;
    flag = 0u;
    bits = 9u;

    while (s.out_total < s.expected && !s.err) {
        unsigned code; /* v1/esi */
        unsigned ncode;
        unsigned direct;
        unsigned sym;
        unsigned off;
        unsigned cur;
        unsigned rootb;

        /* sub_40B7F3 next code */
        ncode = 1u << bits;
        if (flag != 0u && bits != 9u) {
            if (dec2l_bits(&s, 1u) != 0u) {
                unsigned v;
                unsigned thresh;
                v = dec2l_bits(&s, bits - 1u);
                thresh = ncode - (nextFree + 1u);
                if (v + 1u >= thresh) {
                    if (dec2l_bits(&s, 1u) != 0u)
                        v = (ncode - 1u) - v;
                }
                flag >>= 1;
                code = v + 256u;
            } else {
                code = dec2l_bits(&s, 8u);
                flag += 15u;
            }
        } else {
            unsigned v;
            unsigned thresh;
            v = dec2l_bits(&s, bits - 1u);
            thresh = ncode - (nextFree + 1u);
            if (v + 1u >= thresh) {
                if (dec2l_bits(&s, 1u) != 0u)
                    v = (ncode - 1u) - v;
            }
            if (v < 0x100u && bits != 9u)
                flag = 1u;
            code = v;
        }
        if (s.err)
            break;

        if (code > nextFree)
            continue;
        if (code == 0x100u) {
            /* reset */
            for (i = 0; i < 0x102u; i++) {
                t_set_w0(c->tree, i, 0x100u);
                t_set_w2(c->tree, i, 0x100u);
                t_set_ch(c->tree, i, i & 0xFFu);
            }
            nextFree = 0x101u;
            prev = 0x100u;
            flag = 0u;
            bits = 9u;
            continue;
        }
        if (code == 0x101u) {
            /* sub_40B757 RLE block */
            unsigned long len = 0;
            unsigned bval;
            if (dec2l_bits(&s, 1u) != 0u) {
                len = (unsigned long)dec2l_bits(&s, 8u);
            } else if (dec2l_bits(&s, 1u) != 0u) {
                len = (unsigned long)dec2l_bits(&s, 12u);
                len += 0x100UL;
            } else if (dec2l_bits(&s, 1u) != 0u) {
                len = (unsigned long)dec2l_bits(&s, 16u);
                len += 0x1100UL;
            } else {
                unsigned lo;
                unsigned hi;
                lo = dec2l_bits(&s, 16u);
                hi = dec2l_bits(&s, 16u);
                len = (unsigned long)lo + ((unsigned long)hi << 16);
                len += 0x1100UL;
            }
            if (s.err)
                break;
            len += 0x100UL;
            bval = dec2l_bits(&s, 8u);
            if (s.err)
                break;
            dec2_emit_fill(&s, len, bval);
            continue;
        }

        direct = (code < nextFree || prev == 0x100u) ? 1u : 0u;
        sym = direct ? code : prev;

        /* walk parent chain, collect reversed in tmp */
        off = c->b;
        cur = sym;
        while (t_get_w2(c->tree, cur) != 0x100u) {
            unsigned pw;
            if (off == 0) {
                s.err = 7;
                break;
            }
            pw = t_get_w2(c->tree, cur);
            if (pw >= c->b) {
                s.err = 7;
                break;
            }
            off--;
            c->tmp[off] = (unsigned char)t_get_ch(c->tree, cur);
            cur = pw;
            if (cur >= c->b) {
                s.err = 7;
                break;
            }
        }
        if (s.err)
            break;
        rootb = t_get_ch(c->tree, cur);
        if (off == c->b) {
            dec2_emit1(&s, rootb);
        } else {
            unsigned long ln;
            if (off == 0) {
                s.err = 7;
                break;
            }
            off--;
            c->tmp[off] = (unsigned char)rootb;
            ln = (unsigned long)(c->b - off);
            dec2_emit_copy(&s, ln, c->tmp + off);
        }
        if (s.err)
            break;
        if (!direct)
            dec2_emit1(&s, rootb);
        if (s.err)
            break;
        if (prev != 0x100u) {
            unsigned idx = nextFree;
            if (idx < c->b) {
                t_set_w0(c->tree, idx, 0x100u);
                t_set_ch(c->tree, idx, rootb);
                t_set_w2(c->tree, idx, prev);
            }
        }
        nextFree++;
        if ((1u << bits) == nextFree)
            bits++;
        prev = code;
        if (nextFree == c->b) {
            dec2_prune(c, &nextFree);
            while (bits > 1 && ((1u << (bits - 1u)) >= nextFree))
                bits--;
            prev = 0x100u;
            if (nextFree > 0)
                nextFree--;
        }
        if ((s.out_total & 0xFu) == 0u)
            dec2_progress(s.out_total, s.out_total);
    }

    if (!s.err && s.out_fill)
        dec2_flush(&s);
    free(inb);
    free(outb);
    return s.err;
}
