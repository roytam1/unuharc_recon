/* UHARC 0.2/0.3 core 2 (LZW, methods 2..7) - LSB-first, C89 port.
 *
 * This is the *original* UHARC 0.2/0.3 LZW coder (plain LZW with a
 * parent/char dictionary), NOT the 0.6 tree coder: 0.6's core2 uses a
 * different container (5-byte tree nodes, hash table, prune) and an
 * MSB-first bit order.  The two only look alike in the version-0.6
 * sources; 0.2/0.3/0.4 archives do not decode with that algorithm.
 *
 * Reversed from:
 *   0.2: Uharc02.lst sub_1A44C (main), sub_1A32C (next code),
 *        sub_1A400 (byte), sub_19BAC (emit1), sub_19C90 (fill),
 *        sub_19DC4 (copy)
 *   0.3: uharc_u.lst sub_41EEC4 (main), sub_41EDA4 (next code),
 *        sub_41EE78 (byte), sub_41E3BC (emit1), sub_41E4A0 (fill),
 *        sub_41E5D4 (copy), method table sub_41EB58 (2..7 -> widths)
 *
 * Stream format (all bits LSB-first within each byte):
 *
 *   code width starts at 9 bits; ncode = 1<<width, maxcode = (1<<maxbits)-1
 *   with maxbits = method + 8.
 *
 *   A code is read as (width-1) bits; if the value is >= ncode - var2 - 1
 *   one more bit is read and, when set, the value is complemented as
 *   (ncode-1-value).  var2 tracks the encoder's next-free counter and is
 *   capped at maxcode.  The complement scheme encodes the top of the code
 *   range (including 0x100/0x101) in one extra bit.
 *
 *   code <  0x100 : literal / dictionary string (added to the table)
 *   code == 0x100 : reset (width 9, table empty, read+emit one literal)
 *   code == 0x101 : RLE block: 8-bit len_hi, 8-bit len_lo, 8-bit value;
 *                   on the first block after a reset/non-RLE code an
 *                   extra 8-bit literal is read and emitted, and the next
 *                   string then omits its first (already emitted) byte
 *   code >= 0x102 : dictionary string; when it equals nextFree it is the
 *                   usual "previous string + first char of previous"
 *                   (KwKwK) case
 *
 * The original builds each string in a scratch area above the output
 * pointer and slides it down; this port walks the same parent/char chain
 * into ctx->tmp and emits it directly (streaming writer).
 */
#include <stdlib.h>
#include "uharc_dec.h"

typedef struct uharc_dec2_ctx_s {
    unsigned b;                /* 1 << maxbits (= maxcode + 1) */
    unsigned maxbits;
    unsigned short *parent;
    unsigned char *ch;
    unsigned char *tmp;
} uharc_dec2_ctx;

/* ---- public init/free ---- */
int uharc_dec2l_init(void **pctx, int method)
{
    uharc_dec2_ctx *c;
    unsigned b;
    unsigned short *parent;
    unsigned char *ch;
    unsigned char *tmp;
    unsigned i;

    if (!pctx || method < 2 || method > 7)
        return 1;
    b = 1u << (unsigned)(method + 8);
    c = (uharc_dec2_ctx *)malloc(sizeof(*c));
    if (!c)
        return 2;
    parent = (unsigned short *)malloc(sizeof(unsigned short) * (b ? b : 1u));
    ch = (unsigned char *)malloc(b ? b : 1u);
    tmp = (unsigned char *)malloc(b ? b : 1u);
    if (!parent || !ch || !tmp) {
        free(parent);
        free(ch);
        free(tmp);
        free(c);
        return 2;
    }
    for (i = 0; i < b; i++) {
        parent[i] = 0;
        ch[i] = (unsigned char)(i & 0xFFu);
        tmp[i] = 0;
    }
    c->b = b;
    c->maxbits = (unsigned)(method + 8);
    c->parent = parent;
    c->ch = ch;
    c->tmp = tmp;
    *pctx = (void *)c;
    return 0;
}

void uharc_dec2l_free(void *ctx)
{
    uharc_dec2_ctx *c = (uharc_dec2_ctx *)ctx;
    if (!c)
        return;
    free(c->parent);
    free(c->ch);
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
    unsigned long bitbuf;
    unsigned bitcnt;
} dec2_io;

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

/* emit one byte; stops at the expected size (the RLE/string may
 * legitimately overrun the last member of a solid stream) */
static void dec2_emit1(dec2_io *s, unsigned b)
{
    if (s->err || s->out_total >= s->expected)
        return;
    s->outb[s->out_fill++] = (unsigned char)(b & 0xFFu);
    s->out_total++;
    if (s->out_fill == s->out_cap)
        dec2_flush(s);
}

/* memset-like fill, clamped to expected */
static void dec2_emit_fill(dec2_io *s, unsigned long n, unsigned b)
{
    unsigned char v;
    v = (unsigned char)(b & 0xFFu);
    if (n > s->expected - s->out_total)
        n = s->expected - s->out_total;
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

/* LSB-first bit reader (sub_1A32C / sub_41EDA4 use the same layout:
 * bit 0 of each input byte is consumed first) */
static unsigned dec2l_bits(dec2_io *s, unsigned n)
{
    unsigned long mask;
    unsigned long v;
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
        s->bitbuf |= (unsigned long)(unsigned)c << s->bitcnt;
        s->bitcnt += 8;
    }
    if (n >= 32)
        mask = 0xFFFFFFFFUL;
    else
        mask = (1UL << n) - 1UL;
    v = s->bitbuf & mask;
    s->bitbuf >>= n;
    s->bitcnt -= n;
    return (unsigned)v;
}

