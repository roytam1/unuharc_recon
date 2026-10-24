/* UHARC 0.2 decompressor core 3 (methods 8..43, PPM/LZ + range coder).
 * Portable C89 port of the DOS32 decoder in Uharc02.lst / Uharc02.c.
 * All former DGROUP state lives in one malloc'd context; no globals.
 *
 * Method mapping: group 1 = 8..19, group 2 = 20..31, group 3 = 32..43.
 * Dict size = 1 << (((m - 8) % 12) + 10).
 *
 * Original -> port (cseg01 addresses in Uharc02.lst):
 *   sub_18AC8 (init)          d3_init
 *   sub_1BE94 (main loop)     uharc_dec3t_decode
 *   sub_1C600 (token)         d3_token
 *   sub_1C870 (literal)       d3_literal
 *   sub_1C938 (LZ match)      d3_match
 *   sub_1CA90 (byte run)      d3_run
 *   sub_1C6C0 (control)       d3_control
 *   sub_1BEFC/1BF6C           d3_rupd / d3_rget
 *   sub_19A38 (bit in)        d3_bit
 *   sub_1B5D0 (emit literal)  d3_put_orig
 *   sub_1B66C (emit copy)     d3_put_trans
 *   sub_1B8D4 (model update)  d3_model_update
 *   sub_1B7B8 (model rebuild) d3_model_rebuild
 *   sub_1B710 (halve ctx)     d3_model_halve
 *   sub_1C2CC/1C470           d3_dec_tree / d3_dec_tree2
 *   sub_1BF8C (length)        d3_dec_len
 *   sub_1BA30/1BAC4/1BC24/1BCC0  length/distance model updates
 *   sub_1BD60/1BDEC           state/length class updates
 *   sub_19C90 (fill run)      d3_fill_out
 *
 * Return codes follow uharc_dec.h: 0 ok, 1 bad args, 2 nomem, 3 input
 * short, 4 output short, 7 data error.
 *
 * Initial context model: a fresh context block is the Fenwick tree of a
 * uniform 256-symbol distribution (tree[0] = 256, tree[k] = lowbit(k)).
 * This is what the 0.2 streams in the test vectors use.  The listing's
 * 512-byte template at dseg02:3A6BC (copied by sub_18AC8) is a different
 * prior; feeding it instead shifts the first decoded literal (and fails
 * the vectors), so the uniform build is used for both initial setup and
 * model reset.  See the header note in the test report.
 *
 * Verified: 02_M1 (method 9), 02_M2 (21), 02_M3 (33) each decode the
 * 1243-byte solid file stream byte-exactly, CRCs 9166788F/3CD931D7/
 * 5FC97467.  uha02.UHA (method 25, 31447 bytes) matches 6 of its 7
 * per-file CRCs; the 29696-byte binary entry differs in 10 bytes
 * (localized: 1102-1105, 6457-6458, 14298-14299, 14340-14341), so a
 * rare model/context path remains unresolved for some binary data.
 */
#include <stdlib.h>
#include <string.h>

#include "uharc_dec.h"


/* ------------------------------------------------------------------ */
/* static tables extracted from dseg02 (Uharc02.lst)                   */
/* ------------------------------------------------------------------ */

static const unsigned short d3_ctl_init[13] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000A, 0x000B,
    0x000C
};

static const unsigned short d3_state_init[5] = { 0x0000, 0x0001, 0x0002, 0x0C00, 0x2000 };

static const unsigned short d3_len_init[23] = {
    0x0000, 0x0010, 0x0018, 0x0028, 0x0038, 0x0058, 0x007C, 0x00CC, 0x0130, 0x018C, 0x0244, 0x02EF,
    0x0401, 0x04C9, 0x05CD, 0x071F, 0x08E1, 0x0B39, 0x0E59, 0x1241, 0x17B9, 0x2119, 0x2710
};

static const unsigned short d3_dmodel_init[45] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0200, 0x0400, 0x0600, 0x0800, 0x0A00, 0x0C00, 0x0E00, 0x1000
};

static const unsigned short d3_lenclass_init[4] = {
    0x0000, 0x0780, 0x0C80, 0x0F00
};

static const unsigned short d3_small_init[4] = { 0x1000, 0x0000, 0x1194, 0x238C };

static const unsigned short d3_d2_init[14] = {
    0x0000, 0x0190, 0x0352, 0x0546, 0x076C, 0x09C4, 0x0C4E, 0x0F0A, 0x11F8, 0x1518, 0x186A, 0x1BEE,
    0x1FA4, 0x238C
};

static const unsigned char d3_lenbase[22] = {
    0xFF, 0xBF, 0x7F, 0x5F, 0x3F, 0x2F, 0x1F, 0x17, 0x13, 0x0F, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
    0x07, 0x06, 0x05, 0x04, 0x03, 0x02
};

static const unsigned char d3_lenbits[22] = {
    0x00, 0x40, 0x40, 0x20, 0x20, 0x10, 0x10, 0x08, 0x04, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char d3_de8[16] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00
};

static const unsigned char d3_df8[16] = {
    0x00, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04
};

