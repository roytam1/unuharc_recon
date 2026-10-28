/* UHARC 0.4 decompressor core 4 (methods 24..39) - portable C89 port.
 *
 * Ported from the UHARC04.lst disassembly of the UHARC 0.4 BETA decoder
 * (control flow and constants come straight from the listing).
 *
 * Original -> port mapping (UHARC04.lst):
 *   sub_415130 (per-file init)     -> uharc_dec4v_init
 *   sub_415980 (teardown)          -> uharc_dec4v_free
 *   sub_41CE30 (block driver)      -> d4v_block
 *   sub_41CD30 (range coder)       -> d4v_rc
 *   sub_418830 (per-symbol PPM)    -> d4v_sym
 *   sub_41AF70 (escape/long-match) -> d4v_escape
 *   sub_4177A0 (mask/order sel)    -> d4v_ctxsel
 *   sub_417E10 (PPM decoder)       -> d4v_ppmdec
 *   sub_415C40 (LZP/literal path)  -> d4v_lzp
 *   sub_416220 (PPM path)          -> d4v_ppm
 *   sub_41C190 (model emit)        -> d4v_emit
 *   sub_41C670 (context hash)      -> d4v_hash
 *   sub_426EE0 (price prefix sums) -> d4v_prices
 *   sub_40D070 (heap alloc)        -> d4v_alloc (bump over one calloc block)
 *   sub_40D570 (fill, replicated)  -> memset equivalent
 *   sub_425810 (u16 fill)          -> d4v_fill16
 *   sub_40DA80 (heap base query)   -> arena base
 *
 * Memory model: the original allocates ~20 heap blocks via sub_40D070 and
 * addresses everything (including DGROUP globals) by absolute address.
 * The port keeps one zeroed arena; every original absolute address is used
 * as an arena offset (V4O clamps like the 0.6 port's D4O). Block pointers
 * (dword_433B40..) are arena offsets handed out by a bump allocator in the
 * same order and sizes as the original.
 *
 * Register convention: x86 partial-register writes (mov al/dl, [mem] etc.)
 * preserve the other lanes, whose values the original code inherits from
 * whatever the caller left behind. The C port models each register as an
 * unsigned variable, so every worker auto is explicitly zero-initialised
 * at declaration; without this the build reads indeterminate stack garbage
 * (non-deterministic output) where the original reads deterministic
 * leftovers (in practice zero, e.g. xor-cleared lanes).
 */
#include <stdlib.h>
#include <string.h>
#include "uharc_dec.h"

#define V4_ARENA 0x04000000UL

typedef struct dec4v_ctx {
    unsigned char *mem;
    unsigned long hi;
    uharc_dec_read_cb rcb;
    void *rctx;
    uharc_dec_write_cb wcb;
    void *wctx;
    unsigned in_pos;
    unsigned in_avail;
    unsigned out_fill;
    unsigned long out_tot;
    unsigned long out_len;
    int err;
    /* arena offsets of the 64KB stream buffers (dword_436ED0/436ED4) */
    unsigned long p_inb;
    unsigned long p_outb;
    /* model block offsets (dword_433B40.. set) */
    unsigned long p_3b40, p_3b44, p_3b48, p_3b4c, p_3b50, p_3b54, p_3b58;
    unsigned long p_3b5c, p_3b60, p_3b64, p_3b68, p_3b6c, p_3b70, p_3b74;
    unsigned long p_3b78, p_3b7c, p_3b80, p_3b84, p_3b88, p_3b8c, p_af34;
} dec4v_ctx;

static unsigned int d4v_o(unsigned int o)
{
    if (o > (unsigned)(V4_ARENA - 8)) o = (unsigned)(V4_ARENA - 8);
    return o;
}
#define V4O(o)  d4v_o((unsigned)(o))
#define G8(a)   (*(unsigned char  *)(void *)(c->mem + V4O(a)))
#define G16(a)  (*(unsigned short *)(void *)(c->mem + V4O(a)))
#define G32(a)  (*(unsigned int   *)(void *)(c->mem + V4O(a)))
#define M8(o)   (*(unsigned char  *)(void *)(c->mem + V4O(o)))
#define M16(o)  (*(unsigned short *)(void *)(c->mem + V4O(o)))
#define M32(o)  (*(unsigned int   *)(void *)(c->mem + V4O(o)))

/* sub_40D070: bump allocator over the arena (the original manages a real
 * heap with freelists; sizes/aligns below mirror the init sequence). */
unsigned long d4v_alloc(dec4v_ctx *c, unsigned long size)
{
    unsigned long p;
    if (size == 0 || size >= (unsigned long)(V4_ARENA - 16)) return 0;
    p = (c->hi + 15UL) & ~15UL;
    if (p + size > (unsigned long)V4_ARENA) return 0;
    c->hi = p + size;
    return p;
}

/* sub_425810(ebx=count, edx=dst, eax=val): fill count u16 with val. */
void d4v_fill16(dec4v_ctx *c, unsigned int count, unsigned int dst,
                       unsigned int val)
{
    unsigned int i;
    for (i = 0; i < count; i++)
        M16(dst + 2 * i) = (unsigned short)val;
}

/* sub_426EE0: one-time prefix sums word_437968 += word_437966. Both tables
 * are BSS-zero in the original (437966 has no writers) so this only clears
 * byte_43F968; kept as the loop for fidelity. */
void d4v_prices(dec4v_ctx *c)
{
    if (M8(0x43F968)) {
        unsigned int a;
        for (a = 1; a < 0x4000; a++)
            M16(0x437968 + 2 * a) += M16(0x437966 + 2 * a);
        M8(0x43F968) = 0;
    }
}

int d4v_415130(dec4v_ctx *c, int method, unsigned int al_in);

int uharc_dec4v_init(void **pctx, int method)
{
    dec4v_ctx *c;
    int rc;
    c = (dec4v_ctx *)calloc(1, sizeof(*c));
    if (!c) return 2;
    c->mem = (unsigned char *)calloc(1, (size_t)V4_ARENA);
    if (!c->mem) { free(c); return 2; }
    c->hi = 0x1000;
    c->p_inb = d4v_alloc(c, 65536UL);
    c->p_outb = d4v_alloc(c, 65536UL);
    if (!c->p_inb || !c->p_outb) { free(c->mem); free(c); return 2; }
    /* Model blocks must live ABOVE the fixed globals window (DGROUP image
     * 0x42E000-0x440000, .bss 0x440000-0x45DA00); bumping from 0x21000
     * would pave over it. 0x600000 + ~53MB footprint < 64MB arena. */
    c->hi = 0x600000UL;
    /* DGROUP static image (config bytes, pointer tables, price seeds):
     * 1:1 copy so absolute-address reads match the original. */
    {
        extern const unsigned char d4v_dgroup[73728];
        memcpy(c->mem + 0x42E000, d4v_dgroup, 73728);
    }
    rc = d4v_415130(c, method, 0);
    if (rc) { free(c->mem); free(c); return rc; }
    *pctx = c;
    return 0;
}

void uharc_dec4v_free(void *ctx)
{
    dec4v_ctx *c = (dec4v_ctx *)ctx;
    if (!c) return;
    free(c->mem);
    free(c);
}

/* ---- byte pump (sub_4220B0 / sub_4221F0 / sub_422110 / sub_421F50) ---- */

/* sub_4285A0: memmove used by the 421FA0 compact path
 * (dst=eax, src=edx, n=ebx; arena offsets). */
static void d4v_memmove(dec4v_ctx *c, unsigned int dst, unsigned int src,
                        unsigned int n)
{
    memmove(c->mem + V4O(dst), c->mem + V4O(src), (size_t)n);
}

/* indirect stream reader ([4370D8]): read up to n bytes to dst, return
 * bytes read (0 at EOF). 45CD24 is NOT bumped here: consumption is
 * counted per byte by sub_4220B0. */
static unsigned int d4v_4370d8(dec4v_ctx *c, unsigned int dst,
                               unsigned int n, unsigned int aux)
{
    int r;
    (void)aux;
    if (c->err) return 0;
    if (n > 65536U) n = 65536U;
    if (n == 0) return 0;
    r = c->rcb(c->rctx, c->mem + V4O(dst), n);
    if (r <= 0) return 0;
    return (unsigned int)r;
}

/* sub_421F50: refill input buffer through the [4370D8] reader. */
static void d4v_refill(dec4v_ctx *c)
{
    unsigned int got;
    if (c->err) { G32(0x45CD20) = 0; G32(0x45CD18) = 0; return; }
    G32(0x440E70) = G32(0x45CD24);
    G32(0x45CD18) = 0;
    G32(0x440E74) = G32(0x45CD28);
    got = d4v_4370d8(c, G32(0x436ED0), G32(0x45CD2C), 0);
    G32(0x45CD20) = got;
    if (!got) {
        G32(0x436EC8) = 3;
        c->err = 3;
    }
}

/* sub_4220B0: input byte. Increments 45CD24. */
static unsigned int d4v_get_byte(dec4v_ctx *c)
{
    unsigned int pos;
    if (G32(0x45CD18) >= G32(0x45CD20))
        d4v_refill(c);
    if (c->err) return 0;
    pos = G32(0x45CD18);
    G32(0x45CD24) = G32(0x45CD24) + 1;
    G32(0x45CD18) = pos + 1;
    return (unsigned int)M8(G32(0x436ED0) + pos);
}

/* forward flush used by put */
static void d4v_flush_out(dec4v_ctx *c);

/* sub_4221F0: output byte (al). Appends at 436ED4+45CD1C, bumps 45CD1C
 * and 45CD28, flushes via 422110 when 45CD1C reaches 45CD10. */
static void d4v_put_byte(dec4v_ctx *c, unsigned int b)
{
    unsigned int at;
    if (c->err) return;
    at = G32(0x45CD1C);
    M8(G32(0x436ED4) + at) = (unsigned char)b;
    G32(0x45CD1C) = at + 1;
    G32(0x45CD28) = G32(0x45CD28) + 1;
    if (at + 1 >= G32(0x45CD10))
        d4v_flush_out(c);
}

/* sub_422110: flush output buffer through the write callback. */
static void d4v_flush_out(dec4v_ctx *c)
{
    unsigned int n = G32(0x45CD1C);
    if (c->err) return;
    if (n) {
        if (c->wcb(c->wctx, c->mem + G32(0x436ED4), n) != 0)
            c->err = 4;
        G32(0x45CD1C) = 0;
    }
}

/* ---- range coder reset (sub_426F70) ----
 * NOTE: the 417DA0 decode driver calls sub_426F30, not this; 426F70
 * belongs to the 41CE30 block-driver path. Kept for reference. */
static void d4v_426f70(dec4v_ctx *c)
{
    G32(0x45D0B4) = 0;
    G32(0x45D0C4) = 0;
    G32(0x45D0B0) = 0;
    G32(0x45D0C0) = 0;
    G32(0x45D0BC) = 0xFFFFFFFFUL;
}

/* ---- range prime (sub_426F30): 45D0B8 = 0, 45D0BC = 0xFFFFFFFF, then
 * fold 5 input bytes big-endian into 45D0B8 (the decode code register).
 * Entry regs: none. */
static void d4v_426f30(dec4v_ctx *c)
{
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;
    G32(0x45D0B8) = 0; /* 0x426F39 */
    G32(0x45D0BC) = 0xFFFFFFFFUL; /* 0x426F3F */
    eax = 0;
    for (edx = 0; edx < 5; edx++) { /* 0x426F45 loc_426F45 */
        ecx = 0; /* 0x426F45 xor ecx,ecx */
        ecx = d4v_get_byte(c) & 0xFFU; /* 0x426F47 call 4220B0; 0x426F4C mov cl,al */
        eax = G32(0x45D0B8); /* 0x426F4E */
        eax = ((eax << 8) | ecx) & 0xFFFFFFFFUL; /* 0x426F53 shl; 0x426F56 or */
        G32(0x45D0B8) = eax; /* 0x426F59 */
    } /* 0x426F5E cmp edx,5; 0x426F61 jb */
}

/* ---- range reader init (sub_41F2A0) ----
 * Entry esi = 0x45AF68 area base (constant; the second half sets it
 * explicitly and the first half uses the same [eax+4]/[eax+8] shape). */
static void d4v_41f2a0(dec4v_ctx *c)
{
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;
    unsigned int ebx;
    unsigned int edi;
    unsigned int esi = 0x45AF68;
    unsigned int ebp;
    unsigned int var_1c;
    edx = 2; ecx = 2;
    G32(0x45AF68) = edx;
    eax = G32(0x45D0BC);
    edx = 0;
    /* div ecx(2) */
    ebx = eax; eax = ebx / ecx;
    edx = 0;
    G32(0x45D0BC) = eax;
    ecx = eax;
    eax = G32(0x45D0B8);
    /* div ecx */
    ebx = eax; eax = ecx ? ebx / ecx : 0; edx = ecx ? ebx % ecx : 0;
    ebx = eax;
    G8(0x45C67F) = (unsigned char)ebx;
    eax = ebx;
    G32(0x45AF6C) = eax;
    eax = ebx + 1;
    G32(0x45AF70) = eax;
    eax = esi;
    edx = M32(eax + 8);
    eax = M32(eax + 4);
    ecx = G32(0x45D0BC);
    edx = edx - eax;
    eax = eax * ecx;
    edx = edx * ecx;
    ecx = G32(0x45D0B8);
    ecx = ecx - eax;
    while (edx < 0x1000000UL) {
        unsigned int b = d4v_get_byte(c);
        ecx = ((ecx << 8) | (b & 0xFFU)) & 0xFFFFFFFFUL;
        edx = (edx << 8) & 0xFFFFFFFFUL;
    }
    G32(0x45D0B8) = ecx;
    G32(0x45D0BC) = edx;
    if (G8(0x45C67F))
        return;
    ecx = 0;
    var_1c = 0;
    esi = 0x45AF68;
    ebp = 2;
    for (;;) {
        ebx = 0x800;
        G32(0x45AF68) = ebx;
        ecx = ebx;
        ebx = 0;
        eax = G32(0x45D0BC);
        edx = 0;
        ebx = eax; eax = ebx / ecx;
        G32(0x45D0BC) = eax;
        ecx = eax;
        eax = G32(0x45D0B8);
        ebx = eax; eax = ecx ? ebx / ecx : 0;
        ebx = eax & 0xFFFFU;
        G32(0x45AF6C) = ebx;
        eax = ebx + 1;
        G32(0x45AF70) = eax;
        eax = esi;
        edx = M32(eax + 8);
        eax = M32(eax + 4);
        ecx = G32(0x45D0BC);
        edx = edx - eax;
        eax = eax * ecx;
        edx = edx * ecx;
        ecx = G32(0x45D0B8);
        ecx = ecx - eax;
        while (edx < 0x1000000UL) {
            unsigned int b = d4v_get_byte(c);
            ecx = ((ecx << 8) | (b & 0xFFU)) & 0xFFFFFFFFUL;
            edx = (edx << 8) & 0xFFFFFFFFUL;
        }
        G32(0x45D0B8) = ecx;
        G32(0x45D0BC) = edx;
        {
            unsigned int cl = var_1c & 0xFFU;
            eax = var_1c;
            edi = G32(0x43120C);
            G32(0x45AF68) = ebp;
            eax = eax + 0x0BU;
            ebx = (ebx << cl) & 0xFFFFFFFFUL;
            var_1c = eax;
            edi = edi + ebx;
            ecx = ebp;
            G32(0x43120C) = edi;
        }
        ebx = 0;
        eax = G32(0x45D0BC);
        edx = 0;
        ebx = eax; eax = ecx ? ebx / ecx : 0;
        G32(0x45D0BC) = eax;
        ecx = eax;
        eax = G32(0x45D0B8);
        ebx = eax; eax = ecx ? ebx / ecx : 0;
        ebx = eax & 0xFFFFU;
        G32(0x45AF6C) = ebx;
        eax = ebx + 1;
        G32(0x45AF70) = eax;
        eax = esi;
        edx = M32(eax + 8);
        eax = M32(eax + 4);
        ecx = G32(0x45D0BC);
        edx = edx - eax;
        eax = eax * ecx;
        edx = edx * ecx;
        ecx = G32(0x45D0B8);
        ecx = ecx - eax;
        while (edx < 0x1000000UL) {
            unsigned int b = d4v_get_byte(c);
            ecx = ((ecx << 8) | (b & 0xFFU)) & 0xFFFFFFFFUL;
            edx = (edx << 8) & 0xFFFFFFFFUL;
        }
        G32(0x45D0B8) = ecx;
        G32(0x45D0BC) = edx;
        if (!ebx) break;
    }
    G32(0x45BA50) = G32(0x43120C);
}

/* ---- shared worker helpers (single copy; draft-local copies
 * of these macros/helpers were stripped at integration) ---- */
#define U32MASK 0xFFFFFFFFUL
#define ADD8_LO(r,v) ((r) = ((r) & 0xFFFFFF00UL) | (((r) + (v)) & 0xFFUL))
#define SUB8_LO(r,v) ((r) = ((r) & 0xFFFFFF00UL) | (((r) - (v)) & 0xFFUL))
#define DECB_LO(r) ((r) = ((r) & 0xFFFFFF00UL) | (((r) - 1) & 0xFFUL))
#define ADD8_HI(r,v) ((r) = ((r) & 0xFFFF00FFUL) | ((((((r) >> 8) & 0xFFUL) + (v)) & 0xFFUL) << 8))
#define D4V_SP 0x0F000000u /* stand-in stack base for lea modeling (418830);
 * values derived from it are only incremented/compared, never dereferenced */

/* Exact 32-bit arithmetic shift right (x86 SAR semantics, count masked). */
static unsigned long d4v_sar(unsigned long v, unsigned int n)
{
    unsigned long s;
    n &= 31u;
    if (n == 0)
        return v & U32MASK;
    if (v & 0x80000000UL) {
        s = (0xFFFFFFFFUL << (32u - n)) & U32MASK;
        return (s | (v >> n)) & U32MASK;
    }
    return (v >> n) & U32MASK;
}

/* sub_40D570-style byte fill (ebx=len, edx=val, eax=dst in the original). */
static void d4v_fill(dec4v_ctx *c, unsigned int len, unsigned int dst,
                     unsigned int val)
{
    memset(c->mem + V4O(dst), (int)(val & 0xFFU), (size_t)len);
}

/* ---- worker entry points (bodies appended below) ---- */
void d4v_lzp(dec4v_ctx *c);
int d4v_ppm(dec4v_ctx *c);
unsigned int d4v_ppmdec(dec4v_ctx *c);
void d4v_sym(dec4v_ctx *c, unsigned int a_eax);
unsigned int d4v_419970(dec4v_ctx *c, unsigned int eax_in,
                        unsigned int edx_in, unsigned int ebx_in);
unsigned int d4v_419ca0(dec4v_ctx *c, unsigned int eax_in);
unsigned int d4v_41a360(dec4v_ctx *c, unsigned int eax_in,
                        unsigned int edx_in);
void d4v_41ad60(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in,
                unsigned int ebx_in);
void d4v_41ae30(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in,
                unsigned int ebx_in);
unsigned int d4v_41ae70(dec4v_ctx *c, unsigned int tab,
                        unsigned int edx_in);
void d4v_41aeb0(dec4v_ctx *c, unsigned int tab, unsigned int flag_in);
void d4v_415ae0(dec4v_ctx *c);
void d4v_415b50(dec4v_ctx *c);
void d4v_416a00(dec4v_ctx *c, unsigned int idx);
void d4v_416b50(dec4v_ctx *c, unsigned int sym, unsigned int arg);
unsigned int d4v_416d10(dec4v_ctx *c, unsigned int ebx_in);
void d4v_4177a0(dec4v_ctx *c, unsigned int sym, unsigned int flag);
void d4v_4183d0(dec4v_ctx *c, unsigned int sym, unsigned int flag);
unsigned int d4v_418780(dec4v_ctx *c, unsigned int obj);
void d4v_416c80(dec4v_ctx *c, unsigned int len, unsigned int flag);
void d4v_417090(dec4v_ctx *c, unsigned int sym);
void d4v_417220(dec4v_ctx *c, unsigned int sym);
void d4v_417570(dec4v_ctx *c, unsigned int sym);
unsigned int d4v_418590(dec4v_ctx *c, unsigned int len);
int d4v_41cf60(dec4v_ctx *c, unsigned int a);
void d4v_41e5f0(dec4v_ctx *c);
/* S4 leaves (stubs below until draft_s4a/s4b land) */
unsigned int d4v_41d540(dec4v_ctx *c, unsigned int a, unsigned int d,
                        unsigned int e, unsigned int b);
unsigned int d4v_41dbc0(dec4v_ctx *c, unsigned int base, unsigned int count);
unsigned int d4v_41e0a0(dec4v_ctx *c, unsigned int a, unsigned int d,
                        unsigned int b, unsigned int e);
void d4v_41e640(dec4v_ctx *c);
void d4v_41e9f0(dec4v_ctx *c, unsigned int sym, unsigned int arg);
void d4v_421fa0(dec4v_ctx *c, unsigned int n);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
/* init helpers (defined in base, called by the 415130 draft) */
unsigned long d4v_alloc(dec4v_ctx *c, unsigned long size);
void d4v_prices(dec4v_ctx *c);
void d4v_fill16(dec4v_ctx *c, unsigned int count, unsigned int dst,
                unsigned int val);
int d4v_415130(dec4v_ctx *c, int method, unsigned int al_in);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
/* init helpers (defined in base, called by the 415130 draft) */
unsigned long d4v_alloc(dec4v_ctx *c, unsigned long size);
void d4v_prices(dec4v_ctx *c);
void d4v_fill16(dec4v_ctx *c, unsigned int count, unsigned int dst,
                unsigned int val);
int d4v_415130(dec4v_ctx *c, int method, unsigned int al_in);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
/* init helpers (defined in base, called by the 415130 draft) */
unsigned long d4v_alloc(dec4v_ctx *c, unsigned long size);
void d4v_prices(dec4v_ctx *c);
void d4v_fill16(dec4v_ctx *c, unsigned int count, unsigned int dst,
                unsigned int val);
int d4v_415130(dec4v_ctx *c, int method, unsigned int al_in);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
/* init helpers (defined in base, called by the 415130 draft) */
unsigned long d4v_alloc(dec4v_ctx *c, unsigned long size);
void d4v_prices(dec4v_ctx *c);
void d4v_fill16(dec4v_ctx *c, unsigned int count, unsigned int dst,
                unsigned int val);
int d4v_415130(dec4v_ctx *c, int method, unsigned int al_in);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
/* init helpers (defined in base, called by the 415130 draft) */
unsigned long d4v_alloc(dec4v_ctx *c, unsigned long size);
void d4v_prices(dec4v_ctx *c);
void d4v_fill16(dec4v_ctx *c, unsigned int count, unsigned int dst,
                unsigned int val);
int d4v_415130(dec4v_ctx *c, int method, unsigned int al_in);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
/* init helpers (defined in base, called by the 415130 draft) */
unsigned long d4v_alloc(dec4v_ctx *c, unsigned long size);
void d4v_prices(dec4v_ctx *c);
void d4v_fill16(dec4v_ctx *c, unsigned int count, unsigned int dst,
                unsigned int val);
int d4v_415130(dec4v_ctx *c, int method, unsigned int al_in);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
/* init helpers (defined in base, called by the 415130 draft) */
unsigned long d4v_alloc(dec4v_ctx *c, unsigned long size);
void d4v_prices(dec4v_ctx *c);
void d4v_fill16(dec4v_ctx *c, unsigned int count, unsigned int dst,
                unsigned int val);
int d4v_415130(dec4v_ctx *c, int method, unsigned int al_in);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
/* init helpers (defined in base, called by the 415130 draft) */
unsigned long d4v_alloc(dec4v_ctx *c, unsigned long size);
void d4v_prices(dec4v_ctx *c);
void d4v_fill16(dec4v_ctx *c, unsigned int count, unsigned int dst,
                unsigned int val);
int d4v_415130(dec4v_ctx *c, int method, unsigned int al_in);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
/* init helpers (defined in base, called by the 415130 draft) */
unsigned long d4v_alloc(dec4v_ctx *c, unsigned long size);
void d4v_prices(dec4v_ctx *c);
void d4v_fill16(dec4v_ctx *c, unsigned int count, unsigned int dst,
                unsigned int val);
int d4v_415130(dec4v_ctx *c, int method, unsigned int al_in);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4v_41d480(dec4v_ctx *c, unsigned int a);
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);

/* ---- buffered emit (sub_40DBE0, functional port) ----
 * The original manages progress display, watermark globals
 * (431288/28C/290/294/298, only read by its own init sibling 40DA90)
 * and an output callback. Bytes flow through d4v_put_byte into our own
 * 64KB buffer, so the port only propagates write errors to the 436EC8
 * flag the driver loop tests. Args (45CD24, 45CD28) = (edx, eax) as at
 * the native 417DD8 call site. */
static void d4v_emit(dec4v_ctx *c, unsigned int a1, unsigned int a2)
{
    (void)a1;
    (void)a2;
    if (c->err)
        G32(0x436EC8) = 6;
}

/* ---- block driver (sub_417DA0) ----
 * 426F70 + 41F2A0 setup, then per-symbol: maybe 415C40, maybe 416220,
 * 417E10 (->al; if 0, 418830), 40DBE0 emit; until 45CD28 >= 45CD14
 * or 436EC8 error. */
static void d4v_417da0(dec4v_ctx *c)
{
    unsigned int eax;
    unsigned int edx;
    d4v_426f30(c);
    d4v_41f2a0(c);
    for (;;) {
        eax = G32(0x45CD28);
        if (eax >= G32(0x45BA50))
            d4v_lzp(c);
        eax = G32(0x45CD28);
        if (eax >= G32(0x45AFB4))
            d4v_ppm(c);
        eax = d4v_ppmdec(c);
        if (!(eax & 0xFFU))
            d4v_sym(c, eax);
        edx = G32(0x45CD24);
        eax = G32(0x45CD28);
        d4v_emit(c, edx, eax);
        eax = G32(0x45CD28);
        if (eax >= G32(0x45CD14)) break;
        if (G32(0x436EC8)) break;
    }
}

int uharc_dec4v_decode_b(void *ctx, const unsigned long *cum,
                         unsigned long ncum,
                         uharc_dec_read_cb rcb, void *rctx,
                         uharc_dec_write_cb wcb, void *wctx,
                         unsigned long out_len)
{
    dec4v_ctx *c = (dec4v_ctx *)ctx;
    unsigned long ti;
    unsigned long tab;
    if (!c || !c->mem) return 1;
    c->rcb = rcb;
    c->rctx = rctx;
    c->wcb = wcb;
    c->wctx = wctx;
    c->out_len = out_len;
    c->err = 0;
    c->in_pos = 0;
    c->in_avail = 0;
    G32(0x436ED0) = (unsigned int)c->p_inb;
    G32(0x436ED4) = (unsigned int)c->p_outb;
    G32(0x45CD10) = 65536UL;
    G32(0x45CD2C) = 65536UL;
    G32(0x45CD1C) = 0;
    G32(0x45CD18) = 0;
    G32(0x45CD20) = 0;
    G32(0x45CD14) = out_len;
    G32(0x45CD24) = 0;
    G32(0x45CD28) = 0;
    G32(0x436EC8) = 0;
    /* Solid-archive file-boundary table (native 430F1C loader array):
     * table[i] = cumulative end offset of file i. The 416220 block
     * walks it via 45AFB0/45BA2C, tracking 45AFB4. Probed: native
     * table[0..43] == cum[0..43] on gmud-04x (27 ppm hits + EXIT dump). */
    G32(0x45AFB0) = 0;
    G32(0x45BA2C) = 0;
    if (cum && ncum) {
        tab = d4v_alloc(c, (unsigned long)(ncum * 4UL));
        if (tab) {
            for (ti = 0; ti < ncum; ti++)
                M32(tab + ti * 4UL) = (unsigned int)(cum[ti] & 0xFFFFFFFFUL);
            G32(0x45AFB0) = (unsigned int)tab;
        }
    }
    d4v_417da0(c);
    d4v_flush_out(c);
    if (c->err == 3) return 3;
    if (c->err == 4) return 4;
    if (G32(0x436EC8)) return 4;
    if (G32(0x45CD28) != out_len) return 4;
    return 0;
}

/* ---- init (from draft_init.c; replaces hand version) ---- */
int d4v_415130(dec4v_ctx *c, int method, unsigned int al_in)
{
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned int quot = 0, rem = 0, dict = 0;
    unsigned long size = 0;
    unsigned long bound = 0;

    /* prologue pushes (0x415130-0x415134) emit no code; frame noted above */
    esi = G32(0x45BA44); /* 0x415135 mov esi,[45BA44] (entry value; 0 first call) */
    edx = (unsigned int)method; /* entry edx */
    if ((int)edx < 0x18) goto bad_method; /* 0x41513B cmp edx,18h; 0x41513E jl loc_415145 */
    if ((int)edx > 0x27) goto bad_method; /* 0x415140 cmp edx,27h; 0x415143 jle loc_41515C (else fall into bad) */
    goto have_method;
bad_method:
    eax = 1u; /* 0x415145 mov eax,1 */
    esi = G32(0x45BA44); /* 0x41514A mov esi,[45BA44] */
    G32(0x45BA44) = esi; /* 0x415150 mov [45BA44],esi (preserved) */
    return 1; /* 0x415156-0x41515B pops + retn */
have_method:
    eax = (al_in & 0xFFu); /* entry al; bits 31..8 of entry eax are unused: only ah/al are stored below before eax is fully reloaded at 0x415177 */
    eax |= 0x100u; /* 0x41515C mov ah,1 */
    edx -= 0x18u; /* 0x41515E sub edx,18h (m = method-0x18) */
    G8(0x436150) = (unsigned char)((eax >> 8) & 0xFFu); /* 0x415161 mov [436150],ah (=1) */
    G8(0x45C675) = (unsigned char)(eax & 0xFFu); /* 0x415167 mov [45C675],al */
    d4v_prices(c); /* 0x41516C call sub_426EE0 */
    ecx = G32(0x440E78); /* 0x415171 mov ecx,[440E78] */
    eax = edx; /* 0x415177 mov eax,edx (=m) */
    ecx += 0x18C4Eu; /* 0x415179 add ecx,18C4Eh */
    edx = (unsigned int)(((int)edx) >> 31); /* 0x41517F sar edx,1Fh (sign extend; 0 for m>=0) */
    G32(0x440E78) = ecx; /* 0x415182 mov [440E78],ecx */
    ecx = 0x10u; /* 0x415188 mov ecx,10h */
    /* 0x41518D idiv ecx: signed divide, divisor const 16 (nonzero, direct /) */
    quot = (unsigned int)((int)eax / (int)ecx);
    rem = (unsigned int)((int)eax % (int)ecx);
    (void)quot; /* quotient discarded: overwritten by mov eax,1 at 0x415192 */
    edx = rem; /* idiv remainder -> edx (= m%16) */
    edx += 0x0Au; /* 0x41518F add edx,0Ah */
    eax = 1u; /* 0x415192 mov eax,1 */
    ecx = (ecx & 0xFFFFFF00UL) | (edx & 0xFFUL); /* 0x415197 mov cl,dl */
    eax <<= (ecx & 0xFFu); /* 0x415199 shl eax,cl (count = 10+(m%16) <= 25; cpu masks to 5 bits, no effect) */
    dict = eax; /* dict = 1<<(10+(m%16)) */
    G32(0x45BA30) = dict; /* 0x41519B mov [45BA30],eax (=dict) */
    eax--; /* 0x4151A0 dec eax (=dict-1) */
    ebx = 0u; /* 0x4151A1 xor ebx,ebx */
    G32(0x45BA3C) = eax; /* 0x4151A3 mov [45BA3C],eax (mask) */
    eax = G32(0x45BA30); /* 0x4151A8 mov eax,[45BA30] */
    ebp = G32(0x440E78); /* 0x4151AD mov ebp,[440E78] */
    eax += 0x100u; /* 0x4151B3 add eax,100h */
    G32(0x45BA34) = ebx; /* 0x4151B8 mov [45BA34],ebx (=0) */
    ebp += eax; /* 0x4151BE add ebp,eax */
    G32(0x45BA54) = ebx; /* 0x4151C0 mov [45BA54],ebx (=0) */
    G32(0x440E78) = ebp; /* 0x4151C6 mov [440E78],ebp */
    size = (unsigned long)eax; /* 0x4151CC call sub_40D070, size=eax=dict+0x100 */
    eax = (unsigned int)d4v_alloc(c, size);
    G32(0x433B40) = eax; /* 0x4151D1 mov [433B40],eax */
    if (eax == 0u) { /* 0x4151D6 test eax,eax; 0x4151D8 jnz loc_4151F1 (fallthrough is fail) */
        eax = 2u; /* 0x4151DA mov eax,2 */
        esi = G32(0x45BA44); /* 0x4151DF mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x4151E5 mov [45BA44],esi (preserved) */
        return 2; /* 0x4151EB-0x4151F0 pops + retn */
    }
    edx = (edx & 0xFFFF0000UL) | (unsigned int)G16(0x440E7A); /* 0x4151F1 mov dx,[440E78+2] (16-bit load merges; upper kept, here 0) */
    edx += 0x80u; /* 0x4151F8 add edx,80h */
    eax = 0x800000u; /* 0x4151FE mov eax,800000h */
    G16(0x440E7A) = (unsigned short)(edx & 0xFFFFu); /* 0x415203 mov [440E78+2],dx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x41520A call sub_40D070 */
    G32(0x433B44) = eax; /* 0x41520F mov [433B44],eax */
    if (eax == 0u) { /* 0x415214 test eax,eax; 0x415216 jnz loc_41522F */
        eax = 2u; /* 0x415218 mov eax,2 */
        esi = G32(0x45BA44); /* 0x41521D mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x415223 mov [45BA44],esi */
        return 2; /* 0x415229-0x41522E pops + retn */
    }
    ebx = (ebx & 0xFFFF0000UL) | (unsigned int)G16(0x440E7A); /* 0x41522F mov bx,[440E78+2] */
    ebx += 0x14u; /* 0x415236 add ebx,14h */
    eax = 0x140000u; /* 0x415239 mov eax,140000h */
    G16(0x440E7A) = (unsigned short)(ebx & 0xFFFFu); /* 0x41523E mov [440E78+2],bx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x415245 call sub_40D070 */
    G32(0x433B48) = eax; /* 0x41524A mov [433B48],eax */
    if (eax == 0u) { /* 0x41524F test eax,eax; 0x415251 jnz loc_41526A */
        eax = 2u; /* 0x415253 mov eax,2 */
        esi = G32(0x45BA44); /* 0x415258 mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x41525E mov [45BA44],esi */
        return 2; /* 0x415264-0x415269 pops + retn */
    }
    ecx = (ecx & 0xFFFF0000UL) | (unsigned int)G16(0x440E7A); /* 0x41526A mov cx,[440E78+2] */
    ecx += 0x20u; /* 0x415271 add ecx,20h */
    eax = 0x200000u; /* 0x415274 mov eax,200000h */
    G16(0x440E7A) = (unsigned short)(ecx & 0xFFFFu); /* 0x415279 mov [440E78+2],cx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x415280 call sub_40D070 */
    G32(0x433B4C) = eax; /* 0x415285 mov [433B4C],eax */
    if (eax == 0u) { /* 0x41528A test eax,eax; 0x41528C jnz loc_4152A5 */
        eax = 2u; /* 0x41528E mov eax,2 */
        esi = G32(0x45BA44); /* 0x415293 mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x415299 mov [45BA44],esi */
        return 2; /* 0x41529F-0x4152A4 pops + retn */
    }
    edi = (edi & 0xFFFF0000UL) | (unsigned int)G16(0x440E7A); /* 0x4152A5 mov di,[440E78+2] (entry edi upper half kept; unused) */
    edi += 0x20u; /* 0x4152AC add edi,20h */
    eax = 0x200000u; /* 0x4152AF mov eax,200000h */
    G16(0x440E7A) = (unsigned short)(edi & 0xFFFFu); /* 0x4152B4 mov [440E78+2],di */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x4152BB call sub_40D070 */
    G32(0x433B50) = eax; /* 0x4152C0 mov [433B50],eax */
    if (eax == 0u) { /* 0x4152C5 test eax,eax; 0x4152C7 jnz loc_4152E0 */
        eax = 2u; /* 0x4152C9 mov eax,2 */
        esi = G32(0x45BA44); /* 0x4152CE mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x4152D4 mov [45BA44],esi */
        return 2; /* 0x4152DA-0x4152DF pops + retn */
    }
    G16(0x440E7A) = (unsigned short)(((unsigned int)G16(0x440E7A) + 4u) & 0xFFFFu); /* 0x4152E0 add word ptr [440E78+2],4 */
    eax = 0x40000u; /* 0x4152E8 mov eax,40000h */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x4152ED call sub_40D070 */
    G32(0x433B54) = eax; /* 0x4152F2 mov [433B54],eax */
    if (eax == 0u) { /* 0x4152F7 test eax,eax; 0x4152F9 jnz loc_415312 */
        eax = 2u; /* 0x4152FB mov eax,2 */
        esi = G32(0x45BA44); /* 0x415300 mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x415306 mov [45BA44],esi */
        return 2; /* 0x41530C-0x415311 pops + retn */
    }
    edx = (edx & 0xFFFF0000UL) | (unsigned int)G16(0x440E7A); /* 0x415312 mov dx,[440E78+2] */
    edx += 4u; /* 0x415319 add edx,4 */
    eax = 0x40000u; /* 0x41531C mov eax,40000h */
    G16(0x440E7A) = (unsigned short)(edx & 0xFFFFu); /* 0x415321 mov [440E78+2],dx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x415328 call sub_40D070 */
    G32(0x433B58) = eax; /* 0x41532D mov [433B58],eax */
    if (eax == 0u) { /* 0x415332 test eax,eax; 0x415334 jnz loc_41534D */
        eax = 2u; /* 0x415336 mov eax,2 */
        esi = G32(0x45BA44); /* 0x41533B mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x415341 mov [45BA44],esi */
        return 2; /* 0x415347-0x41534C pops + retn */
    }
    ebx = (ebx & 0xFFFF0000UL) | (unsigned int)G16(0x440E7A); /* 0x41534D mov bx,[440E78+2] */
    ebx += 8u; /* 0x415354 add ebx,8 */
    eax = 0x80000u; /* 0x415357 mov eax,80000h */
    G16(0x440E7A) = (unsigned short)(ebx & 0xFFFFu); /* 0x41535C mov [440E78+2],bx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x415363 call sub_40D070 */
    G32(0x433B5C) = eax; /* 0x415368 mov [433B5C],eax */
    if (eax == 0u) { /* 0x41536D test eax,eax; 0x41536F jnz loc_415388 */
        eax = 2u; /* 0x415371 mov eax,2 */
        esi = G32(0x45BA44); /* 0x415376 mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x41537C mov [45BA44],esi */
        return 2; /* 0x415382-0x415387 pops + retn */
    }
    ecx = (ecx & 0xFFFF0000UL) | (unsigned int)G16(0x440E7A); /* 0x415388 mov cx,[440E78+2] */
    ecx += 8u; /* 0x41538F add ecx,8 */
    eax = 0x80000u; /* 0x415392 mov eax,80000h */
    G16(0x440E7A) = (unsigned short)(ecx & 0xFFFFu); /* 0x415397 mov [440E78+2],cx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x41539E call sub_40D070 */
    G32(0x433B60) = eax; /* 0x4153A3 mov [433B60],eax */
    if (eax == 0u) { /* 0x4153A8 test eax,eax; 0x4153AA jnz loc_4153C3 */
        eax = 2u; /* 0x4153AC mov eax,2 */
        esi = G32(0x45BA44); /* 0x4153B1 mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x4153B7 mov [45BA44],esi */
        return 2; /* 0x4153BD-0x4153C2 pops + retn */
    }
    edi = (edi & 0xFFFF0000UL) | (unsigned int)G16(0x440E7A); /* 0x4153C3 mov di,[440E78+2] (16-bit load merges; upper kept from 0x4152AC value) */
    edi += 4u; /* 0x4153CA add edi,4 */
    eax = 0x40000u; /* 0x4153CD mov eax,40000h */
    G16(0x440E7A) = (unsigned short)(edi & 0xFFFFu); /* 0x4153D2 mov [440E78+2],di */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x4153D9 call sub_40D070 */
    G32(0x433B64) = eax; /* 0x4153DE mov [433B64],eax */
    if (eax == 0u) { /* 0x4153E3 test eax,eax; 0x4153E5 jnz loc_4153FE */
        eax = 2u; /* 0x4153E7 mov eax,2 */
        esi = G32(0x45BA44); /* 0x4153EC mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x4153F2 mov [45BA44],esi */
        return 2; /* 0x4153F8-0x4153FD pops + retn */
    }
    G16(0x440E7A) = (unsigned short)(((unsigned int)G16(0x440E7A) + 4u) & 0xFFFFu); /* 0x4153FE add word ptr [440E78+2],4 */
    eax = 0x40000u; /* 0x415406 mov eax,40000h */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x41540B call sub_40D070 */
    G32(0x433B68) = eax; /* 0x415410 mov [433B68],eax */
    if (eax == 0u) { /* 0x415415 test eax,eax; 0x415417 jnz loc_415430 */
        eax = 2u; /* 0x415419 mov eax,2 */
        esi = G32(0x45BA44); /* 0x41541E mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x415424 mov [45BA44],esi */
        return 2; /* 0x41542A-0x41542F pops + retn */
    }
    edx = (edx & 0xFFFF0000UL) | (unsigned int)G16(0x440E7A); /* 0x415430 mov dx,[440E78+2] */
    edx += 4u; /* 0x415437 add edx,4 */
    eax = 0x40000u; /* 0x41543A mov eax,40000h */
    G16(0x440E7A) = (unsigned short)(edx & 0xFFFFu); /* 0x41543F mov [440E78+2],dx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x415446 call sub_40D070 */
    G32(0x433B6C) = eax; /* 0x41544B mov [433B6C],eax */
    if (eax == 0u) { /* 0x415450 test eax,eax; 0x415452 jnz loc_41546B */
        eax = 2u; /* 0x415454 mov eax,2 */
        esi = G32(0x45BA44); /* 0x415459 mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x41545F mov [45BA44],esi */
        return 2; /* 0x415465-0x41546A pops + retn */
    }
    ebx = (ebx & 0xFFFF0000UL) | (unsigned int)G16(0x440E7A); /* 0x41546B mov bx,[440E78+2] */
    ebx += 0x1Cu; /* 0x415472 add ebx,1Ch */
    eax = 0x1C0000u; /* 0x415475 mov eax,1C0000h */
    G16(0x440E7A) = (unsigned short)(ebx & 0xFFFFu); /* 0x41547A mov [440E78+2],bx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x415481 call sub_40D070 */
    G32(0x433B70) = eax; /* 0x415486 mov [433B70],eax */
    if (eax == 0u) { /* 0x41548B test eax,eax; 0x41548D jnz loc_4154A6 */
        eax = 2u; /* 0x41548F mov eax,2 */
        esi = G32(0x45BA44); /* 0x415494 mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x41549A mov [45BA44],esi */
        return 2; /* 0x4154A0-0x4154A5 pops + retn */
    }
    ecx = (ecx & 0xFFFF0000UL) | (unsigned int)G16(0x440E7A); /* 0x4154A6 mov cx,[440E78+2] */
    ecx += 4u; /* 0x4154AD add ecx,4 */
    eax = 0x40000u; /* 0x4154B0 mov eax,40000h */
    G16(0x440E7A) = (unsigned short)(ecx & 0xFFFFu); /* 0x4154B5 mov [440E78+2],cx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x4154BC call sub_40D070 */
    G32(0x433B74) = eax; /* 0x4154C1 mov [433B74],eax */
    if (eax == 0u) { /* 0x4154C6 test eax,eax; 0x4154C8 jnz loc_4154E1 */
        eax = 2u; /* 0x4154CA mov eax,2 */
        esi = G32(0x45BA44); /* 0x4154CF mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x4154D5 mov [45BA44],esi */
        return 2; /* 0x4154DB-0x4154E0 pops + retn */
    }
    G32(0x440E78) = G32(0x440E78) + 0x61A00u; /* 0x4154E1 add [440E78],61A00h */
    eax = 0x61A00u; /* 0x4154EB mov eax,61A00h */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x4154F0 call sub_40D070 */
    G32(0x433B78) = eax; /* 0x4154F5 mov [433B78],eax */
    if (eax == 0u) { /* 0x4154FA test eax,eax; 0x4154FC jnz loc_415515 */
        eax = 2u; /* 0x4154FE mov eax,2 */
        esi = G32(0x45BA44); /* 0x415503 mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x415509 mov [45BA44],esi */
        return 2; /* 0x41550F-0x415514 pops + retn */
    }
    edx = G32(0x440E78); /* 0x415515 mov edx,[440E78] */
    edx += 0x61A00u; /* 0x41551B add edx,61A00h */
    eax = 0x61A00u; /* 0x415521 mov eax,61A00h */
    G32(0x440E78) = edx; /* 0x415526 mov [440E78],edx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x41552C call sub_40D070 */
    G32(0x433B7C) = eax; /* 0x415531 mov [433B7C],eax */
    if (eax == 0u) { /* 0x415536 test eax,eax; 0x415538 jnz loc_415551 */
        eax = 2u; /* 0x41553A mov eax,2 */
        esi = G32(0x45BA44); /* 0x41553F mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x415545 mov [45BA44],esi */
        return 2; /* 0x41554B-0x415550 pops + retn */
    }
    ecx = G32(0x440E78); /* 0x415551 mov ecx,[440E78] */
    ecx += 0x0C000u; /* 0x415557 add ecx,0C000h */
    eax = 0x0C000u; /* 0x41555D mov eax,0C000h */
    G32(0x440E78) = ecx; /* 0x415562 mov [440E78],ecx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x415568 call sub_40D070 */
    G32(0x433B80) = eax; /* 0x41556D mov [433B80],eax */
    if (eax == 0u) { /* 0x415572 test eax,eax; 0x415574 jnz loc_41558D */
        eax = 2u; /* 0x415576 mov eax,2 */
        esi = G32(0x45BA44); /* 0x41557B mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x415581 mov [45BA44],esi */
        return 2; /* 0x415587-0x41558C pops + retn */
    }
    ebx = G32(0x440E78); /* 0x41558D mov ebx,[440E78] */
    ebx += 0x5FEu; /* 0x415593 add ebx,5FEh */
    eax = 0x5FEu; /* 0x415599 mov eax,5FEh */
    G32(0x440E78) = ebx; /* 0x41559E mov [440E78],ebx */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x4155A4 call sub_40D070 */
    G32(0x433B84) = eax; /* 0x4155A9 mov [433B84],eax */
    if (eax == 0u) { /* 0x4155AE test eax,eax; 0x4155B0 jnz loc_4155C9 */
        eax = 2u; /* 0x4155B2 mov eax,2 */
        esi = G32(0x45BA44); /* 0x4155B7 mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x4155BD mov [45BA44],esi */
        return 2; /* 0x4155C3-0x4155C8 pops + retn */
    }
    edi = G32(0x440E78); /* 0x4155C9 mov edi,[440E78] */
    edi += 0x0C0u; /* 0x4155CF add edi,0C0h */
    eax = 0x0C0u; /* 0x4155D5 mov eax,0C0h */
    G32(0x440E78) = edi; /* 0x4155DA mov [440E78],edi */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x4155E0 call sub_40D070 */
    G32(0x433B88) = eax; /* 0x4155E5 mov [433B88],eax */
    if (eax == 0u) { /* 0x4155EA test eax,eax; 0x4155EC jnz loc_415605 */
        eax = 2u; /* 0x4155EE mov eax,2 */
        esi = G32(0x45BA44); /* 0x4155F3 mov esi,[45BA44] */
        G32(0x45BA44) = esi; /* 0x4155F9 mov [45BA44],esi */
        return 2; /* 0x4155FF-0x415604 pops + retn */
    }
    ebp = G32(0x440E78); /* 0x415605 mov ebp,[440E78] */
    ebp += 0x0C0u; /* 0x41560B add ebp,0C0h */
    eax = 0x0C0u; /* 0x415611 mov eax,0C0h */
    G32(0x440E78) = ebp; /* 0x415616 mov [440E78],ebp */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x41561C call sub_40D070 */
    G32(0x433B8C) = eax; /* 0x415621 mov [433B8C],eax */
    if (eax == 0u) goto alloc_fail; /* 0x415626 test eax,eax; 0x415628 jnz loc_415641 (fallthrough is fail) */
    goto alloc_ok;
alloc_fail:
    eax = 2u; /* 0x41562A mov eax,2 (shared exit for 433B8C and 45AF34) */
    esi = G32(0x45BA44); /* 0x41562F mov esi,[45BA44] */
    G32(0x45BA44) = esi; /* 0x415635 mov [45BA44],esi */
    return 2; /* 0x41563B-0x415640 pops + retn */
alloc_ok:
    G32(0x440E78) = G32(0x440E78) + 0x25800u; /* 0x415641 add [440E78],25800h */
    eax = 0x25800u; /* 0x41564B mov eax,25800h */
    eax = (unsigned int)d4v_alloc(c, (unsigned long)eax); /* 0x415650 call sub_40D070 */
    G32(0x45AF34) = eax; /* 0x415655 mov [45AF34],eax */
    if (eax == 0u) goto alloc_fail; /* 0x41565A test eax,eax; 0x41565C jz loc_41562A */
    /* sub_40D570 fills: ebx=len, eax=dst, edx=val -> memset */
    ebx = G32(0x45BA30); /* 0x41565E mov ebx,[45BA30] */
    eax = G32(0x433B40); /* 0x415664 mov eax,[433B40] */
    edx = 0u; /* 0x415669 xor edx,edx */
    ebx += 0x100u; /* 0x41566B add ebx,100h */
    memset((void *)(c->mem + (unsigned)eax), (int)(edx & 0xFFu), (size_t)ebx); /* 0x415671 call sub_40D570 */
    ebx = 0x100000u; /* 0x415676 mov ebx,100000h (NOTE: fill len is 0x100000, smaller than the 0x800000 alloc) */
    eax = G32(0x433B44); /* 0x41567B mov eax,[433B44] */
    edx = 0u; /* 0x415680 xor edx,edx */
    memset((void *)(c->mem + (unsigned)eax), (int)(edx & 0xFFu), (size_t)ebx); /* 0x415682 call sub_40D570 */
    ebx = 0x140000u; /* 0x415687 mov ebx,140000h */
    eax = G32(0x433B48); /* 0x41568C mov eax,[433B48] */
    edx = 0u; /* 0x415691 xor edx,edx */
    memset((void *)(c->mem + (unsigned)eax), (int)(edx & 0xFFu), (size_t)ebx); /* 0x415693 call sub_40D570 */
    ebx = 0x200000u; /* 0x415698 mov ebx,200000h */
    edx = 0xFFu; /* 0x41569D mov edx,0FFh */
    eax = G32(0x433B4C); /* 0x4156A2 mov eax,[433B4C] */
    memset((void *)(c->mem + (unsigned)eax), (int)(edx & 0xFFu), (size_t)ebx); /* 0x4156A7 call sub_40D570 */
    ebx = 0x200000u; /* 0x4156AC mov ebx,200000h */
    edx = 0xFFu; /* 0x4156B1 mov edx,0FFh */
    eax = G32(0x433B50); /* 0x4156B6 mov eax,[433B50] */
    memset((void *)(c->mem + (unsigned)eax), (int)(edx & 0xFFu), (size_t)ebx); /* 0x4156BB call sub_40D570 */
    ebx = 0x40000u; /* 0x4156C0 mov ebx,40000h */
    eax = G32(0x433B54); /* 0x4156C5 mov eax,[433B54] */
    edx = 0u; /* 0x4156CA xor edx,edx */
    memset((void *)(c->mem + (unsigned)eax), (int)(edx & 0xFFu), (size_t)ebx); /* 0x4156CC call sub_40D570 */
    ebx = 0x1F800u; /* 0x4156D1 mov ebx,1F800h */
    eax = G32(0x433B78); /* 0x4156D6 mov eax,[433B78] */
    edx = 0u; /* 0x4156DB xor edx,edx */
    eax += 0x40600u; /* 0x4156DD add eax,40600h */
    memset((void *)(c->mem + (unsigned)eax), (int)(edx & 0xFFu), (size_t)ebx); /* 0x4156E2 call sub_40D570 */
    ebx = 0x1F800u; /* 0x4156E7 mov ebx,1F800h */
    eax = G32(0x433B7C); /* 0x4156EC mov eax,[433B7C] */
    edx = 0u; /* 0x4156F1 xor edx,edx */
    eax += 0x40600u; /* 0x4156F3 add eax,40600h */
    memset((void *)(c->mem + (unsigned)eax), (int)(edx & 0xFFu), (size_t)ebx); /* 0x4156F8 call sub_40D570 */
    /* sub_425810 u16 fills: ebx=count, edx=dst, eax=val -> d4v_fill16 */
    ebx = 0xFCu; /* 0x4156FD mov ebx,0FCh */
    edx = G32(0x433B84); /* 0x415702 mov edx,[433B84] */
    eax = 1u; /* 0x415708 mov eax,1 */
    edx += 0x406u; /* 0x41570D add edx,406h */
    d4v_fill16(c, ebx, edx, eax); /* 0x415713 call sub_425810 */
    ebx = 0x2FFu; /* 0x415718 mov ebx,2FFh */
    edx = 0x45BA5Eu; /* 0x41571D mov edx,offset word_45BA5E */
    eax = 0x20u; /* 0x415722 mov eax,20h */
    d4v_fill16(c, ebx, edx, eax); /* 0x415727 call sub_425810 */
    ebx = 0x2FFu; /* 0x41572C mov ebx,2FFh */
    edx = 0x45C05Cu; /* 0x415731 mov edx,offset word_45C05C */
    eax = 0x10u; /* 0x415736 mov eax,10h */
    d4v_fill16(c, ebx, edx, eax); /* 0x41573B call sub_425810 */
    ebx = 8u; /* 0x415740 mov ebx,8 */
    edx = 0x45C044u; /* 0x415745 mov edx,offset unk_45C044 */
    eax = 0x0Au; /* 0x41574A mov eax,0Ah */
    esi = 6u; /* 0x41574F mov esi,6 (survives the call: callee saves esi; read as si at 0x415774) */
    d4v_fill16(c, ebx, edx, eax); /* 0x415754 call sub_425810 */
    ebx = 0x18u; /* 0x415759 mov ebx,18h */
    edx = 0x45C622u; /* 0x41575E mov edx,offset unk_45C622 */
    eax = 8u; /* 0x415763 mov eax,8 */
    edi = 0u; /* 0x415768 xor edi,edi (survives: callee does not touch edi) */
    d4v_fill16(c, ebx, edx, eax); /* 0x41576A call sub_425810 */
    edx = 0xFFFFFFFFu; /* 0x41576F mov edx,0FFFFFFFFh */
    G16(0x45C45C) = (unsigned short)(esi & 0xFFFFu); /* 0x415774 mov [45C45C],si (=6) */
    ebx = 0u; /* 0x41577B xor ebx,ebx */
    G32(0x45BA50) = edx; /* 0x41577D mov [45BA50],edx (=-1) */
    G32(0x45BA48) = edx; /* 0x415783 mov [45BA48],edx (=-1) */
    G32(0x45BA38) = ebx; /* 0x415789 mov [45BA38],ebx (=0) */
    G32(0x45BA40) = ebx; /* 0x41578F mov [45BA40],ebx (=0) */
    G32(0x45BA44) = edx; /* 0x415795 mov [45BA44],edx (=-1) */
    /* 0x41579B call sub_40DA80: native eax = G32(0x431280) (heap base) */
    eax = 0u; /* PORT SUBSTITUTION for 0x41579B: heap base has no arena meaning; 45AFB0=0 so the tail gate always takes loc_415949 below */
    edx = 1u; /* 0x4157A0 mov edx,1 */
    esi = G32(0x45BA44); /* 0x4157A5 mov esi,[45BA44] (=-1) */
    G32(0x45AFB0) = eax; /* 0x4157AB mov [45AFB0],eax (=0 in this port) */
    G32(0x45AFB4) = ebx; /* 0x4157B0 mov [45AFB4],ebx (=0) */
    G32(0x45BA2C) = ebx; /* 0x4157B6 mov [45BA2C],ebx (=0) */
    G32(0x45BA18) = edi; /* 0x4157BC mov [45BA18],edi (=0) */
    G32(0x45BA1C) = edi; /* 0x4157C2 mov [45BA1C],edi (=0) */
    G32(0x45BA20) = edi; /* 0x4157C8 mov [45BA20],edi (=0) */
    G32(0x45BA28) = edi; /* 0x4157CE mov [45BA28],edi (=0) */
    G32(0x45AF38) = edi; /* 0x4157D4 mov [45AF38],edi (=0) */
    G32(0x45AF48) = edi; /* 0x4157DA mov [45AF48],edi (=0) */
    G32(0x457360) = edi; /* 0x4157E0 mov [457360],edi (=0) */
    G32(0x457374) = edi; /* 0x4157E6 mov [457374],edi (=0) */
    G32(0x45AF2C) = edi; /* 0x4157EC mov [45AF2C],edi (=0) */
    eax = G32(0x45CD14); /* 0x4157F2 mov eax,[45CD14] (caller-provided image dword) */
    G32(0x45AFD0) = edx; /* 0x4157F7 mov [45AFD0],edx (=1) */
    ecx = edx; /* 0x4157FD mov ecx,edx (=1) */
    G32(0x45AFA8) = edx; /* 0x4157FF mov [45AFA8],edx (=1) */
    ebx = edx; /* 0x415805 mov ebx,edx (=1) */
    G32(0x45BA24) = edx; /* 0x415807 mov [45BA24],edx (=1) */
    G32(0x45AF44) = eax; /* 0x41580D mov [45AF44],eax (=[45CD14]) */
    edx &= 0xFFFFFF00UL; /* 0x415812 xor dl,dl (edx 1 -> 0) */
    ebx = (ebx & 0xFFFFFF00UL) | ((ebx ^ ecx) & 0xFFUL); /* 0x415814 xor bl,cl (bl 1^1 -> 0; ebx -> 0) */
    G8(0x45C691) = (unsigned char)(edx & 0xFFu); /* 0x415816 mov [45C691],dl (=0) */
    eax = 0u; /* 0x41581C xor eax,eax */
    G8(0x45C67F) = (unsigned char)(ebx & 0xFFu); /* 0x41581E mov [45C67F],bl (=0) */
    G32(0x457364) = eax; /* 0x415824 mov [457364],eax (=0) */
    G32(0x457368) = eax; /* 0x415829 mov [457368],eax (=0) */
    G32(0x45736C) = eax; /* 0x41582E mov [45736C],eax (=0) */
    G32(0x457370) = eax; /* 0x415833 mov [457370],eax (=0) */
    G32(0x45AF28) = eax; /* 0x415838 mov [45AF28],eax (=0) */
    G32(0x45AF30) = eax; /* 0x41583D mov [45AF30],eax (=0) */
    edx &= 0xFFFF00FFUL; /* 0x415842 xor dh,dh (already 0) */
    ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) ^ (ecx >> 8)) & 0xFFUL) << 8); /* 0x415844 xor bh,ch (bh 0^ch 0 -> 0) */
    G8(0x45C686) = (unsigned char)((edx >> 8) & 0xFFu); /* 0x415846 mov [45C686],dh (=0) */
    G8(0x45C689) = (unsigned char)((ebx >> 8) & 0xFFu); /* 0x41584C mov [45C689],bh (=0) */
    G8(0x45C679) = (unsigned char)(edx & 0xFFu); /* 0x415852 mov [45C679],dl (=0) */
    G8(0x45C67A) = (unsigned char)((edx >> 8) & 0xFFu); /* 0x415858 mov [45C67A],dh (=0) */
    G8(0x45BA5B) = (unsigned char)(edx & 0xFFu); /* 0x41585E mov [45BA5B],dl (=0) */
    ecx = 0u; /* 0x415864 xor ecx,ecx */
    G8(0x45BA5C) = (unsigned char)((edx >> 8) & 0xFFu); /* 0x415866 mov [45BA5C],dh (=0; dh is edx lane, unaffected by xor ecx,ecx) */
    G32(0x45AF4C) = ecx; /* 0x41586C mov [45AF4C],ecx (=0) */
    ecx &= 0xFFFFFF00UL; /* 0x415872 xor cl,cl (already 0) */
    ebx = 0u; /* 0x415874 xor ebx,ebx (already 0) */
    G8(0x45C688) = (unsigned char)(ecx & 0xFFu); /* 0x415876 mov [45C688],cl (=0) */
    G8(0x45C67E) = (unsigned char)(ecx & 0xFFu); /* 0x41587C mov [45C67E],cl (=0) */
    G8(0x45C67B) = (unsigned char)(ecx & 0xFFu); /* 0x415882 mov [45C67B],cl (=0) */
    G8(0x45C67D) = (unsigned char)(ecx & 0xFFu); /* 0x415888 mov [45C67D],cl (=0) */
    G8(0x45C67C) = (unsigned char)(ecx & 0xFFu); /* 0x41588E mov [45C67C],cl (=0) */
    G8(0x45C677) = (unsigned char)(ecx & 0xFFu); /* 0x415894 mov [45C677],cl (=0) */
    G8(0x45C678) = (unsigned char)(ecx & 0xFFu); /* 0x41589A mov [45C678],cl (=0) */
    G8(0x45BA58) = (unsigned char)(ecx & 0xFFu); /* 0x4158A0 mov [45BA58],cl (=0) */
    G8(0x45BA59) = (unsigned char)(ecx & 0xFFu); /* 0x4158A6 mov [45BA59],cl (=0) */
    G8(0x45BA5A) = (unsigned char)(ecx & 0xFFu); /* 0x4158AC mov [45BA5A],cl (=0) */
    G8(0x45BA5D) = (unsigned char)(ecx & 0xFFu); /* 0x4158B2 mov [45BA5D],cl (=0) */
    ecx = 0u; /* 0x4158B8 xor ecx,ecx */
    ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) ^ (eax >> 8)) & 0xFFUL) << 8); /* 0x4158BA xor bh,ah (0^0 -> 0) */
    ecx = (ecx & 0xFFFFFF00UL) | ((ecx ^ eax) & 0xFFUL); /* 0x4158BC xor cl,al (0^0 -> 0) */
    G8(0x45C676) = (unsigned char)((ebx >> 8) & 0xFFu); /* 0x4158BE mov [45C676],bh (=0) */
    G8(0x45C65A) = (unsigned char)(ecx & 0xFFu); /* 0x4158C4 mov [45C65A],cl (=0) */
    ecx = (ecx & 0xFFFF00FFUL) | ((((ecx >> 8) ^ (eax >> 8)) & 0xFFUL) << 8); /* 0x4158CA xor ch,ah (0^0 -> 0) */
    eax &= 0xFFFFFF00UL; /* 0x4158CC xor al,al (already 0) */
    G8(0x45C65B) = (unsigned char)((ecx >> 8) & 0xFFu); /* 0x4158CE mov [45C65B],ch (=0) */
    G8(0x45C65C) = (unsigned char)(eax & 0xFFu); /* 0x4158D4 mov [45C65C],al (=0) */
    G8(0x45C65D) = (unsigned char)(eax & 0xFFu); /* 0x4158D9 mov [45C65D],al (=0) */
    G8(0x45C68E) = (unsigned char)(eax & 0xFFu); /* 0x4158DE mov [45C68E],al (=0) */
    /* 44D920 table, neg-index loop: eax from 0xFFFFF800 while signed <0 */
    eax = 0xFFFFF800u; /* 0x4158E3 mov eax,0FFFFF800h */
neg_loop:
    edx = eax; /* 0x4158E8 mov edx,eax */
    ecx = eax; /* 0x4158EA mov ecx,eax */
    edx &= 0xFFFu; /* 0x4158EC and edx,0FFFh */
    ecx = 0u - ecx; /* 0x4158F2 neg ecx (32-bit wrap) */
    eax++; /* 0x4158F4 inc eax (wraps FFFFFFFF->0) */
    M32(0x44D920u + edx * 4u) = ecx; /* 0x4158F5 mov [44D920+edx*4],ecx */
    if ((int)eax < 0) goto neg_loop; /* 0x4158FC test eax,eax; 0x4158FE jl loc_4158E8 */
    /* 44D920 table, 0..0x800 loop (overwrites indices 0..0x7FF with identity) */
    eax = 0u; /* 0x415900 xor eax,eax */
pos_loop:
    edx = eax; /* 0x415902 mov edx,eax */
    edx &= 0xFFFu; /* 0x415904 and edx,0FFFh */
    M32(0x44D920u + edx * 4u) = eax; /* 0x41590A mov [44D920+edx*4],eax */
    eax++; /* 0x415911 inc eax */
    if ((int)eax < 0x800) goto pos_loop; /* 0x415912 cmp eax,800h; 0x415917 jl loc_415902 */
    /* tail scan gate */
    if (G32(0x45AFB0) == 0u) goto tail_check; /* 0x415919 cmp [45AFB0],0; 0x415920 jz loc_415949 */
    if (G8(0x431209) == 0u) goto tail_check; /* 0x415922 cmp byte [431209],0; 0x415929 jz loc_415949 (static 0x00 in uharc04.exe; caller image) */
    esi = 0u; /* 0x41592B xor esi,esi */
    edx = 0u; /* 0x41592D xor edx,edx */
    bound = 0u;
scan_loop:
    /* DIVERGENCE: native loop is unbounded; 0x800000-iteration safety bound added (unreachable when gated, as in this port) */
    if (bound >= 0x800000u) goto tail_check;
    bound++;
    ecx = G32(0x45AFB0); /* 0x41592F mov ecx,[45AFB0] */
    ecx += edx; /* 0x415935 add ecx,edx */
    eax = M32(ecx); /* 0x415937 mov eax,[ecx] */
    eax -= esi; /* 0x415939 sub eax,esi */
    if (eax == 0xFB670u) goto tail_check; /* 0x41593B cmp eax,0FB670h; 0x415940 jz loc_415949 */
    edx += 4u; /* 0x415942 add edx,4 */
    esi = M32(ecx); /* 0x415945 mov esi,[ecx] */
    goto scan_loop; /* 0x415947 jmp loc_41592F */
tail_check:
    if (esi != 0u) goto tail_found; /* 0x415949 test esi,esi; 0x41594B jnz loc_415965 */
    eax = 0x0Au; /* 0x41594D mov eax,0Ah */
    G8(0x45C681) = (unsigned char)(eax & 0xFFu); /* 0x415952 mov [45C681],al (=0x0A) */
    eax = 0u; /* 0x415957 xor eax,eax */
    G32(0x45BA44) = esi; /* 0x415959 mov [45BA44],esi (=0) */
    return 0; /* 0x41595F-0x415964 pops + retn */
tail_found:
    eax = 0u; /* 0x415965 xor eax,eax */
    G8(0x45C681) = (unsigned char)(eax & 0xFFu); /* 0x415967 mov [45C681],al (=0) */
    eax = 0u; /* 0x41596C xor eax,eax */
    G32(0x45BA44) = esi; /* 0x41596E mov [45BA44],esi (=esi-prev; 0xFFFFFFFF when gate-taken, see header) */
    return 0; /* 0x415974-0x415979 pops + retn */
}

/* ---- workers: LZP/PPM block paths (from draft_415c40.c) ---- */
/* 11-byte seed copied from dword_4150D0 by sub_416220 (416352 movsd/
 * movsd/movsw/movsb). Bytes 8..10 are the code bytes at 0x4150D8
 * (aaa/pop ecx/nop = 37 59 90) read as data.
 */
/* UNCERTAIN(00416352): trailing 3 bytes are code, kept for fidelity. */
static const unsigned char d4v_t4150d0[11] = {
    0x00, 0x01, 0x02, 0x05, 0x08, 0x0D, 0x15, 0x22, 0x37, 0x59, 0x90
};

/* ------------------------------------------------------------------ */
/* sub_415C40 (lst 31954-32415) -> d4v_lzp                              */
/*                                                                      */
/* Role hypothesis: LZP/literal block path. Preloads a 0x100-byte       */
/* window from the dict, runs the LZP copy/match state machine over     */
/* the [t2c..edi) range (tables byte_436976/byte_43691C/off_4362C0),    */
/* emits the 256 buffered bytes via the model-update loop (459E58 +    */
/* dict store), re-seeds the dict guard copy, rescales both models      */
/* (sub_415B50/sub_415AE0), clears the hash tables, then runs the       */
/* literal/match loop keyed on the 45BA18/45BA1C/45BA20 shift           */
/* registers (literals via sub_417220).                                 */
/* Entry-live regs: none (eax/ebx/ecx/edx/esi/edi/ebp all written       */
/* before any read; no stack args; callees take none). Returns void     */
/* (both callers ignore eax).                                           */
/* Frame: 0x114 bytes + 6 pushed regs. Locals: buf[256] @ sp+0,         */
/* t2c @ sp+0x100 (var_2C), t28 @ sp+0x104, t24 @ sp+0x108,             */
/* t20 @ sp+0x10C, t1c @ sp+0x110.                                      */
/* Globals touched: 433B40,45AFD0,45AFA8,45BA34,45BA24,45BA3C,45BA30,   */
/* 45AF38,45C689,45C68C,T436976,459E58,433B4C,433B50,45BA18,45BA1C,     */
/* 45BA20,45C683,45C68F,T43691C,T4362C0,T436450,45AFC4,45AFAC,433B44,   */
/* 45C692,45AF9C,433B78,433B7C,433B84,45C675,45BA28,45AFB4,45BA50,      */
/* 45C67F,45CD28 (T = static image table read through arena).           */
/* ------------------------------------------------------------------ */
void d4v_lzp(dec4v_ctx *c)
{
    unsigned long eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned char buf[256] = {0};
    unsigned long t2c = 0, t28 = 0, t24 = 0, t20 = 0, t1c = 0;
    unsigned long a_ = 0;

    /* prologue 0x00415C40 push ebx; 0x00415C41 push ecx;
     * 0x00415C42 push edx; 0x00415C43 push esi; 0x00415C44 push edi;
     * 0x00415C45 push ebp; 0x00415C46 sub esp,114h.
     * Callee-saved regs become C locals; the esp frame is buf/t2c..t1c. */

    /* 0x00415C4C */ ebp = (unsigned long)G32(0x433B40);
    /* 0x00415C52 */ eax = (unsigned long)G32(0x45AFD0);
    /* 0x00415C57 */ t24 = eax;
    /* 0x00415C5E */ eax = (unsigned long)G32(0x45AFA8);
    /* 0x00415C63 */ edx = (unsigned long)G32(0x45BA34);
    /* 0x00415C69 */ t20 = eax;
    /* 0x00415C70 */ eax = (unsigned long)G32(0x45BA24);
    /* 0x00415C75 */ ecx = (unsigned long)G32(0x45BA3C);
    /* 0x00415C7B */ t28 = eax;
    /* 0x00415C82 */ eax = 0; /* xor eax,eax */
l_415C84:
    /* 0x00415C84 */ ebx = ebp;
    /* 0x00415C86 */ edx = (edx + 1) & U32MASK;
    /* 0x00415C87 */ eax = (eax + 1) & U32MASK;
    /* 0x00415C88 */ ebx = (ebx & 0xFFFFFF00UL) |
        (unsigned long)M8((ebx + edx - 1) & U32MASK);
    /* 0x00415C8C */ edx &= ecx;
    /* 0x00415C8E */ buf[(eax - 1) & 0xFF] = (unsigned char)(ebx & 0xFFUL);
    /* 0x00415C92 */ /* cmp eax,100h */;
    /* 0x00415C97 jb */ if (eax < 0x100UL) goto l_415C84; /* unsigned */
    /* 0x00415C99 */ edx = (unsigned long)G32(0x45BA30);
    /* 0x00415C9F */ eax = (unsigned long)G32(0x45BA34);
    /* 0x00415CA4 */ edx = (edx - 0x140UL) & U32MASK;
    /* 0x00415CAA */ eax = (eax + 0x140UL) & U32MASK;
    /* 0x00415CAF */ edx >>= 1;
    /* 0x00415CB1 */ eax &= ecx;
    /* 0x00415CB3 */ edi = (eax + edx) & U32MASK;
    /* 0x00415CB6 */ edi &= ecx;
    /* 0x00415CB8 */ /* cmp edx,[45AF38] */;
    /* 0x00415CBE jbe */ if (edx <= (unsigned long)G32(0x45AF38)) /* unsigned */
        goto l_415CC2;
    /* 0x00415CC0 */ edi = 0;
l_415CC2:
    /* 0x00415CC2 */ ebx = (unsigned long)G32(0x45BA34);
    /* 0x00415CC8 */ edx &= 0xFFFFFF00UL; /* xor dl,dl */
    /* 0x00415CCA */ t2c = eax;
    /* 0x00415CD1 */ G8(0x45C689) = 0;
    /* 0x00415CD7 */ /* cmp edi,ebx */;
    /* 0x00415CD9 jz */ if (edi == ebx) goto l_415D2F;
l_415CDB:
    /* 0x00415CDB */ edx = (edi + ebp) & U32MASK; /* lea */
    /* 0x00415CDE */ esi = (unsigned long)G32(0x45BA3C);
    /* 0x00415CE4 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8(edx);
    /* 0x00415CE6 */ ecx = (eax + 1) & U32MASK; /* lea */
    /* 0x00415CE9 */ edx = (edx & 0xFFFFFF00UL) |
        (unsigned long)((~(edx & 0xFFUL)) & 0xFFUL); /* not dl */
    /* 0x00415CEB */ ebx = (eax + ebp) & U32MASK; /* lea */
    /* 0x00415CEE */ edx = (edx & 0xFFFF00FFUL) |
        ((unsigned long)G8(0x45C689) << 8); /* mov dh,[45C689] */
    /* 0x00415CF4 */ ecx &= esi;
    /* 0x00415CF6 */ /* cmp dh,1 */;
    /* 0x00415CF9 jnz */ if (((edx >> 8) & 0xFFUL) != 1) goto l_416018;
    /* 0x00415CFF */ esi = (unsigned long)G8(0x45C68C); /* movzx */
    /* 0x00415D06 */ esi = (esi - 0xA6UL) & U32MASK;
    /* 0x00415D0C */ esi = (esi * 0xBUL) & U32MASK; /* imul esi,0Bh */
    /* 0x00415D0F */ eax &= 0xFFFFFF00UL; /* xor al,al */
    /* 0x00415D11 */ G8(0x45C689) = (unsigned char)(eax & 0xFFUL);
    /* 0x00415D16 */ eax = 0; /* xor eax,eax */
    /* 0x00415D18 */ eax = (eax & 0xFFFFFF00UL) | (edx & 0xFFUL); /* mov al,dl */
    /* 0x00415D1A */ a_ = (0x436976UL + esi + (eax & 0xFFUL)) & U32MASK;
    eax = (eax & 0xFFFFFF00UL) | (unsigned long)G8(a_);
l_415D21:
    /* 0x00415D21 */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)((~(eax & 0xFFUL)) & 0xFFUL); /* not al */
    /* 0x00415D23 */ M8(ebx) = (unsigned char)(eax & 0xFFUL);
    /* 0x00415D25 */ eax = ecx;
l_415D27:
    /* 0x00415D27 */ /* cmp edi,eax */;
    /* 0x00415D29 jnz */ if (edi != eax) goto l_4160E5;
l_415D2F:
    /* 0x00415D2F */ edx &= 0xFFFFFF00UL; /* xor dl,dl */
    /* 0x00415D31 */ G8(0x45C689) = (unsigned char)(edx & 0xFFUL);
    /* 0x00415D37 */ edx = (unsigned long)G32(0x45BA54);
    /* 0x00415D3D */ ebx = (unsigned long)G32(0x45BA34);
    /* 0x00415D43 */ edx = (edx + eax) & U32MASK;
    /* 0x00415D45 */ ecx = (unsigned long)G32(0x45BA3C);
    /* 0x00415D4B */ edx = (edx - ebx) & U32MASK;
    /* 0x00415D4D */ esi = 1;
    /* 0x00415D52 */ edx &= ecx;
    /* 0x00415D54 */ G32(0x45BA34) = (unsigned int)eax;
    /* 0x00415D59 */ G32(0x45BA54) = (unsigned int)edx;
    /* 0x00415D5F */ edx = 0; /* xor edx,edx */
l_415D61:
    /* 0x00415D61 */ ecx = eax;
    /* 0x00415D63 */ ecx &= 0xFFUL;
    /* 0x00415D69 */ M32(0x459E58UL + ecx * 4) = (unsigned int)esi; /* =1 */
    /* 0x00415D70 */ ecx = ebp;
    /* 0x00415D72 */ ebx = (ebx & 0xFFFFFF00UL) |
        (unsigned long)buf[edx & 0xFF];
    /* 0x00415D75 */ edi = (unsigned long)G32(0x45BA3C);
    /* 0x00415D7B */ M8((ecx + eax) & U32MASK) = (unsigned char)(ebx & 0xFFUL);
    /* 0x00415D7E */ eax = (eax + esi) & U32MASK;
    /* 0x00415D80 */ edx = (edx + esi) & U32MASK;
    /* 0x00415D82 */ eax &= edi;
    /* 0x00415D84 */ /* cmp edx,100h */;
    /* 0x00415D8A jb */ if (edx < 0x100UL) goto l_415D61; /* unsigned */
    /* 0x00415D8C */ edx = t2c;
    /* 0x00415D93 */ eax = 0x40UL;
    /* 0x00415D98 */ G32(0x433B40) = (unsigned int)ebp;
l_415D9E:
    /* 0x00415D9E */ ebp = (unsigned long)G32(0x45BA3C);
    /* 0x00415DA4 */ edx = (edx - 1) & U32MASK; /* dec */
    /* 0x00415DA5 */ edx &= ebp;
    /* 0x00415DA7 */ ebp = (unsigned long)G32(0x433B40);
    /* 0x00415DAD */ esi = ebp;
    /* 0x00415DAF */ ecx = (edx + ebp) & U32MASK; /* lea */
    /* 0x00415DB2 */ M8(ecx) = 0;
    /* 0x00415DB5 */ eax = (eax - 1) & U32MASK; /* dec */
    /* 0x00415DB6 jnz */ if (eax != 0) goto l_415D9E;
    /* 0x00415DB8 */ ecx = 0x100UL;
    /* 0x00415DBD */ edi = (unsigned long)G32(0x45BA30);
    /* 0x00415DC3 */ ebx = 0x200000UL;
    /* 0x00415DC8 */ edi = (edi + ebp) & U32MASK;
    /* 0x00415DCA */ edx = 0xFFUL;
    /* 0x00415DCF cld (forward moves assumed; no std in function) */;
    /* 0x00415DD0 */ eax = ecx;
    /* 0x00415DD2 */ ecx = (ecx - edi) & U32MASK;
    /* 0x00415DD4 */ ecx = (ecx - eax) & U32MASK;
    /* 0x00415DD6 */ ecx &= 3UL;
    /* 0x00415DD9 */ eax = (eax - ecx) & U32MASK;
    /* 0x00415DDB jle */ if ((int)(eax & U32MASK) > 0) { /* signed 32-bit */
        /* 0x00415DDD rep movsb */
        while (ecx != 0) {
            M8(edi) = M8(esi);
            esi = (esi + 1) & U32MASK;
            edi = (edi + 1) & U32MASK;
            ecx = (ecx - 1) & U32MASK;
        }
        /* 0x00415DDF */ ecx = eax;
        /* 0x00415DE1 */ eax &= 3UL;
        /* 0x00415DE4 */ ecx >>= 2;
        /* 0x00415DE7 rep movsd */
        while (ecx != 0) {
            M32(edi) = M32(esi);
            esi = (esi + 4) & U32MASK;
            edi = (edi + 4) & U32MASK;
            ecx = (ecx - 1) & U32MASK;
        }
    }
l_415DE9:
    /* 0x00415DE9 */ ecx = (ecx + eax) & U32MASK;
    /* 0x00415DEB rep movsb */
    while (ecx != 0) {
        M8(edi) = M8(esi);
        esi = (esi + 1) & U32MASK;
        edi = (edi + 1) & U32MASK;
        ecx = (ecx - 1) & U32MASK;
    }
    /* 0x00415DED */ d4v_415b50(c); /* call sub_415B50 */
    /* 0x00415DF2 */ d4v_415ae0(c); /* call sub_415AE0 */
    /* 0x00415DF7 */ eax = (unsigned long)G32(0x433B4C);
    /* 0x00415DFC */ d4v_fill(c, (unsigned int)ebx /*0x200000*/,
        (unsigned int)eax, (unsigned int)(edx & 0xFFUL) /*0xFF*/);
    /* 0x00415E01 */ ebx = 0x200000UL;
    /* 0x00415E06 */ edx = 0xFFUL;
    /* 0x00415E0B */ eax = (unsigned long)G32(0x433B50);
    /* 0x00415E10 */ esi = t2c;
    /* 0x00415E17 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x00415E1C */ eax = 0; /* xor */
    /* 0x00415E1E */ edx = 0; /* xor */
    /* 0x00415E20 */ ebx &= 0xFFFFFF00UL; /* xor bl,bl */
    /* 0x00415E22 */ G32(0x45BA18) = 0;
    /* 0x00415E27 */ G32(0x45BA1C) = 0;
    /* 0x00415E2C */ G32(0x45BA20) = 0;
    /* 0x00415E31 */ G8(0x45C683) = (unsigned char)(ebx & 0xFFUL); /* =0 */
    /* 0x00415E37 */ edx &= 0xFFFF00FFUL; /* xor dh,ah: dh==ah==0 here */
    /* 0x00415E39 */ eax = (unsigned long)G32(0x45BA34);
    /* 0x00415E3E */ ebx = (unsigned long)G32(0x45BA30);
    /* 0x00415E44 */ G8(0x45C68F) = (unsigned char)((edx >> 8) & 0xFFUL);
    /* 0x00415E4A */ eax = (eax + ebx) & U32MASK;
    /* 0x00415E4C */ edx = (unsigned long)G32(0x45BA3C);
    /* 0x00415E52 */ eax = (eax - esi) & U32MASK;
    /* 0x00415E54 */ eax &= edx;
    /* 0x00415E56 */ /* cmp eax,8000h */;
    /* 0x00415E5B jbe */ if (eax <= 0x8000UL) goto l_416101; /* unsigned */
    /* 0x00415E61 */ eax = (unsigned long)G32(0x45BA34);
    /* 0x00415E66 */ eax = (eax + 0xFFFF8000UL) & U32MASK; /* +-8000h */
    /* 0x00415E6B */ edx &= eax;
    /* 0x00415E6D */ ebp = (unsigned long)G32(0x433B40);
    /* 0x00415E73 */ t1c = edx;
    goto l_415E7A;
l_415E7A:
    /* 0x00415E7A */ edi = 0x40000UL;
l_415E7F:
    /* 0x00415E7F */ ebx = (unsigned long)G32(0x45BA34);
    /* 0x00415E85 */ G32(0x433B40) = (unsigned int)ebp;
    /* 0x00415E8B */ /* cmp esi,ebx */;
    /* 0x00415E8D jz */ if (esi == ebx) goto l_416167;
    /* 0x00415E93 */ eax = (unsigned long)G32(0x45BA18);
    /* 0x00415E98 */ edx = (unsigned long)G32(0x45BA18);
    /* 0x00415E9E */ eax >>= 0xD;
    /* 0x00415EA1 */ eax ^= edx;
    /* 0x00415EA3 */ eax &= 0x3FFFFUL;
    /* 0x00415EA8 */ edx = (unsigned long)G32(0x433B4C);
    /* 0x00415EAE */ eax = (eax << 3) & U32MASK;
    /* 0x00415EB1 */ edx = (edx + eax) & U32MASK;
    /* 0x00415EB3 */ eax = (unsigned long)G32(0x45BA18);
    /* 0x00415EB8 */ ecx = (unsigned long)G32(0x45BA18);
    /* 0x00415EBE */ eax >>= 0xF;
    /* 0x00415EC1 */ ecx ^= eax;
    /* 0x00415EC3 */ eax = (unsigned long)G32(0x45BA1C);
    /* 0x00415EC8 */ ebx = (unsigned long)G32(0x45BA1C);
    /* 0x00415ECE */ eax >>= 0xF;
    /* 0x00415ED1 */ eax ^= ebx;
    /* 0x00415ED3 */ ecx = (ecx << 4) & U32MASK;
    /* 0x00415ED6 */ eax = (eax << 6) & U32MASK;
    /* 0x00415ED9 */ ecx ^= eax;
    /* 0x00415EDB */ eax = (unsigned long)G32(0x45BA20);
    /* 0x00415EE0 */ ebx = (unsigned long)G32(0x45BA20);
    /* 0x00415EE6 */ eax >>= 0xF;
    /* 0x00415EE9 */ eax ^= ebx;
    /* 0x00415EEB */ eax ^= ecx;
    /* 0x00415EED */ eax &= 0x3FFFFUL;
    /* 0x00415EF2 */ eax = (eax << 3) & U32MASK;
    /* 0x00415EF5 */ ecx = (unsigned long)G32(0x433B50);
    /* 0x00415EFB */ eax = (eax + 4) & U32MASK;
    /* 0x00415EFE */ eax = (eax + ecx) & U32MASK;
    /* 0x00415F00 */ ecx = (unsigned long)M32((eax - 4) & U32MASK);
    /* 0x00415F03 */ M32(eax) = (unsigned int)ecx;
    /* 0x00415F05 */ M32((eax - 4) & U32MASK) = (unsigned int)esi;
    /* 0x00415F08 */ eax = (unsigned long)G32(0x45BA18);
    /* 0x00415F0D */ M32((edx + 4) & U32MASK) = (unsigned int)eax;
    /* 0x00415F10 */ eax = t1c;
    /* 0x00415F17 */ M32(edx) = (unsigned int)esi;
    /* 0x00415F19 */ /* cmp esi,eax */;
    /* 0x00415F1B jnz */ if (esi != eax) goto l_416118;
    /* 0x00415F21 */ t1c = 0xFFFFFFFFUL;
l_415F2C:
    /* 0x00415F2C */ eax = (unsigned long)G32(0x45BA18);
    /* 0x00415F31 */ edx = (unsigned long)G32(0x45BA18);
    /* 0x00415F37 */ G32(0x45AFC4) = (unsigned int)eax;
    /* 0x00415F3C */ eax >>= 0xD;
    /* 0x00415F3F */ eax ^= edx;
    /* 0x00415F41 */ G32(0x45AFC4) = (unsigned int)edx;
    /* 0x00415F47 */ eax &= 0x7FFFUL;
    /* 0x00415F4C */ ebx = (unsigned long)G32(0x45AFC4);
    /* 0x00415F52 */ edx = eax;
    /* 0x00415F54 */ G32(0x45AFAC) = (unsigned int)eax;
    /* 0x00415F59 */ edx = (edx << 5) & U32MASK;
l_415F5C:
    /* 0x00415F5C */ eax = (unsigned long)G32(0x433B44);
    /* 0x00415F61 */ eax = (eax + edx) & U32MASK;
    /* 0x00415F63 */ /* cmp byte [eax],0 */;
    /* 0x00415F66 jz */ if (M8(eax) == 0) goto l_41613A;
    /* 0x00415F6C */ /* cmp ebx,[eax+1Ch] */;
    /* 0x00415F6F jnz */ if (ebx != (unsigned long)M32((eax + 0x1C) & U32MASK))
        goto l_416145;
    /* 0x00415F75 */ /* mov ch,0FFh (via stale ecx high byte) */;
    /* 0x00415F77 */ edx = 0; /* xor edx,edx */
    /* 0x00415F79 */ G8(0x45C692) = 0xFF;
    /* 0x00415F7F */ ecx = 0; /* xor ecx,ecx */
    /* 0x00415F81 */ edx |= (unsigned long)M8(eax); /* mov dl,[eax] */
    /* 0x00415F83 */ ecx |= (unsigned long)M8((eax + 1) & U32MASK);
    /* 0x00415F86 */ edx = (edx + ecx) & U32MASK;
    /* 0x00415F88 */ G32(0x45AF9C) = (unsigned int)edx;
    /* 0x00415F8E */ ecx = 0;
    /* 0x00415F90 */ edx = 1;
l_415F95:
    /* 0x00415F95 */ ebx = edx;
    /* 0x00415F97 */ ecx = (ecx & 0xFFFFFF00UL) |
        (unsigned long)M8((ebx + eax + 1) & U32MASK); /* mov cl,[ebx+eax+1] */
    /* 0x00415F9B */ edx = (edx + 1) & U32MASK; /* inc */
    /* 0x00415F9C */ /* test cl,cl */;
    /* 0x00415F9E jnz */ if ((ecx & 0xFFUL) != 0) goto l_416126;
l_415FA4:
    /* 0x00415FA4 */ eax = ebp;
    /* 0x00415FA6 */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)M8((esi + eax) & U32MASK); /* mov al,[esi+eax] */
    /* 0x00415FA9 */ eax &= 0xFFUL;
    /* 0x00415FAE */ G32(0x433B40) = (unsigned int)ebp;
    /* 0x00415FB4 */ d4v_417220(c, (unsigned int)eax); /* call sub_417220 */
    /* 0x00415FB9 */ ebp = (unsigned long)G32(0x433B40);
l_415FBF:
    /* 0x00415FBF */ edx = (unsigned long)G32(0x45BA20);
    /* 0x00415FC5 */ eax = (unsigned long)G32(0x45BA1C);
    /* 0x00415FCA */ edx = (edx << 8) & U32MASK;
    /* 0x00415FCD */ eax >>= 0x18;
    /* 0x00415FD0 */ edx = (edx + eax) & U32MASK;
    /* 0x00415FD2 */ eax = (unsigned long)G32(0x45BA1C);
    /* 0x00415FD7 */ G32(0x45BA20) = (unsigned int)edx;
    /* 0x00415FDD */ edx = (unsigned long)G32(0x45BA18);
    /* 0x00415FE3 */ eax = (eax << 8) & U32MASK;
    /* 0x00415FE6 */ edx >>= 0x18;
    /* 0x00415FE9 */ eax = (eax + edx) & U32MASK;
    /* 0x00415FEB */ G32(0x45BA1C) = (unsigned int)eax;
    /* 0x00415FF0 */ eax = ebp;
    /* 0x00415FF2 */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)M8((esi + eax) & U32MASK); /* mov al,[esi+eax] */
    /* 0x00415FF5 */ edx = (unsigned long)G32(0x45BA18);
    /* 0x00415FFB */ eax &= 0xFFUL;
    /* 0x00416000 */ edx = (edx << 8) & U32MASK;
    /* 0x00416003 */ esi = (esi + 1) & U32MASK; /* inc */
    /* 0x00416004 */ edx = (edx + eax) & U32MASK;
    /* 0x00416006 */ eax = (unsigned long)G32(0x45BA3C);
    /* 0x0041600B */ G32(0x45BA18) = (unsigned int)edx;
    /* 0x00416011 */ esi &= eax;
    /* 0x00416013 jmp */ goto l_415E7F;
l_416018:
    /* 0x00416018 */ /* cmp dl,80h */;
    /* 0x0041601B jb */ if ((edx & 0xFFUL) < 0x80UL) goto l_416052; /* unsigned */
l_41601D:
    /* 0x0041601D */ ecx = 0; /* xor ecx,ecx */
    /* 0x0041601F */ ecx |= (edx & 0xFFUL); /* mov cl,dl */
    /* 0x00416021 */ ebx = (unsigned long)G32(0x45BA3C);
    /* 0x00416027 */ edx = (edx & 0xFFFF00FFUL) |
        ((unsigned long)G8(0x43691CUL + (ecx & 0xFFUL)) << 8);
    /* 0x0041602D */ ecx = (eax + 1) & U32MASK; /* lea */
    /* 0x00416030 */ ecx &= ebx;
    /* 0x00416032 */ /* cmp dh,0C8h */;
    /* 0x00416035 jnb */ if (((edx >> 8) & 0xFFUL) >= 0xC8UL)
        goto l_4160C5; /* unsigned */
    /* 0x0041603B */ /* cmp dh,64h */;
    /* 0x0041603E jnz */ if (((edx >> 8) & 0xFFUL) != 0x64UL) goto l_41605D;
    /* 0x00416040 */ G8(0x45C689) = 1;
    /* 0x00416047 */ G8(0x45C68C) = (unsigned char)(edx & 0xFFUL);
    /* 0x0041604D jmp */ goto l_415D27;
l_416052:
    /* 0x00416052 */ /* test dh,dh */;
    /* 0x00416054 jnz */ if (((edx >> 8) & 0xFFUL) != 0) goto l_41601D;
    /* 0x00416056 */ eax = (eax & 0xFFFFFF00UL) | (edx & 0xFFUL); /* mov al,dl */
    /* 0x00416058 jmp */ goto l_415D21;
l_41605D:
    /* 0x0041605D */ /* cmp dh,63h */;
    /* 0x00416060 jnz */ if (((edx >> 8) & 0xFFUL) != 0x63UL) goto l_41606E;
    /* 0x00416062 */ G8(0x45C689) = 0x20;
    /* 0x00416069 jmp */ goto l_415D27;
l_41606E:
    /* 0x0041606E */ ebx = 0; /* xor ebx,ebx */
    /* 0x00416070 */ ebx |= ((edx >> 8) & 0xFFUL); /* mov bl,dh */
    /* 0x00416072 */ edx = (unsigned long)M32((0x4362C0UL + ebx * 4) & U32MASK);
    /* 0x00416079 */ ebx = (ebx & 0xFFFFFF00UL) |
        (unsigned long)G8(0x436450UL + (ebx & 0xFFUL));
    /* 0x0041607F */ /* cmp [45C689],0 */;
    /* 0x00416086 jnz */ if (G8(0x45C689) != 0) goto l_4160AB;
l_416088:
    /* 0x00416088 */ esi = edx;
    /* 0x0041608A */ ecx = ebp;
    /* 0x0041608C */ eax = (eax + 1) & U32MASK; /* inc */
    /* 0x0041608D */ ebx = (ebx & 0xFFFF00FFUL) |
        ((unsigned long)M8(esi) << 8); /* mov bh,[esi] */
    /* 0x0041608F */ M8((ecx + eax - 1) & U32MASK) =
        (unsigned char)((ebx >> 8) & 0xFFUL);
    /* 0x00416093 */ eax &= (unsigned long)G32(0x45BA3C);
    /* 0x00416099 */ edx = (edx + 1) & U32MASK; /* inc */
    /* 0x0041609A */ /* cmp edi,eax */;
    /* 0x0041609C jz */ if (edi == eax) goto l_415D27;
    /* 0x004160A2 */ DECB_LO(ebx); /* dec bl */
    /* 0x004160A4 jnz */ if ((ebx & 0xFFUL) != 0) goto l_416088;
    /* 0x004160A6 jmp */ goto l_415D27;
l_4160AB:
    /* 0x004160AB */ esi = edx;
    /* 0x004160AD */ ebx = (ebx & 0xFFFF00FFUL) |
        ((unsigned long)M8(esi) << 8); /* mov bh,[esi] */
    /* 0x004160AF */ eax = (eax + ebp) & U32MASK;
    /* 0x004160B1 */ ebx = (ebx & 0xFFFF00FFUL) |
        (unsigned long)(((((ebx >> 8) & 0xFFUL) + 0x20UL) & 0xFFUL) << 8);
    /* 0x004160B4 */ edx = (edx + 1) & U32MASK; /* inc */
    /* 0x004160B5 */ M8(eax) = (unsigned char)((ebx >> 8) & 0xFFUL);
    /* 0x004160B7 */ eax = ecx;
    /* 0x004160B9 */ ecx &= 0xFFFFFF00UL; /* xor cl,cl */
    /* 0x004160BB */ DECB_LO(ebx); /* dec bl */
    /* 0x004160BD */ G8(0x45C689) = (unsigned char)(ecx & 0xFFUL); /* =0 */
    /* 0x004160C3 jmp */ goto l_416088;
l_4160C5:
    /* 0x004160C5 */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(0x45C689);
    /* 0x004160CB */ SUB8_LO(edx, ebx & 0xFFUL); /* sub dl,bl */
    /* 0x004160CD */ ebx = (ebx & 0xFFFFFF00UL) | (edx & 0xFFUL); /* mov bl,dl */
    /* 0x004160CF */ edx = ebp;
    /* 0x004160D1 */ ebx = (ebx & 0xFFFFFF00UL) |
        (unsigned long)((~(ebx & 0xFFUL)) & 0xFFUL); /* not bl */
    /* 0x004160D3 */ M8((edx + eax) & U32MASK) = (unsigned char)(ebx & 0xFFUL);
    /* 0x004160D6 */ ebx &= 0xFFFF00FFUL; /* xor bh,bh */
    /* 0x004160D8 */ eax = ecx;
    /* 0x004160DA */ G8(0x45C689) = (unsigned char)((ebx >> 8) & 0xFFUL);
    /* 0x004160E0 jmp */ goto l_415D27;
l_4160E5:
    /* 0x004160E5 */ esi = (unsigned long)G32(0x45BA3C);
    /* 0x004160EB */ edi = (edi + 1) & U32MASK; /* inc */
    /* 0x004160EC */ edx = (unsigned long)G32(0x45BA34);
    /* 0x004160F2 */ edi &= esi;
    /* 0x004160F4 */ /* cmp edi,edx */;
    /* 0x004160F6 jnz */ if (edi != edx) goto l_415CDB;
    /* 0x004160FC jmp */ goto l_415D2F;
l_416101:
    /* 0x00416101 */ ecx = 0xFFFFFFFFUL;
    /* 0x00416106 */ ebp = (unsigned long)G32(0x433B40);
    /* 0x0041610C */ t1c = ecx;
    /* 0x00416113 jmp */ goto l_415E7A;
l_416118:
    /* 0x00416118 */ /* cmp eax,0FFFFFFFFh */;
    /* 0x0041611B jz */ if (eax == 0xFFFFFFFFUL) goto l_415F2C;
    /* 0x00416121 jmp */ goto l_415FBF;
l_416126:
    /* 0x00416126 */ a_ = (unsigned long)G32(0x45AF9C);
    a_ = (a_ + (ecx & 0xFFUL)) & U32MASK;
    G32(0x45AF9C) = (unsigned int)a_;
    /* 0x0041612C */ /* cmp edx,0Dh */;
    /* 0x0041612F jb */ if (edx < 0xDUL) goto l_415F95; /* unsigned */
    /* 0x00416135 jmp */ goto l_415FA4;
l_41613A:
    /* 0x0041613A */ a_ = (unsigned long)G32(0x45AFAC);
    a_ = (a_ + edi) & U32MASK; /* edi = 0x40000 on this path */
    G32(0x45AFAC) = (unsigned int)a_;
    /* 0x00416140 jmp */ goto l_415FA4;
l_416145:
    /* 0x00416145 */ eax = (unsigned long)G32(0x45AFAC);
    /* 0x0041614A */ eax = (eax + 0x8000UL) & U32MASK;
    /* 0x0041614F */ edx = (edx + 0x100000UL) & U32MASK;
    /* 0x00416155 */ G32(0x45AFAC) = (unsigned int)eax;
    /* 0x0041615A */ /* cmp edi,eax */;
    /* 0x0041615C ja */ if (edi > eax) goto l_415F5C; /* unsigned */
    /* 0x00416162 jmp */ goto l_415FA4;
l_416167:
    /* 0x00416167 */ eax = t24;
    /* 0x0041616E */ G32(0x45AFD0) = (unsigned int)eax;
    /* 0x00416173 */ eax = t20;
    /* 0x0041617A */ G32(0x45AFA8) = (unsigned int)eax;
    /* 0x0041617F */ eax = t28;
    /* 0x00416186 */ ebx = 0x800UL;
    /* 0x0041618B */ G32(0x45BA24) = (unsigned int)eax;
    /* 0x00416190 */ eax = (unsigned long)G32(0x433B78);
    /* 0x00416195 */ edx = 0; /* xor */
    /* 0x00416197 */ eax = (eax + 0x5F600UL) & U32MASK;
    /* 0x0041619C */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x004161A1 */ ebx = 0x800UL;
    /* 0x004161A6 */ eax = (unsigned long)G32(0x433B7C);
    /* 0x004161AB */ edx = 0; /* xor */
    /* 0x004161AD */ eax = (eax + 0x5F600UL) & U32MASK;
    /* 0x004161B2 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x004161B7 */ eax = (unsigned long)G32(0x433B84);
    /* 0x004161BC */ M32((eax + 0x5F6UL) & U32MASK) = 0x10001UL;
    /* 0x004161C6 */ M16((eax + 0x5FAUL) & U32MASK) = 1;
    /* 0x004161CF */ ebx = (ebx & 0xFFFF00FFUL) |
        ((unsigned long)G8(0x45C675) << 8); /* mov bh,[45C675] */
    /* 0x004161D5 */ M16((eax + 0x5FCUL) & U32MASK) = 1;
    /* 0x004161DE */ /* cmp bh,1 */;
    /* 0x004161E1 jnz */ if (((ebx >> 8) & 0xFFUL) != 1) goto l_416218;
    /* 0x004161E3 */ eax = (unsigned long)G32(0x45BA28);
    goto l_4161E8;
l_416218:
    /* 0x00416218 */ eax = (unsigned long)G32(0x45CD28);
    /* 0x0041621D jmp */ goto l_4161E8;
l_4161E8:
    /* 0x004161E8 */ ebp = (unsigned long)G32(0x433B40);
    /* 0x004161EE */ G32(0x45AFB4) = (unsigned int)eax;
    /* 0x004161F3 */ edx = 0xFFFFFFFFUL;
    /* 0x004161F8 */ eax = (eax & 0xFFFFFF00UL) | 1UL; /* mov al,1 */
    /* 0x004161FA */ G32(0x45BA50) = (unsigned int)edx;
    /* 0x00416200 */ G8(0x45C67F) = (unsigned char)(eax & 0xFFUL);
    /* 0x00416205 */ G32(0x433B40) = (unsigned int)ebp;
    /* epilogue 0x0041620B add esp,114h; 0x00416211 pop ebp;
     * 0x00416212 pop edi; 0x00416213 pop esi; 0x00416214 pop edx;
     * 0x00416215 pop ecx; 0x00416216 pop ebx; 0x00416217 retn. */
}

/* ------------------------------------------------------------------ */
/* sub_416220 (lst 32422-33005) -> d4v_ppm                              */
/*                                                                      */
/* Role hypothesis: PPM block path / model (re)initialisation. Zeroes   */
/* the small static tables, builds the 45A280/45A281 seed tables from   */
/* the dword_4150D0 template, biases the 45AAxx/45ACxx tables, copies   */
/* the static 0x436ED8 data into the [0x433B78] model area, rebuilds    */
/* its u16 prefix sums, runs sub_416A00 over 0x203..0x2FE, rescales     */
/* (sub_415AE0), allocates/clears the 45B5F4/45AFD8/4593A0/458798       */
/* tables, then walks the 45AFB0/45BA2C extent list against 45AFB4 to    */
/* pick the block-tail magic (75A45/8F4D0/17E5F8/2CC640/388DC2/7D4C0/   */
/* FB670), re-clearing the LZP tables on some transitions, and finally  */
/* range-decodes the method selector (45D0B8/45D0BC + sub_4220B0) to    */
/* choose between the early return, the sub_41CF60 tail call (45C675    */
/* set path) and the sub_41E5F0 tail call.                              */
/* Entry-live regs: none (ebx/edx/eax set up the first fill call; no    */
/* stack args). Returns int = final eax (tail-call results of           */
/* sub_41CF60/sub_41E5F0, else the range-quotient leftover); both       */
/* in-tree callers (417DC5, 41CE8C) discard it.                         */
/* Frame: 0x1C bytes + 6 pushed regs. Locals: s[11] template @ sp+0     */
/* (var_34), w28 @ sp+0xC (var_28), w24 @ sp+0x10 (var_24, outer        */
/* 0..0x700 step 0x100), w20 @ sp+0x14 (var_20), w1c @ sp+0x18          */
/* (var_1C, 0..0xA).                                                    */
/* Globals touched: 45A278,45A258,45AA80,45A280,45AE80,433B58..433B7C, */
/* 45A25D..45A27E,433B84,433BE8,459BA8,45AFCC,45AFC0,45AFA0,45AFD4,     */
/* 45B5F4,45AF78,45AFD8,4593A0,458798,458F94,45AF80,45AF64,459398,      */
/* 45939C,459BA0,459BA4,45AF60,45AFB0,45AF84,45C67F,45AFB4,45CD14,      */
/* 45BA2C,45BA44,45BA50,45C675,45BA28,45AFA8,45AFD0,45BA24,45BA34,      */
/* 45BA3C,45BA30,433B44,433B48,45C681,45D0BC,45D0B8,45AF68,45AF6C,      */
/* 45AF70,45C67D,T4342C4,T43429C,T434314,45AF3C,45AF40,45C67A,45C688,  */
/* 45C67B,45C679,45AF48 plus static tables T4150D0/T436ED8 (arena).     */
/* ------------------------------------------------------------------ */
int d4v_ppm(dec4v_ctx *c)
{
    unsigned long eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned char s[11] = {0};
    unsigned long w28 = 0, w24 = 0, w20 = 0, w1c = 0;
    unsigned long a_ = 0;
    unsigned long dd_ = 0;

    /* prologue 0x00416220 push ebx; 0x00416221 push ecx;
     * 0x00416222 push edx; 0x00416223 push esi; 0x00416224 push edi;
     * 0x00416225 push ebp; 0x00416226 sub esp,1Ch.
     * Callee-saved regs become C locals; the esp frame is s/w28..w1c. */

    /* 0x00416229 */ ebx = 8;
    /* 0x0041622E */ edx = 1;
    /* 0x00416233 */ eax = 0x45A278UL;
    /* 0x00416238 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x0041623D */ ebx = 0x20UL;
    /* 0x00416242 */ edx = 1;
    /* 0x00416247 */ eax = 0x45A258UL;
    /* 0x0041624C */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x00416251 */ ebx = 0x400UL;
    /* 0x00416256 */ edx = 1;
    /* 0x0041625B */ eax = 0x45AA80UL;
    /* 0x00416260 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x00416265 */ ebx = 0x800UL;
    /* 0x0041626A */ edx = 1;
    /* 0x0041626F */ eax = 0x45A280UL;
    /* 0x00416274 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x00416279 */ ebx = 0x80UL;
    /* 0x0041627E */ edx = 1;
    /* 0x00416283 */ eax = 0x45AE80UL;
    /* 0x00416288 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x0041628D */ ebx = 0x40000UL;
    /* 0x00416292 */ eax = (unsigned long)G32(0x433B58);
    /* 0x00416297 */ edx = 0; /* xor */
    /* 0x00416299 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x0041629E */ ebx = 0x80000UL;
    /* 0x004162A3 */ eax = (unsigned long)G32(0x433B5C);
    /* 0x004162A8 */ edx = 0;
    /* 0x004162AA */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x004162AF */ ebx = 0x80000UL;
    /* 0x004162B4 */ eax = (unsigned long)G32(0x433B60);
    /* 0x004162B9 */ edx = 0;
    /* 0x004162BB */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x004162C0 */ ebx = 0x40000UL;
    /* 0x004162C5 */ eax = (unsigned long)G32(0x433B64);
    /* 0x004162CA */ edx = 0;
    /* 0x004162CC */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x004162D1 */ ebx = 0x40000UL;
    /* 0x004162D6 */ eax = (unsigned long)G32(0x433B68);
    /* 0x004162DB */ edx = 0;
    /* 0x004162DD */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x004162E2 */ ebx = 0x40000UL;
    /* 0x004162E7 */ eax = (unsigned long)G32(0x433B6C);
    /* 0x004162EC */ edx = 0;
    /* 0x004162EE */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x004162F3 */ ebx = 0x1C0000UL;
    /* 0x004162F8 */ eax = (unsigned long)G32(0x433B70);
    /* 0x004162FD */ edx = 0;
    /* 0x004162FF */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x00416304 */ ebx = 0x40000UL;
    /* 0x00416309 */ eax = (unsigned long)G32(0x433B74);
    /* 0x0041630E */ edx = 0;
    /* 0x00416310 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x00416315 */ ebx = 0x40600UL;
    /* 0x0041631A */ eax = (unsigned long)G32(0x433B78);
    /* 0x0041631F */ edx = 0;
    /* 0x00416321 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x00416326 */ ebx = 0x40600UL;
    /* 0x0041632B */ eax = (unsigned long)G32(0x433B7C);
    /* 0x00416330 */ edx = 0;
    /* 0x00416332 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x00416337 */ edx = 0; /* xor */
    /* 0x00416339 */ w24 = edx;
l_41633D:
    /* 0x0041633D mov edi,esp: edi becomes the dest base of the 11-byte
     * copy below (modeled as s[0..10] writes); no C value needed */
    /* 0x0041633F */ eax = w24;
    /* 0x00416343 */ ecx = 0; /* xor */
    /* 0x00416345 */ esi = 0x4150D0UL; /* mov esi,offset dword_4150D0 */
    /* 0x0041634A */ w1c = ecx;
    /* 0x0041634E */ w20 = eax;
    /* 0x00416352 movsd; 0x00416353 movsd; 0x00416354 movsw; 0x00416356 movsb:
     * 11 code bytes at 0x4150D0 copied to s[0..10]. Those bytes are
     * 00 01 02 05 08 0D 15 22 37 59 90 (lifted from UHARC04.EXE text);
     * hard-coded: our arena holds no image of the native code segment. */
    s[0] = 0x00; s[1] = 0x01; s[2] = 0x02; s[3] = 0x05;
    s[4] = 0x08; s[5] = 0x0D; s[6] = 0x15; s[7] = 0x22;
    s[8] = 0x37; s[9] = 0x59; s[10] = 0x90;
    esi = (0x4150DBUL);
l_416357:
    /* 0x00416357 */ edi = w1c;
    /* 0x0041635B */ esi = w20;
    /* 0x0041635F */ ecx = 0; /* xor */
    /* 0x00416361 */ ebp = edi;
    /* 0x00416363 */ ebx = esi;
    /* 0x00416365 */ ebp = (0UL - ebp) & U32MASK; /* neg */
l_416367:
    /* 0x00416367 */ /* cmp ecx,[var_1C] */;
    /* 0x0041636B jb */ if (ecx < w1c) goto l_416912; /* unsigned */
l_416371:
    /* 0x00416371 */ /* cmp ecx,[var_1C] */;
    /* 0x00416375 jbe */ if (ecx <= w1c) goto l_41639E; /* unsigned */
    /* 0x00416377 */ edx = 0; /* xor */
    /* 0x00416379 */ edx |= (unsigned long)s[ebp]; /* mov dl,[esp+ebp+..];
        trace range: ebp = ecx-w1c in 1..7 here (ecx>w1c, ecx<=7) */
    /* 0x0041637C */ eax = edx;
    /* 0x0041637E */ edx = d4v_sar(edx, 31); /* sar edx,1Fh (=0 for bytes) */
    /* 0x00416381 */ eax = (eax - edx) & U32MASK;
    /* 0x00416383 */ eax = d4v_sar(eax, 1);
    /* 0x00416385 */ w28 = eax;
    /* 0x00416389 */ edx = w28;
    /* 0x0041638D */ eax = 0; /* xor */
    /* 0x0041638F */ edx = (edx + 1) & U32MASK; /* inc */
    /* 0x00416390 */ eax |= (unsigned long)G8((0x45A281UL + ebx) & U32MASK);
    /* 0x00416396 */ eax = (eax + edx) & U32MASK;
    /* 0x00416398 */ G8((0x45A281UL + ebx) & U32MASK) =
        (unsigned char)(eax & 0xFFUL);
l_41639E:
    /* 0x0041639E */ edi = (edi - 1) & U32MASK; /* dec */
    /* 0x0041639F */ esi = (esi + 2) & U32MASK;
    /* 0x004163A2 */ ebp = (ebp + 1) & U32MASK; /* inc */
    /* 0x004163A3 */ ecx = (ecx + 1) & U32MASK; /* inc */
    /* 0x004163A4 */ ebx = (ebx + 2) & U32MASK;
    /* 0x004163A7 */ /* cmp ecx,8 */;
    /* 0x004163AA jb */ if (ecx < 8UL) goto l_416367; /* unsigned */
    /* 0x004163AC */ ecx = w20;
    /* 0x004163B0 */ ebx = w1c;
    /* 0x004163B4 */ ecx = (ecx + 0x10UL) & U32MASK;
    /* 0x004163B7 */ ebx = (ebx + 1) & U32MASK; /* inc */
    /* 0x004163B8 */ w20 = ecx;
    /* 0x004163BC */ w1c = ebx;
    /* 0x004163C0 */ /* cmp ebx,0Bh */;
    /* 0x004163C3 jb */ if (ebx < 0xBUL) goto l_416357; /* unsigned */
    /* 0x004163C5 */ edi = w24;
    /* 0x004163C9 */ edi = (edi + 0x100UL) & U32MASK;
    /* 0x004163CF */ w24 = edi;
    /* 0x004163D3 */ /* cmp edi,800h */;
    /* 0x004163D9 jnz */ if (edi != 0x800UL) goto l_41633D;
    /* 0x004163DF */ eax = 0; /* xor */
    /* 0x004163E1 */ edx = 8;
l_4163E6:
    /* 0x004163E6 */ ebx = (ebx & 0xFFFFFF00UL) |
        (unsigned long)G8((0x45AA82UL + eax) & U32MASK); /* mov bl,[..+eax] */
    /* 0x004163EC */ eax = (eax + 8) & U32MASK;
    /* 0x004163EF */ ADD8_LO(ebx, 2UL); /* add bl,2 */
    /* 0x004163F2 */ ecx = (ecx & 0xFFFFFF00UL) |
        (unsigned long)G8((0x45AA7EUL + eax) & U32MASK); /* mov cl,[..] */
    /* 0x004163F8 */ G8((0x45AA7AUL + eax) & U32MASK) =
        (unsigned char)(ebx & 0xFFUL);
    /* 0x004163FE */ ADD8_LO(ecx, edx & 0xFFUL); /* add cl,dl (dl=8) */
    /* 0x00416400 */ ebx = (ebx & 0xFFFF00FFUL) |
        ((unsigned long)G8((0x45AA7CUL + eax) & U32MASK) << 8);
    /* 0x00416406 */ G8((0x45AA7EUL + eax) & U32MASK) =
        (unsigned char)(ecx & 0xFFUL);
    /* 0x0041640C */ ADD8_HI(ebx, 5UL); /* add bh,5 */
    /* 0x0041640F */ ecx = (ecx & 0xFFFF00FFUL) |
        ((unsigned long)G8((0x45AC78UL + eax) & U32MASK) << 8); /* mov ch,[..] */
    /* 0x00416415 */ G8((0x45AA7CUL + eax) & U32MASK) =
        (unsigned char)((ebx >> 8) & 0xFFUL);
    /* 0x0041641B */ ADD8_HI(ecx, 0xDUL); /* add ch,0Dh */
    /* 0x0041641E */ ebx = (ebx & 0xFFFFFF00UL) |
        (unsigned long)G8((0x45AC7AUL + eax) & U32MASK); /* mov bl,[..] */
    /* 0x00416424 */ G8((0x45AC78UL + eax) & U32MASK) =
        (unsigned char)((ecx >> 8) & 0xFFUL);
    /* 0x0041642A */ ADD8_LO(ebx, 0x17UL); /* add bl,17h */
    /* 0x0041642D */ ecx = (ecx & 0xFFFFFF00UL) |
        (unsigned long)G8((0x45AC7EUL + eax) & U32MASK); /* mov cl,[..] */
    /* 0x00416433 */ G8((0x45AC7AUL + eax) & U32MASK) =
        (unsigned char)(ebx & 0xFFUL);
    /* 0x00416439 */ ADD8_LO(ecx, 0x3FUL); /* add cl,3Fh */
    /* 0x0041643C */ ebx = (ebx & 0xFFFF00FFUL) |
        ((unsigned long)G8((0x45AC7CUL + eax) & U32MASK) << 8);
    /* 0x00416442 */ G8((0x45AC7EUL + eax) & U32MASK) =
        (unsigned char)(ecx & 0xFFUL);
    /* 0x00416448 */ ADD8_HI(ebx, 0x27UL); /* add bh,27h */
    /* 0x0041644B */ G8((0x45AC7CUL + eax) & U32MASK) =
        (unsigned char)((ebx >> 8) & 0xFFUL);
    /* 0x00416451 */ /* cmp eax,200h */;
    /* 0x00416456 jnz */ if (eax != 0x200UL) goto l_4163E6;
    /* 0x00416458 */ ecx = (ecx & 0xFFFF00FFUL) |
        ((unsigned long)G8(0x45A25D) << 8); /* mov ch,[..] */
    /* 0x0041645E */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)G8(0x45A25F);
    /* 0x00416463 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)G8(0x45A268);
    /* 0x00416469 */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(0x45A272);
    /* 0x0041646F */ ADD8_HI(ecx, 2UL); /* add ch,2 */
    /* 0x00416472 */ ADD8_LO(eax, 4UL); /* add al,4 */
    /* 0x00416474 */ ADD8_LO(edx, 2UL); /* add dl,2 */
    /* 0x00416477 */ ADD8_LO(ebx, 2UL); /* add bl,2 */
    /* 0x0041647A */ G8(0x45A25D) = (unsigned char)((ecx >> 8) & 0xFFUL);
    /* 0x00416480 */ G8(0x45A25F) = (unsigned char)(eax & 0xFFUL);
    /* 0x00416485 */ G8(0x45A268) = (unsigned char)(edx & 0xFFUL);
    /* 0x0041648B */ G8(0x45A272) = (unsigned char)(ebx & 0xFFUL);
    /* 0x00416491 */ eax = (eax & 0xFFFF00FFUL) | ((unsigned long)G8(0x45A267) << 8); /* mov ah,[..] */
    /* 0x00416497 */ edx = (edx & 0xFFFF00FFUL) | ((unsigned long)G8(0x45A270) << 8); /* mov dh,[..] */
    /* 0x0041649D */ ebx = (ebx & 0xFFFF00FFUL) | ((unsigned long)G8(0x45A27A) << 8); /* mov bh,[..] */
    /* 0x004164A3 */ ecx = (ecx & 0xFFFFFF00UL) | (unsigned long)G8(0x45A27C);
    /* 0x004164A9 */ ADD8_HI(eax, 2UL); /* add ah,2 */
    /* 0x004164AC */ ADD8_HI(edx, 4UL); /* add dh,4 */
    /* 0x004164AF */ ADD8_HI(ebx, 2UL); /* add bh,2 */
    /* 0x004164B2 */ ADD8_LO(ecx, 5UL); /* add cl,5 */
    /* 0x004164B5 */ G8(0x45A267) = (unsigned char)((eax >> 8) & 0xFFUL);
    /* 0x004164BB */ G8(0x45A270) = (unsigned char)((edx >> 8) & 0xFFUL);
    /* 0x004164C1 */ G8(0x45A27C) = (unsigned char)(ecx & 0xFFUL);
    /* 0x004164C7 */ ecx = (ecx & 0xFFFF00FFUL) |
        ((unsigned long)G8(0x45A27E) << 8);
    /* 0x004164CD */ G8(0x45A27A) = (unsigned char)((ebx >> 8) & 0xFFUL);
    /* 0x004164D3 */ ADD8_HI(ecx, 8UL); /* add ch,8 */
    /* 0x004164D6 */ edx = 0x5FE00UL;
    /* 0x004164DB */ G8(0x45A27E) = (unsigned char)((ecx >> 8) & 0xFFUL);
    /* 0x004164E1 */ ebx = 0x200UL;
    /* 0x004164E6 */ ebp = 0x436ED8UL;
l_4164EB:
    /* 0x004164EB */ edi = (unsigned long)G32(0x433B78);
    /* 0x004164F1 */ ecx = ebx;
    /* 0x004164F3 */ esi = ebp;
    /* 0x004164F5 */ edi = (edi + edx) & U32MASK;
    /* 0x004164F7 */ edx = (edx + 0x200UL) & U32MASK;
    /* 0x004164FD cld */;
    /* 0x004164FE */ eax = ecx;
    /* 0x00416500 */ ecx = (ecx - edi) & U32MASK;
    /* 0x00416502 */ ecx = (ecx - eax) & U32MASK;
    /* 0x00416504 */ ecx &= 3UL;
    /* 0x00416507 */ eax = (eax - ecx) & U32MASK;
    /* 0x00416509 jle */ if ((int)(eax & U32MASK) > 0) { /* signed 32-bit */
        /* 0x0041650B rep movsb */
        while (ecx != 0) {
            M8(edi) = M8(esi);
            esi = (esi + 1) & U32MASK;
            edi = (edi + 1) & U32MASK;
            ecx = (ecx - 1) & U32MASK;
        }
        /* 0x0041650D */ ecx = eax;
        /* 0x0041650F */ eax &= 3UL;
        /* 0x00416512 */ ecx >>= 2;
        /* 0x00416515 rep movsd */
        while (ecx != 0) {
            M32(edi) = M32(esi);
            esi = (esi + 4) & U32MASK;
            edi = (edi + 4) & U32MASK;
            ecx = (ecx - 1) & U32MASK;
        }
    }
l_416517:
    /* 0x00416517 */ ecx = (ecx + eax) & U32MASK;
    /* 0x00416519 rep movsb */
    while (ecx != 0) {
        M8(edi) = M8(esi);
        esi = (esi + 1) & U32MASK;
        edi = (edi + 1) & U32MASK;
        ecx = (ecx - 1) & U32MASK;
    }
    /* 0x0041651B */ /* cmp edx,61A00h */;
    /* 0x00416521 jnz */ if (edx != 0x61A00UL) goto l_4164EB;
    /* 0x00416523 */ eax = 0; /* xor */
l_416525:
    /* 0x00416525 */ edx = (unsigned long)G32(0x433B78);
    /* 0x0041652B */ ebx = (ebx & 0xFFFF0000UL) |
        (unsigned long)M16((edx + eax + 0x5FE00UL) & U32MASK);
    /* 0x00416533 */ eax = (eax + 2) & U32MASK;
    /* 0x00416536 */ ebx = (ebx << 3) & U32MASK; /* shl ebx,3 */
    /* NOTE: high 16 of ebx is stale from 0x004164E1 (ebx=0x200, hi=0);
     * kept exact by carrying full u32 ebx through the loop. */
    /* 0x00416539 */ M16((edx + eax + 0x5FDFEUL) & U32MASK) =
        (unsigned short)(ebx & 0xFFFFUL);
    /* 0x00416541 */ /* cmp eax,1C00h */;
    /* 0x00416546 jnz */ if (eax != 0x1C00UL) goto l_416525;
    /* 0x00416548 */ ebx = 0xE00UL;
    /* 0x0041654D */ eax = 8;
    /* 0x00416552 */ edx = (unsigned long)G32(0x433B7C);
    /* 0x00416558 */ ecx = 0x290UL;
    /* 0x0041655D */ edx = (edx + 0x5FE00UL) & U32MASK;
    /* 0x00416563 */ edi = 0x459BA8UL; /* mov edi,offset unk_459BA8 */
    /* 0x00416568 */ d4v_fill16(c, (unsigned int)ebx, (unsigned int)edx,
        (unsigned int)eax);
    /* 0x0041656D */ ebx = 0x203UL;
    /* 0x00416572 */ eax = 1;
    /* 0x00416577 */ edx = (unsigned long)G32(0x433B84);
    /* 0x0041657D */ esi = 0x433BE8UL;
    /* 0x00416582 */ d4v_fill16(c, (unsigned int)ebx, (unsigned int)edx,
        (unsigned int)eax);
    /* 0x00416587 cld */;
    /* 0x00416588 */ eax = ecx;
    /* 0x0041658A */ ecx = (ecx - edi) & U32MASK;
    /* 0x0041658C */ ecx = (ecx - eax) & U32MASK;
    /* 0x0041658E */ ecx &= 3UL;
    /* 0x00416591 */ eax = (eax - ecx) & U32MASK;
    /* 0x00416593 jle */ if ((int)(eax & U32MASK) > 0) { /* signed 32-bit */
        /* 0x00416595 rep movsb */
        while (ecx != 0) {
            M8(edi) = M8(esi);
            esi = (esi + 1) & U32MASK;
            edi = (edi + 1) & U32MASK;
            ecx = (ecx - 1) & U32MASK;
        }
        /* 0x00416597 */ ecx = eax;
        /* 0x00416599 */ eax &= 3UL;
        /* 0x0041659C */ ecx >>= 2;
        /* 0x0041659F rep movsd */
        while (ecx != 0) {
            M32(edi) = M32(esi);
            esi = (esi + 4) & U32MASK;
            edi = (edi + 4) & U32MASK;
            ecx = (ecx - 1) & U32MASK;
        }
    }
l_4165A1:
    /* 0x004165A1 */ ecx = (ecx + eax) & U32MASK;
    /* 0x004165A3 rep movsb */
    while (ecx != 0) {
        M8(edi) = M8(esi);
        esi = (esi + 1) & U32MASK;
        edi = (edi + 1) & U32MASK;
        ecx = (ecx - 1) & U32MASK;
    }
    /* 0x004165A5 */ edx = 0x203UL;
l_4165AA:
    /* 0x004165AA */ eax = edx;
    /* 0x004165AC */ edx = (edx + 1) & U32MASK; /* inc */
    /* 0x004165AD */ d4v_416a00(c, (unsigned int)eax); /* call sub_416A00 */
    /* 0x004165B2 */ /* cmp edx,2FFh */;
    /* 0x004165B8 jb */ if (edx < 0x2FFUL) goto l_4165AA; /* unsigned */
    /* 0x004165BA */ d4v_415ae0(c); /* call sub_415AE0 */
    /* 0x004165BF */ ecx = 0x1FFUL;
    /* 0x004165C4 */ ebx = 0x45B5F4UL;
    /* 0x004165C9 */ esi = 0x45AFD8UL;
    /* 0x004165CE */ ebp = 1;
    /* 0x004165D3 */ eax = 0; /* xor */
    /* 0x004165D5 */ edx = 0; /* xor */
    /* 0x004165D7 */ G32(0x45AFCC) = (unsigned int)eax; /* =0 */
    /* 0x004165DC */ G32(0x45AFC0) = 0;
    /* 0x004165E1 */ G32(0x45AFA0) = (unsigned int)ebx;
    /* 0x004165E7 */ ebx = 0x424UL;
    /* 0x004165EC */ eax = 0x45B5F4UL;
    /* 0x004165F1 */ G32(0x45AFD4) = (unsigned int)ecx;
    /* 0x004165F7 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x004165FC */ ebx = 0x61CUL;
    /* 0x00416601 */ eax = esi;
    /* 0x00416603 */ edx = 0; /* xor */
    /* 0x00416605 */ G32(0x45AF78) = (unsigned int)esi;
    /* 0x0041660B */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x00416610 */ ebx = 0x400UL;
    /* 0x00416615 */ edx = 0x80000000UL;
    /* 0x0041661A */ eax = 0x4593A0UL;
    /* 0x0041661F */ edi = 0; /* xor */
    /* 0x00416621 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL) /* dl=0 */);
    /* 0x00416626 */ ebx = 0x400UL;
    /* 0x0041662B */ eax = 0x458798UL;
    /* 0x00416630 */ edx = 0; /* xor */
    /* 0x00416632 */ ecx = 0x9240UL;
    /* 0x00416637 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x0041663C */ G32(0x45AF90) = (unsigned int)edi; /* =0 */
    /* 0x00416642 */ G32(0x45AF98) = (unsigned int)ebp; /* =1 */
    /* 0x00416648 */ eax = ebp;
    /* 0x0041664A */ G32(0x45AF7C) = (unsigned int)ebp;
    /* 0x00416650 */ G32(0x45AF94) = (unsigned int)edi; /* =0 */
    /* 0x00416656 */ eax ^= ebp; /* =0 */
l_416658:
    /* 0x00416658 */ eax = (eax + 4) & U32MASK;
    /* 0x0041665B */ M32(0x458F94UL + eax) = (unsigned int)ecx; /* =0x9240 */
    /* 0x00416661 */ /* cmp eax,400h */;
    /* 0x00416666 jnz */ if (eax != 0x400UL) goto l_416658;
    /* 0x00416668 */ ebx = 0xFFUL;
    /* 0x0041666D */ esi = 0x91ADC0UL;
    /* 0x00416672 */ edi = 0; /* xor */
    /* 0x00416674 */ G32(0x45AF80) = (unsigned int)ebx;
    /* 0x0041667A */ G32(0x45AF64) = (unsigned int)esi;
    /* 0x00416680 */ G32(0x459398) = 0;
    /* 0x00416686 */ G32(0x45939C) = 0;
    /* 0x0041668C */ G32(0x459BA0) = 0;
    /* 0x00416692 */ G32(0x459BA4) = 0;
    /* 0x00416698 */ G32(0x45AF60) = 0;
    /* 0x0041669E */ esi = (unsigned long)G32(0x45AFB0);
    /* 0x004166A4 */ G32(0x45AF84) = (unsigned int)edi; /* =0 */
    /* 0x004166AA */ /* test esi,esi */;
    /* 0x004166AC jz */ if (esi == 0) goto l_416938;
    /* 0x004166B2 */ ecx = 0; /* xor */
    /* 0x004166B4 */ ecx |= (unsigned long)G8(0x45C67F); /* mov cl,[..] */
    /* 0x004166BA */ eax = 0x40000UL;
    /* 0x004166BF */ ecx = (ecx + ecx) & U32MASK;
    /* 0x004166C1 */ edi = (unsigned long)G32(0x45AFB4);
    /* 0x004166C7 */ eax = d4v_sar(eax, (unsigned int)(ecx & 0xFFUL));
    /* 0x004166C9 */ edi = (edi + eax) & U32MASK;
    /* 0x004166CB */ ebp = (unsigned long)G32(0x45CD14);
    /* 0x004166D1 */ G32(0x45AFB4) = (unsigned int)edi;
    /* 0x004166D7 */ /* cmp edi,ebp */;
    /* 0x004166D9 ja */ if (edi > ebp) goto l_416920; /* unsigned */
l_4166DF:
    /* 0x004166DF */ eax = (unsigned long)G32(0x45BA2C);
    /* 0x004166E4 */ ebp = (unsigned long)G32(0x45BA44);
    /* 0x004166EA */ edi = (unsigned long)G32(0x45BA50);
    /* 0x004166F0 */ ebx = (unsigned long)G32(0x45AFB4);
    /* 0x004166F6 */ eax = (eax << 2) & U32MASK;
l_4166F9:
    /* 0x004166F9 */ edx = (unsigned long)G32(0x45AFB0);
    /* 0x004166FF */ ecx = (edx + eax) & U32MASK; /* lea */
    /* 0x00416702 */ /* cmp ebx,[ecx] */;
    /* 0x00416704 jbe */ if (ebx <= (unsigned long)M32(ecx)) goto l_416722;
    /* 0x00416706 */ eax = (eax + 4) & U32MASK;
    /* 0x00416709 */ esi = (unsigned long)G32(0x45BA2C);
    /* 0x0041670F */ edx = (edx + eax) & U32MASK;
    /* 0x00416711 */ esi = (esi + 1) & U32MASK; /* inc */
    /* 0x00416712 */ ecx = (unsigned long)M32(edx);
    /* 0x00416714 */ G32(0x45BA2C) = (unsigned int)esi;
    /* 0x0041671A */ /* cmp ecx,edi */;
    /* 0x0041671C jb */ if (ecx < edi) goto l_41692B; /* unsigned */
l_416722:
    /* 0x00416722 */ eax = (unsigned long)G32(0x45BA2C);
    /* 0x00416727 */ edx = (unsigned long)G32(0x45AFB0);
    /* 0x0041672D */ eax = (unsigned long)M32((edx + eax * 4) & U32MASK);
l_416730:
    /* 0x00416730 */ G32(0x45AFB4) = (unsigned int)eax;
    /* 0x00416735 */ eax = (unsigned long)G32(0x45BA2C);
    /* 0x0041673A */ /* test eax,eax */;
    /* 0x0041673C jbe */ if (eax <= 0) goto l_416809; /* unsigned (==0) */
    /* 0x00416742 */ ecx = (unsigned long)G32(0x45AFB0);
    /* 0x00416748 */ edx = eax;
    /* 0x0041674A */ eax = (eax << 2) & U32MASK;
    /* 0x0041674D */ eax = (eax + ecx) & U32MASK;
    /* 0x0041674F */ esi = (unsigned long)G32(0x45AFB4);
    /* 0x00416755 */ ecx = (unsigned long)M32((eax - 4) & U32MASK);
    /* 0x00416758 */ esi = (esi - ecx) & U32MASK;
    /* 0x0041675A */ /* cmp edx,1 */;
    /* 0x0041675D jbe */ if (edx <= 1UL) goto l_416762; /* unsigned */
    /* 0x0041675F */ ecx = (ecx - (unsigned long)M32((eax - 8) & U32MASK))
        & U32MASK;
l_416762:
    /* 0x00416762 */ /* cmp [45C675],1 */;
    /* 0x00416769 jnz */ if (G8(0x45C675) != 1) goto l_416942;
    /* 0x0041676F */ edi = (unsigned long)G32(0x45BA28);
l_416775:
    /* 0x00416775 */ /* cmp ecx,75A45h */;
    /* 0x0041677B jnz */ if (ecx != 0x75A45UL) goto l_41694D;
    /* 0x00416781 */ /* cmp esi,68302h */;
    /* 0x00416787 jnz */ if (esi != 0x68302UL) goto l_41694D;
l_41678D:
    /* 0x0041678D */ ebx = 0x100000UL;
    /* 0x00416792 */ eax = (unsigned long)G32(0x433B44);
    /* 0x00416797 */ edx = 0; /* xor */
    /* 0x00416799 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
l_41679E:
    /* 0x0041679E */ /* cmp ecx,17E5F8h */;
    /* 0x004167A4 jnz */ if (ecx != 0x17E5F8UL) goto l_41696A;
    /* 0x004167AA */ /* cmp esi,2CC640h */;
    /* 0x004167B0 jnz */ if (esi != 0x2CC640UL) goto l_41696A;
l_4167B6:
    /* 0x004167B6 */ d4v_415b50(c); /* call sub_415B50 */
    /* 0x004167BB */ d4v_415ae0(c); /* call sub_415AE0 */
l_4167C0:
    /* 0x004167C0 */ /* cmp ecx,388DC2h */;
    /* 0x004167C6 jnz */ if (ecx != 0x388DC2UL) goto l_416987;
    /* 0x004167CC */ /* cmp esi,16DD2Eh */;
    /* 0x004167D2 jnz */ if (esi != 0x16DD2EUL) goto l_416987;
l_4167D8:
    /* 0x004167D8 */ ebx = 0x100000UL;
    /* 0x004167DD */ eax = (unsigned long)G32(0x433B44);
    /* 0x004167E2 */ edx = 0; /* xor */
    /* 0x004167E4 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x004167E9 */ ebx = 0x140000UL;
    /* 0x004167EE */ eax = (unsigned long)G32(0x433B48);
    /* 0x004167F3 */ edx = 0; /* xor */
    /* 0x004167F5 */ d4v_fill(c, (unsigned int)ebx, (unsigned int)eax,
        (unsigned int)(edx & 0xFFUL));
    /* 0x004167FA */ /* cmp edi,[45BA44] */;
    /* 0x00416800 jb */ if (edi < (unsigned long)G32(0x45BA44))
        goto l_416809; /* unsigned */
    /* 0x00416802 */ G8(0x45C681) = 0x0A;
l_416809:
    /* 0x00416809 */ /* cmp [45C675],1 */;
    /* 0x00416810 jz */ if (G8(0x45C675) == 1) goto l_4169C4;
    /* 0x00416816 */ esi = 0xAUL;
    /* 0x0041681B */ ecx = esi;
    /* 0x0041681D */ G32(0x45AF68) = (unsigned int)esi;
    /* 0x00416823 */ eax = (unsigned long)G32(0x45D0BC);
    /* 0x00416828 */ edx = 0; /* xor */
    /* 0x0041682A div ecx: edx:eax / ecx (unsigned) */;
    /* UNCERTAIN(0041682A): faults (#DE) if [45D0BC]/10 == 0, as in orig. */
    dd_ = (unsigned long)eax; /* edx==0 */
    eax = (unsigned long)(dd_ / ecx);
    edx = (unsigned long)(dd_ % ecx);
    /* 0x0041682C */ edx = 0; /* xor (drops remainder) */
    /* 0x0041682E */ G32(0x45D0BC) = (unsigned int)eax;
    /* 0x00416833 */ ecx = eax;
    /* 0x00416835 */ eax = (unsigned long)G32(0x45D0B8);
    /* 0x0041683A div ecx */;
    dd_ = (unsigned long)eax; /* edx==0 */
    eax = (unsigned long)(dd_ / ecx);
    edx = (unsigned long)(dd_ % ecx);
    /* 0x0041683C */ ebx = eax;
    /* 0x0041683E */ eax = 0; /* xor */
    /* 0x00416840 */ eax |= (ebx & 0xFFUL); /* mov al,bl */
    /* 0x00416842 */ esi = (ebx & 0xFFUL); /* movzx esi,bl */
    /* 0x00416845 */ G32(0x45AF6C) = (unsigned int)eax;
    /* 0x0041684A */ eax = (esi + 1) & U32MASK; /* lea */
    /* 0x0041684D */ G32(0x45AF70) = (unsigned int)eax;
    /* 0x00416852 */ eax = 0x45AF68UL;
    /* 0x00416857 */ edx = (unsigned long)M32((eax + 8) & U32MASK);
    /* 0x0041685A */ eax = (unsigned long)M32((eax + 4) & U32MASK);
    /* 0x0041685D */ ecx = (unsigned long)G32(0x45D0BC);
    /* 0x00416863 */ edx = (edx - eax) & U32MASK;
    /* 0x00416865 */ eax = (eax * ecx) & U32MASK; /* imul */
    /* 0x00416868 */ edx = (edx * ecx) & U32MASK; /* imul */
    /* 0x0041686B */ ecx = (unsigned long)G32(0x45D0B8);
    /* 0x00416871 */ ecx = (ecx - eax) & U32MASK;
    /* 0x00416873 */ /* cmp edx,1000000h */;
    /* 0x00416879 jnb */ if (edx >= 0x1000000UL) goto l_416895; /* unsigned */
l_41687B:
    /* 0x0041687B */ eax = d4v_get_byte(c); /* call sub_4220B0, al=byte */
    /* 0x00416880 */ ecx = (ecx << 8) & U32MASK;
    /* 0x00416883 */ eax &= 0xFFUL;
    /* 0x00416888 */ edx = (edx << 8) & U32MASK;
    /* 0x0041688B */ ecx |= eax;
    /* 0x0041688D */ /* cmp edx,1000000h */;
    /* 0x00416893 jb */ if (edx < 0x1000000UL) goto l_41687B; /* unsigned */
l_416895:
    /* 0x00416895 */ G32(0x45D0B8) = (unsigned int)ecx;
    /* 0x0041689B */ G32(0x45D0BC) = (unsigned int)edx;
    /* 0x004168A1 */ /* cmp bl,[45C67D] */;
    /* 0x004168A7 jz */ if ((ebx & 0xFFUL) == (unsigned long)G8(0x45C67D))
        goto l_416908;
    /* 0x004168A9 */ G8(0x45C67D) = (unsigned char)(ebx & 0xFFUL);
    /* 0x004168AF */ eax = (unsigned long)M32((0x4342C4UL + esi * 4) & U32MASK);
    /* 0x004168B6 */ edx = (unsigned long)M32((0x43429CUL + esi * 4) & U32MASK);
    /* 0x004168BD */ G32(0x45AF3C) = (unsigned int)eax;
    /* 0x004168C2 */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)G8((0x434314UL + esi) & U32MASK);
    /* 0x004168C8 */ G32(0x45AF40) = (unsigned int)edx;
    /* 0x004168CE */ G8(0x45C67A) = (unsigned char)(eax & 0xFFUL);
    /* 0x004168D3 */ /* cmp bl,5 */;
    /* 0x004168D6 jnz */ if ((ebx & 0xFFUL) != 5) goto l_4169D8;
l_4168DC:
    /* 0x004168DC */ eax = (eax & 0xFFFFFF00UL) | 1UL; /* mov al,1 */
l_4168DE:
    /* 0x004168DE */ ecx = (ecx & 0xFFFFFF00UL) |
        (unsigned long)G8(0x45C67D); /* mov cl,[..] */
    /* 0x004168E4 */ G8(0x45C688) = (unsigned char)(eax & 0xFFUL);
    /* 0x004168E9 */ ebx &= 0xFFFFFF00UL; /* xor bl,bl */
    /* 0x004168EB */ edi = 0; /* xor edi,edi */
    /* 0x004168ED */ G8(0x45C67B) = 0;
    /* 0x004168F3 */ G8(0x45C679) = 0;
    /* 0x004168F9 */ G32(0x45AF48) = 0;
    /* 0x004168FF */ /* cmp cl,4 */;
    /* 0x00416902 ja */ if ((ecx & 0xFFUL) > 4) goto l_4169E8; /* unsigned */
l_416908:
    /* epilogue (loc_416908 shares this addr) 0x00416908 add esp,1Ch;
     * 0x0041690B pop ebp; 0x0041690C pop edi; 0x0041690D pop esi;
     * 0x0041690E pop edx; 0x0041690F pop ecx; 0x00416910 pop ebx;
     * 0x00416911 retn (eax kept). */
    return (int)(eax & U32MASK);
l_416912:
    /* 0x00416912 */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)s[edi]; /* mov al,[esp+edi+var_34];
        trace range: edi = w1c-ecx in 1..w1c (ecx<w1c) */
    /* 0x00416915 */ a_ = (0x45A280UL + esi) & U32MASK;
    G8(a_) = (unsigned char)((G8(a_) + (eax & 0xFFUL)) & 0xFFUL);
    /* 0x0041691B jmp */ goto l_416371;
l_416920:
    /* 0x00416920 */ G32(0x45AFB4) = (unsigned int)ebp;
    /* 0x00416926 jmp */ goto l_4166DF;
l_41692B:
    /* 0x0041692B */ /* cmp ecx,ebp */;
    /* 0x0041692D jnb */ if (ecx >= ebp) goto l_416722; /* unsigned */
    /* 0x00416933 jmp */ goto l_4166F9;
l_416938:
    /* 0x00416938 */ eax = (unsigned long)G32(0x45CD14);
    /* 0x0041693D jmp */ goto l_416730;
l_416942:
    /* 0x00416942 */ edi = (unsigned long)G32(0x45CD28);
    /* 0x00416948 jmp */ goto l_416775;
l_41694D:
    /* 0x0041694D */ /* cmp ecx,8F4D0h */;
    /* 0x00416953 jnz */ if (ecx != 0x8F4D0UL) goto l_41679E;
    /* 0x00416959 */ /* cmp esi,2D9A22h */;
    /* 0x0041695F jz */ if (esi == 0x2D9A22UL) goto l_41678D;
    /* 0x00416965 jmp */ goto l_41679E;
l_41696A:
    /* 0x0041696A */ /* cmp ecx,2CC640h */;
    /* 0x00416970 jnz */ if (ecx != 0x2CC640UL) goto l_4167C0;
    /* 0x00416976 */ /* cmp esi,6AF80h */;
    /* 0x0041697C jz */ if (esi == 0x6AF80UL) goto l_4167B6;
    /* 0x00416982 jmp */ goto l_4167C0;
l_416987:
    /* 0x00416987 */ /* cmp ecx,7D4C0h */;
    /* 0x0041698D jnz */ if (ecx != 0x7D4C0UL) goto l_41699B;
    /* 0x0041698F */ /* cmp esi,3C41Eh */;
    /* 0x00416995 jz */ if (esi == 0x3C41EUL) goto l_4167D8;
l_41699B:
    /* 0x0041699B */ /* cmp ecx,0FB670h */;
    /* 0x004169A1 jz */ if (ecx == 0xFB670UL) goto l_416809;
    /* 0x004169A7 */ /* cmp esi,0FB670h */;
    /* 0x004169AD jnz */ if (esi != 0xFB670UL) goto l_416809;
    /* 0x004169B3 */ /* cmp edi,[45BA44] */;
    /* 0x004169B9 jnb */ if (edi >= (unsigned long)G32(0x45BA44))
        goto l_4167D8; /* unsigned */
    /* 0x004169BF jmp */ goto l_416809;
l_4169C4:
    /* 0x004169C4 */ eax = 0xFFUL;
    /* 0x004169C9 */ eax = (unsigned long)(unsigned int)d4v_41cf60(c,
        (unsigned int)(eax & 0xFFUL)); /* call sub_41CF60 */
    /* epilogue 0x004169CE add esp,1Ch; 0x004169D1 pop ebp;
     * 0x004169D2 pop edi; 0x004169D3 pop esi; 0x004169D4 pop edx;
     * 0x004169D5 pop ecx; 0x004169D6 pop ebx; 0x004169D7 retn
     * with eax = callee result. */
    return (int)(eax & U32MASK);
l_4169D8:
    /* 0x004169D8 */ /* cmp bl,6 */;
    /* 0x004169DB jz */ if ((ebx & 0xFFUL) == 6) goto l_4168DC;
    /* 0x004169E1 */ eax &= 0xFFFFFF00UL; /* xor al,al */
    /* 0x004169E3 jmp */ goto l_4168DE;
l_4169E8:
    /* 0x004169E8 */ d4v_41e5f0(c); /* call sub_41E5F0 */
    /* epilogue 0x004169ED add esp,1Ch; 0x004169F0 pop ebp;
     * 0x004169F1 pop edi; 0x004169F2 pop esi; 0x004169F3 pop edx;
     * 0x004169F4 pop ecx; 0x004169F5 pop ebx; 0x004169F6 retn
     * (eax stale after void call, as in asm). */
    /* UNCERTAIN(004169E8): eax value on this exit is callee leftover. */
    return (int)(eax & U32MASK);
}

/* ------------------------------------------------------------------ */
/* sub_417E10 (lst 34905-35404) -> d4v_ppmdec                           */
/*                                                                      */
/* Role hypothesis: per-symbol PPM match decoder. Hashes the            */
/* 45BA18/45BA1C/45BA20 shift registers into the [0x433B4C]/[0x433B50] */
/* chains, walks the two-entry chain at edx (top = [var_34]) comparing */
/* the last 0x40 dict bytes (64-byte memcmp, 8-wide then byte tail) to  */
/* find the longest match (var_20, entry edi), then length/flag         */
/* classification (var_24/var_1C via 45BA38/45BA48/45C691/418590/      */
/* 416C80/419CA0/416B50 helpers) and either returns 0 (no PPM token,   */
/* caller falls through to sub_418830) or emits bl bytes through       */
/* sub_4183D0/sub_4177A0 into the dict and returns 1. Called by         */
/* sub_417DA0 per symbol; its al is tested (test al,al).                */
/* Entry-live regs: none (eax/ebx/ecx/edx/esi/edi/ebp all derived       */
/* from globals; no stack args). Returns unsigned int = al (0/1).       */
/* Frame: 0x24 bytes + 6 pushed regs. Locals: v3c @ sp+0, v38 @ sp+4,  */
/* v34 @ sp+8, v30 @ sp+0xC, v2c @ sp+0x10 (dword + byte lanes),       */
/* v28 @ sp+0x14, v24 @ sp+0x18, v20 @ sp+0x1C, v1c @ sp+0x20 (bytes).  */
/* Globals touched: 45BA18,45BA1C,45BA20,433B4C,433B50,45BA34,45BA3C,  */
/* 45BA30,433B40,433B54,45BA38,45BA48,45BA40,45C691,45AFB4,45AFA8,      */
/* 45AFD0,45BA24,45C67F,45C68A,45C65E,45C68E,T43408C,T43407C,45AFA4,    */
/* 45C689 (via helpers), helpers sub_418590/416C80/419CA0/416B50/      */
/* 4183D0/4177A0.                                                       */
/* ------------------------------------------------------------------ */
unsigned int d4v_ppmdec(dec4v_ctx *c)
{
    unsigned long eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned long v3c = 0, v38 = 0, v34 = 0, v30 = 0, v2c = 0;
    unsigned long v28 = 0, v24 = 0, v20 = 0, v1c = 0; /* byte lanes, kept 0..255 */
    unsigned long a_ = 0;
    unsigned long dd_ = 0;

    /* prologue 0x00417E10 push ebx; 0x00417E11 push ecx;
     * 0x00417E12 push edx; 0x00417E13 push esi; 0x00417E14 push edi;
     * 0x00417E15 push ebp; 0x00417E16 sub esp,24h.
     * Callee-saved regs become C locals; the esp frame is v3c..v1c. */

    /* 0x00417E19 */ eax = (unsigned long)G32(0x45BA18);
    /* 0x00417E1E */ edx = (unsigned long)G32(0x45BA18);
    /* 0x00417E24 */ eax >>= 0xD;
    /* 0x00417E27 */ eax ^= edx;
    /* 0x00417E29 */ ecx = (unsigned long)G32(0x45BA18);
    /* 0x00417E2F */ eax &= 0x3FFFFUL;
    /* 0x00417E34 */ edx = (unsigned long)G32(0x433B4C);
    /* 0x00417E3A */ v30 = eax;
    /* 0x00417E3E */ eax = (eax << 3) & U32MASK;
    /* 0x00417E41 */ ebx = (unsigned long)G32(0x45BA1C);
    /* 0x00417E47 */ edx = (edx + eax) & U32MASK;
    /* 0x00417E49 */ eax = (unsigned long)G32(0x45BA1C);
    /* 0x00417E4E */ v34 = edx;
    /* 0x00417E52 */ edx = (unsigned long)G32(0x45BA18);
    /* 0x00417E58 */ eax >>= 0xF;
    /* 0x00417E5B */ edx >>= 0xF;
    /* 0x00417E5E */ eax ^= ebx;
    /* 0x00417E60 */ edx ^= ecx;
    /* 0x00417E62 */ eax = (eax << 6) & U32MASK;
    /* 0x00417E65 */ edx = (edx << 4) & U32MASK;
    /* 0x00417E68 */ edx ^= eax;
    /* 0x00417E6A */ eax = (unsigned long)G32(0x45BA20);
    /* 0x00417E6F */ esi = (unsigned long)G32(0x45BA20);
    /* 0x00417E75 */ eax >>= 0xF;
    /* 0x00417E78 */ eax ^= esi;
    /* 0x00417E7A */ eax ^= edx;
    /* 0x00417E7C */ eax &= 0x3FFFFUL;
    /* 0x00417E81 */ edx = (eax * 8) & U32MASK; /* lea edx,[eax*8] */
    /* 0x00417E88 */ ebx &= 0xFFFF00FFUL; /* xor bh,bh */
    /* 0x00417E8A */ eax = (unsigned long)G32(0x433B50);
    /* 0x00417E8F */ edx = (edx + 4) & U32MASK;
    /* 0x00417E92 */ ecx = (ecx & 0xFFFFFF00UL) | 2UL; /* mov cl,2 */
    /* 0x00417E94 */ edx = (edx + eax) & U32MASK;
    /* 0x00417E96 */ eax &= 0xFFFF00FFUL; /* xor ah,ah */
    /* 0x00417E98 */ v20 = (ebx >> 8) & 0xFFUL; /* =0 */
    /* 0x00417E9C */ v24 = (eax >> 8) & 0xFFUL; /* =0 */
    /* 0x00417EA0 */ v1c = (eax >> 8) & 0xFFUL; /* =0 */
    /* 0x00417EA4 */ eax = (unsigned long)G32(0x45BA34);
    /* 0x00417EA9 */ v28 = ecx & 0xFFUL; /* =2 */
    /* 0x00417EAD */ v38 = eax;
    /* 0x00417EB1 */ /* cmp eax,40h */;
    /* 0x00417EB4 jb */ if (eax < 0x40UL) goto l_417FB7; /* unsigned */
l_417EBA:
    /* 0x00417EBA */ eax = v38;
    /* 0x00417EBE */ eax = (eax - 1) & U32MASK; /* dec */
    /* 0x00417EBF */ v3c = eax;
l_417EC2:
    /* 0x00417EC2 */ ecx = (ecx & 0xFFFF00FFUL) | ((v28 & 0xFFUL) << 8);
    /* 0x00417EC6 */ ecx = (ecx & 0xFFFF00FFUL) |
        ((((((ecx >> 8) & 0xFFUL) - 1) & 0xFFUL)) << 8); /* dec ch */
    /* 0x00417EC8 */ eax = (unsigned long)M32(edx);
    /* 0x00417ECA */ v28 = (ecx >> 8) & 0xFFUL;
    /* 0x00417ECE */ /* cmp eax,0FFFFFFFFh */;
    /* 0x00417ED1 jz */ if (eax == 0xFFFFFFFFUL) goto l_417FA3;
    /* 0x00417ED7 */ ebx = (unsigned long)G32(0x45BA34);
    /* 0x00417EDD */ ecx = eax;
    /* 0x00417EDF */ esi = (unsigned long)G32(0x45BA3C);
    /* 0x00417EE5 */ ecx = (ecx - ebx) & U32MASK;
    /* 0x00417EE7 */ ecx &= esi;
    /* 0x00417EE9 */ /* cmp ecx,140h */;
    /* 0x00417EEF jb */ if (ecx < 0x140UL) goto l_417FA3; /* unsigned */
    /* 0x00417EF5 */ /* cmp eax,40h */;
    /* 0x00417EF8 jnb */ if (eax >= 0x40UL) goto l_417F00; /* unsigned */
    /* 0x00417EFA */ eax = (eax + (unsigned long)G32(0x45BA30)) & U32MASK;
l_417F00:
    /* 0x00417F00 */ ecx = (unsigned long)G32(0x433B40);
    /* 0x00417F06 */ esi = v38;
    /* 0x00417F0A */ ebp = (ecx + eax) & U32MASK; /* lea */
    /* 0x00417F0D */ ebx = (ebx & 0xFFFFFF00UL) |
        (unsigned long)M8((ecx + esi - 0xCUL) & U32MASK);
    /* 0x00417F11 */ /* cmp bl,[ebp-0Ch] */;
    /* 0x00417F14 jnz */ if ((ebx & 0xFFUL) !=
        (unsigned long)M8((ebp - 0xCUL) & U32MASK)) goto l_417FA3;
    /* 0x00417F1A */ eax = (eax - 1) & U32MASK; /* dec */
    /* 0x00417F1B */ ebx = v3c;
    /* 0x00417F1E */ esi = (ecx + eax) & U32MASK; /* lea */
    /* 0x00417F21 */ ebx = (ebx + ecx) & U32MASK;
    /* 0x00417F23 */ ecx = (ecx & 0xFFFFFF00UL) | 0x40UL; /* mov cl,40h */
l_417F25:
    /* 0x00417F25 */ /* cmp cl,8 */;
    /* 0x00417F28 jb */ if ((ecx & 0xFFUL) < 8UL) goto l_417F82; /* unsigned */
    /* 0x00417F2A */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8(ebx);
    /* 0x00417F2C */ /* cmp al,[esi] */;
    /* 0x00417F2E jnz */ if ((eax & 0xFFUL) != (unsigned long)M8(esi))
        goto l_417F93;
    /* 0x00417F30 */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)M8((ebx - 1) & U32MASK);
    /* 0x00417F33 */ DECB_LO(ecx); /* dec cl */
    /* 0x00417F35 */ /* cmp al,[esi-1] */;
    /* 0x00417F38 jnz */ if ((eax & 0xFFUL) !=
        (unsigned long)M8((esi - 1) & U32MASK)) goto l_417F93;
    /* 0x00417F3A */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)M8((ebx - 2) & U32MASK);
    /* 0x00417F3D */ DECB_LO(ecx); /* dec cl */
    /* 0x00417F3F */ /* cmp al,[esi-2] */;
    /* 0x00417F42 jnz */ if ((eax & 0xFFUL) !=
        (unsigned long)M8((esi - 2) & U32MASK)) goto l_417F93;
    /* 0x00417F44 */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)M8((ebx - 3) & U32MASK);
    /* 0x00417F47 */ DECB_LO(ecx); /* dec cl */
    /* 0x00417F49 */ /* cmp al,[esi-3] */;
    /* 0x00417F4C jnz */ if ((eax & 0xFFUL) !=
        (unsigned long)M8((esi - 3) & U32MASK)) goto l_417F93;
    /* 0x00417F4E */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)M8((ebx - 4) & U32MASK);
    /* 0x00417F51 */ DECB_LO(ecx); /* dec cl */
    /* 0x00417F53 */ /* cmp al,[esi-4] */;
    /* 0x00417F56 jnz */ if ((eax & 0xFFUL) !=
        (unsigned long)M8((esi - 4) & U32MASK)) goto l_417F93;
    /* 0x00417F58 */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)M8((ebx - 5) & U32MASK);
    /* 0x00417F5B */ DECB_LO(ecx); /* dec cl */
    /* 0x00417F5D */ /* cmp al,[esi-5] */;
    /* 0x00417F60 jnz */ if ((eax & 0xFFUL) !=
        (unsigned long)M8((esi - 5) & U32MASK)) goto l_417F93;
    /* 0x00417F62 */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)M8((ebx - 6) & U32MASK);
    /* 0x00417F65 */ DECB_LO(ecx); /* dec cl */
    /* 0x00417F67 */ /* cmp al,[esi-6] */;
    /* 0x00417F6A jnz */ if ((eax & 0xFFUL) !=
        (unsigned long)M8((esi - 6) & U32MASK)) goto l_417F93;
    /* 0x00417F6C */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)M8((ebx - 7) & U32MASK);
    /* 0x00417F6F */ DECB_LO(ecx); /* dec cl */
    /* 0x00417F71 */ /* cmp al,[esi-7] */;
    /* 0x00417F74 jnz */ if ((eax & 0xFFUL) !=
        (unsigned long)M8((esi - 7) & U32MASK)) goto l_417F93;
    /* 0x00417F76 */ ebx = (ebx - 8) & U32MASK;
    /* 0x00417F79 */ esi = (esi - 8) & U32MASK;
    /* 0x00417F7C */ DECB_LO(ecx); /* dec cl */
    /* 0x00417F7E jnz */ if ((ecx & 0xFFUL) != 0) goto l_417F25;
    /* 0x00417F80 jmp */ goto l_417F93;
l_417F82:
    /* 0x00417F82 */ esi = (esi - ebx) & U32MASK;
    /* 0x00417F84 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8(ebx);
l_417F86:
    /* 0x00417F86 */ /* cmp al,[ebx+esi] */;
    /* 0x00417F89 jnz */ if ((eax & 0xFFUL) !=
        (unsigned long)M8((ebx + esi) & U32MASK)) goto l_417F93;
    /* 0x00417F8B */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)M8((ebx - 1) & U32MASK);
    /* 0x00417F8E */ ebx = (ebx - 1) & U32MASK; /* dec */
    /* 0x00417F8F */ DECB_LO(ecx); /* dec cl */
    /* 0x00417F91 jnz */ if ((ecx & 0xFFUL) != 0) goto l_417F86;
l_417F93:
    /* 0x00417F93 */ eax = (eax & 0xFFFFFF00UL) | 0x40UL; /* mov al,40h */
    /* 0x00417F95 */ eax = (eax & 0xFFFFFF00UL) |
        (((eax & 0xFFUL) - (ecx & 0xFFUL)) & 0xFFUL); /* sub al,cl */
    /* 0x00417F97 */ /* cmp al,[var_20] */;
    /* 0x00417F9B jb */ if ((eax & 0xFFUL) < v20) goto l_417FA3; /* unsigned */
    /* 0x00417F9D */ v20 = eax & 0xFFUL;
    /* 0x00417FA1 */ edi = (unsigned long)M32(edx);
l_417FA3:
    /* 0x00417FA3 */ /* cmp [var_28],0 */;
    /* 0x00417FA8 jz */ if (v28 == 0) goto l_417FCB;
    /* 0x00417FAA */ edx = (edx - 4) & U32MASK;
    /* 0x00417FAD */ eax = (unsigned long)M32(edx);
    /* 0x00417FAF */ M32((edx + 4) & U32MASK) = (unsigned int)eax;
    /* 0x00417FB2 jmp */ goto l_417EC2;
l_417FB7:
    /* 0x00417FB7 */ ecx = v38;
    /* 0x00417FBB */ eax = (unsigned long)G32(0x45BA30);
    /* 0x00417FC0 */ ecx = (ecx + eax) & U32MASK;
    /* 0x00417FC2 */ v38 = ecx;
    /* 0x00417FC6 jmp */ goto l_417EBA;
l_417FCB:
    /* 0x00417FCB */ eax = (unsigned long)G32(0x45BA34);
    /* 0x00417FD0 */ M32(edx) = (unsigned int)eax;
    /* 0x00417FD2 */ /* cmp [var_20],0Ch */;
    /* 0x00417FD7 jb */ if (v20 < 0xCUL) goto l_417FDE; /* unsigned */
    /* 0x00417FD9 */ v24 = 0x15UL;
l_417FDE:
    /* 0x00417FDE */ /* cmp [45BA38],0 */;
    /* 0x00417FE5 jz */ if (G32(0x45BA38) == 0) goto l_418014;
    /* 0x00417FE7 */ eax = (unsigned long)G32(0x45BA34);
    /* 0x00417FEC */ edx = (unsigned long)G32(0x45BA48);
    /* 0x00417FF2 */ edi = (unsigned long)G32(0x45BA3C);
    /* 0x00417FF8 */ eax = (eax - edx) & U32MASK;
    /* 0x00417FFA */ ecx = (unsigned long)G32(0x45BA38);
    /* 0x00418000 */ edi &= eax;
    /* 0x00418002 */ /* cmp ecx,2 */;
    /* 0x00418005 setnbe al: al = (ecx > 2) unsigned */;
    eax = (eax & 0xFFFFFF00UL) | (unsigned long)(ecx > 2UL ? 1 : 0);
    /* 0x00418008 */ edx = (edx & 0xFFFFFF00UL) | 1UL; /* mov dl,1 */
    /* 0x0041800A */ ADD8_LO(eax, 0x50UL); /* add al,50h */
    /* 0x0041800C */ v1c = edx & 0xFFUL; /* =1 */
    /* 0x00418010 */ v24 = eax & 0xFFUL;
l_418014:
    /* 0x00418014 */ /* cmp [var_24],0 */;
    /* 0x00418019 jnz */ if (v24 != 0) goto l_41809F;
    /* 0x0041801F */ eax = (unsigned long)G32(0x45BA34);
    /* 0x00418024 */ ebx = (unsigned long)G32(0x45BA48);
    /* 0x0041802A */ edi = (unsigned long)G32(0x45BA3C);
    /* 0x00418030 */ eax = (eax - ebx) & U32MASK;
    /* 0x00418032 */ edi &= eax;
    /* 0x00418034 */ esi = (unsigned long)G32(0x45BA34);
    /* 0x0041803A */ eax = edi;
    /* 0x0041803C */ ebp = (unsigned long)G32(0x45BA3C);
    /* 0x00418042 */ eax = (eax - esi) & U32MASK;
    /* 0x00418044 */ eax &= ebp;
    /* 0x00418046 */ /* cmp eax,104h */;
    /* 0x0041804B jb */ if (eax < 0x104UL) goto l_4181D4; /* unsigned */
    /* 0x00418051 */ eax = (edi - 4) & U32MASK; /* lea eax,[edi-4] */
    /* 0x00418054 */ edx = (unsigned long)G32(0x433B40);
    /* 0x0041805A */ eax &= ebp;
    /* 0x0041805C */ eax = (eax + edx) & U32MASK;
    /* 0x0041805E */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8(eax);
    /* 0x00418060 */ v2c = (v2c & 0x00FFFFFFUL) |
        (((edx & 0xFFUL) << 24) & 0xFF000000UL); /* [var_2C+3]=dl */
    /* 0x00418064 */ edx = (edx & 0xFFFFFF00UL) |
        (unsigned long)M8((eax + 1) & U32MASK);
    /* 0x00418067 */ v2c = (v2c & 0xFF00FFFFUL) |
        (((edx & 0xFFUL) << 16) & 0x00FF0000UL); /* [var_2C+2]=dl */
    /* 0x0041806B */ edx = (edx & 0xFFFFFF00UL) |
        (unsigned long)M8((eax + 2) & U32MASK);
    /* 0x0041806E */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)M8((eax + 3) & U32MASK); /* mov al,[eax+3] */
    /* NOTE: 0x0041806E overwrites eax used as base above; lanes first. */
    /* 0x00418071 */ v2c = (v2c & 0xFFFF00FFUL) |
        (((edx & 0xFFUL) << 8) & 0x0000FF00UL); /* [var_2C+1]=dl */
    /* 0x00418075 */ v2c = (v2c & 0xFFFFFF00UL) | (eax & 0xFFUL);
    /* 0x00418079 */ edx = v2c;
    /* 0x0041807D */ eax = (unsigned long)G32(0x45BA18);
    /* 0x00418082 */ edx &= 0xFFFFUL;
    /* 0x00418088 */ eax &= 0xFFFFUL;
    /* 0x0041808D */ /* cmp edx,eax */;
    /* 0x0041808F jnz */ if (edx != eax) goto l_4181A8;
l_418095:
    /* 0x00418095 */ v24 = 0xBUL;
    /* 0x0041809A */ v1c = 1;
l_41809F:
    /* 0x0041809F */ edx = v34;
    /* 0x004180A3 */ eax = (unsigned long)G32(0x45BA34);
    /* 0x004180A8 */ M32(edx) = (unsigned int)eax;
    /* 0x004180AA */ eax = (unsigned long)G32(0x45BA18);
    /* 0x004180AF */ M32((edx + 4) & U32MASK) = (unsigned int)eax;
    /* 0x004180B2 */ /* cmp [var_24],0 */;
    /* 0x004180B7 jz */ if (v24 == 0) goto l_41824E;
    /* 0x004180BD */ ebx = (unsigned long)G32(0x45BA34);
    /* 0x004180C3 */ eax = edi;
    /* 0x004180C5 */ esi = (unsigned long)G32(0x45BA3C);
    /* 0x004180CB */ eax = (eax - ebx) & U32MASK;
    /* 0x004180CD */ eax &= esi;
    /* 0x004180CF */ /* cmp eax,100h */;
    /* 0x004180D4 jb */ if (eax < 0x100UL) goto l_41824E; /* unsigned */
    /* 0x004180DA */ /* cmp [45BA38],0 */;
    /* 0x004180E1 jnz */ if (G32(0x45BA38) != 0) goto l_418112;
    /* 0x004180E3 */ edx = v30;
    /* 0x004180E7 */ eax = (unsigned long)G32(0x433B54);
    /* 0x004180EC */ eax = (eax + edx) & U32MASK;
    /* 0x004180EE */ /* cmp byte [eax],0 */;
    /* 0x004180F1 jz */ if (M8(eax) == 0) goto l_418216;
    /* 0x004180F7 */ eax = 0xFFFFFFFFUL;
l_4180FC:
    /* 0x004180FC */ edx = 0; /* xor */
    /* 0x004180FE */ edx |= (unsigned long)G8(0x45C691); /* mov dl,[..] */
    /* 0x00418104 */ edx = (edx + eax) & U32MASK;
    /* 0x00418106 */ eax = 0; /* xor */
    /* 0x00418108 */ eax |= v24; /* mov al,[var_24] */
    /* 0x0041810C */ eax = (eax + edx) & U32MASK;
    /* 0x0041810E */ v24 = eax & 0xFFUL;
l_418112:
    /* 0x00418112 */ ecx = (unsigned long)G32(0x45BA40);
    /* 0x00418118 */ /* test ecx,ecx */;
    /* 0x0041811A jnz */ if (ecx != 0) goto l_418220;
l_418120:
    /* 0x00418120 */ eax = 0; /* xor */
    /* 0x00418122 */ eax |= v24; /* mov al,[var_24] */
    /* 0x00418126 */ eax = d4v_418590(c, (unsigned int)(eax & 0xFFUL));
    /* 0x0041812B */ /* cmp al,1 */;
    /* 0x0041812D jnz */ if ((eax & 0xFFUL) != 1) goto l_418240;
    /* 0x00418133 */ ebx = (ebx & 0xFFFFFF00UL) | v1c; /* mov bl,[var_1C] */
    /* 0x00418137 */ eax = (unsigned long)G32(0x45AFA8);
    /* 0x0041813C */ /* test bl,bl */;
    /* 0x0041813E setz dl: dl = (bl==0) */;
    edx = (edx & 0xFFFFFF00UL) |
        (unsigned long)((ebx & 0xFFUL) == 0 ? 1 : 0);
    /* 0x00418141 */ eax = (eax << 2) & U32MASK;
    /* 0x00418144 */ G8(0x45C68A) = (unsigned char)(edx & 0xFFUL);
    /* 0x0041814A */ /* test dl,dl */;
    /* 0x0041814C jz */ if ((edx & 0xFFUL) == 0) goto l_418276;
    /* 0x00418152 */ ecx = (unsigned long)G32(0x45AFD0);
    /* 0x00418158 */ edx = 0; /* xor */
    /* 0x0041815A div ecx: edx:eax / ecx (unsigned), edx==0 */;
    dd_ = (unsigned long)eax;
    eax = (unsigned long)(dd_ / ecx);
    edx = (unsigned long)(dd_ % ecx);
    /* 0x0041815C */ ebx = (ebx & 0xFFFF00FFUL) |
        ((unsigned long)G8(0x45C67F) << 8); /* mov bh,[..] */
    /* 0x00418162 */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)G8((0x43408CUL + (eax & U32MASK)) & U32MASK);
    /* 0x00418168 */ edx = (unsigned long)G32(0x45BA24);
    /* 0x0041816E */ SUB8_LO(eax, (ebx >> 8) & 0xFFUL); /* sub al,bh */
    /* 0x00418170 */ /* cmp edx,ecx */;
    /* 0x00418172 jbe */ if (edx <= ecx) goto l_418262; /* unsigned */
    /* 0x00418178 */ eax = (eax & 0xFFFFFF00UL) | 0x1DUL; /* mov al,1Dh */
l_41817A:
    /* 0x0041817A */ G8(0x45C65E) = 3;
l_418181:
    /* 0x00418181 */ eax = (eax & 0xFFFF00FFUL) |
        (((eax & 0xFFUL) << 8) & 0xFF00UL); /* mov ah,al */
    /* 0x00418183 */ eax = (eax & 0xFFFFFF00UL) |
        (((eax & 0xFFUL) - 1) & 0xFFUL); /* dec al */
    /* 0x00418185 */ /* cmp ah,7 */;
    /* 0x00418188 jbe */ if (((eax >> 8) & 0xFFUL) <= 7UL)
        goto l_41828B; /* unsigned */
    /* 0x0041818E */ eax = (eax & 0xFFFF00FFUL) |
        ((unsigned long)G8(0x45C68A) << 8); /* mov ah,[..] */
    /* 0x00418194 */ edx = 0; /* xor */
    /* 0x00418196 */ edx |= ((eax >> 8) & 0xFFUL); /* mov dl,ah */
    /* 0x00418198 */ G8((0x45C65EUL + (edx & 0xFFUL)) & U32MASK) =
        (unsigned char)(eax & 0xFFUL);
    /* 0x0041819E */ ADD8_HI(eax, 1UL); /* inc ah */
    /* 0x004181A0 */ G8(0x45C68A) = (unsigned char)((eax >> 8) & 0xFFUL);
    /* 0x004181A6 jmp */ goto l_418181;
l_4181A8:
    /* 0x004181A8 */ eax = v2c;
    /* 0x004181AC */ edx = (unsigned long)G32(0x45BA18);
    /* 0x004181B2 */ eax &= 0xFFFF00FFUL; /* xor ah,ah */
    /* 0x004181B4 */ edx &= 0xFFFF00FFUL; /* xor dh,dh */
    /* 0x004181B6 */ /* cmp eax,edx */;
    /* 0x004181B8 jz */ if (eax == edx) goto l_418095;
    /* 0x004181BE */ eax = v2c;
    /* 0x004181C2 */ edx = (unsigned long)G32(0x45BA18);
    /* 0x004181C8 */ eax &= 0xFFFFFF00UL; /* xor al,al */
    /* 0x004181CA */ edx &= 0xFFFFFF00UL; /* xor dl,dl */
    /* 0x004181CC */ /* cmp eax,edx */;
    /* 0x004181CE jz */ if (eax == edx) goto l_418095;
l_4181D4:
    /* 0x004181D4 */ /* cmp [var_24],0 */;
    /* 0x004181D9 jnz */ if (v24 != 0) goto l_41809F;
    /* 0x004181DF */ eax = v34;
    /* 0x004181E3 */ ecx = (unsigned long)G32(0x45BA18);
    /* 0x004181E9 */ /* cmp ecx,[eax+4] */;
    /* 0x004181EC jnz */ if (ecx != (unsigned long)M32((eax + 4) & U32MASK))
        goto l_418203;
    /* 0x004181EE */ edi = v34;
    /* 0x004181F2 */ edi = (unsigned long)M32(edi);
    /* 0x004181F4 */ /* cmp edi,0FFFFFFFFh */;
    /* 0x004181F7 setnz al: al = (edi != -1) */;
    eax = (eax & 0xFFFFFF00UL) | (unsigned long)(edi != 0xFFFFFFFFUL ? 1 : 0);
    /* 0x004181FA */ v24 = eax & 0xFFUL;
    /* 0x004181FE jmp */ goto l_41809F;
l_418203:
    /* 0x00418203 */ /* cmp edi,0FFFFFFFFh */;
    /* 0x00418206 jz */ if (edi == 0xFFFFFFFFUL) goto l_41809F;
    /* 0x0041820C */ v24 = 0x1FUL;
    /* 0x00418211 jmp */ goto l_41809F;
l_418216:
    /* 0x00418216 */ eax = 0x27UL;
    /* 0x0041821B jmp */ goto l_4180FC;
l_418220:
    /* 0x00418220 */ eax = (unsigned long)G32(0x433B40);
    /* 0x00418225 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8(ecx);
    /* 0x00418227 */ /* cmp dl,[edi+eax] */;
    /* 0x0041822A jnz */ if ((edx & 0xFFUL) !=
        (unsigned long)M8((edi + eax) & U32MASK)) goto l_418120;
    /* 0x00418230 */ eax = 0; /* xor */
    /* 0x00418232 */ edx = 2;
    /* 0x00418237 */ eax |= v24; /* mov al,[var_24] */
    /* 0x0041823B */ d4v_416c80(c, (unsigned int)(eax & 0xFFUL),
        (unsigned int)(edx & 0xFFUL)); /* call sub_416C80 */
l_418240:
    /* 0x00418240 */ ebx = v30;
    /* 0x00418244 */ eax = (unsigned long)G32(0x433B54);
    /* 0x00418249 */ eax = (eax + ebx) & U32MASK;
    /* 0x0041824B */ M8(eax) = 0;
l_41824E:
    /* 0x0041824E */ esi = 0; /* xor */
    /* 0x00418250 */ eax &= 0xFFFFFF00UL; /* xor al,al */
    /* 0x00418252 */ G32(0x45BA38) = (unsigned int)esi; /* =0 */
    /* epilogue 0x00418258 add esp,24h; 0x0041825B pop ebp;
     * 0x0041825C pop edi; 0x0041825D pop esi; 0x0041825E pop edx;
     * 0x0041825F pop ecx; 0x00418260 pop ebx; 0x00418261 retn, al=0. */
    return (unsigned int)(eax & 0xFFUL);
l_418262:
    /* 0x00418262 */ /* cmp [45C68E],0 */;
    /* 0x00418269 jz */ if (G8(0x45C68E) == 0) goto l_41817A;
    /* 0x0041826F */ ADD8_LO(eax, eax & 0xFFUL); /* add al,al */
    /* 0x00418271 jmp */ goto l_41817A;
l_418276:
    /* 0x00418276 */ ebp = (unsigned long)G32(0x45AFD0);
    /* 0x0041827C */ edx = 0; /* xor */
    /* 0x0041827E div ebp: edx:eax / ebp (unsigned), edx==0 */;
    dd_ = (unsigned long)eax;
    eax = (unsigned long)(dd_ / ebp);
    edx = (unsigned long)(dd_ % ebp);
    /* 0x00418280 */ eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)G8((0x43407CUL + (eax & U32MASK)) & U32MASK);
    /* 0x00418286 jmp */ goto l_418181;
l_41828B:
    /* 0x0041828B */ eax = (unsigned long)G32(0x45BA34);
    /* 0x00418290 */ ecx = (unsigned long)G32(0x45BA3C);
    /* 0x00418296 */ eax = (eax - edi) & U32MASK;
    /* 0x00418298 */ ecx &= eax;
    /* 0x0041829A */ /* cmp [var_1C],0 */;
    /* 0x0041829F jnz */ if (v1c != 0) goto l_4183B2;
    /* 0x004182A5 */ esi = v30;
    /* 0x004182A9 */ eax = (unsigned long)G32(0x433B54);
    /* 0x004182AE */ eax = (eax + esi) & U32MASK;
    /* 0x004182B0 */ /* cmp byte [eax],10h */;
    /* 0x004182B3 jb */ if (M8(eax) < 0x10) goto l_4183B2; /* unsigned */
    /* 0x004182B9 */ edx = 1;
l_4182BE:
    /* 0x004182BE */ ebp = 0x30CUL;
    /* 0x004182C3 */ eax = (unsigned long)G32(0x45BA38);
    /* 0x004182C8 */ edx = (edx + 0x2FBUL) & U32MASK;
    /* 0x004182CE */ G32(0x45AFA4) = (unsigned int)ebp;
    /* 0x004182D4 */ /* test eax,eax */;
    /* 0x004182D6 jz */ if (eax == 0) goto l_4182E8;
    /* 0x004182D8 */ /* cmp eax,2 */;
    /* 0x004182DB setnbe al: al = (eax > 2) unsigned */;
    eax = (eax & 0xFFFFFF00UL) | (unsigned long)(eax > 2UL ? 1 : 0);
    /* 0x004182DE */ edx = 0; /* xor */
    /* 0x004182E0 */ edx |= (eax & 0xFFUL); /* mov dl,al */
    /* 0x004182E2 */ edx = (edx + 0x2FDUL) & U32MASK;
l_4182E8:
    /* 0x004182E8 */ eax = edx;
    /* 0x004182EA */ eax = d4v_419ca0(c, (unsigned int)(eax & U32MASK));
    /* 0x004182EF */ ebx = (ebx & 0xFFFF00FFUL) |
        (((eax & 0xFFUL) << 8) & 0xFF00UL); /* mov bh,al */
    /* 0x004182F1 */ ebx = (ebx & 0xFFFFFF00UL) | (eax & 0xFFUL); /* mov bl,al */
    /* 0x004182F3 */ eax = 0; /* xor */
    /* 0x004182F5 */ eax |= ((ebx >> 8) & 0xFFUL); /* mov al,bh */
    /* 0x004182F7 */ esi = v30;
    /* 0x004182FB */ d4v_416b50(c, (unsigned int)(eax & 0xFFUL),
        (unsigned int)(edx & U32MASK)); /* call sub_416B50 */
    /* 0x00418300 */ eax = (unsigned long)G32(0x433B54);
    /* 0x00418305 */ ebp = 0; /* xor */
    /* 0x00418307 */ eax = (eax + esi) & U32MASK;
    /* 0x00418309 */ G32(0x45BA48) = (unsigned int)ecx; /* callee-preserved */
    /* 0x0041830F */ G32(0x45BA40) = (unsigned int)ebp; /* =0 */
    /* 0x00418315 */ M8(eax) = (unsigned char)((ebx >> 8) & 0xFFUL);
    /* 0x00418317 */ /* cmp bh,0FFh */;
    /* 0x0041831A jz */ if (((ebx >> 8) & 0xFFUL) == 0xFF) goto l_4183B9;
    /* 0x00418320 */ /* cmp bh,7 */;
    /* 0x00418323 jb */ if (((ebx >> 8) & 0xFFUL) < 7UL)
        goto l_418341; /* unsigned */
    /* 0x00418325 */ eax = 0; /* xor */
    /* 0x00418327 */ eax |= ((ebx >> 8) & 0xFFUL); /* mov al,bh */
    /* 0x00418329 */ edx = (unsigned long)G32(0x45BA3C);
    /* 0x0041832F */ eax = (eax + edi) & U32MASK;
    /* 0x00418331 */ eax &= edx;
    /* 0x00418333 */ edx = (unsigned long)G32(0x433B40);
    /* 0x00418339 */ edx = (edx + eax) & U32MASK;
    /* 0x0041833B */ G32(0x45BA40) = (unsigned int)edx;
l_418341:
    /* 0x00418341 */ ecx = 0; /* xor */
    /* 0x00418343 */ G32(0x45BA38) = (unsigned int)ecx; /* =0 */
l_418349:
    /* 0x00418349 */ eax = (unsigned long)G32(0x433B40);
    /* 0x0041834E */ ecx = 0; /* xor */
    /* 0x00418350 */ ebx = (ebx & 0xFFFF00FFUL) |
        ((unsigned long)M8((edi + eax) & U32MASK) << 8); /* mov bh,[edi+eax] */
    /* 0x00418353 */ ecx |= ((ebx >> 8) & 0xFFUL); /* mov cl,bh */
    /* 0x00418355 */ eax = ecx;
    /* 0x00418357 */ edx = 0; /* xor */
    /* 0x00418359 */ d4v_4183d0(c, (unsigned int)(eax & U32MASK),
        (unsigned int)(edx & U32MASK)); /* call sub_4183D0 (ecx dead:
        entry ecx is written before any read; full edx live: mov dl,al) */
    /* 0x0041835E */ /* cmp bl,8 */;
    /* 0x00418361 setbe al: al = (bl <= 8) unsigned */;
    eax = (eax & 0xFFFFFF00UL) |
        (unsigned long)((ebx & 0xFFUL) <= 8UL ? 1 : 0);
    /* 0x00418364 */ edx = (edx & 0xFFFFFF00UL) | (eax & 0xFFUL); /* mov dl,al */
    /* 0x00418366 */ eax = ecx;
    /* 0x00418368 */ d4v_4177a0(c, (unsigned int)(eax & 0xFFUL),
        (unsigned int)(edx & 0xFFUL)); /* call sub_4177A0 */
    /* 0x0041836D */ edx = (unsigned long)G32(0x433B40);
    /* 0x00418373 */ eax = (unsigned long)G32(0x45BA34);
    /* 0x00418378 */ M8((edx + eax) & U32MASK) =
        (unsigned char)((ebx >> 8) & 0xFFUL);
    /* 0x0041837B */ /* cmp eax,100h */;
    /* 0x00418380 jnb */ if (eax >= 0x100UL) goto l_41838B; /* unsigned */
    /* 0x00418382 */ eax = (eax + (unsigned long)G32(0x45BA30)) & U32MASK;
    /* 0x00418388 */ M8((edx + eax) & U32MASK) =
        (unsigned char)((ebx >> 8) & 0xFFUL);
l_41838B:
    /* 0x0041838B */ edx = (unsigned long)G32(0x45BA34);
    /* 0x00418391 */ eax = (unsigned long)G32(0x45BA3C);
    /* 0x00418396 */ edx = (edx + 1) & U32MASK; /* inc */
    /* 0x00418397 */ edi = (edi + 1) & U32MASK; /* inc */
    /* 0x00418398 */ edx &= eax;
    /* 0x0041839A */ edi &= eax;
    /* 0x0041839C */ G32(0x45BA34) = (unsigned int)edx;
    /* 0x004183A2 */ DECB_LO(ebx); /* dec bl */
    /* 0x004183A4 jnz */ if ((ebx & 0xFFUL) != 0) goto l_418349;
    /* 0x004183A6 */ eax = (eax & 0xFFFFFF00UL) | 1UL; /* mov al,1 */
    /* epilogue 0x004183A8 add esp,24h; 0x004183AB pop ebp;
     * 0x004183AC pop edi; 0x004183AD pop esi; 0x004183AE pop edx;
     * 0x004183AF pop ecx; 0x004183B0 pop ebx; 0x004183B1 retn, al=1. */
    return (unsigned int)(eax & 0xFFUL);
l_4183B2:
    /* 0x004183B2 */ edx = 0; /* xor */
    /* 0x004183B4 jmp */ goto l_4182BE;
l_4183B9:
    /* 0x004183B9 */ a_ = (unsigned long)G32(0x45BA38);
    a_ = (a_ + 1) & U32MASK; /* inc */
    G32(0x45BA38) = (unsigned int)a_;
    /* 0x004183BF jmp */ goto l_418349;
}

/* ---- worker: per-symbol PPM (from draft_418830.c) ---- */
void d4v_sym(dec4v_ctx *c, unsigned int a_eax)
{
    unsigned int e_eax = 0, e_ebx = 0, e_ecx = 0, e_edx = 0, e_esi = 0, e_edi = 0, e_ebp = 0;
    unsigned int f_cf = 0, f_zf = 0, f_sf = 0, f_of = 0;
    unsigned UHI64 d4v_q = 0;
    UHI64 d4v_n = 0;
    unsigned char stk[0x50] = {0};
    (void)a_eax; /* dead at entry: overwritten at 0x418839/0x418842 */
    f_cf = f_zf = f_sf = f_of = 0u;
    /* prologue: push ebx,ecx,edx,esi,edi,ebp; sub esp,0x50 (frame 0x68) */ /* 0x00418830,0x00418831,0x00418832,0x00418833,0x00418834,0x00418835,0x00418836 */
    e_eax = ((e_eax & ~0xFFu) | (M8(0x45C67Fu))); stk[0x30] = ((e_eax & 0xFFu)); e_eax = ((e_eax & ~(0xFFu << 8)) | ((0x1u) << 8)); /* 0x00418839,0x0041883E,0x00418842 */
    e_edx = ((e_edx & ~0xFFu) | (M8(0x45C68Eu))); stk[0x20] = (((e_eax >> 8) & 0xFFu)); /* cmp (e_edx & 0xFFu), 0x2u (flags folded into jz @0x00418851 below) */; /* 0x00418844,0x0041884A,0x0041884E */
    if (((e_edx & 0xFFu)) == (0x2u)) goto L_418DD2; /* jz, unsigned=True */ /* 0x00418851 */
    e_eax = (M32(0x45BA18u)); M32(0x45AFC4u) = (e_eax); /* 0x00418857,0x0041885C */
L_418861: e_eax = (M32(0x45AFC4u)); e_edx = (M32(0x45AFC4u)); e_eax = ((e_eax) >> 13); e_eax = ((e_eax) ^ (e_edx)); e_eax = ((e_eax) & (0x7FFFu)); /* 0x00418861,0x00418866,0x0041886C,0x0041886F,0x00418871 */
    e_ebp = (M32(0x433B48u)); M32(0x45AFACu) = (e_eax); e_eax = (e_edx); e_esi = (M32(0x45BA40u)); e_eax = ((e_eax) & (0xFFFFu)); /* 0x00418876,0x0041887C,0x00418881,0x00418883,0x00418889 */
    e_edx = ((e_edx & ~(0xFFu << 8)) | ((0u) << 8)); M32(0x45AFBCu) = (e_eax); M8(0x45C68Fu) = (((e_edx >> 8) & 0xFFu)); M8(0x45C683u) = (((e_edx >> 8) & 0xFFu)); /* 0x0041888E,0x00418890,0x00418895,0x0041889B */
    M8(0x45C687u) = (((e_edx >> 8) & 0xFFu)); M8(0x45C685u) = (((e_edx >> 8) & 0xFFu)); M8(0x45C68Du) = (((e_edx >> 8) & 0xFFu)); /* 0x004188A1,0x004188A7,0x004188AD */
    M8(0x45C68Bu) = (((e_edx >> 8) & 0xFFu)); M8(0x45C680u) = (((e_edx >> 8) & 0xFFu)); M8(0x45C684u) = (((e_edx >> 8) & 0xFFu)); /* 0x004188B3,0x004188B9,0x004188BF */
    M8(0x45C68Au) = (((e_edx >> 8) & 0xFFu)); M8(0x45C690u) = (((e_edx >> 8) & 0xFFu)); M8(0x45C693u) = (((e_edx >> 8) & 0xFFu)); e_edx = (e_eax); /* 0x004188C5,0x004188CB,0x004188D1,0x004188D7 */
    e_eax = ((e_eax) << 2); e_ecx = (0u); e_eax = ((e_eax) + (e_edx)); M32(0x45AF88u) = (e_ecx); e_eax = ((e_eax) << 2); M32(0x45AF8Cu) = (e_ecx); /* 0x004188D9,0x004188DC,0x004188DE,0x004188E0,0x004188E6,0x004188E9 */
    e_ebp = ((e_ebp) + (e_eax)); /* test e_esi, e_esi (flags folded into jz @0x004188F3 below) */; /* 0x004188EF,0x004188F1 */
    if ((((e_esi) & (e_esi)) == 0u)) goto L_41890A; /* jz, unsigned=True */ /* 0x004188F3 */
    M32(0x45BA40u) = (e_ecx); e_eax = ((e_eax & ~0xFFu) | (M8(e_esi))); e_ecx = ((e_ecx & ~0xFFu) | (0x1u)); M8(0x45C65Eu) = ((e_eax & 0xFFu)); /* 0x004188F5,0x004188FB,0x004188FD,0x004188FF */
    M8(0x45C68Au) = ((e_ecx & 0xFFu)); /* 0x00418904 */
L_41890A: /* cmp M32(0x45AF98u), 0x1u (flags folded into jbe @0x00418911 below) */; /* 0x0041890A */
    if ((M32(0x45AF98u)) <= (0x1u)) goto L_418E16; /* jbe, unsigned=True */ /* 0x00418911 */
L_418917: e_ecx = (M32(0x45AF7Cu)); e_ebx = (e_ecx); /* cmp e_ecx, 0x4u (flags folded into ja @0x00418922 below) */; /* 0x00418917,0x0041891D,0x0041891F */
    if ((e_ecx) > (0x4u)) goto L_418936; /* ja, unsigned=True */ /* 0x00418922 */
    e_eax = (M32(0x45AF38u)); e_edx = (0u); /* 0x00418924,0x00418929 */
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) /* div faults on 0, as x86 does */;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) /* 0x0041892B */;
    e_ebx = (M32(0x433B90u + e_ecx * 4)); e_ebx = ((e_ebx) + (e_edx)); /* 0x0041892D,0x00418934 */
L_418936: /* cmp e_ecx, 0x10u (flags folded into ja @0x00418939 below) */; /* 0x00418936 */
    if ((e_ecx) > (0x10u)) goto L_41894D; /* ja, unsigned=True */ /* 0x00418939 */
    e_eax = (M32(0x45AF38u)); e_edx = (0u); /* 0x0041893B,0x00418940 */
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) /* div faults on 0, as x86 does */;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) /* 0x00418942 */;
    e_ecx = (M32(0x433BA4u + e_ecx * 4)); e_ecx = ((e_ecx) + (e_edx)); /* 0x00418944,0x0041894B */
L_41894D: e_eax = (0x45B5F4u); e_ebx = ((e_ebx) << 2); e_eax = ((e_eax) + (e_ebx)); e_ecx = ((e_ecx) << 2); M32(0x45AFA0u) = (e_eax); e_eax = (0x45AFD8u); /* 0x0041894D,0x00418952,0x00418955,0x00418957,0x0041895A,0x0041895F */
    e_eax = ((e_eax) + (e_ecx)); M32(0x45AF78u) = (e_eax); /* 0x00418964,0x00418966 */
L_41896B: /* cmp stk[0x30], 0x0u (flags folded into jz @0x00418970 below) */; /* 0x0041896B */
    if ((stk[0x30]) == (0x0u)) goto L_41899E; /* jz, unsigned=True */ /* 0x00418970 */
    /* cmp M8(0x45C688u), 0x0u (flags folded into jz @0x00418979 below) */; /* 0x00418972 */
    if ((M8(0x45C688u)) == (0x0u)) goto L_418E2A; /* jz, unsigned=True */ /* 0x00418979 */
    e_edx = (M32(0x459BA0u)); e_eax = (e_edx); e_ecx = (M32(0x459BA4u)); e_eax = ((e_eax) << 5); e_ecx = ((e_ecx) << 5); e_eax = ((e_eax) - (e_edx)); /* 0x0041897F,0x00418985,0x00418987,0x0041898D,0x00418990,0x00418993 */
    f_cf = (e_eax < e_ecx); f_zf = (e_eax == e_ecx); f_sf = ((((e_eax) - (e_ecx)) >> 31) & 1u);
    f_of = (((((e_eax) ^ (e_ecx)) & ((e_eax) ^ (((e_eax) - (e_ecx))))) >> 31) & 1u); /* 0x00418995 */
L_418997: e_eax = ((e_eax & ~0xFFu) | (((f_cf) & 0xFFu))); stk[0x30] = ((e_eax & 0xFFu)); /* 0x00418997,0x0041899A */
L_41899E: /* cmp M8(0x45C67Du), 0x0u (flags folded into jz @0x004189A5 below) */; /* 0x0041899E */
    if ((M8(0x45C67Du)) == (0x0u)) goto L_4189C5; /* jz, unsigned=True */ /* 0x004189A5 */
    /* cmp M32(0x45AF98u), 0x4u (flags folded into jbe @0x004189AE below) */; /* 0x004189A7 */
    if ((M32(0x45AF98u)) <= (0x4u)) goto L_418E3C; /* jbe, unsigned=True */ /* 0x004189AE */
    e_eax = (M32(0x45AFA0u)); /* cmp M32(e_eax), 0x3FEu (flags folded into jbe @0x004189BF below) */; /* 0x004189B4,0x004189B9 */
    if ((M32(e_eax)) <= (0x3FEu)) goto L_418E3C; /* jbe, unsigned=True */ /* 0x004189BF */
L_4189C5: e_eax = ((e_eax & ~0xFFu) | (0u)); /* 0x004189C5 */
L_4189C7: e_edx = ((e_edx & ~0xFFu) | (stk[0x30])); stk[0x4C] = ((e_eax & 0xFFu)); /* test (e_edx & 0xFFu), (e_edx & 0xFFu) (flags folded into jz @0x004189D1 below) */; /* 0x004189C7,0x004189CB,0x004189CF */
    if (((((((e_edx & 0xFFu)) & ((e_edx & 0xFFu)))) & 0xFFu) == 0u)) goto L_418E43; /* jz, unsigned=True */ /* 0x004189D1 */
    /* test (e_eax & 0xFFu), (e_eax & 0xFFu) (flags folded into jz @0x004189D9 below) */; /* 0x004189D7 */
    if (((((((e_eax & 0xFFu)) & ((e_eax & 0xFFu)))) & 0xFFu) == 0u)) goto L_418E43; /* jz, unsigned=True */ /* 0x004189D9 */
    e_eax = ((e_eax & ~0xFFu) | (0x1u)); /* 0x004189DF */
L_4189E1: stk[0x28] = ((e_eax & 0xFFu)); /* test (e_eax & 0xFFu), (e_eax & 0xFFu) (flags folded into jz @0x004189E7 below) */; /* 0x004189E1,0x004189E5 */
    if (((((((e_eax & 0xFFu)) & ((e_eax & 0xFFu)))) & 0xFFu) == 0u)) goto L_418E4A; /* jz, unsigned=True */ /* 0x004189E7 */
    e_eax = (M32(0x45AF60u)); /* cmp e_eax, M32(0x45AF84u) (flags folded into jnb @0x004189F8 below) */; /* 0x004189ED,0x004189F2 */
    if ((e_eax) >= (M32(0x45AF84u))) goto L_418E4A; /* jnb, unsigned=True */ /* 0x004189F8 */
    e_eax = ((e_eax & ~0xFFu) | (0x1u)); /* 0x004189FE */
L_418A00: stk[0x1C] = ((e_eax & 0xFFu)); e_eax = (M32(0x45AFACu)); e_edi = (M32(0x45AFC4u)); e_eax = ((e_eax) << 5); /* 0x00418A00,0x00418A04,0x00418A09,0x00418A0F */
L_418A12: e_ebx = (M32(0x433B44u)); e_ebx = ((e_ebx) + (e_eax)); /* cmp M8(e_ebx), 0x0u (flags folded into jz @0x00418A1D below) */; /* 0x00418A12,0x00418A18,0x00418A1A */
    if ((M8(e_ebx)) == (0x0u)) goto L_41924A; /* jz, unsigned=True */ /* 0x00418A1D */
    e_edx = (M32(e_ebx + 0x1Cu)); /* cmp e_edx, e_edi (flags folded into jnz @0x00418A28 below) */; /* 0x00418A23,0x00418A26 */
    if ((e_edx) != (e_edi)) goto L_419257; /* jnz, unsigned=True */ /* 0x00418A28 */
    e_ecx = ((e_ecx & ~0xFFu) | (0xFFu)); e_eax = (0u); e_edx = ((e_edx) ^ (e_edi)); e_eax = ((e_eax & ~0xFFu) | (M8(e_ebx + 0x1u))); /* 0x00418A2E,0x00418A30,0x00418A32,0x00418A34 */
    e_edx = ((e_edx & ~0xFFu) | (M8(e_ebx))); M8(0x45C692u) = ((e_ecx & 0xFFu)); e_edx = ((e_edx) + (e_eax)); e_eax = (0x1u); M32(0x45AF9Cu) = (e_edx); /* 0x00418A37,0x00418A39,0x00418A3F,0x00418A41,0x00418A46 */
    e_edx = (0u); /* 0x00418A4C */
L_418A4E: e_ecx = (e_eax); e_edx = ((e_edx & ~0xFFu) | (M8(e_ecx + e_ebx + 0x1u))); e_eax = ((e_eax) + 1u); /* 0x00418A4E,0x00418A50,0x00418A54 */
    /* test (e_edx & 0xFFu), (e_edx & 0xFFu) (flags folded into jnz @0x00418A57 below) */; /* 0x00418A55 */
    if (((((((e_edx & 0xFFu)) & ((e_edx & 0xFFu)))) & 0xFFu) != 0u)) goto L_418E51; /* jnz, unsigned=True */ /* 0x00418A57 */
L_418A5D: /* cmp M32(0x45AFC0u), 0x0u (flags folded into jnz @0x00418A64 below) */; /* 0x00418A5D */
    if ((M32(0x45AFC0u)) != (0x0u)) goto L_418E65; /* jnz, unsigned=True */ /* 0x00418A64 */
    e_eax = (M32(0x45BA24u)); /* cmp e_eax, M32(0x45AFD0u) (flags folded into ja @0x00418A75 below) */; /* 0x00418A6A,0x00418A6F */
    if ((e_eax) > (M32(0x45AFD0u))) goto L_418E65; /* ja, unsigned=True */ /* 0x00418A75 */
    /* cmp M32(0x45AF98u), 0x1u (flags folded into jbe @0x00418A82 below) */; /* 0x00418A7B */
    if ((M32(0x45AF98u)) <= (0x1u)) goto L_418E65; /* jbe, unsigned=True */ /* 0x00418A82 */
    e_eax = (M32(0x45AFA0u)); /* cmp M32(e_eax), 0x3FEu (flags folded into jbe @0x00418A93 below) */; /* 0x00418A88,0x00418A8D */
    if ((M32(e_eax)) <= (0x3FEu)) goto L_418E65; /* jbe, unsigned=True */ /* 0x00418A93 */
    e_eax = (M32(0x45AF78u)); /* cmp M32(e_eax), 0x0u (flags folded into jbe @0x00418AA1 below) */; /* 0x00418A99,0x00418A9E */
    if ((M32(e_eax)) <= (0x0u)) goto L_418E65; /* jbe, unsigned=True */ /* 0x00418AA1 */
L_418AA7: /* cmp stk[0x20], 0x0u (flags folded into jz @0x00418AAC below) */; /* 0x00418AA7 */
    if ((stk[0x20]) == (0x0u)) goto L_41979F; /* jz, unsigned=True */ /* 0x00418AAC */
    /* cmp M8(0x45C67Fu), 0x0u (flags folded into jz @0x00418AB9 below) */; /* 0x00418AB2 */
    if ((M8(0x45C67Fu)) == (0x0u)) goto L_41927F; /* jz, unsigned=True */ /* 0x00418AB9 */
    e_eax = (unsigned)(d4v_416d10(c, e_ebx)); /* 0x00418ABF (ebx live: mov bh,8 partial) */
L_418AC4: stk[0x38] = (0x1u); e_edx = ((e_edx & ~(0xFFu << 8)) | ((0u) << 8)); /* 0x00418AC4,0x00418AC9 */
    (stk[0x18] = ((e_eax) & 0xFFu), stk[0x19] = (((e_eax) >> 8) & 0xFFu), stk[0x1A] = (((e_eax) >> 16) & 0xFFu), stk[0x1B] = (((e_eax) >> 24) & 0xFFu)); /* 0x00418ACB */
    stk[0x3C] = (((e_edx >> 8) & 0xFFu)); /* test e_eax, e_eax (flags folded into jz @0x00418AD5 below) */; /* 0x00418ACF,0x00418AD3 */
    if ((((e_eax) & (e_eax)) == 0u)) goto L_418ADE; /* jz, unsigned=True */ /* 0x00418AD5 */
    M32(0x45AFC0u) = (((M32(0x45AFC0u)) + (0x2u))); /* 0x00418AD7 */
L_418ADE: e_esi = (stk[0x18] | ((unsigned)stk[0x19] << 8) | ((unsigned)stk[0x1A] << 16) | ((unsigned)stk[0x1B] << 24)); /* 0x00418ADE */
    /* test e_esi, e_esi (flags folded into jnz @0x00418AE4 below) */; /* 0x00418AE2 */
    if ((((e_esi) & (e_esi)) != 0u)) goto L_419286; /* jnz, unsigned=True */ /* 0x00418AE4 */
L_418AEA: e_eax = ((e_eax & ~0xFFu) | (0x1u)); /* 0x00418AEA */
L_418AEC: M8(0x45C68Bu) = ((e_eax & 0xFFu)); /* 0x00418AEC */
L_418AF1: /* cmp M8(0x45C68Bu), 0x0u (flags folded into jz @0x00418AF8 below) */; /* 0x00418AF1 */
    if ((M8(0x45C68Bu)) == (0x0u)) goto L_4192BA; /* jz, unsigned=True */ /* 0x00418AF8 */
L_418AFE: e_ecx = ((e_ecx & ~0xFFu) | (0u)); e_edi = (M32(0x45AFCCu)); stk[0x38] = ((e_ecx & 0xFFu)); /* test e_edi, e_edi (flags folded into jz @0x00418B0C below) */; /* 0x00418AFE,0x00418B00,0x00418B06,0x00418B0A */
    if ((((e_edi) & (e_edi)) == 0u)) goto L_4192DB; /* jz, unsigned=True */ /* 0x00418B0C */
    /* cmp M8(e_ebp + 0x11u), 0x0u (flags folded into jz @0x00418B16 below) */; /* 0x00418B12 */
    if ((M8(e_ebp + 0x11u)) == (0x0u)) goto L_4192DB; /* jz, unsigned=True */ /* 0x00418B16 */
    /* cmp M32(0x45AFD4u), 0xFFFFu (flags folded into jb @0x00418B26 below) */; /* 0x00418B1C */
    if ((M32(0x45AFD4u)) < (0xFFFFu)) goto L_4192CA; /* jb, unsigned=True */ /* 0x00418B26 */
L_418B2C: e_eax = ((e_eax & ~0xFFu) | (0u)); /* 0x00418B2C */
L_418B2E: M8(0x45C68Bu) = ((e_eax & 0xFFu)); /* test (e_eax & 0xFFu), (e_eax & 0xFFu) (flags folded into jz @0x00418B35 below) */; /* 0x00418B2E,0x00418B33 */
    if (((((((e_eax & 0xFFu)) & ((e_eax & 0xFFu)))) & 0xFFu) == 0u)) goto L_418CC5; /* jz, unsigned=True */ /* 0x00418B35 */
    /* cmp M8(e_ebp + 0x0u), 0x0u (flags folded into jz @0x00418B3F below) */; /* 0x00418B3B */
    if ((M8(e_ebp + 0x0u)) == (0x0u)) goto L_418CC5; /* jz, unsigned=True */ /* 0x00418B3F */
    e_edi = (D4V_SP + 0x10u); e_eax = ((e_eax & ~(0xFFu << 8)) | ((0u) << 8)); e_esi = (0x4150E8u); stk[0x48] = (((e_eax >> 8) & 0xFFu)); /* 0x00418B45,0x00418B49,0x00418B4B,0x00418B50 */
    /* movsd-fill: */ stk[0x10] = 1u; stk[0x11] = 1u; stk[0x12] = 1u; stk[0x13] = 1u; stk[0x14] = 1u; stk[0x15] = 1u; stk[0x16] = 1u; stk[0x17] = 1u;
    e_esi += 8u; e_edi += 8u; /* 0x00418B54,0x00418B55 */
    /* above: movsd pair: 8 code-const 0x01 bytes (e_esi was 0x4150E8) to stk[0x10..0x17]; UNCERTAIN(src-bytes): inferred from 01 01 opcode display */
    stk[0x40] = (((e_eax >> 8) & 0xFFu)); e_eax = ((e_eax & ~0xFFu) | (M8(e_ebp + 0x12u))); stk[0x44] = ((e_eax & 0xFFu)); /* 0x00418B56,0x00418B5A,0x00418B5D */
L_418B61: e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | (stk[0x48])); /* cmp M8(e_eax + e_ebp + 0x1u), 0x0u (flags folded into jz @0x00418B6C below) */; /* 0x00418B61,0x00418B63,0x00418B67 */
    if ((M8(e_eax + e_ebp + 0x1u)) == (0x0u)) goto L_418BBB; /* jz, unsigned=True */ /* 0x00418B6C */
    e_ebx = ((e_ebx & ~0xFFu) | (stk[0x40])); e_ebx = ((e_ebx & ~0xFFu) | ((((((e_ebx & 0xFFu)) + 1u)) & 0xFFu))); e_eax = (0u); stk[0x40] = ((e_ebx & 0xFFu)); /* 0x00418B6E,0x00418B72,0x00418B74,0x00418B76 */
L_418B7A: e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | (M8(0x45C68Au))); /* cmp e_eax, e_edx (flags folded into jnb @0x00418B84 below) */; /* 0x00418B7A,0x00418B7C,0x00418B82 */
    if ((e_eax) >= (e_edx)) goto L_418BAC; /* jnb, unsigned=True */ /* 0x00418B84 */
    e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | (stk[0x48])); e_ebx = (e_eax); e_ecx = (e_edx + e_ebp); e_ebx = ((e_ebx & ~0xFFu) | (M8(0x45C65Eu + e_ebx))); /* 0x00418B86,0x00418B88,0x00418B8C,0x00418B8E,0x00418B91 */
    e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((M8(e_ecx + 0x9u)) << 8)); e_eax = ((e_eax) + 1u); /* 0x00418B97,0x00418B9A */
    /* cmp (e_ebx & 0xFFu), ((e_ebx >> 8) & 0xFFu) (flags folded into jnz @0x00418B9D below) */; /* 0x00418B9B */
    if (((e_ebx & 0xFFu)) != (((e_ebx >> 8) & 0xFFu))) goto L_418B7A; /* jnz, unsigned=True */ /* 0x00418B9D */
    e_eax = ((e_eax & ~0xFFu) | (0u)); stk[e_edx + 0x10] = ((e_eax & 0xFFu)); e_eax = ((e_eax & ~0xFFu) | (M8(e_ecx + 0x1u))); /* 0x00418B9F,0x00418BA1,0x00418BA5 */
    stk[0x44] = (((((stk[0x44]) - ((e_eax & 0xFFu)))) & 0xFFu)); /* 0x00418BA8 */
L_418BAC: e_edx = ((e_edx & ~0xFFu) | (stk[0x48])); e_edx = ((e_edx & ~0xFFu) | ((((((e_edx & 0xFFu)) + 1u)) & 0xFFu))); stk[0x48] = ((e_edx & 0xFFu)); /* 0x00418BAC,0x00418BB0,0x00418BB2 */
    /* cmp (e_edx & 0xFFu), 0x8u (flags folded into jb @0x00418BB9 below) */; /* 0x00418BB6 */
    if (((e_edx & 0xFFu)) < (0x8u)) goto L_418B61; /* jb, unsigned=True */ /* 0x00418BB9 */
L_418BBB: e_ebx = ((e_ebx & ~0xFFu) | (0xFFu)); e_eax = (0u); e_ecx = (0u); e_edx = (0u); e_eax = ((e_eax & ~0xFFu) | (stk[0x44])); /* 0x00418BBB,0x00418BBD,0x00418BBF,0x00418BC1,0x00418BC3 */
    e_ecx = ((e_ecx & ~0xFFu) | (M8(e_ebp + 0x0u))); M32(0x45AFC8u) = (e_eax); e_edx = ((e_edx & ~0xFFu) | ((e_eax & 0xFFu))); e_ecx = (e_ecx + e_ecx * 4); /* 0x00418BC7,0x00418BCA,0x00418BCF,0x00418BD1 */
    e_eax = (e_edx * 4); M8(0x45C694u) = ((e_ebx & 0xFFu)); /* cmp e_eax, e_ecx (flags folded into jle @0x00418BE3 below) */; /* 0x00418BD4,0x00418BDB,0x00418BE1 */
    if (((int)(e_eax)) <= ((int)(e_ecx))) goto L_418CC5; /* jle, unsigned=False */ /* 0x00418BE3 */
    e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | (M8(e_ebp + 0x11u))); e_ecx = (M32(0x45AFCCu)); e_eax = ((e_eax) << 2); e_eax = ((e_eax) + (e_ecx)); /* 0x00418BE9,0x00418BEB,0x00418BEE,0x00418BF4,0x00418BF7 */
    /* cmp e_eax, 0x10u (flags folded into jnb @0x00418BFC below) */; /* 0x00418BF9 */
    if ((e_eax) >= (0x10u)) goto L_418CC5; /* jnb, unsigned=True */ /* 0x00418BFC */
    e_eax = ((e_eax & ~0xFFu) | (stk[0x28])); e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((stk[0x1C]) << 8)); M8(0x45C687u) = ((e_eax & 0xFFu)); /* 0x00418C02,0x00418C06,0x00418C0A */
    /* test ((e_ebx >> 8) & 0xFFu), ((e_ebx >> 8) & 0xFFu) (flags folded into jnz @0x00418C11 below) */; /* 0x00418C0F */
    if ((((((((e_ebx >> 8) & 0xFFu)) & (((e_ebx >> 8) & 0xFFu)))) & 0xFFu) != 0u)) goto L_418CC5; /* jnz, unsigned=True */ /* 0x00418C11 */
    /* cmp M8(e_ebp + 0x0u), 0x3u (flags folded into jnz @0x00418C1B below) */; /* 0x00418C17 */
    if ((M8(e_ebp + 0x0u)) != (0x3u)) goto L_419308; /* jnz, unsigned=True */ /* 0x00418C1B */
    /* cmp M32(e_ebp + 0x1u), 0x10u (flags folded into jnb @0x00418C25 below) */; /* 0x00418C21 */
    if ((M32(e_ebp + 0x1u)) >= (0x10u)) goto L_419308; /* jnb, unsigned=True */ /* 0x00418C25 */
    e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | (M8(e_ebp + 0x1u))); e_eax = ((unsigned)((int)(e_eax) >> 2)); e_eax = ((e_eax) + (e_eax)); /* 0x00418C2B,0x00418C2D,0x00418C30,0x00418C33 */
    e_eax = ((e_eax) + (0x45A278u)); /* 0x00418C35 */
    e_eax = (unsigned)(d4v_418780(c, e_eax)); /* 0x00418C3A */
    /* test (e_eax & 0xFFu), (e_eax & 0xFFu) (flags folded into jz @0x00418C41 below) */; /* 0x00418C3F */
    if (((((((e_eax & 0xFFu)) & ((e_eax & 0xFFu)))) & 0xFFu) == 0u)) goto L_4192E2; /* jz, unsigned=True */ /* 0x00418C41 */
    e_eax = ((e_eax & ~0xFFu) | (M8(e_ebp + 0x9u))); stk[0x24] = ((e_eax & 0xFFu)); e_eax = (e_edx); e_edx = ((unsigned)((int)(e_edx) >> 31)); /* 0x00418C47,0x00418C4A,0x00418C4E,0x00418C50 */
    e_edx = ((e_edx) << 2); f_cf = ((e_edx >> 30) & 1u); f_zf = (((e_edx) << 2) == 0u) ; f_sf = ((((e_edx) << 2) >> 31) & 1u); /* 0x00418C53 */
    d4v_q = ((unsigned UHI64)(e_edx) + f_cf) /* borrow-in; f_cf source: shl @00418C53 (kept) */;
    e_eax = ((e_eax) - d4v_q) /* 0x00418C56 */;
    e_eax = ((unsigned)((int)(e_eax) >> 2)); e_ebx = ((e_ebx & ~0xFFu) | (0u)); e_ecx = (0u); stk[0x20] = ((e_ebx & 0xFFu)); /* 0x00418C58,0x00418C5B,0x00418C5D,0x00418C5F */
    e_ecx = ((e_ecx & ~0xFFu) | (M8(e_ebp + 0x1u))); M8(0x45C694u) = ((e_ebx & 0xFFu)); /* cmp e_ecx, e_eax (flags folded into jle @0x00418C6E below) */; /* 0x00418C63,0x00418C66,0x00418C6C */
    if (((int)(e_ecx)) <= ((int)(e_eax))) goto L_418C76; /* jle, unsigned=False */ /* 0x00418C6E */
    M8(0x45C693u) = (((((M8(0x45C693u)) + 1u)) & 0xFFu)); /* 0x00418C70 */
L_418C76: e_eax = (0x45AF68u); e_edx = (M32(e_eax + 0x8u)); e_eax = (M32(e_eax + 0x4u)); e_ecx = (M32(0x45D0BCu)); e_edx = ((e_edx) - (e_eax)); /* 0x00418C76,0x00418C7B,0x00418C7E,0x00418C81,0x00418C87 */
    d4v_q = ((UHI64)(int)(e_eax)) * ((UHI64)(int)(e_ecx));
    e_eax = (unsigned)d4v_q /* imul flags (cf/of/zf/sf) dead here: next setter precedes any consumer */ /* 0x00418C89 */;
    d4v_q = ((UHI64)(int)(e_edx)) * ((UHI64)(int)(e_ecx));
    e_edx = (unsigned)d4v_q /* imul flags (cf/of/zf/sf) dead here: next setter precedes any consumer */ /* 0x00418C8C */;
    e_ecx = (M32(0x45D0B8u)); e_ecx = ((e_ecx) - (e_eax)); /* cmp e_edx, 0x1000000u (flags folded into jnb @0x00418C9D below) */; /* 0x00418C8F,0x00418C95,0x00418C97 */
    if ((e_edx) >= (0x1000000u)) goto L_418CB9; /* jnb, unsigned=True */ /* 0x00418C9D */
L_418C9F: e_eax = (unsigned)(d4v_get_byte(c)); /* 0x00418C9F */
    e_ecx = ((e_ecx) << 8); e_eax = ((e_eax) & (0xFFu)); e_edx = ((e_edx) << 8); e_ecx = ((e_ecx) | (e_eax)); /* 0x00418CA4,0x00418CA7,0x00418CAC,0x00418CAF */
    /* cmp e_edx, 0x1000000u (flags folded into jb @0x00418CB7 below) */; /* 0x00418CB1 */
    if ((e_edx) < (0x1000000u)) goto L_418C9F; /* jb, unsigned=True */ /* 0x00418CB7 */
L_418CB9: M32(0x45D0B8u) = (e_ecx); M32(0x45D0BCu) = (e_edx); /* 0x00418CB9,0x00418CBF */
L_418CC5: e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((stk[0x3C]) << 8)); /* test ((e_ebx >> 8) & 0xFFu), ((e_ebx >> 8) & 0xFFu) (flags folded into jz @0x00418CCB below) */; /* 0x00418CC5,0x00418CC9 */
    if ((((((((e_ebx >> 8) & 0xFFu)) & (((e_ebx >> 8) & 0xFFu)))) & 0xFFu) == 0u)) goto L_419482; /* jz, unsigned=True */ /* 0x00418CCB */
L_418CD1: /* cmp stk[0x20], 0x0u (flags folded into jz @0x00418CD6 below) */; /* 0x00418CD1 */
    if ((stk[0x20]) == (0x0u)) goto L_41979F; /* jz, unsigned=True */ /* 0x00418CD6 */
    e_eax = ((e_eax & ~0xFFu) | (stk[0x28])); e_ecx = (0x201u); M8(0x45C68Du) = ((e_eax & 0xFFu)); e_eax = ((e_eax & ~0xFFu) | (M8(0x45C67Fu))); e_edi = (0x302u); /* 0x00418CDC,0x00418CE0,0x00418CE5,0x00418CEA,0x00418CEF */
    /* test (e_eax & 0xFFu), (e_eax & 0xFFu) (flags folded into jz @0x00418CF6 below) */; /* 0x00418CF4 */
    if (((((((e_eax & 0xFFu)) & ((e_eax & 0xFFu)))) & 0xFFu) == 0u)) goto L_4196B6; /* jz, unsigned=True */ /* 0x00418CF6 */
    e_ebx = (M32(0x45AF98u)); /* cmp e_ebx, 0x1u (flags folded into jbe @0x00418D05 below) */; /* 0x00418CFC,0x00418D02 */
    if ((e_ebx) <= (0x1u)) goto L_4196B6; /* jbe, unsigned=True */ /* 0x00418D05 */
    e_eax = (M32(0x45AFA0u)); /* cmp M32(e_eax), 0x0u (flags folded into jz @0x00418D13 below) */; /* 0x00418D0B,0x00418D10 */
    if ((M32(e_eax)) == (0x0u)) goto L_4196B6; /* jz, unsigned=True */ /* 0x00418D13 */
    e_eax = (M32(0x45BA34u)); e_edx = (M32(0x45BA3Cu)); e_eax = ((e_eax) - (e_ebx)); e_edx = ((e_edx) & (e_eax)); e_eax = (M32(0x433B40u)); e_ebp = (e_ebx); /* 0x00418D19,0x00418D1E,0x00418D24,0x00418D26,0x00418D28,0x00418D2D */
    e_ebx = ((e_ebx & ~0xFFu) | (M8(e_edx + e_eax))); e_edx = (e_ebp); e_ebp = (M32(0x45BA34u)); e_edx = ((e_edx) + (e_edx)); e_ebp = ((e_ebp) - (e_edx)); /* 0x00418D2F,0x00418D32,0x00418D34,0x00418D3A,0x00418D3C */
    e_edx = (e_ebp); e_edx = ((e_edx) & (M32(0x45BA3Cu))); e_esi = (e_ebx & 0xFFu); e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((M8(e_edx + e_eax)) << 8)); /* 0x00418D3E,0x00418D40,0x00418D46,0x00418D49 */
    e_esi = ((e_esi) + (0x100u)); /* cmp (e_ebx & 0xFFu), ((e_ebx >> 8) & 0xFFu) (flags folded into jnz @0x00418D54 below) */; /* 0x00418D4C,0x00418D52 */
    if (((e_ebx & 0xFFu)) != (((e_ebx >> 8) & 0xFFu))) goto L_418D5B; /* jnz, unsigned=True */ /* 0x00418D54 */
    e_ecx = (0x202u); /* 0x00418D56 */
L_418D5B: /* cmp stk[0x4C], 0x0u (flags folded into jz @0x00418D60 below) */; /* 0x00418D5B */
    if ((stk[0x4C]) == (0x0u)) goto L_418D7B; /* jz, unsigned=True */ /* 0x00418D60 */
    e_ecx = (M32(0x45AF3Cu)); e_edi = (M8(0x45C67Du)); e_ebx = ((e_ebx & ~0xFFu) | (M8(0x45C679u))); e_edi = ((e_edi) + (0x302u)); /* 0x00418D62,0x00418D68,0x00418D6F,0x00418D75 */
L_418D7B: /* cmp stk[0x4C], 0x0u (flags folded into jz @0x00418D80 below) */; /* 0x00418D7B */
    if ((stk[0x4C]) == (0x0u)) goto L_4196FE; /* jz, unsigned=True */ /* 0x00418D80 */
    /* cmp M8(0x45C67Bu), 0x0u (flags folded into jz @0x00418D8D below) */; /* 0x00418D86 */
    if ((M8(0x45C67Bu)) == (0x0u)) goto L_4196FE; /* jz, unsigned=True */ /* 0x00418D8D */
    e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((0x1u) << 8)); /* 0x00418D93 */
L_418D95: e_edx = ((e_edx & ~(0xFFu << 8)) | ((stk[0x30]) << 8)); stk[0x2C] = (((e_ebx >> 8) & 0xFFu)); /* 0x00418D95,0x00418D99 */
    /* test ((e_edx >> 8) & 0xFFu), ((e_edx >> 8) & 0xFFu) (flags folded into jz @0x00418D9F below) */; /* 0x00418D9D */
    if ((((((((e_edx >> 8) & 0xFFu)) & (((e_edx >> 8) & 0xFFu)))) & 0xFFu) == 0u)) goto L_41981C; /* jz, unsigned=True */ /* 0x00418D9F */
    e_eax = (0u); /* test ((e_ebx >> 8) & 0xFFu), ((e_ebx >> 8) & 0xFFu) (flags folded into jz @0x00418DA9 below) */; /* 0x00418DA5,0x00418DA7 */
    if ((((((((e_ebx >> 8) & 0xFFu)) & (((e_ebx >> 8) & 0xFFu)))) & 0xFFu) == 0u)) goto L_419705; /* jz, unsigned=True */ /* 0x00418DA9 */
L_418DAF: e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | (M8(0x45C68Au))); /* cmp e_eax, e_edx (flags folded into jnb @0x00418DB9 below) */; /* 0x00418DAF,0x00418DB1,0x00418DB7 */
    if ((e_eax) >= (e_edx)) goto L_419724; /* jnb, unsigned=True */ /* 0x00418DB9 */
    e_edx = ((e_edx & ~0xFFu) | ((e_ebx & 0xFFu))); e_edx = ((e_edx & ~(0xFFu << 8)) | ((M8(0x45C65Eu + e_eax)) << 8)); e_eax = ((e_eax) + 1u); /* 0x00418DBF,0x00418DC1,0x00418DC7 */
    e_edx = ((e_edx & ~0xFFu) | ((((((e_edx & 0xFFu)) - (((e_edx >> 8) & 0xFFu)))) & 0xFFu))); M8(0x45C65Du + e_eax) = ((e_edx & 0xFFu)); /* 0x00418DC8,0x00418DCA */
    goto L_418DAF; /* 0x00418DD0 */
L_418DD2: e_edx = (M32(0x45BA18u)); e_eax = (M32(0x45BA18u)); e_edx = ((e_edx) >> 8); e_eax = ((e_eax) >> 16); e_edx = ((e_edx) & (0xFFu)); e_eax = ((e_eax) & (0xFF00u)); /* 0x00418DD2,0x00418DD8,0x00418DDD,0x00418DE0,0x00418DE3,0x00418DE9 */
    e_edx = ((e_edx) + (e_eax)); e_eax = (M32(0x45BA1Cu)); e_eax = ((e_eax) << 8); e_eax = ((e_eax) & (0xFF0000u)); e_edx = ((e_edx) + (e_eax)); /* 0x00418DEE,0x00418DF0,0x00418DF5,0x00418DF8,0x00418DFD */
    e_eax = (M32(0x45BA1Cu)); e_eax = ((e_eax) & (0xFF000000u)); e_edx = ((e_edx) + (e_eax)); M32(0x45AFC4u) = (e_edx); /* 0x00418DFF,0x00418E04,0x00418E09,0x00418E0B */
    goto L_418861; /* 0x00418E11 */
L_418E16: e_eax = ((e_eax & ~0xFFu) | (M8(0x45AF7Cu))); e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) - (0x2u))) & 0xFFu))); /* 0x00418E16,0x00418E1B */
    /* cmp (e_eax & 0xFFu), 0x2u (flags folded into jbe @0x00418E1F below) */; /* 0x00418E1D */
    if (((e_eax & 0xFFu)) <= (0x2u)) goto L_418917; /* jbe, unsigned=True */ /* 0x00418E1F */
    goto L_41896B; /* 0x00418E25 */
L_418E2A: e_eax = (M32(0x459BA0u)); e_edx = (M32(0x459BA4u)); f_cf = (e_eax < e_edx); f_zf = (e_eax == e_edx); f_sf = ((((e_eax) - (e_edx)) >> 31) & 1u); /* 0x00418E2A,0x00418E2F */
    f_of = (((((e_eax) ^ (e_edx)) & ((e_eax) ^ (((e_eax) - (e_edx))))) >> 31) & 1u); /* 0x00418E35 */
    goto L_418997; /* 0x00418E37 */
L_418E3C: e_eax = ((e_eax & ~0xFFu) | (0x1u)); /* 0x00418E3C */
    goto L_4189C7; /* 0x00418E3E */
L_418E43: e_eax = ((e_eax & ~0xFFu) | (0u)); /* 0x00418E43 */
    goto L_4189E1; /* 0x00418E45 */
L_418E4A: e_eax = ((e_eax & ~0xFFu) | (0u)); /* 0x00418E4A */
    goto L_418A00; /* 0x00418E4C */
L_418E51: M32(0x45AF9Cu) = (((M32(0x45AF9Cu)) + (e_edx))); /* cmp e_eax, 0xDu (flags folded into jb @0x00418E5A below) */; /* 0x00418E51,0x00418E57 */
    if ((e_eax) < (0xDu)) goto L_418A4E; /* jb, unsigned=True */ /* 0x00418E5A */
    goto L_418A5D; /* 0x00418E60 */
L_418E65: e_eax = ((e_eax & ~0xFFu) | (M8(e_ebx + 0x1Bu))); e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) & (0x3Fu))) & 0xFFu))); /* 0x00418E65,0x00418E68 */
    /* cmp (e_eax & 0xFFu), 0x8u (flags folded into jnb @0x00418E6C below) */; /* 0x00418E6A */
    if (((e_eax & 0xFFu)) >= (0x8u)) goto L_418EB5; /* jnb, unsigned=True */ /* 0x00418E6C */
    /* cmp M8(e_ebp + 0x0u), 0x0u (flags folded into jz @0x00418E72 below) */; /* 0x00418E6E */
    if ((M8(e_ebp + 0x0u)) == (0x0u)) goto L_418EB5; /* jz, unsigned=True */ /* 0x00418E72 */
    e_eax = ((e_eax) & (0xFFu)); e_edx = (0x3E0u); e_eax = ((e_eax) << 5); e_edx = ((e_edx) - (e_eax)); e_eax = (e_edx); e_edx = (0u); /* 0x00418E74,0x00418E79,0x00418E7E,0x00418E81,0x00418E83,0x00418E85 */
    e_edx = ((e_edx & ~0xFFu) | (M8(e_ebp + 0x1u))); /* 0x00418E87 */
    d4v_q = ((UHI64)(int)(e_edx)) * ((UHI64)(int)(e_eax));
    e_edx = (unsigned)d4v_q /* imul flags (cf/of/zf/sf) dead here: next setter precedes any consumer */ /* 0x00418E8A */;
    e_ecx = (0u); e_ecx = ((e_ecx & ~0xFFu) | (M8(e_ebp + 0x12u))); e_eax = (e_edx); e_edx = ((unsigned)((int)(e_edx) >> 31)); /* 0x00418E8D,0x00418E8F,0x00418E92,0x00418E94 */
    d4v_n = (((UHI64)(int)e_edx << 32) + (UHI64)e_eax);
    e_eax = (unsigned)(int)(d4v_n / ((UHI64)(int)(e_ecx)));
    e_edx = (unsigned)(int)(d4v_n % ((UHI64)(int)(e_ecx))) /* 0x00418E97 */;
    e_ecx = (e_eax); e_eax = (0u); e_esi = (M32(0x45AF9Cu)); e_eax = ((e_eax & ~0xFFu) | (M8(e_ebx + 0x1u))); e_edx = (0u); e_eax = ((e_eax) << 10); /* 0x00418E99,0x00418E9B,0x00418E9D,0x00418EA3,0x00418EA6,0x00418EA8 */
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) /* div faults on 0, as x86 does */;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_esi)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_esi))) /* 0x00418EAB */;
    /* cmp e_ecx, e_eax (flags folded into ja @0x00418EAF below) */; /* 0x00418EAD */
    if ((e_ecx) > (e_eax)) goto L_418AA7; /* ja, unsigned=True */ /* 0x00418EAF */
L_418EB5: e_eax = ((e_eax & ~0xFFu) | (0x1u)); e_edi = (D4V_SP); M8(0x45C68Fu) = ((e_eax & 0xFFu)); e_eax = (M32(0x45AF9Cu)); e_esi = (0x4150DBu); /* 0x00418EB5,0x00418EB7,0x00418EB9,0x00418EBE,0x00418EC3 */
    M32(0x45AFB8u) = (e_eax); e_eax = ((e_eax & ~(0xFFu << 8)) | ((M8(0x45C68Au)) << 8)); /* 0x00418EC8,0x00418ECD */
    /* movsd-fill: */ stk[0x00] = 1u; stk[0x01] = 1u; stk[0x02] = 1u; stk[0x03] = 1u; stk[0x04] = 1u; stk[0x05] = 1u; stk[0x06] = 1u; stk[0x07] = 1u; stk[0x08] = 1u; stk[0x09] = 1u; stk[0x0A] = 1u; stk[0x0B] = 1u; stk[0x0C] = 1u;
    e_esi += 13u; e_edi += 13u; /* 0x00418ED3,0x00418ED4,0x00418ED5,0x00418ED6 */
    /* above: movsd x3 + movsb: 13 code-const 0x01 bytes (e_esi was 0x4150DB) to stk[0x00..0x0C]; UNCERTAIN(src-bytes): inferred from 01 01 opcode display */
    /* test ((e_eax >> 8) & 0xFFu), ((e_eax >> 8) & 0xFFu) (flags folded into jz @0x00418ED9 below) */; /* 0x00418ED7 */
    if ((((((((e_eax >> 8) & 0xFFu)) & (((e_eax >> 8) & 0xFFu)))) & 0xFFu) == 0u)) goto L_418EF8; /* jz, unsigned=True */ /* 0x00418ED9 */
    e_eax = ((e_eax & ~0xFFu) | (0u)); e_eax = ((e_eax & ~(0xFFu << 8)) | ((M8(0x45C65Eu)) << 8)); stk[0x34] = (((e_eax >> 8) & 0xFFu)); /* 0x00418EDB,0x00418EDD,0x00418EE3 */
L_418EE7: e_ecx = (0u); e_ecx = ((e_ecx & ~0xFFu) | ((e_eax & 0xFFu))); e_edx = (e_ebx + e_ecx); /* 0x00418EE7,0x00418EE9,0x00418EEB */
    /* cmp M8(e_edx + 0x1u), 0x0u (flags folded into jnz @0x00418EF2 below) */; /* 0x00418EEE */
    if ((M8(e_edx + 0x1u)) != (0x0u)) goto L_418FEC; /* jnz, unsigned=True */ /* 0x00418EF2 */
L_418EF8: e_eax = (0u); e_edx = (M32(0x45AFB8u)); e_eax = ((e_eax & ~0xFFu) | (M8(e_ebx))); /* cmp e_eax, e_edx (flags folded into jnb @0x00418F04 below) */; /* 0x00418EF8,0x00418EFA,0x00418F00,0x00418F02 */
    if ((e_eax) >= (e_edx)) goto L_418AA7; /* jnb, unsigned=True */ /* 0x00418F04 */
    e_eax = ((e_eax & ~0xFFu) | (stk[0x28])); M8(0x45C683u) = ((e_eax & 0xFFu)); e_eax = ((e_eax & ~(0xFFu << 8)) | ((stk[0x1C]) << 8)); /* 0x00418F0A,0x00418F0E,0x00418F13 */
    /* test ((e_eax >> 8) & 0xFFu), ((e_eax >> 8) & 0xFFu) (flags folded into setz @0x00418F19 below) */; /* 0x00418F17 */
    e_eax = ((e_eax & ~0xFFu) | (((((((((((e_eax >> 8) & 0xFFu)) & (((e_eax >> 8) & 0xFFu)))) & 0xFFu) == 0u) )) & 0xFFu))); M8(0x45C68Fu) = ((e_eax & 0xFFu)); /* 0x00418F19,0x00418F1C */
    /* test (e_eax & 0xFFu), (e_eax & 0xFFu) (flags folded into jz @0x00418F23 below) */; /* 0x00418F21 */
    if (((((((e_eax & 0xFFu)) & ((e_eax & 0xFFu)))) & 0xFFu) == 0u)) goto L_418AA7; /* jz, unsigned=True */ /* 0x00418F23 */
    /* cmp M8(e_ebx), 0x1u (flags folded into jnz @0x00418F2C below) */; /* 0x00418F29 */
    if ((M8(e_ebx)) != (0x1u)) goto L_41907F; /* jnz, unsigned=True */ /* 0x00418F2C */
    /* cmp M32(e_ebx + 0x1u), 0x50u (flags folded into jnb @0x00418F36 below) */; /* 0x00418F32 */
    if ((M32(e_ebx + 0x1u)) >= (0x50u)) goto L_41907F; /* jnb, unsigned=True */ /* 0x00418F36 */
    e_eax = (0u); e_edx = (M32(0x45AFC4u)); e_eax = ((e_eax & ~0xFFu) | (M8(e_ebx + 0x1u))); e_edx = ((e_edx) & (0xF0u)); e_eax = (M32(0x433E8Cu + e_eax * 4)); /* 0x00418F3C,0x00418F3E,0x00418F44,0x00418F47,0x00418F4D */
    e_eax = ((e_eax) + (e_edx)); /* cmp M8(0x45C67Fu), 0x0u (flags folded into jz @0x00418F5D below) */; /* 0x00418F54,0x00418F56 */
    if ((M8(0x45C67Fu)) == (0x0u)) goto L_419021; /* jz, unsigned=True */ /* 0x00418F5D */
    e_edx = (M32(0x45AFC4u)); e_edx = ((e_edx) >> 11); e_edx = ((e_edx) & (0xCu)); /* 0x00418F63,0x00418F69,0x00418F6C */
L_418F6F: e_eax = ((e_eax) + (e_edx)); e_eax = ((e_eax) + (e_eax)); e_eax = ((e_eax) + (0x45AA80u)); /* 0x00418F6F,0x00418F71,0x00418F73 */
    e_eax = (unsigned)(d4v_418780(c, e_eax)); /* 0x00418F78 */
    /* test (e_eax & 0xFFu), (e_eax & 0xFFu) (flags folded into jz @0x00418F7F below) */; /* 0x00418F7D */
    if (((((((e_eax & 0xFFu)) & ((e_eax & 0xFFu)))) & 0xFFu) == 0u)) goto L_419059; /* jz, unsigned=True */ /* 0x00418F7F */
    e_eax = ((e_eax & ~0xFFu) | (M8(e_ebx + 0xEu))); e_ebx = ((e_ebx & ~0xFFu) | (0u)); stk[0x24] = ((e_eax & 0xFFu)); stk[0x20] = ((e_ebx & 0xFFu)); /* 0x00418F85,0x00418F88,0x00418F8A,0x00418F8E */
    M8(0x45C692u) = ((e_ebx & 0xFFu)); /* 0x00418F92 */
L_418F98: e_eax = (0x45AF68u); e_edx = (M32(e_eax + 0x8u)); e_eax = (M32(e_eax + 0x4u)); e_ecx = (M32(0x45D0BCu)); e_edx = ((e_edx) - (e_eax)); /* 0x00418F98,0x00418F9D,0x00418FA0,0x00418FA3,0x00418FA9 */
    d4v_q = ((UHI64)(int)(e_eax)) * ((UHI64)(int)(e_ecx));
    e_eax = (unsigned)d4v_q /* imul flags (cf/of/zf/sf) dead here: next setter precedes any consumer */ /* 0x00418FAB */;
    d4v_q = ((UHI64)(int)(e_edx)) * ((UHI64)(int)(e_ecx));
    e_edx = (unsigned)d4v_q /* imul flags (cf/of/zf/sf) dead here: next setter precedes any consumer */ /* 0x00418FAE */;
    e_ecx = (M32(0x45D0B8u)); e_ecx = ((e_ecx) - (e_eax)); /* cmp e_edx, 0x1000000u (flags folded into jnb @0x00418FBF below) */; /* 0x00418FB1,0x00418FB7,0x00418FB9 */
    if ((e_edx) >= (0x1000000u)) goto L_418FDB; /* jnb, unsigned=True */ /* 0x00418FBF */
L_418FC1: e_eax = (unsigned)(d4v_get_byte(c)); /* 0x00418FC1 */
    e_ecx = ((e_ecx) << 8); e_eax = ((e_eax) & (0xFFu)); e_edx = ((e_edx) << 8); e_ecx = ((e_ecx) | (e_eax)); /* 0x00418FC6,0x00418FC9,0x00418FCE,0x00418FD1 */
    /* cmp e_edx, 0x1000000u (flags folded into jb @0x00418FD9 below) */; /* 0x00418FD3 */
    if ((e_edx) < (0x1000000u)) goto L_418FC1; /* jb, unsigned=True */ /* 0x00418FD9 */
L_418FDB: M32(0x45D0B8u) = (e_ecx); M32(0x45D0BCu) = (e_edx); /* 0x00418FDB,0x00418FE1 */
    goto L_418AA7; /* 0x00418FE7 */
L_418FEC: e_eax = ((e_eax & ~(0xFFu << 8)) | ((M8(e_edx + 0xEu)) << 8)); /* cmp ((e_eax >> 8) & 0xFFu), stk[0x34] (flags folded into jnz @0x00418FF3 below) */; /* 0x00418FEC,0x00418FEF */
    if ((((e_eax >> 8) & 0xFFu)) != (stk[0x34])) goto L_419012; /* jnz, unsigned=True */ /* 0x00418FF3 */
    e_eax = ((e_eax & ~0xFFu) | (0u)); stk[e_ecx] = ((e_eax & 0xFFu)); e_eax = (0u); e_edi = (M32(0x45AFB8u)); e_eax = ((e_eax & ~0xFFu) | (M8(e_edx + 0x1u))); /* 0x00418FF5,0x00418FF7,0x00418FFA,0x00418FFC,0x00419002 */
    e_edi = ((e_edi) - (e_eax)); M32(0x45AFB8u) = (e_edi); /* 0x00419005,0x00419007 */
    goto L_418EF8; /* 0x0041900D */
L_419012: e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) + 1u)) & 0xFFu))); /* cmp (e_eax & 0xFFu), 0xDu (flags folded into jb @0x00419016 below) */; /* 0x00419012,0x00419014 */
    if (((e_eax & 0xFFu)) < (0xDu)) goto L_418EE7; /* jb, unsigned=True */ /* 0x00419016 */
    goto L_418EF8; /* 0x0041901C */
L_419021: e_edx = (M32(0x45AFC4u)); e_edx = ((e_edx) >> 8); e_edx = ((e_edx) & (0xFFu)); e_ecx = (0u); e_ecx = ((e_ecx & ~0xFFu) | (M8(0x436B8Au + e_edx))); e_edx = (0u); /* 0x00419021,0x00419027,0x0041902A,0x00419030,0x00419032,0x00419038 */
    e_edx = ((e_edx & ~0xFFu) | (M8(0x45AFC4u))); e_ecx = ((e_ecx) << 2); e_edx = ((e_edx & ~0xFFu) | (M8(0x436B8Au + e_edx))); e_edx = ((e_edx) & (0xFFu)); /* 0x0041903A,0x00419040,0x00419043,0x00419049 */
    e_edx = ((e_edx) << 3); e_edx = ((e_edx) + (e_ecx)); /* 0x0041904F,0x00419052 */
    goto L_418F6F; /* 0x00419054 */
L_419059: e_edx = ((e_edx & ~(0xFFu << 8)) | ((0xDu) << 8)); e_eax = ((e_eax & ~0xFFu) | (M8(0x45C68Au))); M8(0x45C692u) = (((e_edx >> 8) & 0xFFu)); e_edx = (0u); /* 0x00419059,0x0041905B,0x00419060,0x00419066 */
    e_edx = ((e_edx & ~0xFFu) | ((e_eax & 0xFFu))); e_eax = ((e_eax & ~(0xFFu << 8)) | ((M8(e_ebx + 0xEu)) << 8)); /* 0x00419068,0x0041906A */
    M8(0x45C65Eu + e_edx) = (((e_eax >> 8) & 0xFFu)); e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) + 1u)) & 0xFFu))); M8(0x45C68Au) = ((e_eax & 0xFFu)); /* 0x0041906D,0x00419073,0x00419075 */
    goto L_418F98; /* 0x0041907A */
L_41907F: e_edi = (0xFFFFFFFFu); e_eax = (M32(0x45AFB8u)); e_ecx = ((e_ecx & ~0xFFu) | (M8(e_ebx))); e_esi = (0u); e_edx = ((e_edx & ~(0xFFu << 8)) | ((0u) << 8)); /* 0x0041907F,0x00419084,0x00419089,0x0041908B,0x0041908D */
    M32(0x45AF68u) = (e_eax); M32(0x45AF6Cu) = (e_esi); M8(0x45C692u) = (((e_edx >> 8) & 0xFFu)); /* 0x0041908F,0x00419094,0x0041909A */
    /* cmp (e_ecx & 0xFFu), 0x20u (flags folded into jb @0x004190A3 below) */; /* 0x004190A0 */
    if (((e_ecx & 0xFFu)) < (0x20u)) goto L_41911A; /* jb, unsigned=True */ /* 0x004190A3 */
L_4190A5: e_ecx = (M32(0x45AF68u)); e_eax = (M32(0x45D0BCu)); e_edx = (0u); /* 0x004190A5,0x004190AB,0x004190B0 */
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) /* div faults on 0, as x86 does */;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) /* 0x004190B2 */;
    e_edx = (0u); M32(0x45D0BCu) = (e_eax); e_ecx = (e_eax); e_eax = (M32(0x45D0B8u)); /* 0x004190B4,0x004190B6,0x004190BB,0x004190BD */
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) /* div faults on 0, as x86 does */;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) /* 0x004190C2 */;
    e_esi = (e_eax); /* 0x004190C4 */
L_4190C6: e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | (M8(0x45C692u))); /* cmp stk[e_eax], 0x0u (flags folded into jz @0x004190D1 below) */; /* 0x004190C6,0x004190C8,0x004190CD */
    if ((stk[e_eax]) == (0x0u)) goto L_419224; /* jz, unsigned=True */ /* 0x004190D1 */
    e_eax = ((e_eax) + (e_ebx)); /* cmp M8(e_eax + 0x1u), 0x0u (flags folded into jnz @0x004190DD below) */; /* 0x004190D7,0x004190D9 */
    if ((M8(e_eax + 0x1u)) != (0x0u)) goto L_4191BE; /* jnz, unsigned=True */ /* 0x004190DD */
    M8(0x45C692u) = (0xDu); /* 0x004190E3 */
L_4190EA: e_eax = (M32(0x45AF68u)); M32(0x45AF70u) = (e_eax); /* 0x004190EA,0x004190EF */
L_4190F4: /* cmp e_edi, 0xFFFFFFFFu (flags folded into jz @0x004190F7 below) */; /* 0x004190F4 */
    if ((e_edi) == (0xFFFFFFFFu)) goto L_418F98; /* jz, unsigned=True */ /* 0x004190F7 */
    e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((stk[0x20]) << 8)); e_edx = (0u); /* 0x004190FD,0x00419101 */
    /* test ((e_ebx >> 8) & 0xFFu), ((e_ebx >> 8) & 0xFFu) (flags folded into setz @0x00419105 below) */; /* 0x00419103 */
    e_edx = ((e_edx & ~0xFFu) | (((((((((((e_ebx >> 8) & 0xFFu)) & (((e_ebx >> 8) & 0xFFu)))) & 0xFFu) == 0u) )) & 0xFFu))); e_eax = (e_edi + e_edi); /* 0x00419105,0x00419108 */
    e_eax = ((e_eax) + (0x45A280u)); /* 0x0041910B */
    d4v_41aeb0(c, e_eax, e_edx); /* 0x00419110 */
    goto L_418F98; /* 0x00419115 */
L_41911A: e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | ((e_ecx & 0xFFu))); e_eax = ((e_eax) - (e_edx)); e_edx = (e_eax); /* 0x0041911A,0x0041911C,0x0041911E,0x00419120 */
    /* cmp e_eax, 0x80u (flags folded into jnb @0x00419127 below) */; /* 0x00419122 */
    if ((e_eax) >= (0x80u)) goto L_4190A5; /* jnb, unsigned=True */ /* 0x00419127 */
    e_ecx = (0u); e_ecx = ((e_ecx & ~0xFFu) | (M8(0x433FCCu + e_eax))); e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | (M8(e_ebx))); e_ecx = ((e_ecx) << 3); /* 0x0041912D,0x0041912F,0x00419135,0x00419137,0x00419139 */
    e_edi = (M8(0x43404Cu + e_eax)); e_edi = ((e_edi) + (e_ecx)); /* cmp M8(0x45C67Fu), 0x0u (flags folded into jz @0x0041914C below) */; /* 0x0041913C,0x00419143,0x00419145 */
    if ((M8(0x45C67Fu)) == (0x0u)) goto L_419173; /* jz, unsigned=True */ /* 0x0041914C */
    e_eax = (M32(0x45AFC4u)); e_eax = ((e_eax) & (0xC0u)); e_eax = ((e_eax) + (e_eax)); /* 0x0041914E,0x00419153,0x00419158 */
L_41915A: e_edi = ((e_edi) + (e_eax)); e_eax = (e_edi + e_edi); e_eax = ((e_eax) + (0x45A280u)); /* 0x0041915A,0x0041915C,0x0041915F */
    e_eax = (unsigned)(d4v_41ae70(c, e_eax, e_edx)); /* 0x00419164 */
    M32(0x45AF68u) = (e_eax); /* 0x00419169 */
    goto L_4190A5; /* 0x0041916E */
L_419173: e_eax = (M32(0x45AFC4u)); e_eax = ((e_eax) >> 8); e_eax = ((e_eax) & (0xFFu)); e_eax = ((e_eax & ~0xFFu) | (M8(0x436B8Au + e_eax))); e_ecx = (e_eax); /* 0x00419173,0x00419178,0x0041917B,0x00419180,0x00419186 */
    e_eax = (0u); e_ecx = ((e_ecx) & (0xFFu)); e_eax = ((e_eax & ~0xFFu) | (M8(0x45AFC4u))); e_ecx = ((e_ecx) << 8); /* 0x00419188,0x0041918A,0x00419190,0x00419195 */
    e_eax = ((e_eax & ~0xFFu) | (M8(0x436B8Au + e_eax))); e_eax = ((e_eax) & (0xFFu)); e_eax = ((e_eax) << 7); e_ecx = ((e_ecx) + (e_eax)); /* 0x00419198,0x0041919E,0x004191A3,0x004191A6 */
    /* cmp M8(0x45AFC4u), 0xDFu (flags folded into setz @0x004191AF below) */; e_eax = ((e_eax & ~0xFFu) | (((((M8(0x45AFC4u)) == (0xDFu) )) & 0xFFu))); /* 0x004191A8,0x004191AF */
    e_eax = ((e_eax) & (0xFFu)); e_eax = ((e_eax) << 9); e_eax = ((e_eax) + (e_ecx)); /* 0x004191B2,0x004191B7,0x004191BA */
    goto L_41915A; /* 0x004191BC */
L_4191BE: e_ecx = (0u); e_edx = (M32(0x45AF6Cu)); e_ecx = ((e_ecx & ~0xFFu) | (M8(e_eax + 0x1u))); e_edx = ((e_edx) + (e_ecx)); M32(0x45AF70u) = (e_edx); e_edx = (0u); /* 0x004191BE,0x004191C0,0x004191C6,0x004191C9,0x004191CB,0x004191D1 */
    e_ecx = (M32(0x45AF70u)); e_edx = ((e_edx & ~0xFFFFu) | ((e_esi & 0xFFFFu))); /* cmp e_edx, e_ecx (flags folded into jnb @0x004191DE below) */; /* 0x004191D3,0x004191D9,0x004191DC */
    if ((e_edx) >= (e_ecx)) goto L_4191FF; /* jnb, unsigned=True */ /* 0x004191DE */
    e_eax = ((e_eax & ~0xFFu) | (M8(e_eax + 0xEu))); e_edx = ((e_edx & ~0xFFu) | (0u)); stk[0x24] = ((e_eax & 0xFFu)); stk[0x20] = ((e_edx & 0xFFu)); /* 0x004191E0,0x004191E3,0x004191E5,0x004191E9 */
    /* cmp M8(0x45C692u), 0xDu (flags folded into jz @0x004191F4 below) */; /* 0x004191ED */
    if ((M8(0x45C692u)) == (0xDu)) goto L_4190EA; /* jz, unsigned=True */ /* 0x004191F4 */
    goto L_4190F4; /* 0x004191FA */
L_4191FF: e_edx = ((e_edx & ~0xFFu) | (M8(0x45C68Au))); e_ecx = (0u); e_ecx = ((e_ecx & ~0xFFu) | ((e_edx & 0xFFu))); e_eax = ((e_eax & ~0xFFu) | (M8(e_eax + 0xEu))); /* 0x004191FF,0x00419205,0x00419207,0x00419209 */
    e_edx = ((e_edx & ~0xFFu) | ((((((e_edx & 0xFFu)) + 1u)) & 0xFFu))); M8(0x45C65Eu + e_ecx) = ((e_eax & 0xFFu)); e_eax = (M32(0x45AF70u)); /* 0x0041920C,0x0041920E,0x00419214 */
    M8(0x45C68Au) = ((e_edx & 0xFFu)); M32(0x45AF6Cu) = (e_eax); /* 0x00419219,0x0041921F */
L_419224: e_eax = ((e_eax & ~0xFFu) | (M8(0x45C692u))); e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) + 1u)) & 0xFFu))); M8(0x45C692u) = ((e_eax & 0xFFu)); /* 0x00419224,0x00419229,0x0041922B */
    /* cmp (e_eax & 0xFFu), 0xDu (flags folded into jb @0x00419232 below) */; /* 0x00419230 */
    if (((e_eax & 0xFFu)) < (0xDu)) goto L_4190C6; /* jb, unsigned=True */ /* 0x00419232 */
    /* cmp M8(0x45C692u), 0xDu (flags folded into jz @0x0041923F below) */; /* 0x00419238 */
    if ((M8(0x45C692u)) == (0xDu)) goto L_4190EA; /* jz, unsigned=True */ /* 0x0041923F */
    goto L_4190F4; /* 0x00419245 */
L_41924A: M16(0x45AFACu + 0x2u) = (((((M16(0x45AFACu + 0x2u)) + (0x4u))) & 0xFFFFu)); /* 0x0041924A */
    goto L_418AA7; /* 0x00419252 */
L_419257: e_edx = (M32(0x45AFACu)); e_edx = ((e_edx) + (0x8000u)); e_eax = ((e_eax) + (0x100000u)); M32(0x45AFACu) = (e_edx); /* 0x00419257,0x0041925D,0x00419263,0x00419268 */
    /* cmp e_edx, 0x40000u (flags folded into jb @0x00419274 below) */; /* 0x0041926E */
    if ((e_edx) < (0x40000u)) goto L_418A12; /* jb, unsigned=True */ /* 0x00419274 */
    goto L_418AA7; /* 0x0041927A */
L_41927F: e_eax = (0u); /* 0x0041927F */
    goto L_418AC4; /* 0x00419281 */
L_419286: e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | (M8(e_esi + 0x1u))); e_ecx = (e_edx * 4); e_ecx = ((e_ecx) + (e_edx)); e_edx = (0u); /* 0x00419286,0x00419288,0x0041928B,0x00419292,0x00419294 */
    e_edx = ((e_edx & ~0xFFu) | (M8(e_esi))); e_eax = (e_edx * 4); e_eax = ((e_eax) - (e_edx)); /* cmp e_eax, e_ecx (flags folded into jl @0x004192A3 below) */; /* 0x00419296,0x00419298,0x0041929F,0x004192A1 */
    if (((int)(e_eax)) < ((int)(e_ecx))) goto L_4192AC; /* jl, unsigned=False */ /* 0x004192A3 */
L_4192A5: e_eax = ((e_eax & ~0xFFu) | (0u)); /* 0x004192A5 */
    goto L_418AEC; /* 0x004192A7 */
L_4192AC: e_eax = ((e_eax & ~0xFFu) | (M8(e_esi + 0x3u))); e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) & (0x7Fu))) & 0xFFu))); /* 0x004192AC,0x004192AF */
    /* cmp (e_eax & 0xFFu), 0x2u (flags folded into ja @0x004192B3 below) */; /* 0x004192B1 */
    if (((e_eax & 0xFFu)) > (0x2u)) goto L_4192A5; /* ja, unsigned=True */ /* 0x004192B3 */
    goto L_418AEA; /* 0x004192B5 */
L_4192BA: /* cmp stk[0x3C], 0x0u (flags folded into jnz @0x004192BF below) */; /* 0x004192BA */
    if ((stk[0x3C]) != (0x0u)) goto L_418AFE; /* jnz, unsigned=True */ /* 0x004192BF */
    goto L_418CC5; /* 0x004192C5 */
L_4192CA: e_eax = (M32(0x45AFA0u)); /* cmp M32(e_eax), 0x3FEu (flags folded into ja @0x004192D5 below) */; /* 0x004192CA,0x004192CF */
    if ((M32(e_eax)) > (0x3FEu)) goto L_418B2C; /* ja, unsigned=True */ /* 0x004192D5 */
L_4192DB: e_eax = ((e_eax & ~0xFFu) | (0x1u)); /* 0x004192DB */
    goto L_418B2E; /* 0x004192DD */
L_4192E2: e_eax = ((e_eax & ~(0xFFu << 8)) | ((0x8u) << 8)); e_edx = (0u); M8(0x45C694u) = (((e_eax >> 8) & 0xFFu)); e_eax = ((e_eax & ~0xFFu) | (M8(0x45C68Au))); /* 0x004192E2,0x004192E4,0x004192E6,0x004192EC */
    e_edx = ((e_edx & ~0xFFu) | ((e_eax & 0xFFu))); e_eax = ((e_eax & ~(0xFFu << 8)) | ((M8(e_ebp + 0x9u)) << 8)); /* 0x004192F1,0x004192F3 */
    M8(0x45C65Eu + e_edx) = (((e_eax >> 8) & 0xFFu)); e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) + 1u)) & 0xFFu))); M8(0x45C68Au) = ((e_eax & 0xFFu)); /* 0x004192F6,0x004192FC,0x004192FE */
    goto L_418C76; /* 0x00419303 */
L_419308: e_ecx = ((e_ecx & ~(0xFFu << 8)) | ((stk[0x40]) << 8)); e_esi = (0xFFFFFFFFu); /* cmp ((e_ecx >> 8) & 0xFFu), 0x4u (flags folded into jbe @0x00419314 below) */; /* 0x00419308,0x0041930C,0x00419311 */
    if ((((e_ecx >> 8) & 0xFFu)) <= (0x4u)) goto L_4193A3; /* jbe, unsigned=True */ /* 0x00419314 */
L_41931A: e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | (stk[0x44])); e_edi = (0u); e_ecx = (e_eax); M32(0x45AF68u) = (e_eax); e_eax = (M32(0x45D0BCu)); e_edx = (0u); /* 0x0041931A,0x0041931C,0x00419320,0x00419322,0x00419324,0x00419329,0x0041932E */
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) /* div faults on 0, as x86 does */;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) /* 0x00419330 */;
    e_edx = (0u); M32(0x45D0BCu) = (e_eax); e_ecx = (e_eax); e_eax = (M32(0x45D0B8u)); /* 0x00419332,0x00419334,0x00419339,0x0041933B */
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) /* div faults on 0, as x86 does */;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) /* 0x00419340 */;
    e_ebx = (e_eax); e_eax = ((e_eax & ~0xFFu) | (0u)); M32(0x45AF6Cu) = (e_edi); M8(0x45C694u) = ((e_eax & 0xFFu)); /* 0x00419342,0x00419344,0x00419346,0x0041934C */
L_419351: e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | (M8(0x45C694u))); /* cmp stk[e_eax + 0x10], 0x0u (flags folded into jz @0x0041935D below) */; /* 0x00419351,0x00419353,0x00419358 */
    if ((stk[e_eax + 0x10]) == (0x0u)) goto L_419459; /* jz, unsigned=True */ /* 0x0041935D */
    e_eax = ((e_eax) + (e_ebp)); e_edx = ((e_edx & ~0xFFu) | (M8(e_eax + 0x1u))); /* 0x00419363,0x00419365 */
    /* test (e_edx & 0xFFu), (e_edx & 0xFFu) (flags folded into jnz @0x0041936A below) */; /* 0x00419368 */
    if (((((((e_edx & 0xFFu)) & ((e_edx & 0xFFu)))) & 0xFFu) != 0u)) goto L_4193DF; /* jnz, unsigned=True */ /* 0x0041936A */
    M8(0x45C694u) = (0x8u); /* 0x0041936C */
L_419373: e_eax = (M32(0x45AF68u)); M32(0x45AF70u) = (e_eax); /* 0x00419373,0x00419378 */
L_41937D: /* cmp e_esi, 0xFFFFFFFFu (flags folded into jz @0x00419380 below) */; /* 0x0041937D */
    if ((e_esi) == (0xFFFFFFFFu)) goto L_418C76; /* jz, unsigned=True */ /* 0x00419380 */
    e_ebx = ((e_ebx & ~0xFFu) | (stk[0x20])); e_edx = (0u); /* test (e_ebx & 0xFFu), (e_ebx & 0xFFu) (flags folded into setz @0x0041938E below) */; /* 0x00419386,0x0041938A,0x0041938C */
    e_edx = ((e_edx & ~0xFFu) | ((((((((((e_ebx & 0xFFu)) & ((e_ebx & 0xFFu)))) & 0xFFu) == 0u) )) & 0xFFu))); e_eax = (e_esi + e_esi); /* 0x0041938E,0x00419391 */
    e_eax = ((e_eax) + (0x45A258u)); /* 0x00419394 */
    d4v_41aeb0(c, e_eax, e_edx); /* 0x00419399 */
    goto L_418C76; /* 0x0041939E */
L_4193A3: e_eax = (0u); e_edx = (0u); e_eax = ((e_eax & ~0xFFu) | (stk[0x44])); e_edx = ((e_edx & ~0xFFu) | (M8(e_ebp + 0x0u))); e_eax = ((e_eax) - (e_edx)); /* 0x004193A3,0x004193A5,0x004193A7,0x004193AB,0x004193AE */
    e_edx = (e_eax); /* cmp e_eax, 0x10u (flags folded into jnb @0x004193B5 below) */; /* 0x004193B0,0x004193B2 */
    if ((e_eax) >= (0x10u)) goto L_41931A; /* jnb, unsigned=True */ /* 0x004193B5 */
    e_ecx = (0u); e_eax = ((e_eax) & (0xCu)); e_ecx = ((e_ecx & ~0xFFu) | (stk[0x40])); e_eax = ((e_eax) + (e_ecx)); e_esi = (e_eax - 0x1u); e_eax = (e_esi + e_esi); /* 0x004193BB,0x004193BD,0x004193C0,0x004193C4,0x004193C6,0x004193C9 */
    e_eax = ((e_eax) + (0x45A258u)); /* 0x004193CC */
    e_eax = (unsigned)(d4v_41ae70(c, e_eax, e_edx)); /* 0x004193D1 */
    stk[0x44] = ((e_eax & 0xFFu)); /* 0x004193D6 */
    goto L_41931A; /* 0x004193DA */
L_4193DF: e_ecx = (0u); e_ecx = ((e_ecx & ~0xFFu) | ((e_edx & 0xFFu))); e_edx = (M32(0x45AF6Cu)); e_edx = ((e_edx) + (e_ecx)); M32(0x45AF70u) = (e_edx); e_edx = (0u); /* 0x004193DF,0x004193E1,0x004193E3,0x004193E9,0x004193EB,0x004193F1 */
    e_edi = (M32(0x45AF70u)); e_edx = ((e_edx & ~0xFFFFu) | ((e_ebx & 0xFFFFu))); /* cmp e_edx, e_edi (flags folded into jnb @0x004193FE below) */; /* 0x004193F3,0x004193F9,0x004193FC */
    if ((e_edx) >= (e_edi)) goto L_419438; /* jnb, unsigned=True */ /* 0x004193FE */
    e_eax = ((e_eax & ~0xFFu) | (M8(e_eax + 0x9u))); stk[0x24] = ((e_eax & 0xFFu)); e_eax = ((e_eax & ~0xFFu) | (0u)); stk[0x20] = ((e_eax & 0xFFu)); e_eax = (0u); /* 0x00419400,0x00419403,0x00419407,0x00419409,0x0041940D */
    e_ecx = ((e_ecx) << 2); e_eax = ((e_eax & ~0xFFu) | (stk[0x44])); /* cmp e_ecx, e_eax (flags folded into jb @0x00419418 below) */; /* 0x0041940F,0x00419412,0x00419416 */
    if ((e_ecx) < (e_eax)) goto L_419426; /* jb, unsigned=True */ /* 0x00419418 */
    /* cmp (e_eax & 0xFFu), 0x10u (flags folded into setnbe @0x0041941C below) */; e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) > (0x10u) )) & 0xFFu))); /* 0x0041941A,0x0041941C */
    e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) + 1u)) & 0xFFu))); M8(0x45C693u) = ((e_eax & 0xFFu)); /* 0x0041941F,0x00419421 */
L_419426: /* cmp M8(0x45C694u), 0x8u (flags folded into jz @0x0041942D below) */; /* 0x00419426 */
    if ((M8(0x45C694u)) == (0x8u)) goto L_419373; /* jz, unsigned=True */ /* 0x0041942D */
    goto L_41937D; /* 0x00419433 */
L_419438: e_edx = ((e_edx & ~0xFFu) | (M8(0x45C68Au))); e_ecx = (0u); M32(0x45AF6Cu) = (e_edi); e_ecx = ((e_ecx & ~0xFFu) | ((e_edx & 0xFFu))); /* 0x00419438,0x0041943E,0x00419440,0x00419446 */
    e_eax = ((e_eax & ~0xFFu) | (M8(e_eax + 0x9u))); e_edx = ((e_edx & ~0xFFu) | ((((((e_edx & 0xFFu)) + 1u)) & 0xFFu))); /* 0x00419448,0x0041944B */
    M8(0x45C65Eu + e_ecx) = ((e_eax & 0xFFu)); M8(0x45C68Au) = ((e_edx & 0xFFu)); /* 0x0041944D,0x00419453 */
L_419459: e_edx = ((e_edx & ~(0xFFu << 8)) | ((M8(0x45C694u)) << 8)); e_edx = ((e_edx & ~(0xFFu << 8)) | ((((((((e_edx >> 8) & 0xFFu)) + 1u)) & 0xFFu)) << 8)); /* 0x00419459,0x0041945F */
    M8(0x45C694u) = (((e_edx >> 8) & 0xFFu)); /* cmp ((e_edx >> 8) & 0xFFu), 0x8u (flags folded into jb @0x0041946A below) */; /* 0x00419461,0x00419467 */
    if ((((e_edx >> 8) & 0xFFu)) < (0x8u)) goto L_419351; /* jb, unsigned=True */ /* 0x0041946A */
    /* cmp M8(0x45C694u), 0x8u (flags folded into jz @0x00419477 below) */; /* 0x00419470 */
    if ((M8(0x45C694u)) == (0x8u)) goto L_419373; /* jz, unsigned=True */ /* 0x00419477 */
    goto L_41937D; /* 0x0041947D */
L_419482: e_eax = ((e_eax & ~0xFFu) | (stk[0x20])); M8(0x45C680u) = ((e_eax & 0xFFu)); /* 0x00419482,0x00419486 */
    /* test (e_eax & 0xFFu), (e_eax & 0xFFu) (flags folded into jz @0x0041948D below) */; /* 0x0041948B */
    if (((((((e_eax & 0xFFu)) & ((e_eax & 0xFFu)))) & 0xFFu) == 0u)) goto L_419612; /* jz, unsigned=True */ /* 0x0041948D */
    e_ecx = ((e_ecx & ~(0xFFu << 8)) | ((0x1u) << 8)); e_eax = ((e_eax & ~0xFFu) | (M8(0x45C67Fu))); stk[0x3C] = (((e_ecx >> 8) & 0xFFu)); /* 0x00419493,0x00419495,0x0041949A */
    e_ebx = ((e_ebx & ~0xFFu) | (((e_ecx >> 8) & 0xFFu))); /* test (e_eax & 0xFFu), (e_eax & 0xFFu) (flags folded into jnz @0x004194A2 below) */; /* 0x0041949E,0x004194A0 */
    if (((((((e_eax & 0xFFu)) & ((e_eax & 0xFFu)))) & 0xFFu) != 0u)) goto L_4194AA; /* jnz, unsigned=True */ /* 0x004194A2 */
    M8(0x45C680u) = (((e_ebx >> 8) & 0xFFu)); /* 0x004194A4 */
L_4194AA: /* cmp (stk[0x18] | ((unsigned)stk[0x19] << 8) | ((unsigned)stk[0x1A] << 16) | ((unsigned)stk[0x1B] << 24)), 0x0u (flags folded into jz @0x004194AF below) */; /* 0x004194AA */
    if (((stk[0x18] | ((unsigned)stk[0x19] << 8) | ((unsigned)stk[0x1A] << 16) | ((unsigned)stk[0x1B] << 24))) == (0x0u)) goto L_419612; /* jz, unsigned=True */ /* 0x004194AF */
    e_edx = (M32(0x45AFC0u)); e_ecx = (stk[0x18] | ((unsigned)stk[0x19] << 8) | ((unsigned)stk[0x1A] << 16) | ((unsigned)stk[0x1B] << 24)); e_edx = ((e_edx) >> 2); /* 0x004194B5,0x004194BB,0x004194BF */
    e_eax = (0u); M32(0x45AFC0u) = (e_edx); /* 0x004194C2,0x004194C4 */
L_4194CA: e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | (M8(0x45C68Au))); /* cmp e_eax, e_edx (flags folded into jnb @0x004194D4 below) */; /* 0x004194CA,0x004194CC,0x004194D2 */
    if ((e_eax) >= (e_edx)) goto L_4194FC; /* jnb, unsigned=True */ /* 0x004194D4 */
    e_edx = (e_eax); e_edx = ((e_edx & ~0xFFu) | (M8(0x45C65Eu + e_edx))); e_edx = ((e_edx & ~(0xFFu << 8)) | ((M8(e_ecx + 0x2u)) << 8)); e_eax = ((e_eax) + 1u); /* 0x004194D6,0x004194D8,0x004194DE,0x004194E1 */
    /* cmp (e_edx & 0xFFu), ((e_edx >> 8) & 0xFFu) (flags folded into jnz @0x004194E4 below) */; /* 0x004194E2 */
    if (((e_edx & 0xFFu)) != (((e_edx >> 8) & 0xFFu))) goto L_4194CA; /* jnz, unsigned=True */ /* 0x004194E4 */
    e_ebx = ((e_ebx & ~0xFFu) | (0u)); e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((M8(e_ecx + 0x1u)) << 8)); /* 0x004194E6,0x004194E8 */
    /* cmp ((e_ebx >> 8) & 0xFFu), 0x3u (flags folded into jbe @0x004194EE below) */; /* 0x004194EB */
    if ((((e_ebx >> 8) & 0xFFu)) <= (0x3u)) goto L_41962D; /* jbe, unsigned=True */ /* 0x004194EE */
L_4194F4: e_ecx = ((e_ecx & ~0xFFu) | (0u)); M8(0x45C680u) = ((e_ecx & 0xFFu)); /* 0x004194F4,0x004194F6 */
L_4194FC: /* test (e_ebx & 0xFFu), (e_ebx & 0xFFu) (flags folded into jz @0x004194FE below) */; /* 0x004194FC */
    if (((((((e_ebx & 0xFFu)) & ((e_ebx & 0xFFu)))) & 0xFFu) == 0u)) goto L_419612; /* jz, unsigned=True */ /* 0x004194FE */
    e_eax = ((e_eax & ~0xFFu) | (stk[0x28])); e_ecx = ((e_ecx & ~(0xFFu << 8)) | ((stk[0x1C]) << 8)); M8(0x45C685u) = ((e_eax & 0xFFu)); /* 0x00419504,0x00419508,0x0041950C */
    /* test ((e_ecx >> 8) & 0xFFu), ((e_ecx >> 8) & 0xFFu) (flags folded into jnz @0x00419513 below) */; /* 0x00419511 */
    if ((((((((e_ecx >> 8) & 0xFFu)) & (((e_ecx >> 8) & 0xFFu)))) & 0xFFu) != 0u)) goto L_419612; /* jnz, unsigned=True */ /* 0x00419513 */
    e_ecx = (M32(0x45BA4Cu)); /* test e_ecx, e_ecx (flags folded into jz @0x00419521 below) */; /* 0x00419519,0x0041951F */
    if ((((e_ecx) & (e_ecx)) == 0u)) goto L_419647; /* jz, unsigned=True */ /* 0x00419521 */
    e_ebx = ((e_ebx & ~0xFFu) | (M8(e_ecx))); e_eax = ((e_eax & ~0xFFu) | (M8(e_ecx + 0x1u))); /* 0x00419527,0x00419529 */
L_41952C: e_esi = (e_ebx & 0xFFu); e_eax = ((e_eax) & (0xFFu)); e_edx = (e_esi + e_eax); e_ecx = (e_edx); M32(0x45AF68u) = (e_edx); e_eax = (M32(0x45D0BCu)); e_edx = (0u); /* 0x0041952C,0x0041952F,0x00419534,0x00419537,0x00419539,0x0041953F,0x00419544 */
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) /* div faults on 0, as x86 does */;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) /* 0x00419546 */;
    e_edx = (0u); M32(0x45D0BCu) = (e_eax); e_ecx = (e_eax); e_eax = (M32(0x45D0B8u)); /* 0x00419548,0x0041954A,0x0041954F,0x00419551 */
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) /* div faults on 0, as x86 does */;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) /* 0x00419556 */;
    e_eax = ((e_eax) & (0xFFFFu)); /* cmp e_eax, e_esi (flags folded into jge @0x0041955F below) */; /* 0x00419558,0x0041955D */
    if (((int)(e_eax)) >= ((int)(e_esi))) goto L_419659; /* jge, unsigned=False */ /* 0x0041955F */
    /* cmp M8(0x45C684u), 0x0u (flags folded into jz @0x0041956C below) */; /* 0x00419565 */
    if ((M8(0x45C684u)) == (0x0u)) goto L_4195A1; /* jz, unsigned=True */ /* 0x0041956C */
    e_eax = (M32(0x45AFA0u)); e_edx = (M32(0x45AFD4u)); M32(e_eax) = (((M32(e_eax)) + (e_edx))); e_eax = (M32(0x45AF78u)); e_esi = (M32(e_eax)); /* 0x0041956E,0x00419573,0x00419579,0x0041957B,0x00419580 */
    e_esi = ((e_esi) + (e_edx)); e_edi = (e_edx + e_edx); M32(e_eax) = (e_esi); M32(0x45AFD4u) = (e_edi); /* 0x00419582,0x00419584,0x00419587,0x00419589 */
    /* cmp e_edi, 0xFFFFu (flags folded into jbe @0x00419595 below) */; /* 0x0041958F */
    if ((e_edi) <= (0xFFFFu)) goto L_4195A1; /* jbe, unsigned=True */ /* 0x00419595 */
    M32(0x45AFD4u) = (0xFFFFu); /* 0x00419597 */
L_4195A1: e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | ((e_ebx & 0xFFu))); e_ecx = (0u); M32(0x45AF70u) = (e_eax); /* 0x004195A1,0x004195A3,0x004195A5,0x004195A7 */
    e_eax = (stk[0x18] | ((unsigned)stk[0x19] << 8) | ((unsigned)stk[0x1A] << 16) | ((unsigned)stk[0x1B] << 24)); e_edx = ((e_edx & ~0xFFu) | (0u)); /* 0x004195AC,0x004195B0 */
    M32(0x45AF6Cu) = (e_ecx); e_eax = ((e_eax & ~0xFFu) | (M8(e_eax + 0x2u))); stk[0x20] = ((e_edx & 0xFFu)); stk[0x24] = ((e_eax & 0xFFu)); /* 0x004195B2,0x004195B8,0x004195BB,0x004195BF */
L_4195C3: e_eax = (0x45AF68u); e_edx = (M32(e_eax + 0x8u)); e_eax = (M32(e_eax + 0x4u)); e_ecx = (M32(0x45D0BCu)); e_edx = ((e_edx) - (e_eax)); /* 0x004195C3,0x004195C8,0x004195CB,0x004195CE,0x004195D4 */
    d4v_q = ((UHI64)(int)(e_eax)) * ((UHI64)(int)(e_ecx));
    e_eax = (unsigned)d4v_q /* imul flags (cf/of/zf/sf) dead here: next setter precedes any consumer */ /* 0x004195D6 */;
    d4v_q = ((UHI64)(int)(e_edx)) * ((UHI64)(int)(e_ecx));
    e_edx = (unsigned)d4v_q /* imul flags (cf/of/zf/sf) dead here: next setter precedes any consumer */ /* 0x004195D9 */;
    e_ecx = (M32(0x45D0B8u)); e_ecx = ((e_ecx) - (e_eax)); /* cmp e_edx, 0x1000000u (flags folded into jnb @0x004195EA below) */; /* 0x004195DC,0x004195E2,0x004195E4 */
    if ((e_edx) >= (0x1000000u)) goto L_419606; /* jnb, unsigned=True */ /* 0x004195EA */
L_4195EC: e_eax = (unsigned)(d4v_get_byte(c)); /* 0x004195EC */
    e_ecx = ((e_ecx) << 8); e_eax = ((e_eax) & (0xFFu)); e_edx = ((e_edx) << 8); e_ecx = ((e_ecx) | (e_eax)); /* 0x004195F1,0x004195F4,0x004195F9,0x004195FC */
    /* cmp e_edx, 0x1000000u (flags folded into jb @0x00419604 below) */; /* 0x004195FE */
    if ((e_edx) < (0x1000000u)) goto L_4195EC; /* jb, unsigned=True */ /* 0x00419604 */
L_419606: M32(0x45D0B8u) = (e_ecx); M32(0x45D0BCu) = (e_edx); /* 0x00419606,0x0041960C */
L_419612: /* cmp stk[0x20], 0x0u (flags folded into jz @0x00419617 below) */; /* 0x00419612 */
    if ((stk[0x20]) == (0x0u)) goto L_418CD1; /* jz, unsigned=True */ /* 0x00419617 */
    /* cmp stk[0x38], 0x0u (flags folded into jnz @0x00419622 below) */; /* 0x0041961D */
    if ((stk[0x38]) != (0x0u)) goto L_418AF1; /* jnz, unsigned=True */ /* 0x00419622 */
    goto L_418CD1; /* 0x00419628 */
L_41962D: e_edx = (0u); e_eax = (e_ecx); e_edx = ((e_edx & ~0xFFu) | (M8(e_ecx))); e_eax = ((e_eax) ^ (e_ecx)); e_edx = ((unsigned)((int)(e_edx) >> 6)); /* 0x0041962D,0x0041962F,0x00419631,0x00419633,0x00419635 */
    e_eax = ((e_eax & ~0xFFu) | (((e_ebx >> 8) & 0xFFu))); /* cmp e_edx, e_eax (flags folded into jg @0x0041963C below) */; /* 0x00419638,0x0041963A */
    if (((int)(e_edx)) > ((int)(e_eax))) goto L_4194F4; /* jg, unsigned=False */ /* 0x0041963C */
    goto L_4194FC; /* 0x00419642 */
L_419647: e_ebx = (stk[0x18] | ((unsigned)stk[0x19] << 8) | ((unsigned)stk[0x1A] << 16) | ((unsigned)stk[0x1B] << 24)); /* 0x00419647 */
    e_eax = (stk[0x18] | ((unsigned)stk[0x19] << 8) | ((unsigned)stk[0x1A] << 16) | ((unsigned)stk[0x1B] << 24)); e_ebx = ((e_ebx & ~0xFFu) | (M8(e_ebx))); /* 0x0041964B,0x0041964F */
    e_eax = ((e_eax & ~0xFFu) | (M8(e_eax + 0x1u))); /* 0x00419651 */
    goto L_41952C; /* 0x00419654 */
L_419659: /* cmp M8(0x45C684u), 0x0u (flags folded into jz @0x00419660 below) */; /* 0x00419659 */
    if ((M8(0x45C684u)) == (0x0u)) goto L_419681; /* jz, unsigned=True */ /* 0x00419660 */
    e_eax = (M32(0x45AFA0u)); M32(e_eax) = (((M32(e_eax)) >> 1)); e_eax = (M32(0x45AF78u)); M32(e_eax) = (((M32(e_eax)) >> 1)); e_eax = (M32(0x45AFD4u)); /* 0x00419662,0x00419667,0x00419669,0x0041966E,0x00419670 */
    e_eax = ((e_eax) >> 1); e_eax = ((e_eax) + (0xFFu)); M32(0x45AFD4u) = (e_eax); /* 0x00419675,0x00419677,0x0041967C */
L_419681: e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | ((e_ebx & 0xFFu))); M32(0x45AF6Cu) = (e_eax); e_eax = (M32(0x45AF68u)); e_edx = (0u); M32(0x45AF70u) = (e_eax); /* 0x00419681,0x00419683,0x00419685,0x0041968A,0x0041968F,0x00419691 */
    e_eax = ((e_eax & ~0xFFu) | (M8(0x45C68Au))); e_ecx = (stk[0x18] | ((unsigned)stk[0x19] << 8) | ((unsigned)stk[0x1A] << 16) | ((unsigned)stk[0x1B] << 24)); /* 0x00419696,0x0041969B */
    e_edx = ((e_edx & ~0xFFu) | ((e_eax & 0xFFu))); e_eax = ((e_eax & ~(0xFFu << 8)) | ((M8(e_ecx + 0x2u)) << 8)); /* 0x0041969F,0x004196A1 */
    M8(0x45C65Eu + e_edx) = (((e_eax >> 8) & 0xFFu)); e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) + 1u)) & 0xFFu))); M8(0x45C68Au) = ((e_eax & 0xFFu)); /* 0x004196A4,0x004196AA,0x004196AC */
    goto L_4195C3; /* 0x004196B1 */
L_4196B6: e_eax = (M32(0x45AFC4u)); e_ebx = ((e_ebx & ~0xFFu) | (M8(0x45AFC4u))); e_eax = ((e_eax) >> 8); e_esi = (e_ebx & 0xFFu); /* 0x004196B6,0x004196BB,0x004196C1,0x004196C4 */
    /* cmp (e_ebx & 0xFFu), (e_eax & 0xFFu) (flags folded into jnz @0x004196C9 below) */; /* 0x004196C7 */
    if (((e_ebx & 0xFFu)) != ((e_eax & 0xFFu))) goto L_4196CC; /* jnz, unsigned=True */ /* 0x004196C9 */
    e_ecx = ((e_ecx) + 1u); /* 0x004196CB */
L_4196CC: /* cmp M8(0x45C67Fu), 0x0u (flags folded into jnz @0x004196D3 below) */; /* 0x004196CC */
    if ((M8(0x45C67Fu)) != (0x0u)) goto L_418D5B; /* jnz, unsigned=True */ /* 0x004196D3 */
    e_eax = (M32(0x45AFC4u)); e_eax = ((e_eax) >> 8); e_eax = ((e_eax) & (0xFFu)); /* cmp M8(0x436B8Au + e_eax), 0x0u (flags folded into jz @0x004196ED below) */; /* 0x004196D9,0x004196DE,0x004196E1,0x004196E6 */
    if ((M8(0x436B8Au + e_eax)) == (0x0u)) goto L_418D5B; /* jz, unsigned=True */ /* 0x004196ED */
    e_esi = ((e_esi) + (0x100u)); /* 0x004196F3 */
    goto L_418D5B; /* 0x004196F9 */
L_4196FE: e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((0u) << 8)); /* 0x004196FE */
    goto L_418D95; /* 0x00419700 */
L_419705: e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | (M8(0x45C68Au))); /* cmp e_eax, e_edx (flags folded into jnb @0x0041970F below) */; /* 0x00419705,0x00419707,0x0041970D */
    if ((e_eax) >= (e_edx)) goto L_419724; /* jnb, unsigned=True */ /* 0x0041970F */
    e_edx = (e_eax); e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((M8(0x45C65Eu + e_edx)) << 8)); /* 0x00419711,0x00419713 */
    e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((((((((e_ebx >> 8) & 0xFFu)) - ((e_ebx & 0xFFu)))) & 0xFFu)) << 8)); e_eax = ((e_eax) + 1u); /* 0x00419719,0x0041971B */
    M8(0x45C65Eu + e_edx) = (((e_ebx >> 8) & 0xFFu)); /* 0x0041971C */
    goto L_419705; /* 0x00419722 */
L_419724: e_edx = (e_edi); e_eax = (e_ecx); e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((stk[0x2C]) << 8)); /* 0x00419724,0x00419726,0x00419728 */
    e_eax = ((e_eax & ~0xFFu) | ((unsigned)(d4v_419970(c, e_eax, e_edx, e_ebx)) & 0xFFu)); /* 0x0041972C */
    /* test ((e_ebx >> 8) & 0xFFu), ((e_ebx >> 8) & 0xFFu) (flags folded into jz @0x00419733 below) */; /* 0x00419731 */
    if ((((((((e_ebx >> 8) & 0xFFu)) & (((e_ebx >> 8) & 0xFFu)))) & 0xFFu) == 0u)) goto L_419766; /* jz, unsigned=True */ /* 0x00419733 */
    e_eax = ((e_eax & ~(0xFFu << 8)) | (((e_ebx & 0xFFu)) << 8)); /* 0x00419735 */
    e_eax = ((e_eax & ~(0xFFu << 8)) | ((((((((e_eax >> 8) & 0xFFu)) - ((e_eax & 0xFFu)))) & 0xFFu)) << 8)); stk[0x24] = (((e_eax >> 8) & 0xFFu)); /* 0x00419737,0x00419739 */
L_41973D: e_ecx = ((e_ecx & ~0xFFu) | (stk[0x2C])); e_eax = (0u); /* test (e_ecx & 0xFFu), (e_ecx & 0xFFu) (flags folded into jz @0x00419745 below) */; /* 0x0041973D,0x00419741,0x00419743 */
    if (((((((e_ecx & 0xFFu)) & ((e_ecx & 0xFFu)))) & 0xFFu) == 0u)) goto L_41976E; /* jz, unsigned=True */ /* 0x00419745 */
L_419747: e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | (M8(0x45C68Au))); /* cmp e_eax, e_edx (flags folded into jnb @0x00419751 below) */; /* 0x00419747,0x00419749,0x0041974F */
    if ((e_eax) >= (e_edx)) goto L_41978D; /* jnb, unsigned=True */ /* 0x00419751 */
    e_edx = ((e_edx & ~0xFFu) | ((e_ebx & 0xFFu))); e_edx = ((e_edx & ~(0xFFu << 8)) | ((M8(0x45C65Eu + e_eax)) << 8)); e_eax = ((e_eax) + 1u); /* 0x00419753,0x00419755,0x0041975B */
    e_edx = ((e_edx & ~0xFFu) | ((((((e_edx & 0xFFu)) - (((e_edx >> 8) & 0xFFu)))) & 0xFFu))); M8(0x45C65Du + e_eax) = ((e_edx & 0xFFu)); /* 0x0041975C,0x0041975E */
    goto L_419747; /* 0x00419764 */
L_419766: e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) + ((e_ebx & 0xFFu)))) & 0xFFu))); stk[0x24] = ((e_eax & 0xFFu)); /* 0x00419766,0x00419768 */
    goto L_41973D; /* 0x0041976C */
L_41976E: e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | (M8(0x45C68Au))); /* cmp e_eax, e_edx (flags folded into jnb @0x00419778 below) */; /* 0x0041976E,0x00419770,0x00419776 */
    if ((e_eax) >= (e_edx)) goto L_41978D; /* jnb, unsigned=True */ /* 0x00419778 */
    e_edx = (e_eax); e_ecx = ((e_ecx & ~(0xFFu << 8)) | ((M8(0x45C65Eu + e_edx)) << 8)); /* 0x0041977A,0x0041977C */
    e_ecx = ((e_ecx & ~(0xFFu << 8)) | ((((((((e_ecx >> 8) & 0xFFu)) + ((e_ebx & 0xFFu)))) & 0xFFu)) << 8)); e_eax = ((e_eax) + 1u); /* 0x00419782,0x00419784 */
    M8(0x45C65Eu + e_edx) = (((e_ecx >> 8) & 0xFFu)); /* 0x00419785 */
    goto L_41976E; /* 0x0041978B */
L_41978D: e_edx = (0x2FFu); e_ebx = (0u); e_eax = (e_esi); e_ebx = ((e_ebx & ~0xFFu) | (stk[0x24])); /* 0x0041978D,0x00419792,0x00419794,0x00419796 */
L_41979A: d4v_41ad60(c, e_eax, e_edx, e_ebx); /* 0x0041979A */
L_41979F: e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | (stk[0x24])); e_eax = (e_edx); /* 0x0041979F,0x004197A1,0x004197A5 */
    d4v_417220(c, e_eax); /* 0x004197A7 */
    /* cmp M8(0x45C68Bu), 0x0u (flags folded into jz @0x004197B3 below) */; /* 0x004197AC */
    if ((M8(0x45C68Bu)) == (0x0u)) goto L_4197BC; /* jz, unsigned=True */ /* 0x004197B3 */
    e_eax = (e_edx); /* 0x004197B5 */
    d4v_417570(c, e_eax); /* 0x004197B7 */
L_4197BC: /* cmp M8(0x45C680u), 0x0u (flags folded into jz @0x004197C3 below) */; /* 0x004197BC */
    if ((M8(0x45C680u)) == (0x0u)) goto L_4197D0; /* jz, unsigned=True */ /* 0x004197C3 */
    e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | (stk[0x24])); /* 0x004197C5,0x004197C7 */
    d4v_417090(c, e_eax); /* 0x004197CB */
L_4197D0: /* cmp stk[0x28], 0x0u (flags folded into jnz @0x004197D5 below) */; /* 0x004197D0 */
    if ((stk[0x28]) != (0x0u)) goto L_419898; /* jnz, unsigned=True */ /* 0x004197D5 */
L_4197DB: e_ebx = ((e_ebx & ~0xFFu) | (M8(0x45C690u))); e_ebx = ((e_ebx & ~0xFFu) | ((((((e_ebx & 0xFFu)) - 1u)) & 0xFFu))); M8(0x45C690u) = ((e_ebx & 0xFFu)); /* 0x004197DB,0x004197E1,0x004197E3 */
    /* cmp (e_ebx & 0xFFu), 0xFFu (flags folded into jz @0x004197EC below) */; /* 0x004197E9 */
    if (((e_ebx & 0xFFu)) == (0xFFu)) goto L_419909; /* jz, unsigned=True */ /* 0x004197EC */
    e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | ((e_ebx & 0xFFu))); e_ecx = (M32(0x458778u + e_eax * 4)); e_edx = (M32(0x458788u + e_eax * 4)); /* 0x004197F2,0x004197F4,0x004197F6,0x004197FD */
    M32(0x45AFA4u) = (e_ecx); e_eax = ((e_eax & ~0xFFu) | (M8(0x45AF50u + e_eax))); e_eax = ((e_eax) & (0xFFu)); /* 0x00419804,0x0041980A,0x00419810 */
    d4v_416b50(c, e_eax, e_edx); /* 0x00419815 (ecx dead: written before read) */
    goto L_4197DB; /* 0x0041981A */
L_41981C: e_edx = (0x2FFu); e_eax = (e_esi); /* 0x0041981C,0x00419821 */
    e_eax = ((e_eax & ~0xFFu) | ((unsigned)(d4v_419970(c, e_eax, e_edx, e_ebx)) & 0xFFu)); /* 0x00419823 */
    stk[0x24] = ((e_eax & 0xFFu)); /* cmp M8(0x45C67Fu), 0x0u (flags folded into jz @0x00419833 below) */; /* 0x00419828,0x0041982C */
    if ((M8(0x45C67Fu)) == (0x0u)) goto L_41979F; /* jz, unsigned=True */ /* 0x00419833 */
    /* test ((e_ebx >> 8) & 0xFFu), ((e_ebx >> 8) & 0xFFu) (flags folded into jz @0x0041983B below) */; /* 0x00419839 */
    if ((((((((e_ebx >> 8) & 0xFFu)) & (((e_ebx >> 8) & 0xFFu)))) & 0xFFu) == 0u)) goto L_41986A; /* jz, unsigned=True */ /* 0x0041983B */
    e_ebx = ((e_ebx & ~(0xFFu << 8)) | (((e_ebx & 0xFFu)) << 8)); /* 0x0041983D */
    e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((((((((e_ebx >> 8) & 0xFFu)) - ((e_eax & 0xFFu)))) & 0xFFu)) << 8)); /* 0x0041983F */
L_419841: e_edx = ((e_edx & ~0xFFu) | (stk[0x2C])); e_eax = (0u); /* test (e_edx & 0xFFu), (e_edx & 0xFFu) (flags folded into jz @0x00419849 below) */; /* 0x00419841,0x00419845,0x00419847 */
    if (((((((e_edx & 0xFFu)) & ((e_edx & 0xFFu)))) & 0xFFu) == 0u)) goto L_419870; /* jz, unsigned=True */ /* 0x00419849 */
L_41984B: e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | (M8(0x45C68Au))); /* cmp e_eax, e_edx (flags folded into jnb @0x00419855 below) */; /* 0x0041984B,0x0041984D,0x00419853 */
    if ((e_eax) >= (e_edx)) goto L_419887; /* jnb, unsigned=True */ /* 0x00419855 */
    e_edx = ((e_edx & ~0xFFu) | ((e_ebx & 0xFFu))); e_edx = ((e_edx & ~(0xFFu << 8)) | ((M8(0x45C65Eu + e_eax)) << 8)); e_eax = ((e_eax) + 1u); /* 0x00419857,0x00419859,0x0041985F */
    e_edx = ((e_edx & ~0xFFu) | ((((((e_edx & 0xFFu)) - (((e_edx >> 8) & 0xFFu)))) & 0xFFu))); M8(0x45C65Du + e_eax) = ((e_edx & 0xFFu)); /* 0x00419860,0x00419862 */
    goto L_41984B; /* 0x00419868 */
L_41986A: e_ebx = ((e_ebx & ~(0xFFu << 8)) | (((e_eax & 0xFFu)) << 8)); /* 0x0041986A */
    e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((((((((e_ebx >> 8) & 0xFFu)) - ((e_ebx & 0xFFu)))) & 0xFFu)) << 8)); /* 0x0041986C */
    goto L_419841; /* 0x0041986E */
L_419870: e_edx = (0u); e_edx = ((e_edx & ~0xFFu) | (M8(0x45C68Au))); /* cmp e_eax, e_edx (flags folded into jnb @0x0041987A below) */; /* 0x00419870,0x00419872,0x00419878 */
    if ((e_eax) >= (e_edx)) goto L_419887; /* jnb, unsigned=True */ /* 0x0041987A */
    e_edx = (e_eax); e_eax = ((e_eax) + 1u); M8(0x45C65Eu + e_edx) = (((((M8(0x45C65Eu + e_edx)) - ((e_ebx & 0xFFu)))) & 0xFFu)); /* 0x0041987C,0x0041987E,0x0041987F */
    goto L_419870; /* 0x00419885 */
L_419887: e_ebx = ((e_ebx & ~0xFFu) | (((e_ebx >> 8) & 0xFFu))); e_ebx = ((e_ebx) & (0xFFu)); e_edx = (e_edi); e_eax = (e_ecx); /* 0x00419887,0x00419889,0x0041988F,0x00419891 */
    goto L_41979A; /* 0x00419893 */
L_419898: e_eax = ((e_eax & ~0xFFu) | (stk[0x24])); e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) - (M8(0x45C679u)))) & 0xFFu))); /* 0x00419898,0x0041989C */
    /* cmp M8(0x45C67Bu), 0x0u (flags folded into jz @0x004198A9 below) */; /* 0x004198A2 */
    if ((M8(0x45C67Bu)) == (0x0u)) goto L_4198AD; /* jz, unsigned=True */ /* 0x004198A9 */
    e_eax = ((e_eax & ~0xFFu) | ((((0u - ((e_eax & 0xFFu)))) & 0xFFu))); /* 0x004198AB */
L_4198AD: e_edx = ((e_edx & ~0xFFu) | (0u)); e_ebx = (0u); M8(0x45C68Au) = ((e_edx & 0xFFu)); e_edx = (0u); e_ebx = ((e_ebx & ~0xFFu) | ((e_eax & 0xFFu))); /* 0x004198AD,0x004198AF,0x004198B1,0x004198B7,0x004198B9 */
    e_edx = ((e_edx & ~0xFFu) | (M8(0x45C67Du))); e_eax = (M32(0x45AF3Cu)); e_edx = ((e_edx) + (0x302u)); /* 0x004198BB,0x004198C1,0x004198C6 */
    d4v_41ae30(c, e_eax, e_edx, e_ebx); /* 0x004198CC */
    /* cmp M8(0x45C68Du), 0x0u (flags folded into jz @0x004198D8 below) */; /* 0x004198D1 */
    if ((M8(0x45C68Du)) == (0x0u)) goto L_4198E5; /* jz, unsigned=True */ /* 0x004198D8 */
    e_eax = (M32(0x45AF8Cu)); M32(0x45AF88u) = (((M32(0x45AF88u)) + (e_eax))); /* 0x004198DA,0x004198DF */
L_4198E5: e_edx = (M32(0x45AF84u)); e_eax = (e_edx); e_eax = ((e_eax) << 5); e_eax = ((e_eax) - (e_edx)); e_edx = (e_eax); e_eax = (M32(0x45AF88u)); /* 0x004198E5,0x004198EB,0x004198ED,0x004198F0,0x004198F2,0x004198F4 */
    e_edx = ((e_edx) >> 5); e_edx = ((e_edx) + (e_eax)); M32(0x45AF84u) = (e_edx); /* 0x004198F9,0x004198FC,0x004198FE */
    goto L_4197DB; /* 0x00419904 */
L_419909: e_ecx = (0u); e_ecx = ((e_ecx & ~0xFFu) | (stk[0x24])); e_eax = (e_ecx); e_edx = (0x1u); /* 0x00419909,0x0041990B,0x0041990F,0x00419911 */
    d4v_4183d0(c, e_eax, e_edx); /* 0x00419916 */
    e_eax = (e_ecx); /* 0x0041991B */
    d4v_4177a0(c, e_eax, e_edx); /* 0x0041991D */
    e_edx = (M32(0x433B40u)); e_eax = (M32(0x45BA34u)); M8(e_edx + e_eax) = ((e_ecx & 0xFFu)); /* cmp e_eax, 0x100u (flags folded into jb @0x00419935 below) */; /* 0x00419922,0x00419928,0x0041992D,0x00419930 */
    if ((e_eax) < (0x100u)) goto L_419955; /* jb, unsigned=True */ /* 0x00419935 */
L_419937: e_edx = (M32(0x45BA34u)); e_eax = (M32(0x45BA3Cu)); e_edx = ((e_edx) + 1u); e_edx = ((e_edx) & (e_eax)); M32(0x45BA34u) = (e_edx); /* 0x00419937,0x0041993D,0x00419942,0x00419943,0x00419945 */
    return; /* retn + epilogue */ /* 0x00419954,0x0041994B,0x0041994E,0x0041994F,0x00419950,0x00419951,0x00419952,0x00419953 */
L_419955: e_eax = ((e_eax) + (M32(0x45BA30u))); e_eax = ((e_eax) + (e_edx)); M8(e_eax) = ((e_ecx & 0xFFu)); /* 0x00419955,0x0041995B,0x0041995D */
    goto L_419937; /* 0x0041995F */
}

/* ---- workers: range-splitter cluster (from draft_s1.c) ---- */
unsigned int d4v_419970(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in);
unsigned int d4v_419ca0(dec4v_ctx *c, unsigned int eax_in);
unsigned int d4v_41a360(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in);
void d4v_41ad60(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in);
void d4v_41ae30(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in);
unsigned int d4v_41ae70(dec4v_ctx *c, unsigned int tab, unsigned int edx_in);
void d4v_41aeb0(dec4v_ctx *c, unsigned int tab, unsigned int flag_in);

/* sub_419970: range/PPM symbol splitter.
 * Role hypothesis: consumes one range-coded decision (45AF64 cursor minus
 * 458F98[45AF80] history), picks one of three bands: out-of-range
 * (<0x6D4250 or >=0xFEFF01) -> estimator only; mid (<0x91ADC0) -> estimator
 * + freq-divide with 459398/459BA0 adaptation; high -> direct freq-divide.
 * Logs (45AFA4/symbol/bl) into the 458778/458788/45AF50 ring at 45C690.
 * Entry: eax, edx (ebx dummy, dead). Frame: 8 (var_1C dword, var_18 byte).
 * Returns al (=bl, decoded symbol). Calls: sub_4220B0, sub_41A360,
 * sub_419CA0.
 */
unsigned int d4v_419970(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int ebp = 0;
    unsigned int var1C = 0;
    unsigned char var18 = 0;
    (void)ebx_in;
    ebx = 0;
    var1C = 0;
    var18 = 0;
    /* push ebx/ECX/esi/edi/ebp; sub esp,8 (frame only) */
    esi = eax_in; /* 0x00419978 */
    ecx = edx_in; /* 0x0041997A */
    edi = G32(0x45AF64); /* 0x0041997C */
    if (edx_in == 0x2FFU) eax = (eax_in & 0xFFFFFF00UL) | 1UL; else eax = (eax_in & 0xFFFFFF00UL); /* 0x00419982 0x00419988 */
    G32(0x45AFA4) = ecx; /* 0x0041998B */
    edx = G32(0x45AF80); /* 0x00419991 */
    ebp = 0U; /* 0x00419997 */
    edx = edx + 1UL; /* 0x00419999 */
    ebx = (ebx & 0xFFFF00FFUL) | ((eax & 0xFFUL) << 8); /* 0x0041999A */
    edx = edx & 0xFFUL; /* 0x0041999C */
    G32(0x45AF74) = ebp; /* 0x004199A2 */
    G32(0x45AF80) = edx; /* 0x004199A8 */
    edx = M32(0x458F98UL + edx * 4UL); /* 0x004199AE */
    eax = (eax & 0xFFFF00FFUL) | 0x100UL; /* 0x004199B5 */
    edi = edi - edx; /* 0x004199B7 */
    var18 = (unsigned char)((eax >> 8) & 0xFFUL); /* 0x004199B9 */
    G32(0x45AF64) = edi; /* 0x004199BD */
    if (edi < 0x6D4250UL) goto L_419C2E; /* 0x004199C3 0x004199C9 */
    if (edi >= 0xFEFF01UL) goto L_419C2E; /* 0x004199CF 0x004199D5 */
    if (edi < 0x91ADC0UL) goto L_419AF4; /* 0x004199DB 0x004199E1 */
    ecx = 0x100UL; /* 0x004199E7 */
    G32(0x45AF68) = ecx; /* 0x004199EC */
    eax = G32(0x45D0BC); /* 0x004199F2 */
    edx = 0U; /* 0x004199F7 */
    eax = eax / ecx; /* 0x004199F9 div ecx, edx=0; /0 faults like x86 */
    edx = 0U; /* 0x004199FB */
    G32(0x45D0BC) = eax; /* 0x004199FD */
    ecx = eax; /* 0x00419A02 */
    eax = G32(0x45D0B8); /* 0x00419A04 */
    eax = eax / ecx; /* 0x00419A09 div ecx, edx=0; /0 faults like x86; rem discarded */
    edx = eax; /* 0x00419A0D */
    ebx = (ebx & 0xFFFFFF00UL) | (eax & 0xFFUL); /* 0x00419A0D */
    eax = 0U; /* 0x00419A0F */
    eax = (eax & 0xFFFFFF00UL) | (edx & 0xFFUL); /* 0x00419A11 */
    G32(0x45AF6C) = eax; /* 0x00419A13 */
    eax = 0U; /* 0x00419A18 */
    eax = (eax & 0xFFFFFF00UL) | (edx & 0xFFUL); /* 0x00419A1A */
    eax = eax + 1UL; /* 0x00419A1C */
    G32(0x45AF70) = eax; /* 0x00419A1D */
    eax = 0x45AF68UL; /* 0x00419A22 */
    edx = M32(eax + 8UL); /* 0x00419A27 */
    eax = M32(eax + 4UL); /* 0x00419A2A */
    ecx = G32(0x45D0BC); /* 0x00419A2D */
    edx = edx - eax; /* 0x00419A33 */
    eax = eax * ecx; /* 0x00419A35 */
    edx = edx * ecx; /* 0x00419A38 */
    ecx = G32(0x45D0B8); /* 0x00419A3B */
    ecx = ecx - eax; /* 0x00419A41 */
    if (edx >= 0x1000000UL) goto L_419A65; /* 0x00419A43 0x00419A49 */
L_419A4B:
    eax = (unsigned int)d4v_get_byte(c); /* 0x00419A4B */
    ecx = ecx << 8; /* 0x00419A50 */
    eax = eax & 0xFFUL; /* 0x00419A53 */
    edx = edx << 8; /* 0x00419A58 */
    ecx = ecx | eax; /* 0x00419A5B */
    if (edx < 0x1000000UL) goto L_419A4B; /* 0x00419A5D 0x00419A63 */
L_419A65:
    G32(0x45D0B8) = ecx; /* 0x00419A65 */
    G32(0x45D0BC) = edx; /* 0x00419A6B */
    if ((G8(0x45C68D) & 0xFFU) == 0U) goto L_419A84; /* 0x00419A71 0x00419A78 */
    G32(0x45AF8C) = G32(0x45AF8C) + 0x9240UL; /* 0x00419A7A */
L_419A84:
    edx = 0U; /* 0x00419A84 */
    eax = esi; /* 0x00419A86 */
    edx = (edx & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x00419A88 */
    (void)d4v_41a360(c, eax, edx); /* 0x00419A8A */
L_419A8F:
    eax = 0U; /* 0x00419A8F */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8(0x45C690); /* 0x00419A91 */
    edx = G32(0x45AFA4); /* 0x00419A96 */
    ecx = (ecx & 0xFFFF00FFUL) | ((eax & 0xFFUL) << 8); /* 0x00419A9C */
    M32(0x458778UL + (eax & 0xFFUL) * 4UL) = edx; /* 0x00419A9E */
    M32(0x458788UL + (eax & 0xFFUL) * 4UL) = esi; /* 0x00419AA5 */
    M8(0x45AF50UL + (eax & 0xFFUL)) = (unsigned char)(ebx & 0xFFUL); /* 0x00419AAC */
    ecx = (ecx & 0xFFFF00FFUL) | (((((ecx >> 8) & 0xFFUL) + 1UL) & 0xFFUL) << 8); /* 0x00419AB2 */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)var18; /* 0x00419AB4 */
    G8(0x45C690) = (unsigned char)((ecx >> 8) & 0xFFUL); /* 0x00419AB8 */
    if ((eax & 0xFFUL) != 0U) goto L_419C55; /* 0x00419ABE 0x00419AC0 */
L_419AC6:
    esi = (ebx >> 8) & 0xFFUL; /* 0x00419AC6 */
    ecx = M32(0x459BA0UL + esi * 4UL); /* 0x00419AC9 */
    eax = ecx; /* 0x00419AD0 */
    eax = eax << 5; /* 0x00419AD2 */
    eax = eax - ecx; /* 0x00419AD5 */
    ecx = G32(0x45AF74); /* 0x00419AD7 */
    eax = eax >> 5; /* 0x00419ADD */
    eax = eax + ecx; /* 0x00419AE0 */
    M32(0x459BA0UL + esi * 4UL) = eax; /* 0x00419AE2 */
    eax = (eax & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x00419AE9 */
    /* add esp,8; pop ebp/edi/esi/ecx/ebx (frame only) */
    return (unsigned char)(eax & 0xFFUL); /* 0x00419AEB 0x00419AF3 */
L_419AF4:
    edx = 0U; /* 0x00419AF4 */
    edx = (edx & 0xFFFFFF00UL) | (eax & 0xFFUL); /* 0x00419AF6 */
    edi = M32(0x459398UL + (edx & 0xFFUL) * 4UL); /* 0x00419AF8 */
    if (edi < M32(0x459BA0UL + (edx & 0xFFUL) * 4UL)) goto L_419BDE; /* 0x00419AFF 0x00419B06 */
    eax = esi; /* 0x00419B0C */
    eax = (unsigned int)d4v_419ca0(c, eax); /* 0x00419B0E */
    ebx = (ebx & 0xFFFFFF00UL) | (eax & 0xFFUL); /* 0x00419B13 */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8(0x45C68D); /* 0x00419B15 */
    ebp = G32(0x45AF74); /* 0x00419B1A */
    if ((eax & 0xFFUL) == 0U) goto L_419B2A; /* 0x00419B20 0x00419B22 */
    G32(0x45AF8C) = G32(0x45AF8C) + ebp; /* 0x00419B24 */
L_419B2A:
    eax = 0U; /* 0x00419B2A */
    G32(0x45AF74) = eax; /* 0x00419B2C */
    eax = (eax & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL); /* 0x00419B31 */
    edx = 0U; /* 0x00419B33 */
    eax = eax + 0x300UL; /* 0x00419B35 */
    edx = (edx & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x00419B3A */
    G32(0x45AFA4) = eax; /* 0x00419B3C */
    eax = 0x200UL; /* 0x00419B41 */
    (void)d4v_41a360(c, eax, edx); /* 0x00419B46 */
    edi = G32(0x45AF74); /* 0x00419B4B */
L_419B51:
    eax = 0U; /* 0x00419B51 */
    eax = (eax & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL); /* 0x00419B53 */
    var1C = eax; /* 0x00419B55 */
    eax = eax + 0x300UL; /* 0x00419B58 */
    G32(0x45AFA4) = eax; /* 0x00419B5D */
    eax = 0U; /* 0x00419B62 */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8(0x45C690); /* 0x00419B64 */
    edx = G32(0x45AFA4); /* 0x00419B69 */
    M32(0x458778UL + (eax & 0xFFUL) * 4UL) = edx; /* 0x00419B6F */
    M32(0x458788UL + (eax & 0xFFUL) * 4UL) = 0x200UL; /* 0x00419B76 */
    G8(0x45C690) = (unsigned char)(G8(0x45C690) + 1U); /* 0x00419B81 */
    edx = G32(0x45AF64); /* 0x00419B87 */
    edx = edx + edi; /* 0x00419B8D */
    M8(0x45AF50UL + (eax & 0xFFUL)) = (unsigned char)(ebx & 0xFFUL); /* 0x00419B8F */
    G32(0x45AF64) = edx; /* 0x00419B95 */
    edx = var1C; /* 0x00419B9B */
    eax = G32(0x45AF80); /* 0x00419B9E */
    edx = edx << 2; /* 0x00419BA3 */
    M32(0x458F98UL + (eax & 0xFFUL) * 4UL) = edi; /* 0x00419BA6 */
    eax = M32(0x459398UL + edx); /* 0x00419BAD */
    var1C = eax; /* 0x00419BB3 */
    eax = eax << 5; /* 0x00419BB6 */
    eax = eax - var1C; /* 0x00419BB9 */
    eax = eax >> 5; /* 0x00419BBC */
    eax = eax + edi; /* 0x00419BBF */
    G32(0x45AF74) = ebp; /* 0x00419BC1 */
    M32(0x459398UL + edx) = eax; /* 0x00419BC7 */
    edx = edx & 0xFFFF00FFUL; /* 0x00419BCD */
    G32(0x45AFA4) = ecx; /* 0x00419BCF */
    var18 = (unsigned char)((edx >> 8) & 0xFFUL); /* 0x00419BD5 */
    goto L_419A8F; /* 0x00419BD9 */
L_419BDE:
    edx = edx + 0x300UL; /* 0x00419BDE */
    eax = 0x200UL; /* 0x00419BE4 */
    G32(0x45AFA4) = edx; /* 0x00419BE9 */
    eax = (unsigned int)d4v_419ca0(c, eax); /* 0x00419BEF */
    edi = G32(0x45AF74); /* 0x00419BF4 */
    edx = (edx & 0xFFFF00FFUL) | ((unsigned int)G8(0x45C68D) << 8); /* 0x00419BFA */
    ebx = (ebx & 0xFFFFFF00UL) | (eax & 0xFFUL); /* 0x00419C00 */
    if (((edx >> 8) & 0xFFUL) == 0U) goto L_419C0C; /* 0x00419C02 0x00419C04 */
    G32(0x45AF8C) = G32(0x45AF8C) + edi; /* 0x00419C06 */
L_419C0C:
    edx = 0U; /* 0x00419C0C */
    eax = esi; /* 0x00419C0E */
    G32(0x45AF74) = edx; /* 0x00419C10 */
    edx = (edx & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x00419C16 */
    G32(0x45AFA4) = ecx; /* 0x00419C18 */
    (void)d4v_41a360(c, eax, edx); /* 0x00419C1E */
    ebp = G32(0x45AF74); /* 0x00419C23 */
    goto L_419B51; /* 0x00419C29 */
L_419C2E:
    eax = esi; /* 0x00419C2E */
    eax = (unsigned int)d4v_419ca0(c, eax); /* 0x00419C30 */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C68D); /* 0x00419C35 */
    ebx = (ebx & 0xFFFFFF00UL) | (eax & 0xFFUL); /* 0x00419C3B */
    if ((edx & 0xFFUL) == 0U) goto L_419A8F; /* 0x00419C3D 0x00419C3F */
    eax = G32(0x45AF74); /* 0x00419C45 */
    G32(0x45AF8C) = G32(0x45AF8C) + eax; /* 0x00419C4A */
    goto L_419A8F; /* 0x00419C50 */
L_419C55:
    eax = G32(0x45AF80); /* 0x00419C55 */
    esi = (ebx >> 8) & 0xFFUL; /* 0x00419C5A */
    edx = G32(0x45AF74); /* 0x00419C5D */
    ecx = M32(0x459398UL + esi * 4UL); /* 0x00419C63 */
    M32(0x458F98UL + (eax & 0xFFUL) * 4UL) = edx; /* 0x00419C6A */
    eax = ecx; /* 0x00419C71 */
    eax = eax << 5; /* 0x00419C73 */
    eax = eax - ecx; /* 0x00419C76 */
    ebp = G32(0x45AF64); /* 0x00419C78 */
    eax = eax >> 5; /* 0x00419C7E */
    ebp = ebp + edx; /* 0x00419C81 */
    eax = eax + edx; /* 0x00419C83 */
    G32(0x45AF64) = ebp; /* 0x00419C85 */
    M32(0x459398UL + esi * 4UL) = eax; /* 0x00419C8B */
    goto L_419AC6; /* 0x00419C92 */
}

/* sub_419CA0: PPM probability estimator.
 * Role hypothesis: sums the word-frequency table at [433B7C] in
 * byte_45C65E order against the totals at [433B78]; range-decodes
 * (45AF68/6C/70 window, 45D0B8/BC coder state, 0x437968 price table);
 * refines with a bh=0x80 bit-plane search over per-symbol slots, then a
 * second estimator pass; returns the decoded symbol in al. The bl==0
 * path (0x41A2F2) is a halving binary search with al=0x80.
 * Entry: eax (context/symbol index). Frame: 0xAC.
 * Returns al. Calls: sub_4220B0.
 */
unsigned int d4v_419ca0(dec4v_ctx *c, unsigned int eax_in)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int ebp = 0;
    unsigned int v30 = 0;
    unsigned int v28 = 0;
    unsigned int v2c = 0;
    unsigned int v34 = 0;
    unsigned int v38 = 0;
    unsigned int v24 = 0;
    unsigned char v20 = 0;
    unsigned char v1C = 0;
    unsigned int c4[256] = {0};
    unsigned short cw[257] = {0};
    unsigned int i = 0;
    for (i = 0; i < 256; i++) c4[i] = 0U;
    for (i = 0; i < 257; i++) cw[i] = 0U;
    ebx = 0U;
    ecx = 0U;
    edx = 0U;
    esi = 0U;
    edi = 0U;
    ebp = 0U;
    v30 = 0U;
    v28 = 0U;
    v2c = 0U;
    v34 = 0U;
    v38 = 0U;
    v24 = 0U;
    v20 = 0;
    v1C = 0;
    /* push ebx/ecx/edx/esi/edi/ebp; sub esp,0ACh (frame only) */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C68A); /* 0x00419CAC */
    v30 = eax_in; /* 0x00419CB2 */
    esi = eax_in; /* 0x00419CB9 */
    eax = M32(0x433B78UL); /* 0x00419CBB */
    esi = esi << 9; /* 0x00419CC0 */
    esi = esi + eax; /* 0x00419CC3 */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(esi); /* 0x00419CC5 */
    edi = v30; /* 0x00419CC8 */
    v28 = eax; /* 0x00419CCF */
    eax = eax & 0xFFFF00FFUL; /* 0x00419CD6 */
    edi = (edi << 8) & U32MASK; /* 0x00419CD8 shl edi,8 */
    v1C = (unsigned char)((eax >> 8) & 0xFFUL); /* 0x00419CDB */
    eax = 0U; /* 0x00419CE2 */
L_419CE4:
    edx = 0U; /* 0x00419CE4 */
    edx = (edx & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x00419CE6 */
    if (eax >= (edx & 0xFFUL)) goto L_419D16; /* 0x00419CE8 0x00419CEA */
    edx = 0U; /* 0x00419CEC */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(0x45C65EUL + eax); /* 0x00419CEE */
    edx = edx + edi; /* 0x00419CF4 */
    ecx = edx + edx; /* 0x00419CF6 */
    edx = M32(0x433B7CUL); /* 0x00419CF9 */
    edx = (edx & 0xFFFF0000UL) | (unsigned int)M16(ecx + edx); /* 0x00419CFF */
    ecx = v28; /* 0x00419D03 */
    ecx = ecx - edx; /* 0x00419D0A */
    eax = eax + 1UL; /* 0x00419D0C */
    v28 = ecx; /* 0x00419D0D */
    goto L_419CE4; /* 0x00419D14 */
L_419D16:
    edx = v28; /* 0x00419D16 */
    G8(0x45C68A) = (unsigned char)(ebx & 0xFFUL); /* 0x00419D1D */
    if ((edx & 0xFFFFUL) != 0U) goto L_419E35; /* 0x00419D23 0x00419D26 */
    eax = G32(0x45AFA4); /* 0x00419D2C */
    v30 = eax; /* 0x00419D31 */
    esi = eax; /* 0x00419D38 */
    eax = M32(0x433B78UL); /* 0x00419D3A */
    esi = esi << 9; /* 0x00419D3F */
    esi = esi + eax; /* 0x00419D42 */
    edi = v30; /* 0x00419D44 */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(esi); /* 0x00419D4B */
    edi = edi << 8; /* 0x00419D4E */
    v28 = eax; /* 0x00419D51 */
    eax = 0U; /* 0x00419D58 */
L_419D5A:
    edx = 0U; /* 0x00419D5A */
    edx = (edx & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x00419D5C */
    G8(0x45C68A) = (unsigned char)(ebx & 0xFFUL); /* 0x00419D5E */
    if (eax >= (edx & 0xFFUL)) goto L_419D92; /* 0x00419D64 0x00419D66 */
    edx = 0U; /* 0x00419D68 */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(0x45C65EUL + eax); /* 0x00419D6A */
    edx = edx + edi; /* 0x00419D70 */
    ecx = edx + edx; /* 0x00419D72 */
    edx = M32(0x433B7CUL); /* 0x00419D75 */
    edx = (edx & 0xFFFF0000UL) | (unsigned int)M16(ecx + edx); /* 0x00419D7B */
    ecx = v28; /* 0x00419D7F */
    ecx = ecx - edx; /* 0x00419D86 */
    eax = eax + 1UL; /* 0x00419D88 */
    v28 = ecx; /* 0x00419D89 */
    goto L_419D5A; /* 0x00419D90 */
L_419D92:
    eax = 0U; /* 0x00419D92 */
    eax = (eax & 0xFFFF0000UL) | (v28 & 0xFFFFUL); /* 0x00419D94 */
    ecx = eax; /* 0x00419D9C */
    G32(0x45AF68) = eax; /* 0x00419D9E */
    eax = G32(0x45D0BC); /* 0x00419DA3 */
    edx = 0U; /* 0x00419DA8 */
    eax = eax / ecx; /* 0x00419DAA div ecx, edx=0; /0 faults like x86 */
    edx = 0U; /* 0x00419DAC */
    G32(0x45D0BC) = eax; /* 0x00419DAE */
    ecx = eax; /* 0x00419DB3 */
    eax = G32(0x45D0B8); /* 0x00419DB5 */
    eax = eax / ecx; /* 0x00419DBA div ecx, edx=0; /0 faults like x86 */
    v2c = eax; /* 0x00419DBC */
L_419DC3:
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C68A); /* 0x00419DC3 */
    if ((ebx & 0xFFUL) == 0U) goto L_41A2F2; /* 0x00419DC9 0x00419DCB */
    ebx = (ebx & 0xFFFF00FFUL) | 0x8000UL; /* 0x00419DD1 */
    edx = v30; /* 0x00419DD3 */
    v34 = esi; /* 0x00419DDA */
    eax = 0U; /* 0x00419DE1 */
    edx = edx << 8; /* 0x00419DE3 */
    ecx = 0U; /* 0x00419DE6 */
    v38 = edx; /* 0x00419DE8 */
    edx = 0U; /* 0x00419DEF */
L_419DF1:
    edi = ebx & 0xFFUL; /* 0x00419DF1 */
    if (eax >= (edi & 0xFFUL)) goto L_41A13E; /* 0x00419DF4 0x00419DF6 */
    edi = (unsigned int)M8(0x45C65EUL + eax); /* 0x00419DFC */
    edi = edi + edi; /* 0x00419E03 */
    edi = edi + esi; /* 0x00419E05 */
    ebp = v38; /* 0x00419E07 */
    c4[edx >> 2] = edi; /* 0x00419E0E */
    edi = (unsigned int)M8(0x45C65EUL + eax); /* 0x00419E11 */
    edi = edi + ebp; /* 0x00419E18 */
    ebp = M32(0x433B7CUL); /* 0x00419E1A */
    edi = edi + edi; /* 0x00419E20 */
    ecx = ecx + 2UL; /* 0x00419E22 */
    edi = edi + ebp; /* 0x00419E25 */
    edx = edx + 4UL; /* 0x00419E27 */
    eax = eax + 1UL; /* 0x00419E2A */
    edi = (edi & 0xFFFF0000UL) | (unsigned int)M16(edi); /* 0x00419E2B */
    cw[ecx >> 1] = (unsigned short)(edi & 0xFFFFUL); /* 0x00419E2E */
    goto L_419DF1; /* 0x00419E33 */
L_419E35:
    eax = v30; /* 0x00419E35 */
    edx = M32(0x433B84UL); /* 0x00419E3C */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(edx + eax * 2UL); /* 0x00419E42 */
    edx = 0U; /* 0x00419E46 */
    eax = eax & 0xFFFFUL; /* 0x00419E48 */
    edx = (edx & 0xFFFF0000UL) | (v28 & 0xFFFFUL); /* 0x00419E4D */
    edx = edx + eax; /* 0x00419E55 */
    ecx = edx; /* 0x00419E57 */
    G32(0x45AF68) = edx; /* 0x00419E59 */
    eax = G32(0x45D0BC); /* 0x00419E5F */
    edx = 0U; /* 0x00419E64 */
    eax = eax / ecx; /* 0x00419E66 div ecx, edx=0; /0 faults like x86 */
    edx = 0U; /* 0x00419E68 */
    G32(0x45D0BC) = eax; /* 0x00419E6A */
    ecx = eax; /* 0x00419E6F */
    eax = G32(0x45D0B8); /* 0x00419E71 */
    eax = eax / ecx; /* 0x00419E76 div ecx, edx=0; /0 faults like x86 */
    ecx = v28; /* 0x00419E78 */
    v2c = eax; /* 0x00419E7F */
    if ((eax & 0xFFFFUL) < (ecx & 0xFFFFUL)) goto L_419DC3; /* 0x00419E86 0x00419E89 */
    eax = 0U; /* 0x00419E8F */
    eax = (eax & 0xFFFF0000UL) | (ecx & 0xFFFFUL); /* 0x00419E91 */
    G32(0x45AF6C) = eax; /* 0x00419E94 */
    eax = G32(0x45AF68); /* 0x00419E99 */
    edx = G32(0x45AF6C); /* 0x00419E9E */
    G32(0x45AF70) = eax; /* 0x00419EA4 */
    eax = eax - edx; /* 0x00419EA9 */
    ecx = G32(0x45AF68); /* 0x00419EAB */
    eax = eax << 14; /* 0x00419EB1 */
    edx = 0U; /* 0x00419EB4 */
    eax = eax / ecx; /* 0x00419EB6 div ecx, edx=0; /0 faults like x86 */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(0x437968UL + (eax & 0xFFFFUL) * 2UL); /* 0x00419EB8 */
    esi = G32(0x45AF74); /* 0x00419EC0 */
    eax = eax & 0xFFFFUL; /* 0x00419EC6 */
    esi = esi + eax; /* 0x00419ECB */
    eax = 0x45AF68UL; /* 0x00419ECD */
    G32(0x45AF74) = esi; /* 0x00419ED2 */
    edx = M32(eax + 8UL); /* 0x00419ED8 */
    eax = M32(eax + 4UL); /* 0x00419EDB */
    ecx = G32(0x45D0BC); /* 0x00419EDE */
    edx = edx - eax; /* 0x00419EE4 */
    eax = eax * ecx; /* 0x00419EE6 */
    edx = edx * ecx; /* 0x00419EE9 */
    ecx = G32(0x45D0B8); /* 0x00419EEC */
    ecx = ecx - eax; /* 0x00419EF2 */
    if (edx >= 0x1000000UL) goto L_419F16; /* 0x00419EF4 0x00419EFA */
L_419EFC:
    eax = (unsigned int)d4v_get_byte(c); /* 0x00419EFC */
    ecx = ecx << 8; /* 0x00419F01 */
    eax = eax & 0xFFUL; /* 0x00419F04 */
    edx = edx << 8; /* 0x00419F09 */
    ecx = ecx | eax; /* 0x00419F0C */
    if (edx < 0x1000000UL) goto L_419EFC; /* 0x00419F0E 0x00419F14 */
L_419F16:
    G32(0x45D0B8) = ecx; /* 0x00419F16 */
    G32(0x45D0BC) = edx; /* 0x00419F1C */
    esi = v30; /* 0x00419F22 */
    eax = M32(0x433B7CUL); /* 0x00419F29 */
    esi = esi << 9; /* 0x00419F2E */
    esi = esi + eax; /* 0x00419F31 */
    eax = G32(0x45AFA4); /* 0x00419F33 */
    edx = M32(0x433B78UL); /* 0x00419F38 */
    eax = eax << 9; /* 0x00419F3E */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16((edx + eax) & U32MASK); /* 0x00419F41 mov ax,[edx+eax] (32-bit address, no mask) */
    edi = G32(0x45AFA4); /* 0x00419F45 */
    eax = eax & 0xFFFFUL; /* 0x00419F4B */
    ebx = ebx & 0xFFFF00FFUL; /* 0x00419F50 */
    G32(0x45AF68) = eax; /* 0x00419F52 */
    edi = edi << 8; /* 0x00419F57 */
L_419F5A:
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C68A); /* 0x00419F5A */
    eax = 0U; /* 0x00419F60 */
    eax = (eax & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL); /* 0x00419F62 */
    if (M16(esi + (eax & 0xFFUL) * 2UL) != 0U) edx = (edx & 0xFFFFFF00UL) | 1U; else edx = (edx & 0xFFFFFF00UL); /* 0x00419F64 0x00419F69 */
    eax = 0U; /* 0x00419F6C */
    ecx = (ecx & 0xFFFFFF00UL) | (edx & 0xFFUL); /* 0x00419F6E */
    if ((edx & 0xFFUL) != 0U) goto L_419F90; /* 0x00419F70 0x00419F72 */
L_419F74:
    edx = 0U; /* 0x00419F74 */
    edx = (edx & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x00419F76 */
    if (eax >= (edx & 0xFFUL)) goto L_419F90; /* 0x00419F78 0x00419F7A */
    edx = eax; /* 0x00419F7C */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)M8(0x45C65EUL + (edx & 0xFFUL)); /* 0x00419F7E */
    eax = eax + 1UL; /* 0x00419F84 */
    if (((ebx >> 8) & 0xFFUL) == (ecx & 0xFFUL)) edx = (edx & 0xFFFFFF00UL) | 1U; else edx = (edx & 0xFFFFFF00UL); /* 0x00419F85 0x00419F87 */
    ecx = (ecx & 0xFFFFFF00UL) | (edx & 0xFFUL); /* 0x00419F8A */
    if ((edx & 0xFFUL) == 0U) goto L_419F74; /* 0x00419F8C 0x00419F8E */
L_419F90:
    if ((ecx & 0xFFUL) == 0U) goto L_419FB7; /* 0x00419F90 0x00419F92 */
    eax = 0U; /* 0x00419F94 */
    eax = (eax & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL); /* 0x00419F96 */
    edx = M32(0x433B7CUL); /* 0x00419F98 */
    eax = eax + edi; /* 0x00419F9E */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(edx + eax * 2UL); /* 0x00419FA0 */
    edx = G32(0x45AF68); /* 0x00419FA4 */
    eax = eax & 0xFFFFUL; /* 0x00419FAA */
    edx = edx - eax; /* 0x00419FAF */
    G32(0x45AF68) = edx; /* 0x00419FB1 */
L_419FB7:
    ebx = (ebx & 0xFFFF00FFUL) | (((((ebx >> 8) & 0xFFUL) + 1UL) & 0xFFUL) << 8); /* 0x00419FB7 */
    G8(0x45C68A) = (unsigned char)(ebx & 0xFFUL); /* 0x00419FB9 */
    if (((ebx >> 8) & 0xFFUL) != 0U) goto L_419F5A; /* 0x00419FBF */
    ecx = G32(0x45AF68); /* 0x00419FC1 */
    eax = G32(0x45D0BC); /* 0x00419FC7 */
    edx = 0U; /* 0x00419FCC */
    eax = eax / ecx; /* 0x00419FCE div ecx, edx=0; /0 faults like x86 */
    edx = 0U; /* 0x00419FD0 */
    G32(0x45D0BC) = eax; /* 0x00419FD2 */
    ecx = eax; /* 0x00419FD7 */
    eax = G32(0x45D0B8); /* 0x00419FD9 */
    eax = eax / ecx; /* 0x00419FDE div ecx, edx=0; /0 faults like x86 */
    edi = G32(0x45AFA4); /* 0x00419FE0 */
    v24 = eax; /* 0x00419FE6 */
    ecx = 0U; /* 0x00419FED */
    edi = edi << 8; /* 0x00419FEF */
L_419FF2:
    v1C = (unsigned char)(v1C - 1U); /* 0x00419FF2 */
    eax = 0U; /* 0x00419FF9 */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)v1C; /* 0x00419FFB */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C68A); /* 0x0041A002 */
    if (M16(esi + (eax & 0xFFUL) * 2UL) == 0U) edx = (edx & 0xFFFFFF00UL) | 1U; else edx = (edx & 0xFFFFFF00UL); /* 0x0041A008 0x0041A00D */
    ebx = (ebx & 0xFFFF00FFUL) | ((edx & 0xFFUL) << 8); /* 0x0041A010 */
    eax = 0U; /* 0x0041A012 */
    if ((edx & 0xFFUL) == 0U) goto L_41A03B; /* 0x0041A014 0x0041A016 */
L_41A018:
    edx = 0U; /* 0x0041A018 */
    edx = (edx & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x0041A01A */
    if (eax >= (edx & 0xFFUL)) goto L_41A03B; /* 0x0041A01C 0x0041A01E */
    edx = eax; /* 0x0041A020 */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(0x45C65EUL + (edx & 0xFFUL)); /* 0x0041A022 */
    edx = (edx & 0xFFFF00FFUL) | ((unsigned int)v1C << 8); /* 0x0041A028 */
    eax = eax + 1UL; /* 0x0041A02F */
    if ((edx & 0xFFUL) != ((edx >> 8) & 0xFFUL)) edx = (edx & 0xFFFFFF00UL) | 1UL; else edx = (edx & 0xFFFFFF00UL); /* 0x0041A030 0x0041A032 */
    ebx = (ebx & 0xFFFF00FFUL) | ((edx & 0xFFUL) << 8); /* 0x0041A035 */
    if ((edx & 0xFFUL) != 0U) goto L_41A018; /* 0x0041A037 0x0041A039 */
L_41A03B:
    if (((ebx >> 8) & 0xFFUL) == 0U) goto L_41A056; /* 0x0041A03B 0x0041A03D */
    eax = 0U; /* 0x0041A03F */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)v1C; /* 0x0041A041 */
    eax = eax + edi; /* 0x0041A048 */
    edx = eax + eax; /* 0x0041A04A */
    eax = M32(0x433B7CUL); /* 0x0041A04D */
    ecx = (ecx & 0xFFFF0000UL) | (((ecx & 0xFFFFUL) + (unsigned int)M16(edx + eax)) & 0xFFFFUL); /* 0x0041A052 */
L_41A056:
    eax = v24; /* 0x0041A056 */
    G8(0x45C68A) = (unsigned char)(ebx & 0xFFUL); /* 0x0041A05D */
    if ((ecx & 0xFFFFUL) <= (eax & 0xFFFFUL)) goto L_419FF2; /* 0x0041A063 0x0041A066 */
    eax = 0U; /* 0x0041A068 */
    eax = (eax & 0xFFFF0000UL) | (ecx & 0xFFFFUL); /* 0x0041A06A */
    edx = 0U; /* 0x0041A06D */
    G32(0x45AF70) = eax; /* 0x0041A06F */
    eax = G32(0x45AFA4); /* 0x0041A074 */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)v1C; /* 0x0041A079 */
    eax = eax << 8; /* 0x0041A080 */
    eax = eax + edx; /* 0x0041A083 */
    edx = M32(0x433B7CUL); /* 0x0041A085 */
    eax = eax + eax; /* 0x0041A08B */
    eax = eax + edx; /* 0x0041A08D */
    edx = 0U; /* 0x0041A08F */
    edx = (edx & 0xFFFF0000UL) | (unsigned int)M16(eax); /* 0x0041A091 */
    eax = 0U; /* 0x0041A094 */
    eax = (eax & 0xFFFF0000UL) | (ecx & 0xFFFFUL); /* 0x0041A096 */
    eax = eax - edx; /* 0x0041A099 */
    G32(0x45AF6C) = eax; /* 0x0041A09B */
    eax = G32(0x45AF70); /* 0x0041A0A0 */
    ecx = G32(0x45AF6C); /* 0x0041A0A5 */
    esi = G32(0x45AF68); /* 0x0041A0AB */
    eax = eax - ecx; /* 0x0041A0B1 */
    edx = 0U; /* 0x0041A0B3 */
    eax = eax << 14; /* 0x0041A0B5 */
    eax = eax / esi; /* 0x0041A0B8 div esi, edx=0; /0 faults like x86 */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(0x437968UL + (eax & 0xFFFFUL) * 2UL); /* 0x0041A0BA */
    edi = G32(0x45AF74); /* 0x0041A0C2 */
    eax = eax & 0xFFFFUL; /* 0x0041A0C8 */
    edi = edi + eax; /* 0x0041A0CD */
    eax = 0x45AF68UL; /* 0x0041A0CF */
    G32(0x45AF74) = edi; /* 0x0041A0D4 */
    edx = M32(eax + 8UL); /* 0x0041A0DA */
    eax = M32(eax + 4UL); /* 0x0041A0DD */
    ecx = G32(0x45D0BC); /* 0x0041A0E0 */
    edx = edx - eax; /* 0x0041A0E6 */
    eax = eax * ecx; /* 0x0041A0E8 */
    edx = edx * ecx; /* 0x0041A0EB */
    ecx = G32(0x45D0B8); /* 0x0041A0EE */
    ecx = ecx - eax; /* 0x0041A0F4 */
    if (edx >= 0x1000000UL) goto L_41A118; /* 0x0041A0F6 0x0041A0FC */
L_41A0FE:
    eax = (unsigned int)d4v_get_byte(c); /* 0x0041A0FE */
    ecx = ecx << 8; /* 0x0041A103 */
    eax = eax & 0xFFUL; /* 0x0041A106 */
    edx = edx << 8; /* 0x0041A10B */
    ecx = ecx | eax; /* 0x0041A10E */
    if (edx < 0x1000000UL) goto L_41A0FE; /* 0x0041A110 0x0041A116 */
L_41A118:
    G32(0x45D0B8) = ecx; /* 0x0041A118 */
    G32(0x45D0BC) = edx; /* 0x0041A11E */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)v1C; /* 0x0041A124 */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C68A); /* 0x0041A12B */
    /* add esp,0ACh; pop ebp/edi/esi/edx/ecx/ebx (frame only) */
    return v1C; /* 0x0041A131 0x0041A13D */
L_41A13E:
    eax = v2c; /* 0x0041A13E */
    v28 = v28 - eax; /* 0x0041A145 */
L_41A14C:
    edi = 0U; /* 0x0041A14C */
    ecx = 0U; /* 0x0041A14E */
    eax = 0U; /* 0x0041A150 */
    edx = 0U; /* 0x0041A152 */
L_41A154:
    ebp = ebx & 0xFFUL; /* 0x0041A154 */
    if (ecx >= (ebp & 0xFFUL)) goto L_41A179; /* 0x0041A157 0x0041A159 */
    ebp = (ebx >> 8) & 0xFFUL; /* 0x0041A15B */
    ebp = ebp + ebp; /* 0x0041A15E */
    ebp = ebp + esi; /* 0x0041A160 */
    if (ebp > c4[eax >> 2]) goto L_41A170; /* 0x0041A162 0x0041A165 */
L_41A167:
    eax = eax + 4UL; /* 0x0041A167 */
    edx = edx + 2UL; /* 0x0041A16A */
    ecx = ecx + 1UL; /* 0x0041A16D */
    goto L_41A154; /* 0x0041A16E */
L_41A170:
    ebp = (unsigned int)cw[(edx + 2UL) >> 1]; /* 0x0041A170 */
    edi = edi + ebp; /* 0x0041A175 */
    goto L_41A167; /* 0x0041A177 */
L_41A179:
    eax = 0U; /* 0x0041A179 */
    eax = (eax & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL); /* 0x0041A17B */
    edx = 0U; /* 0x0041A17D */
    edx = (edx & 0xFFFF0000UL) | (unsigned int)M16(esi + (eax & 0xFFUL) * 2UL); /* 0x0041A17F */
    eax = 0U; /* 0x0041A183 */
    eax = (eax & 0xFFFF0000UL) | (v28 & 0xFFFFUL); /* 0x0041A185 */
    eax = eax + edi; /* 0x0041A18D */
    if (edx >= eax) goto L_41A1DA; /* 0x0041A18F 0x0041A191 */
    edx = 0U; /* 0x0041A193 */
    eax = 0U; /* 0x0041A195 */
L_41A197:
    ecx = 0U; /* 0x0041A197 */
    ecx = (ecx & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x0041A199 */
    if (edx >= (ecx & 0xFFUL)) goto L_41A1BD; /* 0x0041A19B 0x0041A19D */
    ecx = 0U; /* 0x0041A19F */
    ecx = (ecx & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL); /* 0x0041A1A1 */
    ecx = ecx + ecx; /* 0x0041A1A3 */
    ebp = c4[eax >> 2]; /* 0x0041A1A5 */
    ecx = ecx + esi; /* 0x0041A1A8 */
    if (ecx > ebp) goto L_41A1B4; /* 0x0041A1AA 0x0041A1AC */
L_41A1AE:
    eax = eax + 4UL; /* 0x0041A1AE */
    edx = edx + 1UL; /* 0x0041A1B1 */
    goto L_41A197; /* 0x0041A1B2 */
L_41A1B4:
    c4[eax >> 2] = 0xFFFFFFFFUL; /* 0x0041A1B4 */
    goto L_41A1AE; /* 0x0041A1BB */
L_41A1BD:
    eax = 0U; /* 0x0041A1BD */
    eax = (eax & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL); /* 0x0041A1BF */
    eax = eax + eax; /* 0x0041A1C1 */
    esi = esi + eax; /* 0x0041A1C3 */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(esi); /* 0x0041A1C5 */
    edx = v28; /* 0x0041A1C8 */
    eax = eax - edi; /* 0x0041A1CF */
    edx = edx - eax; /* 0x0041A1D1 */
    v28 = edx; /* 0x0041A1D3 */
L_41A1DA:
    ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) & 0xFFUL) >> 1) << 8); /* 0x0041A1DA */
    if (((ebx >> 8) & 0xFFUL) != 0U) goto L_41A14C; /* 0x0041A1DC 0x0041A1DE */
    eax = v2c; /* 0x0041A1E4 */
    ecx = v28; /* 0x0041A1EB */
    edi = v34; /* 0x0041A1F2 */
    ecx = ecx + eax; /* 0x0041A1F9 */
    eax = esi; /* 0x0041A1FB */
    eax = eax - edi; /* 0x0041A1FD */
    G8(0x45C68A) = (unsigned char)(ebx & 0xFFUL); /* 0x0041A1FF */
    eax = (unsigned int)(((int)eax) >> 1); /* 0x0041A205 sar */
    v28 = ecx; /* 0x0041A207 */
    goto L_41A20E; /* 0x0041A207 fallthrough to 0x0041A20E */
L_41A20E:
    v20 = (unsigned char)(eax & 0xFFUL); /* 0x0041A20E */
    eax = 0U; /* 0x0041A215 */
    eax = (eax & 0xFFFF0000UL) | (v28 & 0xFFFFUL); /* 0x0041A217 */
    edx = 0U; /* 0x0041A21F */
    G32(0x45AF70) = eax; /* 0x0041A221 */
    eax = v30; /* 0x0041A226 */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)v20; /* 0x0041A22D */
    eax = eax << 8; /* 0x0041A234 */
    eax = eax + edx; /* 0x0041A237 */
    edx = M32(0x433B7CUL); /* 0x0041A239 */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(edx + eax * 2UL); /* 0x0041A23F */
    edx = 0U; /* 0x0041A243 */
    eax = eax & 0xFFFFUL; /* 0x0041A245 */
    edx = (edx & 0xFFFF0000UL) | (v28 & 0xFFFFUL); /* 0x0041A24A */
    edx = edx - eax; /* 0x0041A252 */
    eax = G32(0x45AF70); /* 0x0041A254 */
    ecx = G32(0x45AF68); /* 0x0041A259 */
    eax = eax - edx; /* 0x0041A25F */
    G32(0x45AF6C) = edx; /* 0x0041A261 */
    eax = eax << 14; /* 0x0041A267 */
    edx = 0U; /* 0x0041A26A */
    eax = eax / ecx; /* 0x0041A26C div ecx, edx=0; /0 faults like x86 */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(0x437968UL + (eax & 0xFFFFUL) * 2UL); /* 0x0041A26E */
    esi = G32(0x45AF74); /* 0x0041A276 */
    eax = eax & 0xFFFFUL; /* 0x0041A27C */
    esi = esi + eax; /* 0x0041A281 */
    eax = 0x45AF68UL; /* 0x0041A283 */
    G32(0x45AF74) = esi; /* 0x0041A288 */
    edx = M32(eax + 8UL); /* 0x0041A28E */
    eax = M32(eax + 4UL); /* 0x0041A291 */
    ecx = G32(0x45D0BC); /* 0x0041A294 */
    edx = edx - eax; /* 0x0041A29A */
    eax = eax * ecx; /* 0x0041A29C */
    edx = edx * ecx; /* 0x0041A29F */
    ecx = G32(0x45D0B8); /* 0x0041A2A2 */
    ecx = ecx - eax; /* 0x0041A2A8 */
    if (edx >= 0x1000000UL) goto L_41A2CC; /* 0x0041A2AA 0x0041A2B0 */
L_41A2B2:
    eax = (unsigned int)d4v_get_byte(c); /* 0x0041A2B2 */
    ecx = ecx << 8; /* 0x0041A2B7 */
    eax = eax & 0xFFUL; /* 0x0041A2BA */
    edx = edx << 8; /* 0x0041A2BF */
    ecx = ecx | eax; /* 0x0041A2C2 */
    if (edx < 0x1000000UL) goto L_41A2B2; /* 0x0041A2C4 0x0041A2CA */
L_41A2CC:
    G32(0x45D0B8) = ecx; /* 0x0041A2CC */
    G32(0x45D0BC) = edx; /* 0x0041A2D2 */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)v20; /* 0x0041A2D8 */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C68A); /* 0x0041A2DF */
    /* add esp,0ACh; pop ebp/edi/esi/edx/ecx/ebx (frame only) */
    return (unsigned char)(eax & 0xFFUL); /* 0x0041A2E5 0x0041A2F1 */
L_41A2F2:
    edx = v2c; /* 0x0041A2F2 */
    edi = v28; /* 0x0041A2F9 */
    eax = (eax & 0xFFFFFF00UL) | 0x80UL; /* 0x0041A300 */
    edi = edi - edx; /* 0x0041A302 */
    ecx = esi; /* 0x0041A304 */
    v28 = edi; /* 0x0041A306 */
L_41A30D:
    edx = 0U; /* 0x0041A30D */
    edx = (edx & 0xFFFFFF00UL) | (eax & 0xFFUL); /* 0x0041A30F */
    edx = edx + edx; /* 0x0041A311 */
    edx = edx + esi; /* 0x0041A313 */
    edi = (edi & 0xFFFF0000UL) | (unsigned int)M16(edx); /* 0x0041A315 */
    if ((edi & 0xFFFFUL) >= (v28 & 0xFFFFUL)) goto L_41A32D; /* 0x0041A318 0x0041A320 */
    esi = edx; /* 0x0041A322 */
    edx = edi; /* 0x0041A324 */
    v28 = v28 - edx; /* 0x0041A326 */
L_41A32D:
    eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFUL) >> 1) & 0xFFUL); /* 0x0041A32D */
    if ((eax & 0xFFUL) != 0U) goto L_41A30D; /* 0x0041A32F 0x0041A331 */
    eax = v2c; /* 0x0041A333 */
    edx = v28; /* 0x0041A33A */
    edx = edx + eax; /* 0x0041A341 */
    eax = esi; /* 0x0041A343 */
    eax = eax - ecx; /* 0x0041A345 */
    G8(0x45C68A) = (unsigned char)(ebx & 0xFFUL); /* 0x0041A347 */
    eax = (unsigned int)(((int)eax) >> 1); /* 0x0041A34D sar */
    v28 = edx; /* 0x0041A34F */
    goto L_41A20E; /* 0x0041A356 */
}

/* sub_41A360: freq-divide helper.
 * Role hypothesis: given a table selector in eax and a symbol byte in dl,
 * subtracts the mapped word frequencies ([433B7C] via byte_45C65E order,
 * plus the 256-slot scan when the direct slot is empty and bh!=0), does
 * the 0x437968 price lookup ((freq<<14)/total), and accumulates into
 * 45AF74. Used by 419970/41AD60/41AE30 after they set 45AFA4/45AF74.
 * Entry: eax, dl. Frame: 8 (var_1C dword, var_18 byte). Returns leftover
 * eax (price remnant; callers ignore -> effectively void).
 */
unsigned int d4v_41a360(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int ebp = 0;
    unsigned int var1C = 0;
    unsigned char var18 = 0;
    (void)c;
    ebx = 0U;
    ecx = 0U;
    esi = 0U;
    edi = 0U;
    ebp = 0U;
    var1C = 0U;
    var18 = 0;
    /* push ebx/ecx/esi/edi/ebp; sub esp,8 (frame only) */
    ebx = (ebx & 0xFFFF00FFUL) | ((unsigned int)G8(0x45C68A) << 8); /* 0x0041A368 */
    ecx = eax_in; /* 0x0041A36E */
    var18 = (unsigned char)(edx_in & 0xFFUL); /* 0x0041A370 */
    edi = M32(0x433B7CUL); /* 0x0041A374 */
    eax = eax_in << 9; /* 0x0041A37A */
    edx = 0U; /* 0x0041A37D */
    edi = edi + eax; /* 0x0041A37F */
    var1C = edx; /* 0x0041A381 */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)var18; /* 0x0041A384 */
    if (M16(edi + (edx & 0xFFUL) * 2UL) != 0U) goto L_41A410; /* 0x0041A388 0x0041A38D */
    edx = M32(0x433B78UL); /* 0x0041A393 */
    edx = (edx & 0xFFFF0000UL) | (unsigned int)M16(edx + eax); /* 0x0041A399 */
    eax = 0U; /* 0x0041A39D */
L_41A39F:
    ebp = (ebx >> 8) & 0xFFUL; /* 0x0041A39F */
    if (eax >= (ebp & 0xFFUL)) goto L_41A3B6; /* 0x0041A3A2 0x0041A3A4 */
    ebp = (unsigned int)M8(0x45C65EUL + eax); /* 0x0041A3A6 */
    esi = (esi & 0xFFFF0000UL) | (unsigned int)M16(edi + (ebp & 0xFFUL) * 2UL); /* 0x0041A3AD */
    eax = eax + 1UL; /* 0x0041A3B1 */
    edx = edx - esi; /* 0x0041A3B2 */
    goto L_41A39F; /* 0x0041A3B4 */
L_41A3B6:
    if ((edx & 0xFFFFUL) == 0U) goto L_41A3FE; /* 0x0041A3B6 0x0041A3B9 */
    eax = M32(0x433B84UL); /* 0x0041A3BB */
    ecx = ecx + ecx; /* 0x0041A3C0 */
    eax = eax + ecx; /* 0x0041A3C2 */
    ecx = 0U; /* 0x0041A3C4 */
    var1C = edi; /* 0x0041A3C6 */
    ecx = (ecx & 0xFFFF0000UL) | (unsigned int)M16(eax); /* 0x0041A3C9 */
    edx = edx & 0xFFFFUL; /* 0x0041A3CC */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(eax); /* 0x0041A3D2 */
    ecx = ecx + edx; /* 0x0041A3D5 */
    eax = eax & 0xFFFFUL; /* 0x0041A3D7 */
    edx = 0U; /* 0x0041A3DC */
    eax = eax << 14; /* 0x0041A3DE */
    eax = eax / ecx; /* 0x0041A3E1 div ecx, edx=0; /0 faults like x86 */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(0x437968UL + (eax & 0xFFFFUL) * 2UL); /* 0x0041A3E3 */
    ecx = G32(0x45AF74); /* 0x0041A3EB */
    eax = eax & 0xFFFFUL; /* 0x0041A3F1 */
    ecx = ecx + eax; /* 0x0041A3F6 */
    G32(0x45AF74) = ecx; /* 0x0041A3F8 */
L_41A3FE:
    ecx = G32(0x45AFA4); /* 0x0041A3FE */
    edi = ecx; /* 0x0041A404 */
    eax = M32(0x433B7CUL); /* 0x0041A406 */
    edi = edi << 9; /* 0x0041A40B */
    edi = edi + eax; /* 0x0041A40E */
L_41A410:
    eax = ecx; /* 0x0041A410 */
    edx = M32(0x433B78UL); /* 0x0041A412 */
    eax = eax << 9; /* 0x0041A418 */
    esi = (esi & 0xFFFF0000UL) | (unsigned int)M16(edx + eax); /* 0x0041A41B */
    if (ecx >= 0x2FFUL) goto L_41A44F; /* 0x0041A41F 0x0041A425 */
    edx = ecx + ecx; /* 0x0041A427 */
    ecx = M32(0x433B84UL); /* 0x0041A42A */
    eax = 0U; /* 0x0041A430 */
    esi = (esi & 0xFFFF0000UL) | (((esi & 0xFFFFUL) + (unsigned int)M16(ecx + edx)) & 0xFFFFUL); /* 0x0041A432 */
L_41A436:
    edx = 0U; /* 0x0041A436 */
    edx = (edx & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL); /* 0x0041A438 */
    if (eax >= (edx & 0xFFUL)) goto L_41A473; /* 0x0041A43A 0x0041A43C */
    edx = 0U; /* 0x0041A43E */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(0x45C65EUL + eax); /* 0x0041A440 */
    ecx = (ecx & 0xFFFF0000UL) | (unsigned int)M16(edi + (edx & 0xFFUL) * 2UL); /* 0x0041A446 */
    eax = eax + 1UL; /* 0x0041A44A */
    esi = esi - ecx; /* 0x0041A44B */
    goto L_41A436; /* 0x0041A44D */
L_41A44F:
    if (((ebx >> 8) & 0xFFUL) != 0U) goto L_41A4B8; /* 0x0041A44F 0x0041A451 */
    if (var1C == 0U) goto L_41A473; /* 0x0041A453 0x0041A457 */
    ebp = var1C; /* 0x0041A459 */
    eax = (eax & 0xFFFFFF00UL); /* 0x0041A45C */
L_41A45E:
    edx = 0U; /* 0x0041A45E */
    edx = (edx & 0xFFFFFF00UL) | (eax & 0xFFUL); /* 0x0041A460 */
    edx = edx + edx; /* 0x0041A462 */
    if (M16(edx + ebp) == 0U) goto L_41A46F; /* 0x0041A464 0x0041A469 */
L_41A46B:
    esi = esi - (unsigned int)M16(edx + edi); /* 0x0041A46B */
L_41A46F:
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFUL) + 1UL) & 0xFFUL)); /* 0x0041A46F */
    if ((eax & 0xFFUL) != 0U) goto L_41A45E; /* 0x0041A471 */
L_41A473:
    eax = 0U; /* 0x0041A473 */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)var18; /* 0x0041A475 */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(edi + (eax & 0xFFUL) * 2UL); /* 0x0041A479 */
    eax = eax & 0xFFFFUL; /* 0x0041A47D */
    ecx = 0U; /* 0x0041A482 */
    edx = 0U; /* 0x0041A484 */
    eax = eax << 14; /* 0x0041A486 */
    ecx = (ecx & 0xFFFF0000UL) | (esi & 0xFFFFUL); /* 0x0041A489 */
    eax = eax / ecx; /* 0x0041A48C div ecx, edx=0; /0 faults like x86 */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(0x437968UL + (eax & 0xFFFFUL) * 2UL); /* 0x0041A48E */
    ecx = G32(0x45AF74); /* 0x0041A496 */
    eax = eax & 0xFFFFUL; /* 0x0041A49C */
    ecx = ecx + eax; /* 0x0041A4A1 */
    G32(0x45AF74) = ecx; /* 0x0041A4A3 */
    G8(0x45C68A) = (unsigned char)((ebx >> 8) & 0xFFUL); /* 0x0041A4A9 */
    /* add esp,8; pop ebp/edi/esi/ecx/ebx (frame only) */
    return eax; /* 0x0041A4AF 0x0041A4B7 */
L_41A4B8:
    if (var1C == 0U) goto L_41A50C; /* 0x0041A4B8 0x0041A4BC */
    ebx = (ebx & 0xFFFFFF00UL); /* 0x0041A4BE */
L_41A4C0:
    eax = 0U; /* 0x0041A4C0 */
    eax = (eax & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x0041A4C2 */
    edx = var1C; /* 0x0041A4C4 */
    eax = eax + eax; /* 0x0041A4C7 */
    eax = eax + edx; /* 0x0041A4C9 */
    if (M16(eax) != 0U) edx = (edx & 0xFFFFFF00UL) | 1U; else edx = (edx & 0xFFFFFF00UL); /* 0x0041A4CB 0x0041A4CF */
    eax = 0U; /* 0x0041A4D2 */
    ecx = (ecx & 0xFFFFFF00UL) | (edx & 0xFFUL); /* 0x0041A4D4 */
    if ((edx & 0xFFUL) != 0U) goto L_41A4F6; /* 0x0041A4D6 0x0041A4D8 */
L_41A4DA:
    edx = 0U; /* 0x0041A4DA */
    edx = (edx & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL); /* 0x0041A4DC */
    if (eax >= (edx & 0xFFUL)) goto L_41A4F6; /* 0x0041A4DE 0x0041A4E0 */
    edx = eax; /* 0x0041A4E2 */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)M8(0x45C65EUL + (edx & 0xFFUL)); /* 0x0041A4E4 */
    eax = eax + 1UL; /* 0x0041A4EA */
    if ((ebx & 0xFFUL) == (ecx & 0xFFUL)) edx = (edx & 0xFFFFFF00UL) | 1U; else edx = (edx & 0xFFFFFF00UL); /* 0x0041A4EB 0x0041A4ED */
    ecx = (ecx & 0xFFFFFF00UL) | (edx & 0xFFUL); /* 0x0041A4F0 */
    if ((edx & 0xFFUL) == 0U) goto L_41A4DA; /* 0x0041A4F2 0x0041A4F4 */
L_41A4F6:
    if ((ecx & 0xFFUL) == 0U) goto L_41A502; /* 0x0041A4F6 0x0041A4F8 */
    eax = 0U; /* 0x0041A4FA */
    eax = (eax & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x0041A4FC */
    esi = esi - (unsigned int)M16(edi + (eax & 0xFFUL) * 2UL); /* 0x0041A4FE */
L_41A502:
    ebx = (ebx & 0xFFFFFF00UL) | ((((ebx & 0xFFUL) + 1UL) & 0xFFUL)); /* 0x0041A502 */
    if ((ebx & 0xFFUL) == 0U) goto L_41A473; /* 0x0041A504 */
    goto L_41A4C0; /* 0x0041A50A */
L_41A50C:
    eax = 0U; /* 0x0041A50C */
L_41A50E:
    edx = 0U; /* 0x0041A50E */
    edx = (edx & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL); /* 0x0041A510 */
    if (eax >= (edx & 0xFFUL)) goto L_41A473; /* 0x0041A512 0x0041A514 */
    edx = 0U; /* 0x0041A51A */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(0x45C65EUL + eax); /* 0x0041A51C */
    ecx = (ecx & 0xFFFF0000UL) | (unsigned int)M16(edi + (edx & 0xFFUL) * 2UL); /* 0x0041A522 */
    eax = eax + 1UL; /* 0x0041A526 */
    esi = esi - ecx; /* 0x0041A527 */
    goto L_41A50E; /* 0x0041A529 */
}

/* sub_41AD60: tiny dispatch to 41A360.
 * Role hypothesis: zeroes 45AF74, runs the freq-divide helper with
 * (eax, bl) after stashing edx into 45AFA4, logs (45AFA4/symbol/bl) into
 * the 458778/458788/45AF50 ring, then adapts the 459398 + 459BA0 slots
 * selected by (entry_edx == 0x2FF).
 * Entry: eax, edx, bl. Void. No frame.
 */
void d4v_41ad60(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    eax = 0U;
    ebx = ebx_in;
    ecx = 0U;
    edx = 0U;
    esi = 0U;
    edi = 0U;
    /* push ecx/esi/edi (save only) */
    esi = eax_in; /* 0x0041AD63 */
    ecx = edx_in; /* 0x0041AD65 */
    edx = 0U; /* 0x0041AD67 */
    G32(0x45AF74) = edx; /* 0x0041AD69 */
    edx = (edx & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x0041AD6F */
    G32(0x45AFA4) = ecx; /* 0x0041AD71 */
    (void)d4v_41a360(c, esi, edx); /* 0x0041AD77 eax=esi implied */
    edx = 0U; /* 0x0041AD7C */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C690); /* 0x0041AD7E */
    edi = G32(0x45AFA4); /* 0x0041AD84 */
    eax = (eax & 0xFFFF00FFUL) | ((edx & 0xFFUL) << 8); /* 0x0041AD8A */
    M32(0x458778UL + (edx & 0xFFUL) * 4UL) = edi; /* 0x0041AD8C */
    M32(0x458788UL + (edx & 0xFFUL) * 4UL) = esi; /* 0x0041AD93 */
    eax = (eax & 0xFFFF00FFUL) | (((((eax >> 8) & 0xFFUL) + 1UL) & 0xFFUL) << 8); /* 0x0041AD9A */
    M8(0x45AF50UL + (edx & 0xFFUL)) = (unsigned char)(ebx & 0xFFUL); /* 0x0041AD9C */
    G8(0x45C690) = (unsigned char)((eax >> 8) & 0xFFUL); /* 0x0041ADA2 */
    if (ecx == 0x2FFUL) eax = 1UL; else eax = 0UL; /* 0x0041ADA8 0x0041ADAE */
    eax = eax & 0xFFUL; /* 0x0041ADB1 */
    edx = M32(0x459398UL + (eax & 0xFFUL) * 4UL); /* 0x0041ADB6 */
    eax = edx; /* 0x0041ADBD */
    eax = eax << 5; /* 0x0041ADBF */
    eax = eax - edx; /* 0x0041ADC2 */
    edx = G32(0x45AF74); /* 0x0041ADC4 */
    eax = eax >> 5; /* 0x0041ADCA */
    edx = edx + eax; /* 0x0041ADCD */
    if (ecx == 0x2FFUL) eax = 1UL; else eax = 0UL; /* 0x0041ADCF 0x0041ADD5 */
    eax = eax & 0xFFUL; /* 0x0041ADD8 */
    M32(0x459398UL + (eax & 0xFFUL) * 4UL) = edx; /* 0x0041ADDD */
    if (ecx == 0x2FFUL) eax = 1UL; else eax = 0UL; /* 0x0041ADE4 0x0041ADEA */
    edx = 0U; /* 0x0041ADED */
    edx = (edx & 0xFFFFFF00UL) | (eax & 0xFFUL); /* 0x0041ADEF */
    edx = M32(0x459BA0UL + (edx & 0xFFUL) * 4UL); /* 0x0041ADF1 */
    eax = edx; /* 0x0041ADF8 */
    eax = eax << 5; /* 0x0041ADFA */
    eax = eax - edx; /* 0x0041ADFD */
    edx = G32(0x45AF74); /* 0x0041ADFF */
    eax = eax >> 5; /* 0x0041AE05 */
    edx = edx + eax; /* 0x0041AE08 */
    if (ecx == 0x2FFUL) eax = 1UL; else eax = 0UL; /* 0x0041AE0A 0x0041AE10 */
    eax = eax & 0xFFUL; /* 0x0041AE13 */
    M32(0x459BA0UL + (eax & 0xFFUL) * 4UL) = edx; /* 0x0041AE18 */
    /* pop edi/esi/ecx; retn (save only) */
}

/* sub_41AE30: tiny context-index shim.
 * Role hypothesis: stashes edx into 45AFA4, zeroes 45AF74, runs the
 * freq-divide helper with passthrough eax + bl, then adapts the 45AF60
 * counter (old*31/32 + 45AF74).
 * Entry: eax (passthrough), edx, bl. Void. No frame.
 */
void d4v_41ae30(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    eax = eax_in;
    ebx = ebx_in;
    ecx = 0U;
    edx = 0U;
    /* push ecx (save only) */
    G32(0x45AFA4) = edx_in; /* 0x0041AE31 */
    edx = 0U; /* 0x0041AE37 */
    ecx = 0U; /* 0x0041AE39 */
    edx = (edx & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* 0x0041AE3B */
    G32(0x45AF74) = ecx; /* 0x0041AE3D */
    (void)d4v_41a360(c, eax, edx); /* 0x0041AE43 eax=entry passthrough */
    edx = G32(0x45AF60); /* 0x0041AE48 */
    eax = edx; /* 0x0041AE4E */
    eax = eax << 5; /* 0x0041AE50 */
    eax = eax - edx; /* 0x0041AE53 */
    edx = eax; /* 0x0041AE55 */
    eax = G32(0x45AF74); /* 0x0041AE57 */
    edx = edx >> 5; /* 0x0041AE5C */
    edx = edx + eax; /* 0x0041AE5F */
    G32(0x45AF60) = edx; /* 0x0041AE61 */
    /* pop ecx; retn (save only) */
}

/* sub_41AE70: tiny shim (freq scale).
 * Role hypothesis: scaled frequency step: ebx = tab[1]*edx; esi = tab[0];
 * eax = ebx/esi (min 1); eax += edx; saturate at 0xFF.
 * Entry: eax = tab (arena pointer: tab[0], tab[1] bytes), edx. Frame: none.
 * Returns eax.
 */
unsigned int d4v_41ae70(dec4v_ctx *c, unsigned int tab, unsigned int edx_in)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    (void)c;
    eax = 0U;
    ebx = 0U;
    ecx = edx_in;
    edx = edx_in;
    esi = 0U;
    /* push ebx/ecx/esi (save only) */
    ecx = edx; /* 0x0041AE73 */
    ebx = 0U; /* 0x0041AE75 */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)M8(tab + 1UL); /* 0x0041AE77 */
    ebx = ebx * edx; /* 0x0041AE7A */
    esi = (unsigned int)M8(tab); /* 0x0041AE7D */
    eax = ebx; /* 0x0041AE80 */
    edx = 0U; /* 0x0041AE82 */
    eax = eax / esi; /* 0x0041AE84 div esi, edx=0; /0 faults like x86 if tab[0]==0 */
    if (eax != 0U) goto L_41AE8F; /* 0x0041AE86 0x0041AE88 */
    eax = 1U; /* 0x0041AE8A */
L_41AE8F:
    eax = eax + ecx; /* 0x0041AE8F */
    if (eax <= 0xFFUL) goto L_41AE9D; /* 0x0041AE91 0x0041AE96 */
    eax = 0xFFUL; /* 0x0041AE98 */
L_41AE9D:
    /* pop esi/ecx/ebx; retn (save only) */
    return eax; /* 0x0041AE9D 0x0041AEA0 */
}

/* sub_41AEB0: tiny shim (2-byte counter with halve on saturate).
 * Role hypothesis: increments tab[1] (flag==0) or tab[0] (flag!=0); if
 * either reaches >=0xFE, stores (x+1)>>1 back into both bytes.
 * Entry: eax = tab (arena pointer), dl = flag. Void. No frame.
 */
void d4v_41aeb0(dec4v_ctx *c, unsigned int tab, unsigned int flag_in)
{
    unsigned int eax = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    (void)c;
    eax = tab;
    ecx = 0U;
    edx = flag_in;
    /* push ecx (save only) */
    if ((edx & 0xFFUL) != 0U) goto L_41AEC5; /* 0x0041AEB1 0x0041AEB3 */
    M8(tab + 1UL) = (unsigned char)(M8(tab + 1UL) + 1U); /* 0x0041AEB5 */
L_41AEB8:
    if (M8(tab) >= 0xFEU) goto L_41AEC9; /* 0x0041AEB8 0x0041AEBB */
    if (M8(tab + 1UL) >= 0xFEU) goto L_41AEC9; /* 0x0041AEBD 0x0041AEC1 */
    /* pop ecx; retn (save only) */
    return; /* 0x0041AEC3 0x0041AEC4 */
L_41AEC5:
    M8(tab) = (unsigned char)(M8(tab) + 1U); /* 0x0041AEC5 */
    goto L_41AEB8; /* 0x0041AEC7 */
L_41AEC9:
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)M8(eax); /* 0x0041AEC9 */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(eax + 1UL); /* 0x0041AECB */
    ecx = (ecx & 0xFFFFFF00UL) | (((ecx & 0xFFUL) + 1UL) & 0xFFUL); /* 0x0041AECE */
    edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFUL) + 1UL) & 0xFFUL); /* 0x0041AED0 */
    M8(eax) = (unsigned char)(ecx & 0xFFUL); /* 0x0041AED2 */
    M8(eax + 1UL) = (unsigned char)(edx & 0xFFUL); /* 0x0041AED4 */
    ecx = (ecx & 0xFFFF00FFUL) | ((ecx & 0xFFUL) << 8); /* 0x0041AED7 ch=cl */
    edx = (edx & 0xFFFF00FFUL) | ((edx & 0xFFUL) << 8); /* 0x0041AED9 dh=dl */
    ecx = (ecx & 0xFFFF00FFUL) | ((((ecx >> 8) & 0xFFUL) >> 1) << 8); /* 0x0041AEDB */
    edx = (edx & 0xFFFF00FFUL) | ((((edx >> 8) & 0xFFUL) >> 1) << 8); /* 0x0041AEDD */
    M8(eax) = (unsigned char)((ecx >> 8) & 0xFFUL); /* 0x0041AEDF */
    M8(eax + 1UL) = (unsigned char)((edx >> 8) & 0xFFUL); /* 0x0041AEE1 */
    /* pop ecx; retn (save only) */
}

/* ---- workers: PPM helpers batch S2 (from draft_s2.c) ---- */
void d4v_415ae0(dec4v_ctx *c)
{
    unsigned long eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0;

    /* prologue 0x00415AE0 push ebx; 0x00415AE1 push ecx;
     * 0x00415AE2 push edx; 0x00415AE3 push esi.
     * Callee-saved regs become C locals; there is no esp frame. */

    /* 0x00415AE4 */ esi = (unsigned long)G32(0x433B48);
    /* 0x00415AEA */ ebx = 0; /* xor ebx,ebx */
l_415AEC:
    /* 0x00415AEC */ ecx = (esi + ebx) & U32MASK; /* lea ecx,[esi+ebx] */
    /* 0x00415AEF cmp byte ptr [ecx],0 */;
    /* 0x00415AF2 jz */ if (M8(ecx) == 0) goto l_415B28; /* equality */
    /* 0x00415AF4 mov al,8 + 0x00415AF6 xor ah,ah (eax high word dead:
     * only al/ah are read below) */ eax = 8UL;
l_415AF8:
    /* 0x00415AF8 */ DECB_LO(eax); /* dec al */
    /* 0x00415AFA */ edx = 0; /* xor edx,edx */
    /* 0x00415AFC */ edx |= (eax & 0xFFUL); /* mov dl,al */
    /* 0x00415AFE */ M8((ecx + edx + 1) & U32MASK) =
        (unsigned char)(M8((ecx + edx + 1) & U32MASK) >> 2); /* shr byte ptr [ecx+edx+1],2 */
    /* 0x00415B03 */ ADD8_HI(eax, (unsigned long)M8((ecx + edx + 1) & U32MASK)); /* add ah,[ecx+edx+1] */
    /* 0x00415B07 test al,al */;
    /* 0x00415B09 jnz */ if ((eax & 0xFFUL) != 0) goto l_415AF8; /* equality */
    /* 0x00415B0B test ah,ah */;
    /* 0x00415B0D jz */ if (((eax >> 8) & 0xFFUL) == 0) goto l_415B3E; /* equality */
    /* 0x00415B0F */ edx = 0; /* xor edx,edx */
    /* 0x00415B11 */ edx |= (unsigned long)M8(ecx); /* mov dl,[ecx] */
    /* 0x00415B13 */ edx = d4v_sar(edx, 2); /* sar edx,2 (operand is a zero-extended byte, so == shr) */
    /* 0x00415B16 */ edx = (edx + 1) & U32MASK; /* inc edx */
l_415B17:
    /* 0x00415B17 */ M8(ecx) = (unsigned char)(edx & 0xFFUL); /* mov [ecx],dl */
    /* 0x00415B19 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8((ecx + 0x11UL) & U32MASK); /* mov dl,[ecx+11h] */
    /* 0x00415B1C */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8(ecx); /* mov al,[ecx] */
    /* 0x00415B1E */ edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) >> 1); /* shr dl,1 */
    /* 0x00415B20 */ ADD8_HI(eax, eax & 0xFFUL); /* add ah,al */
    /* 0x00415B22 */ M8((ecx + 0x11UL) & U32MASK) = (unsigned char)(edx & 0xFFUL); /* mov [ecx+11h],dl */
    /* 0x00415B25 */ M8((ecx + 0x12UL) & U32MASK) = (unsigned char)((eax >> 8) & 0xFFUL); /* mov [ecx+12h],ah */
l_415B28:
    /* 0x00415B28 */ ebx = (ebx + 0x14UL) & U32MASK; /* add ebx,14h */
    /* 0x00415B2B cmp ebx,140000h */;
    /* 0x00415B31 jnz */ if (ebx != 0x140000UL) goto l_415AEC; /* equality */
    /* 0x00415B33 */ G32(0x433B48) = (unsigned int)esi; /* store-back of unchanged base */
    /* epilogue 0x00415B39 pop esi; 0x00415B3A pop edx;
     * 0x00415B3B pop ecx; 0x00415B3C pop ebx; 0x00415B3D retn. */
    return;
l_415B3E:
    /* 0x00415B3E */ edx = 0; /* xor edx,edx */
    /* 0x00415B40 jmp */ goto l_415B17; /* unconditional */
}

/* ------------------------------------------------------------------ */
/* sub_415B50 (lst 31845-31948) -> d4v_415b50                          */
/*                                                                      */
/* Role: rescale/decay the table at [0x433B44] (0x20-byte entries over  */
/* banks 0..3 at +0x8000 index / +0x100000 address steps; halves 13     */
/* count bytes; on zero sum shifts one bank down onto the entry, else   */
/* clears it; folds [edi+1Bh]/[edi] the same way as sub_415AE0).        */
/* Entry-live regs: none (edx zeroed and var_1C initialised before any */
/* read; no stack args). Returns void.                                 */
/* Frame: 4 bytes (var_1C @ sp+0) + 6 pushed regs.                     */
/* Globals touched: 433B44.                                            */
/* ------------------------------------------------------------------ */
void d4v_415b50(dec4v_ctx *c)
{
    unsigned long eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned long t_1c = 0;
    unsigned long sv_edi = 0;

    /* prologue 0x00415B50 push ebx; 0x00415B51 push ecx;
     * 0x00415B52 push edx; 0x00415B53 push esi; 0x00415B54 push edi;
     * 0x00415B55 push ebp; 0x00415B56 sub esp,4.
     * Callee-saved regs become C locals; the esp frame is t_1c. */

    /* 0x00415B59 */ edx = 0; /* xor edx,edx */
    /* 0x00415B5B */ t_1c = edx; /* mov [esp+var_1C],edx */
l_415B5E:
    /* 0x00415B5E */ eax = t_1c; /* mov eax,[esp+var_1C] */
    /* 0x00415B61 */ edi = (unsigned long)G32(0x433B44); /* mov edi,ds:dword_433B44 */
    /* 0x00415B67 */ eax = (eax << 5) & U32MASK; /* shl eax,5 */
    /* 0x00415B6A */ edi = (edi + eax) & U32MASK; /* add edi,eax */
    /* 0x00415B6C cmp byte ptr [edi],0 */;
    /* 0x00415B6F jz */ if (M8(edi) == 0) goto l_415BD8; /* equality */
    /* 0x00415B71 mov dl,0Dh + 0x00415B73 xor dh,dh (edx high word is
     * stale across outer iterations but never read: only dl/dh used) */
    edx = (edx & 0xFFFF0000UL) | 0x000DUL;
l_415B75:
    /* 0x00415B75 */ DECB_LO(edx); /* dec dl */
    /* 0x00415B77 */ eax = 0; /* xor eax,eax */
    /* 0x00415B79 */ eax |= (edx & 0xFFUL); /* mov al,dl */
    /* 0x00415B7B */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)M8((edi + eax + 1) & U32MASK); /* mov bl,[edi+eax+1] */
    /* 0x00415B7F */ ebx = (ebx & 0xFFFFFF00UL) | (((ebx & 0xFFUL) >> 2) & 0xFFUL); /* shr bl,2 */
    /* 0x00415B82 */ ADD8_HI(edx, ebx & 0xFFUL); /* add dh,bl */
    /* 0x00415B84 */ M8((edi + eax + 1) & U32MASK) = (unsigned char)(ebx & 0xFFUL); /* mov [edi+eax+1],bl */
    /* 0x00415B88 test dl,dl */;
    /* 0x00415B8A jnz */ if ((edx & 0xFFUL) != 0) goto l_415B75; /* equality */
    /* 0x00415B8C test dh,dh */;
    /* 0x00415B8E jnz */ if (((edx >> 8) & 0xFFUL) != 0) goto l_415C00; /* equality */
    /* 0x00415B90 */ ebx = t_1c; /* mov ebx,[esp+var_1C] */
    /* 0x00415B93 */ ebp = ebx; /* mov ebp,ebx */
    /* 0x00415B95 */ ebp = (ebp << 5) & U32MASK; /* shl ebp,5 */
l_415B98:
    /* 0x00415B98 */ ebx = (ebx + 0x8000UL) & U32MASK; /* add ebx,8000h */
    /* 0x00415B9E */ ebp = (ebp + 0x100000UL) & U32MASK; /* add ebp,100000h */
    /* 0x00415BA4 cmp ebx,40000h */;
    /* 0x00415BAA jnb */ if (ebx >= 0x40000UL) goto l_415C2C; /* unsigned */
    /* 0x00415BB0 */ edx = (unsigned long)G32(0x433B44); /* mov edx,ds:dword_433B44 */
    /* 0x00415BB6 */ edx = (edx + ebp) & U32MASK; /* add edx,ebp */
    /* 0x00415BB8 cmp byte ptr [edx],0 */;
    /* 0x00415BBB jz */ if (M8(edx) == 0) goto l_415C2C; /* equality */
    /* 0x00415BBD */ ecx = 0x20UL; /* mov ecx,20h */
    /* 0x00415BC2 */ esi = edx; /* mov esi,edx */
    /* 0x00415BC4 */ sv_edi = edi; /* push edi */
    /* 0x00415BC5 */ eax = ecx; /* mov eax,ecx */
    /* 0x00415BC7 */ ecx >>= 2; /* shr ecx,2 */
    /* 0x00415BCA repne movsd (F2 prefix ignored for movs: acts as rep) */
    while (ecx != 0) {
        M32(edi) = M32(esi);
        esi = (esi + 4) & U32MASK;
        edi = (edi + 4) & U32MASK;
        ecx = (ecx - 1) & U32MASK;
    }
    /* 0x00415BCC */ ecx = (ecx & 0xFFFFFF00UL) | (eax & 0xFFUL); /* mov cl,al */
    /* 0x00415BCE */ ecx = (ecx & 0xFFFFFF00UL) | ((ecx & 0xFFUL) & 3UL); /* and cl,3 */
    /* 0x00415BD1 repne movsb (count is full ecx == 0 here: 0x20 & 3) */
    while (ecx != 0) {
        M8(edi) = M8(esi);
        esi = (esi + 1) & U32MASK;
        edi = (edi + 1) & U32MASK;
        ecx = (ecx - 1) & U32MASK;
    }
    /* 0x00415BD3 */ edi = sv_edi; /* pop edi */
    /* 0x00415BD4 */ edi = edx; /* mov edi,edx */
    /* 0x00415BD6 jmp */ goto l_415B98; /* unconditional */
l_415BD8:
    /* 0x00415BD8 */ edi = t_1c; /* mov edi,[esp+var_1C] */
    /* 0x00415BDB */ edi &= 0x7FFFUL; /* and edi,7FFFh */
    /* 0x00415BE1 */ t_1c = edi; /* mov [esp+var_1C],edi */
    /* 0x00415BE4 */ ebp = (edi + 1) & U32MASK; /* lea ebp,[edi+1] */
    /* 0x00415BE7 */ t_1c = ebp; /* mov [esp+var_1C],ebp */
    /* 0x00415BEA cmp ebp,8000h */;
    /* 0x00415BF0 jb */ if (ebp < 0x8000UL) goto l_415B5E; /* unsigned */
    /* epilogue 0x00415BF6 add esp,4; 0x00415BF9 pop ebp;
     * 0x00415BFA pop edi; 0x00415BFB pop esi; 0x00415BFC pop edx;
     * 0x00415BFD pop ecx; 0x00415BFE pop ebx; 0x00415BFF retn. */
    return;
l_415C00:
    /* 0x00415C00 */ eax = 0; /* xor eax,eax */
    /* 0x00415C02 */ ecx = (ecx & 0xFFFFFF00UL) | (unsigned long)M8((edi + 0x1BUL) & U32MASK); /* mov cl,[edi+1Bh] */
    /* 0x00415C05 */ eax |= (unsigned long)M8(edi); /* mov al,[edi] */
    /* 0x00415C07 */ ecx = (ecx & 0xFFFFFF00UL) | (((ecx & 0xFFUL) >> 1) & 0xFFUL); /* shr cl,1 */
    /* 0x00415C09 */ eax = d4v_sar(eax, 2); /* sar eax,2 (operand is a zero-extended byte, so == shr) */
    /* 0x00415C0C */ M8((edi + 0x1BUL) & U32MASK) = (unsigned char)(ecx & 0xFFUL); /* mov [edi+1Bh],cl */
    /* 0x00415C0F */ eax = (eax + 1) & U32MASK; /* inc eax */
    /* 0x00415C10 */ M8(edi) = (unsigned char)(eax & 0xFFUL); /* mov [edi],al */
l_415C12:
    /* 0x00415C12 */ ebx = t_1c; /* mov ebx,[esp+var_1C] */
    /* 0x00415C15 */ ebx = (ebx + 0x8000UL) & U32MASK; /* add ebx,8000h */
    /* 0x00415C1B */ t_1c = ebx; /* mov [esp+var_1C],ebx */
    /* 0x00415C1E cmp ebx,40000h */;
    /* 0x00415C24 jb */ if (ebx < 0x40000UL) goto l_415B5E; /* unsigned */
    /* 0x00415C2A jmp */ goto l_415BD8; /* unconditional */
l_415C2C:
    /* 0x00415C2C */ ecx = t_1c; /* mov ecx,[esp+var_1C] */
    /* 0x00415C2F */ ecx = (ecx + 0xFFFF8000UL) & U32MASK; /* add ecx,0FFFF8000h (subtract 8000h) */
    /* 0x00415C35 */ M8(edi) = 0; /* mov byte ptr [edi],0 */
    /* 0x00415C38 */ t_1c = ecx; /* mov [esp+var_1C],ecx */
    /* 0x00415C3B jmp */ goto l_415C12; /* unconditional */
}
/* ------------------------------------------------------------------ */
/* sub_416A00 (lst 33012-33150) -> d4v_416a00                          */
/*                                                                      */
/* Role: small PPM-context helper. Halves (round-half-up per u16) the   */
/* 0x200-byte plane at [0x433B7C]+idx*0x200 into itself while summing   */
/* all halved dwords into ebp; copies 0x1FE bytes [0x433B7C]+idx*0x200  */
/* to [0x433B78]+idx*0x200+2; folds the sum (lo16+hi16) as a u16 at     */
/* [0x433B78]+idx*0x200; rebuilds a doubling prefix structure over the  */
/* [0x433B78] bytes; for idx < 0x2FF touches the u16 at                 */
/* [0x433B84]+idx*2 (((old >> 2) + 1)).                                 */
/* Entry-live regs: eax = idx (saved to var_1C, then edx=eax, ebx=eax; */
/* no stack args). Returns void (callers at 4165AD/416C65/416C71 ignore */
/* eax; both exits leave callee residue in eax).                       */
/* Frame: 4 bytes (var_1C @ sp+0) + 6 pushed regs.                     */
/* Globals touched: 433B78, 433B7C, 433B84.                            */
/* ------------------------------------------------------------------ */
void d4v_416a00(dec4v_ctx *c, unsigned int idx)
{
    unsigned long eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned long t_1c = 0;

    /* prologue 0x00416A00 push ebx; 0x00416A01 push ecx;
     * 0x00416A02 push edx; 0x00416A03 push esi; 0x00416A04 push edi;
     * 0x00416A05 push ebp; 0x00416A06 sub esp,4.
     * Callee-saved regs become C locals; the esp frame is t_1c. */

    /* 0x00416A09 */ t_1c = (unsigned long)idx; /* mov [esp+var_1C],eax */
    /* 0x00416A0C */ edx = (unsigned long)idx; /* mov edx,eax */
    /* 0x00416A0E */ ebx = (unsigned long)idx; /* mov ebx,eax */
    /* 0x00416A10 */ eax = (unsigned long)G32(0x433B7C); /* mov eax,ds:dword_433B7C */
    /* 0x00416A15 */ ebx = (ebx << 9) & U32MASK; /* shl ebx,9 */
    /* 0x00416A18 */ ebp = 0; /* xor ebp,ebp */
    /* 0x00416A1A */ ebx = (ebx + eax) & U32MASK; /* add ebx,eax */
    /* 0x00416A1C */ edx = (edx << 8) & U32MASK; /* shl edx,8 */
    /* 0x00416A1F */ edi = (ebx + 0x200UL) & U32MASK; /* lea edi,[ebx+200h] */
l_416A25:
    /* 0x00416A25 */ ecx = (unsigned long)G32((ebx + 0) & U32MASK); /* mov ecx,[ebx] */
    /* 0x00416A27 */ eax = (unsigned long)G32((ebx + 4) & U32MASK); /* mov eax,[ebx+4] */
    /* 0x00416A2A */ ecx = (ecx + 0x10001UL) & U32MASK; /* add ecx,10001h */
    /* 0x00416A30 */ eax = (eax + 0x10001UL) & U32MASK; /* add eax,10001h */
    /* 0x00416A35 */ ecx &= 0xFFFEFFFEUL; /* and ecx,0FFFEFFFEh */
    /* 0x00416A3B */ eax &= 0xFFFEFFFEUL; /* and eax,0FFFEFFFEh */
    /* 0x00416A40 */ ecx >>= 1; /* shr ecx,1 */
    /* 0x00416A42 */ eax >>= 1; /* shr eax,1 */
    /* 0x00416A44 */ ebx = (ebx + 0x10UL) & U32MASK; /* add ebx,10h */
    /* 0x00416A47 */ esi = (ecx + eax) & U32MASK; /* lea esi,[ecx+eax] */
    /* 0x00416A4A */ M32((ebx - 0x0CUL) & U32MASK) = (unsigned int)eax; /* mov [ebx-0Ch],eax */
    /* 0x00416A4D */ M32((ebx - 0x10UL) & U32MASK) = (unsigned int)ecx; /* mov [ebx-10h],ecx */
    /* 0x00416A50 */ eax = (unsigned long)G32((ebx - 4) & U32MASK); /* mov eax,[ebx-4] */
    /* 0x00416A53 */ ecx = (unsigned long)G32((ebx - 8) & U32MASK); /* mov ecx,[ebx-8] */
    /* 0x00416A56 */ eax = (eax + 0x10001UL) & U32MASK; /* add eax,10001h */
    /* 0x00416A5B */ ecx = (ecx + 0x10001UL) & U32MASK; /* add ecx,10001h */
    /* 0x00416A61 */ eax &= 0xFFFEFFFEUL; /* and eax,0FFFEFFFEh */
    /* 0x00416A66 */ ecx &= 0xFFFEFFFEUL; /* and ecx,0FFFEFFFEh */
    /* 0x00416A6C */ eax >>= 1; /* shr eax,1 */
    /* 0x00416A6E */ ecx >>= 1; /* shr ecx,1 */
    /* 0x00416A70 */ ebp = (ebp + esi) & U32MASK; /* add ebp,esi (first-pair sum) */
    /* 0x00416A72 */ esi = (ecx + eax) & U32MASK; /* lea esi,[ecx+eax] */
    /* 0x00416A75 */ M32((ebx - 4) & U32MASK) = (unsigned int)eax; /* mov [ebx-4],eax */
    /* 0x00416A78 */ ebp = (ebp + esi) & U32MASK; /* add ebp,esi (second-pair sum) */
    /* 0x00416A7A */ M32((ebx - 8) & U32MASK) = (unsigned int)ecx; /* mov [ebx-8],ecx */
    /* 0x00416A7D cmp ebx,edi */;
    /* 0x00416A7F jnz */ if (ebx != edi) goto l_416A25; /* equality */
    /* 0x00416A81 */ ecx = 0x1FEUL; /* mov ecx,1FEh */
    /* 0x00416A86 */ ebx = (edx + edx) & U32MASK; /* lea ebx,[edx+edx] */
    /* 0x00416A89 */ eax = (unsigned long)G32(0x433B78); /* mov eax,ds:dword_433B78 */
    /* 0x00416A8E */ edi = (ebx + 2) & U32MASK; /* lea edi,[ebx+2] */
    /* 0x00416A91 */ esi = (unsigned long)G32(0x433B7C); /* mov esi,ds:dword_433B7C */
    /* 0x00416A97 */ edi = (edi + eax) & U32MASK; /* add edi,eax */
    /* 0x00416A99 */ esi = (esi + ebx) & U32MASK; /* add esi,ebx */
    /* 0x00416A9B cld (forward moves assumed; no std in function) */;
    /* 0x00416A9C */ eax = ecx; /* mov eax,ecx */
    /* 0x00416A9E */ ecx = (ecx - edi) & U32MASK; /* sub ecx,edi */
    /* 0x00416AA0 */ ecx = (ecx - eax) & U32MASK; /* sub ecx,eax */
    /* 0x00416AA2 */ ecx &= 3UL; /* and ecx,3 */
    /* 0x00416AA5 */ eax = (eax - ecx) & U32MASK; /* sub eax,ecx */
    /* 0x00416AA7 jle */ if ((long)eax <= 0) goto l_416AB5; /* signed */
    /* 0x00416AA9 rep movsb */
    while (ecx != 0) {
        M8(edi) = M8(esi);
        esi = (esi + 1) & U32MASK;
        edi = (edi + 1) & U32MASK;
        ecx = (ecx - 1) & U32MASK;
    }
    /* 0x00416AAB */ ecx = eax; /* mov ecx,eax */
    /* 0x00416AAD */ eax &= 3UL; /* and eax,3 */
    /* 0x00416AB0 */ ecx >>= 2; /* shr ecx,2 */
    /* 0x00416AB3 rep movsd */
    while (ecx != 0) {
        M32(edi) = M32(esi);
        esi = (esi + 4) & U32MASK;
        edi = (edi + 4) & U32MASK;
        ecx = (ecx - 1) & U32MASK;
    }
l_416AB5:
    /* 0x00416AB5 */ ecx = (ecx + eax) & U32MASK; /* add ecx,eax */
    /* 0x00416AB7 rep movsb */
    while (ecx != 0) {
        M8(edi) = M8(esi);
        esi = (esi + 1) & U32MASK;
        edi = (edi + 1) & U32MASK;
        ecx = (ecx - 1) & U32MASK;
    }
    /* 0x00416AB9 */ ecx = ebp; /* mov ecx,ebp */
    /* 0x00416ABB */ eax = ebp; /* mov eax,ebp */
    /* 0x00416ABD */ ecx &= 0xFFFFUL; /* and ecx,0FFFFh */
    /* 0x00416AC3 */ eax >>= 0x10; /* shr eax,10h */
    /* 0x00416AC6 */ eax = (eax + ecx) & U32MASK; /* add eax,ecx */
    /* 0x00416AC8 */ ecx = eax; /* mov ecx,eax */
    /* 0x00416ACA */ eax = (unsigned long)G32(0x433B78); /* mov eax,ds:dword_433B78 */
    /* 0x00416ACF */ M16((ebx + eax) & U32MASK) = (unsigned short)(ecx & 0xFFFFUL); /* mov [ebx+eax],cx */
    /* 0x00416AD3 */ ebx = (ebx & 0xFFFF00FFUL) | (1UL << 8); /* mov bh,1 */
l_416AD5:
    /* 0x00416AD5 */ ebx = (ebx & 0xFFFFFF00UL) | (((ebx >> 8) & 0xFFUL) << 0); /* mov bl,bh */
    /* 0x00416AD7 */ edx &= 0xFFFFFF00UL; /* mov dl,0 */
    /* 0x00416AD9 */ ebx = (ebx & 0xFFFFFF00UL) | (((ebx & 0xFFUL) + ((ebx >> 8) & 0xFFUL)) & 0xFFUL); /* add bl,bh */
    /* 0x00416ADB */ edx = (edx & 0xFFFFFF00UL) | ((0UL - (ebx & 0xFFUL)) & 0xFFUL); /* sub dl,bl */
    /* 0x00416ADD */ ecx = (unsigned long)G32(0x433B78); /* mov ecx,ds:dword_433B78 */
    /* 0x00416AE3 */ eax = (edx + edx) & U32MASK; /* lea eax,[edx+edx] */
    /* 0x00416AE6 */ eax = (eax + ecx) & U32MASK; /* add eax,ecx */
l_416AE8:
    /* 0x00416AE8 */ ecx = 0; /* xor ecx,ecx */
    /* 0x00416AEA */ ecx |= ((ebx >> 8) & 0xFFUL); /* mov cl,bh */
    /* 0x00416AEC */ esi = eax; /* mov esi,eax */
    /* 0x00416AEE */ ecx = (ecx + ecx) & U32MASK; /* add ecx,ecx */
    /* 0x00416AF0 */ esi = (esi - ecx) & U32MASK; /* sub esi,ecx */
    /* 0x00416AF2 */ ecx = (ecx & 0xFFFF0000UL) | (unsigned long)M16(esi); /* mov cx,[esi] */
    /* 0x00416AF5 */ esi = (esi & 0xFFFF0000UL) | (unsigned long)M16(eax); /* mov si,[eax] */
    /* 0x00416AF8 */ esi = (esi + ecx) & U32MASK; /* add esi,ecx (low word used) */
    /* 0x00416AFA */ ecx = 0; /* xor ecx,ecx */
    /* 0x00416AFC */ ecx |= (ebx & 0xFFUL); /* mov cl,bl */
    /* 0x00416AFE */ ecx = (ecx + ecx) & U32MASK; /* add ecx,ecx */
    /* 0x00416B00 */ M16(eax) = (unsigned short)(esi & 0xFFFFUL); /* mov [eax],si */
    /* 0x00416B03 */ eax = (eax - ecx) & U32MASK; /* sub eax,ecx */
    /* 0x00416B05 */ edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFUL) - (ebx & 0xFFUL)) & 0xFFUL); /* sub dl,bl */
    /* 0x00416B07 jnz */ if ((edx & 0xFFUL) != 0) goto l_416AE8; /* equality */
    /* 0x00416B09 */ ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) & 0xFFUL) + ((ebx >> 8) & 0xFFUL)) & 0xFFUL) << 8; /* add bh,bh */
    /* 0x00416B0B cmp bh,80h */;
    /* 0x00416B0E jb */ if (((ebx >> 8) & 0xFFUL) < 0x80UL) goto l_416AD5; /* unsigned */
    /* 0x00416B10 */ edx = t_1c; /* mov edx,[esp+var_1C] */
    /* 0x00416B13 cmp edx,2FFh */;
    /* 0x00416B19 jb */ if (edx < 0x2FFUL) goto l_416B25; /* unsigned */
    /* epilogue 0x00416B1B add esp,4; 0x00416B1E pop ebp;
     * 0x00416B1F pop edi; 0x00416B20 pop esi; 0x00416B21 pop edx;
     * 0x00416B22 pop ecx; 0x00416B23 pop ebx; 0x00416B24 retn. */
    return;
l_416B25:
    /* 0x00416B25 */ eax = (unsigned long)G32(0x433B84); /* mov eax,ds:dword_433B84 */
    /* 0x00416B2A */ edi = (edi & 0xFFFF0000UL) | (unsigned long)M16((eax + edx * 2) & U32MASK); /* mov di,[eax+edx*2] */
    /* 0x00416B2E */ edi = (edi & 0xFFFF0000UL) | (((edi & 0xFFFFUL) >> 2) & 0xFFFFUL); /* shr di,2 */
    /* 0x00416B32 */ ebx = edi; /* mov ebx,edi */
    /* 0x00416B34 */ M16((eax + edx * 2) & U32MASK) = (unsigned short)(edi & 0xFFFFUL); /* mov [eax+edx*2],di */
    /* 0x00416B38 */ ebx = (ebx + 1) & U32MASK; /* inc ebx */
    /* 0x00416B39 */ M16((eax + edx * 2) & U32MASK) = (unsigned short)(ebx & 0xFFFFUL); /* mov [eax+edx*2],bx (overwrites previous store) */
    /* epilogue 0x00416B3D add esp,4; 0x00416B40 pop ebp;
     * 0x00416B41 pop edi; 0x00416B42 pop esi; 0x00416B43 pop edx;
     * 0x00416B44 pop ecx; 0x00416B45 pop ebx; 0x00416B46 retn. */
}

/* ------------------------------------------------------------------ */
/* sub_416B50 (lst 33156-33283) -> d4v_416b50                          */
/*                                                                      */
/* Role: PPM order-N update helper. Indexes the u16 plane at [0x433B7C] */
/* by (arg*0x100+sym), prices against word_45BA5E/word_45C05C side      */
/* tables, accumulates into the [0x433B78] prefix plane (lowest-set-bit */
/* stepping) and the [0x433B84] totals; rescales via d4v_416a00 when a  */
/* total reaches 0x4000.                                                */
/* Entry-live regs: al = sym (mov bl,al), edx = arg (mov ebp,edx and   */
/* mov eax,edx; no stack args). Verified at all four call sites        */
/* (4182FB/419815/41C183/41CC66: al=sym byte, edx=dword arg). Returns  */
/* void (callers ignore eax; exits leave callee residue).              */
/* Frame: 8 bytes (var_1C @ sp+0, var_18 @ sp+4) + 5 pushed regs.      */
/* Globals touched: 433B78, 433B7C, 433B84, 45AFA4, 45BA5E, 45C05C.     */
/* Calls sub_416A00 as d4v_416a00 (eax = rescale index).                */
/* ------------------------------------------------------------------ */
void d4v_416b50(dec4v_ctx *c, unsigned int sym, unsigned int arg)
{
    unsigned long eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned long t_1c = 0, t_18 = 0;

    /* prologue 0x00416B50 push ebx; 0x00416B51 push ecx;
     * 0x00416B52 push esi; 0x00416B53 push edi; 0x00416B54 push ebp;
     * 0x00416B55 sub esp,8.
     * Callee-saved regs become C locals; the esp frame is t_1c/t_18. */

    /* 0x00416B58 */ ebx = (ebx & 0xFFFFFF00UL) | ((unsigned long)sym & 0xFFUL); /* mov bl,al */    /* 0x00416B5A */ ebp = (unsigned long)arg; /* mov ebp,edx */
    /* 0x00416B5C */ eax = (unsigned long)arg; /* mov eax,edx */
    /* 0x00416B5E */ edx = 0; /* xor edx,edx */
    /* 0x00416B60 */ eax = (eax << 8) & U32MASK; /* shl eax,8 */
    /* 0x00416B63 */ edx |= (ebx & 0xFFUL); /* mov dl,bl */
    /* 0x00416B65 */ eax = (eax + edx) & U32MASK; /* add eax,edx (arg*0x100+sym) */
    /* 0x00416B67 */ esi = (unsigned long)G32(0x433B7C); /* mov esi,ds:dword_433B7C */
    /* 0x00416B6D */ eax = (eax + eax) & U32MASK; /* add eax,eax */
    /* 0x00416B6F */ ecx = (esi + eax) & U32MASK; /* lea ecx,[esi+eax] */
    /* 0x00416B72 */ eax = (ebp + ebp) & U32MASK; /* lea eax,[ebp+ebp+0] (arg*2) */
    /* 0x00416B76 */ edi = (edi & 0xFFFF0000UL) | (unsigned long)G16((0x45BA5EUL + eax) & U32MASK); /* mov di,ds:word_45BA5E[eax] */
    /* 0x00416B7D */ t_1c = edi & 0xFFFFUL; /* mov [esp+var_1C],edi (word) */
    /* 0x00416B80 cmp word ptr [ecx],0 */;
    /* 0x00416B84 jz */ if (M16(ecx) == 0) goto l_416BF2; /* equality */
l_416B86:
    /* 0x00416B86 cmp ebp,200h */;
    /* 0x00416B8C jz */ if (ebp == 0x200UL) goto l_416BE9; /* equality */
    /* 0x00416B8E */ eax = t_1c; /* mov eax,[esp+var_1C] */
    /* 0x00416B91 */ edx = (edx & 0xFFFF0000UL) | (unsigned long)M16(ecx); /* mov dx,[ecx] */
    /* 0x00416B94 */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x00416B96 */ eax = ebp; /* mov eax,ebp */
    /* 0x00416B98 */ M16(ecx) = (unsigned short)(edx & 0xFFFFUL); /* mov [ecx],dx */
    /* 0x00416B9B */ ecx = (unsigned long)G32(0x433B78); /* mov ecx,ds:dword_433B78 */
    /* 0x00416BA1 */ eax = (eax << 9) & U32MASK; /* shl eax,9 */
    /* 0x00416BA4 */ ecx = (ecx + eax) & U32MASK; /* add ecx,eax */
    /* 0x00416BA6 */ ebx = (ebx & 0xFFFFFF00UL) | (((ebx & 0xFFUL) + 1) & 0xFFUL); /* inc bl */
    /* 0x00416BA8 jz */ if ((ebx & 0xFFUL) == 0) goto l_416BBD; /* equality (bl wrapped) */
    /* 0x00416BAA */ edx = t_1c; /* mov edx,[esp+var_1C] */
l_416BAD:
    /* 0x00416BAD */ eax = 0; /* xor eax,eax */
    /* 0x00416BAF */ eax |= (ebx & 0xFFUL); /* mov al,bl */
    /* 0x00416BB1 */ M16((ecx + eax * 2) & U32MASK) = (unsigned short)((M16((ecx + eax * 2) & U32MASK) + (edx & 0xFFFFUL)) & 0xFFFFUL); /* add [ecx+eax*2],dx */
    /* 0x00416BB5 */ eax = (eax & 0xFFFFFF00UL) | ((0UL - (eax & 0xFFUL)) & 0xFFUL); /* neg al */
    /* 0x00416BB7 */ eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & (ebx & 0xFFUL)); /* and al,bl */
    /* 0x00416BB9 */ ebx = (ebx & 0xFFFFFF00UL) | (((ebx & 0xFFUL) + (eax & 0xFFUL)) & 0xFFUL); /* add bl,al (lowest-set-bit step) */
    /* 0x00416BBB jnz */ if ((ebx & 0xFFUL) != 0) goto l_416BAD; /* equality */
l_416BBD:
    /* 0x00416BBD */ edx = (unsigned long)G32(0x433B84); /* mov edx,ds:dword_433B84 */
    /* 0x00416BC3 */ eax = (ebp + ebp) & U32MASK; /* lea eax,[ebp+ebp+0] */
    /* 0x00416BC7 */ eax = (eax + edx) & U32MASK; /* add eax,edx */
    /* 0x00416BC9 */ edx = 0; /* xor edx,edx */
    /* 0x00416BCB */ edi = (edi & 0xFFFF0000UL) | (unsigned long)M16(ecx); /* mov di,[ecx] */
    /* 0x00416BCE */ edx = (edx & 0xFFFF0000UL) | (unsigned long)M16(eax); /* mov dx,[eax] */
    /* 0x00416BD1 */ edi = (edi + t_1c) & U32MASK; /* add edi,[esp+var_1C] */
    /* 0x00416BD4 */ eax = 0; /* xor eax,eax */
    /* 0x00416BD6 */ eax |= (edi & 0xFFFFUL); /* mov ax,di */
    /* 0x00416BD9 */ eax = (eax + edx) & U32MASK; /* add eax,edx */
    /* 0x00416BDB */ M16(ecx) = (unsigned short)(edi & 0xFFFFUL); /* mov [ecx],di */
    /* 0x00416BDE cmp eax,4000h */;
    /* 0x00416BE3 jge */ if ((long)eax >= 0x4000L) goto l_416C6F; /* signed */
l_416BE9:
    /* epilogue 0x00416BE9 add esp,8; 0x00416BEC pop ebp;
     * 0x00416BED pop edi; 0x00416BEE pop esi; 0x00416BEF pop ecx;
     * 0x00416BF0 pop ebx; 0x00416BF1 retn. */
    return;
l_416BF2:
    /* 0x00416BF2 */ edi = (edi & 0xFFFF0000UL) | (unsigned long)G16((0x45C05CUL + eax) & U32MASK); /* mov di,ds:word_45C05C[eax] */
    /* 0x00416BF9 */ t_18 = edi & 0xFFFFUL; /* mov [esp+var_18],edi (word) */
    /* 0x00416BFD */ edi = (unsigned long)G32(0x433B84); /* mov edi,ds:dword_433B84 */
    /* 0x00416C03 */ eax = (eax + edi) & U32MASK; /* add eax,edi */
    /* 0x00416C05 */ edi = t_1c; /* mov edi,[esp+var_1C] */
    /* 0x00416C08 */ M16(eax) = (unsigned short)((M16(eax) + (edi & 0xFFFFUL)) & 0xFFFFUL); /* add [eax],di */
    /* 0x00416C0B */ eax = (unsigned long)G32(0x45AFA4); /* mov eax,ds:dword_45AFA4 */
    /* 0x00416C10 */ eax = (eax << 8) & U32MASK; /* shl eax,8 */
    /* 0x00416C13 */ eax = (eax + edx) & U32MASK; /* add eax,edx (edx = sym byte here) */
    /* 0x00416C15 */ edx = t_18; /* mov edx,[esp+var_18] */
    /* 0x00416C19 */ M16((esi + eax * 2) & U32MASK) = (unsigned short)((M16((esi + eax * 2) & U32MASK) + (edx & 0xFFFFUL)) & 0xFFFFUL); /* add [esi+eax*2],dx */
    /* 0x00416C1D */ esi = (unsigned long)G32(0x45AFA4); /* mov esi,ds:dword_45AFA4 */
    /* 0x00416C23 */ eax = (unsigned long)G32(0x433B78); /* mov eax,ds:dword_433B78 */
    /* 0x00416C28 */ esi = (esi << 9) & U32MASK; /* shl esi,9 */
    /* 0x00416C2B */ esi = (esi + eax) & U32MASK; /* add esi,eax */
    /* 0x00416C2D */ eax = (eax & 0xFFFFFF00UL) | (ebx & 0xFFUL); /* mov al,bl */
    /* 0x00416C2F */ eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFUL) + 1) & 0xFFUL); /* inc al */
    /* 0x00416C31 jz */ if ((eax & 0xFFUL) == 0) goto l_416C49; /* equality (al wrapped) */
    /* 0x00416C33 */ edi = t_18; /* mov edi,[esp+var_18] */
l_416C37:
    /* 0x00416C37 */ edx = 0; /* xor edx,edx */
    /* 0x00416C39 */ edx |= (eax & 0xFFUL); /* mov dl,al */
    /* 0x00416C3B */ eax = (eax & 0xFFFF00FFUL) | ((eax & 0xFFUL) << 8); /* mov ah,al */
    /* 0x00416C3D */ eax = (eax & 0xFFFF00FFUL) | ((((0UL - ((eax >> 8) & 0xFFUL))) & 0xFFUL) << 8); /* neg ah */
    /* 0x00416C3F */ eax = (eax & 0xFFFF00FFUL) | (((((eax >> 8) & 0xFFUL) & (eax & 0xFFUL))) << 8); /* and ah,al */
    /* 0x00416C41 */ M16((esi + (edx & 0xFFUL) * 2) & U32MASK) = (unsigned short)((M16((esi + (edx & 0xFFUL) * 2) & U32MASK) + (edi & 0xFFFFUL)) & 0xFFFFUL); /* add [esi+edx*2],di */
    /* 0x00416C45 */ eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFUL) + ((eax >> 8) & 0xFFUL)) & 0xFFUL); /* add al,ah (lowest-set-bit step) */
    /* 0x00416C47 jnz */ if ((eax & 0xFFUL) != 0) goto l_416C37; /* equality */
l_416C49:
    /* 0x00416C49 */ eax = t_18; /* mov eax,[esp+var_18] */
    /* 0x00416C4D */ edx = (edx & 0xFFFF0000UL) | (unsigned long)M16(esi); /* mov dx,[esi] */
    /* 0x00416C50 */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x00416C52 */ M16(esi) = (unsigned short)(edx & 0xFFFFUL); /* mov [esi],dx */
    /* 0x00416C55 cmp dx,4000h */;
    /* 0x00416C5A jb */ if ((edx & 0xFFFFUL) < 0x4000UL) goto l_416B86; /* unsigned */
    /* 0x00416C60 */ eax = (unsigned long)G32(0x45AFA4); /* mov eax,ds:dword_45AFA4 */
    /* 0x00416C65 */ d4v_416a00(c, (unsigned int)eax); /* call sub_416A00, eax=rescale index */
    /* 0x00416C6A jmp */ goto l_416B86; /* unconditional */
l_416C6F:
    /* 0x00416C6F */ eax = ebp; /* mov eax,ebp */
    /* 0x00416C71 */ d4v_416a00(c, (unsigned int)eax); /* call sub_416A00, eax=rescale index */
    /* epilogue 0x00416C76 add esp,8; 0x00416C79 pop ebp;
     * 0x00416C7A pop edi; 0x00416C7B pop esi; 0x00416C7C pop ecx;
     * 0x00416C7D pop ebx; 0x00416C7E retn. */
}
/* ------------------------------------------------------------------ */
/* sub_418780 (lst 35765-35840) -> d4v_418780                          */
/*                                                                      */
/* Role: tiny PPM param setter. Treats entry eax as a 2-byte object    */
/* (counts at [obj]/[obj+1]); prices sum=[0]+[1] against 45D0BC/45D0B8 */
/* quotients, records the al outcome into 45AF6C/45AF68/45AF70, bumps  */
/* one lane, and halves both bytes when either reaches 0xFE.           */
/* Entry-live regs: eax = obj, an arena offset (deref [eax]/[eax+1];   */
/* verified at both call sites: 418C3A builds an arena pointer, 418F78 */
/* builds one from unk_45AA80). Returns al in eax (0/1 compare result; */
/* consumed via test al,al at 418C3F/418F7D).                          */
/* Frame: no locals; 3 pushed regs (ebx/ecx/edx).                      */
/* Globals touched: 45AF68, 45AF6C, 45AF70, 45D0B8, 45D0BC.             */
/* ------------------------------------------------------------------ */
unsigned int d4v_418780(dec4v_ctx *c, unsigned int obj)
{
    unsigned long eax = 0, ebx = 0, ecx = 0, edx = 0;

    /* prologue 0x00418780 push ebx; 0x00418781 push ecx;
     * 0x00418782 push edx.
     * Callee-saved regs become C locals; there is no esp frame. */

    /* 0x00418783 */ eax = (unsigned long)obj; /* entry eax=obj (kept: 418787 reads [eax+1] before xor) */
    /* 0x00418783 */ ebx = (unsigned long)obj; /* mov ebx,eax */
    /* 0x00418785 */ ecx = 0; /* xor ecx,ecx */
    /* 0x00418787 */ ecx |= (unsigned long)M8((eax + 1) & U32MASK); /* mov cl,[eax+1] */
    /* 0x0041878A */ eax = 0; /* xor eax,eax */
    /* 0x0041878C */ eax |= (unsigned long)M8(ebx); /* mov al,[ebx] */
    /* 0x0041878E */ eax = (eax + ecx) & U32MASK; /* add eax,ecx */
    /* 0x00418790 */ ecx = eax; /* mov ecx,eax */
    /* 0x00418792 */ G32(0x45AF68) = (unsigned int)eax; /* mov ds:dword_45AF68,eax */
    /* 0x00418797 */ eax = (unsigned long)G32(0x45D0BC); /* mov eax,ds:dword_45D0BC */
    /* 0x0041879C */ edx = 0; /* xor edx,edx */
    /* 0x0041879E div ecx (edx high half is zero: direct 32-bit divide;
     * faults on a zero divisor exactly like x86) */
    eax = eax / ecx;
    edx = 0; /* 0x004187A0 xor edx,edx (div remainder discarded) */
    /* 0x004187A2 */ G32(0x45D0BC) = (unsigned int)eax; /* mov ds:dword_45D0BC,eax */
    /* 0x004187A7 */ ecx = eax; /* mov ecx,eax */
    /* 0x004187A9 */ eax = (unsigned long)G32(0x45D0B8); /* mov eax,ds:dword_45D0B8 */
    /* 0x004187AE div ecx (edx high half is zero: direct 32-bit divide;
     * faults on a zero divisor exactly like x86; remainder in edx is
     * dead: never read afterwards) */
    eax = eax / ecx;
    /* 0x004187B0 */ ecx = 0; /* xor ecx,ecx */
    /* 0x004187B2 */ ecx |= (unsigned long)M8(ebx); /* mov cl,[ebx] */
    /* 0x004187B4 */ eax &= 0xFFFFUL; /* and eax,0FFFFh */
    /* 0x004187B9 cmp eax,ecx */;
    /* 0x004187BB setl ah */ eax = (eax & 0xFFFF00FFUL) | (((long)eax < (long)(ecx & 0xFFFFUL) ? 1UL : 0UL) << 8); /* signed */
    /* 0x004187BE */ eax = (eax & 0xFFFFFF00UL) | ((eax >> 8) & 0xFFUL); /* mov al,ah */
    /* 0x004187C0 test ah,ah */;
    /* 0x004187C2 jnz */ if (((eax >> 8) & 0xFFUL) != 0) goto l_4187F1; /* equality */
    /* 0x004187C4 */ ecx = 0; /* xor ecx,ecx */
    /* 0x004187C6 */ eax = (eax & 0xFFFF00FFUL) | ((unsigned long)M8((ebx + 1) & U32MASK) << 8); /* mov ah,[ebx+1] (eax lane: opcode 8A 63 01) */
    /* 0x004187C9 */ ecx |= (unsigned long)M8(ebx); /* mov cl,[ebx] */
    /* 0x004187CB */ eax = (eax & 0xFFFF00FFUL) | (((((eax >> 8) & 0xFFUL) + 1) & 0xFFUL) << 8); /* inc ah (eax lane: opcode FE C4) */
    /* 0x004187CD */ G32(0x45AF6C) = (unsigned int)(ecx & 0xFFUL); /* mov ds:dword_45AF6C,ecx (low byte; high bytes of ecx are zero) */
    /* 0x004187D3 */ ecx = (unsigned long)G32(0x45AF68); /* mov ecx,ds:dword_45AF68 */
    /* 0x004187D9 */ M8((ebx + 1) & U32MASK) = (unsigned char)((eax >> 8) & 0xFFUL); /* mov [ebx+1],ah (eax lane: opcode 88 63 01) */
    /* 0x004187DC */ G32(0x45AF70) = (unsigned int)ecx; /* mov ds:dword_45AF70,ecx */
l_4187E2:
    /* 0x004187E2 cmp byte ptr [ebx],0FEh */;
    /* 0x004187E5 jnb */ if (M8(ebx) >= 0xFEU) goto l_41880B; /* unsigned */
    /* 0x004187E7 cmp byte ptr [ebx+1],0FEh */;
    /* 0x004187EB jnb */ if (M8((ebx + 1) & U32MASK) >= 0xFEU) goto l_41880B; /* unsigned */
    /* epilogue 0x004187ED pop edx; 0x004187EE pop ecx;
     * 0x004187EF pop ebx; 0x004187F0 retn (eax = al compare result). */
    return (unsigned int)(eax & 0xFFUL);
l_4187F1:
    /* 0x004187F1 */ ecx = 0; /* xor ecx,ecx */
    /* 0x004187F3 */ edx = 0; /* xor edx,edx */
    /* 0x004187F5 */ ecx |= (unsigned long)M8(ebx); /* mov cl,[ebx] */
    /* 0x004187F7 */ G32(0x45AF6C) = (unsigned int)edx; /* mov ds:dword_45AF6C,edx (zero) */
    /* 0x004187FD */ edx = (edx & 0xFFFFFF00UL) | (ecx & 0xFFUL); /* mov dl,cl */
    /* 0x004187FF */ edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFUL) + 1) & 0xFFUL); /* inc dl */
    /* 0x00418801 */ G32(0x45AF70) = (unsigned int)(ecx & 0xFFUL); /* mov ds:dword_45AF70,ecx (low byte; high bytes of ecx are zero) */
    /* 0x00418807 */ M8(ebx) = (unsigned char)(edx & 0xFFUL); /* mov [ebx],dl */
    /* 0x00418809 jmp */ goto l_4187E2; /* unconditional */
l_41880B:
    /* 0x0041880B */ ecx = (ecx & 0xFFFF00FFUL) | ((unsigned long)M8(ebx) << 8); /* mov ch,[ebx] */
    /* 0x0041880D */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8((ebx + 1) & U32MASK); /* mov dl,[ebx+1] */
    /* 0x00418810 */ ecx = (ecx & 0xFFFF00FFUL) | ((((ecx >> 8) & 0xFFUL) + 1) & 0xFFUL) << 8; /* inc ch */
    /* 0x00418812 */ edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFUL) + 1) & 0xFFUL); /* inc dl */
    /* 0x00418814 */ M8(ebx) = (unsigned char)((ecx >> 8) & 0xFFUL); /* mov [ebx],ch */
    /* 0x00418816 */ eax = (eax & 0xFFFF00FFUL) | ((((ecx >> 8) & 0xFFUL)) << 8); /* mov ah,ch */
    /* 0x00418818 */ M8((ebx + 1) & U32MASK) = (unsigned char)(edx & 0xFFUL); /* mov [ebx+1],dl */
    /* 0x0041881B */ edx = (edx & 0xFFFF00FFUL) | ((edx & 0xFFUL) << 8); /* mov dh,dl */
    /* 0x0041881D */ eax = (eax & 0xFFFF00FFUL) | ((((eax >> 8) & 0xFFUL) >> 1) & 0xFFUL) << 8; /* shr ah,1 */
    /* 0x0041881F */ edx = (edx & 0xFFFF00FFUL) | ((((edx >> 8) & 0xFFUL) >> 1) & 0xFFUL) << 8; /* shr dh,1 */
    /* 0x00418821 */ M8(ebx) = (unsigned char)((eax >> 8) & 0xFFUL); /* mov [ebx],ah */
    /* 0x00418823 */ M8((ebx + 1) & U32MASK) = (unsigned char)((edx >> 8) & 0xFFUL); /* mov [ebx+1],dh */
    /* epilogue 0x00418826 pop edx; 0x00418827 pop ecx;
     * 0x00418828 pop ebx; 0x00418829 retn (eax = al compare result). */
    return (unsigned int)(eax & 0xFFUL);
}
/* ------------------------------------------------------------------ */
/* sub_416D10 (lst 33352-33644) -> d4v_416d10                          */
/*                                                                      */
/* Role: PPM exclusion/counter update. Folds the 45BA18/45BA1C shift    */
/* registers plus the 45AF7C/433B40/45BA34/45BA3C context bytes into    */
/* the 459E38..459E54 accumulators, prices candidate orders 0..7 with   */
/* an idiv-based cost (best in bh, best cost in ecx), plants a          */
/* 45AE80-derived selector into 45BA4C for the winning order/length    */
/* class, and returns.                                                  */
/* Entry-live regs: EBX (mov bh,8 is a partial write; residue feeds bl/bh loop). Others fully written first. Takes ebx_in.    */
/* any read; no stack args). Returns eax: 0 on the exhausted-model     */
/* tail path (41707B xor), otherwise the stale [0x433B58]-derived      */
/* pointer residue of the normal path - both callers consume it        */
/* (418AC4 stores it to var_50; 41B214 stores it to var_54 and tests    */
/* it for zero). Documented as a deviation: the nonzero value is not   */
/* a designed status, just whatever the transliteration leaves in eax. */
/* Frame: 12 bytes (var_24/var_20/var_1C @ sp+0/4/8) + 6 pushed regs.  */
/* Globals touched: 433B40, 433B58, 433B74, 459E38..459E54, 45AF7C,     */
/* 45AFA0, 45AF98, 45BA18, 45BA1C, 45BA4C, 45C681, 45C682, 45C684.       */
/* Deviations: div @417053 and idiv @416EF0 are direct C / with        */
/* comments (zero divisor faults like x86, no guard added).            */
/* ------------------------------------------------------------------ */
unsigned int d4v_416d10(dec4v_ctx *c, unsigned int ebx_in)
{
    unsigned long ebx0 = ebx_in; /* entry ebx (callee-saved, live) */
    unsigned long eax = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned long ebx = ebx0;
    unsigned long t_24 = 0, t_20 = 0, t_1c = 0;

    /* prologue 0x00416D10 push ebx; 0x00416D11 push ecx;
     * 0x00416D12 push edx; 0x00416D13 push esi; 0x00416D14 push edi;
     * 0x00416D15 push ebp; 0x00416D16 sub esp,0Ch.
     * Callee-saved regs become C locals; the esp frame is t_24/t_20/t_1c. */

    /* 0x00416D19 */ ebx = (ebx & 0xFFFF00FFUL) | (8UL << 8); /* mov bh,8 */
    /* 0x00416D1B */ esi = (unsigned long)G32(0x45BA18); /* mov esi,ds:dword_45BA18 */
    /* 0x00416D21 */ edx = (unsigned long)G32(0x45BA18); /* mov edx,ds:dword_45BA18 */
    /* 0x00416D27 */ edi = (unsigned long)G32(0x45BA18); /* mov edi,ds:dword_45BA18 */
    /* 0x00416D2D */ ecx = 0; /* xor ecx,ecx */
    /* 0x00416D2F */ eax = 0; /* xor eax,eax */
    /* 0x00416D31 */ G32(0x45BA4C) = (unsigned int)ecx; /* mov ds:dword_45BA4C,ecx */
    /* 0x00416D37 */ esi >>= 8; /* shr esi,8 */
    /* 0x00416D3A */ edx >>= 0x10; /* shr edx,10h */
    /* 0x00416D3D */ edi &= 0xFFFF0000UL; /* xor di,di */
    /* 0x00416D40 */ ebx &= 0xFFFFFF00UL; /* xor bl,bl */
    /* 0x00416D42 cmp edi,0FFFF0000h */;
    /* 0x00416D48 jnz */ if (edi != 0xFFFF0000UL) goto l_416FB2; /* equality */
    /* 0x00416D4E */ eax = 0x10000UL; /* mov eax,10000h */
l_416D53:
    /* 0x00416D53 */ edi = edx; /* mov edi,edx */
    /* 0x00416D55 */ ebp = (unsigned long)G32(0x45BA1C); /* mov ebp,ds:dword_45BA1C */
    /* 0x00416D5B */ edi &= 0xFF00UL; /* and edi,0FF00h */
    /* 0x00416D61 */ ebp >>= 0x18; /* shr ebp,18h */
    /* 0x00416D64 */ edx &= 0xFFFFUL; /* and edx,0FFFFh */
    /* 0x00416D6A */ G32(0x459E40) = (unsigned int)edx; /* mov ds:dword_459E40,edx */
    /* 0x00416D70 */ ebp = (ebp + edi) & U32MASK; /* add ebp,edi */
    /* 0x00416D72 */ edx = esi; /* mov edx,esi */
    /* 0x00416D74 */ G32(0x459E38) = (unsigned int)ebp; /* mov ds:dword_459E38,ebp */
    /* 0x00416D7A */ edx &= 0xFF00UL; /* and edx,0FF00h */
    /* 0x00416D80 */ ebp = esi; /* mov ebp,esi */
    /* 0x00416D82 */ t_24 = edx; /* mov [esp+var_24],edx */
    /* 0x00416D85 */ ebp &= 0xFFFFUL; /* and ebp,0FFFFh */
    /* 0x00416D8B */ edx = (unsigned long)G32(0x45BA18); /* mov edx,ds:dword_45BA18 */
    /* 0x00416D91 */ G32(0x459E3C) = (unsigned int)ebp; /* mov ds:dword_459E3C,ebp */
    /* 0x00416D97 */ edx &= 0xFFUL; /* and edx,0FFh */
    /* 0x00416D9D */ ebp = t_24; /* mov ebp,[esp+var_24] */
    /* 0x00416DA0 */ ebp = (ebp + edx) & U32MASK; /* add ebp,edx */
    /* 0x00416DA2 */ esi &= 0xFFUL; /* and esi,0FFh */
    /* 0x00416DA8 */ G32(0x459E44) = (unsigned int)ebp; /* mov ds:dword_459E44,ebp */
    /* 0x00416DAE */ ebp = (edi + esi) & U32MASK; /* lea ebp,[edi+esi] */
    /* 0x00416DB1 */ esi = (unsigned long)G32(0x45BA1C); /* mov esi,ds:dword_45BA1C */
    /* 0x00416DB7 */ G32(0x459E48) = (unsigned int)ebp; /* mov ds:dword_459E48,ebp */
    /* 0x00416DBD */ esi &= 0xFFFFUL; /* and esi,0FFFFh */
    /* 0x00416DC3 */ edi = (edi + edx) & U32MASK; /* add edi,edx */
    /* 0x00416DC5 */ esi = (esi + eax) & U32MASK; /* add esi,eax */
    /* 0x00416DC7 */ G32(0x459E4C) = (unsigned int)edi; /* mov ds:dword_459E4C,edi */
    /* 0x00416DCD */ G32(0x459E50) = (unsigned int)esi; /* mov ds:dword_459E50,esi */
    /* 0x00416DD3 cmp edx,0FFh */;
    /* 0x00416DD9 jnz */ if (edx != 0xFFUL) goto l_416DF9; /* equality */
    /* 0x00416DDB */ edx = (edx & 0xFFFF0000UL) | (unsigned long)G16(0x459E3EUL); /* mov dx,word ptr [459E3C+2] */
    /* 0x00416DE2 */ esi = (esi & 0xFFFF0000UL) | (unsigned long)G16(0x459E42UL); /* mov si,word ptr [459E40+2] */
    /* 0x00416DE9 */ edx = (edx + 1) & U32MASK; /* inc edx */
    /* 0x00416DEA */ esi = (esi + 1) & U32MASK; /* inc esi */
    /* 0x00416DEB */ M16(0x459E3EUL) = (unsigned short)(edx & 0xFFFFUL); /* mov word ptr [459E3C+2],dx */
    /* 0x00416DF2 */ M16(0x459E42UL) = (unsigned short)(esi & 0xFFFFUL); /* mov word ptr [459E40+2],si */
l_416DF9:
    /* 0x00416DF9 */ eax = (unsigned long)G32(0x45BA34); /* mov eax,ds:dword_45BA34 */
    /* 0x00416DFE */ esi = (unsigned long)G32(0x45AF7C); /* mov esi,ds:dword_45AF7C */
    /* 0x00416E04 */ edx = (unsigned long)G32(0x45BA3C); /* mov edx,ds:dword_45BA3C */
    /* 0x00416E0A */ eax = (eax - esi) & U32MASK; /* sub eax,esi */
    /* 0x00416E0C */ edx = (edx & eax) & U32MASK; /* and edx,eax */
    /* 0x00416E0E */ eax = (unsigned long)G32(0x433B40); /* mov eax,ds:dword_433B40 */
    /* 0x00416E13 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8((edx + eax) & U32MASK); /* mov dl,[edx+eax] */
    /* 0x00416E16 */ edx &= 0xFFUL; /* and edx,0FFh */
    /* 0x00416E1C */ t_20 = edx; /* mov [esp+var_20],edx */
    /* 0x00416E20 */ edx = (esi + esi) & U32MASK; /* lea edx,[esi+esi] */
    /* 0x00416E23 */ esi = (unsigned long)G32(0x45BA34); /* mov esi,ds:dword_45BA34 */
    /* 0x00416E29 */ esi = (esi - edx) & U32MASK; /* sub esi,edx */
    /* 0x00416E2B */ edi = (unsigned long)G32(0x45BA3C); /* mov edi,ds:dword_45BA3C */
    /* 0x00416E31 */ edx = esi; /* mov edx,esi */
    /* 0x00416E33 */ edx = (edx & edi) & U32MASK; /* and edx,edi */
    /* 0x00416E35 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((edx + eax) & U32MASK); /* mov al,[edx+eax] */
    /* 0x00416E38 */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00416E3D */ t_1c = eax; /* mov [esp+var_1C],eax */
    /* 0x00416E41 */ eax = 0; /* xor eax,eax */
    /* 0x00416E43 */ ebp = (unsigned long)G32(0x45AF7C); /* mov ebp,ds:dword_45AF7C */
    /* 0x00416E49 */ eax |= (unsigned long)G8(0x45C681); /* mov al,ds:byte_45C681 */
    /* 0x00416E4E cmp eax,ebp */;
    /* 0x00416E50 jnz */ if (eax != ebp) goto l_41704C; /* equality */
    /* 0x00416E56 cmp byte ptr ds:dword_45BA18,1 */;
    /* 0x00416E5D jnz */ if (M8(0x45BA18UL) != 1) goto l_416E61; /* equality */
    /* 0x00416E5F */ ebx = (ebx & 0xFFFFFF00UL) | 8UL; /* mov bl,8 */
l_416E61:
    /* 0x00416E61 */ eax = (unsigned long)G32(0x45BA18); /* mov eax,ds:dword_45BA18 */
    /* 0x00416E66 */ eax >>= 0x10; /* shr eax,10h */
    /* 0x00416E69 cmp eax,0F6FFh */;
    /* 0x00416E6E setz al */ eax = (eax & 0xFFFFFF00UL) | ((eax == 0xF6FFUL ? 1UL : 0UL)); /* equality */
    /* 0x00416E71 */ eax &= 0xFFUL; /* and eax,0FFh */
l_416E76:
    /* 0x00416E76 */ eax = (eax << 8) & U32MASK; /* shl eax,8 */
    /* 0x00416E79 */ G32(0x459E54) = (unsigned int)eax; /* mov ds:dword_459E54,eax */
    /* 0x00416E7E */ eax = (eax & 0xFFFFFF00UL) | (t_20 & 0xFFUL); /* mov al,byte ptr [esp+var_20] */
    /* 0x00416E82 */ edx = (edx & 0xFFFFFF00UL) | (t_1c & 0xFFUL); /* mov dl,byte ptr [esp+var_1C] */
    /* 0x00416E86 */ eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFUL) + (eax & 0xFFUL)) & 0xFFUL); /* add al,al */
    /* 0x00416E88 */ SUB8_LO(eax, edx & 0xFFUL); /* sub al,dl */
    /* 0x00416E8A */ eax = (eax & 0xFFFF00FFUL) | ((t_20 & 0xFFUL) << 8); /* mov ah,byte ptr [esp+var_20] */
    /* 0x00416E8E */ eax = (eax & 0xFFFF00FFUL) | (((((eax >> 8) & 0xFFUL) - (edx & 0xFFUL))) & 0xFFUL) << 8; /* sub ah,dl */
    /* 0x00416E90 */ edx = 0; /* xor edx,edx */
    /* 0x00416E92 */ edi = (unsigned long)G32(0x459E54); /* mov edi,ds:dword_459E54 */
    /* 0x00416E98 */ edx |= ((eax >> 8) & 0xFFUL); /* mov dl,ah */
    /* 0x00416E9A */ edi = (edi + edx) & U32MASK; /* add edi,edx */
    /* 0x00416E9C */ edx = (unsigned long)G32(0x433B74); /* mov edx,ds:dword_433B74 */
    /* 0x00416EA2 */ esi = edi; /* mov esi,edi */
    /* 0x00416EA4 */ G32(0x459E54) = (unsigned int)edi; /* mov ds:dword_459E54,edi */
    /* 0x00416EAA */ M8((edx + esi * 4 + 2) & U32MASK) = (unsigned char)(eax & 0xFFUL); /* mov [edx+esi*4+2],al */
    /* 0x00416EAE cmp bl,8 */;
    /* 0x00416EB1 jnb */ if ((ebx & 0xFFUL) >= 8UL) goto l_416F01; /* unsigned */
l_416EB3:
    /* 0x00416EB3 */ eax = 0; /* xor eax,eax */
    /* 0x00416EB5 */ eax |= (ebx & 0xFFUL); /* mov al,bl */
    /* 0x00416EB7 */ edx = (unsigned long)G32((0x459E38UL + eax * 4) & U32MASK); /* mov edx,ds:dword_459E38[eax*4] */
    /* 0x00416EBE */ edx = (edx << 2) & U32MASK; /* shl edx,2 */
    /* 0x00416EC1 */ eax = (unsigned long)G32((0x433B58UL + eax * 4) & U32MASK); /* mov eax,ds:dword_433B58[eax*4] */
    /* 0x00416EC8 */ eax = (eax + edx) & U32MASK; /* add eax,edx */
    /* 0x00416ECA cmp byte ptr [eax+1],0 */;
    /* 0x00416ECE jz */ if (M8((eax + 1) & U32MASK) == 0) goto l_416EFA; /* equality */
    /* 0x00416ED0 cmp byte ptr [eax+3],80h */;
    /* 0x00416ED4 jnb */ if (M8((eax + 3) & U32MASK) >= 0x80U) goto l_416EFA; /* unsigned */
    /* 0x00416ED6 */ edx = 0; /* xor edx,edx */
    /* 0x00416ED8 */ edx |= (unsigned long)M8(eax); /* mov dl,[eax] */
    /* 0x00416EDA */ esi = edx; /* mov esi,edx */
    /* 0x00416EDC */ edx = 0; /* xor edx,edx */
    /* 0x00416EDE */ esi = (esi << 8) & U32MASK; /* shl esi,8 */
    /* 0x00416EE1 */ edx |= (unsigned long)M8((eax + 3) & U32MASK); /* mov dl,[eax+3] */
    /* 0x00416EE4 imul edx,esi (32-bit wrap == signed product low half) */ edx = (edx * esi) & U32MASK;
    /* 0x00416EE7 */ esi = (unsigned long)M8((eax + 1) & U32MASK); /* movzx esi,byte ptr [eax+1] */
    /* 0x00416EEB */ eax = edx; /* mov eax,edx */
    /* 0x00416EED */ edx = d4v_sar(edx, 0x1F); /* sar edx,1Fh (sign extension for idiv) */
    /* 0x00416EF0 idiv esi (signed 64/32; dividend is the sign-extended
     * eax; direct C signed divide; faults on a zero divisor like x86) */
    eax = (unsigned long)((int)eax / (int)esi);
    /* 0x00416EF2 cmp eax,ecx */;
    /* 0x00416EF4 jbe */ if (eax <= ecx) goto l_416EFA; /* unsigned */
    /* 0x00416EF6 */ ecx = eax; /* mov ecx,eax */
    /* 0x00416EF8 */ ebx = (ebx & 0xFFFF00FFUL) | (((ebx & 0xFFUL)) << 8); /* mov bh,bl */
l_416EFA:
    /* 0x00416EFA */ ebx = (ebx & 0xFFFFFF00UL) | (((ebx & 0xFFUL) + 1) & 0xFFUL); /* inc bl */
    /* 0x00416EFC cmp bl,8 */;
    /* 0x00416EFF jb */ if ((ebx & 0xFFUL) < 8UL) goto l_416EB3; /* unsigned */
l_416F01:
    /* 0x00416F01 cmp bh,8 */;
    /* 0x00416F04 jnb */ if (((ebx >> 8) & 0xFFUL) >= 8UL) goto l_41706B; /* unsigned */
    /* 0x00416F0A */ eax = 0; /* xor eax,eax */
    /* 0x00416F0C */ eax |= ((ebx >> 8) & 0xFFUL); /* mov al,bh */
    /* 0x00416F0E */ edx = (unsigned long)G32((0x459E38UL + eax * 4) & U32MASK); /* mov edx,ds:dword_459E38[eax*4] */
    /* 0x00416F15 */ edx = (edx << 2) & U32MASK; /* shl edx,2 */
    /* 0x00416F18 */ eax = (unsigned long)G32((0x433B58UL + eax * 4) & U32MASK); /* mov eax,ds:dword_433B58[eax*4] */
    /* 0x00416F1F */ eax = (eax + edx) & U32MASK; /* add eax,edx */
    /* 0x00416F21 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8(eax); /* mov dl,[eax] */
    /* 0x00416F23 cmp dl,[eax+1] */;
    /* 0x00416F26 jbe */ if ((edx & 0xFFUL) <= (unsigned long)M8((eax + 1) & U32MASK)) goto l_41705C; /* unsigned */
l_416F2C:
    /* 0x00416F2C cmp ds:dword_45AF7C,1 */;
    /* 0x00416F33 jbe */ if ((unsigned long)G32(0x45AF7C) <= 1UL) goto l_416F41; /* unsigned */
    /* 0x00416F35 cmp bh,7 */;
    /* 0x00416F38 setz dl */ edx = (edx & 0xFFFFFF00UL) | ((((ebx >> 8) & 0xFFUL) == 7UL ? 1UL : 0UL)); /* equality */
    /* 0x00416F3B */ M8(0x45C684UL) = (unsigned char)(edx & 0xFFUL); /* mov ds:byte_45C684,dl */
l_416F41:
    /* 0x00416F41 cmp byte ptr [eax+1],2 */;
    /* 0x00416F45 jnz */ if (M8((eax + 1) & U32MASK) != 2) goto l_416F70; /* equality */
    /* 0x00416F47 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8(eax); /* mov dl,[eax] */
    /* 0x00416F49 */ edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFUL) - 3UL) & 0xFFUL); /* sub dl,3 */
    /* 0x00416F4C cmp dl,4 */;
    /* 0x00416F4F jnb */ if ((edx & 0xFFUL) >= 4UL) goto l_416F70; /* unsigned */
    /* 0x00416F51 */ edx = 0; /* xor edx,edx */
    /* 0x00416F53 */ edx |= (unsigned long)M8(eax); /* mov dl,[eax] */
    /* 0x00416F55 */ ecx = 0; /* xor ecx,ecx */
    /* 0x00416F57 */ edx = (edx << 3) & U32MASK; /* shl edx,3 */
    /* 0x00416F5A */ ecx |= ((ebx >> 8) & 0xFFUL); /* mov cl,bh */
    /* 0x00416F5C */ edx = (edx - 0x18UL) & U32MASK; /* sub edx,18h */
    /* 0x00416F5F */ edx = (edx + ecx) & U32MASK; /* add edx,ecx */
    /* 0x00416F61 */ ecx = 0x45AE80UL; /* mov ecx,offset unk_45AE80 */
    /* 0x00416F66 */ edx = (edx + edx) & U32MASK; /* add edx,edx */
    /* 0x00416F68 */ ecx = (ecx + edx) & U32MASK; /* add ecx,edx */
    /* 0x00416F6A */ G32(0x45BA4C) = (unsigned int)ecx; /* mov ds:dword_45BA4C,ecx */
l_416F70:
    /* 0x00416F70 cmp byte ptr [eax+1],3 */;
    /* 0x00416F74 jnz */ if (M8((eax + 1) & U32MASK) != 3) goto l_416FA2; /* equality */
    /* 0x00416F76 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8(eax); /* mov dl,[eax] */
    /* 0x00416F78 */ edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFUL) - 4UL) & 0xFFUL); /* sub dl,4 */
    /* 0x00416F7B cmp dl,4 */;
    /* 0x00416F7E jnb */ if ((edx & 0xFFUL) >= 4UL) goto l_416FA2; /* unsigned */
    /* 0x00416F80 */ edx = 0; /* xor edx,edx */
    /* 0x00416F82 */ edx |= (unsigned long)M8(eax); /* mov dl,[eax] */
    /* 0x00416F84 */ ecx = 0; /* xor ecx,ecx */
    /* 0x00416F86 */ edx = (edx << 3) & U32MASK; /* shl edx,3 */
    /* 0x00416F89 */ ecx |= ((ebx >> 8) & 0xFFUL); /* mov cl,bh */
    /* 0x00416F8B */ edx = (edx - 0x20UL) & U32MASK; /* sub edx,20h */
    /* 0x00416F8E */ edx = (edx + ecx) & U32MASK; /* add edx,ecx */
    /* 0x00416F90 */ edx = (edx + edx) & U32MASK; /* add edx,edx */
    /* 0x00416F92 */ ecx = 0x45AE80UL; /* mov ecx,offset unk_45AE80 */
    /* 0x00416F97 */ edx = (edx + 0x40UL) & U32MASK; /* add edx,40h */
    /* 0x00416F9A */ ecx = (ecx + edx) & U32MASK; /* add ecx,edx */
    /* 0x00416F9C */ G32(0x45BA4C) = (unsigned int)ecx; /* mov ds:dword_45BA4C,ecx */
l_416FA2:
    /* 0x00416FA2 */ M8(0x45C682UL) = (unsigned char)((ebx >> 8) & 0xFFUL); /* mov ds:byte_45C682,bh */
    /* epilogue 0x00416FA8 add esp,0Ch; 0x00416FAB pop ebp;
     * 0x00416FAC pop edi; 0x00416FAD pop esi; 0x00416FAE pop edx;
     * 0x00416FAF pop ecx; 0x00416FB0 pop ebx; 0x00416FB1 retn
     * (eax = stale pointer residue; consumed by callers). */
    return (unsigned int)eax;
l_416FB2:
    /* 0x00416FB2 */ edi = (unsigned long)G32(0x45BA18); /* mov edi,ds:dword_45BA18 */
    /* 0x00416FB8 */ edi &= 0xFFFFUL; /* and edi,0FFFFh */
    /* 0x00416FBE cmp edi,0FFFFh */;
    /* 0x00416FC4 jnz */ if (edi != 0xFFFFUL) goto l_416FD0; /* equality */
    /* 0x00416FC6 */ eax = 0x20000UL; /* mov eax,20000h */
    /* 0x00416FCB jmp */ goto l_416D53; /* unconditional */
l_416FD0:
    /* 0x00416FD0 */ edi = (unsigned long)G32(0x45BA18); /* mov edi,ds:dword_45BA18 */
    /* 0x00416FD6 */ edi &= 0xFF000000UL; /* and edi,0FF000000h */
    /* 0x00416FDC cmp edi,0FF000000h */;
    /* 0x00416FE2 jnz */ if (edi != 0xFF000000UL) goto l_416FEE; /* equality */
    /* 0x00416FE4 */ eax = 0x30000UL; /* mov eax,30000h */
    /* 0x00416FE9 jmp */ goto l_416D53; /* unconditional */
l_416FEE:
    /* 0x00416FEE */ edi = (unsigned long)G32(0x45BA18); /* mov edi,ds:dword_45BA18 */
    /* 0x00416FF4 */ edi &= 0xFF0000UL; /* and edi,0FF0000h */
    /* 0x00416FFA cmp edi,0FF0000h */;
    /* 0x00417000 jnz */ if (edi != 0xFF0000UL) goto l_41700C; /* equality */
    /* 0x00417002 */ eax = 0x40000UL; /* mov eax,40000h */
    /* 0x00417007 jmp */ goto l_416D53; /* unconditional */
l_41700C:
    /* 0x0041700C */ edi = (unsigned long)G32(0x45BA18); /* mov edi,ds:dword_45BA18 */
    /* 0x00417012 */ edi &= 0xFF00UL; /* and edi,0FF00h */
    /* 0x00417018 cmp edi,0FF00h */;
    /* 0x0041701E jnz */ if (edi != 0xFF00UL) goto l_41702A; /* equality */
    /* 0x00417020 */ eax = 0x50000UL; /* mov eax,50000h */
    /* 0x00417025 jmp */ goto l_416D53; /* unconditional */
l_41702A:
    /* 0x0041702A */ edi = (unsigned long)G32(0x45BA18); /* mov edi,ds:dword_45BA18 */
    /* 0x00417030 */ edi &= 0xFFUL; /* and edi,0FFh */
    /* 0x00417036 cmp edi,0FFh */;
    /* 0x0041703C jnz */ if (edi != 0xFFUL) goto l_416D53; /* equality */
    /* 0x00417042 */ eax = 0x60000UL; /* mov eax,60000h */
    /* 0x00417047 jmp */ goto l_416D53; /* unconditional */
l_41704C:
    /* 0x0041704C */ eax = (unsigned long)G32(0x45AF38); /* mov eax,ds:dword_45AF38 */
    /* 0x00417051 */ edx = 0; /* xor edx,edx */
    /* 0x00417053 div ebp (edx high half is zero: direct 32-bit divide;
     * faults on a zero divisor exactly like x86) */
    edx = eax % ebp;
    eax = eax / ebp;
    /* 0x00417055 */ eax = edx; /* mov eax,edx (remainder) */
    /* 0x00417057 jmp */ goto l_416E76; /* unconditional */
l_41705C:
    /* 0x0041705C */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8((eax + 3) & U32MASK); /* mov dl,[eax+3] */
    /* 0x0041705F */ edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0x7FUL); /* and dl,7Fh */
    /* 0x00417062 cmp dl,1 */;
    /* 0x00417065 ja */ if ((edx & 0xFFUL) > 1UL) goto l_416F2C; /* unsigned */
l_41706B:
    /* 0x0041706B cmp ds:dword_45AF98,1 */;
    /* 0x00417072 jbe */ if ((unsigned long)G32(0x45AF98) <= 1UL) goto l_41707B; /* unsigned */
    /* 0x00417074 */ eax = (unsigned long)G32(0x45AFA0); /* mov eax,ds:dword_45AFA0 */
    /* 0x00417079 */ M32(eax) = M32(eax) >> 1; /* shr dword ptr [eax],1 */
l_41707B:
    /* 0x0041707B */ eax = 0; /* xor eax,eax */
    /* epilogue 0x0041707D add esp,0Ch; 0x00417080 pop ebp;
     * 0x00417081 pop edi; 0x00417082 pop esi; 0x00417083 pop edx;
     * 0x00417084 pop ecx; 0x00417085 pop ebx; 0x00417086 retn. */
    return (unsigned int)eax;
}
/* ------------------------------------------------------------------ */
/* sub_4177A0 (lst 34383-34854) -> d4v_4177a0                          */
/*                                                                      */
/* Role: PPM mask/order selector. Prices the symbol (entry al) against  */
/* the 4593A0 last-pos table and the 458798 frequency table (with      */
/* periodic halving into 458794), maintains the 45AF94/45AF98/45AF7C   */
/* best-order state, bumps 45AF38/45AF90, then drives one of several    */
/* order/mask update paths keyed on 45C67D/45C67F/symbol range (shift   */
/* registers 45BA18/45BA1C/45BA20, tables 434328/434338/434438,        */
/* 457360/45BA58, or the 41E9F0/41E640 helpers). With nonzero entry    */
/* dl it first runs the 45C65A/45C65C nibble-tickler path.             */
/* Entry-live regs: al = sym (mov cl,al), dl = flag (test dl,dl).      */
/* Only the low bytes are live; high bytes are dead on every path      */
/* (no full-width read precedes an overwrite; verified by inspection). */
/* Callers pass high-zeroed eax (418366/41991B mov eax,ecx after       */
/* xor; 41C030 xor eax,eax; 41CCD2 and eax,0FFh). Returns void         */
/* (callers ignore eax).                                               */
/* Frame: 4 bytes (var_18 byte @ sp+0) + 5 pushed regs (ebx/ecx/si/    */
/* di/bp; edx is the flag input and is NOT preserved).                 */
/* Globals touched: 433B40, 434328, 434338, 434438, 457360, 458794,    */
/* 458798, 45879C, 4593A0, 45AF2C, 45AF38, 45AF3C, 45AF40, 45AF48,      */
/* 45AF90, 45AF94, 45AF98, 45AF7C, 45BA18, 45BA1C, 45BA20, 45BA58,      */
/* 45BA34, 45BA3C, 45C65A, 45C65C, 45C677, 45C678, 45C679, 45C67A,      */
/* 45C67B, 45C67C, 45C67D, 45C67F, 45C68E.                              */
/* Calls d4v_41e9f0 (eax=sym, edx=derived byte) and d4v_41e640.         */
/* Deviations: div @417A38 (edx pre-zeroed) and @417CB3 (divisor is    */
/* const 6) are direct C / with comments (zero divisor faults like     */
/* x86); shr eax,cl @417A51 masks the count to 5 bits per x86.         */
/* ------------------------------------------------------------------ */
void d4v_4177a0(dec4v_ctx *c, unsigned int sym, unsigned int flag)
{
    unsigned long eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned long t_18b = 0;

    /* prologue 0x004177A0 push ebx; 0x004177A1 push ecx;
     * 0x004177A2 push esi; 0x004177A3 push edi; 0x004177A4 push ebp;
     * 0x004177A5 sub esp,4.
     * Callee-saved regs become C locals; the esp frame is t_18b. */

    /* 0x004177A8 */ ebx = (unsigned long)G32(0x45AF90); /* mov ebx,ds:dword_45AF90 */
    /* 0x004177AE */ ecx = (unsigned long)sym & 0xFFUL; /* mov cl,al (ecx high dead: never read full before overwrite) */
    /* 0x004177B0 */ edx = (unsigned long)flag; /* entry dl (edx high dead: xored before any full read) */
    /* 0x004177B0 test dl,dl */;
    /* 0x004177B2 jnz */ if ((edx & 0xFFUL) != 0) goto l_417A7F; /* equality */
l_4177B8:
    /* 0x004177B8 test cl,cl */;
    /* 0x004177BA jz */ if ((ecx & 0xFFUL) == 0) goto l_41790C; /* equality */
    /* 0x004177C0 cmp cl,0FFh */;
    /* 0x004177C3 jz */ if ((ecx & 0xFFUL) == 0xFFUL) goto l_41790C; /* equality */
    /* 0x004177C9 */ eax = 0; /* xor eax,eax */
    /* 0x004177CB */ eax |= (ecx & 0xFFUL); /* mov al,cl */
    /* 0x004177CD */ edx = (eax * 4) & U32MASK; /* lea edx,ds:0[eax*4] */
    /* 0x004177D4 */ eax = (unsigned long)G32(0x45BA34); /* mov eax,ds:dword_45BA34 */
    /* 0x004177D9 */ esi = (unsigned long)G32((0x4593A0UL + edx) & U32MASK); /* mov esi,ds:dword_4593A0[edx] */
    /* 0x004177DF */ edi = (unsigned long)G32(0x45BA3C); /* mov edi,ds:dword_45BA3C */
    /* 0x004177E5 */ eax = (eax - esi) & U32MASK; /* sub eax,esi */
    /* 0x004177E7 */ eax = (eax & edi) & U32MASK; /* and eax,edi */
    /* 0x004177E9 cmp eax,100h */;
    /* 0x004177EE jb */ if (eax < 0x100UL) goto l_417831; /* unsigned */
    /* 0x004177F0 */ eax = (eax & 0xFFFFFF00UL) | (ecx & 0xFFUL); /* mov al,cl */
    /* 0x004177F2 */ eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFUL) - 1UL) & 0xFFUL); /* dec al */
    /* 0x004177F4 */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x004177F9 */ edx = (eax * 4) & U32MASK; /* lea edx,ds:0[eax*4] */
    /* 0x00417800 */ eax = (unsigned long)G32(0x45BA34); /* mov eax,ds:dword_45BA34 */
    /* 0x00417805 */ eax = (eax - (unsigned long)G32((0x4593A0UL + edx) & U32MASK)) & U32MASK; /* sub eax,ds:dword_4593A0[edx] */
    /* 0x0041780B */ eax = (eax & edi) & U32MASK; /* and eax,edi */
    /* 0x0041780D cmp eax,100h */;
    /* 0x00417812 jb */ if (eax < 0x100UL) goto l_417831; /* unsigned */
    /* 0x00417814 */ eax = (eax & 0xFFFFFF00UL) | (ecx & 0xFFUL); /* mov al,cl */
    /* 0x00417816 */ eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFUL) + 1UL) & 0xFFUL); /* inc al */
    /* 0x00417818 */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x0041781D */ edx = (eax * 4) & U32MASK; /* lea edx,ds:0[eax*4] */
    /* 0x00417824 */ eax = (unsigned long)G32(0x45BA34); /* mov eax,ds:dword_45BA34 */
    /* 0x00417829 */ eax = (eax - (unsigned long)G32((0x4593A0UL + edx) & U32MASK)) & U32MASK; /* sub eax,ds:dword_4593A0[edx] */
    /* 0x0041782F */ eax = (eax & edi) & U32MASK; /* and eax,edi */
l_417831:
    /* 0x00417831 test eax,eax */;
    /* 0x00417833 jz */ if (eax == 0) goto l_4178C8; /* equality */
    /* 0x00417839 cmp eax,100h */;
    /* 0x0041783E jnb */ if (eax >= 0x100UL) goto l_4178C8; /* unsigned */
    /* 0x00417844 */ ebp = (unsigned long)G32((0x458798UL + eax * 4) & U32MASK); /* mov ebp,ds:dword_458798[eax*4] */
    /* 0x0041784B */ ebp = (ebp + 1) & U32MASK; /* inc ebp */
    /* 0x0041784C */ edx = (unsigned long)G32(0x45AF98); /* mov edx,ds:dword_45AF98 */
    /* 0x00417852 */ M32((0x458798UL + eax * 4) & U32MASK) = (unsigned int)ebp; /* mov ds:dword_458798[eax*4],ebp */
    /* 0x00417859 cmp eax,edx */;
    /* 0x0041785B jz */ if (eax == edx) goto l_4178C8; /* equality */
    /* 0x0041785D */ edx = (unsigned long)G32(0x45BA34); /* mov edx,ds:dword_45BA34 */
    /* 0x00417863 */ esi = (unsigned long)G32(0x45AF98); /* mov esi,ds:dword_45AF98 */
    /* 0x00417869 */ edi = (unsigned long)G32(0x45BA3C); /* mov edi,ds:dword_45BA3C */
    /* 0x0041786F */ edx = (edx - esi) & U32MASK; /* sub edx,esi */
    /* 0x00417871 */ esi = (unsigned long)G32(0x433B40); /* mov esi,ds:dword_433B40 */
    /* 0x00417877 */ edx = (edx & edi) & U32MASK; /* and edx,edi */
    /* 0x00417879 cmp cl,[edx+esi] */;
    /* 0x0041787C jnz */ if ((ecx & 0xFFUL) != (unsigned long)M8((edx + esi) & U32MASK)) goto l_417B42; /* equality */
    /* 0x00417882 */ edx = (unsigned long)G32(0x45AF98); /* mov edx,ds:dword_45AF98 */
    /* 0x00417888 */ M32((0x458798UL + edx * 4) & U32MASK) = M32((0x458798UL + edx * 4) & U32MASK) + 1; /* inc ds:dword_458798[edx*4] */
l_41788F:
    /* 0x0041788F */ edx = (unsigned long)G32(0x45AF94); /* mov edx,ds:dword_45AF94 */
    /* 0x00417895 cmp eax,edx */;
    /* 0x00417897 jnz */ if (eax != edx) goto l_417B48; /* equality */
    /* 0x0041789D */ esi = (unsigned long)G32(0x45AF98); /* mov esi,ds:dword_45AF98 */
    /* 0x004178A3 */ edx = (unsigned long)G32((0x458798UL + edx * 4) & U32MASK); /* mov edx,ds:dword_458798[edx*4] */
    /* 0x004178AA cmp edx,ds:dword_458798[esi*4] */;
    /* 0x004178B1 jb */ if (edx < (unsigned long)G32((0x458798UL + esi * 4) & U32MASK)) goto l_4178C8; /* unsigned */
    /* 0x004178B3 */ G32(0x45AF94) = (unsigned int)esi; /* mov ds:dword_45AF94,esi */
    /* 0x004178B9 */ G32(0x45AF98) = (unsigned int)eax; /* mov ds:dword_45AF98,eax */
    /* 0x004178BE cmp eax,1 */;
    /* 0x004178C1 jbe */ if (eax <= 1UL) goto l_4178C8; /* unsigned */
    /* 0x004178C3 */ G32(0x45AF7C) = (unsigned int)eax; /* mov ds:dword_45AF7C,eax */
l_4178C8:
    /* 0x004178C8 */ eax = ebx; /* mov eax,ebx */
    /* 0x004178CA */ ebx = (ebx + 1) & U32MASK; /* inc ebx */
    /* 0x004178CB cmp eax,40h */;
    /* 0x004178CE jb */ if (eax < 0x40UL) goto l_4178FB; /* unsigned */
    /* 0x004178D0 */ ebp = (unsigned long)G32(0x45879C); /* mov ebp,ds:dword_45879C */
    /* 0x004178D6 */ ebx = 0; /* xor ebx,ebx */
    /* 0x004178D8 */ ebp = (ebp + 1) & U32MASK; /* inc ebp */
    /* 0x004178D9 */ eax = 0; /* xor eax,eax */
    /* 0x004178DB */ G32(0x45879C) = (unsigned int)ebp; /* mov ds:dword_45879C,ebp */
l_4178E1:
    /* 0x004178E1 */ edx = (unsigned long)G32((0x458798UL + eax) & U32MASK); /* mov edx,ds:dword_458798[eax] */
    /* 0x004178E7 */ edx >>= 1; /* shr edx,1 */
    /* 0x004178E9 */ eax = (eax + 4) & U32MASK; /* add eax,4 */
    /* 0x004178EC */ ebx = (ebx + edx) & U32MASK; /* add ebx,edx */
    /* 0x004178EE */ M32((0x458794UL + eax) & U32MASK) = (unsigned int)edx; /* mov ds:dword_458794[eax],edx (post-increment eax) */
    /* 0x004178F4 cmp eax,400h */;
    /* 0x004178F9 jnz */ if (eax != 0x400UL) goto l_4178E1; /* equality */
l_4178FB:
    /* 0x004178FB */ eax = 0; /* xor eax,eax */
    /* 0x004178FD */ eax |= (ecx & 0xFFUL); /* mov al,cl */
    /* 0x004178FF */ edx = (unsigned long)G32(0x45BA34); /* mov edx,ds:dword_45BA34 */
    /* 0x00417905 */ M32((0x4593A0UL + (eax & 0xFFUL) * 4) & U32MASK) = (unsigned int)edx; /* mov ds:dword_4593A0[eax*4],edx */
l_41790C:
    /* 0x0041790C */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)G8(0x45C67D); /* mov al,ds:byte_45C67D */
    /* 0x00417911 */ G32(0x45AF90) = (unsigned int)ebx; /* mov ds:dword_45AF90,ebx */
    /* 0x00417917 test al,al */;
    /* 0x00417919 jz */ if ((eax & 0xFFUL) == 0) goto l_41799B; /* equality */
    /* 0x0041791F */ eax = (eax & 0xFFFFFF00UL) | (ecx & 0xFFUL); /* mov al,cl */
    /* 0x00417921 */ eax = (eax & 0xFFFF00FFUL) | ((unsigned long)G8(0x45C679) << 8); /* mov ah,ds:byte_45C679 */
    /* 0x00417927 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)G8(0x45C67B); /* mov dl,ds:byte_45C67B */
    /* 0x0041792D */ eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFUL) - ((eax >> 8) & 0xFFUL)) & 0xFFUL); /* sub al,ah */
    /* 0x0041792F test dl,dl */;
    /* 0x00417931 jz */ if ((edx & 0xFFUL) == 0) goto l_417935; /* equality */
    /* 0x00417933 */ eax = (eax & 0xFFFFFF00UL) | (((0UL - (eax & 0xFFUL))) & 0xFFUL); /* neg al */
l_417935:
    /* 0x00417935 cmp al,80h */;
    /* 0x00417937 jb */ if ((eax & 0xFFUL) < 0x80UL) goto l_41794A; /* unsigned */
    /* 0x00417939 */ edx = (edx & 0xFFFF00FFUL) | ((unsigned long)G8(0x45C67B) << 8); /* mov dh,ds:byte_45C67B */
    /* 0x0041793F test dh,dh */;
    /* 0x00417941 setz ah (flags from test dh,dh @0041793F: ah=(dh==0)) */ eax = (eax & 0xFFFF00FFUL) | (((((edx >> 8) & 0xFFUL) == 0UL) ? 1UL : 0UL) << 8); /* equality */
    /* 0x00417944 */ M8(0x45C67BUL) = (unsigned char)((eax >> 8) & 0xFFUL); /* mov ds:byte_45C67B,ah */
l_41794A:
    /* 0x0041794A */ ecx = (ecx & 0xFFFF00FFUL) | ((unsigned long)G8(0x45C67D) << 8); /* mov ch,ds:byte_45C67D */
    /* 0x00417950 cmp ch,1 */;
    /* 0x00417953 jnz */ if (((ecx >> 8) & 0xFFUL) != 1) goto l_417B52; /* equality */
    /* 0x00417959 */ edx = (unsigned long)G32(0x45AF3C); /* mov edx,ds:dword_45AF3C */
    /* 0x0041795F */ edx = (edx - 0x203UL) & U32MASK; /* sub edx,203h */
    /* 0x00417965 */ edx &= 0xFUL; /* and edx,0Fh */
    /* 0x00417968 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8((0x434328UL + (edx & 0xFFUL)) & U32MASK); /* mov dl,ds:byte_434328[edx] */
    /* 0x0041796E */ edx &= 0xFFUL; /* and edx,0FFh */
    /* 0x00417974 */ edx = (edx << 4) & U32MASK; /* shl edx,4 */
    /* 0x00417977 */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x0041797C */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((0x434338UL + (eax & 0xFFUL)) & U32MASK); /* mov al,ds:byte_434338[eax] */
    /* 0x00417982 */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00417987 */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x00417989 */ edx = (edx + 0x203UL) & U32MASK; /* add edx,203h */
    /* 0x0041798F */ G32(0x45AF90) = (unsigned int)ebx; /* mov ds:dword_45AF90,ebx */
l_417995:
    /* 0x00417995 */ G32(0x45AF3C) = (unsigned int)edx; /* mov ds:dword_45AF3C,edx */
l_41799B:
    /* 0x0041799B */ ebp = (unsigned long)G32(0x45AF38); /* mov ebp,ds:dword_45AF38 */
    /* 0x004179A1 */ ebx = (unsigned long)G32(0x45AF90); /* mov ebx,ds:dword_45AF90 */
    /* 0x004179A7 */ ebp = (ebp + 1) & U32MASK; /* inc ebp */
    /* 0x004179A8 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)G8(0x45C67F); /* mov dl,ds:byte_45C67F */
    /* 0x004179AE */ G32(0x45AF38) = (unsigned int)ebp; /* mov ds:dword_45AF38,ebp */
    /* 0x004179B4 test dl,dl */;
    /* 0x004179B6 jnz */ if ((edx & 0xFFUL) != 0) goto l_417C5C; /* equality */
    /* 0x004179BC cmp cl,4 */;
    /* 0x004179BF jb */ if ((ecx & 0xFFUL) < 4UL) goto l_417C5C; /* unsigned */
    /* 0x004179C5 cmp cl,6 */;
    /* 0x004179C8 ja */ if ((ecx & 0xFFUL) > 6UL) goto l_417C5C; /* unsigned */
    /* 0x004179CE cmp byte ptr ds:dword_45BA18,0DFh */;
    /* 0x004179D5 jz */ if (M8(0x45BA18UL) == 0xDFU) goto l_417A1B; /* equality */
    /* 0x004179D7 */ edx = (unsigned long)G32(0x45BA20); /* mov edx,ds:dword_45BA20 */
    /* 0x004179DD */ eax = (unsigned long)G32(0x45BA1C); /* mov eax,ds:dword_45BA1C */
    /* 0x004179E2 */ edx = (edx << 8) & U32MASK; /* shl edx,8 */
    /* 0x004179E5 */ eax >>= 0x18; /* shr eax,18h */
    /* 0x004179E8 */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x004179EA */ eax = (unsigned long)G32(0x45BA18); /* mov eax,ds:dword_45BA18 */
    /* 0x004179EF */ G32(0x45BA20) = (unsigned int)edx; /* mov ds:dword_45BA20,edx */
    /* 0x004179F5 */ edx = (unsigned long)G32(0x45BA1C); /* mov edx,ds:dword_45BA1C */
    /* 0x004179FB */ eax >>= 0x18; /* shr eax,18h */
    /* 0x004179FE */ edx = (edx << 8) & U32MASK; /* shl edx,8 */
    /* 0x00417A01 */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x00417A03 */ eax = (unsigned long)G32(0x45BA18); /* mov eax,ds:dword_45BA18 */
    /* 0x00417A08 */ eax = (eax << 8) & U32MASK; /* shl eax,8 */
    /* 0x00417A0B */ eax = (eax + 0xDFUL) & U32MASK; /* add eax,0DFh */
    /* 0x00417A10 */ G32(0x45BA1C) = (unsigned int)edx; /* mov ds:dword_45BA1C,edx */
    /* 0x00417A16 */ G32(0x45BA18) = (unsigned int)eax; /* mov ds:dword_45BA18,eax */
l_417A1B:
    /* 0x00417A1B */ ecx = (ecx & 0xFFFFFF00UL) | (unsigned long)G8(0x45C67D); /* mov cl,ds:byte_45C67D */
    /* 0x00417A21 */ G32(0x45AF90) = (unsigned int)ebx; /* mov ds:dword_45AF90,ebx */
    /* 0x00417A27 test cl,cl */;
    /* 0x00417A29 jz */ if ((ecx & 0xFFUL) == 0) goto l_417A70; /* equality */
    /* 0x00417A2B */ eax = (unsigned long)G32(0x45AF38); /* mov eax,ds:dword_45AF38 */
    /* 0x00417A30 */ ecx = (unsigned long)G32(0x45AF40); /* mov ecx,ds:dword_45AF40 */
    /* 0x00417A36 */ edx = 0; /* xor edx,edx */
    /* 0x00417A38 div ecx (edx high half is zero: direct 32-bit divide;
     * faults on a zero divisor exactly like x86) */
    { unsigned long q_eax = eax; eax = q_eax / ecx; edx = q_eax % ecx; }
    /* 0x00417A3A */ edi = (unsigned long)G32(0x45AF48); /* mov edi,ds:dword_45AF48 */
    /* 0x00417A40 */ ecx = (ecx & 0xFFFFFF00UL) | (unsigned long)G8(0x45C67A); /* mov cl,ds:byte_45C67A */
    /* 0x00417A46 */ eax = (unsigned long)G32(0x45BA18); /* mov eax,ds:dword_45BA18 */
    /* 0x00417A4B */ G32(0x45AF48) = (unsigned int)edx; /* mov ds:dword_45AF48,edx */
    /* 0x00417A51 shr eax,cl (x86 masks the count to 5 bits) */ eax >>= (ecx & 31UL);
    /* 0x00417A53 */ ecx = (ecx & 0xFFFF00FFUL) | ((unsigned long)G8(0x45C67D) << 8); /* mov ch,ds:byte_45C67D */
    /* 0x00417A59 */ M8(0x45C679UL) = (unsigned char)(eax & 0xFFUL); /* mov ds:byte_45C679,al */
    /* 0x00417A5E cmp ch,3 */;
    /* 0x00417A61 jz */ if (((ecx >> 8) & 0xFFUL) == 3) goto l_417CA7; /* equality */
    /* 0x00417A67 cmp ch,5 */;
    /* 0x00417A6A jnb */ if (((ecx >> 8) & 0xFFUL) >= 5) goto l_417D81; /* unsigned */
l_417A70:
    /* 0x00417A70 */ ebx = (unsigned long)G32(0x45AF90); /* mov ebx,ds:dword_45AF90 */
    /* epilogue 0x00417A76 add esp,4; 0x00417A79 pop ebp;
     * 0x00417A7A pop edi; 0x00417A7B pop esi; 0x00417A7C pop ecx;
     * 0x00417A7D pop ebx; 0x00417A7E retn. */
    return;
l_417A7F:
    /* 0x00417A7F */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)G8(0x45AF38); /* mov al,byte ptr ds:dword_45AF38 */
    /* 0x00417A84 */ eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 1UL); /* and al,1 */
    /* 0x00417A86 */ edx = 0; /* xor edx,edx */
    /* 0x00417A88 */ t_18b = eax & 0xFFUL; /* mov [esp+var_18],al */
    /* 0x00417A8B */ edx |= (eax & 0xFFUL); /* mov dl,al */
    /* 0x00417A8D */ eax = 0x45C65AUL; /* mov eax,offset byte_45C65A */
    /* 0x00417A92 */ eax = (eax + edx) & U32MASK; /* add eax,edx */
    /* 0x00417A94 */ ecx = (ecx & 0xFFFF00FFUL) | ((unsigned long)M8(eax) << 8); /* mov ch,[eax] */
    /* 0x00417A96 */ ecx = (ecx & 0xFFFF00FFUL) | (((((ecx >> 8) & 0xFFUL) + ((ecx >> 8) & 0xFFUL))) & 0xFFUL) << 8; /* add ch,ch */
    /* 0x00417A98 */ ecx = (ecx & 0xFFFF00FFUL) | ((((ecx >> 8) & 0xFFUL) & 0xFUL) << 8); /* and ch,0Fh */
    /* 0x00417A9B */ edx = (edx + 0x45C65CUL) & U32MASK; /* add edx,offset byte_45C65C (edx was 0/1: pointer) */
    /* 0x00417AA1 */ M8(eax) = (unsigned char)((ecx >> 8) & 0xFFUL); /* mov [eax],ch */
    /* 0x00417AA3 */ ecx = (ecx & 0xFFFF00FFUL) | ((unsigned long)M8(edx) << 8); /* mov ch,[edx] */
    /* 0x00417AA5 */ ecx = (ecx & 0xFFFF00FFUL) | (((((ecx >> 8) & 0xFFUL) + ((ecx >> 8) & 0xFFUL))) & 0xFFUL) << 8; /* add ch,ch */
    /* 0x00417AA7 */ ecx = (ecx & 0xFFFF00FFUL) | ((((ecx >> 8) & 0xFFUL) & 0xFUL) << 8); /* and ch,0Fh */
    /* 0x00417AAA */ M8(edx) = (unsigned char)((ecx >> 8) & 0xFFUL); /* mov [edx],ch */
    /* 0x00417AAC */ ecx &= 0xFFFF00FFUL; /* xor ch,ch */
    /* 0x00417AAE */ M8(0x45C68EUL) = (unsigned char)((ecx >> 8) & 0xFFUL); /* mov ds:byte_45C68E,ch (zero) */
    /* 0x00417AB4 cmp cl,80h */;
    /* 0x00417AB7 jb */ if ((ecx & 0xFFUL) < 0x80UL) goto l_4177B8; /* unsigned */
    /* 0x00417ABD cmp cl,0FFh */;
    /* 0x00417AC0 jnz */ if ((ecx & 0xFFUL) != 0xFFUL) goto l_417AF8; /* equality */
    /* 0x00417AC2 */ ecx = (ecx & 0xFFFF00FFUL) | ((unsigned long)M8(edx) << 8); /* mov ch,[edx] */
    /* 0x00417AC4 */ ecx = (ecx & 0xFFFF00FFUL) | (((((ecx >> 8) & 0xFFUL) + 1UL)) & 0xFFUL) << 8; /* inc ch */
    /* 0x00417AC6 */ eax = (eax & 0xFFFF00FFUL) | ((t_18b & 0xFFUL) << 8); /* mov ah,[esp+var_18] */
    /* 0x00417AC9 */ M8(edx) = (unsigned char)((ecx >> 8) & 0xFFUL); /* mov [edx],ch */
    /* 0x00417ACB test ah,ah */;
    /* 0x00417ACD setz al */ eax = (eax & 0xFFFFFF00UL) | (((((eax >> 8) & 0xFFUL) == 0UL) ? 1UL : 0UL)); /* equality */
    /* 0x00417AD0 */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00417AD5 */ ecx = (ecx & 0xFFFF00FFUL) | ((unsigned long)M8((0x45C65AUL + (eax & 0xFFUL)) & U32MASK) << 8); /* mov ch,ds:byte_45C65A[eax] */
    /* 0x00417ADB cmp ch,0Fh */;
    /* 0x00417ADE jnz */ if (((ecx >> 8) & 0xFFUL) != 0x0FUL) goto l_4177B8; /* equality */
    /* 0x00417AE4 cmp ch,[edx] */;
    /* 0x00417AE6 jnz */ if (((ecx >> 8) & 0xFFUL) != (unsigned long)M8(edx)) goto l_4177B8; /* equality */
    /* 0x00417AEC */ M8(0x45C68EUL) = 2; /* mov ds:byte_45C68E,2 */
    /* 0x00417AF3 jmp */ goto l_4177B8; /* unconditional */
l_417AF8:
    /* 0x00417AF8 cmp cl,0DFh */;
    /* 0x00417AFB ja */ if ((ecx & 0xFFUL) > 0xDFUL) goto l_417B2F; /* unsigned */
l_417AFD:
    /* 0x00417AFD */ M8(eax) = (unsigned char)((M8(eax) + 1) & 0xFFUL); /* inc byte ptr [eax] */
    /* 0x00417AFF cmp [esp+var_18],0 */;
    /* 0x00417B03 setz dl */ edx = (edx & 0xFFFFFF00UL) | (((t_18b & 0xFFUL) == 0UL) ? 1UL : 0UL); /* equality */
    /* 0x00417B06 */ edx &= 0xFFUL; /* and edx,0FFh */
    /* 0x00417B0C */ ecx = (ecx & 0xFFFF00FFUL) | ((unsigned long)M8((0x45C65CUL + (edx & 0xFFUL)) & U32MASK) << 8); /* mov ch,ds:byte_45C65C[edx] */
    /* 0x00417B12 cmp ch,0Fh */;
    /* 0x00417B15 jnz */ if (((ecx >> 8) & 0xFFUL) != 0x0FUL) goto l_4177B8; /* equality */
    /* 0x00417B1B cmp ch,[eax] */;
    /* 0x00417B1D jnz */ if (((ecx >> 8) & 0xFFUL) != (unsigned long)M8(eax)) goto l_4177B8; /* equality */
    /* 0x00417B23 */ M8(0x45C68EUL) = 1; /* mov ds:byte_45C68E,1 */
    /* 0x00417B2A jmp */ goto l_4177B8; /* unconditional */
l_417B2F:
    /* 0x00417B2F cmp cl,0F2h */;
    /* 0x00417B32 jb */ if ((ecx & 0xFFUL) < 0xF2UL) goto l_4177B8; /* unsigned */
    /* 0x00417B38 cmp cl,0F6h */;
    /* 0x00417B3B jbe */ if ((ecx & 0xFFUL) <= 0xF6UL) goto l_417AFD; /* unsigned */
    /* 0x00417B3D jmp */ goto l_4177B8; /* unconditional */
l_417B42:
    /* 0x00417B42 */ ebx = (ebx + 1) & U32MASK; /* inc ebx */
    /* 0x00417B43 jmp */ goto l_41788F; /* unconditional */
l_417B48:
    /* 0x00417B48 */ G32(0x45AF94) = (unsigned int)eax; /* mov ds:dword_45AF94,eax */
    /* 0x00417B4D jmp */ goto l_4178C8; /* unconditional */
l_417B52:
    /* 0x00417B52 */ edx = (unsigned long)G32(0x45AF48); /* mov edx,ds:dword_45AF48 */
    /* 0x00417B58 */ edx = (edx << 4) & U32MASK; /* shl edx,4 */
    /* 0x00417B5B cmp ch,2 */;
    /* 0x00417B5E jnz */ if (((ecx >> 8) & 0xFFUL) != 2) goto l_417B87; /* equality */
    /* 0x00417B60 */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00417B65 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((0x434338UL + (eax & 0xFFUL)) & U32MASK); /* mov al,ds:byte_434338[eax] */
    /* 0x00417B6B */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00417B70 */ eax = (eax + edx) & U32MASK; /* add eax,edx */
    /* 0x00417B72 */ eax = (eax + 0x223UL) & U32MASK; /* add eax,223h */
    /* 0x00417B77 */ G32(0x45AF90) = (unsigned int)ebx; /* mov ds:dword_45AF90,ebx */
    /* 0x00417B7D */ G32(0x45AF3C) = (unsigned int)eax; /* mov ds:dword_45AF3C,eax */
    /* 0x00417B82 jmp */ goto l_41799B; /* unconditional */
l_417B87:
    /* 0x00417B87 cmp ch,3 */;
    /* 0x00417B8A jnz */ if (((ecx >> 8) & 0xFFUL) != 3) goto l_417C22; /* equality */
    /* 0x00417B90 cmp ds:byte_45C677,0 */;
    /* 0x00417B97 jz */ if (G8(0x45C677) == 0) goto l_417BD0; /* equality */
    /* 0x00417B99 */ eax = (unsigned long)G32(0x45BA18); /* mov eax,ds:dword_45BA18 */
    /* 0x00417B9E */ edx = 0; /* xor edx,edx */
    /* 0x00417BA0 */ eax >>= 0x10; /* shr eax,10h */
    /* 0x00417BA3 */ edx |= (ecx & 0xFFUL); /* mov dl,cl */
    /* 0x00417BA5 */ edx = (edx - eax) & U32MASK; /* sub edx,eax */
    /* 0x00417BA7 */ eax = edx; /* mov eax,edx */
    /* 0x00417BA9 */ eax = (eax & 0xFFFF00FFUL) | ((unsigned long)G8(0x45C67C) << 8); /* mov ah,ds:byte_45C67C */
    /* 0x00417BAF */ M8(0x45C67BUL) = (unsigned char)((eax >> 8) & 0xFFUL); /* mov ds:byte_45C67B,ah */
    /* 0x00417BB5 test ah,ah */;
    /* 0x00417BB7 jz */ if (((eax >> 8) & 0xFFUL) == 0) goto l_417BBB; /* equality */
    /* 0x00417BB9 */ eax = (eax & 0xFFFFFF00UL) | (((0UL - (eax & 0xFFUL))) & 0xFFUL); /* neg al */
l_417BBB:
    /* 0x00417BBB cmp al,80h */;
    /* 0x00417BBD jb */ if ((eax & 0xFFUL) < 0x80UL) goto l_417BD0; /* unsigned */
    /* 0x00417BBF */ ecx = (ecx & 0xFFFF00FFUL) | ((unsigned long)G8(0x45C67B) << 8); /* mov ch,ds:byte_45C67B */
    /* 0x00417BC5 test ch,ch */;
    /* 0x00417BC7 setz ah */ eax = (eax & 0xFFFF00FFUL) | (((((ecx >> 8) & 0xFFUL) == 0UL) ? 1UL : 0UL) << 8); /* equality */
    /* 0x00417BCA */ M8(0x45C67BUL) = (unsigned char)((eax >> 8) & 0xFFUL); /* mov ds:byte_45C67B,ah */
l_417BD0:
    /* 0x00417BD0 */ esi = (unsigned long)G8(0x45C678); /* movzx esi,ds:byte_45C678 */
    /* 0x00417BD7 */ eax = (eax & 0xFFFF00FFUL) | ((unsigned long)M8((0x45BA58UL + (esi & 0xFFUL)) & U32MASK) << 8); /* mov ah,ds:byte_45BA58[esi] */
    /* 0x00417BDD */ edx = (esi * 4) & U32MASK; /* lea edx,ds:0[esi*4] */
    /* 0x00417BE4 cmp cl,ah */;
    /* 0x00417BE6 jnz */ if ((ecx & 0xFFUL) != ((eax >> 8) & 0xFFUL)) goto l_417C1A; /* equality */
    /* 0x00417BE8 */ M32((0x457360UL + edx) & U32MASK) = M32((0x457360UL + edx) & U32MASK) + 1; /* inc ds:dword_457360[edx] */
l_417BEE:
    /* 0x00417BEE */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00417BF3 */ edx = 0; /* xor edx,edx */
    /* 0x00417BF5 */ edx |= (unsigned long)M8((0x434438UL + (eax & 0xFFUL)) & U32MASK); /* mov dl,ds:byte_434438[eax] */
    /* 0x00417BFB */ eax = (unsigned long)G32(0x45AF48); /* mov eax,ds:dword_45AF48 */
    /* 0x00417C00 */ eax = (eax << 5) & U32MASK; /* shl eax,5 */
    /* 0x00417C03 */ eax = (eax + edx) & U32MASK; /* add eax,edx */
    /* 0x00417C05 */ eax = (eax + 0x243UL) & U32MASK; /* add eax,243h */
    /* 0x00417C0A */ G32(0x45AF90) = (unsigned int)ebx; /* mov ds:dword_45AF90,ebx */
    /* 0x00417C10 */ G32(0x45AF3C) = (unsigned int)eax; /* mov ds:dword_45AF3C,eax */
    /* 0x00417C15 jmp */ goto l_41799B; /* unconditional */
l_417C1A:
    /* 0x00417C1A */ M32((0x457360UL + edx) & U32MASK) = M32((0x457360UL + edx) & U32MASK) >> 1; /* shr ds:dword_457360[edx],1 */
    /* 0x00417C20 jmp */ goto l_417BEE; /* unconditional */
l_417C22:
    /* 0x00417C22 */ G32(0x45AF90) = (unsigned int)ebx; /* mov ds:dword_45AF90,ebx */
    /* 0x00417C28 cmp ch,4 */;
    /* 0x00417C2B jnz */ if (((ecx >> 8) & 0xFFUL) != 4) goto l_417C4A; /* equality */
    /* 0x00417C2D */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00417C32 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((0x434338UL + (eax & 0xFFUL)) & U32MASK); /* mov al,ds:byte_434338[eax] */
    /* 0x00417C38 */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00417C3D */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x00417C3F */ edx = (edx + 0x2A3UL) & U32MASK; /* add edx,2A3h */
    /* 0x00417C45 jmp */ goto l_417995; /* unconditional */
l_417C4A:
    /* 0x00417C4A */ edx = 0; /* xor edx,edx */
    /* 0x00417C4C */ edx |= (eax & 0xFFUL); /* mov dl,al */
    /* 0x00417C4E */ eax = 0; /* xor eax,eax */
    /* 0x00417C50 */ eax |= (ecx & 0xFFUL); /* mov al,cl */
    /* 0x00417C52 */ d4v_41e9f0(c, (unsigned int)(eax & 0xFFUL), (unsigned int)(edx & 0xFFUL)); /* call sub_41E9F0 */
    /* 0x00417C57 jmp */ goto l_41799B; /* unconditional */
l_417C5C:
    /* 0x00417C5C */ edx = (unsigned long)G32(0x45BA20); /* mov edx,ds:dword_45BA20 */
    /* 0x00417C62 */ eax = (unsigned long)G32(0x45BA1C); /* mov eax,ds:dword_45BA1C */
    /* 0x00417C67 */ edx = (edx << 8) & U32MASK; /* shl edx,8 */
    /* 0x00417C6A */ eax >>= 0x18; /* shr eax,18h */
    /* 0x00417C6D */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x00417C6F */ eax = (unsigned long)G32(0x45BA1C); /* mov eax,ds:dword_45BA1C */
    /* 0x00417C74 */ G32(0x45BA20) = (unsigned int)edx; /* mov ds:dword_45BA20,edx */
    /* 0x00417C7A */ edx = (unsigned long)G32(0x45BA18); /* mov edx,ds:dword_45BA18 */
    /* 0x00417C80 */ eax = (eax << 8) & U32MASK; /* shl eax,8 */
    /* 0x00417C83 */ edx >>= 0x18; /* shr edx,18h */
    /* 0x00417C86 */ eax = (eax + edx) & U32MASK; /* add eax,edx */
    /* 0x00417C88 */ edx = (unsigned long)G32(0x45BA18); /* mov edx,ds:dword_45BA18 */
    /* 0x00417C8E */ G32(0x45BA1C) = (unsigned int)eax; /* mov ds:dword_45BA1C,eax */
    /* 0x00417C93 */ eax = 0; /* xor eax,eax */
    /* 0x00417C95 */ edx = (edx << 8) & U32MASK; /* shl edx,8 */
    /* 0x00417C98 */ eax |= (ecx & 0xFFUL); /* mov al,cl */
    /* 0x00417C9A */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x00417C9C */ G32(0x45BA18) = (unsigned int)edx; /* mov ds:dword_45BA18,edx */
    /* 0x00417CA2 jmp */ goto l_417A1B; /* unconditional */
l_417CA7:
    /* 0x00417CA7 */ ecx = 6; /* mov ecx,6 */
    /* 0x00417CAC */ eax = (unsigned long)G32(0x45AF38); /* mov eax,ds:dword_45AF38 */
    /* 0x00417CB1 */ edx = 0; /* xor edx,edx */
    /* 0x00417CB3 div ecx (divisor is const 6, nonzero; direct divide;
     * faults on a zero divisor exactly like x86) */
    { unsigned long q_eax = eax; eax = q_eax / ecx; edx = q_eax % ecx; }
    /* 0x00417CB5 */ eax = 0; /* xor eax,eax */
    /* 0x00417CB7 */ eax |= (edx & 0xFFUL); /* mov al,dl (remainder) */
    /* 0x00417CB9 */ ecx = (unsigned long)G32(0x45BA18); /* mov ecx,ds:dword_45BA18 */
    /* 0x00417CBF */ esi = (unsigned long)G32((0x457360UL + (eax & 0xFFUL) * 4) & U32MASK); /* mov esi,ds:dword_457360[eax*4] */
    /* 0x00417CC6 */ M8(0x45C678UL) = (unsigned char)(edx & 0xFFUL); /* mov ds:byte_45C678,dl */
    /* 0x00417CCC cmp esi,8 */;
    /* 0x00417CCF setnbe dl */ edx = (edx & 0xFFFFFF00UL) | ((esi > 8UL ? 1UL : 0UL)); /* unsigned */
    /* 0x00417CD2 */ ecx >>= 0x18; /* shr ecx,18h */
    /* 0x00417CD5 */ esi = (unsigned long)M8(0x45BA18UL); /* movzx esi,byte ptr ds:dword_45BA18 */
    /* 0x00417CDC */ esi = (esi - ecx) & U32MASK; /* sub esi,ecx */
    /* 0x00417CDE */ ecx = 0; /* xor ecx,ecx */
    /* 0x00417CE0 */ ecx |= (unsigned long)G8(0x45C679); /* mov cl,ds:byte_45C679 */
    /* 0x00417CE6 */ ecx = (ecx + esi) & U32MASK; /* add ecx,esi */
    /* 0x00417CE8 */ edx = (edx & 0xFFFF00FFUL) | ((edx & 0xFFUL) << 8); /* mov dh,dl */
    /* 0x00417CEA */ M8((0x45BA58UL + (eax & 0xFFUL)) & U32MASK) = (unsigned char)(ecx & 0xFFUL); /* mov ds:byte_45BA58[eax],cl */
    /* 0x00417CF0 test dl,dl */;
    /* 0x00417CF2 jz */ if ((edx & 0xFFUL) == 0) goto l_417D61; /* equality */
    /* 0x00417CF4 cmp ds:dword_45AF2C,5 */;
    /* 0x00417CFB jnb */ if ((unsigned long)G32(0x45AF2C) >= 5UL) goto l_417D61; /* unsigned */
    /* 0x00417CFD */ eax = (eax & 0xFFFFFF00UL) | 1UL; /* mov al,1 */
l_417CFF:
    /* 0x00417CFF */ M8(0x45C677UL) = (unsigned char)(eax & 0xFFUL); /* mov ds:byte_45C677,al */
    /* 0x00417D04 test al,al */;
    /* 0x00417D06 jz */ if ((eax & 0xFFUL) == 0) goto l_417D3B; /* equality */
    /* 0x00417D08 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)G8(0x45C67B); /* mov al,ds:byte_45C67B */
    /* 0x00417D0D */ M8(0x45C67CUL) = (unsigned char)(eax & 0xFFUL); /* mov ds:byte_45C67C,al */
    /* 0x00417D12 */ eax &= 0xFFFF00FFUL; /* xor ah,ah */
    /* 0x00417D14 */ M8(0x45C67BUL) = (unsigned char)((eax >> 8) & 0xFFUL); /* mov ds:byte_45C67B,ah (zero) */
    /* 0x00417D1A */ eax = edi; /* mov eax,edi */
    /* 0x00417D1C */ eax = (eax << 5) & U32MASK; /* shl eax,5 */
    /* 0x00417D1F */ eax = (eax + 0x253UL) & U32MASK; /* add eax,253h */
    /* 0x00417D24 */ G32(0x45AF3C) = (unsigned int)eax; /* mov ds:dword_45AF3C,eax */
    /* 0x00417D29 */ eax = 0; /* xor eax,eax */
    /* 0x00417D2B */ eax |= (unsigned long)G8(0x45C678); /* mov al,ds:byte_45C678 */
    /* 0x00417D30 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((0x45BA58UL + (eax & 0xFFUL)) & U32MASK); /* mov al,ds:byte_45BA58[eax] */
    /* 0x00417D36 */ M8(0x45C679UL) = (unsigned char)(eax & 0xFFUL); /* mov ds:byte_45C679,al */
l_417D3B:
    /* 0x00417D3B test dh,dh */;
    /* 0x00417D3D jz */ if (((edx >> 8) & 0xFFUL) == 0) goto l_417D65; /* equality */
    /* 0x00417D3F */ edx = (unsigned long)G32(0x45AF2C); /* mov edx,ds:dword_45AF2C */
    /* 0x00417D45 */ edx = (edx + 1) & U32MASK; /* inc edx */
    /* 0x00417D46 */ G32(0x45AF90) = (unsigned int)ebx; /* mov ds:dword_45AF90,ebx */
    /* 0x00417D4C */ G32(0x45AF2C) = (unsigned int)edx; /* mov ds:dword_45AF2C,edx */
    /* 0x00417D52 */ ebx = (unsigned long)G32(0x45AF90); /* mov ebx,ds:dword_45AF90 */
    /* epilogue 0x00417D58 add esp,4; 0x00417D5B pop ebp;
     * 0x00417D5C pop edi; 0x00417D5D pop esi; 0x00417D5E pop ecx;
     * 0x00417D5F pop ebx; 0x00417D60 retn. */
    return;
l_417D61:
    /* 0x00417D61 */ eax = (eax & 0xFFFFFF00UL); /* xor al,al (lane only; ah stale but dead: al==0 takes the jz below) */
    /* 0x00417D63 jmp */ goto l_417CFF; /* unconditional */
l_417D65:
    /* 0x00417D65 */ eax = 0; /* xor eax,eax */
    /* 0x00417D67 */ G32(0x45AF90) = (unsigned int)ebx; /* mov ds:dword_45AF90,ebx */
    /* 0x00417D6D */ G32(0x45AF2C) = (unsigned int)eax; /* mov ds:dword_45AF2C,eax (zero) */
    /* 0x00417D72 */ ebx = (unsigned long)G32(0x45AF90); /* mov ebx,ds:dword_45AF90 */
    /* epilogue 0x00417D78 add esp,4; 0x00417D7B pop ebp;
     * 0x00417D7C pop edi; 0x00417D7D pop esi; 0x00417D7E pop ecx;
     * 0x00417D7F pop ebx; 0x00417D80 retn. */
    return;
l_417D81:
    /* 0x00417D81 */ d4v_41e640(c); /* call sub_41E640 */
    /* 0x00417D86 */ ebx = (unsigned long)G32(0x45AF90); /* mov ebx,ds:dword_45AF90 */
    /* epilogue 0x00417D8C add esp,4; 0x00417D8F pop ebp;
     * 0x00417D90 pop edi; 0x00417D91 pop esi; 0x00417D92 pop ecx;
     * 0x00417D93 pop ebx; 0x00417D94 retn. */
    return;
}

/* ------------------------------------------------------------------ */
/* sub_4183D0 (lst 35411-35586) -> d4v_4183d0                          */
/*                                                                      */
/* Role: PPM child-table allocator/init. Folds the symbol (entry al,   */
/* complemented into dl) with the 45C689 escape-state machine and the  */
/* 45CD28/45BA44/43120C position bounds: either plants an escape flag  */
/* (45C689=1/0x20), emits the raw byte via sub_4221F0, runs the        */
/* 43691C-classified multi-byte emit loop (tables off_4362C0/436450,   */
/* 436976/45C68C-derived index), or the differenced single emit.       */
/* Entry-live regs: al = sym ONLY (first use mov dl,al). Deviation:    */
/* entry edx is destroyed by that same mov (callers set it anyway: 0   */
/* at 418357 via xor edx,edx, 1 at 419911 via mov edx,1), so the flag  */
/* parameter is currently dead; entry ecx is likewise never read       */
/* before overwrite (first read test cl,cl follows mov cl,[45C689]).   */
/* Kept as (c, sym, flag) per caller discipline, with flag documented  */
/* dead in this build.                                                 */
/* UNCERTAIN(004183D4): whether the edx slot is a vestigial flag or    */
/* padding is unknowable from two call sites; the translation keeps    */
/* it verbatim (dl overwritten on entry). Returns void (both callers  */
/* ignore eax; all exits leave call residue). Frame: no locals;        */
/* 4 pushed regs (ebx/ecx/edx/esi).                                    */
/* Globals touched: 43120C, 436976, 43691C, 4362C0, 436450, 45BA44, */
/* 45CD28, 45C689, 45C68C. */
/* Calls d4v_4221f0 (eax = emit item).                                 */
/* ------------------------------------------------------------------ */
void d4v_4183d0(dec4v_ctx *c, unsigned int sym, unsigned int flag)
{
    unsigned long eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0;

    /* prologue 0x004183D0 push ebx; 0x004183D1 push ecx;
     * 0x004183D2 push edx; 0x004183D3 push esi.
     * Callee-saved regs become C locals; there is no esp frame. */

    /* 0x004183D4 */ edx = (flag & 0xFFFFFF00UL) | ((unsigned long)sym & 0xFFUL); /* mov dl,al (entry edx destroyed: flag dead) */
    /* 0x004183D6 */ eax = (eax & 0xFFFF00FFUL) | ((unsigned long)G8(0x45C689) << 8); /* mov ah,ds:byte_45C689 (eax high dead: ah/al rewritten on all paths) */
    /* 0x004183DC */ edx = (edx & 0xFFFFFF00UL) | (((~(edx & 0xFFUL))) & 0xFFUL); /* not dl */
    /* 0x004183DE cmp ah,1 */;
    /* 0x004183E1 jz */ if (((eax >> 8) & 0xFFUL) == 1) goto l_418470; /* equality */
    /* 0x004183E7 */ eax = (unsigned long)G32(0x45CD28); /* mov eax,ds:dword_45CD28 */
    /* 0x004183EC cmp eax,ds:dword_45BA44 */;
    /* 0x004183F2 jb */ if (eax < (unsigned long)G32(0x45BA44)) goto l_4183FD; /* unsigned */
    /* 0x004183F4 cmp dl,0FEh */;
    /* 0x004183F7 jz */ if ((edx & 0xFFUL) == 0xFEUL) goto l_418503; /* equality */
l_4183FD:
    /* 0x004183FD */ eax = (unsigned long)G32(0x45CD28); /* mov eax,ds:dword_45CD28 */
    /* 0x00418402 cmp eax,ds:dword_43120C */;
    /* 0x00418408 jnb */ if (eax >= (unsigned long)G32(0x43120C)) goto l_41851C; /* unsigned */
    /* 0x0041840E cmp dl,80h */;
    /* 0x00418411 jb */ if ((edx & 0xFFUL) < 0x80UL) goto l_41850F; /* unsigned */
l_418417:
    /* 0x00418417 */ eax = 0; /* xor eax,eax */
    /* 0x00418419 */ eax |= (edx & 0xFFUL); /* mov al,dl */
    /* 0x0041841B */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((0x43691CUL + (eax & 0xFFUL)) & U32MASK); /* mov al,ds:byte_43691C[eax] */
    /* 0x00418421 cmp al,0C8h */;
    /* 0x00418423 jnb */ if ((eax & 0xFFUL) >= 0xC8UL) goto l_418569; /* unsigned */
    /* 0x00418429 cmp al,64h */;
    /* 0x0041842B jz */ if ((eax & 0xFFUL) == 0x64UL) goto l_41852A; /* equality */
    /* 0x00418431 cmp al,63h */;
    /* 0x00418433 jz */ if ((eax & 0xFFUL) == 0x63UL) goto l_41853D; /* equality */
    /* 0x00418439 */ ebx = 0; /* xor ebx,ebx */
    /* 0x0041843B */ ebx |= (eax & 0xFFUL); /* mov bl,al */
    /* 0x0041843D */ ecx = (ecx & 0xFFFFFF00UL) | (unsigned long)G8(0x45C689); /* mov cl,ds:byte_45C689 */
    /* 0x00418443 */ edx = (unsigned long)G32((0x4362C0UL + (ebx & 0xFFUL) * 4) & U32MASK); /* mov edx,ds:off_4362C0[ebx*4] */
    /* 0x0041844A */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)M8((0x436450UL + (ebx & 0xFFUL)) & U32MASK); /* mov bl,ds:byte_436450[ebx] */
    /* 0x00418450 test cl,cl */;
    /* 0x00418452 jnz */ if ((ecx & 0xFFUL) != 0) goto l_418549; /* equality */
l_418458:
    /* 0x00418458 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8(edx); /* mov al,[edx] */
    /* 0x0041845A */ eax = (eax & 0xFFFFFF00UL) | (((~(eax & 0xFFUL))) & 0xFFUL); /* not al */
    /* 0x0041845C */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00418461 */ edx = (edx + 1) & U32MASK; /* inc edx */
    /* 0x00418462 */ d4v_put_byte(c, (unsigned int)eax); /* call sub_4221F0 */
    /* 0x00418467 */ ebx = (ebx & 0xFFFFFF00UL) | (((ebx & 0xFFUL) - 1UL) & 0xFFUL); /* dec bl */
    /* 0x00418469 jnz */ if ((ebx & 0xFFUL) != 0) goto l_418458; /* equality */
    /* epilogue 0x0041846B pop esi; 0x0041846C pop edx;
     * 0x0041846D pop ecx; 0x0041846E pop ebx; 0x0041846F retn. */
    return;
l_418470:
    /* 0x00418470 */ eax = (unsigned long)G32(0x45CD28); /* mov eax,ds:dword_45CD28 */
    /* 0x00418475 */ edx &= 0xFFFF00FFUL; /* xor dh,dh */
    /* 0x00418477 */ esi = (unsigned long)G32(0x45BA44); /* mov esi,ds:dword_45BA44 */
    /* 0x0041847D */ M8(0x45C689UL) = (unsigned char)((edx >> 8) & 0xFFUL); /* mov ds:byte_45C689,dh (zero) */
    /* 0x00418483 cmp eax,esi */;
    /* 0x00418485 jb */ if (eax < esi) goto l_4184CB; /* unsigned */
    /* 0x00418487 cmp dl,0FEh */;
    /* 0x0041848A jnz */ if ((edx & 0xFFUL) != 0xFEUL) goto l_41849B; /* equality */
    /* 0x0041848C */ eax = 0xFEUL; /* mov eax,0FEh */
    /* 0x00418491 */ d4v_put_byte(c, (unsigned int)eax); /* call sub_4221F0 */
    /* epilogue 0x00418496 pop esi; 0x00418497 pop edx;
     * 0x00418498 pop ecx; 0x00418499 pop ebx; 0x0041849A retn. */
    return;
l_41849B:
    /* 0x0041849B */ eax = 0; /* xor eax,eax */
    /* 0x0041849D */ d4v_put_byte(c, (unsigned int)eax); /* call sub_4221F0 */
    /* 0x004184A2 */ eax = 0; /* xor eax,eax */
    /* 0x004184A4 */ d4v_put_byte(c, (unsigned int)eax); /* call sub_4221F0 */
    /* 0x004184A9 */ eax = 3; /* mov eax,3 */
    /* 0x004184AE */ d4v_put_byte(c, (unsigned int)eax); /* call sub_4221F0 */
    /* 0x004184B3 */ eax = 3; /* mov eax,3 */
    /* 0x004184B8 */ d4v_put_byte(c, (unsigned int)eax); /* call sub_4221F0 */
    /* 0x004184BD */ eax = 0; /* xor eax,eax */
    /* 0x004184BF */ eax |= (edx & 0xFFUL); /* mov al,dl */
    /* 0x004184C1 */ d4v_put_byte(c, (unsigned int)eax); /* call sub_4221F0 */
    /* epilogue 0x004184C6 pop esi; 0x004184C7 pop edx;
     * 0x004184C8 pop ecx; 0x004184C9 pop ebx; 0x004184CA retn. */
    return;
l_4184CB:
    /* 0x004184CB */ ecx = 0; /* xor ecx,ecx */
    /* 0x004184CD */ ecx |= (unsigned long)G8(0x45C68C); /* mov cl,ds:byte_45C68C */
    /* 0x004184D3 */ ecx = (ecx - 0xA6UL) & U32MASK; /* sub ecx,0A6h */
    /* 0x004184D9 */ eax = (ecx * 4) & U32MASK; /* lea eax,ds:0[ecx*4] */
    /* 0x004184E0 */ eax = (eax - ecx) & U32MASK; /* sub eax,ecx */
    /* 0x004184E2 */ eax = (eax << 2) & U32MASK; /* shl eax,2 */
    /* 0x004184E5 */ eax = (eax - ecx) & U32MASK; /* sub eax,ecx (eax = 11*(cl-0xA6)) */
    /* 0x004184E7 */ ecx = eax; /* mov ecx,eax */
    /* 0x004184E9 */ eax = 0; /* xor eax,eax */
    /* 0x004184EB */ eax |= (edx & 0xFFUL); /* mov al,dl */
    /* 0x004184ED */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((0x436976UL + ecx + (eax & 0xFFUL)) & U32MASK); /* mov al,ds:byte_436976[ecx+eax] */
    /* 0x004184F4 */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x004184F9 */ d4v_put_byte(c, (unsigned int)eax); /* call sub_4221F0 */
    /* epilogue 0x004184FE pop esi; 0x004184FF pop edx;
     * 0x00418500 pop ecx; 0x00418501 pop ebx; 0x00418502 retn. */
    return;
l_418503:
    /* 0x00418503 */ M8(0x45C689UL) = 1; /* mov ds:byte_45C689,1 */
    /* epilogue 0x0041850A pop esi; 0x0041850B pop edx;
     * 0x0041850C pop ecx; 0x0041850D pop ebx; 0x0041850E retn. */
    return;
l_41850F:
    /* 0x0041850F cmp ds:byte_45C689,0 */;
    /* 0x00418516 jnz */ if (G8(0x45C689) != 0) goto l_418417; /* equality */
l_41851C:
    /* 0x0041851C */ eax = 0; /* xor eax,eax */
    /* 0x0041851E */ eax |= (edx & 0xFFUL); /* mov al,dl */
    /* 0x00418520 */ d4v_put_byte(c, (unsigned int)eax); /* call sub_4221F0 */
    /* epilogue 0x00418525 pop esi; 0x00418526 pop edx;
     * 0x00418527 pop ecx; 0x00418528 pop ebx; 0x00418529 retn. */
    return;
l_41852A:
    /* 0x0041852A */ eax = (eax & 0xFFFF00FFUL) | (1UL << 8); /* mov ah,1 */
    /* 0x0041852C */ M8(0x45C68CUL) = (unsigned char)(edx & 0xFFUL); /* mov ds:byte_45C68C,dl */
    /* 0x00418532 */ M8(0x45C689UL) = (unsigned char)((eax >> 8) & 0xFFUL); /* mov ds:byte_45C689,ah */
    /* epilogue 0x00418538 pop esi; 0x00418539 pop edx;
     * 0x0041853A pop ecx; 0x0041853B pop ebx; 0x0041853C retn. */
    return;
l_41853D:
    /* 0x0041853D */ M8(0x45C689UL) = 0x20; /* mov ds:byte_45C689,20h */
    /* epilogue 0x00418544 pop esi; 0x00418545 pop edx;
     * 0x00418546 pop ecx; 0x00418547 pop ebx; 0x00418548 retn. */
    return;
l_418549:
    /* 0x00418549 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8(edx); /* mov al,[edx] */
    /* 0x0041854B */ eax = (eax & 0xFFFFFF00UL) | (((~(eax & 0xFFUL))) & 0xFFUL); /* not al */
    /* 0x0041854D */ edx = (edx + 1) & U32MASK; /* inc edx */
    /* 0x0041854E */ eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFUL) - 0x20UL) & 0xFFUL); /* sub al,20h */
    /* 0x00418550 */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00418555 */ d4v_put_byte(c, (unsigned int)eax); /* call sub_4221F0 */
    /* 0x0041855A */ ecx &= 0xFFFF00FFUL; /* xor ch,ch */
    /* 0x0041855C */ DECB_LO(ebx); /* dec bl */
    /* 0x0041855E */ M8(0x45C689UL) = (unsigned char)((ecx >> 8) & 0xFFUL); /* mov ds:byte_45C689,ch (zero) */
    /* 0x00418564 jmp */ goto l_418458; /* unconditional */
l_418569:
    /* 0x00418569 */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(0x45C689); /* mov bl,ds:byte_45C689 (ebx high dead: only bl read) */
    /* 0x0041856F */ eax = (eax & 0xFFFFFF00UL) | (edx & 0xFFUL); /* mov al,dl */
    /* 0x00418571 */ SUB8_LO(eax, ebx & 0xFFUL); /* sub al,bl */
    /* 0x00418573 */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00418578 */ ebx &= 0xFFFF00FFUL; /* xor bh,bh */
    /* 0x0041857A */ d4v_put_byte(c, (unsigned int)eax); /* call sub_4221F0 */
    /* 0x0041857F */ M8(0x45C689UL) = (unsigned char)((ebx >> 8) & 0xFFUL); /* mov ds:byte_45C689,bh (zero) */
    /* epilogue 0x00418585 pop esi; 0x00418586 pop edx;
     * 0x00418587 pop ecx; 0x00418588 pop ebx; 0x00418589 retn. */
    return;
}

/* ---- workers: PPM helpers batch S3 (from draft_s3.c) ---- */
void d4v_416c80(dec4v_ctx *c, unsigned int len, unsigned int flag);
void d4v_417090(dec4v_ctx *c, unsigned int sym);
void d4v_417220(dec4v_ctx *c, unsigned int sym);
void d4v_417570(dec4v_ctx *c, unsigned int sym);
unsigned int d4v_418590(dec4v_ctx *c, unsigned int len);
int d4v_41cf60(dec4v_ctx *c, unsigned int a);
void d4v_41e5f0(dec4v_ctx *c);

/* sub_416C80 (UHARC04.lst:33289-33346).
 * Role: PPM per-length touch/rescale on the 8-byte entry at
 * unk_459BA8 + (len&0xFF)*8 (words at +2/+4/+6), then rotate
 * byte_45C691 through byte_433E77[2*old + dl].
 * Entry-live regs (first use): eax (ANDed to 0xFF at 0x416C83),
 * edx (dl compared at 0x416C90). ebx/esi are scratch (xor-clear
 * before use at 0x416CD1-0x416CD3); ecx is scratch.
 * Frame: no locals; pushes ebx/ecx/esi only.
 * Deviation: none (void return; original leaves garbage in eax).
 */
void d4v_416c80(dec4v_ctx *c, unsigned int len, unsigned int flag)
{
    unsigned int p = 0;    /* entry = 0x459BA8 + (len & 0xFF) * 8 */
    unsigned int eax = 0;  /* eax/al scratch */
    unsigned int ebx = 0;  /* bx/ebx scratch */
    unsigned int ecx = 0;  /* cx/ecx scratch */
    unsigned int esi = 0;  /* esi scratch */
    unsigned int dl = 0;   /* dl flag lane */

    /* push ebx */ /* 0x416C80 */
    /* push ecx */ /* 0x416C81 */
    /* push esi */ /* 0x416C82 */
    dl = flag & 0xFFU; /* (dl on entry) */
    eax = len & 0xFFU; /* 0x416C83 and eax,0FFh */
    eax = (eax << 3) & U32MASK; /* 0x416C88 shl eax,3 */
    p = 0x459BA8UL + eax; /* 0x416C8B add eax,offset unk_459BA8 */
    if (dl == 1) /* 0x416C90 cmp dl,1 */ /* 0x416C93 jz short loc_416CC5 */
        goto l_416CC5;
l_416C95:
    ecx = (unsigned int)M16(p + 6); /* 0x416C95 mov cx,[eax+6] */
    ecx = (ecx + 0x20U) & U32MASK; /* 0x416C99 add ecx,20h */
    M16(p + 6) = (unsigned short)ecx; /* 0x416C9C mov [eax+6],cx */
    if ((ecx & 0xFFFFU) >= 0x4000U) /* 0x416CA0 cmp cx,4000h */ /* 0x416CA5 jnb short loc_416CD1 */
        goto l_416CD1;
l_416CA7:
    eax = 0; /* 0x416CA7 xor eax,eax */
    eax = (unsigned int)G8(0x45C691); /* 0x416CA9 mov al,ds:byte_45C691 */
    ecx = (eax + eax) & U32MASK; /* 0x416CAE lea ecx,[eax+eax] */
    eax = 0; /* 0x416CB1 xor eax,eax */
    eax = dl; /* 0x416CB3 mov al,dl */
    eax = (unsigned int)M8(0x433E77UL + ecx + eax); /* 0x416CB5 mov al,ds:byte_433E77[ecx+eax] */
    G8(0x45C691) = (unsigned char)eax; /* 0x416CBC mov ds:byte_45C691,al */
    /* pop esi */ /* 0x416CC1 */
    /* pop ecx */ /* 0x416CC2 */
    /* pop ebx */ /* 0x416CC3 */
    return; /* 0x416CC4 retn */
l_416CC5:
    ecx = 0; /* 0x416CC5 xor ecx,ecx */
    ecx = dl; /* 0x416CC7 mov cl,dl */
    M16(p + 2 + ecx * 2) = (unsigned short)(M16(p + 2 + ecx * 2) + 0x20U); /* 0x416CC9 add word ptr [eax+ecx*2+2],20h */
    goto l_416C95; /* 0x416CCF jmp short loc_416C95 */
l_416CD1:
    ecx = 0; /* 0x416CD1 xor ecx,ecx */
    ebx = 0; /* 0x416CD3 xor ebx,ebx */
    ecx = (unsigned int)M16(p + 4); /* 0x416CD5 mov cx,[eax+4] */
    ebx = (unsigned int)M16(p + 2); /* 0x416CD9 mov bx,[eax+2] */
    esi = ecx; /* 0x416CDD mov esi,ecx */
    esi = (esi - ebx) & U32MASK; /* 0x416CDF sub esi,ebx */
    ebx = (esi + 1) & U32MASK; /* 0x416CE1 lea ebx,[esi+1] */
    esi = (unsigned int)M16(p + 6); /* 0x416CE4 xor esi,esi */ /* 0x416CE6 mov si,[eax+6] */
    esi = (esi - ecx) & U32MASK; /* 0x416CEA sub esi,ecx */
    ecx = (esi + 1) & U32MASK; /* 0x416CEC lea ecx,[esi+1] */
    ecx = (unsigned int)d4v_sar(ecx, 1); /* 0x416CEF sar ecx,1 */
    ebx = (unsigned int)d4v_sar(ebx, 1); /* 0x416CF1 sar ebx,1 */
    esi = ecx; /* 0x416CF3 mov esi,ecx */
    ecx = (unsigned int)M16(p + 2); /* 0x416CF5 mov cx,[eax+2] */
    ebx = (ebx + ecx) & U32MASK; /* 0x416CF9 add ebx,ecx */
    esi = (esi + ebx) & U32MASK; /* 0x416CFB add esi,ebx */
    M16(p + 4) = (unsigned short)ebx; /* 0x416CFD mov [eax+4],bx */
    M16(p + 6) = (unsigned short)esi; /* 0x416D01 mov [eax+6],si */
    goto l_416CA7; /* 0x416D05 jmp short loc_416CA7 */
}

/* sub_41E5F0 (UHARC04.lst:44053-44077).
 * Role: reset the small model tables: fill 0x1E40 bytes at 0x451920
 * with 0, set dword_451928 = 2, and store -1 into the three dwords of
 * each of 4 entries stepping 0x7C over 0x453560+{0x7C..0x1F0}.
 * Entry-live regs: none (ebx/eax/edx/ecx all set from immediates
 * before first use at 0x41E5F3-0x41E5FF).
 * Frame: no locals; pushes ebx/ecx/edx only.
 * Deviation: void return; the original retn leaves garbage eax = 0x1F0.
 */
void d4v_41e5f0(dec4v_ctx *c)
{
    unsigned int eax = 0;  /* loop cursor 0x7C..0x1F0 */
    unsigned int ecx = 0;  /* fill/count constant */
    unsigned int edx = 0;  /* fill value / 2 */

    /* push ebx */ /* 0x41E5F0 */
    /* push ecx */ /* 0x41E5F1 */
    /* push edx */ /* 0x41E5F2 */
    /* ebx = 0x1E40 */ /* 0x41E5F3 mov ebx,1E40h */
    /* eax = 0x451920; edx = 0; ecx = -1 */ /* 0x41E5F8 mov eax,offset dword_451920 */ /* 0x41E5FD xor edx,edx */ /* 0x41E5FF mov ecx,0FFFFFFFFh */
    d4v_fill(c, 0x1E40U, 0x451920UL, 0U); /* 0x41E604 call sub_40D570 */
    edx = 2; /* 0x41E609 mov edx,2 */
    eax = 0; /* 0x41E60E xor eax,eax */
    G32(0x451928) = edx; /* 0x41E610 mov ds:dword_451928,edx */
l_41E616:
    eax = (eax + 0x7CU) & U32MASK; /* 0x41E616 add eax,7Ch */
    ecx = 0xFFFFFFFFUL;
    M32(0x453560UL + eax) = ecx; /* 0x41E619 mov ds:dword_453560[eax],ecx */
    M32(0x453564UL + eax) = ecx; /* 0x41E61F mov ds:dword_453564[eax],ecx */
    M32(0x453568UL + eax) = ecx; /* 0x41E625 mov ds:dword_453568[eax],ecx */
    if (eax != 0x1F0U) /* 0x41E62B cmp eax,1F0h */ /* 0x41E630 jnz short loc_41E616 */
        goto l_41E616;
    /* pop edx */ /* 0x41E632 */
    /* pop ecx */ /* 0x41E633 */
    /* pop ebx */ /* 0x41E634 */
    return; /* 0x41E635 retn (garbage eax = 0x1F0 discarded) */
}

/* sub_418590 (UHARC04.lst:35592-35759).
 * Role: PPM seen/escape-table range-decode for one length class:
 * stage 1 range-decodes an item of class (al) over the 8-byte entry at
 * unk_459BA8 + (al&0xFF)*8 (returns the surviving stage index 1..3 in
 * al after touching the model via sub_416C80); stage 2 (index 0)
 * range-decodes a 10-way symbol, updates byte_45C67D + the
 * dword_4342C4/43429C/byte_434314 tables, and loops until stage 1
 * yields nonzero (calling sub_41E5F0 when the new class exceeds 4).
 * Entry-live regs (first use): eax (only al saved at 0x418598).
 * Frame: 8 bytes locals (var_18 byte=len, var_1C byte=stage) over
 * pushes ebx/ecx/edx/esi/ebp (esp+1Ch addressing).
 * Deviations: div-by-zero: direct `/` with comment (4 sites: the two
 * stage-1 divs and the two stage-2 divs); x86 div would also #DE on
 * quotient overflow, which the model keeps in range.
 */
unsigned int d4v_418590(dec4v_ctx *c, unsigned int len)
{
    unsigned int p = 0;      /* 8-byte table entry */
    unsigned int ebp = 0;    /* = 0x45AF68 (3-dword range-decoder slot) */
    unsigned int eax = 0;    /* eax/ax/al scratch, div quotients */
    unsigned int ebx = 0;    /* ebx/bx/bl scratch, table cursor/symbol */
    unsigned int ecx = 0;    /* ecx/cx/ch scratch */
    unsigned int edx = 0;    /* edx/dx/dl scratch, range */
    unsigned int esi = 0;    /* esi/symbol index scratch */
    unsigned int var_18 = 0; /* saved entry al (length class) */
    unsigned int var_1C = 0; /* stage counter 3..0 */

    /* push ebx */ /* 0x418590 */
    /* push ecx */ /* 0x418591 */
    /* push edx */ /* 0x418592 */
    /* push esi */ /* 0x418593 */
    /* push ebp */ /* 0x418594 */
    /* sub esp,8 */ /* 0x418595 */
    var_18 = len & 0xFFU; /* 0x418598 mov [esp+1Ch+var_18],al */
    ebp = 0x45AF68UL; /* 0x41859C mov ebp,offset dword_45AF68 */
l_4185A1:
    eax = var_18; /* 0x4185A1 xor eax,eax */ /* 0x4185A3 mov al,[esp+1Ch+var_18] */
    eax = (eax << 3) & U32MASK; /* 0x4185A7 shl eax,3 */
    p = 0x459BA8UL + eax; /* 0x4185AA add eax,offset unk_459BA8 */
    ebx = p + 6; /* 0x4185AF lea ebx,[eax+6] */
    eax = 0; /* 0x4185B2 xor eax,eax */
    eax = (unsigned int)M16(ebx); /* 0x4185B4 mov ax,[ebx] */
    ecx = eax; /* 0x4185B7 mov ecx,eax */
    G32(0x45AF68) = eax; /* 0x4185B9 mov ds:dword_45AF68,eax */
    eax = G32(0x45D0BC); /* 0x4185BE mov eax,ds:dword_45D0BC */
    /* 0x4185C3 xor edx,edx; 0x4185C5 div ecx: direct `/` (div-by-zero if ecx==0) */
    eax = eax / ecx; /* 0x4185C5 div ecx */
    edx = 0; /* 0x4185C7 xor edx,edx */
    G32(0x45D0BC) = eax; /* 0x4185C9 mov ds:dword_45D0BC,eax */
    ecx = eax; /* 0x4185CE mov ecx,eax */
    eax = G32(0x45D0B8); /* 0x4185D0 mov eax,ds:dword_45D0B8 */
    /* 0x4185D5 div ecx: direct `/` (div-by-zero if ecx==0) */
    eax = eax / ecx; /* 0x4185D5 div ecx */
    var_1C = 3; /* 0x4185D7 mov [esp+1Ch+var_1C],3 */
l_4185DB:
    var_1C = (var_1C - 1) & 0xFFU; /* 0x4185DB dec [esp+1Ch+var_1C] */
    edx = (edx & 0xFFFF0000UL) | (unsigned int)M16(ebx - 2); /* 0x4185DE mov dx,[ebx-2] (dx lane only) */
    ebx = (ebx - 2) & U32MASK; /* 0x4185E2 sub ebx,2 */
    if ((eax & 0xFFFFU) >= (edx & 0xFFFFU)) /* 0x4185E5 cmp ax,dx */ /* 0x4185E8 jnb short loc_4185F0 */
        goto l_4185F0;
    if (var_1C != 0) /* 0x4185EA cmp [esp+1Ch+var_1C],0 */ /* 0x4185EE jnz short loc_4185DB */
        goto l_4185DB;
l_4185F0:
    eax = 0; /* 0x4185F0 xor eax,eax */
    eax = (unsigned int)M16(ebx); /* 0x4185F2 mov ax,[ebx] */
    G32(0x45AF6C) = eax; /* 0x4185F5 mov ds:dword_45AF6C,eax */
    eax = 0; /* 0x4185FA xor eax,eax */
    eax = (unsigned int)M16(ebx + 2); /* 0x4185FC mov ax,[ebx+2] */
    G32(0x45AF70) = eax; /* 0x418600 mov ds:dword_45AF70,eax */
    eax = ebp; /* 0x418605 mov eax,ebp */
    edx = M32(eax + 8); /* 0x418607 mov edx,[eax+8] */
    eax = M32(eax + 4); /* 0x41860A mov eax,[eax+4] */
    ecx = G32(0x45D0BC); /* 0x41860D mov ecx,ds:dword_45D0BC */
    edx = (edx - eax) & U32MASK; /* 0x418613 sub edx,eax */
    eax = (eax * ecx) & U32MASK; /* 0x418615 imul eax,ecx */
    edx = (edx * ecx) & U32MASK; /* 0x418618 imul edx,ecx */
    ecx = G32(0x45D0B8); /* 0x41861B mov ecx,ds:dword_45D0B8 */
    ecx = (ecx - eax) & U32MASK; /* 0x418621 sub ecx,eax */
    if (edx >= 0x1000000UL) /* 0x418623 cmp edx,1000000h */ /* 0x418629 jnb short loc_418645 */
        goto l_418645;
l_41862B:
    eax = d4v_get_byte(c); /* 0x41862B call sub_4220B0 */
    ecx = (ecx << 8) & U32MASK; /* 0x418630 shl ecx,8 */
    eax &= 0xFFU; /* 0x418633 and eax,0FFh */
    edx = (edx << 8) & U32MASK; /* 0x418638 shl edx,8 */
    ecx |= eax; /* 0x41863B or ecx,eax */
    if (edx < 0x1000000UL) /* 0x41863D cmp edx,1000000h */ /* 0x418643 jb short loc_41862B */
        goto l_41862B;
l_418645:
    G32(0x45D0B8) = ecx; /* 0x418645 mov ds:dword_45D0B8,ecx */
    G32(0x45D0BC) = edx; /* 0x41864B mov ds:dword_45D0BC,edx */
    ecx = (ecx & 0xFFFF00FFUL) | ((var_1C & 0xFFU) << 8); /* 0x418651 mov ch,[esp+1Ch+var_1C] */
    if (((ecx >> 8) & 0xFFU) == 0) /* 0x418654 test ch,ch */ /* 0x418656 jz short loc_418672 */
        goto l_418672;
    eax = 0; /* 0x418658 xor eax,eax */
    edx = 0; /* 0x41865A xor edx,edx */
    eax = var_18; /* 0x41865C mov al,[esp+1Ch+var_18] */
    edx = var_1C & 0xFFU; /* 0x418660 mov dl,ch */
    d4v_416c80(c, eax, edx); /* 0x418662 call sub_416C80 */
    eax = var_1C & 0xFFU; /* 0x418667 mov al,ch */
    /* UNCERTAIN(00418667): `mov al,ch` leaves stale ah (garbage from the
     * sub_416C80 call); callers use al only, so we zero-extend here. */
    /* add esp,8; pop ebp/esi/edx/ecx/ebx */ /* 0x418669 add esp,8 */ /* 0x41866C pop ebp */ /* 0x41866D pop esi */ /* 0x41866E pop edx */ /* 0x41866F pop ecx */ /* 0x418670 pop ebx */
    return eax; /* 0x418671 retn */
l_418672:
    edx = 10; /* 0x418672 mov edx,0Ah */
    ecx = edx; /* 0x418677 mov ecx,edx */
    G32(0x45AF68) = edx; /* 0x418679 mov ds:dword_45AF68,edx */
    eax = G32(0x45D0BC); /* 0x41867F mov eax,ds:dword_45D0BC */
    edx = 0; /* 0x418684 xor edx,edx */
    /* 0x418686 div ecx: direct `/` (div-by-zero if ecx==0; here ecx=10) */
    eax = eax / ecx; /* 0x418686 div ecx */
    edx = 0; /* 0x418688 xor edx,edx */
    G32(0x45D0BC) = eax; /* 0x41868A mov ds:dword_45D0BC,eax */
    ecx = eax; /* 0x41868F mov ecx,eax */
    eax = G32(0x45D0B8); /* 0x418691 mov eax,ds:dword_45D0B8 */
    /* 0x418696 div ecx: direct `/` (div-by-zero if ecx==0) */
    eax = eax / ecx; /* 0x418696 div ecx */
    ebx = eax; /* 0x418698 mov ebx,eax */
    eax = 0; /* 0x41869A xor eax,eax */
    eax = ebx & 0xFFU; /* 0x41869C mov al,bl */
    esi = ebx & 0xFFU; /* 0x41869E movzx esi,bl */
    G32(0x45AF6C) = eax; /* 0x4186A1 mov ds:dword_45AF6C,eax */
    eax = (esi + 1) & U32MASK; /* 0x4186A6 lea eax,[esi+1] */
    G32(0x45AF70) = eax; /* 0x4186A9 mov ds:dword_45AF70,eax */
    eax = ebp; /* 0x4186AE mov eax,ebp */
    edx = M32(eax + 8); /* 0x4186B0 mov edx,[eax+8] */
    eax = M32(eax + 4); /* 0x4186B3 mov eax,[eax+4] */
    ecx = G32(0x45D0BC); /* 0x4186B6 mov ecx,ds:dword_45D0BC */
    edx = (edx - eax) & U32MASK; /* 0x4186BC sub edx,eax */
    eax = (eax * ecx) & U32MASK; /* 0x4186BE imul eax,ecx */
    edx = (edx * ecx) & U32MASK; /* 0x4186C1 imul edx,ecx */
    ecx = G32(0x45D0B8); /* 0x4186C4 mov ecx,ds:dword_45D0B8 */
    ecx = (ecx - eax) & U32MASK; /* 0x4186CA sub ecx,eax */
    if (edx >= 0x1000000UL) /* 0x4186CC cmp edx,1000000h */ /* 0x4186D2 jnb short loc_4186EE */
        goto l_4186EE;
l_4186D4:
    eax = d4v_get_byte(c); /* 0x4186D4 call sub_4220B0 */
    ecx = (ecx << 8) & U32MASK; /* 0x4186D9 shl ecx,8 */
    eax &= 0xFFU; /* 0x4186DC and eax,0FFh */
    edx = (edx << 8) & U32MASK; /* 0x4186E1 shl edx,8 */
    ecx |= eax; /* 0x4186E4 or ecx,eax */
    if (edx < 0x1000000UL) /* 0x4186E6 cmp edx,1000000h */ /* 0x4186EC jb short loc_4186D4 */
        goto l_4186D4;
l_4186EE:
    G32(0x45D0B8) = ecx; /* 0x4186EE mov ds:dword_45D0B8,ecx */
    G32(0x45D0BC) = edx; /* 0x4186F4 mov ds:dword_45D0BC,edx */
    if ((ebx & 0xFFU) == (unsigned int)G8(0x45C67D)) /* 0x4186FA cmp bl,ds:byte_45C67D */ /* 0x418700 jz loc_4185A1 */
        goto l_4185A1;
    G8(0x45C67D) = (unsigned char)(ebx & 0xFFU); /* 0x418706 mov ds:byte_45C67D,bl */
    eax = M32(0x4342C4UL + esi * 4); /* 0x41870C mov eax,ds:dword_4342C4[esi*4] */
    ecx = M32(0x43429CUL + esi * 4); /* 0x418713 mov ecx,ds:dword_43429C[esi*4] */
    G32(0x45AF3C) = eax; /* 0x41871A mov ds:dword_45AF3C,eax */
    eax = (unsigned int)M8(0x434314UL + esi); /* 0x41871F mov al,ds:byte_434314[esi] */
    G32(0x45AF40) = ecx; /* 0x418725 mov ds:dword_45AF40,ecx */
    G8(0x45C67A) = (unsigned char)eax; /* 0x41872B mov ds:byte_45C67A,al */
    if ((ebx & 0xFFU) == 5) /* 0x418730 cmp bl,5 */ /* 0x418733 jnz short loc_41876B */
        goto l_418735;
l_41876B:
    if ((ebx & 0xFFU) == 6) /* 0x41876B cmp bl,6 */ /* 0x41876E jz short loc_418735 */
        goto l_418735;
    eax &= 0xFFFFFF00UL; /* 0x418770 xor al,al (al lane only) */
    goto l_418737; /* 0x418772 jmp short loc_418737 */
l_418735:
    eax = 1; /* 0x418735 mov al,1 */
l_418737:
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)G8(0x45C67D) << 8) & 0xFF00U); /* 0x418737 mov dh,ds:byte_45C67D */
    G8(0x45C688) = (unsigned char)eax; /* 0x41873D mov ds:byte_45C688,al */
    ecx = 0; /* 0x418742 xor ecx,ecx */
    eax &= 0xFFFF00FFUL; /* 0x418744 xor ah,ah */
    G32(0x45AF48) = ecx; /* 0x418746 mov ds:dword_45AF48,ecx */
    G8(0x45C67B) = 0; /* 0x41874C mov ds:byte_45C67B,ah */
    G8(0x45C679) = 0; /* 0x418752 mov ds:byte_45C679,ah */
    if (((edx >> 8) & 0xFFU) <= 4) /* 0x418758 cmp dh,4 */ /* 0x41875B jbe loc_4185A1 */
        goto l_4185A1;
    d4v_41e5f0(c); /* 0x418761 call sub_41E5F0 */
    goto l_4185A1; /* 0x418766 jmp loc_4185A1 */
}

/* sub_417090 (UHARC04.lst:33650-33823).
 * Role: PPM per-order frequency rescale for one symbol: for each of the
 * 8 orders (bh = 0..7) locate the context entry via dword_459E38[bh]
 * and dword_433B58[bh], add the price term
 * word_437968[((cnt0|cnt1 combined) << 14)/(cnt0+cnt1)] to dword_45AF88,
 * bump the matching symbol/total counters (halving both lanes at
 * 0xFE), and re-target a mismatched entry at byte_45C67D-class rules.
 * Entry-live regs (first use): eax (only al saved to bl at 0x4170A2).
 * Frame: no locals; pushes ebx/ecx/edx/esi/edi/ebp only.
 * Deviations: div-by-zero: direct `/` with comment (0x4170FD site);
 * x86 div would also #DE on quotient overflow, model-kept in range.
 */
void d4v_417090(dec4v_ctx *c, unsigned int sym)
{
    unsigned int p = 0;    /* context entry pointer */
    unsigned int eax = 0;  /* eax/ax/ah/al scratch, div quotient */
    unsigned int ebx = 0;  /* bl = symbol (bh is the order loop counter) */
    unsigned int ecx = 0;  /* ecx ptr scratch */
    unsigned int edx = 0;  /* edx/dl/dh scratch, div remainder */
    unsigned int edi = 0;  /* edi scratch */
    unsigned int esi = 0;  /* esi = dword_45BA4C (never rewritten here) */
    unsigned int ebp = 0;  /* ebp = dword_45AF88 accumulator */
    unsigned int bh = 0;   /* order counter 0..7 (bh lane of ebx) */

    /* push ebx/ecx/edx/esi/edi/ebp */ /* 0x417090 push ebx */ /* 0x417091 push ecx */ /* 0x417092 push edx */ /* 0x417093 push esi */ /* 0x417094 push edi */ /* 0x417095 push ebp */
    esi = G32(0x45BA4C); /* 0x417096 mov esi,ds:dword_45BA4C */
    ebp = G32(0x45AF88); /* 0x41709C mov ebp,ds:dword_45AF88 */
    ebx = sym & 0xFFU; /* 0x4170A2 mov bl,al */
    ebx &= 0xFFFF00FFUL; /* 0x4170A4 xor bh,bh */
    bh = 0;
l_4170A6:
    eax = 0; /* 0x4170A6 xor eax,eax */
    eax = bh & 0xFFU; /* 0x4170A8 mov al,bh */
    edi = M32(0x459E38UL + eax * 4); /* 0x4170AA mov edi,ds:dword_459E38[eax*4] */
    ecx = M32(0x433B58UL + eax * 4); /* 0x4170B1 mov ecx,ds:dword_433B58[eax*4] */
    edi = (edi << 2) & U32MASK; /* 0x4170B8 shl edi,2 */
    p = (ecx + edi) & U32MASK; /* 0x4170BB add ecx,edi */
    eax = ((unsigned int)M8(p + 1) << 8) & 0xFF00U; /* 0x4170BD mov ah,[ecx+1] (ah lane) */
    if (((eax >> 8) & 0xFFU) == 0) /* 0x4170C0 test ah,ah */ /* 0x4170C2 jz loc_417213 */
        goto l_417213;
    if (bh != (unsigned int)G8(0x45C682)) /* 0x4170C8 cmp bh,ds:byte_45C682 */ /* 0x4170CE jnz short loc_417145 */
        goto l_417145;
    if (G8(0x45C685) == 0) /* 0x4170D0 cmp ds:byte_45C685,0 */ /* 0x4170D7 jz short loc_41710E */
        goto l_41710E;
    if ((ebx & 0xFFU) != (unsigned int)M8(p + 2)) /* 0x4170D9 cmp bl,[ecx+2] */ /* 0x4170DC jnz loc_4171A9 */
        goto l_4171A9;
    eax &= 0xFFFF00FFUL; /* 0x4170E2 xor ah,ah */
    G8(0x45C68D) = (unsigned char)((eax >> 8) & 0xFFU); /* 0x4170E4 mov ds:byte_45C68D,ah */
    eax = 0; /* 0x4170EA xor eax,eax */
    eax = (unsigned int)M8(p); /* 0x4170EC mov al,[ecx] */
l_4170EE:
    edx = 0; /* 0x4170EE xor edx,edx */
    edi = (unsigned int)M8(p); /* 0x4170F0 movzx edi,byte ptr [ecx] */
    edx = (unsigned int)M8(p + 1); /* 0x4170F3 mov dl,[ecx+1] */
    eax = (eax << 14) & U32MASK; /* 0x4170F6 shl eax,0Eh */
    edi = (edi + edx) & U32MASK; /* 0x4170F9 add edi,edx */
    edx = 0; /* 0x4170FB xor edx,edx */
    /* 0x4170FD div edi: direct `/` (div-by-zero if edi==0) */
    eax = eax / edi; /* 0x4170FD div edi */
    eax = (unsigned int)M16(0x437968UL + (eax & U32MASK) * 2); /* 0x4170FF mov ax,ds:word_437968[eax*2] */
    eax &= 0xFFFFU; /* 0x417107 and eax,0FFFFh */
    ebp = (ebp + eax) & U32MASK; /* 0x41710C add ebp,eax */
l_41710E:
    if (esi == 0) /* 0x41710E test esi,esi */ /* 0x417110 jz short loc_417145 */
        goto l_417145;
    if ((ebx & 0xFFU) != (unsigned int)M8(p + 2)) /* 0x417112 cmp bl,[ecx+2] */ /* 0x417115 jnz loc_4171B3 */
        goto l_4171B3;
    M8(esi) = (unsigned char)(M8(esi) + 1); /* 0x41711B inc byte ptr [esi] */
l_41711D:
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(esi) << 8) & 0xFF00U); /* 0x41711D mov dh,[esi] */
    eax = esi; /* 0x41711F mov eax,esi (result unused below; kept literal) */
    if (((edx >> 8) & 0xFFU) < 0xFEU) /* 0x417121 cmp dh,0FEh */ /* 0x417124 jb loc_4171BB */
        goto l_4171BB;
l_41712A:
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(esi) << 8) & 0xFF00U); /* 0x41712A mov dh,[esi] */
    edx = (edx & 0xFFFF00FFUL) | ((((((edx >> 8) & 0xFFU) + 1) & 0xFFU) << 8) & 0xFF00U); /* 0x41712C inc dh */
    M8(esi) = (unsigned char)((edx >> 8) & 0xFFU); /* 0x41712E mov [esi],dh */
    edx = (edx & 0xFFFFFF00UL) | (((((edx >> 8) & 0xFFU) >> 1) & 0xFFU)); /* 0x417130 mov dl,dh */ /* 0x417132 shr dl,1 */
    M8(esi) = (unsigned char)(edx & 0xFFU); /* 0x417134 mov [esi],dl */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(esi + 1) << 8) & 0xFF00U); /* 0x417136 mov dh,[esi+1] */
    edx = (edx & 0xFFFF00FFUL) | ((((((edx >> 8) & 0xFFU) + 1) & 0xFFU) << 8) & 0xFF00U); /* 0x417139 inc dh */
    M8(esi + 1) = (unsigned char)((edx >> 8) & 0xFFU); /* 0x41713B mov [esi+1],dh */
    edx = (edx & 0xFFFFFF00UL) | (((((edx >> 8) & 0xFFU) >> 1) & 0xFFU)); /* 0x41713E mov dl,dh */ /* 0x417140 shr dl,1 */
    M8(esi + 1) = (unsigned char)(edx & 0xFFU); /* 0x417142 mov [esi+1],dl */
l_417145:
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(p + 3) << 8) & 0xFF00U); /* 0x417145 mov dh,[ecx+3] */
    edx &= 0xFFFF7FFFUL; /* 0x417148 and dh,7Fh */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8(p + 2); /* 0x41714B mov al,[ecx+2] */
    M8(p + 3) = (unsigned char)((edx >> 8) & 0xFFU); /* 0x41714E mov [ecx+3],dh */
    if ((ebx & 0xFFU) != (eax & 0xFFU)) /* 0x417151 cmp bl,al */ /* 0x417153 jnz short loc_4171CA */
        goto l_4171CA;
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(p); /* 0x417155 mov dl,[ecx] */
    if ((edx & 0xFFU) < 0xFEU) /* 0x417157 cmp dl,0FEh */ /* 0x41715A jb short loc_417177 */
        goto l_417177;
    edx = (edx & 0xFFFF00FFUL) | ((edx & 0xFFU) << 8); /* 0x41715C mov dh,dl */
    edx = (edx & 0xFFFF00FFUL) | ((((((edx >> 8) & 0xFFU) + 1) & 0xFFU) << 8) & 0xFF00U); /* 0x41715E inc dh */
    eax = (eax & 0xFFFFFF00UL) | (((edx >> 8) & 0xFFU)); /* 0x417160 mov al,dh */
    M8(p) = (unsigned char)((edx >> 8) & 0xFFU); /* 0x417162 mov [ecx],dh */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) >> 1) & 0xFFU)); /* 0x417164 shr al,1 */
    M8(p) = (unsigned char)(eax & 0xFFU); /* 0x417166 mov [ecx],al */
    eax = (eax & 0xFFFF00FFUL) | (((unsigned int)M8(p + 1) << 8) & 0xFF00U); /* 0x417168 mov ah,[ecx+1] */
    eax = (eax & 0xFFFF00FFUL) | ((((((eax >> 8) & 0xFFU) + 1) & 0xFFU) << 8) & 0xFF00U); /* 0x41716B inc ah */
    edx = (edx & 0xFFFFFF00UL) | (((eax >> 8) & 0xFFU)); /* 0x41716D mov dl,ah */
    M8(p + 1) = (unsigned char)((eax >> 8) & 0xFFU); /* 0x41716F mov [ecx+1],ah */
    edx = (edx & 0xFFFFFF00UL) | ((((edx & 0xFFU) >> 1) & 0xFFU)); /* 0x417172 shr dl,1 */
    M8(p + 1) = (unsigned char)(edx & 0xFFU); /* 0x417174 mov [ecx+1],dl */
    goto l_417177;
l_417177:
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(p) << 8) & 0xFF00U); /* 0x417177 mov dh,[ecx] */
    edx = (edx & 0xFFFF00FFUL) | ((((((edx >> 8) & 0xFFU) + 1) & 0xFFU) << 8) & 0xFF00U); /* 0x417179 inc dh */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8(p + 3); /* 0x41717B mov al,[ecx+3] */
    M8(p) = (unsigned char)((edx >> 8) & 0xFFU); /* 0x41717E mov [ecx],dh */
    if ((eax & 0xFFU) >= 0x7FU) /* 0x417180 cmp al,7Fh */ /* 0x417182 jnb short loc_41718B */
        goto l_41718B;
    eax = (eax & 0xFFFF00FFUL) | (((eax & 0xFFU) << 8) & 0xFF00U); /* 0x417184 mov ah,al */
    eax = (eax & 0xFFFF00FFUL) | ((((((eax >> 8) & 0xFFU) + 1) & 0xFFU) << 8) & 0xFF00U); /* 0x417186 inc ah */
l_417188:
    M8(p + 3) = (unsigned char)((eax >> 8) & 0xFFU); /* 0x417188 mov [ecx+3],ah */
l_41718B:
    bh = (bh + 1) & 0xFFU; /* 0x41718B inc bh */
    if (bh < 8) /* 0x41718D cmp bh,8 */ /* 0x417190 jb loc_4170A6 */
        goto l_4170A6;
    G32(0x45AF88) = ebp; /* 0x417196 mov ds:dword_45AF88,ebp */
    G32(0x45BA4C) = esi; /* 0x41719C mov ds:dword_45BA4C,esi */
    /* pop ebp/edi/esi/edx/ecx/ebx */ /* 0x4171A2 pop ebp */ /* 0x4171A3 pop edi */ /* 0x4171A4 pop esi */ /* 0x4171A5 pop edx */ /* 0x4171A6 pop ecx */ /* 0x4171A7 pop ebx */
    return; /* 0x4171A8 retn */
l_4171A9:
    eax = 0; /* 0x4171A9 xor eax,eax */
    eax = (unsigned int)M8(p + 1); /* 0x4171AB mov al,[ecx+1] */
    goto l_4170EE; /* 0x4171AE jmp loc_4170EE */
l_4171B3:
    M8(esi + 1) = (unsigned char)(M8(esi + 1) + 1); /* 0x4171B3 inc byte ptr [esi+1] */
    goto l_41711D; /* 0x4171B6 jmp loc_41711D */
l_4171BB:
    if ((unsigned int)M8(esi + 1) >= 0xFEU) /* 0x4171BB cmp byte ptr [esi+1],0FEh */ /* 0x4171BF jnb loc_41712A */
        goto l_41712A;
    goto l_417145; /* 0x4171C5 jmp loc_417145 */
l_4171CA:
    if ((unsigned int)M8(p + 1) < 0xFEU) /* 0x4171CA cmp byte ptr [ecx+1],0FEh */ /* 0x4171CE jb short loc_4171EB */
        goto l_4171EB;
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(p); /* 0x4171D0 mov dl,[ecx] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8(p + 1); /* 0x4171D2 mov al,[ecx+1] */
    edx = (edx & 0xFFFFFF00UL) | (((edx + 1) & 0xFFU)); /* 0x4171D5 inc dl */
    eax = (eax & 0xFFFFFF00UL) | (((eax + 1) & 0xFFU)); /* 0x4171D7 inc al */
    M8(p) = (unsigned char)(edx & 0xFFU); /* 0x4171D9 mov [ecx],dl */
    M8(p + 1) = (unsigned char)(eax & 0xFFU); /* 0x4171DB mov [ecx+1],al */
    edx = (edx & 0xFFFF00FFUL) | ((edx & 0xFFU) << 8); /* 0x4171DE mov dh,dl */
    eax = (eax & 0xFFFF00FFUL) | ((eax & 0xFFU) << 8); /* 0x4171E0 mov ah,al */
    edx = (edx & 0xFFFF00FFUL) | (((((edx >> 8) & 0xFFU) >> 1) & 0xFFU) << 8); /* 0x4171E2 shr dh,1 */
    eax = (eax & 0xFFFF00FFUL) | (((((eax >> 8) & 0xFFU) >> 1) & 0xFFU) << 8); /* 0x4171E4 shr ah,1 */
    M8(p) = (unsigned char)((edx >> 8) & 0xFFU); /* 0x4171E6 mov [ecx],dh */
    M8(p + 1) = (unsigned char)((eax >> 8) & 0xFFU); /* 0x4171E8 mov [ecx+1],ah */
l_4171EB:
    M8(p + 1) = (unsigned char)(M8(p + 1) + 1); /* 0x4171EB inc byte ptr [ecx+1] */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(p + 3) << 8) & 0xFF00U); /* 0x4171EE mov dh,[ecx+3] */
    if (((edx >> 8) & 0xFFU) >= 2) /* 0x4171F1 cmp dh,2 */ /* 0x4171F4 jnb short loc_417203 */
        goto l_417203;
    M8(p + 3) = 0; /* 0x4171F6 mov byte ptr [ecx+3],0 */
    M8(p + 2) = (unsigned char)(ebx & 0xFFU); /* 0x4171FA mov [ecx+2],bl */
    M8(p + 3) = (unsigned char)(M8(p + 3) + 0x80U); /* 0x4171FD add byte ptr [ecx+3],80h */
    goto l_41718B; /* 0x417201 jmp short loc_41718B */
l_417203:
    eax = (eax & 0xFFFFFF00UL) | (((edx >> 8) & 0xFFU)); /* 0x417203 mov al,dh */
    eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFU) >> 1) & 0xFFU); /* 0x417205 shr al,1 (al lane only) */
    M8(p + 3) = (unsigned char)(eax & 0xFFU); /* 0x417207 mov [ecx+3],al */
    M8(p + 3) = (unsigned char)(M8(p + 3) + 0x80U); /* 0x41720A add byte ptr [ecx+3],80h */
    goto l_41718B; /* 0x41720E jmp loc_41718B */
l_417213:
    M16(p) = 0x0202; /* 0x417213 mov word ptr [ecx],202h */
    M8(p + 2) = (unsigned char)(ebx & 0xFFU); /* 0x417218 mov [ecx+2],bl */
    goto l_417188; /* 0x41721B jmp loc_417188 (ah==0 here, so [ecx+3]=0) */
}

/* sub_417220 (UHARC04.lst:33828-34157).
 * Role: PPM per-literal escape/list walker over the 0x20-byte entry at
 * dword_433B44 + idx*32 (idx = dword_45AFAC, or idx&0x7FFF banks when
 * idx >= 0x40000): linear-search the symbol slot (byte_45C692 cursor,
 * 0xFF = unknown), price one context via word_437968, halve the
 * 14 count bytes on escape, insert/sort the new symbol, and account
 * dword_45AFD0/dword_45AFA8/dword_45BA24 (halved past 0x800).
 * Entry-live regs (first use): eax (only al saved at 0x41722F).
 * Frame: 8 bytes locals (var_1C byte=symbol, var_20 byte=bank score)
 * over pushes ebx/ecx/edx/esi/edi/ebp (esp+20h addressing).
 * Deviations: div-by-zero: direct `/` with comment (0x4172D5 site);
 * x86 div would also #DE on quotient overflow, model-kept in range.
 */
void d4v_417220(dec4v_ctx *c, unsigned int sym)
{
    unsigned int eax = 0;    /* eax/ax/ah/al scratch */
    unsigned int ebx = 0;    /* ebx/bx/bl/bh scratch, bank cursor */
    unsigned int ecx = 0;    /* ecx/cx/cl/ch scratch */
    unsigned int edx = 0;    /* edx/dx/dl/dh scratch */
    unsigned int esi = 0;    /* esi scratch */
    unsigned int edi = 0;    /* edi scratch */
    unsigned int ebp = 0;    /* ebp scratch */
    unsigned int var_1C = 0; /* saved entry al (symbol) */
    unsigned int var_20 = 0; /* bank-scan best score byte */

    /* push ebx/ecx/edx/esi/edi/ebp */ /* 0x417220 push ebx */ /* 0x417221 push ecx */ /* 0x417222 push edx */ /* 0x417223 push esi */ /* 0x417224 push edi */ /* 0x417225 push ebp */
    /* sub esp,8 */ /* 0x417226 sub esp,8 */
    ecx = (unsigned int)G8(0x45C692); /* 0x417229 mov cl,ds:byte_45C692 */
    var_1C = sym & 0xFFU; /* 0x41722F mov [esp+20h+var_1C],al */
    edx = G32(0x45AFAC); /* 0x417233 mov edx,ds:dword_45AFAC */
    if (edx >= 0x40000UL) /* 0x417239 cmp edx,40000h */ /* 0x41723F jnb loc_4174AC */
        goto l_4174AC;
    ebx = edx; /* 0x417245 mov ebx,edx */
    eax = G32(0x433B44); /* 0x417247 mov eax,ds:dword_433B44 */
    ebx = (ebx << 5) & U32MASK; /* 0x41724C shl ebx,5 */
    ebx = (ebx + eax) & U32MASK; /* 0x41724F add ebx,eax (ebx was missing base before) */
    if ((ecx & 0xFFU) != 0xFFU) /* 0x417251 cmp cl,0FFh */ /* 0x417254 jnz short loc_41726A */
        goto l_41726A;
    ecx &= 0xFFFFFF00UL; /* 0x417256 xor cl,cl */
l_417258:
    eax = 0; /* 0x417258 xor eax,eax */
    eax = ecx & 0xFFU; /* 0x41725A mov al,cl */
    eax = (unsigned int)M8(ebx + eax + 0x0E); /* 0x41725C mov al,[ebx+eax+0Eh] */
    if ((eax & 0xFFU) != (var_1C & 0xFFU)) /* 0x417260 cmp al,[esp+20h+var_1C] */ /* 0x417264 jnz loc_417400 */
        goto l_417400;
l_41726A:
    if (G8(0x45C68F) == 0) /* 0x41726A cmp ds:byte_45C68F,0 */ /* 0x417271 jz short loc_417297 */
        goto l_417297;
    if ((ecx & 0xFFU) >= 0x0DU) /* 0x417273 cmp cl,0Dh */ /* 0x417276 jnb short loc_417297 */
        goto l_417297;
    eax = 0; /* 0x417278 xor eax,eax */
    eax = ecx & 0xFFU; /* 0x41727A mov al,cl */
    eax = (unsigned int)M8(ebx + eax + 1); /* 0x41727C mov al,[ebx+eax+1] */
    eax &= 0xFFU; /* 0x417280 and eax,0FFh */
    edx = 0; /* 0x417285 xor edx,edx */
    eax = (unsigned int)d4v_sar(eax, 6); /* 0x417287 sar eax,6 */
    edx = (unsigned int)M8(ebx); /* 0x41728A mov dl,[ebx] */
    if (eax <= edx) /* 0x41728C cmp eax,edx (signed jle; both sides non-negative here) */ /* 0x41728E jle short loc_417297 */
        goto l_417297;
    G32(0x45BA24) = (G32(0x45BA24) + 4) & U32MASK; /* 0x417290 add ds:dword_45BA24,4 */
l_417297:
    if (G8(0x45C683) == 0) /* 0x417297 cmp ds:byte_45C683,0 */ /* 0x41729E jz short loc_4172F2 */
        goto l_4172F2;
    if ((ecx & 0xFFU) >= 0x0DU) /* 0x4172A0 cmp cl,0Dh */ /* 0x4172A3 jnb loc_417410 */
        goto l_417410;
    ecx &= 0xFFFF00FFUL; /* 0x4172A9 xor ch,ch */
    eax = 0; /* 0x4172AB xor eax,eax */
    G8(0x45C687) = (unsigned char)((ecx >> 8) & 0xFFU); /* 0x4172AD mov ds:byte_45C687,ch */
    eax = ecx & 0xFFU; /* 0x4172B3 mov al,cl */
    G8(0x45C685) = (unsigned char)((ecx >> 8) & 0xFFU); /* 0x4172B5 mov ds:byte_45C685,ch */
    G8(0x45C68D) = (unsigned char)((ecx >> 8) & 0xFFU); /* 0x4172BB mov ds:byte_45C68D,ch */
    eax = (unsigned int)M8(ebx + eax + 1); /* 0x4172C1 mov al,[ebx+eax+1] */
    eax &= 0xFFU; /* 0x4172C5 and eax,0FFh */
l_4172CA:
    edi = G32(0x45AFB8); /* 0x4172CA mov edi,ds:dword_45AFB8 */
    eax = (eax << 14) & U32MASK; /* 0x4172D0 shl eax,0Eh */
    edx = 0; /* 0x4172D3 xor edx,edx */
    /* 0x4172D5 div edi: direct `/` (div-by-zero if edi==0) */
    eax = eax / edi; /* 0x4172D5 div edi */
    eax = (unsigned int)M16(0x437968UL + (eax & U32MASK) * 2); /* 0x4172D7 mov ax,ds:word_437968[eax*2] */
    ebp = G32(0x45AF88); /* 0x4172DF mov ebp,ds:dword_45AF88 */
    eax &= 0xFFFFU; /* 0x4172E5 and eax,0FFFFh */
    ebp = (ebp + eax) & U32MASK; /* 0x4172EA add ebp,eax */
    G32(0x45AF88) = ebp; /* 0x4172EC mov ds:dword_45AF88,ebp */
l_4172F2:
    eax = 0; /* 0x4172F2 xor eax,eax */
    eax = (unsigned int)M8(ebx); /* 0x4172F4 mov al,[ebx] */
    eax = (unsigned int)M8(0x43409CUL + (eax & 0xFFU)); /* 0x4172F6 mov al,ds:byte_43409C[eax] */
    edx = G32(0x45AF9C); /* 0x4172FC mov edx,ds:dword_45AF9C */
    eax &= 0xFFU; /* 0x417302 and eax,0FFh */
    eax = (eax < edx) ? 1 : 0; /* 0x417307 cmp eax,edx */ /* 0x417309 setb al (eax = cmp result) */
    eax = (eax & 0xFFFF00FFUL) | ((eax & 0xFFU) << 8); /* 0x41730C mov ah,al */
    if ((ecx & 0xFFU) >= 0x0DU) /* 0x41730E cmp cl,0Dh */ /* 0x417311 jnb short loc_417342 */
        goto l_417342;
    edx = 0; /* 0x417313 xor edx,edx */
    edx = ecx & 0xFFU; /* 0x417315 mov dl,cl */
    esi = (unsigned int)M8(ebx + edx + 1); /* 0x417317 movzx esi,byte ptr [edx+ebx+1] */
    edx = 0; /* 0x41731C xor edx,edx */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8(ebx + 0x1B); /* 0x41731E mov al,[ebx+1Bh] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(ebx + 1); /* 0x417321 mov dl,[ebx+1] (dl lane only) */
    esi = (esi + 1) & U32MASK; /* 0x417324 inc esi */
    edx = (unsigned int)d4v_sar(edx, 1); /* 0x417325 sar edx,1 */
    eax = (eax & 0xFFFFFF00UL) | (eax & 0xC0U); /* 0x417327 and al,0C0h (al lane only) */
    if (esi >= edx) /* 0x417329 cmp esi,edx */ /* 0x41732B jge loc_417419 */
        goto l_417419;
    eax = (eax & 0xFFFFFF00UL) | ((eax + 0x40U) & 0xFFU); /* 0x417331 add al,40h (al lane only; may wrap without touching ah) */
    if ((eax & 0xFFU) != 0) /* 0x417333 jnz short loc_417337 */
        goto l_417337;
    eax = (eax & 0xFFFF00FFUL) | 0x0100U; /* 0x417335 mov ah,1 */
l_417337:
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(ebx + 0x1B); /* 0x417337 mov dl,[ebx+1Bh] (dl lane only) */
    edx = (edx & 0xFFFFFF00UL) | (edx & 0x3FU); /* 0x41733A and dl,3Fh (dl lane only) */
    edx |= (eax & 0xFFU); /* 0x41733D or dl,al */
    M8(ebx + 0x1B) = (unsigned char)(edx & 0xFFU); /* 0x41733F mov [ebx+1Bh],dl */
l_417342:
    if (((eax >> 8) & 0xFFU) == 0) /* 0x417342 test ah,ah */ /* 0x417344 jz short loc_41735D */
        goto l_41735D;
    eax = (ebx + 0x0D) & U32MASK; /* 0x417346 lea eax,[ebx+0Dh] */
    edx = (ebx - 1) & U32MASK; /* 0x417349 lea edx,[ebx-1] */
l_41734C:
    ecx = (ecx & 0xFFFF00FFUL) | ((unsigned int)M8(eax) << 8); /* 0x41734C mov ch,[eax] */
    eax = (eax - 1) & U32MASK; /* 0x41734E dec eax */
    ecx = (ecx & 0xFFFF00FFUL) | ((((((ecx >> 8) & 0xFFU) + 1) & 0xFFU)) << 8); /* 0x41734F inc ch */
    M8(eax + 1) = (unsigned char)((ecx >> 8) & 0xFFU); /* 0x417351 mov [eax+1],ch */
    ecx = (ecx & 0xFFFF00FFUL) | (((((ecx >> 8) & 0xFFU) >> 1) & 0xFFU) << 8); /* 0x417354 shr ch,1 */
    M8(eax + 1) = (unsigned char)((ecx >> 8) & 0xFFU); /* 0x417356 mov [eax+1],ch */
    if (eax != edx) /* 0x417359 cmp eax,edx */ /* 0x41735B jnz short loc_41734C */
        goto l_41734C;
l_41735D:
    if ((ecx & 0xFFU) < 0x0DU) /* 0x41735D cmp cl,0Dh */ /* 0x417360 jb loc_41742C */
        goto l_41742C;
    eax = (eax & 0xFFFFFF00UL) | ((G8(0x45C693) != 0) ? 1 : 0); /* 0x417366 cmp ds:byte_45C693,0 */ /* 0x41736D setnz al (al lane only) */
    eax = (eax & 0xFFFF00FFUL) | 0x0200U; /* 0x417370 mov ah,2 */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(ebx); /* 0x417372 mov dl,[ebx] (dl lane only) */
    eax = (eax & 0xFFFF00FFUL) | ((((((eax >> 8) & 0xFFU) - (eax & 0xFFU)) & 0xFFU)) << 8); /* 0x417374 sub ah,al */
    edx = (edx & 0xFFFFFF00UL) | (((edx + ((eax >> 8) & 0xFFU)) & 0xFFU)); /* 0x417376 add dl,ah (dl lane only) */
    ecx = (ecx & 0xFFFFFF00UL) | 1U; /* 0x417378 mov cl,1 */
    M8(ebx) = (unsigned char)(edx & 0xFFU); /* 0x41737A mov [ebx],dl */
l_41737C:
    eax = 0; /* 0x41737C xor eax,eax */
    eax = ecx & 0xFFU; /* 0x41737E mov al,cl */
    if (M8(ebx + eax + 1) != 0) /* 0x417380 cmp byte ptr [ebx+eax+1],0 */ /* 0x417385 jz short loc_41738E */
        goto scan_next;
    goto l_41738E;
scan_next:
    ecx = (ecx & 0xFFFFFF00UL) | (((ecx + 1) & 0xFFU)); /* 0x417387 inc cl */
    if ((ecx & 0xFFU) != 0x0CU) /* 0x417389 cmp cl,0Ch */ /* 0x41738C jnz short loc_41737C */
        goto l_41737C;
l_41738E:
    eax = 0; /* 0x41738E xor eax,eax */
    eax = ecx & 0xFFU; /* 0x417390 mov al,cl */
    eax = (eax + ebx) & U32MASK; /* 0x417392 add eax,ebx */
    if (M8(eax + 1) >= 6) /* 0x417394 cmp byte ptr [eax+1],6 */ /* 0x417398 jnb short loc_4173B1 */
        goto l_4173B1;
    edx = (edx & 0xFFFFFF00UL) | (((G8(0x45C693) == 2) ? 1 : 0)); /* 0x41739A cmp ds:byte_45C693,2 */ /* 0x4173A1 setz dl (dl lane only) */
    edx = (edx & 0xFFFFFF00UL) | (((edx + 2) & 0xFFU)); /* 0x4173A4 add dl,2 (dl lane only) */
    M8(eax + 1) = (unsigned char)(edx & 0xFFU); /* 0x4173A7 mov [eax+1],dl */
    edx = (edx & 0xFFFFFF00UL) | (var_1C & 0xFFU); /* 0x4173AA mov dl,[esp+20h+var_1C] (dl lane only) */
    M8(eax + 0x0E) = (unsigned char)(edx & 0xFFU); /* 0x4173AE mov [eax+0Eh],dl */
l_4173B1:
    eax = (unsigned int)M8(ebx + 0x1B); /* 0x4173B1 mov al,[ebx+1Bh] */
    eax = (eax & 0xFFFFFF00UL) | (eax & 0x3FU); /* 0x4173B4 and al,3Fh (al lane only) */
    edx = eax; /* 0x4173B6 mov edx,eax */
    edx &= 0xFFU; /* 0x4173B8 and edx,0FFh */
    eax = (unsigned int)M8(ebx + 0x1B); /* 0x4173BE mov al,[ebx+1Bh] */
    edx = (unsigned int)d4v_sar(edx, 1); /* 0x4173C1 sar edx,1 */
    eax = (eax & 0xFFFFFF00UL) | (eax & 0xC0U); /* 0x4173C3 and al,0C0h (al lane only) */
    eax &= 0xFFU; /* 0x4173C5 and eax,0FFh */
    eax = (eax + edx) & U32MASK; /* 0x4173CA add eax,edx */
    M8(ebx + 0x1B) = (unsigned char)(eax & 0xFFU); /* 0x4173CC mov [ebx+1Bh],al */
l_4173CF:
    if ((ecx & 0xFFU) == 0) /* 0x4173CF test cl,cl */ /* 0x4173D1 jz loc_41744A */
        goto l_41744A;
    eax = 0; /* 0x4173D7 xor eax,eax */
    eax = ecx & 0xFFU; /* 0x4173D9 mov al,cl */
    eax = (eax + ebx) & U32MASK; /* 0x4173DB add eax,ebx */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(eax + 1); /* 0x4173DD mov dl,[eax+1] (dl lane only) */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(eax) << 8) & 0xFF00U); /* 0x4173E0 mov dh,[eax] */
    if ((edx & 0xFFU) < ((edx >> 8) & 0xFFU)) /* 0x4173E2 cmp dl,dh */ /* 0x4173E4 jb short loc_41744A */
        goto l_41744A;
    edx = (edx & 0xFFFFFF00UL) | (((edx >> 8) & 0xFFU)); /* 0x4173E6 mov dl,dh */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(eax + 1) << 8) & 0xFF00U); /* 0x4173E8 mov dh,[eax+1] (reload: [eax+1] may have changed since dl was read) */
    M8(eax) = (unsigned char)((edx >> 8) & 0xFFU); /* 0x4173EB mov [eax],dh */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(eax + 0x0E) << 8) & 0xFF00U); /* 0x4173ED mov dh,[eax+0Eh] */
    ecx = (ecx & 0xFFFF00FFUL) | (((unsigned int)M8(eax + 0x0D) << 8) & 0xFF00U); /* 0x4173F0 mov ch,[eax+0Dh] */
    M8(eax + 0x0D) = (unsigned char)((edx >> 8) & 0xFFU); /* 0x4173F3 mov [eax+0Dh],dh */
    M8(eax + 1) = (unsigned char)(edx & 0xFFU); /* 0x4173F6 mov [eax+1],dl */
    M8(eax + 0x0E) = (unsigned char)((ecx >> 8) & 0xFFU); /* 0x4173F9 mov [eax+0Eh],ch */
    ecx = (ecx & 0xFFFFFF00UL) | (((ecx - 1) & 0xFFU)); /* 0x4173FC dec cl */
    goto l_4173CF; /* 0x4173FE jmp short loc_4173CF */
l_417400:
    ecx = (ecx & 0xFFFFFF00UL) | (((ecx + 1) & 0xFFU)); /* 0x417400 inc cl */
    if ((ecx & 0xFFU) < 0x0DU) /* 0x417402 cmp cl,0Dh */ /* 0x417405 jb loc_417258 */
        goto l_417258;
    goto l_41726A; /* 0x41740B jmp loc_41726A */
l_417410:
    eax = 0; /* 0x417410 xor eax,eax */
    eax = (unsigned int)M8(ebx); /* 0x417412 mov al,[ebx] */
    goto l_4172CA; /* 0x417414 jmp loc_4172CA */
l_417419:
    edx = 0; /* 0x417419 xor edx,edx */
    edx = eax & 0xFFU; /* 0x41741B mov dl,al */
    edx = (unsigned int)d4v_sar(edx, 1); /* 0x41741D sar edx,1 */
    edx &= 0xC0U; /* 0x41741F and edx,0C0h */
    eax = (eax & 0xFFFFFF00UL) | (edx & 0xFFU); /* 0x417425 mov al,dl */
    goto l_417337; /* 0x417427 jmp loc_417337 */
l_41742C:
    eax = 0; /* 0x41742C xor eax,eax */
    eax = ecx & 0xFFU; /* 0x41742E mov al,cl */
    M8(ebx + eax + 1) = (unsigned char)(M8(ebx + eax + 1) + 2); /* 0x417430 add byte ptr [ebx+eax+1],2 */
    eax = (unsigned int)M8(ebx + 0x1B); /* 0x417435 mov al,[ebx+1Bh] */
    eax = (eax & 0xFFFFFF00UL) | (eax & 0x3FU); /* 0x417438 and al,3Fh (al lane only) */
    if ((eax & 0xFFU) >= 0x3FU) /* 0x41743A cmp al,3Fh */ /* 0x41743C jnb short loc_417441 */
        goto l_417441;
    M8(ebx + 0x1B) = (unsigned char)(M8(ebx + 0x1B) + 1); /* 0x41743E inc byte ptr [ebx+1Bh] */
l_417441:
    G32(0x45AFA8) = (G32(0x45AFA8) + 5) & U32MASK; /* 0x417441 add ds:dword_45AFA8,5 */
    goto l_4173CF; /* 0x417448 jmp short loc_4173CF */
l_41744A:
    edi = G32(0x45AFD0); /* 0x41744A mov edi,ds:dword_45AFD0 */
    edi = (edi + 2) & U32MASK; /* 0x417450 add edi,2 */
    G8(0x45C692) = (unsigned char)(ecx & 0xFFU); /* 0x417453 mov ds:byte_45C692,cl */
    G32(0x45AFD0) = edi; /* 0x417459 mov ds:dword_45AFD0,edi */
    if (edi > 0x800U) /* 0x41745F cmp edi,800h */ /* 0x417465 ja short loc_417477 */
        goto l_417477;
l_417467:
    ecx = (unsigned int)G8(0x45C692); /* 0x417467 mov cl,ds:byte_45C692 */
    /* add esp,8; pop ebp/edi/esi/edx/ecx/ebx */ /* 0x41746D add esp,8 */ /* 0x417470 pop ebp */ /* 0x417471 pop edi */ /* 0x417472 pop esi */ /* 0x417473 pop edx */ /* 0x417474 pop ecx */ /* 0x417475 pop ebx */
    return; /* 0x417476 retn */
l_417477:
    edx = G32(0x45AFA8); /* 0x417477 mov edx,ds:dword_45AFA8 */
    eax = edi; /* 0x41747D mov eax,edi */
    ebx = G32(0x45BA24); /* 0x41747F mov ebx,ds:dword_45BA24 */
    eax >>= 1; /* 0x417485 shr eax,1 */
    edx >>= 1; /* 0x417487 shr edx,1 */
    ebx >>= 1; /* 0x417489 shr ebx,1 */
    G32(0x45AFD0) = eax; /* 0x41748B mov ds:dword_45AFD0,eax */
    G32(0x45AFA8) = edx; /* 0x417490 mov ds:dword_45AFA8,edx */
    G32(0x45BA24) = ebx; /* 0x417496 mov ds:dword_45BA24,ebx */
    ecx = (unsigned int)G8(0x45C692); /* 0x41749C mov cl,ds:byte_45C692 */
    /* add esp,8; pop ebp/edi/esi/edx/ecx/ebx */ /* 0x4174A2 add esp,8 */ /* 0x4174A5 pop ebp */ /* 0x4174A6 pop edi */ /* 0x4174A7 pop esi */ /* 0x4174A8 pop edx */ /* 0x4174A9 pop ecx */ /* 0x4174AA pop ebx */
    return; /* 0x4174AB retn */
l_4174AC:
    ebx = edx & 0x7FFFU; /* 0x4174AC mov ebx,edx */ /* 0x4174AE and ebx,7FFFh */
    edi = ebx; /* 0x4174B4 mov edi,ebx */
    eax = G32(0x433B44); /* 0x4174B6 mov eax,ds:dword_433B44 */
    edi = (edi << 5) & U32MASK; /* 0x4174BB shl edi,5 */
    esi = 0; /* 0x4174BE xor esi,esi */
    edi = (edi + eax) & U32MASK; /* 0x4174C0 add edi,eax */
    G32(0x45AFAC) = ebx; /* 0x4174C2 mov ds:dword_45AFAC,ebx */
    ebx = 0; /* 0x4174C8 xor ebx,ebx */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(edi); /* 0x4174CA mov dl,[edi] (dl lane only) */
    eax = edi; /* 0x4174CC mov eax,edi */
    if ((edx & 0xFFU) == 0) /* 0x4174CE test dl,dl */ /* 0x4174D0 jz short loc_417514 */
        goto l_417514;
l_4174D2:
    edx = (unsigned int)M8(eax + 0x1B); /* 0x4174D2 mov dl,[eax+1Bh] */
    edx = (edx & 0xFFFFFF00UL) | (edx & 0x3FU); /* 0x4174D5 and dl,3Fh (dl lane only) */
    var_20 = edx & 0xFFU; /* 0x4174D8 mov [esp+20h+var_20],dl */
    edx = ebx; /* 0x4174DB mov edx,ebx */
    edx = (edx << 5) & U32MASK; /* 0x4174DD shl edx,5 */
    edx = (unsigned int)M8((((edx + edi) & U32MASK) + 0x1B) & U32MASK); /* 0x4174E0 mov dl,[edx+edi+1Bh] */
    ecx = (ecx & 0xFFFF00FFUL) | ((var_20 & 0xFFU) << 8); /* 0x4174E4 mov ch,[esp+20h+var_20] */
    edx = (edx & 0xFFFFFF00UL) | (edx & 0x3FU); /* 0x4174E7 and dl,3Fh (dl lane only) */
    if ((edx & 0xFFU) > ((ecx >> 8) & 0xFFU)) /* 0x4174EA cmp dl,ch */ /* 0x4174EC jbe short loc_4174F0 */
        goto take_bank;
    goto l_4174F0;
take_bank:
    ebx = esi; /* 0x4174EE mov ebx,esi */
l_4174F0:
    eax = (eax + 0x100000UL) & U32MASK; /* 0x4174F0 add eax,100000h */
    esi = (esi + 0x8000UL) & U32MASK; /* 0x4174F5 add esi,8000h */
    G8(0x45C692) = (unsigned char)(ecx & 0xFFU); /* 0x4174FB mov ds:byte_45C692,cl */
    if (esi >= 0x40000UL) /* 0x417501 cmp esi,40000h */ /* 0x417507 jnb short loc_41751C */
        goto l_41751C;
    ecx = (unsigned int)G8(0x45C692); /* 0x417509 mov cl,ds:byte_45C692 */
    if (M8(eax) != 0) /* 0x41750F cmp byte ptr [eax],0 */ /* 0x417512 jnz short loc_4174D2 */
        goto l_4174D2;
l_417514:
    ebx = esi; /* 0x417514 mov ebx,esi */
    G8(0x45C692) = (unsigned char)(ecx & 0xFFU); /* 0x417516 mov ds:byte_45C692,cl */
l_41751C:
    ebx = (ebx << 5) & U32MASK; /* 0x41751C shl ebx,5 */
    edx = 0; /* 0x41751F xor edx,edx */
    edi = (edi + ebx) & U32MASK; /* 0x417521 add edi,ebx */
    ebx = 0x20; /* 0x417523 mov ebx,20h */
    eax = edi; /* 0x417528 mov eax,edi */
    d4v_fill(c, ebx, eax, edx); /* 0x41752A call sub_40D570 */
    eax = (unsigned int)G8(0x45C693); /* 0x41752F mov al,ds:byte_45C693 */
    M8(edi) = 1; /* 0x417534 mov byte ptr [edi],1 */
    eax = (eax & 0xFFFFFF00UL) | (((eax + 1) & 0xFFU)); /* 0x417537 inc al (al lane only) */
    M8(edi + 1) = (unsigned char)(eax & 0xFFU); /* 0x417539 mov [edi+1],al */
    eax = var_1C & 0xFFU; /* 0x41753C mov al,[esp+20h+var_1C] */
    M8(edi + 0x0E) = (unsigned char)(eax & 0xFFU); /* 0x417540 mov [edi+0Eh],al */
    eax = G32(0x45AFC4); /* 0x417543 mov eax,ds:dword_45AFC4 */
    M32(edi + 0x1C) = eax; /* 0x417548 mov [edi+1Ch],eax */
    if (esi >= 0x38000UL) /* 0x41754B cmp esi,38000h */ /* 0x417551 jnb loc_417467 (shares the plain-return tail) */
        goto l_417467;
    M8((edi + 0x100000UL) & U32MASK) = 0; /* 0x417557 mov byte ptr [edi+100000h],0 */
    ecx = (unsigned int)G8(0x45C692); /* 0x41755E mov cl,ds:byte_45C692 */
    /* add esp,8; pop ebp/edi/esi/edx/ecx/ebx */ /* 0x417564 add esp,8 */ /* 0x417567 pop ebp */ /* 0x417568 pop edi */ /* 0x417569 pop esi */ /* 0x41756A pop edx */ /* 0x41756B pop ecx */ /* 0x41756C pop ebx */
    return; /* 0x41756D retn */
}

/* sub_417570 (UHARC04.lst:34163-34377).
 * Role: PPM suffix/context builder over the 0x14-byte entry at
 * dword_433B48 + dword_45AFBC*20 (5 dwords + count/symbol lanes):
 * linear-search the symbol slot (byte_45C694 cursor, 0xFF = unknown),
 * price one context via word_437968, halve the 9 count bytes on
 * escape, insert/sort the new symbol, and account
 * dword_45AFD0/dword_45AFA8/dword_45BA24 (halved past 0x800).
 * Entry-live regs (first use): eax (only al saved to ch at 0x41757C).
 * Frame: no locals; pushes ebx/ecx/edx/esi/edi/ebp only.
 * Deviations: div-by-zero: direct `/` with comment (0x4175EE site);
 * x86 div would also #DE on quotient overflow, model-kept in range.
 */
void d4v_417570(dec4v_ctx *c, unsigned int sym)
{
    unsigned int eax = 0;    /* eax/ax/ah/al scratch */
    unsigned int ebx = 0;    /* ebx/bx/bl/bh scratch */
    unsigned int ecx = 0;    /* ecx/cx/cl/ch scratch */
    unsigned int edx = 0;    /* edx/dx/dl/dh scratch */
    unsigned int esi = 0;    /* (reused as entry pointer p; see below) */
    unsigned int edi = 0;    /* edi scratch */
    unsigned int ebp = 0;    /* ebp scratch */

    /* push ebx/ecx/edx/esi/edi/ebp */ /* 0x417570 push ebx */ /* 0x417571 push ecx */ /* 0x417572 push edx */ /* 0x417573 push esi */ /* 0x417574 push edi */ /* 0x417575 push ebp */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C694); /* 0x417576 mov cl,ds:byte_45C694 */
    ecx = (ecx & 0xFFFF00FFUL) | ((sym & 0xFFU) << 8); /* 0x41757C mov ch,al */
    ebx = G32(0x45AFBC); /* 0x41757E mov ebx,ds:dword_45AFBC */
    eax = (ebx << 2) & U32MASK; /* 0x417584 lea eax,ds:0[ebx*4] */
    eax = (eax + ebx) & U32MASK; /* 0x41758B add eax,ebx */
    esi = G32(0x433B48); /* 0x41758D mov esi,ds:dword_433B48 */
    eax = (eax << 2) & U32MASK; /* 0x417593 shl eax,2 */
    esi = (esi + eax) & U32MASK; /* 0x417596 add esi,eax (p eliminated: track live esi) */
    if (M8(esi) == 0) /* 0x417598 cmp byte ptr [esi],0 */ /* 0x41759B jz loc_417777 */
        goto l_417777;
    if ((ecx & 0xFFU) != 0xFFU) /* 0x4175A1 cmp cl,0FFh */ /* 0x4175A4 jnz short loc_4175B6 */
        goto l_4175B6;
    ecx &= 0xFFFFFF00UL; /* 0x4175A6 xor cl,cl */
l_4175A8:
    eax = 0; /* 0x4175A8 xor eax,eax */
    eax = ecx & 0xFFU; /* 0x4175AA mov al,cl */
    if (((ecx >> 8) & 0xFFU) != (unsigned int)M8((esi + eax + 9) & U32MASK)) /* 0x4175AC cmp ch,[esi+eax+9] */ /* 0x4175B0 jnz loc_4176BB */
        goto l_4176BB;
l_4175B6:
    if (G8(0x45C687) == 0) /* 0x4175B6 cmp ds:byte_45C687,0 */ /* 0x4175BD jz short loc_41760B */
        goto l_41760B;
    if ((ecx & 0xFFU) >= 8) /* 0x4175BF cmp cl,8 */ /* 0x4175C2 jnb loc_4176CB */
        goto l_4176CB;
    eax = 0; /* 0x4175C8 xor eax,eax */
    ebx &= 0xFFFFFF00UL; /* 0x4175CA xor bl,bl (bl lane only) */
    eax = ecx & 0xFFU; /* 0x4175CC mov al,cl */
    G8(0x45C685) = (unsigned char)(ebx & 0xFFU); /* 0x4175CE mov ds:byte_45C685,bl */
    G8(0x45C68D) = (unsigned char)(ebx & 0xFFU); /* 0x4175D4 mov ds:byte_45C68D,bl */
    eax = (unsigned int)M8((esi + eax + 1) & U32MASK); /* 0x4175DA mov al,[esi+eax+1] */
    eax &= 0xFFU; /* 0x4175DE and eax,0FFh */
l_4175E3:
    ebx = G32(0x45AFC8); /* 0x4175E3 mov ebx,ds:dword_45AFC8 */
    eax = (eax << 14) & U32MASK; /* 0x4175E9 shl eax,0Eh */
    edx = 0; /* 0x4175EC xor edx,edx */
    /* 0x4175EE div ebx: direct `/` (div-by-zero if ebx==0) */
    eax = eax / ebx; /* 0x4175EE div ebx */
    eax = (unsigned int)M16(0x437968UL + (eax & U32MASK) * 2); /* 0x4175F0 mov ax,ds:word_437968[eax*2] */
    edi = G32(0x45AF88); /* 0x4175F8 mov edi,ds:dword_45AF88 */
    eax &= 0xFFFFU; /* 0x4175FE and eax,0FFFFh */
    edi = (edi + eax) & U32MASK; /* 0x417603 add edi,eax */
    G32(0x45AF88) = edi; /* 0x417605 mov ds:dword_45AF88,edi */
l_41760B:
    ebx = 0; /* 0x41760B xor ebx,ebx */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)M8(esi); /* 0x41760D mov bl,[esi] (bl lane only) */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8(esi + 0x12); /* 0x41760F mov al,[esi+12h] (al lane only) */
    if ((eax & 0xFFU) <= (unsigned int)M8(0x43419CUL + (ebx & 0xFFU))) /* 0x417612 cmp al,ds:byte_43419C[ebx] */ /* 0x417618 jbe short loc_41763F */
        goto l_41763F;
    eax = (esi + 8) & U32MASK; /* 0x41761A lea eax,[esi+8] */
    M8(esi + 0x12) = 0; /* 0x41761D mov byte ptr [esi+12h],0 */
    ebx = (esi - 1) & U32MASK; /* 0x417621 lea ebx,[esi-1] */
l_417624:
    edx = (unsigned int)M8(eax); /* 0x417624 mov dl,[eax] */
    edx = (edx & 0xFFFFFF00UL) | ((edx + 1) & 0xFFU); /* 0x417626 inc dl (dl lane only) */
    M8(eax) = (unsigned char)(edx & 0xFFU); /* 0x417628 mov [eax],dl */
    edx = (edx & 0xFFFF00FFUL) | ((((edx & 0xFFU) >> 1) & 0xFFU) << 8); /* 0x41762A mov dh,dl */ /* 0x41762C shr dh,1 */
    M8(eax) = (unsigned char)((edx >> 8) & 0xFFU); /* 0x41762E mov [eax],dh */
    edx = (edx & 0xFFFFFF00UL) | (((edx >> 8) & 0xFFU)); /* 0x417630 mov dl,dh */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(esi + 0x12) << 8) & 0xFF00U); /* 0x417632 mov dh,[esi+12h] */
    edx = (edx & 0xFFFF00FFUL) | ((((((edx >> 8) & 0xFFU) + (edx & 0xFFU)) & 0xFFU)) << 8); /* 0x417635 add dh,dl */
    eax = (eax - 1) & U32MASK; /* 0x417637 dec eax */
    M8(esi + 0x12) = (unsigned char)((edx >> 8) & 0xFFU); /* 0x417638 mov [esi+12h],dh */
    if (eax != ebx) /* 0x41763B cmp eax,ebx */ /* 0x41763D jnz short loc_417624 */
        goto l_417624;
l_41763F:
    M8(esi + 0x12) = (unsigned char)(M8(esi + 0x12) + 4); /* 0x41763F add byte ptr [esi+12h],4 */
    if ((ecx & 0xFFU) < 8) /* 0x417643 cmp cl,8 */ /* 0x417646 jb loc_4176D4 */
        goto l_4176D4;
    ecx = (ecx & 0xFFFFFF00UL) | 1U; /* 0x41764C mov cl,1 */
    ebp = G32(0x45AFCC); /* 0x41764E mov ebp,ds:dword_45AFCC */
    ebx = (ebx & 0xFFFF00FFUL) | (((unsigned int)M8(esi + 0x11) << 8) & 0xFF00U); /* 0x417654 mov bh,[esi+11h] (bh lane only; bl kept) */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8(esi); /* 0x417657 mov al,[esi] (al lane only) */
    ebp = (ebp + 1) & U32MASK; /* 0x417659 inc ebp */
    ebx = (ebx & 0xFFFF00FFUL) | ((((((ebx >> 8) & 0xFFU) + (ecx & 0xFFU)) & 0xFFU)) << 8); /* 0x41765A add bh,cl */
    eax = (eax & 0xFFFFFF00UL) | (((eax + 4) & 0xFFU)); /* 0x41765C add al,4 (al lane only) */
    M8(esi + 0x11) = (unsigned char)((ebx >> 8) & 0xFFU); /* 0x41765E mov [esi+11h],bh */
    G32(0x45AFCC) = ebp; /* 0x417661 mov ds:dword_45AFCC,ebp */
    M8(esi) = (unsigned char)(eax & 0xFFU); /* 0x417667 mov [esi],al */
l_417669:
    eax = 0; /* 0x417669 xor eax,eax */
    eax = ecx & 0xFFU; /* 0x41766B mov al,cl */
    if (M8((esi + eax + 1) & U32MASK) != 0) /* 0x41766D cmp byte ptr [esi+eax+1],0 */ /* 0x417672 jz short loc_41767B */
        goto scan_next2;
    goto l_41767B;
scan_next2:
    ecx = (ecx & 0xFFFFFF00UL) | (((ecx + 1) & 0xFFU)); /* 0x417674 inc cl */
    if ((ecx & 0xFFU) != 7) /* 0x417676 cmp cl,7 */ /* 0x417679 jnz short loc_417669 */
        goto l_417669;
l_41767B:
    eax = 0; /* 0x41767B xor eax,eax */
    eax = ecx & 0xFFU; /* 0x41767D mov al,cl */
    eax = (eax + esi) & U32MASK; /* 0x41767F add eax,esi */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)M8(eax + 1); /* 0x417681 mov bl,[eax+1] */
    if ((ebx & 0xFFU) >= 0x0CU) /* 0x417684 cmp bl,0Ch */ /* 0x417687 jb loc_417707 */
        goto l_41768D;
    goto l_417707;
l_41768D:
    if ((ecx & 0xFFU) == 0) /* 0x41768D test cl,cl */ /* 0x41768F jz loc_41771A */
        goto l_41771A;
    eax = 0; /* 0x417695 xor eax,eax */
    eax = ecx & 0xFFU; /* 0x417697 mov al,cl */
    eax = (eax + esi) & U32MASK; /* 0x417699 add eax,esi */
    edx = (unsigned int)M8(eax + 1); /* 0x41769B mov dl,[eax+1] */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)M8(eax); /* 0x41769E mov bl,[eax] */
    if ((edx & 0xFFU) < (ebx & 0xFFU)) /* 0x4176A0 cmp dl,bl */ /* 0x4176A2 jb short loc_41771A */
        goto l_41771A;
    edx = (edx & 0xFFFF00FFUL) | ((ebx & 0xFFU) << 8); /* 0x4176A4 mov dh,bl */
    M8(eax) = (unsigned char)(edx & 0xFFU); /* 0x4176A6 mov [eax],dl */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(eax + 9); /* 0x4176A8 mov dl,[eax+9] */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)M8(eax + 8); /* 0x4176AB mov bl,[eax+8] */
    M8(eax + 8) = (unsigned char)(edx & 0xFFU); /* 0x4176AE mov [eax+8],dl */
    M8(eax + 1) = (unsigned char)((edx >> 8) & 0xFFU); /* 0x4176B1 mov [eax+1],dh */
    ecx = (ecx & 0xFFFFFF00UL) | (((ecx - 1) & 0xFFU)); /* 0x4176B4 dec cl */
    M8(eax + 9) = (unsigned char)(ebx & 0xFFU); /* 0x4176B6 mov [eax+9],bl */
    goto l_41768D; /* 0x4176B9 jmp short loc_41768D */
l_4176BB:
    ecx = (ecx & 0xFFFFFF00UL) | (((ecx + 1) & 0xFFU)); /* 0x4176BB inc cl */
    if ((ecx & 0xFFU) < 8) /* 0x4176BD cmp cl,8 */ /* 0x4176C0 jb loc_4175A8 */
        goto l_4175A8;
    goto l_4175B6; /* 0x4176C6 jmp loc_4175B6 */
l_4176CB:
    eax = 0; /* 0x4176CB xor eax,eax */
    eax = (unsigned int)M8(esi); /* 0x4176CD mov al,[esi] */
    goto l_4175E3; /* 0x4176CF jmp loc_4175E3 */
l_4176D4:
    ebx = G32(0x45AFCC); /* 0x4176D4 mov ebx,ds:dword_45AFCC */
    eax = (ebx << 2) & U32MASK; /* 0x4176DA lea eax,ds:0[ebx*4] */
    eax = (eax - ebx) & U32MASK; /* 0x4176E1 sub eax,ebx */
    eax >>= 2; /* 0x4176E3 shr eax,2 */
    G32(0x45AFCC) = eax; /* 0x4176E6 mov ds:dword_45AFCC,eax */
    eax = 0; /* 0x4176EB xor eax,eax */
    eax = (unsigned int)M8(esi + 0x11); /* 0x4176ED mov al,[esi+11h] */
    eax = (unsigned int)d4v_sar(eax, 1); /* 0x4176F0 sar eax,1 */
    M8(esi + 0x11) = (unsigned char)(eax & 0xFFU); /* 0x4176F2 mov [esi+11h],al */
    eax = 0; /* 0x4176F5 xor eax,eax */
    eax = ecx & 0xFFU; /* 0x4176F7 mov al,cl */
    M8((esi + eax + 1) & U32MASK) = (unsigned char)(M8((esi + eax + 1) & U32MASK) + 4); /* 0x4176F9 add byte ptr [esi+eax+1],4 */
    G32(0x45AFA8) = (G32(0x45AFA8) + 5) & U32MASK; /* 0x4176FE add ds:dword_45AFA8,5 */
    goto l_41768D; /* 0x417705 jmp short loc_41768D */
l_417707:
    edx = 4; /* 0x417707 mov dl,4 */
    edx = (edx & 0xFFFFFF00UL) | (((edx - (ebx & 0xFFU)) & 0xFFU)); /* 0x417709 sub dl,bl (dl lane only) */
    M8(esi + 0x12) = (unsigned char)(M8(esi + 0x12) + (edx & 0xFFU)); /* 0x41770B add [esi+12h],dl */
    M8(eax + 1) = 4; /* 0x41770E mov byte ptr [eax+1],4 */
    M8(eax + 9) = (unsigned char)((ecx >> 8) & 0xFFU); /* 0x417712 mov [eax+9],ch */
    goto l_41768D; /* 0x417715 jmp loc_41768D */
l_41771A:
    edx = G32(0x45AFD0); /* 0x41771A mov edx,ds:dword_45AFD0 */
    edx = (edx + 2) & U32MASK; /* 0x417720 add edx,2 */
    G8(0x45C694) = (unsigned char)(ecx & 0xFFU); /* 0x417723 mov ds:byte_45C694,cl */
    G32(0x45AFD0) = edx; /* 0x417729 mov ds:dword_45AFD0,edx */
    if (edx > 0x800U) /* 0x41772F cmp edx,800h */ /* 0x417735 ja short loc_417744 */
        goto l_417744;
tail_plain:
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C694); /* 0x417737 mov cl,ds:byte_45C694 */
    /* pop ebp/edi/esi/edx/ecx/ebx */ /* 0x41773D pop ebp */ /* 0x41773E pop edi */ /* 0x41773F pop esi */ /* 0x417740 pop edx */ /* 0x417741 pop ecx */ /* 0x417742 pop ebx */
    return; /* 0x417743 retn */
l_417744:
    edi = G32(0x45AFA8); /* 0x417744 mov edi,ds:dword_45AFA8 */
    esi = edx; /* 0x41774A mov esi,edx */
    ebp = G32(0x45BA24); /* 0x41774C mov ebp,ds:dword_45BA24 */
    esi >>= 1; /* 0x417752 shr esi,1 */
    edi >>= 1; /* 0x417754 shr edi,1 */
    ebp >>= 1; /* 0x417756 shr ebp,1 */
    G32(0x45AFD0) = esi; /* 0x417758 mov ds:dword_45AFD0,esi */
    G32(0x45AFA8) = edi; /* 0x41775E mov ds:dword_45AFA8,edi */
    G32(0x45BA24) = ebp; /* 0x417764 mov ds:dword_45BA24,ebp */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C694); /* 0x41776A mov cl,ds:byte_45C694 */
    /* pop ebp/edi/esi/edx/ecx/ebx */ /* 0x417770 pop ebp */ /* 0x417771 pop edi */ /* 0x417772 pop esi */ /* 0x417773 pop edx */ /* 0x417774 pop ecx */ /* 0x417775 pop ebx */
    return; /* 0x417776 retn */
l_417777:
    ebx = 0x14; /* 0x417777 mov ebx,14h */
    eax = esi; /* 0x41777C mov eax,esi */
    edx = 0; /* 0x41777E xor edx,edx */
    d4v_fill(c, ebx, eax, edx); /* 0x417780 call sub_40D570 */
    M16(esi) = 0x0303; /* 0x417785 mov word ptr [esi],303h */
    M8(esi + 0x12) = 6; /* 0x41778A mov byte ptr [esi+12h],6 */
    M8(esi + 9) = (unsigned char)((ecx >> 8) & 0xFFU); /* 0x41778E mov [esi+9],ch */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)G8(0x45C694); /* 0x417791 mov cl,ds:byte_45C694 */
    /* pop ebp/edi/esi/edx/ecx/ebx */ /* 0x417797 pop ebp */ /* 0x417798 pop edi */ /* 0x417799 pop esi */ /* 0x41779A pop edx */ /* 0x41779B pop ecx */ /* 0x41779C pop ebx */
    return; /* 0x41779D retn */
}

/* sub_41CF60 (UHARC04.lst:41992-42443).
 * Role: PPM rescale/escape worker: clamp the window (ebp =
 * dword_45AFB4 - dword_45BA28, capped at 0x3200), snapshot 20 bytes of
 * the model, score candidates 0..4 via sub_41D540 (or sub_41DBC0 for
 * windows over 0x100), pick the class in var_24, then range-decode
 * the stage-1 item and the 10-way stage-2 symbol like sub_418590
 * (via sub_4221F0 output bytes), refresh byte_45C67D + the
 * dword_4342C4/43429C/byte_434314 tables, and return
 * dword_45BA28 + ebp in eax.
 * Entry-live regs (first use): eax (only al saved at 0x41CF69).
 * Frame: 0x2C bytes locals (var_1C/var_24/var_28/var_20 bytes,
 * var_2C/var_30 dwords, var_44 = 5-dword score array) over pushes
 * ebx/ecx/edx/esi/edi/ebp (esp+44h addressing).
 * Deviations: div-by-zero: direct `/` with comment (0x41D091,
 * 0x41D09A, 0x41D19F, 0x41D268 sites); x86 div would also #DE on
 * quotient overflow, model-kept in range. The 0x41CFDB rep-movsb/
 * movsd sequence is folded to a 20-byte loop (see UNCERTAIN).
 */
int d4v_41cf60(dec4v_ctx *c, unsigned int a)
{
    unsigned int eax = 0;  /* eax/ax/ah/al scratch, div quotients */
    unsigned int ebx = 0;  /* ebx/bx/bl/bh scratch */
    unsigned int ecx = 0;  /* ecx/cx/ch/cl scratch */
    unsigned int edx = 0;  /* edx/dx/dl/dh scratch */
    unsigned int esi = 0;  /* esi scratch */
    unsigned int edi = 0;  /* edi scratch */
    unsigned int ebp = 0;  /* window size */
    unsigned int i = 0;    /* copy-loop index */
    unsigned int var_1C = 0; /* saved entry al */
    unsigned int var_24 = 0; /* winning class byte */
    unsigned int var_28 = 0; /* initial class byte */
    unsigned int var_20 = 0; /* min-loop cursor byte */
    unsigned int var_2C = 0; /* stream pointer dword */
    unsigned int var_30 = 0; /* symbol index dword */
    unsigned int t[5] = {0};   /* var_44 score array */

    /* push ebx/ecx/edx/esi/edi/ebp */ /* 0x41CF60 push ebx */ /* 0x41CF61 push ecx */ /* 0x41CF62 push edx */ /* 0x41CF63 push esi */ /* 0x41CF64 push edi */ /* 0x41CF65 push ebp */
    /* sub esp,2Ch */ /* 0x41CF66 sub esp,2Ch */
    var_1C = a & 0xFFU; /* 0x41CF69 mov [esp+44h+var_1C],al */
    eax = 0; /* 0x41CF6D xor eax,eax */
    eax = (unsigned int)G8(0x45C67D); /* 0x41CF6F mov al,ds:byte_45C67D */
    ebp = G32(0x45AFB4); /* 0x41CF74 mov ebp,ds:dword_45AFB4 */
    edx = G32(0x45BA28); /* 0x41CF7A mov edx,ds:dword_45BA28 */
    eax = (unsigned int)M8(0x43431EUL + (eax & 0xFFU)); /* 0x41CF80 mov al,ds:byte_43431E[eax] */
    ebp = (ebp - edx) & U32MASK; /* 0x41CF86 sub ebp,edx */
    var_28 = eax & 0xFFU; /* 0x41CF88 mov [esp+44h+var_28],al */
    var_24 = eax & 0xFFU; /* 0x41CF8C mov [esp+44h+var_24],al */
    if (ebp < 0x4B00UL) /* 0x41CF90 cmp ebp,4B00h */ /* 0x41CF96 jb short loc_41CF9D */
        goto l_41CF9D;
    ebp = 0x3200; /* 0x41CF98 mov ebp,3200h */
l_41CF9D:
    if (G8(0x436152) == 0) /* 0x41CF9D cmp ds:byte_436152,0 */ /* 0x41CFA4 jz loc_41D140 */
        goto l_41D140;
    if (G8(0x45C67F) == 0) /* 0x41CFAA cmp ds:byte_45C67F,0 */ /* 0x41CFB1 jz loc_41D140 */
        goto l_41D140;
    eax = G32(0x45BA28); /* 0x41CFB7 mov eax,ds:dword_45BA28 */
    if (eax >= G32(0x45BA44)) /* 0x41CFBC cmp eax,ds:dword_45BA44 */ /* 0x41CFC2 jnb loc_41D140 */
        goto l_41D140;
    if (ebp <= 0x100U) /* 0x41CFC8 cmp ebp,100h */ /* 0x41CFCE jbe short loc_41CFDB */
        goto l_41CFDB;
    eax = (ebp - 0x100U) & U32MASK; /* 0x41CFD0 lea eax,[ebp-100h] */
    d4v_421fa0(c, eax); /* 0x41CFD6 call sub_421FA0 */
l_41CFDB:
    ecx = 0x14; /* 0x41CFDB mov ecx,14h */
    edi = 0x45AF00UL; /* 0x41CFE0 mov edi,offset dword_45AF00 */
    eax = G32(0x436ED0); /* 0x41CFE5 mov eax,ds:dword_436ED0 */
    edx = G32(0x45CD18); /* 0x41CFEA mov edx,ds:dword_45CD18 */
    esi = 0x45AF14UL; /* 0x41CFF0 mov esi,offset dword_45AF14 */
    edx = (edx + eax) & U32MASK; /* 0x41CFF5 add edx,eax */
    /* 0x41CFF7 cld (no C effect) */
    /* 0x41CFF8 mov eax,ecx */ /* 0x41CFFA sub ecx,edi */ /* 0x41CFFC sub ecx,eax */ /* 0x41CFFE and ecx,3 */ /* 0x41D001 sub eax,ecx */ /* 0x41D003 jle short loc_41D011 */ /* 0x41D005 rep movsb */ /* 0x41D007 mov ecx,eax */ /* 0x41D009 and eax,3 */ /* 0x41D00C shr ecx,2 */ /* 0x41D00F rep movsd */ /* 0x41D011 add ecx,eax */ /* 0x41D013 rep movsb */
    /* UNCERTAIN(0041CFDB): alignment prologue folded; net effect is a
     * straight 20-byte copy [0x45AF14]->[0x45AF00]. */
    for (i = 0; i < 0x14; i++)
        M8((0x45AF00UL + i) & U32MASK) = M8((0x45AF14UL + i) & U32MASK);
    var_2C = edx; /* 0x41D015 mov [esp+44h+var_2C],edx */
    if (ebp > 0x100U) /* 0x41D019 cmp ebp,100h */ /* 0x41D01F ja loc_41D370 */
        goto l_41D370;
l_41D025:
    esi = var_24 & 0xFFU; /* 0x41D025 movzx esi,[esp+44h+var_24] */
    ebx = 0xFFFFFFFFUL; /* 0x41D02A mov ebx,0FFFFFFFFh */
    eax = var_2C; /* 0x41D02F mov eax,[esp+44h+var_2C] */
    edx = ebp; /* 0x41D033 mov edx,ebp */
    ecx = esi; /* 0x41D035 mov ecx,esi */
    eax = d4v_41d540(c, eax, edx, ecx, ebx); /* 0x41D037 call sub_41D540 */
    edx &= 0xFFFF00FFUL; /* 0x41D03C xor dh,dh */
    t[esi & 0xFFU] = eax; /* 0x41D03E mov [esp+esi*4+44h+var_44],eax */
    var_20 = 0; /* 0x41D041 mov [esp+44h+var_20],dh */
l_41D045:
    eax = var_20 & 0xFFU; /* 0x41D045 mov al,[esp+44h+var_20] */
    if ((eax & 0xFFU) == (var_28 & 0xFFU)) /* 0x41D049 cmp al,[esp+44h+var_28] */ /* 0x41D04D jz short loc_41D077 */
        goto l_41D077;
    edi = var_24 & 0xFFU; /* 0x41D04F movzx edi,[esp+44h+var_24] */
    esi = eax & 0xFFU; /* 0x41D054 movzx esi,al */
    edx = ebp; /* 0x41D057 mov edx,ebp */
    eax = var_2C; /* 0x41D059 mov eax,[esp+44h+var_2C] */
    ecx = esi; /* 0x41D05D mov ecx,esi */
    ebx = t[edi & 0xFFU]; /* 0x41D05F mov ebx,[esp+edi*4+44h+var_44] */
    eax = d4v_41d540(c, eax, edx, ecx, ebx); /* 0x41D062 call sub_41D540 */
    t[esi & 0xFFU] = eax; /* 0x41D067 mov [esp+esi*4+44h+var_44],eax */
    if (eax >= t[edi & 0xFFU]) /* 0x41D06A cmp eax,[esp+edi*4+44h+var_44] */ /* 0x41D06D jnb short loc_41D077 */
        goto l_41D077;
    eax = var_20 & 0xFFU; /* 0x41D06F mov al,[esp+44h+var_20] */
    var_24 = eax & 0xFFU; /* 0x41D073 mov [esp+44h+var_24],al */
l_41D077:
    ebx = (ebx & 0xFFFF00FFUL) | ((var_20 & 0xFFU) << 8); /* 0x41D077 mov bh,[esp+44h+var_20] (bh lane; rest of ebx preserved) */
    ebx = (ebx & 0xFFFF00FFUL) | ((((ebx & 0xFFU) + 1) & 0xFFU) << 8); /* 0x41D07B inc bh */
    var_20 = (ebx >> 8) & 0xFFU; /* 0x41D07D mov [esp+44h+var_20],bh */
    if (((ebx >> 8) & 0xFFU) <= 4) /* 0x41D081 cmp bh,4 */ /* 0x41D084 jbe short loc_41D045 */
        goto l_41D045;
    eax = 0; /* 0x41D086 xor eax,eax */
    eax = var_24 & 0xFFU; /* 0x41D088 mov al,[esp+44h+var_24] */
    edx = 0; /* 0x41D08C xor edx,edx */
    eax = t[eax & 0xFFU]; /* 0x41D08E mov eax,[esp+eax*4+44h+var_44] */
    /* 0x41D091 div ebp: direct `/` (div-by-zero if ebp==0) */
    eax = eax / ebp; /* 0x41D091 div ebp */
    ecx = 0x249; /* 0x41D093 mov ecx,249h */
    edx = 0; /* 0x41D098 xor edx,edx */
    /* 0x41D09A div ecx: direct `/` (div-by-zero if ecx==0; here 0x249) */
    eax = eax / ecx; /* 0x41D09A div ecx */
    ebx = eax; /* 0x41D09C mov ebx,eax */
    if (eax >= 0x0AU) /* 0x41D09E cmp eax,0Ah */ /* 0x41D0A1 jnb loc_41D386 */
        goto l_41D386;
l_41D0A7:
    edx = 0x0A; /* 0x41D0A7 mov edx,0Ah */
    edx = (edx - eax) & U32MASK; /* 0x41D0AC sub edx,eax */
    ecx = (ecx & 0xFFFF00FFUL) | ((var_24 & 0xFFU) << 8); /* 0x41D0AE mov ch,[esp+44h+var_24] */
    esi = (edx * 4) & U32MASK; /* 0x41D0B2 lea esi,ds:0[edx*4] */
    if (((ecx >> 8) & 0xFFU) != 1) /* 0x41D0B9 cmp ch,1 */ /* 0x41D0BC jnz loc_41D390 */
        goto l_41D390;
    eax = 0x50; /* 0x41D0C2 mov eax,50h */
l_41D0C7:
    G32(0x45AF30) = ebp; /* 0x41D0C7 mov ds:dword_45AF30,ebp */
    eax = (eax - esi) & U32MASK; /* 0x41D0CD sub eax,esi */
    edx &= 0xFFFFFF00UL; /* 0x41D0CF xor dl,dl (dl lane only; rest of edx preserved) */
    edi = 0; /* 0x41D0D1 xor edi,edi */
    esi = eax; /* 0x41D0D3 mov esi,eax */
    eax = (eax & 0xFFFFFF00UL) | (var_24 & 0xFFU); /* 0x41D0D5 mov al,[esp+44h+var_24] */
    G32(0x45AF28) = edi; /* 0x41D0D9 mov ds:dword_45AF28,edi */
    if ((eax & 0xFFU) == 0) /* 0x41D0DF test al,al */ /* 0x41D0E1 jz loc_41D39A */
        goto l_41D39A;
    if ((eax & 0xFFU) == 3) /* 0x41D0E7 cmp al,3 */ /* 0x41D0E9 jz loc_41D39A */
        goto l_41D39A;
    eax = 0; /* 0x41D0EF xor eax,eax */
    eax = var_24 & 0xFFU; /* 0x41D0F1 mov al,[esp+44h+var_24] */
    ecx = (eax * 4) & U32MASK; /* 0x41D0F5 lea ecx,ds:0[eax*4] */
    esi = (esi * M32((0x45AF00UL + ecx) & U32MASK)) & U32MASK; /* 0x41D0FC imul esi,ds:dword_45AF00[ecx] */
    edi = M32(0x45AF00); /* 0x41D103 mov edi,ds:dword_45AF00 */
    edi = (edi << 6) & U32MASK; /* 0x41D109 shl edi,6 */
    if (esi <= edi) /* 0x41D10C cmp esi,edi */ /* 0x41D10E jbe loc_41D39A */
        goto l_41D39A;
    edx = ebp; /* 0x41D114 mov edx,ebp */
    ebx = t[(ecx >> 2) & 0xFFU]; /* 0x41D116 mov ebx,[esp+ecx+44h+var_44] */
    ecx = eax; /* 0x41D119 mov ecx,eax */
    eax = var_2C; /* 0x41D11B mov eax,[esp+44h+var_2C] */
l_41D11F:
    eax = d4v_41e0a0(c, eax, edx, ebx, ecx); /* 0x41D11F call sub_41E0A0 */
    edx = (edx & 0xFFFFFF00UL) | (eax & 0xFFU); /* 0x41D124 mov dl,al */
l_41D126:
    if ((edx & 0xFFU) == 0) /* 0x41D126 test dl,dl */ /* 0x41D128 jz loc_41D449 */
        goto l_41D449;
    var_24 = edx & 0xFFU; /* 0x41D12E mov [esp+44h+var_24],dl */
    ecx = 0; /* 0x41D132 xor ecx,ecx */
    G32(0x45AF30) = ebp; /* 0x41D134 mov ds:dword_45AF30,ebp */
    G32(0x45AF28) = ecx; /* 0x41D13A mov ds:dword_45AF28,ecx */
l_41D140:
    if ((var_1C & 0xFFU) != 0xFFU) /* 0x41D140 cmp [esp+44h+var_1C],0FFh */ /* 0x41D145 jnz loc_41D45A */
        goto l_41D45A;
l_41D14B:
    ebx = (ebx & 0xFFFFFF00UL) | (var_1C & 0xFFU); /* 0x41D14B mov bl,[esp+44h+var_1C] (bl lane only) */
    if ((ebx & 0xFFU) == 0xFFU) /* 0x41D14F cmp bl,0FFh */ /* 0x41D152 jz loc_41D22C */
        goto l_41D22C;
    eax = 0; /* 0x41D158 xor eax,eax */
    eax = ebx & 0xFFU; /* 0x41D15A mov al,bl */
    eax = (eax << 3) & U32MASK; /* 0x41D15C shl eax,3 */
    eax = (eax + 0x459BA8UL) & U32MASK; /* 0x41D15F add eax,offset unk_459BA8 */
    edx = 0; /* 0x41D164 xor edx,edx */
    edx = (edx & 0xFFFF0000UL) | (unsigned int)M16(eax + 6); /* 0x41D166 mov dx,[eax+6] (dx lane only) */
    G32(0x45AF68) = edx; /* 0x41D16A mov ds:dword_45AF68,edx */
    edx = 0; /* 0x41D170 xor edx,edx */
    edx = (edx & 0xFFFF0000UL) | (unsigned int)M16(eax); /* 0x41D172 mov dx,[eax] (dx lane only) */
    G32(0x45AF6C) = edx; /* 0x41D175 mov ds:dword_45AF6C,edx */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16(eax + 2); /* 0x41D17B mov ax,[eax+2] (ax lane only; high half kept, masked next) */
    eax &= 0xFFFFU; /* 0x41D17F and eax,0FFFFh */
    G32(0x45AF70) = eax; /* 0x41D184 mov ds:dword_45AF70,eax */
    eax = 0x45AF68UL; /* 0x41D189 mov eax,offset dword_45AF68 */
    ebx = M32(eax); /* 0x41D18E mov ebx,[eax] */
    ecx = M32(eax + 4); /* 0x41D190 mov ecx,[eax+4] */
    edi = M32(eax + 8); /* 0x41D193 mov edi,[eax+8] */
    eax = G32(0x45D0BC); /* 0x41D196 mov eax,ds:dword_45D0BC */
    edx = 0; /* 0x41D19B xor edx,edx */
    edi = (edi - ecx) & U32MASK; /* 0x41D19D sub edi,ecx */
    /* 0x41D19F div ebx: direct `/` (div-by-zero if ebx==0) */
    eax = eax / ebx; /* 0x41D19F div ebx */
    ebx = eax; /* 0x41D1A1 mov ebx,eax */
    edx = 0; /* 0x41D1A3 xor edx,edx */
    ecx = (ecx * eax) & U32MASK; /* 0x41D1A5 imul ecx,eax */
    ebx = (ebx * edi) & U32MASK; /* 0x41D1A8 imul ebx,edi */
    edi = G32(0x45D0C4); /* 0x41D1AB mov edi,ds:dword_45D0C4 */
    { /* 0x41D1B1 add ecx,ds:dword_45D0B4 + 0x41D1B7 adc edi,edx: carry out of the add feeds edi */
        unsigned int addend = G32(0x45D0B4);
        unsigned int old = ecx;
        ecx = (ecx + addend) & U32MASK;
        edi = (edi + edx + ((ecx < old) ? 1U : 0U)) & U32MASK;
    }
    esi = G32(0x45D0B0); /* 0x41D1B9 mov esi,ds:dword_45D0B0 */
    if (ebx >= 0x1000000UL) /* 0x41D1BF cmp ebx,1000000h */ /* 0x41D1C5 jnb short loc_41D214 */
        goto l_41D214;
l_41D1C7:
    if (edi != 0) /* 0x41D1C7 test edi,edi */ /* 0x41D1C9 jnz short loc_41D1D3 */
        goto l_41D1D3;
    if (ecx >= 0xFF000000UL) /* 0x41D1CB cmp ecx,0FF000000h */ /* 0x41D1D1 jnb short loc_41D203 */
        goto l_41D203;
l_41D1D3:
    eax = G32(0x45D0C0); /* 0x41D1D3 mov eax,ds:dword_45D0C0 */
    edx = 0xFF; /* 0x41D1D8 mov edx,0FFh */
    eax = (eax + edi) & U32MASK; /* 0x41D1DD add eax,edi */
    eax &= edx; /* 0x41D1DF and eax,edx */
    d4v_put_byte(c, eax); /* 0x41D1E1 call sub_4221F0 */
    if (esi == 0) /* 0x41D1E6 test esi,esi */ /* 0x41D1E8 jz short loc_41D1F8 */
        goto l_41D1F8;
    eax = edi; /* 0x41D1EA mov eax,edi */
    eax = (eax + edx) & U32MASK; /* 0x41D1EC add eax,edx */
    eax &= edx; /* 0x41D1EE and eax,edx */
l_41D1F0:
    d4v_put_byte(c, eax); /* 0x41D1F0 call sub_4221F0 */
    esi = (esi - 1) & U32MASK; /* 0x41D1F5 dec esi */
    if (esi != 0) /* 0x41D1F6 jnz short loc_41D1F0 */
        goto l_41D1F0;
l_41D1F8:
    eax = ecx; /* 0x41D1F8 mov eax,ecx */
    esi = (esi - 1) & U32MASK; /* 0x41D1FA dec esi */
    eax >>= 0x18; /* 0x41D1FB shr eax,18h */
    G32(0x45D0C0) = eax; /* 0x41D1FE mov ds:dword_45D0C0,eax */
l_41D203:
    ebx = (ebx << 8) & U32MASK; /* 0x41D203 shl ebx,8 */
    esi = (esi + 1) & U32MASK; /* 0x41D206 inc esi */
    ecx = (ecx << 8) & U32MASK; /* 0x41D207 shl ecx,8 */
    edi = 0; /* 0x41D20A xor edi,edi */
    if (ebx < 0x1000000UL) /* 0x41D20C cmp ebx,1000000h */ /* 0x41D212 jb short loc_41D1C7 */
        goto l_41D1C7;
l_41D214:
    G32(0x45D0BC) = ebx; /* 0x41D214 mov ds:dword_45D0BC,ebx */
    G32(0x45D0B4) = ecx; /* 0x41D21A mov ds:dword_45D0B4,ecx */
    G32(0x45D0C4) = edi; /* 0x41D220 mov ds:dword_45D0C4,edi */
    G32(0x45D0B0) = esi; /* 0x41D226 mov ds:dword_45D0B0,esi */
l_41D22C:
    eax = 0; /* 0x41D22C xor eax,eax */
    eax = var_24 & 0xFFU; /* 0x41D22E mov al,[esp+44h+var_24] */
    G32(0x45AF6C) = eax; /* 0x41D232 mov ds:dword_45AF6C,eax */
    eax = 0; /* 0x41D237 xor eax,eax */
    eax = var_24 & 0xFFU; /* 0x41D239 mov al,[esp+44h+var_24] */
    var_30 = eax; /* 0x41D23D mov [esp+44h+var_30],eax */
    eax = (eax + 1) & U32MASK; /* 0x41D241 inc eax (full-reg; eax holds the var_24 byte) */
    ebx = 0x0A; /* 0x41D242 mov ebx,0Ah */
    G32(0x45AF70) = eax; /* 0x41D247 mov ds:dword_45AF70,eax */
    eax = 0x45AF68UL; /* 0x41D24C mov eax,offset dword_45AF68 */
    G32(0x45AF68) = ebx; /* 0x41D251 mov ds:dword_45AF68,ebx */
    ebx = M32(eax); /* 0x41D257 mov ebx,[eax] */
    ecx = M32(eax + 4); /* 0x41D259 mov ecx,[eax+4] */
    edi = M32(eax + 8); /* 0x41D25C mov edi,[eax+8] */
    eax = G32(0x45D0BC); /* 0x41D25F mov eax,ds:dword_45D0BC */
    edx = 0; /* 0x41D264 xor edx,edx */
    edi = (edi - ecx) & U32MASK; /* 0x41D266 sub edi,ecx */
    /* 0x41D268 div ebx: direct `/` (div-by-zero if ebx==0; here ebx=10) */
    eax = eax / ebx; /* 0x41D268 div ebx */
    ebx = eax; /* 0x41D26A mov ebx,eax */
    edx = 0; /* 0x41D26C xor edx,edx */
    ecx = (ecx * eax) & U32MASK; /* 0x41D26E imul ecx,eax */
    ebx = (ebx * edi) & U32MASK; /* 0x41D271 imul ebx,edi */
    edi = G32(0x45D0C4); /* 0x41D274 mov edi,ds:dword_45D0C4 */
    { /* 0x41D27A add ecx,ds:dword_45D0B4 + 0x41D280 adc edi,edx: carry out of the add feeds edi */
        unsigned int addend = G32(0x45D0B4);
        unsigned int old = ecx;
        ecx = (ecx + addend) & U32MASK;
        edi = (edi + edx + ((ecx < old) ? 1U : 0U)) & U32MASK;
    }
    esi = G32(0x45D0B0); /* 0x41D282 mov esi,ds:dword_45D0B0 */
    if (ebx >= 0x1000000UL) /* 0x41D288 cmp ebx,1000000h */ /* 0x41D28E jnb short loc_41D2DD */
        goto l_41D2DD;
l_41D290:
    if (edi != 0) /* 0x41D290 test edi,edi */ /* 0x41D292 jnz short loc_41D29C */
        goto l_41D29C;
    if (ecx >= 0xFF000000UL) /* 0x41D294 cmp ecx,0FF000000h */ /* 0x41D29A jnb short loc_41D2CC */
        goto l_41D2CC;
l_41D29C:
    eax = G32(0x45D0C0); /* 0x41D29C mov eax,ds:dword_45D0C0 */
    edx = 0xFF; /* 0x41D2A1 mov edx,0FFh */
    eax = (eax + edi) & U32MASK; /* 0x41D2A6 add eax,edi */
    eax &= edx; /* 0x41D2A8 and eax,edx */
    d4v_put_byte(c, eax); /* 0x41D2AA call sub_4221F0 */
    if (esi == 0) /* 0x41D2AF test esi,esi */ /* 0x41D2B1 jz short loc_41D2C1 */
        goto l_41D2C1;
    eax = edi; /* 0x41D2B3 mov eax,edi */
    eax = (eax + edx) & U32MASK; /* 0x41D2B5 add eax,edx */
    eax &= edx; /* 0x41D2B7 and eax,edx */
l_41D2B9:
    d4v_put_byte(c, eax); /* 0x41D2B9 call sub_4221F0 */
    esi = (esi - 1) & U32MASK; /* 0x41D2BE dec esi */
    if (esi != 0) /* 0x41D2BF jnz short loc_41D2B9 */
        goto l_41D2B9;
l_41D2C1:
    eax = ecx; /* 0x41D2C1 mov eax,ecx */
    esi = (esi - 1) & U32MASK; /* 0x41D2C3 dec esi */
    eax >>= 0x18; /* 0x41D2C4 shr eax,18h */
    G32(0x45D0C0) = eax; /* 0x41D2C7 mov ds:dword_45D0C0,eax */
l_41D2CC:
    ebx = (ebx << 8) & U32MASK; /* 0x41D2CC shl ebx,8 */
    esi = (esi + 1) & U32MASK; /* 0x41D2CF inc esi */
    ecx = (ecx << 8) & U32MASK; /* 0x41D2D0 shl ecx,8 */
    edi = 0; /* 0x41D2D3 xor edi,edi */
    if (ebx < 0x1000000UL) /* 0x41D2D5 cmp ebx,1000000h */ /* 0x41D2DB jb short loc_41D290 */
        goto l_41D290;
l_41D2DD:
    G32(0x45D0BC) = ebx; /* 0x41D2DD mov ds:dword_45D0BC,ebx */
    G32(0x45D0B4) = ecx; /* 0x41D2E3 mov ds:dword_45D0B4,ecx */
    G32(0x45D0C4) = edi; /* 0x41D2E9 mov ds:dword_45D0C4,edi */
    G32(0x45D0B0) = esi; /* 0x41D2EF mov ds:dword_45D0B0,esi */
    eax = var_24 & 0xFFU; /* 0x41D2F5 mov al,[esp+44h+var_24] */
    if ((eax & 0xFFU) == (unsigned int)G8(0x45C67D)) /* 0x41D2F9 cmp al,ds:byte_45C67D */ /* 0x41D2FF jz short loc_41D35A */
        goto l_41D35A;
    G8(0x45C67D) = (unsigned char)(eax & 0xFFU); /* 0x41D301 mov ds:byte_45C67D,al */
    eax = var_30; /* 0x41D306 mov eax,[esp+44h+var_30] */
    edx = M32((0x43429CUL + eax * 4) & U32MASK); /* 0x41D30A mov edx,ds:dword_43429C[eax*4] */
    eax = M32((0x4342C4UL + eax * 4) & U32MASK); /* 0x41D311 mov eax,ds:dword_4342C4[eax*4] */
    G32(0x45AF3C) = eax; /* 0x41D318 mov ds:dword_45AF3C,eax */
    eax = var_30; /* 0x41D31D mov eax,[esp+44h+var_30] */
    ecx = (ecx & 0xFFFFFF00UL) | (var_24 & 0xFFU); /* 0x41D321 mov cl,[esp+44h+var_24] */
    eax = (unsigned int)M8((0x434314UL + eax) & U32MASK); /* 0x41D325 mov al,ds:byte_434314[eax] */
    G32(0x45AF40) = edx; /* 0x41D32B mov ds:dword_45AF40,edx */
    G8(0x45C67A) = (unsigned char)(eax & 0xFFU); /* 0x41D331 mov ds:byte_45C67A,al */
    if ((ecx & 0xFFU) != 5) /* 0x41D336 cmp cl,5 */ /* 0x41D339 jnz loc_41D46F */
        goto l_41D46F;
l_41D33F:
    eax = (eax & 0xFFFFFF00UL) | 1U; /* 0x41D33F mov al,1 (al lane only) */
l_41D341:
    G8(0x45C688) = (unsigned char)(eax & 0xFFU); /* 0x41D341 mov ds:byte_45C688,al */
    esi = 0; /* 0x41D346 xor esi,esi */
    eax = 0; /* 0x41D348 xor al,al (al lane) */
    eax &= 0xFFFFFF00UL; /* (al = 0) */
    G32(0x45AF48) = esi; /* 0x41D34A mov ds:dword_45AF48,esi */
    G8(0x45C67B) = 0; /* 0x41D350 mov ds:byte_45C67B,al */
    G8(0x45C679) = 0; /* 0x41D355 mov ds:byte_45C679,al */
l_41D35A:
    eax = G32(0x45BA28); /* 0x41D35A mov eax,ds:dword_45BA28 */
    eax = (eax + ebp) & U32MASK; /* 0x41D35F add eax,ebp */
    G32(0x45AF4C) = eax; /* 0x41D361 mov ds:dword_45AF4C,eax */
    /* add esp,2Ch; pop ebp/edi/esi/edx/ecx/ebx */ /* 0x41D366 add esp,2Ch */ /* 0x41D369 pop ebp */ /* 0x41D36A pop edi */ /* 0x41D36B pop esi */ /* 0x41D36C pop edx */ /* 0x41D36D pop ecx */ /* 0x41D36E pop ebx */
    return (int)eax; /* 0x41D36F retn */
l_41D370:
    eax = var_2C; /* 0x41D370 mov eax,[esp+44h+var_2C] */
    edx = (ebp - 0x100U) & U32MASK; /* 0x41D374 lea edx,[ebp-100h] */
    eax = d4v_41dbc0(c, eax, edx); /* 0x41D37A call sub_41DBC0 */
    ebp = eax; /* 0x41D37F mov ebp,eax */
    goto l_41D025; /* 0x41D381 jmp loc_41D025 */
l_41D386:
    eax = 0x0A; /* 0x41D386 mov eax,0Ah */
    goto l_41D0A7; /* 0x41D38B jmp loc_41D0A7 */
l_41D390:
    eax = 0x4B; /* 0x41D390 mov eax,4Bh */
    goto l_41D0C7; /* 0x41D395 jmp loc_41D0C7 */
l_41D39A:
    ecx = G32(0x45AF08); /* 0x41D39A mov ecx,ds:dword_45AF08 */
    esi = G32(0x45AF04); /* 0x41D3A0 mov esi,ds:dword_45AF04 */
    eax = 1; /* 0x41D3A6 mov eax,1 */
    if (ecx <= esi) /* 0x41D3AB cmp ecx,esi */ /* 0x41D3AD jbe short loc_41D3B4 */
        goto l_41D3B4;
    eax = 2; /* 0x41D3AF mov eax,2 */
l_41D3B4:
    ecx = G32(0x45AF0C); /* 0x41D3B4 mov ecx,ds:dword_45AF0C */
    if (ecx <= M32((0x45AF00UL + eax * 4) & U32MASK)) /* 0x41D3BA cmp ecx,ds:dword_45AF00[eax*4] */ /* 0x41D3C1 jbe short loc_41D3C8 */
        goto l_41D3C8;
    eax = 3; /* 0x41D3C3 mov eax,3 */
l_41D3C8:
    ecx = G32(0x45AF10); /* 0x41D3C8 mov ecx,ds:dword_45AF10 */
    if (ecx <= M32((0x45AF00UL + eax * 4) & U32MASK)) /* 0x41D3CE cmp ecx,ds:dword_45AF00[eax*4] */ /* 0x41D3D5 jbe short loc_41D431 */
        goto l_41D431;
    eax = 4; /* 0x41D3D7 mov eax,4 */
l_41D3DC:
    if (ebx >= 0x0AU) /* 0x41D3DC cmp ebx,0Ah */ /* 0x41D3DF jnb short loc_41D43B */
        goto l_41D43B;
l_41D3E1:
    ecx = 0x0A; /* 0x41D3E1 mov ecx,0Ah */
    ecx = (ecx - ebx) & U32MASK; /* 0x41D3E6 sub ecx,ebx */
    ebx = (ecx * 4) & U32MASK; /* 0x41D3E8 lea ebx,ds:0[ecx*4] */
    if (ebp >= 0x800U) /* 0x41D3EF cmp ebp,800h */ /* 0x41D3F5 jnb short loc_41D442 */
        goto l_41D442;
    ecx = 0x41; /* 0x41D3F7 mov ecx,41h */
l_41D3FC:
    edi = M32(0x45AF00); /* 0x41D3FC mov edi,ds:dword_45AF00 */
    ebx = (ebx + ecx) & U32MASK; /* 0x41D402 add ebx,ecx */
    ebx = (ebx * edi) & U32MASK; /* 0x41D404 imul ebx,edi */
    ecx = M32((0x45AF00UL + eax * 4) & U32MASK); /* 0x41D407 mov ecx,ds:dword_45AF00[eax*4] */
    ecx = (ecx << 6) & U32MASK; /* 0x41D40E shl ecx,6 */
    if (ecx <= ebx) /* 0x41D411 cmp ecx,ebx */ /* 0x41D413 jbe loc_41D126 */
        goto l_41D126; /* (dl == 0 on this path -> loc_41D449) */
    edx = ebp; /* 0x41D419 mov edx,ebp */
    ebx = 0; /* 0x41D41B xor ebx,ebx */
    ecx = 0; /* 0x41D41D xor ecx,ecx */
    ebx = (ebx & 0xFFFFFF00UL) | (var_24 & 0xFFU); /* 0x41D41F mov bl,[esp+44h+var_24] (bl lane only) */
    ecx = (ecx & 0xFFFFFF00UL) | (eax & 0xFFU); /* 0x41D423 mov cl,al (cl lane only) */
    eax = var_2C; /* 0x41D425 mov eax,[esp+44h+var_2C] */
    ebx = t[ebx & 0xFFU]; /* 0x41D429 mov ebx,[esp+ebx*4+44h+var_44] */
    goto l_41D11F; /* 0x41D42C jmp loc_41D11F */
l_41D431:
    if (eax != 3) /* 0x41D431 cmp eax,3 */ /* 0x41D434 jnz short loc_41D3DC */
        goto l_41D3DC;
    goto l_41D126; /* 0x41D436 jmp loc_41D126 (dl == 0 here -> loc_41D449) */
l_41D43B:
    ebx = 0x0A; /* 0x41D43B mov ebx,0Ah */
    goto l_41D3E1; /* 0x41D440 jmp short loc_41D3E1 */
l_41D442:
    ecx = 0x50; /* 0x41D442 mov ecx,50h */
    goto l_41D3FC; /* 0x41D447 jmp short loc_41D3FC */
l_41D449:
    eax = 0; /* 0x41D449 xor eax,eax */
    G32(0x45AF28) = eax; /* 0x41D44B mov ds:dword_45AF28,eax */
    G32(0x45AF30) = eax; /* 0x41D450 mov ds:dword_45AF30,eax */
    goto l_41D140; /* 0x41D455 jmp loc_41D140 */
l_41D45A:
    eax = var_24 & 0xFFU; /* 0x41D45A mov al,[esp+44h+var_24] (al lane) */
    if ((eax & 0xFFU) != (unsigned int)G8(0x45C67D)) /* 0x41D45E cmp al,ds:byte_45C67D */ /* 0x41D464 jnz loc_41D14B */
        goto l_41D14B;
    goto l_41D35A; /* 0x41D46A jmp loc_41D35A */
l_41D46F:
    if ((ecx & 0xFFU) == 6) /* 0x41D46F cmp cl,6 */ /* 0x41D472 jz loc_41D33F */
        goto l_41D33F;
    eax = 0; /* placeholder removed */
    eax &= 0xFFFFFF00UL; /* 0x41D478 xor al,al */
    goto l_41D341; /* 0x41D47A jmp loc_41D341 */
}

/* APPEND */

/* ---- workers: S4b leaves (from draft_s4b.c) ---- */
void d4v_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel)
{
    unsigned int ebp = 0;
    unsigned int eax = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int ebx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int saved = 0;
    ebp = sym & 0xFFU; /* 0x41CD38 movzx ebp,al */
    eax = ebp * 8U; /* 0x41CD3B lea eax,[ebp*8] */
    eax += 0x459BA8U; /* 0x41CD42 add unk_459BA8 */
    ecx = 0; /* 0x41CD47 xor ecx,ecx */
    ecx = (unsigned int)M16(eax + 6); /* 0x41CD49 mov cx,[eax+6] */
    G32(0x45AF68) = ecx; /* 0x41CD4D */
    edx = sel & 0xFFU; /* 0x41CD53 and edx,0FFh */
    saved = edx; /* 0x41CD59 mov [var_18],edx */
    edx += edx; /* 0x41CD5C add edx,edx */
    eax += edx; /* 0x41CD5E add eax,edx */
    edx = 0; /* 0x41CD60 xor edx,edx */
    edx = (unsigned int)M16(eax); /* 0x41CD62 mov dx,[eax] */
    G32(0x45AF6C) = edx; /* 0x41CD65 */
    eax = (unsigned int)M16(eax + 2) & 0xFFFFU; /* 0x41CD6B mov ax,[eax+2]; 0x41CD6F and eax,0FFFFh */
    G32(0x45AF70) = eax; /* 0x41CD74 */
    eax = 0x45AF68U; /* 0x41CD79 mov eax,offset dword_45AF68 */
    ebx = M32(eax); /* 0x41CD7E mov ebx,[eax] */
    ecx = M32(eax + 4); /* 0x41CD80 mov ecx,[eax+4] */
    edi = M32(eax + 8); /* 0x41CD83 mov edi,[eax+8] */
    eax = G32(0x45D0BC); /* 0x41CD86 */
    edx = 0; /* 0x41CD8B xor edx,edx (div high half) */
    edi -= ecx; /* 0x41CD8D sub edi,ecx */
    ebx = ebx ? eax / ebx : 0; /* 0x41CD8F div ebx: edx:eax/ebx; UNCERTAIN(0x41CD8F): tot==0 would #DE on hw; guarded here, keep direct `/` per rule */
    ebx = eax; /* 0x41CD91 mov ebx,eax (quot) */
    edx = 0; /* 0x41CD93 xor edx,edx (adc base; imul does not touch edx) */
    ecx *= eax; /* 0x41CD95 imul ecx,eax (low*quot, wraps) */
    ebx *= edi; /* 0x41CD98 imul ebx,edi (diff*quot = new range, wraps) */
    edi = G32(0x45D0C4); /* 0x41CD9B */
    {
        unsigned int lo = ecx + G32(0x45D0B4); /* 0x41CDA1 add ecx,[45D0B4] */
        edi += (lo < ecx ? 1U : 0U); /* 0x41CDA7 adc edi,edx(=0)+CF */
        ecx = lo;
    }
    esi = G32(0x45D0B0); /* 0x41CDA9 */
    if (ebx >= 0x1000000UL) goto rc_done; /* 0x41CDAF cmp; 0x41CDB5 jnb (unsigned) */
    for (;;) { /* 0x41CDB7 loc_41CDB7 */
        if (edi != 0) goto rc_emit; /* 0x41CDB7 test edi,edi; 0x41CDB9 jnz */
        if (ecx >= 0xFF000000UL) goto rc_shift; /* 0x41CDBB cmp; 0x41CDC1 jnb (unsigned) */
rc_emit: /* 0x41CDC3 loc_41CDC3 */
        eax = G32(0x45D0C0); /* 0x41CDC3 */
        edx = 0xFFU; /* 0x41CDC8 mov edx,0FFh */
        eax += edi; /* 0x41CDCD add eax,edi */
        eax &= edx; /* 0x41CDCF and eax,edx */
        d4v_put_byte(c, eax); /* 0x41CDD1 call sub_4221F0 */
        if (esi == 0) goto rc_nocarry; /* 0x41CDD6 test esi,esi; 0x41CDD8 jz */
        eax = edi; /* 0x41CDDA mov eax,edi */
        eax += edx; /* 0x41CDDC add eax,edx */
        eax &= edx; /* 0x41CDDE and eax,edx */
        do { /* 0x41CDE0 loc_41CDE0 */
            d4v_put_byte(c, eax); /* 0x41CDE0 call sub_4221F0 */
            esi--; /* 0x41CDE5 dec esi */
        } while (esi != 0); /* 0x41CDE6 jnz (unsigned wraparound loop) */
rc_nocarry: /* 0x41CDE8 loc_41CDE8 */
        eax = ecx; /* 0x41CDE8 mov eax,ecx */
        esi--; /* 0x41CDEA dec esi */
        eax >>= 24; /* 0x41CDEB shr eax,18h */
        G32(0x45D0C0) = eax; /* 0x41CDEE */
rc_shift: /* 0x41CDF3 loc_41CDF3 */
        ebx <<= 8; /* 0x41CDF3 shl ebx,8 */
        esi++; /* 0x41CDF6 inc esi */
        ecx <<= 8; /* 0x41CDF7 shl ecx,8 */
        edi = 0; /* 0x41CDFA xor edi,edi */
        if (ebx < 0x1000000UL) continue; /* 0x41CDFC cmp; 0x41CE02 jb (unsigned) */
        break;
    }
rc_done: /* 0x41CE04 loc_41CE04 */
    G32(0x45D0BC) = ebx; /* 0x41CE04 */
    G32(0x45D0B4) = ecx; /* 0x41CE0A */
    G32(0x45D0C4) = edi; /* 0x41CE10 */
    G32(0x45D0B0) = esi; /* 0x41CE16 */
    edx = saved; /* 0x41CE1C mov edx,[var_18] */
    eax = ebp; /* 0x41CE1F mov eax,ebp */
    d4v_416c80(c, eax, edx); /* 0x41CE21 call sub_416C80 */
    /* 0x41CE26 add esp,4; pops; 0x41CE2E retn */
}

/* ---- sub_421FA0 (lst 50486..50545, 60 lines) ----
 * Role hypothesis: input-buffer manager. Fast path: enough unread bytes
 * (45CD20-45CD18 >= n) so just publish pos. Slow path: compact the
 * buffer (sub_4285A0 memmove), snapshot counters, then pull more bytes
 * through the [0x4370D8] reader until n bytes available; sticky error 3
 * (ebx=3) into 436EC8 on short read.
 * Entry-live regs (first-use): eax=n (mov edi,eax; no prior eax def).
 * esi/edi/edx/ecx/ebx all defined before use. Confirmed.
 * Frame: 5 pushes, no locals.
 */
void d4v_421fa0(dec4v_ctx *c, unsigned int n)
{
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int edx = 0;
    unsigned int ecx = 0;
    unsigned int ebx = 0;
    unsigned int eax = 0;
    esi = G32(0x45CD20); /* 0x421FA5 */
    edi = n; /* 0x421FAB mov edi,eax (entry eax=n) */
    edx = G32(0x45CD18); /* 0x421FAD */
    ecx = esi - edx; /* 0x421FB3 mov ecx,esi; 0x421FB5 sub ecx,edx */
    if (ecx >= edi) { /* 0x421FB7 cmp ecx,eax; 0x421FB9 jb (unsigned) */
        G32(0x45CD20) = esi; /* 0x421FBB */
        return; /* pops; 0x421FC6 retn */
    }
    eax = G32(0x436ED0); /* 0x421FC7 loc_421FC7 */
    ebx = ecx; /* 0x421FCC mov ebx,ecx */
    edx += eax; /* 0x421FCE add edx,eax (src = base+readpos) */
    esi = ecx; /* 0x421FD0 mov esi,ecx */
    d4v_memmove(c, eax, edx, ebx); /* 0x421FD2 call sub_4285A0 (dst=eax,src=edx,n=ebx) */
    ecx = 0; /* 0x421FD7 xor ecx,ecx */
    eax = G32(0x45CD24); /* 0x421FD9 */
    ebx = 3; /* 0x421FDE mov ebx,3 */
    G32(0x440E70) = eax; /* 0x421FE3 */
    eax = G32(0x45CD28); /* 0x421FE8 */
    G32(0x45CD18) = ecx; /* 0x421FED */
    G32(0x440E74) = eax; /* 0x421FF3 */
    for (;;) { /* 0x421FF8 loc_421FF8 */
        edx = G32(0x45CD2C); /* 0x421FF8 */
        eax = G32(0x436ED0); /* 0x421FFE */
        ecx = esi; /* 0x422003 mov ecx,esi */
        edx -= esi; /* 0x422005 sub edx,esi */
        eax += esi; /* 0x422007 add eax,esi */
        G32(0x45CD20) = esi; /* 0x422009a mov [45CD20],esi */
        eax = d4v_4370d8(c, eax, edx, ecx); /* 0x422009b call [4370D8]; UNCERTAIN(0x422009): arg mapping guessed */
        esi = eax; /* 0x422015 mov esi,eax */
        if (eax == 0) { /* 0x422017 test eax,eax; 0x422019 jnz */
            G32(0x436EC8) = ebx; /* 0x42201B (ebx=3) */
            return; /* DIVERGENCE-NOTE vs native spin: EOF here means a
                     * truncated stream; 436EC8=3 still fails the run */
        }
        /* 0x422021 loc_422021 */
        esi += ecx; /* 0x422021 add esi,ecx */
        if (esi < edi) continue; /* 0x422023 cmp esi,edi; 0x422025 jb (unsigned) */
        break;
    }
    G32(0x45CD20) = esi; /* 0x422027 */
    /* pops; 0x422032 retn */
}

/* ---- sub_41E640 (lst 44083..44351, 269 lines) ----
 * Role hypothesis: per-position context/predictor selector. Reads order
 * byte 45C67D and valid flag 45C67E: if flag clear, replays the
 * (ebx,67B,679) record at 45AF34[45AF28] (bump 45AF28); else computes a
 * signed-div16 blended score into 451D3C..50 from the 0x45356C model
 * (order<7: fixed base; order>=7: entry AF48*124) plus history tables
 * 451B3C/45192C.., picks output bytes 45C679/45C67B and a class ebx
 * (0x2E3/0x2E7/edi+0x2EB/edi+0x2F3/2/edi+0x2F6/4 via [ecx+70/74/78]
 * presence flags), appends a new record when 45AF28 < 45AF30.
 * Entry-live regs (first-use): NONE confirmed. eax first def by lea
 * (0x41E686); ebx by xor (0x41E672); ecx by mov-offset (0x41E776, and
 * 0x41E837-path lea before any read); edx by mem load (0x41E66C);
 * ebp by mem load (0x41E674); esi/edi from 45AF28/45AF48 loads.
 * Frame: 6 pushes, no locals.
 */
void d4v_41e640(dec4v_ctx *c)
{
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int ebp = 0;
    esi = G32(0x45AF28); /* 0x41E646 */
    edi = G32(0x45AF48); /* 0x41E64C */
    if (G8(0x45C67E) != 0) goto e640_main; /* 0x41E652 cmp; 0x41E659 jz */
    /* 0x41E7E8 loc_41E7E8 (flag clear: replay path) */
    if (esi >= G32(0x45AF30)) goto e640_main; /* 0x41E7E8 cmp; 0x41E7EE jnb (unsigned) */
    {
        unsigned int base = 0;
        edx = esi * 8U; /* 0x41E7F4 lea edx,[esi*8] */
        eax = G32(0x45AF34); /* 0x41E7FB */
        esi++; /* 0x41E800 inc esi */
        base = eax + edx; /* implicit address */
        ebx = M32(base); /* 0x41E801 mov ebx,[edx+eax] */
        ecx = (unsigned int)M8(base + 4); /* 0x41E804 mov cl,[edx+eax+4] */
        edx = (unsigned int)M8(base + 5); /* 0x41E808 mov dl,[edx+eax+5] */
        G8(0x45C67B) = (unsigned char)ecx; /* 0x41E80C */
        G8(0x45C679) = (unsigned char)edx; /* 0x41E812 */
    }
    goto e640_store; /* 0x41E818 jmp loc_41E7CF */
e640_main: /* 0x41E65F loc_41E65F */
    if (G8(0x45C67D) >= 7) goto e640_hi; /* 0x41E65F cmp; 0x41E666 jnb (unsigned) */
    edx = G32(0x45192C); /* 0x41E66C */
    ebx = 0; /* 0x41E672 xor ebx,ebx */
    ebp = G32(0x451924); /* 0x41E674 */
    G32(0x451D3C) = ebx; /* 0x41E67A */
    G32(0x451D40) = edx; /* 0x41E680 */
    eax = edx + edx; /* 0x41E686 lea eax,[edx+edx] */
    ebx = G32(0x45AF38); /* 0x41E689 */
    edx = G32(0x451930); /* 0x41E68F */
    ebx -= ebp; /* 0x41E695 sub ebx,ebp */
    eax -= edx; /* 0x41E697 sub eax,edx */
    ebx &= 0x7FU; /* 0x41E699 and ebx,7Fh */
    G32(0x451D44) = eax; /* 0x41E69C */
    edx = M32(0x451B3C + ebx * 4); /* 0x41E6A1 */
    eax = G32(0x453574); /* 0x41E6A8 */
    G32(0x45357C) = edx; /* 0x41E6AD */
    G32(0x453578) = eax; /* 0x41E6B3 */
    eax = G32(0x453570); /* 0x41E6B8 */
    edx = G32(0x453598); /* 0x41E6BD */
    G32(0x453574) = eax; /* 0x41E6C3 */
    edx -= G32(0x45356C); /* 0x41E6C8 sub edx,[45356C] */
    eax = G32(0x453580); /* 0x41E6CE */
    G32(0x453570) = edx; /* 0x41E6D3 */
    edx = G32(0x453598); /* 0x41E6D9 */
    eax *= edx; /* 0x41E6DF imul eax,edx (signed, wraps) */
    G32(0x45356C) = edx; /* 0x41E6E2 */
    edx = G32(0x453594); /* 0x41E6E8 */
    edx <<= 4; /* 0x41E6EE shl edx,4 */
    ebp = G32(0x453570); /* 0x41E6F1 */
    eax += edx; /* 0x41E6F7 add eax,edx */
    edx = G32(0x453584); /* 0x41E6F9 */
    edx *= ebp; /* 0x41E6FF imul edx,ebp */
    ebp = G32(0x453574); /* 0x41E702 */
    eax += edx; /* 0x41E708 add eax,edx */
    edx = G32(0x453588); /* 0x41E70A */
    edx *= ebp; /* 0x41E710 imul edx,ebp */
    ebp = G32(0x453578); /* 0x41E713 */
    eax += edx; /* 0x41E719 add eax,edx */
    edx = G32(0x45358C); /* 0x41E71B */
    edx *= ebp; /* 0x41E721 imul edx,ebp */
    ebp = G32(0x45357C); /* 0x41E724 */
    eax += edx; /* 0x41E72A add eax,edx */
    edx = G32(0x453590); /* 0x41E72C */
    edx *= ebp; /* 0x41E732 imul edx,ebp */
    edx += eax; /* 0x41E735 add edx,eax */
    {
        unsigned int cf = 0; /* sar/sbb/shl/sar = C-truncating /16 */
        eax = edx; /* 0x41E739a mov eax,edx */
        edx = (unsigned int)((int)edx >> 31); /* 0x41E739b sar edx,1Fh */
        cf = edx & 1U; /* CF out of 0x41E73C shl edx,4 (0 or 1) */
        edx <<= 4; /* 0x41E73C shl edx,4 */
        eax = eax - edx - cf; /* 0x41E73F sbb eax,edx */
        eax = (unsigned int)((int)eax >> 4); /* 0x41E741 sar eax,4 */
    }
    G32(0x451D48) = eax; /* 0x41E744 */
    eax = M32(0x451B3C + ebx * 4); /* 0x41E749 */
    edx = G32(0x451928); /* 0x41E750 */
    G32(0x451D4C) = eax; /* 0x41E756 */
    eax = G32(0x45AF38); /* 0x41E75B */
    eax -= edx; /* 0x41E760 sub eax,edx */
    eax &= 0x7FU; /* 0x41E762 and eax,7Fh */
    eax = M32(0x451B3C + eax * 4); /* 0x41E765 */
    G32(0x451D50) = eax; /* 0x41E76C */
    eax = G32(0x451920); /* 0x41E771 */
    ecx = 0x45356CU; /* 0x41E776 mov ecx,offset dword_45356C */
    eax = (unsigned int)M8((0x451D3CUL + eax * 4UL) & U32MASK); /* 0x41E77B mov al,byte ptr ds:dword_451D3C[eax*4] (array base; NOT contents) */
    ebx = G32(0x451920); /* 0x41E782 */
    G8(0x45C679) = (unsigned char)eax; /* 0x41E788 */
    if (ebx >= 4) { /* 0x41E78D cmp ebx,4; 0x41E790 jb (unsigned) */
        edx = (unsigned int)G8(0x45192C); /* 0x41E792 mov dl,[45192C] (low byte) */
        eax = (eax + edx) & 0xFFU; /* 0x41E798 add al,dl */
        G8(0x45C679) = (unsigned char)eax; /* 0x41E79A */
    }
    /* 0x41E79F loc_41E79F */
    if (G8(0x45C67D) != 5) { /* 0x41E79F cmp; 0x41E7A6 jnz */
        ebx = 0x2E7U; /* 0x41E81A loc_41E81A */
    } else {
        edx = (unsigned int)G8(0x45C679); /* 0x41E7A8 */
        edx = (edx + 0x80U) & 0xFFU; /* 0x41E7AE add dl,80h */
        ebx = 0x2E3U; /* 0x41E7B1 mov ebx,2E3h */
        G8(0x45C679) = (unsigned char)edx; /* 0x41E7B6 */
    }
    /* 0x41E7BC loc_41E7BC: class adjust via presence flags at [ecx+70/74/78] */
    if (M32(ecx + 0x70) != 0) goto e640_t74; /* 0x41E7BC cmp; 0x41E7C0 jnz */
    ebx++; /* 0x41E7C2 inc ebx */
    goto e640_classdone; /* 0x41E7C3 loc_41E7C3 tests below */
e640_t74: /* 0x41E821 loc_41E821 */
    if (M32(ecx + 0x74) == 0) { /* 0x41E821 cmp; 0x41E825 jnz */
        ebx += 2; /* 0x41E827 add ebx,2 */
        goto e640_classdone; /* 0x41E82A jmp loc_41E7C3 */
    }
    /* 0x41E82C loc_41E82C */
    if (M32(ecx + 0x78) != 0) goto e640_classdone; /* 0x41E82C cmp; 0x41E830 jnz */
    ebx += 3; /* 0x41E832 add ebx,3 */
    goto e640_classdone; /* 0x41E835 jmp loc_41E7C3 */
e640_hi: /* 0x41E837 loc_41E837 (order>=7: entry AF48*124 at 0x45356C) */
    ecx = edi * 4U; /* 0x41E837 lea ecx,[edi*4] */
    edx = ecx; /* 0x41E83E mov edx,ecx */
    ecx <<= 5; /* 0x41E840 shl ecx,5 */
    ecx -= edx; /* 0x41E843 sub ecx,edx (ecx=edi*124) */
    ecx += 0x45356CU; /* 0x41E845 add offset dword_45356C */
    edx = M32(ecx + 4); /* 0x41E84B */
    eax = M32(ecx + 8); /* 0x41E84E */
    M32(ecx + 8) = edx; /* 0x41E851 */
    ebx = M32(ecx); /* 0x41E854 */
    edx = M32(ecx + 0x2C); /* 0x41E856 */
    M32(ecx + 0x0C) = eax; /* 0x41E859 */
    edx -= ebx; /* 0x41E85C sub edx,ebx */
    M32(ecx + 4) = edx; /* 0x41E85E */
    eax = M32(ecx + 0x2C); /* 0x41E861 */
    edx = M32(ecx + 0x14); /* 0x41E864 */
    edx *= eax; /* 0x41E867 imul edx,eax */
    M32(ecx) = eax; /* 0x41E86A */
    eax = M32(ecx + 0x28); /* 0x41E86C */
    eax <<= 4; /* 0x41E86F shl eax,4 */
    ebx = M32(ecx + 4); /* 0x41E872 */
    eax += edx; /* 0x41E875 add eax,edx */
    edx = M32(ecx + 0x18); /* 0x41E877 */
    edx *= ebx; /* 0x41E87A imul edx,ebx */
    ebp = M32(ecx + 8); /* 0x41E87D */
    eax += edx; /* 0x41E880 add eax,edx */
    edx = M32(ecx + 0x1C); /* 0x41E882 */
    edx *= ebp; /* 0x41E885 imul edx,ebp */
    ebx = M32(ecx + 0x0C); /* 0x41E888 */
    eax += edx; /* 0x41E88B add eax,edx */
    edx = M32(ecx + 0x20); /* 0x41E88D */
    edx *= ebx; /* 0x41E890 imul edx,ebx */
    ebp = G32(0x451934); /* 0x41E893 */
    eax += edx; /* 0x41E899 add eax,edx */
    edx = M32(ecx + 0x24); /* 0x41E89B */
    edx *= ebp; /* 0x41E89E imul edx,ebp */
    edx += eax; /* 0x41E8A1 add edx,eax */
    {
        unsigned int cf = 0;
        eax = edx; /* 0x41E8A3 mov eax,edx */
        edx = (unsigned int)((int)edx >> 31); /* 0x41E8A5 sar edx,1Fh */
        cf = edx & 1U; /* CF out of 0x41E8A8 shl edx,4 */
        edx <<= 4; /* 0x41E8A8 shl edx,4 */
        eax = eax - edx - cf; /* 0x41E8AB sbb eax,edx */
        eax = (unsigned int)((int)eax >> 4); /* 0x41E8AD sar eax,4 */
    }
    G32(0x451D3C) = eax; /* 0x41E8B0 */
    edx = M32(ecx + 0x5C); /* 0x41E8B5 */
    if (edx <= M32(ecx + 0x60)) { /* 0x41E8B8 cmp; 0x41E8BB jbe (unsigned) */
        edx = (unsigned int)G8(0x451D3C); /* 0x41E901 loc_41E901: low byte of dword just stored */
    } else {
        edx = (unsigned int)M8(ecx + 0x28); /* 0x41E8BD mov dl,[ecx+28h] */
    }
    /* 0x41E8C0 loc_41E8C0 */
    G8(0x45C679) = (unsigned char)edx; /* 0x41E8C0 */
    edx = (edx & ~0xFF00U) | ((unsigned int)G8(0x45C67D) << 8); /* 0x41E8C6 mov dh,[45C67D] (sets bits 8-15 only) */
    if (((edx >> 8) & 0xFFU) != 7) goto e640_not7; /* 0x41E8CC cmp dh,7; 0x41E8CF jnz */
    ebx = (ebx & 0xFFFF00FFUL) | ((unsigned int)G8(0x45C679) << 8); /* 0x41E8D1 mov bh,[45C679] */
    ebx = (ebx & 0xFFFF00FFUL) | ((((((ebx >> 8) & 0xFFU) + 0x80U) & 0xFFU)) << 8); /* 0x41E8D7 add bh,80h */
    ebp = M32(ecx + 0x70); /* 0x41E8DA */
    G8(0x45C679) = (unsigned char)((ebx >> 8) & 0xFFU); /* 0x41E8DD mov [45C679],bh */
    ebx = edi + 0x2EBU; /* 0x41E8E3 lea ebx,[edi+2EBh] */
    if (ebp != 0) goto e640_7t74; /* 0x41E8E9 test ebp,ebp; 0x41E8EB jnz */
    ebx += 2; /* 0x41E8ED add ebx,2 */
    goto e640_emit67b; /* 0x41E8F0 loc_41E8F0 */
e640_7t74: /* 0x41E909 loc_41E909 */
    if (M32(ecx + 0x74) == 0) { /* 0x41E909 cmp; 0x41E90D jnz */
        ebx += 4; /* 0x41E90F add ebx,4 */
        goto e640_emit67b; /* 0x41E912 jmp loc_41E8F0 */
    }
    /* 0x41E914 loc_41E914 */
    if (M32(ecx + 0x78) != 0) goto e640_emit67b; /* 0x41E914 cmp; 0x41E918 jnz */
    ebx += 6; /* 0x41E91A add ebx,6 */
    goto e640_emit67b; /* 0x41E91D jmp loc_41E8F0 */
e640_not7: /* 0x41E91F loc_41E91F */
    if (((edx >> 8) & 0xFFU) != 8) goto e640_not78; /* 0x41E91F cmp dh,8; 0x41E922 jnz */
    if (M32(ecx + 0x70) != 0) { /* 0x41E924 cmp; 0x41E928 jz */
        ebx = edi; /* 0x41E92A mov ebx,edi */
        goto e640_8t; /* 0x41E92C loc_41E92C below */
    }
    /* 0x41E943 loc_41E943 */
    ebx = 2; /* 0x41E943 mov ebx,2 */
    goto e640_8t; /* 0x41E948 jmp loc_41E92C */
e640_not78: /* 0x41E94A loc_41E94A */
    if (M32(ecx + 0x70) == 0) { /* 0x41E94A cmp; 0x41E94E jz */
        ebx = 4; /* 0x41E969 loc_41E969 */
        goto e640_9t; /* 0x41E96E jmp loc_41E952 */
    }
    ebx = edi; /* 0x41E950 mov ebx,edi */
e640_9t: /* 0x41E952 loc_41E952 */
    ebx += 0x2F6U; /* 0x41E952 add ebx,2F6h */
    edx = (unsigned int)M8(0x45375CU + edi); /* 0x41E958 mov dl,[45375C+edi] */
    G8(0x45C67B) = (unsigned char)edx; /* 0x41E95E */
    goto e640_classdone; /* 0x41E964 jmp loc_41E7C3 */
e640_8t: /* 0x41E92C loc_41E92C */
    ebx += 0x2F3U; /* 0x41E92C add ebx,2F3h */
    edx = (unsigned int)M8(0x45375CU + edi); /* 0x41E932 mov dl,[45375C+edi] */
    G8(0x45C67B) = (unsigned char)edx; /* 0x41E938 */
    goto e640_classdone; /* 0x41E93E jmp loc_41E7C3 */
e640_emit67b: /* 0x41E8F0 loc_41E8F0 */
    edx = (unsigned int)M8(0x45375CU + edi); /* 0x41E8F0 mov dl,[45375C+edi] */
    G8(0x45C67B) = (unsigned char)edx; /* 0x41E8F6 */
    goto e640_classdone; /* 0x41E8FC jmp loc_41E7C3 */
e640_classdone: /* 0x41E7C3 loc_41E7C3 */
    if (esi >= G32(0x45AF30)) goto e640_store; /* 0x41E7C3 cmp esi,[45AF30]; 0x41E7C9 jb (unsigned) */
    {
        unsigned int base = 0;
        edx = G32(0x45AF34); /* 0x41E970 */
        eax = esi * 8U; /* 0x41E976 lea eax,[esi*8] */
        edx += eax; /* 0x41E97D add edx,eax */
        ecx = (unsigned int)G8(0x45C67B); /* 0x41E97F mov cl,[45C67B] */
        M32(edx) = ebx; /* 0x41E985 mov [edx],ebx */
        M8(edx + 4) = (unsigned char)ecx; /* 0x41E987 mov [edx+4],cl */
        ecx = (unsigned int)G8(0x45C679); /* 0x41E98A mov cl,[45C679] */
        esi++; /* 0x41E990 inc esi */
        M8(edx + 5) = (unsigned char)ecx; /* 0x41E991 mov [edx+5],cl */
    }
    goto e640_store; /* 0x41E994 jmp loc_41E7CF */
e640_store: /* 0x41E7CF loc_41E7CF */
    G32(0x45AF48) = edi; /* 0x41E7CF */
    G32(0x45AF28) = esi; /* 0x41E7D5 */
    G32(0x45AF3C) = ebx; /* 0x41E7DB */
    /* pops; retn */
}

/* ---- sub_41E9F0 (lst 44378..45083, 706 lines) ----
 * Role hypothesis: predictor/model updater called per decoded symbol.
 * Two variants share one shape: order<=6 path (fixed 0x45356C model
 * base, ACCT-style [ebx+30h..58h] accumulators via 44D920 weights,
 * min-rebalance of slots +34h..58h with 10-case switch on
 * [ebx+14h..24h]) and order>=7 path (per-entry edx=45356C+AF48*124,
 * extra 435D50 [5Ch/60h] accumulators, zero-generation reset,
 * 451934/38 swap at order 9, order-7 [74h/78h] lane store).
 * Entry-live regs (first-use): al=sym (mov cl,al), dl=arg (stored to
 * var_18). Callers confirm: 0x417C52 (al=cl-sym, dl=old-al) and
 * 0x41E58A (al=bh-sym, dl=bl). High bytes of ecx on entry are garbage
 * (only cl is read before full-width overwrite); edx callers zero it.
 * Frame: 5 pushes + sub esp,30h (var_44..var_18 locals).
 */
void d4v_41e9f0(dec4v_ctx *c, unsigned int sym, unsigned int arg)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int ebp = 0;
    unsigned int v20 = 0; /* var_20: signed symbol */
    unsigned int v24 = 0; /* var_24: sym - 45192C */
    unsigned int varg = 0; /* var_18: saved dl byte */
    unsigned int v1c = 0; /* var_1C temp */
    unsigned int v28 = 0; /* var_28 temp */
    unsigned int v2c = 0; /* var_2C temp */
    unsigned int va[6] = {0}; /* var_44,var_40,var_3C,var_38,var_34,var_30 */
    ecx = sym & 0xFFU; /* 0x41E9F8 mov cl,al (entry al=sym; high bytes UNCERTAIN, never read before overwrite) */
    varg = arg & 0xFFU; /* 0x41E9FA mov [var_18],dl (entry dl=arg) */
    if (G8(0x45C67E) == 0) { /* 0x41E9FE cmp byte_45C67E,0; 0x41EA05 jz */
        eax = G32(0x45AF28); /* 0x41EE46 loc_41EE46 */
        if (eax < G32(0x45AF30)) return; /* 0x41EE4B cmp; 0x41EE51 jnb (unsigned): early out */
    }
    /* 0x41EA0B loc_41EA0B */
    edx = (unsigned int)G8(0x45C67D); /* 0x41EA0B mov dl,[45C67D] */
    if (edx >= 7) goto e9f0_hi; /* 0x41EA11 cmp dl,7; 0x41EA14 jnb (unsigned) */
    ebx = 0x45356CU; /* 0x41EA1A mov ebx,offset dword_45356C */
    if (edx != 6) { /* 0x41EA1F cmp dl,6; 0x41EA22 jnz loc_41EE60 */
        eax = 0; /* 0x41EE60 xor eax,eax */
        eax = ecx & 0xFFU; /* 0x41EE62 mov al,cl */
        eax -= 0x80U; /* 0x41EE64 sub eax,80h */
        goto e9f0_updlo; /* 0x41EE69 jmp loc_41EA2B */
    }
    eax = (unsigned int)(int)(signed char)(ecx & 0xFFU); /* 0x41EA28 movsx eax,cl */
e9f0_updlo: /* 0x41EA2B loc_41EA2B */
    v20 = eax; /* 0x41EA2B mov [var_20],eax */
    eax = v20; /* 0x41EA2F mov eax,[var_20] */
    ebp = G32(0x45192C); /* 0x41EA33 */
    eax -= ebp; /* 0x41EA39 sub eax,ebp */
    v24 = eax; /* 0x41EA3B mov [var_24],eax */
    eax = (unsigned int)G8(0x451D48); /* 0x41EA3F mov al,[451D48] (byte at static addr) */
    eax = (eax - (v20 & 0xFFU)) & 0xFFU; /* 0x41EA44 sub al,[var_20] */
    eax = (unsigned int)(int)(signed char)(eax & 0xFFU); /* 0x41EA48 movsx eax,al */
    eax <<= 4; /* 0x41EA4B shl eax,4 */
    edx = eax & 0xFFFU; /* 0x41EA4E mov edx,eax; 0x41EA50 and edx,0FFFh */
    ecx = M32(ebx + 0x30); /* 0x41EA56 mov ecx,[ebx+30h] */
    edx = M32(0x44D920 + edx * 4); /* 0x41EA59 mov edx,[44D920+edx*4] */
    esi = M32(ebx); /* 0x41EA60 mov esi,[ebx] */
    ecx += edx; /* 0x41EA62 add ecx,edx */
    edx = (eax - esi) & 0xFFFU; /* 0x41EA64 mov edx,eax; 0x41EA66 sub edx,esi; 0x41EA6B and edx,0FFFh */
    M32(ebx + 0x30) = ecx; /* 0x41EA68 mov [ebx+30h],ecx */
    edi = M32(ebx + 0x34); /* 0x41EA71 mov edi,[ebx+34h] */
    edi += M32(0x44D920 + edx * 4); /* 0x41EA74 add edi,[44D920+edx*4] */
    edx = M32(ebx); /* 0x41EA7B mov edx,[ebx] */
    edx += eax; /* 0x41EA7D add edx,eax */
    M32(ebx + 0x34) = edi; /* 0x41EA7F mov [ebx+34h],edi */
    edx &= 0xFFFU; /* 0x41EA82 and edx,0FFFh */
    ebp = M32(ebx + 0x38); /* 0x41EA88 mov ebp,[ebx+38h] */
    edx = M32(0x44D920 + edx * 4); /* 0x41EA8B mov edx,[44D920+edx*4] */
    ecx = M32(ebx + 4); /* 0x41EA92 mov ecx,[ebx+4] */
    ebp += edx; /* 0x41EA95 add ebp,edx */
    edx = (eax - ecx) & 0xFFFU; /* 0x41EA97 mov edx,eax; 0x41EA99 sub edx,ecx; 0x41EA9E and edx,0FFFh */
    M32(ebx + 0x38) = ebp; /* 0x41EA9B mov [ebx+38h],ebp */
    esi = M32(ebx + 0x3C); /* 0x41EAA4 mov esi,[ebx+3Ch] */
    esi += M32(0x44D920 + edx * 4); /* 0x41EAA7 add esi,[44D920+edx*4] */
    edx = M32(ebx + 4); /* 0x41EAAE mov edx,[ebx+4] */
    edx += eax; /* 0x41EAB1 add edx,eax */
    M32(ebx + 0x3C) = esi; /* 0x41EAB3 mov [ebx+3Ch],esi */
    edx &= 0xFFFU; /* 0x41EAB6 and edx,0FFFh */
    edi = M32(ebx + 0x40); /* 0x41EABC mov edi,[ebx+40h] */
    edx = M32(0x44D920 + edx * 4); /* 0x41EABF mov edx,[44D920+edx*4] */
    ebp = M32(ebx + 8); /* 0x41EAC6 mov ebp,[ebx+8] */
    edi += edx; /* 0x41EAC9 add edi,edx */
    edx = (eax - ebp) & 0xFFFU; /* 0x41EACB mov edx,eax; 0x41EACD sub edx,ebp; 0x41EACF and edx,0FFFh */
    M32(ebx + 0x40) = edi; /* 0x41EAD2 mov [ebx+40h],edi */
    ecx = M32(ebx + 0x44); /* 0x41EAD8 mov ecx,[ebx+44h] */
    ecx += M32(0x44D920 + edx * 4); /* 0x41EADB add ecx,[44D920+edx*4] */
    edx = M32(ebx + 8); /* 0x41EAE2 mov edx,[ebx+8] */
    edx += eax; /* 0x41EAE5 add edx,eax */
    M32(ebx + 0x44) = ecx; /* 0x41EAE7 mov [ebx+44h],ecx */
    edx &= 0xFFFU; /* 0x41EAEA and edx,0FFFh */
    esi = M32(ebx + 0x48); /* 0x41EAF0 mov esi,[ebx+48h] */
    edx = M32(0x44D920 + edx * 4); /* 0x41EAF3 mov edx,[44D920+edx*4] */
    edi = M32(ebx + 0x0C); /* 0x41EAFA mov edi,[ebx+0Ch] */
    esi += edx; /* 0x41EAFD add esi,edx */
    edx = (eax - edi) & 0xFFFU; /* 0x41EAFF mov edx,eax; 0x41EB01 sub edx,edi; 0x41EB06 and edx,0FFFh */
    M32(ebx + 0x48) = esi; /* 0x41EB03 mov [ebx+48h],esi */
    ebp = M32(ebx + 0x4C); /* 0x41EB0C mov ebp,[ebx+4Ch] */
    ebp += M32(0x44D920 + edx * 4); /* 0x41EB0F add ebp,[44D920+edx*4] */
    edx = M32(ebx + 0x0C); /* 0x41EB16 mov edx,[ebx+0Ch] */
    edx += eax; /* 0x41EB19 add edx,eax */
    M32(ebx + 0x4C) = ebp; /* 0x41EB1B mov [ebx+4Ch],ebp */
    edx &= 0xFFFU; /* 0x41EB1E and edx,0FFFh */
    ecx = M32(ebx + 0x50); /* 0x41EB24 mov ecx,[ebx+50h] */
    edx = M32(0x44D920 + edx * 4); /* 0x41EB27 mov edx,[44D920+edx*4] */
    esi = M32(ebx + 0x10); /* 0x41EB2E mov esi,[ebx+10h] */
    ecx += edx; /* 0x41EB31 add ecx,edx */
    edx = (eax - esi) & 0xFFFU; /* 0x41EB33 mov edx,eax; 0x41EB35 sub edx,esi; 0x41EB3A and edx,0FFFh */
    M32(ebx + 0x50) = ecx; /* 0x41EB37 mov [ebx+50h],ecx */
    edi = M32(ebx + 0x54); /* 0x41EB40 mov edi,[ebx+54h] */
    edx = M32(0x44D920 + edx * 4); /* 0x41EB43 mov edx,[44D920+edx*4] */
    ebp = M32(ebx + 0x10); /* 0x41EB4A mov ebp,[ebx+10h] */
    edi += edx; /* 0x41EB4D add edi,edx */
    eax += ebp; /* 0x41EB4F add eax,ebp */
    M32(ebx + 0x54) = edi; /* 0x41EB51 mov [ebx+54h],edi */
    eax &= 0xFFFU; /* 0x41EB54 and eax,0FFFh */
    edx = M32(ebx + 0x58); /* 0x41EB59 mov edx,[ebx+58h] */
    edx += M32(0x44D920 + eax * 4); /* 0x41EB5C add edx,[44D920+eax*4] */
    ecx = M32(ebx + 0x6C); /* 0x41EB63 mov ecx,[ebx+6Ch] */
    M32(ebx + 0x58) = edx; /* 0x41EB66 mov [ebx+58h],edx */
    eax = v20 & 0xFFU; /* 0x41EB69 mov al,[var_20] */
    edx = (unsigned int)M8(ebx + 0x28); /* 0x41EB6D mov dl,[ebx+28h] */
    ecx++; /* 0x41EB70 inc ecx */
    eax = (eax - (edx & 0xFFU)) & 0xFFU; /* 0x41EB71 sub al,dl */
    M32(ebx + 0x6C) = ecx; /* 0x41EB73 mov [ebx+6Ch],ecx */
    eax = (unsigned int)(int)(signed char)(eax & 0xFFU); /* 0x41EB76 movsx eax,al */
    M32(ebx + 0x2C) = eax; /* 0x41EB79 mov [ebx+2Ch],eax */
    eax = v20; /* 0x41EB7C mov eax,[var_20] */
    edx = (edx & ~0xFF00U) | ((unsigned int)M8(ebx + 0x6C) << 8); /* 0x41EB80 mov dh,[ebx+6Ch] (sets bits 8-15 only) */
    M32(ebx + 0x28) = eax; /* 0x41EB83 mov [ebx+28h],eax (dword store) */
    if (((edx >> 8) & 0xFU) != 0) goto e9f0_lodone; /* 0x41EB86 test dh,0Fh; 0x41EB89 jnz loc_41EBD1 */
    ecx = 1; /* 0x41EB8B mov ecx,1 */
    eax = ebx + 4; /* 0x41EB90 lea eax,[ebx+4] */
    esi = 0; /* 0x41EB93 xor esi,esi */
    edx = M32(ebx + 0x30); /* 0x41EB95 mov edx,[ebx+30h] */
    M32(ebx + 0x30) = esi; /* 0x41EB98 mov [ebx+30h],esi (=0) */
    for (;;) { /* 0x41EB9B loc_41EB9B */
        edi = M32(eax + 0x30); /* 0x41EB9B mov edi,[eax+30h] */
        if (edx > edi) { /* 0x41EB9E cmp edx,edi; 0x41EBA0 jbe (unsigned) */
            edx = edi; /* 0x41EBA2 mov edx,edi */
            esi = ecx; /* 0x41EBA4 mov esi,ecx */
        }
        /* 0x41EBA6 loc_41EBA6 */
        eax += 4; /* 0x41EBA6 add eax,4 */
        ecx++; /* 0x41EBA9 inc ecx */
        M32(eax + 0x2C) = 0; /* 0x41EBAA mov dword [eax+2Ch],0 */
        if ((int)ecx < 0x0B) continue; /* 0x41EBB1 cmp ecx,0Bh; 0x41EBB4 jl (signed) */
        break;
    }
    esi--; /* 0x41EBB6 dec esi */
    if (esi <= 9) { /* 0x41EBB7 cmp esi,9; 0x41EBBA ja (unsigned) */
        switch (esi) { /* 0x41EBBC jmp off_41E99C[esi*4] */
        case 0: /* 0x41EBC3 loc_41EBC3 */
            ebp = M32(ebx + 0x14); /* 0x41EBC3 */
            if ((int)ebp <= (int)0xFFFFFFE0U) break; /* 0x41EBC6 cmp; 0x41EBC9 jle (signed) */
            eax = ebp - 1; /* 0x41EBCB lea eax,[ebp-1] */
            M32(ebx + 0x14) = eax; /* 0x41EBCE */
            break;
        case 1: /* 0x41EE6E loc_41EE6E */
            esi = M32(ebx + 0x14); /* 0x41EE6E */
            if ((int)esi >= 0x20) break; /* 0x41EE71 cmp esi,20h; 0x41EE74 jge (signed) */
            edi = esi + 1; /* 0x41EE7A lea edi,[esi+1] */
            M32(ebx + 0x14) = edi; /* 0x41EE7D */
            break;
        case 2: /* 0x41EE85 loc_41EE85 */
            edx = M32(ebx + 0x18); /* 0x41EE85 */
            if ((int)edx <= (int)0xFFFFFFE0U) break; /* 0x41EE88 cmp; 0x41EE8B jle (signed) */
            ecx = edx - 1; /* 0x41EE91 lea ecx,[edx-1] */
            M32(ebx + 0x18) = ecx; /* 0x41EE94 */
            break;
        case 3: /* 0x41EE9C loc_41EE9C */
            ebp = M32(ebx + 0x18); /* 0x41EE9C */
            if ((int)ebp >= 0x20) break; /* 0x41EE9F cmp; 0x41EEA2 jge (signed) */
            eax = ebp + 1; /* 0x41EEA8 lea eax,[ebp+1] */
            M32(ebx + 0x18) = eax; /* 0x41EEAB */
            break;
        case 4: /* 0x41EEB3 loc_41EEB3 */
            esi = M32(ebx + 0x1C); /* 0x41EEB3 */
            if ((int)esi <= (int)0xFFFFFFE0U) break; /* 0x41EEB6 cmp; 0x41EEB9 jle (signed) */
            edi = esi - 1; /* 0x41EEBF lea edi,[esi-1] */
            M32(ebx + 0x1C) = edi; /* 0x41EEC2 */
            break;
        case 5: /* 0x41EECA loc_41EECA */
            edx = M32(ebx + 0x1C); /* 0x41EECA */
            if ((int)edx >= 0x20) break; /* 0x41EECD cmp; 0x41EED0 jge (signed) */
            ecx = edx + 1; /* 0x41EED6 lea ecx,[edx+1] */
            M32(ebx + 0x1C) = ecx; /* 0x41EED9 */
            break;
        case 6: /* 0x41EEE1 loc_41EEE1 */
            ebp = M32(ebx + 0x20); /* 0x41EEE1 */
            if ((int)ebp <= (int)0xFFFFFFE0U) break; /* 0x41EEE4 cmp; 0x41EEE7 jle (signed) */
            eax = ebp - 1; /* 0x41EEED lea eax,[ebp-1] */
            M32(ebx + 0x20) = eax; /* 0x41EEF0 */
            break;
        case 7: /* 0x41EEF8 loc_41EEF8 */
            esi = M32(ebx + 0x20); /* 0x41EEF8 */
            if ((int)esi >= 0x20) break; /* 0x41EEFB cmp; 0x41EEFE jge (signed) */
            edi = esi + 1; /* 0x41EF04 lea edi,[esi+1] */
            M32(ebx + 0x20) = edi; /* 0x41EF07 */
            break;
        case 8: /* 0x41EF0F loc_41EF0F */
            if ((int)M32(ebx + 0x20) <= (int)0xFFFFFFE0U) break; /* 0x41EF0F cmp [ebx+20h]; 0x41EF13 jle (signed) */
            M32(ebx + 0x24) = M32(ebx + 0x24) - 1; /* 0x41EF19 dec [ebx+24h] */
            break;
        case 9: /* 0x41EF21 loc_41EF21 */
            if ((int)M32(ebx + 0x20) >= 0x20) break; /* 0x41EF21 cmp [ebx+20h]; 0x41EF25 jge (signed) */
            M32(ebx + 0x24) = M32(ebx + 0x24) + 1; /* 0x41EF2B inc [ebx+24h] */
            break;
        }
    }
e9f0_lodone: /* 0x41EBD1 loc_41EBD1 (also switch default) */
    eax = G32(0x45AF38) & 0x7FU; /* 0x41EBD1 mov eax,[45AF38]; 0x41EBD6 and eax,7Fh */
    edx = v24; /* 0x41EBD9 mov edx,[var_24] */
    M32(0x451B3C + eax * 4) = edx; /* 0x41EBDD mov [451B3C+eax*4],edx */
    eax = G32(0x451920); /* 0x41EBE4 */
    edx = G32(0x45AF38); /* 0x41EBE9 */
    if ((M32(0x434538 + eax * 4) & edx) != 0) goto e9f0_vtab; /* 0x41EBEF test [434538+eax*4],edx; 0x41EBF6 jnz loc_41ED1D */
    edi = 0xFFFFU; /* 0x41EBFC mov edi,0FFFFh */
    esi = 2; /* 0x41EC01 mov esi,2 */
    edx = 8; /* 0x41EC06 mov edx,8 */
    G32(0x451924) = esi; /* 0x41EC0B */
    G32(0x451928) = esi; /* 0x41EC11 */
    eax = esi; /* 0x41EC17 mov eax,esi (=2) */
    for (;;) { /* 0x41EC19 loc_41EC19 */
        ecx = G32(0x45AF38); /* 0x41EC19 */
        esi = ecx - 1; /* 0x41EC1F lea esi,[ecx-1] */
        ebp = esi - eax; /* 0x41EC24 sub ebp,eax (esi-eax; note: ebp=esi copy at 0x41EC22) */
        esi &= 0x7FU; /* 0x41EC26 and esi,7Fh */
        ebp &= 0x7FU; /* 0x41EC29 and ebp,7Fh */
        esi = M32(0x451B3C + esi * 4); /* 0x41EC2C */
        ebp <<= 2; /* 0x41EC33 shl ebp,2 */
        v1c = esi; /* 0x41EC36 mov [var_1C],esi */
        esi = M32(0x451B3C + ebp); /* 0x41EC3A mov esi,[451B3C+ebp] (ss: == ds: assumed) */
        ebp = v1c; /* 0x41EC40 mov ebp,[var_1C] */
        ebp -= esi; /* 0x41EC44 sub ebp,esi */
        esi = ebp & 0xFFFU; /* 0x41EC46 mov esi,ebp; 0x41EC48 and esi,0FFFh */
        esi <<= 2; /* 0x41EC4E shl esi,2 */
        v2c = esi; /* 0x41EC51 mov [var_2C],esi */
        esi = ecx - eax; /* 0x41EC55 mov esi,ecx; 0x41EC57 sub esi,eax */
        esi &= 0x7FU; /* 0x41EC59 and esi,7Fh */
        esi <<= 2; /* 0x41EC5C shl esi,2 */
        v28 = esi; /* 0x41EC5F mov [var_28],esi */
        esi = ecx & 0x7FU; /* 0x41EC63 mov esi,ecx; 0x41EC65 and esi,7Fh */
        esi = M32(0x451B3C + esi * 4); /* 0x41EC68 */
        v1c = esi; /* 0x41EC6F mov [var_1C],esi */
        esi = v28; /* 0x41EC73 mov esi,[var_28] */
        ebp = v1c; /* 0x41EC77 mov ebp,[var_1C] */
        esi = M32(0x451B3C + esi); /* 0x41EC7B mov esi,[451B3C+esi] */
        ebp -= esi; /* 0x41EC81 sub ebp,esi */
        esi = ebp & 0xFFFU; /* 0x41EC83 mov esi,ebp; 0x41EC85 and esi,0FFFh */
        esi = M32(0x44D920 + esi * 4); /* 0x41EC8B */
        v1c = esi; /* 0x41EC92 mov [var_1C],esi */
        esi = v2c; /* 0x41EC96 mov esi,[var_2C] */
        ebp = v1c; /* 0x41EC9A mov ebp,[var_1C] */
        esi = M32(0x44D920 + esi); /* 0x41EC9E mov esi,[44D920+esi] */
        ecx -= 2; /* 0x41ECA4 sub ecx,2 */
        ebp += esi; /* 0x41ECA7 add ebp,esi */
        esi = ecx - eax; /* 0x41ECA9 mov esi,ecx; 0x41ECAB sub esi,eax */
        ecx &= 0x7FU; /* 0x41ECAD and ecx,7Fh */
        esi &= 0x7FU; /* 0x41ECB0 and esi,7Fh */
        v1c = ebp; /* 0x41ECB3 mov [var_1C],ebp */
        ecx = M32(0x451B3C + ecx * 4); /* 0x41ECB7 */
        ecx -= M32(0x451B3C + esi * 4); /* 0x41ECBE sub ecx,[451B3C+esi*4] */
        ecx &= 0xFFFU; /* 0x41ECC5 and ecx,0FFFh */
        esi = v1c; /* 0x41ECCB mov esi,[var_1C] */
        ebp = M32(0x44D920 + ecx * 4); /* 0x41ECCF */
        ecx = esi + ebp; /* 0x41ECD6 lea ecx,[esi+ebp] */
        esi = (unsigned int)((unsigned long)M32(0x45193C + edx) * 13UL & 0xFFFFFFFFUL); /* 0x41ECD9 imul esi,[45193C+edx],0Dh (signed*13, wraps; computed unsigned for C89) */
        esi >>= 4; /* 0x41ECE0 shr esi,4 (logical) */
        esi += ecx; /* 0x41ECE3 add esi,ecx */
        M32(0x45193C + edx) = esi; /* 0x41ECE5 */
        ebp = M32(0x45193C + edx); /* 0x41ECEB */
        esi = G32(0x451924); /* 0x41ECF1 */
        if (ebp < (unsigned int)M32(0x45193C + esi * 4)) /* 0x41ECF7 cmp; 0x41ECFE jnb (unsigned) */
            G32(0x451924) = eax; /* 0x41ED00 */
        /* 0x41ED05 loc_41ED05 */
        if (ecx < edi) { /* 0x41ED05 cmp ecx,edi; 0x41ED07 jnb (unsigned) */
            edi = ecx; /* 0x41ED09 mov edi,ecx */
            G32(0x451928) = eax; /* 0x41ED0B */
        }
        /* 0x41ED10 loc_41ED10 */
        eax++; /* 0x41ED10 inc eax */
        edx += 4; /* 0x41ED11 add edx,4 */
        if (eax < 0x7CU) continue; /* 0x41ED14 cmp eax,7Ch; 0x41ED17 jb (unsigned) */
        break;
    }
e9f0_vtab: /* 0x41ED1D loc_41ED1D */
    eax = v20; /* 0x41ED1D mov eax,[var_20] */
    eax -= G32(0x451D3C); /* 0x41ED21 sub eax,[451D3C] */
    ecx = G32(0x451D40); /* 0x41ED27 */
    va[0] = eax; /* 0x41ED2D mov [var_44],eax */
    eax = v20; /* 0x41ED30 mov eax,[var_20] */
    eax -= ecx; /* 0x41ED34 sub eax,ecx */
    esi = G32(0x451D44); /* 0x41ED36 */
    va[1] = eax; /* 0x41ED3C mov [var_40],eax */
    eax = v20; /* 0x41ED40 mov eax,[var_20] */
    eax -= esi; /* 0x41ED44 sub eax,esi */
    edi = G32(0x451D48); /* 0x41ED46 */
    va[2] = eax; /* 0x41ED4C mov [var_3C],eax */
    eax = v20; /* 0x41ED50 mov eax,[var_20] */
    eax -= edi; /* 0x41ED54 sub eax,edi */
    ebp = G32(0x451D4C); /* 0x41ED56 */
    va[3] = eax; /* 0x41ED5C mov [var_38],eax */
    eax = v24; /* 0x41ED60 mov eax,[var_24] */
    edx = G32(0x451D50); /* 0x41ED64 */
    eax -= ebp; /* 0x41ED6A sub eax,ebp */
    ecx = 0; /* 0x41ED6C xor ecx,ecx */
    va[4] = eax; /* 0x41ED6E mov [var_34],eax */
    eax = v24; /* 0x41ED72 mov eax,[var_24] */
    G32(0x451920) = ecx; /* 0x41ED76 mov [451920],ecx (=0) */
    eax -= edx; /* 0x41ED7C sub eax,edx */
    esi = 0; /* 0x41ED7E xor esi,esi */
    va[5] = eax; /* 0x41ED80 mov [var_30],eax */
    edx = 0; /* 0x41ED84 xor edx,edx */
    eax = 0x451D54U; /* 0x41ED86 mov eax,offset unk_451D54 */
    for (;;) { /* 0x41ED8B loc_41ED8B: 6 rows, stride 0x404 */
        ecx = G32(0x45AF38) & 0xFFU; /* 0x41ED8B mov ecx,[45AF38]; 0x41ED91 and ecx,0FFh */
        ebp = M32(eax + 0x400); /* 0x41ED97 mov ebp,[eax+400h] */
        edi = M32(eax + ecx * 4); /* 0x41ED9D mov edi,[eax+ecx*4] */
        ebp -= edi; /* 0x41EDA0 sub ebp,edi */
        M32(eax + 0x400) = ebp; /* 0x41EDA2 mov [eax+400h],ebp */
        edi = va[esi >> 2]; /* 0x41EDA8 mov edi,[esp+esi+var_44] */
        edi &= 0x1FFU; /* 0x41EDAB and edi,1FFh */
        edi = M32(0x434550 + edi * 4); /* 0x41EDB1 mov edi,[434550+edi*4] */
        M32(eax + ecx * 4) = edi; /* 0x41EDB8 mov [eax+ecx*4],edi */
        ecx = edi; /* 0x41EDBB mov ecx,edi */
        M32(eax + 0x400) = M32(eax + 0x400) + ecx; /* 0x41EDBD add [eax+400h],ecx */
        edi = (unsigned int)((unsigned long)G32(0x451920) * 0x404UL & 0xFFFFFFFFUL); /* 0x41EDC3 imul edi,[451920],404h (wraps) */
        ecx = M32(eax + 0x400); /* 0x41EDCD mov ecx,[eax+400h] */
        if ((int)ecx >= (int)M32(0x452154 + edi)) goto e9f0_rownext; /* 0x41EDD3 cmp; 0x41EDD9 jge (signed) */
        G32(0x451920) = edx; /* 0x41EDDB mov [451920],edx */
e9f0_rownext: /* 0x41EDE1 loc_41EDE1 */
        esi += 4; /* 0x41EDE1 add esi,4 */
        edx++; /* 0x41EDE4 inc edx */
        eax += 0x404U; /* 0x41EDE5 add eax,404h */
        if ((int)edx < 6) continue; /* 0x41EDEA cmp edx,6; 0x41EDED jl (signed) */
        break;
    }
    eax = G32(0x45192C); /* 0x41EDEF */
    G32(0x451930) = eax; /* 0x41EDF4 */
    eax = v20; /* 0x41EDF9 mov eax,[var_20] */
    edx = M32(ebx + 0x70); /* 0x41EDFD mov edx,[ebx+70h] */
    G32(0x45192C) = eax; /* 0x41EE00 */
    eax = 0; /* 0x41EE05 xor eax,eax */
    edx <<= 8; /* 0x41EE07 shl edx,8 */
    eax = varg; /* 0x41EE0A mov al,[var_18] */
    ecx = M32(ebx + 0x74); /* 0x41EE0E mov ecx,[ebx+74h] */
    edx += eax; /* 0x41EE11 add edx,eax */
    eax = 0; /* 0x41EE13 xor eax,eax */
    M32(ebx + 0x70) = edx; /* 0x41EE15 mov [ebx+70h],edx */
    eax = varg; /* 0x41EE18 mov al,[var_18] */
    ecx <<= 8; /* 0x41EE1C shl ecx,8 */
    edx = M32(0x434D50 + eax * 4); /* 0x41EE1F mov edx,[434D50+eax*4] */
    ecx += edx; /* 0x41EE26 add ecx,edx */
    edx = M32(ebx + 0x78); /* 0x41EE28 mov edx,[ebx+78h] */
    M32(ebx + 0x74) = ecx; /* 0x41EE2B mov [ebx+74h],ecx */
    edx <<= 8; /* 0x41EE2E shl edx,8 */
    eax = M32(0x435550 + eax * 4); /* 0x41EE31 mov eax,[435550+eax*4] */
    edx += eax; /* 0x41EE38 add edx,eax */
    M32(ebx + 0x78) = edx; /* 0x41EE3A mov [ebx+78h],edx */
    return; /* 0x41EE3D loc_41EE3D: add esp,30h; pops; retn */
e9f0_hi: /* 0x41EF33 loc_41EF33 (order>=7) */
    eax = G32(0x45AF48); /* 0x41EF33 */
    eax <<= 2; /* 0x41EF38 shl eax,2 */
    edx = eax; /* 0x41EF3B mov edx,eax */
    eax <<= 5; /* 0x41EF3D shl eax,5 */
    eax -= edx; /* 0x41EF40 sub eax,edx (eax=AF48*124) */
    edx = 0x45356CU; /* 0x41EF42 mov edx,offset dword_45356C */
    ebx = G32(0x45AF48); /* 0x41EF47 */
    edx += eax; /* 0x41EF4D add edx,eax (per-entry base) */
    eax = (unsigned int)G8(0x45C67B); /* 0x41EF4F mov al,[45C67B] */
    M8(0x45375CU + ebx) = (unsigned char)eax; /* 0x41EF54 mov [45375C+ebx],al */
    if (G8(0x45C67D) < 8) { /* 0x41EF5A cmp byte,8; 0x41EF61 jb loc_41F1CB (unsigned) */
        eax = 0; /* 0x41F1CB xor eax,eax */
        eax = ecx & 0xFFU; /* 0x41F1CD mov al,cl */
        eax -= 0x80U; /* 0x41F1CF sub eax,80h */
        goto e9f0_updhi; /* 0x41F1D4 jmp loc_41EF6A */
    }
    eax = (unsigned int)(int)(signed char)(ecx & 0xFFU); /* 0x41EF67 movsx eax,cl */
e9f0_updhi: /* 0x41EF6A loc_41EF6A */
    v20 = eax; /* 0x41EF6A mov [var_20],eax */
    ebx = v20 & 0xFFU; /* 0x41EF6E mov bh,[var_20] (byte; kept in bh var) */
    eax = (unsigned int)G8(0x451D3C); /* 0x41EF72 mov al,[451D3C] (low byte of dword) */
    eax = (eax - (ebx & 0xFFU)) & 0xFFU; /* 0x41EF77 sub al,bh */
    eax = (unsigned int)(int)(signed char)(eax & 0xFFU); /* 0x41EF79 movsx eax,al */
    eax <<= 4; /* 0x41EF7C shl eax,4 */
    ecx = eax & 0xFFFU; /* 0x41EF7F mov ecx,eax; 0x41EF81 and ecx,0FFFh */
    ebx = M32(edx + 0x30); /* 0x41EF87 mov ebx,[edx+30h] */
    ecx = M32(0x44D920 + ecx * 4); /* 0x41EF8A mov ecx,[44D920+ecx*4] */
    esi = M32(edx); /* 0x41EF91 mov esi,[edx] */
    ebx += ecx; /* 0x41EF93 add ebx,ecx */
    ecx = (eax - esi) & 0xFFFU; /* 0x41EF95 mov ecx,eax; 0x41EF97 sub ecx,esi; 0x41EF9C and ecx,0FFFh */
    M32(edx + 0x30) = ebx; /* 0x41EF99 mov [edx+30h],ebx */
    edi = M32(edx + 0x34); /* 0x41EFA2 mov edi,[edx+34h] */
    edi += M32(0x44D920 + ecx * 4); /* 0x41EFA5 add edi,[44D920+ecx*4] */
    ecx = M32(edx); /* 0x41EFAC mov ecx,[edx] */
    ecx += eax; /* 0x41EFAE add ecx,eax */
    M32(edx + 0x34) = edi; /* 0x41EFB0 mov [edx+34h],edi */
    ecx &= 0xFFFU; /* 0x41EFB3 and ecx,0FFFh */
    ebp = M32(edx + 0x38); /* 0x41EFB9 mov ebp,[edx+38h] */
    ecx = M32(0x44D920 + ecx * 4); /* 0x41EFBC mov ecx,[44D920+ecx*4] */
    ebx = M32(edx + 4); /* 0x41EFC3 mov ebx,[edx+4] */
    ebp += ecx; /* 0x41EFC6 add ebp,ecx */
    ecx = (eax - ebx) & 0xFFFU; /* 0x41EFC8 mov ecx,eax; 0x41EFCA sub ecx,ebx; 0x41EFCF and ecx,0FFFh */
    M32(edx + 0x38) = ebp; /* 0x41EFCC mov [edx+38h],ebp */
    esi = M32(edx + 0x3C); /* 0x41EFD5 mov esi,[edx+3Ch] */
    esi += M32(0x44D920 + ecx * 4); /* 0x41EFD8 add esi,[44D920+ecx*4] */
    ecx = M32(edx + 4); /* 0x41EFDF mov ecx,[edx+4] */
    ecx += eax; /* 0x41EFE2 add ecx,eax */
    M32(edx + 0x3C) = esi; /* 0x41EFE4 mov [edx+3Ch],esi */
    ecx &= 0xFFFU; /* 0x41EFE7 and ecx,0FFFh */
    edi = M32(edx + 0x40); /* 0x41EFED mov edi,[edx+40h] */
    ecx = M32(0x44D920 + ecx * 4); /* 0x41EFF0 mov ecx,[44D920+ecx*4] */
    ebp = M32(edx + 8); /* 0x41EFF7 mov ebp,[edx+8] */
    edi += ecx; /* 0x41EFFA add edi,ecx */
    ecx = (eax - ebp) & 0xFFFU; /* 0x41EFFC mov ecx,eax; 0x41EFFE sub ecx,ebp; 0x41F003 and ecx,0FFFh */
    M32(edx + 0x40) = edi; /* 0x41F000 mov [edx+40h],edi */
    ebx = M32(edx + 0x44); /* 0x41F009 mov ebx,[edx+44h] */
    ebx += M32(0x44D920 + ecx * 4); /* 0x41F00C add ebx,[44D920+ecx*4] */
    ecx = M32(edx + 8); /* 0x41F013 mov ecx,[edx+8] */
    ecx += eax; /* 0x41F016 add ecx,eax */
    M32(edx + 0x44) = ebx; /* 0x41F018 mov [edx+44h],ebx */
    ecx &= 0xFFFU; /* 0x41F01B and ecx,0FFFh */
    esi = M32(edx + 0x48); /* 0x41F021 mov esi,[edx+48h] */
    ecx = M32(0x44D920 + ecx * 4); /* 0x41F024 mov ecx,[44D920+ecx*4] */
    edi = M32(edx + 0x0C); /* 0x41F02B mov edi,[edx+0Ch] */
    esi += ecx; /* 0x41F02E add esi,ecx */
    ecx = (eax - edi) & 0xFFFU; /* 0x41F030 mov ecx,eax; 0x41F032 sub ecx,edi; 0x41F037 and ecx,0FFFh */
    M32(edx + 0x48) = esi; /* 0x41F034 mov [edx+48h],esi */
    ebp = M32(edx + 0x4C); /* 0x41F03D mov ebp,[edx+4Ch] */
    ebp += M32(0x44D920 + ecx * 4); /* 0x41F040 add ebp,[44D920+ecx*4] */
    ecx = M32(edx + 0x0C); /* 0x41F047 mov ecx,[edx+0Ch] */
    ecx += eax; /* 0x41F04A add ecx,eax */
    M32(edx + 0x4C) = ebp; /* 0x41F04C mov [edx+4Ch],ebp */
    ecx &= 0xFFFU; /* 0x41F04F and ecx,0FFFh */
    ebx = M32(edx + 0x50); /* 0x41F055 mov ebx,[edx+50h] */
    ecx = M32(0x44D920 + ecx * 4); /* 0x41F058 mov ecx,[44D920+ecx*4] */
    ebx += ecx; /* 0x41F05F add ebx,ecx */
    M32(edx + 0x50) = ebx; /* 0x41F061 mov [edx+50h],ebx */
    ecx = eax; /* 0x41F064 mov ecx,eax */
    esi = G32(0x451934); /* 0x41F066 mov esi,[451934] */
    ecx = (ecx - esi) & 0xFFFU; /* 0x41F06C sub ecx,esi; 0x41F06E and ecx,0FFFh */
    edi = M32(edx + 0x54); /* 0x41F074 mov edi,[edx+54h] */
    edi += M32(0x44D920 + ecx * 4); /* 0x41F077 add edi,[44D920+ecx*4] */
    eax += esi; /* 0x41F07E add eax,esi */
    M32(edx + 0x54) = edi; /* 0x41F080 mov [edx+54h],edi */
    eax &= 0xFFFU; /* 0x41F083 and eax,0FFFh */
    ecx = M32(edx + 0x58); /* 0x41F088 mov ecx,[edx+58h] */
    eax = M32(0x44D920 + eax * 4); /* 0x41F08B mov eax,[44D920+eax*4] */
    ecx += eax; /* 0x41F092 add ecx,eax */
    M32(edx + 0x58) = ecx; /* 0x41F094 mov [edx+58h],ecx */
    ecx = v20 & 0xFFU; /* 0x41F097 mov cl,[var_20] */
    eax = (unsigned int)G8(0x451D3C); /* 0x41F09B mov al,[451D3C] (low byte of dword) */
    eax = (eax - (ecx & 0xFFU)) & 0xFFU; /* 0x41F0A0 sub al,cl */
    eax &= 0xFFU; /* 0x41F0A2 and eax,0FFh */
    ebx = M32(edx + 0x5C); /* 0x41F0A7 mov ebx,[edx+5Ch] */
    ebx += M32(0x435D50 + eax * 4); /* 0x41F0AA add ebx,[435D50+eax*4] */
    eax = (unsigned int)M8(edx + 0x28); /* 0x41F0B1 mov al,[edx+28h] */
    M32(edx + 0x5C) = ebx; /* 0x41F0B4 mov [edx+5Ch],ebx */
    eax = (eax - (ecx & 0xFFU)) & 0xFFU; /* 0x41F0B7 sub al,cl */
    eax &= 0xFFU; /* 0x41F0B9 and eax,0FFh */
    esi = M32(edx + 0x60); /* 0x41F0BE mov esi,[edx+60h] */
    esi += M32(0x435D50 + eax * 4); /* 0x41F0C1 add esi,[435D50+eax*4] */
    eax = ecx & 0xFFU; /* 0x41F0C8 mov al,cl */
    eax = (eax - ((unsigned int)M8(edx + 0x28) & 0xFFU)) & 0xFFU; /* 0x41F0CA sub al,[edx+28h] */
    M32(edx + 0x60) = esi; /* 0x41F0CD mov [edx+60h],esi */
    eax = (unsigned int)(int)(signed char)(eax & 0xFFU); /* 0x41F0D0 movsx eax,al */
    edi = M32(edx + 0x6C); /* 0x41F0D3 mov edi,[edx+6Ch] */
    M32(edx + 0x2C) = eax; /* 0x41F0D6 mov [edx+2Ch],eax */
    edi++; /* 0x41F0D9 inc edi */
    G32(0x451934) = eax; /* 0x41F0DA mov [451934],eax */
    M32(edx + 0x6C) = edi; /* 0x41F0DF mov [edx+6Ch],edi */
    eax = v20; /* 0x41F0E2 mov eax,[var_20] */
    ebx = (unsigned int)M8(edx + 0x6C); /* 0x41F0E6 mov bl,[edx+6Ch] */
    M32(edx + 0x28) = eax; /* 0x41F0E9 mov [edx+28h],eax (dword store) */
    if ((ebx & 0xFU) != 0) goto e9f0_hidone; /* 0x41F0EC test bl,0Fh; 0x41F0EF jnz loc_41F159 */
    ecx = 1; /* 0x41F0F1 mov ecx,1 */
    eax = edx + 4; /* 0x41F0F6 lea eax,[edx+4] */
    esi = 0; /* 0x41F0F9 xor esi,esi */
    ebx = M32(edx + 0x30); /* 0x41F0FB mov ebx,[edx+30h] */
    M32(edx + 0x30) = esi; /* 0x41F0FE mov [edx+30h],esi (=0) */
    for (;;) { /* 0x41F101 loc_41F101 */
        edi = M32(eax + 0x30); /* 0x41F101 mov edi,[eax+30h] */
        if (ebx > edi) { /* 0x41F104 cmp ebx,edi; 0x41F106 jbe (unsigned) */
            ebx = edi; /* 0x41F108 mov ebx,edi */
            esi = ecx; /* 0x41F10A mov esi,ecx */
        }
        /* 0x41F10C loc_41F10C */
        eax += 4; /* 0x41F10C add eax,4 */
        ecx++; /* 0x41F10F inc ecx */
        M32(eax + 0x2C) = 0; /* 0x41F110 mov dword [eax+2Ch],0 */
        if ((int)ecx < 0x0B) continue; /* 0x41F117 cmp ecx,0Bh; 0x41F11A jl (signed) */
        break;
    }
    esi--; /* 0x41F11C dec esi */
    if (esi <= 9) { /* 0x41F11D cmp esi,9; 0x41F120 ja (unsigned) */
        switch (esi) { /* 0x41F122 jmp off_41E9C4[esi*4] */
        case 0: /* 0x41F129 loc_41F129 */
            ebp = M32(edx + 0x14); /* 0x41F129 */
            if ((int)ebp <= (int)0xFFFFFFE0U) break; /* 0x41F12C cmp; 0x41F12F jle (signed) */
            eax = ebp - 1; /* 0x41F131 lea eax,[ebp-1] */
            M32(edx + 0x14) = eax; /* 0x41F134 */
            break;
        case 1: /* 0x41F1D9 loc_41F1D9 */
            esi = M32(edx + 0x14); /* 0x41F1D9 */
            if ((int)esi >= 0x20) break; /* 0x41F1DC cmp esi,20h; 0x41F1DF jge (signed) */
            edi = esi + 1; /* 0x41F1E5 lea edi,[esi+1] */
            M32(edx + 0x14) = edi; /* 0x41F1E8 */
            break;
        case 2: /* 0x41F1F0 loc_41F1F0 */
            ecx = M32(edx + 0x18); /* 0x41F1F0 */
            if ((int)ecx <= (int)0xFFFFFFE0U) break; /* 0x41F1F3 cmp; 0x41F1F6 jle (signed) */
            ebx = ecx - 1; /* 0x41F1FC lea ebx,[ecx-1] */
            M32(edx + 0x18) = ebx; /* 0x41F1FF */
            break;
        case 3: /* 0x41F207 loc_41F207 */
            ebp = M32(edx + 0x18); /* 0x41F207 */
            if ((int)ebp >= 0x20) break; /* 0x41F20A cmp; 0x41F20D jge (signed) */
            eax = ebp + 1; /* 0x41F213 lea eax,[ebp+1] */
            M32(edx + 0x18) = eax; /* 0x41F216 */
            break;
        case 4: /* 0x41F21E loc_41F21E */
            esi = M32(edx + 0x1C); /* 0x41F21E */
            if ((int)esi <= (int)0xFFFFFFE0U) break; /* 0x41F221 cmp; 0x41F224 jle (signed) */
            edi = esi - 1; /* 0x41F22A lea edi,[esi-1] */
            M32(edx + 0x1C) = edi; /* 0x41F22D */
            break;
        case 5: /* 0x41F235 loc_41F235 */
            ecx = M32(edx + 0x1C); /* 0x41F235 */
            if ((int)ecx >= 0x20) break; /* 0x41F238 cmp; 0x41F23B jge (signed) */
            ebx = ecx + 1; /* 0x41F241 lea ebx,[ecx+1] */
            M32(edx + 0x1C) = ebx; /* 0x41F244 */
            break;
        case 6: /* 0x41F24C loc_41F24C */
            ebp = M32(edx + 0x20); /* 0x41F24C */
            if ((int)ebp <= (int)0xFFFFFFE0U) break; /* 0x41F24F cmp; 0x41F252 jle (signed) */
            eax = ebp - 1; /* 0x41F258 lea eax,[ebp-1] */
            M32(edx + 0x20) = eax; /* 0x41F25B */
            break;
        case 7: /* 0x41F263 loc_41F263 */
            esi = M32(edx + 0x20); /* 0x41F263 */
            if ((int)esi >= 0x20) break; /* 0x41F266 cmp; 0x41F269 jge (signed) */
            edi = esi + 1; /* 0x41F26F lea edi,[esi+1] */
            M32(edx + 0x20) = edi; /* 0x41F272 */
            break;
        case 8: /* 0x41F27A loc_41F27A */
            if ((int)M32(edx + 0x20) <= (int)0xFFFFFFE0U) break; /* 0x41F27A cmp [edx+20h]; 0x41F27E jle (signed) */
            M32(edx + 0x24) = M32(edx + 0x24) - 1; /* 0x41F284 dec [edx+24h] */
            break;
        case 9: /* 0x41F28C loc_41F28C */
            if ((int)M32(edx + 0x20) >= 0x20) break; /* 0x41F28C cmp [edx+20h]; 0x41F290 jge (signed) */
            M32(edx + 0x24) = M32(edx + 0x24) + 1; /* 0x41F296 inc [edx+24h] */
            break;
        }
    }
    /* 0x41F137 loc_41F137 (switch default: zero-generation reset) */
    if (((unsigned int)M8(edx + 0x6C) & 0xFFU) != 0) goto e9f0_hidone; /* 0x41F137 test byte [edx+6Ch],0FFh; 0x41F13B jnz loc_41F159 */
    eax = M32(edx + 0x64); /* 0x41F13D mov eax,[edx+64h] */
    ecx = M32(edx + 0x5C); /* 0x41F140 mov ecx,[edx+5Ch] */
    ecx -= eax; /* 0x41F143 sub ecx,eax */
    M32(edx + 0x5C) = ecx; /* 0x41F145 mov [edx+5Ch],ecx */
    ebx = M32(edx + 0x60); /* 0x41F148 mov ebx,[edx+60h] */
    eax = M32(edx + 0x68); /* 0x41F14B mov eax,[edx+68h] */
    M32(edx + 0x64) = ecx; /* 0x41F14E mov [edx+64h],ecx */
    ebx -= eax; /* 0x41F151 sub ebx,eax */
    M32(edx + 0x60) = ebx; /* 0x41F153 mov [edx+60h],ebx */
    M32(edx + 0x68) = ebx; /* 0x41F156 mov [edx+68h],ebx */
e9f0_hidone: /* 0x41F159 loc_41F159 */
    if (G8(0x45C67D) != 9) goto e9f0_histore; /* 0x41F159 cmp byte,9; 0x41F160 jnz loc_41F178 */
    eax = G32(0x451938); /* 0x41F162 */
    ecx = G32(0x451934); /* 0x41F167 */
    G32(0x451934) = eax; /* 0x41F16D */
    G32(0x451938) = ecx; /* 0x41F172 */
e9f0_histore: /* 0x41F178 loc_41F178 */
    ecx = 0; /* 0x41F178 xor ecx,ecx */
    eax = M32(edx + 0x70); /* 0x41F17A mov eax,[edx+70h] */
    ecx = varg; /* 0x41F17D mov cl,[var_18] */
    eax <<= 8; /* 0x41F181 shl eax,8 */
    eax += ecx; /* 0x41F184 add eax,ecx */
    ecx = (unsigned int)G8(0x45C67D); /* 0x41F186 mov cl,[45C67D] */
    M32(edx + 0x70) = eax; /* 0x41F18C mov [edx+70h],eax */
    if ((ecx & 0xFFU) != 7) return; /* 0x41F18F cmp cl,7; 0x41F192 jnz loc_41EE3D */
    eax = 0; /* 0x41F198 xor eax,eax */
    ebx = M32(edx + 0x74); /* 0x41F19A mov ebx,[edx+74h] */
    eax = varg; /* 0x41F19D mov al,[var_18] */
    ebx <<= 8; /* 0x41F1A1 shl ebx,8 */
    ecx = M32(0x435150 + eax * 4); /* 0x41F1A4 mov ecx,[435150+eax*4] */
    ebx += ecx; /* 0x41F1AB add ebx,ecx */
    ecx = M32(edx + 0x78); /* 0x41F1AD mov ecx,[edx+78h] */
    M32(edx + 0x74) = ebx; /* 0x41F1B0 mov [edx+74h],ebx */
    ecx <<= 8; /* 0x41F1B3 shl ecx,8 */
    eax = M32(0x435950 + eax * 4); /* 0x41F1B6 mov eax,[435950+eax*4] */
    ecx += eax; /* 0x41F1BD add ecx,eax */
    M32(edx + 0x78) = ecx; /* 0x41F1BF mov [edx+78h],ecx */
    return; /* 0x41F1C2: add esp,30h; pops; retn */
}

/* MORE */

/* ---- sub_41D480 (UHARC04.lst:42448-42514) -> d4v_41d480 ----
 * Rescale the model pair: halve 32 dwords at 433B80[a<<9] (packed
 * 16-bit lanes: +0x10001, mask 0xFFFEFFFE, shr 1, cross-sum into ebx),
 * then bump the 433B8C[a] word (shr 2, inc) and publish
 * 433B88[a] = (ebx + (word&0xFFFF)) low word. Entry-live: eax only. */
void d4v_41d480(dec4v_ctx *c, unsigned int a)
{
    unsigned int ebp, ecx, eax, ebx, edi, esi, edx;
    ebp = a & 0xFFFFFFFFUL; /* 0x41D486 mov ebp,eax */
    ecx = a & 0xFFFFFFFFUL; /* 0x41D488 mov ecx,eax */
    eax = G32(0x433B80); /* 0x41D48A mov eax,ds:dword_433B80 */
    ecx = (ecx << 9) & 0xFFFFFFFFUL; /* 0x41D48F shl ecx,9 */
    ecx = (ecx + eax) & 0xFFFFFFFFUL; /* 0x41D492 add ecx,eax */
    ebx = 0; /* 0x41D494 xor ebx,ebx */
    edi = (ecx + 0x200U) & 0xFFFFFFFFUL; /* 0x41D496 lea edi,[ecx+200h] */
    for (;;) { /* 0x41D49C loc_41D49C */
        eax = M32(ecx); /* 0x41D49C mov eax,[ecx] */
        edx = M32(ecx + 4); /* 0x41D49E mov edx,[ecx+4] */
        eax = (eax + 0x10001U) & 0xFFFFFFFFUL; /* 0x41D4A1 */
        edx = (edx + 0x10001U) & 0xFFFFFFFFUL; /* 0x41D4A6 */
        eax &= 0xFFFEFFFEUL; /* 0x41D4AC */
        edx &= 0xFFFEFFFEUL; /* 0x41D4B1 */
        eax >>= 1; /* 0x41D4B7 */
        edx >>= 1; /* 0x41D4B9 */
        ecx = (ecx + 0x10U) & 0xFFFFFFFFUL; /* 0x41D4BB */
        esi = (eax + edx) & 0xFFFFFFFFUL; /* 0x41D4BE lea esi,[eax+edx] */
        M32(ecx - 0x0CU) = edx; /* 0x41D4C1 */
        M32(ecx - 0x10U) = eax; /* 0x41D4C4 */
        edx = M32(ecx - 4); /* 0x41D4C7 */
        eax = M32(ecx - 8); /* 0x41D4CA */
        edx = (edx + 0x10001U) & 0xFFFFFFFFUL; /* 0x41D4CD */
        eax = (eax + 0x10001U) & 0xFFFFFFFFUL; /* 0x41D4D3 */
        edx &= 0xFFFEFFFEUL; /* 0x41D4D8 */
        eax &= 0xFFFEFFFEUL; /* 0x41D4DE */
        edx >>= 1; /* 0x41D4E3 */
        eax >>= 1; /* 0x41D4E5 */
        ebx = (ebx + esi) & 0xFFFFFFFFUL; /* 0x41D4E7 */
        esi = (eax + edx) & 0xFFFFFFFFUL; /* 0x41D4E9 */
        M32(ecx - 4) = edx; /* 0x41D4EC */
        ebx = (ebx + esi) & 0xFFFFFFFFUL; /* 0x41D4EF */
        M32(ecx - 8) = eax; /* 0x41D4F1 */
        if (ecx == edi) break; /* 0x41D4F4 cmp; 0x41D4F6 jnz */
    }
    eax = G32(0x433B8C); /* 0x41D4F8 */
    edx = (edx & 0xFFFF0000UL) | M16(eax + ebp * 2); /* 0x41D4FD mov dx,[eax+ebp*2] */
    edx = (edx & 0xFFFF0000UL) | ((edx & 0xFFFFUL) >> 2); /* 0x41D501 shr dx,2 */
    ecx = edx; /* 0x41D505 mov ecx,edx */
    M16(eax + ebp * 2) = (unsigned short)(edx & 0xFFFFUL); /* 0x41D507 */
    ecx = (ecx + 1) & 0xFFFFFFFFUL; /* 0x41D50B inc ecx */
    M16(eax + ebp * 2) = (unsigned short)(ecx & 0xFFFFUL); /* 0x41D50C */
    eax = ebx & 0xFFFFFFFFUL; /* 0x41D510 mov eax,ebx */
    edx = 0; /* 0x41D512 xor edx,edx */
    eax &= 0xFFFFUL; /* 0x41D514 and eax,0FFFFh */
    ebx >>= 16; /* 0x41D519 shr ebx,10h */
    edx = (edx & 0xFFFF0000UL) | (ecx & 0xFFFFUL); /* 0x41D51C mov dx,cx */
    ebx = (ebx + eax) & 0xFFFFFFFFUL; /* 0x41D51F add ebx,eax */
    eax = (ebx + edx) & 0xFFFFFFFFUL; /* 0x41D521 lea eax,[ebx+edx] */
    edx = G32(0x433B88); /* 0x41D524 */
    M16(edx + ebp * 2) = (unsigned short)(eax & 0xFFFFUL); /* 0x41D52A */
}

/* ---- workers: S4a cluster (from draft_s4a.c) ---- */
unsigned int d4v_41d540(dec4v_ctx *c, unsigned int a, unsigned int d,
    unsigned int e, unsigned int b)
{
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned int v1c = 0, v20 = 0, v24 = 0, v28 = 0, v2c = 0, v30 = 0, v34 = 0, v38 = 0, v3c = 0, v40 = 0;
    unsigned int v44 = 0, v48 = 0, v4c = 0, v50 = 0, v54 = 0, v58 = 0, v5c = 0;
    unsigned char v10 = 0, v14 = 0, v18 = 0;
    unsigned char alb = 0, ahb = 0, clb = 0, chb = 0, dlb = 0, dhb = 0;
    /* prologue: 0x41D540 push esi; 0x41D541 push edi; 0x41D542 push ebp;
     * 0x41D543 sub esp,50h */
    ebp = G32(0x45AF54); /* 0x41D546 */
    v30 = a; /* 0x41D54C mov [var_30],eax */
    v58 = d; /* 0x41D550 mov [var_58],edx */
    v40 = b; /* 0x41D554 mov [var_40],ebx */
    v14 = (unsigned char)(e & 0xFFU); /* 0x41D558 mov [var_14],cl */
    edx = 0; /* 0x41D55C xor edx,edx */
    clb = v14; /* entry cl */
    edx = clb; /* 0x41D55E mov dl,cl (edx was zero) */
    ecx = G32(0x4342C4 + edx * 4U); /* 0x41D560 */
    eax = G32(0x4342EC + edx * 4U); /* 0x41D567 */
    v24 = ecx; /* 0x41D56E mov [var_24],ecx */
    v50 = eax; /* 0x41D572 mov [var_50],eax */
    edx <<= 8; /* 0x41D576 shl edx,8 */
    eax = G32(0x45BA34); /* 0x41D579 */
    ecx = 0; /* 0x41D57E xor ecx,ecx */
    edx += 0x30200U; /* 0x41D580 add edx,30200h */
    v44 = ecx; /* 0x41D586 mov [var_44],ecx (=0) */
    v20 = eax; /* 0x41D58A mov [var_20],eax */
    alb = v14; /* 0x41D58E mov al,[var_14] */
    v28 = ecx; /* 0x41D592 mov [var_28],ecx (=0) */
    ahb = G8(0x45C67D); /* 0x41D596 mov ah,byte_45C67D */
    v54 = edx; /* 0x41D59C mov [var_54],edx */
    if (alb != ahb) goto s540_6a2; /* 0x41D5A0 cmp al,ah; 0x41D5A2 jnz */
    esi = v24; /* 0x41D5A8 mov esi,[var_24] */
    ebx = G32(0x45AF3C); /* 0x41D5AC */
    ebx -= esi; /* 0x41D5B2 sub ebx,esi */
    goto s540_5b4;
s540_6a2: /* 0x41D6A2 loc_41D6A2 */
    ebx = 0; /* 0x41D6A2 xor ebx,ebx */
    goto s540_5b4; /* 0x41D6A4 jmp */
s540_5b4: /* 0x41D5B4 loc_41D5B4 */
    eax = G32(0x45AF38); /* 0x41D5B4 */
    v2c = eax; /* 0x41D5B9 mov [var_2C],eax */
    alb = G8(0x45C67B); /* 0x41D5BD mov al,byte_45C67B */
    v18 = alb; /* 0x41D5C2 mov [var_18],al */
    eax = 0; /* 0x41D5C6 xor eax,eax */
    eax = v14; /* 0x41D5C8 mov al,[var_14] (eax was zero) */
    dlb = v14; /* 0x41D5CC mov dl,[var_14] (edx high half stale, dead) */
    alb = M8(0x434314 + eax); /* 0x41D5D0 mov al,byte_434314[eax] */
    G32(0x45AF54) = ebp; /* 0x41D5D6 */
    v10 = alb; /* 0x41D5DC mov [var_10],al */
    if (dlb == 0) goto s540_6a9; /* 0x41D5E0 test dl,dl; 0x41D5E2 jz */
    goto s540_5e8;
s540_5e8: /* 0x41D5E8 loc_41D5E8 (table setup copy + interleave) */
    edx = v50; /* 0x41D5E8 mov edx,[var_50] */
    eax = v24; /* 0x41D5EC mov eax,[var_24] */
    edi = G32(0x433B80); /* 0x41D5F0 */
    esi = G32(0x433B7C); /* 0x41D5F6 */
    edx <<= 9; /* 0x41D5FC shl edx,9 */
    eax <<= 9; /* 0x41D5FF shl eax,9 */
    ecx = edx; /* 0x41D602 mov ecx,edx */
    esi += eax; /* 0x41D604 add esi,eax */
    v48 = eax; /* 0x41D606 mov [var_48],eax */
    /* 0x41D60A cld (forward) */
    eax = ecx; /* 0x41D60B mov eax,ecx */
    ecx -= edi; /* 0x41D60D sub ecx,edi */
    ecx -= eax; /* 0x41D60F sub ecx,eax */
    ecx &= 3U; /* 0x41D611 and ecx,3 */
    eax -= ecx; /* 0x41D614 sub eax,ecx */
    if ((int)eax > 0) { /* 0x41D616 jle (signed): skip head+dwords */
        while (ecx) { M8(edi) = M8(esi); esi++; edi++; ecx--; } /* 0x41D618 rep movsb */
        ecx = eax; /* 0x41D61A mov ecx,eax */
        eax &= 3U; /* 0x41D61C and eax,3 */
        ecx >>= 2; /* 0x41D61F shr ecx,2 */
        while (ecx) { M32(edi) = M32(esi); esi += 4; edi += 4; ecx--; } /* 0x41D622 rep movsd */
    }
s540_624: /* 0x41D624 loc_41D624 */
    ecx += eax; /* 0x41D624 add ecx,eax */
    while (ecx) { M8(edi) = M8(esi); esi++; edi++; ecx--; } /* 0x41D626 rep movsb */
    edx += v48; /* 0x41D628 add edx,[var_48] */
    ecx = v50; /* 0x41D62C mov ecx,[var_50] */
    esi = v24; /* 0x41D630 mov esi,[var_24] */
    ecx += ecx; /* 0x41D634 add ecx,ecx */
    esi += esi; /* 0x41D636 add esi,esi */
    eax = ecx; /* 0x41D638 mov eax,ecx */
    esi += ecx; /* 0x41D63A add esi,ecx */
s540_63c: /* 0x41D63C loc_41D63C */
    edi = v50; /* 0x41D63C mov edi,[var_50] */
    ebp = G32(0x45AF54); /* 0x41D640 */
    edx -= 0x200U; /* 0x41D646 sub edx,200h */
    eax -= 2U; /* 0x41D64C sub eax,2 */
    edi--; /* 0x41D64F dec edi */
    esi -= 2U; /* 0x41D650 sub esi,2 */
    v50 = edi; /* 0x41D653 mov [var_50],edi */
    if (eax == 0xFFFFFFFEUL) goto s540_731; /* 0x41D657 cmp eax,0FFFFFFFEh; 0x41D65A jz */
    edi = G32(0x433B8C); /* 0x41D660 */
    ecx = G32(0x433B84); /* 0x41D666 */
    edi += eax; /* 0x41D66C add edi,eax */
    ecx = (ecx & 0xFFFF0000U) | M16(ecx + esi); /* 0x41D66E mov cx,[ecx+esi] */
    M16(edi) = (unsigned short)(ecx & 0xFFFFU); /* 0x41D672 mov [edi],cx */
    ecx &= 0xFFFFU; /* 0x41D675 and ecx,0FFFFh */
    v48 = ecx; /* 0x41D67B mov [var_48],ecx */
    ecx = G32(0x433B78); /* 0x41D67F */
    ecx = (ecx & 0xFFFF0000U) | M16(ecx + edx); /* 0x41D685 mov cx,[ecx+edx] */
    edi = v48; /* 0x41D689 mov edi,[var_48] */
    ecx &= 0xFFFFU; /* 0x41D68D and ecx,0FFFFh */
    ecx += edi; /* 0x41D693 add ecx,edi */
    edi = G32(0x433B88); /* 0x41D695 */
    edi += eax; /* 0x41D69B add edi,eax */
    M16(edi) = (unsigned short)(ecx & 0xFFFFU); /* 0x41D69D mov [edi],cx */
    goto s540_63c; /* 0x41D6A0 jmp */
s540_6a9: /* 0x41D6A9 loc_41D6A9 (order-0 reset: copy static tables) */
    ecx = 0x400U; /* 0x41D6A9 mov ecx,400h */
    edi = 0x4597A0U; /* 0x41D6AE mov edi,offset dword_4597A0 */
    esi = 0x4593A0U; /* 0x41D6B3 mov esi,offset dword_4593A0 */
    /* 0x41D6B8 cld (forward) */
    eax = ecx; /* 0x41D6B9 mov eax,ecx */
    ecx -= edi; /* 0x41D6BB sub ecx,edi */
    ecx -= eax; /* 0x41D6BD sub ecx,eax */
    ecx &= 3U; /* 0x41D6BF and ecx,3 */
    eax -= ecx; /* 0x41D6C2 sub eax,ecx */
    if ((int)eax > 0) { /* 0x41D6C4 jle (signed) */
        while (ecx) { M8(edi) = M8(esi); esi++; edi++; ecx--; } /* 0x41D6C6 rep movsb */
        ecx = eax; /* 0x41D6C8 mov ecx,eax */
        eax &= 3U; /* 0x41D6CA and eax,3 */
        ecx >>= 2; /* 0x41D6CD shr ecx,2 */
        while (ecx) { M32(edi) = M32(esi); esi += 4; edi += 4; ecx--; } /* 0x41D6D0 rep movsd */
    }
s540_6d2: /* 0x41D6D2 loc_41D6D2 */
    ecx += eax; /* 0x41D6D2 add ecx,eax */
    while (ecx) { M8(edi) = M8(esi); esi++; edi++; ecx--; } /* 0x41D6D4 rep movsb */
    ecx = 0x400U; /* 0x41D6D6 mov ecx,400h */
    edi = 0x458B98U; /* 0x41D6DB mov edi,offset dword_458B98 */
    esi = 0x458798U; /* 0x41D6E0 mov esi,offset dword_458798 */
    /* 0x41D6E5 cld (forward) */
    eax = ecx; /* 0x41D6E6 mov eax,ecx */
    ecx -= edi; /* 0x41D6E8 sub ecx,edi */
    ecx -= eax; /* 0x41D6EA sub ecx,eax */
    ecx &= 3U; /* 0x41D6EC and ecx,3 */
    eax -= ecx; /* 0x41D6EF sub eax,ecx */
    if ((int)eax > 0) { /* 0x41D6F1 jle (signed) */
        while (ecx) { M8(edi) = M8(esi); esi++; edi++; ecx--; } /* 0x41D6F3 rep movsb */
        ecx = eax; /* 0x41D6F5 mov ecx,eax */
        eax &= 3U; /* 0x41D6F7 and eax,3 */
        ecx >>= 2; /* 0x41D6FA shr ecx,2 */
        while (ecx) { M32(edi) = M32(esi); esi += 4; edi += 4; ecx--; } /* 0x41D6FD rep movsd */
    }
s540_6ff: /* 0x41D6FF loc_41D6FF */
    ecx += eax; /* 0x41D6FF add ecx,eax */
    while (ecx) { M8(edi) = M8(esi); esi++; edi++; ecx--; } /* 0x41D701 rep movsb */
    edi = 0x201U; /* 0x41D703 mov edi,201h */
    ebp = G32(0x45AF90); /* 0x41D708 */
    eax = G32(0x45AF98); /* 0x41D70E */
    G32(0x45AF54) = ebp; /* 0x41D713 */
    G32(0x45AF5C) = eax; /* 0x41D719 */
    eax = G32(0x45AF94); /* 0x41D71E */
    v24 = edi; /* 0x41D723 mov [var_24],edi */
    G32(0x45AF58) = eax; /* 0x41D727 */
    goto s540_5e8; /* 0x41D72C jmp */
s540_731: /* 0x41D731 loc_41D731 (main loop setup) */
    eax = G32(0x45BA18); /* 0x41D731 */
    v24 = eax; /* 0x41D736 mov [var_24],eax */
    eax = v54; /* 0x41D73A mov eax,[var_54] */
    eax += eax; /* 0x41D73E add eax,eax */
    edx = v30; /* 0x41D740 mov edx,[var_30] */
    v5c = eax; /* 0x41D744 mov [var_5C],eax */
    eax = v44; /* 0x41D747 mov eax,[var_44] */
    eax += edx; /* 0x41D74B add eax,edx */
    v34 = eax; /* 0x41D74D mov [var_34],eax */
s540_751: /* 0x41D751 loc_41D751 */
    if (v44 >= 0x100U) goto s540_94a; /* 0x41D751 cmp [var_44],100h; 0x41D759 jnb (unsigned) */
    edx = v20; /* 0x41D75F mov edx,[var_20] */
    ecx = G32(0x433B40); /* 0x41D763 */
    ecx += edx; /* 0x41D769 add ecx,edx */
    chb = M8(ecx); /* 0x41D76B mov ch,[ecx] (other ecx bytes dead until 0x41D911 reload) */
s540_76d: /* 0x41D76D loc_41D76D */
    if (v14 == 0) goto s540_964; /* 0x41D76D cmp [var_14],0; 0x41D772 jz */
    clb = v10; /* 0x41D778 mov cl,[var_10] */
    eax = v24; /* 0x41D77C mov eax,[var_24] */
    eax >>= (clb & 31U); /* 0x41D780 shr eax,cl (count mod 32) */
    if (v18 == 0) goto s540_95b; /* 0x41D782 cmp [var_18],0; 0x41D787 jz */
    clb = (unsigned char)(eax & 0xFFU); /* 0x41D78D mov cl,al */
    clb -= chb; /* 0x41D78F sub cl,ch */
s540_791: /* 0x41D791 loc_41D791 */
    edx = 0; /* 0x41D791 xor edx,edx */
    eax = ebx; /* 0x41D793 mov eax,ebx */
    edx = clb; /* 0x41D795 mov dl,cl (edx was zero) */
    eax <<= 8; /* 0x41D797 shl eax,8 */
    v3c = edx; /* 0x41D79A mov [var_3C],edx */
    eax += edx; /* 0x41D79E add eax,edx */
    edx = ebx + ebx; /* 0x41D7A0 lea edx,[ebx+ebx] */
    esi = G32(0x433B80); /* 0x41D7A3 */
    v38 = edx; /* 0x41D7A9 mov [var_38],edx */
    eax += eax; /* 0x41D7AD add eax,eax */
    edi = v38; /* 0x41D7AF mov edi,[var_38] */
    edx = G32(0x433B88); /* 0x41D7B3 */
    esi += eax; /* 0x41D7B9 add esi,eax */
    edi += edx; /* 0x41D7BB add edi,edx */
    eax = (eax & 0xFFFF0000U) | M16(esi); /* 0x41D7BD mov ax,[esi] (stale high half: cleared 0x41D840 or dead) */
    edx = (edx & 0xFFFF0000U) | M16(edi); /* 0x41D7C0 mov dx,[edi] */
    v4c = edi; /* 0x41D7C3 mov [var_4C],edi */
    v1c = edx; /* 0x41D7C7 mov [var_1C],edx (high half dead: only low word read at 0x41D849) */
    if ((eax & 0xFFFFU) != 0) goto s540_840; /* 0x41D7CB test ax,ax; 0x41D7CE jnz */
    eax = v54; /* 0x41D7D0 mov eax,[var_54] */
    eax += v3c; /* 0x41D7D4 add eax,[var_3C] */
    edx = G32(0x433B7C); /* 0x41D7D8 */
    eax = (eax & 0xFFFF0000U) | M16(edx + eax * 2U); /* 0x41D7DE mov ax,[edx+eax*2] */
    edi = v5c; /* 0x41D7E2 mov edi,[var_5C] */
    edx = G32(0x433B78); /* 0x41D7E5 */
    eax &= 0xFFFFU; /* 0x41D7EB and eax,0FFFFh */
    edx += edi; /* 0x41D7F0 add edx,edi */
    eax <<= 14; /* 0x41D7F2 shl eax,0Eh */
    edx = (edx & 0xFFFF0000U) | M16(edx); /* 0x41D7F5 mov dx,[edx] */
    edx &= 0xFFFFU; /* 0x41D7F8 and edx,0FFFFh */
    v48 = edx; /* 0x41D7FE mov [var_48],edx */
    edi = v48; /* 0x41D802 mov edi,[var_48] */
    edx = 0; /* 0x41D806 xor edx,edx */
    eax = eax / edi; /* 0x41D808 div edi (edx:eax/edi; hw #DE on div-by-zero/overflow, kept direct) */
    eax = (eax & 0xFFFF0000U) | M16(0x437968 + eax * 2U); /* 0x41D80A mov ax,word_437968[eax*2] */
    edx = v28; /* 0x41D812 mov edx,[var_28] */
    eax &= 0xFFFFU; /* 0x41D816 and eax,0FFFFh */
    edx += eax; /* 0x41D81B add edx,eax */
    edi = v38; /* 0x41D81D mov edi,[var_38] */
    v28 = edx; /* 0x41D821 mov [var_28],edx */
    edx = G32(0x433B8C); /* 0x41D825 */
    edx += edi; /* 0x41D82B add edx,edi */
    eax = (eax & 0xFFFF0000U) | M16(edx); /* 0x41D82D mov ax,[edx] */
    edi = eax; /* 0x41D830 mov edi,eax (full; high stale, dead) */
    edi += 0x20U; /* 0x41D832 add edi,20h */
    M16(edx) = (unsigned short)(edi & 0xFFFFU); /* 0x41D835 mov [edx],di */
    edx = v4c; /* 0x41D838 mov edx,[var_4C] */
    M16(edx) = (unsigned short)(M16(edx) + 0x20U); /* 0x41D83C add word ptr [edx],20h */
s540_840: /* 0x41D840 loc_41D840 */
    eax &= 0xFFFFU; /* 0x41D840 and eax,0FFFFh */
    edi = 0; /* 0x41D845 xor edi,edi */
    edx = 0; /* 0x41D847 xor edx,edx */
    edi = v1c & 0xFFFFU; /* 0x41D849 mov di,word ptr [var_1C] (edi was zero) */
    eax <<= 14; /* 0x41D84E shl eax,0Eh */
    eax = eax / edi; /* 0x41D851 div edi (hw #DE on div-by-zero/overflow, kept direct) */
    eax = (eax & 0xFFFF0000U) | M16(0x437968 + eax * 2U); /* 0x41D853 mov ax,word_437968[eax*2] */
    edx = v28; /* 0x41D85B mov edx,[var_28] */
    eax &= 0xFFFFU; /* 0x41D85F and eax,0FFFFh */
    edx += eax; /* 0x41D864 add edx,eax */
    edi = v40; /* 0x41D866 mov edi,[var_40] */
    v28 = edx; /* 0x41D86A mov [var_28],edx */
    if (edx > edi) goto s540_939; /* 0x41D86E cmp edx,edi; 0x41D870 ja (unsigned) */
    eax = (eax & 0xFFFF0000U) | M16(esi); /* 0x41D876 mov ax,[esi] (eax high is 0 from 0x41D840) */
    eax += 0x20U; /* 0x41D879 add eax,20h */
    edx = G32(0x433B88); /* 0x41D87E */
    M16(esi) = (unsigned short)(eax & 0xFFFFU); /* 0x41D884 mov [esi],ax */
    esi = (esi & 0xFFFF0000U) | M16(edx + ebx * 2U); /* 0x41D887 mov si,[edx+ebx*2] (high half dead: only si stored/compared) */
    esi += 0x20U; /* 0x41D88B add esi,20h */
    G32(0x45AF54) = ebp; /* 0x41D88E */
    M16(edx + ebx * 2U) = (unsigned short)(esi & 0xFFFFU); /* 0x41D894 mov [edx+ebx*2],si */
    if ((esi & 0xFFFFU) < 0x4000U) goto s540_8a6; /* 0x41D898 cmp si,4000h; 0x41D89D jb (unsigned) */
    eax = ebx; /* 0x41D89F mov eax,ebx */
    d4v_41d480(c, eax); /* 0x41D8A1 call sub_41D480 */
s540_8a6: /* 0x41D8A6 loc_41D8A6 */
    dlb = v14; /* 0x41D8A6 mov dl,[var_14] (edx high half stale, dead) */
    ebp = G32(0x45AF54); /* 0x41D8AA */
    if (dlb == 0) goto s540_a6b; /* 0x41D8B0 test dl,dl; 0x41D8B2 jz */
    if (clb < 0x80U) goto s540_8ca; /* 0x41D8B8 cmp cl,80h; 0x41D8BB jb (unsigned) */
    dhb = v18; /* 0x41D8BD mov dh,[var_18] */
    alb = (dhb == 0) ? 1 : 0; /* 0x41D8C1 test dh,dh; 0x41D8C3 setz al */
    v18 = alb; /* 0x41D8C6 mov [var_18],al */
s540_8ca: /* 0x41D8CA loc_41D8CA */
    alb = v14; /* 0x41D8CA mov al,[var_14] */
    if (alb != 1) goto s540_a0a; /* 0x41D8CE cmp al,1; 0x41D8D0 jnz */
    eax = ebx & 0xFU; /* 0x41D8D6 mov eax,ebx; 0x41D8D8 and eax,0Fh */
    alb = M8(0x434328 + eax); /* 0x41D8DB mov al,byte_434328[eax] */
    ebx = eax; /* 0x41D8E1 mov ebx,eax */
    eax = 0; /* 0x41D8E3 xor eax,eax */
    ebx &= 0xFFU; /* 0x41D8E5 and ebx,0FFh */
    eax = clb; /* 0x41D8EB mov al,cl (eax was zero) */
    ebx <<= 4; /* 0x41D8ED shl ebx,4 */
    alb = M8(0x434338 + eax); /* 0x41D8F0 mov al,byte_434338[eax] */
    eax = alb; /* 0x41D8F6 and eax,0FFh (eax high is 0) */
s540_8fb: /* 0x41D8FB loc_41D8FB */
    ebx += eax; /* 0x41D8FB add ebx,eax */
s540_8fd: /* 0x41D8FD loc_41D8FD */
    v2c++; /* 0x41D8FD inc [var_2C] */
s540_901: /* 0x41D901 loc_41D901 (emit/history step) */
    edx = v24; /* 0x41D901 mov edx,[var_24] */
    edi = v20; /* 0x41D905 mov edi,[var_20] */
    eax = 0; /* 0x41D909 xor eax,eax */
    edx <<= 8; /* 0x41D90B shl edx,8 */
    eax = chb; /* 0x41D90E mov al,ch (eax was zero) */
    edi++; /* 0x41D910 inc edi */
    ecx = v58; /* 0x41D911 mov ecx,[var_58] */
    v20 = edi; /* 0x41D915 mov [var_20],edi */
    edx += eax; /* 0x41D919 add edx,eax */
    eax = v34; /* 0x41D91B mov eax,[var_34] */
    v24 = edx; /* 0x41D91F mov [var_24],edx */
    edx = v44; /* 0x41D923 mov edx,[var_44] */
    eax++; /* 0x41D927 inc eax */
    edx++; /* 0x41D928 inc edx */
    v34 = eax; /* 0x41D929 mov [var_34],eax */
    v44 = edx; /* 0x41D92D mov [var_44],edx */
    if (edx < ecx) goto s540_751; /* 0x41D931 cmp edx,ecx; 0x41D933 jb (unsigned) */
    /* fall through to s540_939 */
s540_939: /* 0x41D939 loc_41D939 (return accumulated price) */
    eax = v28; /* 0x41D939 mov eax,[var_28] */
    G32(0x45AF54) = ebp; /* 0x41D93D */
    /* epilogue: 0x41D943 add esp,50h; 0x41D946 pop ebp; 0x41D947 pop edi;
     * 0x41D948 pop esi */
    return eax; /* 0x41D949 retn */
s540_94a: /* 0x41D94A loc_41D94A (history byte once var_44 >= 0x100) */
    ecx = v34; /* 0x41D94A mov ecx,[var_34] */
    chb = M8(ecx - 0x100U); /* 0x41D94E mov ch,[ecx-100h] (other ecx bytes dead until 0x41D911 reload) */
    chb = (unsigned char)~chb; /* 0x41D954 not ch */
    goto s540_76d; /* 0x41D956 jmp */
s540_95b: /* 0x41D95B loc_41D95B (order!=0, flag clear) */
    clb = chb; /* 0x41D95B mov cl,ch */
    clb -= (unsigned char)(eax & 0xFFU); /* 0x41D95D sub cl,al */
    goto s540_791; /* 0x41D95F jmp */
s540_964: /* 0x41D964 loc_41D964 (order-0 symbol path) */
    esi = G32(0x45AF5C); /* 0x41D964 */
    ebx = 0; /* 0x41D96A xor ebx,ebx */
    if (esi <= 1U) goto s540_9ed; /* 0x41D96C cmp esi,1; 0x41D96F jbe (unsigned) */
    eax = v44; /* 0x41D975 mov eax,[var_44] */
    eax -= esi; /* 0x41D979 sub eax,esi */
    if ((int)eax < 0) goto s540_9af; /* 0x41D97B test eax,eax; 0x41D97D jl (signed) */
    eax += v30; /* 0x41D97F add eax,[var_30] */
    alb = M8(eax); /* 0x41D983 mov al,[eax] */
    alb = (unsigned char)~alb; /* 0x41D985 not al */
s540_987: /* 0x41D987 loc_41D987 */
    edx = G32(0x45AF5C); /* 0x41D987 */
    esi = v44; /* 0x41D98D mov esi,[var_44] */
    edx += edx; /* 0x41D991 add edx,edx */
    esi -= edx; /* 0x41D993 sub esi,edx */
    if ((int)esi < 0) goto s540_9c8; /* 0x41D995 test esi,esi; 0x41D997 jl (signed) */
    esi += v30; /* 0x41D999 add esi,[var_30] */
    ahb = M8(esi); /* 0x41D99D mov ah,[esi] */
    ahb = (unsigned char)~ahb; /* 0x41D99F not ah */
    if (alb != ahb) goto s540_9a6; /* 0x41D9A1 cmp al,ah; 0x41D9A3 jnz */
    ebx++; /* 0x41D9A5 inc ebx */
s540_9a6: /* 0x41D9A6 loc_41D9A6 */
    clb = chb; /* 0x41D9A6 mov cl,ch */
    clb -= alb; /* 0x41D9A8 sub cl,al */
    goto s540_791; /* 0x41D9AA jmp */
s540_9af: /* 0x41D9AF loc_41D9AF */
    eax = v20; /* 0x41D9AF mov eax,[var_20] */
    edi = G32(0x45BA3C); /* 0x41D9B3 */
    eax -= esi; /* 0x41D9B9 sub eax,esi (esi=45AF5C from 0x41D964) */
    edx = G32(0x433B40); /* 0x41D9BB */
    eax &= edi; /* 0x41D9C1 and eax,edi */
    alb = M8(edx + eax); /* 0x41D9C3 mov al,[edx+eax] */
    goto s540_987; /* 0x41D9C6 jmp */
s540_9c8: /* 0x41D9C8 loc_41D9C8 */
    esi = v20; /* 0x41D9C8 mov esi,[var_20] */
    esi -= edx; /* 0x41D9CC sub esi,edx (edx=2*45AF5C) */
    edx = esi; /* 0x41D9CE mov edx,esi */
    edx &= G32(0x45BA3C); /* 0x41D9D0 and edx,dword_45BA3C */
    esi = G32(0x433B40); /* 0x41D9D6 */
    ahb = M8(edx + esi); /* 0x41D9DC mov ah,[edx+esi] */
    if (alb != ahb) goto s540_9a6; /* 0x41D9DF cmp al,ah; 0x41D9E1 jnz */
    ebx++; /* 0x41D9E3 inc ebx */
    clb = chb; /* 0x41D9E4 mov cl,ch */
    clb -= alb; /* 0x41D9E6 sub cl,al */
    goto s540_791; /* 0x41D9E8 jmp */
s540_9ed: /* 0x41D9ED loc_41D9ED */
    edx = v24; /* 0x41D9ED mov edx,[var_24] */
    alb = (unsigned char)(v24 & 0xFFU); /* 0x41D9F1 mov al,byte ptr [var_24] */
    edx >>= 8; /* 0x41D9F5 shr edx,8 */
    if ((edx & 0xFFU) != alb) goto s540_9a6; /* 0x41D9F8 cmp dl,al; 0x41D9FA jnz */
    ebx = 1; /* 0x41D9FC mov ebx,1 */
    clb = chb; /* 0x41DA01 mov cl,ch */
    clb -= alb; /* 0x41DA03 sub cl,al */
    goto s540_791; /* 0x41DA05 jmp */
s540_a0a: /* 0x41DA0A loc_41DA0A (order>=2 context select) */
    if (alb != 2) goto s540_a29; /* 0x41DA0A cmp al,2; 0x41DA0C jnz */
    eax = 0; /* 0x41DA0E xor eax,eax */
    eax = clb; /* 0x41DA10 mov al,cl (eax was zero) */
    ebx = 0; /* 0x41DA12 xor ebx,ebx */
    ebx = M8(0x434338 + eax); /* 0x41DA14 mov bl,byte_434338[eax] (ebx was zero) */
    eax = v2c; /* 0x41DA1A mov eax,[var_2C] */
    eax &= 1U; /* 0x41DA1E and eax,1 */
    eax <<= 4; /* 0x41DA21 shl eax,4 */
    goto s540_8fb; /* 0x41DA24 jmp */
s540_a29: /* 0x41DA29 loc_41DA29 */
    if (alb != 3) goto s540_a50; /* 0x41DA29 cmp al,3; 0x41DA2B jnz */
    eax = 0; /* 0x41DA2D xor eax,eax */
    esi = 3; /* 0x41DA2F mov esi,3 */
    eax = clb; /* 0x41DA34 mov al,cl (eax was zero) */
    ebx = 0; /* 0x41DA36 xor ebx,ebx */
    edx = 0; /* 0x41DA38 xor edx,edx */
    ebx = M8(0x434438 + eax); /* 0x41DA3A mov bl,byte_434438[eax] (ebx was zero) */
    eax = v2c; /* 0x41DA40 mov eax,[var_2C] */
    edx = eax % esi; /* 0x41DA44 div esi: remainder (edx was zero) */
    eax = eax / esi; /* 0x41DA44 div esi: quotient (discarded after; hw #DE only if div-by-zero, impossible with esi=3) */
    edx <<= 5; /* 0x41DA46 shl edx,5 */
    ebx += edx; /* 0x41DA49 add ebx,edx */
    goto s540_8fd; /* 0x41DA4B jmp */
s540_a50: /* 0x41DA50 loc_41DA50 (order>=4 context select) */
    eax = 0; /* 0x41DA50 xor eax,eax */
    eax = clb; /* 0x41DA52 mov al,cl (eax was zero) */
    ebx = 0; /* 0x41DA54 xor ebx,ebx */
    ebx = M8(0x434338 + eax); /* 0x41DA56 mov bl,byte_434338[eax] (ebx was zero) */
    eax = v2c; /* 0x41DA5C mov eax,[var_2C] */
    eax &= 3U; /* 0x41DA60 and eax,3 */
    eax <<= 4; /* 0x41DA63 shl eax,4 */
    goto s540_8fb; /* 0x41DA66 jmp */
s540_a6b: /* 0x41DA6B loc_41DA6B (order-0 model update) */
    if (chb == 0) goto s540_901; /* 0x41DA6B test ch,ch; 0x41DA6D jz */
    if (chb == 0xFFU) goto s540_901; /* 0x41DA73 cmp ch,0FFh; 0x41DA76 jz */
    eax = 0; /* 0x41DA7C xor eax,eax */
    eax = chb; /* 0x41DA7E mov al,ch (eax was zero) */
    edx = v20; /* 0x41DA80 mov edx,[var_20] */
    esi = G32(0x4597A0 + eax * 4U); /* 0x41DA84 */
    eax = edx; /* 0x41DA8B mov eax,edx */
    edi = G32(0x45BA3C); /* 0x41DA8D */
    eax -= esi; /* 0x41DA93 sub eax,esi */
    eax &= edi; /* 0x41DA95 and eax,edi */
    if (eax < 0x100U) goto s540_ad1; /* 0x41DA97 cmp eax,100h; 0x41DA9C jb (unsigned) */
    alb = chb; /* 0x41DA9E mov al,ch */
    alb--; /* 0x41DAA0 dec al */
    eax = alb; /* 0x41DAA2 and eax,0FFh (eax high is stale; al lane only) */
    esi = G32(0x4597A0 + eax * 4U); /* 0x41DAA7 */
    eax = edx; /* 0x41DAAE mov eax,edx */
    eax -= esi; /* 0x41DAB0 sub eax,esi */
    eax &= edi; /* 0x41DAB2 and eax,edi */
    if (eax < 0x100U) goto s540_ad1; /* 0x41DAB4 cmp eax,100h; 0x41DAB9 jb (unsigned) */
    alb = chb; /* 0x41DABB mov al,ch */
    alb++; /* 0x41DABD inc al */
    eax = alb; /* 0x41DABF and eax,0FFh (eax high is stale; al lane only) */
    esi = G32(0x4597A0 + eax * 4U); /* 0x41DAC4 */
    eax = edx; /* 0x41DACB mov eax,edx */
    eax -= esi; /* 0x41DACD sub eax,esi */
    eax &= edi; /* 0x41DACF and eax,edi */
s540_ad1: /* 0x41DAD1 loc_41DAD1 */
    if (eax == 0) goto s540_b53; /* 0x41DAD1 test eax,eax; 0x41DAD3 jz */
    if (eax >= 0x100U) goto s540_b53; /* 0x41DAD9 cmp eax,100h; 0x41DADE jnb (unsigned) */
    edx = G32(0x458B98 + eax * 4U); /* 0x41DAE0 */
    edx++; /* 0x41DAE7 inc edx */
    esi = G32(0x45AF5C); /* 0x41DAE8 */
    G32(0x458B98 + eax * 4U) = edx; /* 0x41DAEE */
    if (eax == esi) goto s540_b53; /* 0x41DAF5 cmp eax,esi; 0x41DAF7 jz */
    edx = v44; /* 0x41DAF9 mov edx,[var_44] */
    edx -= esi; /* 0x41DAFD sub edx,esi */
    if ((int)edx < 0) goto s540_b98; /* 0x41DAFF test edx,edx; 0x41DB01 jl (signed) */
    edx += v30; /* 0x41DB07 add edx,[var_30] */
    dlb = M8(edx); /* 0x41DB0B mov dl,[edx] */
    dlb = (unsigned char)~dlb; /* 0x41DB0D not dl */
s540_b0f: /* 0x41DB0F loc_41DB0F */
    if (dlb != chb) goto s540_bb2; /* 0x41DB0F cmp dl,ch; 0x41DB11 jnz */
    edx = G32(0x45AF5C); /* 0x41DB17 */
    G32(0x458B98 + edx * 4U)++; /* 0x41DB1D inc dword_458B98[edx*4] */
s540_b24: /* 0x41DB24 loc_41DB24 */
    edx = G32(0x45AF58); /* 0x41DB24 */
    if (eax != edx) goto s540_bb8; /* 0x41DB2A cmp eax,edx; 0x41DB2C jnz */
    esi = G32(0x45AF5C); /* 0x41DB32 */
    edx = G32(0x458B98 + edx * 4U); /* 0x41DB38 */
    if (edx < G32(0x458B98 + esi * 4U)) goto s540_b53; /* 0x41DB3F cmp; 0x41DB46 jb (unsigned) */
    G32(0x45AF58) = esi; /* 0x41DB48 */
    G32(0x45AF5C) = eax; /* 0x41DB4E */
s540_b53: /* 0x41DB53 loc_41DB53 */
    eax = ebp; /* 0x41DB53 mov eax,ebp */
    ebp++; /* 0x41DB55 inc ebp */
    if (eax < 0x40U) goto s540_b84; /* 0x41DB56 cmp eax,40h; 0x41DB59 jb (unsigned) */
    eax = G32(0x458B9C); /* 0x41DB5B */
    eax++; /* 0x41DB60 inc eax */
    ebp = 0; /* 0x41DB61 xor ebp,ebp */
    G32(0x458B9C) = eax; /* 0x41DB63 */
    eax = 0; /* 0x41DB68 xor eax,eax */
    do { /* 0x41DB6A loc_41DB6A (halve 0x100 counters) */
        esi = G32(0x458B98 + eax); /* 0x41DB6A mov esi,dword_458B98[eax] */
        esi >>= 1; /* 0x41DB70 shr esi,1 */
        eax += 4; /* 0x41DB72 add eax,4 */
        ebp += esi; /* 0x41DB75 add ebp,esi */
        G32(0x458B94 + eax) = esi; /* 0x41DB77 mov dword_458B94[eax],esi */
    } while (eax != 0x400U); /* 0x41DB7D cmp eax,400h; 0x41DB82 jnz */
s540_b84: /* 0x41DB84 loc_41DB84 */
    eax = 0; /* 0x41DB84 xor eax,eax */
    eax = chb; /* 0x41DB86 mov al,ch (eax was zero) */
    edx = v20; /* 0x41DB88 mov edx,[var_20] */
    G32(0x4597A0 + eax * 4U) = edx; /* 0x41DB8C */
    goto s540_901; /* 0x41DB93 jmp */
s540_b98: /* 0x41DB98 loc_41DB98 */
    edx = v20; /* 0x41DB98 mov edx,[var_20] */
    edx -= esi; /* 0x41DB9C sub edx,esi (esi=45AF5C from 0x41DAE8) */
    edx &= G32(0x45BA3C); /* 0x41DB9E and edx,dword_45BA3C */
    esi = G32(0x433B40); /* 0x41DBA4 */
    dlb = M8(edx + esi); /* 0x41DBAA mov dl,[edx+esi] */
    goto s540_b0f; /* 0x41DBAD jmp */
s540_bb2: /* 0x41DBB2 loc_41DBB2 */
    ebp++; /* 0x41DBB2 inc ebp */
    goto s540_b24; /* 0x41DBB3 jmp */
s540_bb8: /* 0x41DBB8 loc_41DBB8 */
    G32(0x45AF58) = eax; /* 0x41DBB8 */
    goto s540_b53; /* 0x41DBBD jmp */
    /* 0x41DBBD sub_41D540 endp */
}

/* ---- sub_41DBC0 (lst 43104..43513, 409 lines) ----
 * Role hypothesis: block model-order selector feeding sub_41CF60's ebp
 * (the sole caller at lst 42350..42353 passes eax=src ptr, edx=ebp-0x100
 * and takes back eax). For short inputs (edx <= 0x800) it returns the
 * entry count untouched. Otherwise it zeroes the 45AF00/45AF14 score
 * tables and the 455F60/457378 histograms (via sub_40D570), builds five
 * lagged byte-difference lanes over 0x400 windows (diff bytes into the
 * 453760 area, counts into 455F60, squared-count scores into 45AF00;
 * then the same shape over the second half into 457378/45AF14), and
 * finally walks positions, maintaining per-lane 455F60/457378 costs and
 * picking per position the cheaper of two predictors (45AF00 minimum
 * with 3x-vs-4x hysteresis, then 45AF14 minimum); positions where both
 * agree advance, otherwise it records byte_45C676 and returns pos-0x200.
 * Entry-live regs (first-use): eax (saved 0x41DBC8, reloaded 0x41DC69),
 * edx (saved 0x41DBCC, compared 0x41DBD9). ebx/ecx/esi/edi/ebp are
 * pushed and written before any read (ebx 0x41DBF7, ecx 0x41DC19,
 * esi 0x41DC03, edi 0x41DC6F, ebp 0x41DC6D). DEVIATION: none from the
 * 2-arg call-site shape. Frame: 5 pushes + sub esp,60h = 0x74
 * (var_74..var_18).
 * NOTE: `mov esi,400h` (0x41DC03) is read much later (0x41DC34) across
 * two d4v_40d570 calls, and `xor ecx,ecx` (0x41DC19) across one; the
 * draft reads the C locals, matching the original's register survival.
 */
unsigned int d4v_41dbc0(dec4v_ctx *c, unsigned int base, unsigned int count)
{
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned int v24 = 0, v28 = 0, v2c = 0, v30 = 0, v34 = 0, v38 = 0, v3c = 0, v40 = 0, v44 = 0, v48 = 0, v50 = 0;
    unsigned int v4c = 0, v54 = 0, v58 = 0, v5c = 0, v60 = 0, v64 = 0, v68 = 0, v6c = 0, v70 = 0, v74 = 0;
    unsigned char v18 = 0, v20 = 0, v1c = 0;
    unsigned char alb = 0, blb = 0, bhb = 0, clb = 0;
    /* prologue: 0x41DBC0 push ebx; 0x41DBC1 push ecx; 0x41DBC2 push esi;
     * 0x41DBC3 push edi; 0x41DBC4 push ebp; 0x41DBC5 sub esp,60h */
    v40 = base; /* 0x41DBC8 mov [var_40],eax */
    v6c = count; /* 0x41DBCC mov [var_6C],edx */
    alb = G8(0x45C676); /* 0x41DBD0 mov al,byte_45C676 */
    v18 = alb; /* 0x41DBD5 mov [var_18],al */
    if (v6c > 0x800U) goto sdbc_0bf7; /* 0x41DBD9 cmp edx,800h; 0x41DBDF ja (unsigned) */
sdbc_0be1: /* 0x41DBE1 loc_41DBE1 (short-input return) */
    alb = v18; /* 0x41DBE1 mov al,[var_18] */
    G8(0x45C676) = alb; /* 0x41DBE5 */
    eax = v6c; /* 0x41DBEA mov eax,[var_6C] */
    /* epilogue: 0x41DBEE add esp,60h; 0x41DBF1 pop ebp; 0x41DBF2 pop edi;
     * 0x41DBF3 pop esi; 0x41DBF4 pop ecx; 0x41DBF5 pop ebx */
    return eax; /* 0x41DBF6 retn */
sdbc_0bf7: /* 0x41DBF7 loc_41DBF7 */
    ebx = 0x14U; /* 0x41DBF7 mov ebx,14h */
    eax = 0x45AF00U; /* 0x41DBFC mov eax,offset dword_45AF00 */
    edx = 0; /* 0x41DC01 xor edx,edx */
    esi = 0x400U; /* 0x41DC03 mov esi,400h */
    d4v_fill(c, ebx, eax, edx); /* 0x41DC08 call sub_40D570 */
    ebx = 0x14U; /* 0x41DC0D mov ebx,14h */
    eax = 0x45AF14U; /* 0x41DC12 mov eax,offset dword_45AF14 */
    edx = 0; /* 0x41DC17 xor edx,edx */
    ecx = 0; /* 0x41DC19 xor ecx,ecx */
    d4v_fill(c, ebx, eax, edx); /* 0x41DC1B call sub_40D570 */
    v64 = ecx; /* 0x41DC20 mov [var_64],ecx (=0) */
    v3c = ecx; /* 0x41DC24 mov [var_3C],ecx (=0) */
    ebx = 0x1400U; /* 0x41DC28 mov ebx,1400h */
    eax = 0x455F60U; /* 0x41DC2D mov eax,offset dword_455F60 */
    edx = 0; /* 0x41DC32 xor edx,edx */
    v54 = esi; /* 0x41DC34 mov [var_54],esi (=400h) */
    d4v_fill(c, ebx, eax, edx); /* 0x41DC38 call sub_40D570 */
    ebx = 0x1400U; /* 0x41DC3D mov ebx,1400h */
    eax = 0x457378U; /* 0x41DC42 mov eax,offset dword_457378 */
    edx = 0; /* 0x41DC47 xor edx,edx */
    v5c = ecx; /* 0x41DC49 mov [var_5C],ecx (=0) */
    d4v_fill(c, ebx, eax, edx); /* 0x41DC4D call sub_40D570 */
sdbc_0c52: /* 0x41DC52 loc_41DC52 (outer lane k=0..4, first half) */
    eax = v64; /* 0x41DC52 mov eax,[var_64] */
    esi = v3c; /* 0x41DC56 mov esi,[var_3C] */
    ecx = v64; /* 0x41DC5A mov ecx,[var_64] */
    edx = 0; /* 0x41DC5E xor edx,edx */
    eax <<= 10; /* 0x41DC60 shl eax,0Ah */
    ecx = 0U - ecx; /* 0x41DC63 neg ecx */
    v34 = eax; /* 0x41DC65 mov [var_34],eax */
    eax = v40; /* 0x41DC69 mov eax,[var_40] */
    ebp = esi; /* 0x41DC6D mov ebp,esi */
    edi = eax + ecx; /* 0x41DC6F lea edi,[eax+ecx] */
    ecx = esi; /* 0x41DC72 mov ecx,esi */
sdbc_0c74: /* 0x41DC74 loc_41DC74 (0x400 window) */
    ebx = v64; /* 0x41DC74 mov ebx,[var_64] */
    if (ebx == 0) goto sdbc_1060; /* 0x41DC78 test ebx,ebx; 0x41DC7A jz */
    if (edx < ebx) goto sdbc_1060; /* 0x41DC80 cmp edx,ebx; 0x41DC82 jb (unsigned) */
    blb = M8(eax); /* 0x41DC88 mov bl,[eax] */
    bhb = M8(edi); /* 0x41DC8A mov bh,[edi] */
    blb -= bhb; /* 0x41DC8C sub bl,bh */
    M8(0x453760 + ebp) = blb; /* 0x41DC8E mov ss:byte_453760[ebp],bl (ss==flat) */
sdbc_0c94: /* 0x41DC94 loc_41DC94 */
    esi++; /* 0x41DC94 inc esi */
    ebx = 0; /* 0x41DC95 xor ebx,ebx */
    ebp++; /* 0x41DC97 inc ebp */
    ebx = M8(0x453760 + ecx); /* 0x41DC98 mov bl,byte_453760[ecx] (ebx was zero) */
    eax++; /* 0x41DC9E inc eax */
    ebx <<= 2; /* 0x41DC9F shl ebx,2 */
    edi++; /* 0x41DCA2 inc edi */
    v70 = ebx; /* 0x41DCA3 mov [var_70],ebx */
    ebx = v34; /* 0x41DCA7 mov ebx,[var_34] */
    ecx++; /* 0x41DCAB inc ecx */
    ebx += v70; /* 0x41DCAC add ebx,[var_70] */
    edx++; /* 0x41DCB0 inc edx */
    G32(0x455F60 + ebx)++; /* 0x41DCB1 inc dword_455F60[ebx] */
    if (edx < 0x400U) goto sdbc_0c74; /* 0x41DCB7 cmp edx,400h; 0x41DCBD jb (unsigned) */
    edx = v5c; /* 0x41DCBF mov edx,[var_5C] */
    eax = v64; /* 0x41DCC3 mov eax,[var_64] */
    ebx = v54; /* 0x41DCC7 mov ebx,[var_54] */
    eax <<= 10; /* 0x41DCCB shl eax,0Ah */
sdbc_0cce: /* 0x41DCCE loc_41DCCE (fold squared counts into 45AF00) */
    ecx = G32(0x455F60 + eax); /* 0x41DCCE */
    ecx *= ecx; /* 0x41DCD4 imul ecx,ecx (low 32 bits) */
    edi = G32(0x45AF00 + edx); /* 0x41DCD7 */
    edi += ecx; /* 0x41DCDD add edi,ecx */
    eax += 4; /* 0x41DCDF add eax,4 */
    G32(0x45AF00 + edx) = edi; /* 0x41DCE2 */
    if (eax != ebx) goto sdbc_0cce; /* 0x41DCE8 cmp eax,ebx; 0x41DCEA jnz */
    ebp = v3c; /* 0x41DCEC mov ebp,[var_3C] */
    eax = v54; /* 0x41DCF0 mov eax,[var_54] */
    edx = v5c; /* 0x41DCF4 mov edx,[var_5C] */
    ecx = v64; /* 0x41DCF8 mov ecx,[var_64] */
    ebp += 0x800U; /* 0x41DCFC add ebp,800h */
    eax += 0x400U; /* 0x41DD02 add eax,400h */
    edx += 4; /* 0x41DD07 add edx,4 */
    ecx++; /* 0x41DD0A inc ecx */
    v3c = ebp; /* 0x41DD0B mov [var_3C],ebp */
    v54 = eax; /* 0x41DD0F mov [var_54],eax */
    v5c = edx; /* 0x41DD13 mov [var_5C],edx */
    v64 = ecx; /* 0x41DD17 mov [var_64],ecx */
    if (ecx < 5U) goto sdbc_0c52; /* 0x41DD1B cmp ecx,5; 0x41DD1E jb (unsigned) */
    esi = 0x800U; /* 0x41DD24 mov esi,800h */
    eax = 0x400U; /* 0x41DD29 mov eax,400h */
    ebp = 4; /* 0x41DD2E mov ebp,4 */
    edi = 0; /* 0x41DD33 xor edi,edi */
    v44 = esi; /* 0x41DD35 mov [var_44],esi */
    v50 = eax; /* 0x41DD39 mov [var_50],eax */
    eax = v40; /* 0x41DD3D mov eax,[var_40] */
    v48 = edi; /* 0x41DD41 mov [var_48],edi (=0) */
    eax += 0x400U; /* 0x41DD45 add eax,400h */
    v60 = edi; /* 0x41DD4A mov [var_60],edi (=0) */
    v68 = eax; /* 0x41DD4E mov [var_68],eax */
sdbc_0d52: /* 0x41DD52 loc_41DD52 (outer lane k=0..4, second half) */
    ecx = v68; /* 0x41DD52 mov ecx,[var_68] */
    eax = edi; /* 0x41DD56 mov eax,edi */
    ebx = edi; /* 0x41DD58 mov ebx,edi */
    eax <<= 10; /* 0x41DD5A shl eax,0Ah */
    ebx = 0U - ebx; /* 0x41DD5D neg ebx */
    v30 = eax; /* 0x41DD5F mov [var_30],eax */
    esi = ecx + ebx; /* 0x41DD63 lea esi,[ecx+ebx] */
    eax = v48; /* 0x41DD66 mov eax,[var_48] */
    ebx = v44; /* 0x41DD6A mov ebx,[var_44] */
    eax += 0x400U; /* 0x41DD6E add eax,400h */
    v2c = ebx; /* 0x41DD73 mov [var_2C],ebx */
    edx = eax; /* 0x41DD77 mov edx,eax */
sdbc_0d79: /* 0x41DD79 loc_41DD79 */
    if (edi == 0) goto sdbc_106d; /* 0x41DD79 test edi,edi; 0x41DD7B jz */
    blb = M8(ecx); /* 0x41DD81 mov bl,[ecx] */
    bhb = M8(esi); /* 0x41DD83 mov bh,[esi] */
    blb -= bhb; /* 0x41DD85 sub bl,bh */
sdbc_0d87: /* 0x41DD87 loc_41DD87 */
    M8(0x453760 + edx) = blb; /* 0x41DD87 mov byte_453760[edx],bl */
    ebx = 0; /* 0x41DD8D xor ebx,ebx */
    ebx = M8(0x453760 + eax); /* 0x41DD8F mov bl,byte_453760[eax] (ebx was zero) */
    ebx <<= 2; /* 0x41DD95 shl ebx,2 */
    edx++; /* 0x41DD98 inc edx */
    ebx += v30; /* 0x41DD99 add ebx,[var_30] */
    ecx++; /* 0x41DD9D inc ecx */
    eax++; /* 0x41DD9E inc eax */
    G32(0x457378 + ebx)++; /* 0x41DD9F inc dword_457378[ebx] */
    ebx = v2c; /* 0x41DDA5 mov ebx,[var_2C] */
    esi++; /* 0x41DDA9 inc esi */
    if (eax != ebx) goto sdbc_0d79; /* 0x41DDAA cmp eax,ebx; 0x41DDAC jnz */
    edx = v60; /* 0x41DDAE mov edx,[var_60] */
    eax = edi; /* 0x41DDB2 mov eax,edi */
    ebx = v50; /* 0x41DDB4 mov ebx,[var_50] */
    eax <<= 10; /* 0x41DDB8 shl eax,0Ah */
sdbc_0dbb: /* 0x41DDBB loc_41DDBB (fold squared counts into 45AF14) */
    ecx = G32(0x457378 + eax); /* 0x41DDBB */
    ecx *= ecx; /* 0x41DDC1 imul ecx,ecx (low 32 bits) */
    esi = G32(0x45AF14 + edx); /* 0x41DDC4 */
    esi += ecx; /* 0x41DDCA add esi,ecx */
    eax += 4; /* 0x41DDCC add eax,4 */
    G32(0x45AF14 + edx) = esi; /* 0x41DDCF */
    if (eax != ebx) goto sdbc_0dbb; /* 0x41DDD5 cmp eax,ebx; 0x41DDD7 jnz */
    eax = v44; /* 0x41DDD9 mov eax,[var_44] */
    edx = v48; /* 0x41DDDD mov edx,[var_48] */
    ecx = v50; /* 0x41DDE1 mov ecx,[var_50] */
    ebx = v60; /* 0x41DDE5 mov ebx,[var_60] */
    edi++; /* 0x41DDE9 inc edi */
    eax += 0x800U; /* 0x41DDEA add eax,800h */
    edx += 0x800U; /* 0x41DDEF add edx,800h */
    ecx += 0x400U; /* 0x41DDF5 add ecx,400h */
    ebx += ebp; /* 0x41DDFB add ebx,ebp (ebp=4) */
    v44 = eax; /* 0x41DDFD mov [var_44],eax */
    v48 = edx; /* 0x41DE01 mov [var_48],edx */
    v50 = ecx; /* 0x41DE05 mov [var_50],ecx */
    v60 = ebx; /* 0x41DE09 mov [var_60],ebx */
    if (edi < 5U) goto sdbc_0d52; /* 0x41DE0D cmp edi,5; 0x41DE10 jb (unsigned) */
    esi = 0x800U; /* 0x41DE16 mov esi,800h */
    edi = v6c; /* 0x41DE1B mov edi,[var_6C] */
    v38 = esi; /* 0x41DE1F mov [var_38],esi (=800h) */
    if (edi <= esi) goto sdbc_0be1; /* 0x41DE23 cmp edi,esi; 0x41DE25 jbe (unsigned) */
    eax = v40; /* 0x41DE2B mov eax,[var_40] */
    ebx = 0xC00U; /* 0x41DE2F mov ebx,0C00h */
    eax += esi; /* 0x41DE34 add eax,esi */
    v4c = ebx; /* 0x41DE36 mov [var_4C],ebx (=0C00h) */
    v58 = eax; /* 0x41DE3A mov [var_58],eax */
sdbc_0e3e: /* 0x41DE3E loc_41DE3E (per-position predictor walk) */
    edi = v38; /* 0x41DE3E mov edi,[var_38] */
    eax = v4c; /* 0x41DE42 mov eax,[var_4C] */
    esi = v40; /* 0x41DE46 mov esi,[var_40] */
    ebp = v38; /* 0x41DE4A mov ebp,[var_38] */
    clb = 0; /* 0x41DE4E xor cl,cl (other ecx bytes dead until 0x41DF75) */
    edx = 0; /* 0x41DE50 xor edx,edx */
    ebx = 0; /* 0x41DE52 xor ebx,ebx */
    v20 = clb; /* 0x41DE54 mov [var_20],cl (=0) */
    v1c = clb; /* 0x41DE58 mov [var_1C],cl (=0) */
    edi &= 0x7FFU; /* 0x41DE5C and edi,7FFh */
    eax &= 0x7FFU; /* 0x41DE62 and eax,7FFh */
    esi += ebp; /* 0x41DE67 add esi,ebp */
    ecx = v58; /* 0x41DE69 mov ecx,[var_58] */
    v74 = eax; /* 0x41DE6D mov [var_74],eax */
    v24 = edi; /* 0x41DE70 mov [var_24],edi */
    v28 = ecx; /* 0x41DE74 mov [var_28],ecx */
    eax = 0; /* 0x41DE78 xor eax,eax */
sdbc_0e7a: /* 0x41DE7A loc_41DE7A (5 cost lanes) */
    ecx = v24; /* 0x41DE7A mov ecx,[var_24] */
    clb = M8(0x453760 + ecx); /* 0x41DE7E mov cl,byte_453760[ecx] */
    ecx = clb; /* 0x41DE84 and ecx,0FFh */
    ebp = G32(0x455F60 + ebx + ecx * 4U); /* 0x41DE8A */
    G32(0x45AF00 + eax) -= ebp; /* 0x41DE91 sub dword_45AF00[eax],ebp */
    ebp--; /* 0x41DE97 dec ebp */
    G32(0x455F60 + ebx + ecx * 4U) = ebp; /* 0x41DE98 */
    ecx = ebp; /* 0x41DE9F mov ecx,ebp */
    ebp = G32(0x45AF00 + eax); /* 0x41DEA1 */
    ebp -= ecx; /* 0x41DEA7 sub ebp,ecx */
    ecx = v74; /* 0x41DEA9 mov ecx,[var_74] */
    G32(0x45AF00 + eax) = ebp; /* 0x41DEAC */
    clb = M8(0x453760 + ecx); /* 0x41DEB2 mov cl,byte_453760[ecx] */
    ecx = clb; /* 0x41DEB8 and ecx,0FFh */
    ebp = G32(0x455F60 + ebx + ecx * 4U); /* 0x41DEBE */
    G32(0x45AF00 + eax) += ebp; /* 0x41DEC5 add dword_45AF00[eax],ebp */
    ebp++; /* 0x41DECB inc ebp */
    G32(0x455F60 + ebx + ecx * 4U) = ebp; /* 0x41DECC */
    G32(0x45AF00 + eax) += ebp; /* 0x41DED3 add dword_45AF00[eax],ebp */
    ebp = G32(0x457378 + ebx + ecx * 4U); /* 0x41DED9 */
    G32(0x45AF14 + eax) -= ebp; /* 0x41DEE0 sub dword_45AF14[eax],ebp */
    ebp--; /* 0x41DEE6 dec ebp */
    G32(0x457378 + ebx + ecx * 4U) = ebp; /* 0x41DEE7 */
    ecx = ebp; /* 0x41DEEE mov ecx,ebp */
    G32(0x45AF14 + eax) -= ecx; /* 0x41DEF0 sub dword_45AF14[eax],ecx */
    if (edx == 0) goto sdbc_1074; /* 0x41DEF6 test edx,edx; 0x41DEF8 jz */
    ecx = v28; /* 0x41DEFE mov ecx,[var_28] */
    clb = M8(ecx); /* 0x41DF02 mov cl,[ecx] */
    clb -= M8(esi); /* 0x41DF04 sub cl,[esi] */
sdbc_0f06: /* 0x41DF06 loc_41DF06 */
    M8(0x453760 + edi) = clb; /* 0x41DF06 mov byte_453760[edi],cl */
    ecx = clb; /* 0x41DF0C and ecx,0FFh */
    esi--; /* 0x41DF12 dec esi */
    eax += 4; /* 0x41DF13 add eax,4 */
    ebp = G32(0x457378 + ebx + ecx * 4U); /* 0x41DF16 */
    ebx += 0x400U; /* 0x41DF1D add ebx,400h */
    G32(0x45AF10 + eax) += ebp; /* 0x41DF23 add dword_45AF10[eax],ebp */
    ebp++; /* 0x41DF29 inc ebp */
    edi += 0x800U; /* 0x41DF2A add edi,800h */
    G32(0x456F78 + ebx + ecx * 4U) = ebp; /* 0x41DF30 */
    ecx = ebp; /* 0x41DF37 mov ecx,ebp */
    ebp = G32(0x45AF10 + eax); /* 0x41DF39 */
    edx++; /* 0x41DF3F inc edx */
    ebp += ecx; /* 0x41DF40 add ebp,ecx */
    ecx = v74; /* 0x41DF42 mov ecx,[var_74] */
    G32(0x45AF10 + eax) = ebp; /* 0x41DF45 */
    ebp = v24; /* 0x41DF4B mov ebp,[var_24] */
    ecx += 0x800U; /* 0x41DF4F add ecx,800h */
    ebp += 0x800U; /* 0x41DF55 add ebp,800h */
    v74 = ecx; /* 0x41DF5B mov [var_74],ecx */
    v24 = ebp; /* 0x41DF5E mov [var_24],ebp */
    if (edx < 5U) goto sdbc_0e7a; /* 0x41DF62 cmp edx,5; 0x41DF65 jb (unsigned) */
    edx = 1; /* 0x41DF6B mov edx,1 */
    eax = 4; /* 0x41DF70 mov eax,4 */
sdbc_0f75: /* 0x41DF75 loc_41DF75 (pick cheaper predictor of each family) */
    ecx = 0; /* 0x41DF75 xor ecx,ecx */
    ecx = v20; /* 0x41DF77 mov cl,[var_20] */
    ebx = ecx * 4U; /* 0x41DF7B lea ebx,[ecx*4] */
    ecx = G32(0x45AF00 + eax); /* 0x41DF82 */
    if (ecx <= G32(0x45AF00 + ebx)) goto sdbc_0f94; /* 0x41DF88 cmp; 0x41DF8E jbe (unsigned) */
    v20 = (unsigned char)(edx & 0xFFU); /* 0x41DF90 mov [var_20],dl */
sdbc_0f94: /* 0x41DF94 loc_41DF94 */
    ecx = 0; /* 0x41DF94 xor ecx,ecx */
    ecx = v1c; /* 0x41DF96 mov cl,[var_1C] */
    ebx = G32(0x45AF14 + eax); /* 0x41DF9A */
    if (ebx <= G32(0x45AF14 + ecx * 4U)) goto sdbc_0fad; /* 0x41DFA0 cmp; 0x41DFA7 jbe (unsigned) */
    v1c = (unsigned char)(edx & 0xFFU); /* 0x41DFA9 mov [var_1C],dl */
sdbc_0fad: /* 0x41DFAD loc_41DFAD */
    edx++; /* 0x41DFAD inc edx */
    eax += 4; /* 0x41DFAE add eax,4 */
    if (edx < 5U) goto sdbc_0f75; /* 0x41DFB1 cmp edx,5; 0x41DFB4 jb (unsigned) */
    alb = v20; /* 0x41DFB6 mov al,[var_20] */
    if (alb == v18) goto sdbc_0fec; /* 0x41DFBA cmp al,[var_18]; 0x41DFBE jz */
    eax = v20; /* 0x41DFC0 xor eax,eax; 0x41DFC2 mov al,[var_20] */
    edx = G32(0x45AF00 + eax * 4U); /* 0x41DFC6 */
    eax = v18; /* 0x41DFCD xor eax,eax; 0x41DFCF mov al,[var_18] */
    eax = G32(0x45AF00 + eax * 4U); /* 0x41DFD3 */
    edx = edx * 2U + edx; /* 0x41DFDA lea edx,[edx+edx*2] */
    eax <<= 2; /* 0x41DFDD shl eax,2 */
    if (edx >= eax) goto sdbc_0fec; /* 0x41DFE0 cmp edx,eax; 0x41DFE2 jnb (unsigned) */
    alb = v18; /* 0x41DFE4 mov al,[var_18] */
    v20 = alb; /* 0x41DFE8 mov [var_20],al */
sdbc_0fec: /* 0x41DFEC loc_41DFEC */
    alb = v1c; /* 0x41DFEC mov al,[var_1C] */
    if (alb == v18) goto sdbc_1022; /* 0x41DFF0 cmp al,[var_18]; 0x41DFF4 jz */
    eax = v1c; /* 0x41DFF6 xor eax,eax; 0x41DFFA mov al,[var_1C] */
    edx = v18; /* 0x41DFF8 xor edx,edx; 0x41DFFE mov dl,[var_18] */
    eax = G32(0x45AF14 + eax * 4U); /* 0x41E002 */
    edx = G32(0x45AF14 + edx * 4U); /* 0x41E009 */
    eax = eax * 2U + eax; /* 0x41E010 lea eax,[eax+eax*2] */
    edx <<= 2; /* 0x41E013 shl edx,2 */
    if (eax >= edx) goto sdbc_1022; /* 0x41E016 cmp eax,edx; 0x41E018 jnb (unsigned) */
    alb = v18; /* 0x41E01A mov al,[var_18] */
    v1c = alb; /* 0x41E01E mov [var_1C],al */
sdbc_1022: /* 0x41E022 loc_41E022 */
    alb = v20; /* 0x41E022 mov al,[var_20] */
    if (alb != v1c) goto sdbc_107f; /* 0x41E026 cmp al,[var_1C]; 0x41E02A jnz */
    alb = v20; /* 0x41E02C mov al,[var_20] */
    ebp = v58; /* 0x41E030 mov ebp,[var_58] */
    edx = v38; /* 0x41E034 mov edx,[var_38] */
    ecx = v6c; /* 0x41E038 mov ecx,[var_6C] */
    v18 = alb; /* 0x41E03C mov [var_18],al */
    ebp++; /* 0x41E040 inc ebp */
    edx++; /* 0x41E041 inc edx */
    eax = v4c; /* 0x41E042 mov eax,[var_4C] */
    v58 = ebp; /* 0x41E046 mov [var_58],ebp */
    eax++; /* 0x41E04A inc eax */
    v38 = edx; /* 0x41E04B mov [var_38],edx */
    v4c = eax; /* 0x41E04F mov [var_4C],eax */
    if (edx < ecx) goto sdbc_0e3e; /* 0x41E053 cmp edx,ecx; 0x41E055 jb (unsigned) */
    goto sdbc_0be1; /* 0x41E05B jmp */
sdbc_1060: /* 0x41E060 loc_41E060 (lane k==0 or head: direct copy) */
    blb = M8(eax); /* 0x41E060 mov bl,[eax] */
    M8(0x453760 + esi) = blb; /* 0x41E062 mov byte_453760[esi],bl */
    goto sdbc_0c94; /* 0x41E068 jmp */
sdbc_106d: /* 0x41E06D loc_41E06D (second half lane k==0: direct copy) */
    blb = M8(ecx); /* 0x41E06D mov bl,[ecx] */
    goto sdbc_0d87; /* 0x41E06F jmp */
sdbc_1074: /* 0x41E074 loc_41E074 (first cost lane: no diff) */
    ecx = v28; /* 0x41E074 mov ecx,[var_28] */
    clb = M8(ecx); /* 0x41E078 mov cl,[ecx] */
    goto sdbc_0f06; /* 0x41E07A jmp */
sdbc_107f: /* 0x41E07F loc_41E07F (predictors disagree: record and return) */
    G8(0x45C676) = alb; /* 0x41E07F mov byte_45C676,al (al=[var_20]) */
    eax = v38; /* 0x41E084 mov eax,[var_38] */
    eax -= 0x200U; /* 0x41E088 sub eax,200h */
    /* epilogue: 0x41E08D add esp,60h; 0x41E090 pop ebp; 0x41E091 pop edi;
     * 0x41E092 pop esi; 0x41E093 pop ecx; 0x41E094 pop ebx */
    return eax; /* 0x41E095 retn */
    /* 0x41E095 sub_41DBC0 endp */
}

/* ---- sub_41E0A0 (lst 43519..44047, 528 lines) ----
 * Role hypothesis: adaptive order-N scorer with online model update, the
 * second candidate scorer called from sub_41CF60's loc_41D11F (lst 42137..
 * 42143: eax=src ptr, edx=ebp=len, ebx=slot score, ecx=old value with the
 * order byte live in cl; the second entry (lst 42404..42411) passes the
 * same shape: edx=len, ebx=slot score indexed by the order byte,
 * cl=selected class). It first estimates an order class v14 in
 * {5..9} by counting history bytes <0x40 (weight 1) versus escape-like
 * bytes >=0xC0 (weight = running index, -1 sentinel logic), over the
 * 0x100 history window plus the inverted recent buffer; on an order-class
 * change it resets the model (stores 45AF3C/45C67B/45C679/45AF48/45C67D,
 * calls sub_41E5F0). The main loop (while v24 < entry len) prices one
 * symbol per iteration into ebp (starting 0), bailing to the restore path
 * once ebp exceeds the entry score; accepted symbols update the model via
 * sub_41E9F0(al=bh-sym, dl=bl), a div-based renormalization and sub_41E640.
 * Returns the order-class byte in al, or 0 when the price loses.
 * Entry-live regs (first-use): eax (saved 0x41E0A6, reloaded 0x41E155),
 * edx (saved 0x41E0AA, reloaded 0x41E116), ebx (saved 0x41E0AE, reloaded
 * 0x41E4EE), cl only of ecx (compared 0x41E100; full ecx first written
 * 0x41E108/0x41E29C). esi/edi/ebp written before read (esi 0x41E116,
 * edi 0x41E2A5/0x41E1CA, ebp 0x41E0EE xor). DEVIATION: none from the 4-arg
 * call-site shape; only CL of the 4th arg is live. Frame: 3 pushes +
 * sub esp,5Ch = 0x68 (var_68..var_10).
 * UNCERTAIN(0x41E58A): d4v_41e9f0/d4v_41e640 take the callees' own entry
 * footage on trust (al=bh, dl=bl here matches draft_s4b.c's 0x41E58A
 * caller note; E640 takes no regs per its first-use prologue there).
 */
unsigned int d4v_41e0a0(dec4v_ctx *c, unsigned int a, unsigned int d,
    unsigned int b, unsigned int e)
{
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0, esi = 0, edi = 0, ebp = 0;
    unsigned int v20 = 0, v24 = 0, v28 = 0, v2c = 0, v30 = 0, v34 = 0, v38 = 0, v3c = 0, v40 = 0, v44 = 0, v48 = 0;
    unsigned int v4c = 0, v50 = 0, v54 = 0, v58 = 0, v5c = 0, v60 = 0, v64 = 0, v68 = 0;
    unsigned char v10 = 0, v14 = 0, v18 = 0, v1c = 0;
    unsigned char alb = 0, ahb = 0, clb = 0, chb = 0, blb = 0, bhb = 0, dlb = 0, dhb = 0;
    /* prologue: 0x41E0A0 push esi; 0x41E0A1 push edi; 0x41E0A2 push ebp;
     * 0x41E0A3 sub esp,5Ch */
    v64 = a; /* 0x41E0A6 mov [var_64],eax */
    v34 = d; /* 0x41E0AA mov [var_34],edx */
    v3c = b; /* 0x41E0AE mov [var_3C],ebx */
    eax = G32(0x45BA34); /* 0x41E0B2 */
    v2c = eax; /* 0x41E0B7 mov [var_2C],eax */
    eax = G32(0x45AF38); /* 0x41E0BB */
    v44 = eax; /* 0x41E0C0 mov [var_44],eax */
    eax = G32(0x45AF48); /* 0x41E0C4 */
    v68 = eax; /* 0x41E0C9 mov [var_68],eax */
    eax = G32(0x45AF3C); /* 0x41E0CC */
    v4c = eax; /* 0x41E0D1 mov [var_4C],eax */
    alb = G8(0x45C67B); /* 0x41E0D5 mov al,byte_45C67B */
    v10 = alb; /* 0x41E0DA mov [var_10],al */
    alb = G8(0x45C67D); /* 0x41E0DE mov al,byte_45C67D */
    edx = 0; /* 0x41E0E3 xor edx,edx */
    v1c = alb; /* 0x41E0E5 mov [var_1C],al */
    alb = G8(0x45C679); /* 0x41E0E9 mov al,byte_45C679 */
    ebp = 0; /* 0x41E0EE xor ebp,ebp */
    v18 = alb; /* 0x41E0F0 mov [var_18],al */
    ahb = 1; /* 0x41E0F4 mov ah,1 */
    clb = (unsigned char)(e & 0xFFU); /* entry cl */
    v24 = edx; /* 0x41E0F6 mov [var_24],edx (=0) */
    G8(0x45C67E) = ahb; /* 0x41E0FA mov byte_45C67E,ah (=1) */
    if (clb != ahb) goto se0a0_293; /* 0x41E100 cmp cl,ah; 0x41E102 jnz */
    ecx = 0x100U; /* 0x41E108 mov ecx,100h */
    ebx = v2c; /* 0x41E10D mov ebx,[var_2C] */
    eax = G32(0x433B40); /* 0x41E111 */
    esi = v34; /* 0x41E116 mov esi,[var_34] */
    eax += ebx; /* 0x41E11A add eax,ebx */
    if (esi < ecx) goto se0a0_26c; /* 0x41E11C cmp esi,ecx; 0x41E11E jb (unsigned) */
se0a0_124: /* 0x41E124 loc_41E124 */
    esi = 0xFFFFFFFFUL; /* 0x41E124 mov esi,0FFFFFFFFh */
se0a0_129: /* 0x41E129 loc_41E129 */
    blb = M8(eax); /* 0x41E129 mov bl,[eax] */
    if (blb >= 0x40U) goto se0a0_273; /* 0x41E12B cmp bl,40h; 0x41E12E jnb (unsigned) */
se0a0_134: /* 0x41E134 loc_41E134 */
    ebx = 1; /* 0x41E134 mov ebx,1 */
se0a0_139: /* 0x41E139 loc_41E139 */
    eax++; /* 0x41E139 inc eax */
    edx += ebx; /* 0x41E13A add edx,ebx */
    ecx--; /* 0x41E13C dec ecx */
    if (ecx != 0) goto se0a0_129; /* 0x41E13D jnz */
    eax = v34; /* 0x41E13F mov eax,[var_34] */
    if (eax <= 0x100U) goto se0a0_171; /* 0x41E143 cmp eax,100h; 0x41E148 jbe (unsigned) */
    ecx = eax - 0x100U; /* 0x41E14A lea ecx,[eax-100h] */
    esi = 0xFFFFFFFFUL; /* 0x41E150 mov esi,0FFFFFFFFh */
    eax = v64; /* 0x41E155 mov eax,[var_64] */
se0a0_159: /* 0x41E159 loc_41E159 */
    blb = M8(eax); /* 0x41E159 mov bl,[eax] */
    blb = (unsigned char)~blb; /* 0x41E15B not bl */
    if (blb >= 0x40U) goto se0a0_283; /* 0x41E15D cmp bl,40h; 0x41E160 jnb (unsigned) */
se0a0_166: /* 0x41E166 loc_41E166 */
    ebx = 1; /* 0x41E166 mov ebx,1 */
se0a0_16b: /* 0x41E16B loc_41E16B */
    eax++; /* 0x41E16B inc eax */
    edx += ebx; /* 0x41E16C add edx,ebx */
    ecx--; /* 0x41E16E dec ecx */
    if (ecx != 0) goto se0a0_159; /* 0x41E16F jnz */
se0a0_171: /* 0x41E171 loc_41E171 */
    alb = ((int)edx > 0) ? 1 : 0; /* 0x41E171 test edx,edx; 0x41E173 setnle al (signed >0) */
    alb += 5; /* 0x41E176 add al,5 */
se0a0_178: /* 0x41E178 loc_41E178 */
    v14 = alb; /* 0x41E178 mov [var_14],al */
se0a0_17c: /* 0x41E17C loc_41E17C */
    edx = v14; /* 0x41E17C xor edx,edx; 0x41E17E mov dl,[var_14] */
    ecx = G32(0x43429C + edx * 4U); /* 0x41E182 */
    eax = G32(0x4342EC + edx * 4U); /* 0x41E189 */
    v54 = ecx; /* 0x41E190 mov [var_54],ecx */
    v30 = eax; /* 0x41E194 mov [var_30],eax */
    alb = v14; /* 0x41E198 mov al,[var_14] */
    ecx = G32(0x4342C4 + edx * 4U); /* 0x41E19C */
    edx <<= 8; /* 0x41E1A3 shl edx,8 */
    v5c = ecx; /* 0x41E1A6 mov [var_5C],ecx */
    edx += 0x30200U; /* 0x41E1AA add edx,30200h */
    clb = G8(0x45C67D); /* 0x41E1B0 mov cl,byte_45C67D */
    v38 = edx; /* 0x41E1B6 mov [var_38],edx */
    if (alb != clb) goto se0a0_383; /* 0x41E1BA cmp al,cl; 0x41E1BC jnz */
se0a0_1c2: /* 0x41E1C2 loc_41E1C2 (table setup copy + interleave) */
    edx = v30; /* 0x41E1C2 mov edx,[var_30] */
    ebx = v5c; /* 0x41E1C6 mov ebx,[var_5C] */
    edi = G32(0x433B80); /* 0x41E1CA */
    esi = G32(0x433B7C); /* 0x41E1D0 */
    edx <<= 9; /* 0x41E1D6 shl edx,9 */
    ebx <<= 9; /* 0x41E1D9 shl ebx,9 */
    ecx = edx; /* 0x41E1DC mov ecx,edx */
    esi += ebx; /* 0x41E1DE add esi,ebx */
    /* 0x41E1E0 cld (forward) */
    eax = ecx; /* 0x41E1E1 mov eax,ecx */
    ecx -= edi; /* 0x41E1E3 sub ecx,edi */
    ecx -= eax; /* 0x41E1E5 sub ecx,eax */
    ecx &= 3U; /* 0x41E1E7 and ecx,3 */
    eax -= ecx; /* 0x41E1EA sub eax,ecx */
    if ((int)eax > 0) { /* 0x41E1EC jle (signed): skip head+dwords */
        while (ecx) { M8(edi) = M8(esi); esi++; edi++; ecx--; } /* 0x41E1EE rep movsb */
        ecx = eax; /* 0x41E1F0 mov ecx,eax */
        eax &= 3U; /* 0x41E1F2 and eax,3 */
        ecx >>= 2; /* 0x41E1F5 shr ecx,2 */
        while (ecx) { M32(edi) = M32(esi); esi += 4; edi += 4; ecx--; } /* 0x41E1F8 rep movsd */
    }
se0a0_1fa: /* 0x41E1FA loc_41E1FA */
    ecx += eax; /* 0x41E1FA add ecx,eax */
    while (ecx) { M8(edi) = M8(esi); esi++; edi++; ecx--; } /* 0x41E1FC rep movsb */
    ecx = v30; /* 0x41E1FE mov ecx,[var_30] */
    edx += ebx; /* 0x41E202 add edx,ebx */
    ebx = v5c; /* 0x41E204 mov ebx,[var_5C] */
    ecx += ecx; /* 0x41E208 add ecx,ecx */
    ebx += ebx; /* 0x41E20A add ebx,ebx */
    eax = ecx; /* 0x41E20C mov eax,ecx */
    ecx += ebx; /* 0x41E20E add ecx,ebx */
se0a0_210: /* 0x41E210 loc_41E210 */
    ebx = v30; /* 0x41E210 mov ebx,[var_30] */
    edx -= 0x200U; /* 0x41E214 sub edx,200h */
    eax -= 2U; /* 0x41E21A sub eax,2 */
    ebx--; /* 0x41E21D dec ebx */
    ecx -= 2U; /* 0x41E21E sub ecx,2 */
    v30 = ebx; /* 0x41E221 mov [var_30],ebx */
    if (eax == 0xFFFFFFFEUL) goto se0a0_3b5; /* 0x41E225 cmp eax,0FFFFFFFEh; 0x41E228 jz */
    ebx = G32(0x433B84); /* 0x41E22E */
    esi = ebx + ecx; /* 0x41E234 lea esi,[ebx+ecx] */
    ebx = G32(0x433B8C); /* 0x41E237 */
    esi = (esi & 0xFFFF0000U) | M16(esi); /* 0x41E23D mov si,[esi] (stale high half: cleared 0x41E24C) */
    M16(ebx + eax) = (unsigned short)(esi & 0xFFFFU); /* 0x41E240 mov [ebx+eax],si */
    ebx = esi; /* 0x41E244 mov ebx,esi (full; high stale, dead) */
    esi = G32(0x433B78); /* 0x41E246 */
    ebx &= 0xFFFFU; /* 0x41E24C and ebx,0FFFFh */
    esi = (esi & 0xFFFF0000U) | M16(edx + esi); /* 0x41E252 mov si,[edx+esi] (esi fresh 0x41E246) */
    esi &= 0xFFFFU; /* 0x41E256 and esi,0FFFFh */
    ebx += esi; /* 0x41E25C add ebx,esi */
    esi = ebx; /* 0x41E25E mov esi,ebx */
    ebx = G32(0x433B88); /* 0x41E260 */
    M16(ebx + eax) = (unsigned short)(esi & 0xFFFFU); /* 0x41E266 mov [ebx+eax],si */
    goto se0a0_210; /* 0x41E26A jmp */
se0a0_26c: /* 0x41E26C loc_41E26C */
    ecx = esi; /* 0x41E26C mov ecx,esi */
    goto se0a0_124; /* 0x41E26E jmp */
se0a0_273: /* 0x41E273 loc_41E273 */
    if (blb >= 0xC0U) goto se0a0_134; /* 0x41E273 cmp bl,0C0h; 0x41E276 jnb (unsigned) */
    ebx = esi; /* 0x41E27C mov ebx,esi (=-1) */
    goto se0a0_139; /* 0x41E27E jmp */
se0a0_283: /* 0x41E283 loc_41E283 */
    if (blb >= 0xC0U) goto se0a0_166; /* 0x41E283 cmp bl,0C0h; 0x41E286 jnb (unsigned) */
    ebx = esi; /* 0x41E28C mov ebx,esi (=-1) */
    goto se0a0_16b; /* 0x41E28E jmp */
se0a0_293: /* 0x41E293 loc_41E293 */
    if (clb != 2) goto se0a0_379; /* 0x41E293 cmp cl,2; 0x41E296 jnz */
    edx = 0x100U; /* 0x41E29C mov edx,100h */
    ebx = v2c; /* 0x41E2A1 mov ebx,[var_2C] */
    edi = v34; /* 0x41E2A5 mov edi,[var_34] */
    eax = G32(0x433B40); /* 0x41E2A9 */
    esi = 0; /* 0x41E2AE xor esi,esi */
    ecx = 0; /* 0x41E2B0 xor ecx,ecx */
    eax += ebx; /* 0x41E2B2 add eax,ebx */
    if (edi < edx) goto se0a0_30f; /* 0x41E2B4 cmp edi,edx; 0x41E2B6 jb (unsigned) */
se0a0_2b8: /* 0x41E2B8 loc_41E2B8 */
    edi = 0xFFFFFFFFUL; /* 0x41E2B8 mov edi,0FFFFFFFFh */
se0a0_2bd: /* 0x41E2BD loc_41E2BD */
    blb = M8(eax); /* 0x41E2BD mov bl,[eax] */
    if (blb >= 0x40U) goto se0a0_313; /* 0x41E2BF cmp bl,40h; 0x41E2C2 jnb (unsigned) */
se0a0_2c4: /* 0x41E2C4 loc_41E2C4 */
    ebx = 1; /* 0x41E2C4 mov ebx,1 */
se0a0_2c9: /* 0x41E2C9 loc_41E2C9 */
    eax++; /* 0x41E2C9 inc eax */
    esi += ebx; /* 0x41E2CA add esi,ebx */
    edx--; /* 0x41E2CC dec edx */
    if (edx != 0) goto se0a0_31c; /* 0x41E2CD jnz */
se0a0_2cf: /* 0x41E2CF loc_41E2CF */
    edi = v34; /* 0x41E2CF mov edi,[var_34] */
    if (edi <= 0x100U) goto se0a0_2fe; /* 0x41E2D3 cmp edi,100h; 0x41E2D9 jbe (unsigned) */
    edx = edi - 0x100U; /* 0x41E2DB lea edx,[edi-100h] */
    eax = v64; /* 0x41E2E1 mov eax,[var_64] */
    edi = 0xFFFFFFFFUL; /* 0x41E2E5 mov edi,0FFFFFFFFh */
se0a0_2ea: /* 0x41E2EA loc_41E2EA */
    blb = M8(eax); /* 0x41E2EA mov bl,[eax] */
    blb = (unsigned char)~blb; /* 0x41E2EC not bl */
    if (blb >= 0x40U) goto se0a0_339; /* 0x41E2EE cmp bl,40h; 0x41E2F1 jnb (unsigned) */
se0a0_2f3: /* 0x41E2F3 loc_41E2F3 */
    ebx = 1; /* 0x41E2F3 mov ebx,1 */
se0a0_2f8: /* 0x41E2F8 loc_41E2F8 */
    eax++; /* 0x41E2F8 inc eax */
    esi += ebx; /* 0x41E2F9 add esi,ebx */
    edx--; /* 0x41E2FB dec edx */
    if (edx != 0) goto se0a0_342; /* 0x41E2FC jnz */
se0a0_2fe: /* 0x41E2FE loc_41E2FE */
    if ((int)esi <= 0) goto se0a0_36c; /* 0x41E2FE test esi,esi; 0x41E300 jle (signed) */
se0a0_306: /* 0x41E306 loc_41E306 */
    alb = 1; /* 0x41E306 mov al,1 */
    alb += 7; /* 0x41E308 add al,7 */
    goto se0a0_178; /* 0x41E30A jmp */
se0a0_30f: /* 0x41E30F loc_41E30F */
    edx = edi; /* 0x41E30F mov edx,edi */
    goto se0a0_2b8; /* 0x41E311 jmp */
se0a0_313: /* 0x41E313 loc_41E313 */
    if (blb >= 0xC0U) goto se0a0_2c4; /* 0x41E313 cmp bl,0C0h; 0x41E316 jnb (unsigned) */
    ebx = edi; /* 0x41E318 mov ebx,edi */
    goto se0a0_2c9; /* 0x41E31A jmp */
se0a0_31c: /* 0x41E31C loc_41E31C */
    blb = M8(eax); /* 0x41E31C mov bl,[eax] */
    if (blb >= 0x40U) goto se0a0_330; /* 0x41E31E cmp bl,40h; 0x41E321 jnb (unsigned) */
se0a0_323: /* 0x41E323 loc_41E323 */
    ebx = 1; /* 0x41E323 mov ebx,1 */
se0a0_328: /* 0x41E328 loc_41E328 */
    eax++; /* 0x41E328 inc eax */
    ecx += ebx; /* 0x41E329 add ecx,ebx */
    edx--; /* 0x41E32B dec edx */
    if (edx != 0) goto se0a0_2bd; /* 0x41E32C jnz */
    goto se0a0_2cf; /* 0x41E32E jmp */
se0a0_330: /* 0x41E330 loc_41E330 */
    if (blb >= 0xC0U) goto se0a0_323; /* 0x41E330 cmp bl,0C0h; 0x41E333 jnb (unsigned) */
    ebx = edi; /* 0x41E335 mov ebx,edi */
    goto se0a0_328; /* 0x41E337 jmp */
se0a0_339: /* 0x41E339 loc_41E339 */
    if (blb >= 0xC0U) goto se0a0_2f3; /* 0x41E339 cmp bl,0C0h; 0x41E33C jnb (unsigned) */
    ebx = edi; /* 0x41E33E mov ebx,edi */
    goto se0a0_2f8; /* 0x41E340 jmp */
se0a0_342: /* 0x41E342 loc_41E342 */
    blb = M8(eax); /* 0x41E342 mov bl,[eax] */
    blb = (unsigned char)~blb; /* 0x41E344 not bl */
    if (blb >= 0x40U) goto se0a0_363; /* 0x41E346 cmp bl,40h; 0x41E349 jnb (unsigned) */
se0a0_34b: /* 0x41E34B loc_41E34B */
    ebx = 1; /* 0x41E34B mov ebx,1 */
se0a0_350: /* 0x41E350 loc_41E350 */
    eax++; /* 0x41E350 inc eax */
    ecx += ebx; /* 0x41E351 add ecx,ebx */
    edx--; /* 0x41E353 dec edx */
    if (edx != 0) goto se0a0_2ea; /* 0x41E354 jnz */
    if ((int)esi <= 0) goto se0a0_36c; /* 0x41E356 test esi,esi; 0x41E358 jle (signed) */
    alb = 1; /* 0x41E35A mov al,1 */
    alb += 7; /* 0x41E35C add al,7 */
    goto se0a0_178; /* 0x41E35E jmp */
se0a0_363: /* 0x41E363 loc_41E363 */
    if (blb >= 0xC0U) goto se0a0_34b; /* 0x41E363 cmp bl,0C0h; 0x41E366 jnb (unsigned) */
    ebx = edi; /* 0x41E368 mov ebx,edi */
    goto se0a0_350; /* 0x41E36A jmp */
se0a0_36c: /* 0x41E36C loc_41E36C */
    if ((int)ecx > 0) goto se0a0_306; /* 0x41E36C test ecx,ecx; 0x41E36E jg (signed) */
    alb = 0; /* 0x41E370 xor al,al */
    alb += 7; /* 0x41E372 add al,7 */
    goto se0a0_178; /* 0x41E374 jmp */
se0a0_379: /* 0x41E379 loc_41E379 (order class 9) */
    v14 = 9; /* 0x41E379 mov [var_14],9 */
    goto se0a0_17c; /* 0x41E37E jmp */
se0a0_383: /* 0x41E383 loc_41E383 (order-class change: reset model) */
    eax = v5c; /* 0x41E383 mov eax,[var_5C] */
    chb = 0; /* 0x41E387 xor ch,ch (cl lane untouched, dead) */
    edx = 0; /* 0x41E389 xor edx,edx */
    G32(0x45AF3C) = eax; /* 0x41E38B */
    G8(0x45C67B) = chb; /* 0x41E390 */
    G8(0x45C679) = chb; /* 0x41E396 */
    alb = v14; /* 0x41E39C mov al,[var_14] */
    G32(0x45AF48) = edx; /* 0x41E3A0 */
    G8(0x45C67D) = alb; /* 0x41E3A6 */
    d4v_41e5f0(c); /* 0x41E3AB call sub_41E5F0 */
    goto se0a0_1c2; /* 0x41E3B0 jmp */
se0a0_3b5: /* 0x41E3B5 loc_41E3B5 (main loop setup) */
    eax = v38; /* 0x41E3B5 mov eax,[var_38] */
    eax += eax; /* 0x41E3B9 add eax,eax */
    esi = v64; /* 0x41E3BB mov esi,[var_64] */
    v60 = eax; /* 0x41E3BF mov [var_60],eax */
    eax = v24; /* 0x41E3C3 mov eax,[var_24] */
    eax += esi; /* 0x41E3C7 add eax,esi */
    v40 = eax; /* 0x41E3C9 mov [var_40],eax */
se0a0_3cd: /* 0x41E3CD loc_41E3CD */
    if (v24 >= 0x100U) goto se0a0_54c; /* 0x41E3CD cmp [var_24],100h; 0x41E3D5 jnb (unsigned) */
    edx = v2c; /* 0x41E3DB mov edx,[var_2C] */
    eax = G32(0x433B40); /* 0x41E3DF */
    eax += edx; /* 0x41E3E4 add eax,edx */
    bhb = M8(eax); /* 0x41E3E6 mov bh,[eax] */
se0a0_3e8: /* 0x41E3E8 loc_41E3E8 */
    ahb = G8(0x45C679); /* 0x41E3E8 mov ah,byte_45C679 */
    blb = bhb; /* 0x41E3EE mov bl,bh */
    dlb = G8(0x45C67B); /* 0x41E3F0 mov dl,byte_45C67B */
    blb -= ahb; /* 0x41E3F6 sub bl,ah */
    if (dlb == 0) goto se0a0_3fe; /* 0x41E3F8 test dl,dl; 0x41E3FA jz */
    blb = (unsigned char)(0U - blb); /* 0x41E3FC neg bl */
se0a0_3fe: /* 0x41E3FE loc_41E3FE */
    if (blb < 0x80U) goto se0a0_413; /* 0x41E3FE cmp bl,80h; 0x41E401 jb (unsigned) */
    dhb = G8(0x45C67B); /* 0x41E403 mov dh,byte_45C67B */
    alb = (dhb == 0) ? 1 : 0; /* 0x41E409 test dh,dh; 0x41E40B setz al */
    G8(0x45C67B) = alb; /* 0x41E40E */
se0a0_413: /* 0x41E413 loc_41E413 */
    esi = v5c; /* 0x41E413 mov esi,[var_5C] */
    ecx = G32(0x45AF3C); /* 0x41E417 */
    ecx -= esi; /* 0x41E41D sub ecx,esi */
    edx = 0; /* 0x41E41F xor edx,edx */
    eax = ecx; /* 0x41E421 mov eax,ecx */
    edx = blb; /* 0x41E423 mov dl,bl (edx was zero) */
    eax <<= 8; /* 0x41E425 shl eax,8 */
    eax += edx; /* 0x41E428 add eax,edx */
    esi = G32(0x433B80); /* 0x41E42A */
    eax += eax; /* 0x41E430 add eax,eax */
    esi += eax; /* 0x41E432 add esi,eax */
    v48 = esi; /* 0x41E434 mov [var_48],esi */
    eax = (eax & 0xFFFF0000U) | M16(esi); /* 0x41E438 mov ax,[esi] (stale high half: cleared 0x41E4CE or dead) */
    esi = ecx + ecx; /* 0x41E43B lea esi,[ecx+ecx] */
    v58 = esi; /* 0x41E43E mov [var_58],esi */
    edi = v58; /* 0x41E442 mov edi,[var_58] */
    esi = G32(0x433B88); /* 0x41E446 */
    edi += esi; /* 0x41E44C add edi,esi */
    esi = (esi & 0xFFFF0000U) | M16(edi); /* 0x41E44E mov si,[edi] (esi fresh 0x41E446) */
    v20 = esi; /* 0x41E451 mov [var_20],esi (full; high half dead: only low word read at 0x41E4D7) */
    esi = G32(0x45AF3C); /* 0x41E455 */
    v28 = edi; /* 0x41E45B mov [var_28],edi */
    esi = (esi & 0xFFFF0000U) | M16(0x45BA5E + esi * 2U); /* 0x41E45F mov si,word_45BA5E[esi*2] (high half: only si lane used) */
    if ((eax & 0xFFFFU) != 0) goto se0a0_4ce; /* 0x41E467 test ax,ax; 0x41E46A jnz */
    edi = v38; /* 0x41E46C mov edi,[var_38] */
    eax = G32(0x433B7C); /* 0x41E470 */
    edx += edi; /* 0x41E475 add edx,edi (edx=bl byte lane) */
    eax = (eax & 0xFFFF0000U) | M16(eax + edx * 2U); /* 0x41E477 mov ax,[eax+edx*2] */
    edi = v60; /* 0x41E47B mov edi,[var_60] */
    edx = G32(0x433B78); /* 0x41E47F */
    eax &= 0xFFFFU; /* 0x41E485 and eax,0FFFFh */
    edx += edi; /* 0x41E48A add edx,edi */
    eax <<= 14; /* 0x41E48C shl eax,0Eh */
    edx = (edx & 0xFFFF0000U) | M16(edx); /* 0x41E48F mov dx,[edx] */
    edx &= 0xFFFFU; /* 0x41E492 and edx,0FFFFh */
    v50 = edx; /* 0x41E498 mov [var_50],edx */
    edi = v50; /* 0x41E49C mov edi,[var_50] */
    edx = 0; /* 0x41E4A0 xor edx,edx */
    eax = eax / edi; /* 0x41E4A2 div edi (hw #DE on div-by-zero/overflow, kept direct) */
    eax = (eax & 0xFFFF0000U) | M16(0x437968 + eax * 2U); /* 0x41E4A4 mov ax,word_437968[eax*2] */
    eax &= 0xFFFFU; /* 0x41E4AC and eax,0FFFFh */
    edx = G32(0x433B8C); /* 0x41E4B1 */
    ebp += eax; /* 0x41E4B7 add ebp,eax */
    edx += v58; /* 0x41E4B9 add edx,[var_58] */
    eax = M16(edx); /* 0x41E4BD mov ax,[edx] (eax high is 0 from 0x41E4AC) */
    edi = eax; /* 0x41E4C0 mov edi,eax */
    edi += (esi & 0xFFFFU); /* 0x41E4C2 add edi,esi (only di stored at 0x41E4C4) */
    M16(edx) = (unsigned short)(edi & 0xFFFFU); /* 0x41E4C4 mov [edx],di */
    edx = v28; /* 0x41E4C7 mov edx,[var_28] */
    M16(edx) = (unsigned short)(M16(edx) + (esi & 0xFFFFU)); /* 0x41E4CB add [edx],si */
se0a0_4ce: /* 0x41E4CE loc_41E4CE */
    eax &= 0xFFFFU; /* 0x41E4CE and eax,0FFFFh */
    edi = 0; /* 0x41E4D3 xor edi,edi */
    edx = 0; /* 0x41E4D5 xor edx,edx */
    edi = (v20 & 0xFFFFU); /* 0x41E4D7 mov di,word ptr [var_20] (edi was zero) */
    eax <<= 14; /* 0x41E4DC shl eax,0Eh */
    eax = eax / edi; /* 0x41E4DF div edi (hw #DE on div-by-zero/overflow, kept direct) */
    eax = (eax & 0xFFFF0000U) | M16(0x437968 + eax * 2U); /* 0x41E4E1 mov ax,word_437968[eax*2] */
    eax &= 0xFFFFU; /* 0x41E4E9 and eax,0FFFFh */
    edx = v3c; /* 0x41E4EE mov edx,[var_3C] */
    ebp += eax; /* 0x41E4F2 add ebp,eax */
    if (ebp <= edx) goto se0a0_55d; /* 0x41E4F4 cmp ebp,edx; 0x41E4F6 jbe (unsigned) */
se0a0_4f8: /* 0x41E4F8 loc_41E4F8 (restore + return select) */
    eax = v44; /* 0x41E4F8 mov eax,[var_44] */
    G32(0x45AF38) = eax; /* 0x41E4FC */
    eax = v68; /* 0x41E501 mov eax,[var_68] */
    G32(0x45AF48) = eax; /* 0x41E504 */
    eax = v4c; /* 0x41E509 mov eax,[var_4C] */
    G32(0x45AF3C) = eax; /* 0x41E50D */
    alb = v10; /* 0x41E512 mov al,[var_10] */
    ecx = v3c; /* 0x41E516 mov ecx,[var_3C] */
    G8(0x45C67B) = alb; /* 0x41E51A */
    alb = v1c; /* 0x41E51F mov al,[var_1C] */
    blb = 0; /* 0x41E523 xor bl,bl */
    G8(0x45C67D) = alb; /* 0x41E525 */
    alb = v18; /* 0x41E52A mov al,[var_18] */
    G8(0x45C67E) = blb; /* 0x41E52E mov byte_45C67E,bl (=0) */
    G8(0x45C679) = alb; /* 0x41E534 */
    if (ebp >= ecx) goto se0a0_5dd; /* 0x41E539 cmp ebp,ecx; 0x41E53B jnb (unsigned) */
    alb = v14; /* 0x41E541 mov al,[var_14] */
    /* epilogue: 0x41E545 add esp,5Ch; 0x41E548 pop ebp; 0x41E549 pop edi;
     * 0x41E54A pop esi */
    return alb; /* 0x41E54B retn */
se0a0_54c: /* 0x41E54C loc_41E54C (history byte once var_24 >= 0x100) */
    ebx = v40; /* 0x41E54C mov ebx,[var_40] */
    bhb = M8(ebx - 0x100U); /* 0x41E550 mov bh,[ebx-100h] (other ebx bytes dead: only bl/bh used later) */
    bhb = (unsigned char)~bhb; /* 0x41E556 not bh */
    goto se0a0_3e8; /* 0x41E558 jmp */
se0a0_55d: /* 0x41E55D loc_41E55D (accept symbol, update model) */
    eax = v48; /* 0x41E55D mov eax,[var_48] */
    M16(eax) = (unsigned short)(M16(eax) + (esi & 0xFFFFU)); /* 0x41E561 add [eax],si */
    edx = G32(0x433B88); /* 0x41E564 */
    edi = (edi & 0xFFFF0000U) | M16(edx + ecx * 2U); /* 0x41E56A mov di,[edx+ecx*2] */
    edi += esi; /* 0x41E56E add edi,esi (full; only di observed below) */
    M16(edx + ecx * 2U) = (unsigned short)(edi & 0xFFFFU); /* 0x41E570 mov [edx+ecx*2],di */
    if ((edi & 0xFFFFU) < 0x4000U) goto se0a0_582; /* 0x41E574 cmp di,4000h; 0x41E579 jb (unsigned) */
    eax = ecx; /* 0x41E57B mov eax,ecx */
    d4v_41d480(c, eax); /* 0x41E57D call sub_41D480 */
se0a0_582: /* 0x41E582 loc_41E582 */
    edx = 0; /* 0x41E582 xor edx,edx */
    eax = 0; /* 0x41E584 xor eax,eax */
    edx = blb; /* 0x41E586 mov dl,bl (edx was zero) */
    eax = bhb; /* 0x41E588 mov al,bh (eax was zero) */
    d4v_41e9f0(c, eax, edx); /* 0x41E58A call sub_41E9F0 */
    ecx = G32(0x45AF38); /* 0x41E58F */
    ebx = v54; /* 0x41E595 mov ebx,[var_54] */
    ecx++; /* 0x41E599 inc ecx */
    edx = 0; /* 0x41E59A xor edx,edx */
    eax = ecx; /* 0x41E59C mov eax,ecx */
    edx = eax % ebx; /* 0x41E59E div ebx: remainder (hw #DE on div-by-zero, kept direct) */
    eax = eax / ebx; /* 0x41E59E div ebx: quotient (discarded afterwards) */
    esi = v2c; /* 0x41E5A0 mov esi,[var_2C] */
    edi = v40; /* 0x41E5A4 mov edi,[var_40] */
    esi++; /* 0x41E5A8 inc esi */
    edi++; /* 0x41E5A9 inc edi */
    v2c = esi; /* 0x41E5AA mov [var_2C],esi */
    G32(0x45AF38) = ecx; /* 0x41E5AE */
    G32(0x45AF48) = edx; /* 0x41E5B4 */
    d4v_41e640(c); /* 0x41E5BA call sub_41E640 */
    eax = v24; /* 0x41E5BF mov eax,[var_24] */
    v40 = edi; /* 0x41E5C3 mov [var_40],edi */
    eax++; /* 0x41E5C7 inc eax */
    edx = v34; /* 0x41E5C8 mov edx,[var_34] */
    v24 = eax; /* 0x41E5CC mov [var_24],eax */
    if (eax < edx) goto se0a0_3cd; /* 0x41E5D0 cmp eax,edx; 0x41E5D2 jb (unsigned) */
    goto se0a0_4f8; /* 0x41E5D8 jmp */
se0a0_5dd: /* 0x41E5DD loc_41E5DD (price lost: return 0) */
    alb = 0; /* 0x41E5DD xor al,al */
    /* epilogue: 0x41E5DF add esp,5Ch; 0x41E5E2 pop ebp; 0x41E5E3 pop edi;
     * 0x41E5E4 pop esi */
    return alb; /* 0x41E5E5 retn */
    /* 0x41E5E5 sub_41E0A0 endp */
}

/* END-S4A */