/* one code: w1 = width-1 bits, plus the optional range bit */
static unsigned dec2l_next(dec2_io *s, unsigned w1, unsigned ncode,
                           unsigned var2)
{
    unsigned v;
    unsigned thresh;
    v = dec2l_bits(s, w1);
    if (s->err)
        return 0;
    thresh = (var2 < ncode) ? (ncode - var2) : 0u;
    if (v + 1u >= thresh) {
        if (dec2l_bits(s, 1u) != 0u)
            v = (ncode - 1u) - v;
    }
    return v;
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
    unsigned maxcode;
    unsigned bits;      /* current code width (starts at 9) */
    unsigned ncode;     /* 1 << bits */
    unsigned w1;        /* bits - 1: normal code length */
    unsigned nextFree;  /* first free dictionary slot (0x102..) */
    unsigned var2;      /* encoder's next-free counter (incl. 1 extra) */
    unsigned prev;      /* previous code */
    unsigned cl;        /* first char of the previous code's string */
    unsigned pending;   /* RLE supplied the next string's first byte */
    unsigned i;

    if (!c || !rcb || !wcb)
        return 1;
    if (out_len == 0)
        return 0;
    if (c->b == 0 || !c->parent || !c->ch || !c->tmp)
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

    maxcode = c->b - 1u;
    bits = 9u;
    ncode = 1u << bits;
    w1 = bits - 1u;
    nextFree = 0x102u;
    var2 = 0x102u;
    prev = 0x100u;
    cl = 0u;
    pending = 0u;

    if (maxcode < 0x102u) {   /* methods 2..7 always give >= 0x3FF */
        s.err = 7;
    } else {
        /* start: read the first code and emit its low byte.  The
         * original (sub_1A44C / sub_41EEC4) does not add a table entry
         * for it and leaves nextFree at 0x102. */
        unsigned code;
        code = dec2l_next(&s, w1, ncode, var2);
        if (!s.err) {
            cl = code & 0xFFu;
            prev = code;
            var2++;
            dec2_emit1(&s, cl);
        }
    }

    while (s.out_total < s.expected && !s.err) {
        unsigned code;
        unsigned orig;
        unsigned special;
        unsigned len;
        unsigned cur;
        unsigned root;

        /* ---- next code (sub_41EDA4 / sub_1A32C) ---- */
        code = dec2l_next(&s, w1, ncode, var2);
        if (s.err)
            break;

        if (code == 0x100u) {
            /* reset: empty table, 9-bit codes. The following code is
             * read and emitted immediately as a literal. */
            bits = 9u;
            ncode = 1u << bits;
            w1 = bits - 1u;
            nextFree = 0x102u;
            var2 = 0x102u;
            code = dec2l_next(&s, w1, ncode, var2);
            if (s.err)
                break;
            cl = code & 0xFFu;
            prev = code;
            var2++;
            dec2_emit1(&s, cl);
            continue;
        }

        if (code == 0x101u) {
            /* RLE block (sub_41EFFB / sub_1A57F): len_hi, len_lo,
             * value; optionally one literal byte (first block). */
            unsigned hi;
            unsigned lo;
            unsigned val;
            hi = dec2l_bits(&s, 8u);
            lo = dec2l_bits(&s, 8u);
            val = dec2l_bits(&s, 8u);
            if (s.err)
                break;
            if (pending == 0u) {
                unsigned extra;
                extra = dec2l_bits(&s, 8u);
                if (s.err)
                    break;
                dec2_emit1(&s, extra);
                pending = 1u;
            }
            dec2_emit_fill(&s, ((unsigned long)hi << 8) | lo, val);
            continue;
        }

        /* ---- string (literal or dictionary code) ---- */
        orig = code;
        special = (code >= nextFree) ? 1u : 0u;
        if (special)
            code = prev;

        len = 0;
        cur = code;
        while (cur >= 0x102u) {
            if (len >= c->b || cur >= c->b) {
                s.err = 7;
                break;
            }
            c->tmp[len++] = c->ch[cur];
            cur = c->parent[cur];
        }
        if (s.err)
            break;
        if (cur >= c->b) {
            s.err = 7;
            break;
        }
        root = cur & 0xFFu;
        c->tmp[len++] = (unsigned char)root;
        cl = root;

        /* tmp holds the string reversed: last..first at 0..len-1.
         * Emit forward (highest index first).  When the RLE supplied
         * the first byte, skip it. */
        if (pending) {
            pending = 0u;
            if (len >= 2u)
                for (i = len - 1u; i-- > 0u; )
                    dec2_emit1(&s, c->tmp[i]);
        } else {
            for (i = len; i-- > 0u; )
                dec2_emit1(&s, c->tmp[i]);
        }
        if (s.err)
            break;
        if (special) {
            /* KwKwK: previous string + its first char, appended last */
            dec2_emit1(&s, cl);
            if (s.err)
                break;
        }

        /* ---- add table entry: parent = prev, char = first byte ---- */
        if (nextFree < c->b) {
            c->parent[nextFree] = (unsigned short)prev;
            c->ch[nextFree] = (unsigned char)root;
        }
        nextFree++;
        if (nextFree == ncode && bits < c->maxbits) {
            ncode <<= 1;
            bits++;
            w1 = bits - 1u;
        }
        prev = orig;
        if (var2 <= maxcode)
            var2++;
    }

    if (!s.err && s.out_fill)
        dec2_flush(&s);
    free(inb);
    free(outb);
    return s.err;
}