static const unsigned char d3_ef8[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A, 0x0A,
    0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D,
    0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E,
    0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
    0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12,
    0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13,
    0x13, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x16, 0x16, 0x16, 0x17, 0x17, 0x18, 0x18, 0x19, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

static const unsigned char d3_excl[16] = {
    0x00, 0x00, 0x01, 0x02, 0x02, 0x00, 0x00, 0x01, 0x04, 0x02, 0x00, 0x00, 0x06, 0x04, 0x02, 0x00
};

static const unsigned char d3_db[4] = {
    0x00, 0x05, 0x08, 0x10
};

static const unsigned long d3_distbase[44] = {
    0x001E0000UL, 0x001C0000UL, 0x001A0000UL, 0x00180000UL, 0x00160000UL, 0x00140000UL,
    0x00120000UL, 0x00100000UL, 0x000E0000UL, 0x000C0000UL, 0x000A0000UL, 0x00080000UL,
    0x00060000UL, 0x00040000UL, 0x00030000UL, 0x00020000UL, 0x00018000UL, 0x00010000UL,
    0x0000C000UL, 0x00008000UL, 0x00006000UL, 0x00004000UL, 0x00003000UL, 0x00002000UL,
    0x00001800UL, 0x00001000UL, 0x00000C00UL, 0x00000800UL, 0x00000600UL, 0x00000400UL,
    0x00000300UL, 0x00000200UL, 0x00000100UL, 0x00000080UL, 0x00000040UL, 0x00000020UL,
    0x00000010UL, 0x00000008UL, 0x00000004UL, 0x00000002UL, 0x00000001UL, 0x00000003UL,
    0x00000001UL, 0x00000000UL
};

static const unsigned long d3_distspan[40] = {
    0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL,
    0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL,
    0x00020000UL, 0x00020000UL, 0x00010000UL, 0x00010000UL, 0x00008000UL, 0x00008000UL,
    0x00004000UL, 0x00004000UL, 0x00002000UL, 0x00002000UL, 0x00001000UL, 0x00001000UL,
    0x00000800UL, 0x00000800UL, 0x00000400UL, 0x00000400UL, 0x00000200UL, 0x00000200UL,
    0x00000100UL, 0x00000100UL, 0x00000100UL, 0x00000080UL, 0x00000040UL, 0x00000020UL,
    0x00000010UL, 0x00000008UL, 0x00000004UL, 0x00000002UL
};

static const unsigned long d3_fillthr[37] = {
    0xFFFFFFFFUL, 0x001E0000UL, 0x001C0000UL, 0x001A0000UL, 0x00180000UL, 0x00160000UL,
    0x00140000UL, 0x00120000UL, 0x00100000UL, 0x000E0000UL, 0x000C0000UL, 0x000A0000UL,
    0x00080000UL, 0x00060000UL, 0x00040000UL, 0x00030000UL, 0x00020000UL, 0x00018000UL,
    0x00010000UL, 0x0000C000UL, 0x00008000UL, 0x00006000UL, 0x00004000UL, 0x00003000UL,
    0x00002000UL, 0x00001800UL, 0x00001000UL, 0x00000C00UL, 0x00000800UL, 0x00000600UL,
    0x00000400UL, 0x00000300UL, 0x00000200UL, 0x00000100UL, 0x00000080UL, 0x00000040UL,
    0x00000020UL
};


/* ------------------------------------------------------------------ */
/* context                                                             */
/* ------------------------------------------------------------------ */
typedef struct d3ctx_s {
    uharc_dec_read_cb rcb;
    void *rctx;
    uharc_dec_write_cb wcb;
    void *wctx;

    /* input, bit reader */
    unsigned char inbuf[8192];
    unsigned in_pos;
    unsigned in_len;
    int in_eof;
    unsigned char cur_byte;
    unsigned bit_mask;

    /* output */
    unsigned char outbuf[65536];
    unsigned out_fill;
    unsigned long out_total;
    unsigned long out_expect;

    int err;                    /* 0, 3 input short, 4 output, 7 data */

    /* range coder */
    unsigned long code, range;
    unsigned long rtotal, rlow, rhigh;

    /* model */
    unsigned short *tree;       /* 256 ctx * 256 words (Fenwick) */
    unsigned short *freq;       /* 256 ctx * 256 words */
    unsigned short state[8][5]; /* word_3C3A0 */
    unsigned short lenA[23];    /* unk_3C59A */
    unsigned short lenB[23];    /* unk_3C56C */
    unsigned short dmodel[45];  /* word_3C50C, total at 44 */
    unsigned short smallm[4];   /* 3C564..3C56A */
    unsigned short d2[14];      /* word_3C45C, total at byte_3C5ED */
    unsigned short lenclass[4]; /* word_3C3F0 */

    unsigned long *distlist;    /* dword_3AD8C */
    unsigned char *dict;        /* dword_3A580, dict_size + 256 */
    unsigned long dict_size;    /* dword_3C498 */
    unsigned long dict_wrap;    /* dword_3C48C = dict_size - 255 */
    unsigned long dict_mask;    /* dword_3C4A0 = dict_size - 1 */
    unsigned long ring_pos;     /* dword_3C490 */
    unsigned long ring_fill;    /* dword_3C49C */

    unsigned long hash;         /* dword_3C44C */
    unsigned long counter;      /* dword_3C478 */
    unsigned long match_pos;    /* dword_3C4A4 */
    unsigned long list_max;     /* byte_3C5EB (5/8/16) */
    unsigned long class_max;    /* byte_3C5ED (2/5/13) */
    unsigned char group;

    unsigned char state_idx;    /* byte_3C5EA */
    unsigned char prev_ctx;     /* byte_3C5EF */
    unsigned char last_out;     /* byte_3C5E1 */
    unsigned char last_sym;     /* byte_3C5EC */
    unsigned char mode;         /* byte_3C5E6 */
    unsigned char db;           /* byte_3C5DB */
    unsigned char order;        /* byte_3C5DC */
    unsigned char prev_order;   /* byte_3C5DF */
    unsigned char flag7;        /* byte_3C5E7 */
    unsigned char raw_lit;      /* byte_3C5E3 */
    unsigned char exc_lit;      /* byte_3C5E5 */
    unsigned char len_sel;      /* byte_3C5FA */
    unsigned char len_val;      /* byte_3C5FB */
    unsigned char fill_idx;     /* byte_3AB70 */
} d3ctx;

/* ------------------------------------------------------------------ */
/* I/O                                                                 */
/* ------------------------------------------------------------------ */
static int d3_in_byte(d3ctx *c)
{
    if (c->in_pos >= c->in_len) {
        int n;
        if (c->in_eof) { c->err = 3; return 0; }
        n = c->rcb(c->rctx, c->inbuf, (int)sizeof(c->inbuf));
        if (n <= 0) { c->in_eof = 1; c->err = 3; return 0; }
        c->in_len = (unsigned)n;
        c->in_pos = 0;
    }
    return c->inbuf[c->in_pos++];
}

static int d3_bit(d3ctx *c)
{
    int b;
    if (c->bit_mask == 0) {
        c->cur_byte = (unsigned char)d3_in_byte(c);
        c->bit_mask = 0x80;
    }
    b = (c->cur_byte & c->bit_mask) ? 1 : 0;
    c->bit_mask >>= 1;
    return b;
}

static void d3_flush(d3ctx *c)
{
    if (c->out_fill) {
        if (c->wcb(c->wctx, c->outbuf, c->out_fill) != 0)
            c->err = 4;
        c->out_fill = 0;
    }
}

static void d3_out_byte(d3ctx *c, unsigned char b)
{
    if (c->out_fill >= sizeof(c->outbuf)) d3_flush(c);
    c->outbuf[c->out_fill++] = b;
    c->out_total++;
}

static void d3_fill_out(d3ctx *c, unsigned char b, unsigned long n)
{
    while (n) {
        unsigned room = (unsigned)(sizeof(c->outbuf) - c->out_fill);
        unsigned take;
        if (room == 0) { d3_flush(c); room = (unsigned)sizeof(c->outbuf); }
        take = (n < room) ? (unsigned)n : room;
        memset(c->outbuf + c->out_fill, b, take);
        c->out_fill += take;
        c->out_total += take;
        n -= take;
    }
}

/* ------------------------------------------------------------------ */
/* range coder (sub_1BF6C / sub_1BEFC)                                 */
/* ------------------------------------------------------------------ */
static unsigned long d3_rget(d3ctx *c, unsigned long total)
{
    unsigned long num;
    if (total == 0) { c->err = 7; return 0; }
    num = (((c->code + 1) * total) - 1) & 0xFFFFFFFFUL;
    if (c->range == 0) { c->err = 7; return 0; }
    return (num / c->range) & 0xFFFFFFFFUL;
}

static void d3_rupd(d3ctx *c)
{
    unsigned long r = c->range;
    unsigned long lo, hi, span;
    if (c->rtotal == 0 || r == 0) { c->err = 7; return; }
    lo = ((c->rlow * r) & 0xFFFFFFFFUL) / c->rtotal;
    c->code = (c->code - lo) & 0xFFFFFFFFUL;
    hi = ((c->rhigh * r) & 0xFFFFFFFFUL) / c->rtotal;
    span = (hi - lo) & 0xFFFFFFFFUL;
    while (span <= 0x8000UL) {
        int b = d3_bit(c);
        span = (span << 1) & 0xFFFFFFFFUL;
        c->code = ((c->code << 1) + (unsigned long)b) & 0xFFFFFFFFUL;
        if (c->err) return;
    }
    c->range = span;
}

/* decode a byte from a 256-symbol context block: sub_1C2CC */
static unsigned d3_dec_tree(d3ctx *c, const unsigned short *w)
{
    unsigned long v;
    unsigned edx, idx, step, mask, i;
    unsigned char cl = 0;
    unsigned short total = w[0];

    edx = total;
    v = d3_rget(c, total);
    if (v < (unsigned long)((unsigned short)(edx - w[128]))) {
        idx = 128; cl = 0x80; edx = (unsigned)(edx - w[128]);
    } else {
        idx = 0;
    }
    step = 64; mask = 0x40;
    for (i = 0; i < 7; i++) {
        unsigned di = (unsigned)((edx - w[idx + step]) & 0xFFFF);
        if (v < (unsigned long)di) {
            idx += step;
            cl = (unsigned char)(cl | mask);
            edx = di;
        }
        step >>= 1;
        mask >>= 1;
    }
    c->rhigh = edx & 0xFFFF;
    if (cl == 0xFF) {
        c->rlow = 0;
    } else if (!(cl & 1)) {
        c->rlow = (unsigned)((edx - w[idx + 1]) & 0xFFFF);
    } else {
        unsigned c2 = (unsigned)((cl + 1) & 0xFF);
        unsigned long p = 0;
        unsigned val = total;
        mask = 0x80;
        for (i = 0; i < 8; i++) {
            if (c2 & mask) {
                p += mask * 2UL;
                val = (unsigned)((val - w[p >> 1]) & 0xFFFF);
            }
            mask >>= 1;
        }
        c->rlow = val & 0xFFFF;
    }
    c->rtotal = total;
    return (unsigned)cl;
}

/* decode a byte from the high half of a context block: sub_1C470 */
static unsigned d3_dec_tree2(d3ctx *c, const unsigned short *w)
{
    unsigned long v;
    unsigned edx, idx, step, mask, i;
    unsigned char cl = 0x80;
    unsigned short total;

    edx = (unsigned)((w[0] - w[128]) & 0xFFFF);
    total = (unsigned short)edx;
    v = d3_rget(c, edx);
    idx = 128;
    {
        unsigned di = (unsigned)((edx - w[192]) & 0xFFFF);
        if (v < (unsigned long)di) {
            cl = 0xC0; idx += 64; edx = di;
        }
    }
    step = 32; mask = 0x20;
    for (i = 0; i < 6; i++) {
        unsigned di = (unsigned)((edx - w[idx + step]) & 0xFFFF);
        if (v < (unsigned long)di) {
            idx += step;
            cl = (unsigned char)(cl | mask);
            edx = di;
        }
        step >>= 1;
        mask >>= 1;
    }
    c->rhigh = edx & 0xFFFF;
    if (cl == 0xFF) {
        c->rlow = 0;
    } else if (!(cl & 1)) {
        c->rlow = (unsigned)((edx - w[idx + 1]) & 0xFFFF);
    } else {
        unsigned c2 = (unsigned)((cl + 1) & 0xFF);
        unsigned long p = 0;
        unsigned val = w[0];
        mask = 0x80;
        for (i = 0; i < 8; i++) {
            if (c2 & mask) {
                p += mask * 2UL;
                val = (unsigned)((val - w[p >> 1]) & 0xFFFF);
            }
            mask >>= 1;
        }
        c->rlow = val & 0xFFFF;
    }
    c->rtotal = total;
    return (unsigned)cl;
}

/* ------------------------------------------------------------------ */
/* model update / rebuild                                              */
/* ------------------------------------------------------------------ */
static void d3_halve_pairs(unsigned short *p, unsigned n)
{
    /* exact dword trick from sub_1B710/sub_1B7B8:
       d = ((d + 0x10001) & 0xFFFEFFFE) >> 1 */
    unsigned i;
    for (i = 0; i + 1 < n; i += 2) {
        unsigned long d = (unsigned long)p[i] | ((unsigned long)p[i + 1] << 16);
        d = (d + 0x10001UL) & 0xFFFEFFFEUL;
        d >>= 1;
        p[i] = (unsigned short)(d & 0xFFFF);
        p[i + 1] = (unsigned short)((d >> 16) & 0xFFFF);
    }
    if (n & 1) p[n - 1] = (unsigned short)((p[n - 1] + 1) >> 1);
}

static void d3_model_rebuild(d3ctx *c, unsigned ctx)
{
    unsigned short *tr = c->tree + (unsigned long)ctx * 256;
    unsigned short *fr = c->freq + (unsigned long)ctx * 256;
    unsigned long total = 0;
    unsigned i;

    d3_halve_pairs(fr, 256);
    for (i = 0; i < 256; i++) total += fr[i];
    total &= 0xFFFF;
    tr[0] = (unsigned short)total;
    for (i = 1; i < 256; i++) tr[i] = fr[i - 1];
    /* Fenwick build */
    for (i = 1; i < 256; i++) {
        unsigned j = i + (i & (0u - i));
        if (j < 256) tr[j] = (unsigned short)(tr[j] + tr[i]);
    }
}

static void d3_model_halve(d3ctx *c, unsigned ctx, unsigned shift)
{
    unsigned short *fr = c->freq + (unsigned long)ctx * 256;
    unsigned n;
    for (n = 1; n < shift; n++) d3_halve_pairs(fr, 256);
    d3_model_rebuild(c, ctx);
}

static void d3_model_update(d3ctx *c, unsigned char sym)
{
    static const unsigned short inc_tab[4] = { 2, 8, 32, 128 };
    unsigned inc = inc_tab[c->order & 3];
    unsigned long base = (unsigned long)c->prev_ctx * 256;
    unsigned short *tr = c->tree + base;
    unsigned short *fr = c->freq + base;

    fr[sym] = (unsigned short)(fr[sym] + inc);
    if (sym != 0xFF) {
        unsigned i = (unsigned)(sym + 1) & 0xFF;
        unsigned bit = 1;
        while (i) {
            if (i & bit) {
                tr[i] = (unsigned short)(tr[i] + inc);
                i = (i + bit) & 0xFF;
            }
            bit = (bit << 1) & 0xFF;
            if (bit == 0) break;
        }
    }
    tr[0] = (unsigned short)(tr[0] + inc);
    if (tr[0] >= 0x4000) d3_model_rebuild(c, c->prev_ctx);

    switch (c->mode) {
    case 0:
    case 5:
        c->prev_ctx = sym;
        break;
    case 1:
        c->prev_ctx = (unsigned char)((d3_de8[c->prev_ctx & 15] << 4) +
                                      d3_df8[sym]);
        break;
    case 3:
        c->prev_ctx = (unsigned char)(d3_ef8[sym] +
                                      ((c->counter % 3) << 5));
        break;
    default:
        c->prev_ctx = (unsigned char)(((c->counter & c->db) << 4) +
                                      d3_df8[sym]);
        break;
    }
    c->counter++;
}

/* ------------------------------------------------------------------ */
/* output transforms (sub_1B5D0 / sub_1B66C)                           */
/* ------------------------------------------------------------------ */
static unsigned char d3_hash_byte(d3ctx *c)
{
    return (unsigned char)((c->hash >> (8 * (c->db & 3))) & 0xFF);
}

static void d3_put_orig(d3ctx *c, unsigned char sym)
{
    unsigned char d = (unsigned char)(~sym);
    if (c->mode) {
        if (!c->flag7) d = (unsigned char)(0 - d);
        if ((unsigned char)(~sym) >= 0x80) c->flag7 = (unsigned char)!c->flag7;
        d = (unsigned char)(d + d3_hash_byte(c));
    }
    c->hash = ((c->hash << 8) + d) & 0xFFFFFFFFUL;
    c->last_out = d;
    d3_out_byte(c, d);
}

static void d3_put_trans(d3ctx *c, unsigned char b)
{
    unsigned char d;
    if (c->mode) {
        d = (unsigned char)(b - d3_hash_byte(c));
        if (!c->flag7) d = (unsigned char)(0 - d);
        if (d >= 0x80) c->flag7 = (unsigned char)!c->flag7;
    } else {
        d = b;
    }
    d = (unsigned char)(~d);
    c->hash = ((c->hash << 8) + b) & 0xFFFFFFFFUL;
    c->last_sym = d;
    d3_out_byte(c, b);
}

/* ------------------------------------------------------------------ */
/* token / length helpers                                              */
/* ------------------------------------------------------------------ */
static void d3_upd_state(d3ctx *c, unsigned v)
{
    unsigned short *m = c->state[c->state_idx];
    if (v == 2) m[3] = (unsigned short)(m[3] + 64);
    m[4] = (unsigned short)(m[4] + 64);
    if (m[4] >= 0x4000) {
        unsigned long f2 = (unsigned)(m[3] - m[2]);
        unsigned long f3 = (unsigned)(m[4] - m[3]);
        unsigned long h2 = (f2 + 1) >> 1;
        unsigned long h3 = (f3 + 1) >> 1;
        m[3] = (unsigned short)(m[2] + h2);
        m[4] = (unsigned short)(m[3] + h3);
    }
    c->state_idx = (unsigned char)((v + 2 * c->state_idx - 2) & 7);
}

static void d3_upd_lenclass(d3ctx *c, unsigned v)
{
    unsigned short *m = c->lenclass;
    if (v == 0) m[1] = (unsigned short)(m[1] + 64);
    if (v <= 1) m[2] = (unsigned short)(m[2] + 64);
    m[3] = (unsigned short)(m[3] + 64);
    if (m[3] >= 0x4000) {
        unsigned long f0 = m[1] - m[0];
        unsigned long f1 = m[2] - m[1];
        unsigned long f2 = m[3] - m[2];
        unsigned long h0 = (f0 + 1) >> 1;
        unsigned long h1 = (f1 + 1) >> 1;
        unsigned long h2 = (f2 + 1) >> 1;
        m[1] = (unsigned short)(m[0] + h0);
        m[2] = (unsigned short)(m[1] + h1);
        m[3] = (unsigned short)(m[2] + h2);
    }
}

/* generic cumulative model rescale: sub_1BC24 / sub_1BCC0 / sub_1BA30 */
static void d3_rescale_cum(unsigned short *m, unsigned last)
{
    unsigned d;
    for (d = last; d >= 1; d--) {
        unsigned long diff = (unsigned)(m[d] - m[d - 1]);
        m[d] = (unsigned short)((diff + 1) >> 1);
        if (d == 0) break;
    }
    {
        unsigned a;
        for (a = 2; a <= last; a++)
            m[a] = (unsigned short)(m[a] + m[a - 1]);
    }
}

static void d3_upd_dmodel(d3ctx *c, unsigned v) /* sub_1BC24 */
{
    unsigned a;
    for (a = (unsigned)v + 1; a < 44; a++)
        c->dmodel[a] = (unsigned short)(c->dmodel[a] + 0x20);
    c->dmodel[44] = (unsigned short)(c->dmodel[44] + 0x20);
    if (c->dmodel[44] >= 0x4000) d3_rescale_cum(c->dmodel, 44);
}

static void d3_fill_step(d3ctx *c, unsigned idx) /* sub_1BCC0 */
{
    unsigned inc = (unsigned)(c->dmodel[idx + 1] + 1) >> 1;
    unsigned a;
    for (a = idx; a < 45; a++)
        c->dmodel[a] = (unsigned short)(c->dmodel[a] + inc);
    d3_rescale_cum(c->dmodel, 44);
}

static void d3_upd_lenmodel(d3ctx *c, unsigned v) /* sub_1BA30 */
{
    unsigned short *m = c->len_sel ? c->lenB : c->lenA;
    unsigned last = c->len_sel ? 22 : 21;
    unsigned a;
    for (a = (unsigned)v + 1; a < last; a++)
        m[a] = (unsigned short)(m[a] + 0x40);
    m[last] = (unsigned short)(m[last] + 0x40);
    if (m[last] >= 0x4000) d3_rescale_cum(m, last);
}

static void d3_upd_distlist(d3ctx *c, unsigned v) /* sub_1BAC4 */
{
    unsigned char cl = (unsigned char)c->class_max;
    if (v < 3) {
        if (v == 2) c->smallm[2] = (unsigned short)(c->smallm[2] + 0x20);
        c->smallm[3] = (unsigned short)(c->smallm[3] + 0x20);
        if (c->smallm[3] >= 0x4000) {
            unsigned long f1 = (unsigned)(c->smallm[1] - c->smallm[0]);
            unsigned long f2 = (unsigned)(c->smallm[3] - c->smallm[1]);
            unsigned long h1 = (f1 + 1) >> 1;
            unsigned long h2 = (f2 + 1) >> 1;
            if (h1 < 0x400) h1 = 0x400;
            if (h2 < 0x400) h2 = 0x400;
            c->smallm[1] = (unsigned short)(c->smallm[0] + h1);
            c->smallm[3] = (unsigned short)(c->smallm[1] + h2);
        }
    } else {
        unsigned a = (unsigned)c->list_max - 1 - v;
        for (;;) {
            a++;
            if (a >= cl) break;
            c->d2[a] = (unsigned short)(c->d2[a] + 0x20);
        }
        c->d2[cl] = (unsigned short)(c->d2[cl] + 0x20);
        if (c->d2[cl] >= 0x4000) {
            unsigned d;
            for (d = cl; d >= 1; d--) {
                unsigned long diff = (unsigned)(c->d2[d] - c->d2[d - 1]);
                unsigned long h = (diff + 1) >> 1;
                if (h < 0x100) h = 0x100;
                c->d2[d] = (unsigned short)h;
                if (d == 0) break;
            }
            {
                unsigned x;
                for (x = 2; x <= cl; x++)
                    c->d2[x] = (unsigned short)(c->d2[x] + c->d2[x - 1]);
            }
        }
    }
    c->class_max = cl;
}

/* sub_1BF8C: decode a match length, returns the length class symbol */
static unsigned d3_dec_len(d3ctx *c)
{
    unsigned short *m = c->len_sel ? c->lenB : c->lenA;
    unsigned start = c->len_sel ? 22 : 21;
    unsigned long v;
    unsigned d = start;
    unsigned bits, extra = 0;

    c->rtotal = m[start];
    v = d3_rget(c, c->rtotal);
    do {
        d--;
    } while (v < (unsigned long)m[d] && d);
    c->rlow = m[d];
    c->rhigh = m[d + 1];
    d3_rupd(c);

    bits = d3_lenbits[d];
    if (bits) {
        c->rtotal = bits;
        extra = (unsigned)d3_rget(c, bits);
        c->rlow = extra & 0xFF;
        c->rhigh = (extra + 1) & 0xFFFF;
        d3_rupd(c);
    }
    c->len_val = (unsigned char)(d3_lenbase[d] + (extra & 0xFF));
    return d;
}

/* sub_1C054: decode distance class / repeat index */
static unsigned d3_dec_dist(d3ctx *c)
{
    unsigned long v;
    unsigned d, ebx, ex, low, high;
    unsigned short *m = c->dmodel;

    c->rtotal = m[44];
    v = d3_rget(c, c->rtotal);
    d = 44;
    do {
        d--;
    } while (v < (unsigned long)m[d] && d);
    c->rlow = m[d];
    c->rhigh = m[d + 1];
    d3_rupd(c);
    c->match_pos = d3_distbase[d];

    if (d == 40) {
        c->len_sel = 0;
        return d;
    }
    if (d == 41) {
        unsigned short *m2 = c->d2;
        ebx = c->class_max;
        ex = ebx * 2;
        c->rtotal = m2[ebx];
        v = d3_rget(c, c->rtotal);
        do {
            high = m2[ebx - 1];
            ex -= 2;
            ebx--;
            if (v >= (unsigned long)high) break;
        } while (ebx);
        c->rlow = m2[ebx];
        c->rhigh = m2[ebx + 1];
        d3_rupd(c);
        c->match_pos += (unsigned long)(c->class_max - 1 - ebx);
        c->len_sel = 1;
        return d;
    }
    if (d == 42) {
        unsigned short *sm = c->smallm;
        ebx = 2;
        c->rtotal = sm[3];
        v = d3_rget(c, c->rtotal);
        do {
            high = sm[ebx];
            ebx--;
            if (v >= (unsigned long)high) break;
        } while (ebx);
        c->rlow = sm[ebx + 1];
        c->rhigh = sm[ebx + 2];
        d3_rupd(c);
        c->match_pos += (unsigned long)(1 - ebx);
        c->len_sel = 1;
        return d;
    }
    if (d == 43) {
        c->len_sel = 1;
        return d;
    }

    /* d <= 39: refine distance inside its class */
    ebx = (unsigned)d3_distspan[d];
    ex = (unsigned)(c->match_pos + ebx - 1);
    if (ex > c->ring_fill) ebx = (unsigned)(c->ring_fill - c->match_pos + 1);
    if (ebx >= 2) {
        if (ebx > 0x1000) {
            unsigned long q;
            c->rtotal = ((ebx - 1) >> 12) + 1;
            q = d3_rget(c, c->rtotal) & 0xFFFF;
            c->rlow = (unsigned)q;
            c->rhigh = (unsigned)(q + 1);
            d3_rupd(c);
            c->match_pos = (c->match_pos + (q << 12)) & 0xFFFFFFFFUL;
            if (c->rhigh == c->rtotal) {
                ebx -= (unsigned)(q << 12);
            } else {
                ebx = 0x1000;
            }
        }
        if (ebx >= 2) {
            c->rtotal = ebx;
            low = (unsigned)d3_rget(c, ebx) & 0xFFFF;
            c->rlow = low;
            c->rhigh = (unsigned)(low + 1);
            d3_rupd(c);
            c->match_pos = (c->match_pos + low) & 0xFFFFFFFFUL;
        }
    }
    c->len_sel = 0;
    return d;
}

/* sub_1C600: decode the per-symbol token (0..3) */
static unsigned d3_token(d3ctx *c)
{
    unsigned short *m = c->state[c->state_idx];
    unsigned long v;
    unsigned d = 4;

    c->rtotal = m[4];
    v = d3_rget(c, c->rtotal);
    do {
        d--;
        if (v >= (unsigned long)m[d]) break;
    } while (d);
    c->rlow = m[d];
    c->rhigh = m[d + 1];
    d3_rupd(c);
    if (d <= 1) return d;
    d3_upd_state(c, d);
    return d;
}

/* ------------------------------------------------------------------ */
/* token handlers                                                      */
/* ------------------------------------------------------------------ */
static void d3_reset_models(d3ctx *c)
{
    unsigned i;
    for (i = 0; i < 256; i++) {
        unsigned j;
        unsigned short *tr = c->tree + (unsigned long)i * 256;
        unsigned short *fr = c->freq + (unsigned long)i * 256;
        for (j = 0; j < 256; j++) fr[j] = 1;
        tr[0] = 256;
        for (j = 1; j < 256; j++) tr[j] = 1;
        for (j = 1; j < 256; j++) {
            unsigned k = j + (j & (0u - j));
            if (k < 256) tr[k] = (unsigned short)(tr[k] + tr[j]);
        }
    }
}

/* sub_1C6C0 */
static void d3_control(d3ctx *c)
{
    unsigned long v;
    unsigned d = 12;
    int v15 = 0, v14 = 0;

    c->rtotal = 12;
    v = d3_rget(c, 12);
    do {
        d--;
    } while (v < (unsigned long)d3_ctl_init[d] && d);
    c->rlow = d3_ctl_init[d];
    c->rhigh = d3_ctl_init[d + 1];
    d3_rupd(c);
    if (d > 11) { c->err = 7; return; }

    if (d <= 5) v15 = 1;
    else if (d <= 9) v14 = 1;
    else if (d == 10) c->raw_lit = (unsigned char)!c->raw_lit;
    else if (d == 11) c->exc_lit = (unsigned char)!c->exc_lit;

    if (v15) {
        c->mode = (unsigned char)d;
        c->db = (unsigned char)((d - 1) & 3);
        if (c->out_total) d3_reset_models(c);
    }
    if (v14) {
        unsigned shift;
        c->prev_order = c->order;
        c->order = (unsigned char)(d - 6);
        if (c->out_total && !v15) {
            shift = d3_excl[4 * c->prev_order + c->order];
            if (shift) {
                unsigned i;
                for (i = 0; i < 256; i++)
                    d3_model_halve(c, i, shift);
            }
        }
    }
}

/* sub_1C870 */
static void d3_literal(d3ctx *c)
{
    unsigned sym;
    if (c->raw_lit) {
        c->rtotal = 256;
        sym = (unsigned)d3_rget(c, 256) & 0xFF;
        c->rlow = sym;
        c->rhigh = (sym + 1) & 0xFFFF;
        d3_rupd(c);
    } else if (c->exc_lit) {
        sym = d3_dec_tree2(c, c->tree + (unsigned long)c->prev_ctx * 256);
        d3_rupd(c);
    } else {
        sym = d3_dec_tree(c, c->tree + (unsigned long)c->prev_ctx * 256);
        d3_rupd(c);
    }
    d3_model_update(c, (unsigned char)sym);
    d3_put_orig(c, (unsigned char)sym);
    c->dict[c->ring_pos] = c->last_out;
    if (c->ring_fill < c->dict_wrap) c->ring_fill++;
    c->ring_pos = (c->ring_pos + 1) & c->dict_mask;
}

/* sub_1C938 */
static void d3_match(d3ctx *c)
{
    unsigned dist_idx, sym, len_sym;
    unsigned long dist;
    unsigned long src;
    unsigned bl;

    for (;;) {
        unsigned idx = c->fill_idx;
        if (c->ring_fill >= d3_fillthr[idx]) {
            c->fill_idx = (unsigned char)(idx - 1);
            d3_fill_step(c, idx);
        } else break;
    }

    sym = d3_dec_dist(c);
    d3_upd_dmodel(c, sym);

    if (c->len_sel) {
        if (c->len_sel != 1) { c->err = 7; return; }
        dist_idx = (unsigned)(c->match_pos & 0xFF);
        if (dist_idx >= 16) { c->err = 7; return; }
        dist = c->distlist[dist_idx];
        if (dist_idx) d3_upd_distlist(c, dist_idx);
        bl = dist_idx;
    } else {
        dist = c->match_pos;
        bl = (unsigned)c->list_max - 1;
    }
    if (bl && dist > 1) {
        unsigned k = bl;
        while (k) {
            c->distlist[k] = c->distlist[k - 1];
            k--;
        }
        c->distlist[0] = dist;
    }

    src = (c->ring_pos - dist) & c->dict_mask;
    len_sym = d3_dec_len(c);
    d3_upd_lenmodel(c, len_sym);
    bl = c->len_val;
    if (bl < 2) { c->err = 7; return; }
    for (;;) {
        unsigned char b = c->dict[src];
        c->dict[c->ring_pos] = b;
        if (c->ring_fill < c->dict_wrap) c->ring_fill++;
        c->ring_pos = (c->ring_pos + 1) & c->dict_mask;
        src = (src + 1) & c->dict_mask;
        d3_put_trans(c, b);
        d3_model_update(c, c->last_sym);
        bl--;
        if (!bl) break;
    }
}

/* sub_1CA90 */
static void d3_run(d3ctx *c)
{
    unsigned long v;
    unsigned d = 3;
    unsigned long len;
    unsigned char b;
    unsigned i;

    c->rtotal = c->lenclass[3];
    v = d3_rget(c, c->lenclass[3]);
    do {
        d--;
    } while (v < (unsigned long)c->lenclass[d] && d);
    c->rlow = c->lenclass[d];
    c->rhigh = c->lenclass[d + 1];
    d3_rupd(c);
    d3_upd_lenclass(c, d);

    len = 0x100UL;
    if (d == 2) {
        c->rtotal = 256;
        v = d3_rget(c, 256) & 0xFF;
        c->rlow = (unsigned)v;
        c->rhigh = (unsigned)(v + 1);
        d3_rupd(c);
        len += (v & 0xFF) << 16;
    }
    if (d == 1 || d == 2) {
        c->rtotal = 256;
        v = d3_rget(c, 256) & 0xFF;
        c->rlow = (unsigned)v;
        c->rhigh = (unsigned)(v + 1);
        d3_rupd(c);
        len += (v & 0xFF) << 8;
    }
    c->rtotal = 256;
    v = d3_rget(c, 256) & 0xFF;
    c->rlow = (unsigned)v;
    c->rhigh = (unsigned)(v + 1);
    d3_rupd(c);
    len += v & 0xFF;

    c->rtotal = 256;
    v = d3_rget(c, 256) & 0xFF;
    c->rlow = (unsigned)v;
    c->rhigh = (unsigned)(v + 1);
    d3_rupd(c);
    b = (unsigned char)v;

    /* fill dictionary ring with 255 copies (only used as match source) */
    if (c->ring_pos + 255 < c->dict_size) {
        memset(c->dict + c->ring_pos, b, 255);
        {
            unsigned long nf = c->ring_fill + 255;
            if (nf > c->dict_wrap) nf = c->dict_wrap;
            c->ring_fill = nf;
        }
        c->ring_pos += 255;
    } else {
        for (i = 0; i < 255; i++) {
            c->dict[c->ring_pos] = b;
            if (c->ring_fill < c->dict_wrap) c->ring_fill++;
            c->ring_pos = (c->ring_pos + 1) & c->dict_mask;
        }
    }

    /* five model-emitted bytes */
    for (i = 0; i < 5; i++) {
        d3_put_trans(c, b);
        d3_model_update(c, c->last_sym);
    }
    len -= 5;
    d3_fill_out(c, b, len);
    while (len) {
        d3_model_update(c, c->last_sym);
        len--;
    }
}

/* ------------------------------------------------------------------ */
/* public API                                                          */
/* ------------------------------------------------------------------ */
static void d3_free(d3ctx *c)
{
    if (!c) return;
    free(c->tree);
    free(c->freq);
    free(c->distlist);
    free(c->dict);
    free(c);
}

int uharc_dec3t_init(void **pctx, int method)
{
    d3ctx *c;
    unsigned group, i;
    unsigned long dict;

    if (!pctx) return 1;
    *pctx = 0;
    if (method < 8 || method > 43) return 1;

    group = (method < 20) ? 1u : (method < 32 ? 2u : 3u);

    c = (d3ctx *)calloc(1, sizeof(*c));
    if (!c) return 2;
    c->tree = (unsigned short *)malloc(256UL * 256UL * sizeof(unsigned short));
    c->freq = (unsigned short *)malloc(256UL * 256UL * sizeof(unsigned short));
    c->distlist = (unsigned long *)malloc(16 * sizeof(unsigned long));
    dict = 1UL << (((unsigned)(method - 8) % 12u) + 10u);
    c->dict = (unsigned char *)malloc(dict + 256);
    if (!c->tree || !c->freq || !c->distlist || !c->dict) {
        d3_free(c);
        return 2;
    }

    c->dict_size = dict;
    c->dict_wrap = dict - 255;
    c->dict_mask = dict - 1;
    c->group = (unsigned char)group;
    c->list_max = d3_db[group];
    c->class_max = (unsigned long)(d3_db[group] - 3);

    for (i = 0; i < 256; i++) {
        unsigned j;
        unsigned short *tr = c->tree + (unsigned long)i * 256;
        unsigned short *fr = c->freq + (unsigned long)i * 256;
        for (j = 0; j < 256; j++) fr[j] = 1;
        tr[0] = 256;
        for (j = 1; j < 256; j++) tr[j] = 1;
        for (j = 1; j < 256; j++) {
            unsigned k = j + (j & (0u - j));
            if (k < 256) tr[k] = (unsigned short)(tr[k] + tr[j]);
        }
    }
    for (i = 0; i < 8; i++) memcpy(c->state[i], d3_state_init, 10);
    memcpy(c->lenA, d3_len_init, sizeof(c->lenA));
    memcpy(c->lenB, d3_len_init, sizeof(c->lenB));
    memcpy(c->dmodel, d3_dmodel_init, sizeof(c->dmodel));
    memcpy(c->smallm, d3_small_init, sizeof(c->smallm));
    memcpy(c->d2, d3_d2_init, sizeof(c->d2));
    memcpy(c->lenclass, d3_lenclass_init, sizeof(c->lenclass));
    for (i = 0; i < 16; i++) c->distlist[i] = 0xFFFFFFFFUL;

    c->state_idx = 7;
    c->prev_ctx = 0xFF;
    c->last_out = 0;
    c->last_sym = 0;
    c->mode = 0;
    c->db = 0;
    c->order = 3;
    c->prev_order = 3;
    c->flag7 = 1;
    c->raw_lit = 0;
    c->exc_lit = 0;
    c->len_sel = 0;
    c->len_val = 0;
    c->fill_idx = (unsigned char)(sizeof(d3_fillthr) / sizeof(d3_fillthr[0]) - 1);
    c->hash = 0xFFFFFFFFUL;
    c->counter = 0;
    c->match_pos = 0xFFFFFFFFUL;
    c->ring_pos = 0;
    c->ring_fill = 31;
    c->code = 0;
    c->range = 0xFFFFUL;

    *pctx = c;
    return 0;
}

int uharc_dec3t_decode(void *ctx, uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len)
{
    d3ctx *c = (d3ctx *)ctx;
    unsigned i;

    if (!c || !rcb || !wcb) return 1;
    c->rcb = rcb; c->rctx = rctx;
    c->wcb = wcb; c->wctx = wctx;
    c->out_expect = out_len;
    c->out_total = 0;
    c->out_fill = 0;
    c->in_pos = c->in_len = 0;
    c->in_eof = 0;
    c->err = 0;
    c->bit_mask = 0;

    /* sub_206D0: 16 bits into code, range = 0x10000 */
    c->code = 0;
    for (i = 0; i < 16; i++)
        c->code = ((c->code << 1) | (unsigned long)d3_bit(c)) & 0xFFFFFFFFUL;
    c->range = 0x10000UL;
    if (c->err) return c->err;

    while (c->out_total < c->out_expect && !c->err) {
        unsigned t = d3_token(c);
        switch (t) {
        case 0: d3_control(c); break;
        case 1: d3_run(c); break;
        case 2: d3_match(c); break;
        case 3: d3_literal(c); break;
        default: c->err = 7; break;
        }
    }
    d3_flush(c);
    if (c->err) return c->err;
    if (c->out_total < c->out_expect) return 4;
    return 0;
}

void uharc_dec3t_free(void *ctx)
{
    d3_free((d3ctx *)ctx);
}
