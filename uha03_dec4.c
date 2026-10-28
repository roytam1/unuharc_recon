/* UHARC 0.3 decompressor core 4 (methods 22..39) - portable C89 port.
 *
 * Ported from the uharc_u.lst disassembly + uharc_u.c decompile of the
 * UHARC 0.3 (uharc_u) decoder (control flow and constants come straight
 * from the listing; the decompile was hints only).
 *
 * Original -> port mapping (uharc_u.lst):
 *   sub_40BD9C (decode dispatch, methods 22..35) -> uharc_dec4u_decode_b
 *   sub_415F6C (per-stream driver)     -> d3u_415f6c
 *   sub_413838 (startup: carve+memset) -> d3u_startup (draft_a3.c)
 *   sub_415FD4 (match decode)          -> d3u_415fd4 (draft_m3.c)
 *   sub_41680C (fallback decoder)      -> d3u_41680c (draft_f3.c)
 *   sub_4164B8/416640/4177C0          -> d3u_4164b8/16640/177c0 (draft_l3.c)
 *   sub_414290/414818 (top-ups)        -> d3u_414290/14818 (draft_t3.c)
 *   sub_423188/423140 (range coder)    -> d3u_423188/23140 (draft_r3.c)
 *   sub_41E2B0/41E17C (stream input)   -> d3u_41e2b0/1e17c (draft_r3.c)
 *   sub_41E3BC/41E300 (stream output)  -> d3u_41e3bc/1e300 (here)
 *   sub_40D010 (progress callback)     -> d3u_40d010 (no-op stub here)
 *   sub_40C4B3 (bump carve)            -> d3u_carve (draft_a3.c, static)
 *   sub_40C9C0 (memset)                -> d3u_fill (here)
 *
 * Memory model: the original carves ~21 blocks via sub_40C4B3 over the
 * pool cursor dword_436E48 and addresses everything (including DGROUP
 * globals) by absolute address. The port keeps one zeroed arena; every
 * original absolute address is used as an arena offset. Block pointers
 * (dword_4299B8..) are arena offsets handed out by the same carve order
 * and sizes as the original. The const slice uha03_dec4_dg.c (lifted
 * from uharc_u.exe) is copied 1:1 over 0x428000.. at init; BSS stays
 * calloc-zero.
 *
 * Register convention: x86 partial-register writes preserve the other
 * lanes, whose values the original inherits from caller leftovers. The C
 * port models each register as an unsigned variable, so every worker
 * auto is explicitly zero-initialised at declaration.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h> /* TEMP-PROGRESS (revert) */
#include <time.h> /* TEMP-PROGRESS (revert) */
#include "uharc_dec.h"

#define V3_ARENA 0x04000000UL


typedef struct dec3w_ctx {
    unsigned char *mem;
    unsigned long hi;
    uharc_dec_read_cb rcb;
    void *rctx;
    uharc_dec_write_cb wcb;
    void *wctx;
    int err;
    int method;
    unsigned long p_inb;
    unsigned long p_outb;
} dec3w_ctx;

static unsigned int d3w_o(unsigned int o)
{
    if (o > (unsigned)(V3_ARENA - 8)) o = (unsigned)(V3_ARENA - 8);
    return o;
}
#define V3O(o)  d3w_o((unsigned)(o))
#define G8(a)   (*(unsigned char  *)(void *)(c->mem + V3O(a)))
#define G16(a)  (*(unsigned short *)(void *)(c->mem + V3O(a)))
#define G32(a)  (*(unsigned int   *)(void *)(c->mem + V3O(a)))
#define M8(o)   (*(unsigned char  *)(void *)(c->mem + V3O(o)))
#define M16(o)  (*(unsigned short *)(void *)(c->mem + V3O(o)))
#define M32(o)  (*(unsigned int   *)(void *)(c->mem + V3O(o)))
#define U32MASK 0xFFFFFFFFUL

/* sub_40C9C0-style byte fill (ebx=len, eax=dst, edx=val). */
void d3u_fill(dec3w_ctx *c, unsigned int len, unsigned int dst,
              unsigned int val)
{
    memset(c->mem + V3O(dst), (int)(val & 0xFFU), (size_t)len);
}

/* ---- worker prototypes (bodies appended below) ---- */
int d3u_startup(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in);
void d3u_421284(dec3w_ctx *c, unsigned int ebx, unsigned int edx,
                unsigned int eax, unsigned int ecx, unsigned int esi,
                unsigned int edi);
void d3u_40cecc(dec3w_ctx *c);
void d3u_4230f0(dec3w_ctx *c);
unsigned int d3u_415fd4(dec3w_ctx *c, unsigned int edi_in);
void d3u_41680c(dec3w_ctx *c, unsigned int edi_in);
void d3u_4164b8(dec3w_ctx *c, unsigned int sym);
unsigned int d3u_416640(dec3w_ctx *c, unsigned int al_in);
unsigned int d3u_4177c0(dec3w_ctx *c, unsigned int eax_in);
void d3u_414290(dec3w_ctx *c);
void d3u_414818(dec3w_ctx *c);
void d3u_414d78(dec3w_ctx *c, unsigned int eax_in);
unsigned int d3u_423188(dec3w_ctx *c, unsigned int total);
void d3u_423140(dec3w_ctx *c, unsigned int p);
void d3u_42335c(dec3w_ctx *c);
void d3u_423388(dec3w_ctx *c);
unsigned int d3u_41e2b0(dec3w_ctx *c);
void d3u_41e1c0(dec3w_ctx *c, unsigned int eax_v);
void d3u_41e17c(dec3w_ctx *c);
void d3u_414ec0(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v);
void d3u_414fd4(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v);
void d3u_4153c0(dec3w_ctx *c, unsigned int eax_v);
void d3u_415534(dec3w_ctx *c, unsigned int eax_v);
void d3u_415800(dec3w_ctx *c, unsigned int eax_v);
void d3u_4159f4(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v);
void d3u_41855c(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v,
                unsigned int ebx_v);
void d3u_418620(dec3w_ctx *c, unsigned int edx_v, unsigned int ebx_v);
void d3u_41bfcc(dec3w_ctx *c);
unsigned int d3u_419f44(dec3w_ctx *c, unsigned int sym);
unsigned int d3u_416774(dec3w_ctx *c, unsigned int p);
unsigned int d3u_415058(dec3w_ctx *c);
unsigned int d3u_4174fc(dec3w_ctx *c, unsigned int eax_in,
                        unsigned int edx_in);
void d3u_4141d4(dec3w_ctx *c);
void d3u_41422c(dec3w_ctx *c);
void d3u_41b2d4(dec3w_ctx *c);
void d3u_423228(dec3w_ctx *c, unsigned int p);
unsigned int d3u_41a33c(dec3w_ctx *c, unsigned int eax_in,
                        unsigned int edx_in, unsigned int ecx_in,
                        unsigned int ebx_in);
unsigned int d3u_41adf0(dec3w_ctx *c, unsigned int eax_in,
                        unsigned int edx_in, unsigned int ecx_in,
                        unsigned int ebx_in);
unsigned int d3u_41a9c4(dec3w_ctx *c, unsigned int eax_in,
                        unsigned int edx_in);
unsigned int d3u_417d74(dec3w_ctx *c, unsigned int eax_in,
                        unsigned int dl_in);
unsigned int d3u_41a284(dec3w_ctx *c, unsigned int eax_in);
void d3u_41b318(dec3w_ctx *c);
void d3u_41b69c(dec3w_ctx *c, unsigned int al_in, unsigned int dl_in);
void d3u_415f6c(dec3w_ctx *c);
void d3u_424147(dec3w_ctx *c, unsigned int dst, unsigned int src,
                unsigned int n);
void d3u_41e300(dec3w_ctx *c);
void d3u_41e3bc(dec3w_ctx *c, unsigned int b);
void d3u_40d010(dec3w_ctx *c);
void d3u_41bef8(dec3w_ctx *c);

/* sub_41BEF8: decode primer. Seeds 4441E8/EC/F0, decodes via 423188/
 * 423140, accumulating 428EDC until the coder returns nonzero, then
 * publishes 444254. Loop trips 5x on gmud-03x (totals 2048+16k). */
void d3u_41bef8(dec3w_ctx *c)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int ebp = 0;
    /* 0x0041BEF8 push ebx/ecx/edx/esi/edi/ebp (frame only) */
    /* 0x0041BEFE mov edx,2 */ edx = 2;
    /* 0x0041BF03 mov eax,edx */ eax = edx;
    /* 0x0041BF05 mov dword_4441E8,edx */ G32(0x4441E8) = edx;
    /* 0x0041BF0B call sub_423188 */ eax = d3u_423188(c, eax & 0xFFFFUL);
    /* 0x0041BF10 mov edx,eax */ edx = eax;
    /* 0x0041BF12 mov byte_444EA0,al */ M8(0x444EA0) = (unsigned char)(eax & 0xFFU);
    /* 0x0041BF17 xor eax,eax */ eax = 0;
    /* 0x0041BF19 mov al,dl */ eax = (eax & ~0xFFUL) | (edx & 0xFFUL);
    /* 0x0041BF1B mov dword_4441EC,eax */ G32(0x4441EC) = eax;
    /* 0x0041BF20 xor eax,eax */ eax = 0;
    /* 0x0041BF22 mov al,dl */ eax = (eax & ~0xFFUL) | (edx & 0xFFUL);
    /* 0x0041BF24 inc eax */ eax = (eax + 1) & U32MASK;
    /* 0x0041BF25 mov dword_4441F0,eax */ G32(0x4441F0) = eax;
    /* 0x0041BF2A mov eax,offset dword_4441E8 */ eax = 0x4441E8U;
    /* 0x0041BF2F call sub_423140 */ d3u_423140(c, eax);
    /* 0x0041BF34 cmp byte_444EA0,0 */ /* 0x0041BF3B jnz loc_41BFC3 */
    if (M8(0x444EA0) != 0) goto loc_41BFC3;
    /* 0x0041BF41 mov esi,offset dword_4441E8 */ esi = 0x4441E8U;
    /* 0x0041BF46 mov ebp,2 */ ebp = 2;
    /* 0x0041BF4B xor ebx,ebx */ ebx = 0;
loc_41BF4D:
    /* 0x0041BF4D mov ecx,800h */ ecx = 0x800U;
    /* 0x0041BF52 xor edx,edx */ edx = 0;
    /* 0x0041BF54 mov eax,ecx */ eax = ecx;
    /* 0x0041BF56 mov dword_4441E8,ecx */ G32(0x4441E8) = ecx;
    /* 0x0041BF5C call sub_423188 */ eax = d3u_423188(c, eax & 0xFFFFUL);
    /* 0x0041BF61 mov dx,ax */ edx = (edx & ~0xFFFFUL) | (eax & 0xFFFFUL);
    /* 0x0041BF64 mov dword_4441EC,edx */ G32(0x4441EC) = edx;
    /* 0x0041BF6A lea eax,[edx+1] */ eax = (edx + 1) & U32MASK;
    /* 0x0041BF6D mov cl,bl */ ecx = (ecx & ~0xFFUL) | (ebx & 0xFFUL);
    /* 0x0041BF6F mov dword_4441F0,eax */ G32(0x4441F0) = eax;
    /* 0x0041BF74 mov eax,esi */ eax = esi;
    /* 0x0041BF76 shl edx,cl */ edx = (edx << (ecx & 31U)) & U32MASK;
    /* 0x0041BF78 call sub_423140 */ d3u_423140(c, eax);
    /* 0x0041BF7D mov edi,dword_428EDC */ edi = G32(0x428EDC);
    /* 0x0041BF83 mov dword_4441E8,ebp */ G32(0x4441E8) = ebp;
    /* 0x0041BF89 add edi,edx */ edi = (edi + edx) & U32MASK;
    /* 0x0041BF8B mov eax,ebp */ eax = ebp;
    /* 0x0041BF8D mov dword_428EDC,edi */ G32(0x428EDC) = edi;
    /* 0x0041BF93 xor edx,edx */ edx = 0;
    /* 0x0041BF95 call sub_423188 */ eax = d3u_423188(c, eax & 0xFFFFUL);
    /* 0x0041BF9A mov dx,ax */ edx = (edx & ~0xFFFFUL) | (eax & 0xFFFFUL);
    /* 0x0041BF9D mov dword_4441EC,edx */ G32(0x4441EC) = edx;
    /* 0x0041BFA3 lea eax,[edx+1] */ eax = (edx + 1) & U32MASK;
    /* 0x0041BFA6 mov dword_4441F0,eax */ G32(0x4441F0) = eax;
    /* 0x0041BFAB mov eax,esi */ eax = esi;
    /* 0x0041BFAD add ebx,0Bh */ ebx = (ebx + 0x0BUL) & U32MASK;
    /* 0x0041BFB0 call sub_423140 */ d3u_423140(c, eax);
    /* 0x0041BFB5 test edx,edx */ /* 0x0041BFB7 jnz loc_41BF4D */
    if (edx != 0) goto loc_41BF4D;
    /* 0x0041BFB9 mov eax,dword_428EDC */ eax = G32(0x428EDC);
    /* 0x0041BFBE mov dword_444254,eax */ G32(0x444254) = eax;
loc_41BFC3:;
}

/* sub_424147: memmove(dst=eax, src=edx, n=ebx) with overlap handling.
 * C memmove is exact (backward/forward as needed). */
void d3u_424147(dec3w_ctx *c, unsigned int dst, unsigned int src,
                unsigned int n)
{
    memmove(c->mem + V3O(dst), c->mem + V3O(src), (size_t)n);
}

/* sub_41E300: flush staged output [42C9AC..+444EEC) via the write
 * callback (dword_42CBB4). Snapshots 444EF4->436E40, 444EF8->436E44;
 * short write sets 42C9A0=4; 444EEC=0 either way. */
void d3u_41e300(dec3w_ctx *c)
{
    unsigned int n;
    int wr;
    G32(0x436E40) = G32(0x444EF4);
    G32(0x436E44) = G32(0x444EF8);
    n = G32(0x444EEC);
    wr = c->wcb(c->wctx, c->mem + G32(0x42C9AC), n);
    if (wr != 0)
        G32(0x42C9A0) = 4;
    G32(0x444EEC) = 0;
}

/* sub_41E3BC: stage output byte al at [42C9AC+444EEC]; 444EEC++;
 * 444EF8++; flush via 41E300 when 444EEC==444EE0. */
void d3u_41e3bc(dec3w_ctx *c, unsigned int b)
{
    unsigned int eec;
    eec = G32(0x444EEC);
    M8(G32(0x42C9AC) + eec) = (unsigned char)(b & 0xFFU);
    eec = (eec + 1) & U32MASK;
    G32(0x444EEC) = eec;
    G32(0x444EF8) = (G32(0x444EF8) + 1) & U32MASK;
    if (eec == G32(0x444EE0))
        d3u_41e300(c);
}

/* sub_40D010: progress/tick callback. Walks the file-boundary table and
 * polls for user cancel (sets 42C9A0=6). Decode runs unattended: no-op.
 * The abort flag is only ever set here on cancel, so loops run to end. */
void d3u_40d010(dec3w_ctx *c)
{
    (void)c;
}

/* sub_415F6C: per-stream decode driver (methods 22..35).
 * 42335C + 41BEF8 setup, then per block: top-ups 414290/414818,
 * 415FD4 match attempt (al==1) else 41680C fallback, 40D010 progress,
 * until 444EF8 >= 444EE4. Returns 444EF8. */
void d3u_415f6c(dec3w_ctx *c)
{
    unsigned int eax;
    d3u_42335c(c);
    d3u_41bef8(c);
    do {
        eax = G32(0x444EF8);
        if (eax >= G32(0x444254))
            d3u_414290(c);
        eax = G32(0x444EF8);
        if (eax >= G32(0x444230))
            d3u_414818(c);
        eax = d3u_415fd4(c, 0U);
        if ((eax & 0xFFU) == 0U)
            d3u_41680c(c, 0U);
        d3u_40d010(c);
        eax = G32(0x444EF8);
    } while (eax < G32(0x444EE4));
}

int uharc_dec4u_init(void **pctx, int method)
{
    dec3w_ctx *c;
    (void)method;
    c = (dec3w_ctx *)calloc(1, sizeof(*c));
    if (!c)
        return 2;
    c->mem = (unsigned char *)calloc(1, (size_t)V3_ARENA);
    if (!c->mem) {
        free(c);
        return 2;
    }
    /* Model blocks must live ABOVE the fixed DGROUP window (const slice
     * 0x428000-0x434CCC); bumping from 0x1000 paves over it (the dict
     * fill [0x1000,+0x800100) wipes consts/slots). Same fix as the 0.4
     * port: 0x600000 + ~28MB footprint < 64MB arena. Nonzero base also
     * keeps offset 0 as the alloc-fail NULL sentinel. */
    c->hi = 0x600000UL;
    {
        extern const unsigned char d3u_dgroup[];
        extern const unsigned long d3u_dgroup_base;
        extern const unsigned long d3u_dgroup_len;
        memcpy(c->mem + d3u_dgroup_base, d3u_dgroup,
               (size_t)d3u_dgroup_len);
    }
    /* stream buffers: input 1MB at 0x3800000, staging 64KB at 0x3D00000 */
    c->p_inb = 0x03800000UL;
    c->p_outb = 0x03D00000UL;
    c->method = method;
    *pctx = c;
    return 0;
}

int uharc_dec4u_decode_b(void *ctx,
                         const unsigned long *cum, unsigned long ncum,
                         uharc_dec_read_cb rcb, void *rctx,
                         uharc_dec_write_cb wcb, void *wctx,
                         unsigned long out_len)
{
    dec3w_ctx *c = (dec3w_ctx *)ctx;
    int method;
    (void)cum;
    (void)ncum;
    if (!c || !c->mem)
        return 1;
    method = c->method;
    c->rcb = rcb;
    c->rctx = rctx;
    c->wcb = wcb;
    c->wctx = wctx;
    c->err = 0;
    /* method comes from the container via init */
    if (d3u_startup(c, 0U, (unsigned int)method) != 0)
        return 2;
    G32(0x444EE4) = (unsigned int)out_len;
    /* Container cumulative file-end table (dword_44422C). The table lives
     * in the arena as 32-bit entries (the original was a malloc'ed int[]),
     * with one extra zero terminator entry. The top-up walker in
     * d3u_414818 consults it via dword_44422C; without it (cell 0) the
     * decoder takes the B7D fallback (EE4 threshold), top-up maintenance
     * never runs, models drift, and decode dies late with SIGFPE.
     * Same pattern as the 0.4 port's A_PROG staging. */
    if (cum && ncum) {
        unsigned long i;
        unsigned long base = c->hi;
        c->hi = base + 4UL * (ncum + 1UL);
        for (i = 0; i < ncum; i++)
            M32(base + 4UL * i) = (unsigned int)cum[i];
        M32(base + 4UL * ncum) = 0U;
        M32(0x44422C) = (unsigned int)base;
    } else {
        M32(0x44422C) = 0U;
    }
    G32(0x42C9A8) = (unsigned int)c->p_inb;
    G32(0x42C9AC) = (unsigned int)c->p_outb;
    G32(0x444EE0) = 0xFF00UL;
    G32(0x444EFC) = 0x40000UL;
    G32(0x444EF0) = 0;
    G32(0x444EE8) = 0;
    G32(0x444EF4) = 0;
    G32(0x42C9A0) = 0;
    G32(0x444EEC) = 0;
    G32(0x444EF8) = 0;
    d3u_415f6c(c);
    if (G32(0x444EEC) != 0)
        d3u_41e300(c);
    if (c->err)
        return c->err;
    if (G32(0x42C9A0) != 0)
        return 4;
    if (G32(0x444EF8) != (unsigned int)out_len)
        return 4;
    return 0;
}

void uharc_dec4u_free(void *ctx)
{
    dec3w_ctx *c = (dec3w_ctx *)ctx;
    if (!c)
        return;
    if (c->mem)
        free(c->mem);
    free(c);
}

/* ---- startup (from draft_a3.c) ---- */
/* bump-carve mirror of sub_40C4B3: carve `size` bytes, return arena
 * offset, or 0 on zero-size/wrap (mirrors test eax,eax -> mov eax,2). */
static unsigned int d3u_carve(dec3w_ctx *c, unsigned int size)
{
    unsigned long p = 0;
    p = c->hi;
    c->hi = p + (unsigned long)size;
    if (size == 0 || c->hi < p)
        return 0;
    return (unsigned int)p;
}
/* sub_4230F0 (0x004230F0): rebuild word_42D274 from word_42D272 deltas
 * while byte_435274 set, then clear it. Callee saves ebx/ecx/edx
 * (push/pop); all four regs written-before-read, so zero-init, no params. */
void d3u_4230f0(dec3w_ctx *c)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    /* 0x004230F0 push ebx */ /* 0x004230F1 push ecx */ /* 0x004230F2 push edx */
    /* 0x004230F0 cmp byte_435274,0 */
    /* 0x004230FA jz loc_42312F */
    if (G8(0x435274) != 0) {
        /* 0x004230FC mov eax,1 */ eax = 1UL;
    /* 0x00423101 loc_423101: */
    agn:
        /* 0x00423101 xor edx,edx */ edx = 0UL;
        /* 0x00423103 mov dx,ax */ edx = (edx & ~0xFFFFUL) | (eax & 0xFFFFUL);
        /* 0x00423106 mov bx,word_42D272[edx*2] */ ebx = (ebx & ~0xFFFFUL) | (unsigned long)M16(0x42D272 + (edx & 0xFFFFUL) * 2UL);
        /* 0x0042310E mov cx,word_42D274[edx*2] */ ecx = (ecx & ~0xFFFFUL) | (unsigned long)M16(0x42D274 + (edx & 0xFFFFUL) * 2UL);
        /* 0x00423116 add ecx,ebx */ ecx = (ecx + ebx) & U32MASK;
        /* 0x00423118 inc eax */ eax = (eax + 1UL) & U32MASK;
        /* 0x00423119 mov word_42D274[edx*2],cx */ M16(0x42D274 + (edx & 0xFFFFUL) * 2UL) = (unsigned short)(ecx & 0xFFFFUL);
        /* 0x00423121 cmp ax,4000h */
        /* 0x00423125 jb loc_423101 */ if ((eax & 0xFFFFUL) < 0x4000UL) goto agn;
        /* 0x00423127 xor dl,dl */ edx &= ~0xFFUL;
        /* 0x00423129 mov byte_435274,dl */ G8(0x435274) = (unsigned char)(edx & 0xFFUL);
    }
    /* 0x0042312F loc_42312F: */
    /* 0x0042312F pop edx */ /* 0x00423130 pop ecx */ /* 0x00423131 pop ebx */
    /* 0x00423132 retn */
}
/* sub_40CECC (0x0040CECC): pure load of dword_428F48 into eax.
 * Contract wants void; the eax live-out is materialized by the caller
 * at 0x413EBC (see d3u_startup). */
void d3u_40cecc(dec3w_ctx *c)
{
    /* 0x0040CECC mov eax,dword_428F48 */ (void)M32(0x428F48);
    /* 0x0040CED1 retn */
}
/* sub_421284 (0x00421284): word-pattern fill of ebx*2 bytes at edx
 * with low-16 of eax replicated. Callee saves/restores entry ecx/esi
 * (push/pop); edi untouched. Only ebx/edx/eax drive the fill. */
void d3u_421284(dec3w_ctx *c, unsigned int ebx, unsigned int edx, unsigned int eax, unsigned int ecx, unsigned int esi, unsigned int edi)
{
    unsigned int sv1 = 0;
    unsigned int sv2 = 0;
    unsigned int t16 = 0;
    (void)edi;
    /* 0x00421284 push ecx */
    /* 0x00421285 push esi */
    /* 0x00421286 mov esi,eax */ esi = eax;
    /* 0x00421288 mov eax,edx */ eax = edx;
    /* 0x0042128A test ebx,ebx */
    /* 0x0042128C jz loc_421332 */ if (ebx == 0) goto ddone;
    /* 0x00421292 mov ecx,ebx */ ecx = ebx;
    /* 0x00421294 mov edx,esi */ edx = esi;
    /* 0x00421296 push dx */ t16 = (unsigned int)(edx & 0xFFFFU);
    /* 0x00421298 shl edx,10h */ edx <<= 16;
    /* 0x0042129B pop dx */ edx = (unsigned int)((edx & 0xFFFF0000U) | t16);
    /* 0x0042129D add ecx,ecx */ ecx += ecx;
    /* 0x0042129F cmp [eax],dl (prefetch read, flags discarded) */ (void)G8(eax);
    /* 0x004212A1 loc_4212A1: */
dbyte:
    /* 0x004212A1 test al,3 */
    /* 0x004212A3 jz loc_4212AE */ if ((eax & 3U) == 0) goto ddword;
    /* 0x004212A5 mov [eax],dl */ G8(eax) = (unsigned char)(edx & 0xFFU);
    /* 0x004212A7 inc eax */ eax += 1U;
    /* 0x004212A8 ror edx,8 */ edx = (edx >> 8) | (edx << 24);
    /* 0x004212AB dec ecx */ ecx -= 1U;
    /* 0x004212AC jnz loc_4212A1 */ if (ecx != 0) goto dbyte;
    /* 0x004212AE loc_4212AE: */
ddword:
    /* 0x004212AE push ecx */ sv1 = ecx;
    /* 0x004212AF shr ecx,2 */ ecx >>= 2;
    /* 0x004212B2 jz loc_42131B */ if (ecx == 0) goto dtailb;
    /* 0x004212B4 loc_4212B4: */
dalign:
    /* 0x004212B4 test al,1Fh */
    /* 0x004212B6 jz loc_4212C0 */ if ((eax & 0x1FU) == 0) goto dblock;
    /* 0x004212B8 mov [eax],edx */ M32(eax) = edx;
    /* 0x004212BA lea eax,[eax+4] */ eax += 4U;
    /* 0x004212BD dec ecx */ ecx -= 1U;
    /* 0x004212BE jnz loc_4212B4 */ if (ecx != 0) goto dalign;
    /* 0x004212C0 loc_4212C0: */
dblock:
    /* 0x004212C0 push ecx */ sv2 = ecx;
    /* 0x004212C1 shr ecx,2 */ ecx >>= 2;
    /* 0x004212C4 jz loc_421300 */ if (ecx == 0) goto drem;
    /* 0x004212C6 dec ecx */ ecx -= 1U;
    /* 0x004212C7 jz loc_4212F2 */ if (ecx == 0) goto dfour;
    /* 0x004212C9 loc_4212C9: */
dbig:
    /* 0x004212C9 mov [eax],edx */ M32(eax) = edx;
    /* 0x004212CB mov [eax+4],edx */ M32(eax + 4U) = edx;
    /* 0x004212CE dec ecx */ ecx -= 1U;
    /* 0x004212CF mov [eax+8],edx */ M32(eax + 8U) = edx;
    /* 0x004212D2 mov [eax+0Ch],edx */ M32(eax + 0x0CU) = edx;
    /* 0x004212D5 jz loc_4212EF */ if (ecx == 0) goto dstep;
    /* 0x004212D7 cmp [eax+20h],dl (prefetch read, discarded) */ (void)G8(eax + 0x20U);
    /* 0x004212DA mov [eax+10h],edx */ M32(eax + 0x10U) = edx;
    /* 0x004212DD mov [eax+14h],edx */ M32(eax + 0x14U) = edx;
    /* 0x004212E0 dec ecx */ ecx -= 1U;
    /* 0x004212E1 mov [eax+18h],edx */ M32(eax + 0x18U) = edx;
    /* 0x004212E4 mov [eax+1Ch],edx */ M32(eax + 0x1CU) = edx;
    /* 0x004212E7 lea eax,[eax+20h] */ eax += 0x20U;
    /* 0x004212EA jnz loc_4212C9 */ if (ecx != 0) goto dbig;
    /* 0x004212EC lea eax,[eax-10h] */ eax -= 0x10U;
    /* 0x004212EF loc_4212EF: */
dstep:
    /* 0x004212EF lea eax,[eax+10h] */ eax += 0x10U;
    /* 0x004212F2 loc_4212F2: */
dfour:
    /* 0x004212F2 mov [eax],edx */ M32(eax) = edx;
    /* 0x004212F4 mov [eax+4],edx */ M32(eax + 4U) = edx;
    /* 0x004212F7 mov [eax+8],edx */ M32(eax + 8U) = edx;
    /* 0x004212FA mov [eax+0Ch],edx */ M32(eax + 0x0CU) = edx;
    /* 0x004212FD lea eax,[eax+10h] */ eax += 0x10U;
    /* 0x00421300 loc_421300: */
drem:
    /* 0x00421300 pop ecx */ ecx = sv2;
    /* 0x00421301 and ecx,3 */ ecx &= 3U;
    /* 0x00421304 jz loc_42131B */ if (ecx == 0) goto dtailb;
    /* 0x00421306 mov [eax],edx */ M32(eax) = edx;
    /* 0x00421308 lea eax,[eax+4] */ eax += 4U;
    /* 0x0042130B dec ecx */ ecx -= 1U;
    /* 0x0042130C jz loc_42131B */ if (ecx == 0) goto dtailb;
    /* 0x0042130E mov [eax],edx */ M32(eax) = edx;
    /* 0x00421310 lea eax,[eax+4] */ eax += 4U;
    /* 0x00421313 dec ecx */ ecx -= 1U;
    /* 0x00421314 jz loc_42131B */ if (ecx == 0) goto dtailb;
    /* 0x00421316 mov [eax],edx */ M32(eax) = edx;
    /* 0x00421318 lea eax,[eax+4] */ eax += 4U;
    /* 0x0042131B loc_42131B: */
dtailb:
    /* 0x0042131B pop ecx */ ecx = sv1;
    /* 0x0042131C and ecx,3 */ ecx &= 3U;
    /* 0x0042131F jz loc_421332 */ if (ecx == 0) goto ddone;
    /* 0x00421321 mov [eax],dl */ G8(eax) = (unsigned char)(edx & 0xFFU);
    /* 0x00421323 dec ecx */ ecx -= 1U;
    /* 0x00421324 jz loc_421332 */ if (ecx == 0) goto ddone;
    /* 0x00421326 mov [eax+1],dh */ G8(eax + 1U) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x00421329 dec ecx */ ecx -= 1U;
    /* 0x0042132A jz loc_421332 */ if (ecx == 0) goto ddone;
    /* 0x0042132C ror edx,8 */ edx = (edx >> 8) | (edx << 24);
    /* 0x0042132F mov [eax+2],dh */ G8(eax + 2U) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x00421332 loc_421332: */
ddone:
    /* 0x00421332 pop esi */
    /* 0x00421333 pop ecx */
    /* 0x00421334 retn */
    ;
}
/* sub_413838 (0x00413838): core-4 startup. Entry-live al (header byte,
 * stored to byte_444E99) and edx (method 22..35) arrive as params per
 * rule 6; all other regs are written-before-read, hence zero-init.
 * Returns 0 ok / 1 method out of range / 2 alloc-fail. */
int d3u_startup(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    unsigned long esi = 0;
    unsigned long edi = 0;
    unsigned long ebp = 0;
    unsigned long t_eax = 0;
    unsigned int p = 0;
    eax = (unsigned long)eax_in;
    edx = (unsigned long)edx_in;
    /* 0x00413838 push ebx */ /* 0x00413839 push ecx */ /* 0x0041383A push esi */
    /* 0x0041383B push edi */ /* 0x0041383C push ebp (callee-saved; C locals) */
    /* 0x0041383D mov esi,dword_444228 */ esi = (unsigned long)M32(0x444228);
    /* 0x00413843 cmp edx,16h */
    /* 0x00413846 jl loc_41384D (signed) */
    if (((edx & 0x80000000UL) != 0) || edx < 0x16UL) goto failrng;
    /* 0x00413848 cmp edx,23h */
    /* 0x0041384B jle loc_41385D (signed) */
    if (((edx & 0x80000000UL) != 0) || edx > 0x23UL) goto failrng;
    goto ok;
failrng:
    /* 0x0041384D loc_41384D: */
    /* 0x0041384D mov eax,1 */ eax = 1UL;
    /* 0x00413852 mov esi,dword_444228 */ esi = (unsigned long)M32(0x444228);
    /* 0x00413858 jmp loc_414069 */ goto epi;
ok:
    /* 0x0041385D loc_41385D: */
    /* 0x0041385D mov ah,1 */ eax = (eax & ~0xFF00UL) | 0x0100UL;
    /* 0x0041385F sub edx,16h */ edx = (edx - 0x16UL) & U32MASK;
    /* 0x00413862 mov byte_42BC6C,ah */ G8(0x42BC6C) = (unsigned char)((eax >> 8) & 0xFFUL);
    /* 0x00413868 mov byte_444E99,al */ G8(0x444E99) = (unsigned char)(eax & 0xFFUL);
    /* 0x0041386D call sub_4230F0 */ d3u_4230f0(c);
    /* 0x00413872 mov ebx,dword_436E48 */ ebx = (unsigned long)M32(0x436E48);
    /* 0x00413878 mov eax,edx */ eax = edx;
    /* 0x0041387A add ebx,16DB2h */ ebx = (ebx + 0x16DB2UL) & U32MASK;
    /* 0x00413880 sar edx,1Fh */ edx = ((edx & 0x80000000UL) != 0) ? 0xFFFFFFFFUL : 0UL;
    /* 0x00413883 mov dword_436E48,ebx */ M32(0x436E48) = (unsigned int)ebx;
    /* 0x00413889 mov ebx,0Eh */ ebx = 0x0EUL;
    /* 0x0041388E idiv ebx (div0/overflow fault impossible: ebx nonzero, tiny non-negative sign-extended dividend) */
    t_eax = eax; eax = (t_eax / ebx) & U32MASK; edx = (t_eax % ebx) & U32MASK;
    /* 0x00413890 add edx,0Ah */ edx = (edx + 0x0AUL) & U32MASK;
    /* 0x00413893 mov eax,1 */ eax = 1UL;
    /* 0x00413898 mov cl,dl */ ecx = (ecx & ~0xFFUL) | (edx & 0xFFUL);
    /* 0x0041389A shl eax,cl (count masked to 0x1F) */ eax = (eax << (ecx & 31UL)) & U32MASK;
    /* 0x0041389C mov dword_444270,eax */ M32(0x444270) = (unsigned int)eax;
    /* 0x004138A1 dec eax */ eax = (eax - 1UL) & U32MASK;
    /* 0x004138A2 mov ebp,dword_436E48 */ ebp = (unsigned long)M32(0x436E48);
    /* 0x004138A8 mov dword_444278,eax */ M32(0x444278) = (unsigned int)eax;
    /* 0x004138AD mov eax,dword_444270 */ eax = (unsigned long)M32(0x444270);
    /* 0x004138B2 xor ecx,ecx */ ecx = 0UL;
    /* 0x004138B4 add eax,100h */ eax = (eax + 0x100UL) & U32MASK;
    /* 0x004138B9 mov dword_444274,ecx */ M32(0x444274) = (unsigned int)ecx;
    /* 0x004138BF add ebp,eax */ ebp = (ebp + eax) & U32MASK;
    /* 0x004138C1 mov dword_444280,ecx */ M32(0x444280) = (unsigned int)ecx;
    /* 0x004138C7 mov dword_436E48,ebp */ M32(0x436E48) = (unsigned int)ebp;
    /* 0x004138CD call sub_40C4B3 (carve dict+0x100) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x004138D2 mov dword_4299B8,eax */ M32(0x4299B8) = p;
    /* 0x004138D7 test eax,eax */ /* 0x004138D9 jnz loc_4138F2 */ if (p == 0) goto afail;
    /* 0x004138F2 mov dx,word ptr dword_436E48+2 */ edx = (edx & 0xFFFF0000UL) | (unsigned long)M16(0x436E48 + 2);
    /* 0x004138F9 add edx,80h */ edx = (edx + 0x80UL) & U32MASK;
    /* 0x004138FF mov eax,800000h */ eax = 0x800000UL;
    /* 0x00413904 mov word ptr dword_436E48+2,dx */ M16(0x436E48 + 2) = (unsigned short)(edx & 0xFFFFUL);
    /* 0x0041390B call sub_40C4B3 (carve 0x800000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413910 mov dword_4299BC,eax */ M32(0x4299BC) = p;
    /* 0x00413915 test eax,eax */ /* 0x00413917 jnz loc_413930 */ if (p == 0) goto afail;
    /* 0x00413930 mov bx,word ptr dword_436E48+2 */ ebx = (ebx & 0xFFFF0000UL) | (unsigned long)M16(0x436E48 + 2);
    /* 0x00413937 add ebx,14h */ ebx = (ebx + 0x14UL) & U32MASK;
    /* 0x0041393A mov eax,140000h */ eax = 0x140000UL;
    /* 0x0041393F mov word ptr dword_436E48+2,bx */ M16(0x436E48 + 2) = (unsigned short)(ebx & 0xFFFFUL);
    /* 0x00413946 call sub_40C4B3 (carve 0x140000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x0041394B mov dword_4299C0,eax */ M32(0x4299C0) = p;
    /* 0x00413950 test eax,eax */ /* 0x00413952 jnz loc_41396B */ if (p == 0) goto afail;
    /* 0x0041396B mov cx,word ptr dword_436E48+2 */ ecx = (ecx & 0xFFFF0000UL) | (unsigned long)M16(0x436E48 + 2);
    /* 0x00413972 add ecx,20h */ ecx = (ecx + 0x20UL) & U32MASK;
    /* 0x00413975 mov eax,200000h */ eax = 0x200000UL;
    /* 0x0041397A mov word ptr dword_436E48+2,cx */ M16(0x436E48 + 2) = (unsigned short)(ecx & 0xFFFFUL);
    /* 0x00413981 call sub_40C4B3 (carve 0x200000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413986 mov dword_4299C4,eax */ M32(0x4299C4) = p;
    /* 0x0041398B test eax,eax */ /* 0x0041398D jnz loc_4139A6 */ if (p == 0) goto afail;
    /* 0x004139A6 mov di,word ptr dword_436E48+2 */ edi = (edi & 0xFFFF0000UL) | (unsigned long)M16(0x436E48 + 2);
    /* 0x004139AD add edi,20h */ edi = (edi + 0x20UL) & U32MASK;
    /* 0x004139B0 mov eax,200000h */ eax = 0x200000UL;
    /* 0x004139B5 mov word ptr dword_436E48+2,di */ M16(0x436E48 + 2) = (unsigned short)(edi & 0xFFFFUL);
    /* 0x004139BC call sub_40C4B3 (carve 0x200000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x004139C1 mov dword_4299C8,eax */ M32(0x4299C8) = p;
    /* 0x004139C6 test eax,eax */ /* 0x004139C8 jnz loc_4139E1 */ if (p == 0) goto afail;
    /* 0x004139E1 add word ptr dword_436E48+2,4 */ M16(0x436E48 + 2) = (unsigned short)(((unsigned long)M16(0x436E48 + 2) + 4UL) & 0xFFFFUL);
    /* 0x004139E9 mov eax,40000h */ eax = 0x40000UL;
    /* 0x004139EE call sub_40C4B3 (carve 0x40000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x004139F3 mov dword_4299CC,eax */ M32(0x4299CC) = p;
    /* 0x004139F8 test eax,eax */ /* 0x004139FA jnz loc_413A13 */ if (p == 0) goto afail;
    /* 0x00413A13 mov dx,word ptr dword_436E48+2 */ edx = (edx & 0xFFFF0000UL) | (unsigned long)M16(0x436E48 + 2);
    /* 0x00413A1A add edx,4 */ edx = (edx + 4UL) & U32MASK;
    /* 0x00413A1D mov eax,40000h */ eax = 0x40000UL;
    /* 0x00413A22 mov word ptr dword_436E48+2,dx */ M16(0x436E48 + 2) = (unsigned short)(edx & 0xFFFFUL);
    /* 0x00413A29 call sub_40C4B3 (carve 0x40000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413A2E mov dword_4299D0,eax */ M32(0x4299D0) = p;
    /* 0x00413A33 test eax,eax */ /* 0x00413A35 jnz loc_413A4E */ if (p == 0) goto afail;
    /* 0x00413A4E mov bx,word ptr dword_436E48+2 */ ebx = (ebx & 0xFFFF0000UL) | (unsigned long)M16(0x436E48 + 2);
    /* 0x00413A55 add ebx,8 */ ebx = (ebx + 8UL) & U32MASK;
    /* 0x00413A58 mov eax,80000h */ eax = 0x80000UL;
    /* 0x00413A5D mov word ptr dword_436E48+2,bx */ M16(0x436E48 + 2) = (unsigned short)(ebx & 0xFFFFUL);
    /* 0x00413A64 call sub_40C4B3 (carve 0x80000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413A69 mov dword_4299D4,eax */ M32(0x4299D4) = p;
    /* 0x00413A6E test eax,eax */ /* 0x00413A70 jnz loc_413A89 */ if (p == 0) goto afail;
    /* 0x00413A89 mov cx,word ptr dword_436E48+2 */ ecx = (ecx & 0xFFFF0000UL) | (unsigned long)M16(0x436E48 + 2);
    /* 0x00413A90 add ecx,8 */ ecx = (ecx + 8UL) & U32MASK;
    /* 0x00413A93 mov eax,80000h */ eax = 0x80000UL;
    /* 0x00413A98 mov word ptr dword_436E48+2,cx */ M16(0x436E48 + 2) = (unsigned short)(ecx & 0xFFFFUL);
    /* 0x00413A9F call sub_40C4B3 (carve 0x80000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413AA4 mov dword_4299D8,eax */ M32(0x4299D8) = p;
    /* 0x00413AA9 test eax,eax */ /* 0x00413AAB jnz loc_413AC4 */ if (p == 0) goto afail;
    /* 0x00413AC4 mov di,word ptr dword_436E48+2 */ edi = (edi & 0xFFFF0000UL) | (unsigned long)M16(0x436E48 + 2);
    /* 0x00413ACB add edi,4 */ edi = (edi + 4UL) & U32MASK;
    /* 0x00413ACE mov eax,40000h */ eax = 0x40000UL;
    /* 0x00413AD3 mov word ptr dword_436E48+2,di */ M16(0x436E48 + 2) = (unsigned short)(edi & 0xFFFFUL);
    /* 0x00413ADA call sub_40C4B3 (carve 0x40000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413ADF mov dword_4299DC,eax */ M32(0x4299DC) = p;
    /* 0x00413AE4 test eax,eax */ /* 0x00413AE6 jnz loc_413AFF */ if (p == 0) goto afail;
    /* 0x00413AFF add word ptr dword_436E48+2,4 */ M16(0x436E48 + 2) = (unsigned short)(((unsigned long)M16(0x436E48 + 2) + 4UL) & 0xFFFFUL);
    /* 0x00413B07 mov eax,40000h */ eax = 0x40000UL;
    /* 0x00413B0C call sub_40C4B3 (carve 0x40000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413B11 mov dword_4299E0,eax */ M32(0x4299E0) = p;
    /* 0x00413B16 test eax,eax */ /* 0x00413B18 jnz loc_413B31 */ if (p == 0) goto afail;
    /* 0x00413B31 mov dx,word ptr dword_436E48+2 */ edx = (edx & 0xFFFF0000UL) | (unsigned long)M16(0x436E48 + 2);
    /* 0x00413B38 add edx,4 */ edx = (edx + 4UL) & U32MASK;
    /* 0x00413B3B mov eax,40000h */ eax = 0x40000UL;
    /* 0x00413B40 mov word ptr dword_436E48+2,dx */ M16(0x436E48 + 2) = (unsigned short)(edx & 0xFFFFUL);
    /* 0x00413B47 call sub_40C4B3 (carve 0x40000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413B4C mov dword_4299E4,eax */ M32(0x4299E4) = p;
    /* 0x00413B51 test eax,eax */ /* 0x00413B53 jnz loc_413B6C */ if (p == 0) goto afail;
    /* 0x00413B6C mov bx,word ptr dword_436E48+2 */ ebx = (ebx & 0xFFFF0000UL) | (unsigned long)M16(0x436E48 + 2);
    /* 0x00413B73 add ebx,1Ch */ ebx = (ebx + 0x1CUL) & U32MASK;
    /* 0x00413B76 mov eax,1C0000h */ eax = 0x1C0000UL;
    /* 0x00413B7B mov word ptr dword_436E48+2,bx */ M16(0x436E48 + 2) = (unsigned short)(ebx & 0xFFFFUL);
    /* 0x00413B82 call sub_40C4B3 (carve 0x1C0000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413B87 mov dword_4299E8,eax */ M32(0x4299E8) = p;
    /* 0x00413B8C test eax,eax */ /* 0x00413B8E jnz loc_413BA7 */ if (p == 0) goto afail;
    /* 0x00413BA7 mov cx,word ptr dword_436E48+2 */ ecx = (ecx & 0xFFFF0000UL) | (unsigned long)M16(0x436E48 + 2);
    /* 0x00413BAE add ecx,4 */ ecx = (ecx + 4UL) & U32MASK;
    /* 0x00413BB1 mov eax,40000h */ eax = 0x40000UL;
    /* 0x00413BB6 mov word ptr dword_436E48+2,cx */ M16(0x436E48 + 2) = (unsigned short)(ecx & 0xFFFFUL);
    /* 0x00413BBD call sub_40C4B3 (carve 0x40000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413BC2 mov dword_4299EC,eax */ M32(0x4299EC) = p;
    /* 0x00413BC7 test eax,eax */ /* 0x00413BC9 jnz loc_413BE2 */ if (p == 0) goto afail;
    /* 0x00413BE2 add dword_436E48,61600h */ M32(0x436E48) = (unsigned int)(((unsigned long)M32(0x436E48) + 0x61600UL) & U32MASK);
    /* 0x00413BEC mov eax,61600h */ eax = 0x61600UL;
    /* 0x00413BF1 call sub_40C4B3 (carve 0x61600) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413BF6 mov dword_4299F0,eax */ M32(0x4299F0) = p;
    /* 0x00413BFB test eax,eax */ /* 0x00413BFD jnz loc_413C16 */ if (p == 0) goto afail;
    /* 0x00413C16 mov edx,dword_436E48 */ edx = (unsigned long)M32(0x436E48);
    /* 0x00413C1C add edx,61600h */ edx = (edx + 0x61600UL) & U32MASK;
    /* 0x00413C22 mov eax,61600h */ eax = 0x61600UL;
    /* 0x00413C27 mov dword_436E48,edx */ M32(0x436E48) = (unsigned int)edx;
    /* 0x00413C2D call sub_40C4B3 (carve 0x61600) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413C32 mov dword_4299F4,eax */ M32(0x4299F4) = p;
    /* 0x00413C37 test eax,eax */ /* 0x00413C39 jnz loc_413C52 */ if (p == 0) goto afail;
    /* 0x00413C52 mov ebx,dword_436E48 */ ebx = (unsigned long)M32(0x436E48);
    /* 0x00413C58 add ebx,0C000h */ ebx = (ebx + 0x0C000UL) & U32MASK;
    /* 0x00413C5E mov eax,0C000h */ eax = 0x0C000UL;
    /* 0x00413C63 mov dword_436E48,ebx */ M32(0x436E48) = (unsigned int)ebx;
    /* 0x00413C69 call sub_40C4B3 (carve 0xC000) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413C6E mov dword_4299F8,eax */ M32(0x4299F8) = p;
    /* 0x00413C73 test eax,eax */ /* 0x00413C75 jnz loc_413C8E */ if (p == 0) goto afail;
    /* 0x00413C8E mov ecx,dword_436E48 */ ecx = (unsigned long)M32(0x436E48);
    /* 0x00413C94 add ecx,5FAh */ ecx = (ecx + 0x5FAUL) & U32MASK;
    /* 0x00413C9A mov eax,5FAh */ eax = 0x5FAUL;
    /* 0x00413C9F mov dword_436E48,ecx */ M32(0x436E48) = (unsigned int)ecx;
    /* 0x00413CA5 call sub_40C4B3 (carve 0x5FA) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413CAA mov dword_4299FC,eax */ M32(0x4299FC) = p;
    /* 0x00413CAF test eax,eax */ /* 0x00413CB1 jnz loc_413CCA */ if (p == 0) goto afail;
    /* 0x00413CCA mov edi,dword_436E48 */ edi = (unsigned long)M32(0x436E48);
    /* 0x00413CD0 add edi,0C0h */ edi = (edi + 0x0C0UL) & U32MASK;
    /* 0x00413CD6 mov eax,0C0h */ eax = 0x0C0UL;
    /* 0x00413CDB mov dword_436E48,edi */ M32(0x436E48) = (unsigned int)edi;
    /* 0x00413CE1 call sub_40C4B3 (carve 0xC0) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413CE6 mov dword_429A00,eax */ M32(0x429A00) = p;
    /* 0x00413CEB test eax,eax */ /* 0x00413CED jnz loc_413D06 */ if (p == 0) goto afail;
    /* 0x00413D06 mov ebp,dword_436E48 */ ebp = (unsigned long)M32(0x436E48);
    /* 0x00413D0C add ebp,0C0h */ ebp = (ebp + 0x0C0UL) & U32MASK;
    /* 0x00413D12 mov eax,0C0h */ eax = 0x0C0UL;
    /* 0x00413D17 mov dword_436E48,ebp */ M32(0x436E48) = (unsigned int)ebp;
    /* 0x00413D1D call sub_40C4B3 (carve 0xC0) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413D22 mov dword_429A04,eax */ M32(0x429A04) = p;
    /* 0x00413D27 test eax,eax */ /* 0x00413D29 jnz loc_413D42 */ if (p == 0) goto afail;
    /* 0x00413D42 add dword_436E48,25800h */ M32(0x436E48) = (unsigned int)(((unsigned long)M32(0x436E48) + 0x25800UL) & U32MASK);
    /* 0x00413D4C mov eax,25800h */ eax = 0x25800UL;
    /* 0x00413D51 call sub_40C4B3 (carve 0x25800) */
    p = d3u_carve(c, (unsigned int)eax);
    /* 0x00413D56 mov dword_4441BC,eax */ M32(0x4441BC) = p;
    /* 0x00413D5B test eax,eax */ /* 0x00413D5D jnz loc_413D76 */ if (p == 0) goto afail;
    /* 0x00413D76 loc_413D76: */
    /* 0x00413D76 mov ebx,dword_444270 */ ebx = (unsigned long)M32(0x444270);
    /* 0x00413D7C mov eax,dword_4299B8 */ eax = (unsigned long)M32(0x4299B8);
    /* 0x00413D81 xor edx,edx */ edx = 0UL;
    /* 0x00413D83 add ebx,100h */ ebx = (ebx + 0x100UL) & U32MASK;
    /* 0x00413D89 call sub_40C9C0 (memset len=ebx dst=eax fill=edx) */
    memset(c->mem + eax, (int)(edx & 0xFFUL), (size_t)ebx);
    /* 0x00413D8E mov ebx,800000h */ ebx = 0x800000UL;
    /* 0x00413D93 mov eax,dword_4299BC */ eax = (unsigned long)M32(0x4299BC);
    /* 0x00413D98 xor edx,edx */ edx = 0UL;
    /* 0x00413D9A call sub_40C9C0 (memset len=ebx dst=eax fill=edx) */
    memset(c->mem + eax, (int)(edx & 0xFFUL), (size_t)ebx);
    /* 0x00413D9F mov ebx,140000h */ ebx = 0x140000UL;
    /* 0x00413DA4 mov eax,dword_4299C0 */ eax = (unsigned long)M32(0x4299C0);
    /* 0x00413DA9 xor edx,edx */ edx = 0UL;
    /* 0x00413DAB call sub_40C9C0 (memset len=ebx dst=eax fill=edx) */
    memset(c->mem + eax, (int)(edx & 0xFFUL), (size_t)ebx);
    /* 0x00413DB0 mov ebx,200000h */ ebx = 0x200000UL;
    /* 0x00413DB5 mov edx,0FFh */ edx = 0xFFUL;
    /* 0x00413DBA mov eax,dword_4299C4 */ eax = (unsigned long)M32(0x4299C4);
    /* 0x00413DBF call sub_40C9C0 (memset len=ebx dst=eax fill=edx) */
    memset(c->mem + eax, (int)(edx & 0xFFUL), (size_t)ebx);
    /* 0x00413DC4 mov ebx,200000h */ ebx = 0x200000UL;
    /* 0x00413DC9 mov edx,0FFh */ edx = 0xFFUL;
    /* 0x00413DCE mov eax,dword_4299C8 */ eax = (unsigned long)M32(0x4299C8);
    /* 0x00413DD3 call sub_40C9C0 (memset len=ebx dst=eax fill=edx) */
    memset(c->mem + eax, (int)(edx & 0xFFUL), (size_t)ebx);
    /* 0x00413DD8 mov ebx,40000h */ ebx = 0x40000UL;
    /* 0x00413DDD mov eax,dword_4299CC */ eax = (unsigned long)M32(0x4299CC);
    /* 0x00413DE2 xor edx,edx */ edx = 0UL;
    /* 0x00413DE4 call sub_40C9C0 (memset len=ebx dst=eax fill=edx) */
    memset(c->mem + eax, (int)(edx & 0xFFUL), (size_t)ebx);
    /* 0x00413DE9 mov ebx,1F400h */ ebx = 0x1F400UL;
    /* 0x00413DEE mov eax,dword_4299F0 */ eax = (unsigned long)M32(0x4299F0);
    /* 0x00413DF3 xor edx,edx */ edx = 0UL;
    /* 0x00413DF5 add eax,40600h */ eax = (eax + 0x40600UL) & U32MASK;
    /* 0x00413DFA call sub_40C9C0 (memset len=ebx dst=eax fill=edx) */
    memset(c->mem + eax, (int)(edx & 0xFFUL), (size_t)ebx);
    /* 0x00413DFF mov ebx,1F400h */ ebx = 0x1F400UL;
    /* 0x00413E04 mov eax,dword_4299F4 */ eax = (unsigned long)M32(0x4299F4);
    /* 0x00413E09 xor edx,edx */ edx = 0UL;
    /* 0x00413E0B add eax,40600h */ eax = (eax + 0x40600UL) & U32MASK;
    /* 0x00413E10 call sub_40C9C0 (memset len=ebx dst=eax fill=edx) */
    memset(c->mem + eax, (int)(edx & 0xFFUL), (size_t)ebx);
    /* 0x00413E15 mov ebx,0FAh */ ebx = 0xFAUL;
    /* 0x00413E1A mov edx,dword_4299FC */ edx = (unsigned long)M32(0x4299FC);
    /* 0x00413E20 mov eax,1 */ eax = 1UL;
    /* 0x00413E25 add edx,406h */ edx = (edx + 0x406UL) & U32MASK;
    /* 0x00413E2B call sub_421284 */
    d3u_421284(c, (unsigned int)ebx, (unsigned int)edx, (unsigned int)eax, (unsigned int)ecx, (unsigned int)esi, (unsigned int)edi);
    /* 0x00413E30 mov ebx,2FDh */ ebx = 0x2FDUL;
    /* 0x00413E35 mov edx,offset word_44428A */ edx = 0x44428AUL;
    /* 0x00413E3A mov eax,20h */ eax = 0x20UL;
    /* 0x00413E3F call sub_421284 */
    d3u_421284(c, (unsigned int)ebx, (unsigned int)edx, (unsigned int)eax, (unsigned int)ecx, (unsigned int)esi, (unsigned int)edi);
    /* 0x00413E44 mov ebx,2FDh */ ebx = 0x2FDUL;
    /* 0x00413E49 mov edx,offset word_444884 */ edx = 0x444884UL;
    /* 0x00413E4E mov eax,10h */ eax = 0x10UL;
    /* 0x00413E53 mov esi,6 */ esi = 6UL;
    /* 0x00413E58 call sub_421284 */
    d3u_421284(c, (unsigned int)ebx, (unsigned int)edx, (unsigned int)eax, (unsigned int)ecx, (unsigned int)esi, (unsigned int)edi);
    /* 0x00413E5D mov ebx,8 */ ebx = 8UL;
    /* 0x00413E62 mov edx,offset dword_444870 */ edx = 0x444870UL;
    /* 0x00413E67 mov eax,0Ah */ eax = 0x0AUL;
    /* 0x00413E6C xor ecx,ecx */ ecx = 0UL;
    /* 0x00413E6E call sub_421284 */
    d3u_421284(c, (unsigned int)ebx, (unsigned int)edx, (unsigned int)eax, (unsigned int)ecx, (unsigned int)esi, (unsigned int)edi);
    /* 0x00413E73 mov ebx,18h */ ebx = 0x18UL;
    /* 0x00413E78 mov edx,offset word_444E4A */ edx = 0x444E4AUL;
    /* 0x00413E7D mov eax,8 */ eax = 8UL;
    /* 0x00413E82 xor edi,edi */ edi = 0UL;
    /* 0x00413E84 call sub_421284 */
    d3u_421284(c, (unsigned int)ebx, (unsigned int)edx, (unsigned int)eax, (unsigned int)ecx, (unsigned int)esi, (unsigned int)edi);
    /* 0x00413E89 mov edx,0FFFFFFFFh */ edx = 0xFFFFFFFFUL;
    /* 0x00413E8E mov word_444C84,si */ M16(0x444C84) = (unsigned short)(esi & 0xFFFFUL);
    /* 0x00413E95 mov dword_444218,ecx */ M32(0x444218) = (unsigned int)ecx;
    /* 0x00413E9B xor ebx,ebx */ ebx = 0UL;
    /* 0x00413E9D mov dword_444254,edx */ M32(0x444254) = (unsigned int)edx;
    /* 0x00413EA3 mov dword_444244,edx */ M32(0x444244) = (unsigned int)edx;
    /* 0x00413EA9 mov dword_444228,edx */ M32(0x444228) = (unsigned int)edx;
    /* 0x00413EAF call sub_40CECC */ d3u_40cecc(c);
    /* (eax = dword_428F48 live-out of sub_40CECC) */ eax = (unsigned long)M32(0x428F48);
    /* 0x00413EB4 xor bl,cl */ ebx = (ebx & ~0xFFUL) | ((ebx ^ ecx) & 0xFFUL);
    /* 0x00413EB6 mov esi,dword_444228 */ esi = (unsigned long)M32(0x444228);
    /* 0x00413EBC mov dword_44422C,eax */ M32(0x44422C) = (unsigned int)eax;
    /* 0x00413EC1 mov dword_444230,ecx */ M32(0x444230) = (unsigned int)ecx;
    /* 0x00413EC7 mov dword_44426C,ecx */ M32(0x44426C) = (unsigned int)ecx;
    /* 0x00413ECD mov dword_444250,ecx */ M32(0x444250) = (unsigned int)ecx;
    /* 0x00413ED3 mov dword_444220,ecx */ M32(0x444220) = (unsigned int)ecx;
    /* 0x00413ED9 mov dword_444264,ecx */ M32(0x444264) = (unsigned int)ecx;
    /* 0x00413EDF mov dword_444258,ecx */ M32(0x444258) = (unsigned int)ecx;
    /* 0x00413EE5 mov dword_44425C,ecx */ M32(0x44425C) = (unsigned int)ecx;
    /* 0x00413EEB mov dword_444260,ecx */ M32(0x444260) = (unsigned int)ecx;
    /* 0x00413EF1 mov dword_444268,ecx */ M32(0x444268) = (unsigned int)ecx;
    /* 0x00413EF7 mov byte_444EA0,bl */ G8(0x444EA0) = (unsigned char)(ebx & 0xFFUL);
    /* 0x00413EFD mov dword_4441C0,ecx */ M32(0x4441C0) = (unsigned int)ecx;
    /* 0x00413F03 mov dword_4441D4,ecx */ M32(0x4441D4) = (unsigned int)ecx;
    /* 0x00413F09 mov dword_4441D0,ecx */ M32(0x4441D0) = (unsigned int)ecx;
    /* 0x00413F0F mov dword_440AF0,edi */ M32(0x440AF0) = (unsigned int)edi;
    /* 0x00413F15 mov dword_440AF4,edi */ M32(0x440AF4) = (unsigned int)edi;
    /* 0x00413F1B mov dword_4441B4,edi */ M32(0x4441B4) = (unsigned int)edi;
    /* 0x00413F21 xor edx,edx */ edx = 0UL;
    /* 0x00413F23 mov dword_4441B0,edi */ M32(0x4441B0) = (unsigned int)edi;
    /* 0x00413F29 mov dl,7 */ edx = (edx & ~0xFFUL) | 7UL;
    /* 0x00413F2B mov eax,dword_444EE4 */ eax = (unsigned long)M32(0x444EE4);
    /* 0x00413F30 xor bh,ch */ ebx = (ebx & ~0xFF00UL) | ((((ebx >> 8) & 0xFFUL) ^ ((ecx >> 8) & 0xFFUL)) << 8);
    /* 0x00413F32 mov byte_444EB6,dl */ G8(0x444EB6) = (unsigned char)(edx & 0xFFUL);
    /* 0x00413F38 mov byte_444EA7,bh */ G8(0x444EA7) = (unsigned char)((ebx >> 8) & 0xFFUL);
    /* 0x00413F3E mov dword_4441CC,eax */ M32(0x4441CC) = (unsigned int)eax;
    /* 0x00413F43 xor dh,ch */ edx = (edx & ~0xFF00UL) | ((((edx >> 8) & 0xFFUL) ^ ((ecx >> 8) & 0xFFUL)) << 8);
    /* 0x00413F45 xor eax,eax */ eax = 0UL;
    /* 0x00413F47 xor ebx,ebx */ ebx = 0UL;
    /* 0x00413F49 mov byte_444EA5,dh */ G8(0x444EA5) = (unsigned char)((edx >> 8) & 0xFFUL);
    /* 0x00413F4F mov dword_440AF8,eax */ M32(0x440AF8) = (unsigned int)eax;
    /* 0x00413F54 mov dword_440AFC,eax */ M32(0x440AFC) = (unsigned int)eax;
    /* 0x00413F59 mov dword_440B00,eax */ M32(0x440B00) = (unsigned int)eax;
    /* 0x00413F5E mov dword_440B04,eax */ M32(0x440B04) = (unsigned int)eax;
    /* 0x00413F63 xor cl,cl */ ecx &= ~0xFFUL;
    /* 0x00413F65 mov dword_4441B8,eax */ M32(0x4441B8) = (unsigned int)eax;
    /* 0x00413F6A mov byte_444EA6,cl */ G8(0x444EA6) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413F70 mov byte_444EAB,cl */ G8(0x444EAB) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413F76 mov byte_444EA8,cl */ G8(0x444EA8) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413F7C mov byte_444EA9,cl */ G8(0x444EA9) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413F82 mov byte_444E9D,cl */ G8(0x444E9D) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413F88 mov byte_444E9E,cl */ G8(0x444E9E) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413F8E mov byte_444E9F,cl */ G8(0x444E9F) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413F94 mov byte_444E9B,cl */ G8(0x444E9B) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413F9A mov byte_444E9C,cl */ G8(0x444E9C) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413FA0 mov byte_444284,cl */ G8(0x444284) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413FA6 mov byte_444285,cl */ G8(0x444285) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413FAC mov byte_444286,cl */ G8(0x444286) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413FB2 mov byte_444287,cl */ G8(0x444287) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413FB8 mov byte_444288,cl */ G8(0x444288) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413FBE mov byte_444289,cl */ G8(0x444289) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413FC4 xor ecx,ecx */ ecx = 0UL;
    /* 0x00413FC6 xor bh,ah */ ebx = (ebx & ~0xFF00UL) | ((((ebx >> 8) & 0xFFUL) ^ ((eax >> 8) & 0xFFUL)) << 8);
    /* 0x00413FC8 xor cl,al */ ecx = (ecx & ~0xFFUL) | (((ecx & 0xFFUL) ^ (eax & 0xFFUL)) & 0xFFUL);
    /* 0x00413FCA mov byte_444E9A,bh */ G8(0x444E9A) = (unsigned char)((ebx >> 8) & 0xFFUL);
    /* 0x00413FD0 mov byte_444E7E,cl */ G8(0x444E7E) = (unsigned char)(ecx & 0xFFUL);
    /* 0x00413FD6 xor ch,ah */ ecx = (ecx & ~0xFF00UL) | ((((ecx >> 8) & 0xFFUL) ^ ((eax >> 8) & 0xFFUL)) << 8);
    /* 0x00413FD8 xor al,al */ eax &= ~0xFFUL;
    /* 0x00413FDA mov byte_444E7F,ch */ G8(0x444E7F) = (unsigned char)((ecx >> 8) & 0xFFUL);
    /* 0x00413FE0 mov byte_444E80,al */ G8(0x444E80) = (unsigned char)(eax & 0xFFUL);
    /* 0x00413FE5 mov byte_444E81,al */ G8(0x444E81) = (unsigned char)(eax & 0xFFUL);
    /* 0x00413FEA mov byte_444EAF,al */ G8(0x444EAF) = (unsigned char)(eax & 0xFFUL);
    /* 0x00413FEF mov eax,0FFFFF800h */ eax = 0xFFFFF800UL;
    /* 0x00413FF4 loc_413FF4: */
lfill1:
    /* 0x00413FF4 mov ebx,eax */ ebx = eax;
    /* 0x00413FF6 mov edx,eax */ edx = eax;
    /* 0x00413FF8 and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x00413FFE neg edx */ edx = (0UL - edx) & U32MASK;
    /* 0x00414000 inc eax */ eax = (eax + 1UL) & U32MASK;
    /* 0x00414001 mov dword_4398B0[ebx*4],edx */ M32(0x4398B0 + ebx * 4UL) = (unsigned int)edx;
    /* 0x00414008 test eax,eax */
    /* 0x0041400A jl loc_413FF4 (signed) */ if ((eax & 0x80000000UL) != 0) goto lfill1;
    /* 0x0041400C xor eax,eax */ eax = 0UL;
    /* 0x0041400E loc_41400E: */
lfill2:
    /* 0x0041400E mov edx,eax */ edx = eax;
    /* 0x00414010 and edx,0FFFh */ edx &= 0xFFFUL;
    /* 0x00414016 mov dword_4398B0[edx*4],eax */ M32(0x4398B0 + edx * 4UL) = (unsigned int)eax;
    /* 0x0041401D inc eax */ eax = (eax + 1UL) & U32MASK;
    /* 0x0041401E cmp eax,800h */
    /* 0x00414023 jl loc_41400E (signed) */ if (((eax & 0x80000000UL) != 0) || eax < 0x800UL) goto lfill2;
    /* 0x00414025 cmp dword_44422C,0 */
    /* 0x0041402C jz loc_414055 */ if (M32(0x44422C) == 0) goto lskip;
    /* 0x0041402E cmp byte_428ED9,0 */
    /* 0x00414035 jz loc_414055 */ if (G8(0x428ED9) == 0) goto lskip;
    /* 0x00414037 xor esi,esi */ esi = 0UL;
    /* 0x00414039 xor edx,edx */ edx = 0UL;
    /* 0x0041403B loc_41403B: */
lscan:
    /* 0x0041403B mov ebx,dword_44422C */ ebx = (unsigned long)M32(0x44422C);
    /* 0x00414041 add ebx,edx */ ebx = (ebx + edx) & U32MASK;
    /* 0x00414043 mov eax,[ebx] */ eax = (unsigned long)M32((unsigned int)ebx);
    /* 0x00414045 sub eax,esi */ eax = (eax - esi) & U32MASK;
    /* 0x00414047 cmp eax,0FB670h */
    /* 0x0041404C jz loc_414055 */ if (eax == 0xFB670UL) goto lskip;
    /* 0x0041404E add edx,4 */ edx = (edx + 4UL) & U32MASK;
    /* 0x00414051 mov esi,[ebx] */ esi = (unsigned long)M32((unsigned int)ebx);
    /* 0x00414053 jmp loc_41403B */ goto lscan;
    /* 0x00414055 loc_414055: */
lskip:
    /* 0x00414055 test esi,esi */
    /* 0x00414057 jnz loc_414060 */ if (esi != 0) goto lret0;
    /* 0x00414059 mov eax,0Ah */ eax = 0x0AUL;
    /* 0x0041405E jmp loc_414062 */ goto lset;
    /* 0x00414060 loc_414060: */
lret0:
    /* 0x00414060 xor eax,eax */ eax = 0UL;
    /* 0x00414062 loc_414062: */
lset:
    /* 0x00414062 mov byte_444EA2,al */ G8(0x444EA2) = (unsigned char)(eax & 0xFFUL);
    /* 0x00414067 xor eax,eax */ eax = 0UL;
    /* 0x00414069 loc_414069: */
epi:
    /* 0x00414069 mov dword_444228,esi */ M32(0x444228) = (unsigned int)esi;
    /* 0x0041406F pop ebp */ /* 0x00414070 pop edi */ /* 0x00414071 pop esi */
    /* 0x00414072 pop ecx */ /* 0x00414073 pop ebx (restore; no-op in C) */
    /* 0x00414074 retn */ return (int)(eax & U32MASK);
afail:
    /* alloc-fail shared tail: mov eax,2; mov esi,dword_444228;
       mov dword_444228,esi; pops; retn (per site 0x4138DB..0x4138F1 etc.) */
    eax = 2UL;
    esi = (unsigned long)M32(0x444228);
    M32(0x444228) = (unsigned int)esi;
    return 2;
}

/* ---- match decoder (from draft_m3.c) ---- */
unsigned int d3u_415fd4(dec3w_ctx *c, unsigned int edi_in)
{
    unsigned long r_eax = 0;
    unsigned long r_ebx = 0;
    unsigned long r_ecx = 0;
    unsigned long r_edx = 0;
    unsigned long r_esi = 0;
    unsigned long r_edi = 0;
    unsigned long r_ebp = 0;
    unsigned long stk3c = 0;
    unsigned long stk38 = 0;
    unsigned long stk34 = 0;
    unsigned long stk30 = 0;
    unsigned long stk2c = 0;
    unsigned char stk28 = 0;
    unsigned char stk24 = 0;
    unsigned char stk20 = 0;
    unsigned char stk1c = 0;
    r_edi = (unsigned long)edi_in;
    /* 0x00415FD4 push ebx (callee-saved, no C equivalent) */
    /* 0x00415FD5 push ecx (callee-saved, no C equivalent) */
    /* 0x00415FD6 push edx (callee-saved, no C equivalent) */
    /* 0x00415FD7 push esi (callee-saved, no C equivalent) */
    /* 0x00415FD8 push edi (callee-saved, live entry value kept in r_edi) */
    /* 0x00415FD9 push ebp (callee-saved, no C equivalent) */
    /* 0x00415FDA sub esp,24h (stack frame, locals above) */
    /* 0x00415FDD mov eax,dword_444258 */ r_eax = (unsigned long)G32(0x444258UL);
    /* 0x00415FE2 mov edx,dword_444258 */ r_edx = (unsigned long)G32(0x444258UL);
    /* 0x00415FE8 shr eax,0Dh */ r_eax = (r_eax >> 13) & U32MASK;
    /* 0x00415FEB xor eax,edx */ r_eax = (r_eax ^ r_edx) & U32MASK;
    /* 0x00415FED and eax,3FFFFh */ r_eax &= 0x3FFFFUL;
    /* 0x00415FF2 mov edx,dword_4299C4 */ r_edx = (unsigned long)G32(0x4299C4UL);
    /* 0x00415FF8 mov [var_34],eax */ stk34 = r_eax;
    /* 0x00415FFC shl eax,3 */ r_eax = (r_eax << 3) & U32MASK;
    /* 0x00415FFF mov ebx,dword_44425C */ r_ebx = (unsigned long)G32(0x44425CUL);
    /* 0x00416005 add edx,eax */ r_edx = (r_edx + r_eax) & U32MASK;
    /* 0x00416007 mov ecx,dword_444258 */ r_ecx = (unsigned long)G32(0x444258UL);
    /* 0x0041600D mov [var_38],edx */ stk38 = r_edx;
    /* 0x00416011 mov edx,dword_44425C */ r_edx = (unsigned long)G32(0x44425CUL);
    /* 0x00416017 mov eax,dword_444258 */ r_eax = (unsigned long)G32(0x444258UL);
    /* 0x0041601C shr edx,0Fh */ r_edx = (r_edx >> 15) & U32MASK;
    /* 0x0041601F shr eax,0Fh */ r_eax = (r_eax >> 15) & U32MASK;
    /* 0x00416022 xor edx,ebx */ r_edx = (r_edx ^ r_ebx) & U32MASK;
    /* 0x00416024 xor eax,ecx */ r_eax = (r_eax ^ r_ecx) & U32MASK;
    /* 0x00416026 shl edx,4 */ r_edx = (r_edx << 4) & U32MASK;
    /* 0x00416029 xor edx,eax */ r_edx = (r_edx ^ r_eax) & U32MASK;
    /* 0x0041602B mov eax,dword_444260 */ r_eax = (unsigned long)G32(0x444260UL);
    /* 0x00416030 mov esi,dword_444260 */ r_esi = (unsigned long)G32(0x444260UL);
    /* 0x00416036 shr eax,0Fh */ r_eax = (r_eax >> 15) & U32MASK;
    /* 0x00416039 xor eax,esi */ r_eax = (r_eax ^ r_esi) & U32MASK;
    /* 0x0041603B shl eax,6 */ r_eax = (r_eax << 6) & U32MASK;
    /* 0x0041603E xor eax,edx */ r_eax = (r_eax ^ r_edx) & U32MASK;
    /* 0x00416040 and eax,3FFFFh */ r_eax &= 0x3FFFFUL;
    /* 0x00416045 mov cl,2 */ r_ecx = (r_ecx & ~0xFFUL) | 2UL;
    /* 0x00416047 shl eax,3 */ r_eax = (r_eax << 3) & U32MASK;
    /* 0x0041604A mov edx,dword_4299C8 */ r_edx = (unsigned long)G32(0x4299C8UL);
    /* 0x00416050 add eax,4 */ r_eax = (r_eax + 4UL) & U32MASK;
    /* 0x00416053 mov [var_1C],cl */ stk1c = (unsigned char)(r_ecx & 0xFFUL);
    /* 0x00416057 add edx,eax */ r_edx = (r_edx + r_eax) & U32MASK;
    /* 0x00416059 xor ah,ah */ r_eax &= ~0xFF00UL;
    /* 0x0041605B xor bh,bh */ r_ebx &= ~0xFF00UL;
    /* 0x0041605D mov [var_28],ah */ stk28 = (unsigned char)((r_eax >> 8) & 0xFFUL);
    /* 0x00416061 mov [var_24],ah */ stk24 = (unsigned char)((r_eax >> 8) & 0xFFUL);
    /* 0x00416065 mov eax,dword_444274 */ r_eax = (unsigned long)G32(0x444274UL);
    /* 0x0041606A mov [var_20],bh */ stk20 = (unsigned char)((r_ebx >> 8) & 0xFFUL);
    /* 0x0041606E mov [var_2C],eax */ stk2c = r_eax;
    /* 0x00416072 cmp eax,40h */ /* 0x00416075 jnb loc_416086 */
    if (r_eax >= 0x40UL) goto l_416086; /* jnb = unsigned >= */
    /* 0x00416077 mov ebx,[var_2C] */ r_ebx = stk2c;
    /* 0x0041607B mov eax,dword_444270 */ r_eax = (unsigned long)G32(0x444270UL);
    /* 0x00416080 add ebx,eax */ r_ebx = (r_ebx + r_eax) & U32MASK;
    /* 0x00416082 mov [var_2C],ebx */ stk2c = r_ebx;
l_416086:
    /* 0x00416086 loc_416086: */
    /* 0x00416086 mov eax,[var_2C] */ r_eax = stk2c;
    /* 0x0041608A dec eax */ r_eax = (r_eax - 1UL) & U32MASK;
    /* 0x0041608B mov [var_3C],eax */ stk3c = r_eax;
l_41608E:
    /* 0x0041608E loc_41608E: */
    /* 0x0041608E mov ch,[var_1C] */ r_ecx = (r_ecx & ~0xFF00UL) | ((unsigned long)stk1c << 8);
    /* 0x00416092 dec ch */ r_ecx = (r_ecx & ~0xFF00UL) | (((((r_ecx >> 8) & 0xFFUL) - 1UL) & 0xFFUL) << 8);
    /* 0x00416094 mov eax,[edx] */ r_eax = (unsigned long)M32((r_edx) & U32MASK);
    /* 0x00416096 mov [var_1C],ch */ stk1c = (unsigned char)((r_ecx >> 8) & 0xFFUL);
    /* 0x0041609A cmp eax,0FFFFFFFFh */ /* 0x0041609D jz loc_41616D */
    if (r_eax == 0xFFFFFFFFUL) goto l_41616D; /* jz = == */
    /* 0x004160A3 mov ecx,dword_444274 */ r_ecx = (unsigned long)G32(0x444274UL);
    /* 0x004160A9 mov ebx,eax */ r_ebx = r_eax;
    /* 0x004160AB mov esi,dword_444278 */ r_esi = (unsigned long)G32(0x444278UL);
    /* 0x004160B1 sub ebx,ecx */ r_ebx = (r_ebx - r_ecx) & U32MASK;
    /* 0x004160B3 and ebx,esi */ r_ebx = (r_ebx & r_esi) & U32MASK;
    /* 0x004160B5 cmp ebx,140h */ /* 0x004160BB jb loc_41616D */
    if (r_ebx < 0x140UL) goto l_41616D; /* jb = unsigned < */
    /* 0x004160C1 cmp eax,40h */ /* 0x004160C4 jnb loc_4160CC */
    if (r_eax >= 0x40UL) goto l_4160CC; /* jnb = unsigned >= */
    /* 0x004160C6 add eax,dword_444270 */ r_eax = (r_eax + (unsigned long)G32(0x444270UL)) & U32MASK;
l_4160CC:
    /* 0x004160CC loc_4160CC: */
    /* 0x004160CC mov esi,[var_2C] */ r_esi = stk2c;
    /* 0x004160D0 mov ebx,dword_4299B8 */ r_ebx = (unsigned long)G32(0x4299B8UL);
    /* 0x004160D6 mov cl,[esi+ebx-0Ch] */ r_ecx = (r_ecx & ~0xFFUL) | (unsigned long)M8((r_esi + r_ebx - 0x0CUL) & U32MASK);
    /* 0x004160DA cmp cl,[ebx+eax-0Ch] */ /* 0x004160DE jnz loc_41616D */
    if ((r_ecx & 0xFFUL) != (unsigned long)M8((r_ebx + r_eax - 0x0CUL) & U32MASK)) goto l_41616D; /* jnz = != */
    /* 0x004160E4 dec eax */ r_eax = (r_eax - 1UL) & U32MASK;
    /* 0x004160E5 lea esi,[ebx+eax] */ r_esi = (r_ebx + r_eax) & U32MASK;
    /* 0x004160E8 mov eax,[var_3C] */ r_eax = stk3c;
    /* 0x004160EB mov cl,40h */ r_ecx = (r_ecx & ~0xFFUL) | 0x40UL;
    /* 0x004160ED add ebx,eax */ r_ebx = (r_ebx + r_eax) & U32MASK;
l_4160EF:
    /* 0x004160EF loc_4160EF: */
    /* 0x004160EF cmp cl,8 */ /* 0x004160F2 jb loc_41614C */
    if ((r_ecx & 0xFFUL) < 8UL) goto l_41614C; /* jb = unsigned < */
    /* 0x004160F4 mov al,[ebx] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((r_ebx) & U32MASK);
    /* 0x004160F6 cmp al,[esi] */ /* 0x004160F8 jnz loc_41615D */
    if ((r_eax & 0xFFUL) != (unsigned long)M8((r_esi) & U32MASK)) goto l_41615D; /* jnz = != */
    /* 0x004160FA mov al,[ebx-1] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((r_ebx - 1UL) & U32MASK);
    /* 0x004160FD dec cl */ r_ecx = (r_ecx & ~0xFFUL) | ((r_ecx - 1UL) & 0xFFUL);
    /* 0x004160FF cmp al,[esi-1] */ /* 0x00416102 jnz loc_41615D */
    if ((r_eax & 0xFFUL) != (unsigned long)M8((r_esi - 1UL) & U32MASK)) goto l_41615D; /* jnz = != */
    /* 0x00416104 mov al,[ebx-2] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((r_ebx - 2UL) & U32MASK);
    /* 0x00416107 dec cl */ r_ecx = (r_ecx & ~0xFFUL) | ((r_ecx - 1UL) & 0xFFUL);
    /* 0x00416109 cmp al,[esi-2] */ /* 0x0041610C jnz loc_41615D */
    if ((r_eax & 0xFFUL) != (unsigned long)M8((r_esi - 2UL) & U32MASK)) goto l_41615D; /* jnz = != */
    /* 0x0041610E mov al,[ebx-3] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((r_ebx - 3UL) & U32MASK);
    /* 0x00416111 dec cl */ r_ecx = (r_ecx & ~0xFFUL) | ((r_ecx - 1UL) & 0xFFUL);
    /* 0x00416113 cmp al,[esi-3] */ /* 0x00416116 jnz loc_41615D */
    if ((r_eax & 0xFFUL) != (unsigned long)M8((r_esi - 3UL) & U32MASK)) goto l_41615D; /* jnz = != */
    /* 0x00416118 mov al,[ebx-4] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((r_ebx - 4UL) & U32MASK);
    /* 0x0041611B dec cl */ r_ecx = (r_ecx & ~0xFFUL) | ((r_ecx - 1UL) & 0xFFUL);
    /* 0x0041611D cmp al,[esi-4] */ /* 0x00416120 jnz loc_41615D */
    if ((r_eax & 0xFFUL) != (unsigned long)M8((r_esi - 4UL) & U32MASK)) goto l_41615D; /* jnz = != */
    /* 0x00416122 mov al,[ebx-5] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((r_ebx - 5UL) & U32MASK);
    /* 0x00416125 dec cl */ r_ecx = (r_ecx & ~0xFFUL) | ((r_ecx - 1UL) & 0xFFUL);
    /* 0x00416127 cmp al,[esi-5] */ /* 0x0041612A jnz loc_41615D */
    if ((r_eax & 0xFFUL) != (unsigned long)M8((r_esi - 5UL) & U32MASK)) goto l_41615D; /* jnz = != */
    /* 0x0041612C mov al,[ebx-6] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((r_ebx - 6UL) & U32MASK);
    /* 0x0041612F dec cl */ r_ecx = (r_ecx & ~0xFFUL) | ((r_ecx - 1UL) & 0xFFUL);
    /* 0x00416131 cmp al,[esi-6] */ /* 0x00416134 jnz loc_41615D */
    if ((r_eax & 0xFFUL) != (unsigned long)M8((r_esi - 6UL) & U32MASK)) goto l_41615D; /* jnz = != */
    /* 0x00416136 mov al,[ebx-7] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((r_ebx - 7UL) & U32MASK);
    /* 0x00416139 dec cl */ r_ecx = (r_ecx & ~0xFFUL) | ((r_ecx - 1UL) & 0xFFUL);
    /* 0x0041613B cmp al,[esi-7] */ /* 0x0041613E jnz loc_41615D */
    if ((r_eax & 0xFFUL) != (unsigned long)M8((r_esi - 7UL) & U32MASK)) goto l_41615D; /* jnz = != */
    /* 0x00416140 sub ebx,8 */ r_ebx = (r_ebx - 8UL) & U32MASK;
    /* 0x00416143 sub esi,8 */ r_esi = (r_esi - 8UL) & U32MASK;
    /* 0x00416146 dec cl */ r_ecx = (r_ecx & ~0xFFUL) | ((r_ecx - 1UL) & 0xFFUL);
    /* 0x00416148 jnz loc_4160EF */
    if ((r_ecx & 0xFFUL) != 0UL) goto l_4160EF; /* jnz = != */
    /* 0x0041614A jmp loc_41615D */ goto l_41615D;
l_41614C:
    /* 0x0041614C loc_41614C: */
    /* 0x0041614C sub esi,ebx */ r_esi = (r_esi - r_ebx) & U32MASK;
    /* 0x0041614E mov al,[ebx] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((r_ebx) & U32MASK);
l_416150:
    /* 0x00416150 loc_416150: */
    /* 0x00416150 cmp al,[ebx+esi] */ /* 0x00416153 jnz loc_41615D */
    if ((r_eax & 0xFFUL) != (unsigned long)M8((r_ebx + r_esi) & U32MASK)) goto l_41615D; /* jnz = != */
    /* 0x00416155 mov al,[ebx-1] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((r_ebx - 1UL) & U32MASK);
    /* 0x00416158 dec ebx */ r_ebx = (r_ebx - 1UL) & U32MASK;
    /* 0x00416159 dec cl */ r_ecx = (r_ecx & ~0xFFUL) | ((r_ecx - 1UL) & 0xFFUL);
    /* 0x0041615B jnz loc_416150 */
    if ((r_ecx & 0xFFUL) != 0UL) goto l_416150; /* jnz = != */
l_41615D:
    /* 0x0041615D loc_41615D: */
    /* 0x0041615D mov al,40h */ r_eax = (r_eax & ~0xFFUL) | 0x40UL;
    /* 0x0041615F sub al,cl */ r_eax = (r_eax & ~0xFFUL) | (((r_eax & 0xFFUL) - (r_ecx & 0xFFUL)) & 0xFFUL);
    /* 0x00416161 cmp al,[var_20] */ /* 0x00416165 jb loc_41616D */
    if ((r_eax & 0xFFUL) < (unsigned long)stk20) goto l_41616D; /* jb = unsigned < */
    /* 0x00416167 mov [var_20],al */ stk20 = (unsigned char)(r_eax & 0xFFUL);
    /* 0x0041616B mov edi,[edx] */ r_edi = (unsigned long)M32((r_edx) & U32MASK);
l_41616D:
    /* 0x0041616D loc_41616D: */
    /* 0x0041616D cmp [var_1C],0 */ /* 0x00416172 jz loc_416181 */
    if (stk1c == 0) goto l_416181; /* jz = == */
    /* 0x00416174 sub edx,4 */ r_edx = (r_edx - 4UL) & U32MASK;
    /* 0x00416177 mov eax,[edx] */ r_eax = (unsigned long)M32((r_edx) & U32MASK);
    /* 0x00416179 mov [edx+4],eax */ M32((r_edx + 4UL) & U32MASK) = (unsigned int)r_eax;
    /* 0x0041617C jmp loc_41608E */ goto l_41608E;
l_416181:
    /* 0x00416181 loc_416181: */
    /* 0x00416181 mov eax,dword_444274 */ r_eax = (unsigned long)G32(0x444274UL);
    /* 0x00416186 mov bh,[var_20] */ r_ebx = (r_ebx & ~0xFF00UL) | ((unsigned long)stk20 << 8);
    /* 0x0041618A mov [edx],eax */ M32((r_edx) & U32MASK) = (unsigned int)r_eax;
    /* 0x0041618C cmp bh,0Ch */ /* 0x0041618F jb loc_416196 */
    if (((r_ebx >> 8) & 0xFFUL) < 0x0CUL) goto l_416196; /* jb = unsigned < */
    /* 0x00416191 mov [var_28],11h */ stk28 = 0x11;
l_416196:
    /* 0x00416196 loc_416196: */
    /* 0x00416196 cmp [var_28],0 */ /* 0x0041619B jnz loc_416262 */
    if (stk28 != 0) goto l_416262; /* jnz = != */
    /* 0x004161A1 mov eax,dword_444274 */ r_eax = (unsigned long)G32(0x444274UL);
    /* 0x004161A6 mov edx,dword_444244 */ r_edx = (unsigned long)G32(0x444244UL);
    /* 0x004161AC mov edi,dword_444278 */ r_edi = (unsigned long)G32(0x444278UL);
    /* 0x004161B2 sub eax,edx */ r_eax = (r_eax - r_edx) & U32MASK;
    /* 0x004161B4 and edi,eax */ r_edi = (r_edi & r_eax) & U32MASK;
    /* 0x004161B6 mov ebx,dword_444274 */ r_ebx = (unsigned long)G32(0x444274UL);
    /* 0x004161BC mov eax,edi */ r_eax = r_edi;
    /* 0x004161BE mov ecx,dword_444278 */ r_ecx = (unsigned long)G32(0x444278UL);
    /* 0x004161C4 sub eax,ebx */ r_eax = (r_eax - r_ebx) & U32MASK;
    /* 0x004161C6 and eax,ecx */ r_eax = (r_eax & r_ecx) & U32MASK;
    /* 0x004161C8 cmp eax,104h */ /* 0x004161CD jb loc_41623C */
    if (r_eax < 0x104UL) goto l_41623C; /* jb = unsigned < */
    /* 0x004161CF lea eax,[edi-4] */ r_eax = (r_edi - 4UL) & U32MASK;
    /* 0x004161D2 mov edx,dword_4299B8 */ r_edx = (unsigned long)G32(0x4299B8UL);
    /* 0x004161D8 and eax,ecx */ r_eax = (r_eax & r_ecx) & U32MASK;
    /* 0x004161DA add eax,edx */ r_eax = (r_eax + r_edx) & U32MASK;
    /* 0x004161DC mov dl,[eax] */ r_edx = (r_edx & ~0xFFUL) | (unsigned long)M8((r_eax) & U32MASK);
    /* 0x004161DE mov [var_30+3],dl */ stk30 = (stk30 & ~0xFF000000UL) | ((r_edx & 0xFFUL) << 24);
    /* 0x004161E2 mov dl,[eax+1] */ r_edx = (r_edx & ~0xFFUL) | (unsigned long)M8((r_eax + 1UL) & U32MASK);
    /* 0x004161E5 mov [var_30+2],dl */ stk30 = (stk30 & ~0x00FF0000UL) | ((r_edx & 0xFFUL) << 16);
    /* 0x004161E9 mov dl,[eax+2] */ r_edx = (r_edx & ~0xFFUL) | (unsigned long)M8((r_eax + 2UL) & U32MASK);
    /* 0x004161EC mov al,[eax+3] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((r_eax + 3UL) & U32MASK);
    /* 0x004161EF mov [var_30+1],dl */ stk30 = (stk30 & ~0x0000FF00UL) | ((r_edx & 0xFFUL) << 8);
    /* 0x004161F3 mov [var_30],al */ stk30 = (stk30 & ~0x000000FFUL) | (r_eax & 0xFFUL);
    /* 0x004161F7 mov edx,[var_30] */ r_edx = stk30;
    /* 0x004161FB mov eax,dword_444258 */ r_eax = (unsigned long)G32(0x444258UL);
    /* 0x00416200 and edx,0FFFFh */ r_edx &= 0xFFFFUL;
    /* 0x00416206 and eax,0FFFFh */ r_eax &= 0xFFFFUL;
    /* 0x0041620B cmp edx,eax */ /* 0x0041620D jz loc_416232 */
    if (r_edx == r_eax) goto l_416232; /* jz = == */
    /* 0x0041620F mov eax,[var_30] */ r_eax = stk30;
    /* 0x00416213 mov edx,dword_444258 */ r_edx = (unsigned long)G32(0x444258UL);
    /* 0x00416219 xor ah,ah */ r_eax &= ~0xFF00UL;
    /* 0x0041621B xor dh,dh */ r_edx &= ~0xFF00UL;
    /* 0x0041621D cmp eax,edx */ /* 0x0041621F jz loc_416232 */
    if (r_eax == r_edx) goto l_416232; /* jz = == */
    /* 0x00416221 mov edx,[var_30] */ r_edx = stk30;
    /* 0x00416225 mov eax,dword_444258 */ r_eax = (unsigned long)G32(0x444258UL);
    /* 0x0041622A xor dl,dl */ r_edx &= ~0xFFUL;
    /* 0x0041622C xor al,al */ r_eax &= ~0xFFUL;
    /* 0x0041622E cmp edx,eax */ /* 0x00416230 jnz loc_41623C */
    if (r_edx != r_eax) goto l_41623C; /* jnz = != */
l_416232:
    /* 0x00416232 loc_416232: */
    /* 0x00416232 mov [var_28],9 */ stk28 = 9;
    /* 0x00416237 mov [var_24],1 */ stk24 = 1;
l_41623C:
    /* 0x0041623C loc_41623C: */
    /* 0x0041623C cmp [var_28],0 */ /* 0x00416241 jnz loc_416262 */
    if (stk28 != 0) goto l_416262; /* jnz = != */
    /* 0x00416243 mov eax,[var_38] */ r_eax = stk38;
    /* 0x00416247 mov ebp,dword_444258 */ r_ebp = (unsigned long)G32(0x444258UL);
    /* 0x0041624D cmp ebp,[eax+4] */ /* 0x00416250 jnz loc_416258 */
    if (r_ebp != (unsigned long)M32((r_eax + 4UL) & U32MASK)) goto l_416258; /* jnz = != */
    /* 0x00416252 mov edi,[var_38] */ r_edi = stk38;
    /* 0x00416256 mov edi,[edi] */ r_edi = (unsigned long)M32((r_edi) & U32MASK);
l_416258:
    /* 0x00416258 loc_416258: */
    /* 0x00416258 cmp edi,0FFFFFFFFh */ /* 0x0041625B setnz al */
    r_eax = (r_eax & ~0xFFUL) | ((r_edi != 0xFFFFFFFFUL) ? 1UL : 0UL);
    /* 0x0041625E mov [var_28],al */ stk28 = (unsigned char)(r_eax & 0xFFUL);
l_416262:
    /* 0x00416262 loc_416262: */
    /* 0x00416262 mov edx,[var_38] */ r_edx = stk38;
    /* 0x00416266 mov eax,dword_444274 */ r_eax = (unsigned long)G32(0x444274UL);
    /* 0x0041626B mov [edx],eax */ M32((r_edx) & U32MASK) = (unsigned int)r_eax;
    /* 0x0041626D mov eax,dword_444258 */ r_eax = (unsigned long)G32(0x444258UL);
    /* 0x00416272 mov [edx+4],eax */ M32((r_edx + 4UL) & U32MASK) = (unsigned int)r_eax;
    /* 0x00416275 cmp [var_28],0 */ /* 0x0041627A jz loc_4164AB */
    if (stk28 == 0) goto l_4164AB; /* jz = == */
    /* 0x00416280 mov edx,dword_444274 */ r_edx = (unsigned long)G32(0x444274UL);
    /* 0x00416286 mov eax,edi */ r_eax = r_edi;
    /* 0x00416288 mov ebx,dword_444278 */ r_ebx = (unsigned long)G32(0x444278UL);
    /* 0x0041628E sub eax,edx */ r_eax = (r_eax - r_edx) & U32MASK;
    /* 0x00416290 and eax,ebx */ r_eax = (r_eax & r_ebx) & U32MASK;
    /* 0x00416292 cmp eax,100h */ /* 0x00416297 jb loc_4164AB */
    if (r_eax < 0x100UL) goto l_4164AB; /* jb = unsigned < */
    /* 0x0041629D mov bl,[var_28] */ r_ebx = (r_ebx & ~0xFFUL) | (unsigned long)stk28;
    /* 0x004162A1 mov ecx,[var_34] */ r_ecx = stk34;
    /* 0x004162A5 mov eax,dword_4299CC */ r_eax = (unsigned long)G32(0x4299CCUL);
    /* 0x004162AA dec bl */ r_ebx = (r_ebx & ~0xFFUL) | ((r_ebx - 1UL) & 0xFFUL);
    /* 0x004162AC add eax,ecx */ r_eax = (r_eax + r_ecx) & U32MASK;
    /* 0x004162AE mov [var_28],bl */ stk28 = (unsigned char)(r_ebx & 0xFFUL);
    /* 0x004162B2 cmp byte ptr [eax],0 */ /* 0x004162B5 jnz loc_4162C0 */
    if (M8((r_eax) & U32MASK) != 0) goto l_4162C0; /* jnz = != */
    /* 0x004162B7 mov cl,bl */ r_ecx = (r_ecx & ~0xFFUL) | (r_ebx & 0xFFUL);
    /* 0x004162B9 add cl,18h */ r_ecx = (r_ecx & ~0xFFUL) | ((r_ecx + 0x18UL) & 0xFFUL);
    /* 0x004162BC mov [var_28],cl */ stk28 = (unsigned char)(r_ecx & 0xFFUL);
l_4162C0:
    /* 0x004162C0 loc_4162C0: */
    /* 0x004162C0 mov esi,dword_444218 */ r_esi = (unsigned long)G32(0x444218UL);
    /* 0x004162C6 test esi,esi */ /* 0x004162C8 jz loc_4162DA */
    if (r_esi == 0) goto l_4162DA; /* jz = == */
    /* 0x004162CA mov eax,dword_4299B8 */ r_eax = (unsigned long)G32(0x4299B8UL);
    /* 0x004162CF mov dl,[esi] */ r_edx = (r_edx & ~0xFFUL) | (unsigned long)M8((r_esi) & U32MASK);
    /* 0x004162D1 cmp dl,[edi+eax] */ /* 0x004162D4 jz loc_41648D */
    if ((r_edx & 0xFFUL) == (unsigned long)M8((r_edi + r_eax) & U32MASK)) goto l_41648D; /* jz = == */
l_4162DA:
    /* 0x004162DA loc_4162DA: */
    /* 0x004162DA xor eax,eax */ r_eax = 0;
    /* 0x004162DC mov al,[var_28] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)stk28;
    /* 0x004162E0 call sub_416640 */ r_eax = (r_eax & ~0xFFUL) | d3u_416640(c, (unsigned int)(r_eax & 0xFFUL));
    /* 0x004162E5 cmp al,1 */ /* 0x004162E7 jnz loc_41649D */
    if ((r_eax & 0xFFUL) != 1UL) goto l_41649D; /* jnz = != */
    /* 0x004162ED mov ah,[var_24] */ r_eax = (r_eax & ~0xFF00UL) | ((unsigned long)stk24 << 8);
    /* 0x004162F1 mov ebx,dword_444220 */ r_ebx = (unsigned long)G32(0x444220UL);
    /* 0x004162F7 test ah,ah */ /* 0x004162F9 setz al */
    r_eax = (r_eax & ~0xFFUL) | (((((r_eax >> 8) & 0xFFUL) == 0) ? 1UL : 0UL));
    /* 0x004162FC shl ebx,2 */ r_ebx = (r_ebx << 2) & U32MASK;
    /* 0x004162FF mov byte_444EAA,al */ M8(0x444EAAUL) = (unsigned char)(r_eax & 0xFFUL);
    /* 0x00416304 test al,al */ /* 0x00416306 jz loc_416350 */
    if ((r_eax & 0xFFUL) == 0UL) goto l_416350; /* jz = == */
    /* 0x00416308 mov ecx,dword_444250 */ r_ecx = (unsigned long)G32(0x444250UL);
    /* 0x0041630E mov al,7 */ r_eax = (r_eax & ~0xFFUL) | 7UL;
    /* 0x00416310 test ecx,ecx */ /* 0x00416312 jz loc_41632A */
    if (r_ecx == 0) goto l_41632A; /* jz = == */
    /* 0x00416314 mov eax,ebx */ r_eax = r_ebx;
    /* 0x00416316 xor edx,edx */ r_edx = 0;
    /* 0x00416318 div ecx */ r_eax = (r_ebx / r_ecx) & U32MASK; r_edx = (r_ebx % r_ecx) & U32MASK; /* x86 div faults on div0/overflow: direct / % */
    /* 0x0041631A mov dl,byte_444EA0 */ r_edx = (r_edx & ~0xFFUL) | (unsigned long)M8(0x444EA0UL);
    /* 0x00416320 mov al,byte_429BA8[eax] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((0x429BA8UL + r_eax) & U32MASK);
    /* 0x00416326 sub al,dl */ r_eax = (r_eax & ~0xFFUL) | (((r_eax & 0xFFUL) - (r_edx & 0xFFUL)) & 0xFFUL);
    /* 0x00416328 add al,7 */ r_eax = (r_eax & ~0xFFUL) | (((r_eax & 0xFFUL) + 7UL) & 0xFFUL);
l_41632A:
    /* 0x0041632A loc_41632A: */
    /* 0x0041632A mov edx,dword_444264 */ r_edx = (unsigned long)G32(0x444264UL);
    /* 0x00416330 cmp edx,dword_444250 */ /* 0x00416336 jbe loc_41633C */
    if (r_edx <= (unsigned long)G32(0x444250UL)) goto l_41633C; /* jbe = unsigned <= */
    /* 0x00416338 mov al,1Dh */ r_eax = (r_eax & ~0xFFUL) | 0x1DUL;
    /* 0x0041633A jmp loc_416347 */ goto l_416347;
l_41633C:
    /* 0x0041633C loc_41633C: */
    /* 0x0041633C cmp byte_444EAF,0 */ /* 0x00416343 jz loc_416347 */
    if (M8(0x444EAFUL) == 0) goto l_416347; /* jz = == */
    /* 0x00416345 add al,al */ r_eax = (r_eax & ~0xFFUL) | (((r_eax & 0xFFUL) + (r_eax & 0xFFUL)) & 0xFFUL);
l_416347:
    /* 0x00416347 loc_416347: */
    /* 0x00416347 mov byte_444E82,3 */ M8(0x444E82UL) = 3;
    /* 0x0041634E jmp loc_41636A */ goto l_41636A;
l_416350:
    /* 0x00416350 loc_416350: */
    /* 0x00416350 mov ebp,dword_444250 */ r_ebp = (unsigned long)G32(0x444250UL);
    /* 0x00416356 mov al,3 */ r_eax = (r_eax & ~0xFFUL) | 3UL;
    /* 0x00416358 test ebp,ebp */ /* 0x0041635A jz loc_41636A */
    if (r_ebp == 0) goto l_41636A; /* jz = == */
    /* 0x0041635C mov eax,ebx */ r_eax = r_ebx;
    /* 0x0041635E xor edx,edx */ r_edx = 0;
    /* 0x00416360 div ebp */ r_eax = (r_ebx / r_ebp) & U32MASK; r_edx = (r_ebx % r_ebp) & U32MASK; /* x86 div faults on div0/overflow: direct / % */
    /* 0x00416362 mov al,byte_429B98[eax] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)M8((0x429B98UL + r_eax) & U32MASK);
    /* 0x00416368 add al,3 */ r_eax = (r_eax & ~0xFFUL) | (((r_eax & 0xFFUL) + 3UL) & 0xFFUL);
l_41636A:
    /* 0x0041636A loc_41636A: */
    /* 0x0041636A mov ah,al */ r_eax = (r_eax & ~0xFF00UL) | ((r_eax & 0xFFUL) << 8);
    /* 0x0041636C dec al */ r_eax = (r_eax & ~0xFFUL) | ((r_eax - 1UL) & 0xFFUL);
    /* 0x0041636E cmp ah,7 */ /* 0x00416371 jbe loc_41638D */
    if (((r_eax >> 8) & 0xFFUL) <= 7UL) goto l_41638D; /* jbe = unsigned <= */
    /* 0x00416373 mov ah,byte_444EAA */ r_eax = (r_eax & ~0xFF00UL) | ((unsigned long)M8(0x444EAAUL) << 8);
    /* 0x00416379 xor edx,edx */ r_edx = 0;
    /* 0x0041637B mov dl,ah */ r_edx = (r_edx & ~0xFFUL) | ((r_eax >> 8) & 0xFFUL);
    /* 0x0041637D mov byte_444E82[edx],al */ M8((0x444E82UL + r_edx) & U32MASK) = (unsigned char)(r_eax & 0xFFUL);
    /* 0x00416383 inc ah */ r_eax = (r_eax & ~0xFF00UL) | (((((r_eax >> 8) & 0xFFUL) + 1UL) & 0xFFUL) << 8);
    /* 0x00416385 mov byte_444EAA,ah */ M8(0x444EAAUL) = (unsigned char)((r_eax >> 8) & 0xFFUL);
    /* 0x0041638B jmp loc_41636A */ goto l_41636A;
l_41638D:
    /* 0x0041638D loc_41638D: */
    /* 0x0041638D cmp [var_24],0 */ /* 0x00416392 jnz loc_4163AB */
    if (stk24 != 0) goto l_4163AB; /* jnz = != */
    /* 0x00416394 mov edx,[var_34] */ r_edx = stk34;
    /* 0x00416398 mov eax,dword_4299CC */ r_eax = (unsigned long)G32(0x4299CCUL);
    /* 0x0041639D add eax,edx */ r_eax = (r_eax + r_edx) & U32MASK;
    /* 0x0041639F cmp byte ptr [eax],10h */ /* 0x004163A2 jb loc_4163AB */
    if (M8((r_eax) & U32MASK) < 0x10UL) goto l_4163AB; /* jb = unsigned < */
    /* 0x004163A4 mov esi,1 */ r_esi = 1;
    /* 0x004163A9 jmp loc_4163AD */ goto l_4163AD;
l_4163AB:
    /* 0x004163AB loc_4163AB: */
    /* 0x004163AB xor esi,esi */ r_esi = 0;
l_4163AD:
    /* 0x004163AD loc_4163AD: */
    /* 0x004163AD mov ebx,30Ah */ r_ebx = 0x30AUL;
    /* 0x004163B2 add esi,2FBh */ r_esi = (r_esi + 0x2FBUL) & U32MASK;
    /* 0x004163B8 xor edx,edx */ r_edx = 0;
    /* 0x004163BA mov eax,esi */ r_eax = r_esi;
    /* 0x004163BC mov dword_44421C,ebx */ M32(0x44421CUL) = (unsigned int)r_ebx;
    /* 0x004163C2 call sub_4177C0 */ r_eax = (r_eax & ~0xFFUL) | d3u_4177c0(c, (unsigned int)r_eax);
    /* 0x004163C7 mov bl,al */ r_ebx = (r_ebx & ~0xFFUL) | (r_eax & 0xFFUL);
    /* 0x004163C9 mov cl,al */ r_ecx = (r_ecx & ~0xFFUL) | (r_eax & 0xFFUL);
    /* 0x004163CB mov dl,al */ r_edx = (r_edx & ~0xFFUL) | (r_eax & 0xFFUL);
    /* 0x004163CD mov eax,esi */ r_eax = r_esi;
    /* 0x004163CF call sub_414EC0 */ d3u_414ec0(c, (unsigned int)r_eax, (unsigned int)r_edx);
    /* 0x004163D4 mov esi,[var_34] */ r_esi = stk34;
    /* 0x004163D8 mov eax,dword_4299CC */ r_eax = (unsigned long)G32(0x4299CCUL);
    /* 0x004163DD mov edx,dword_444274 */ r_edx = (unsigned long)G32(0x444274UL);
    /* 0x004163E3 add eax,esi */ r_eax = (r_eax + r_esi) & U32MASK;
    /* 0x004163E5 sub edx,edi */ r_edx = (r_edx - r_edi) & U32MASK;
    /* 0x004163E7 mov [eax],bl */ M8((r_eax) & U32MASK) = (unsigned char)(r_ebx & 0xFFUL);
    /* 0x004163E9 mov eax,dword_444278 */ r_eax = (unsigned long)G32(0x444278UL);
    /* 0x004163EE xor ebp,ebp */ r_ebp = 0;
    /* 0x004163F0 and edx,eax */ r_edx = (r_edx & r_eax) & U32MASK;
    /* 0x004163F2 mov dword_444218,ebp */ M32(0x444218UL) = (unsigned int)r_ebp;
    /* 0x004163F8 mov dword_444244,edx */ M32(0x444244UL) = (unsigned int)r_edx;
    /* 0x004163FE cmp bl,7 */ /* 0x00416401 jb loc_416424 */
    if ((r_ebx & 0xFFUL) < 7UL) goto l_416424; /* jb = unsigned < */
    /* 0x00416403 cmp bl,0FFh */ /* 0x00416406 jnb loc_416424 */
    if ((r_ebx & 0xFFUL) >= 0xFFUL) goto l_416424; /* jnb = unsigned >= */
    /* 0x00416408 xor eax,eax */ r_eax = 0;
    /* 0x0041640A mov al,bl */ r_eax = (r_eax & ~0xFFUL) | (r_ebx & 0xFFUL);
    /* 0x0041640C mov edx,dword_444278 */ r_edx = (unsigned long)G32(0x444278UL);
    /* 0x00416412 add eax,edi */ r_eax = (r_eax + r_edi) & U32MASK;
    /* 0x00416414 and eax,edx */ r_eax = (r_eax & r_edx) & U32MASK;
    /* 0x00416416 mov edx,dword_4299B8 */ r_edx = (unsigned long)G32(0x4299B8UL);
    /* 0x0041641C add edx,eax */ r_edx = (r_edx + r_eax) & U32MASK;
    /* 0x0041641E mov dword_444218,edx */ M32(0x444218UL) = (unsigned int)r_edx;
l_416424:
    /* 0x00416424 loc_416424: */
    /* 0x00416424 mov eax,dword_4299B8 */ r_eax = (unsigned long)G32(0x4299B8UL);
    /* 0x00416429 xor ebx,ebx */ r_ebx = 0;
    /* 0x0041642B mov ch,[edi+eax] */ r_ecx = (r_ecx & ~0xFF00UL) | ((unsigned long)M8((r_edi + r_eax) & U32MASK) << 8);
    /* 0x0041642E mov bl,ch */ r_ebx = (r_ebx & ~0xFFUL) | ((r_ecx >> 8) & 0xFFUL);
    /* 0x00416430 mov eax,ebx */ r_eax = r_ebx;
    /* 0x00416432 xor edx,edx */ r_edx = 0;
    /* 0x00416434 call sub_4164B8 */ d3u_4164b8(c, (unsigned int)(r_eax & 0xFFUL));
    /* 0x00416439 cmp cl,8 */ /* 0x0041643C setbe al */
    r_eax = (r_eax & ~0xFFUL) | ((((r_ecx & 0xFFUL) <= 8UL) ? 1UL : 0UL));
    /* 0x0041643F mov dl,al */ r_edx = (r_edx & ~0xFFUL) | (r_eax & 0xFFUL);
    /* 0x00416441 mov eax,ebx */ r_eax = r_ebx;
    /* 0x00416443 call sub_4159F4 */ d3u_4159f4(c, (unsigned int)r_eax, (unsigned int)r_edx);
    /* 0x00416448 mov edx,dword_4299B8 */ r_edx = (unsigned long)G32(0x4299B8UL);
    /* 0x0041644E mov eax,dword_444274 */ r_eax = (unsigned long)G32(0x444274UL);
    /* 0x00416453 mov [edx+eax],ch */ M8((r_edx + r_eax) & U32MASK) = (unsigned char)((r_ecx >> 8) & 0xFFUL);
    /* 0x00416456 cmp eax,100h */ /* 0x0041645B jnb loc_416466 */
    if (r_eax >= 0x100UL) goto l_416466; /* jnb = unsigned >= */
    /* 0x0041645D add eax,dword_444270 */ r_eax = (r_eax + (unsigned long)G32(0x444270UL)) & U32MASK;
    /* 0x00416463 mov [edx+eax],ch */ M8((r_edx + r_eax) & U32MASK) = (unsigned char)((r_ecx >> 8) & 0xFFUL);
l_416466:
    /* 0x00416466 loc_416466: */
    /* 0x00416466 mov edx,dword_444274 */ r_edx = (unsigned long)G32(0x444274UL);
    /* 0x0041646C mov eax,dword_444278 */ r_eax = (unsigned long)G32(0x444278UL);
    /* 0x00416471 inc edx */ r_edx = (r_edx + 1UL) & U32MASK;
    /* 0x00416472 inc edi */ r_edi = (r_edi + 1UL) & U32MASK;
    /* 0x00416473 and edx,eax */ r_edx = (r_edx & r_eax) & U32MASK;
    /* 0x00416475 and edi,eax */ r_edi = (r_edi & r_eax) & U32MASK;
    /* 0x00416477 mov dword_444274,edx */ M32(0x444274UL) = (unsigned int)r_edx;
    /* 0x0041647D dec cl */ r_ecx = (r_ecx & ~0xFFUL) | ((r_ecx - 1UL) & 0xFFUL);
    /* 0x0041647F jnz loc_416424 */
    if ((r_ecx & 0xFFUL) != 0UL) goto l_416424; /* jnz = != */
    /* 0x00416481 mov al,1 */ r_eax = (r_eax & ~0xFFUL) | 1UL;
    /* 0x00416483 add esp,24h (stack release, no C equivalent) */
    /* 0x00416486 pop ebp (no C equivalent) */
    /* 0x00416487 pop edi (no C equivalent) */
    /* 0x00416488 pop esi (no C equivalent) */
    /* 0x00416489 pop edx (no C equivalent) */
    /* 0x0041648A pop ecx (no C equivalent) */
    /* 0x0041648B pop ebx (no C equivalent) */
    /* 0x0041648C retn */ return (r_eax & 0xFFUL);
l_41648D:
    /* 0x0041648D loc_41648D: */
    /* 0x0041648D xor eax,eax */ r_eax = 0;
    /* 0x0041648F mov edx,2 */ r_edx = 2;
    /* 0x00416494 mov al,[var_28] */ r_eax = (r_eax & ~0xFFUL) | (unsigned long)stk28;
    /* 0x00416498 call sub_414FD4 */ (void)d3u_414fd4(c, (unsigned int)(r_eax & 0xFFUL), (unsigned int)r_edx);
l_41649D:
    /* 0x0041649D loc_41649D: */
    /* 0x0041649D mov edi,[var_34] */ r_edi = stk34;
    /* 0x004164A1 mov eax,dword_4299CC */ r_eax = (unsigned long)G32(0x4299CCUL);
    /* 0x004164A6 add eax,edi */ r_eax = (r_eax + r_edi) & U32MASK;
    /* 0x004164A8 mov byte ptr [eax],0 */ M8((r_eax) & U32MASK) = 0;
l_4164AB:
    /* 0x004164AB loc_4164AB: */
    /* 0x004164AB xor al,al */ r_eax = (r_eax & ~0xFFUL) | 0UL;
    /* 0x004164AD add esp,24h (stack release, no C equivalent) */
    /* 0x004164B0 pop ebp (no C equivalent) */
    /* 0x004164B1 pop edi (no C equivalent) */
    /* 0x004164B2 pop esi (no C equivalent) */
    /* 0x004164B3 pop edx (no C equivalent) */
    /* 0x004164B4 pop ecx (no C equivalent) */
    /* 0x004164B5 pop ebx (no C equivalent) */
    /* 0x004164B6 retn */ return (r_eax & 0xFFUL);
}

/* ---- fallback decoder (from draft_f3.c) ---- */
void d3u_41680c(dec3w_ctx *c, unsigned int edi_in)
{
unsigned long eax = 0;
unsigned long ebx = 0;
unsigned long ecx = 0;
unsigned long edx = 0;
unsigned long esi = 0;
unsigned long edi = 0;
unsigned long ebp = 0;
unsigned long var_1C = 0;
unsigned long var_4C = 0;
unsigned long var_50 = 0;
unsigned long var_3C = 0;
unsigned long var_48 = 0;
unsigned long var_2C = 0;
unsigned long var_30 = 0;
unsigned long var_40 = 0;
unsigned long var_28 = 0;
unsigned long var_34 = 0;
unsigned long var_38 = 0;
unsigned long var_44 = 0;
unsigned long var_24 = 0;
unsigned long var_20 = 0;
unsigned char v68[13] = {0};
unsigned char v58[8] = {0};
unsigned long tmp = 0;
unsigned long q = 0;
unsigned long r = 0;
edi = (unsigned long)edi_in; /* 0x0041680C entry edi live */
eax = (eax & ~0xFFUL) | (((unsigned long)M8(0x444EA0)) & 0xFFUL); /* 0x00416815 mov al,byte_444EA0 */
var_1C = (eax & 0xFFUL) & 0xFFUL; /* 0x0041681A mov [var_1C],al */
edx = (unsigned long)M32(0x444258); /* 0x0041681E mov edx,dword_444258 */
edx &= U32MASK; /* lane init */
var_4C = 1UL; /* 0x00416824 mov [var_4C],1 */
eax = edx & U32MASK; /* 0x00416830 mov eax,edx */
M32(0x444240) = (unsigned int)(eax & U32MASK); /* 0x00416832 mov dword_444240,eax */
if ((unsigned long)M8(0x444EAF) != 2UL) goto L_41686F_b; /* 0x00416829 cmp / 0x00416837 jnz */
edx = ((edx >> 8) & U32MASK); /* 0x00416839 shr edx,8 */
eax = ((eax >> 16) & U32MASK); /* 0x0041683C shr eax,10h */
edx &= 0xFFUL; /* 0x0041683F and edx,0FFh */
eax &= 0xFF00UL; /* 0x00416845 and eax,0FF00h */
edx = (edx + eax) & U32MASK; /* 0x0041684A add edx,eax */
eax = (unsigned long)M32(0x44425C); /* 0x0041684C mov eax,dword_44425C */
eax = ((eax << 8) & U32MASK); /* 0x00416851 shl eax,8 */
eax &= 0xFF0000UL; /* 0x00416854 and eax,0FF0000h */
edx = (edx + eax) & U32MASK; /* 0x00416859 add edx,eax */
eax = (unsigned long)M32(0x44425C); /* 0x0041685B mov eax,dword_44425C */
eax &= 0xFF000000UL; /* 0x00416860 and eax,0FF000000h */
edx = (edx + eax) & U32MASK; /* 0x00416865 add edx,eax */
M32(0x444240) = (unsigned int)(edx & U32MASK); /* 0x00416867 mov dword_444240,edx */
eax = edx & U32MASK; /* 0x0041686D mov eax,edx */
L_41686F_b:;
eax = ((eax >> 13) & U32MASK); /* 0x0041686F shr eax,0Dh */
eax ^= edx; /* 0x00416872 xor eax,edx */
eax &= 0x7FFFUL; /* 0x00416874 and eax,7FFFh */
ebp = (unsigned long)M32(0x4299C0); /* 0x00416879 mov ebp,dword_4299C0 */
M32(0x444224) = (unsigned int)(eax & U32MASK); /* 0x0041687F mov dword_444224,eax */
eax = edx & U32MASK; /* 0x00416884 mov eax,edx */
esi = (unsigned long)M32(0x444218); /* 0x00416886 mov esi,dword_444218 */
eax &= 0xFFFFUL; /* 0x0041688C and eax,0FFFFh */
edx &= 0xFFFF00FFUL; /* 0x00416891 xor dh,dh (dh=0 lane-merge) */
edx = (edx & ~0xFF00UL) | (((0UL) & 0xFFUL) << 8);
M32(0x444238) = (unsigned int)(eax & U32MASK); /* 0x00416893 mov dword_444238,eax */
M32(0x444EB0) = 0; /* 0x00416898 mov dword ptr byte_444EB0,0 */
M8(0x444EA0 + 0x15) = 0; /* 0x004168A7 mov [eax+15h],dh */
M8(0x444EA0 + 0x0E) = 0; /* 0x004168AA mov [eax+0Eh],dh */
M8(0x444EA0 + 0x0C) = 0; /* 0x004168AD mov [eax+0Ch],dh */
M8(0x444EA0 + 0x01) = 0; /* 0x004168B0 mov [eax+1],dh */
M8(0x444EA0 + 0x04) = 0; /* 0x004168B3 mov [eax+4],dh */
M8(0x444EA0 + 0x0A) = 0; /* 0x004168B6 mov [eax+0Ah],dh */
eax = (unsigned long)M32(0x444238); /* 0x004168B9 mov eax,dword_444238 */
goto L_4168EA; /* 0x004168BE jmp */
L_4168EA:;
edx = eax & U32MASK; /* 0x004168EA mov edx,eax */
eax = ((eax << 2) & U32MASK); /* 0x004168EC shl eax,2 */
ebx = 0UL; /* 0x004168EF xor ebx,ebx */
eax = (eax + edx) & U32MASK; /* 0x004168F1 add eax,edx */
M32(0x444204) = 0; /* 0x004168F3 mov dword_444204,ebx */
eax = ((eax << 2) & U32MASK); /* 0x004168F9 shl eax,2 */
M32(0x44420C) = 0; /* 0x004168FC mov dword_44420C,ebx */
ebp = (ebp + eax) & U32MASK; /* 0x00416902 add ebp,eax */
if (esi == 0UL) goto L_41691D; /* 0x00416904 test esi,esi / 0x00416906 jz */
M32(0x444218) = 0; /* 0x00416908 mov dword_444218,ebx */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)esi)) & 0xFFUL); /* 0x0041690E mov al,[esi] UNCERTAIN: esi is arena-external pointer */
ebx = (ebx & 0xFFFF00FFUL) | (1UL << 8); /* 0x00416910 mov bh,1 lane-merge */
ebx = (ebx & ~0xFF00UL) | (((1UL) & 0xFFUL) << 8);
M8(0x444E82) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416912 mov byte_444E82,al */
M8(0x444EAA) = (unsigned char)(((ebx >> 8) & 0xFFUL) & 0xFFUL); /* 0x00416917 mov byte_444EAA,bh */
L_41691D:;
if (((unsigned long)M8(0x444EBA) > 1UL)) goto L_416931; /* 0x0041691D cmp / 0x00416924 ja (EBA>1 skips the EB9-gate block) */
eax = (eax & ~0xFFUL) | (((unsigned long)M8(0x444EB9)) & 0xFFUL); /* 0x00416926 mov al,byte_444EB9 */
eax = (eax & ~0xFFUL) | (((((eax & 0xFFUL) - 2) & 0xFFUL)) & 0xFFUL); /* 0x0041692B sub al,2 */
if (((eax & 0xFFUL) & 0xFFUL) > 2UL) goto L_416961; /* 0x0041692D cmp / 0x0041692F ja */
L_416931:;
ebx &= 0xFFFFFF00UL; /* 0x00416931 xor ebx,ebx(partial bl later) */
ebx = 0UL;
ebx = (ebx & ~0xFFUL) | (((unsigned long)M8(0x444EB9)) & 0xFFUL); /* 0x00416933 mov bl,byte_444EB9 */
ebx &= 0xFFUL;
if ((ebx & U32MASK) > 4UL) goto L_416952; /* 0x00416939 cmp / 0x0041693C ja */
eax = (unsigned long)M32(0x4441C0); /* 0x0041693E mov eax,dword_4441C0 */
edx = 0UL; /* 0x00416943 xor edx,edx */
/* 0x00416945 div ebx : x86 div faults if ebx==0; direct / % */
q = ebx ? (eax / ebx) : 0UL; r = ebx ? (eax % ebx) : 0UL;
eax = q & U32MASK; edx = r & U32MASK;
ebx = ((ebx << 2) & U32MASK); /* 0x00416947 shl ebx,2 */
edx = (edx + ebx) & U32MASK; /* 0x0041694A add edx,ebx */
ebx = (edx + 0xF8UL) & U32MASK; /* 0x0041694C lea ebx,[edx+0F8h] */
L_416952:;
eax = 0x442528UL; /* 0x00416952 mov eax,offset dword_442528 */
ebx = ((ebx << 2) & U32MASK); /* 0x00416957 shl ebx,2 */
eax = (eax + ebx) & U32MASK; /* 0x0041695A add eax,ebx */
M32(0x4441F8) = (unsigned int)(eax & U32MASK); /* 0x0041695C mov dword_4441F8,eax */
L_416961:;
if ((var_1C & 0xFFUL) == 0UL) goto L_41699F; /* 0x00416961 cmp / 0x00416966 jz */
if ((unsigned long)M8(0x444EA6) == 0UL) goto L_41698B; /* 0x00416968 cmp / 0x0041696F jz */
edx = (unsigned long)M32(0x443B60); /* 0x00416971 mov edx,dword_443B60 */
eax = edx & U32MASK; /* 0x00416977 mov eax,edx */
ebx = (unsigned long)M32(0x443B64); /* 0x00416979 mov ebx,dword_443B64 */
eax = ((eax << 5) & U32MASK); /* 0x0041697F shl eax,5 */
ebx = ((ebx << 5) & U32MASK); /* 0x00416982 shl ebx,5 */
eax = (eax - edx) & U32MASK; /* 0x00416985 sub eax,edx */
if (!((eax & U32MASK) < (ebx & U32MASK))) goto L_416998_set0; /* 0x00416987 cmp (jb) */
goto L_416998_set1; /* 0x00416989 jmp */
L_41698B:;
eax = (unsigned long)M32(0x443B60); /* 0x0041698B mov eax,dword_443B60 */
edx = (unsigned long)M32(0x443B64); /* 0x00416990 mov edx,dword_443B64 */
tmp = ((eax & U32MASK) < (edx & U32MASK)) ? 1UL : 0UL; /* 0x00416996 cmp / setb */
goto L_416998_done;
L_416998_set0:;
tmp = 0UL; /* setb false path */
goto L_416998_done;
L_416998_set1:;
tmp = 1UL; /* setb true path */
goto L_416998_done;
L_416998_done:;
L_416998:;
eax = (eax & ~0xFFUL) | ((tmp & 0xFFUL) & 0xFFUL); /* 0x00416998 setb al */
var_1C = (eax & 0xFFUL) & 0xFFUL; /* 0x0041699B mov [var_1C],al */
L_41699F:;
if ((unsigned long)M8(0x444EA9) == 0UL) goto L_4169CF; /* 0x0041699F cmp / 0x004169A6 jz */
if (!((unsigned long)M8(0x444EBA) > 4UL)) goto L_4169BE; /* 0x004169A8 cmp / 0x004169AF jbe */
eax = (unsigned long)M32(0x4441F8); /* 0x004169B1 mov eax,dword_4441F8 */
if (!((unsigned long)M32((unsigned long)eax) > 0x3FEUL)) goto L_4169BE; /* 0x004169B6 cmp / 0x004169BC ja */
goto L_4169C5;
L_4169BE:;
eax = 1UL; /* 0x004169BE mov eax,1 */
goto L_4169C7;
L_4169C5:;
eax = 0UL; /* 0x004169C5 xor eax,eax */
L_4169C7:;
if (eax == 0UL) goto L_4169CF; /* 0x004169C7 test / 0x004169C9 jz */
eax = (eax & ~0xFFUL) | ((1UL) & 0xFFUL); /* 0x004169CB mov al,1 */
goto L_4169D1;
L_4169CF:;
eax = (eax & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x004169CF xor al,al */
L_4169D1:;
edx = (edx & ~0xFF00UL) | ((((var_1C & 0xFFUL)) & 0xFFUL) << 8); /* 0x004169D1 mov dh,[var_1C] (byte) */
var_3C = (eax & 0xFFUL) & 0xFFUL; /* 0x004169D5 mov [var_3C],al */
if ((((edx >> 8) & 0xFFUL) & 0xFFUL) == 0UL) goto L_4169E5; /* 0x004169D9 test / 0x004169DB jz */
if (((eax & 0xFFUL) & 0xFFUL) == 0UL) goto L_4169E5; /* 0x004169DD test / 0x004169DF jz */
eax = (eax & ~0xFFUL) | ((1UL) & 0xFFUL); /* 0x004169E1 mov al,1 */
goto L_4169E7;
L_4169E5:;
eax = (eax & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x004169E5 xor al,al */
L_4169E7:;
var_48 = (eax & 0xFFUL) & 0xFFUL; /* 0x004169E7 mov [var_48],al */
if (((eax & 0xFFUL) & 0xFFUL) == 0UL) goto L_416A00; /* 0x004169EB test / 0x004169ED jz */
eax = (unsigned long)M32(0x4441E0); /* 0x004169EF mov eax,dword_4441E0 */
if (!((eax & U32MASK) < (unsigned long)M32(0x444200))) goto L_416A00; /* 0x004169F4 cmp / 0x004169FA jnb (unsigned <) */
eax = (eax & ~0xFFUL) | ((1UL) & 0xFFUL); /* 0x004169FC mov al,1 */
goto L_416A02;
L_416A00:;
eax = (eax & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x00416A00 xor al,al */
L_416A02:;
esi = (unsigned long)M32(0x444224); /* 0x00416A02 mov esi,dword_444224 */
var_2C = (eax & 0xFFUL) & 0xFFUL; /* 0x00416A08 mov [var_2C],al */
if (!((esi & U32MASK) < 0x40000UL)) goto L_416CE9; /* 0x00416A0C cmp / 0x00416A12 jnb */
eax = esi & U32MASK; /* 0x00416A18 mov eax,esi */
eax = ((eax << 5) & U32MASK); /* 0x00416A1A shl eax,5 */
L_416A1D:;
ebx = (unsigned long)M32(0x4299BC); /* 0x00416A1D mov ebx,dword_4299BC */
ebx = (ebx + eax) & U32MASK; /* 0x00416A23 add ebx,eax */
if ((unsigned long)M8((unsigned long)ebx) == 0UL) goto L_416CC6; /* 0x00416A25 cmp / 0x00416A28 jz */
ecx = (unsigned long)M32(0x444240); /* 0x00416A2E mov ecx,dword_444240 */
if (ecx != (unsigned long)M32((unsigned long)(ebx + 0x1C))) goto L_416CC6; /* 0x00416A34 cmp / 0x00416A37 jnz */
eax = 0UL; /* 0x00416A3D xor eax,eax */
ecx = (ecx & 0xFFFFFF00UL) | 0xFFUL; /* 0x00416A3F mov ch,0FFh lane-merge */
ecx = (ecx & ~0xFF00UL) | (((0xFFUL) & 0xFFUL) << 8);
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)ebx)) & 0xFFUL); /* 0x00416A41 mov al,[ebx] lane-merge */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
M8(0x444EB7) = (unsigned char)(((ecx >> 8) & 0xFFUL) & 0xFFUL); /* 0x00416A43 mov byte_444EB7,ch */
M32(0x444214) = (unsigned int)(eax & U32MASK); /* 0x00416A49 mov dword_444214,eax */
eax = ebx & U32MASK; /* 0x00416A4E mov eax,ebx */
ecx = (ebx + 0x0DUL) & U32MASK; /* 0x00416A50 lea ecx,[ebx+0Dh] */
L_416A53:;
edx = 0UL; /* 0x00416A53 xor edx,edx */
esi = (unsigned long)M32(0x444214); /* 0x00416A55 mov esi,dword_444214 */
edx = (edx & ~0xFFUL) | (((unsigned long)M8((unsigned long)(eax + 1))) & 0xFFUL); /* 0x00416A5B mov dl,[eax+1] lane-merge */
edx &= 0xFFUL;
esi = (esi + edx) & U32MASK; /* 0x00416A5E add esi,edx */
eax = (eax + 1) & U32MASK; /* 0x00416A60 inc eax */
M32(0x444214) = (unsigned int)(esi & U32MASK); /* 0x00416A61 mov dword_444214,esi */
if (eax != ecx) goto L_416A53; /* 0x00416A67 cmp / 0x00416A69 jnz */
if ((unsigned long)M32(0x44423C) != 0UL) goto L_416A9B; /* 0x00416A6B cmp / 0x00416A72 jnz */
eax = (unsigned long)M32(0x444264); /* 0x00416A74 mov eax,dword_444264 */
if (!((eax & U32MASK) <= (unsigned long)M32(0x444250))) goto L_416A9B; /* 0x00416A79 cmp / 0x00416A7F ja ( !(a>b) ) */
if (!((unsigned long)M8(0x444EBA) > 1UL)) goto L_416A9B; /* 0x00416A81 cmp / 0x00416A88 jbe */
eax = (unsigned long)M32(0x4441F8); /* 0x00416A8A mov eax,dword_4441F8 */
if ((unsigned long)M32((unsigned long)eax) > 0x3FEUL) goto L_416CE9; /* 0x00416A8F cmp / 0x00416A95 ja */
L_416A9B:;
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(ebx + 0x1B))) & 0xFFUL); /* 0x00416A9B mov al,[ebx+1Bh] */
eax = (eax & ~0xFFUL) | (((eax & 0xFFUL) & 0x3FUL) & 0xFFUL); /* 0x00416A9E and al,3Fh */
if (!(((eax & 0xFFUL) & 0xFFUL) < 8UL)) goto L_416AE9; /* 0x00416AA0 cmp / 0x00416AA2 jnb (unsigned <8) */
if ((unsigned long)M8((unsigned long)ebp) == 0UL) goto L_416AE9; /* 0x00416AA4 cmp / 0x00416AA8 jz */
eax = (eax & 0xFFUL) & 0xFFUL; /* 0x00416AAA and eax,0FFh */
edx = 0x3E0UL; /* 0x00416AAF mov edx,3E0h */
eax = ((eax << 5) & U32MASK); /* 0x00416AB4 shl eax,5 */
edx = (edx - eax) & U32MASK; /* 0x00416AB7 sub edx,eax */
eax = 0UL; /* 0x00416AB9 xor eax,eax */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(ebp + 1))) & 0xFFUL); /* 0x00416ABB mov al,[ebp+1] lane-merge */
eax &= 0xFFUL;
edx = (edx * eax) & U32MASK; /* 0x00416ABE imul edx,eax */
ecx = 0UL; /* 0x00416AC1 xor ecx,ecx */
ecx = (ecx & ~0xFFUL) | (((unsigned long)M8((unsigned long)(ebp + 0x12))) & 0xFFUL); /* 0x00416AC3 mov cl,[ebp+12h] lane-merge */
ecx &= 0xFFUL;
eax = edx & U32MASK; /* 0x00416AC6 mov eax,edx */
edx = ((((long)eax) < 0) ? U32MASK : 0UL); /* 0x00416AC8 sar edx,1Fh (sign extend) */
/* 0x00416ACB idiv ecx : signed div, faults if ecx==0; direct / % */
if (ecx == 0UL) { eax = 0UL; edx = 0UL; } else { q = (unsigned long)(((long)eax) / (long)(long)ecx); r = (unsigned long)(((long)eax) % (long)(long)ecx); eax = q & U32MASK; edx = r & U32MASK; }
ecx = eax & U32MASK; /* 0x00416ACD mov ecx,eax */
eax = 0UL; /* 0x00416ACF xor eax,eax */
esi = (unsigned long)M32(0x444214); /* 0x00416AD1 mov esi,dword_444214 */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(ebx + 1))) & 0xFFUL); /* 0x00416AD7 mov al,[ebx+1] lane-merge */
eax &= 0xFFUL;
edx = 0UL; /* 0x00416ADA xor edx,edx */
eax = ((eax << 10) & U32MASK); /* 0x00416ADC shl eax,0Ah */
/* 0x00416ADF div esi : faults if esi==0; direct / % */
if (esi == 0UL) { eax = 0UL; } else { q = (eax / esi) & U32MASK; r = (eax % esi) & U32MASK; eax = q; edx = r; }
if (!((ecx & U32MASK) > (eax & U32MASK))) goto L_416AE9; /* 0x00416AE1 cmp / 0x00416AE3 ja (unsigned >) */
goto L_416CE9;
L_416AE9:;
edx = (edx & ~0xFFUL) | ((1UL) & 0xFFUL); /* 0x00416AE9 mov dl,1 lane-merge */
edx = (edx & 0xFFFFFF00UL) | 0x01UL;
edi = 0UL; /* 0x00416AEB mov edi,esp (stack buf base; model v68 index 0) */
esi = 0x4137F0UL; /* 0x00416AED mov esi,offset loc_4137F0 */
eax = (unsigned long)M32(0x444214); /* 0x00416AF2 mov eax,dword_444214 */
M8(0x444EB0) = (unsigned char)((edx & 0xFFUL) & 0xFFUL); /* 0x00416AF7 mov byte_444EB0,dl */
M32(0x444234) = (unsigned int)(eax & U32MASK); /* 0x00416AFD mov dword_444234,eax */
edx = (edx & ~0xFF00UL) | ((((unsigned long)M8(0x444EAA)) & 0xFFUL) << 8); /* 0x00416B02 mov dh,byte_444EAA lane-merge */
edx = (edx & 0xFFFF00FFUL) | ((((edx >> 8) & 0xFFUL) & 0xFFUL) << 8);
    { unsigned int _k; static const unsigned char _b13[13] = {1,1,1,1,1,1,1,1,1,1,1,1,1};
      for (_k = 0; _k < 13; _k++) v68[_k] = _b13[_k]; }
    /* 0x00416B08 movsd x3 + 0x00416B0B movsb: 13 bytes from loc_4137F0 (file image: thirteen 0x01) to v68 */
if (((((edx >> 8) & 0xFFUL)) & 0xFFUL) == 0UL) goto L_416B52; /* 0x00416B0C test dh,dh / 0x00416B0E jz */
eax = (eax & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x00416B10 xor al,al */
eax = (eax & ~0xFF00UL) | ((((unsigned long)M8(0x444E82)) & 0xFFUL) << 8); /* 0x00416B12 mov ah,byte_444E82 (NO lane-merge after: native falls straight into mov [var_34],ah; a merge line here cleared ah and forced var_34=0) */
var_34 = (((eax >> 8) & 0xFFUL)) & 0xFFUL; /* 0x00416B18 mov [var_34],ah */
L_416B1C:;
ecx = 0UL; /* 0x00416B1C xor ecx,ecx */
ecx = (ecx & ~0xFFUL) | (((eax & 0xFFUL) & 0xFFUL) & 0xFFUL); /* 0x00416B1E mov cl,al */
ecx &= 0xFFUL;
edx = (ebx + ecx) & U32MASK; /* 0x00416B20 lea edx,[ebx+ecx] */
if ((unsigned long)M8((unsigned long)(edx + 1)) == 0UL) goto L_416B52; /* 0x00416B23 cmp / 0x00416B27 jz */
eax = (eax & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(edx + 0x0E))) & 0xFFUL) << 8); /* 0x00416B29 mov ah,[edx+0Eh] */
eax = (eax & 0xFFFF00FFUL) | ((((eax >> 8) & 0xFFUL) & 0xFFUL) << 8);
if (((((eax >> 8) & 0xFFUL)) & 0xFFUL) != ((var_34) & 0xFFUL)) goto L_416B4C; /* 0x00416B2C cmp / 0x00416B30 jnz */
eax = (eax & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x00416B32 xor al,al */
eax = (eax & 0xFFFFFF00UL) | 0UL;
if (ecx < 13UL) v68[ecx] = 0; /* 0x00416B34 mov [esp+ecx+var_68],al */
eax = 0UL; /* 0x00416B37 xor eax,eax */
edi = (unsigned long)M32(0x444234); /* 0x00416B39 mov edi,dword_444234 */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(edx + 1))) & 0xFFUL); /* 0x00416B3F mov al,[edx+1] lane-merge */
eax &= 0xFFUL;
edi = (edi - eax) & U32MASK; /* 0x00416B42 sub edi,eax */
M32(0x444234) = (unsigned int)(edi & U32MASK); /* 0x00416B44 mov dword_444234,edi */
goto L_416B52; /* 0x00416B4A jmp */
L_416B4C:;
eax = (eax & ~0xFFUL) | (((((eax & 0xFFUL) + 1) & 0xFFUL)) & 0xFFUL); /* 0x00416B4C inc al */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
if (((eax & 0xFFUL) & 0xFFUL) < 0x0DUL) goto L_416B1C; /* 0x00416B4E cmp / 0x00416B50 jb (unsigned <) */
L_416B52:;
eax = 0UL; /* 0x00416B52 xor eax,eax */
edx = (unsigned long)M32(0x444234); /* 0x00416B54 mov edx,dword_444234 */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)ebx)) & 0xFFUL); /* 0x00416B5A mov al,[ebx] lane-merge */
eax &= 0xFFUL;
if (!((eax & U32MASK) < (edx & U32MASK))) goto L_416CE9; /* 0x00416B5C cmp / 0x00416B5E jnb (unsigned <) */
eax = (eax & ~0xFFUL) | (((var_48 & 0xFFUL)) & 0xFFUL); /* 0x00416B64 mov al,[var_48] */
M8(0x444EB1) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416B68 mov byte_444EB1,al */
eax = (eax & ~0xFF00UL) | ((((var_2C & 0xFFUL)) & 0xFFUL) << 8); /* 0x00416B6D mov ah,[var_2C] */
eax = (eax & 0xFFFF00FFUL) | ((((eax >> 8) & 0xFFUL) & 0xFFUL) << 8);
if (((((eax >> 8) & 0xFFUL)) & 0xFFUL) == 0UL) goto L_416B7D_z; /* 0x00416B71 test / setz below */
eax = (eax & ~0xFFUL) | ((0UL) & 0xFFUL); goto L_416B76_done;
L_416B7D_z:;
eax = (eax & ~0xFFUL) | ((1UL) & 0xFFUL);
L_416B76_done:;
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL); /* 0x00416B73 setz al */
M8(0x444EB0) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416B76 mov byte_444EB0,al */
if (((eax & 0xFFUL) & 0xFFUL) == 0UL) goto L_416CE9; /* 0x00416B7B test / 0x00416B7D jz */
ecx = (unsigned long)M32(0x444214); /* 0x00416B83 mov ecx,dword_444214 */
if (!((ecx & U32MASK) <= 8UL)) goto L_416BF4; /* 0x00416B89 cmp / 0x00416B8C ja (unsigned >8) */
eax = 0UL; /* 0x00416B8E xor eax,eax */
esi = (unsigned long)M8((unsigned long)ebx); /* 0x00416B90 movzx esi,byte ptr [ebx] */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(ebx + 1))) & 0xFFUL); /* 0x00416B93 mov al,[ebx+1] lane-merge */
eax &= 0xFFUL;
edx = (esi + eax) & U32MASK; /* 0x00416B96 lea edx,[esi+eax] */
if (edx != ecx) goto L_416BF4; /* 0x00416B99 cmp / 0x00416B9B jnz */
esi = (((long)esi) >> 1) & U32MASK; /* 0x00416B9D sar esi,1 */
eax = (((long)(eax & U32MASK)) >> 1) & U32MASK; /* 0x00416B9F sar eax,1 */
esi = ((esi << 2) & U32MASK); /* 0x00416BA1 shl esi,2 */
eax = (eax + esi) & U32MASK; /* 0x00416BA4 add eax,esi */
eax = (eax + eax) & U32MASK; /* 0x00416BA6 add eax,eax */
eax = (eax + 0x444128UL) & U32MASK; /* 0x00416BA8 add eax,offset dword_444128 */
eax = (eax & ~0xFFUL) | (d3u_416774(c, (unsigned int)eax) & 0xFFUL); /* 0x00416BAD call sub_416774 */
if (((eax & 0xFFUL) & 0xFFUL) == 0UL) goto L_416BCE; /* 0x00416BB2 test / 0x00416BB4 jz */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(ebx + 0x0E))) & 0xFFUL); /* 0x00416BB6 mov al,[ebx+0Eh] */
ebx &= 0xFFFF00FFUL; /* 0x00416BB9 xor bl,bl lane-merge */
ebx = (ebx & ~0xFFUL) | ((0UL) & 0xFFUL); ebx &= 0xFFFFFF00UL;
var_28 = (eax & 0xFFUL) & 0xFFUL; /* 0x00416BBB mov [var_28],al */
var_4C = ((ebx & 0xFFUL)) & 0xFFUL; /* 0x00416BBF mov [var_4C],bl */
M8(0x444EB7) = (unsigned char)((ebx & 0xFFUL) & 0xFFUL); /* 0x00416BC3 mov byte_444EB7,bl */
goto L_416CBA; /* 0x00416BC9 jmp */
L_416BCE:;
edx = (edx & ~0xFF00UL) | (((0x0DUL) & 0xFFUL) << 8); /* 0x00416BCE mov dh,0Dh lane-merge */
edx = (edx & 0xFFFF00FFUL) | (0x0DUL << 8);
eax = (eax & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x00416BD0 mov al,byte_444EAA */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
M8(0x444EB7) = (unsigned char)(((edx >> 8) & 0xFFUL) & 0xFFUL); /* 0x00416BD5 mov byte_444EB7,dh */
edx = 0UL; /* 0x00416BDB xor edx,edx */
edx = (edx & ~0xFFUL) | (((eax & 0xFFUL) & 0xFFUL) & 0xFFUL); /* 0x00416BDD mov dl,al */
edx &= 0xFFUL;
eax = (eax & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(ebx + 0x0E))) & 0xFFUL) << 8); /* 0x00416BDF mov ah,[ebx+0Eh] */
eax = (eax & 0xFFFF00FFUL) | ((((eax >> 8) & 0xFFUL) & 0xFFUL) << 8);
M8((unsigned long)(0x444E82UL + (edx & 0xFFUL))) = (unsigned char)(((eax >> 8) & 0xFFUL) & 0xFFUL); /* 0x00416BE2 mov byte_444E82[edx],ah */
eax = (eax & ~0xFFUL) | (((((eax & 0xFFUL) + 1) & 0xFFUL)) & 0xFFUL); /* 0x00416BE8 inc al */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
M8(0x444EAA) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416BEA mov byte_444EAA,al */
goto L_416CBA; /* 0x00416BEF jmp */
L_416BF4:;
eax = (unsigned long)M32(0x444234); /* 0x00416BF4 mov eax,dword_444234 */
M32(0x4441E8) = (unsigned int)(eax & U32MASK); /* 0x00416BF9 mov dword_4441E8,eax */
eax &= 0xFFFFUL; /* 0x00416BFE mov ax,word ptr / 0x00416C04 and eax,0FFFFh */
esi = (esi & 0xFFFF0000UL) | (d3u_423188(c, (unsigned int)(eax & 0xFFFFUL)) & 0xFFFFUL); /* 0x00416C09 call sub_423188 */
edx = (edx & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x00416C0E xor dl,dl */
edx = (edx & 0xFFFFFF00UL) | 0UL;
eax = 0UL; /* 0x00416C12 xor eax,eax */
M8(0x444EB7) = (unsigned char)((edx & 0xFFUL) & 0xFFUL); /* 0x00416C14 mov byte_444EB7,dl */
M32(0x4441EC) = 0; /* 0x00416C1A mov dword_4441EC,eax */
L_416C1F:;
eax = 0UL; /* 0x00416C1F xor eax,eax */
eax = (eax & ~0xFFUL) | (((unsigned long)M8(0x444EB7)) & 0xFFUL); /* 0x00416C21 mov al,byte_444EB7 */
eax &= 0xFFUL;
    /* 0x00416C26 cmp [esp+eax+var_68],0 / 0x00416C2A jz (no ecx>=13 clause: not in listing) */
    if (((unsigned long)v68[eax & 0xFFUL]) == 0UL) goto L_416C93;
eax = (eax + ebx) & U32MASK; /* 0x00416C2C add eax,ebx */
if ((unsigned long)M8((unsigned long)(eax + 1)) != 0UL) goto L_416C3D; /* 0x00416C2E cmp / 0x00416C32 jnz */
M8(0x444EB7) = 0x0D; /* 0x00416C34 mov byte_444EB7,0Dh */
goto L_416CA7; /* 0x00416C3B jmp */
L_416C3D:;
ecx = 0UL; /* 0x00416C3D xor ecx,ecx */
edx = (unsigned long)M32(0x4441EC); /* 0x00416C3F mov edx,dword_4441EC */
ecx = (ecx & ~0xFFUL) | (((unsigned long)M8((unsigned long)(eax + 1))) & 0xFFUL); /* 0x00416C45 mov cl,[eax+1] lane-merge */
ecx &= 0xFFUL;
edx = (edx + ecx) & U32MASK; /* 0x00416C48 add edx,ecx */
M32(0x4441F0) = (unsigned int)(edx & U32MASK); /* 0x00416C4A mov dword_4441F0,edx */
edx = 0UL; /* 0x00416C50 xor edx,edx */
ecx = (unsigned long)M32(0x4441F0); /* 0x00416C52 mov ecx,dword_4441F0 */
edx = (esi & 0xFFFFUL); /* 0x00416C58 mov dx,si lane-merge */
edx &= 0xFFFFUL;
if (!((edx & U32MASK) < (ecx & U32MASK))) goto L_416C6E; /* 0x00416C5B cmp / 0x00416C5D jnb (unsigned <) */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(eax + 0x0E))) & 0xFFUL); /* 0x00416C5F mov al,[eax+0Eh] */
edx = (edx & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x00416C62 xor dl,dl (native zeroes dl: var_4C=0 on this path) */
var_28 = (eax & 0xFFUL) & 0xFFUL; /* 0x00416C64 mov [var_28],al */
var_4C = ((edx & 0xFFUL)) & 0xFFUL; /* 0x00416C68 mov [var_4C],dl */
goto L_416CA7; /* 0x00416C6C jmp */
L_416C6E:;
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x00416C6E mov dl,byte_444EAA */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL);
ecx = 0UL; /* 0x00416C74 xor ecx,ecx */
ecx = (ecx & ~0xFFUL) | (((edx & 0xFFUL) & 0xFFUL) & 0xFFUL); /* 0x00416C76 mov cl,dl */
ecx &= 0xFFUL;
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(eax + 0x0E))) & 0xFFUL); /* 0x00416C78 mov al,[eax+0Eh] */
eax &= 0xFFUL;
edx = (edx & ~0xFFUL) | (((((edx & 0xFFUL) + 1) & 0xFFUL)) & 0xFFUL); /* 0x00416C7B inc dl */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL);
M8((unsigned long)(0x444E82UL + (ecx & 0xFFUL))) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416C7D mov byte_444E82[ecx],al */
eax = (unsigned long)M32(0x4441F0); /* 0x00416C83 mov eax,dword_4441F0 */
M8(0x444EAA) = (unsigned char)((edx & 0xFFUL) & 0xFFUL); /* 0x00416C88 mov byte_444EAA,dl */
M32(0x4441EC) = (unsigned int)(eax & U32MASK); /* 0x00416C8E mov dword_4441EC,eax */
L_416C93:;
eax = (eax & ~0xFFUL) | (((unsigned long)M8(0x444EB7)) & 0xFFUL); /* 0x00416C93 mov al,byte_444EB7 */
eax = (eax & ~0xFFUL) | (((((eax & 0xFFUL) + 1) & 0xFFUL)) & 0xFFUL); /* 0x00416C98 inc al */
M8(0x444EB7) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416C9A mov byte_444EB7,al */
if (((eax & 0xFFUL) & 0xFFUL) < 0x0DUL) goto L_416C1F; /* 0x00416C9F cmp / 0x00416CA1 jb (unsigned <) */
L_416CA7:;
if ((unsigned long)M8(0x444EB7) != 0x0DUL) goto L_416CBA; /* 0x00416CA7 cmp / 0x00416CAE jnz */
eax = (unsigned long)M32(0x4441E8); /* 0x00416CB0 mov eax,dword_4441E8 */
M32(0x4441F0) = (unsigned int)(eax & U32MASK); /* 0x00416CB5 mov dword_4441F0,eax */
L_416CBA:;
eax = 0x4441E8UL; /* 0x00416CBA mov eax,offset dword_4441E8 */
d3u_423140(c, 0x4441E8UL); /* 0x00416CBF call sub_423140 */
goto L_416CE9; /* 0x00416CC4 jmp */
L_416CC6:;
edi = (unsigned long)M32(0x444224); /* 0x00416CC6 mov edi,dword_444224 */
edi = (edi + 0x8000UL) & U32MASK; /* 0x00416CCC add edi,8000h */
eax = (eax + 0x100000UL) & U32MASK; /* 0x00416CD2 add eax,100000h */
M32(0x444224) = (unsigned int)(edi & U32MASK); /* 0x00416CD7 mov dword_444224,edi */
if ((edi & U32MASK) < 0x40000UL) goto L_416A1D; /* 0x00416CDD cmp / 0x00416CE3 jb (unsigned <) */
L_416CE9:;
if ((var_4C & 0xFFUL) == 0UL) goto L_4173C0; /* 0x00416CE9 cmp / 0x00416CEE jz */
if ((unsigned long)M8(0x444EA0) == 0UL) goto L_416D04; /* 0x00416CF4 cmp / 0x00416CFB jz */
eax = d3u_415058(c); /* 0x00416CFD call sub_415058 */
goto L_416D06; /* 0x00416D02 jmp */
L_416D04:;
eax = 0UL; /* 0x00416D04 xor eax,eax */
L_416D06:;
ecx = (ecx & ~0xFF00UL) | (((1UL) & 0xFFUL) << 8); /* 0x00416D06 mov ch,1 lane-merge */
ecx = (ecx & 0xFFFF00FFUL) | (1UL << 8);
var_50 = eax & U32MASK; /* 0x00416D08 mov [var_50],eax */
edx = (edx & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x00416D0C xor dl,dl */
edx = (edx & 0xFFFFFF00UL) | 0UL;
var_30 = (((ecx >> 8) & 0xFFUL)) & 0xFFUL; /* 0x00416D0E mov [var_30],ch */
var_40 = ((edx & 0xFFUL)) & 0xFFUL; /* 0x00416D12 mov [var_40],dl */
if (eax == 0UL) goto L_416D21; /* 0x00416D16 test / 0x00416D18 jz */
M32(0x44423C) = (unsigned int)(((unsigned long)M32(0x44423C) + 2UL) & U32MASK); /* 0x00416D1A add dword_44423C,2 */
L_416D21:;
edi = var_50 & U32MASK; /* 0x00416D21 mov edi,[var_50] */
if (edi == 0UL) goto L_416D5E; /* 0x00416D25 test / 0x00416D27 jz */
edx = 0UL; /* 0x00416D29 xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8((unsigned long)(edi + 1))) & 0xFFUL); /* 0x00416D2B mov dl,[edi+1] lane-merge */
edx &= 0xFFUL;
ebx = (edx * 4UL) & U32MASK; /* 0x00416D2E lea ebx,[edx*4] */
ebx = (ebx + edx) & U32MASK; /* 0x00416D35 add ebx,edx */
edx = 0UL; /* 0x00416D37 xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8((unsigned long)edi)) & 0xFFUL); /* 0x00416D39 mov dl,[edi] lane-merge */
edx &= 0xFFUL;
eax = (edx * 4UL) & U32MASK; /* 0x00416D3B lea eax,[edx*4] */
eax = (eax - edx) & U32MASK; /* 0x00416D42 sub eax,edx */
if (!((long)(eax & U32MASK) < (long)(ebx & U32MASK))) goto L_416D58; /* 0x00416D44 cmp / 0x00416D46 jge (SIGNED <) */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(edi + 3))) & 0xFFUL); /* 0x00416D48 mov al,[edi+3] */
eax = (eax & ~0xFFUL) | (((eax & 0xFFUL) & 0x7FUL) & 0xFFUL); /* 0x00416D4B and al,7Fh */
if (((eax & 0xFFUL) & 0xFFUL) > 2UL) goto L_416D58; /* 0x00416D4D cmp / 0x00416D4F ja (unsigned >) */
eax = 1UL; /* 0x00416D51 mov eax,1 */
goto L_416D5A; /* 0x00416D56 jmp */
L_416D58:;
eax = 0UL; /* 0x00416D58 xor eax,eax */
L_416D5A:;
if (eax == 0UL) goto L_416D62; /* 0x00416D5A test / 0x00416D5C jz */
L_416D5E:;
eax = (eax & ~0xFFUL) | ((1UL) & 0xFFUL); /* 0x00416D5E mov al,1 */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
goto L_416D64; /* 0x00416D60 jmp */
L_416D62:;
eax = (eax & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x00416D62 xor al,al */
eax = (eax & 0xFFFFFF00UL) | 0UL;
L_416D64:;
M8(0x444EAC) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416D64 mov byte_444EAC,al */
L_416D69:;
if ((unsigned long)M8(0x444EAC) == 0UL) goto L_416D7D_chk; /* 0x00416D69 cmp / 0x00416D70 jnz inverted */
goto L_416D7D;
L_416D7D_chk:;
if ((var_40 & 0xFFUL) == 0UL) goto L_416FDA; /* 0x00416D72 cmp / 0x00416D77 jz */
L_416D7D:;
ebx = (ebx & ~0xFF00UL) | (((0UL) & 0xFFUL) << 8); /* 0x00416D7D xor bh,bh lane-merge */
ebx = (ebx & 0xFFFF00FFUL) | 0UL;
eax = (unsigned long)M32(0x44424C); /* 0x00416D7F mov eax,dword_44424C */
var_30 = (((ebx >> 8) & 0xFFUL)) & 0xFFUL; /* 0x00416D84 mov [var_30],bh */
if (eax == 0UL) goto L_416DAB; /* 0x00416D88 test / 0x00416D8A jz */
if ((unsigned long)M8((unsigned long)(ebp + 0x11)) == 0UL) goto L_416DAB; /* 0x00416D8C cmp / 0x00416D90 jz */
if (!((unsigned long)M32(0x444208) < 0xFFFFUL)) goto L_416DB2; /* 0x00416D92 cmp / 0x00416D9C jnb (unsigned <) */
eax = (unsigned long)M32(0x4441F8); /* 0x00416D9E mov eax,dword_4441F8 */
if ((unsigned long)M32((unsigned long)eax) > 0x3FEUL) goto L_416DB2; /* 0x00416DA3 cmp / 0x00416DA9 ja (unsigned >) */
L_416DAB:;
eax = 1UL; /* 0x00416DAB mov eax,1 */
goto L_416DB4; /* 0x00416DB0 jmp */
L_416DB2:;
eax = 0UL; /* 0x00416DB2 xor eax,eax */
L_416DB4:;
if (eax == 0UL) goto L_416DBE_z; /* 0x00416DB4 test / setnz */
eax = (eax & ~0xFFUL) | ((1UL) & 0xFFUL); goto L_416DB9_done;
L_416DBE_z:;
eax = (eax & ~0xFFUL) | ((0UL) & 0xFFUL);
L_416DB9_done:;
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL); /* 0x00416DB6 setnz al */
M8(0x444EAC) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416DB9 mov byte_444EAC,al */
if (((eax & 0xFFUL) & 0xFFUL) == 0UL) goto L_416FDA; /* 0x00416DBE test / 0x00416DC0 jz */
if ((unsigned long)M8((unsigned long)ebp) == 0UL) goto L_416FDA; /* 0x00416DC6 cmp / 0x00416DCA jz */
edi = 0UL; /* 0x00416DD0 lea edi,[var_58] (stack buf base) */
eax = (eax & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x00416DD4 xor al,al */
eax = (eax & 0xFFFFFF00UL) | 0UL;
esi = 0x4137FDUL; /* 0x00416DD6 mov esi,(loc_4137FC+1) */
var_38 = (eax & 0xFFUL) & 0xFFUL; /* 0x00416DDB mov [var_38],al */
    { unsigned int _k; static const unsigned char _b8[8] = {0xA5,0xA5,0x88,0x44,0x24,0x24,0x8A,0x45};
      for (_k = 0; _k < 8; _k++) v58[_k] = _b8[_k]; }
    /* 0x00416DDF movsd x2: 8 bytes from 0x416DDF (file image) to v58 */
var_44 = (eax & 0xFFUL) & 0xFFUL; /* 0x00416DE1 mov [var_44],al */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(ebp + 0x12))) & 0xFFUL); /* 0x00416DE5 mov al,[ebp+12h] */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
var_24 = (eax & 0xFFUL) & 0xFFUL; /* 0x00416DE8 mov [var_24],al */
L_416DEC:;
eax = 0UL; /* 0x00416DEC xor eax,eax */
eax = (eax & ~0xFFUL) | (((var_38 & 0xFFUL)) & 0xFFUL); /* 0x00416DEE mov al,[var_38] */
eax &= 0xFFUL;
if ((unsigned long)M8((unsigned long)(eax + ebp + 1)) == 0UL) goto L_416E46; /* 0x00416DF2 cmp / 0x00416DF7 jz */
edx = (edx & ~0xFF00UL) | ((((var_44 & 0xFFUL)) & 0xFFUL) << 8); /* 0x00416DF9 mov dh,[var_44] */
edx = (edx & ~0xFF00UL) | (((((((edx >> 8) & 0xFFUL) + 1) & 0xFFUL)) & 0xFFUL) << 8); /* 0x00416DFD inc dh */
eax = 0UL; /* 0x00416DFF xor eax,eax */
var_44 = ((edx >> 8) & 0xFFUL) & 0xFFUL; /* 0x00416E01 mov [var_44],dh */
L_416E05:;
edx = 0UL; /* 0x00416E05 xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x00416E07 mov dl,byte_444EAA */
edx &= 0xFFUL;
if (!((eax & U32MASK) < (edx & U32MASK))) goto L_416E37; /* 0x00416E0D cmp / 0x00416E0F jnb (unsigned <) */
ebx = 0UL; /* 0x00416E11 xor ebx,ebx */
ebx = (ebx & ~0xFFUL) | (((var_38 & 0xFFUL)) & 0xFFUL); /* 0x00416E13 mov bl,[var_38] */
ebx &= 0xFFUL;
ecx = eax & U32MASK; /* 0x00416E17 mov ecx,eax */
edx = (ebx + ebp) & U32MASK; /* 0x00416E19 lea edx,[ebx+ebp] */
ecx = (ecx & ~0xFFUL) | (((unsigned long)M8((unsigned long)(0x444E82UL + (ecx & 0xFFUL)))) & 0xFFUL); /* 0x00416E1C mov cl,byte_444E82[ecx] */
ecx = (ecx & 0xFFFFFF00UL) | ((ecx & 0xFFUL) & 0xFFUL);
ecx = (ecx & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(edx + 9))) & 0xFFUL) << 8); /* 0x00416E22 mov ch,[edx+9] */
ecx = (ecx & 0xFFFF00FFUL) | ((((ecx >> 8) & 0xFFUL) & 0xFFUL) << 8);
eax = (eax + 1) & U32MASK; /* 0x00416E25 inc eax */
if ((((ecx & 0xFFUL)) & 0xFFUL) != ((((ecx >> 8) & 0xFFUL)) & 0xFFUL)) goto L_416E05; /* 0x00416E26 cmp / 0x00416E28 jnz */
eax = (eax & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x00416E2A xor al,al */
eax = (eax & 0xFFFFFF00UL) | 0UL;
if (ebx < 8UL) v58[ebx] = 0; /* 0x00416E2C mov [esp+ebx+var_58],al */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(edx + 1))) & 0xFFUL); /* 0x00416E30 mov al,[edx+1] */
eax &= 0xFFUL;
var_24 = ((var_24 - ((eax & 0xFFUL) & 0xFFUL)) & 0xFFUL); /* 0x00416E33 sub [var_24],al */
L_416E37:;
edx = (edx & ~0xFFUL) | (((var_38 & 0xFFUL)) & 0xFFUL); /* 0x00416E37 mov dl,[var_38] */
edx = (edx & ~0xFFUL) | (((((edx & 0xFFUL) + 1) & 0xFFUL)) & 0xFFUL); /* 0x00416E3B inc dl */
var_38 = (edx & 0xFFUL) & 0xFFUL; /* 0x00416E3D mov [var_38],dl */
if (((edx & 0xFFUL) & 0xFFUL) < 8UL) goto L_416DEC; /* 0x00416E41 cmp / 0x00416E44 jb (unsigned <) */
L_416E46:;
ebx = (ebx & ~0xFFUL) | ((0xFFUL) & 0xFFUL); /* 0x00416E46 mov bl,0FFh */
ebx = (ebx & 0xFFFFFF00UL) | 0xFFUL;
eax = 0UL; /* 0x00416E48 xor eax,eax */
edx = 0UL; /* 0x00416E4A xor edx,edx */
eax = (eax & ~0xFFUL) | (((var_24 & 0xFFUL)) & 0xFFUL); /* 0x00416E4C mov al,[var_24] */
eax &= 0xFFUL;
edx = (edx & ~0xFFUL) | (((var_24 & 0xFFUL)) & 0xFFUL); /* 0x00416E50 mov dl,[var_24] */
edx &= 0xFFUL;
M32(0x444248) = (unsigned int)(eax & U32MASK); /* 0x00416E54 mov dword_444248,eax */
eax = 0UL; /* 0x00416E59 xor eax,eax */
M8(0x444EB8) = (unsigned char)((ebx & 0xFFUL) & 0xFFUL); /* 0x00416E5B mov byte_444EB8,bl */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)ebp)) & 0xFFUL); /* 0x00416E61 mov al,[ebp+0] lane-merge */
eax &= 0xFFUL;
edx = ((edx << 2) & U32MASK); /* 0x00416E64 shl edx,2 */
ebx = (eax + eax * 4UL) & U32MASK; /* 0x00416E67 lea ebx,[eax+eax*4] */
if (!((edx & U32MASK) > (ebx & U32MASK))) goto L_416FDA; /* 0x00416E6A cmp / 0x00416E6C jle (unsigned >) */
edx = 0UL; /* 0x00416E72 xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8((unsigned long)(ebp + 0x11))) & 0xFFUL); /* 0x00416E74 mov dl,[ebp+11h] lane-merge */
edx &= 0xFFUL;
ecx = (unsigned long)M32(0x44424C); /* 0x00416E77 mov ecx,dword_44424C */
edx = ((edx << 2) & U32MASK); /* 0x00416E7D shl edx,2 */
edx = (edx + ecx) & U32MASK; /* 0x00416E80 add edx,ecx */
if (!((edx & U32MASK) < 0x10UL)) goto L_416FDA; /* 0x00416E82 cmp / 0x00416E85 jnb (unsigned <) */
edx = (edx & ~0xFFUL) | (((var_48 & 0xFFUL)) & 0xFFUL); /* 0x00416E8B mov dl,[var_48] */
ebx = (ebx & ~0xFF00UL) | ((((var_2C & 0xFFUL)) & 0xFFUL) << 8); /* 0x00416E8F mov bh,[var_2C] */
ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) & 0xFFUL) & 0xFFUL) << 8);
M8(0x444EB5) = (unsigned char)((edx & 0xFFUL) & 0xFFUL); /* 0x00416E93 mov byte_444EB5,dl */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL);
if (((((ebx >> 8) & 0xFFUL)) & 0xFFUL) != 0UL) goto L_416FDA; /* 0x00416E99 test / 0x00416E9B jnz */
if (!((var_24 & 0xFFUL) > 0x10UL)) goto L_416EA8_chk; /* 0x00416EA1 cmp / 0x00416EA6 ja */
goto L_416F11;
L_416EA8_chk:;
ecx = (ecx & ~0xFF00UL) | ((((var_44 & 0xFFUL)) & 0xFFUL) << 8); /* 0x00416EA8 mov ch,[var_44] */
if (((((ecx >> 8) & 0xFFUL)) & 0xFFUL) != 1UL) goto L_416F11; /* 0x00416EAC cmp / 0x00416EAF jnz */
eax = (((long)(eax & U32MASK)) >> 2) & U32MASK; /* 0x00416EB1 sar eax,2 */
edx = (eax * 4UL) & U32MASK; /* 0x00416EB4 lea edx,[eax*4] */
eax = 0UL; /* 0x00416EBB xor eax,eax */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(ebp + 1))) & 0xFFUL); /* 0x00416EBD mov al,[ebp+1] lane-merge */
eax &= 0xFFUL;
eax = (((long)(eax & U32MASK)) >> 2) & U32MASK; /* 0x00416EC0 sar eax,2 */
eax = (eax + edx) & U32MASK; /* 0x00416EC3 add eax,edx */
eax = (eax + eax) & U32MASK; /* 0x00416EC5 add eax,eax */
eax = (eax + 0x444108UL) & U32MASK; /* 0x00416EC7 add eax,offset dword_444108 */
eax = (eax & ~0xFFUL) | (d3u_416774(c, (unsigned int)eax) & 0xFFUL); /* 0x00416ECC call sub_416774 */
if (((eax & 0xFFUL) & 0xFFUL) == 0UL) goto L_416EEB; /* 0x00416ED1 test / 0x00416ED3 jz */
var_4C = (((ebx >> 8) & 0xFFUL)) & 0xFFUL; /* 0x00416ED5 mov [var_4C],bh */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(ebp + 9))) & 0xFFUL); /* 0x00416ED9 mov al,[ebp+9] */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
M8(0x444EB8) = (unsigned char)(((ebx >> 8) & 0xFFUL) & 0xFFUL); /* 0x00416EDC mov byte_444EB8,bh */
var_28 = (eax & 0xFFUL) & 0xFFUL; /* 0x00416EE2 mov [var_28],al */
goto L_416FD0; /* 0x00416EE6 jmp */
L_416EEB:;
eax = (eax & ~0xFF00UL) | (((8UL) & 0xFFUL) << 8); /* 0x00416EEB mov ah,8 lane-merge */
eax = (eax & 0xFFFF00FFUL) | (8UL << 8);
edx = 0UL; /* 0x00416EED xor edx,edx */
M8(0x444EB8) = (unsigned char)(((eax >> 8) & 0xFFUL) & 0xFFUL); /* 0x00416EEF mov byte_444EB8,ah */
eax = (eax & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x00416EF5 mov al,byte_444EAA */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
edx = (edx & ~0xFFUL) | (((eax & 0xFFUL) & 0xFFUL) & 0xFFUL); /* 0x00416EFA mov dl,al */
edx &= 0xFFUL;
eax = (eax & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(ebp + 9))) & 0xFFUL) << 8); /* 0x00416EFC mov ah,[ebp+9] */
eax = (eax & 0xFFFF00FFUL) | ((((eax >> 8) & 0xFFUL) & 0xFFUL) << 8);
M8((unsigned long)(0x444E82UL + (edx & 0xFFUL))) = (unsigned char)(((eax >> 8) & 0xFFUL) & 0xFFUL); /* 0x00416EFF mov byte_444E82[edx],ah */
eax = (eax & ~0xFFUL) | (((((eax & 0xFFUL) + (((ecx >> 8) & 0xFFUL) & 0xFFUL)) & 0xFFUL)) & 0xFFUL); /* 0x00416F05 add al,ch */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
M8(0x444EAA) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416F07 mov byte_444EAA,al */
goto L_416FD0; /* 0x00416F0C jmp */
L_416F11:;
eax = 0UL; /* 0x00416F11 xor eax,eax */
eax = (eax & ~0xFFUL) | (((var_24 & 0xFFUL)) & 0xFFUL); /* 0x00416F13 mov al,[var_24] */
eax &= 0xFFUL;
M32(0x4441E8) = (unsigned int)(eax & U32MASK); /* 0x00416F17 mov dword_4441E8,eax */
eax = 0UL; /* 0x00416F1C xor eax,eax */
eax = (eax & ~0xFFUL) | (((var_24 & 0xFFUL)) & 0xFFUL); /* 0x00416F1E mov al,[var_24] */
eax &= 0xFFUL;
esi = 0UL; /* 0x00416F22 xor esi,esi */
ecx = (ecx & 0xFFFF0000UL) | (d3u_423188(c, (unsigned int)(eax & 0xFFFFUL)) & 0xFFFFUL); /* 0x00416F24 call sub_423188 */
eax = (eax & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x00416F2B xor al,al */
eax = (eax & 0xFFFFFF00UL) | 0UL;
M32(0x4441EC) = (unsigned int)(esi & U32MASK); /* 0x00416F2D mov dword_4441EC,esi */
M8(0x444EB8) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416F33 mov byte_444EB8,al */
L_416F38:;
eax = 0UL; /* 0x00416F38 xor eax,eax */
eax = (eax & ~0xFFUL) | (((unsigned long)M8(0x444EB8)) & 0xFFUL); /* 0x00416F3A mov al,byte_444EB8 */
eax &= 0xFFUL;
    /* 0x00416F3F cmp [esp+eax+var_58],0 / 0x00416F44 jz (no eax>=8 clause: not in listing) */
    if (((unsigned long)v58[eax & 0xFFUL]) == 0UL) goto L_416FA9;
eax = (eax + ebp) & U32MASK; /* 0x00416F46 add eax,ebp */
if ((unsigned long)M8((unsigned long)(eax + 1)) != 0UL) goto L_416F57; /* 0x00416F48 cmp / 0x00416F4C jnz */
M8(0x444EB8) = 8; /* 0x00416F4E mov byte_444EB8,8 */
goto L_416FBD; /* 0x00416F55 jmp */
L_416F57:;
ebx = 0UL; /* 0x00416F57 xor ebx,ebx */
edx = (unsigned long)M32(0x4441EC); /* 0x00416F59 mov edx,dword_4441EC */
ebx = (ebx & ~0xFFUL) | (((unsigned long)M8((unsigned long)(eax + 1))) & 0xFFUL); /* 0x00416F5F mov bl,[eax+1] lane-merge */
ebx &= 0xFFUL;
edx = (edx + ebx) & U32MASK; /* 0x00416F62 add edx,ebx */
M32(0x4441F0) = (unsigned int)(edx & U32MASK); /* 0x00416F64 mov dword_4441F0,edx */
edx = 0UL; /* 0x00416F6A xor edx,edx */
edi = (unsigned long)M32(0x4441F0); /* 0x00416F6C mov edi,dword_4441F0 */
edx = (ecx & 0xFFFFUL); /* 0x00416F72 mov dx,cx lane-merge */
edx &= 0xFFFFUL;
if (!((edx & U32MASK) < (edi & U32MASK))) goto L_416F88; /* 0x00416F75 cmp / 0x00416F77 jnb (unsigned <) */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(eax + 9))) & 0xFFUL); /* 0x00416F79 mov al,[eax+9] */
eax &= 0xFFUL; /* lane-keep al (dropped spurious edx zeroing: no native insn between 0x416F79 and 0x416F7C; it broke xor dl,cl) */
edx = (edx & ~0xFFUL) | (((((edx & 0xFFUL) ^ (ecx & 0xFFUL)) & 0xFFUL)) & 0xFFUL); /* 0x00416F7C xor dl,cl */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL);
var_28 = (eax & 0xFFUL) & 0xFFUL; /* 0x00416F7E mov [var_28],al */
var_4C = ((edx & 0xFFUL)) & 0xFFUL; /* 0x00416F82 mov [var_4C],dl */
goto L_416FBD; /* 0x00416F86 jmp */
L_416F88:;
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x00416F88 mov dl,byte_444EAA */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL);
ebx = 0UL; /* 0x00416F8E xor ebx,ebx */
M32(0x4441EC) = (unsigned int)(edi & U32MASK); /* 0x00416F90 mov dword_4441EC,edi */
ebx = (ebx & ~0xFFUL) | (((edx & 0xFFUL) & 0xFFUL) & 0xFFUL); /* 0x00416F96 mov bl,dl */
ebx &= 0xFFUL;
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(eax + 9))) & 0xFFUL); /* 0x00416F98 mov al,[eax+9] */
eax &= 0xFFUL;
edx = (edx & ~0xFFUL) | (((((edx & 0xFFUL) + 1) & 0xFFUL)) & 0xFFUL); /* 0x00416F9B inc dl */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL);
M8((unsigned long)(0x444E82UL + (ebx & 0xFFUL))) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416F9D mov byte_444E82[ebx],al */
M8(0x444EAA) = (unsigned char)((edx & 0xFFUL) & 0xFFUL); /* 0x00416FA3 mov byte_444EAA,dl */
L_416FA9:;
eax = (eax & ~0xFFUL) | (((unsigned long)M8(0x444EB8)) & 0xFFUL); /* 0x00416FA9 mov al,byte_444EB8 */
eax = (eax & ~0xFFUL) | (((((eax & 0xFFUL) + 1) & 0xFFUL)) & 0xFFUL); /* 0x00416FAE inc al */
M8(0x444EB8) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416FB0 mov byte_444EB8,al */
if (((eax & 0xFFUL) & 0xFFUL) < 8UL) goto L_416F38; /* 0x00416FB5 cmp / 0x00416FB7 jb (unsigned <) */
L_416FBD:;
if ((unsigned long)M8(0x444EB8) != 8UL) goto L_416FD0; /* 0x00416FBD cmp / 0x00416FC4 jnz */
eax = (unsigned long)M32(0x4441E8); /* 0x00416FC6 mov eax,dword_4441E8 */
M32(0x4441F0) = (unsigned int)(eax & U32MASK); /* 0x00416FCB mov dword_4441F0,eax */
L_416FD0:;
eax = 0x4441E8UL; /* 0x00416FD0 mov eax,offset dword_4441E8 */
d3u_423140(c, 0x4441E8UL); /* 0x00416FD5 call sub_423140 */
L_416FDA:;
ebx = (ebx & ~0xFF00UL) | ((((var_40 & 0xFFUL)) & 0xFFUL) << 8); /* 0x00416FDA mov bh,[var_40] */
ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) & 0xFFUL) & 0xFFUL) << 8);
if (((((ebx >> 8) & 0xFFUL)) & 0xFFUL) != 0UL) goto L_41717D; /* 0x00416FDE test / 0x00416FE0 jnz */
eax = (eax & ~0xFFUL) | (((var_4C & 0xFFUL)) & 0xFFUL); /* 0x00416FE6 mov al,[var_4C] */
M8(0x444EA1) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00416FEA mov byte_444EA1,al */
if (((eax & 0xFFUL) & 0xFFUL) == 0UL) goto L_41716B; /* 0x00416FEF test / 0x00416FF1 jz */
if ((unsigned long)M8(0x444EA0) != 0UL) goto L_417006; /* 0x00416FF7 cmp / 0x00416FFE jnz */
M8(0x444EA1) = (unsigned char)(((ebx >> 8) & 0xFFUL) & 0xFFUL); /* 0x00417000 mov byte_444EA1,bh */
L_417006:;
var_40 = 1UL; /* 0x00417006 mov [var_40],1 */
if (var_50 == 0UL) goto L_41716B; /* 0x0041700B cmp / 0x00417010 jz */
edx = (unsigned long)M32(0x44423C); /* 0x00417016 mov edx,dword_44423C */
ebx = var_50 & U32MASK; /* 0x0041701C mov ebx,[var_50] */
edx = ((edx >> 2) & U32MASK); /* 0x00417020 shr edx,2 */
eax = 0UL; /* 0x00417023 xor eax,eax */
M32(0x44423C) = (unsigned int)(edx & U32MASK); /* 0x00417025 mov dword_44423C,edx */
L_41702B:;
edx = 0UL; /* 0x0041702B xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x0041702D mov dl,byte_444EAA */
edx &= 0xFFUL;
if (!((eax & U32MASK) < (edx & U32MASK))) goto L_41704F; /* 0x00417033 cmp / 0x00417035 jnb (unsigned <) */
edx = eax & U32MASK; /* 0x00417037 mov edx,eax */
edx = (edx & ~0xFFUL) | (((unsigned long)M8((unsigned long)(0x444E82UL + (edx & 0xFFUL)))) & 0xFFUL); /* 0x00417039 mov dl,byte_444E82[edx] */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL);
edx = (edx & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(ebx + 2))) & 0xFFUL) << 8); /* 0x0041703F mov dh,[ebx+2] */
edx = (edx & 0xFFFF00FFUL) | ((((edx >> 8) & 0xFFUL) & 0xFFUL) << 8);
eax = (eax + 1) & U32MASK; /* 0x00417042 inc eax */
if ((((edx & 0xFFUL)) & 0xFFUL) != ((((edx >> 8) & 0xFFUL)) & 0xFFUL)) goto L_41702B; /* 0x00417043 cmp / 0x00417045 jnz */
ebx = (ebx & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x00417047 xor bl,bl lane-merge */
ebx = (ebx & 0xFFFFFF00UL) | 0UL;
M8(0x444EA1) = (unsigned char)((ebx & 0xFFUL) & 0xFFUL); /* 0x00417049 mov byte_444EA1,bl */
L_41704F:;
if ((unsigned long)M8(0x444EA1) == 0UL) goto L_41716B; /* 0x0041704F cmp / 0x00417056 jz */
eax = (eax & ~0xFFUL) | (((var_48 & 0xFFUL)) & 0xFFUL); /* 0x0041705C mov al,[var_48] */
ecx = (ecx & ~0xFFUL) | (((var_2C & 0xFFUL)) & 0xFFUL); /* 0x00417060 mov cl,[var_2C] */
M8(0x444EB2) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x00417064 mov byte_444EB2,al */
if ((((ecx & 0xFFUL)) & 0xFFUL) != 0UL) goto L_41716B; /* 0x00417069 test / 0x0041706B jnz */
ebx = (unsigned long)M32(0x44427C); /* 0x00417071 mov ebx,dword_44427C */
if (ebx == 0UL) goto L_417082; /* 0x00417077 test / 0x00417079 jz */
edx = (edx & ~0xFFUL) | (((unsigned long)M8((unsigned long)ebx)) & 0xFFUL); /* 0x0041707B mov dl,[ebx] */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(ebx + 1))) & 0xFFUL); /* 0x0041707D mov al,[ebx+1] */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL); eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
goto L_41708F; /* 0x00417080 jmp */
L_417082:;
edx = var_50 & U32MASK; /* 0x00417082 mov edx,[var_50] */
eax = var_50 & U32MASK; /* 0x00417086 mov eax,[var_50] */
edx = (edx & ~0xFFUL) | (((unsigned long)M8((unsigned long)edx)) & 0xFFUL); /* 0x0041708A mov dl,[edx] */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)eax + 1)) & 0xFFUL); /* 0x0041708C mov al,[eax+1] */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL); eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
L_41708F:;
ebx = 0UL; /* 0x0041708F xor ebx,ebx */
ebx = (ebx & ~0xFFUL) | ((((edx & 0xFFUL) & 0xFFUL)) & 0xFFUL); /* 0x00417091 mov bl,dl */
ebx &= 0xFFUL;
eax &= 0xFFUL; /* 0x00417093 and eax,0FFh */
ecx = (ebx + eax) & U32MASK; /* 0x00417098 lea ecx,[ebx+eax] */
M32(0x4441E8) = (unsigned int)(ecx & U32MASK); /* 0x0041709B mov dword_4441E8,ecx */
eax = 0UL; /* 0x004170A1 xor eax,eax */
eax = (M32(0x4441E8) & 0xFFFFUL); /* 0x004170A3 mov ax,word ptr dword_4441E8 */
eax = d3u_423188(c, (unsigned int)(eax & 0xFFFFUL)); /* 0x004170A9 call sub_423188 */
eax &= 0xFFFFUL; /* 0x004170AE and eax,0FFFFh */
if (!((eax & U32MASK) < (ebx & U32MASK))) goto L_417110; /* 0x004170B3 cmp / 0x004170B5 jge (SIGNED >=, treated unsigned here per rule: jge is signed; keep signed) */
if ((unsigned long)M8(0x444EA4) == 0UL) goto L_4170EC; /* 0x004170B7 cmp / 0x004170BE jz */
eax = (unsigned long)M32(0x4441F8); /* 0x004170C0 mov eax,dword_4441F8 */
ebx = (unsigned long)M32(0x444208); /* 0x004170C5 mov ebx,dword_444208 */
esi = (unsigned long)M32((unsigned long)eax); /* 0x004170CB mov esi,[eax] */
esi = (esi + ebx) & U32MASK; /* 0x004170CD add esi,ebx */
edi = (ebx + ebx) & U32MASK; /* 0x004170CF lea edi,[ebx+ebx] */
M32((unsigned long)eax) = (unsigned int)(esi & U32MASK); /* 0x004170D2 mov [eax],esi */
M32(0x444208) = (unsigned int)(edi & U32MASK); /* 0x004170D4 mov dword_444208,edi */
if (!((edi & U32MASK) > 0xFFFFUL)) goto L_4170EC; /* 0x004170DA cmp / 0x004170E0 jbe (unsigned <=) */
M32(0x444208) = 0xFFFF; /* 0x004170E2 mov dword_444208,0FFFFh */
L_4170EC:;
eax = 0UL; /* 0x004170EC xor eax,eax */
eax = (eax & ~0xFFUL) | ((((edx & 0xFFUL) & 0xFFUL)) & 0xFFUL); /* 0x004170EE mov al,dl */
eax &= 0xFFUL;
M32(0x4441F0) = (unsigned int)(eax & U32MASK); /* 0x004170F0 mov dword_4441F0,eax */
eax = var_50 & U32MASK; /* 0x004170F5 mov eax,[var_50] */
eax = (eax & ~0xFFUL) | (((unsigned long)M8((unsigned long)(eax + 2))) & 0xFFUL); /* 0x004170F9 mov al,[eax+2] */
eax &= 0xFFUL;
ecx = 0UL; /* 0x004170FC xor ecx,ecx */
var_28 = (eax & 0xFFUL) & 0xFFUL; /* 0x004170FE mov [var_28],al */
eax = (eax & ~0xFF00UL) | (((0UL) & 0xFFUL) << 8); /* 0x00417102 xor ah,ah lane-merge */
eax = (eax & 0xFFFF00FFUL) | 0UL;
M32(0x4441EC) = 0; /* 0x00417104 mov dword_4441EC,ecx */
var_4C = (((eax >> 8) & 0xFFUL)) & 0xFFUL; /* 0x0041710A mov [var_4C],ah */
goto L_417161; /* 0x0041710E jmp */
L_417110:;
if ((unsigned long)M8(0x444EA4) == 0UL) goto L_417131; /* 0x00417110 cmp / 0x00417117 jz */
eax = (unsigned long)M32(0x4441F8); /* 0x00417119 mov eax,dword_4441F8 */
M32((unsigned long)eax) = (unsigned int)(((unsigned long)M32((unsigned long)eax) >> 1) & U32MASK); /* 0x0041711E shr dword ptr [eax],1 */
eax = (unsigned long)M32(0x444208); /* 0x00417120 mov eax,dword_444208 */
eax = ((eax >> 1) & U32MASK); /* 0x00417125 shr eax,1 */
eax = (eax + 0xFFUL) & U32MASK; /* 0x00417127 add eax,0FFh */
M32(0x444208) = (unsigned int)(eax & U32MASK); /* 0x0041712C mov dword_444208,eax */
L_417131:;
eax = 0UL; /* 0x00417131 xor eax,eax */
eax = (eax & ~0xFFUL) | ((((edx & 0xFFUL) & 0xFFUL)) & 0xFFUL); /* 0x00417133 mov al,dl */
eax &= 0xFFUL;
M32(0x4441EC) = (unsigned int)(eax & U32MASK); /* 0x00417135 mov dword_4441EC,eax */
eax = (unsigned long)M32(0x4441E8); /* 0x0041713A mov eax,dword_4441E8 */
edx = 0UL; /* 0x0041713F xor edx,edx */
M32(0x4441F0) = (unsigned int)(eax & U32MASK); /* 0x00417141 mov dword_4441F0,eax */
eax = (eax & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x00417146 mov al,byte_444EAA */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
ebx = var_50 & U32MASK; /* 0x0041714B mov ebx,[var_50] */
edx = (edx & ~0xFFUL) | (((eax & 0xFFUL) & 0xFFUL) & 0xFFUL); /* 0x0041714F mov dl,al */
edx &= 0xFFUL;
eax = (eax & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(ebx + 2))) & 0xFFUL) << 8); /* 0x00417151 mov ah,[ebx+2] */
eax = (eax & 0xFFFF00FFUL) | ((((eax >> 8) & 0xFFUL) & 0xFFUL) << 8);
M8((unsigned long)(0x444E82UL + (edx & 0xFFUL))) = (unsigned char)(((eax >> 8) & 0xFFUL) & 0xFFUL); /* 0x00417154 mov byte_444E82[edx],ah */
eax = (eax & ~0xFFUL) | (((((eax & 0xFFUL) + 1) & 0xFFUL)) & 0xFFUL); /* 0x0041715A inc al */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
M8(0x444EAA) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x0041715C mov byte_444EAA,al */
L_417161:;
eax = 0x4441E8UL; /* 0x00417161 mov eax,offset dword_4441E8 */
d3u_423140(c, 0x4441E8UL); /* 0x00417166 call sub_423140 */
L_41716B:;
if ((var_4C & 0xFFUL) == 0UL) goto L_41717D; /* 0x0041716B cmp / 0x00417170 jz */
if ((var_30 & 0xFFUL) != 0UL) goto L_416D69; /* 0x00417172 cmp / 0x00417177 jnz (loop iff !=0) */
goto L_41717D;
L_41717D:;
if ((var_4C & 0xFFUL) == 0UL) goto L_4173C0; /* 0x0041717D cmp / 0x00417182 jz */
ecx = 0x201UL; /* 0x00417188 mov ecx,201h */
esi = 0x300UL; /* 0x0041718D mov esi,300h */
eax = (eax & ~0xFFUL) | (((var_48 & 0xFFUL)) & 0xFFUL); /* 0x00417192 mov al,[var_48] */
ebx = (ebx & ~0xFF00UL) | ((((unsigned long)M8(0x444EA0)) & 0xFFUL) << 8); /* 0x00417196 mov bh,byte_444EA0 */
ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) & 0xFFUL) & 0xFFUL) << 8);
M8(0x444EAE) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x0041719C mov byte_444EAE,al */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
if (((((ebx >> 8) & 0xFFUL)) & 0xFFUL) == 0UL) goto L_417208; /* 0x004171A1 test / 0x004171A3 jz */
if (!((unsigned long)M8(0x444EBA) > 1UL)) goto L_417208; /* 0x004171A5 cmp / 0x004171AC jbe */
eax = (unsigned long)M32(0x4441F8); /* 0x004171AE mov eax,dword_4441F8 */
if ((unsigned long)M32((unsigned long)eax) == 0UL) goto L_417208; /* 0x004171B3 cmp / 0x004171B6 jz */
eax = 0UL; /* 0x004171B8 xor eax,eax */
edx = (unsigned long)M32(0x444274); /* 0x004171BA mov edx,dword_444274 */
eax = (eax & ~0xFFUL) | (((unsigned long)M8(0x444EBA)) & 0xFFUL); /* 0x004171C0 mov al,byte_444EBA */
eax &= 0xFFUL;
ebp = (unsigned long)M32(0x444278); /* 0x004171C5 mov ebp,dword_444278 */
edx = (edx - eax) & U32MASK; /* 0x004171CB sub edx,eax */
eax = (unsigned long)M32(0x4299B8); /* 0x004171CD mov eax,dword_4299B8 */
edx &= ebp; /* 0x004171D2 and edx,ebp */
ebx = (ebx & ~0xFFUL) | (((unsigned long)M8((unsigned long)(edx + eax))) & 0xFFUL); /* 0x004171D4 mov bl,[edx+eax] */
ebx = (ebx & 0xFFFFFF00UL) | ((ebx & 0xFFUL) & 0xFFUL);
edx = 0UL; /* 0x004171D7 xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EBA)) & 0xFFUL); /* 0x004171D9 mov dl,byte_444EBA */
edx &= 0xFFUL;
edi = (unsigned long)M32(0x444274); /* 0x004171DF mov edi,dword_444274 */
edx = (edx + edx) & U32MASK; /* 0x004171E5 add edx,edx */
edi = (edi - edx) & U32MASK; /* 0x004171E7 sub edi,edx */
edx = edi & U32MASK; /* 0x004171E9 mov edx,edi */
edx &= (ebp & U32MASK); /* 0x004171EB and edx,dword_444278 */
ebp = ((ebx & 0xFFUL) & 0xFFUL); /* 0x004171F1 movzx ebp,bl */
ebx = (ebx & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(edx + eax))) & 0xFFUL) << 8); /* 0x004171F4 mov bh,[edx+eax] */
ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) & 0xFFUL) & 0xFFUL) << 8);
ebp = (ebp + 0x100UL) & U32MASK; /* 0x004171F7 add ebp,100h */
if ((((ebx & 0xFFUL)) & 0xFFUL) != ((((ebx >> 8) & 0xFFUL)) & 0xFFUL)) goto L_417243; /* 0x004171FD cmp / 0x004171FF jnz */
ecx = 0x202UL; /* 0x00417201 mov ecx,202h */
goto L_417243; /* 0x00417206 jmp */
L_417208:;
eax = (unsigned long)M32(0x444240); /* 0x00417208 mov eax,dword_444240 */
ebx = (ebx & ~0xFFUL) | (((eax & 0xFFUL)) & 0xFFUL); /* 0x0041720D mov bl,byte ptr dword_444240 */
ebx = (ebx & 0xFFFFFF00UL) | ((ebx & 0xFFUL) & 0xFFUL);
eax = ((eax >> 8) & U32MASK); /* 0x00417213 shr eax,8 */
ebp = ((ebx & 0xFFUL) & 0xFFUL); /* 0x00417216 movzx ebp,bl */
if ((((ebx & 0xFFUL)) & 0xFFUL) == ((eax) & 0xFFUL)) goto L_41721D_inc; /* 0x00417219 cmp / 0x0041721B jnz inverted */
goto L_41721E;
L_41721D_inc:;
ecx = (ecx + 1) & U32MASK; /* 0x0041721D inc ecx */
L_41721E:;
if ((unsigned long)M8(0x444EA0) != 0UL) goto L_417243; /* 0x0041721E cmp / 0x00417225 jnz */
eax = (unsigned long)M32(0x444240); /* 0x00417227 mov eax,dword_444240 */
eax = ((eax >> 8) & U32MASK); /* 0x0041722C shr eax,8 */
eax &= 0xFFUL; /* 0x0041722F and eax,0FFh */
if ((unsigned long)M8((unsigned long)(0x42C6A6UL + (eax & 0xFFUL))) == 0UL) goto L_417243; /* 0x00417234 cmp / 0x0041723B jz */
ebp = (ebp + 0x100UL) & U32MASK; /* 0x0041723D add ebp,100h */
L_417243:;
if ((var_3C & 0xFFUL) == 0UL) goto L_417263; /* 0x00417243 cmp / 0x00417248 jz */
ecx = (unsigned long)M32(0x4441C4); /* 0x0041724A mov ecx,dword_4441C4 */
esi = (unsigned long)M8(0x444EA9); /* 0x00417250 movzx esi,byte_444EA9 */
ebx = (ebx & ~0xFFUL) | (((unsigned long)M8(0x444E9D)) & 0xFFUL); /* 0x00417257 mov bl,byte_444E9D */
ebx = (ebx & 0xFFFFFF00UL) | ((ebx & 0xFFUL) & 0xFFUL);
esi = (esi + 0x300UL) & U32MASK; /* 0x0041725D add esi,300h */
L_417263:;
if ((var_3C & 0xFFUL) == 0UL) goto L_417277; /* 0x00417263 cmp / 0x00417268 jz */
if ((unsigned long)M8(0x444EA8) == 0UL) goto L_417277; /* 0x0041726A cmp / 0x00417271 jz */
ebx = (ebx & ~0xFF00UL) | (((1UL) & 0xFFUL) << 8); /* 0x00417273 mov bh,1 */
ebx = (ebx & 0xFFFF00FFUL) | (1UL << 8);
goto L_417279; /* 0x00417275 jmp */
L_417277:;
ebx = (ebx & ~0xFF00UL) | (((0UL) & 0xFFUL) << 8); /* 0x00417277 xor bh,bh */
ebx = (ebx & 0xFFFF00FFUL) | 0UL;
L_417279:;
edx = (edx & ~0xFF00UL) | ((((var_1C & 0xFFUL)) & 0xFFUL) << 8); /* 0x00417279 mov dh,[var_1C] */
var_20 = (((ebx >> 8) & 0xFFUL)) & 0xFFUL; /* 0x0041727D mov [var_20],bh */
if (((((edx >> 8) & 0xFFUL)) & 0xFFUL) == 0UL) goto L_417348; /* 0x00417281 test / 0x00417283 jz */
eax = 0UL; /* 0x00417289 xor eax,eax */
if (((((ebx >> 8) & 0xFFUL)) & 0xFFUL) == 0UL) goto L_4172AE; /* 0x0041728B test / 0x0041728D jz */
L_41728F:;
edx = 0UL; /* 0x0041728F xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x00417291 mov dl,byte_444EAA */
edx &= 0xFFUL;
if (!((eax & U32MASK) < (edx & U32MASK))) goto L_4172CD; /* 0x00417299 cmp / jnb via 0x00417299 jnb */
edx = (edx & ~0xFFUL) | ((((ebx & 0xFFUL) & 0xFFUL)) & 0xFFUL); /* 0x0041729B mov dl,bl */
edx = (edx & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(0x444E82UL + (eax & 0xFFUL)))) & 0xFFUL) << 8); /* 0x0041729D mov dh,byte_444E82[eax] */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL); edx = (edx & 0xFFFF00FFUL) | ((((edx >> 8) & 0xFFUL) & 0xFFUL) << 8);
eax = (eax + 1) & U32MASK; /* 0x004172A3 inc eax */
edx = (edx & ~0xFFUL) | (((((edx & 0xFFUL) - ((edx >> 8) & 0xFFUL)) & 0xFFUL)) & 0xFFUL); /* 0x004172A4 sub dl,dh */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL);
M8((unsigned long)(0x444E81UL + (eax & 0xFFUL))) = (unsigned char)((edx & 0xFFUL) & 0xFFUL); /* 0x004172A6 mov byte_444E81[eax],dl */
goto L_41728F; /* 0x004172AC jmp */
L_4172AE:;
edx = 0UL; /* 0x004172AE xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x004172B0 mov dl,byte_444EAA */
edx &= 0xFFUL;
if (!((eax & U32MASK) < (edx & U32MASK))) goto L_4172CD; /* 0x004172B6 cmp / 0x004172B8 jnb (unsigned <) */
edx = eax & U32MASK; /* 0x004172BA mov edx,eax */
ebx = (ebx & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(0x444E82UL + (edx & 0xFFUL)))) & 0xFFUL) << 8); /* 0x004172BC mov bh,byte_444E82[edx] */
ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) & 0xFFUL) & 0xFFUL) << 8);
ebx = (ebx & ~0xFF00UL) | (((((((ebx >> 8) & 0xFFUL) - (ebx & 0xFFUL)) & 0xFFUL)) & 0xFFUL) << 8); /* 0x004172C2 sub bh,bl */
ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) & 0xFFUL) & 0xFFUL) << 8);
eax = (eax + 1) & U32MASK; /* 0x004172C4 inc eax */
M8((unsigned long)(0x444E82UL + (edx & 0xFFUL))) = (unsigned char)(((ebx >> 8) & 0xFFUL) & 0xFFUL); /* 0x004172C5 mov byte_444E82[edx],bh */
goto L_4172AE; /* 0x004172CB jmp */
L_4172CD:;
edx = esi & U32MASK; /* 0x004172CD mov edx,esi */
eax = ecx & U32MASK; /* 0x004172CF mov eax,ecx */
ebx = (ebx & ~0xFF00UL) | ((((var_20 & 0xFFUL)) & 0xFFUL) << 8); /* 0x004172D1 mov bh,[var_20] */
ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) & 0xFFUL) & 0xFFUL) << 8);
eax = (eax & ~0xFFUL) | (d3u_4174fc(c, (unsigned int)eax, (unsigned int)edx) & 0xFFUL); /* 0x004172D5 call sub_4174FC */
if (((((ebx >> 8) & 0xFFUL)) & 0xFFUL) == 0UL) goto L_4172E8; /* 0x004172DA test / 0x004172DC jz */
eax = (eax & ~0xFF00UL) | (((((ebx & 0xFFUL) & 0xFFUL)) & 0xFFUL) << 8); /* 0x004172DE mov ah,bl */
eax = (eax & ~0xFF00UL) | (((((((eax >> 8) & 0xFFUL) - ((eax & 0xFFUL) & 0xFFUL)) & 0xFFUL)) & 0xFFUL) << 8); /* 0x004172E0 sub ah,al */
eax = (eax & 0xFFFF00FFUL) | ((((eax >> 8) & 0xFFUL) & 0xFFUL) << 8);
var_28 = ((eax >> 8) & 0xFFUL) & 0xFFUL; /* 0x004172E2 mov [var_28],ah */
goto L_4172EE; /* 0x004172E6 jmp */
L_4172E8:;
eax = (eax & ~0xFFUL) | (((((eax & 0xFFUL) + (ebx & 0xFFUL)) & 0xFFUL)) & 0xFFUL); /* 0x004172E8 add al,bl */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
var_28 = (eax & 0xFFUL) & 0xFFUL; /* 0x004172EA mov [var_28],al */
L_4172EE:;
ecx = (ecx & ~0xFFUL) | (((var_20 & 0xFFUL)) & 0xFFUL); /* 0x004172EE mov cl,[var_20] */
ecx = (ecx & 0xFFFFFF00UL) | ((ecx & 0xFFUL) & 0xFFUL);
eax = 0UL; /* 0x004172F2 xor eax,eax */
if ((((ecx & 0xFFUL)) & 0xFFUL) == 0UL) goto L_417317; /* 0x004172F4 test / 0x004172F6 jz */
L_4172F8:;
edx = 0UL; /* 0x004172F8 xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x004172FA mov dl,byte_444EAA */
edx &= 0xFFUL;
if (!((eax & U32MASK) < (edx & U32MASK))) goto L_417336; /* 0x00417300 cmp / 0x00417302 jnb (unsigned <) */
edx = (edx & ~0xFFUL) | ((((ebx & 0xFFUL) & 0xFFUL)) & 0xFFUL); /* 0x00417304 mov dl,bl */
edx = (edx & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(0x444E82UL + (eax & 0xFFUL)))) & 0xFFUL) << 8); /* 0x00417306 mov dh,byte_444E82[eax] */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL); edx = (edx & 0xFFFF00FFUL) | ((((edx >> 8) & 0xFFUL) & 0xFFUL) << 8);
eax = (eax + 1) & U32MASK; /* 0x0041730D inc eax (via 0x0041730C) */
edx = (edx & ~0xFFUL) | (((((edx & 0xFFUL) - ((edx >> 8) & 0xFFUL)) & 0xFFUL)) & 0xFFUL); /* 0x0041730D sub dl,dh */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL);
M8((unsigned long)(0x444E81UL + (eax & 0xFFUL))) = (unsigned char)((edx & 0xFFUL) & 0xFFUL); /* 0x0041730F mov byte_444E81[eax],dl */
goto L_4172F8; /* 0x00417315 jmp */
L_417317:;
edx = 0UL; /* 0x00417317 xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x00417319 mov dl,byte_444EAA */
edx &= 0xFFUL;
if (!((eax & U32MASK) < (edx & U32MASK))) goto L_417336; /* 0x0041731F cmp / 0x00417321 jnb (unsigned <) */
edx = eax & U32MASK; /* 0x00417323 mov edx,eax */
ecx = (ecx & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(0x444E82UL + (edx & 0xFFUL)))) & 0xFFUL) << 8); /* 0x00417325 mov ch,byte_444E82[edx] */
ecx = (ecx & 0xFFFF00FFUL) | ((((ecx >> 8) & 0xFFUL) & 0xFFUL) << 8);
ecx = (ecx & ~0xFF00UL) | (((((((ecx >> 8) & 0xFFUL) + (ebx & 0xFFUL)) & 0xFFUL)) & 0xFFUL) << 8); /* 0x0041732B add ch,bl */
ecx = (ecx & 0xFFFF00FFUL) | ((((ecx >> 8) & 0xFFUL) & 0xFFUL) << 8);
eax = (eax + 1) & U32MASK; /* 0x0041732D inc eax */
M8((unsigned long)(0x444E82UL + (edx & 0xFFUL))) = (unsigned char)(((ecx >> 8) & 0xFFUL) & 0xFFUL); /* 0x0041732E mov byte_444E82[edx],ch */
goto L_417317; /* 0x00417334 jmp */
L_417336:;
edx = 0x2FDUL; /* 0x00417336 mov edx,2FDh */
ebx = 0UL; /* 0x0041733B xor ebx,ebx */
eax = ebp & U32MASK; /* 0x0041733D mov eax,ebp */
ebx = (ebx & ~0xFFUL) | (((var_28 & 0xFFUL)) & 0xFFUL); /* 0x0041733F mov bl,[var_28] */
ebx = (ebx & 0xFFFFFF00UL) | ((ebx & 0xFFUL) & 0xFFUL); /* lane-merge (split bl) */
ebx &= 0xFFUL;
goto L_4173BB; /* 0x00417343 jmp */
L_417348:;
edx = 0x2FDUL; /* 0x00417348 mov edx,2FDh */
eax = ebp & U32MASK; /* 0x0041734D mov eax,ebp */
eax = (eax & ~0xFFUL) | (d3u_4174fc(c, (unsigned int)eax, (unsigned int)edx) & 0xFFUL); /* 0x0041734F call sub_4174FC */
var_28 = (eax & 0xFFUL) & 0xFFUL; /* 0x00417354 mov [var_28],al */
if ((unsigned long)M8(0x444EA0) == 0UL) goto L_4173C0; /* 0x00417358 cmp / 0x0041735F jz */
if (((((ebx >> 8) & 0xFFUL)) & 0xFFUL) == 0UL) goto L_41736B; /* 0x00417361 test / 0x00417363 jz */
ebx = (ebx & ~0xFF00UL) | (((((ebx & 0xFFUL) & 0xFFUL)) & 0xFFUL) << 8); /* 0x00417365 mov bh,bl */
ebx = (ebx & ~0xFF00UL) | (((((((ebx >> 8) & 0xFFUL) - ((eax & 0xFFUL) & 0xFFUL)) & 0xFFUL)) & 0xFFUL) << 8); /* 0x00417367 sub bh,al */
ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) & 0xFFUL) & 0xFFUL) << 8);
goto L_41736F; /* 0x00417369 jmp */
L_41736B:;
ebx = (ebx & ~0xFF00UL) | ((((eax & 0xFFUL) & 0xFFUL) & 0xFFUL) << 8); /* 0x0041736B mov bh,al */
ebx = (ebx & ~0xFF00UL) | (((((((ebx >> 8) & 0xFFUL) - ((ebx & 0xFFUL) & 0xFFUL)) & 0xFFUL)) & 0xFFUL) << 8); /* 0x0041736D sub bh,bl */
ebx = (ebx & 0xFFFF00FFUL) | ((((ebx >> 8) & 0xFFUL) & 0xFFUL) << 8);
L_41736F:;
edx = (edx & ~0xFFUL) | (((var_20 & 0xFFUL)) & 0xFFUL); /* 0x0041736F mov dl,[var_20] */
eax = 0UL; /* 0x00417373 xor eax,eax */
if ((((edx & 0xFFUL)) & 0xFFUL) == 0UL) goto L_417398; /* 0x00417375 test / 0x00417377 jz */
L_417379:;
edx = 0UL; /* 0x00417379 xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x0041737B mov dl,byte_444EAA */
edx &= 0xFFUL;
if (!((eax & U32MASK) < (edx & U32MASK))) goto L_4173AF; /* 0x00417381 cmp / 0x00417383 jnb (unsigned <) */
edx = (edx & ~0xFFUL) | ((((ebx & 0xFFUL) & 0xFFUL)) & 0xFFUL); /* 0x00417385 mov dl,bl */
edx = (edx & ~0xFF00UL) | ((((unsigned long)M8((unsigned long)(0x444E82UL + (eax & 0xFFUL)))) & 0xFFUL) << 8); /* 0x00417387 mov dh,byte_444E82[eax] */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL); edx = (edx & 0xFFFF00FFUL) | ((((edx >> 8) & 0xFFUL) & 0xFFUL) << 8);
eax = (eax + 1) & U32MASK; /* 0x0041738D inc eax */
edx = (edx & ~0xFFUL) | (((((edx & 0xFFUL) - ((edx >> 8) & 0xFFUL)) & 0xFFUL)) & 0xFFUL); /* 0x0041738E sub dl,dh */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL);
M8((unsigned long)(0x444E81UL + (eax & 0xFFUL))) = (unsigned char)((edx & 0xFFUL) & 0xFFUL); /* 0x00417390 mov byte_444E81[eax],dl */
goto L_417379; /* 0x00417396 jmp */
L_417398:;
edx = 0UL; /* 0x00417398 xor edx,edx */
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EAA)) & 0xFFUL); /* 0x0041739A mov dl,byte_444EAA */
edx &= 0xFFUL;
if (!((eax & U32MASK) < (edx & U32MASK))) goto L_4173AF; /* 0x004173A0 cmp / 0x004173A2 jnb (unsigned <) */
edx = eax & U32MASK; /* 0x004173A4 mov edx,eax */
eax = (eax + 1) & U32MASK; /* 0x004173A6 inc eax */
tmp = (unsigned long)M8((unsigned long)(0x444E82UL + (edx & 0xFFUL))); /* 0x004173A7 sub byte_444E82[edx],bl */
tmp = ((tmp - ((ebx & 0xFFUL) & 0xFFUL)) & 0xFFUL);
M8((unsigned long)(0x444E82UL + (edx & 0xFFUL))) = (unsigned char)tmp;
goto L_417398; /* 0x004173AD jmp */
L_4173AF:;
ebx = (ebx & ~0xFFUL) | (((((ebx >> 8) & 0xFFUL) & 0xFFUL)) & 0xFFUL); /* 0x004173AF mov bl,bh */
ebx = (ebx & 0xFFFFFF00UL) | ((ebx & 0xFFUL) & 0xFFUL);
ebx &= 0xFFUL; /* 0x004173B1 and ebx,0FFh */
edx = esi & U32MASK; /* 0x004173B7 mov edx,esi */
eax = ecx & U32MASK; /* 0x004173B9 mov eax,ecx */
L_4173BB:;
d3u_41855c(c, (unsigned int)eax, (unsigned int)edx, (unsigned int)ebx); /* 0x004173BB call sub_41855C */
L_4173C0:;
edx = 0UL; /* 0x004173C0 xor edx,edx */
edx = (edx & ~0xFFUL) | (((var_28 & 0xFFUL)) & 0xFFUL); /* 0x004173C2 mov dl,[var_28] */
edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFUL) & 0xFFUL); /* lane-merge (split dl) */
edx &= 0xFFUL;
eax = edx & U32MASK; /* 0x004173C6 mov eax,edx */
d3u_415534(c, (unsigned int)eax); /* 0x004173C8 call sub_415534 */
if ((unsigned long)M8(0x444EAC) == 0UL) goto L_4173DD; /* 0x004173CD cmp / 0x004173D4 jz */
eax = edx & U32MASK; /* 0x004173D6 mov eax,edx */
d3u_415800(c, (unsigned int)eax); /* 0x004173D8 call sub_415800 */
L_4173DD:;
if ((unsigned long)M8(0x444EA1) == 0UL) goto L_4173F1; /* 0x004173DD cmp / 0x004173E4 jz */
eax = 0UL; /* 0x004173E6 xor eax,eax */
eax = (eax & ~0xFFUL) | (((var_28 & 0xFFUL)) & 0xFFUL); /* 0x004173E8 mov al,[var_28] */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL); /* lane-merge (split al) */
eax &= 0xFFUL;
d3u_4153c0(c, (unsigned int)eax); /* 0x004173EC call sub_4153C0 */
L_4173F1:;
if ((var_48 & 0xFFUL) == 0UL) goto L_417464; /* 0x004173F1 cmp / 0x004173F6 jz */
eax = (eax & ~0xFFUL) | (((var_28 & 0xFFUL)) & 0xFFUL); /* 0x004173F8 mov al,[var_28] */
eax = (eax & ~0xFFUL) | (((((eax & 0xFFUL) - (unsigned long)M8(0x444E9D)) & 0xFFUL)) & 0xFFUL); /* 0x004173FC sub al,byte_444E9D */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
if ((unsigned long)M8(0x444EA8) == 0UL) goto L_41740D; /* 0x00417402 cmp / 0x00417409 jz */
eax = (eax & ~0xFFUL) | ((((0UL - (eax & 0xFFUL)) & 0xFFUL)) & 0xFFUL); /* 0x0041740B neg al */
eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFUL) & 0xFFUL);
L_41740D:;
edx = (edx & ~0xFFUL) | ((0UL) & 0xFFUL); /* 0x0041740D xor dl,dl */
ebx = 0UL; /* 0x0041740F xor ebx,ebx */
M8(0x444EAA) = (unsigned char)((edx & 0xFFUL) & 0xFFUL); /* 0x00417411 mov byte_444EAA,dl */
edx = 0UL; /* 0x00417417 xor edx,edx */
ebx = (ebx & ~0xFFUL) | (((eax & 0xFFUL) & 0xFFUL) & 0xFFUL); /* 0x00417419 mov bl,al */
ebx &= 0xFFUL;
edx = (edx & ~0xFFUL) | (((unsigned long)M8(0x444EA9)) & 0xFFUL); /* 0x0041741B mov dl,byte_444EA9 */
edx &= 0xFFUL;
eax = (unsigned long)M32(0x4441C4); /* 0x00417421 mov eax,dword_4441C4 */
edx = (edx + 0x300UL) & U32MASK; /* 0x00417426 add edx,300h */
d3u_418620(c, (unsigned int)edx, (unsigned int)ebx); /* 0x0041742C call sub_418620 */
if ((unsigned long)M8(0x444EAE) == 0UL) goto L_417445; /* 0x00417431 cmp / 0x00417438 jz */
eax = (unsigned long)M32(0x44420C); /* 0x0041743A mov eax,dword_44420C */
M32(0x444204) = (unsigned int)(((unsigned long)M32(0x444204) + eax) & U32MASK); /* 0x0041743F add dword_444204,eax */
L_417445:;
edx = (unsigned long)M32(0x444200); /* 0x00417445 mov edx,dword_444200 */
eax = edx & U32MASK; /* 0x0041744B mov eax,edx */
eax = ((eax << 5) & U32MASK); /* 0x0041744D shl eax,5 */
eax = (eax - edx) & U32MASK; /* 0x00417450 sub eax,edx */
edx = eax & U32MASK; /* 0x00417452 mov edx,eax */
eax = (unsigned long)M32(0x444204); /* 0x00417454 mov eax,dword_444204 */
edx = ((edx >> 5) & U32MASK); /* 0x00417459 shr edx,5 */
edx = (edx + eax) & U32MASK; /* 0x0041745C add edx,eax */
M32(0x444200) = (unsigned int)(edx & U32MASK); /* 0x0041745E mov dword_444200,edx */
L_417464:;
ebx = (ebx & ~0xFFUL) | (((unsigned long)M8(0x444EB3)) & 0xFFUL); /* 0x00417464 mov bl,byte_444EB3 */
ebx = (ebx & ~0xFFUL) | (((((ebx & 0xFFUL) - 1) & 0xFFUL)) & 0xFFUL); /* 0x0041746A dec bl */
M8(0x444EB3) = (unsigned char)((ebx & 0xFFUL) & 0xFFUL); /* 0x0041746C mov byte_444EB3,bl */
if ((((ebx & 0xFFUL)) & 0xFFUL) == 0xFFUL) goto L_4174A0; /* 0x00417472 cmp / 0x00417475 jz (==0FFh after dec) */
eax = 0UL; /* 0x00417477 xor eax,eax */
eax = (eax & ~0xFFUL) | (((ebx & 0xFFUL) & 0xFFUL) & 0xFFUL); /* 0x00417479 mov al,bl */
eax &= 0xFFUL;
ebx = (unsigned long)M32((unsigned long)(0x441F08UL + (eax & 0xFFUL) * 4UL)); /* 0x0041747B mov ebx,dword_441F08[eax*4] static array */
M32(0x44421C) = (unsigned int)(ebx & U32MASK); /* 0x00417482 mov dword_44421C,ebx */
ebx = 0UL; /* 0x00417488 xor ebx,ebx */
ebx = (ebx & ~0xFFUL) | (((unsigned long)M8((unsigned long)(0x4441D8UL + (eax & 0xFFUL)))) & 0xFFUL); /* 0x0041748A mov bl,byte_4441D8[eax] static array */
ebx &= 0xFFUL;
edx = ebx & U32MASK; /* 0x00417490 mov edx,ebx */
eax = (unsigned long)M32((unsigned long)(0x442018UL + (eax & 0xFFUL) * 4UL)); /* 0x00417492 mov eax,dword_442018[eax*4] static array */
d3u_414ec0(c, (unsigned int)(eax & U32MASK), (unsigned int)(edx & U32MASK)); /* 0x00417499 call sub_414EC0 */
goto L_417464; /* 0x0041749E jmp */
L_4174A0:;
ebx = 0UL; /* 0x004174A0 xor ebx,ebx */
ebx = (ebx & ~0xFFUL) | (((var_28 & 0xFFUL)) & 0xFFUL); /* 0x004174A2 mov bl,[var_28] */
ebx = (ebx & 0xFFFFFF00UL) | ((ebx & 0xFFUL) & 0xFFUL); /* lane-merge (split bl) */
ebx &= 0xFFUL;
eax = ebx & U32MASK; /* 0x004174A6 mov eax,ebx */
edx = 1UL; /* 0x004174A8 mov edx,1 */
d3u_4164b8(c, (unsigned int)(eax & U32MASK)); /* 0x004174AD call sub_4164B8 */
eax = ebx & U32MASK; /* 0x004174B2 mov eax,ebx */
d3u_4159f4(c, (unsigned int)(eax & U32MASK), 1); /* 0x004174B4 call sub_4159F4 */
edx = (unsigned long)M32(0x4299B8); /* 0x004174B9 mov edx,dword_4299B8 */
eax = (unsigned long)M32(0x444274); /* 0x004174BF mov eax,dword_444274 */
M8((unsigned long)(edx + (eax & U32MASK))) = (unsigned char)((ebx & 0xFFUL) & 0xFFUL); /* 0x004174C4 mov [edx+eax],bl */
if (!((eax & U32MASK) < 0x100UL)) goto L_4174DC; /* 0x004174C7 cmp / 0x004174CC jnb (unsigned <) */
eax = (eax + (unsigned long)M32(0x444270)) & U32MASK; /* 0x004174CE add eax,dword_444270 */
edx = (edx + eax) & U32MASK; /* 0x004174D4 add edx,eax */
eax = (eax & ~0xFFUL) | (((var_28 & 0xFFUL)) & 0xFFUL); /* 0x004174D6 mov al,[var_28] */
M8((unsigned long)edx) = (unsigned char)((eax & 0xFFUL) & 0xFFUL); /* 0x004174DA mov [edx],al */
L_4174DC:;
eax = (unsigned long)M32(0x444274); /* 0x004174DC mov eax,dword_444274 */
edx = (unsigned long)M32(0x444278); /* 0x004174E1 mov edx,dword_444278 */
eax = (eax + 1) & U32MASK; /* 0x004174E7 inc eax */
eax &= edx; /* 0x004174E8 and eax,edx */
M32(0x444274) = (unsigned int)(eax & U32MASK); /* 0x004174EA mov dword_444274,eax */
}

/* ---- length/offset coders (from draft_l3.c) ---- */
/* sub_4164B8: entry al=sym live (dl=al, dl=~al). Escape flag byte_444EA7.
 * Tables: 42C492 (mapped bytes), 42C438 (class), 42BDDC (dword ptrs),
 * 42BF6C (lengths). Calls sub_41E3BC. Void return.
 */
void d3u_4164b8(dec3w_ctx *c, unsigned int sym)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    unsigned long esi = 0;
    /* 0x004164B8 push ebx/ecx/edx/esi (frame only) */
    /* 0x004164BC mov dl,al */
    edx = (edx & ~0xFFUL) | (sym & 0xFFUL);
    /* 0x004164BE mov ah,byte_444EA7 */
    eax = (sym & 0xFFUL);
    eax = (eax & ~0xFF00UL) | (((unsigned long)M8(0x444EA7) & 0xFFUL) << 8);
    /* 0x004164C4 not dl */
    edx = (edx & ~0xFFUL) | ((~(edx & 0xFFUL)) & 0xFFUL);
    /* 0x004164C6 cmp ah,1 */
    /* 0x004164C9 jnz loc_416562 */
    if (((eax >> 8) & 0xFFUL) != 1UL) goto l_416562;
    /* 0x004164CF mov eax,dword_444EF8 */
    eax = (unsigned long)M32(0x444EF8);
    /* 0x004164D4 xor dh,dh */
    edx = edx & ~0xFF00UL;
    /* 0x004164D6 mov esi,dword_444228 */
    esi = (unsigned long)M32(0x444228);
    /* 0x004164DC mov byte_444EA7,dh */
    M8(0x444EA7) = (unsigned char)((edx >> 8) & 0xFFUL);
    /* 0x004164E2 cmp eax,esi */
    /* 0x004164E4 jb loc_41652A */
    if (eax < esi) goto l_41652a;
    /* 0x004164E6 cmp dl,0FEh */
    /* 0x004164E9 jnz loc_4164FA */
    if ((edx & 0xFFUL) != 0xFEUL) goto l_4164fa;
    /* 0x004164EB mov eax,0FEh */
    eax = 0xFEUL;
    /* 0x004164F0 call sub_41E3BC */
    d3u_41e3bc(c, (unsigned int)(eax & U32MASK));
    /* 0x004164F5 pop esi/edx/ecx/ebx */
    /* 0x004164F9 retn */
    return;
l_4164fa:
    /* 0x004164FA xor eax,eax */
    eax = 0;
    /* 0x004164FC call sub_41E3BC */
    d3u_41e3bc(c, (unsigned int)(eax & U32MASK));
    /* 0x00416501 xor eax,eax */
    eax = 0;
    /* 0x00416503 call sub_41E3BC */
    d3u_41e3bc(c, (unsigned int)(eax & U32MASK));
    /* 0x00416508 mov eax,3 */
    eax = 3;
    /* 0x0041650D call sub_41E3BC */
    d3u_41e3bc(c, (unsigned int)(eax & U32MASK));
    /* 0x00416512 mov eax,3 */
    eax = 3;
    /* 0x00416517 call sub_41E3BC */
    d3u_41e3bc(c, (unsigned int)(eax & U32MASK));
l_41651c:
    /* 0x0041651C xor eax,eax */
    eax = 0;
    /* 0x0041651E mov al,dl */
    eax = (eax & ~0xFFUL) | (edx & 0xFFUL);
    /* 0x00416520 call sub_41E3BC */
    d3u_41e3bc(c, (unsigned int)(eax & U32MASK));
    /* 0x00416525 pop esi/edx/ecx/ebx */
    /* 0x00416529 retn */
    return;
l_41652a:
    /* 0x0041652A xor ebx,ebx */
    ebx = 0;
    /* 0x0041652C mov bl,byte_444EAD */
    ebx = (ebx & ~0xFFUL) | ((unsigned long)M8(0x444EAD) & 0xFFUL);
    /* 0x00416532 sub ebx,0A6h */
    ebx = (ebx - 0xA6UL) & U32MASK;
    /* 0x00416538 lea eax,ds:0[ebx*4] */
    eax = (ebx * 4UL) & U32MASK;
    /* 0x0041653F sub eax,ebx */
    eax = (eax - ebx) & U32MASK;
    /* 0x00416541 shl eax,2 */
    eax = (eax << 2) & U32MASK;
    /* 0x00416544 sub eax,ebx */
    eax = (eax - ebx) & U32MASK;
    /* 0x00416546 mov ebx,eax */
    ebx = eax;
    /* 0x00416548 xor eax,eax */
    eax = 0;
    /* 0x0041654A mov al,dl */
    eax = (eax & ~0xFFUL) | (edx & 0xFFUL);
    /* 0x0041654C mov al,byte_42C492[ebx+eax] */
    eax = (eax & ~0xFFUL) | ((unsigned long)G8((0x42C492UL + ebx + (eax & 0xFFUL)) & U32MASK) & 0xFFUL);
    /* 0x00416553 and eax,0FFh */
    eax &= 0xFFUL;
    /* 0x00416558 call sub_41E3BC */
    d3u_41e3bc(c, (unsigned int)(eax & U32MASK));
    /* 0x0041655D pop esi/edx/ecx/ebx */
    /* 0x00416561 retn */
    return;
l_416562:
    /* 0x00416562 mov eax,dword_444EF8 */
    eax = (unsigned long)M32(0x444EF8);
    /* 0x00416567 cmp eax,dword_444228 */
    /* 0x0041656D jb loc_416580 */
    if (eax < (unsigned long)M32(0x444228)) goto l_416580;
    /* 0x0041656F cmp dl,0FEh */
    /* 0x00416572 jnz loc_416580 */
    if ((edx & 0xFFUL) != 0xFEUL) goto l_416580;
    /* 0x00416574 mov byte_444EA7,1 */
    M8(0x444EA7) = 1;
    /* 0x0041657B pop esi/edx/ecx/ebx */
    /* 0x0041657F retn */
    return;
l_416580:
    /* 0x00416580 mov eax,dword_444EF8 */
    eax = (unsigned long)M32(0x444EF8);
    /* 0x00416585 cmp eax,dword_428EDC */
    /* 0x0041658B jnb loc_41651C */
    if (eax >= (unsigned long)M32(0x428EDC)) goto l_41651c;
    /* 0x0041658D cmp dl,80h */
    /* 0x00416590 jnb loc_41659B */
    if ((edx & 0xFFUL) >= 0x80UL) goto l_41659b;
    /* 0x00416592 cmp byte_444EA7,0 */
    /* 0x00416599 jz loc_41651C */
    if (M8(0x444EA7) == 0) goto l_41651c;
l_41659b:
    /* 0x0041659B xor eax,eax */
    eax = 0;
    /* 0x0041659D mov al,dl */
    eax = (eax & ~0xFFUL) | (edx & 0xFFUL);
    /* 0x0041659F mov al,byte_42C438[eax] */
    eax = (eax & ~0xFFUL) | ((unsigned long)G8((0x42C438UL + (eax & 0xFFUL)) & U32MASK) & 0xFFUL);
    /* 0x004165A5 cmp al,0C8h */
    /* 0x004165A7 jnb loc_41661F */
    if ((eax & 0xFFUL) >= 0xC8UL) goto l_41661f;
    /* 0x004165AD cmp al,64h */
    /* 0x004165AF jnz loc_4165C4 */
    if ((eax & 0xFFUL) != 0x64UL) goto l_4165c4;
    /* 0x004165B1 mov ah,1 */
    eax = (eax & ~0xFF00UL) | (1UL << 8);
    /* 0x004165B3 mov byte_444EAD,dl */
    M8(0x444EAD) = (unsigned char)(edx & 0xFFUL);
    /* 0x004165B9 mov byte_444EA7,ah */
    M8(0x444EA7) = (unsigned char)((eax >> 8) & 0xFFUL);
    /* 0x004165BF pop esi/edx/ecx/ebx */
    /* 0x004165C3 retn */
    return;
l_4165c4:
    /* 0x004165C4 cmp al,63h */
    /* 0x004165C6 jnz loc_4165D4 */
    if ((eax & 0xFFUL) != 0x63UL) goto l_4165d4;
    /* 0x004165C8 mov byte_444EA7,20h */
    M8(0x444EA7) = 0x20;
    /* 0x004165CF pop esi/edx/ecx/ebx */
    /* 0x004165D3 retn */
    return;
l_4165d4:
    /* 0x004165D4 xor ebx,ebx */
    ebx = 0;
    /* 0x004165D6 mov bl,al */
    ebx = (ebx & ~0xFFUL) | (eax & 0xFFUL);
    /* 0x004165D8 mov cl,byte_444EA7 */
    ecx = (ecx & ~0xFFUL) | ((unsigned long)M8(0x444EA7) & 0xFFUL);
    /* 0x004165DE mov edx,off_42BDDC[ebx*4] */
    edx = (unsigned long)G32((0x42BDDCUL + (ebx & 0xFFUL) * 4UL) & U32MASK);
    /* 0x004165E5 mov bl,byte_42BF6C[ebx] */
    ebx = (ebx & ~0xFFUL) | ((unsigned long)G8((0x42BF6CUL + (ebx & 0xFFUL)) & U32MASK) & 0xFFUL);
    /* 0x004165EB test cl,cl */
    /* 0x004165ED jz loc_41660A */
    if ((ecx & 0xFFUL) == 0) goto l_41660a;
    /* 0x004165EF mov al,[edx] */
    eax = (eax & ~0xFFUL) | ((unsigned long)M8(edx & U32MASK) & 0xFFUL);
    /* 0x004165F1 not al */
    eax = (eax & ~0xFFUL) | ((~(eax & 0xFFUL)) & 0xFFUL);
    /* 0x004165F3 inc edx */
    edx = (edx + 1UL) & U32MASK;
    /* 0x004165F4 sub al,20h */
    eax = (eax & ~0xFFUL) | (((eax & 0xFFUL) - 0x20UL) & 0xFFUL);
    /* 0x004165F6 and eax,0FFh */
    eax &= 0xFFUL;
    /* 0x004165FB call sub_41E3BC */
    d3u_41e3bc(c, (unsigned int)(eax & U32MASK));
    /* 0x00416600 xor ch,ch */
    ecx = ecx & ~0xFF00UL;
    /* 0x00416602 dec bl */
    ebx = (ebx & ~0xFFUL) | (((ebx & 0xFFUL) - 1UL) & 0xFFUL);
    /* 0x00416604 mov byte_444EA7,ch */
    M8(0x444EA7) = (unsigned char)((ecx >> 8) & 0xFFUL);
l_41660a:
    /* 0x0041660A mov al,[edx] */
    eax = (eax & ~0xFFUL) | ((unsigned long)M8(edx & U32MASK) & 0xFFUL);
    /* 0x0041660C not al */
    eax = (eax & ~0xFFUL) | ((~(eax & 0xFFUL)) & 0xFFUL);
    /* 0x0041660E and eax,0FFh */
    eax &= 0xFFUL;
    /* 0x00416613 inc edx */
    edx = (edx + 1UL) & U32MASK;
    /* 0x00416614 call sub_41E3BC */
    d3u_41e3bc(c, (unsigned int)(eax & U32MASK));
    /* 0x00416619 dec bl */
    ebx = (ebx & ~0xFFUL) | (((ebx & 0xFFUL) - 1UL) & 0xFFUL);
    /* 0x0041661B jz loc_41663B */
    if ((ebx & 0xFFUL) == 0) goto l_41663b;
    /* 0x0041661D jmp loc_41660A */
    goto l_41660a;
l_41661f:
    /* 0x0041661F mov bl,byte_444EA7 */
    ebx = (ebx & ~0xFFUL) | ((unsigned long)M8(0x444EA7) & 0xFFUL);
    /* 0x00416625 mov al,dl */
    eax = (eax & ~0xFFUL) | (edx & 0xFFUL);
    /* 0x00416627 sub al,bl */
    eax = (eax & ~0xFFUL) | (((eax & 0xFFUL) - (ebx & 0xFFUL)) & 0xFFUL);
    /* 0x00416629 and eax,0FFh */
    eax &= 0xFFUL;
    /* 0x0041662E xor bh,bh */
    ebx = ebx & ~0xFF00UL;
    /* 0x00416630 call sub_41E3BC */
    d3u_41e3bc(c, (unsigned int)(eax & U32MASK));
    /* 0x00416635 mov byte_444EA7,bh */
    M8(0x444EA7) = (unsigned char)((ebx >> 8) & 0xFFUL);
l_41663b:
    /* 0x0041663B pop esi/edx/ecx/ebx */
    /* 0x0041663F retn */
    return;
}
/* sub_416640: length-symbol coder. Model dword_443B68 (8-byte entries).
 * Calls sub_423188/sub_423140, escape reloads 429DB8/429DE0/429E30,
 * calls sub_41B2D4 + sub_414FD4. Entry al (cl) live but signature is
 * void(c) per unit contract: cl zero-inits (UNCERTAIN 0041664A, see
 * summary). Exit al=bl stashed in c->hi (signature void, see summary).
 */
unsigned int d3u_416640(dec3w_ctx *c, unsigned int al_in)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    unsigned long esi = 0;
    unsigned long ebp = 0;
    /* 0x00416640 push ebx/ecx/edx/esi/ebp (frame only) */
    /* 0x00416645 mov ebp,offset dword_4441E8 */
    ebp = 0x4441E8UL;
    eax = (eax & ~0xFFUL) | (al_in & 0xFFUL); /* entry al live */
    /* 0x0041664A mov cl,al */
    ecx = (ecx & ~0xFFUL) | (eax & 0xFFUL);
l_41664c:
    /* 0x0041664C xor eax,eax */
    eax = 0;
    /* 0x0041664E xor edx,edx */
    edx = 0;
    /* 0x00416650 mov al,byte_444EB6 */
    eax = (eax & ~0xFFUL) | ((unsigned long)M8(0x444EB6) & 0xFFUL);
    /* 0x00416655 mov dl,cl */
    edx = (edx & ~0xFFUL) | (ecx & 0xFFUL);
    /* 0x00416657 add eax,edx */
    eax = (eax + edx) & U32MASK;
    /* 0x00416659 shl eax,3 */
    eax = (eax << 3) & U32MASK;
    /* 0x0041665C add eax,offset dword_443B68 */
    eax = (eax + 0x443B68UL) & U32MASK;
    /* 0x00416661 lea edx,[eax+6] */
    edx = (eax + 6UL) & U32MASK;
    /* 0x00416664 xor eax,eax */
    eax = 0;
    /* 0x00416666 mov ax,[edx] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16(edx & U32MASK) & 0xFFFFUL);
    /* 0x00416669 mov dword_4441E8,eax */
    M32(0x4441E8) = (unsigned int)(eax & U32MASK);
    /* 0x0041666E xor eax,eax */
    eax = 0;
    /* 0x00416670 mov ax,word ptr dword_4441E8 */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16(0x4441E8) & 0xFFFFUL);
    /* 0x00416676 mov bl,3 */
    ebx = (ebx & ~0xFFUL) | 3UL;
    /* 0x00416678 call sub_423188 */
    eax = ((unsigned int (*)(dec3w_ctx *, unsigned int))d3u_423188)(c, (unsigned int)(eax & 0xFFFFUL));
l_41667d:
    /* 0x0041667D dec bl */
    ebx = (ebx & ~0xFFUL) | (((ebx & 0xFFUL) - 1UL) & 0xFFUL);
    /* 0x0041667F mov si,[edx-2] */
    esi = (esi & ~0xFFFFUL) | ((unsigned long)M16((edx - 2UL) & U32MASK) & 0xFFFFUL);
    /* 0x00416683 sub edx,2 */
    edx = (edx - 2UL) & U32MASK;
    /* 0x00416686 cmp ax,si */
    /* 0x00416689 jnb loc_41668F */
    if ((eax & 0xFFFFUL) >= (esi & 0xFFFFUL)) goto l_41668f;
    /* 0x0041668B test bl,bl */
    /* 0x0041668D jnz loc_41667D */
    if ((ebx & 0xFFUL) != 0) goto l_41667d;
l_41668f:
    /* 0x0041668F xor eax,eax */
    eax = 0;
    /* 0x00416691 mov ax,[edx] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16(edx & U32MASK) & 0xFFFFUL);
    /* 0x00416694 mov dword_4441EC,eax */
    M32(0x4441EC) = (unsigned int)(eax & U32MASK);
    /* 0x00416699 xor eax,eax */
    eax = 0;
    /* 0x0041669B mov ax,[edx+2] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16((edx + 2UL) & U32MASK) & 0xFFFFUL);
    /* 0x0041669F mov dword_4441F0,eax */
    M32(0x4441F0) = (unsigned int)(eax & U32MASK);
    /* 0x004166A4 mov eax,ebp */
    eax = ebp;
    /* 0x004166A6 call sub_423140 */
    d3u_423140(c, (unsigned int)(eax & U32MASK));
    /* 0x004166AB test bl,bl */
    /* 0x004166AD jnz loc_41675D */
    if ((ebx & 0xFFUL) != 0) goto l_41675d;
    /* 0x004166B3 mov edx,0Ah */
    edx = 0x0AUL;
    /* 0x004166B8 mov eax,edx */
    eax = edx;
    /* 0x004166BA mov dword_4441E8,edx */
    M32(0x4441E8) = (unsigned int)(edx & U32MASK);
    /* 0x004166C0 call sub_423188 */
    eax = ((unsigned int (*)(dec3w_ctx *, unsigned int))d3u_423188)(c, (unsigned int)(eax & 0xFFFFUL));
    /* 0x004166C5 mov edx,eax */
    edx = eax;
    /* 0x004166C7 xor eax,eax */
    eax = 0;
    /* 0x004166C9 xor ebx,ebx */
    ebx = 0;
    /* 0x004166CB mov al,dl */
    eax = (eax & ~0xFFUL) | (edx & 0xFFUL);
    /* 0x004166CD mov bl,dl */
    ebx = (ebx & ~0xFFUL) | (edx & 0xFFUL);
    /* 0x004166CF mov dword_4441EC,eax */
    M32(0x4441EC) = (unsigned int)(eax & U32MASK);
    /* 0x004166D4 lea eax,[ebx+1] */
    eax = (ebx + 1UL) & U32MASK;
    /* 0x004166D7 mov dword_4441F0,eax */
    M32(0x4441F0) = (unsigned int)(eax & U32MASK);
    /* 0x004166DC mov eax,ebp */
    eax = ebp;
    /* 0x004166DE call sub_423140 */
    d3u_423140(c, (unsigned int)(eax & U32MASK));
    /* 0x004166E3 cmp dl,byte_444EA9 */
    /* 0x004166E9 jz loc_41664C */
    if ((edx & 0xFFUL) == ((unsigned long)M8(0x444EA9) & 0xFFUL)) goto l_41664c;
    /* 0x004166EF mov byte_444EA9,dl */
    M8(0x444EA9) = (unsigned char)(edx & 0xFFUL);
    /* 0x004166F5 mov eax,dword_429DE0[ebx*4] */
    eax = (unsigned long)M32((0x429DE0UL + (ebx & 0xFFUL) * 4UL) & U32MASK);
    /* 0x004166FC mov esi,dword_429DB8[ebx*4] */
    esi = (unsigned long)M32((0x429DB8UL + (ebx & 0xFFUL) * 4UL) & U32MASK);
    /* 0x00416703 mov dword_4441C4,eax */
    M32(0x4441C4) = (unsigned int)(eax & U32MASK);
    /* 0x00416708 mov al,byte_429E30[ebx] */
    eax = (eax & ~0xFFUL) | ((unsigned long)M8((0x429E30UL + (ebx & 0xFFUL)) & U32MASK) & 0xFFUL);
    /* 0x0041670E mov dword_4441C8,esi */
    M32(0x4441C8) = (unsigned int)(esi & U32MASK);
    /* 0x00416714 mov byte_444E9E,al */
    M8(0x444E9E) = (unsigned char)(eax & 0xFFUL);
    /* 0x00416719 cmp dl,5 */
    /* 0x0041671C jz loc_416723 */
    if ((edx & 0xFFUL) == 5UL) goto l_416723;
    /* 0x0041671E cmp dl,6 */
    /* 0x00416721 jnz loc_416727 */
    if ((edx & 0xFFUL) != 6UL) goto l_416727;
l_416723:
    /* 0x00416723 mov al,1 */
    eax = (eax & ~0xFFUL) | 1UL;
    /* 0x00416725 jmp loc_416729 */
    goto l_416729;
l_416727:
    /* 0x00416727 xor al,al */
    eax = eax & ~0xFFUL;
l_416729:
    /* 0x00416729 mov byte_444EA6,al */
    M8(0x444EA6) = (unsigned char)(eax & 0xFFUL);
    /* 0x0041672E xor dl,dl */
    edx = edx & ~0xFFUL;
    /* 0x00416730 xor ebx,ebx */
    ebx = 0;
    /* 0x00416732 mov byte_444EA8,dl */
    M8(0x444EA8) = (unsigned char)(edx & 0xFFUL);
    /* 0x00416738 mov dword_4441D0,ebx */
    M32(0x4441D0) = (unsigned int)(ebx & U32MASK);
    /* 0x0041673E mov bl,byte_444EA9 */
    ebx = (ebx & ~0xFFUL) | ((unsigned long)M8(0x444EA9) & 0xFFUL);
    /* 0x00416744 mov byte_444E9D,dl */
    M8(0x444E9D) = (unsigned char)(edx & 0xFFUL);
    /* 0x0041674A cmp bl,4 */
    /* 0x0041674D jbe loc_41664C */
    if ((ebx & 0xFFUL) <= 4UL) goto l_41664c;
    /* 0x00416753 call sub_41B2D4 */
    d3u_41b2d4(c);
    /* 0x00416758 jmp loc_41664C */
    goto l_41664c;
l_41675d:
    /* 0x0041675D xor edx,edx */
    edx = 0;
    /* 0x0041675F xor eax,eax */
    eax = 0;
    /* 0x00416761 mov dl,bl */
    edx = (edx & ~0xFFUL) | (ebx & 0xFFUL);
    /* 0x00416763 mov al,cl */
    eax = (eax & ~0xFFUL) | (ecx & 0xFFUL);
    /* 0x00416765 call sub_414FD4 */
    d3u_414fd4(c, (unsigned int)(eax & U32MASK), (unsigned int)(edx & U32MASK));
    /* 0x0041676A mov al,bl */
    eax = (eax & ~0xFFUL) | (ebx & 0xFFUL);
    /* 0x0041676C pop ebp/esi/edx/ecx/ebx */
    /* 0x00416771 retn */
    return (eax & 0xFFUL);
}
/* sub_4177C0: offset/selector coder. Tables dword_4299F0/4299F4/4299FC,
 * const word_42D274. Calls sub_423188/sub_423140. Entry eax live, taken
 * as x (UNCERTAIN frame sizes: var_C4 22 dwords / var_68 words modelled
 * as 256-entry arrays, zero-init). Exit al (cl or bh) stashed in c->hi
 * because the unit signature is void (see summary).
 */
unsigned int d3u_4177c0(dec3w_ctx *c, unsigned int x)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    unsigned long esi = 0;
    unsigned long edi = 0;
    unsigned long ebp = 0;
    unsigned long v34 = 0;
    unsigned long v20 = 0;
    unsigned long v24 = 0;
    unsigned long v38 = 0;
    unsigned long v30 = 0;
    unsigned long v28 = 0;
    unsigned long v2c = 0;
    unsigned long v1c = 0;
    unsigned long c4[256] = {0};
    unsigned long cw[256] = {0};
    unsigned long ta = 0;
    unsigned long ti = 0;
    /* 0x004177C0 push ebx/ecx/edx/esi/edi/ebp; sub esp,0ACh (frame only) */
    /* 0x004177CC mov bl,byte_444EAA */
    ebx = (ebx & ~0xFFUL) | ((unsigned long)M8(0x444EAA) & 0xFFUL);
    /* 0x004177D2 mov [var_34],eax */
    v34 = (unsigned long)x & U32MASK;
    /* 0x004177D9 mov esi,eax */
    esi = (unsigned long)x & U32MASK;
    /* 0x004177DB mov eax,dword_4299F0 */
    eax = (unsigned long)G32(0x4299F0);
    /* 0x004177E0 shl esi,9 */
    esi = (esi << 9) & U32MASK;
    /* 0x004177E3 add esi,eax */
    esi = (esi + eax) & U32MASK;
    /* 0x004177E5 mov ebp,[var_34] */
    ebp = v34;
    /* 0x004177EC xor cl,cl */
    ecx = ecx & ~0xFFUL;
    /* 0x004177EE mov ax,[esi] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16(esi & U32MASK) & 0xFFFFUL);
    /* 0x004177F1 shl ebp,8 */
    ebp = (ebp << 8) & U32MASK;
    /* 0x004177F4 mov [var_20],eax */
    v20 = eax;
    /* 0x004177FB xor eax,eax */
    eax = 0;
l_4177fd:
    /* 0x004177FD xor edx,edx */
    edx = 0;
    /* 0x004177FF mov dl,bl */
    edx = (edx & ~0xFFUL) | (ebx & 0xFFUL);
    /* 0x00417801 cmp eax,edx */
    /* 0x00417803 jnb loc_417833 */
    if (eax >= edx) goto l_417833;
    /* 0x00417805 xor edx,edx */
    edx = 0;
    /* 0x00417807 mov dl,byte_444E82[eax] */
    edx = (edx & ~0xFFUL) | ((unsigned long)M8((0x444E82UL + (eax & U32MASK)) & U32MASK) & 0xFFUL);
    /* 0x0041780D add edx,ebp */
    edx = (edx + ebp) & U32MASK;
    /* 0x0041780F lea edi,ds:0[edx*2] */
    edi = (edx * 2UL) & U32MASK;
    /* 0x00417816 mov edx,dword_4299F4 */
    edx = (unsigned long)G32(0x4299F4);
    /* 0x0041781C mov dx,[edx+edi] */
    edx = (edx & ~0xFFFFUL) | ((unsigned long)M16((edx + edi) & U32MASK) & 0xFFFFUL);
    /* 0x00417820 mov edi,[var_20] */
    edi = v20;
    /* 0x00417827 sub edi,edx */
    edi = (edi - edx) & U32MASK;
    /* 0x00417829 inc eax */
    eax = (eax + 1UL) & U32MASK;
    /* 0x0041782A mov [var_20],edi */
    v20 = edi;
    /* 0x00417831 jmp loc_4177FD */
    goto l_4177fd;
l_417833:
    /* 0x00417833 mov edx,[var_20] */
    edx = v20;
    /* 0x0041783A mov byte_444EAA,bl */
    M8(0x444EAA) = (unsigned char)(ebx & 0xFFUL);
    /* 0x00417840 test dx,dx */
    /* 0x00417843 jnz loc_417948 */
    if ((edx & 0xFFFFUL) != 0) goto l_417948;
    /* 0x00417849 mov eax,dword_44421C */
    eax = (unsigned long)M32(0x44421C);
    /* 0x0041784E mov [var_34],eax */
    v34 = eax;
    /* 0x00417855 mov esi,eax */
    esi = eax;
    /* 0x00417857 mov eax,dword_4299F0 */
    eax = (unsigned long)G32(0x4299F0);
    /* 0x0041785C shl esi,9 */
    esi = (esi << 9) & U32MASK;
    /* 0x0041785F add esi,eax */
    esi = (esi + eax) & U32MASK;
    /* 0x00417861 mov ebp,[var_34] */
    ebp = v34;
    /* 0x00417868 mov ax,[esi] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16(esi & U32MASK) & 0xFFFFUL);
    /* 0x0041786B shl ebp,8 */
    ebp = (ebp << 8) & U32MASK;
    /* 0x0041786E mov [var_20],eax */
    v20 = eax;
    /* 0x00417875 xor eax,eax */
    eax = 0;
l_417877:
    /* 0x00417877 xor edx,edx */
    edx = 0;
    /* 0x00417879 mov dl,bl */
    edx = (edx & ~0xFFUL) | (ebx & 0xFFUL);
    /* 0x0041787B mov byte_444EAA,bl */
    M8(0x444EAA) = (unsigned char)(ebx & 0xFFUL);
    /* 0x00417881 cmp eax,edx */
    /* 0x00417883 jnb loc_4178B3 */
    if (eax >= edx) goto l_4178b3;
    /* 0x00417885 xor edx,edx */
    edx = 0;
    /* 0x00417887 mov dl,byte_444E82[eax] */
    edx = (edx & ~0xFFUL) | ((unsigned long)M8((0x444E82UL + (eax & U32MASK)) & U32MASK) & 0xFFUL);
    /* 0x0041788D add edx,ebp */
    edx = (edx + ebp) & U32MASK;
    /* 0x0041788F lea ecx,ds:0[edx*2] */
    ecx = (edx * 2UL) & U32MASK;
    /* 0x00417896 mov edx,dword_4299F4 */
    edx = (unsigned long)G32(0x4299F4);
    /* 0x0041789C mov dx,[ecx+edx] */
    edx = (edx & ~0xFFFFUL) | ((unsigned long)M16((ecx + edx) & U32MASK) & 0xFFFFUL);
    /* 0x004178A0 mov ecx,[var_20] */
    ecx = v20;
    /* 0x004178A7 sub ecx,edx */
    ecx = (ecx - edx) & U32MASK;
    /* 0x004178A9 inc eax */
    eax = (eax + 1UL) & U32MASK;
    /* 0x004178AA mov [var_20],ecx */
    v20 = ecx;
    /* 0x004178B1 jmp loc_417877 */
    goto l_417877;
l_4178b3:
    /* 0x004178B3 xor eax,eax */
    eax = 0;
    /* 0x004178B5 mov ax,word ptr [var_20] */
    eax = (eax & ~0xFFFFUL) | (v20 & 0xFFFFUL);
    /* 0x004178BD mov dword_4441E8,eax */
    M32(0x4441E8) = (unsigned int)(eax & U32MASK);
    /* 0x004178C2 xor eax,eax */
    eax = 0;
    /* 0x004178C4 mov ax,word ptr dword_4441E8 */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16(0x4441E8) & 0xFFFFUL);
    /* 0x004178CA call sub_423188 */
    eax = ((unsigned int (*)(dec3w_ctx *, unsigned int))d3u_423188)(c, (unsigned int)(eax & 0xFFFFUL));
    /* 0x004178CF mov [var_24],eax */
    v24 = eax;
l_4178d6:
    /* 0x004178D6 mov bl,byte_444EAA */
    ebx = (ebx & ~0xFFUL) | ((unsigned long)M8(0x444EAA) & 0xFFUL);
    /* 0x004178DC test bl,bl */
    /* 0x004178DE jz loc_417C80 */
    if ((ebx & 0xFFUL) == 0) goto l_417c80;
    /* 0x004178E4 mov bh,80h */
    ebx = (ebx & ~0xFF00UL) | (0x80UL << 8);
    /* 0x004178E6 mov edx,[var_34] */
    edx = v34;
    /* 0x004178ED mov [var_38],esi */
    v38 = esi;
    /* 0x004178F4 xor eax,eax */
    eax = 0;
    /* 0x004178F6 shl edx,8 */
    edx = (edx << 8) & U32MASK;
    /* 0x004178F9 xor ecx,ecx */
    ecx = 0;
    /* 0x004178FB mov [var_30],edx */
    v30 = edx;
    /* 0x00417902 xor edx,edx */
    edx = 0;
l_417904:
    /* 0x00417904 movzx edi,bl */
    edi = ebx & 0xFFUL;
    /* 0x00417907 cmp eax,edi */
    /* 0x00417909 jnb loc_417BB6 */
    if (eax >= edi) goto l_417bb6;
    /* 0x0041790F movzx edi,byte_444E82[eax] */
    edi = (unsigned long)M8((0x444E82UL + (eax & U32MASK)) & U32MASK) & 0xFFUL;
    /* 0x00417916 add edi,edi */
    edi = (edi + edi) & U32MASK;
    /* 0x00417918 add edi,esi */
    edi = (edi + esi) & U32MASK;
    /* 0x0041791A mov ebp,[var_30] */
    ebp = v30;
    /* 0x00417921 mov [esp+edx+var_C4],edi */
    ta = (edx >> 2) & U32MASK;
    ti = 0;
    c4[ta & 0xFFUL] = edi;
    /* 0x00417924 movzx edi,byte_444E82[eax] */
    edi = (unsigned long)M8((0x444E82UL + (eax & U32MASK)) & U32MASK) & 0xFFUL;
    /* 0x0041792B add edi,ebp */
    edi = (edi + ebp) & U32MASK;
    /* 0x0041792D mov ebp,dword_4299F4 */
    ebp = (unsigned long)G32(0x4299F4);
    /* 0x00417933 add edi,edi */
    edi = (edi + edi) & U32MASK;
    /* 0x00417935 add ecx,2 */
    ecx = (ecx + 2UL) & U32MASK;
    /* 0x00417938 add edi,ebp */
    edi = (edi + ebp) & U32MASK;
    /* 0x0041793A add edx,4 */
    edx = (edx + 4UL) & U32MASK;
    /* 0x0041793D inc eax */
    eax = (eax + 1UL) & U32MASK;
    /* 0x0041793E mov di,[edi] */
    edi = (edi & ~0xFFFFUL) | ((unsigned long)M16(edi & U32MASK) & 0xFFFFUL);
    /* 0x00417941 mov [esp+ecx+var_6A],di */
    ti = (ecx >> 1) & U32MASK;
    cw[(ti - 1UL) & 0xFFUL] = edi & 0xFFFFUL;
    /* 0x00417946 jmp loc_417904 */
    goto l_417904;
l_417948:
    /* 0x00417948 mov eax,[var_34] */
    eax = v34;
    /* 0x0041794F mov edx,dword_4299FC */
    edx = (unsigned long)G32(0x4299FC);
    /* 0x00417955 mov ax,[edx+eax*2] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16((edx + (eax & U32MASK) * 2UL) & U32MASK) & 0xFFFFUL);
    /* 0x00417959 xor edx,edx */
    edx = 0;
    /* 0x0041795B and eax,0FFFFh */
    eax &= 0xFFFFUL;
    /* 0x00417960 mov dx,word ptr [var_20] */
    edx = (edx & ~0xFFFFUL) | (v20 & 0xFFFFUL);
    /* 0x00417968 add edx,eax */
    edx = (edx + eax) & U32MASK;
    /* 0x0041796A mov dword_4441E8,edx */
    M32(0x4441E8) = (unsigned int)(edx & U32MASK);
    /* 0x00417970 xor eax,eax */
    eax = 0;
    /* 0x00417972 mov ax,word ptr dword_4441E8 */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16(0x4441E8) & 0xFFFFUL);
    /* 0x00417978 mov edi,[var_20] */
    edi = v20;
    /* 0x0041797F call sub_423188 */
    eax = ((unsigned int (*)(dec3w_ctx *, unsigned int))d3u_423188)(c, (unsigned int)(eax & 0xFFFFUL));
    /* 0x00417984 mov [var_24],eax */
    v24 = eax;
    /* 0x0041798B cmp ax,di */
    /* 0x0041798E jb loc_4178D6 */
    if ((eax & 0xFFFFUL) < (edi & 0xFFFFUL)) goto l_4178d6;
    /* 0x00417994 xor eax,eax */
    eax = 0;
    /* 0x00417996 mov ax,di */
    eax = (eax & ~0xFFFFUL) | (edi & 0xFFFFUL);
    /* 0x00417999 mov dword_4441EC,eax */
    M32(0x4441EC) = (unsigned int)(eax & U32MASK);
    /* 0x0041799E mov eax,dword_4441E8 */
    eax = (unsigned long)M32(0x4441E8);
    /* 0x004179A3 mov edx,dword_4441EC */
    edx = (unsigned long)M32(0x4441EC);
    /* 0x004179A9 mov dword_4441F0,eax */
    M32(0x4441F0) = (unsigned int)(eax & U32MASK);
    /* 0x004179AE sub eax,edx */
    eax = (eax - edx) & U32MASK;
    /* 0x004179B0 mov esi,dword_4441E8 */
    esi = (unsigned long)M32(0x4441E8);
    /* 0x004179B6 shl eax,0Eh */
    eax = (eax << 14) & U32MASK;
    /* 0x004179B9 xor edx,edx */
    edx = 0;
    /* 0x004179BB div esi */
    eax = (esi == 0) ? 0 : (eax / esi) & U32MASK;
    edx = 0;
    /* 0x004179BD mov ax,word_42D274[eax*2] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)G16((0x42D274UL + (eax & U32MASK) * 2UL) & U32MASK) & 0xFFFFUL);
    /* 0x004179C5 mov edi,dword_4441F4 */
    edi = (unsigned long)M32(0x4441F4);
    /* 0x004179CB and eax,0FFFFh */
    eax &= 0xFFFFUL;
    /* 0x004179D0 add edi,eax */
    edi = (edi + eax) & U32MASK;
    /* 0x004179D2 mov eax,offset dword_4441E8 */
    eax = 0x4441E8UL;
    /* 0x004179D7 mov dword_4441F4,edi */
    M32(0x4441F4) = (unsigned int)(edi & U32MASK);
    /* 0x004179DD call sub_423140 */
    d3u_423140(c, (unsigned int)(eax & U32MASK));
    /* 0x004179E2 mov eax,[var_34] */
    eax = v34;
    /* 0x004179E9 mov edx,dword_4299F4 */
    edx = (unsigned long)G32(0x4299F4);
    /* 0x004179EF shl eax,9 */
    eax = (eax << 9) & U32MASK;
    /* 0x004179F2 add edx,eax */
    edx = (edx + eax) & U32MASK;
    /* 0x004179F4 mov eax,dword_44421C */
    eax = (unsigned long)M32(0x44421C);
    /* 0x004179F9 mov [var_28],edx */
    v28 = edx;
    /* 0x00417A00 mov edx,dword_4299F0 */
    edx = (unsigned long)G32(0x4299F0);
    /* 0x00417A06 shl eax,9 */
    eax = (eax << 9) & U32MASK;
    /* 0x00417A09 mov ax,[edx+eax] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16((edx + eax) & U32MASK) & 0xFFFFUL);
    /* 0x00417A0D mov esi,dword_44421C */
    esi = (unsigned long)M32(0x44421C);
    /* 0x00417A13 and eax,0FFFFh */
    eax &= 0xFFFFUL;
    /* 0x00417A18 xor bh,bh */
    ebx = ebx & ~0xFF00UL;
    /* 0x00417A1A mov dword_4441E8,eax */
    M32(0x4441E8) = (unsigned int)(eax & U32MASK);
    /* 0x00417A1F shl esi,8 */
    esi = (esi << 8) & U32MASK;
l_417a22:
    /* 0x00417A22 mov bl,byte_444EAA */
    ebx = (ebx & ~0xFFUL) | ((unsigned long)M8(0x444EAA) & 0xFFUL);
    /* 0x00417A28 xor eax,eax */
    eax = 0;
    /* 0x00417A2A mov al,bh */
    eax = (eax & ~0xFFUL) | ((ebx >> 8) & 0xFFUL);
    /* 0x00417A2C mov edx,[var_28] */
    edx = v28;
    /* 0x00417A33 add eax,eax */
    eax = (eax + eax) & U32MASK;
    /* 0x00417A35 add eax,edx */
    eax = (eax + edx) & U32MASK;
    /* 0x00417A37 cmp word ptr [eax],0 */
    /* 0x00417A3B setnz dl */
    edx = (edx & ~0xFFUL) | ((M16(eax & U32MASK) != 0) ? 1UL : 0UL);
    /* 0x00417A3E xor eax,eax */
    eax = 0;
    /* 0x00417A40 mov [var_1C],dl */
    v1c = edx & 0xFFUL;
    /* 0x00417A47 test dl,dl */
    /* 0x00417A49 jnz loc_417A6C */
    if ((edx & 0xFFUL) != 0) goto l_417a6c;
l_417a4b:
    /* 0x00417A4B xor edx,edx */
    edx = 0;
    /* 0x00417A4D mov dl,bl */
    edx = (edx & ~0xFFUL) | (ebx & 0xFFUL);
    /* 0x00417A4F cmp eax,edx */
    /* 0x00417A51 jnb loc_417A6C */
    if (eax >= edx) goto l_417a6c;
    /* 0x00417A53 mov edx,eax */
    edx = eax;
    /* 0x00417A55 mov ch,byte_444E82[edx] */
    ecx = (ecx & ~0xFF00UL) | ((((unsigned long)M8((0x444E82UL + (edx & U32MASK)) & U32MASK) & 0xFFUL)) << 8);
    /* 0x00417A5B inc eax */
    eax = (eax + 1UL) & U32MASK;
    /* 0x00417A5C cmp bh,ch */
    /* 0x00417A5E setz dl */
    edx = (edx & ~0xFFUL) | ((((ebx >> 8) & 0xFFUL) == ((ecx >> 8) & 0xFFUL)) ? 1UL : 0UL);
    /* 0x00417A61 mov [var_1C],dl */
    v1c = edx & 0xFFUL;
    /* 0x00417A68 test dl,dl */
    /* 0x00417A6A jz loc_417A4B */
    if ((edx & 0xFFUL) == 0) goto l_417a4b;
l_417a6c:
    /* 0x00417A6C cmp [var_1C],0 */
    /* 0x00417A74 jz loc_417A99 */
    if ((v1c & 0xFFUL) == 0) goto l_417a99;
    /* 0x00417A76 xor eax,eax */
    eax = 0;
    /* 0x00417A78 mov al,bh */
    eax = (eax & ~0xFFUL) | ((ebx >> 8) & 0xFFUL);
    /* 0x00417A7A mov edx,dword_4299F4 */
    edx = (unsigned long)G32(0x4299F4);
    /* 0x00417A80 add eax,esi */
    eax = (eax + esi) & U32MASK;
    /* 0x00417A82 mov ax,[edx+eax*2] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16((edx + (eax & U32MASK) * 2UL) & U32MASK) & 0xFFFFUL);
    /* 0x00417A86 mov edi,dword_4441E8 */
    edi = (unsigned long)M32(0x4441E8);
    /* 0x00417A8C and eax,0FFFFh */
    eax &= 0xFFFFUL;
    /* 0x00417A91 sub edi,eax */
    edi = (edi - eax) & U32MASK;
    /* 0x00417A93 mov dword_4441E8,edi */
    M32(0x4441E8) = (unsigned int)(edi & U32MASK);
l_417a99:
    /* 0x00417A99 inc bh */
    ebx = (ebx & ~0xFF00UL) | ((((ebx >> 8) & 0xFFUL) + 1UL & 0xFFUL) << 8);
    /* 0x00417A9B mov byte_444EAA,bl */
    M8(0x444EAA) = (unsigned char)(ebx & 0xFFUL);
    /* 0x00417AA1 jnz loc_417A22 */
    if (((ebx >> 8) & 0xFFUL) != 0) goto l_417a22;
    /* 0x00417AA7 xor eax,eax */
    eax = 0;
    /* 0x00417AA9 mov ax,word ptr dword_4441E8 */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16(0x4441E8) & 0xFFFFUL);
    /* 0x00417AAF call sub_423188 */
    eax = ((unsigned int (*)(dec3w_ctx *, unsigned int))d3u_423188)(c, (unsigned int)(eax & 0xFFFFUL));
    /* 0x00417AB4 mov edi,eax */
    edi = eax;
    /* 0x00417AB6 mov eax,dword_44421C */
    eax = (unsigned long)M32(0x44421C);
    /* 0x00417ABB mov ebp,[var_28] */
    ebp = v28;
    /* 0x00417AC2 shl eax,8 */
    eax = (eax << 8) & U32MASK;
    /* 0x00417AC5 xor esi,esi */
    esi = 0;
    /* 0x00417AC7 mov [var_2C],eax */
    v2c = eax;
l_417ace:
    /* 0x00417ACE dec cl */
    ecx = (ecx & ~0xFFUL) | (((ecx & 0xFFUL) - 1UL) & 0xFFUL);
    /* 0x00417AD0 xor eax,eax */
    eax = 0;
    /* 0x00417AD2 mov al,cl */
    eax = (eax & ~0xFFUL) | (ecx & 0xFFUL);
    /* 0x00417AD4 add eax,eax */
    eax = (eax + eax) & U32MASK;
    /* 0x00417AD6 add eax,ebp */
    eax = (eax + ebp) & U32MASK;
    /* 0x00417AD8 mov bl,byte_444EAA */
    ebx = (ebx & ~0xFFUL) | ((unsigned long)M8(0x444EAA) & 0xFFUL);
    /* 0x00417ADE cmp word ptr [eax],0 */
    /* 0x00417AE2 setz dl */
    edx = (edx & ~0xFFUL) | ((M16(eax & U32MASK) == 0) ? 1UL : 0UL);
    /* 0x00417AE5 mov bh,dl */
    ebx = (ebx & ~0xFF00UL) | ((edx & 0xFFUL) << 8);
    /* 0x00417AE7 xor eax,eax */
    eax = 0;
    /* 0x00417AE9 test dl,dl */
    /* 0x00417AEB jz loc_417B09 */
    if ((edx & 0xFFUL) == 0) goto l_417b09;
l_417aed:
    /* 0x00417AED xor edx,edx */
    edx = 0;
    /* 0x00417AEF mov dl,bl */
    edx = (edx & ~0xFFUL) | (ebx & 0xFFUL);
    /* 0x00417AF1 cmp eax,edx */
    /* 0x00417AF3 jnb loc_417B09 */
    if (eax >= edx) goto l_417b09;
    /* 0x00417AF5 mov edx,eax */
    edx = eax;
    /* 0x00417AF7 mov bh,byte_444E82[edx] */
    ebx = (ebx & ~0xFF00UL) | ((((unsigned long)M8((0x444E82UL + (edx & U32MASK)) & U32MASK) & 0xFFUL)) << 8);
    /* 0x00417AFD inc eax */
    eax = (eax + 1UL) & U32MASK;
    /* 0x00417AFE cmp cl,bh */
    /* 0x00417B00 setnz dl */
    edx = (edx & ~0xFFUL) | (((ecx & 0xFFUL) != ((ebx >> 8) & 0xFFUL)) ? 1UL : 0UL);
    /* 0x00417B03 mov bh,dl */
    ebx = (ebx & ~0xFF00UL) | ((edx & 0xFFUL) << 8);
    /* 0x00417B05 test dl,dl */
    /* 0x00417B07 jnz loc_417AED */
    if ((edx & 0xFFUL) != 0) goto l_417aed;
l_417b09:
    /* 0x00417B09 test bh,bh */
    /* 0x00417B0B jz loc_417B24 */
    if (((ebx >> 8) & 0xFFUL) == 0) goto l_417b24;
    /* 0x00417B0D xor eax,eax */
    eax = 0;
    /* 0x00417B0F mov edx,[var_2C] */
    edx = v2c;
    /* 0x00417B16 mov al,cl */
    eax = (eax & ~0xFFUL) | (ecx & 0xFFUL);
    /* 0x00417B18 add eax,edx */
    eax = (eax + edx) & U32MASK;
    /* 0x00417B1A mov edx,dword_4299F4 */
    edx = (unsigned long)G32(0x4299F4);
    /* 0x00417B20 add si,[edx+eax*2] */
    esi = (esi + ((unsigned long)M16((edx + (eax & U32MASK) * 2UL) & U32MASK) & 0xFFFFUL)) & U32MASK;
l_417b24:
    /* 0x00417B24 mov byte_444EAA,bl */
    M8(0x444EAA) = (unsigned char)(ebx & 0xFFUL);
    /* 0x00417B2A cmp si,di */
    /* 0x00417B2D jbe loc_417ACE */
    if ((esi & 0xFFFFUL) <= (edi & 0xFFFFUL)) goto l_417ace;
    /* 0x00417B2F xor eax,eax */
    eax = 0;
    /* 0x00417B31 mov ax,si */
    eax = (eax & ~0xFFFFUL) | (esi & 0xFFFFUL);
    /* 0x00417B34 xor edx,edx */
    edx = 0;
    /* 0x00417B36 mov dword_4441F0,eax */
    M32(0x4441F0) = (unsigned int)(eax & U32MASK);
    /* 0x00417B3B mov eax,dword_44421C */
    eax = (unsigned long)M32(0x44421C);
    /* 0x00417B40 mov dl,cl */
    edx = (edx & ~0xFFUL) | (ecx & 0xFFUL);
    /* 0x00417B42 shl eax,8 */
    eax = (eax << 8) & U32MASK;
    /* 0x00417B45 add eax,edx */
    eax = (eax + edx) & U32MASK;
    /* 0x00417B47 mov edx,dword_4299F4 */
    edx = (unsigned long)G32(0x4299F4);
    /* 0x00417B4D add eax,eax */
    eax = (eax + eax) & U32MASK;
    /* 0x00417B4F add eax,edx */
    eax = (eax + edx) & U32MASK;
    /* 0x00417B51 xor edx,edx */
    edx = 0;
    /* 0x00417B53 mov dx,[eax] */
    edx = (edx & ~0xFFFFUL) | ((unsigned long)M16(eax & U32MASK) & 0xFFFFUL);
    /* 0x00417B56 xor eax,eax */
    eax = 0;
    /* 0x00417B58 mov ax,si */
    eax = (eax & ~0xFFFFUL) | (esi & 0xFFFFUL);
    /* 0x00417B5B sub eax,edx */
    eax = (eax - edx) & U32MASK;
    /* 0x00417B5D mov dword_4441EC,eax */
    M32(0x4441EC) = (unsigned int)(eax & U32MASK);
    /* 0x00417B62 mov eax,dword_4441F0 */
    eax = (unsigned long)M32(0x4441F0);
    /* 0x00417B67 mov esi,dword_4441EC */
    esi = (unsigned long)M32(0x4441EC);
    /* 0x00417B6D mov edi,dword_4441E8 */
    edi = (unsigned long)M32(0x4441E8);
    /* 0x00417B73 sub eax,esi */
    eax = (eax - esi) & U32MASK;
    /* 0x00417B75 xor edx,edx */
    edx = 0;
    /* 0x00417B77 shl eax,0Eh */
    eax = (eax << 14) & U32MASK;
    /* 0x00417B7A div edi */
    eax = (edi == 0) ? 0 : (eax / edi) & U32MASK;
    edx = 0;
    /* 0x00417B7C mov ax,word_42D274[eax*2] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)G16((0x42D274UL + (eax & U32MASK) * 2UL) & U32MASK) & 0xFFFFUL);
    /* 0x00417B84 mov ebp,dword_4441F4 */
    ebp = (unsigned long)M32(0x4441F4);
    /* 0x00417B8A and eax,0FFFFh */
    eax &= 0xFFFFUL;
    /* 0x00417B8F add ebp,eax */
    ebp = (ebp + eax) & U32MASK;
    /* 0x00417B91 mov eax,offset dword_4441E8 */
    eax = 0x4441E8UL;
    /* 0x00417B96 mov dword_4441F4,ebp */
    M32(0x4441F4) = (unsigned int)(ebp & U32MASK);
    /* 0x00417B9C call sub_423140 */
    d3u_423140(c, (unsigned int)(eax & U32MASK));
    /* 0x00417BA1 mov al,cl */
    eax = (eax & ~0xFFUL) | (ecx & 0xFFUL);
    /* 0x00417BA3 mov bl,byte_444EAA */
    ebx = (ebx & ~0xFFUL) | ((unsigned long)M8(0x444EAA) & 0xFFUL);
    /* 0x00417BA9 add esp,0ACh */
    /* 0x00417BAF pop ebp/edi/esi/edx/ecx/ebx */
    /* 0x00417BB5 retn */
    return (eax & 0xFFUL);
l_417bb6:
    /* 0x00417BB6 mov eax,[var_24] */
    eax = v24;
    /* 0x00417BBD sub [var_20],eax */
    v20 = (v20 - eax) & U32MASK;
l_417bc4:
    /* 0x00417BC4 xor edi,edi */
    edi = 0;
    /* 0x00417BC6 xor eax,eax */
    eax = 0;
    /* 0x00417BC8 xor ecx,ecx */
    ecx = 0;
    /* 0x00417BCA xor edx,edx */
    edx = 0;
l_417bcc:
    /* 0x00417BCC movzx ebp,bl */
    ebp = ebx & 0xFFUL;
    /* 0x00417BCF cmp eax,ebp */
    /* 0x00417BD1 jnb loc_417BEF */
    if (eax >= ebp) goto l_417bef;
    /* 0x00417BD3 movzx ebp,bh */
    ebp = (ebx >> 8) & 0xFFUL;
    /* 0x00417BD6 add ebp,ebp */
    ebp = (ebp + ebp) & U32MASK;
    /* 0x00417BD8 add ebp,esi */
    ebp = (ebp + esi) & U32MASK;
    /* 0x00417BDA cmp ebp,[esp+ecx+var_C4] */
    ta = (ecx >> 2) & U32MASK;
    /* 0x00417BDD jbe loc_417BE6 */
    if (ebp <= c4[ta & 0xFFUL]) goto l_417be6;
    /* 0x00417BDF movzx ebp,[esp+edx+var_68] */
    ti = (edx >> 1) & U32MASK;
    ebp = cw[ti & 0xFFUL] & 0xFFFFUL;
    /* 0x00417BE4 add edi,ebp */
    edi = (edi + ebp) & U32MASK;
l_417be6:
    /* 0x00417BE6 add ecx,4 */
    ecx = (ecx + 4UL) & U32MASK;
    /* 0x00417BE9 add edx,2 */
    edx = (edx + 2UL) & U32MASK;
    /* 0x00417BEC inc eax */
    eax = (eax + 1UL) & U32MASK;
    /* 0x00417BED jmp loc_417BCC */
    goto l_417bcc;
l_417bef:
    /* 0x00417BEF xor eax,eax */
    eax = 0;
    /* 0x00417BF1 mov al,bh */
    eax = (eax & ~0xFFUL) | ((ebx >> 8) & 0xFFUL);
    /* 0x00417BF3 xor edx,edx */
    edx = 0;
    /* 0x00417BF5 mov dx,[esi+eax*2] */
    edx = (edx & ~0xFFFFUL) | ((unsigned long)M16((esi + (eax & 0xFFUL) * 2UL) & U32MASK) & 0xFFFFUL);
    /* 0x00417BF9 xor eax,eax */
    eax = 0;
    /* 0x00417BFB mov ax,word ptr [var_20] */
    eax = (eax & ~0xFFFFUL) | (v20 & 0xFFFFUL);
    /* 0x00417C03 add eax,edi */
    eax = (eax + edi) & U32MASK;
    /* 0x00417C05 cmp edx,eax */
    /* 0x00417C07 jnb loc_417C4E */
    if ((edx & 0xFFFFUL) >= (eax & 0xFFFFUL)) goto l_417c4e;
    /* 0x00417C09 xor edx,edx */
    edx = 0;
    /* 0x00417C0B xor eax,eax */
    eax = 0;
l_417c0d:
    /* 0x00417C0D xor ecx,ecx */
    ecx = 0;
    /* 0x00417C0F mov cl,bl */
    ecx = (ecx & ~0xFFUL) | (ebx & 0xFFUL);
    /* 0x00417C11 cmp edx,ecx */
    /* 0x00417C13 jnb loc_417C31 */
    if (edx >= ecx) goto l_417c31;
    /* 0x00417C15 xor ecx,ecx */
    ecx = 0;
    /* 0x00417C17 mov cl,bh */
    ecx = (ecx & ~0xFFUL) | ((ebx >> 8) & 0xFFUL);
    /* 0x00417C19 add ecx,ecx */
    ecx = (ecx + ecx) & U32MASK;
    /* 0x00417C1B mov ebp,[esp+eax+var_C4] */
    ta = (eax >> 2) & U32MASK;
    ebp = c4[ta & 0xFFUL];
    /* 0x00417C1E add ecx,esi */
    ecx = (ecx + esi) & U32MASK;
    /* 0x00417C20 cmp ecx,ebp */
    /* 0x00417C22 jbe loc_417C2B */
    if (ecx <= ebp) goto l_417c2b;
    /* 0x00417C24 mov [esp+eax+var_C4],0FFFFFFFFh */
    c4[ta & 0xFFUL] = 0xFFFFFFFFUL;
l_417c2b:
    /* 0x00417C2B add eax,4 */
    eax = (eax + 4UL) & U32MASK;
    /* 0x00417C2E inc edx */
    edx = (edx + 1UL) & U32MASK;
    /* 0x00417C2F jmp loc_417C0D */
    goto l_417c0d;
l_417c31:
    /* 0x00417C31 xor eax,eax */
    eax = 0;
    /* 0x00417C33 mov al,bh */
    eax = (eax & ~0xFFUL) | ((ebx >> 8) & 0xFFUL);
    /* 0x00417C35 add eax,eax */
    eax = (eax + eax) & U32MASK;
    /* 0x00417C37 add esi,eax */
    esi = (esi + eax) & U32MASK;
    /* 0x00417C39 mov ax,[esi] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16(esi & U32MASK) & 0xFFFFUL);
    /* 0x00417C3C mov edx,[var_20] */
    edx = v20;
    /* 0x00417C43 sub eax,edi */
    eax = (eax - edi) & U32MASK;
    /* 0x00417C45 sub edx,eax */
    edx = (edx - eax) & U32MASK;
    /* 0x00417C47 mov [var_20],edx */
    v20 = edx;
l_417c4e:
    /* 0x00417C4E shr bh,1 */
    ebx = (ebx & ~0xFF00UL) | ((((ebx >> 8) & 0xFFUL) >> 1) << 8);
    /* 0x00417C50 test bh,bh */
    /* 0x00417C52 jnz loc_417BC4 */
    if (((ebx >> 8) & 0xFFUL) != 0) goto l_417bc4;
    /* 0x00417C58 mov eax,[var_24] */
    eax = v24;
    /* 0x00417C5F mov ecx,[var_20] */
    ecx = v20;
    /* 0x00417C66 mov edi,[var_38] */
    edi = v38;
    /* 0x00417C6D add ecx,eax */
    ecx = (ecx + eax) & U32MASK;
    /* 0x00417C6F mov eax,esi */
    eax = esi;
    /* 0x00417C71 sub eax,edi */
    eax = (eax - edi) & U32MASK;
    /* 0x00417C73 sar eax,1 */
    eax = ((unsigned long)(((int)(eax & U32MASK)) >> 1)) & U32MASK;
    /* 0x00417C75 mov bh,al */
    ebx = (ebx & ~0xFF00UL) | ((eax & 0xFFUL) << 8);
    /* 0x00417C77 mov [var_20],ecx */
    v20 = ecx;
    /* 0x00417C7E jmp loc_417CE0 */
    goto l_417ce0;
l_417c80:
    /* 0x00417C80 mov edx,[var_24] */
    edx = v24;
    /* 0x00417C87 mov edi,[var_20] */
    edi = v20;
    /* 0x00417C8E mov al,80h */
    eax = (eax & ~0xFFUL) | 0x80UL;
    /* 0x00417C90 sub edi,edx */
    edi = (edi - edx) & U32MASK;
    /* 0x00417C92 mov ecx,esi */
    ecx = esi;
    /* 0x00417C94 mov [var_20],edi */
    v20 = edi;
l_417c9b:
    /* 0x00417C9B xor edx,edx */
    edx = 0;
    /* 0x00417C9D mov dl,al */
    edx = (edx & ~0xFFUL) | (eax & 0xFFUL);
    /* 0x00417C9F add edx,edx */
    edx = (edx + edx) & U32MASK;
    /* 0x00417CA1 add edx,esi */
    edx = (edx + esi) & U32MASK;
    /* 0x00417CA3 mov di,[edx] */
    edi = (edi & ~0xFFFFUL) | ((unsigned long)M16(edx & U32MASK) & 0xFFFFUL);
    /* 0x00417CA6 cmp di,word ptr [var_20] */
    /* 0x00417CAE jnb loc_417CBB */
    if ((edi & 0xFFFFUL) >= (v20 & 0xFFFFUL)) goto l_417cbb;
    /* 0x00417CB0 mov esi,edx */
    esi = edx;
    /* 0x00417CB2 mov edx,edi */
    edx = edi;
    /* 0x00417CB4 sub [var_20],edx */
    v20 = (v20 - edx) & U32MASK;
l_417cbb:
    /* 0x00417CBB shr al,1 */
    eax = (eax & ~0xFFUL) | (((eax & 0xFFUL) >> 1) & 0xFFUL);
    /* 0x00417CBD test al,al */
    /* 0x00417CBF jnz loc_417C9B */
    if ((eax & 0xFFUL) != 0) goto l_417c9b;
    /* 0x00417CC1 mov eax,[var_24] */
    eax = v24;
    /* 0x00417CC8 mov edx,[var_20] */
    edx = v20;
    /* 0x00417CCF add edx,eax */
    edx = (edx + eax) & U32MASK;
    /* 0x00417CD1 mov eax,esi */
    eax = esi;
    /* 0x00417CD3 sub eax,ecx */
    eax = (eax - ecx) & U32MASK;
    /* 0x00417CD5 sar eax,1 */
    eax = ((unsigned long)(((int)(eax & U32MASK)) >> 1)) & U32MASK;
    /* 0x00417CD7 mov bh,al */
    ebx = (ebx & ~0xFF00UL) | ((eax & 0xFFUL) << 8);
    /* 0x00417CD9 mov [var_20],edx */
    v20 = edx;
l_417ce0:
    /* 0x00417CE0 mov byte_444EAA,bl */
    M8(0x444EAA) = (unsigned char)(ebx & 0xFFUL);
    /* 0x00417CE6 xor eax,eax */
    eax = 0;
    /* 0x00417CE8 mov ax,word ptr [var_20] */
    eax = (eax & ~0xFFFFUL) | (v20 & 0xFFFFUL);
    /* 0x00417CF0 xor edx,edx */
    edx = 0;
    /* 0x00417CF2 mov dword_4441F0,eax */
    M32(0x4441F0) = (unsigned int)(eax & U32MASK);
    /* 0x00417CF7 mov eax,[var_34] */
    eax = v34;
    /* 0x00417CFE mov dl,bh */
    edx = (edx & ~0xFFUL) | ((ebx >> 8) & 0xFFUL);
    /* 0x00417D00 shl eax,8 */
    eax = (eax << 8) & U32MASK;
    /* 0x00417D03 add eax,edx */
    eax = (eax + edx) & U32MASK;
    /* 0x00417D05 mov edx,dword_4299F4 */
    edx = (unsigned long)G32(0x4299F4);
    /* 0x00417D0B mov ax,[edx+eax*2] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)M16((edx + (eax & U32MASK) * 2UL) & U32MASK) & 0xFFFFUL);
    /* 0x00417D0F xor edx,edx */
    edx = 0;
    /* 0x00417D11 and eax,0FFFFh */
    eax &= 0xFFFFUL;
    /* 0x00417D16 mov dx,word ptr [var_20] */
    edx = (edx & ~0xFFFFUL) | (v20 & 0xFFFFUL);
    /* 0x00417D1E sub edx,eax */
    edx = (edx - eax) & U32MASK;
    /* 0x00417D20 mov eax,dword_4441F0 */
    eax = (unsigned long)M32(0x4441F0);
    /* 0x00417D25 mov ecx,dword_4441E8 */
    ecx = (unsigned long)M32(0x4441E8);
    /* 0x00417D2B sub eax,edx */
    eax = (eax - edx) & U32MASK;
    /* 0x00417D2D mov dword_4441EC,edx */
    M32(0x4441EC) = (unsigned int)(edx & U32MASK);
    /* 0x00417D33 shl eax,0Eh */
    eax = (eax << 14) & U32MASK;
    /* 0x00417D36 xor edx,edx */
    edx = 0;
    /* 0x00417D38 div ecx */
    eax = (ecx == 0) ? 0 : (eax / ecx) & U32MASK;
    edx = 0;
    /* 0x00417D3A mov ax,word_42D274[eax*2] */
    eax = (eax & ~0xFFFFUL) | ((unsigned long)G16((0x42D274UL + (eax & U32MASK) * 2UL) & U32MASK) & 0xFFFFUL);
    /* 0x00417D42 mov esi,dword_4441F4 */
    esi = (unsigned long)M32(0x4441F4);
    /* 0x00417D48 and eax,0FFFFh */
    eax &= 0xFFFFUL;
    /* 0x00417D4D add esi,eax */
    esi = (esi + eax) & U32MASK;
    /* 0x00417D4F mov eax,offset dword_4441E8 */
    eax = 0x4441E8UL;
    /* 0x00417D54 mov dword_4441F4,esi */
    M32(0x4441F4) = (unsigned int)(esi & U32MASK);
    /* 0x00417D5A call sub_423140 */
    d3u_423140(c, (unsigned int)(eax & U32MASK));
    /* 0x00417D5F mov al,bh */
    eax = (eax & ~0xFFUL) | ((ebx >> 8) & 0xFFUL);
    /* 0x00417D61 mov bl,byte_444EAA */
    ebx = (ebx & ~0xFFUL) | ((unsigned long)M8(0x444EAA) & 0xFFUL);
    /* 0x00417D67 add esp,0ACh */
    /* 0x00417D6D pop ebp/edi/esi/edx/ecx/ebx */
    /* 0x00417D73 retn */
    return (eax & 0xFFUL);
}

/* ---- range coder + stream + models (from draft_r3.c) ---- */
/* ---- compressed-input stream (sub_41E17C / sub_41E2B0 / sub_41E1C0) ---- */

void d3u_41e17c(dec3w_ctx *c)
{
    unsigned int eax = 0;
    unsigned int edx = 0;
    unsigned int cb = 0;
    /* 0x0041E17C push edx */
    /* 0x0041E17D mov eax, dword_444EF4 */
    eax = G32(0x444EF4);
    /* 0x0041E182 xor edx, edx */
    edx = 0;
    /* 0x0041E184 mov dword_436E40, eax */
    G32(0x436E40) = eax;
    /* 0x0041E189 mov dword_444EE8, edx */
    G32(0x444EE8) = edx;
    /* 0x0041E18F mov eax, dword_444EF8 */
    eax = G32(0x444EF8);
    /* 0x0041E194 mov edx, dword_444EFC */
    edx = G32(0x444EFC);
    /* 0x0041E19A mov dword_436E44, eax */
    G32(0x436E44) = eax;
    /* 0x0041E19F mov eax, dword_42C9A8 */
    eax = G32(0x42C9A8);
    /* 0x0041E1A4 call dword_42CBB0 */
    cb = G32(0x42CBB0);
    (void)cb;
    /* framework refill: read (edx capped to buffer) at arena eax via rcb */
    { unsigned int _room = ((eax & U32MASK) >= G32(0x42C9A8UL)) ? 0x100000U - ((eax & U32MASK) - G32(0x42C9A8UL)) : 0U;
      unsigned int _n = (edx & U32MASK) < _room ? (edx & U32MASK) : _room;
      eax = (c->rcb != 0 && _n != 0) ? (unsigned int)c->rcb(c->rctx, c->mem + (eax & U32MASK), _n) : 0U; }
    /* 0x0041E1AA mov dword_444EF0, eax */
    G32(0x444EF0) = eax;
    /* 0x0041E1AF test eax, eax */
    /* 0x0041E1B1 jnz loc_41E1BD */
    if (eax != 0) goto loc_41E1BD;
    /* 0x0041E1B3 mov dword_42C9A0, 3 */
    G32(0x42C9A0) = 3;
loc_41E1BD:
    /* 0x0041E1BD pop edx */
    /* 0x0041E1BE retn */
    ;
}
unsigned int d3u_41e2b0(dec3w_ctx *c)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int edx = 0;
    /* 0x0041E2B0 push ebx */
    /* 0x0041E2B1 push edx */
    /* 0x0041E2B2 mov eax, dword_444EE8 */
    eax = G32(0x444EE8);
    /* 0x0041E2B7 cmp eax, dword_444EF0 */
    /* 0x0041E2BD jnz loc_41E2C4 */
    if (eax != G32(0x444EF0)) goto loc_41E2C4;
    /* 0x0041E2BF call sub_41E17C */
    d3u_41e17c(c);
loc_41E2C4:
    /* 0x0041E2C4 mov ebx, dword_444EF4 */
    ebx = G32(0x444EF4);
    /* 0x0041E2CA inc ebx */
    ebx = (ebx + 1) & U32MASK;
    /* 0x0041E2CB mov eax, dword_444EE8 */
    eax = G32(0x444EE8);
    /* 0x0041E2D0 mov dword_444EF4, ebx */
    G32(0x444EF4) = ebx;
    /* 0x0041E2D6 lea ebx, [eax+1] */
    ebx = (eax + 1) & U32MASK;
    /* 0x0041E2D9 mov edx, dword_42C9A8 */
    edx = G32(0x42C9A8);
    /* 0x0041E2DF mov dword_444EE8, ebx */
    G32(0x444EE8) = ebx;
    /* 0x0041E2E5 mov al, [edx+eax] */
    /* framework-owned host read through the input base (lane-merge into al) */
    eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((edx + eax) & U32MASK); /* 0x0041E2E5 mov al,[edx+eax] (arena: edx=input base) */
    /* 0x0041E2E8 pop edx */
    /* 0x0041E2E9 pop ebx */
    /* 0x0041E2EA retn */
    return eax & U32MASK;
}
void d3u_41e1c0(dec3w_ctx *c, unsigned int eax_v)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int cb = 0;
    /* 0x0041E1C0 push ebx/ecx/edx/esi/edi */
    /* 0x0041E1C5 mov ecx, dword_444EF0 */
    ecx = G32(0x444EF0);
    /* 0x0041E1CB mov esi, eax */
    esi = eax_v;
    eax = eax_v;
    /* 0x0041E1CD mov edx, dword_444EE8 */
    edx = G32(0x444EE8);
    /* 0x0041E1D5 mov edi, ecx */
    edi = ecx;
    /* 0x0041E1D5 sub edi, edx */
    edi = (edi - edx) & U32MASK;
    /* 0x0041E1D7 cmp edi, eax */
    /* 0x0041E1D9 jnb loc_41E23B */
    if (edi >= eax) goto loc_41E23B;
    /* 0x0041E1DB mov eax, dword_42C9A8 */
    eax = G32(0x42C9A8);
    /* 0x0041E1E0 mov ebx, edi */
    ebx = edi;
    /* 0x0041E1E2 mov ecx, edi */
    ecx = edi;
    /* 0x0041E1E4 add edx, eax */
    edx = (edx + eax) & U32MASK;
    /* 0x0041E1E6 mov edi, 3 */
    edi = 3;
    /* 0x0041E1EB call sub_424147 */
    d3u_424147(c, eax, edx, ebx);
    /* 0x0041E1F0 mov eax, dword_444EF4 */
    eax = G32(0x444EF4);
    /* 0x0041E1F5 xor ebx, ebx */
    ebx = 0;
    /* 0x0041E1F7 mov dword_436E40, eax */
    G32(0x436E40) = eax;
    /* 0x0041E1FC mov eax, dword_444EF8 */
    eax = G32(0x444EF8);
    /* 0x0041E201 mov dword_444EE8, ebx */
    G32(0x444EE8) = ebx;
    /* 0x0041E207 mov dword_436E44, eax */
    G32(0x436E44) = eax;
loc_41E20C:
    /* 0x0041E20C mov edx, dword_444EFC */
    edx = G32(0x444EFC);
    /* 0x0041E212 mov eax, dword_42C9A8 */
    eax = G32(0x42C9A8);
    /* 0x0041E217 mov ebx, ecx */
    ebx = ecx;
    /* 0x0041E219 sub edx, ecx */
    edx = (edx - ecx) & U32MASK;
    /* 0x0041E21B add eax, ecx */
    eax = (eax + ecx) & U32MASK;
    /* 0x0041E21D mov dword_444EF0, ecx */
    G32(0x444EF0) = ecx;
    /* 0x0041E223 call dword_42CBB0 */
    cb = G32(0x42CBB0);
    (void)cb;
    /* framework refill: read (edx capped) at arena eax via rcb */
    { unsigned int _room = ((eax & U32MASK) >= G32(0x42C9A8UL)) ? 0x100000U - ((eax & U32MASK) - G32(0x42C9A8UL)) : 0U;
      unsigned int _n = (edx & U32MASK) < _room ? (edx & U32MASK) : _room;
      eax = (c->rcb != 0 && _n != 0) ? (unsigned int)c->rcb(c->rctx, c->mem + (eax & U32MASK), _n) : 0U; }
    /* 0x0041E229 mov ecx, eax */
    ecx = eax;
    /* 0x0041E22B test eax, eax */
    /* 0x0041E22D jnz loc_41E235 */
    if (eax != 0) goto loc_41E235;
    /* 0x0041E22F mov dword_42C9A0, edi */
    G32(0x42C9A0) = edi;
loc_41E235:
    /* 0x0041E235 add ecx, ebx */
    ecx = (ecx + ebx) & U32MASK;
    /* 0x0041E237 cmp ecx, esi */
    /* 0x0041E239 jb loc_41E20C */
    if (ecx < esi) goto loc_41E20C;
loc_41E23B:
    /* 0x0041E23B mov dword_444EF0, ecx */
    G32(0x444EF0) = ecx;
    /* 0x0041E241 pop edi/esi/edx/ecx/ebx */
    /* 0x0041E246 retn */
    ;
}
/* ---- range coder (sub_423188 / sub_423140 / sub_42335C / sub_423388) ---- */

unsigned int d3u_423188(dec3w_ctx *c, unsigned int total)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    /* 0x00423188 push ebx/ecx/edx/esi/edi */
    /* 0x0042318D mov ebx, dword_4450F4 */
    ebx = G32(0x4450F4);
    /* 0x00423193 mov ecx, dword_4450F8 */
    ecx = G32(0x4450F8);
    /* 0x00423199 mov esi, eax */
    esi = total;
    /* 0x0042319B cmp ecx, 800000h */
    /* 0x004231A1 ja loc_4231F0 */
    if (ecx > 0x800000UL) goto loc_4231F0;
loc_4231A3:
    /* 0x004231A3 xor eax, eax */
    eax = 0;
    /* 0x004231A5 mov al, byte_4450FC */
    eax = (unsigned int)G8(0x4450FC);
    /* 0x004231AA mov edx, eax */
    edx = eax;
    /* 0x004231AC shl edx, 7 */
    edx = (edx << 7) & U32MASK;
    /* 0x004231AF shl ebx, 8 */
    ebx = (ebx << 8) & U32MASK;
    /* 0x004231B2 and edx, 0FFh */
    edx &= 0xFFU;
    /* 0x004231B8 or ebx, edx */
    ebx |= edx;
    /* 0x004231BA mov dword_4450F8, ecx */
    G32(0x4450F8) = ecx;
    /* 0x004231C0 mov dword_4450F4, ebx */
    G32(0x4450F4) = ebx;
    /* 0x004231C6 call sub_41E2B0 */
    eax = d3u_41e2b0(c);
    /* 0x004231CB mov ecx, dword_4450F8 */
    ecx = G32(0x4450F8);
    /* 0x004231D1 mov ebx, dword_4450F4 */
    ebx = G32(0x4450F4);
    /* 0x004231D7 mov byte_4450FC, al */
    G8(0x4450FC) = (unsigned char)(eax & 0xFFU);
    /* 0x004231DC and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x004231E1 sar eax, 1 */
    eax = (unsigned int)(((int)eax) >> 1);
    /* 0x004231E3 shl ecx, 8 */
    ecx = (ecx << 8) & U32MASK;
    /* 0x004231E6 or ebx, eax */
    ebx |= eax;
    /* 0x004231E8 cmp ecx, 800000h */
    /* 0x004231EE jbe loc_4231A3 */
    if (ecx <= 0x800000UL) goto loc_4231A3;
loc_4231F0:
    /* 0x004231F0 and esi, 0FFFFh */
    esi &= 0xFFFFU;
    /* 0x004231F6 mov eax, ecx */
    eax = ecx;
    /* 0x004231F8 xor edx, edx */
    edx = 0;
    /* 0x004231FA div esi */
    /* x86 div faults on div0/overflow: direct / % below */
    eax = ecx / esi;
    edx = ecx % esi;
    /* 0x004231FC mov dword_4450F0, eax */
    G32(0x4450F0) = eax;
    /* 0x00423201 xor edx, edx */
    edx = 0;
    /* 0x00423203 mov edi, dword_4450F0 */
    edi = G32(0x4450F0);
    /* 0x00423209 mov eax, ebx */
    eax = ebx;
    /* 0x0042320B div edi */
    /* x86 div faults on div0 (quotient 0 when range < total); remainder dropped */
    eax = ebx / edi;
    /* 0x0042320D cmp eax, esi */
    /* 0x0042320F jb loc_423214 */
    if (eax < esi) goto loc_423214;
    /* 0x00423211 lea eax, [esi-1] */
    eax = (esi - 1) & U32MASK;
loc_423214:
    /* 0x00423214 mov dword_4450F8, ecx */
    G32(0x4450F8) = ecx;
    /* 0x0042321A mov dword_4450F4, ebx */
    G32(0x4450F4) = ebx;
    /* 0x00423220 pop edi/esi/edx/ecx/ebx */
    /* 0x00423225 retn */
    return eax & U32MASK;
}
void d3u_423140(dec3w_ctx *c, unsigned int p)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int ebp = 0;
    /* 0x00423140 push ebx/ecx/edx/esi/ebp */
    /* 0x00423145 mov edx, dword_4450F0 */
    edx = G32(0x4450F0);
    /* 0x0042314B imul edx, [eax+4] */
    edx = (edx * M32(p + 4)) & U32MASK;
    /* 0x0042314F mov ecx, dword_4450F4 */
    ecx = G32(0x4450F4);
    /* 0x00423155 mov esi, [eax] */
    esi = M32(p + 0);
    /* 0x00423157 sub ecx, edx */
    ecx = (ecx - edx) & U32MASK;
    /* 0x00423159 mov ebx, [eax+8] */
    ebx = M32(p + 8);
    /* 0x0042315C mov dword_4450F4, ecx */
    G32(0x4450F4) = ecx;
    /* 0x00423162 cmp ebx, esi */
    /* 0x00423164 jnb loc_42317C */
    if (ebx >= esi) goto loc_42317C;
    /* 0x00423166 mov edx, ebx */
    edx = ebx;
    /* 0x00423168 mov ebp, [eax+4] */
    ebp = M32(p + 4);
    /* 0x0042316B mov eax, dword_4450F0 */
    eax = G32(0x4450F0);
    /* 0x00423170 sub edx, ebp */
    edx = (edx - ebp) & U32MASK;
    /* 0x00423172 imul eax, edx */
    eax = (eax * edx) & U32MASK;
    /* 0x00423175 mov dword_4450F8, eax */
    G32(0x4450F8) = eax;
    /* 0x0042317A jmp loc_423182 */
    goto loc_423182;
loc_42317C:
    /* 0x0042317C sub dword_4450F8, edx */
    G32(0x4450F8) = (G32(0x4450F8) - edx) & U32MASK;
loc_423182:
    /* 0x00423182 pop ebp/esi/edx/ecx/ebx */
    /* 0x00423187 retn */
    ;
}
void d3u_42335c(dec3w_ctx *c)
{
    unsigned int eax = 0;
    unsigned int edx = 0;
    /* 0x0042335C push edx */
    /* 0x0042335D call sub_41E2B0 */
    eax = d3u_41e2b0(c);
    /* 0x00423362 call sub_41E2B0 */
    /* first byte discarded; only the second byte seeds the coder */
    eax = d3u_41e2b0(c);
    /* 0x00423367 mov byte_4450FC, al */
    G8(0x4450FC) = (unsigned char)(eax & 0xFFU);
    /* 0x0042336C and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00423371 mov edx, 80h */
    edx = 0x80U;
    /* 0x00423376 sar eax, 1 */
    eax = (unsigned int)(((int)eax) >> 1);
    /* 0x00423378 mov dword_4450F8, edx */
    G32(0x4450F8) = edx;
    /* 0x0042337E mov dword_4450F4, eax */
    G32(0x4450F4) = eax;
    /* 0x00423383 pop edx */
    /* 0x00423384 retn */
}
void d3u_423388(dec3w_ctx *c)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int edx = 0;
    /* 0x00423388 push ebx/edx */
    /* 0x0042338A mov ebx, 80000000h */
    ebx = 0x80000000UL;
    /* 0x0042338F xor edx, edx */
    edx = 0;
    /* 0x00423391 mov al, byte ptr dword_444EE4 */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8(0x444EE4);
    /* 0x00423396 mov dword_4450F4, edx */
    G32(0x4450F4) = edx;
    /* 0x0042339C mov byte_4450FC, al */
    /* intermediate store; overwritten below, kept for faithfulness */
    G8(0x4450FC) = (unsigned char)(eax & 0xFFU);
    /* 0x004233A1 mov ah, al */
    eax = (eax & 0xFFFF00FFUL) | ((eax & 0xFFU) << 8);
    /* 0x004233A3 mov dword_4450F8, ebx */
    G32(0x4450F8) = ebx;
    /* 0x004233A9 not ah */
    eax = (eax & 0xFFFF00FFUL) | (((~(eax >> 8)) & 0xFFU) << 8);
    /* 0x004233AB mov dword_4450F0, edx */
    G32(0x4450F0) = edx;
    /* 0x004233B1 mov byte_4450FC, ah */
    G8(0x4450FC) = (unsigned char)((eax >> 8) & 0xFFU);
    /* 0x004233B7 pop edx/ebx */
    /* 0x004233B9 retn */
}
/* sub_414EC0: entry eax = context id, dl = symbol. */
void d3u_414ec0(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int ebp = 0;
    unsigned int v1c = 0;
    unsigned int v18 = 0;
    unsigned int adr = 0;
    /* 0x00414EC0 push ebx/ecx/esi/edi/ebp; sub esp, 8 */
    /* 0x00414EC8 mov [var_1C], eax */
    v1c = eax_v;
    /* 0x00414ECB xor ebx, ebx */
    ebx = 0;
    /* 0x00414ECD shl eax, 8 */
    eax = (eax_v << 8) & U32MASK;
    /* 0x00414ED0 mov bl, dl */
    ebx = (ebx & 0xFFFFFF00UL) | (edx_v & 0xFFU);
    /* 0x00414ED2 add eax, ebx */
    eax = (eax + ebx) & U32MASK;
    /* 0x00414ED4 mov ebp, dword_4299F4 */
    ebp = G32(0x4299F4);
    /* 0x00414EDA add eax, eax */
    eax = (eax + eax) & U32MASK;
    /* 0x00414EDC lea ecx, [eax+ebp] */
    ecx = (eax + ebp) & U32MASK;
    /* 0x00414EDF mov eax, [var_1C] */
    eax = v1c;
    /* 0x00414EE2 add eax, eax */
    eax = (eax + eax) & U32MASK;
    /* 0x00414EE4 mov di, [ecx] */
    edi = (edi & 0xFFFF0000UL) | (unsigned int)M16(ecx);
    /* 0x00414EE7 mov si, word_44428A[eax] */
    esi = (esi & 0xFFFF0000UL) | (unsigned int)M16((0x44428AU + eax) & U32MASK);
    /* 0x00414EEE test di, di */
    /* 0x00414EF1 jnz loc_414F6C */
    if ((edi & 0xFFFFU) != 0) goto loc_414F6C;
    /* 0x00414EF7 mov di, word_444884[eax] */
    edi = (edi & 0xFFFF0000UL) | (unsigned int)M16((0x444884U + eax) & U32MASK);
    /* 0x00414EFE mov [var_18], edi */
    /* upper 16 = entry edi (unknown); only the low 16 is read below */
    v18 = edi & 0xFFFFU;
    /* 0x00414F02 mov edi, dword_4299FC */
    edi = G32(0x4299FC);
    /* 0x00414F08 add eax, edi */
    eax = (eax + edi) & U32MASK;
    /* 0x00414F0A add [eax], si */
    M16(eax) = (unsigned short)(((unsigned int)M16(eax) + (esi & 0xFFFFU)) & 0xFFFFU);
    /* 0x00414F0D mov eax, dword_44421C */
    eax = G32(0x44421C);
    /* 0x00414F12 shl eax, 8 */
    eax = (eax << 8) & U32MASK;
    /* 0x00414F15 add eax, ebx */
    eax = (eax + ebx) & U32MASK;
    /* 0x00414F17 mov ebx, [var_18] */
    ebx = v18;
    /* 0x00414F1B add [ebp+eax*2], bx */
    adr = (ebp + (eax << 1)) & U32MASK;
    M16(adr) = (unsigned short)(((unsigned int)M16(adr) + (ebx & 0xFFFFU)) & 0xFFFFU);
    /* 0x00414F20 mov ebp, dword_44421C */
    ebp = G32(0x44421C);
    /* 0x00414F26 mov eax, dword_4299F0 */
    eax = G32(0x4299F0);
    /* 0x00414F2B shl ebp, 9 */
    ebp = (ebp << 9) & U32MASK;
    /* 0x00414F2E add ebp, eax */
    ebp = (ebp + eax) & U32MASK;
    /* 0x00414F30 mov al, dl */
    eax = (eax & 0xFFFFFF00UL) | (edx_v & 0xFFU);
    /* 0x00414F32 inc al */
    eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFU) + 1) & 0xFFU);
    /* 0x00414F34 jz loc_414F4D */
    if ((eax & 0xFFU) == 0) goto loc_414F4D;
    /* 0x00414F36 mov edi, [var_18] */
    edi = v18;
loc_414F3A:
    /* 0x00414F3A xor ebx, ebx */
    ebx = 0;
    /* 0x00414F3C mov bl, al */
    ebx = eax & 0xFFU;
    /* 0x00414F3E mov ah, al */
    eax = (eax & 0xFFFF00FFUL) | ((eax & 0xFFU) << 8);
    /* 0x00414F40 neg ah */
    eax = (eax & 0xFFFF00FFUL) | (((0U - ((eax >> 8) & 0xFFU)) & 0xFFU) << 8);
    /* 0x00414F42 and ah, al */
    eax = (eax & 0xFFFF00FFUL) | (((((eax >> 8) & 0xFFU) & (eax & 0xFFU))) << 8);
    /* 0x00414F44 add [ebp+ebx*2], di */
    adr = (ebp + (ebx << 1)) & U32MASK;
    M16(adr) = (unsigned short)(((unsigned int)M16(adr) + (edi & 0xFFFFU)) & 0xFFFFU);
    /* 0x00414F49 add al, ah */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) + ((eax >> 8) & 0xFFU))) & 0xFFU);
    /* 0x00414F4B jnz loc_414F3A */
    if ((eax & 0xFFU) != 0) goto loc_414F3A;
loc_414F4D:
    /* 0x00414F4D mov eax, [var_18] */
    eax = v18;
    /* 0x00414F51 mov bx, [ebp+0] */
    ebx = (ebx & 0xFFFF0000UL) | (unsigned int)M16(ebp);
    /* 0x00414F55 add ebx, eax */
    ebx = (ebx + eax) & U32MASK;
    /* 0x00414F57 mov [ebp+0], bx */
    M16(ebp) = (unsigned short)(ebx & 0xFFFFU);
    /* 0x00414F5B cmp bx, 4000h */
    /* 0x00414F60 jb loc_414F6C */
    if ((ebx & 0xFFFFU) < 0x4000U) goto loc_414F6C;
    /* 0x00414F62 mov eax, dword_44421C */
    eax = G32(0x44421C);
    /* 0x00414F67 call sub_414D78 */
    d3u_414d78(c, eax);
loc_414F6C:
    /* 0x00414F6C mov ebp, [var_1C] */
    ebp = v1c;
    /* 0x00414F6F cmp ebp, 200h */
    /* 0x00414F75 jz loc_414FCB */
    if (ebp == 0x200U) goto loc_414FCB;
    /* 0x00414F77 add [ecx], si */
    M16(ecx) = (unsigned short)(((unsigned int)M16(ecx) + (esi & 0xFFFFU)) & 0xFFFFU);
    /* 0x00414F7A mov eax, ebp */
    eax = ebp;
    /* 0x00414F7C mov ecx, dword_4299F0 */
    ecx = G32(0x4299F0);
    /* 0x00414F82 shl eax, 9 */
    eax = (eax << 9) & U32MASK;
    /* 0x00414F85 add ecx, eax */
    ecx = (ecx + eax) & U32MASK;
    /* 0x00414F87 inc dl */
    edx = edx_v;
    edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFU) + 1) & 0xFFU);
    /* 0x00414F89 jz loc_414F9B */
    if ((edx & 0xFFU) == 0) goto loc_414F9B;
loc_414F8B:
    /* 0x00414F8B xor eax, eax */
    eax = 0;
    /* 0x00414F8D mov al, dl */
    eax = edx & 0xFFU;
    /* 0x00414F8F add [ecx+eax*2], si */
    adr = (ecx + (eax << 1)) & U32MASK;
    M16(adr) = (unsigned short)(((unsigned int)M16(adr) + (esi & 0xFFFFU)) & 0xFFFFU);
    /* 0x00414F93 neg al */
    eax = (0U - (eax & 0xFFU)) & 0xFFU;
    /* 0x00414F95 and al, dl */
    eax = (eax & (edx & 0xFFU)) & 0xFFU;
    /* 0x00414F97 add dl, al */
    edx = (edx & 0xFFFFFF00UL) | ((((edx & 0xFFU) + (eax & 0xFFU))) & 0xFFU);
    /* 0x00414F99 jnz loc_414F8B */
    if ((edx & 0xFFU) != 0) goto loc_414F8B;
loc_414F9B:
    /* 0x00414F9B mov eax, [var_1C] */
    eax = v1c;
    /* 0x00414F9E mov ebx, dword_4299FC */
    ebx = G32(0x4299FC);
    /* 0x00414FA4 add eax, eax */
    eax = (eax + eax) & U32MASK;
    /* 0x00414FA6 mov di, [ecx] */
    edi = (edi & 0xFFFF0000UL) | (unsigned int)M16(ecx);
    /* 0x00414FA9 add eax, ebx */
    eax = (eax + ebx) & U32MASK;
    /* 0x00414FAB xor ebx, ebx */
    ebx = 0;
    /* 0x00414FAD add edi, esi */
    edi = (edi + esi) & U32MASK;
    /* 0x00414FAF mov bx, [eax] */
    ebx = (unsigned int)M16(eax);
    /* 0x00414FB2 xor eax, eax */
    eax = 0;
    /* 0x00414FB4 mov ax, di */
    eax = edi & 0xFFFFU;
    /* 0x00414FB7 add eax, ebx */
    eax = (eax + ebx) & U32MASK;
    /* 0x00414FB9 mov [ecx], di */
    M16(ecx) = (unsigned short)(edi & 0xFFFFU);
    /* 0x00414FBC cmp eax, 4000h */
    /* 0x00414FC1 jl loc_414FCB */
    if ((int)eax < 0x4000) goto loc_414FCB;
    /* 0x00414FC3 mov eax, [var_1C] */
    eax = v1c;
    /* 0x00414FC6 call sub_414D78 */
    d3u_414d78(c, eax);
loc_414FCB:
    /* 0x00414FCB add esp, 8; pop ebp/edi/esi/ecx/ebx */
    /* 0x00414FD3 retn */
    ;
}
/* sub_414FD4: entry al = index, dl = flag. */
void d3u_414fd4(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int adr = 0;
    /* 0x00414FD4 push ebx/ecx/esi */
    /* 0x00414FD7 xor ebx, ebx */
    ebx = 0;
    /* 0x00414FD9 mov bl, al */
    ebx = eax_v & 0xFFU;
    edx = edx_v;
    /* 0x00414FDB xor eax, eax */
    eax = 0;
    /* 0x00414FDD mov al, byte_444EB6 */
    eax = (unsigned int)G8(0x444EB6);
    /* 0x00414FE2 add eax, ebx */
    eax = (eax + ebx) & U32MASK;
    /* 0x00414FE4 shl eax, 3 */
    eax = (eax << 3) & U32MASK;
    /* 0x00414FE7 add eax, offset dword_443B68 */
    eax = (eax + 0x443B68U) & U32MASK;
    /* 0x00414FEC cmp dl, 1 */
    /* 0x00414FEF jnz loc_414FFB */
    if ((edx & 0xFFU) != 1) goto loc_414FFB;
    /* 0x00414FF1 xor ebx, ebx */
    ebx = 0;
    /* 0x00414FF3 mov bl, dl */
    ebx = edx & 0xFFU;
    /* 0x00414FF5 add word ptr [eax+ebx*2+2], 40h */
    adr = (eax + (ebx << 1) + 2) & U32MASK;
    M16(adr) = (unsigned short)(((unsigned int)M16(adr) + 0x40U) & 0xFFFFU);
loc_414FFB:
    /* 0x00414FFB mov si, [eax+6] */
    /* esi high = entry garbage; only low 16 observed below */
    esi = (unsigned int)M16((eax + 6) & U32MASK);
    /* 0x00414FFF add esi, 40h */
    esi = (esi + 0x40U) & U32MASK;
    /* 0x00415002 mov [eax+6], si */
    M16((eax + 6) & U32MASK) = (unsigned short)(esi & 0xFFFFU);
    /* 0x00415006 cmp si, 4000h */
    /* 0x0041500B jb loc_41503F */
    if ((esi & 0xFFFFU) < 0x4000U) goto loc_41503F;
    /* 0x0041500D xor ebx, ebx */
    ebx = 0;
    /* 0x0041500F xor ecx, ecx */
    ecx = 0;
    /* 0x00415011 mov bx, [eax+4] */
    ebx = (unsigned int)M16((eax + 4) & U32MASK);
    /* 0x00415015 mov cx, [eax+2] */
    ecx = (unsigned int)M16((eax + 2) & U32MASK);
    /* 0x00415019 mov esi, ebx */
    esi = ebx;
    /* 0x0041501B sub esi, ecx */
    esi = (esi - ecx) & U32MASK;
    /* 0x0041501D lea ecx, [esi+1] */
    ecx = (esi + 1) & U32MASK;
    /* 0x00415020 sar ecx, 1 */
    ecx = (unsigned int)(((int)ecx) >> 1);
    /* 0x00415022 mov esi, ecx */
    esi = ecx;
    /* 0x00415024 xor ecx, ecx */
    ecx = 0;
    /* 0x00415026 mov cx, [eax+6] */
    ecx = (unsigned int)M16((eax + 6) & U32MASK);
    /* 0x0041502A sub ecx, ebx */
    ecx = (ecx - ebx) & U32MASK;
    /* 0x0041502C inc ecx */
    ecx = (ecx + 1) & U32MASK;
    /* 0x0041502D mov bx, [eax+2] */
    ebx = (ebx & 0xFFFF0000UL) | (unsigned int)M16((eax + 2) & U32MASK);
    /* 0x00415031 sar ecx, 1 */
    ecx = (unsigned int)(((int)ecx) >> 1);
    /* 0x00415033 add esi, ebx */
    esi = (esi + ebx) & U32MASK;
    /* 0x00415035 add ecx, esi */
    ecx = (ecx + esi) & U32MASK;
    /* 0x00415037 mov [eax+4], si */
    M16((eax + 4) & U32MASK) = (unsigned short)(esi & 0xFFFFU);
    /* 0x0041503B mov [eax+6], cx */
    M16((eax + 6) & U32MASK) = (unsigned short)(ecx & 0xFFFFU);
loc_41503F:
    /* 0x0041503F mov al, byte_444EB6 */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8(0x444EB6);
    /* 0x00415044 add al, al */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) + (eax & 0xFFU))) & 0xFFU);
    /* 0x00415046 add al, dl */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) + (edx & 0xFFU))) & 0xFFU);
    /* 0x00415048 dec al */
    eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFU) - 1) & 0xFFU);
    /* 0x0041504A and al, 7 */
    eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFU) & 7U);
    /* 0x0041504C mov byte_444EB6, al */
    G8(0x444EB6) = (unsigned char)(eax & 0xFFU);
    /* 0x00415051 pop esi/ecx/ebx */
    /* 0x00415054 retn */
}
/* sub_4153C0: entry al = symbol. Pure 8-way model update; no coder calls. */
void d3u_4153c0(dec3w_ctx *c, unsigned int eax_v)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int ebp = 0;
    /* 0x004153C0 push ebx/ecx/edx/esi/edi/ebp */
    /* 0x004153C6 mov esi, dword_44427C */
    esi = G32(0x44427C);
    /* 0x004153CC mov ebp, dword_444204 */
    ebp = G32(0x444204);
    /* 0x004153D2 mov cl, al */
    /* 0x004153D4 xor ch, ch */
    /* ecx high = entry garbage; only cl/ch lanes observed below */
    ecx = eax_v & 0xFFU;
loc_4153D6:
    /* 0x004153D6 xor eax, eax */
    eax = 0;
    /* 0x004153D8 mov al, ch */
    eax = (ecx >> 8) & 0xFFU;
    /* 0x004153DA mov edi, dword_443CE8[eax*4] */
    edi = M32((0x443CE8U + (eax << 2)) & U32MASK);
    /* 0x004153E1 mov ebx, dword_4299D0[eax*4] */
    ebx = M32((0x4299D0U + (eax << 2)) & U32MASK);
    /* 0x004153E8 shl edi, 2 */
    edi = (edi << 2) & U32MASK;
    /* 0x004153EB add ebx, edi */
    ebx = (ebx + edi) & U32MASK;
    /* 0x004153ED mov ah, [ebx+1] */
    eax = (eax & 0xFFFF00FFUL) | (((unsigned int)M8((ebx + 1) & U32MASK)) << 8);
    /* 0x004153F0 test ah, ah */
    /* 0x004153F2 jz loc_415506 */
    if (((eax >> 8) & 0xFFU) == 0) goto loc_415506;
    /* 0x004153F8 cmp ch, byte_444EA3 */
    /* 0x004153FE jnz loc_41547F */
    if (((ecx >> 8) & 0xFFU) != (unsigned int)G8(0x444EA3)) goto loc_41547F;
    /* 0x00415404 cmp byte_444EB2, 0 */
    /* 0x0041540B jz loc_415445 */
    if (G8(0x444EB2) == 0) goto loc_415445;
    /* 0x0041540D cmp cl, [ebx+2] */
    /* 0x00415410 jnz loc_415420 */
    if ((ecx & 0xFFU) != (unsigned int)M8((ebx + 2) & U32MASK)) goto loc_415420;
    /* 0x00415412 xor ah, ah */
    eax &= 0xFFFF00FFUL;
    /* 0x00415414 mov byte_444EAE, ah */
    G8(0x444EAE) = 0;
    /* 0x0041541A xor eax, eax */
    eax = 0;
    /* 0x0041541C mov al, [ebx] */
    eax = (unsigned int)M8(ebx);
    /* 0x0041541E jmp loc_415425 */
    goto loc_415425;
loc_415420:
    /* 0x00415420 xor eax, eax */
    eax = 0;
    /* 0x00415422 mov al, [ebx+1] */
    eax = (unsigned int)M8((ebx + 1) & U32MASK);
loc_415425:
    /* 0x00415425 xor edx, edx */
    edx = 0;
    /* 0x00415427 movzx edi, byte ptr [ebx] */
    edi = (unsigned int)M8(ebx);
    /* 0x0041542A mov dl, [ebx+1] */
    edx = (unsigned int)M8((ebx + 1) & U32MASK);
    /* 0x0041542D shl eax, 0Eh */
    eax = (eax << 14) & U32MASK;
    /* 0x00415430 add edi, edx */
    edi = (edi + edx) & U32MASK;
    /* 0x00415432 xor edx, edx */
    edx = 0;
    /* 0x00415434 div edi */
    /* x86 div faults on div0 (both counts zero); direct / % below */
    eax = eax / edi;
    /* remainder in edx discarded by the original */
    /* 0x00415436 mov ax, word_42D274[eax*2] */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16((0x42D274U + (eax << 1)) & U32MASK);
    /* 0x0041543E and eax, 0FFFFh */
    eax &= 0xFFFFU;
    /* 0x00415443 add ebp, eax */
    ebp = (ebp + eax) & U32MASK;
    /* 0x00415445 loc_415445: falls through below */
loc_415445:
    /* 0x00415445 test esi, esi */
    /* 0x00415447 jz loc_41547F */
    if (esi == 0) goto loc_41547F;
    /* 0x00415449 cmp cl, [ebx+2] */
    /* 0x0041544C jnz loc_415452 */
    if ((ecx & 0xFFU) != (unsigned int)M8((ebx + 2) & U32MASK)) goto loc_415452;
    /* 0x0041544E inc byte ptr [esi] */
    M8(esi) = (unsigned char)(((unsigned int)M8(esi) + 1) & 0xFFU);
    /* 0x00415450 jmp loc_415455 */
    goto loc_415455;
loc_415452:
    /* 0x00415452 inc byte ptr [esi+1] */
    M8((esi + 1) & U32MASK) = (unsigned char)(((unsigned int)M8((esi + 1) & U32MASK) + 1) & 0xFFU);
loc_415455:
    /* 0x00415455 mov dh, [esi] */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(esi)) << 8);
    /* 0x00415457 mov eax, esi */
    eax = esi;
    /* 0x00415459 cmp dh, 0FEh */
    /* 0x0041545C jnb loc_415464 */
    if (((edx >> 8) & 0xFFU) >= 0xFEU) goto loc_415464;
    /* 0x0041545E cmp byte ptr [esi+1], 0FEh */
    /* 0x00415462 jb loc_41547F */
    if ((unsigned int)M8((esi + 1) & U32MASK) < 0xFEU) goto loc_41547F;
loc_415464:
    /* 0x00415464 mov dh, [esi] */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(esi)) << 8);
    /* 0x00415466 inc dh */
    edx = (edx & 0xFFFF00FFUL) | (((((edx >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    /* 0x00415468 mov [esi], dh */
    M8(esi) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x0041546A mov dl, dh */
    edx = (edx & 0xFFFFFF00UL) | ((edx >> 8) & 0xFFU);
    /* 0x0041546C shr dl, 1 */
    edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFU) >> 1) & 0xFFU);
    /* 0x0041546E mov [esi], dl */
    M8(esi) = (unsigned char)(edx & 0xFFU);
    /* 0x00415470 mov dh, [esi+1] */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8((esi + 1) & U32MASK)) << 8);
    /* 0x00415473 inc dh */
    edx = (edx & 0xFFFF00FFUL) | (((((edx >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    /* 0x00415475 mov [esi+1], dh */
    M8((esi + 1) & U32MASK) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x00415478 mov dl, dh */
    edx = (edx & 0xFFFFFF00UL) | ((edx >> 8) & 0xFFU);
    /* 0x0041547A shr dl, 1 */
    edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFU) >> 1) & 0xFFU);
    /* 0x0041547C mov [esi+1], dl */
    M8((esi + 1) & U32MASK) = (unsigned char)(edx & 0xFFU);
loc_41547F:
    /* 0x0041547F mov dh, [ebx+3] */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8((ebx + 3) & U32MASK)) << 8);
    /* 0x00415482 and dh, 7Fh */
    edx = (edx & 0xFFFF00FFUL) | ((((edx >> 8) & 0xFFU) & 0x7FU) << 8);
    /* 0x00415485 mov al, [ebx+2] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8((ebx + 2) & U32MASK);
    /* 0x00415488 mov [ebx+3], dh */
    M8((ebx + 3) & U32MASK) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x0041548B cmp cl, al */
    /* 0x0041548D jnz loc_4154C4 */
    if ((ecx & 0xFFU) != (eax & 0xFFU)) goto loc_4154C4;
    /* 0x0041548F mov dl, [ebx] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(ebx);
    /* 0x00415491 cmp dl, 0FEh */
    /* 0x00415494 jb loc_4154B1 */
    if ((edx & 0xFFU) < 0xFEU) goto loc_4154B1;
    /* 0x00415496 mov dh, dl */
    edx = (edx & 0xFFFF00FFUL) | ((edx & 0xFFU) << 8);
    /* 0x00415498 inc dh */
    edx = (edx & 0xFFFF00FFUL) | (((((edx >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    /* 0x0041549A mov al, dh */
    eax = (eax & 0xFFFFFF00UL) | ((edx >> 8) & 0xFFU);
    /* 0x0041549C mov [ebx], dh */
    M8(ebx) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x0041549E shr al, 1 */
    eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFU) >> 1) & 0xFFU);
    /* 0x004154A0 mov [ebx], al */
    M8(ebx) = (unsigned char)(eax & 0xFFU);
    /* 0x004154A2 mov ah, [ebx+1] */
    eax = (eax & 0xFFFF00FFUL) | (((unsigned int)M8((ebx + 1) & U32MASK)) << 8);
    /* 0x004154A5 inc ah */
    eax = (eax & 0xFFFF00FFUL) | (((((eax >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    /* 0x004154A7 mov dl, ah */
    edx = (edx & 0xFFFFFF00UL) | ((eax >> 8) & 0xFFU);
    /* 0x004154A9 mov [ebx+1], ah */
    M8((ebx + 1) & U32MASK) = (unsigned char)((eax >> 8) & 0xFFU);
    /* 0x004154AC shr dl, 1 */
    edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFU) >> 1) & 0xFFU);
    /* 0x004154AE mov [ebx+1], dl */
    M8((ebx + 1) & U32MASK) = (unsigned char)(edx & 0xFFU);
loc_4154B1:
    /* 0x004154B1 mov dh, [ebx] */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8(ebx)) << 8);
    /* 0x004154B3 inc dh */
    edx = (edx & 0xFFFF00FFUL) | (((((edx >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    /* 0x004154B5 mov al, [ebx+3] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8((ebx + 3) & U32MASK);
    /* 0x004154B8 mov [ebx], dh */
    M8(ebx) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x004154BA cmp al, 7Fh */
    /* 0x004154BC jnb loc_415513 */
    if ((eax & 0xFFU) >= 0x7FU) goto loc_415513;
    /* 0x004154BE mov ah, al */
    eax = (eax & 0xFFFF00FFUL) | ((eax & 0xFFU) << 8);
    /* 0x004154C0 inc ah */
    eax = (eax & 0xFFFF00FFUL) | (((((eax >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    /* 0x004154C2 jmp loc_415510 */
    goto loc_415510;
loc_4154C4:
    /* 0x004154C4 cmp byte ptr [ebx+1], 0FEh */
    /* 0x004154C8 jb loc_4154E5 */
    if ((unsigned int)M8((ebx + 1) & U32MASK) < 0xFEU) goto loc_4154E5;
    /* 0x004154CA mov dl, [ebx] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(ebx);
    /* 0x004154CC mov al, [ebx+1] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8((ebx + 1) & U32MASK);
    /* 0x004154CF inc dl */
    edx = (edx & 0xFFFFFF00UL) | ((((edx & 0xFFU) + 1)) & 0xFFU);
    /* 0x004154D1 inc al */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) + 1)) & 0xFFU);
    /* 0x004154D3 mov [ebx], dl */
    M8(ebx) = (unsigned char)(edx & 0xFFU);
    /* 0x004154D5 mov [ebx+1], al */
    M8((ebx + 1) & U32MASK) = (unsigned char)(eax & 0xFFU);
    /* 0x004154D8 mov dh, dl */
    edx = (edx & 0xFFFF00FFUL) | ((edx & 0xFFU) << 8);
    /* 0x004154DA mov ah, al */
    eax = (eax & 0xFFFF00FFUL) | ((eax & 0xFFU) << 8);
    /* 0x004154DC shr dh, 1 */
    edx = (edx & 0xFFFF00FFUL) | (((((edx >> 8) & 0xFFU) >> 1)) << 8);
    /* 0x004154DE shr ah, 1 */
    eax = (eax & 0xFFFF00FFUL) | (((((eax >> 8) & 0xFFU) >> 1)) << 8);
    /* 0x004154E0 mov [ebx], dh */
    M8(ebx) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x004154E2 mov [ebx+1], ah */
    M8((ebx + 1) & U32MASK) = (unsigned char)((eax >> 8) & 0xFFU);
loc_4154E5:
    /* 0x004154E5 inc byte ptr [ebx+1] */
    M8((ebx + 1) & U32MASK) = (unsigned char)(((unsigned int)M8((ebx + 1) & U32MASK) + 1) & 0xFFU);
    /* 0x004154E8 mov dh, [ebx+3] */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8((ebx + 3) & U32MASK)) << 8);
    /* 0x004154EB cmp dh, 2 */
    /* 0x004154EE jnb loc_4154F9 */
    if (((edx >> 8) & 0xFFU) >= 2U) goto loc_4154F9;
    /* 0x004154F0 mov byte ptr [ebx+3], 0 */
    M8((ebx + 3) & U32MASK) = 0;
    /* 0x004154F4 mov [ebx+2], cl */
    M8((ebx + 2) & U32MASK) = (unsigned char)(ecx & 0xFFU);
    /* 0x004154F7 jmp loc_415500 */
    goto loc_415500;
loc_4154F9:
    /* 0x004154F9 mov al, dh */
    eax = (eax & 0xFFFFFF00UL) | ((edx >> 8) & 0xFFU);
    /* 0x004154FB shr al, 1 */
    eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFU) >> 1) & 0xFFU);
    /* 0x004154FD mov [ebx+3], al */
    M8((ebx + 3) & U32MASK) = (unsigned char)(eax & 0xFFU);
loc_415500:
    /* 0x00415500 add byte ptr [ebx+3], 80h */
    M8((ebx + 3) & U32MASK) = (unsigned char)(((unsigned int)M8((ebx + 3) & U32MASK) + 0x80U) & 0xFFU);
    /* 0x00415504 jmp loc_415513 */
    goto loc_415513;
loc_415506:
    /* 0x00415506 mov byte ptr [ebx], 2 */
    M8(ebx) = 2;
    /* 0x00415509 mov byte ptr [ebx+1], 2 */
    M8((ebx + 1) & U32MASK) = 2;
    /* 0x0041550D mov [ebx+2], cl */
    M8((ebx + 2) & U32MASK) = (unsigned char)(ecx & 0xFFU);
loc_415510:
    /* 0x00415510 mov [ebx+3], ah */
    M8((ebx + 3) & U32MASK) = (unsigned char)((eax >> 8) & 0xFFU);
loc_415513:
    /* 0x00415513 inc ch */
    ecx = (ecx & 0xFFFF00FFUL) | (((((ecx >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    /* 0x00415515 cmp ch, 8 */
    /* 0x00415518 jb loc_4153D6 */
    if (((ecx >> 8) & 0xFFU) < 8U) goto loc_4153D6;
    /* 0x0041551E mov dword_444204, ebp */
    G32(0x444204) = ebp;
    /* 0x00415524 mov dword_44427C, esi */
    G32(0x44427C) = esi;
    /* 0x0041552A pop ebp/edi/esi/edx/ecx/ebx */
    /* 0x00415530 retn */
}
/* sub_415534: entry al = symbol. Order-model update; calls sub_40C9C0. */
void d3u_415534(dec3w_ctx *c, unsigned int eax_v)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int ebp = 0;
    unsigned int adr = 0;
    /* 0x00415534 push ebx/ecx/edx/esi/edi/ebp */
    /* 0x0041553A mov cl, byte_444EB7 */
    /* ecx high = entry garbage; only cl/ch lanes observed below */
    ecx = (unsigned int)G8(0x444EB7);
    /* 0x00415540 mov ch, al */
    ecx = (ecx & 0xFFFF00FFUL) | ((eax_v & 0xFFU) << 8);
    /* 0x00415542 mov edx, dword_444224 */
    edx = G32(0x444224);
    /* 0x00415548 cmp edx, 40000h */
    /* 0x0041554E jnb loc_415762 */
    if (edx >= 0x40000UL) goto loc_415762;
    /* 0x00415554 mov ebx, edx */
    ebx = edx;
    /* 0x00415556 mov eax, dword_4299BC */
    eax = G32(0x4299BC);
    /* 0x0041555B shl ebx, 5 */
    ebx = (ebx << 5) & U32MASK;
    /* 0x0041555E add ebx, eax */
    ebx = (ebx + eax) & U32MASK;
    /* 0x00415560 cmp cl, 0FFh */
    /* 0x00415563 jnz loc_415578 */
    if ((ecx & 0xFFU) != 0xFFU) goto loc_415578;
    /* 0x00415565 xor cl, cl */
    ecx &= 0xFFFFFF00UL;
loc_415567:
    /* 0x00415567 xor eax, eax */
    eax = 0;
    /* 0x00415569 mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x0041556B cmp ch, [ebx+eax+0Eh] */
    /* 0x0041556F jz loc_415578 */
    if (((ecx >> 8) & 0xFFU) == (unsigned int)M8((ebx + eax + 0x0E) & U32MASK)) goto loc_415578;
    /* 0x00415571 inc cl */
    ecx = (ecx & 0xFFFFFF00UL) | ((((ecx & 0xFFU) + 1)) & 0xFFU);
    /* 0x00415573 cmp cl, 0Dh */
    /* 0x00415576 jb loc_415567 */
    if ((ecx & 0xFFU) < 0x0DU) goto loc_415567;
loc_415578:
    /* 0x00415578 cmp byte_444EB0, 0 */
    /* 0x0041557F jz loc_4155A8 */
    if (G8(0x444EB0) == 0) goto loc_4155A8;
    /* 0x00415581 cmp cl, 0Dh */
    /* 0x00415584 jnb loc_4155A8 */
    if ((ecx & 0xFFU) >= 0x0DU) goto loc_4155A8;
    /* 0x00415586 xor eax, eax */
    eax = 0;
    /* 0x00415588 mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x0041558A mov al, [ebx+eax+1] */
    eax = (unsigned int)M8((ebx + eax + 1) & U32MASK);
    /* 0x0041558E mov esi, eax */
    esi = eax;
    /* 0x00415590 and esi, 0FFh */
    esi &= 0xFFU;
    /* 0x00415596 xor eax, eax */
    eax = 0;
    /* 0x00415598 sar esi, 6 */
    esi = (unsigned int)(((int)esi) >> 6);
    /* 0x0041559B mov al, [ebx] */
    eax = (unsigned int)M8(ebx);
    /* 0x0041559D cmp esi, eax */
    /* 0x0041559F jle loc_4155A8 */
    if ((int)esi <= (int)eax) goto loc_4155A8;
    /* 0x004155A1 add dword_444264, 4 */
    G32(0x444264) = (G32(0x444264) + 4) & U32MASK;
loc_4155A8:
    /* 0x004155A8 cmp byte_444EB1, 0 */
    /* 0x004155AF jz loc_415605 */
    if (G8(0x444EB1) == 0) goto loc_415605;
    /* 0x004155B1 cmp cl, 0Dh */
    /* 0x004155B4 jnb loc_4155D9 */
    if ((ecx & 0xFFU) >= 0x0DU) goto loc_4155D9;
    /* 0x004155B6 xor dl, dl */
    edx &= 0xFFFFFF00UL;
    /* 0x004155B8 xor eax, eax */
    eax = 0;
    /* 0x004155BA mov byte_444EB5, dl */
    G8(0x444EB5) = 0;
    /* 0x004155C0 mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x004155C2 mov byte_444EB2, dl */
    G8(0x444EB2) = 0;
    /* 0x004155C8 mov byte_444EAE, dl */
    G8(0x444EAE) = 0;
    /* 0x004155CE mov al, [ebx+eax+1] */
    eax = (unsigned int)M8((ebx + eax + 1) & U32MASK);
    /* 0x004155D2 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x004155D7 jmp loc_4155DD */
    goto loc_4155DD;
loc_4155D9:
    /* 0x004155D9 xor eax, eax */
    eax = 0;
    /* 0x004155DB mov al, [ebx] */
    eax = (unsigned int)M8(ebx);
loc_4155DD:
    /* 0x004155DD mov edi, dword_444234 */
    edi = G32(0x444234);
    /* 0x004155E3 shl eax, 0Eh */
    eax = (eax << 14) & U32MASK;
    /* 0x004155E6 xor edx, edx */
    edx = 0;
    /* 0x004155E8 div edi */
    /* x86 div faults on div0; direct / % below; remainder discarded */
    eax = eax / edi;
    /* 0x004155EA mov ax, word_42D274[eax*2] */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16((0x42D274U + (eax << 1)) & U32MASK);
    /* 0x004155F2 mov ebp, dword_444204 */
    ebp = G32(0x444204);
    /* 0x004155F8 and eax, 0FFFFh */
    eax &= 0xFFFFU;
    /* 0x004155FD add ebp, eax */
    ebp = (ebp + eax) & U32MASK;
    /* 0x004155FF mov dword_444204, ebp */
    G32(0x444204) = ebp;
loc_415605:
    /* 0x00415605 xor eax, eax */
    eax = 0;
    /* 0x00415607 mov al, [ebx] */
    eax = (unsigned int)M8(ebx);
    /* 0x00415609 mov al, byte_429BB8[eax] */
    eax = (unsigned int)G8((0x429BB8U + (eax & 0xFFU)) & U32MASK);
    /* 0x0041560F mov edx, dword_444214 */
    edx = G32(0x444214);
    /* 0x00415615 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x0041561A cmp eax, edx */
    /* 0x0041561C setb al */
    eax = (eax & 0xFFFFFF00UL) | ((eax < edx) ? 1U : 0U);
    /* 0x0041561F mov ah, al */
    eax = (eax & 0xFFFF00FFUL) | ((eax & 0xFFU) << 8);
    /* 0x00415621 cmp cl, 0Dh */
    /* 0x00415624 jnb loc_415660 */
    if ((ecx & 0xFFU) >= 0x0DU) goto loc_415660;
    /* 0x00415626 movzx esi, cl */
    esi = ecx & 0xFFU;
    /* 0x00415629 mov al, [ebx+1Bh] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8((ebx + 0x1B) & U32MASK);
    /* 0x0041562C movzx edi, byte ptr [esi+ebx+1] */
    edi = (unsigned int)M8((esi + ebx + 1) & U32MASK);
    /* 0x00415631 movzx esi, byte ptr [ebx+1] */
    esi = (unsigned int)M8((ebx + 1) & U32MASK);
    /* 0x00415635 inc edi */
    edi = (edi + 1) & U32MASK;
    /* 0x00415636 sar esi, 1 */
    esi = (unsigned int)(((int)esi) >> 1);
    /* 0x00415638 and al, 0C0h */
    eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFU) & 0xC0U);
    /* 0x0041563A cmp edi, esi */
    /* 0x0041563C jge loc_415646 */
    if ((int)edi >= (int)esi) goto loc_415646;
    /* 0x0041563E add al, 40h */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) + 0x40U)) & 0xFFU);
    /* 0x00415640 jnz loc_415655 */
    if ((eax & 0xFFU) != 0) goto loc_415655;
    /* 0x00415642 mov ah, 1 */
    eax = (eax & 0xFFFF00FFUL) | (1U << 8);
    /* 0x00415644 jmp loc_415655 */
    goto loc_415655;
loc_415646:
    /* 0x00415646 movzx esi, al */
    esi = eax & 0xFFU;
    /* 0x00415649 mov edx, esi */
    edx = esi;
    /* 0x0041564B sar edx, 1 */
    edx = (unsigned int)(((int)edx) >> 1);
    /* 0x0041564D and edx, 0C0h */
    edx &= 0xC0U;
    /* 0x00415653 mov al, dl */
    eax = (eax & 0xFFFFFF00UL) | (edx & 0xFFU);
loc_415655:
    /* 0x00415655 mov dl, [ebx+1Bh] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8((ebx + 0x1B) & U32MASK);
    /* 0x00415658 and dl, 3Fh */
    edx = (edx & 0xFFFFFF00UL) | ((edx & 0xFFU) & 0x3FU);
    /* 0x0041565B or dl, al */
    edx = (edx & 0xFFFFFF00UL) | ((((edx & 0xFFU) | (eax & 0xFFU))) & 0xFFU);
    /* 0x0041565D mov [ebx+1Bh], dl */
    M8((ebx + 0x1B) & U32MASK) = (unsigned char)(edx & 0xFFU);
loc_415660:
    /* 0x00415660 test ah, ah */
    /* 0x00415662 jz loc_41567C */
    if (((eax >> 8) & 0xFFU) == 0) goto loc_41567C;
    /* 0x00415664 lea eax, [ebx+0Dh] */
    eax = (ebx + 0x0D) & U32MASK;
    /* 0x00415667 lea esi, [ebx-1] */
    esi = (ebx - 1) & U32MASK;
loc_41566A:
    /* 0x0041566A mov dl, [eax] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(eax);
    /* 0x0041566C inc dl */
    edx = (edx & 0xFFFFFF00UL) | ((((edx & 0xFFU) + 1)) & 0xFFU);
    /* 0x0041566E mov [eax], dl */
    M8(eax) = (unsigned char)(edx & 0xFFU);
    /* 0x00415670 mov dh, dl */
    edx = (edx & 0xFFFF00FFUL) | ((edx & 0xFFU) << 8);
    /* 0x00415672 dec eax */
    eax = (eax - 1) & U32MASK;
    /* 0x00415673 shr dh, 1 */
    edx = (edx & 0xFFFF00FFUL) | (((((edx >> 8) & 0xFFU) >> 1)) << 8);
    /* 0x00415675 mov [eax+1], dh */
    M8((eax + 1) & U32MASK) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x00415678 cmp eax, esi */
    /* 0x0041567A jnz loc_41566A */
    if (eax != esi) goto loc_41566A;
loc_41567C:
    /* 0x0041567C cmp cl, 0Dh */
    /* 0x0041567F jnb loc_41569F */
    if ((ecx & 0xFFU) >= 0x0DU) goto loc_41569F;
    /* 0x00415681 xor eax, eax */
    eax = 0;
    /* 0x00415683 mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x00415685 add byte ptr [ebx+eax+1], 2 */
    adr = (ebx + eax + 1) & U32MASK;
    M8(adr) = (unsigned char)(((unsigned int)M8(adr) + 2) & 0xFFU);
    /* 0x0041568A mov al, [ebx+1Bh] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8((ebx + 0x1B) & U32MASK);
    /* 0x0041568D and al, 3Fh */
    eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFU) & 0x3FU);
    /* 0x0041568F cmp al, 3Fh */
    /* 0x00415691 jnb loc_415696 */
    if ((eax & 0xFFU) >= 0x3FU) goto loc_415696;
    /* 0x00415693 inc byte ptr [ebx+1Bh] */
    M8((ebx + 0x1B) & U32MASK) = (unsigned char)(((unsigned int)M8((ebx + 0x1B) & U32MASK) + 1) & 0xFFU);
loc_415696:
    /* 0x00415696 add dword_444220, 5 */
    G32(0x444220) = (G32(0x444220) + 5) & U32MASK;
    /* 0x0041569D jmp loc_4156E5 */
    goto loc_4156E5;
loc_41569F:
    /* 0x0041569F mov al, [ebx] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8(ebx);
    /* 0x004156A1 add al, 2 */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) + 2)) & 0xFFU);
    /* 0x004156A3 xor cl, cl */
    ecx &= 0xFFFFFF00UL;
    /* 0x004156A5 mov [ebx], al */
    M8(ebx) = (unsigned char)(eax & 0xFFU);
loc_4156A7:
    /* 0x004156A7 xor eax, eax */
    eax = 0;
    /* 0x004156A9 mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x004156AB cmp byte ptr [ebx+eax+1], 0 */
    /* 0x004156B0 jz loc_4156B9 */
    if (M8((ebx + eax + 1) & U32MASK) == 0) goto loc_4156B9;
    /* 0x004156B2 inc cl */
    ecx = (ecx & 0xFFFFFF00UL) | ((((ecx & 0xFFU) + 1)) & 0xFFU);
    /* 0x004156B4 cmp cl, 0Ch */
    /* 0x004156B7 jb loc_4156A7 */
    if ((ecx & 0xFFU) < 0x0CU) goto loc_4156A7;
loc_4156B9:
    /* 0x004156B9 xor eax, eax */
    eax = 0;
    /* 0x004156BB mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x004156BD add eax, ebx */
    eax = (eax + ebx) & U32MASK;
    /* 0x004156BF cmp byte ptr [eax+1], 6 */
    /* 0x004156C3 jnb loc_4156CC */
    if ((unsigned int)M8((eax + 1) & U32MASK) >= 6U) goto loc_4156CC;
    /* 0x004156C5 mov byte ptr [eax+1], 2 */
    M8((eax + 1) & U32MASK) = 2;
    /* 0x004156C9 mov [eax+0Eh], ch */
    M8((eax + 0x0E) & U32MASK) = (unsigned char)((ecx >> 8) & 0xFFU);
loc_4156CC:
    /* 0x004156CC mov al, [ebx+1Bh] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8((ebx + 0x1B) & U32MASK);
    /* 0x004156CF and al, 3Fh */
    eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFU) & 0x3FU);
    /* 0x004156D1 movzx esi, al */
    esi = eax & 0xFFU;
    /* 0x004156D4 mov al, [ebx+1Bh] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8((ebx + 0x1B) & U32MASK);
    /* 0x004156D7 sar esi, 1 */
    esi = (unsigned int)(((int)esi) >> 1);
    /* 0x004156D9 and al, 0C0h */
    eax = (eax & 0xFFFFFF00UL) | ((eax & 0xFFU) & 0xC0U);
    /* 0x004156DB and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x004156E0 add eax, esi */
    eax = (eax + esi) & U32MASK;
    /* 0x004156E2 mov [ebx+1Bh], al */
    M8((ebx + 0x1B) & U32MASK) = (unsigned char)(eax & 0xFFU);
loc_4156E5:
    /* 0x004156E5 test cl, cl */
    /* 0x004156E7 jz loc_41570F */
    if ((ecx & 0xFFU) == 0) goto loc_41570F;
    /* 0x004156E9 xor eax, eax */
    eax = 0;
    /* 0x004156EB mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x004156ED add eax, ebx */
    eax = (eax + ebx) & U32MASK;
    /* 0x004156EF mov dl, [eax+1] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8((eax + 1) & U32MASK);
    /* 0x004156F2 mov ch, [eax] */
    ecx = (ecx & 0xFFFF00FFUL) | (((unsigned int)M8(eax)) << 8);
    /* 0x004156F4 cmp dl, ch */
    /* 0x004156F6 jb loc_41570F */
    if ((edx & 0xFFU) < ((ecx >> 8) & 0xFFU)) goto loc_41570F;
    /* 0x004156F8 mov dh, ch */
    edx = (edx & 0xFFFF00FFUL) | (((ecx >> 8) & 0xFFU) << 8);
    /* 0x004156FA mov [eax], dl */
    M8(eax) = (unsigned char)(edx & 0xFFU);
    /* 0x004156FC mov dl, [eax+0Eh] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8((eax + 0x0E) & U32MASK);
    /* 0x004156FF mov ch, [eax+0Dh] */
    ecx = (ecx & 0xFFFF00FFUL) | (((unsigned int)M8((eax + 0x0D) & U32MASK)) << 8);
    /* 0x00415702 mov [eax+0Dh], dl */
    M8((eax + 0x0D) & U32MASK) = (unsigned char)(edx & 0xFFU);
    /* 0x00415705 mov [eax+1], dh */
    M8((eax + 1) & U32MASK) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x00415708 mov [eax+0Eh], ch */
    M8((eax + 0x0E) & U32MASK) = (unsigned char)((ecx >> 8) & 0xFFU);
    /* 0x0041570B dec cl */
    ecx = (ecx & 0xFFFFFF00UL) | ((((ecx & 0xFFU) - 1)) & 0xFFU);
    /* 0x0041570D jmp loc_4156E5 */
    goto loc_4156E5;
loc_41570F:
    /* 0x0041570F mov edi, dword_444250 */
    edi = G32(0x444250);
    /* 0x00415715 add edi, 2 */
    edi = (edi + 2) & U32MASK;
    /* 0x00415718 mov byte_444EB7, cl */
    G8(0x444EB7) = (unsigned char)(ecx & 0xFFU);
    /* 0x0041571E mov dword_444250, edi */
    G32(0x444250) = edi;
    /* 0x00415724 cmp edi, 800h */
    /* 0x0041572A jbe loc_4157F0 */
    if (edi <= 0x800U) goto loc_4157F0;
    /* 0x00415730 mov edx, dword_444220 */
    edx = G32(0x444220);
    /* 0x00415736 mov eax, edi */
    eax = edi;
    /* 0x00415738 mov ebx, dword_444264 */
    ebx = G32(0x444264);
    /* 0x0041573E shr eax, 1 */
    eax >>= 1;
    /* 0x00415740 shr edx, 1 */
    edx >>= 1;
    /* 0x00415742 shr ebx, 1 */
    ebx >>= 1;
    /* 0x00415744 mov dword_444250, eax */
    G32(0x444250) = eax;
    /* 0x00415749 mov dword_444220, edx */
    G32(0x444220) = edx;
    /* 0x0041574F mov dword_444264, ebx */
    G32(0x444264) = ebx;
    /* 0x00415755 mov cl, byte_444EB7 */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)G8(0x444EB7);
    /* 0x0041575B pop ebp/edi/esi/edx/ecx/ebx */
    /* 0x00415761 retn (early) */
    return;
loc_415762:
    /* 0x00415762 lea ebx, [edx-40000h] */
    ebx = (edx - 0x40000UL) & U32MASK;
    /* 0x00415768 mov esi, ebx */
    esi = ebx;
    /* 0x0041576A mov eax, dword_4299BC */
    eax = G32(0x4299BC);
    /* 0x0041576F shl esi, 5 */
    esi = (esi << 5) & U32MASK;
    /* 0x00415772 add esi, eax */
    esi = (esi + eax) & U32MASK;
    /* 0x00415774 xor ah, ah */
    eax &= 0xFFFF00FFUL;
    /* 0x00415776 mov dword_444224, ebx */
    G32(0x444224) = ebx;
    /* 0x0041577C xor al, al */
    eax &= 0xFFFFFF00UL;
    /* 0x0041577E mov dh, 3Fh */
    edx = (edx & 0xFFFF00FFUL) | (0x3FU << 8);
loc_415780:
    /* 0x00415780 xor ebx, ebx */
    ebx = 0;
    /* 0x00415782 mov bl, al */
    ebx = eax & 0xFFU;
    /* 0x00415784 shl ebx, 14h */
    ebx = (ebx << 20) & U32MASK;
    /* 0x00415787 add ebx, esi */
    ebx = (ebx + esi) & U32MASK;
    /* 0x00415789 mov dl, [ebx] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(ebx);
    /* 0x0041578B mov cl, byte_444EB7 */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)G8(0x444EB7);
    /* 0x00415791 test dl, dl */
    /* 0x00415793 jnz loc_415799 */
    if ((edx & 0xFFU) != 0) goto loc_415799;
    /* 0x00415795 mov ah, al */
    eax = (eax & 0xFFFF00FFUL) | ((eax & 0xFFU) << 8);
    /* 0x00415797 jmp loc_4157C4 */
    goto loc_4157C4;
loc_415799:
    /* 0x00415799 mov dl, [ebx+1Bh] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8((ebx + 0x1B) & U32MASK);
    /* 0x0041579C xor ebx, ebx */
    ebx = 0;
    /* 0x0041579E mov bl, ah */
    ebx = (eax >> 8) & 0xFFU;
    /* 0x004157A0 and dl, dh */
    edx = (edx & 0xFFFFFF00UL) | ((((edx & 0xFFU) & ((edx >> 8) & 0xFFU))) & 0xFFU);
    /* 0x004157A2 shl ebx, 14h */
    ebx = (ebx << 20) & U32MASK;
    /* 0x004157A5 movzx edi, dl */
    edi = edx & 0xFFU;
    /* 0x004157A8 mov dl, [esi+ebx+1Bh] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8((esi + ebx + 0x1B) & U32MASK);
    /* 0x004157AC and dl, dh */
    edx = (edx & 0xFFFFFF00UL) | ((((edx & 0xFFU) & ((edx >> 8) & 0xFFU))) & 0xFFU);
    /* 0x004157AE xor ebx, ebx */
    ebx = 0;
    /* 0x004157B0 mov bl, dl */
    ebx = edx & 0xFFU;
    /* 0x004157B2 cmp edi, ebx */
    /* 0x004157B4 jge loc_4157B8 */
    if ((int)edi >= (int)ebx) goto loc_4157B8;
    /* 0x004157B6 mov ah, al */
    eax = (eax & 0xFFFF00FFUL) | ((eax & 0xFFU) << 8);
loc_4157B8:
    /* 0x004157B8 inc al */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) + 1)) & 0xFFU);
    /* 0x004157BA mov byte_444EB7, cl */
    G8(0x444EB7) = (unsigned char)(ecx & 0xFFU);
    /* 0x004157C0 cmp al, 8 */
    /* 0x004157C2 jb loc_415780 */
    if ((eax & 0xFFU) < 8U) goto loc_415780;
loc_4157C4:
    /* 0x004157C4 mov al, ah */
    eax = (eax & 0xFFFFFF00UL) | ((eax >> 8) & 0xFFU);
    /* 0x004157C6 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x004157CB shl eax, 14h */
    eax = (eax << 20) & U32MASK;
    /* 0x004157CE mov ebx, 20h */
    ebx = 0x20U;
    /* 0x004157D3 add esi, eax */
    esi = (esi + eax) & U32MASK;
    /* 0x004157D5 xor edx, edx */
    edx = 0;
    /* 0x004157D7 mov eax, esi */
    eax = esi;
    /* 0x004157D9 call sub_40C9C0 */
    d3u_fill(c, ebx, eax, edx);
    /* 0x004157DE mov byte ptr [esi], 1 */
    M8(esi) = 1;
    /* 0x004157E1 mov byte ptr [esi+1], 1 */
    M8((esi + 1) & U32MASK) = 1;
    /* 0x004157E5 mov eax, dword_444240 */
    eax = G32(0x444240);
    /* 0x004157EA mov [esi+0Eh], ch */
    M8((esi + 0x0E) & U32MASK) = (unsigned char)((ecx >> 8) & 0xFFU);
    /* 0x004157ED mov [esi+1Ch], eax */
    M32((esi + 0x1C) & U32MASK) = eax;
loc_4157F0:
    /* 0x004157F0 mov cl, byte_444EB7 */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)G8(0x444EB7);
    /* 0x004157F6 pop ebp/edi/esi/edx/ecx/ebx */
    /* 0x004157FC retn */
    ;
}
/* sub_415800: entry al = symbol. Order-model update; calls sub_40C9C0. */
void d3u_415800(dec3w_ctx *c, unsigned int eax_v)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int ebp = 0;
    unsigned int adr = 0;
    /* 0x00415800 push ebx/ecx/edx/esi/edi/ebp */
    /* 0x00415806 mov cl, byte_444EB8 */
    /* ecx high = entry garbage; only cl/ch lanes observed below */
    ecx = (unsigned int)G8(0x444EB8);
    /* 0x0041580C mov ch, al */
    ecx = (ecx & 0xFFFF00FFUL) | ((eax_v & 0xFFU) << 8);
    /* 0x0041580E mov ebx, dword_444238 */
    ebx = G32(0x444238);
    /* 0x00415814 lea eax, [ebx*4] */
    eax = (ebx << 2) & U32MASK;
    /* 0x0041581B add eax, ebx */
    eax = (eax + ebx) & U32MASK;
    /* 0x0041581D mov esi, dword_4299C0 */
    esi = G32(0x4299C0);
    /* 0x00415823 shl eax, 2 */
    eax = (eax << 2) & U32MASK;
    /* 0x00415826 add esi, eax */
    esi = (esi + eax) & U32MASK;
    /* 0x00415828 cmp byte ptr [esi], 0 */
    /* 0x0041582B jz loc_4159CA */
    if (M8(esi) == 0) goto loc_4159CA;
    /* 0x00415831 cmp cl, 0FFh */
    /* 0x00415834 jnz loc_415849 */
    if ((ecx & 0xFFU) != 0xFFU) goto loc_415849;
    /* 0x00415836 xor cl, cl */
    ecx &= 0xFFFFFF00UL;
loc_415838:
    /* 0x00415838 xor eax, eax */
    eax = 0;
    /* 0x0041583A mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x0041583C cmp ch, [esi+eax+9] */
    /* 0x00415840 jz loc_415849 */
    if (((ecx >> 8) & 0xFFU) == (unsigned int)M8((esi + eax + 9) & U32MASK)) goto loc_415849;
    /* 0x00415842 inc cl */
    ecx = (ecx & 0xFFFFFF00UL) | ((((ecx & 0xFFU) + 1)) & 0xFFU);
    /* 0x00415844 cmp cl, 8 */
    /* 0x00415847 jb loc_415838 */
    if ((ecx & 0xFFU) < 8U) goto loc_415838;
loc_415849:
    /* 0x00415849 cmp byte_444EB5, 0 */
    /* 0x00415850 jz loc_4158A0 */
    if (G8(0x444EB5) == 0) goto loc_4158A0;
    /* 0x00415852 cmp cl, 8 */
    /* 0x00415855 jnb loc_415874 */
    if ((ecx & 0xFFU) >= 8U) goto loc_415874;
    /* 0x00415857 xor eax, eax */
    eax = 0;
    /* 0x00415859 xor bl, bl */
    ebx &= 0xFFFFFF00UL;
    /* 0x0041585B mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x0041585D mov byte_444EB2, bl */
    G8(0x444EB2) = (unsigned char)(ebx & 0xFFU);
    /* 0x00415863 mov byte_444EAE, bl */
    G8(0x444EAE) = (unsigned char)(ebx & 0xFFU);
    /* 0x00415869 mov al, [esi+eax+1] */
    eax = (unsigned int)M8((esi + eax + 1) & U32MASK);
    /* 0x0041586D and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415872 jmp loc_415878 */
    goto loc_415878;
loc_415874:
    /* 0x00415874 xor eax, eax */
    eax = 0;
    /* 0x00415876 mov al, [esi] */
    eax = (unsigned int)M8(esi);
loc_415878:
    /* 0x00415878 mov ebx, dword_444248 */
    ebx = G32(0x444248);
    /* 0x0041587E shl eax, 0Eh */
    eax = (eax << 14) & U32MASK;
    /* 0x00415881 xor edx, edx */
    edx = 0;
    /* 0x00415883 div ebx */
    /* x86 div faults on div0; direct / % below; remainder discarded */
    eax = eax / ebx;
    /* 0x00415885 mov ax, word_42D274[eax*2] */
    eax = (eax & 0xFFFF0000UL) | (unsigned int)M16((0x42D274U + (eax << 1)) & U32MASK);
    /* 0x0041588D mov edi, dword_444204 */
    edi = G32(0x444204);
    /* 0x00415893 and eax, 0FFFFh */
    eax &= 0xFFFFU;
    /* 0x00415898 add edi, eax */
    edi = (edi + eax) & U32MASK;
    /* 0x0041589A mov dword_444204, edi */
    G32(0x444204) = edi;
loc_4158A0:
    /* 0x004158A0 xor ebx, ebx */
    ebx = 0;
    /* 0x004158A2 mov bl, [esi] */
    ebx = (unsigned int)M8(esi);
    /* 0x004158A4 mov al, [esi+12h] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8((esi + 0x12) & U32MASK);
    /* 0x004158A7 cmp al, byte_429CB8[ebx] */
    /* 0x004158AD jbe loc_4158D4 */
    if ((eax & 0xFFU) <= (unsigned int)G8((0x429CB8U + ebx) & U32MASK)) goto loc_4158D4;
    /* 0x004158AF lea eax, [esi+8] */
    eax = (esi + 8) & U32MASK;
    /* 0x004158B2 mov byte ptr [esi+12h], 0 */
    M8((esi + 0x12) & U32MASK) = 0;
    /* 0x004158B6 lea ebx, [esi-1] */
    ebx = (esi - 1) & U32MASK;
loc_4158B9:
    /* 0x004158B9 mov dl, [eax] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8(eax);
    /* 0x004158BB inc dl */
    edx = (edx & 0xFFFFFF00UL) | ((((edx & 0xFFU) + 1)) & 0xFFU);
    /* 0x004158BD mov [eax], dl */
    M8(eax) = (unsigned char)(edx & 0xFFU);
    /* 0x004158BF mov dh, dl */
    edx = (edx & 0xFFFF00FFUL) | ((edx & 0xFFU) << 8);
    /* 0x004158C1 shr dh, 1 */
    edx = (edx & 0xFFFF00FFUL) | (((((edx >> 8) & 0xFFU) >> 1)) << 8);
    /* 0x004158C3 mov [eax], dh */
    M8(eax) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x004158C5 mov dl, dh */
    edx = (edx & 0xFFFFFF00UL) | ((edx >> 8) & 0xFFU);
    /* 0x004158C7 mov dh, [esi+12h] */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)M8((esi + 0x12) & U32MASK)) << 8);
    /* 0x004158CA add dh, dl */
    edx = (edx & 0xFFFF00FFUL) | ((((((edx >> 8) & 0xFFU) + (edx & 0xFFU)) & 0xFFU)) << 8); /* 0x004158CA add dh,dl */
    /* 0x004158CC dec eax */
    eax = (eax - 1) & U32MASK;
    /* 0x004158CD mov [esi+12h], dh */
    M8((esi + 0x12) & U32MASK) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x004158D0 cmp eax, ebx */
    /* 0x004158D2 jnz loc_4158B9 */
    if (eax != ebx) goto loc_4158B9;
loc_4158D4:
    /* 0x004158D4 add byte ptr [esi+12h], 4 */
    M8((esi + 0x12) & U32MASK) = (unsigned char)(((unsigned int)M8((esi + 0x12) & U32MASK) + 4) & 0xFFU);
    /* 0x004158D8 cmp cl, 8 */
    /* 0x004158DB jnb loc_415910 */
    if ((ecx & 0xFFU) >= 8U) goto loc_415910;
    /* 0x004158DD mov ebx, dword_44424C */
    ebx = G32(0x44424C);
    /* 0x004158E3 lea eax, [ebx*4] */
    eax = (ebx << 2) & U32MASK;
    /* 0x004158EA sub eax, ebx */
    eax = (eax - ebx) & U32MASK;
    /* 0x004158EC shr eax, 2 */
    eax >>= 2;
    /* 0x004158EF mov dword_44424C, eax */
    G32(0x44424C) = eax;
    /* 0x004158F4 xor eax, eax */
    eax = 0;
    /* 0x004158F6 mov al, [esi+11h] */
    eax = (unsigned int)M8((esi + 0x11) & U32MASK);
    /* 0x004158F9 sar eax, 1 */
    eax = (unsigned int)(((int)eax) >> 1);
    /* 0x004158FB mov [esi+11h], al */
    M8((esi + 0x11) & U32MASK) = (unsigned char)(eax & 0xFFU);
    /* 0x004158FE xor eax, eax */
    eax = 0;
    /* 0x00415900 mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x00415902 add byte ptr [esi+eax+1], 4 */
    adr = (esi + eax + 1) & U32MASK;
    M8(adr) = (unsigned char)(((unsigned int)M8(adr) + 4) & 0xFFU);
    /* 0x00415907 add dword_444220, 5 */
    G32(0x444220) = (G32(0x444220) + 5) & U32MASK;
    /* 0x0041590E jmp loc_41595B */
    goto loc_41595B;
loc_415910:
    /* 0x00415910 mov ebp, dword_44424C */
    ebp = G32(0x44424C);
    /* 0x00415916 mov bh, [esi+11h] */
    ebx = (ebx & 0xFFFF00FFUL) | (((unsigned int)M8((esi + 0x11) & U32MASK)) << 8);
    /* 0x00415919 mov al, [esi] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)M8(esi);
    /* 0x0041591B xor cl, cl */
    ecx &= 0xFFFFFF00UL;
    /* 0x0041591D inc ebp */
    ebp = (ebp + 1) & U32MASK;
    /* 0x0041591E inc bh */
    ebx = (ebx & 0xFFFF00FFUL) | (((((ebx >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    /* 0x00415920 add al, 4 */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) + 4)) & 0xFFU);
    /* 0x00415922 mov [esi+11h], bh */
    M8((esi + 0x11) & U32MASK) = (unsigned char)((ebx >> 8) & 0xFFU);
    /* 0x00415925 mov dword_44424C, ebp */
    G32(0x44424C) = ebp;
    /* 0x0041592B mov [esi], al */
    M8(esi) = (unsigned char)(eax & 0xFFU);
loc_41592D:
    /* 0x0041592D xor eax, eax */
    eax = 0;
    /* 0x0041592F mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x00415931 cmp byte ptr [esi+eax+1], 0 */
    /* 0x00415936 jz loc_41593F */
    if (M8((esi + eax + 1) & U32MASK) == 0) goto loc_41593F;
    /* 0x00415938 inc cl */
    ecx = (ecx & 0xFFFFFF00UL) | ((((ecx & 0xFFU) + 1)) & 0xFFU);
    /* 0x0041593A cmp cl, 7 */
    /* 0x0041593D jb loc_41592D */
    if ((ecx & 0xFFU) < 7U) goto loc_41592D;
loc_41593F:
    /* 0x0041593F xor eax, eax */
    eax = 0;
    /* 0x00415941 mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x00415943 add eax, esi */
    eax = (eax + esi) & U32MASK;
    /* 0x00415945 mov bl, [eax+1] */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)M8((eax + 1) & U32MASK);
    /* 0x00415948 cmp bl, 0Ch */
    /* 0x0041594B jnb loc_41595B */
    if ((ebx & 0xFFU) >= 0x0CU) goto loc_41595B;
    /* 0x0041594D mov dl, 4 */
    edx = (edx & 0xFFFFFF00UL) | 4U;
    /* 0x0041594F sub dl, bl */
    edx = (edx & 0xFFFFFF00UL) | ((((edx & 0xFFU) - (ebx & 0xFFU))) & 0xFFU);
    /* 0x00415951 add [esi+12h], dl */
    M8((esi + 0x12) & U32MASK) = (unsigned char)(((unsigned int)M8((esi + 0x12) & U32MASK) + (edx & 0xFFU)) & 0xFFU);
    /* 0x00415954 mov byte ptr [eax+1], 4 */
    M8((eax + 1) & U32MASK) = 4;
    /* 0x00415958 mov [eax+9], ch */
    M8((eax + 9) & U32MASK) = (unsigned char)((ecx >> 8) & 0xFFU);
loc_41595B:
    /* 0x0041595B test cl, cl */
    /* 0x0041595D jz loc_415985 */
    if ((ecx & 0xFFU) == 0) goto loc_415985;
    /* 0x0041595F xor eax, eax */
    eax = 0;
    /* 0x00415961 mov al, cl */
    eax = ecx & 0xFFU;
    /* 0x00415963 add eax, esi */
    eax = (eax + esi) & U32MASK;
    /* 0x00415965 mov dl, [eax+1] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8((eax + 1) & U32MASK);
    /* 0x00415968 mov bl, [eax] */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)M8(eax);
    /* 0x0041596A cmp dl, bl */
    /* 0x0041596C jb loc_415985 */
    if ((edx & 0xFFU) < (ebx & 0xFFU)) goto loc_415985;
    /* 0x0041596E mov dh, bl */
    edx = (edx & 0xFFFF00FFUL) | ((ebx & 0xFFU) << 8);
    /* 0x00415970 mov [eax], dl */
    M8(eax) = (unsigned char)(edx & 0xFFU);
    /* 0x00415972 mov dl, [eax+9] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)M8((eax + 9) & U32MASK);
    /* 0x00415975 mov bl, [eax+8] */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)M8((eax + 8) & U32MASK);
    /* 0x00415978 mov [eax+8], dl */
    M8((eax + 8) & U32MASK) = (unsigned char)(edx & 0xFFU);
    /* 0x0041597B mov [eax+1], dh */
    M8((eax + 1) & U32MASK) = (unsigned char)((edx >> 8) & 0xFFU);
    /* 0x0041597E dec cl */
    ecx = (ecx & 0xFFFFFF00UL) | ((((ecx & 0xFFU) - 1)) & 0xFFU);
    /* 0x00415980 mov [eax+9], bl */
    M8((eax + 9) & U32MASK) = (unsigned char)(ebx & 0xFFU);
    /* 0x00415983 jmp loc_41595B */
    goto loc_41595B;
loc_415985:
    /* 0x00415985 mov edx, dword_444250 */
    edx = G32(0x444250);
    /* 0x0041598B add edx, 2 */
    edx = (edx + 2) & U32MASK;
    /* 0x0041598E mov byte_444EB8, cl */
    G8(0x444EB8) = (unsigned char)(ecx & 0xFFU);
    /* 0x00415994 mov dword_444250, edx */
    G32(0x444250) = edx;
    /* 0x0041599A cmp edx, 800h */
    /* 0x004159A0 jbe loc_4159E6 */
    if (edx <= 0x800U) goto loc_4159E6;
    /* 0x004159A2 mov edi, dword_444220 */
    edi = G32(0x444220);
    /* 0x004159A8 mov esi, edx */
    esi = edx;
    /* 0x004159AA mov ebp, dword_444264 */
    ebp = G32(0x444264);
    /* 0x004159B0 shr esi, 1 */
    esi >>= 1;
    /* 0x004159B2 shr edi, 1 */
    edi >>= 1;
    /* 0x004159B4 shr ebp, 1 */
    ebp >>= 1;
    /* 0x004159B6 mov dword_444250, esi */
    G32(0x444250) = esi;
    /* 0x004159BC mov dword_444220, edi */
    G32(0x444220) = edi;
    /* 0x004159C2 mov dword_444264, ebp */
    G32(0x444264) = ebp;
    /* 0x004159C8 jmp loc_4159E6 */
    goto loc_4159E6;
loc_4159CA:
    /* 0x004159CA mov ebx, 14h */
    ebx = 0x14U;
    /* 0x004159CF mov eax, esi */
    eax = esi;
    /* 0x004159D1 xor edx, edx */
    edx = 0;
    /* 0x004159D3 call sub_40C9C0 */
    d3u_fill(c, ebx, eax, edx);
    /* 0x004159D8 mov byte ptr [esi], 3 */
    M8(esi) = 3;
    /* 0x004159DB mov byte ptr [esi+1], 3 */
    M8((esi + 1) & U32MASK) = 3;
    /* 0x004159DF mov byte ptr [esi+12h], 6 */
    M8((esi + 0x12) & U32MASK) = 6;
    /* 0x004159E3 mov [esi+9], ch */
    M8((esi + 9) & U32MASK) = (unsigned char)((ecx >> 8) & 0xFFU);
loc_4159E6:
    /* 0x004159E6 mov cl, byte_444EB8 */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)G8(0x444EB8);
    /* 0x004159EC pop ebp/edi/esi/edx/ecx/ebx */
    /* 0x004159F2 retn */
    ;
}
/* sub_4159F4: entry al = symbol (bh), dl = flag. Calls sub_41B69C/41B318. */
void d3u_4159f4(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v)
{
    static unsigned long _tc = 0;
    static unsigned long _nc = 0;
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int ebp = 0;
    /* 0x004159F4 push ebx/ecx/esi/edi/ebp */
    /* 0x004159F9 mov edi, dword_444210 */
    edi = G32(0x444210);
    /* 0x004159FF mov bh, al */
    /* ebx high-16 = entry garbage; only bl/bh lanes observed below */
    ebx = (ebx & 0xFFFF00FFUL) | ((eax_v & 0xFFU) << 8);
    /* 0x00415A01 test dl, dl */
    /* 0x00415A03 jz loc_415AA4 */
    edx = edx_v;
    if ((edx & 0xFFU) == 0) goto loc_415AA4;
    /* 0x00415A09 mov bl, byte ptr dword_4441C0 */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8(0x4441C0);
    /* 0x00415A0F xor eax, eax */
    eax = 0;
    /* 0x00415A11 and bl, 1 */
    ebx = (ebx & 0xFFFFFF00UL) | ((ebx & 0xFFU) & 1U);
    /* 0x00415A14 mov edx, offset byte_444E7E */
    edx = 0x444E7EU;
    /* 0x00415A19 mov al, bl */
    eax = ebx & 0xFFU;
    /* 0x00415A1B add edx, eax */
    edx = (edx + eax) & U32MASK;
    /* 0x00415A1D mov cl, [edx] */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)M8(edx);
    /* 0x00415A1F add cl, cl */
    ecx = (ecx & 0xFFFFFF00UL) | ((((ecx & 0xFFU) + (ecx & 0xFFU))) & 0xFFU);
    /* 0x00415A21 and cl, 0Fh */
    ecx = (ecx & 0xFFFFFF00UL) | ((ecx & 0xFFU) & 0x0FU);
    /* 0x00415A24 add eax, offset byte_444E80 */
    eax = (eax + 0x444E80U) & U32MASK;
    /* 0x00415A29 mov [edx], cl */
    M8(edx) = (unsigned char)(ecx & 0xFFU);
    /* 0x00415A2B mov cl, [eax] */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)M8(eax);
    /* 0x00415A2D add cl, cl */
    ecx = (ecx & 0xFFFFFF00UL) | ((((ecx & 0xFFU) + (ecx & 0xFFU))) & 0xFFU);
    /* 0x00415A2F and cl, 0Fh */
    ecx = (ecx & 0xFFFFFF00UL) | ((ecx & 0xFFU) & 0x0FU);
    /* 0x00415A32 mov [eax], cl */
    M8(eax) = (unsigned char)(ecx & 0xFFU);
    /* 0x00415A34 xor cl, cl */
    ecx &= 0xFFFFFF00UL;
    /* 0x00415A36 mov byte_444EAF, cl */
    G8(0x444EAF) = 0;
    /* 0x00415A3C cmp bh, 80h */
    /* 0x00415A3F jb loc_415AA4 */
    if (((ebx >> 8) & 0xFFU) < 0x80U) goto loc_415AA4;
    /* 0x00415A45 cmp bh, 0FFh */
    /* 0x00415A48 jnz loc_415A6F */
    if (((ebx >> 8) & 0xFFU) != 0xFFU) goto loc_415A6F;
    /* 0x00415A4A inc byte ptr [eax] */
    M8(eax) = (unsigned char)(((unsigned int)M8(eax) + 1) & 0xFFU);
    /* 0x00415A4C test bl, bl */
    /* 0x00415A4E setz dl */
    edx = (edx & 0xFFFFFF00UL) | ((((ebx & 0xFFU) == 0)) ? 1U : 0U);
    /* 0x00415A51 and edx, 0FFh */
    edx &= 0xFFU;
    /* 0x00415A57 mov bl, byte_444E7E[edx] */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8((0x444E7EU + edx) & U32MASK);
    /* 0x00415A5D cmp bl, 0Fh */
    /* 0x00415A60 jnz loc_415AA4 */
    if ((ebx & 0xFFU) != 0x0FU) goto loc_415AA4;
    /* 0x00415A62 cmp bl, [eax] */
    /* 0x00415A64 jnz loc_415AA4 */
    if ((ebx & 0xFFU) != (unsigned int)M8(eax)) goto loc_415AA4;
    /* 0x00415A66 mov byte_444EAF, 2 */
    G8(0x444EAF) = 2;
    /* 0x00415A6D jmp loc_415AA4 */
    goto loc_415AA4;
loc_415A6F:
    /* 0x00415A6F cmp bh, 0DFh */
    /* 0x00415A72 jbe loc_415A7E */
    if (((ebx >> 8) & 0xFFU) <= 0xDFU) goto loc_415A7E;
    /* 0x00415A74 cmp bh, 0F2h */
    /* 0x00415A77 jb loc_415AA4 */
    if (((ebx >> 8) & 0xFFU) < 0xF2U) goto loc_415AA4;
    /* 0x00415A79 cmp bh, 0F6h */
    /* 0x00415A7C ja loc_415AA4 */
    if (((ebx >> 8) & 0xFFU) > 0xF6U) goto loc_415AA4;
loc_415A7E:
    /* 0x00415A7E mov ch, [edx] */
    ecx = (ecx & 0xFFFF00FFUL) | (((unsigned int)M8(edx)) << 8);
    /* 0x00415A80 inc ch */
    ecx = (ecx & 0xFFFF00FFUL) | (((((ecx >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    /* 0x00415A82 mov [edx], ch */
    M8(edx) = (unsigned char)((ecx >> 8) & 0xFFU);
    /* 0x00415A84 test bl, bl */
    /* 0x00415A86 setz al */
    eax = (eax & 0xFFFFFF00UL) | ((((ebx & 0xFFU) == 0)) ? 1U : 0U);
    /* 0x00415A89 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415A8E mov bl, byte_444E80[eax] */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8((0x444E80U + eax) & U32MASK);
    /* 0x00415A94 cmp bl, 0Fh */
    /* 0x00415A97 jnz loc_415AA4 */
    if ((ebx & 0xFFU) != 0x0FU) goto loc_415AA4;
    /* 0x00415A99 cmp ch, bl */
    /* 0x00415A9B jnz loc_415AA4 */
    if (((ecx >> 8) & 0xFFU) != (ebx & 0xFFU)) goto loc_415AA4;
    /* 0x00415A9D mov byte_444EAF, 1 */
    G8(0x444EAF) = 1;
loc_415AA4:
    /* 0x00415AA4 test bh, bh */
    /* 0x00415AA6 jz loc_415BE2 */
    if (((ebx >> 8) & 0xFFU) == 0) goto loc_415BE2;
    /* 0x00415AAC cmp bh, 0FFh */
    /* 0x00415AAF jz loc_415BE2 */
    if (((ebx >> 8) & 0xFFU) == 0xFFU) goto loc_415BE2;
    /* 0x00415AB5 xor eax, eax */
    eax = 0;
    /* 0x00415AB7 mov al, bh */
    eax = (ebx >> 8) & 0xFFU;
    /* 0x00415AB9 lea edx, [eax*4] */
    edx = (eax << 2) & U32MASK;
    /* 0x00415AC0 mov eax, dword_444274 */
    eax = G32(0x444274);
    /* 0x00415AC5 mov ecx, dword_443360[edx] */
    ecx = M32((0x443360U + edx) & U32MASK);
    /* 0x00415ACB mov esi, dword_444278 */
    esi = G32(0x444278);
    /* 0x00415AD1 sub eax, ecx */
    eax = (eax - ecx) & U32MASK;
    /* 0x00415AD3 and eax, esi */
    eax &= esi;
    /* 0x00415AD5 cmp eax, 100h */
    /* 0x00415ADA jb loc_415B1D */
    if (eax < 0x100U) goto loc_415B1D;
    /* 0x00415ADC mov al, bh */
    eax = (eax & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFU);
    /* 0x00415ADE dec al */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) - 1)) & 0xFFU);
    /* 0x00415AE0 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415AE5 lea edx, [eax*4] */
    edx = (eax << 2) & U32MASK;
    /* 0x00415AEC mov eax, dword_444274 */
    eax = G32(0x444274);
    /* 0x00415AF1 sub eax, dword_443360[edx] */
    eax = (eax - M32((0x443360U + edx) & U32MASK)) & U32MASK;
    /* 0x00415AF7 and eax, esi */
    eax &= esi;
    /* 0x00415AF9 cmp eax, 100h */
    /* 0x00415AFE jb loc_415B1D */
    if (eax < 0x100U) goto loc_415B1D;
    /* 0x00415B00 mov al, bh */
    eax = (eax & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFU);
    /* 0x00415B02 inc al */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) + 1)) & 0xFFU);
    /* 0x00415B04 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415B09 lea edx, [eax*4] */
    edx = (eax << 2) & U32MASK;
    /* 0x00415B10 mov eax, dword_444274 */
    eax = G32(0x444274);
    /* 0x00415B15 sub eax, dword_443360[edx] */
    eax = (eax - M32((0x443360U + edx) & U32MASK)) & U32MASK;
    /* 0x00415B1B and eax, esi */
    eax &= esi;
loc_415B1D:
    /* 0x00415B1D test eax, eax */
    /* 0x00415B1F jz loc_415BCC */
    if (eax == 0) goto loc_415BCC;
    /* 0x00415B25 cmp eax, 100h */
    /* 0x00415B2A jnb loc_415BCC */
    if (eax >= 0x100U) goto loc_415BCC;
    /* 0x00415B30 lea ebp, [eax*4] */
    ebp = (eax << 2) & U32MASK;
    /* 0x00415B37 mov ecx, ss:dword_442128[ebp] */
    ecx = M32((0x442128U + ebp) & U32MASK);
    /* 0x00415B3D xor edx, edx */
    edx = 0;
    /* 0x00415B3F inc ecx */
    ecx = (ecx + 1) & U32MASK;
    /* 0x00415B40 mov dl, byte_444EBA */
    edx = (unsigned int)G8(0x444EBA);
    /* 0x00415B46 mov ss:dword_442128[ebp], ecx */
    M32((0x442128U + ebp) & U32MASK) = ecx;
    /* 0x00415B4C cmp eax, edx */
    /* 0x00415B4E jz loc_415BA5 */
    if (eax == edx) goto loc_415BA5;
    /* 0x00415B50 mov bl, byte_442D58[eax] */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8((0x442D58U + eax) & U32MASK);
loc_415B56:
    /* 0x00415B56 xor edx, edx */
    edx = 0;
    /* 0x00415B58 mov dl, bl */
    edx = ebx & 0xFFU;
    /* 0x00415B5A mov ecx, ss:dword_442128[ebp] */
    ecx = M32((0x442128U + ebp) & U32MASK);
    /* 0x00415B60 cmp ecx, dword_442128[edx*4] */
    /* 0x00415B67 jb loc_415BA5 */
    if (ecx < M32((0x442128U + (edx << 2)) & U32MASK)) goto loc_415BA5;
    /* 0x00415B69 mov cl, byte_442D58[edx] */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)G8((0x442D58U + edx) & U32MASK);
    /* 0x00415B6F mov byte_442D58[eax], cl */
    G8((0x442D58U + eax) & U32MASK) = (unsigned char)(ecx & 0xFFU);
    /* 0x00415B75 mov cl, byte_443260[eax] */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)G8((0x443260U + eax) & U32MASK);
    /* 0x00415B7B mov byte_443260[edx], cl */
    G8((0x443260U + edx) & U32MASK) = (unsigned char)(ecx & 0xFFU);
    /* 0x00415B81 mov byte_442D58[edx], al */
    G8((0x442D58U + edx) & U32MASK) = (unsigned char)(eax & 0xFFU);
    /* 0x00415B87 mov byte_443260[eax], bl */
    G8((0x443260U + eax) & U32MASK) = (unsigned char)(ebx & 0xFFU);
    /* 0x00415B8D mov bl, byte_442D58[eax] */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8((0x442D58U + eax) & U32MASK);
    /* 0x00415B93 test bl, bl */
    /* 0x00415B95 jnz loc_415B56 */
    if ((ebx & 0xFFU) != 0) goto loc_415B56;
    /* 0x00415B97 mov byte_444EBA, al */
    G8(0x444EBA) = (unsigned char)(eax & 0xFFU);
    /* 0x00415B9C cmp al, 1 */
    /* 0x00415B9E jbe loc_415BA5 */
    if ((eax & 0xFFU) <= 1U) goto loc_415BA5;
    /* 0x00415BA0 mov byte_444EB9, al */
    G8(0x444EB9) = (unsigned char)(eax & 0xFFU);
loc_415BA5:
    /* 0x00415BA5 inc edi */
    edi = (edi + 1) & U32MASK;
    /* 0x00415BA6 cmp edi, 100h */
    /* 0x00415BAC jbe loc_415BCC */
    if (edi <= 0x100U) goto loc_415BCC;
    /* 0x00415BAE xor edi, edi */
    edi = 0;
    /* 0x00415BB0 xor eax, eax */
    eax = 0;
loc_415BB2:
    /* 0x00415BB2 mov ebp, dword_442128[eax] */
    ebp = M32((0x442128U + eax) & U32MASK);
    /* 0x00415BB8 shr ebp, 1 */
    ebp >>= 1;
    /* 0x00415BBA add eax, 4 */
    eax = (eax + 4) & U32MASK;
    /* 0x00415BBD add edi, ebp */
    edi = (edi + ebp) & U32MASK;
    /* 0x00415BBF mov dword_442124[eax], ebp */
    M32((0x442124U + eax) & U32MASK) = ebp;
    /* 0x00415BC5 cmp eax, 400h */
    /* 0x00415BCA jnz loc_415BB2 */
    if (eax != 0x400U) goto loc_415BB2;
loc_415BCC:
    /* 0x00415BCC xor eax, eax */
    eax = 0;
    /* 0x00415BCE mov al, bh */
    eax = (ebx >> 8) & 0xFFU;
    /* 0x00415BD0 lea edx, [eax*4] */
    edx = (eax << 2) & U32MASK;
    /* 0x00415BD7 mov eax, dword_444274 */
    eax = G32(0x444274);
    /* 0x00415BDC mov dword_443360[edx], eax */
    M32((0x443360U + edx) & U32MASK) = eax;
loc_415BE2:
    /* 0x00415BE2 mov al, byte_444EA9 */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8(0x444EA9);
    /* 0x00415BE7 mov dword_444210, edi */
    G32(0x444210) = edi;
    /* 0x00415BED test al, al */
    /* 0x00415BEF jz loc_415D79 */
    if ((eax & 0xFFU) == 0) goto loc_415D79;
    /* 0x00415BF5 mov al, bh */
    eax = (eax & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFU);
    /* 0x00415BF7 mov ah, byte_444E9D */
    eax = (eax & 0xFFFF00FFUL) | (((unsigned int)G8(0x444E9D)) << 8);
    /* 0x00415BFD mov dl, byte_444EA8 */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)G8(0x444EA8);
    /* 0x00415C03 sub al, ah */
    eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFU) - ((eax >> 8) & 0xFFU))) & 0xFFU);
    /* 0x00415C05 test dl, dl */
    /* 0x00415C07 jz loc_415C0B */
    if ((edx & 0xFFU) == 0) goto loc_415C0B;
    /* 0x00415C09 neg al */
    eax = (eax & 0xFFFFFF00UL) | (((0U - (eax & 0xFFU))) & 0xFFU);
loc_415C0B:
    /* 0x00415C0B cmp al, 80h */
    /* 0x00415C0D jb loc_415C20 */
    if ((eax & 0xFFU) < 0x80U) goto loc_415C20;
    /* 0x00415C0F mov dh, byte_444EA8 */
    edx = (edx & 0xFFFF00FFUL) | (((unsigned int)G8(0x444EA8)) << 8);
    /* 0x00415C15 test dh, dh */
    /* 0x00415C17 setz ah */
    eax = (eax & 0xFFFF00FFUL) | ((((((edx >> 8) & 0xFFU) == 0)) ? 1U : 0U) << 8);
    /* 0x00415C1A mov byte_444EA8, ah */
    G8(0x444EA8) = (unsigned char)((eax >> 8) & 0xFFU);
loc_415C20:
    /* 0x00415C20 mov bl, byte_444EA9 */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8(0x444EA9);
    /* 0x00415C26 cmp bl, 1 */
    /* 0x00415C29 jnz loc_415C70 */
    if ((ebx & 0xFFU) != 1U) goto loc_415C70;
    /* 0x00415C2B mov edx, dword_4441C4 */
    edx = G32(0x4441C4);
    /* 0x00415C31 sub edx, 203h */
    edx = (edx - 0x203U) & U32MASK;
    /* 0x00415C37 and edx, 0Fh */
    edx &= 0x0FU;
    /* 0x00415C3A mov dl, byte_429E44[edx] */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)G8((0x429E44U + edx) & U32MASK);
    /* 0x00415C40 and edx, 0FFh */
    edx &= 0xFFU;
    /* 0x00415C46 shl edx, 4 */
    edx = (edx << 4) & U32MASK;
    /* 0x00415C49 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415C4E mov al, byte_429E54[eax] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8((0x429E54U + eax) & U32MASK);
    /* 0x00415C54 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415C59 add eax, edx */
    eax = (eax + edx) & U32MASK;
    /* 0x00415C5B add eax, 203h */
    eax = (eax + 0x203U) & U32MASK;
    /* 0x00415C60 mov dword_444210, edi */
    G32(0x444210) = edi;
    /* 0x00415C66 mov dword_4441C4, eax */
    G32(0x4441C4) = eax;
    /* 0x00415C6B jmp loc_415D79 */
    goto loc_415D79;
loc_415C70:
    /* 0x00415C70 mov edx, dword_4441D0 */
    edx = G32(0x4441D0);
    /* 0x00415C76 shl edx, 4 */
    edx = (edx << 4) & U32MASK;
    /* 0x00415C79 cmp bl, 2 */
    /* 0x00415C7C jnz loc_415CA7 */
    if ((ebx & 0xFFU) != 2U) goto loc_415CA7;
    /* 0x00415C7E and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415C83 mov al, byte_429E54[eax] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8((0x429E54U + eax) & U32MASK);
    /* 0x00415C89 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415C8E add edx, eax */
    edx = (edx + eax) & U32MASK;
    /* 0x00415C90 add edx, 223h */
    edx = (edx + 0x223U) & U32MASK;
    /* 0x00415C96 mov dword_444210, edi */
    G32(0x444210) = edi;
    /* 0x00415C9C mov dword_4441C4, edx */
    G32(0x4441C4) = edx;
    /* 0x00415CA2 jmp loc_415D79 */
    goto loc_415D79;
loc_415CA7:
    /* 0x00415CA7 cmp bl, 3 */
    /* 0x00415CAA jnz loc_415D43 */
    if ((ebx & 0xFFU) != 3U) goto loc_415D43;
    /* 0x00415CB0 cmp byte_444E9B, 0 */
    /* 0x00415CB7 jz loc_415CEF */
    if (G8(0x444E9B) == 0) goto loc_415CEF;
    /* 0x00415CB9 mov edx, dword_444258 */
    edx = G32(0x444258);
    /* 0x00415CBF xor eax, eax */
    eax = 0;
    /* 0x00415CC1 shr edx, 10h */
    edx >>= 16;
    /* 0x00415CC4 mov al, bh */
    eax = (ebx >> 8) & 0xFFU;
    /* 0x00415CC6 sub eax, edx */
    eax = (eax - edx) & U32MASK;
    /* 0x00415CC8 mov ah, byte_444E9F */
    eax = (eax & 0xFFFF00FFUL) | (((unsigned int)G8(0x444E9F)) << 8);
    /* 0x00415CCE mov byte_444EA8, ah */
    G8(0x444EA8) = (unsigned char)((eax >> 8) & 0xFFU);
    /* 0x00415CD4 test ah, ah */
    /* 0x00415CD6 jz loc_415CDA */
    if (((eax >> 8) & 0xFFU) == 0) goto loc_415CDA;
    /* 0x00415CD8 neg al */
    eax = (eax & 0xFFFFFF00UL) | (((0U - (eax & 0xFFU))) & 0xFFU);
loc_415CDA:
    /* 0x00415CDA cmp al, 80h */
    /* 0x00415CDC jb loc_415CEF */
    if ((eax & 0xFFU) < 0x80U) goto loc_415CEF;
    /* 0x00415CDE mov bl, byte_444EA8 */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8(0x444EA8);
    /* 0x00415CE4 test bl, bl */
    /* 0x00415CE6 setz ah */
    eax = (eax & 0xFFFF00FFUL) | (((((ebx & 0xFFU) == 0)) ? 1U : 0U) << 8);
    /* 0x00415CE9 mov byte_444EA8, ah */
    G8(0x444EA8) = (unsigned char)((eax >> 8) & 0xFFU);
loc_415CEF:
    /* 0x00415CEF xor ecx, ecx */
    ecx = 0;
    /* 0x00415CF1 mov cl, byte_444E9C */
    ecx = (unsigned int)G8(0x444E9C);
    /* 0x00415CF7 mov ah, byte_444284[ecx] */
    eax = (eax & 0xFFFF00FFUL) | (((unsigned int)G8((0x444284U + ecx) & U32MASK)) << 8);
    /* 0x00415CFD lea edx, [ecx*4] */
    edx = (ecx << 2) & U32MASK;
    /* 0x00415D04 cmp bh, ah */
    /* 0x00415D06 jnz loc_415D10 */
    if (((ebx >> 8) & 0xFFU) != ((eax >> 8) & 0xFFU)) goto loc_415D10;
    /* 0x00415D08 inc dword_440AF0[edx] */
    M32((0x440AF0U + edx) & U32MASK) = (M32((0x440AF0U + edx) & U32MASK) + 1) & U32MASK;
    /* 0x00415D0E jmp loc_415D16 */
    goto loc_415D16;
loc_415D10:
    /* 0x00415D10 shr dword_440AF0[edx], 1 */
    M32((0x440AF0U + edx) & U32MASK) = M32((0x440AF0U + edx) & U32MASK) >> 1;
loc_415D16:
    /* 0x00415D16 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415D1B mov al, byte_429F54[eax] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8((0x429F54U + eax) & U32MASK);
    /* 0x00415D21 mov edx, dword_4441D0 */
    edx = G32(0x4441D0);
    /* 0x00415D27 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415D2C shl edx, 5 */
    edx = (edx << 5) & U32MASK;
    /* 0x00415D2F add eax, edx */
    eax = (eax + edx) & U32MASK;
    /* 0x00415D31 add eax, 243h */
    eax = (eax + 0x243U) & U32MASK;
    /* 0x00415D36 mov dword_444210, edi */
    G32(0x444210) = edi;
    /* 0x00415D3C mov dword_4441C4, eax */
    G32(0x4441C4) = eax;
    /* 0x00415D41 jmp loc_415D79 */
    goto loc_415D79;
loc_415D43:
    /* 0x00415D43 mov dword_444210, edi */
    G32(0x444210) = edi;
    /* 0x00415D49 cmp bl, 4 */
    /* 0x00415D4C jnz loc_415D6C */
    if ((ebx & 0xFFU) != 4U) goto loc_415D6C;
    /* 0x00415D4E and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415D53 mov al, byte_429E54[eax] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8((0x429E54U + eax) & U32MASK);
    /* 0x00415D59 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00415D5E add eax, edx */
    eax = (eax + edx) & U32MASK;
    /* 0x00415D60 add eax, 2A3h */
    eax = (eax + 0x2A3U) & U32MASK;
    /* 0x00415D65 mov dword_4441C4, eax */
    G32(0x4441C4) = eax;
    /* 0x00415D6A jmp loc_415D79 */
    goto loc_415D79;
loc_415D6C:
    /* 0x00415D6C xor edx, edx */
    edx = 0;
    /* 0x00415D6E mov dl, al */
    edx = eax & 0xFFU;
    /* 0x00415D70 xor eax, eax */
    eax = 0;
    /* 0x00415D72 mov al, bh */
    eax = (ebx >> 8) & 0xFFU;
    /* 0x00415D74 call sub_41B69C */
    d3u_41b69c(c, eax, edx);
loc_415D79:
    /* 0x00415D79 mov ebp, dword_4441C0 */
    ebp = G32(0x4441C0);
    /* 0x00415D7F mov edi, dword_444210 */
    edi = G32(0x444210);
    /* 0x00415D85 inc ebp */
    ebp = (ebp + 1) & U32MASK;
    /* 0x00415D86 mov dl, byte_444EA0 */
    edx = (edx & 0xFFFFFF00UL) | (unsigned int)G8(0x444EA0);
    /* 0x00415D8C mov dword_4441C0, ebp */
    G32(0x4441C0) = ebp;
    /* 0x00415D92 test dl, dl */
    /* 0x00415D94 jnz loc_415DF2 */
    if ((edx & 0xFFU) != 0) goto loc_415DF2;
    /* 0x00415D96 cmp bh, 4 */
    /* 0x00415D99 jb loc_415DF2 */
    if (((ebx >> 8) & 0xFFU) < 4U) goto loc_415DF2;
    /* 0x00415D9B cmp bh, 6 */
    /* 0x00415D9E ja loc_415DF2 */
    if (((ebx >> 8) & 0xFFU) > 6U) goto loc_415DF2;
    /* 0x00415DA0 cmp byte ptr dword_444258, 0DFh */
    /* 0x00415DA7 jz loc_415E38 */
    if (G8(0x444258) == 0xDFU) goto loc_415E38;
    /* 0x00415DAD mov edx, dword_444260 */
    edx = G32(0x444260);
    /* 0x00415DB3 mov eax, dword_44425C */
    eax = G32(0x44425C);
    /* 0x00415DB8 shl edx, 8 */
    edx = (edx << 8) & U32MASK;
    /* 0x00415DBB shr eax, 18h */
    eax >>= 24;
    /* 0x00415DBE add edx, eax */
    edx = (edx + eax) & U32MASK;
    /* 0x00415DC0 mov eax, dword_44425C */
    eax = G32(0x44425C);
    /* 0x00415DC5 mov dword_444260, edx */
    G32(0x444260) = edx;
    /* 0x00415DCB mov edx, dword_444258 */
    edx = G32(0x444258);
    /* 0x00415DD1 shl eax, 8 */
    eax = (eax << 8) & U32MASK;
    /* 0x00415DD4 shr edx, 18h */
    edx >>= 24;
    /* 0x00415DD7 add eax, edx */
    eax = (eax + edx) & U32MASK;
    /* 0x00415DD9 mov dword_44425C, eax */
    G32(0x44425C) = eax;
    /* 0x00415DDE mov eax, dword_444258 */
    eax = G32(0x444258);
    /* 0x00415DE3 shl eax, 8 */
    eax = (eax << 8) & U32MASK;
    /* 0x00415DE6 add eax, 0DFh */
    eax = (eax + 0xDFU) & U32MASK;
    /* 0x00415DEB mov dword_444258, eax */
    G32(0x444258) = eax;
    /* 0x00415DF0 jmp loc_415E38 */
    goto loc_415E38;
loc_415DF2:
    /* 0x00415DF2 mov edx, dword_444260 */
    edx = G32(0x444260);
    /* 0x00415DF8 mov eax, dword_44425C */
    eax = G32(0x44425C);
    /* 0x00415DFD shl edx, 8 */
    edx = (edx << 8) & U32MASK;
    /* 0x00415E00 shr eax, 18h */
    eax >>= 24;
    /* 0x00415E03 add edx, eax */
    edx = (edx + eax) & U32MASK;
    /* 0x00415E05 mov eax, dword_44425C */
    eax = G32(0x44425C);
    /* 0x00415E0A mov dword_444260, edx */
    G32(0x444260) = edx;
    /* 0x00415E10 mov edx, dword_444258 */
    edx = G32(0x444258);
    /* 0x00415E16 shl eax, 8 */
    eax = (eax << 8) & U32MASK;
    /* 0x00415E19 shr edx, 18h */
    edx >>= 24;
    /* 0x00415E1C add eax, edx */
    eax = (eax + edx) & U32MASK;
    /* 0x00415E1E mov edx, dword_444258 */
    edx = G32(0x444258);
    /* 0x00415E24 mov dword_44425C, eax */
    G32(0x44425C) = eax;
    /* 0x00415E29 xor eax, eax */
    eax = 0;
    /* 0x00415E2B shl edx, 8 */
    edx = (edx << 8) & U32MASK;
    /* 0x00415E2E mov al, bh */
    eax = (ebx >> 8) & 0xFFU;
    /* 0x00415E30 add edx, eax */
    edx = (edx + eax) & U32MASK;
    /* 0x00415E32 mov dword_444258, edx */
    G32(0x444258) = edx;
loc_415E38:
    /* 0x00415E38 mov bl, byte_444EA9 */
    ebx = (ebx & 0xFFFFFF00UL) | (unsigned int)G8(0x444EA9);
    /* 0x00415E3E mov dword_444210, edi */
    G32(0x444210) = edi;
    /* 0x00415E44 test bl, bl */
    /* 0x00415E46 jz loc_415F5D */
    if ((ebx & 0xFFU) == 0) goto loc_415F5D;
    /* 0x00415E4C mov eax, dword_4441C0 */
    eax = G32(0x4441C0);
    /* 0x00415E51 mov ebx, dword_4441C8 */
    ebx = G32(0x4441C8);
    /* 0x00415E57 xor edx, edx */
    edx = 0;
    /* 0x00415E59 div ebx */
    /* x86 div faults on div0; direct / % below */
    edx = eax % ebx;
    eax = eax / ebx;
    /* 0x00415E5B mov ebp, dword_4441D0 */
    ebp = G32(0x4441D0);
    /* 0x00415E61 mov cl, byte_444E9E */
    ecx = (ecx & 0xFFFFFF00UL) | (unsigned int)G8(0x444E9E);
    /* 0x00415E67 mov eax, dword_444258 */
    eax = G32(0x444258);
    /* 0x00415E6C mov bh, byte_444EA9 */
    ebx = (ebx & 0xFFFF00FFUL) | (((unsigned int)G8(0x444EA9)) << 8);
    /* 0x00415E72 shr eax, cl */
    /* x86 masks the count to 5 bits; direct shift below */
    eax >>= ((ecx & 0xFFU) & 31U);
    /* 0x00415E74 mov dword_4441D0, edx */
    G32(0x4441D0) = edx;
    /* 0x00415E7A mov byte_444E9D, al */
    G8(0x444E9D) = (unsigned char)(eax & 0xFFU);
    /* 0x00415E7F cmp bh, 3 */
    /* 0x00415E82 jnz loc_415F53 */
    if (((ebx >> 8) & 0xFFU) != 3U) goto loc_415F53;
    /* 0x00415E88 mov ecx, 6 */
    ecx = 6;
    /* 0x00415E8D mov eax, dword_4441C0 */
    eax = G32(0x4441C0);
    /* 0x00415E92 xor edx, edx */
    edx = 0;
    /* 0x00415E94 div ecx */
    /* x86 div: divisor is 6 (nonzero); direct / % below */
    edx = eax % 6U;
    eax = eax / 6U;
    /* 0x00415E96 mov byte_444E9C, dl */
    G8(0x444E9C) = (unsigned char)(edx & 0xFFU);
    /* 0x00415E9C xor edx, edx */
    edx = 0;
    /* 0x00415E9E mov dl, byte_444E9C */
    edx = (unsigned int)G8(0x444E9C);
    /* 0x00415EA4 cmp dword_440AF0[edx*4], 8 */
    /* 0x00415EAC setnbe al */
    eax = (eax & 0xFFFFFF00UL) | ((M32((0x440AF0U + (edx << 2)) & U32MASK) > 8U) ? 1U : 0U);
    /* 0x00415EAF mov ecx, dword_444258 */
    ecx = G32(0x444258);
    /* 0x00415EB5 movzx esi, byte ptr dword_444258 */
    esi = (unsigned int)G8(0x444258);
    /* 0x00415EBC shr ecx, 18h */
    ecx >>= 24;
    /* 0x00415EBF sub esi, ecx */
    esi = (esi - ecx) & U32MASK;
    /* 0x00415EC1 xor ecx, ecx */
    ecx = 0;
    /* 0x00415EC3 mov cl, byte_444E9D */
    ecx = (unsigned int)G8(0x444E9D);
    /* 0x00415EC9 add ecx, esi */
    ecx = (ecx + esi) & U32MASK;
    /* 0x00415ECB mov bl, al */
    ebx = (ebx & 0xFFFFFF00UL) | (eax & 0xFFU);
    /* 0x00415ECD mov byte_444284[edx], cl */
    G8((0x444284U + edx) & U32MASK) = (unsigned char)(ecx & 0xFFU);
    /* 0x00415ED3 test al, al */
    /* 0x00415ED5 jz loc_415EE4 */
    if ((eax & 0xFFU) == 0) goto loc_415EE4;
    /* 0x00415ED7 cmp dword_4441B4, 5 */
    /* 0x00415EDE jnb loc_415EE4 */
    if (G32(0x4441B4) >= 5U) goto loc_415EE4;
    /* 0x00415EE0 mov al, 1 */
    eax = (eax & 0xFFFFFF00UL) | 1U;
    /* 0x00415EE2 jmp loc_415EE6 */
    goto loc_415EE6;
loc_415EE4:
    /* 0x00415EE4 xor al, al */
    eax &= 0xFFFFFF00UL;
loc_415EE6:
    /* 0x00415EE6 mov byte_444E9B, al */
    G8(0x444E9B) = (unsigned char)(eax & 0xFFU);
    /* 0x00415EEB test al, al */
    /* 0x00415EED jz loc_415F22 */
    if ((eax & 0xFFU) == 0) goto loc_415F22;
    /* 0x00415EEF mov al, byte_444EA8 */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8(0x444EA8);
    /* 0x00415EF4 mov byte_444E9F, al */
    G8(0x444E9F) = (unsigned char)(eax & 0xFFU);
    /* 0x00415EF9 mov eax, ebp */
    eax = ebp;
    /* 0x00415EFB shl eax, 5 */
    eax = (eax << 5) & U32MASK;
    /* 0x00415EFE add eax, 253h */
    eax = (eax + 0x253U) & U32MASK;
    /* 0x00415F03 mov dword_4441C4, eax */
    G32(0x4441C4) = eax;
    /* 0x00415F08 xor eax, eax */
    eax = 0;
    /* 0x00415F0A mov al, byte_444E9C */
    eax = (unsigned int)G8(0x444E9C);
    /* 0x00415F0F xor ch, ch */
    ecx &= 0xFFFF00FFUL;
    /* 0x00415F11 mov al, byte_444284[eax] */
    eax = (eax & 0xFFFFFF00UL) | (unsigned int)G8((0x444284U + (eax & 0xFFU)) & U32MASK);
    /* 0x00415F17 mov byte_444EA8, ch */
    G8(0x444EA8) = 0;
    /* 0x00415F1D mov byte_444E9D, al */
    G8(0x444E9D) = (unsigned char)(eax & 0xFFU);
loc_415F22:
    /* 0x00415F22 test bl, bl */
    /* 0x00415F24 jz loc_415F39 */
    if ((ebx & 0xFFU) == 0) goto loc_415F39;
    /* 0x00415F26 mov eax, dword_4441B4 */
    eax = G32(0x4441B4);
    /* 0x00415F2B inc eax */
    eax = (eax + 1) & U32MASK;
    /* 0x00415F2C mov dword_444210, edi */
    G32(0x444210) = edi;
    /* 0x00415F32 mov dword_4441B4, eax */
    G32(0x4441B4) = eax;
    /* 0x00415F37 jmp loc_415F5D */
    goto loc_415F5D;
loc_415F39:
    /* 0x00415F39 xor ebp, ebp */
    ebp = 0;
    /* 0x00415F3B mov dword_444210, edi */
    G32(0x444210) = edi;
    /* 0x00415F41 mov dword_4441B4, ebp */
    G32(0x4441B4) = ebp;
    /* 0x00415F47 mov edi, dword_444210 */
    edi = G32(0x444210);
    /* 0x00415F4D pop ebp/edi/esi/ecx/ebx */
    /* 0x00415F52 retn (early) */
    return;
loc_415F53:
    /* 0x00415F53 cmp bh, 5 */
    /* 0x00415F56 jb loc_415F5D */
    if (((ebx >> 8) & 0xFFU) < 5U) goto loc_415F5D;
    /* 0x00415F58 call sub_41B318 */
    d3u_41b318(c);
loc_415F5D:
    /* 0x00415F5D mov edi, dword_444210 */
    edi = G32(0x444210);
    /* 0x00415F63 pop ebp/edi/esi/ecx/ebx */
    /* 0x00415F68 retn */
    ;
}
/* sub_41855C: entry eax = count, edx = ctx id, bl = symbol. Coder-agnostic. */
void d3u_41855c(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v, unsigned int ebx_v)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    /* 0x0041855C push ecx/esi/edi */
    /* 0x0041855F mov esi, eax */
    esi = eax_v;
    /* 0x00418561 mov ecx, edx */
    ecx = edx_v;
    /* 0x00418563 xor edx, edx */
    edx = 0;
    /* 0x00418565 mov dword_4441F4, edx */
    G32(0x4441F4) = edx;
    /* 0x0041856B mov dl, bl */
    edx = ebx_v & 0xFFU;
    ebx = ebx_v;
    /* 0x0041856D mov dword_44421C, ecx */
    G32(0x44421C) = ecx;
    /* 0x00418573 call sub_417D74 */
    d3u_417d74(c, esi, edx);
    /* 0x00418578 xor edx, edx */
    edx = 0;
    /* 0x0041857A mov dl, byte_444EB3 */
    edx = (unsigned int)G8(0x444EB3);
    /* 0x00418580 mov edi, dword_44421C */
    edi = G32(0x44421C);
    /* 0x00418586 mov ah, dl */
    /* eax high lanes = entry garbage; only ah observed below */
    eax = (eax & 0xFFFF00FFUL) | ((edx & 0xFFU) << 8);
    /* 0x00418588 mov dword_441F08[edx*4], edi */
    M32((0x441F08U + (edx << 2)) & U32MASK) = edi;
    /* 0x0041858F mov dword_442018[edx*4], esi */
    M32((0x442018U + (edx << 2)) & U32MASK) = esi;
    /* 0x00418596 inc ah */
    eax = (eax & 0xFFFF00FFUL) | (((((eax >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    /* 0x00418598 mov byte_4441D8[edx], bl */
    G8((0x4441D8U + edx) & U32MASK) = (unsigned char)(ebx & 0xFFU);
    /* 0x0041859E mov byte_444EB3, ah */
    G8(0x444EB3) = (unsigned char)((eax >> 8) & 0xFFU);
    /* 0x004185A4 cmp ecx, 2FDh */
    /* 0x004185AA setz al */
    eax = (eax & 0xFFFFFF00UL) | ((ecx == 0x2FDU) ? 1U : 0U);
    /* 0x004185AD and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x004185B2 mov edx, dword_443258[eax*4] */
    edx = M32((0x443258U + (eax << 2)) & U32MASK);
    /* 0x004185B9 mov eax, edx */
    eax = edx;
    /* 0x004185BB shl eax, 5 */
    eax = (eax << 5) & U32MASK;
    /* 0x004185BE sub eax, edx */
    eax = (eax - edx) & U32MASK;
    /* 0x004185C0 mov edx, dword_4441F4 */
    edx = G32(0x4441F4);
    /* 0x004185C6 shr eax, 5 */
    eax >>= 5;
    /* 0x004185C9 add edx, eax */
    edx = (edx + eax) & U32MASK;
    /* 0x004185CB cmp ecx, 2FDh */
    /* 0x004185D1 setz al */
    eax = (eax & 0xFFFFFF00UL) | ((ecx == 0x2FDU) ? 1U : 0U);
    /* 0x004185D4 and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x004185D9 mov dword_443258[eax*4], edx */
    M32((0x443258U + (eax << 2)) & U32MASK) = edx;
    /* 0x004185E0 cmp ecx, 2FDh */
    /* 0x004185E6 setz al */
    eax = (eax & 0xFFFFFF00UL) | ((ecx == 0x2FDU) ? 1U : 0U);
    /* 0x004185E9 xor edx, edx */
    edx = 0;
    /* 0x004185EB mov dl, al */
    edx = eax & 0xFFU;
    /* 0x004185ED mov edx, dword_443B60[edx*4] */
    edx = M32((0x443B60U + (edx << 2)) & U32MASK);
    /* 0x004185F4 mov eax, edx */
    eax = edx;
    /* 0x004185F6 shl eax, 5 */
    eax = (eax << 5) & U32MASK;
    /* 0x004185F9 sub eax, edx */
    eax = (eax - edx) & U32MASK;
    /* 0x004185FB mov edx, dword_4441F4 */
    edx = G32(0x4441F4);
    /* 0x00418601 shr eax, 5 */
    eax >>= 5;
    /* 0x00418604 add edx, eax */
    edx = (edx + eax) & U32MASK;
    /* 0x00418606 cmp ecx, 2FDh */
    /* 0x0041860C setz al */
    eax = (eax & 0xFFFFFF00UL) | ((ecx == 0x2FDU) ? 1U : 0U);
    /* 0x0041860F and eax, 0FFh */
    eax &= 0xFFU;
    /* 0x00418614 mov dword_443B60[eax*4], edx */
    M32((0x443B60U + (eax << 2)) & U32MASK) = edx;
    /* 0x0041861B pop edi/esi/ecx */
    /* 0x0041861E retn */
}
/* sub_418620: entry edx = ctx id, bl = symbol. Coder-agnostic. */
void d3u_418620(dec3w_ctx *c, unsigned int edx_v, unsigned int ebx_v)
{
    unsigned int eax = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    /* 0x00418620 push ecx */
    /* 0x00418621 mov dword_44421C, edx */
    G32(0x44421C) = edx_v;
    /* 0x00418627 xor edx, edx */
    edx = 0;
    /* 0x00418629 xor ecx, ecx */
    ecx = 0;
    /* 0x0041862B mov dl, bl */
    edx = ebx_v & 0xFFU;
    /* 0x0041862D mov dword_4441F4, ecx */
    G32(0x4441F4) = ecx;
    /* 0x00418633 call sub_417D74 */
    /* 0x00418633 call sub_417D74.
     * eax_in is the dispatcher-leftover M32(0x4441C4): native 418620 never
     * writes eax, so 417d74 receives the caller's eax (set at 0x00417421).
     * Passing 0 here starved the db3 loop and overheated the E1E0 EMA. */
    d3u_417d74(c, (G32(0x4441C4) & U32MASK), edx);
    /* 0x00418638 mov edx, dword_4441E0 */
    edx = G32(0x4441E0);
    /* 0x0041863E mov eax, edx */
    eax = edx;
    /* 0x00418640 shl eax, 5 */
    eax = (eax << 5) & U32MASK;
    /* 0x00418643 sub eax, edx */
    eax = (eax - edx) & U32MASK;
    /* 0x00418645 mov edx, eax */
    edx = eax;
    /* 0x00418647 mov eax, dword_4441F4 */
    eax = G32(0x4441F4);
    /* 0x0041864C shr edx, 5 */
    edx >>= 5;
    /* 0x0041864F add edx, eax */
    edx = (edx + eax) & U32MASK;
    /* 0x00418651 mov dword_4441E0, edx */
    G32(0x4441E0) = edx;
    /* 0x00418657 pop ecx */
    /* 0x00418658 retn */
}

/* ---- top-ups (from draft_t3.c) ---- */
/* sub_414D78: entry-live reg is eax only (ebx,ecx,edx,esi,edi,ebp pushed).
 * Halves 32 dwords at heapF4+a1*512 ((w+0x10001)&0xFFFEFFFE)>>1, sums into
 * ebp, copies 510B heapF4-block to heapF0-block+2, stores ebp-folded word,
 * prefix-sums the block, bumps heapFC word if a1<0x2FD. Return (eax) unused
 * in this span, hence void. */
void d3u_414d78(dec3w_ctx *c, unsigned int eax_in)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    unsigned long esi = 0;
    unsigned long edi = 0;
    unsigned long ebp = 0;
    unsigned long a1 = 0;
    /* 0x00414D81 */ a1 = (unsigned long)eax_in; /* mov [var_1C],eax */
    /* 0x00414D84 */ edx = a1; /* mov edx,eax */
    /* 0x00414D86 */ ebx = a1; /* mov ebx,eax */
    /* 0x00414D88 */ eax = (unsigned long)G32(0x4299F4); /* mov eax,dword_4299F4 */
    /* 0x00414D8D */ ebx = (ebx << 9) & U32MASK; /* shl ebx,9 */
    /* 0x00414D90 */ ebp = 0; /* xor ebp,ebp */
    /* 0x00414D92 */ ebx = (ebx + eax) & U32MASK; /* add ebx,eax */
    /* 0x00414D94 */ edx = (edx << 8) & U32MASK; /* shl edx,8 */
    /* 0x00414D97 */ edi = (ebx + 0x200UL) & U32MASK; /* lea edi,[ebx+200h] */
l_414d9d:
    /* 0x00414D9D */ ecx = (unsigned long)M32(ebx); /* mov ecx,[ebx] */
    /* 0x00414D9F */ eax = (unsigned long)M32((ebx + 4UL) & U32MASK); /* mov eax,[ebx+4] */
    /* 0x00414DA2 */ ecx = (ecx + 0x10001UL) & U32MASK; /* add ecx,10001h */
    /* 0x00414DA8 */ eax = (eax + 0x10001UL) & U32MASK; /* add eax,10001h */
    /* 0x00414DAD */ ecx &= 0xFFFEFFFEUL; /* and ecx,0FFFEFFFEh */
    /* 0x00414DB3 */ eax &= 0xFFFEFFFEUL; /* and eax,0FFFEFFFEh */
    /* 0x00414DB8 */ ecx >>= 1; /* shr ecx,1 */
    /* 0x00414DBA */ eax >>= 1; /* shr eax,1 */
    /* 0x00414DBC */ ebx = (ebx + 0x10UL) & U32MASK; /* add ebx,10h */
    /* 0x00414DBF */ esi = (ecx + eax) & U32MASK; /* lea esi,[ecx+eax] */
    /* 0x00414DC2 */ M32((ebx - 0xCUL) & U32MASK) = (unsigned int)eax; /* mov [ebx-0Ch],eax */
    /* 0x00414DC5 */ M32((ebx - 0x10UL) & U32MASK) = (unsigned int)ecx; /* mov [ebx-10h],ecx */
    /* 0x00414DC8 */ eax = (unsigned long)M32((ebx - 4UL) & U32MASK); /* mov eax,[ebx-4] */
    /* 0x00414DCB */ ecx = (unsigned long)M32((ebx - 8UL) & U32MASK); /* mov ecx,[ebx-8] */
    /* 0x00414DCE */ eax = (eax + 0x10001UL) & U32MASK; /* add eax,10001h */
    /* 0x00414DD3 */ ecx = (ecx + 0x10001UL) & U32MASK; /* add ecx,10001h */
    /* 0x00414DD9 */ eax &= 0xFFFEFFFEUL; /* and eax,0FFFEFFFEh */
    /* 0x00414DDE */ ecx &= 0xFFFEFFFEUL; /* and ecx,0FFFEFFFEh */
    /* 0x00414DE4 */ eax >>= 1; /* shr eax,1 */
    /* 0x00414DE6 */ ecx >>= 1; /* shr ecx,1 */
    /* 0x00414DE8 */ ebp = (ebp + esi) & U32MASK; /* add ebp,esi */
    /* 0x00414DEA */ esi = (ecx + eax) & U32MASK; /* lea esi,[ecx+eax] */
    /* 0x00414DED */ M32((ebx - 4UL) & U32MASK) = (unsigned int)eax; /* mov [ebx-4],eax */
    /* 0x00414DF0 */ ebp = (ebp + esi) & U32MASK; /* add ebp,esi */
    /* 0x00414DF2 */ M32((ebx - 8UL) & U32MASK) = (unsigned int)ecx; /* mov [ebx-8],ecx */
    /* 0x00414DF5 cmp ebx,edi */ /* 0x00414DF7 jnz */ if (ebx != edi) goto l_414d9d;
    /* 0x00414DF9 */ ecx = 0x1FEUL; /* mov ecx,1FEh */
    /* 0x00414DFE */ ebx = (edx * 2UL) & U32MASK; /* lea ebx,[edx*2] (ebx=a1*512) */
    /* 0x00414E05 */ eax = (unsigned long)G32(0x4299F0); /* mov eax,dword_4299F0 */
    /* 0x00414E0A */ edi = (ebx + 2UL) & U32MASK; /* lea edi,[ebx+2] */
    /* 0x00414E0D */ esi = (unsigned long)G32(0x4299F4); /* mov esi,dword_4299F4 */
    /* 0x00414E13 */ edi = (edi + eax) & U32MASK; /* add edi,eax (dst=heapF0+a1*512+2) */
    /* 0x00414E15 */ esi = (esi + ebx) & U32MASK; /* add esi,ebx (src=heapF4+a1*512) */
    /* 0x00414E17 cld */;
    /* 0x00414E18 */ eax = ecx; /* mov eax,ecx */
    /* 0x00414E1A */ ecx = (ecx - edi) & U32MASK; /* sub ecx,edi */
    /* 0x00414E1C */ ecx = (ecx - eax) & U32MASK; /* sub ecx,eax */
    /* 0x00414E1E */ ecx &= 3UL; /* and ecx,3 */
    /* 0x00414E21 */ eax = (eax - ecx) & U32MASK; /* sub eax,ecx */
    /* 0x00414E23 jle */ if ((int)(eax & U32MASK) > 0) { /* signed */
        /* 0x00414E25 rep movsb */
        while (ecx != 0) { M8(edi) = M8(esi); esi = (esi + 1UL) & U32MASK; edi = (edi + 1UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
        /* 0x00414E27 */ ecx = eax; /* mov ecx,eax */
        /* 0x00414E29 */ eax &= 3UL; /* and eax,3 */
        /* 0x00414E2C */ ecx >>= 2; /* shr ecx,2 */
        /* 0x00414E2F rep movsd */
        while (ecx != 0) { M32(edi) = M32(esi); esi = (esi + 4UL) & U32MASK; edi = (edi + 4UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
    }
l_414e31:
    /* 0x00414E31 */ ecx = (ecx + eax) & U32MASK; /* add ecx,eax */
    /* 0x00414E33 rep movsb */
    while (ecx != 0) { M8(edi) = M8(esi); esi = (esi + 1UL) & U32MASK; edi = (edi + 1UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
    /* 0x00414E35 */ ecx = ebp; /* mov ecx,ebp */
    /* 0x00414E37 */ eax = ebp; /* mov eax,ebp */
    /* 0x00414E39 */ ecx &= 0xFFFFUL; /* and ecx,0FFFFh */
    /* 0x00414E3F */ eax >>= 16; /* shr eax,10h */
    /* 0x00414E42 */ eax = (eax + ecx) & U32MASK; /* add eax,ecx */
    /* 0x00414E44 */ ecx = eax; /* mov ecx,eax */
    /* 0x00414E46 */ eax = (unsigned long)G32(0x4299F0); /* mov eax,dword_4299F0 */
    /* 0x00414E4B */ M16((ebx + eax) & U32MASK) = (unsigned short)(ecx & 0xFFFFUL); /* mov [ebx+eax],cx */
    /* 0x00414E4F */ ebx = (ebx & 0xFFFF00FFUL) | 0x100UL; /* mov bh,1 (lane) */
l_414e51:
    /* 0x00414E51 */ ebx = (ebx & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL); /* mov bl,bh */
    /* 0x00414E53 */ edx &= 0xFFFFFF00UL; /* mov dl,0 (high lanes keep (a1<<8)&~FFh) */
    /* 0x00414E55 */ ebx = (ebx & 0xFFFFFF00UL) | ((((ebx & 0xFFUL) + ((ebx >> 8) & 0xFFUL))) & 0xFFUL); /* add bl,bh */
    /* 0x00414E57 */ edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFUL) - (ebx & 0xFFUL)) & 0xFFUL); /* sub dl,bl */
    /* 0x00414E59 */ ecx = (unsigned long)G32(0x4299F0); /* mov ecx,dword_4299F0 */
    /* 0x00414E5F */ eax = (edx * 2UL) & U32MASK; /* lea eax,[edx*2] */
    /* 0x00414E66 */ eax = (eax + ecx) & U32MASK; /* add eax,ecx */
l_414e68:
    /* 0x00414E68 */ ecx = 0; /* xor ecx,ecx */
    /* 0x00414E6A */ ecx = (ebx >> 8) & 0xFFUL; /* mov cl,bh */
    /* 0x00414E6C */ esi = eax; /* mov esi,eax */
    /* 0x00414E6E */ ecx = (ecx + ecx) & U32MASK; /* add ecx,ecx */
    /* 0x00414E70 */ esi = (esi - ecx) & U32MASK; /* sub esi,ecx */
    /* 0x00414E72 */ ecx = (ecx & 0xFFFF0000UL) | (unsigned long)M16(esi); /* mov cx,[esi] */
    /* 0x00414E75 */ esi = (esi & 0xFFFF0000UL) | (unsigned long)M16(eax); /* mov si,[eax] */
    /* 0x00414E78 */ esi = (esi + ecx) & U32MASK; /* add esi,ecx */
    /* 0x00414E7A */ ecx = 0; /* xor ecx,ecx */
    /* 0x00414E7C */ ecx = ebx & 0xFFUL; /* mov cl,bl */
    /* 0x00414E7E */ ecx = (ecx + ecx) & U32MASK; /* add ecx,ecx */
    /* 0x00414E80 */ M16(eax) = (unsigned short)(esi & 0xFFFFUL); /* mov [eax],si */
    /* 0x00414E83 */ eax = (eax - ecx) & U32MASK; /* sub eax,ecx */
    /* 0x00414E85 */ edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFUL) - (ebx & 0xFFUL)) & 0xFFUL); /* sub dl,bl */
    /* 0x00414E87 jnz */ if ((edx & 0xFFUL) != 0UL) goto l_414e68;
    /* 0x00414E89 */ ebx = (ebx & 0xFFFF00FFUL) | (((((ebx >> 8) & 0xFFUL) * 2UL) & 0xFFUL) << 8); /* add bh,bh */
    /* 0x00414E8B cmp bh,80h */ /* 0x00414E8E jb */ if (((ebx >> 8) & 0xFFUL) < 0x80UL) goto l_414e51; /* unsigned */
    /* 0x00414E90 */ edx = a1; /* mov edx,[var_1C] */
    /* 0x00414E93 cmp edx,2FDh */ /* 0x00414E99 jnb */ if (edx >= 0x2FDUL) goto l_414eb3; /* unsigned */
    /* 0x00414E9B */ eax = (unsigned long)G32(0x4299FC); /* mov eax,dword_4299FC */
    /* 0x00414EA0 */ edi = (edi & 0xFFFF0000UL) | (unsigned long)M16((eax + edx * 2UL) & U32MASK); /* mov di,[eax+edx*2] */
    /* 0x00414EA4 */ edi = (edi & 0xFFFF0000UL) | ((edi & 0xFFFFUL) >> 2); /* shr di,2 */
    /* 0x00414EA8 */ ebx = edi; /* mov ebx,edi (full) */
    /* 0x00414EAA */ M16((eax + edx * 2UL) & U32MASK) = (unsigned short)(edi & 0xFFFFUL); /* mov [eax+edx*2],di */
    /* 0x00414EAD */ ebx = (ebx + 1UL) & U32MASK; /* inc ebx */
    /* 0x00414EAF */ M16((eax + edx * 2UL) & U32MASK) = (unsigned short)(ebx & 0xFFFFUL); /* mov [eax+edx*2],bx */
l_414eb3:
    /* epilogue: add esp,4; pop ebp,edi,esi,edx,ecx,ebx; retn */
    return;
}
/* sub_414290: input top-up / hash-table rebuild. No reg inputs (all pushed). */
void d3u_414290(dec3w_ctx *c)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    unsigned long esi = 0;
    unsigned long edi = 0;
    unsigned long ebp = 0;
    unsigned long v1c = 0;
    unsigned long v20 = 0;
    unsigned long v24 = 0;
    unsigned long v28 = 0;
    unsigned long v2c = 0;
    unsigned char stk[256] = {0};
    /* 0x00414296 */ ebp = (unsigned long)G32(0x4299B8); /* mov ebp,dword_4299B8 */
    /* 0x004142A2 */ eax = (unsigned long)G32(0x444250); /* mov eax,dword_444250 */
    /* 0x004142A7 */ v24 = eax; /* mov [var_24],eax */
    /* 0x004142AE */ eax = (unsigned long)G32(0x444220); /* mov eax,dword_444220 */
    /* 0x004142B3 */ v2c = eax; /* mov [var_2C],eax */
    /* 0x004142BA */ eax = (unsigned long)G32(0x444264); /* mov eax,dword_444264 */
    /* 0x004142BF */ edx = (unsigned long)G32(0x444274); /* mov edx,dword_444274 */
    /* 0x004142C5 */ v20 = eax; /* mov [var_20],eax */
    /* 0x004142CC */ eax = 0; /* xor eax,eax */
l_4142ce:
    /* 0x004142CE */ ebx = ebp; /* mov ebx,ebp */
    /* 0x004142D0 */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)M8((ebx + edx) & U32MASK); /* mov bl,[edx+ebx] */
    /* 0x004142D3 */ edx = (edx + 1UL) & U32MASK; /* inc edx */
    /* 0x004142D4 */ stk[eax] = (unsigned char)(ebx & 0xFFUL); /* mov [esp+eax+var_12C],bl */
    /* 0x004142D7 */ ebx = (unsigned long)G32(0x444278); /* mov ebx,dword_444278 */
    /* 0x004142DD */ eax = (eax + 1UL) & U32MASK; /* inc eax */
    /* 0x004142DE */ edx &= ebx; /* and edx,ebx */
    /* 0x004142E0 cmp eax,100h */ /* 0x004142E5 jb */ if (eax < 0x100UL) goto l_4142ce; /* unsigned */
    /* 0x004142E7 */ edx = (unsigned long)G32(0x444270); /* mov edx,dword_444270 */
    /* 0x004142ED */ eax = (unsigned long)G32(0x444274); /* mov eax,dword_444274 */
    /* 0x004142F2 */ edx = (edx - 0x140UL) & U32MASK; /* sub edx,140h */
    /* 0x004142F8 */ eax = (eax + 0x140UL) & U32MASK; /* add eax,140h */
    /* 0x004142FD */ edx >>= 1; /* shr edx,1 */
    /* 0x004142FF */ eax &= ebx; /* and eax,ebx */
    /* 0x00414301 */ edi = (eax + edx) & U32MASK; /* lea edi,[eax+edx] */
    /* 0x00414304 */ edi &= ebx; /* and edi,ebx */
    /* 0x00414306 cmp edx,dword_4441C0 */ /* 0x0041430C jbe */ if (edx > (unsigned long)G32(0x4441C0)) { /* unsigned */
    /* 0x0041430E */ edi = 0; /* xor edi,edi */
    }
l_414310:
    /* 0x00414310 */ ecx = (unsigned long)G32(0x444274); /* mov ecx,dword_444274 */
    /* 0x00414316 */ edx &= 0xFFFFFF00UL; /* xor dl,dl */
    /* 0x00414318 */ v28 = eax; /* mov [var_28],eax */
    /* 0x0041431F */ G8(0x444EA7) = (unsigned char)(edx & 0xFFUL); /* mov byte_444EA7,dl (=0) */
    /* 0x00414325 cmp edi,ecx */ /* 0x00414327 jz */ if (edi == ecx) goto l_414463;
l_41432d:
    /* 0x0041432D */ ebx = ebp; /* mov ebx,ebp */
    /* 0x0041432F */ ecx = (eax + 1UL) & U32MASK; /* lea ecx,[eax+1] */
    /* 0x00414332 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8((edi + ebx) & U32MASK); /* mov dl,[edi+ebx] */
    /* 0x00414335 */ ebx = (eax + ebp) & U32MASK; /* lea ebx,[eax+ebp] */
    /* 0x00414338 */ edx = (edx & 0xFFFFFF00UL) | ((~edx) & 0xFFUL); /* not dl */
    /* 0x0041433A */ v1c = ebx; /* mov [var_1C],ebx */
    /* 0x00414341 */ ebx = (unsigned long)G32(0x444278); /* mov ebx,dword_444278 */
    /* 0x00414347 */ edx = (edx & 0xFFFF00FFUL) | ((unsigned long)G8(0x444EA7) << 8); /* mov dh,byte_444EA7 */
    /* 0x0041434D */ ecx &= ebx; /* and ecx,ebx */
    /* 0x0041434F cmp dh,1 */ /* 0x00414352 jnz */ if (((edx >> 8) & 0xFFUL) != 1UL) goto l_41437d;
    /* 0x00414354 */ ebx &= 0xFFFFFF00UL; /* xor bl,bl */
    /* 0x00414356 */ G8(0x444EA7) = (unsigned char)(ebx & 0xFFUL); /* mov byte_444EA7,bl (=0) */
    /* 0x0041435C */ ebx = 0; /* xor ebx,ebx */
    /* 0x0041435E */ ebx = (unsigned long)G8(0x444EAD); /* mov bl,byte_444EAD */
    /* 0x00414364 */ ebx = (ebx - 0xA6UL) & U32MASK; /* sub ebx,0A6h */
    /* 0x0041436A */ ebx = (ebx * 11UL) & U32MASK; /* imul ebx,0Bh (mod 2^32) */
    /* 0x0041436D */ eax = 0; /* xor eax,eax */
    /* 0x0041436F */ eax = edx & 0xFFUL; /* mov al,dl */
    /* 0x00414371 */ eax = (unsigned long)G8((0x42C492UL + ebx + eax) & U32MASK); /* mov al,byte_42C492[ebx+eax] */
    /* 0x00414378 jmp */ goto l_41443b;
l_41437d:
    /* 0x0041437D cmp dl,80h */ /* 0x00414380 jnb */ if ((edx & 0xFFUL) >= 0x80UL) goto l_41438a; /* unsigned */
    /* 0x00414382 test dh,dh */ /* 0x00414384 jz */ if (((edx >> 8) & 0xFFUL) == 0UL) goto l_414439;
l_41438a:
    /* 0x0041438A */ ebx = 0; /* xor ebx,ebx */
    /* 0x0041438C */ ebx = edx & 0xFFUL; /* mov bl,dl */
    /* 0x0041438E */ esi = (unsigned long)G32(0x444278); /* mov esi,dword_444278 */
    /* 0x00414394 */ ecx = (eax + 1UL) & U32MASK; /* lea ecx,[eax+1] */
    /* 0x00414397 */ edx = (edx & 0xFFFF00FFUL) | ((unsigned long)G8(0x42C438UL + ebx) << 8); /* mov dh,byte_42C438[ebx] */
    /* 0x0041439D */ ecx &= esi; /* and ecx,esi */
    /* 0x0041439F cmp dh,0C8h */ /* 0x004143A2 jnb */ if (((edx >> 8) & 0xFFUL) >= 0xC8UL) goto l_414420; /* unsigned */
    /* 0x004143A8 cmp dh,64h */ /* 0x004143AB jnz */ if (((edx >> 8) & 0xFFUL) != 0x64UL) goto l_4143bf;
    /* 0x004143AD */ G8(0x444EA7) = 1; /* mov byte_444EA7,1 */
    /* 0x004143B4 */ G8(0x444EAD) = (unsigned char)(edx & 0xFFUL); /* mov byte_444EAD,dl */
    /* 0x004143BA jmp */ goto l_414448;
l_4143bf:
    /* 0x004143BF cmp dh,63h */ /* 0x004143C2 jnz */ if (((edx >> 8) & 0xFFUL) != 0x63UL) goto l_4143d0;
    /* 0x004143C4 */ G8(0x444EA7) = 0x20; /* mov byte_444EA7,20h */
    /* 0x004143CB jmp */ goto l_414448;
l_4143d0:
    /* 0x004143D0 */ ebx = 0; /* xor ebx,ebx */
    /* 0x004143D2 */ ebx = (edx >> 8) & 0xFFUL; /* mov bl,dh */
    /* 0x004143D4 */ edx = (unsigned long)G32((0x42BDDCUL + ebx * 4UL) & U32MASK); /* mov edx,off_42BDDC[ebx*4] */
    /* 0x004143DB */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8((0x42BF6CUL + ebx) & U32MASK); /* mov bl,byte_42BF6C[ebx] */
    /* 0x004143E1 cmp byte_444EA7,0 */ /* 0x004143E8 jz */ if (G8(0x444EA7) == 0) goto l_414402;
    /* 0x004143EA */ esi = edx; /* mov esi,edx */
    /* 0x004143EC */ ebx = (ebx & 0xFFFF00FFUL) | ((unsigned long)M8(esi) << 8); /* mov bh,[esi] */
    /* 0x004143EE */ eax = (eax + ebp) & U32MASK; /* add eax,ebp */
    /* 0x004143F0 */ ebx = (ebx & 0xFFFF00FFUL) | (((((ebx >> 8) & 0xFFUL) + 0x20UL) & 0xFFUL) << 8); /* add bh,20h */
    /* 0x004143F3 */ edx = (edx + 1UL) & U32MASK; /* inc edx */
    /* 0x004143F4 */ M8(eax) = (unsigned char)((ebx >> 8) & 0xFFUL); /* mov [eax],bh */
    /* 0x004143F6 */ eax = ecx; /* mov eax,ecx */
    /* 0x004143F8 */ ecx &= 0xFFFFFF00UL; /* xor cl,cl */
    /* 0x004143FA */ ebx = (ebx & 0xFFFFFF00UL) | ((ebx - 1UL) & 0xFFUL); /* dec bl */
    /* 0x004143FC */ G8(0x444EA7) = (unsigned char)(ecx & 0xFFUL); /* mov byte_444EA7,cl (=0) */
l_414402:
    /* 0x00414402 */ esi = ebp; /* mov esi,ebp */
l_414404:
    /* 0x00414404 */ ecx = edx; /* mov ecx,edx */
    /* 0x00414406 */ eax = (eax + 1UL) & U32MASK; /* inc eax */
    /* 0x00414407 */ ebx = (ebx & 0xFFFF00FFUL) | ((unsigned long)M8(ecx) << 8); /* mov bh,[ecx] */
    /* 0x00414409 */ ecx = (unsigned long)G32(0x444278); /* mov ecx,dword_444278 */
    /* 0x0041440F */ M8((esi + eax - 1UL) & U32MASK) = (unsigned char)((ebx >> 8) & 0xFFUL); /* mov [esi+eax-1],bh */
    /* 0x00414413 */ eax &= ecx; /* and eax,ecx */
    /* 0x00414415 */ edx = (edx + 1UL) & U32MASK; /* inc edx */
    /* 0x00414416 cmp edi,eax */ /* 0x00414418 jz */ if (edi == eax) goto l_414448;
    /* 0x0041441A */ ebx = (ebx & 0xFFFFFF00UL) | ((ebx - 1UL) & 0xFFUL); /* dec bl */
    /* 0x0041441C jnz */ if ((ebx & 0xFFUL) != 0UL) goto l_414404;
    /* 0x0041441E jmp */ goto l_414448;
l_414420:
    /* 0x00414420 */ ebx = (ebx & 0xFFFFFF00UL) | ((ebx - (unsigned long)G8(0x444EA7)) & 0xFFUL); /* sub bl,byte_444EA7 */
    /* 0x00414426 */ edx = ebp; /* mov edx,ebp */
    /* 0x00414428 */ ebx = (ebx & 0xFFFFFF00UL) | ((~ebx) & 0xFFUL); /* not bl */
    /* 0x0041442A */ M8((edx + eax) & U32MASK) = (unsigned char)(ebx & 0xFFUL); /* mov [edx+eax],bl */
    /* 0x0041442D */ eax = ecx; /* mov eax,ecx */
    /* 0x0041442F */ ecx &= 0xFFFFFF00UL; /* xor cl,cl */
    /* 0x00414431 */ G8(0x444EA7) = (unsigned char)(ecx & 0xFFUL); /* mov byte_444EA7,cl (=0) */
    /* 0x00414437 jmp */ goto l_414448;
l_414439:
    /* 0x00414439 */ eax = (eax & 0xFFFFFF00UL) | (edx & 0xFFUL); /* mov al,dl */
l_41443b:
    /* 0x0041443B */ edx = v1c; /* mov edx,[var_1C] */
    /* 0x00414442 */ eax = (eax & 0xFFFFFF00UL) | ((~eax) & 0xFFUL); /* not al */
    /* 0x00414444 */ M8(edx) = (unsigned char)(eax & 0xFFUL); /* mov [edx],al */
    /* 0x00414446 */ eax = ecx; /* mov eax,ecx */
l_414448:
    /* 0x00414448 cmp edi,eax */ /* 0x0041444A jz */ if (edi == eax) goto l_414463;
    /* 0x0041444C */ esi = (unsigned long)G32(0x444278); /* mov esi,dword_444278 */
    /* 0x00414452 */ edi = (edi + 1UL) & U32MASK; /* inc edi */
    /* 0x00414453 */ edx = (unsigned long)G32(0x444274); /* mov edx,dword_444274 */
    /* 0x00414459 */ edi &= esi; /* and edi,esi */
    /* 0x0041445B cmp edi,edx */ /* 0x0041445D jnz */ if (edi != edx) goto l_41432d;
l_414463:
    /* 0x00414463 */ edx = (unsigned long)G32(0x444280); /* mov edx,dword_444280 */
    /* 0x00414469 */ ebx &= 0xFFFF00FFUL; /* xor bh,bh */
    /* 0x0041446B */ esi = (unsigned long)G32(0x444274); /* mov esi,dword_444274 */
    /* 0x00414471 */ G8(0x444EA7) = (unsigned char)((ebx >> 8) & 0xFFUL); /* mov byte_444EA7,bh (=0) */
    /* 0x00414477 */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x00414479 */ ebx = (unsigned long)G32(0x444278); /* mov ebx,dword_444278 */
    /* 0x0041447F */ edx = (edx - esi) & U32MASK; /* sub edx,esi */
    /* 0x00414481 */ edi = 1; /* mov edi,1 */
    /* 0x00414486 */ edx &= ebx; /* and edx,ebx */
    /* 0x00414488 */ G32(0x444274) = (unsigned int)eax; /* mov dword_444274,eax */
    /* 0x0041448D */ G32(0x444280) = (unsigned int)edx; /* mov dword_444280,edx */
    /* 0x00414493 */ edx = 0; /* xor edx,edx */
l_414495:
    /* 0x00414495 */ ebx = eax; /* mov ebx,eax */
    /* 0x00414497 */ ebx &= 0xFFUL; /* and ebx,0FFh */
    /* 0x0041449D */ ecx = (eax + ebp) & U32MASK; /* lea ecx,[eax+ebp] */
    /* 0x004144A0 */ G32((0x443D08UL + ebx * 4UL) & U32MASK) = (unsigned int)edi; /* mov dword_443D08[ebx*4],edi */
    /* 0x004144A7 */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)stk[edx]; /* mov bl,[esp+edx+var_12C] */
    /* 0x004144AA */ eax = (eax + edi) & U32MASK; /* add eax,edi */
    /* 0x004144AC */ M8(ecx) = (unsigned char)(ebx & 0xFFUL); /* mov [ecx],bl */
    /* 0x004144AE */ ebx = (unsigned long)G32(0x444278); /* mov ebx,dword_444278 */
    /* 0x004144B4 */ edx = (edx + edi) & U32MASK; /* add edx,edi */
    /* 0x004144B6 */ eax &= ebx; /* and eax,ebx */
    /* 0x004144B8 cmp edx,100h */ /* 0x004144BE jb */ if (edx < 0x100UL) goto l_414495; /* unsigned */
    /* 0x004144C0 */ eax = v28; /* mov eax,[var_28] */
    /* 0x004144C7 */ edx = 0x40UL; /* mov edx,40h */
    /* 0x004144CC */ G32(0x4299B8) = (unsigned int)ebp; /* mov dword_4299B8,ebp */
l_4144d2:
    /* 0x004144D2 */ ecx = (unsigned long)G32(0x444278); /* mov ecx,dword_444278 */
    /* 0x004144D8 */ ebp = (unsigned long)G32(0x4299B8); /* mov ebp,dword_4299B8 */
    /* 0x004144DE */ eax = (eax - 1UL) & U32MASK; /* dec eax */
    /* 0x004144DF */ esi = ebp; /* mov esi,ebp */
    /* 0x004144E1 */ eax &= ecx; /* and eax,ecx */
    /* 0x004144E3 */ M8((esi + eax) & U32MASK) = 0; /* mov byte ptr [esi+eax],0 */
    /* 0x004144E7 */ edx = (edx - 1UL) & U32MASK; /* dec edx */
    /* 0x004144E8 jnz */ if (edx != 0UL) goto l_4144d2;
    /* 0x004144EA */ ecx = 0x100UL; /* mov ecx,100h */
    /* 0x004144EF */ edi = (unsigned long)G32(0x444270); /* mov edi,dword_444270 */
    /* 0x004144F5 */ ebx = 0x200000UL; /* mov ebx,200000h */
    /* 0x004144FA */ edi = (edi + ebp) & U32MASK; /* add edi,ebp */
    /* 0x004144FC */ edx = 0xFFUL; /* mov edx,0FFh */
    /* 0x00414501 cld */;
    /* esi is stale = ebp here (no esi write since zero loop); 256B copy esi->edi */
    /* 0x00414502 */ eax = ecx; /* mov eax,ecx */
    /* 0x00414504 */ ecx = (ecx - edi) & U32MASK; /* sub ecx,edi */
    /* 0x00414506 */ ecx = (ecx - eax) & U32MASK; /* sub ecx,eax */
    /* 0x00414508 */ ecx &= 3UL; /* and ecx,3 */
    /* 0x0041450B */ eax = (eax - ecx) & U32MASK; /* sub eax,ecx */
    /* 0x0041450D jle */ if ((int)(eax & U32MASK) > 0) { /* signed */
        /* 0x0041450F rep movsb */
        while (ecx != 0) { M8(edi) = M8(esi); esi = (esi + 1UL) & U32MASK; edi = (edi + 1UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
        /* 0x00414511 */ ecx = eax; /* mov ecx,eax */
        /* 0x00414513 */ eax &= 3UL; /* and eax,3 */
        /* 0x00414516 */ ecx >>= 2; /* shr ecx,2 */
        /* 0x00414519 rep movsd */
        while (ecx != 0) { M32(edi) = M32(esi); esi = (esi + 4UL) & U32MASK; edi = (edi + 4UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
    }
l_41451b:
    /* 0x0041451B */ ecx = (ecx + eax) & U32MASK; /* add ecx,eax */
    /* 0x0041451D rep movsb */
    while (ecx != 0) { M8(edi) = M8(esi); esi = (esi + 1UL) & U32MASK; edi = (edi + 1UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
    /* 0x0041451F */ d3u_4141d4(c); /* call sub_4141D4 */
    /* 0x00414524 */ d3u_41422c(c); /* call sub_41422C */
    /* 0x00414529 */ eax = (unsigned long)G32(0x4299C4); /* mov eax,dword_4299C4 */
    /* 0x0041452E */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* call sub_40C9C0: len=200000h dst=4299C4 fill=FFh */
    /* 0x00414533 */ esi = 0; /* xor esi,esi */
    /* 0x00414535 */ ebx = 0x200000UL; /* mov ebx,200000h */
    /* 0x0041453A */ edx = 0xFFUL; /* mov edx,0FFh */
    /* 0x0041453F */ eax = (unsigned long)G32(0x4299C8); /* mov eax,dword_4299C8 */
    /* 0x00414544 */ ecx &= 0xFFFFFF00UL; /* xor cl,cl (ecx==0 here) */
    /* 0x00414546 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* call sub_40C9C0: len=200000h dst=4299C8 fill=FFh */
    /* 0x0041454B */ eax = (unsigned long)G32(0x444274); /* mov eax,dword_444274 */
    /* 0x00414550 */ edx = (unsigned long)G32(0x444270); /* mov edx,dword_444270 */
    /* 0x00414556 */ G32(0x444258) = (unsigned int)esi; /* mov dword_444258,esi (=0) */
    /* 0x0041455C */ G32(0x44425C) = (unsigned int)esi; /* mov dword_44425C,esi (=0) */
    /* 0x00414562 */ G32(0x444260) = (unsigned int)esi; /* mov dword_444260,esi (=0) */
    /* 0x00414568 */ G8(0x444EB0) = (unsigned char)(ecx & 0xFFUL); /* mov byte_444EB0,cl (=0) */
    /* 0x0041456E */ ecx &= 0xFFFF00FFUL; /* xor ch,ch */
    /* 0x00414570 */ eax = (eax + edx) & U32MASK; /* add eax,edx */
    /* 0x00414572 */ G8(0x444EB1) = (unsigned char)((ecx >> 8) & 0xFFUL); /* mov byte_444EB1,ch (=0) */
    /* 0x00414578 */ ecx = v28; /* mov ecx,[var_28] */
    /* 0x0041457F */ esi = (unsigned long)G32(0x444278); /* mov esi,dword_444278 */
    /* 0x00414585 */ eax = (eax - ecx) & U32MASK; /* sub eax,ecx */
    /* 0x00414587 */ eax &= esi; /* and eax,esi */
    /* 0x00414589 cmp eax,8000h */ /* 0x0041458E jbe */ if (eax > 0x8000UL) { /* unsigned */
    /* 0x00414590 */ esi = (unsigned long)G32(0x444274); /* mov esi,dword_444274 */
    /* 0x00414596 */ edi = (unsigned long)G32(0x444278); /* mov edi,dword_444278 */
    /* 0x0041459C */ esi = (esi + 0xFFFF8000UL) & U32MASK; /* add esi,0FFFF8000h (-8000h) */
    /* 0x004145A2 */ ebp = (unsigned long)G32(0x4299B8); /* mov ebp,dword_4299B8 */
    /* 0x004145A8 */ esi &= edi; /* and esi,edi */
    /* 0x004145AA jmp */ goto l_4145b7;
    }
    /* 0x004145AC */ esi = 0xFFFFFFFFUL; /* mov esi,0FFFFFFFFh */
    /* 0x004145B1 */ ebp = (unsigned long)G32(0x4299B8); /* mov ebp,dword_4299B8 */
l_4145b7:
    /* 0x004145B7 */ eax = (unsigned long)G32(0x444274); /* mov eax,dword_444274 */
    /* 0x004145BC */ G32(0x4299B8) = (unsigned int)ebp; /* mov dword_4299B8,ebp */
    /* 0x004145C2 cmp ecx,eax */ /* 0x004145C4 jz */ if (ecx == eax) goto l_414769;
    /* 0x004145CA */ eax = (unsigned long)G32(0x444258); /* mov eax,dword_444258 */
    /* 0x004145CF */ edx = (unsigned long)G32(0x444258); /* mov edx,dword_444258 */
    /* 0x004145D5 */ eax = (eax >> 13) & U32MASK; /* shr eax,0Dh */
    /* 0x004145D8 */ eax ^= edx; /* xor eax,edx */
    /* 0x004145DA */ eax &= 0x3FFFFUL; /* and eax,3FFFFh */
    /* 0x004145DF */ edx = (unsigned long)G32(0x4299C4); /* mov edx,dword_4299C4 */
    /* 0x004145E5 */ eax = (eax << 3) & U32MASK; /* shl eax,3 */
    /* 0x004145E8 */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x004145EA */ eax = (unsigned long)G32(0x44425C); /* mov eax,dword_44425C */
    /* 0x004145EF */ ebx = (unsigned long)G32(0x44425C); /* mov ebx,dword_44425C */
    /* 0x004145F5 */ eax = (eax >> 15) & U32MASK; /* shr eax,0Fh */
    /* 0x004145F8 */ ebx ^= eax; /* xor ebx,eax */
    /* 0x004145FA */ eax = (unsigned long)G32(0x444258); /* mov eax,dword_444258 */
    /* 0x004145FF */ edi = (unsigned long)G32(0x444258); /* mov edi,dword_444258 */
    /* 0x00414605 */ eax = (eax >> 15) & U32MASK; /* shr eax,0Fh */
    /* 0x00414608 */ ebx = (ebx << 4) & U32MASK; /* shl ebx,4 */
    /* 0x0041460B */ eax ^= edi; /* xor eax,edi */
    /* 0x0041460D */ ebx ^= eax; /* xor ebx,eax */
    /* 0x0041460F */ eax = (unsigned long)G32(0x444260); /* mov eax,dword_444260 */
    /* 0x00414614 */ edi = (unsigned long)G32(0x444260); /* mov edi,dword_444260 */
    /* 0x0041461A */ eax = (eax >> 15) & U32MASK; /* shr eax,0Fh */
    /* 0x0041461D */ eax ^= edi; /* xor eax,edi */
    /* 0x0041461F */ eax = (eax << 6) & U32MASK; /* shl eax,6 */
    /* 0x00414622 */ ebx ^= eax; /* xor ebx,eax */
    /* 0x00414624 */ ebx &= 0x3FFFFUL; /* and ebx,3FFFFh */
    /* 0x0041462A */ eax = (ebx * 8UL) & U32MASK; /* lea eax,[ebx*8] */
    /* 0x00414631 */ ebx = (unsigned long)G32(0x4299C8); /* mov ebx,dword_4299C8 */
    /* 0x00414637 */ eax = (eax + 4UL) & U32MASK; /* add eax,4 */
    /* 0x0041463A */ eax = (eax + ebx) & U32MASK; /* add eax,ebx */
    /* 0x0041463C */ ebx = (unsigned long)M32((eax - 4UL) & U32MASK); /* mov ebx,[eax-4] */
    /* 0x0041463F */ M32(eax) = (unsigned int)ebx; /* mov [eax],ebx */
    /* 0x00414641 */ M32((eax - 4UL) & U32MASK) = (unsigned int)ecx; /* mov [eax-4],ecx */
    /* 0x00414644 */ eax = (unsigned long)G32(0x444258); /* mov eax,dword_444258 */
    /* 0x00414649 */ M32(edx) = (unsigned int)ecx; /* mov [edx],ecx */
    /* 0x0041464B */ M32((edx + 4UL) & U32MASK) = (unsigned int)eax; /* mov [edx+4],eax */
    /* 0x0041464E cmp ecx,esi */ /* 0x00414650 jnz */ if (ecx != esi) goto l_414657;
    /* 0x00414652 */ esi = 0xFFFFFFFFUL; /* mov esi,0FFFFFFFFh */
l_414657:
    /* 0x00414657 cmp esi,0FFFFFFFFh */ /* 0x0041465A jnz */ if (esi != 0xFFFFFFFFUL) goto l_414711;
    /* 0x00414660 */ eax = (unsigned long)G32(0x444258); /* mov eax,dword_444258 */
    /* 0x00414665 */ edx = (unsigned long)G32(0x444258); /* mov edx,dword_444258 */
    /* 0x0041466B */ G32(0x444240) = (unsigned int)eax; /* mov dword_444240,eax */
    /* 0x00414670 */ eax = (eax >> 13) & U32MASK; /* shr eax,0Dh */
    /* 0x00414673 */ eax ^= edx; /* xor eax,edx */
    /* 0x00414675 */ eax &= 0x7FFFUL; /* and eax,7FFFh */
    /* 0x0041467A */ G32(0x444240) = (unsigned int)edx; /* mov dword_444240,edx */
    /* 0x00414680 */ G32(0x444224) = (unsigned int)eax; /* mov dword_444224,eax */
    /* 0x00414685 cmp eax,40000h */ /* 0x0041468A jnb */ if (eax >= 0x40000UL) goto l_4146f6; /* unsigned; DEAD: eax<=7FFFh after and */
    /* 0x0041468C */ eax = (eax << 5) & U32MASK; /* shl eax,5 */
l_41468f:
    /* 0x0041468F */ edx = (unsigned long)G32(0x4299BC); /* mov edx,dword_4299BC */
    /* 0x00414695 */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x00414697 cmp byte ptr [edx],0 */ /* 0x0041469A jz */ if (M8(edx) == 0) goto l_4146d7;
    /* 0x0041469C */ edi = (unsigned long)G32(0x444240); /* mov edi,dword_444240 */
    /* 0x004146A2 cmp edi,[edx+1Ch] */ /* 0x004146A5 jnz */ if (edi != (unsigned long)M32((edx + 0x1CUL) & U32MASK)) goto l_4146d7;
    /* 0x004146A7 */ eax = 0; /* xor eax,eax */
    /* 0x004146A9 */ ebx = (ebx & 0xFFFF00FFUL) | 0xFF00UL; /* mov bh,0FFh (lane) */
    /* 0x004146AB */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8(edx); /* mov al,[edx] (bh=FFh lane kept) */
    /* 0x004146AD */ G8(0x444EB7) = (unsigned char)((ebx >> 8) & 0xFFUL); /* mov byte_444EB7,bh (=FFh) */
    /* 0x004146B3 */ G32(0x444214) = (unsigned int)eax; /* mov dword_444214,eax (eax=FF00h|byte) */
    /* 0x004146B8 */ eax = edx; /* mov eax,edx */
    /* 0x004146BA */ ebx = (edx + 0xDUL) & U32MASK; /* lea ebx,[edx+0Dh] */
l_4146bd:
    /* 0x004146BD */ edx = 0; /* xor edx,edx */
    /* 0x004146BF */ edi = (unsigned long)G32(0x444214); /* mov edi,dword_444214 */
    /* 0x004146C5 */ edx = (unsigned long)M8((eax + 1UL) & U32MASK); /* mov dl,[eax+1] */
    /* 0x004146C8 */ edi = (edi + edx) & U32MASK; /* add edi,edx */
    /* 0x004146CA */ eax = (eax + 1UL) & U32MASK; /* inc eax */
    /* 0x004146CB */ G32(0x444214) = (unsigned int)edi; /* mov dword_444214,edi */
    /* 0x004146D1 cmp eax,ebx */ /* 0x004146D3 jz */ if (eax == ebx) goto l_4146f6;
    /* 0x004146D5 jmp */ goto l_4146bd;
l_4146d7:
    /* 0x004146D7 */ edi = (unsigned long)G32(0x444224); /* mov edi,dword_444224 */
    /* 0x004146DD */ edi = (edi + 0x8000UL) & U32MASK; /* add edi,8000h */
    /* 0x004146E3 */ eax = (eax + 0x100000UL) & U32MASK; /* add eax,100000h */
    /* 0x004146E8 */ G32(0x444224) = (unsigned int)edi; /* mov dword_444224,edi */
    /* 0x004146EE cmp edi,40000h */ /* 0x004146F4 jb */ if (edi < 0x40000UL) goto l_41468f; /* unsigned */
l_4146f6:
    /* 0x004146F6 */ eax = ebp; /* mov eax,ebp */
    /* 0x004146F8 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((ecx + eax) & U32MASK); /* mov al,[ecx+eax] */
    /* 0x004146FB */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00414700 */ G32(0x4299B8) = (unsigned int)ebp; /* mov dword_4299B8,ebp */
    /* 0x00414706 */ d3u_415534(c, (unsigned int)eax); /* call sub_415534 (al in) */
l_414711:
    /* 0x00414711 */ eax = (unsigned long)G32(0x444260); /* mov eax,dword_444260 */
    /* 0x00414716 */ edx = (unsigned long)G32(0x44425C); /* mov edx,dword_44425C */
    /* 0x0041471C */ eax = (eax << 8) & U32MASK; /* shl eax,8 */
    /* 0x0041471F */ edx = (edx >> 24) & U32MASK; /* shr edx,18h */
    /* 0x00414722 */ eax = (eax + edx) & U32MASK; /* add eax,edx */
    /* 0x00414724 */ edx = (unsigned long)G32(0x444258); /* mov edx,dword_444258 */
    /* 0x0041472A */ G32(0x444260) = (unsigned int)eax; /* mov dword_444260,eax */
    /* 0x0041472F */ eax = (unsigned long)G32(0x44425C); /* mov eax,dword_44425C */
    /* 0x00414734 */ edx = (edx >> 24) & U32MASK; /* shr edx,18h */
    /* 0x00414737 */ eax = (eax << 8) & U32MASK; /* shl eax,8 */
    /* 0x0041473A */ eax = (eax + edx) & U32MASK; /* add eax,edx */
    /* 0x0041473C */ G32(0x44425C) = (unsigned int)eax; /* mov dword_44425C,eax */
    /* 0x00414741 */ eax = ebp; /* mov eax,ebp */
    /* 0x00414743 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((ecx + eax) & U32MASK); /* mov al,[ecx+eax] */
    /* 0x00414746 */ edx = (unsigned long)G32(0x444258); /* mov edx,dword_444258 */
    /* 0x0041474C */ eax &= 0xFFUL; /* and eax,0FFh */
    /* 0x00414751 */ edx = (edx << 8) & U32MASK; /* shl edx,8 */
    /* 0x00414754 */ ecx = (ecx + 1UL) & U32MASK; /* inc ecx */
    /* 0x00414755 */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x00414757 */ eax = (unsigned long)G32(0x444278); /* mov eax,dword_444278 */
    /* 0x0041475C */ G32(0x444258) = (unsigned int)edx; /* mov dword_444258,edx */
    /* 0x00414762 */ ecx &= eax; /* and ecx,eax */
    /* 0x00414764 jmp */ goto l_4145b7;
l_414769:
    /* 0x00414769 */ eax = v24; /* mov eax,[var_24] */
    /* 0x00414770 */ G32(0x444250) = (unsigned int)eax; /* mov dword_444250,eax */
    /* 0x00414775 */ eax = v2c; /* mov eax,[var_2C] */
    /* 0x0041477C */ G32(0x444220) = (unsigned int)eax; /* mov dword_444220,eax */
    /* 0x00414781 */ eax = v20; /* mov eax,[var_20] */
    /* 0x00414788 */ ebx = 0x400UL; /* mov ebx,400h */
    /* 0x0041478D */ G32(0x444264) = (unsigned int)eax; /* mov dword_444264,eax */
    /* 0x00414792 */ eax = (unsigned long)G32(0x4299F0); /* mov eax,dword_4299F0 */
    /* 0x00414797 */ edx = 0; /* xor edx,edx */
    /* 0x00414799 */ eax = (eax + 0x5F600UL) & U32MASK; /* add eax,5F600h */
    /* 0x0041479E */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* call sub_40C9C0: len=400h dst=4299F0+5F600h fill=0 */
    /* 0x004147A3 */ ebx = 0x400UL; /* mov ebx,400h */
    /* 0x004147A8 */ eax = (unsigned long)G32(0x4299F4); /* mov eax,dword_4299F4 */
    /* 0x004147AD */ edx = 0; /* xor edx,edx */
    /* 0x004147AF */ eax = (eax + 0x5F600UL) & U32MASK; /* add eax,5F600h */
    /* 0x004147B4 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* call sub_40C9C0: len=400h dst=4299F4+5F600h fill=0 */
    /* 0x004147B9 */ eax = (unsigned long)G32(0x4299FC); /* mov eax,dword_4299FC */
    /* 0x004147BE */ M16((eax + 0x5F6UL) & U32MASK) = (unsigned short)1; /* mov word ptr [eax+5F6h],1 */
    /* 0x004147C7 */ M16((eax + 0x5F8UL) & U32MASK) = (unsigned short)1; /* mov word ptr [eax+5F8h],1 */
    /* 0x004147D0 cmp byte_444E99,1 */ /* 0x004147D7 jnz */ if (G8(0x444E99) != 1) goto l_4147e0;
    /* 0x004147D9 */ eax = (unsigned long)G32(0x444268); /* mov eax,dword_444268 */
    /* 0x004147DE jmp */ goto l_4147e5;
l_4147e0:
    /* 0x004147E0 */ eax = (unsigned long)G32(0x444EF8); /* mov eax,dword_444EF8 */
l_4147e5:
    /* 0x004147E5 */ ebp = (unsigned long)G32(0x4299B8); /* mov ebp,dword_4299B8 */
    /* 0x004147EB */ G32(0x444230) = (unsigned int)eax; /* mov dword_444230,eax */
    /* 0x004147F0 */ edx = 0xFFFFFFFFUL; /* mov edx,0FFFFFFFFh */
    /* 0x004147F5 */ ecx = (ecx & 0xFFFFFF00UL) | 1UL; /* mov cl,1 */
    /* 0x004147F7 */ G32(0x444254) = (unsigned int)edx; /* mov dword_444254,edx */
    /* 0x004147FD */ G8(0x444EA0) = (unsigned char)(ecx & 0xFFUL); /* mov byte_444EA0,cl (=1) */
    /* 0x00414803 */ G32(0x4299B8) = (unsigned int)ebp; /* mov dword_4299B8,ebp */
    /* epilogue: add esp,114h; pop ebp,edi,esi,edx,ecx,ebx; retn */
}
/* sub_414818: model-table re-init. No reg inputs (all pushed). */
void d3u_414818(dec3w_ctx *c)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    unsigned long esi = 0;
    unsigned long edi = 0;
    unsigned long ebp = 0;
    unsigned long v1c = 0;
    /* 0x00414821 */ ebp = (unsigned long)G32(0x44426C); /* mov ebp,dword_44426C */
    /* 0x00414827 */ ebx = 0x20UL; /* mov ebx,20h */
    /* 0x0041482C */ edx = 1; /* mov edx,1 */
    /* 0x00414831 */ eax = 0x444108UL; /* mov eax,offset dword_444108 */
    /* 0x00414836 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=20h dst=444108h fill=1 */
    /* 0x0041483B */ ebx = 0x20UL; /* mov ebx,20h */
    /* 0x00414840 */ edx = 1; /* mov edx,1 */
    /* 0x00414845 */ eax = 0x444128UL; /* mov eax,offset dword_444128 */
    /* 0x0041484A */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=20h dst=444128h fill=1 */
    /* 0x0041484F */ ebx = 0x40UL; /* mov ebx,40h */
    /* 0x00414854 */ edx = 1; /* mov edx,1 */
    /* 0x00414859 */ eax = 0x444148UL; /* mov eax,offset dword_444148 */
    /* 0x0041485E */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=40h dst=444148h fill=1 */
    /* 0x00414863 */ ebx = 0x40000UL; /* mov ebx,40000h */
    /* 0x00414868 */ eax = (unsigned long)G32(0x4299D0); /* mov eax,dword_4299D0 */
    /* 0x0041486D */ edx = 0; /* xor edx,edx */
    /* 0x0041486F */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=40000h dst=4299D0 fill=0 */
    /* 0x00414874 */ ebx = 0x80000UL; /* mov ebx,80000h */
    /* 0x00414879 */ eax = (unsigned long)G32(0x4299D4); /* mov eax,dword_4299D4 */
    /* 0x0041487E */ edx = 0; /* xor edx,edx */
    /* 0x00414880 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=80000h dst=4299D4 fill=0 */
    /* 0x00414885 */ ebx = 0x80000UL; /* mov ebx,80000h */
    /* 0x0041488A */ eax = (unsigned long)G32(0x4299D8); /* mov eax,dword_4299D8 */
    /* 0x0041488F */ edx = 0; /* xor edx,edx */
    /* 0x00414891 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=80000h dst=4299D8 fill=0 */
    /* 0x00414896 */ ebx = 0x40000UL; /* mov ebx,40000h */
    /* 0x0041489B */ eax = (unsigned long)G32(0x4299DC); /* mov eax,dword_4299DC */
    /* 0x004148A0 */ edx = 0; /* xor edx,edx */
    /* 0x004148A2 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=40000h dst=4299DC fill=0 */
    /* 0x004148A7 */ ebx = 0x40000UL; /* mov ebx,40000h */
    /* 0x004148AC */ eax = (unsigned long)G32(0x4299E0); /* mov eax,dword_4299E0 */
    /* 0x004148B1 */ edx = 0; /* xor edx,edx */
    /* 0x004148B3 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=40000h dst=4299E0 fill=0 */
    /* 0x004148B8 */ ebx = 0x40000UL; /* mov ebx,40000h */
    /* 0x004148BD */ eax = (unsigned long)G32(0x4299E4); /* mov eax,dword_4299E4 */
    /* 0x004148C2 */ edx = 0; /* xor edx,edx */
    /* 0x004148C4 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=40000h dst=4299E4 fill=0 */
    /* 0x004148C9 */ ebx = 0x1C0000UL; /* mov ebx,1C0000h */
    /* 0x004148CE */ eax = (unsigned long)G32(0x4299E8); /* mov eax,dword_4299E8 */
    /* 0x004148D3 */ edx = 0; /* xor edx,edx */
    /* 0x004148D5 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=1C0000h dst=4299E8 fill=0 */
    /* 0x004148DA */ ebx = 0x40000UL; /* mov ebx,40000h */
    /* 0x004148DF */ eax = (unsigned long)G32(0x4299EC); /* mov eax,dword_4299EC */
    /* 0x004148E4 */ edx = 0; /* xor edx,edx */
    /* 0x004148E6 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=40000h dst=4299EC fill=0 */
    /* 0x004148EB */ ebx = 0x40600UL; /* mov ebx,40600h */
    /* 0x004148F0 */ eax = (unsigned long)G32(0x4299F0); /* mov eax,dword_4299F0 */
    /* 0x004148F5 */ edx = 0; /* xor edx,edx */
    /* 0x004148F7 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=40600h dst=4299F0 fill=0 */
    /* 0x004148FC */ ebx = 0x40600UL; /* mov ebx,40600h */
    /* 0x00414901 */ eax = (unsigned long)G32(0x4299F4); /* mov eax,dword_4299F4 */
    /* 0x00414906 */ edx = 0; /* xor edx,edx */
    /* 0x00414908 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=40600h dst=4299F4 fill=0 */
    /* 0x0041490D */ edx = 0x5FA00UL; /* mov edx,5FA00h */
l_414912:
    /* 0x00414912 */ ecx = 0x200UL; /* mov ecx,200h */
    /* 0x00414917 */ edi = (unsigned long)G32(0x4299F0); /* mov edi,dword_4299F0 */
    /* 0x0041491D */ esi = 0x42C9B0UL; /* mov esi,offset dword_42C9B0 */
    /* 0x00414922 */ edi = (edi + edx) & U32MASK; /* add edi,edx */
    /* 0x00414924 */ edx = (edx + 0x200UL) & U32MASK; /* add edx,200h */
    /* 0x0041492A cld */;
    /* 0x0041492B */ eax = ecx; /* mov eax,ecx */
    /* 0x0041492D */ ecx = (ecx - edi) & U32MASK; /* sub ecx,edi */
    /* 0x0041492F */ ecx = (ecx - eax) & U32MASK; /* sub ecx,eax */
    /* 0x00414931 */ ecx &= 3UL; /* and ecx,3 */
    /* 0x00414934 */ eax = (eax - ecx) & U32MASK; /* sub eax,ecx */
    /* 0x00414936 jle */ if ((int)(eax & U32MASK) > 0) { /* signed */
        /* 0x00414938 rep movsb */
        while (ecx != 0) { M8(edi) = M8(esi); esi = (esi + 1UL) & U32MASK; edi = (edi + 1UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
        /* 0x0041493A */ ecx = eax; /* mov ecx,eax */
        /* 0x0041493C */ eax &= 3UL; /* and eax,3 */
        /* 0x0041493F */ ecx >>= 2; /* shr ecx,2 */
        /* 0x00414942 rep movsd */
        while (ecx != 0) { M32(edi) = M32(esi); esi = (esi + 4UL) & U32MASK; edi = (edi + 4UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
    }
l_414944:
    /* 0x00414944 */ ecx = (ecx + eax) & U32MASK; /* add ecx,eax */
    /* 0x00414946 rep movsb */
    while (ecx != 0) { M8(edi) = M8(esi); esi = (esi + 1UL) & U32MASK; edi = (edi + 1UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
    /* 0x00414948 cmp edx,61600h */ /* 0x0041494E jnz */ if (edx != 0x61600UL) goto l_414912;
    /* 0x00414950 */ eax = 0; /* xor eax,eax */
l_414952:
    /* 0x00414952 */ edx = (unsigned long)G32(0x4299F0); /* mov edx,dword_4299F0 */
    /* 0x00414958 */ ebx = (ebx & 0xFFFF0000UL) | (unsigned long)M16((edx + eax + 0x5FA00UL) & U32MASK); /* mov bx,[edx+eax+5FA00h] (high lanes stale, only bx stored later) */
    /* 0x00414960 */ eax = (eax + 2UL) & U32MASK; /* add eax,2 */
    /* 0x00414963 */ ebx = (ebx << 3) & U32MASK; /* shl ebx,3 */
    /* 0x00414966 */ M16((edx + eax + 0x5F9FEUL) & U32MASK) = (unsigned short)(ebx & 0xFFFFUL); /* mov [edx+eax+5F9FEh],bx (in-place u16 x8) */
    /* 0x0041496E cmp eax,1C00h */ /* 0x00414973 jnz */ if (eax != 0x1C00UL) goto l_414952;
    /* 0x00414975 */ edx = 0xE00UL; /* mov edx,0E00h */
    /* 0x0041497A */ ebx = 0xE00UL; /* mov ebx,0E00h */
    /* 0x0041497F */ eax = 8; /* mov eax,8 */
    /* 0x00414984 */ v1c = edx; /* mov [var_1C],edx */
    /* 0x00414987 */ edx = (unsigned long)G32(0x4299F4); /* mov edx,dword_4299F4 */
    /* 0x0041498D */ ecx = 0x180UL; /* mov ecx,180h */
    /* 0x00414992 */ edx = (edx + 0x5FA00UL) & U32MASK; /* add edx,5FA00h */
    /* 0x00414998 */ edi = 0x443B68UL; /* mov edi,offset dword_443B68 */
    /* 0x0041499D */ d3u_421284(c, (unsigned int)ebx, (unsigned int)edx, (unsigned int)eax, (unsigned int)ecx, (unsigned int)esi, (unsigned int)edi); /* call sub_421284 (esi stale, callee ignores esi/edi/ecx) */
    /* 0x004149A2 */ ebx = 0x203UL; /* mov ebx,203h */
    /* 0x004149A7 */ eax = 1; /* mov eax,1 */
    /* 0x004149AC */ edx = (unsigned long)G32(0x4299FC); /* mov edx,dword_4299FC */
    /* 0x004149B2 */ esi = 0x429A08UL; /* mov esi,offset dword_429A08 */
    /* 0x004149B7 */ d3u_421284(c, (unsigned int)ebx, (unsigned int)edx, (unsigned int)eax, (unsigned int)ecx, (unsigned int)esi, (unsigned int)edi); /* call sub_421284 (ecx=180h,edi=443B68h preserved) */
    /* 0x004149BC */ ebx = 0x203UL; /* mov ebx,203h */
    /* 0x004149C1 cld */;
    /* 0x004149C2 */ eax = ecx; /* mov eax,ecx (=180h: ecx preserved by callee) */
    /* 0x004149C4 */ ecx = (ecx - edi) & U32MASK; /* sub ecx,edi */
    /* 0x004149C6 */ ecx = (ecx - eax) & U32MASK; /* sub ecx,eax */
    /* 0x004149C8 */ ecx &= 3UL; /* and ecx,3 */
    /* 0x004149CB */ eax = (eax - ecx) & U32MASK; /* sub eax,ecx */
    /* 0x004149CD jle */ if ((int)(eax & U32MASK) > 0) { /* signed */
        /* 0x004149CF rep movsb */
        while (ecx != 0) { M8(edi) = M8(esi); esi = (esi + 1UL) & U32MASK; edi = (edi + 1UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
        /* 0x004149D1 */ ecx = eax; /* mov ecx,eax */
        /* 0x004149D3 */ eax &= 3UL; /* and eax,3 */
        /* 0x004149D6 */ ecx >>= 2; /* shr ecx,2 */
        /* 0x004149D9 rep movsd */
        while (ecx != 0) { M32(edi) = M32(esi); esi = (esi + 4UL) & U32MASK; edi = (edi + 4UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
    }
l_4149db:
    /* 0x004149DB */ ecx = (ecx + eax) & U32MASK; /* add ecx,eax */
    /* 0x004149DD rep movsb */
    while (ecx != 0) { M8(edi) = M8(esi); esi = (esi + 1UL) & U32MASK; edi = (edi + 1UL) & U32MASK; ecx = (ecx - 1UL) & U32MASK; }
    /* 0x004149DF */ v1c = ebx; /* mov [var_1C],ebx (=203h) */
l_4149e2:
    /* 0x004149E2 */ ecx = v1c; /* mov ecx,[var_1C] */
    /* 0x004149E5 */ eax = v1c; /* mov eax,[var_1C] */
    /* 0x004149E8 */ ecx = (ecx + 1UL) & U32MASK; /* inc ecx */
    /* 0x004149E9 */ d3u_414d78(c, (unsigned int)eax); /* call sub_414D78 (eax in; ecx preserved by callee) */
    /* 0x004149EE */ v1c = ecx; /* mov [var_1C],ecx */
    /* 0x004149F1 cmp ecx,2FDh */ /* 0x004149F7 jb */ if (ecx < 0x2FDUL) goto l_4149e2; /* unsigned */
    /* 0x004149F9 */ d3u_41422c(c); /* call sub_41422C */
    /* 0x004149FE */ ebx = 0x1FFUL; /* mov ebx,1FFh */
    /* 0x00414A03 */ ecx = 0x442528UL; /* mov ecx,offset dword_442528 */
    /* 0x00414A08 */ edi = 0; /* xor edi,edi */
    /* 0x00414A0A */ edx = 0; /* xor edx,edx */
    /* 0x00414A0C */ G32(0x444210) = (unsigned int)edi; /* mov dword_444210,edi (=0) */
    /* 0x00414A12 */ G32(0x44424C) = (unsigned int)edi; /* mov dword_44424C,edi (=0) */
    /* 0x00414A18 */ G32(0x44423C) = (unsigned int)edi; /* mov dword_44423C,edi (=0) */
    /* 0x00414A1E */ G32(0x444208) = (unsigned int)ebx; /* mov dword_444208,ebx (=1FFh) */
    /* 0x00414A24 */ eax = ecx; /* mov eax,ecx */
    /* 0x00414A26 */ ebx = 0x430UL; /* mov ebx,430h */
    /* 0x00414A2B */ G32(0x4441F8) = (unsigned int)ecx; /* mov dword_4441F8,ecx */
    /* 0x00414A31 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=430h dst=442528h fill=0 */
    /* 0x00414A36 */ ebx = 0x400UL; /* mov ebx,400h */
    /* 0x00414A3B */ edx = 0xFFFFFFFFUL; /* mov edx,0FFFFFFFFh */
    /* 0x00414A40 */ eax = 0x443360UL; /* mov eax,offset dword_443360 */
    /* 0x00414A45 */ esi = 1; /* mov esi,1 */
    /* 0x00414A4A */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=400h dst=443360h fill=FFh */
    /* 0x00414A4F */ ebx = 0x400UL; /* mov ebx,400h */
    /* 0x00414A54 */ eax = 0x442128UL; /* mov eax,offset dword_442128 */
    /* 0x00414A59 */ edx = 0; /* xor edx,edx */
    /* 0x00414A5B */ v1c = esi; /* mov [var_1C],esi (=1) */
    /* 0x00414A5E */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=400h dst=442128h fill=0 */
    /* 0x00414A63 */ edx &= 0xFFFFFF00UL; /* xor dl,dl */
    /* 0x00414A65 */ eax &= 0xFFFF00FFUL; /* xor ah,ah (eax=442028h) */
    /* 0x00414A67 */ G8(0x442D58) = (unsigned char)(edx & 0xFFUL); /* mov byte_442D58,dl (=0) */
    /* 0x00414A6D */ edx = (edx & 0xFFFF00FFUL) | 0x100UL; /* mov dh,1 */
    /* 0x00414A6F */ G8(0x443260) = (unsigned char)((eax >> 8) & 0xFFUL); /* mov byte_443260,ah (=0) */
    /* 0x00414A75 */ G8(0x444EBA) = (unsigned char)((edx >> 8) & 0xFFUL); /* mov byte_444EBA,dh (=1) */
    /* 0x00414A7B */ G8(0x444EB9) = (unsigned char)((edx >> 8) & 0xFFUL); /* mov byte_444EB9,dh (=1) */
l_414a81:
    /* 0x00414A81 */ edx = (edx & 0xFFFFFF00UL) | (v1c & 0xFFUL); /* mov dl,byte ptr [var_1C] */
    /* 0x00414A84 */ eax = v1c; /* mov eax,[var_1C] */
    /* 0x00414A87 */ edx = (edx & 0xFFFFFF00UL) | ((edx - 1UL) & 0xFFUL); /* dec dl */
    /* 0x00414A89 */ G8((0x442D58UL + eax) & U32MASK) = (unsigned char)(edx & 0xFFUL); /* mov byte_442D58[eax],dl */
    /* 0x00414A8F */ edx = (edx & 0xFFFFFF00UL) | (v1c & 0xFFUL); /* mov dl,byte ptr [var_1C] */
    /* 0x00414A92 */ edi = (eax + 1UL) & U32MASK; /* lea edi,[eax+1] */
    /* 0x00414A95 */ edx = (edx & 0xFFFFFF00UL) | ((edx + 1UL) & 0xFFUL); /* inc dl */
    /* 0x00414A97 */ v1c = edi; /* mov [var_1C],edi */
    /* 0x00414A9A */ G8((0x443260UL + eax) & U32MASK) = (unsigned char)(edx & 0xFFUL); /* mov byte_443260[eax],dl */
    /* 0x00414AA0 cmp edi,100h */ /* 0x00414AA6 jb */ if (edi < 0x100UL) goto l_414a81; /* unsigned */
    /* 0x00414AA8 */ edx = 0x9240UL; /* mov edx,9240h */
    /* 0x00414AAD */ ebp = (unsigned long)G32(0x44426C); /* mov ebp,dword_44426C */
    /* 0x00414AB3 */ eax = 0; /* xor eax,eax */
l_414ab5:
    /* 0x00414AB5 */ eax = (eax + 4UL) & U32MASK; /* add eax,4 */
    /* 0x00414AB8 */ G32((0x442E54UL + eax) & U32MASK) = (unsigned int)edx; /* mov dword_442E54[eax],edx */
    /* 0x00414ABE cmp eax,400h */ /* 0x00414AC3 jnz */ if (eax != 0x400UL) goto l_414ab5;
    /* 0x00414AC5 */ ebx = 0xFFUL; /* mov ebx,0FFh */
    /* 0x00414ACA */ ecx = 0x91ADC0UL; /* mov ecx,91ADC0h */
    /* 0x00414ACF */ esi = 0; /* xor esi,esi */
    /* 0x00414AD1 */ G32(0x4441FC) = (unsigned int)ebx; /* mov dword_4441FC,ebx (=FFh) */
    /* 0x00414AD7 */ G32(0x443258) = (unsigned int)esi; /* mov dword_443258,esi (=0) */
    /* 0x00414ADD */ G32(0x44325C) = (unsigned int)esi; /* mov dword_44325C,esi (=0) */
    /* 0x00414AE3 */ G32(0x443B60) = (unsigned int)esi; /* mov dword_443B60,esi (=0) */
    /* 0x00414AE9 */ G32(0x443B64) = (unsigned int)esi; /* mov dword_443B64,esi (=0) */
    /* 0x00414AEF */ G32(0x4441E0) = (unsigned int)esi; /* mov dword_4441E0,esi (=0) */
    /* 0x00414AF5 */ G32(0x444200) = (unsigned int)esi; /* mov dword_444200,esi (=0) */
    /* 0x00414AFB */ esi = (unsigned long)G32(0x44422C); /* mov esi,dword_44422C */
    /* 0x00414B01 */ G32(0x4441E4) = (unsigned int)ecx; /* mov dword_4441E4,ecx (=91ADC0h) */
    /* 0x00414B07 test esi,esi */ /* 0x00414B09 jz */ if (esi == 0UL) goto l_414b7d;
    /* 0x00414B0F */ ecx = 0; /* xor ecx,ecx */
    /* 0x00414B11 */ ecx = (unsigned long)G8(0x444EA0); /* mov cl,byte_444EA0 */
    /* 0x00414B17 */ eax = 0x40000UL; /* mov eax,40000h */
    /* 0x00414B1C */ ecx = (ecx + ecx) & U32MASK; /* add ecx,ecx */
    /* 0x00414B1E */ edi = (unsigned long)G32(0x444230); /* mov edi,dword_444230 */
    /* 0x00414B24 */ eax = (unsigned long)(((long)0x40000) >> (ecx & 31UL)); /* sar eax,cl (x86 masks count to 5 bits) */
    /* 0x00414B26 */ edi = (edi + eax) & U32MASK; /* add edi,eax */
    /* 0x00414B28 */ edx = (unsigned long)G32(0x444EE4); /* mov edx,dword_444EE4 */
    /* 0x00414B2E */ G32(0x444230) = (unsigned int)edi; /* mov dword_444230,edi */
    /* 0x00414B34 cmp edi,edx */ /* 0x00414B36 jbe */ if (edi > edx) { /* unsigned */
    /* 0x00414B38 */ G32(0x444230) = (unsigned int)edx; /* mov dword_444230,edx */
    }
l_414b3e:
    /* 0x00414B3E */ esi = (unsigned long)G32(0x444228); /* mov esi,dword_444228 */
    /* 0x00414B44 */ ecx = (unsigned long)G32(0x444254); /* mov ecx,dword_444254 */
    /* 0x00414B4A */ ebx = (unsigned long)G32(0x444230); /* mov ebx,dword_444230 */
    /* 0x00414B50 */ eax = (ebp * 4UL) & U32MASK; /* lea eax,[ebp*4] */
l_414b57:
    /* 0x00414B57 */ edx = (unsigned long)G32(0x44422C); /* mov edx,dword_44422C */
    /* 0x00414B5D cmp ebx,[edx+eax] */ /* 0x00414B60 jbe */ if (ebx <= (unsigned long)M32((edx + eax) & U32MASK)) goto l_414b72; /* unsigned */
    /* 0x00414B62 */ eax = (eax + 4UL) & U32MASK; /* add eax,4 */
    /* 0x00414B65 */ edx = (edx + eax) & U32MASK; /* add edx,eax */
    /* 0x00414B67 */ edi = (unsigned long)M32(edx); /* mov edi,[edx] */
    /* 0x00414B69 */ ebp = (ebp + 1UL) & U32MASK; /* inc ebp */
    /* 0x00414B6A cmp edi,ecx */ /* 0x00414B6C jnb */ if (edi >= ecx) goto l_414b72; /* unsigned */
    /* 0x00414B6E cmp edi,esi */ /* 0x00414B70 jb */ if (edi < esi) goto l_414b57; /* unsigned */
l_414b72:
    /* 0x00414B72 */ edx = (unsigned long)G32(0x44422C); /* mov edx,dword_44422C */
    /* 0x00414B78 */ eax = (unsigned long)M32((edx + ((ebp * 4UL) & U32MASK)) & U32MASK); /* mov eax,[edx+ebp*4] */
    /* 0x00414B7B jmp */ goto l_414b82;
l_414b7d:
    /* 0x00414B7D */ eax = (unsigned long)G32(0x444EE4); /* mov eax,dword_444EE4 */
l_414b82:
    /* 0x00414B82 */ G32(0x444230) = (unsigned int)eax; /* mov dword_444230,eax */
    /* 0x00414B87 */ G32(0x44426C) = (unsigned int)ebp; /* mov dword_44426C,ebp */
    /* 0x00414B8D test ebp,ebp */ /* 0x00414B8F jbe */ if (ebp == 0UL) goto l_414c9f; /* unsigned jbe 0 */
    /* 0x00414B95 */ ecx = (unsigned long)G32(0x44422C); /* mov ecx,dword_44422C */
    /* 0x00414B9B */ eax = (ebp * 4UL) & U32MASK; /* lea eax,[ebp*4] */
    /* 0x00414BA2 */ eax = (eax + ecx) & U32MASK; /* add eax,ecx */
    /* 0x00414BA4 */ ecx = (unsigned long)G32(0x444230); /* mov ecx,dword_444230 */
    /* 0x00414BAA */ edi = (unsigned long)M32((eax - 4UL) & U32MASK); /* mov edi,[eax-4] */
    /* 0x00414BAD */ ecx = (ecx - edi) & U32MASK; /* sub ecx,edi */
    /* 0x00414BAF cmp ebp,1 */ /* 0x00414BB2 jbe */ if (ebp > 1UL) { /* unsigned */
    /* 0x00414BB4 */ edi = (edi - (unsigned long)M32((eax - 8UL) & U32MASK)) & U32MASK; /* sub edi,[eax-8] */
    }
l_414bb7:
    /* 0x00414BB7 cmp byte_444E99,1 */ /* 0x00414BBE jnz */ if (G8(0x444E99) != 1) goto l_414bc7;
    /* 0x00414BC0 */ eax = (unsigned long)G32(0x444268); /* mov eax,dword_444268 */
    /* 0x00414BC5 jmp */ goto l_414bcc;
l_414bc7:
    /* 0x00414BC7 */ eax = (unsigned long)G32(0x444EF8); /* mov eax,dword_444EF8 */
l_414bcc:
    /* 0x00414BCC */ v1c = eax; /* mov [var_1C],eax */
    /* 0x00414BCF */ G32(0x44426C) = (unsigned int)ebp; /* mov dword_44426C,ebp */
    /* 0x00414BD5 cmp edi,75A45h */ /* 0x00414BDB jnz */ if (edi != 0x75A45UL) goto l_414be5;
    /* 0x00414BDD cmp ecx,68302h */ /* 0x00414BE3 jz */ if (ecx == 0x68302UL) goto l_414bf5;
l_414be5:
    /* 0x00414BE5 cmp edi,8F4D0h */ /* 0x00414BEB jnz */ if (edi != 0x8F4D0UL) goto l_414c06;
    /* 0x00414BED cmp ecx,2D9A22h */ /* 0x00414BF3 jnz */ if (ecx != 0x2D9A22UL) goto l_414c06;
l_414bf5:
    /* 0x00414BF5 */ ebx = 0x800000UL; /* mov ebx,800000h */
    /* 0x00414BFA */ eax = (unsigned long)G32(0x4299BC); /* mov eax,dword_4299BC */
    /* 0x00414BFF */ edx = 0; /* xor edx,edx */
    /* 0x00414C01 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=800000h dst=4299BC fill=0 */
l_414c06:
    /* 0x00414C06 cmp edi,17E5F8h */ /* 0x00414C0C jnz */ if (edi != 0x17E5F8UL) goto l_414c16;
    /* 0x00414C0E cmp ecx,2CC640h */ /* 0x00414C14 jz */ if (ecx == 0x2CC640UL) goto l_414c26;
l_414c16:
    /* 0x00414C16 cmp edi,2CC640h */ /* 0x00414C1C jnz */ if (edi != 0x2CC640UL) goto l_414c30;
    /* 0x00414C1E cmp ecx,6AF80h */ /* 0x00414C24 jnz */ if (ecx != 0x6AF80UL) goto l_414c30;
l_414c26:
    /* 0x00414C26 */ d3u_4141d4(c); /* call sub_4141D4 */
    /* 0x00414C2B */ d3u_41422c(c); /* call sub_41422C */
l_414c30:
    /* 0x00414C30 cmp edi,388DC2h */ /* 0x00414C36 jnz */ if (edi != 0x388DC2UL) goto l_414c40;
    /* 0x00414C38 cmp ecx,16DD2Eh */ /* 0x00414C3E jz */ if (ecx == 0x16DD2EUL) goto l_414c6b;
l_414c40:
    /* 0x00414C40 cmp edi,7D4C0h */ /* 0x00414C46 jnz */ if (edi != 0x7D4C0UL) goto l_414c50;
    /* 0x00414C48 cmp ecx,3C41Eh */ /* 0x00414C4E jz */ if (ecx == 0x3C41EUL) goto l_414c6b;
l_414c50:
    /* 0x00414C50 cmp edi,0FB670h */ /* 0x00414C56 jz */ if (edi == 0xFB670UL) goto l_414c9f;
    /* 0x00414C58 cmp ecx,0FB670h */ /* 0x00414C5E jnz */ if (ecx != 0xFB670UL) goto l_414c9f;
    /* 0x00414C60 */ eax = v1c; /* mov eax,[var_1C] */
    /* 0x00414C63 cmp eax,dword_444228 */ /* 0x00414C69 jb */ if (eax < (unsigned long)G32(0x444228)) goto l_414c9f; /* unsigned */
l_414c6b:
    /* 0x00414C6B */ ebx = 0x800000UL; /* mov ebx,800000h */
    /* 0x00414C70 */ eax = (unsigned long)G32(0x4299BC); /* mov eax,dword_4299BC */
    /* 0x00414C75 */ edx = 0; /* xor edx,edx */
    /* 0x00414C77 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=800000h dst=4299BC fill=0 */
    /* 0x00414C7C */ ebx = 0x140000UL; /* mov ebx,140000h */
    /* 0x00414C81 */ eax = (unsigned long)G32(0x4299C0); /* mov eax,dword_4299C0 */
    /* 0x00414C86 */ edx = 0; /* xor edx,edx */
    /* 0x00414C88 */ d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* len=140000h dst=4299C0 fill=0 */
    /* 0x00414C8D */ eax = v1c; /* mov eax,[var_1C] */
    /* 0x00414C90 cmp eax,dword_444228 */ /* 0x00414C96 jb */ if (eax < (unsigned long)G32(0x444228)) goto l_414c9f; /* unsigned */
    /* 0x00414C98 */ G8(0x444EA2) = 0x0A; /* mov byte_444EA2,0Ah */
l_414c9f:
    /* 0x00414C9F cmp byte_444E99,1 */ /* 0x00414CA6 jnz */ if (G8(0x444E99) != 1) goto l_414cb7;
    /* 0x00414CA8 */ eax = 0xFFUL; /* mov eax,0FFh */
    /* 0x00414CAD */ d3u_419f44(c, (unsigned int)eax); /* call sub_419F44 (al=FFh in) */
    /* 0x00414CB2 jmp */ goto l_414d67;
l_414cb7:
    /* 0x00414CB7 */ esi = 0x0AUL; /* mov esi,0Ah */
    /* 0x00414CBC */ eax = esi; /* mov eax,esi */
    /* 0x00414CBE */ G32(0x4441E8) = (unsigned int)esi; /* mov dword_4441E8,esi (=0Ah) */
    /* 0x00414CC4 */ eax = (unsigned long)d3u_423188(c, (unsigned int)eax); /* call sub_423188 (eax=0Ah in, al out) */
    /* 0x00414CC9 */ edx = eax; /* mov edx,eax */
    /* 0x00414CCB */ eax = 0; /* xor eax,eax */
    /* 0x00414CCD */ ecx = 0; /* xor ecx,ecx */
    /* 0x00414CCF */ eax = edx & 0xFFUL; /* mov al,dl */
    /* 0x00414CD1 */ ecx = edx & 0xFFUL; /* mov cl,dl */
    /* 0x00414CD3 */ G32(0x4441EC) = (unsigned int)eax; /* mov dword_4441EC,eax */
    /* 0x00414CD8 */ eax = (ecx + 1UL) & U32MASK; /* lea eax,[ecx+1] */
    /* 0x00414CDB */ G32(0x4441F0) = (unsigned int)eax; /* mov dword_4441F0,eax */
    /* 0x00414CE0 */ eax = 0x4441E8UL; /* mov eax,offset dword_4441E8 */
    /* 0x00414CE5 */ d3u_423140(c, (unsigned int)eax); /* call sub_423140 (pops edx: caller dl preserved) */
    /* 0x00414CEA cmp dl,byte_444EA9 */ /* 0x00414CF0 jz */ if ((edx & 0xFFUL) == (unsigned long)G8(0x444EA9)) goto l_414d67;
    /* 0x00414CF6 */ ebp = (unsigned long)G32(0x44426C); /* mov ebp,dword_44426C */
    /* 0x00414CFC */ G8(0x444EA9) = (unsigned char)(edx & 0xFFUL); /* mov byte_444EA9,dl */
    /* 0x00414D02 */ eax = (unsigned long)G32((0x429DE0UL + ecx * 4UL) & U32MASK); /* mov eax,dword_429DE0[ecx*4] */
    /* 0x00414D09 */ edi = (unsigned long)G32((0x429DB8UL + ecx * 4UL) & U32MASK); /* mov edi,dword_429DB8[ecx*4] */
    /* 0x00414D10 */ G32(0x4441C4) = (unsigned int)eax; /* mov dword_4441C4,eax */
    /* 0x00414D15 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)G8((0x429E30UL + ecx) & U32MASK); /* mov al,byte_429E30[ecx] */
    /* 0x00414D1B */ G32(0x4441C8) = (unsigned int)edi; /* mov dword_4441C8,edi */
    /* 0x00414D21 */ G8(0x444E9E) = (unsigned char)(eax & 0xFFUL); /* mov byte_444E9E,al */
    /* 0x00414D26 cmp dl,5 */ /* 0x00414D29 jz */ if ((edx & 0xFFUL) == 5UL) goto l_414d30;
    /* 0x00414D2B cmp dl,6 */ /* 0x00414D2E jnz */ if ((edx & 0xFFUL) != 6UL) goto l_414d34;
l_414d30:
    /* 0x00414D30 */ eax = (eax & 0xFFFFFF00UL) | 1UL; /* mov al,1 */
    /* 0x00414D32 jmp */ goto l_414d36;
l_414d34:
    /* 0x00414D34 */ eax = (eax & 0xFFFFFF00UL); /* xor al,al */
l_414d36:
    /* 0x00414D36 */ edx = (edx & 0xFFFF00FFUL) | ((unsigned long)G8(0x444EA9) << 8); /* mov dh,byte_444EA9 */
    /* 0x00414D3C */ G8(0x444EA6) = (unsigned char)(eax & 0xFFUL); /* mov byte_444EA6,al */
    /* 0x00414D41 */ G32(0x44426C) = (unsigned int)ebp; /* mov dword_44426C,ebp */
    /* 0x00414D47 */ edi = 0; /* xor edi,edi */
    /* 0x00414D49 */ eax &= 0xFFFF00FFUL; /* xor ah,ah */
    /* 0x00414D4B */ G32(0x4441D0) = (unsigned int)edi; /* mov dword_4441D0,edi (=0) */
    /* 0x00414D51 */ G8(0x444EA8) = (unsigned char)((eax >> 8) & 0xFFUL); /* mov byte_444EA8,ah (=0) */
    /* 0x00414D57 */ G8(0x444E9D) = (unsigned char)((eax >> 8) & 0xFFUL); /* mov byte_444E9D,ah (=0) */
    /* 0x00414D5D cmp dh,4 */ /* 0x00414D60 jbe */ if (((edx >> 8) & 0xFFUL) > 4UL) { /* unsigned */
    /* 0x00414D62 */ d3u_41b2d4(c); /* call sub_41B2D4 */
    }
l_414d67:
    /* 0x00414D67 */ ebp = (unsigned long)G32(0x44426C); /* mov ebp,dword_44426C */
    /* epilogue: add esp,4; pop ebp,edi,esi,edx,ecx,ebx; retn */
}

/* ---- match model update (from draft_e3.c) ---- */
/* sub_419F44: match price/model update. Takes the length-code byte in al
 * (param sym), picks the best of 5 price slots, maybe refines via sub_41ADF0,
 * then runs the two sub_423228 range-model switches and refreshes the
 * 4441C4/C8/444E9E/444EA6/4441D0/444EA8/444E9D state when the code changed.
 * Returns (G32(0x444268)+ebp)&U32MASK. sub_41A33C is called x2 (0x0041A028,
 * 0x0041A053), sub_423228 x2. UNCERTAIN: d3u_41a9c4/d3u_41a33c/d3u_41adf0
 * are void under this draft contract, so their register results (ebp update,
 * v-slot prices, bl refinement) are lost; the C locals keep pre-call values.
 * Entry-live: al=sym only. Frame: w40[5] dwords, w2c dword, w28/w24/w1c
 * bytes, w20 input byte. */
unsigned int d3u_419f44(dec3w_ctx *c, unsigned int sym)
{
    unsigned int eax = 0;
    unsigned int ebx = 0;
    unsigned int ecx = 0;
    unsigned int edx = 0;
    unsigned int esi = 0;
    unsigned int edi = 0;
    unsigned int ebp = 0;
    unsigned int w40[5] = {0, 0, 0, 0, 0};
    unsigned int w2c = 0;
    unsigned char w28 = 0;
    unsigned char w24 = 0;
    unsigned char w20 = 0;
    unsigned char w1c = 0;
    unsigned int i = 0;
    w20 = (unsigned char)(sym & 0xFFUL); /* 0x00419F4D mov [var_20],al */
    eax = 0UL; /* 0x00419F51 xor eax,eax */
    eax = (eax & ~0xFFUL) | (unsigned int)G8(0x444EA9); /* 0x00419F53 mov al,byte_444EA9 */
    ebp = G32(0x444230); /* 0x00419F58 mov ebp,dword_444230 */
    edx = G32(0x444268); /* 0x00419F5E mov edx,dword_444268 */
    eax = (eax & ~0xFFUL) | (unsigned int)M8((0x429E3AUL + (eax & 0xFFUL)) & U32MASK); /* 0x00419F64 mov al,byte_429E3A[eax] */
    ebp = (ebp - edx) & U32MASK; /* 0x00419F6A sub ebp,edx */
    w24 = (unsigned char)(eax & 0xFFUL); /* 0x00419F6C mov [var_24],al */
    w1c = (unsigned char)(eax & 0xFFUL); /* 0x00419F70 mov [var_1C],al */
    if (ebp < 0x4B00UL) goto L_419F81; /* 0x00419F74 cmp / 0x00419F7A jb unsigned */
    ebp = 0x3200UL; /* 0x00419F7C mov ebp,3200h */
L_419F81:
    if (G8(0x42BC6E) == 0) goto L_41A17B; /* 0x00419F81 cmp / 0x00419F88 jz */
    if (G8(0x444EA0) == 0) goto L_41A17B; /* 0x00419F8E cmp / 0x00419F95 jz */
    eax = G32(0x444268); /* 0x00419F9B mov eax,dword_444268 */
    if (eax >= G32(0x444228)) goto L_41A17B; /* 0x00419FA0 cmp / 0x00419FA6 jnb unsigned */
    if (ebp <= 0x100UL) goto L_419FBF; /* 0x00419FAC cmp / 0x00419FB2 jbe unsigned */
    eax = (ebp - 0x100UL) & U32MASK; /* 0x00419FB4 lea eax,[ebp-100h] */
    d3u_41e1c0(c, eax); /* 0x00419FBA call sub_41E1C0 */
L_419FBF:
    ecx = 0x14UL; /* 0x00419FBF mov ecx,14h */
    edi = 0x444188UL; /* 0x00419FC4 mov edi,offset dword_444188 */
    eax = G32(0x42C9A8); /* 0x00419FC9 mov eax,dword_42C9A8 */
    edx = G32(0x444EE8); /* 0x00419FCE mov edx,dword_444EE8 */
    esi = 0x44419CUL; /* 0x00419FD4 mov esi,offset dword_44419C */
    edx = (edx + eax) & U32MASK; /* 0x00419FD9 add edx,eax */
    /* 0x00419FDB cld */
    eax = ecx; /* 0x00419FDC mov eax,ecx */
    ecx = (ecx - edi) & U32MASK; /* 0x00419FDE sub ecx,edi */
    ecx = (ecx - eax) & U32MASK; /* 0x00419FE0 sub ecx,eax */
    ecx &= 3UL; /* 0x00419FE2 and ecx,3 */
    eax = (eax - ecx) & U32MASK; /* 0x00419FE5 sub eax,ecx */
    if ((int)eax <= 0) goto L_419FF5; /* 0x00419FE7 jle SIGNED */
    /* 0x00419FE9 rep movsb (head); 0x00419FEB mov ecx,eax; 0x00419FED and eax,3; */
    /* 0x00419FF0 shr ecx,2; 0x00419FF3 rep movsd; 0x00419FF5 add ecx,eax; */
    /* 0x00419FF7 rep movsb (tail): net 20-byte copy 44419C -> 444188 */
    for (i = 0; i < 20UL; i++) M8((0x444188UL + i) & U32MASK) = M8((0x44419CUL + i) & U32MASK);
L_419FF5:
    ecx = (ecx + eax) & U32MASK; /* 0x00419FF5 add ecx,eax */
    w2c = edx; /* 0x00419FF9 mov [var_2C],edx */
    if (ebp <= 0x100UL) goto L_41A016; /* 0x00419FFD cmp / 0x0041A003 jbe unsigned */
    eax = w2c; /* 0x0041A005 mov eax,[var_2C] */
    edx = (ebp - 0x100UL) & U32MASK; /* 0x0041A009 lea edx,[ebp-100h] */
    ebp = d3u_41a9c4(c, (unsigned int)eax, (unsigned int)edx); /* 0x0041A00F call sub_41A9C4 + 0x0041A014 mov ebp,eax */
L_41A016:
    esi = w1c; /* 0x0041A016 movzx esi,[var_1C] */
    ebx = 0xFFFFFFFFUL; /* 0x0041A01B mov ebx,0FFFFFFFFh */
    eax = w2c; /* 0x0041A020 mov eax,[var_2C] */
    edx = ebp; /* 0x0041A024 mov edx,ebp */
    ecx = esi; /* 0x0041A026 mov ecx,esi */
    eax = d3u_41a33c(c, (unsigned int)eax, (unsigned int)edx, (unsigned int)ecx, (unsigned int)ebx); /* 0x0041A028 call sub_41A33C */
    edx &= ~0xFF00UL; /* 0x0041A02D xor dh,dh */
    w40[esi] = eax; /* 0x0041A02F mov [esp+esi*4+var_40],eax */
    w28 = 0; /* 0x0041A032 mov [var_28],dh, dh==0 after 41A02D */
L_41A036:
    eax = (eax & ~0xFFUL) | (unsigned int)w28; /* 0x0041A036 mov al,[var_28] */
    if ((eax & 0xFFUL) == (unsigned int)w24) goto L_41A068; /* 0x0041A03A cmp / 0x0041A03E jz */
    edi = w1c; /* 0x0041A040 movzx edi,[var_1C] */
    esi = eax & 0xFFUL; /* 0x0041A045 movzx esi,al */
    edx = ebp; /* 0x0041A048 mov edx,ebp */
    eax = w2c; /* 0x0041A04A mov eax,[var_2C] */
    ecx = esi; /* 0x0041A04E mov ecx,esi */
    ebx = w40[edi]; /* 0x0041A050 mov ebx,[esp+edi*4+var_40] */
    eax = d3u_41a33c(c, (unsigned int)eax, (unsigned int)edx, (unsigned int)ecx, (unsigned int)ebx); /* 0x0041A053 call sub_41A33C */
    w40[esi] = eax; /* 0x0041A058 mov [esp+esi*4+var_40],eax */
    if (eax >= w40[edi]) goto L_41A068; /* 0x0041A05B cmp / 0x0041A05E jnb unsigned */
    eax = (eax & ~0xFFUL) | (unsigned int)w28; /* 0x0041A060 mov al,[var_28] */
    w1c = (unsigned char)(eax & 0xFFUL); /* 0x0041A064 mov [var_1C],al */
L_41A068:
    ebx = (ebx & ~0xFFUL) | (unsigned int)w28; /* 0x0041A068 mov bl,[var_28] */
    ebx = (ebx & ~0xFFUL) | (((ebx & 0xFFUL) + 1UL) & 0xFFUL); /* 0x0041A06C inc bl */
    w28 = (unsigned char)(ebx & 0xFFUL); /* 0x0041A06E mov [var_28],bl */
    if ((ebx & 0xFFUL) <= 4UL) goto L_41A036; /* 0x0041A072 cmp / 0x0041A075 jbe unsigned */
    ecx = 0UL; /* 0x0041A077 xor ecx,ecx */
    G32(0x4441B8) = ebp; /* 0x0041A079 mov dword_4441B8,ebp */
    G32(0x4441B0) = ecx; /* 0x0041A07F mov dword_4441B0,ecx */
    ecx = (ecx & ~0xFFUL) | (unsigned int)w1c; /* 0x0041A085 mov cl,[var_1C] */
    ebx = (ebx & ~0xFFUL) | 0UL; /* 0x0041A089 xor bl,bl */
    if ((ecx & 0xFFUL) == 0UL) goto L_41A0CB; /* 0x0041A08B test cl,cl / 0x0041A08D jz */
    if ((ecx & 0xFFUL) == 3UL) goto L_41A0CB; /* 0x0041A08F cmp / 0x0041A092 jz */
    ecx = 0UL; /* 0x0041A094 xor ecx,ecx */
    ecx = (ecx & ~0xFFUL) | (unsigned int)w1c; /* 0x0041A096 mov cl,[var_1C] */
    edx = ((ecx & 0xFFUL) * 4UL) & U32MASK; /* 0x0041A09A lea edx,ds:0[ecx*4] */
    esi = M32((0x444188UL + edx) & U32MASK); /* 0x0041A0A1 mov esi,dword_444188[edx] */
    eax = (esi * 4UL) & U32MASK; /* 0x0041A0A7 lea eax,ds:0[esi*4] */
    eax = (eax + esi) & U32MASK; /* 0x0041A0AE add eax,esi */
    esi = G32(0x444188); /* 0x0041A0B0 mov esi,dword_444188 */
    esi = (esi << 2) & U32MASK; /* 0x0041A0B6 shl esi,2 */
    if (eax <= esi) goto L_41A0CB; /* 0x0041A0B9 cmp / 0x0041A0BB jbe unsigned */
    eax = w2c; /* 0x0041A0BD mov eax,[var_2C] */
    ebx = w40[(edx >> 2) & 0xFFUL]; /* 0x0041A0C1 mov ebx,[esp+edx+var_40] */
    edx = ebp; /* 0x0041A0C4 mov edx,ebp */
    goto L_41A150; /* 0x0041A0C6 jmp */
L_41A0CB:
    eax = 1UL; /* 0x0041A0CB mov eax,1 */
    edx = (edx & ~0xFFUL) | ((ebp < 0x800UL) ? 1UL : 0UL); /* 0x0041A0D0 cmp / 0x0041A0D6 setb dl unsigned */
    esi = 5UL; /* 0x0041A0D9 mov esi,5 */
    edx &= 0xFFUL; /* 0x0041A0DE and edx,0FFh */
    edi = G32(0x44418C); /* 0x0041A0E4 mov edi,dword_44418C */
    esi = (esi - edx) & U32MASK; /* 0x0041A0EA sub esi,edx */
    if (edi >= G32(0x444190)) goto L_41A0F9; /* 0x0041A0EC cmp / 0x0041A0F2 jnb unsigned */
    eax = 2UL; /* 0x0041A0F4 mov eax,2 */
L_41A0F9:
    edx = G32(0x444194); /* 0x0041A0F9 mov edx,dword_444194 */
    if (edx <= M32((0x444188UL + (eax * 4UL)) & U32MASK)) goto L_41A10D; /* 0x0041A0FF cmp / 0x0041A106 jbe unsigned */
    eax = 3UL; /* 0x0041A108 mov eax,3 */
L_41A10D:
    edx = G32(0x444198); /* 0x0041A10D mov edx,dword_444198 */
    if (edx <= M32((0x444188UL + (eax * 4UL)) & U32MASK)) goto L_41A121; /* 0x0041A113 cmp / 0x0041A11A jbe unsigned */
    eax = 4UL; /* 0x0041A11C mov eax,4 */
L_41A121:
    if (eax == 3UL) goto L_41A157; /* 0x0041A121 cmp / 0x0041A124 jz */
    edx = G32(0x444188); /* 0x0041A126 mov edx,dword_444188 */
    edx = (edx * esi) & U32MASK; /* 0x0041A12C imul edx,esi */
    ecx = M32((0x444188UL + (eax * 4UL)) & U32MASK); /* 0x0041A12F mov ecx,dword_444188[eax*4] */
    ecx = (ecx << 2) & U32MASK; /* 0x0041A136 shl ecx,2 */
    if (ecx <= edx) goto L_41A157; /* 0x0041A139 cmp / 0x0041A13B jbe unsigned */
    ecx = 0UL; /* 0x0041A13D xor ecx,ecx */
    ecx = (ecx & ~0xFFUL) | (eax & 0xFFUL); /* 0x0041A13F mov cl,al */
    eax = 0UL; /* 0x0041A141 xor eax,eax */
    eax = (eax & ~0xFFUL) | (unsigned int)w1c; /* 0x0041A143 mov al,[var_1C] */
    edx = ebp; /* 0x0041A147 mov edx,ebp */
    ebx = w40[eax & 0xFFUL]; /* 0x0041A149 mov ebx,[esp+eax*4+var_40] */
    eax = w2c; /* 0x0041A14C mov eax,[var_2C] */
L_41A150:
    eax = (eax & ~0xFFUL) | d3u_41adf0(c, (unsigned int)eax, (unsigned int)edx, (unsigned int)ecx, (unsigned int)ebx); /* 0x0041A150 call sub_41ADF0 */
    ebx = (ebx & ~0xFFUL) | (eax & 0xFFUL); /* 0x0041A155 mov bl,al */
L_41A157:
    if ((ebx & 0xFFUL) == 0UL) goto L_41A16F; /* 0x0041A157 test bl,bl / 0x0041A159 jz */
    w1c = (unsigned char)(ebx & 0xFFUL); /* 0x0041A15B mov [var_1C],bl */
    ebx = 0UL; /* 0x0041A15F xor ebx,ebx */
    G32(0x4441B8) = ebp; /* 0x0041A161 mov dword_4441B8,ebp */
    G32(0x4441B0) = ebx; /* 0x0041A167 mov dword_4441B0,ebx */
    goto L_41A17B; /* 0x0041A16D jmp */
L_41A16F:
    eax = 0UL; /* 0x0041A16F xor eax,eax */
    G32(0x4441B0) = eax; /* 0x0041A171 mov dword_4441B0,eax */
    G32(0x4441B8) = eax; /* 0x0041A176 mov dword_4441B8,eax */
L_41A17B:
    if (w20 == 0xFF) goto L_41A192; /* 0x0041A17B cmp / 0x0041A180 jz */
    eax = (eax & ~0xFFUL) | (unsigned int)w1c; /* 0x0041A182 mov al,[var_1C] */
    if ((eax & 0xFFUL) == (unsigned int)G8(0x444EA9)) goto L_41A26D; /* 0x0041A186 cmp / 0x0041A18C jz */
L_41A192:
    ebx = (ebx & ~0xFFUL) | (unsigned int)w20; /* 0x0041A192 mov bl,[var_20] */
    if ((ebx & 0xFFUL) == 0xFFUL) goto L_41A1DF; /* 0x0041A196 cmp / 0x0041A199 jz */
    eax = 0UL; /* 0x0041A19B xor eax,eax */
    edx = 0UL; /* 0x0041A19D xor edx,edx */
    eax = (eax & ~0xFFUL) | (unsigned int)G8(0x444EB6); /* 0x0041A19F mov al,byte_444EB6 */
    edx = (edx & ~0xFFUL) | (ebx & 0xFFUL); /* 0x0041A1A4 mov dl,bl */
    eax = (eax + (edx & 0xFFUL)) & U32MASK; /* 0x0041A1A6 add eax,edx */
    eax = (eax << 3) & U32MASK; /* 0x0041A1A8 shl eax,3 */
    eax = (eax + 0x443B68UL) & U32MASK; /* 0x0041A1AB add eax,offset dword_443B68 */
    edx = 0UL; /* 0x0041A1B0 xor edx,edx */
    edx = (edx & ~0xFFFFUL) | (unsigned int)M16((eax + 6UL) & U32MASK); /* 0x0041A1B2 mov dx,[eax+6] */
    G32(0x4441E8) = edx; /* 0x0041A1B6 mov dword_4441E8,edx */
    edx = 0UL; /* 0x0041A1BC xor edx,edx */
    edx = (edx & ~0xFFFFUL) | (unsigned int)M16(eax); /* 0x0041A1BE mov dx,[eax] */
    G32(0x4441EC) = edx; /* 0x0041A1C1 mov dword_4441EC,edx */
    eax = (eax & ~0xFFFFUL) | (unsigned int)M16((eax + 2UL) & U32MASK); /* 0x0041A1C7 mov ax,[eax+2] */
    eax &= 0xFFFFUL; /* 0x0041A1CB and eax,0FFFFh */
    G32(0x4441F0) = eax; /* 0x0041A1D0 mov dword_4441F0,eax */
    eax = 0x4441E8UL; /* 0x0041A1D5 mov eax,offset dword_4441E8 */
    d3u_423228(c, eax); /* 0x0041A1DA call sub_423228 */
L_41A1DF:
    eax = 0UL; /* 0x0041A1DF xor eax,eax */
    ebx = 0UL; /* 0x0041A1E1 xor ebx,ebx */
    eax = (eax & ~0xFFUL) | (unsigned int)w1c; /* 0x0041A1E3 mov al,[var_1C] */
    ebx = (ebx & ~0xFFUL) | (eax & 0xFFUL); /* 0x0041A1E7 mov bl,al */
    G32(0x4441EC) = eax; /* 0x0041A1E9 mov dword_4441EC,eax */
    eax = (ebx + 1UL) & U32MASK; /* 0x0041A1EE lea eax,[ebx+1], ebx high clean via 41A1E1 */
    esi = 0x0AUL; /* 0x0041A1F1 mov esi,0Ah */
    G32(0x4441F0) = eax; /* 0x0041A1F6 mov dword_4441F0,eax */
    eax = 0x4441E8UL; /* 0x0041A1FB mov eax,offset dword_4441E8 */
    G32(0x4441E8) = esi; /* 0x0041A200 mov dword_4441E8,esi */
    d3u_423228(c, eax); /* 0x0041A206 call sub_423228 */
    ecx = (ecx & ~0xFFUL) | (unsigned int)G8(0x444EA9); /* 0x0041A20B mov cl,byte_444EA9 */
    eax = (eax & ~0xFFUL) | (ebx & 0xFFUL); /* 0x0041A211 mov al,bl */
    if ((eax & 0xFFUL) == (ecx & 0xFFUL)) goto L_41A26D; /* 0x0041A213 cmp / 0x0041A215 jz */
    G8(0x444EA9) = (unsigned char)(eax & 0xFFUL); /* 0x0041A217 mov byte_444EA9,al */
    ecx = (ecx & ~0xFF00UL) | ((ebx & 0xFFUL) << 8); /* 0x0041A21C mov ch,bl */
    eax = M32((0x429DE0UL + (ebx & 0xFFUL) * 4UL) & U32MASK); /* 0x0041A21E mov eax,dword_429DE0[ebx*4] */
    edx = M32((0x429DB8UL + (ebx & 0xFFUL) * 4UL) & U32MASK); /* 0x0041A225 mov edx,dword_429DB8[ebx*4] */
    G32(0x4441C4) = eax; /* 0x0041A22C mov dword_4441C4,eax */
    eax = (eax & ~0xFFUL) | (unsigned int)M8((0x429E30UL + (ebx & 0xFFUL)) & U32MASK); /* 0x0041A231 mov al,byte_429E30[ebx] */
    G32(0x4441C8) = edx; /* 0x0041A237 mov dword_4441C8,edx */
    G8(0x444E9E) = (unsigned char)(eax & 0xFFUL); /* 0x0041A23D mov byte_444E9E,al */
    if (((ecx >> 8) & 0xFFUL) == 5UL) goto L_41A24C; /* 0x0041A242 cmp / 0x0041A245 jz */
    if ((ebx & 0xFFUL) != 6UL) goto L_41A250; /* 0x0041A247 cmp / 0x0041A24A jnz */
L_41A24C:
    eax = (eax & ~0xFFUL) | 1UL; /* 0x0041A24C mov al,1 */
    goto L_41A252; /* 0x0041A24E jmp */
L_41A250:
    eax &= ~0xFFUL; /* 0x0041A250 xor al,al */
L_41A252:
    G8(0x444EA6) = (unsigned char)(eax & 0xFFUL); /* 0x0041A252 mov byte_444EA6,al */
    edi = 0UL; /* 0x0041A257 xor edi,edi */
    eax &= ~0xFF00UL; /* 0x0041A259 xor ah,ah */
    G32(0x4441D0) = edi; /* 0x0041A25B mov dword_4441D0,edi */
    G8(0x444EA8) = (unsigned char)((eax >> 8) & 0xFFUL); /* 0x0041A261 mov byte_444EA8,ah */
    G8(0x444E9D) = (unsigned char)((eax >> 8) & 0xFFUL); /* 0x0041A267 mov byte_444E9D,ah */
L_41A26D:
    eax = G32(0x444268); /* 0x0041A26D mov eax,dword_444268 */
    eax = (eax + ebp) & U32MASK; /* 0x0041A272 add eax,ebp */
    G32(0x4441D4) = eax; /* 0x0041A274 mov dword_4441D4,eax */
    return eax & U32MASK; /* epilogue retn, eax=444268+ebp */
}

/* ---- small helpers (from draft_h3.c) ---- */
/* exit al=boolean (setl at 0x4167A4 survives; ah/bh lanes clobbered). */
unsigned int d3u_416774(dec3w_ctx *c, unsigned int p)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int ecx_ = 0;
unsigned int edx_ = 0;
/* 0x00416774 push ebx */
/* 0x00416775 push ecx */
/* 0x00416776 push edx */
/* 0x00416777 mov edx, eax */ edx_ = p;
/* 0x00416779 xor ebx, ebx */ ebx_ = 0u;
/* 0x0041677B mov bl, [eax+1] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(p + 1u);
/* 0x0041677E xor eax, eax */ eax_ = 0u;
/* 0x00416780 mov al, [edx] */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)G8(edx_);
/* 0x00416782 add eax, ebx */ eax_ = (eax_ + ebx_) & U32MASK;
/* 0x00416784 mov dword_4441E8, eax */ M32(0x4441E8u) = eax_;
/* 0x00416789 mov ax, word ptr dword_4441E8 */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)M16(0x4441E8u);
/* 0x0041678F and eax, 0FFFFh */ eax_ &= 0xFFFFu;
/* 0x00416794 xor ebx, ebx */ ebx_ = 0u;
/* 0x00416796 call sub_423188 */ eax_ = d3u_423188(c, eax_);
/* 0x0041679B mov bl, [edx] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(edx_);
/* 0x0041679D and eax, 0FFFFh */ eax_ &= 0xFFFFu;
/* 0x004167A2 cmp eax, ebx */ /* flags for setl */
/* 0x004167A4 setl al */ eax_ = (eax_ & 0xFFFFFF00UL) | ((((int)eax_ < (int)ebx_) ? 1u : 0u));
/* 0x004167A7 test al, al */
/* 0x004167A9 jz short loc_4167C1 */ if ((eax_ & 0xFFu) == 0u) goto l_4167c1;
/* 0x004167AB xor ebx, ebx */ ebx_ = 0u;
/* 0x004167AD mov dword_4441EC, ebx */ M32(0x4441ECu) = ebx_;
/* 0x004167B3 mov bl, [edx] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(edx_);
/* 0x004167B5 mov dword_4441F0, ebx */ M32(0x4441F0u) = ebx_;
/* 0x004167BB inc bl */ ebx_ = (ebx_ & 0xFFFFFF00UL) | ((ebx_ + 1u) & 0xFFu);
/* 0x004167BD mov [edx], bl */ M8(edx_) = (unsigned char)(ebx_ & 0xFFu);
/* 0x004167BF jmp short loc_4167DF */ goto l_4167df;
l_4167c1:
/* 0x004167C1 xor ebx, ebx */ ebx_ = 0u;
/* 0x004167C3 mov ah, [edx+1] */ eax_ = (eax_ & 0xFFFF00FFUL) | (((unsigned int)G8(edx_ + 1u) << 8) & 0xFF00u);
/* 0x004167C6 mov bl, [edx] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(edx_);
/* 0x004167C8 inc ah */ eax_ = (eax_ & 0xFFFF00FFUL) | (((((eax_ >> 8) & 0xFFu) + 1u) & 0xFFu) << 8);
/* 0x004167CA mov dword_4441EC, ebx */ M32(0x4441ECu) = ebx_;
/* 0x004167D0 mov ebx, dword_4441E8 */ ebx_ = M32(0x4441E8u);
/* 0x004167D6 mov [edx+1], ah */ M8(edx_ + 1u) = (unsigned char)((eax_ >> 8) & 0xFFu);
/* 0x004167D9 mov dword_4441F0, ebx */ M32(0x4441F0u) = ebx_;
l_4167df:
/* 0x004167DF cmp byte ptr [edx], 0FEh */
/* 0x004167E2 jnb short loc_4167EA */ if (G8(edx_) >= 0xFEu) goto l_4167ea;
/* 0x004167E4 cmp byte ptr [edx+1], 0FEh */
/* 0x004167E8 jb short loc_416805 */ if (G8(edx_ + 1u) < 0xFEu) goto l_416805;
l_4167ea:
/* 0x004167EA mov ch, [edx] */ ecx_ = (ecx_ & 0xFFFF00FFUL) | (((unsigned int)G8(edx_) << 8) & 0xFF00u);
/* 0x004167EC mov bl, [edx+1] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(edx_ + 1u);
/* 0x004167EF inc ch */ ecx_ = (ecx_ & 0xFFFF00FFUL) | (((((ecx_ >> 8) & 0xFFu) + 1u) & 0xFFu) << 8);
/* 0x004167F1 inc bl */ ebx_ = (ebx_ & 0xFFFFFF00UL) | ((ebx_ + 1u) & 0xFFu);
/* 0x004167F3 mov [edx], ch */ M8(edx_) = (unsigned char)((ecx_ >> 8) & 0xFFu);
/* 0x004167F5 mov ah, ch */ eax_ = (eax_ & 0xFFFF00FFUL) | (ecx_ & 0xFF00u);
/* 0x004167F7 mov [edx+1], bl */ M8(edx_ + 1u) = (unsigned char)(ebx_ & 0xFFu);
/* 0x004167FA mov bh, bl */ ebx_ = (ebx_ & 0xFFFF00FFUL) | ((ebx_ & 0xFFu) << 8);
/* 0x004167FC shr ah, 1 */ eax_ = (eax_ & 0xFFFF00FFUL) | (((((eax_ >> 8) & 0xFFu) >> 1) & 0xFFu) << 8);
/* 0x004167FE shr bh, 1 */ ebx_ = (ebx_ & 0xFFFF00FFUL) | (((((ebx_ >> 8) & 0xFFu) >> 1) & 0xFFu) << 8);
/* 0x00416800 mov [edx], ah */ M8(edx_) = (unsigned char)((eax_ >> 8) & 0xFFu);
/* 0x00416802 mov [edx+1], bh */ M8(edx_ + 1u) = (unsigned char)((ebx_ >> 8) & 0xFFu);
l_416805:
/* 0x00416805 pop edx */
/* 0x00416806 pop ecx */
/* 0x00416807 pop ebx */
/* 0x00416808 retn */ return (eax_ & 0xFFu);
}
/* write is xor at 0x41506F). Exit eax = table-entry pointer, or 0. */
unsigned int d3u_415058(dec3w_ctx *c)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int ecx_ = 0;
unsigned int edx_ = 0;
unsigned int esi_ = 0;
unsigned int edi_ = 0;
unsigned int ebp_ = 0;
unsigned int v24 = 0;
unsigned int v20 = 0;
unsigned int v1c = 0;
unsigned int qv = 0;
/* 0x00415058 push ebx */
/* 0x00415059 push ecx */
/* 0x0041505A push edx */
/* 0x0041505B push esi */
/* 0x0041505C push edi */
/* 0x0041505D push ebp */
/* 0x0041505E sub esp, 0Ch */
/* 0x00415061 mov bh, 8 */ ebx_ = (ebx_ & 0xFFFF00FFUL) | (8u << 8);
/* 0x00415063 mov edx, dword_444258 */ edx_ = M32(0x444258u);
/* 0x00415069 mov esi, edx */ esi_ = edx_;
/* 0x0041506B mov edi, edx */ edi_ = edx_;
/* 0x0041506D xor ecx, ecx */ ecx_ = 0u;
/* 0x0041506F xor eax, eax */ eax_ = 0u;
/* 0x00415071 mov dword_44427C, ecx */ M32(0x44427Cu) = ecx_;
/* 0x00415077 jmp short loc_41507F */ goto l_41507f;
l_41507f:
/* 0x0041507F shr edx, 8 */ edx_ >>= 8;
/* 0x00415082 shr esi, 10h */ esi_ >>= 16;
/* 0x00415085 xor di, di */ edi_ &= 0xFFFF0000UL;
/* 0x00415088 xor bl, bl */ ebx_ &= 0xFFFFFF00UL;
/* 0x0041508A cmp edi, 0FFFF0000h */
/* 0x00415090 jnz short loc_41509C */ if (edi_ != 0xFFFF0000UL) goto l_41509c;
/* 0x00415092 mov eax, 10000h */ eax_ = 0x10000u;
/* 0x00415097 jmp loc_415124 */ goto l_415124;
l_41509c:
/* 0x0041509C mov edi, dword_444258 */ edi_ = M32(0x444258u);
/* 0x004150A2 and edi, 0FFFFh */ edi_ &= 0xFFFFu;
/* 0x004150A8 cmp edi, 0FFFFh */
/* 0x004150AE jnz short loc_4150BA */ if (edi_ != 0xFFFFu) goto l_4150ba;
/* 0x004150B0 mov eax, 20000h */ eax_ = 0x20000u;
/* 0x004150B5 jmp loc_415124 */ goto l_415124;
l_4150ba:
/* 0x004150BA mov edi, dword_444258 */ edi_ = M32(0x444258u);
/* 0x004150C0 and edi, 0FF000000h */ edi_ &= 0xFF000000UL;
/* 0x004150C6 cmp edi, 0FF000000h */
/* 0x004150CC jnz short loc_4150D5 */ if (edi_ != 0xFF000000UL) goto l_4150d5;
/* 0x004150CE mov eax, 30000h */ eax_ = 0x30000u;
/* 0x004150D3 jmp short loc_415124 */ goto l_415124;
l_4150d5:
/* 0x004150D5 mov edi, dword_444258 */ edi_ = M32(0x444258u);
/* 0x004150DB and edi, 0FF0000h */ edi_ &= 0xFF0000u;
/* 0x004150E1 cmp edi, 0FF0000h */
/* 0x004150E7 jnz short loc_4150F0 */ if (edi_ != 0xFF0000u) goto l_4150f0;
/* 0x004150E9 mov eax, 40000h */ eax_ = 0x40000u;
/* 0x004150EE jmp short loc_415124 */ goto l_415124;
l_4150f0:
/* 0x004150F0 mov edi, dword_444258 */ edi_ = M32(0x444258u);
/* 0x004150F6 and edi, 0FF00h */ edi_ &= 0xFF00u;
/* 0x004150FC cmp edi, 0FF00h */
/* 0x00415102 jnz short loc_41510B */ if (edi_ != 0xFF00u) goto l_41510b;
/* 0x00415104 mov eax, 50000h */ eax_ = 0x50000u;
/* 0x00415109 jmp short loc_415124 */ goto l_415124;
l_41510b:
/* 0x0041510B mov edi, dword_444258 */ edi_ = M32(0x444258u);
/* 0x00415111 and edi, 0FFh */ edi_ &= 0xFFu;
/* 0x00415117 cmp edi, 0FFh */
/* 0x0041511D jnz short loc_415124 */ if (edi_ != 0xFFu) goto l_415124;
/* 0x0041511F mov eax, 60000h */ eax_ = 0x60000u;
l_415124:
/* 0x00415124 mov edi, esi */ edi_ = esi_;
/* 0x00415126 mov ebp, dword_44425C */ ebp_ = M32(0x44425Cu);
/* 0x0041512C and edi, 0FF00h */ edi_ &= 0xFF00u;
/* 0x00415132 shr ebp, 18h */ ebp_ >>= 24;
/* 0x00415135 and esi, 0FFFFh */ esi_ &= 0xFFFFu;
/* 0x0041513B mov dword_443CF0, esi */ M32(0x443CF0u) = esi_;
/* 0x00415141 add ebp, edi */ ebp_ = (ebp_ + edi_) & U32MASK;
/* 0x00415143 mov esi, edx */ esi_ = edx_;
/* 0x00415145 mov dword_443CE8, ebp */ M32(0x443CE8u) = ebp_;
/* 0x0041514B and esi, 0FF00h */ esi_ &= 0xFF00u;
/* 0x00415151 mov ebp, edx */ ebp_ = edx_;
/* 0x00415153 mov [esp+24h+var_24], esi */ v24 = esi_;
/* 0x00415156 and ebp, 0FFFFh */ ebp_ &= 0xFFFFu;
/* 0x0041515C mov esi, dword_444258 */ esi_ = M32(0x444258u);
/* 0x00415162 mov dword_443CEC, ebp */ M32(0x443CECu) = ebp_;
/* 0x00415168 and esi, 0FFh */ esi_ &= 0xFFu;
/* 0x0041516E mov ebp, [esp+24h+var_24] */ ebp_ = v24;
/* 0x00415171 add ebp, esi */ ebp_ = (ebp_ + esi_) & U32MASK;
/* 0x00415173 and edx, 0FFh */ edx_ &= 0xFFu;
/* 0x00415179 mov dword_443CF4, ebp */ M32(0x443CF4u) = ebp_;
/* 0x0041517F lea ebp, [edi+edx] */ ebp_ = (edi_ + edx_) & U32MASK;
/* 0x00415182 mov edx, dword_44425C */ edx_ = M32(0x44425Cu);
/* 0x00415188 mov dword_443CF8, ebp */ M32(0x443CF8u) = ebp_;
/* 0x0041518E and edx, 0FFFFh */ edx_ &= 0xFFFFu;
/* 0x00415194 add edi, esi */ edi_ = (edi_ + esi_) & U32MASK;
/* 0x00415196 add edx, eax */ edx_ = (edx_ + eax_) & U32MASK;
/* 0x00415198 mov dword_443CFC, edi */ M32(0x443CFCu) = edi_;
/* 0x0041519E mov dword_443D00, edx */ M32(0x443D00u) = edx_;
/* 0x004151A4 cmp esi, 0FFh */
/* 0x004151AA jnz short loc_4151CA */ if (esi_ != 0xFFu) goto l_4151ca;
/* 0x004151AC mov dx, word ptr dword_443CEC+2 */ edx_ = (edx_ & 0xFFFF0000UL) | (unsigned int)M16(0x443CECu + 2u);
/* 0x004151B3 mov si, word ptr dword_443CF0+2 */ esi_ = (esi_ & 0xFFFF0000UL) | (unsigned int)M16(0x443CF0u + 2u);
/* 0x004151BA inc edx */ edx_ = (edx_ + 1u) & U32MASK;
/* 0x004151BB inc esi */ esi_ = (esi_ + 1u) & U32MASK;
/* 0x004151BC mov word ptr dword_443CEC+2, dx */ M16(0x443CECu + 2u) = (unsigned short)(edx_ & 0xFFFFu);
/* 0x004151C3 mov word ptr dword_443CF0+2, si */ M16(0x443CF0u + 2u) = (unsigned short)(esi_ & 0xFFFFu);
l_4151ca:
/* 0x004151CA movzx esi, byte_444EB9 */ esi_ = (unsigned int)M8(0x444EB9u);
/* 0x004151D1 mov edx, dword_444274 */ edx_ = M32(0x444274u);
/* 0x004151D7 mov edi, dword_444278 */ edi_ = M32(0x444278u);
/* 0x004151DD sub edx, esi */ edx_ = (edx_ - esi_) & U32MASK;
/* 0x004151DF mov eax, dword_4299B8 */ eax_ = M32(0x4299B8u);
/* 0x004151E4 and edx, edi */ edx_ &= edi_;
/* 0x004151E6 mov dl, [edx+eax] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(edx_ + eax_);
/* 0x004151E9 and edx, 0FFh */ edx_ &= 0xFFu;
/* 0x004151EF mov [esp+24h+var_1C], edx */ v1c = edx_;
/* 0x004151F3 xor edx, edx */ edx_ = 0u;
/* 0x004151F5 mov dl, byte_444EB9 */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EB9u);
/* 0x004151FB mov edi, dword_444274 */ edi_ = M32(0x444274u);
/* 0x00415201 add edx, edx */ edx_ = (edx_ + edx_) & U32MASK;
/* 0x00415203 sub edi, edx */ edi_ = (edi_ - edx_) & U32MASK;
/* 0x00415205 mov ebp, dword_444278 */ ebp_ = M32(0x444278u);
/* 0x0041520B mov edx, edi */ edx_ = edi_;
/* 0x0041520D and edx, ebp */ edx_ &= ebp_;
/* 0x0041520F mov al, [edx+eax] */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)G8(edx_ + eax_);
/* 0x00415212 and eax, 0FFh */ eax_ &= 0xFFu;
/* 0x00415217 mov [esp+24h+var_20], eax */ v20 = eax_;
/* 0x0041521B mov al, byte_444EB9 */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EB9u);
/* 0x00415220 cmp al, byte_444EA2 */
/* 0x00415226 jnz short loc_415252 */ if ((eax_ & 0xFFu) != (unsigned int)M8(0x444EA2u)) goto l_415252;
/* 0x00415228 cmp byte ptr dword_444258, 1 */
/* 0x0041522F jnz short loc_415233 */ if (M8(0x444258u) != 1u) goto l_415233;
/* 0x00415231 mov bl, 8 */ ebx_ = (ebx_ & 0xFFFFFF00UL) | 8u;
l_415233:
/* 0x00415233 mov eax, dword_444258 */ eax_ = M32(0x444258u);
/* 0x00415238 shr eax, 10h */ eax_ >>= 16;
/* 0x0041523B cmp eax, 0F6FFh */
/* 0x00415240 setz al */ eax_ = (eax_ & 0xFFFFFF00UL) | ((eax_ == 0xF6FFu) ? 1u : 0u);
/* 0x00415243 and eax, 0FFh */ eax_ &= 0xFFu;
/* 0x00415248 shl eax, 8 */ eax_ = (eax_ << 8) & U32MASK;
/* 0x0041524B mov dword_443D04, eax */ M32(0x443D04u) = eax_;
/* 0x00415250 jmp short loc_415264 */ goto l_415264;
l_415252:
/* 0x00415252 mov eax, dword_4441C0 */ eax_ = M32(0x4441C0u);
/* 0x00415257 xor edx, edx */ edx_ = 0u;
/* 0x00415259 div esi */ qv = eax_; eax_ = qv / esi_; edx_ = qv % esi_;
/* 0x0041525B shl edx, 8 */ edx_ = (edx_ << 8) & U32MASK;
/* 0x0041525E mov dword_443D04, edx */ M32(0x443D04u) = edx_;
l_415264:
/* 0x00415264 mov dl, byte ptr [esp+24h+var_1C] */ edx_ = (edx_ & 0xFFFFFF00UL) | (v1c & 0xFFu);
/* 0x00415268 add dl, dl */ edx_ = (edx_ & 0xFFFFFF00UL) | (((edx_ & 0xFFu) + (edx_ & 0xFFu)) & 0xFFu);
/* 0x0041526A mov dh, byte ptr [esp+24h+var_20] */ edx_ = (edx_ & 0xFFFF00FFUL) | ((v20 & 0xFFu) << 8);
/* 0x0041526E mov al, byte ptr [esp+24h+var_1C] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v1c & 0xFFu);
/* 0x00415272 sub dl, dh */ edx_ = (edx_ & 0xFFFFFF00UL) | (((edx_ & 0xFFu) - ((edx_ >> 8) & 0xFFu)) & 0xFFu);
/* 0x00415274 sub al, dh */ eax_ = (eax_ & 0xFFFFFF00UL) | (((eax_ & 0xFFu) - ((edx_ >> 8) & 0xFFu)) & 0xFFu);
/* 0x00415276 mov esi, dword_443D04 */ esi_ = M32(0x443D04u);
/* 0x0041527C and eax, 0FFh */ eax_ &= 0xFFu;
/* 0x00415281 add esi, eax */ esi_ = (esi_ + eax_) & U32MASK;
/* 0x00415283 mov eax, esi */ eax_ = esi_;
/* 0x00415285 mov dword_443D04, esi */ M32(0x443D04u) = esi_;
/* 0x0041528B mov esi, dword_4299EC */ esi_ = M32(0x4299ECu);
/* 0x00415291 mov [esi+eax*4+2], dl */ M8(esi_ + eax_ * 4u + 2u) = (unsigned char)(edx_ & 0xFFu);
/* 0x00415295 cmp bl, 8 */
/* 0x00415298 jnb short loc_4152E5 */ if ((ebx_ & 0xFFu) >= 8u) goto l_4152e5;
l_41529a:
/* 0x0041529A xor eax, eax */ eax_ = 0u;
/* 0x0041529C mov al, bl */ eax_ = (eax_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x0041529E mov edx, dword_443CE8[eax*4] */ edx_ = M32(0x443CE8u + (eax_ & 0xFFu) * 4u);
/* 0x004152A5 shl edx, 2 */ edx_ = (edx_ << 2) & U32MASK;
/* 0x004152A8 mov eax, dword_4299D0[eax*4] */ eax_ = M32(0x4299D0u + (eax_ & 0xFFu) * 4u);
/* 0x004152AF add eax, edx */ eax_ = (eax_ + edx_) & U32MASK;
/* 0x004152B1 cmp byte ptr [eax+1], 0 */
/* 0x004152B5 jz short loc_4152DE */ if (G8(eax_ + 1u) == 0u) goto l_4152de;
/* 0x004152B7 cmp byte ptr [eax+3], 80h */
/* 0x004152BB jnb short loc_4152DE */ if (G8(eax_ + 3u) >= 0x80u) goto l_4152de;
/* 0x004152BD movzx esi, byte ptr [eax] */ esi_ = (unsigned int)G8(eax_);
/* 0x004152C0 xor edx, edx */ edx_ = 0u;
/* 0x004152C2 shl esi, 8 */ esi_ = (esi_ << 8) & U32MASK;
/* 0x004152C5 mov dl, [eax+3] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_ + 3u);
/* 0x004152C8 imul edx, esi */ edx_ = (edx_ * esi_) & U32MASK;
/* 0x004152CB movzx esi, byte ptr [eax+1] */ esi_ = (unsigned int)G8(eax_ + 1u);
/* 0x004152CF mov eax, edx */ eax_ = edx_;
/* 0x004152D1 sar edx, 1Fh */ edx_ = (unsigned int)(((int)edx_) >> 31) & U32MASK;
/* 0x004152D4 idiv esi */ qv = eax_; eax_ = (unsigned int)(((int)qv) / ((int)esi_)); edx_ = (unsigned int)(((int)qv) % ((int)esi_));
/* 0x004152D6 cmp eax, ecx */
/* 0x004152D8 jbe short loc_4152DE */ if (eax_ <= ecx_) goto l_4152de;
/* 0x004152DA mov ecx, eax */ ecx_ = eax_;
/* 0x004152DC mov bh, bl */ ebx_ = (ebx_ & 0xFFFF00FFUL) | ((ebx_ & 0xFFu) << 8);
l_4152de:
/* 0x004152DE inc bl */ ebx_ = (ebx_ & 0xFFFFFF00UL) | ((ebx_ + 1u) & 0xFFu);
/* 0x004152E0 cmp bl, 8 */
/* 0x004152E3 jb short loc_41529A */ if ((ebx_ & 0xFFu) < 8u) goto l_41529a;
l_4152e5:
/* 0x004152E5 cmp bh, 8 */
/* 0x004152E8 jnb loc_4153A2 */ if (((ebx_ >> 8) & 0xFFu) >= 8u) goto l_4153a2;
/* 0x004152EE xor eax, eax */ eax_ = 0u;
/* 0x004152F0 mov al, bh */ eax_ = (eax_ & 0xFFFFFF00UL) | ((ebx_ >> 8) & 0xFFu);
/* 0x004152F2 mov edx, dword_443CE8[eax*4] */ edx_ = M32(0x443CE8u + (eax_ & 0xFFu) * 4u);
/* 0x004152F9 shl edx, 2 */ edx_ = (edx_ << 2) & U32MASK;
/* 0x004152FC mov eax, dword_4299D0[eax*4] */ eax_ = M32(0x4299D0u + (eax_ & 0xFFu) * 4u);
/* 0x00415303 add eax, edx */ eax_ = (eax_ + edx_) & U32MASK;
/* 0x00415305 mov dl, [eax] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x00415307 cmp dl, [eax+1] */
/* 0x0041530A ja short loc_41531B */ if ((edx_ & 0xFFu) > (unsigned int)G8(eax_ + 1u)) goto l_41531b;
/* 0x0041530C mov dl, [eax+3] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_ + 3u);
/* 0x0041530F and dl, 7Fh */ edx_ = (edx_ & 0xFFFFFF00UL) | ((edx_ & 0xFFu) & 0x7Fu);
/* 0x00415312 cmp dl, 1 */
/* 0x00415315 jbe loc_4153A2 */ if ((edx_ & 0xFFu) <= 1u) goto l_4153a2;
l_41531b:
/* 0x0041531B cmp byte_444EB9, 1 */
/* 0x00415322 jbe short loc_415330 */ if ((unsigned int)M8(0x444EB9u) <= 1u) goto l_415330;
/* 0x00415324 cmp bh, 7 */
/* 0x00415327 setz dl */ edx_ = (edx_ & 0xFFFFFF00UL) | (((((ebx_ >> 8) & 0xFFu) == 7u) ? 1u : 0u));
/* 0x0041532A mov byte_444EA4, dl */ M8(0x444EA4u) = (unsigned char)(edx_ & 0xFFu);
l_415330:
/* 0x00415330 mov ch, [eax+1] */ ecx_ = (ecx_ & 0xFFFF00FFUL) | (((unsigned int)G8(eax_ + 1u) << 8) & 0xFF00u);
/* 0x00415333 cmp ch, 2 */
/* 0x00415336 jnz short loc_415360 */ if (((ecx_ >> 8) & 0xFFu) != 2u) goto l_415360;
/* 0x00415338 mov dl, [eax] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x0041533A sub dl, 3 */ edx_ = (edx_ & 0xFFFFFF00UL) | (((edx_ & 0xFFu) - 3u) & 0xFFu);
/* 0x0041533D cmp dl, ch */
/* 0x0041533F jnb short loc_415360 */ if ((edx_ & 0xFFu) >= ((ecx_ >> 8) & 0xFFu)) goto l_415360;
/* 0x00415341 xor edx, edx */ edx_ = 0u;
/* 0x00415343 mov dl, [eax] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x00415345 xor ecx, ecx */ ecx_ = 0u;
/* 0x00415347 shl edx, 3 */ edx_ = (edx_ << 3) & U32MASK;
/* 0x0041534A mov cl, bh */ ecx_ = (ecx_ & 0xFFFFFF00UL) | ((ebx_ >> 8) & 0xFFu);
/* 0x0041534C sub edx, 18h */ edx_ = (edx_ - 0x18u) & U32MASK;
/* 0x0041534F add edx, ecx */ edx_ = (edx_ + ecx_) & U32MASK;
/* 0x00415351 mov ecx, offset dword_444148 */ ecx_ = 0x444148u;
/* 0x00415356 add edx, edx */ edx_ = (edx_ + edx_) & U32MASK;
/* 0x00415358 add ecx, edx */ ecx_ = (ecx_ + edx_) & U32MASK;
/* 0x0041535A mov dword_44427C, ecx */ M32(0x44427Cu) = ecx_;
l_415360:
/* 0x00415360 cmp byte ptr [eax+1], 3 */
/* 0x00415364 jnz short loc_415392 */ if (G8(eax_ + 1u) != 3u) goto l_415392;
/* 0x00415366 mov dl, [eax] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x00415368 sub dl, 4 */ edx_ = (edx_ & 0xFFFFFF00UL) | (((edx_ & 0xFFu) - 4u) & 0xFFu);
/* 0x0041536B cmp dl, 2 */
/* 0x0041536E jnb short loc_415392 */ if ((edx_ & 0xFFu) >= 2u) goto l_415392;
/* 0x00415370 xor edx, edx */ edx_ = 0u;
/* 0x00415372 mov dl, [eax] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x00415374 xor ecx, ecx */ ecx_ = 0u;
/* 0x00415376 shl edx, 3 */ edx_ = (edx_ << 3) & U32MASK;
/* 0x00415379 mov cl, bh */ ecx_ = (ecx_ & 0xFFFFFF00UL) | ((ebx_ >> 8) & 0xFFu);
/* 0x0041537B sub edx, 20h */ edx_ = (edx_ - 0x20u) & U32MASK;
/* 0x0041537E add edx, ecx */ edx_ = (edx_ + ecx_) & U32MASK;
/* 0x00415380 add edx, edx */ edx_ = (edx_ + edx_) & U32MASK;
/* 0x00415382 mov ecx, offset dword_444148 */ ecx_ = 0x444148u;
/* 0x00415387 add edx, 20h */ edx_ = (edx_ + 0x20u) & U32MASK;
/* 0x0041538A add ecx, edx */ ecx_ = (ecx_ + edx_) & U32MASK;
/* 0x0041538C mov dword_44427C, ecx */ M32(0x44427Cu) = ecx_;
l_415392:
/* 0x00415392 mov byte_444EA3, bh */ M8(0x444EA3u) = (unsigned char)((ebx_ >> 8) & 0xFFu);
/* 0x00415398 add esp, 0Ch */
/* 0x0041539B pop ebp */
/* 0x0041539C pop edi */
/* 0x0041539D pop esi */
/* 0x0041539E pop edx */
/* 0x0041539F pop ecx */
/* 0x004153A0 pop ebx */
/* 0x004153A1 retn */ return eax_;
l_4153a2:
/* 0x004153A2 cmp byte_444EBA, 1 */
/* 0x004153A9 jbe short loc_4153B2 */ if ((unsigned int)M8(0x444EBAu) <= 1u) goto l_4153b2;
/* 0x004153AB mov eax, dword_4441F8 */ eax_ = M32(0x4441F8u);
/* 0x004153B0 shr dword ptr [eax], 1 */ M32(eax_) >>= 1;
l_4153b2:
/* 0x004153B2 xor eax, eax */ eax_ = 0u;
/* 0x004153B4 add esp, 0Ch */
/* 0x004153B7 pop ebp */
/* 0x004153B8 pop edi */
/* 0x004153B9 pop esi */
/* 0x004153BA pop edx */
/* 0x004153BB pop ecx */
/* 0x004153BC pop ebx */
/* 0x004153BD retn */ return eax_;
}
/* ebx access is write mov bh,al at 0x417526. Exit al (mov al,bl tail). */
unsigned int d3u_4174fc(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int ecx_ = 0;
unsigned int edx_ = 0;
unsigned int esi_ = 0;
unsigned int edi_ = 0;
unsigned int ebp_ = 0;
unsigned int stk_d = 0;
unsigned int stk_b = 0;
/* 0x004174FC push ebx */
/* 0x004174FD push ecx */
/* 0x004174FE push esi */
/* 0x004174FF push edi */
/* 0x00417500 push ebp */
/* 0x00417501 sub esp, 8 */
/* 0x00417504 mov ecx, eax */ ecx_ = eax_in;
/* 0x00417506 mov esi, edx */ esi_ = edx_in;
/* 0x00417508 mov edi, dword_4441E4 */ edi_ = M32(0x4441E4u);
/* 0x0041750E cmp edx, 2FDh */
/* 0x00417514 setz al */ eax_ = (eax_ & 0xFFFFFF00UL) | ((edx_in == 0x2FDu) ? 1u : 0u);
/* 0x00417517 mov dword_44421C, esi */ M32(0x44421Cu) = esi_;
/* 0x0041751D mov edx, dword_4441FC */ edx_ = M32(0x4441FCu);
/* 0x00417523 xor ebp, ebp */ ebp_ = 0u;
/* 0x00417525 inc edx */ edx_ = (edx_ + 1u) & U32MASK;
/* 0x00417526 mov bh, al */ ebx_ = (ebx_ & 0xFFFF00FFUL) | ((eax_ & 0xFFu) << 8);
/* 0x00417528 and edx, 0FFh */ edx_ &= 0xFFu;
/* 0x0041752E mov dword_4441F4, ebp */ M32(0x4441F4u) = ebp_;
/* 0x00417534 mov dword_4441FC, edx */ M32(0x4441FCu) = edx_;
/* 0x0041753A mov edx, dword_442E58[edx*4] */ edx_ = M32(0x442E58u + edx_ * 4u);
/* 0x00417541 mov ah, 1 */ eax_ = (eax_ & 0xFFFF00FFUL) | (1u << 8);
/* 0x00417543 sub edi, edx */ edi_ = (edi_ - edx_) & U32MASK;
/* 0x00417545 mov [esp+1Ch+var_18], ah */ stk_b = (eax_ >> 8) & 0xFFu;
/* 0x00417549 mov dword_4441E4, edi */ M32(0x4441E4u) = edi_;
/* 0x0041754F cmp edi, 6D4250h */
/* 0x00417555 jb loc_4176FB */ if (edi_ < 0x6D4250u) goto l_4176fb;
/* 0x0041755B cmp edi, 0FEFF01h */
/* 0x00417561 jnb loc_4176FB */ if (edi_ >= 0xFEFF01u) goto l_4176fb;
/* 0x00417567 cmp edi, 91ADC0h */
/* 0x0041756D jb short loc_4175CB */ if (edi_ < 0x91ADC0u) goto l_4175cb;
/* 0x0041756F mov esi, 100h */ esi_ = 0x100u;
/* 0x00417574 xor eax, eax */ eax_ = 0u;
/* 0x00417576 mov dword_4441E8, esi */ M32(0x4441E8u) = esi_;
/* 0x0041757C mov ax, word ptr dword_4441E8 */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)M16(0x4441E8u);
/* 0x00417582 call sub_423188 */ eax_ = d3u_423188(c, eax_);
/* 0x00417587 mov edx, eax */ edx_ = eax_;
/* 0x00417589 mov bl, al */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (eax_ & 0xFFu);
/* 0x0041758B xor eax, eax */ eax_ = 0u;
/* 0x0041758D mov al, dl */ eax_ = (eax_ & 0xFFFFFF00UL) | (edx_ & 0xFFu);
/* 0x0041758F mov dword_4441EC, eax */ M32(0x4441ECu) = eax_;
/* 0x00417594 xor eax, eax */ eax_ = 0u;
/* 0x00417596 mov al, dl */ eax_ = (eax_ & 0xFFFFFF00UL) | (edx_ & 0xFFu);
/* 0x00417598 inc eax */ eax_ = (eax_ + 1u) & U32MASK;
/* 0x00417599 mov dword_4441F0, eax */ M32(0x4441F0u) = eax_;
/* 0x0041759E mov eax, offset dword_4441E8 */ eax_ = 0x4441E8u;
/* 0x004175A3 call sub_423140 */ d3u_423140(c, eax_);
/* 0x004175A8 cmp byte_444EAE, 0 */
/* 0x004175AF jz short loc_4175BB */ if (M8(0x444EAEu) == 0u) goto l_4175bb;
/* 0x004175B1 add dword_44420C, 9240h */ M32(0x44420Cu) = (M32(0x44420Cu) + 0x9240u) & U32MASK;
l_4175bb:
/* 0x004175BB xor edx, edx */ edx_ = 0u;
/* 0x004175BD mov eax, ecx */ eax_ = ecx_;
/* 0x004175BF mov dl, bl */ edx_ = (edx_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x004175C1 call sub_417D74 */ eax_ = d3u_417d74(c, eax_, edx_);
/* 0x004175C6 jmp loc_417719 */ goto l_417719;
l_4175cb:
/* 0x004175CB xor edx, edx */ edx_ = 0u;
/* 0x004175CD mov dl, al */ edx_ = (edx_ & 0xFFFFFF00UL) | (eax_ & 0xFFu);
/* 0x004175CF mov edi, dword_443258[edx*4] */ edi_ = M32(0x443258u + (edx_ & 0xFFu) * 4u);
/* 0x004175D6 cmp edi, dword_443B60[edx*4] */
/* 0x004175DD jb short loc_417626 */ if (edi_ < M32(0x443B60u + (edx_ & 0xFFu) * 4u)) goto l_417626;
/* 0x004175DF mov eax, ecx */ eax_ = ecx_;
/* 0x004175E1 call sub_4177C0 */ eax_ = d3u_4177c0(c, eax_);
/* 0x004175E6 mov bl, al */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (eax_ & 0xFFu);
/* 0x004175E8 mov al, byte_444EAE */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EAEu);
/* 0x004175ED mov ebp, dword_4441F4 */ ebp_ = M32(0x4441F4u);
/* 0x004175F3 test al, al */
/* 0x004175F5 jz short loc_4175FD */ if ((eax_ & 0xFFu) == 0u) goto l_4175fd;
/* 0x004175F7 add dword_44420C, ebp */ M32(0x44420Cu) = (M32(0x44420Cu) + ebp_) & U32MASK;
l_4175fd:
/* 0x004175FD xor eax, eax */ eax_ = 0u;
/* 0x004175FF mov dword_4441F4, eax */ M32(0x4441F4u) = eax_;
/* 0x00417604 mov al, bh */ eax_ = (eax_ & 0xFFFFFF00UL) | ((ebx_ >> 8) & 0xFFu);
/* 0x00417606 xor edx, edx */ edx_ = 0u;
/* 0x00417608 add eax, 2FEh */ eax_ = (eax_ + 0x2FEu) & U32MASK;
/* 0x0041760D mov dl, bl */ edx_ = (edx_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x0041760F mov dword_44421C, eax */ M32(0x44421Cu) = eax_;
/* 0x00417614 mov eax, 200h */ eax_ = 0x200u;
/* 0x00417619 call sub_417D74 */ eax_ = d3u_417d74(c, eax_, edx_);
/* 0x0041761E mov edi, dword_4441F4 */ edi_ = M32(0x4441F4u);
/* 0x00417624 jmp short loc_417671 */ goto l_417671;
l_417626:
/* 0x00417626 add edx, 2FEh */ edx_ = (edx_ + 0x2FEu) & U32MASK;
/* 0x0041762C mov eax, 200h */ eax_ = 0x200u;
/* 0x00417631 mov dword_44421C, edx */ M32(0x44421Cu) = edx_;
/* 0x00417637 call sub_4177C0 */ eax_ = d3u_4177c0(c, eax_);
/* 0x0041763C mov edi, dword_4441F4 */ edi_ = M32(0x4441F4u);
/* 0x00417642 mov dh, byte_444EAE */ edx_ = (edx_ & 0xFFFF00FFUL) | (((unsigned int)M8(0x444EAEu) << 8) & 0xFF00u);
/* 0x00417648 mov bl, al */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (eax_ & 0xFFu);
/* 0x0041764A test dh, dh */
/* 0x0041764C jz short loc_417654 */ if (((edx_ >> 8) & 0xFFu) == 0u) goto l_417654;
/* 0x0041764E add dword_44420C, edi */ M32(0x44420Cu) = (M32(0x44420Cu) + edi_) & U32MASK;
l_417654:
/* 0x00417654 xor edx, edx */ edx_ = 0u;
/* 0x00417656 mov eax, ecx */ eax_ = ecx_;
/* 0x00417658 mov dword_4441F4, edx */ M32(0x4441F4u) = edx_;
/* 0x0041765E mov dl, bl */ edx_ = (edx_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x00417660 mov dword_44421C, esi */ M32(0x44421Cu) = esi_;
/* 0x00417666 call sub_417D74 */ eax_ = d3u_417d74(c, eax_, edx_);
/* 0x0041766B mov ebp, dword_4441F4 */ ebp_ = M32(0x4441F4u);
l_417671:
/* 0x00417671 xor eax, eax */ eax_ = 0u;
/* 0x00417673 mov al, bh */ eax_ = (eax_ & 0xFFFFFF00UL) | ((ebx_ >> 8) & 0xFFu);
/* 0x00417675 mov [esp+1Ch+var_1C], eax */ stk_d = eax_;
/* 0x00417678 add eax, 2FEh */ eax_ = (eax_ + 0x2FEu) & U32MASK;
/* 0x0041767D mov dword_44421C, eax */ M32(0x44421Cu) = eax_;
/* 0x00417682 xor eax, eax */ eax_ = 0u;
/* 0x00417684 mov al, byte_444EB3 */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EB3u);
/* 0x00417689 mov edx, dword_44421C */ edx_ = M32(0x44421Cu);
/* 0x0041768F mov dword_441F08[eax*4], edx */ M32(0x441F08u + (eax_ & 0xFFu) * 4u) = edx_;
/* 0x00417696 mov dword_442018[eax*4], 200h */ M32(0x442018u + (eax_ & 0xFFu) * 4u) = 0x200u;
/* 0x004176A1 inc byte_444EB3 */ M8(0x444EB3u) = (unsigned char)((M8(0x444EB3u) + 1u) & 0xFFu);
/* 0x004176A7 mov edx, dword_4441E4 */ edx_ = M32(0x4441E4u);
/* 0x004176AD add edx, edi */ edx_ = (edx_ + edi_) & U32MASK;
/* 0x004176AF mov byte_4441D8[eax], bl */ M8(0x4441D8u + (eax_ & 0xFFu)) = (unsigned char)(ebx_ & 0xFFu);
/* 0x004176B5 mov dword_4441E4, edx */ M32(0x4441E4u) = edx_;
/* 0x004176BB mov edx, [esp+1Ch+var_1C] */ edx_ = stk_d;
/* 0x004176BE mov eax, dword_4441FC */ eax_ = M32(0x4441FCu);
/* 0x004176C3 shl edx, 2 */ edx_ = (edx_ << 2) & U32MASK;
/* 0x004176C6 mov dword_442E58[eax*4], edi */ M32(0x442E58u + eax_ * 4u) = edi_;
/* 0x004176CD mov eax, dword_443258[edx] */ eax_ = M32(0x443258u + edx_);
/* 0x004176D3 mov [esp+1Ch+var_1C], eax */ stk_d = eax_;
/* 0x004176D6 shl eax, 5 */ eax_ = (eax_ << 5) & U32MASK;
/* 0x004176D9 sub eax, [esp+1Ch+var_1C] */ eax_ = (eax_ - stk_d) & U32MASK;
/* 0x004176DC shr eax, 5 */ eax_ >>= 5;
/* 0x004176DF add eax, edi */ eax_ = (eax_ + edi_) & U32MASK;
/* 0x004176E1 mov dword_4441F4, ebp */ M32(0x4441F4u) = ebp_;
/* 0x004176E7 mov dword_443258[edx], eax */ M32(0x443258u + edx_) = eax_;
/* 0x004176ED xor dh, dh */ edx_ = (edx_ & 0xFFFF00FFUL);
/* 0x004176EF mov dword_44421C, esi */ M32(0x44421Cu) = esi_;
/* 0x004176F5 mov [esp+1Ch+var_18], dh */ stk_b = (edx_ >> 8) & 0xFFu;
/* 0x004176F9 jmp short loc_417719 */ goto l_417719;
l_4176fb:
/* 0x004176FB mov eax, ecx */ eax_ = ecx_;
/* 0x004176FD call sub_4177C0 */ eax_ = d3u_4177c0(c, eax_);
/* 0x00417702 mov dl, byte_444EAE */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EAEu);
/* 0x00417708 mov bl, al */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (eax_ & 0xFFu);
/* 0x0041770A test dl, dl */
/* 0x0041770C jz short loc_417719 */ if ((edx_ & 0xFFu) == 0u) goto l_417719;
/* 0x0041770E mov eax, dword_4441F4 */ eax_ = M32(0x4441F4u);
/* 0x00417713 add dword_44420C, eax */ M32(0x44420Cu) = (M32(0x44420Cu) + eax_) & U32MASK;
l_417719:
/* 0x00417719 xor eax, eax */ eax_ = 0u;
/* 0x0041771B mov al, byte_444EB3 */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EB3u);
/* 0x00417720 mov esi, dword_44421C */ esi_ = M32(0x44421Cu);
/* 0x00417726 mov dword_441F08[eax*4], esi */ M32(0x441F08u + (eax_ & 0xFFu) * 4u) = esi_;
/* 0x0041772D mov dword_442018[eax*4], ecx */ M32(0x442018u + (eax_ & 0xFFu) * 4u) = ecx_;
/* 0x00417734 mov byte_4441D8[eax], bl */ M8(0x4441D8u + (eax_ & 0xFFu)) = (unsigned char)(ebx_ & 0xFFu);
/* 0x0041773A mov ah, al */ eax_ = (eax_ & 0xFFFF00FFUL) | ((eax_ & 0xFFu) << 8);
/* 0x0041773C inc ah */ eax_ = (eax_ & 0xFFFF00FFUL) | (((((eax_ >> 8) & 0xFFu) + 1u) & 0xFFu) << 8);
/* 0x0041773E mov dl, [esp+1Ch+var_18] */ edx_ = (edx_ & 0xFFFFFF00UL) | (stk_b & 0xFFu);
/* 0x00417742 mov byte_444EB3, ah */ M8(0x444EB3u) = (unsigned char)((eax_ >> 8) & 0xFFu);
/* 0x00417748 test dl, dl */
/* 0x0041774A jz short loc_417790 */ if ((edx_ & 0xFFu) == 0u) goto l_417790;
/* 0x0041774C mov edx, dword_4441F4 */ edx_ = M32(0x4441F4u);
/* 0x00417752 mov ebp, dword_4441E4 */ ebp_ = M32(0x4441E4u);
/* 0x00417758 mov eax, dword_4441FC */ eax_ = M32(0x4441FCu);
/* 0x0041775D add ebp, edx */ ebp_ = (ebp_ + edx_) & U32MASK;
/* 0x0041775F mov dword_442E58[eax*4], edx */ M32(0x442E58u + eax_ * 4u) = edx_;
/* 0x00417766 xor edx, edx */ edx_ = 0u;
/* 0x00417768 mov dl, bh */ edx_ = (edx_ & 0xFFFFFF00UL) | ((ebx_ >> 8) & 0xFFu);
/* 0x0041776A mov ecx, dword_443258[edx*4] */ ecx_ = M32(0x443258u + (edx_ & 0xFFu) * 4u);
/* 0x00417771 mov eax, ecx */ eax_ = ecx_;
/* 0x00417773 shl eax, 5 */ eax_ = (eax_ << 5) & U32MASK;
/* 0x00417776 sub eax, ecx */ eax_ = (eax_ - ecx_) & U32MASK;
/* 0x00417778 mov ecx, dword_4441F4 */ ecx_ = M32(0x4441F4u);
/* 0x0041777E shr eax, 5 */ eax_ >>= 5;
/* 0x00417781 add eax, ecx */ eax_ = (eax_ + ecx_) & U32MASK;
/* 0x00417783 mov dword_4441E4, ebp */ M32(0x4441E4u) = ebp_;
/* 0x00417789 mov dword_443258[edx*4], eax */ M32(0x443258u + (edx_ & 0xFFu) * 4u) = eax_;
l_417790:
/* 0x00417790 xor ecx, ecx */ ecx_ = 0u;
/* 0x00417792 mov cl, bh */ ecx_ = (ecx_ & 0xFFFFFF00UL) | ((ebx_ >> 8) & 0xFFu);
/* 0x00417794 mov edx, dword_443B60[ecx*4] */ edx_ = M32(0x443B60u + (ecx_ & 0xFFu) * 4u);
/* 0x0041779B mov eax, edx */ eax_ = edx_;
/* 0x0041779D shl eax, 5 */ eax_ = (eax_ << 5) & U32MASK;
/* 0x004177A0 sub eax, edx */ eax_ = (eax_ - edx_) & U32MASK;
/* 0x004177A2 mov esi, dword_4441F4 */ esi_ = M32(0x4441F4u);
/* 0x004177A8 shr eax, 5 */ eax_ >>= 5;
/* 0x004177AB add eax, esi */ eax_ = (eax_ + esi_) & U32MASK;
/* 0x004177AD mov dword_443B60[ecx*4], eax */ M32(0x443B60u + (ecx_ & 0xFFu) * 4u) = eax_;
/* 0x004177B4 mov al, bl */ eax_ = (eax_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x004177B6 add esp, 8 */
/* 0x004177B9 pop ebp */
/* 0x004177BA pop edi */
/* 0x004177BB pop esi */
/* 0x004177BC pop ecx */
/* 0x004177BD pop ebx */
/* 0x004177BE retn */ return (eax_ & 0xFFu);
}
/* 0x414C26 ignore eax (next instr overwrites or calls). Void. */
void d3u_4141d4(dec3w_ctx *c)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int ecx_ = 0;
unsigned int edx_ = 0;
unsigned int esi_ = 0;
/* 0x004141D4 push ebx */
/* 0x004141D5 push ecx */
/* 0x004141D6 push edx */
/* 0x004141D7 push esi */
/* 0x004141D8 mov esi, dword_4299BC */ esi_ = M32(0x4299BCu);
/* 0x004141DE xor ecx, ecx */ ecx_ = 0u;
l_4141e0:
/* 0x004141E0 lea ebx, [esi+ecx] */ ebx_ = (esi_ + ecx_) & U32MASK;
/* 0x004141E3 cmp byte ptr [ebx], 0 */
/* 0x004141E6 jz short loc_414214 */ if (G8(ebx_) == 0u) goto l_414214;
/* 0x004141E8 mov al, 0Dh */ eax_ = (eax_ & 0xFFFFFF00UL) | 0x0Du;
/* 0x004141EA xor ah, ah */ eax_ = (eax_ & 0xFFFF00FFUL);
l_4141ec:
/* 0x004141EC dec al */ eax_ = (eax_ & 0xFFFFFF00UL) | (((eax_ & 0xFFu) - 1u) & 0xFFu);
/* 0x004141EE xor edx, edx */ edx_ = 0u;
/* 0x004141F0 mov dl, al */ edx_ = (edx_ & 0xFFFFFF00UL) | (eax_ & 0xFFu);
/* 0x004141F2 shr byte ptr [edx+ebx+1], 2 */ M8(edx_ + ebx_ + 1u) = (unsigned char)(M8(edx_ + ebx_ + 1u) >> 2);
/* 0x004141F7 add ah, [edx+ebx+1] */ eax_ = (eax_ & 0xFFFF00FFUL) | (((((eax_ >> 8) & 0xFFu) + (unsigned int)G8(edx_ + ebx_ + 1u)) & 0xFFu) << 8);
/* 0x004141FB test al, al */
/* 0x004141FD jnz short loc_4141EC */ if ((eax_ & 0xFFu) != 0u) goto l_4141ec;
/* 0x004141FF test ah, ah */
/* 0x00414201 jz short loc_41420D */ if (((eax_ >> 8) & 0xFFu) == 0u) goto l_41420d;
/* 0x00414203 xor edx, edx */ edx_ = 0u;
/* 0x00414205 mov dl, [ebx] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(ebx_);
/* 0x00414207 sar edx, 2 */ edx_ = (unsigned int)(((int)edx_) >> 2) & U32MASK;
/* 0x0041420A inc edx */ edx_ = (edx_ + 1u) & U32MASK;
/* 0x0041420B jmp short loc_41420F */ goto l_41420f;
l_41420d:
/* 0x0041420D xor edx, edx */ edx_ = 0u;
l_41420f:
/* 0x0041420F mov [ebx], dl */ M8(ebx_) = (unsigned char)(edx_ & 0xFFu);
/* 0x00414211 shr byte ptr [ebx+1Bh], 1 */ M8(ebx_ + 0x1Bu) = (unsigned char)(M8(ebx_ + 0x1Bu) >> 1);
l_414214:
/* 0x00414214 add ecx, 20h */ ecx_ = (ecx_ + 0x20u) & U32MASK;
/* 0x00414217 cmp ecx, 800000h */
/* 0x0041421D jnz short loc_4141E0 */ if (ecx_ != 0x800000u) goto l_4141e0;
/* 0x0041421F mov dword_4299BC, esi */ M32(0x4299BCu) = esi_;
/* 0x00414225 pop esi */
/* 0x00414226 pop edx */
/* 0x00414227 pop ecx */
/* 0x00414228 pop ebx */
/* 0x00414229 retn */ return;
}
/* return; callers overwrite eax next. Void. */
void d3u_41422c(dec3w_ctx *c)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int ecx_ = 0;
unsigned int edx_ = 0;
unsigned int esi_ = 0;
/* 0x0041422C push ebx */
/* 0x0041422D push ecx */
/* 0x0041422E push edx */
/* 0x0041422F push esi */
/* 0x00414230 mov esi, dword_4299C0 */ esi_ = M32(0x4299C0u);
/* 0x00414236 xor ecx, ecx */ ecx_ = 0u;
l_414238:
/* 0x00414238 lea ebx, [esi+ecx] */ ebx_ = (esi_ + ecx_) & U32MASK;
/* 0x0041423B cmp byte ptr [ebx], 0 */
/* 0x0041423E jz short loc_414278 */ if (G8(ebx_) == 0u) goto l_414278;
/* 0x00414240 mov al, 8 */ eax_ = (eax_ & 0xFFFFFF00UL) | 8u;
/* 0x00414242 xor ah, ah */ eax_ = (eax_ & 0xFFFF00FFUL);
l_414244:
/* 0x00414244 dec al */ eax_ = (eax_ & 0xFFFFFF00UL) | (((eax_ & 0xFFu) - 1u) & 0xFFu);
/* 0x00414246 xor edx, edx */ edx_ = 0u;
/* 0x00414248 mov dl, al */ edx_ = (edx_ & 0xFFFFFF00UL) | (eax_ & 0xFFu);
/* 0x0041424A shr byte ptr [edx+ebx+1], 2 */ M8(edx_ + ebx_ + 1u) = (unsigned char)(M8(edx_ + ebx_ + 1u) >> 2);
/* 0x0041424F add ah, [edx+ebx+1] */ eax_ = (eax_ & 0xFFFF00FFUL) | (((((eax_ >> 8) & 0xFFu) + (unsigned int)G8(edx_ + ebx_ + 1u)) & 0xFFu) << 8);
/* 0x00414253 test al, al */
/* 0x00414255 jnz short loc_414244 */ if ((eax_ & 0xFFu) != 0u) goto l_414244;
/* 0x00414257 test ah, ah */
/* 0x00414259 jz short loc_414265 */ if (((eax_ >> 8) & 0xFFu) == 0u) goto l_414265;
/* 0x0041425B xor edx, edx */ edx_ = 0u;
/* 0x0041425D mov dl, [ebx] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(ebx_);
/* 0x0041425F sar edx, 2 */ edx_ = (unsigned int)(((int)edx_) >> 2) & U32MASK;
/* 0x00414262 inc edx */ edx_ = (edx_ + 1u) & U32MASK;
/* 0x00414263 jmp short loc_414267 */ goto l_414267;
l_414265:
/* 0x00414265 xor edx, edx */ edx_ = 0u;
l_414267:
/* 0x00414267 mov [ebx], dl */ M8(ebx_) = (unsigned char)(edx_ & 0xFFu);
/* 0x00414269 mov dl, [ebx+11h] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(ebx_ + 0x11u);
/* 0x0041426C mov al, [ebx] */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)G8(ebx_);
/* 0x0041426E shr dl, 1 */ edx_ = (edx_ & 0xFFFFFF00UL) | (((edx_ & 0xFFu) >> 1) & 0xFFu);
/* 0x00414270 add ah, al */ eax_ = (eax_ & 0xFFFF00FFUL) | (((((eax_ >> 8) & 0xFFu) + (eax_ & 0xFFu)) & 0xFFu) << 8);
/* 0x00414272 mov [ebx+11h], dl */ M8(ebx_ + 0x11u) = (unsigned char)(edx_ & 0xFFu);
/* 0x00414275 mov [ebx+12h], ah */ M8(ebx_ + 0x12u) = (unsigned char)((eax_ >> 8) & 0xFFu);
l_414278:
/* 0x00414278 add ecx, 14h */ ecx_ = (ecx_ + 0x14u) & U32MASK;
/* 0x0041427B cmp ecx, 140000h */
/* 0x00414281 jnz short loc_414238 */ if (ecx_ != 0x140000u) goto l_414238;
/* 0x00414283 mov dword_4299C0, esi */ M32(0x4299C0u) = esi_;
/* 0x00414289 pop esi */
/* 0x0041428A pop edx */
/* 0x0041428B pop ecx */
/* 0x0041428C pop ebx */
/* 0x0041428D retn */ return;
}
/* written before use). Tail sets no return. Void. */
void d3u_41b2d4(dec3w_ctx *c)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int edx_ = 0;
/* 0x0041B2D4 push ebx */
/* 0x0041B2D5 push edx */
/* 0x0041B2D6 mov ebx, 1E40h */ ebx_ = 0x1E40u;
/* 0x0041B2DB mov eax, offset dword_43D8B0 */ eax_ = 0x43D8B0u;
/* 0x0041B2E0 xor edx, edx */ edx_ = 0u;
/* 0x0041B2E2 call sub_40C9C0 */ d3u_fill(c, ebx_, eax_, edx_);
/* 0x0041B2E7 mov edx, 2 */ edx_ = 2u;
/* 0x0041B2EC mov ebx, 0FFFFFFFFh */ ebx_ = 0xFFFFFFFFUL;
/* 0x0041B2F1 xor eax, eax */ eax_ = 0u;
/* 0x0041B2F3 mov dword_43D8B8, edx */ M32(0x43D8B8u) = edx_;
l_41b2f9:
/* 0x0041B2F9 add eax, 7Ch */ eax_ = (eax_ + 0x7Cu) & U32MASK;
/* 0x0041B2FC mov dword_43F4F0[eax], ebx */ M32(0x43F4F0u + eax_) = ebx_;
/* 0x0041B302 mov dword_43F4F4[eax], ebx */ M32(0x43F4F4u + eax_) = ebx_;
/* 0x0041B308 mov dword_43F4F8[eax], ebx */ M32(0x43F4F8u + eax_) = ebx_;
/* 0x0041B30E cmp eax, 1F0h */
/* 0x0041B313 jnz short loc_41B2F9 */ if (eax_ != 0x1F0u) goto l_41b2f9;
/* 0x0041B315 pop edx */
/* 0x0041B316 pop ebx */
/* 0x0041B317 retn */ return;
}
/* 0x410624 caller overwrites eax with mov eax,2 next). Void. */
void d3u_423228(dec3w_ctx *c, unsigned int p)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int ecx_ = 0;
unsigned int edx_ = 0;
unsigned int esi_ = 0;
unsigned int edi_ = 0;
unsigned int qv = 0;
/* 0x00423228 push ebx */
/* 0x00423229 push ecx */
/* 0x0042322A push edx */
/* 0x0042322B push esi */
/* 0x0042322C push edi */
/* 0x0042322D mov ebx, dword_4450F0 */ ebx_ = M32(0x4450F0u);
/* 0x00423233 mov edx, dword_4450F8 */ edx_ = M32(0x4450F8u);
/* 0x00423239 mov ecx, eax */ ecx_ = p;
/* 0x0042323B cmp edx, 800000h */
/* 0x00423241 ja loc_423318 */ if (edx_ > 0x800000u) goto l_423318;
/* 0x00423247 mov esi, 0FFh */ esi_ = 0xFFu;
l_42324c:
/* 0x0042324C cmp dword_4450F4, 7F800000h */
/* 0x00423256 jnb short loc_423299 */ if (M32(0x4450F4u) >= 0x7F800000UL) goto l_423299;
/* 0x00423258 xor eax, eax */ eax_ = 0u;
/* 0x0042325A mov al, byte_4450FC */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)M8(0x4450FCu);
/* 0x0042325F mov dword_4450F0, ebx */ M32(0x4450F0u) = ebx_;
/* 0x00423265 call sub_41E3BC */ d3u_41e3bc(c, eax_);
/* 0x0042326A mov ebx, dword_4450F0 */ ebx_ = M32(0x4450F0u);
/* 0x00423270 test ebx, ebx */
/* 0x00423272 jz short loc_42328A */ if (ebx_ == 0u) goto l_42328a;
l_423274:
/* 0x00423274 mov eax, esi */ eax_ = esi_;
/* 0x00423276 mov dword_4450F0, ebx */ M32(0x4450F0u) = ebx_;
/* 0x0042327C call sub_41E3BC */ d3u_41e3bc(c, eax_);
/* 0x00423281 mov ebx, dword_4450F0 */ ebx_ = M32(0x4450F0u);
/* 0x00423287 dec ebx */ ebx_ = (ebx_ - 1u) & U32MASK;
/* 0x00423288 jnz short loc_423274 */ if (ebx_ != 0u) goto l_423274;
l_42328a:
/* 0x0042328A mov eax, dword_4450F4 */ eax_ = M32(0x4450F4u);
/* 0x0042328F shr eax, 17h */ eax_ >>= 23;
/* 0x00423292 mov byte_4450FC, al */ M8(0x4450FCu) = (unsigned char)(eax_ & 0xFFu);
/* 0x00423297 jmp short loc_4232EB */ goto l_4232eb;
l_423299:
/* 0x00423299 test byte ptr dword_4450F4+3, 80h */
/* 0x004232A0 jz short loc_4232EA */ if ((M8(0x4450F4u + 3u) & 0x80u) == 0u) goto l_4232ea;
/* 0x004232A2 mov al, byte_4450FC */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)M8(0x4450FCu);
/* 0x004232A7 inc al */ eax_ = (eax_ & 0xFFFFFF00UL) | (((eax_ & 0xFFu) + 1u) & 0xFFu);
/* 0x004232A9 and eax, 0FFh */ eax_ &= 0xFFu;
/* 0x004232AE mov dword_4450F0, ebx */ M32(0x4450F0u) = ebx_;
/* 0x004232B4 call sub_41E3BC */ d3u_41e3bc(c, eax_);
/* 0x004232B9 mov ebx, dword_4450F0 */ ebx_ = M32(0x4450F0u);
/* 0x004232BF test ebx, ebx */
/* 0x004232C1 jz short loc_4232DB */ if (ebx_ == 0u) goto l_4232db;
/* 0x004232C3 xor edx, edx */ edx_ = 0u;
l_4232c5:
/* 0x004232C5 mov eax, edx */ eax_ = edx_;
/* 0x004232C7 mov dword_4450F0, ebx */ M32(0x4450F0u) = ebx_;
/* 0x004232CD call sub_41E3BC */ d3u_41e3bc(c, eax_);
/* 0x004232D2 mov ebx, dword_4450F0 */ ebx_ = M32(0x4450F0u);
/* 0x004232D8 dec ebx */ ebx_ = (ebx_ - 1u) & U32MASK;
/* 0x004232D9 jnz short loc_4232C5 */ if (ebx_ != 0u) goto l_4232c5;
l_4232db:
/* 0x004232DB mov eax, dword_4450F4 */ eax_ = M32(0x4450F4u);
/* 0x004232E0 shr eax, 17h */ eax_ >>= 23;
/* 0x004232E3 mov byte_4450FC, al */ M8(0x4450FCu) = (unsigned char)(eax_ & 0xFFu);
/* 0x004232E8 jmp short loc_4232EB */ goto l_4232eb;
l_4232ea:
/* 0x004232EA inc ebx */ ebx_ = (ebx_ + 1u) & U32MASK;
l_4232eb:
/* 0x004232EB mov eax, dword_4450F4 */ eax_ = M32(0x4450F4u);
/* 0x004232F0 mov edi, dword_4450F8 */ edi_ = M32(0x4450F8u);
/* 0x004232F6 shl eax, 8 */ eax_ = (eax_ << 8) & U32MASK;
/* 0x004232F9 shl edi, 8 */ edi_ = (edi_ << 8) & U32MASK;
/* 0x004232FC and eax, 7FFFFFFFh */ eax_ &= 0x7FFFFFFFUL;
/* 0x00423301 mov dword_4450F8, edi */ M32(0x4450F8u) = edi_;
/* 0x00423307 mov dword_4450F4, eax */ M32(0x4450F4u) = eax_;
/* 0x0042330C cmp edi, 800000h */
/* 0x00423312 jbe loc_42324C */ if (edi_ <= 0x800000u) goto l_42324c;
l_423318:
/* 0x00423318 mov eax, dword_4450F8 */ eax_ = M32(0x4450F8u);
/* 0x0042331D mov esi, [ecx] */ esi_ = G32(ecx_);
/* 0x0042331F xor edx, edx */ edx_ = 0u;
/* 0x00423321 div esi */ qv = eax_; eax_ = qv / esi_; edx_ = qv % esi_;
/* 0x00423323 mov esi, [ecx+4] */ esi_ = G32(ecx_ + 4u);
/* 0x00423326 imul esi, eax */ esi_ = (esi_ * eax_) & U32MASK;
/* 0x00423329 mov edi, [ecx] */ edi_ = G32(ecx_);
/* 0x0042332B mov edx, eax */ edx_ = eax_;
/* 0x0042332D mov eax, [ecx+8] */ eax_ = G32(ecx_ + 8u);
/* 0x00423330 cmp eax, edi */
/* 0x00423332 jnb short loc_423342 */ if (eax_ >= edi_) goto l_423342;
/* 0x00423334 sub eax, [ecx+4] */ eax_ = (eax_ - G32(ecx_ + 4u)) & U32MASK;
/* 0x00423337 imul edx, eax */ edx_ = (edx_ * eax_) & U32MASK;
/* 0x0042333A mov dword_4450F8, edx */ M32(0x4450F8u) = edx_;
/* 0x00423340 jmp short loc_423348 */ goto l_423348;
l_423342:
/* 0x00423342 sub dword_4450F8, esi */ M32(0x4450F8u) = (M32(0x4450F8u) - esi_) & U32MASK;
l_423348:
/* 0x00423348 add dword_4450F4, esi */ M32(0x4450F4u) = (M32(0x4450F4u) + esi_) & U32MASK;
/* 0x0042334E mov dword_4450F0, ebx */ M32(0x4450F0u) = ebx_;
/* 0x00423354 pop edi */
/* 0x00423355 pop esi */
/* 0x00423356 pop edx */
/* 0x00423357 pop ecx */
/* 0x00423358 pop ebx */
/* 0x00423359 retn */ return;
}
/* agrees: (eax,edx,cl,ebx)). Exit eax = accumulated var_2C. */
unsigned int d3u_41a33c(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ecx_in, unsigned int ebx_in)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int ecx_ = 0;
unsigned int edx_ = 0;
unsigned int esi_ = 0;
unsigned int edi_ = 0;
unsigned int ebp_ = 0;
unsigned int v60 = 0;
unsigned int v5c = 0;
unsigned int v58 = 0;
unsigned int v54 = 0;
unsigned int v50 = 0;
unsigned int v4c = 0;
unsigned int v48 = 0;
unsigned int v44 = 0;
unsigned int v40 = 0;
unsigned int v3c = 0;
unsigned int v38 = 0;
unsigned int v34 = 0;
unsigned int v30 = 0;
unsigned int v28 = 0;
unsigned int v24 = 0;
unsigned int v20 = 0;
unsigned int v2c = 0;
unsigned int v1c = 0;
unsigned int v18 = 0;
unsigned int v14 = 0;
unsigned int v10 = 0;
unsigned int qv = 0;
int sv = 0;
/* 0x0041A33C push esi */
/* 0x0041A33D push edi */
/* 0x0041A33E push ebp */
/* 0x0041A33F sub esp, 54h */
/* 0x0041A342 mov ebp, dword_4441DC */ ebp_ = M32(0x4441DCu);
/* 0x0041A348 mov [esp+60h+var_54], eax */ v54 = eax_in;
/* 0x0041A34C mov [esp+60h+var_5C], edx */ v5c = edx_in;
/* 0x0041A350 mov [esp+60h+var_44], ebx */ v44 = ebx_in;
/* 0x0041A354 mov [esp+60h+var_1C], cl */ v1c = ecx_in & 0xFFu;
/* 0x0041A358 xor eax, eax */ eax_ = 0u;
/* 0x0041A35A mov al, cl */ eax_ = (eax_ & 0xFFFFFF00UL) | (ecx_in & 0xFFu);
/* 0x0041A35C mov ebx, dword_429DE0[eax*4] */ ebx_ = M32(0x429DE0u + (eax_ & 0xFFu) * 4u);
/* 0x0041A363 mov edx, dword_429E08[eax*4] */ edx_ = M32(0x429E08u + (eax_ & 0xFFu) * 4u);
/* 0x0041A36A mov [esp+60h+var_34], ebx */ v34 = ebx_;
/* 0x0041A36E mov [esp+60h+var_3C], edx */ v3c = edx_;
/* 0x0041A372 shl eax, 8 */ eax_ = (eax_ << 8) & U32MASK;
/* 0x0041A375 xor edx, edx */ edx_ = 0u;
/* 0x0041A377 add eax, 30000h */ eax_ = (eax_ + 0x30000u) & U32MASK;
/* 0x0041A37C mov [esp+60h+var_28], edx */ v28 = edx_;
/* 0x0041A380 mov [esp+60h+var_2C], edx */ v2c = edx_;
/* 0x0041A384 mov [esp+60h+var_58], eax */ v58 = eax_;
/* 0x0041A388 mov edx, dword_444274 */ edx_ = M32(0x444274u);
/* 0x0041A38E mov ah, byte_444EA9 */ eax_ = (eax_ & 0xFFFF00FFUL) | (((unsigned int)M8(0x444EA9u) << 8) & 0xFF00u);
/* 0x0041A394 mov [esp+60h+var_24], edx */ v24 = edx_;
/* 0x0041A398 cmp cl, ah */
/* 0x0041A39A jnz short loc_41A3A5 */ if ((ecx_in & 0xFFu) != ((eax_ >> 8) & 0xFFu)) goto l_41a3a5;
/* 0x0041A39C mov eax, dword_4441C4 */ eax_ = M32(0x4441C4u);
/* 0x0041A3A1 sub eax, ebx */ eax_ = (eax_ - ebx_) & U32MASK;
/* 0x0041A3A3 jmp short loc_41A3A7 */ goto l_41a3a7;
l_41a3a5:
/* 0x0041A3A5 xor eax, eax */ eax_ = 0u;
l_41a3a7:
/* 0x0041A3A7 mov [esp+60h+var_40], eax */ v40 = eax_;
/* 0x0041A3AB mov eax, dword_4441C0 */ eax_ = M32(0x4441C0u);
/* 0x0041A3B0 mov [esp+60h+var_48], eax */ v48 = eax_;
/* 0x0041A3B4 mov al, byte_444EA8 */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EA8u);
/* 0x0041A3B9 mov [esp+60h+var_14], al */ v14 = eax_ & 0xFFu;
/* 0x0041A3BD xor eax, eax */ eax_ = 0u;
/* 0x0041A3BF mov al, [esp+60h+var_1C] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v1c & 0xFFu);
/* 0x0041A3C3 mov dl, [esp+60h+var_1C] */ edx_ = (edx_ & 0xFFFFFF00UL) | (v1c & 0xFFu);
/* 0x0041A3C7 mov al, byte_429E30[eax] */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)G8(0x429E30u + (eax_ & 0xFFu));
/* 0x0041A3CD mov dword_4441DC, ebp */ M32(0x4441DCu) = ebp_;
/* 0x0041A3D3 mov [esp+60h+var_18], al */ v18 = eax_ & 0xFFu;
/* 0x0041A3D7 test dl, dl */
/* 0x0041A3D9 jnz loc_41A4B2 */ if ((edx_ & 0xFFu) != 0u) goto l_41a4b2;
/* 0x0041A3DF mov ecx, 400h */ ecx_ = 0x400u;
/* 0x0041A3E4 mov edi, offset dword_443760 */ edi_ = 0x443760u;
/* 0x0041A3E9 mov esi, offset dword_443360 */ esi_ = 0x443360u;
/* 0x0041A3EE cld */
/* 0x0041A3EF mov eax, ecx */ eax_ = ecx_;
/* 0x0041A3F1 sub ecx, edi */ ecx_ = (ecx_ - edi_) & U32MASK;
/* 0x0041A3F3 sub ecx, eax */ ecx_ = (ecx_ - eax_) & U32MASK;
/* 0x0041A3F5 and ecx, 3 */ ecx_ &= 3u;
/* 0x0041A3F8 sub eax, ecx */ eax_ = (eax_ - ecx_) & U32MASK;
/* 0x0041A3FA jle short loc_41A408 */ if ((int)eax_ <= 0) goto l_41a408;
/* 0x0041A3FC rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041A3FE mov ecx, eax */ ecx_ = eax_;
/* 0x0041A400 and eax, 3 */ eax_ &= 3u;
/* 0x0041A403 shr ecx, 2 */ ecx_ >>= 2;
/* 0x0041A406 rep movsd */ while (ecx_ != 0u) { M32(edi_) = G32(esi_); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
l_41a408:
/* 0x0041A408 add ecx, eax */ ecx_ = (ecx_ + eax_) & U32MASK;
/* 0x0041A40A rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041A40C mov ecx, 400h */ ecx_ = 0x400u;
/* 0x0041A411 mov edi, offset dword_442958 */ edi_ = 0x442958u;
/* 0x0041A416 mov esi, offset dword_442128 */ esi_ = 0x442128u;
/* 0x0041A41B cld */
/* 0x0041A41C mov eax, ecx */ eax_ = ecx_;
/* 0x0041A41E sub ecx, edi */ ecx_ = (ecx_ - edi_) & U32MASK;
/* 0x0041A420 sub ecx, eax */ ecx_ = (ecx_ - eax_) & U32MASK;
/* 0x0041A422 and ecx, 3 */ ecx_ &= 3u;
/* 0x0041A425 sub eax, ecx */ eax_ = (eax_ - ecx_) & U32MASK;
/* 0x0041A427 jle short loc_41A435 */ if ((int)eax_ <= 0) goto l_41a435;
/* 0x0041A429 rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041A42B mov ecx, eax */ ecx_ = eax_;
/* 0x0041A42D and eax, 3 */ eax_ &= 3u;
/* 0x0041A430 shr ecx, 2 */ ecx_ >>= 2;
/* 0x0041A433 rep movsd */ while (ecx_ != 0u) { M32(edi_) = G32(esi_); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
l_41a435:
/* 0x0041A435 add ecx, eax */ ecx_ = (ecx_ + eax_) & U32MASK;
/* 0x0041A437 rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041A439 mov ecx, 100h */ ecx_ = 0x100u;
/* 0x0041A43E mov edi, offset byte_441F18 */ edi_ = 0x441F18u;
/* 0x0041A443 mov esi, offset byte_442D58 */ esi_ = 0x442D58u;
/* 0x0041A448 cld */
/* 0x0041A449 mov eax, ecx */ eax_ = ecx_;
/* 0x0041A44B sub ecx, edi */ ecx_ = (ecx_ - edi_) & U32MASK;
/* 0x0041A44D sub ecx, eax */ ecx_ = (ecx_ - eax_) & U32MASK;
/* 0x0041A44F and ecx, 3 */ ecx_ &= 3u;
/* 0x0041A452 sub eax, ecx */ eax_ = (eax_ - ecx_) & U32MASK;
/* 0x0041A454 jle short loc_41A462 */ if ((int)eax_ <= 0) goto l_41a462;
/* 0x0041A456 rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041A458 mov ecx, eax */ ecx_ = eax_;
/* 0x0041A45A and eax, 3 */ eax_ &= 3u;
/* 0x0041A45D shr ecx, 2 */ ecx_ >>= 2;
/* 0x0041A460 rep movsd */ while (ecx_ != 0u) { M32(edi_) = G32(esi_); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
l_41a462:
/* 0x0041A462 add ecx, eax */ ecx_ = (ecx_ + eax_) & U32MASK;
/* 0x0041A464 rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041A466 mov ecx, 100h */ ecx_ = 0x100u;
/* 0x0041A46B mov edi, offset byte_442028 */ edi_ = 0x442028u;
/* 0x0041A470 mov esi, offset byte_443260 */ esi_ = 0x443260u;
/* 0x0041A475 cld */
/* 0x0041A476 mov eax, ecx */ eax_ = ecx_;
/* 0x0041A478 sub ecx, edi */ ecx_ = (ecx_ - edi_) & U32MASK;
/* 0x0041A47A sub ecx, eax */ ecx_ = (ecx_ - eax_) & U32MASK;
/* 0x0041A47C and ecx, 3 */ ecx_ &= 3u;
/* 0x0041A47F sub eax, ecx */ eax_ = (eax_ - ecx_) & U32MASK;
/* 0x0041A481 jle short loc_41A48F */ if ((int)eax_ <= 0) goto l_41a48f;
/* 0x0041A483 rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041A485 mov ecx, eax */ ecx_ = eax_;
/* 0x0041A487 and eax, 3 */ eax_ &= 3u;
/* 0x0041A48A shr ecx, 2 */ ecx_ >>= 2;
/* 0x0041A48D rep movsd */ while (ecx_ != 0u) { M32(edi_) = G32(esi_); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
l_41a48f:
/* 0x0041A48F add ecx, eax */ ecx_ = (ecx_ + eax_) & U32MASK;
/* 0x0041A491 rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041A493 mov esi, 201h */ esi_ = 0x201u;
/* 0x0041A498 mov ebp, dword_444210 */ ebp_ = M32(0x444210u);
/* 0x0041A49E mov al, byte_444EBA */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EBAu);
/* 0x0041A4A3 mov dword_4441DC, ebp */ M32(0x4441DCu) = ebp_;
/* 0x0041A4A9 mov byte_444EB4, al */ M8(0x444EB4u) = (unsigned char)(eax_ & 0xFFu);
/* 0x0041A4AE mov [esp+60h+var_34], esi */ v34 = esi_;
l_41a4b2:
/* 0x0041A4B2 mov edx, [esp+60h+var_3C] */ edx_ = v3c;
/* 0x0041A4B6 mov ebx, [esp+60h+var_34] */ ebx_ = v34;
/* 0x0041A4BA mov edi, dword_4299F8 */ edi_ = M32(0x4299F8u);
/* 0x0041A4C0 mov esi, dword_4299F4 */ esi_ = M32(0x4299F4u);
/* 0x0041A4C6 shl edx, 9 */ edx_ = (edx_ << 9) & U32MASK;
/* 0x0041A4C9 shl ebx, 9 */ ebx_ = (ebx_ << 9) & U32MASK;
/* 0x0041A4CC mov ecx, edx */ ecx_ = edx_;
/* 0x0041A4CE add esi, ebx */ esi_ = (esi_ + ebx_) & U32MASK;
/* 0x0041A4D0 cld */
/* 0x0041A4D1 mov eax, ecx */ eax_ = ecx_;
/* 0x0041A4D3 sub ecx, edi */ ecx_ = (ecx_ - edi_) & U32MASK;
/* 0x0041A4D5 sub ecx, eax */ ecx_ = (ecx_ - eax_) & U32MASK;
/* 0x0041A4D7 and ecx, 3 */ ecx_ &= 3u;
/* 0x0041A4DA sub eax, ecx */ eax_ = (eax_ - ecx_) & U32MASK;
/* 0x0041A4DC jle short loc_41A4EA */ if ((int)eax_ <= 0) goto l_41a4ea;
/* 0x0041A4DE rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041A4E0 mov ecx, eax */ ecx_ = eax_;
/* 0x0041A4E2 and eax, 3 */ eax_ &= 3u;
/* 0x0041A4E5 shr ecx, 2 */ ecx_ >>= 2;
/* 0x0041A4E8 rep movsd */ while (ecx_ != 0u) { M32(edi_) = G32(esi_); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
l_41a4ea:
/* 0x0041A4EA add ecx, eax */ ecx_ = (ecx_ + eax_) & U32MASK;
/* 0x0041A4EC rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041A4EE lea ecx, [edx+ebx] */ ecx_ = (edx_ + ebx_) & U32MASK;
/* 0x0041A4F1 mov edx, [esp+60h+var_3C] */ edx_ = v3c;
/* 0x0041A4F5 mov ebx, [esp+60h+var_34] */ ebx_ = v34;
/* 0x0041A4F9 add edx, edx */ edx_ = (edx_ + edx_) & U32MASK;
/* 0x0041A4FB add ebx, ebx */ ebx_ = (ebx_ + ebx_) & U32MASK;
/* 0x0041A4FD mov eax, edx */ eax_ = edx_;
/* 0x0041A4FF add edx, ebx */ edx_ = (edx_ + ebx_) & U32MASK;
l_41a501:
/* 0x0041A501 mov edi, [esp+60h+var_3C] */ edi_ = v3c;
/* 0x0041A505 mov ebp, dword_4441DC */ ebp_ = M32(0x4441DCu);
/* 0x0041A50B sub ecx, 200h */ ecx_ = (ecx_ - 0x200u) & U32MASK;
/* 0x0041A511 sub eax, 2 */ eax_ = (eax_ - 2u) & U32MASK;
/* 0x0041A514 dec edi */ edi_ = (edi_ - 1u) & U32MASK;
/* 0x0041A515 sub edx, 2 */ edx_ = (edx_ - 2u) & U32MASK;
/* 0x0041A518 mov [esp+60h+var_3C], edi */ v3c = edi_;
/* 0x0041A51C cmp eax, 0FFFFFFFEh */
/* 0x0041A51F jz short loc_41A55A */ if (eax_ == 0xFFFFFFFEUL) goto l_41a55a;
/* 0x0041A521 mov ebx, dword_4299FC */ ebx_ = M32(0x4299FCu);
/* 0x0041A527 mov esi, dword_429A04 */ esi_ = M32(0x429A04u);
/* 0x0041A52D mov bx, [edx+ebx] */ ebx_ = (ebx_ & 0xFFFF0000UL) | (unsigned int)G16(edx_ + ebx_);
/* 0x0041A531 xor edi, edi */ edi_ = 0u;
/* 0x0041A533 mov [esi+eax], bx */ M16(esi_ + eax_) = (unsigned short)(ebx_ & 0xFFFFu);
/* 0x0041A537 mov esi, dword_4299F0 */ esi_ = M32(0x4299F0u);
/* 0x0041A53D mov di, bx */ edi_ = (edi_ & 0xFFFF0000UL) | (ebx_ & 0xFFFFu);
/* 0x0041A540 mov si, [ecx+esi] */ esi_ = (esi_ & 0xFFFF0000UL) | (unsigned int)G16(ecx_ + esi_);
/* 0x0041A544 and esi, 0FFFFh */ esi_ &= 0xFFFFu;
/* 0x0041A54A add esi, edi */ esi_ = (esi_ + edi_) & U32MASK;
/* 0x0041A54C mov ebx, esi */ ebx_ = esi_;
/* 0x0041A54E mov esi, dword_429A00 */ esi_ = M32(0x429A00u);
/* 0x0041A554 mov [esi+eax], bx */ M16(esi_ + eax_) = (unsigned short)(ebx_ & 0xFFFFu);
/* 0x0041A558 jmp short loc_41A501 */ goto l_41a501;
l_41a55a:
/* 0x0041A55A mov eax, dword_444258 */ eax_ = M32(0x444258u);
/* 0x0041A55F mov [esp+60h+var_34], eax */ v34 = eax_;
/* 0x0041A563 mov eax, [esp+60h+var_58] */ eax_ = v58;
/* 0x0041A567 add eax, eax */ eax_ = (eax_ + eax_) & U32MASK;
/* 0x0041A569 mov edx, [esp+60h+var_54] */ edx_ = v54;
/* 0x0041A56D mov [esp+60h+var_60], eax */ v60 = eax_;
/* 0x0041A570 mov eax, [esp+60h+var_28] */ eax_ = v28;
/* 0x0041A574 add eax, edx */ eax_ = (eax_ + edx_) & U32MASK;
/* 0x0041A576 mov [esp+60h+var_30], eax */ v30 = eax_;
l_41a57a:
/* 0x0041A57A cmp [esp+60h+var_28], 100h */
/* 0x0041A582 jnb short loc_41A597 */ if (v28 >= 0x100u) goto l_41a597;
/* 0x0041A584 mov ecx, [esp+60h+var_24] */ ecx_ = v24;
/* 0x0041A588 mov eax, dword_4299B8 */ eax_ = M32(0x4299B8u);
/* 0x0041A58D add eax, ecx */ eax_ = (eax_ + ecx_) & U32MASK;
/* 0x0041A58F mov al, [eax] */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x0041A591 mov [esp+60h+var_10], al */ v10 = eax_ & 0xFFu;
/* 0x0041A595 jmp short loc_41A5AD */ goto l_41a5ad;
l_41a597:
/* 0x0041A597 mov eax, [esp+60h+var_30] */ eax_ = v30;
/* 0x0041A59B mov al, [eax-100h] */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_ - 0x100u);
/* 0x0041A5A1 mov dh, al */ edx_ = (edx_ & 0xFFFF00FFUL) | ((eax_ & 0xFFu) << 8);
/* 0x0041A5A3 mov [esp+60h+var_10], al */ v10 = eax_ & 0xFFu;
/* 0x0041A5A7 not dh */ edx_ = (edx_ & 0xFFFF00FFUL) | (((~((edx_ >> 8) & 0xFFu)) & 0xFFu) << 8);
/* 0x0041A5A9 mov [esp+60h+var_10], dh */ v10 = (edx_ >> 8) & 0xFFu;
l_41a5ad:
/* 0x0041A5AD cmp [esp+60h+var_1C], 0 */
/* 0x0041A5B2 jz short loc_41A5D6 */ if ((v1c & 0xFFu) == 0u) goto l_41a5d6;
/* 0x0041A5B4 mov cl, [esp+60h+var_18] */ ecx_ = (ecx_ & 0xFFFFFF00UL) | (v18 & 0xFFu);
/* 0x0041A5B8 mov eax, [esp+60h+var_34] */ eax_ = v34;
/* 0x0041A5BC shr eax, cl */ eax_ >>= (ecx_ & 0xFFu);
/* 0x0041A5BE cmp [esp+60h+var_14], 0 */
/* 0x0041A5C3 jz loc_41A66D */ if ((v14 & 0xFFu) == 0u) goto l_41a66d;
/* 0x0041A5C9 mov ah, [esp+60h+var_10] */ eax_ = (eax_ & 0xFFFF00FFUL) | ((v10 & 0xFFu) << 8);
/* 0x0041A5CD mov bl, al */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (eax_ & 0xFFu);
/* 0x0041A5CF sub bl, ah */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (((ebx_ & 0xFFu) - ((eax_ >> 8) & 0xFFu)) & 0xFFu);
/* 0x0041A5D1 jmp loc_41A673 */ goto l_41a673;
l_41a5d6:
/* 0x0041A5D6 xor esi, esi */ esi_ = 0u;
/* 0x0041A5D8 mov bh, byte_444EB4 */ ebx_ = (ebx_ & 0xFFFF00FFUL) | (((unsigned int)M8(0x444EB4u)) << 8);
/* 0x0041A5DE mov [esp+60h+var_40], esi */ v40 = esi_;
/* 0x0041A5E2 cmp bh, 1 */
/* 0x0041A5E5 jbe loc_41A656 */ if (((ebx_ >> 8) & 0xFFu) <= 1u) goto l_41a656;
/* 0x0041A5EB xor edx, edx */ edx_ = 0u;
/* 0x0041A5ED mov eax, [esp+60h+var_28] */ eax_ = v28;
/* 0x0041A5F1 mov dl, bh */ edx_ = (edx_ & 0xFFFFFF00UL) | ((ebx_ >> 8) & 0xFFu);
/* 0x0041A5F3 sub eax, edx */ eax_ = (eax_ - edx_) & U32MASK;
/* 0x0041A5F5 test eax, eax */
/* 0x0041A5F7 jge short loc_41A611 */ if ((int)eax_ >= 0) goto l_41a611;
/* 0x0041A5F9 mov eax, [esp+60h+var_24] */ eax_ = v24;
/* 0x0041A5FD sub eax, edx */ eax_ = (eax_ - edx_) & U32MASK;
/* 0x0041A5FF mov edx, dword_444278 */ edx_ = M32(0x444278u);
/* 0x0041A605 and edx, eax */ edx_ &= eax_;
/* 0x0041A607 mov eax, dword_4299B8 */ eax_ = M32(0x4299B8u);
/* 0x0041A60C mov al, [edx+eax] */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)G8(edx_ + eax_);
/* 0x0041A60F jmp short loc_41A619 */ goto l_41a619;
l_41a611:
/* 0x0041A611 add eax, [esp+60h+var_54] */ eax_ = (eax_ + v54) & U32MASK;
/* 0x0041A615 mov al, [eax] */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x0041A617 not al */ eax_ = (eax_ & 0xFFFFFF00UL) | ((~(eax_ & 0xFFu)) & 0xFFu);
l_41a619:
/* 0x0041A619 xor ecx, ecx */ ecx_ = 0u;
/* 0x0041A61B mov cl, byte_444EB4 */ ecx_ = (ecx_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EB4u);
/* 0x0041A621 mov edx, [esp+60h+var_28] */ edx_ = v28;
/* 0x0041A625 add ecx, ecx */ ecx_ = (ecx_ + ecx_) & U32MASK;
/* 0x0041A627 sub edx, ecx */ edx_ = (edx_ - ecx_) & U32MASK;
/* 0x0041A629 test edx, edx */
/* 0x0041A62B jge short loc_41A644 */ if ((int)edx_ >= 0) goto l_41a644;
/* 0x0041A62D mov edx, [esp+60h+var_24] */ edx_ = v24;
/* 0x0041A631 sub edx, ecx */ edx_ = (edx_ - ecx_) & U32MASK;
/* 0x0041A633 and edx, dword_444278 */ edx_ &= M32(0x444278u);
/* 0x0041A639 mov ecx, dword_4299B8 */ ecx_ = M32(0x4299B8u);
/* 0x0041A63F mov ah, [ecx+edx] */ eax_ = (eax_ & 0xFFFF00FFUL) | (((unsigned int)G8(ecx_ + edx_) << 8) & 0xFF00u);
/* 0x0041A642 jmp short loc_41A64C */ goto l_41a64c;
l_41a644:
/* 0x0041A644 add edx, [esp+60h+var_54] */ edx_ = (edx_ + v54) & U32MASK;
/* 0x0041A648 mov ah, [edx] */ eax_ = (eax_ & 0xFFFF00FFUL) | (((unsigned int)G8(edx_) << 8) & 0xFF00u);
/* 0x0041A64A not ah */ eax_ = (eax_ & 0xFFFF00FFUL) | (((~((eax_ >> 8) & 0xFFu)) & 0xFFu) << 8);
l_41a64c:
/* 0x0041A64C cmp al, ah */
/* 0x0041A64E jnz short loc_41A66D */ if ((eax_ & 0xFFu) != ((eax_ >> 8) & 0xFFu)) goto l_41a66d;
/* 0x0041A650 inc [esp+60h+var_40] */ v40 = (v40 + 1u) & U32MASK;
/* 0x0041A654 jmp short loc_41A66D */ goto l_41a66d;
l_41a656:
/* 0x0041A656 mov edx, [esp+60h+var_34] */ edx_ = v34;
/* 0x0041A65A mov al, byte ptr [esp+60h+var_34] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v34 & 0xFFu);
/* 0x0041A65E shr edx, 8 */ edx_ >>= 8;
/* 0x0041A661 cmp dl, al */
/* 0x0041A663 jnz short loc_41A66D */ if ((edx_ & 0xFFu) != (eax_ & 0xFFu)) goto l_41a66d;
/* 0x0041A665 mov [esp+60h+var_40], 1 */ v40 = 1u;
l_41a66d:
/* 0x0041A66D mov bl, [esp+60h+var_10] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (v10 & 0xFFu);
/* 0x0041A671 sub bl, al */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (((ebx_ & 0xFFu) - (eax_ & 0xFFu)) & 0xFFu);
l_41a673:
/* 0x0041A673 mov eax, [esp+60h+var_40] */ eax_ = v40;
/* 0x0041A677 xor edx, edx */ edx_ = 0u;
/* 0x0041A679 shl eax, 8 */ eax_ = (eax_ << 8) & U32MASK;
/* 0x0041A67C mov dl, bl */ edx_ = (edx_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x0041A67E mov edi, [esp+60h+var_40] */ edi_ = v40;
/* 0x0041A682 add eax, edx */ eax_ = (eax_ + edx_) & U32MASK;
/* 0x0041A684 mov ecx, dword_429A00 */ ecx_ = M32(0x429A00u);
/* 0x0041A68A lea esi, ds:0[eax*2] */ esi_ = (eax_ * 2u) & U32MASK;
/* 0x0041A691 add edi, edi */ edi_ = (edi_ + edi_) & U32MASK;
/* 0x0041A693 mov eax, dword_4299F8 */ eax_ = M32(0x4299F8u);
/* 0x0041A698 add ecx, edi */ ecx_ = (ecx_ + edi_) & U32MASK;
/* 0x0041A69A add esi, eax */ esi_ = (esi_ + eax_) & U32MASK;
/* 0x0041A69C mov [esp+60h+var_38], edx */ v38 = edx_;
/* 0x0041A6A0 mov dx, [ecx] */ edx_ = (edx_ & 0xFFFF0000UL) | (unsigned int)G16(ecx_);
/* 0x0041A6A3 mov ax, [esi] */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)G16(esi_);
/* 0x0041A6A6 mov [esp+60h+var_20], edx */ v20 = edx_;
/* 0x0041A6AA test ax, ax */
/* 0x0041A6AD jnz short loc_41A71D */ if ((eax_ & 0xFFFFu) != 0u) goto l_41a71d;
/* 0x0041A6AF mov eax, [esp+60h+var_58] */ eax_ = v58;
/* 0x0041A6B3 add eax, [esp+60h+var_38] */ eax_ = (eax_ + v38) & U32MASK;
/* 0x0041A6B7 mov edx, dword_4299F4 */ edx_ = M32(0x4299F4u);
/* 0x0041A6BD add eax, eax */ eax_ = (eax_ + eax_) & U32MASK;
/* 0x0041A6BF add eax, edx */ eax_ = (eax_ + edx_) & U32MASK;
/* 0x0041A6C1 mov ax, [eax] */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)G16(eax_);
/* 0x0041A6C4 and eax, 0FFFFh */ eax_ &= 0xFFFFu;
/* 0x0041A6C9 shl eax, 0Eh */ eax_ = (eax_ << 14) & U32MASK;
/* 0x0041A6CC mov edx, [esp+60h+var_60] */ edx_ = v60;
/* 0x0041A6CF mov [esp+60h+var_50], eax */ v50 = eax_;
/* 0x0041A6D3 mov eax, dword_4299F0 */ eax_ = M32(0x4299F0u);
/* 0x0041A6D8 add eax, edx */ eax_ = (eax_ + edx_) & U32MASK;
/* 0x0041A6DA mov ax, [eax] */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)G16(eax_);
/* 0x0041A6DD and eax, 0FFFFh */ eax_ &= 0xFFFFu;
/* 0x0041A6E2 xor edx, edx */ edx_ = 0u;
/* 0x0041A6E4 mov [esp+60h+var_4C], eax */ v4c = eax_;
/* 0x0041A6E8 mov eax, [esp+60h+var_50] */ eax_ = v50;
/* 0x0041A6EC div [esp+60h+var_4C] */ qv = eax_; eax_ = qv / v4c; edx_ = qv % v4c;
/* 0x0041A6F0 mov ax, word_42D274[eax*2] */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)G16(0x42D274u + eax_ * 2u);
/* 0x0041A6F8 mov edx, [esp+60h+var_2C] */ edx_ = v2c;
/* 0x0041A6FC and eax, 0FFFFh */ eax_ &= 0xFFFFu;
/* 0x0041A701 add edx, eax */ edx_ = (edx_ + eax_) & U32MASK;
/* 0x0041A703 mov eax, dword_429A04 */ eax_ = M32(0x429A04u);
/* 0x0041A708 add edi, eax */ edi_ = (edi_ + eax_) & U32MASK;
/* 0x0041A70A mov ax, [edi] */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)G16(edi_);
/* 0x0041A70D mov [esp+60h+var_2C], edx */ v2c = edx_;
/* 0x0041A711 mov edx, eax */ edx_ = eax_;
/* 0x0041A713 add edx, 20h */ edx_ = (edx_ + 0x20u) & U32MASK;
/* 0x0041A716 mov [edi], dx */ M16(edi_) = (unsigned short)(edx_ & 0xFFFFu);
/* 0x0041A719 add word ptr [ecx], 20h */ M16(ecx_) = (unsigned short)((G16(ecx_) + 0x20u) & 0xFFFFu);
l_41a71d:
/* 0x0041A71D and eax, 0FFFFh */ eax_ &= 0xFFFFu;
/* 0x0041A722 xor ecx, ecx */ ecx_ = 0u;
/* 0x0041A724 xor edx, edx */ edx_ = 0u;
/* 0x0041A726 mov cx, word ptr [esp+60h+var_20] */ ecx_ = (ecx_ & 0xFFFF0000UL) | (v20 & 0xFFFFu);
/* 0x0041A72B shl eax, 0Eh */ eax_ = (eax_ << 14) & U32MASK;
/* 0x0041A72E div ecx */ qv = eax_; eax_ = qv / ecx_; edx_ = qv % ecx_;
/* 0x0041A730 mov ax, word_42D274[eax*2] */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)G16(0x42D274u + eax_ * 2u);
/* 0x0041A738 mov ecx, [esp+60h+var_2C] */ ecx_ = v2c;
/* 0x0041A73C and eax, 0FFFFh */ eax_ &= 0xFFFFu;
/* 0x0041A741 add ecx, eax */ ecx_ = (ecx_ + eax_) & U32MASK;
/* 0x0041A743 mov edi, [esp+60h+var_44] */ edi_ = v44;
/* 0x0041A747 mov [esp+60h+var_2C], ecx */ v2c = ecx_;
/* 0x0041A74B cmp ecx, edi */
/* 0x0041A74D ja loc_41A9B1 */ if (ecx_ > edi_) goto l_41a9b1;
/* 0x0041A753 mov cx, [esi] */ ecx_ = (ecx_ & 0xFFFF0000UL) | (unsigned int)G16(esi_);
/* 0x0041A756 mov eax, [esp+60h+var_40] */ eax_ = v40;
/* 0x0041A75A add ecx, 20h */ ecx_ = (ecx_ + 0x20u) & U32MASK;
/* 0x0041A75D mov edx, dword_429A00 */ edx_ = M32(0x429A00u);
/* 0x0041A763 mov [esi], cx */ M16(esi_) = (unsigned short)(ecx_ & 0xFFFFu);
/* 0x0041A766 mov si, [edx+eax*2] */ esi_ = (esi_ & 0xFFFF0000UL) | (unsigned int)G16(edx_ + eax_ * 2u);
/* 0x0041A76A add esi, 20h */ esi_ = (esi_ + 0x20u) & U32MASK;
/* 0x0041A76D mov dword_4441DC, ebp */ M32(0x4441DCu) = ebp_;
/* 0x0041A773 mov [edx+eax*2], si */ M16(edx_ + eax_ * 2u) = (unsigned short)(esi_ & 0xFFFFu);
/* 0x0041A777 cmp si, 4000h */
/* 0x0041A77C jb short loc_41A783 */ if ((esi_ & 0xFFFFu) < 0x4000u) goto l_41a783;
/* 0x0041A77E call sub_41A284 */ eax_ = d3u_41a284(c, eax_);
l_41a783:
/* 0x0041A783 mov dl, [esp+60h+var_1C] */ edx_ = (edx_ & 0xFFFFFF00UL) | (v1c & 0xFFu);
/* 0x0041A787 mov ebp, dword_4441DC */ ebp_ = M32(0x4441DCu);
/* 0x0041A78D test dl, dl */
/* 0x0041A78F jz loc_41A852 */ if ((edx_ & 0xFFu) == 0u) goto l_41a852;
/* 0x0041A795 cmp bl, 80h */
/* 0x0041A798 jb short loc_41A7A7 */ if ((ebx_ & 0xFFu) < 0x80u) goto l_41a7a7;
/* 0x0041A79A mov bh, [esp+60h+var_14] */ ebx_ = (ebx_ & 0xFFFF00FFUL) | ((v14 & 0xFFu) << 8);
/* 0x0041A79E test bh, bh */
/* 0x0041A7A0 setz al */ eax_ = (eax_ & 0xFFFFFF00UL) | (((((ebx_ >> 8) & 0xFFu) == 0u) ? 1u : 0u));
/* 0x0041A7A3 mov [esp+60h+var_14], al */ v14 = eax_ & 0xFFu;
l_41a7a7:
/* 0x0041A7A7 mov cl, [esp+60h+var_1C] */ ecx_ = (ecx_ & 0xFFFFFF00UL) | (v1c & 0xFFu);
/* 0x0041A7AB cmp cl, 1 */
/* 0x0041A7AE jnz short loc_41A7DF */ if ((ecx_ & 0xFFu) != 1u) goto l_41a7df;
/* 0x0041A7B0 mov eax, [esp+60h+var_40] */ eax_ = v40;
/* 0x0041A7B4 and eax, 0Fh */ eax_ &= 0xFu;
/* 0x0041A7B7 mov al, byte_429E44[eax] */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)G8(0x429E44u + (eax_ & 0xFu));
/* 0x0041A7BD mov edx, eax */ edx_ = eax_;
/* 0x0041A7BF xor eax, eax */ eax_ = 0u;
/* 0x0041A7C1 and edx, 0FFh */ edx_ &= 0xFFu;
/* 0x0041A7C7 mov al, bl */ eax_ = (eax_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x0041A7C9 shl edx, 4 */ edx_ = (edx_ << 4) & U32MASK;
/* 0x0041A7CC mov al, byte_429E54[eax] */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)G8(0x429E54u + (eax_ & 0xFFu));
/* 0x0041A7D2 and eax, 0FFh */ eax_ &= 0xFFu;
/* 0x0041A7D7 add edx, eax */ edx_ = (edx_ + eax_) & U32MASK;
/* 0x0041A7D9 mov [esp+60h+var_40], edx */ v40 = edx_;
/* 0x0041A7DD jmp short loc_41A849 */ goto l_41a849;
l_41a7df:
/* 0x0041A7DF cmp cl, 2 */
/* 0x0041A7E2 jnz short loc_41A7F9 */ if ((ecx_ & 0xFFu) != 2u) goto l_41a7f9;
/* 0x0041A7E4 xor eax, eax */ eax_ = 0u;
/* 0x0041A7E6 mov al, bl */ eax_ = (eax_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x0041A7E8 xor edx, edx */ edx_ = 0u;
/* 0x0041A7EA mov dl, byte_429E54[eax] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(0x429E54u + (eax_ & 0xFFu));
/* 0x0041A7F0 mov eax, [esp+60h+var_48] */ eax_ = v48;
/* 0x0041A7F4 and eax, 1 */ eax_ &= 1u;
/* 0x0041A7F7 jmp short loc_41A840 */ goto l_41a840;
l_41a7f9:
/* 0x0041A7F9 cmp cl, 3 */
/* 0x0041A7FC jnz short loc_41A829 */ if ((ecx_ & 0xFFu) != 3u) goto l_41a829;
/* 0x0041A7FE xor eax, eax */ eax_ = 0u;
/* 0x0041A800 xor ecx, ecx */ ecx_ = 0u;
/* 0x0041A802 mov al, bl */ eax_ = (eax_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x0041A804 xor edx, edx */ edx_ = 0u;
/* 0x0041A806 mov ebx, 3 */ ebx_ = 3u;
/* 0x0041A80B mov cl, byte_429F54[eax] */ ecx_ = (ecx_ & 0xFFFFFF00UL) | (unsigned int)G8(0x429F54u + (eax_ & 0xFFu));
/* 0x0041A811 mov eax, [esp+60h+var_48] */ eax_ = v48;
/* 0x0041A815 div ebx */ qv = eax_; eax_ = qv / ebx_; edx_ = qv % ebx_;
/* 0x0041A817 shl edx, 5 */ edx_ = (edx_ << 5) & U32MASK;
/* 0x0041A81A add edx, ecx */ edx_ = (edx_ + ecx_) & U32MASK;
/* 0x0041A81C mov [esp+60h+var_40], edx */ v40 = edx_;
/* 0x0041A820 inc [esp+60h+var_48] */ v48 = (v48 + 1u) & U32MASK;
/* 0x0041A824 jmp loc_41A977 */ goto l_41a977;
l_41a829:
/* 0x0041A829 xor edx, edx */ edx_ = 0u;
/* 0x0041A82B mov dl, bl */ edx_ = (edx_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x0041A82D mov dl, byte_429E54[edx] */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)G8(0x429E54u + (edx_ & 0xFFu));
/* 0x0041A833 mov eax, [esp+60h+var_48] */ eax_ = v48;
/* 0x0041A837 and eax, 3 */ eax_ &= 3u;
/* 0x0041A83A and edx, 0FFh */ edx_ &= 0xFFu;
l_41a840:
/* 0x0041A840 shl eax, 4 */ eax_ = (eax_ << 4) & U32MASK;
/* 0x0041A843 add eax, edx */ eax_ = (eax_ + edx_) & U32MASK;
/* 0x0041A845 mov [esp+60h+var_40], eax */ v40 = eax_;
l_41a849:
/* 0x0041A849 inc [esp+60h+var_48] */ v48 = (v48 + 1u) & U32MASK;
/* 0x0041A84D jmp loc_41A977 */ goto l_41a977;
l_41a852:
/* 0x0041A852 mov dh, [esp+60h+var_10] */ edx_ = (edx_ & 0xFFFF00FFUL) | ((v10 & 0xFFu) << 8);
/* 0x0041A856 test dh, dh */
/* 0x0041A858 jz loc_41A977 */ if (((edx_ >> 8) & 0xFFu) == 0u) goto l_41a977;
/* 0x0041A85E cmp dh, 0FFh */
/* 0x0041A861 jz loc_41A977 */ if (((edx_ >> 8) & 0xFFu) == 0xFFu) goto l_41a977;
/* 0x0041A867 xor eax, eax */ eax_ = 0u;
/* 0x0041A869 mov al, dh */ eax_ = (eax_ & 0xFFFFFF00UL) | ((edx_ >> 8) & 0xFFu);
/* 0x0041A86B mov edx, [esp+60h+var_24] */ edx_ = v24;
/* 0x0041A86F mov ebx, dword_443760[eax*4] */ ebx_ = M32(0x443760u + (eax_ & 0xFFu) * 4u);
/* 0x0041A876 mov eax, edx */ eax_ = edx_;
/* 0x0041A878 mov ecx, dword_444278 */ ecx_ = M32(0x444278u);
/* 0x0041A87E sub eax, ebx */ eax_ = (eax_ - ebx_) & U32MASK;
/* 0x0041A880 and eax, ecx */ eax_ &= ecx_;
/* 0x0041A882 cmp eax, 100h */
/* 0x0041A887 jb short loc_41A8C0 */ if (eax_ < 0x100u) goto l_41a8c0;
/* 0x0041A889 mov al, [esp+60h+var_10] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v10 & 0xFFu);
/* 0x0041A88D dec al */ eax_ = (eax_ & 0xFFFFFF00UL) | (((eax_ & 0xFFu) - 1u) & 0xFFu);
/* 0x0041A88F and eax, 0FFh */ eax_ &= 0xFFu;
/* 0x0041A894 mov esi, dword_443760[eax*4] */ esi_ = M32(0x443760u + eax_ * 4u);
/* 0x0041A89B mov eax, edx */ eax_ = edx_;
/* 0x0041A89D sub eax, esi */ eax_ = (eax_ - esi_) & U32MASK;
/* 0x0041A89F and eax, ecx */ eax_ &= ecx_;
/* 0x0041A8A1 cmp eax, 100h */
/* 0x0041A8A6 jb short loc_41A8C0 */ if (eax_ < 0x100u) goto l_41a8c0;
/* 0x0041A8A8 mov al, [esp+60h+var_10] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v10 & 0xFFu);
/* 0x0041A8AC inc al */ eax_ = (eax_ & 0xFFFFFF00UL) | (((eax_ & 0xFFu) + 1u) & 0xFFu);
/* 0x0041A8AE and eax, 0FFh */ eax_ &= 0xFFu;
/* 0x0041A8B3 mov ebx, dword_443760[eax*4] */ ebx_ = M32(0x443760u + eax_ * 4u);
/* 0x0041A8BA mov eax, edx */ eax_ = edx_;
/* 0x0041A8BC sub eax, ebx */ eax_ = (eax_ - ebx_) & U32MASK;
/* 0x0041A8BE and eax, ecx */ eax_ &= ecx_;
l_41a8c0:
/* 0x0041A8C0 test eax, eax */
/* 0x0041A8C2 jz loc_41A966 */ if (eax_ == 0u) goto l_41a966;
/* 0x0041A8C8 cmp eax, 100h */
/* 0x0041A8CD jnb loc_41A966 */ if (eax_ >= 0x100u) goto l_41a966;
/* 0x0041A8D3 lea edi, ds:0[eax*4] */ edi_ = (eax_ * 4u) & U32MASK;
/* 0x0041A8DA mov esi, dword_442958[edi] */ esi_ = M32(0x442958u + edi_);
/* 0x0041A8E0 xor edx, edx */ edx_ = 0u;
/* 0x0041A8E2 inc esi */ esi_ = (esi_ + 1u) & U32MASK;
/* 0x0041A8E3 mov dl, byte_444EB4 */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EB4u);
/* 0x0041A8E9 mov dword_442958[edi], esi */ M32(0x442958u + edi_) = esi_;
/* 0x0041A8EF cmp eax, edx */
/* 0x0041A8F1 jz short loc_41A93F */ if (eax_ == edx_) goto l_41a93f;
/* 0x0041A8F3 mov bl, byte_441F18[eax] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(0x441F18u + eax_);
l_41a8f9:
/* 0x0041A8F9 xor edx, edx */ edx_ = 0u;
/* 0x0041A8FB mov dl, bl */ edx_ = (edx_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x0041A8FD mov esi, dword_442958[edi] */ esi_ = M32(0x442958u + edi_);
/* 0x0041A903 cmp esi, dword_442958[edx*4] */
/* 0x0041A90A jb short loc_41A93F */ if (esi_ < M32(0x442958u + (edx_ & 0xFFu) * 4u)) goto l_41a93f;
/* 0x0041A90C mov bh, byte_441F18[edx] */ ebx_ = (ebx_ & 0xFFFF00FFUL) | (((unsigned int)G8(0x441F18u + (edx_ & 0xFFu))) << 8);
/* 0x0041A912 mov byte_441F18[eax], bh */ M8(0x441F18u + eax_) = (unsigned char)((ebx_ >> 8) & 0xFFu);
/* 0x0041A918 mov bh, byte_442028[eax] */ ebx_ = (ebx_ & 0xFFFF00FFUL) | (((unsigned int)G8(0x442028u + eax_)) << 8);
/* 0x0041A91E mov byte_442028[edx], bh */ M8(0x442028u + (edx_ & 0xFFu)) = (unsigned char)((ebx_ >> 8) & 0xFFu);
/* 0x0041A924 mov byte_441F18[edx], al */ M8(0x441F18u + (edx_ & 0xFFu)) = (unsigned char)(eax_ & 0xFFu);
/* 0x0041A92A mov byte_442028[eax], bl */ M8(0x442028u + eax_) = (unsigned char)(ebx_ & 0xFFu);
/* 0x0041A930 mov bl, byte_441F18[eax] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(0x441F18u + eax_);
/* 0x0041A936 test bl, bl */
/* 0x0041A938 jnz short loc_41A8F9 */ if ((ebx_ & 0xFFu) != 0u) goto l_41a8f9;
/* 0x0041A93A mov byte_444EB4, al */ M8(0x444EB4u) = (unsigned char)(eax_ & 0xFFu);
l_41a93f:
/* 0x0041A93F inc ebp */ ebp_ = (ebp_ + 1u) & U32MASK;
/* 0x0041A940 cmp ebp, 100h */
/* 0x0041A946 jbe short loc_41A966 */ if (ebp_ <= 0x100u) goto l_41a966;
/* 0x0041A948 xor ebp, ebp */ ebp_ = 0u;
/* 0x0041A94A xor eax, eax */ eax_ = 0u;
l_41a94c:
/* 0x0041A94C mov esi, dword_442958[eax] */ esi_ = M32(0x442958u + eax_);
/* 0x0041A952 shr esi, 1 */ esi_ >>= 1;
/* 0x0041A954 add eax, 4 */ eax_ = (eax_ + 4u) & U32MASK;
/* 0x0041A957 add ebp, esi */ ebp_ = (ebp_ + esi_) & U32MASK;
/* 0x0041A959 mov dword_442954[eax], esi */ M32(0x442954u + eax_) = esi_;
/* 0x0041A95F cmp eax, 400h */
/* 0x0041A964 jnz short loc_41A94C */ if (eax_ != 0x400u) goto l_41a94c;
l_41a966:
/* 0x0041A966 xor eax, eax */ eax_ = 0u;
/* 0x0041A968 mov al, [esp+60h+var_10] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v10 & 0xFFu);
/* 0x0041A96C mov edx, [esp+60h+var_24] */ edx_ = v24;
/* 0x0041A970 mov dword_443760[eax*4], edx */ M32(0x443760u + (eax_ & 0xFFu) * 4u) = edx_;
l_41a977:
/* 0x0041A977 mov eax, [esp+60h+var_34] */ eax_ = v34;
/* 0x0041A97B mov edi, [esp+60h+var_24] */ edi_ = v24;
/* 0x0041A97F xor edx, edx */ edx_ = 0u;
/* 0x0041A981 mov ebx, [esp+60h+var_5C] */ ebx_ = v5c;
/* 0x0041A985 mov dl, [esp+60h+var_10] */ edx_ = (edx_ & 0xFFFFFF00UL) | (v10 & 0xFFu);
/* 0x0041A989 inc edi */ edi_ = (edi_ + 1u) & U32MASK;
/* 0x0041A98A shl eax, 8 */ eax_ = (eax_ << 8) & U32MASK;
/* 0x0041A98D mov [esp+60h+var_24], edi */ v24 = edi_;
/* 0x0041A991 add eax, edx */ eax_ = (eax_ + edx_) & U32MASK;
/* 0x0041A993 mov edx, [esp+60h+var_28] */ edx_ = v28;
/* 0x0041A997 mov [esp+60h+var_34], eax */ v34 = eax_;
/* 0x0041A99B mov eax, [esp+60h+var_30] */ eax_ = v30;
/* 0x0041A99F inc edx */ edx_ = (edx_ + 1u) & U32MASK;
/* 0x0041A9A0 inc eax */ eax_ = (eax_ + 1u) & U32MASK;
/* 0x0041A9A1 mov [esp+60h+var_28], edx */ v28 = edx_;
/* 0x0041A9A5 mov [esp+60h+var_30], eax */ v30 = eax_;
/* 0x0041A9A9 cmp edx, ebx */
/* 0x0041A9AB jb loc_41A57A */ if (edx_ < ebx_) goto l_41a57a;
l_41a9b1:
/* 0x0041A9B1 mov eax, [esp+60h+var_2C] */ eax_ = v2c;
/* 0x0041A9B5 mov dword_4441DC, ebp */ M32(0x4441DCu) = ebp_;
/* 0x0041A9BB add esp, 54h */
/* 0x0041A9BE pop ebp */
/* 0x0041A9BF pop edi */
/* 0x0041A9C0 pop esi */
/* 0x0041A9C1 retn */ sv = 0; qv = 0; return eax_;
}
/* agrees: (eax,edx,cl,ebx)). Exit al (0 or var_18 byte). */
unsigned int d3u_41adf0(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ecx_in, unsigned int ebx_in)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int ecx_ = 0;
unsigned int edx_ = 0;
unsigned int esi_ = 0;
unsigned int edi_ = 0;
unsigned int ebp_ = 0;
unsigned int v68 = 0;
unsigned int v64 = 0;
unsigned int v60 = 0;
unsigned int v5c = 0;
unsigned int v58 = 0;
unsigned int v54 = 0;
unsigned int v50 = 0;
unsigned int v4c = 0;
unsigned int v48 = 0;
unsigned int v44 = 0;
unsigned int v40 = 0;
unsigned int v3c = 0;
unsigned int v38 = 0;
unsigned int v34 = 0;
unsigned int v30 = 0;
unsigned int v28 = 0;
unsigned int v24 = 0;
unsigned int v20 = 0;
unsigned int v2c = 0;
unsigned int v1c = 0;
unsigned int v18 = 0;
unsigned int v14 = 0;
unsigned int v10 = 0;
unsigned int qv = 0;
int sv = 0;
/* 0x0041ADF0 push esi */
/* 0x0041ADF1 push edi */
/* 0x0041ADF2 push ebp */
/* 0x0041ADF3 sub esp, 5Ch */
/* 0x0041ADF6 mov [esp+68h+var_5C], eax */ v5c = eax_in;
/* 0x0041ADFA mov [esp+68h+var_38], edx */ v38 = edx_in;
/* 0x0041ADFE mov [esp+68h+var_3C], ebx */ v3c = ebx_in;
/* 0x0041AE02 mov eax, dword_444274 */ eax_ = M32(0x444274u);
/* 0x0041AE07 mov [esp+68h+var_2C], eax */ v2c = eax_;
/* 0x0041AE0B mov eax, dword_4441C0 */ eax_ = M32(0x4441C0u);
/* 0x0041AE10 mov [esp+68h+var_54], eax */ v54 = eax_;
/* 0x0041AE14 mov eax, dword_4441D0 */ eax_ = M32(0x4441D0u);
/* 0x0041AE19 mov [esp+68h+var_48], eax */ v48 = eax_;
/* 0x0041AE1D mov eax, dword_4441C4 */ eax_ = M32(0x4441C4u);
/* 0x0041AE22 mov [esp+68h+var_4C], eax */ v4c = eax_;
/* 0x0041AE26 mov al, byte_444EA8 */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EA8u);
/* 0x0041AE2B mov [esp+68h+var_10], al */ v10 = eax_ & 0xFFu;
/* 0x0041AE2F mov al, byte_444EA9 */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EA9u);
/* 0x0041AE34 xor edx, edx */ edx_ = 0u;
/* 0x0041AE36 mov [esp+68h+var_14], al */ v14 = eax_ & 0xFFu;
/* 0x0041AE3A mov al, byte_444E9D */ eax_ = (eax_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444E9Du);
/* 0x0041AE3F xor ebp, ebp */ ebp_ = 0u;
/* 0x0041AE41 mov [esp+68h+var_1C], al */ v1c = eax_ & 0xFFu;
/* 0x0041AE45 mov ah, 1 */ eax_ = (eax_ & 0xFFFF00FFUL) | (1u << 8);
/* 0x0041AE47 mov [esp+68h+var_24], edx */ v24 = edx_;
/* 0x0041AE4B mov byte_444EAB, ah */ M8(0x444EABu) = (unsigned char)((eax_ >> 8) & 0xFFu);
/* 0x0041AE51 cmp cl, ah */
/* 0x0041AE53 jnz loc_41AEDA */ if ((ecx_in & 0xFFu) != ((eax_ >> 8) & 0xFFu)) goto l_41aeda;
/* 0x0041AE59 mov ecx, 100h */ ecx_ = 0x100u;
/* 0x0041AE5E mov ebx, [esp+68h+var_2C] */ ebx_ = v2c;
/* 0x0041AE62 mov eax, dword_4299B8 */ eax_ = M32(0x4299B8u);
/* 0x0041AE67 mov esi, [esp+68h+var_38] */ esi_ = v38;
/* 0x0041AE6B add eax, ebx */ eax_ = (eax_ + ebx_) & U32MASK;
/* 0x0041AE6D cmp esi, ecx */
/* 0x0041AE6F jnb short loc_41AE73 */ if (esi_ >= ecx_) goto l_41ae73;
/* 0x0041AE71 mov ecx, esi */ ecx_ = esi_;
l_41ae73:
/* 0x0041AE73 mov edi, 0FFFFFFFFh */ edi_ = 0xFFFFFFFFUL;
l_41ae78:
/* 0x0041AE78 mov bl, [eax] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x0041AE7A cmp bl, 40h */
/* 0x0041AE7D jb short loc_41AE84 */ if ((ebx_ & 0xFFu) < 0x40u) goto l_41ae84;
/* 0x0041AE7F cmp bl, 0C0h */
/* 0x0041AE82 jb short loc_41AE8B */ if ((ebx_ & 0xFFu) < 0xC0u) goto l_41ae8b;
l_41ae84:
/* 0x0041AE84 mov esi, 1 */ esi_ = 1u;
/* 0x0041AE89 jmp short loc_41AE8D */ goto l_41ae8d;
l_41ae8b:
/* 0x0041AE8B mov esi, edi */ esi_ = edi_;
l_41ae8d:
/* 0x0041AE8D inc eax */ eax_ = (eax_ + 1u) & U32MASK;
/* 0x0041AE8E add edx, esi */ edx_ = (edx_ + esi_) & U32MASK;
/* 0x0041AE90 dec ecx */ ecx_ = (ecx_ - 1u) & U32MASK;
/* 0x0041AE91 jnz short loc_41AE78 */ if (ecx_ != 0u) goto l_41ae78;
/* 0x0041AE93 mov eax, [esp+68h+var_38] */ eax_ = v38;
/* 0x0041AE97 cmp eax, 100h */
/* 0x0041AE9C jbe short loc_41AECA */ if (eax_ <= 0x100u) goto l_41aeca;
/* 0x0041AE9E lea ecx, [eax-100h] */ ecx_ = (eax_ - 0x100u) & U32MASK;
/* 0x0041AEA4 mov esi, 0FFFFFFFFh */ esi_ = 0xFFFFFFFFUL;
/* 0x0041AEA9 mov eax, [esp+68h+var_5C] */ eax_ = v5c;
l_41aead:
/* 0x0041AEAD mov bl, [eax] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x0041AEAF not bl */ ebx_ = (ebx_ & 0xFFFFFF00UL) | ((~(ebx_ & 0xFFu)) & 0xFFu);
/* 0x0041AEB1 cmp bl, 40h */
/* 0x0041AEB4 jb short loc_41AEBB */ if ((ebx_ & 0xFFu) < 0x40u) goto l_41aebb;
/* 0x0041AEB6 cmp bl, 0C0h */
/* 0x0041AEB9 jb short loc_41AEC2 */ if ((ebx_ & 0xFFu) < 0xC0u) goto l_41aec2;
l_41aebb:
/* 0x0041AEBB mov ebx, 1 */ ebx_ = 1u;
/* 0x0041AEC0 jmp short loc_41AEC4 */ goto l_41aec4;
l_41aec2:
/* 0x0041AEC2 mov ebx, esi */ ebx_ = esi_;
l_41aec4:
/* 0x0041AEC4 inc eax */ eax_ = (eax_ + 1u) & U32MASK;
/* 0x0041AEC5 add edx, ebx */ edx_ = (edx_ + ebx_) & U32MASK;
/* 0x0041AEC7 dec ecx */ ecx_ = (ecx_ - 1u) & U32MASK;
/* 0x0041AEC8 jnz short loc_41AEAD */ if (ecx_ != 0u) goto l_41aead;
l_41aeca:
/* 0x0041AECA test edx, edx */
/* 0x0041AECC setnle al */ eax_ = (eax_ & 0xFFFFFF00UL) | ((((int)edx_ > 0) ? 1u : 0u));
/* 0x0041AECF add al, 5 */ eax_ = (eax_ & 0xFFFFFF00UL) | (((eax_ & 0xFFu) + 5u) & 0xFFu);
/* 0x0041AED1 mov [esp+68h+var_18], al */ v18 = eax_ & 0xFFu;
/* 0x0041AED5 jmp loc_41AFAC */ goto l_41afac;
l_41aeda:
/* 0x0041AEDA cmp cl, 2 */
/* 0x0041AEDD jnz loc_41AFA7 */ if ((ecx_in & 0xFFu) != 2u) goto l_41afa7;
/* 0x0041AEE3 mov edx, 100h */ edx_ = 0x100u;
/* 0x0041AEE8 mov ebx, [esp+68h+var_2C] */ ebx_ = v2c;
/* 0x0041AEEC mov edi, [esp+68h+var_38] */ edi_ = v38;
/* 0x0041AEF0 mov eax, dword_4299B8 */ eax_ = M32(0x4299B8u);
/* 0x0041AEF5 xor esi, esi */ esi_ = 0u;
/* 0x0041AEF7 xor ecx, ecx */ ecx_ = 0u;
/* 0x0041AEF9 add eax, ebx */ eax_ = (eax_ + ebx_) & U32MASK;
/* 0x0041AEFB cmp edi, edx */
/* 0x0041AEFD jnb short loc_41AF01 */ if (edi_ >= edx_) goto l_41af01;
/* 0x0041AEFF mov edx, edi */ edx_ = edi_;
l_41af01:
/* 0x0041AF01 mov edi, 0FFFFFFFFh */ edi_ = 0xFFFFFFFFUL;
l_41af06:
/* 0x0041AF06 mov bl, [eax] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x0041AF08 cmp bl, 40h */
/* 0x0041AF0B jb short loc_41AF12 */ if ((ebx_ & 0xFFu) < 0x40u) goto l_41af12;
/* 0x0041AF0D cmp bl, 0C0h */
/* 0x0041AF10 jb short loc_41AF19 */ if ((ebx_ & 0xFFu) < 0xC0u) goto l_41af19;
l_41af12:
/* 0x0041AF12 mov ebx, 1 */ ebx_ = 1u;
/* 0x0041AF17 jmp short loc_41AF1B */ goto l_41af1b;
l_41af19:
/* 0x0041AF19 mov ebx, edi */ ebx_ = edi_;
l_41af1b:
/* 0x0041AF1B inc eax */ eax_ = (eax_ + 1u) & U32MASK;
/* 0x0041AF1C add esi, ebx */ esi_ = (esi_ + ebx_) & U32MASK;
/* 0x0041AF1E dec edx */ edx_ = (edx_ - 1u) & U32MASK;
/* 0x0041AF1F jz short loc_41AF3C */ if (edx_ == 0u) goto l_41af3c;
/* 0x0041AF21 mov bl, [eax] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x0041AF23 cmp bl, 40h */
/* 0x0041AF26 jb short loc_41AF2D */ if ((ebx_ & 0xFFu) < 0x40u) goto l_41af2d;
/* 0x0041AF28 cmp bl, 0C0h */
/* 0x0041AF2B jb short loc_41AF34 */ if ((ebx_ & 0xFFu) < 0xC0u) goto l_41af34;
l_41af2d:
/* 0x0041AF2D mov ebx, 1 */ ebx_ = 1u;
/* 0x0041AF32 jmp short loc_41AF36 */ goto l_41af36;
l_41af34:
/* 0x0041AF34 mov ebx, edi */ ebx_ = edi_;
l_41af36:
/* 0x0041AF36 inc eax */ eax_ = (eax_ + 1u) & U32MASK;
/* 0x0041AF37 add ecx, ebx */ ecx_ = (ecx_ + ebx_) & U32MASK;
/* 0x0041AF39 dec edx */ edx_ = (edx_ - 1u) & U32MASK;
/* 0x0041AF3A jnz short loc_41AF06 */ if (edx_ != 0u) goto l_41af06;
l_41af3c:
/* 0x0041AF3C mov edi, [esp+68h+var_38] */ edi_ = v38;
/* 0x0041AF40 cmp edi, 100h */
/* 0x0041AF46 jbe short loc_41AF91 */ if (edi_ <= 0x100u) goto l_41af91;
/* 0x0041AF48 lea edx, [edi-100h] */ edx_ = (edi_ - 0x100u) & U32MASK;
/* 0x0041AF4E mov eax, [esp+68h+var_5C] */ eax_ = v5c;
/* 0x0041AF52 mov edi, 0FFFFFFFFh */ edi_ = 0xFFFFFFFFUL;
l_41af57:
/* 0x0041AF57 mov bl, [eax] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x0041AF59 not bl */ ebx_ = (ebx_ & 0xFFFFFF00UL) | ((~(ebx_ & 0xFFu)) & 0xFFu);
/* 0x0041AF5B cmp bl, 40h */
/* 0x0041AF5E jb short loc_41AF65 */ if ((ebx_ & 0xFFu) < 0x40u) goto l_41af65;
/* 0x0041AF60 cmp bl, 0C0h */
/* 0x0041AF63 jb short loc_41AF6C */ if ((ebx_ & 0xFFu) < 0xC0u) goto l_41af6c;
l_41af65:
/* 0x0041AF65 mov ebx, 1 */ ebx_ = 1u;
/* 0x0041AF6A jmp short loc_41AF6E */ goto l_41af6e;
l_41af6c:
/* 0x0041AF6C mov ebx, edi */ ebx_ = edi_;
l_41af6e:
/* 0x0041AF6E inc eax */ eax_ = (eax_ + 1u) & U32MASK;
/* 0x0041AF6F add esi, ebx */ esi_ = (esi_ + ebx_) & U32MASK;
/* 0x0041AF71 dec edx */ edx_ = (edx_ - 1u) & U32MASK;
/* 0x0041AF72 jz short loc_41AF91 */ if (edx_ == 0u) goto l_41af91;
/* 0x0041AF74 mov bl, [eax] */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (unsigned int)G8(eax_);
/* 0x0041AF76 not bl */ ebx_ = (ebx_ & 0xFFFFFF00UL) | ((~(ebx_ & 0xFFu)) & 0xFFu);
/* 0x0041AF78 cmp bl, 40h */
/* 0x0041AF7B jb short loc_41AF82 */ if ((ebx_ & 0xFFu) < 0x40u) goto l_41af82;
/* 0x0041AF7D cmp bl, 0C0h */
/* 0x0041AF80 jb short loc_41AF89 */ if ((ebx_ & 0xFFu) < 0xC0u) goto l_41af89;
l_41af82:
/* 0x0041AF82 mov ebx, 1 */ ebx_ = 1u;
/* 0x0041AF87 jmp short loc_41AF8B */ goto l_41af8b;
l_41af89:
/* 0x0041AF89 mov ebx, edi */ ebx_ = edi_;
l_41af8b:
/* 0x0041AF8B inc eax */ eax_ = (eax_ + 1u) & U32MASK;
/* 0x0041AF8C add ecx, ebx */ ecx_ = (ecx_ + ebx_) & U32MASK;
/* 0x0041AF8E dec edx */ edx_ = (edx_ - 1u) & U32MASK;
/* 0x0041AF8F jnz short loc_41AF57 */ if (edx_ != 0u) goto l_41af57;
l_41af91:
/* 0x0041AF91 test esi, esi */
/* 0x0041AF93 jg short loc_41AF99 */ if ((int)esi_ > 0) goto l_41af99;
/* 0x0041AF95 test ecx, ecx */
/* 0x0041AF97 jle short loc_41AF9D */ if ((int)ecx_ <= 0) goto l_41af9d;
l_41af99:
/* 0x0041AF99 mov al, 1 */ eax_ = (eax_ & 0xFFFFFF00UL) | 1u;
/* 0x0041AF9B jmp short loc_41AF9F */ goto l_41af9f;
l_41af9d:
/* 0x0041AF9D xor al, al */ eax_ = (eax_ & 0xFFFFFF00UL);
l_41af9f:
/* 0x0041AF9F add al, 7 */ eax_ = (eax_ & 0xFFFFFF00UL) | (((eax_ & 0xFFu) + 7u) & 0xFFu);
/* 0x0041AFA1 mov [esp+68h+var_18], al */ v18 = eax_ & 0xFFu;
/* 0x0041AFA5 jmp short loc_41AFAC */ goto l_41afac;
l_41afa7:
/* 0x0041AFA7 mov [esp+68h+var_18], 9 */ v18 = 9u;
l_41afac:
/* 0x0041AFAC xor edx, edx */ edx_ = 0u;
/* 0x0041AFAE mov dl, [esp+68h+var_18] */ edx_ = (edx_ & 0xFFFFFF00UL) | (v18 & 0xFFu);
/* 0x0041AFB2 mov cl, byte_444EA9 */ ecx_ = (ecx_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EA9u);
/* 0x0041AFB8 mov ebx, dword_429DB8[edx*4] */ ebx_ = M32(0x429DB8u + (edx_ & 0xFFu) * 4u);
/* 0x0041AFBF mov eax, dword_429E08[edx*4] */ eax_ = M32(0x429E08u + (edx_ & 0xFFu) * 4u);
/* 0x0041AFC6 mov [esp+68h+var_68], ebx */ v68 = ebx_;
/* 0x0041AFC9 mov [esp+68h+var_34], eax */ v34 = eax_;
/* 0x0041AFCD mov ebx, dword_429DE0[edx*4] */ ebx_ = M32(0x429DE0u + (edx_ & 0xFFu) * 4u);
/* 0x0041AFD4 shl edx, 8 */ edx_ = (edx_ << 8) & U32MASK;
/* 0x0041AFD7 mov al, [esp+68h+var_18] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v18 & 0xFFu);
/* 0x0041AFDB add edx, 30000h */ edx_ = (edx_ + 0x30000u) & U32MASK;
/* 0x0041AFE1 mov [esp+68h+var_60], ebx */ v60 = ebx_;
/* 0x0041AFE5 mov [esp+68h+var_64], edx */ v64 = edx_;
/* 0x0041AFE9 cmp al, cl */
/* 0x0041AFEB jz short loc_41B01A */ if ((eax_ & 0xFFu) == (ecx_ & 0xFFu)) goto l_41b01a;
/* 0x0041AFED mov eax, ebx */ eax_ = ebx_;
/* 0x0041AFEF mov dword_4441C4, ebx */ M32(0x4441C4u) = ebx_;
/* 0x0041AFF5 xor ch, ch */ ecx_ = (ecx_ & 0xFFFF00FFUL);
/* 0x0041AFF7 xor edx, edx */ edx_ = 0u;
/* 0x0041AFF9 xor al, bl */ eax_ = (eax_ & 0xFFFFFF00UL) | (((eax_ & 0xFFu) ^ (ebx_ & 0xFFu)) & 0xFFu);
/* 0x0041AFFB mov byte_444EA8, ch */ M8(0x444EA8u) = (unsigned char)((ecx_ >> 8) & 0xFFu);
/* 0x0041B001 mov byte_444E9D, al */ M8(0x444E9Du) = (unsigned char)(eax_ & 0xFFu);
/* 0x0041B006 mov al, [esp+68h+var_18] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v18 & 0xFFu);
/* 0x0041B00A mov dword_4441D0, edx */ M32(0x4441D0u) = edx_;
/* 0x0041B010 mov byte_444EA9, al */ M8(0x444EA9u) = (unsigned char)(eax_ & 0xFFu);
/* 0x0041B015 call sub_41B2D4 */ d3u_41b2d4(c);
l_41b01a:
/* 0x0041B01A mov edx, [esp+68h+var_34] */ edx_ = v34;
/* 0x0041B01E mov ebx, [esp+68h+var_60] */ ebx_ = v60;
/* 0x0041B022 mov edi, dword_4299F8 */ edi_ = M32(0x4299F8u);
/* 0x0041B028 mov esi, dword_4299F4 */ esi_ = M32(0x4299F4u);
/* 0x0041B02E shl edx, 9 */ edx_ = (edx_ << 9) & U32MASK;
/* 0x0041B031 shl ebx, 9 */ ebx_ = (ebx_ << 9) & U32MASK;
/* 0x0041B034 mov ecx, edx */ ecx_ = edx_;
/* 0x0041B036 add esi, ebx */ esi_ = (esi_ + ebx_) & U32MASK;
/* 0x0041B038 cld */
/* 0x0041B039 mov eax, ecx */ eax_ = ecx_;
/* 0x0041B03B sub ecx, edi */ ecx_ = (ecx_ - edi_) & U32MASK;
/* 0x0041B03D sub ecx, eax */ ecx_ = (ecx_ - eax_) & U32MASK;
/* 0x0041B03F and ecx, 3 */ ecx_ &= 3u;
/* 0x0041B042 sub eax, ecx */ eax_ = (eax_ - ecx_) & U32MASK;
/* 0x0041B044 jle short loc_41B052 */ if ((int)eax_ <= 0) goto l_41b052;
/* 0x0041B046 rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041B048 mov ecx, eax */ ecx_ = eax_;
/* 0x0041B04A and eax, 3 */ eax_ &= 3u;
/* 0x0041B04D shr ecx, 2 */ ecx_ >>= 2;
/* 0x0041B050 rep movsd */ while (ecx_ != 0u) { M32(edi_) = G32(esi_); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
l_41b052:
/* 0x0041B052 add ecx, eax */ ecx_ = (ecx_ + eax_) & U32MASK;
/* 0x0041B054 rep movsb */ while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
/* 0x0041B056 add edx, ebx */ edx_ = (edx_ + ebx_) & U32MASK;
/* 0x0041B058 mov ebx, [esp+68h+var_34] */ ebx_ = v34;
/* 0x0041B05C mov ecx, [esp+68h+var_60] */ ecx_ = v60;
/* 0x0041B060 add ebx, ebx */ ebx_ = (ebx_ + ebx_) & U32MASK;
/* 0x0041B062 add ecx, ecx */ ecx_ = (ecx_ + ecx_) & U32MASK;
/* 0x0041B064 mov eax, ebx */ eax_ = ebx_;
/* 0x0041B066 add ecx, ebx */ ecx_ = (ecx_ + ebx_) & U32MASK;
l_41b068:
/* 0x0041B068 mov ebx, [esp+68h+var_34] */ ebx_ = v34;
/* 0x0041B06C sub edx, 200h */ edx_ = (edx_ - 0x200u) & U32MASK;
/* 0x0041B072 sub eax, 2 */ eax_ = (eax_ - 2u) & U32MASK;
/* 0x0041B075 dec ebx */ ebx_ = (ebx_ - 1u) & U32MASK;
/* 0x0041B076 sub ecx, 2 */ ecx_ = (ecx_ - 2u) & U32MASK;
/* 0x0041B079 mov [esp+68h+var_34], ebx */ v34 = ebx_;
/* 0x0041B07D cmp eax, 0FFFFFFFEh */
/* 0x0041B080 jz short loc_41B0BA */ if (eax_ == 0xFFFFFFFEUL) goto l_41b0ba;
/* 0x0041B082 mov ebx, dword_4299FC */ ebx_ = M32(0x4299FCu);
/* 0x0041B088 mov esi, dword_429A04 */ esi_ = M32(0x429A04u);
/* 0x0041B08E mov bx, [ecx+ebx] */ ebx_ = (ebx_ & 0xFFFF0000UL) | (unsigned int)G16(ecx_ + ebx_);
/* 0x0041B092 mov [esi+eax], bx */ M16(esi_ + eax_) = (unsigned short)(ebx_ & 0xFFFFu);
/* 0x0041B096 mov esi, dword_4299F0 */ esi_ = M32(0x4299F0u);
/* 0x0041B09C and ebx, 0FFFFh */ ebx_ &= 0xFFFFu;
/* 0x0041B0A2 mov si, [edx+esi] */ esi_ = (esi_ & 0xFFFF0000UL) | (unsigned int)G16(edx_ + esi_);
/* 0x0041B0A6 and esi, 0FFFFh */ esi_ &= 0xFFFFu;
/* 0x0041B0AC add ebx, esi */ ebx_ = (ebx_ + esi_) & U32MASK;
/* 0x0041B0AE mov esi, dword_429A00 */ esi_ = M32(0x429A00u);
/* 0x0041B0B4 mov [esi+eax], bx */ M16(esi_ + eax_) = (unsigned short)(ebx_ & 0xFFFFu);
/* 0x0041B0B8 jmp short loc_41B068 */ goto l_41b068;
l_41b0ba:
/* 0x0041B0BA mov eax, [esp+68h+var_64] */ eax_ = v64;
/* 0x0041B0BE add eax, eax */ eax_ = (eax_ + eax_) & U32MASK;
/* 0x0041B0C0 mov ecx, [esp+68h+var_5C] */ ecx_ = v5c;
/* 0x0041B0C4 mov [esp+68h+var_40], eax */ v40 = eax_;
/* 0x0041B0C8 mov eax, [esp+68h+var_24] */ eax_ = v24;
/* 0x0041B0CC add eax, ecx */ eax_ = (eax_ + ecx_) & U32MASK;
/* 0x0041B0CE mov [esp+68h+var_44], eax */ v44 = eax_;
l_41b0d2:
/* 0x0041B0D2 cmp [esp+68h+var_24], 100h */
/* 0x0041B0DA jnb short loc_41B0EB */ if (v24 >= 0x100u) goto l_41b0eb;
/* 0x0041B0DC mov edi, [esp+68h+var_2C] */ edi_ = v2c;
/* 0x0041B0E0 mov eax, dword_4299B8 */ eax_ = M32(0x4299B8u);
/* 0x0041B0E5 add eax, edi */ eax_ = (eax_ + edi_) & U32MASK;
/* 0x0041B0E7 mov bh, [eax] */ ebx_ = (ebx_ & 0xFFFF00FFUL) | (((unsigned int)G8(eax_)) << 8);
/* 0x0041B0E9 jmp short loc_41B0F7 */ goto l_41b0f7;
l_41b0eb:
/* 0x0041B0EB mov ebx, [esp+68h+var_44] */ ebx_ = v44;
/* 0x0041B0EF mov bh, [ebx-100h] */ ebx_ = (ebx_ & 0xFFFF00FFUL) | (((unsigned int)G8(ebx_ - 0x100u)) << 8);
/* 0x0041B0F5 not bh */ ebx_ = (ebx_ & 0xFFFF00FFUL) | (((~((ebx_ >> 8) & 0xFFu)) & 0xFFu) << 8);
l_41b0f7:
/* 0x0041B0F7 mov ah, byte_444E9D */ eax_ = (eax_ & 0xFFFF00FFUL) | (((unsigned int)M8(0x444E9Du) << 8) & 0xFF00u);
/* 0x0041B0FD mov bl, bh */ ebx_ = (ebx_ & 0xFFFFFF00UL) | ((ebx_ >> 8) & 0xFFu);
/* 0x0041B0FF mov dl, byte_444EA8 */ edx_ = (edx_ & 0xFFFFFF00UL) | (unsigned int)M8(0x444EA8u);
/* 0x0041B105 sub bl, ah */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (((ebx_ & 0xFFu) - ((eax_ >> 8) & 0xFFu)) & 0xFFu);
/* 0x0041B107 test dl, dl */
/* 0x0041B109 jz short loc_41B10D */ if ((edx_ & 0xFFu) == 0u) goto l_41b10d;
/* 0x0041B10B neg bl */ ebx_ = (ebx_ & 0xFFFFFF00UL) | (((-(int)(ebx_ & 0xFFu)) & 0xFFu));
l_41b10d:
/* 0x0041B10D cmp bl, 80h */
/* 0x0041B110 jb short loc_41B122 */ if ((ebx_ & 0xFFu) < 0x80u) goto l_41b122;
/* 0x0041B112 mov dh, byte_444EA8 */ edx_ = (edx_ & 0xFFFF00FFUL) | (((unsigned int)M8(0x444EA8u) << 8) & 0xFF00u);
/* 0x0041B118 test dh, dh */
/* 0x0041B11A setz al */ eax_ = (eax_ & 0xFFFFFF00UL) | (((((edx_ >> 8) & 0xFFu) == 0u) ? 1u : 0u));
/* 0x0041B11D mov byte_444EA8, al */ M8(0x444EA8u) = (unsigned char)(eax_ & 0xFFu);
l_41b122:
/* 0x0041B122 mov eax, [esp+68h+var_60] */ eax_ = v60;
/* 0x0041B126 mov ecx, dword_4441C4 */ ecx_ = M32(0x4441C4u);
/* 0x0041B12C sub ecx, eax */ ecx_ = (ecx_ - eax_) & U32MASK;
/* 0x0041B12E xor edx, edx */ edx_ = 0u;
/* 0x0041B130 mov eax, ecx */ eax_ = ecx_;
/* 0x0041B132 mov dl, bl */ edx_ = (edx_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x0041B134 shl eax, 8 */ eax_ = (eax_ << 8) & U32MASK;
/* 0x0041B137 add eax, edx */ eax_ = (eax_ + edx_) & U32MASK;
/* 0x0041B139 mov esi, dword_4299F8 */ esi_ = M32(0x4299F8u);
/* 0x0041B13F add eax, eax */ eax_ = (eax_ + eax_) & U32MASK;
/* 0x0041B141 add esi, eax */ esi_ = (esi_ + eax_) & U32MASK;
/* 0x0041B143 mov [esp+68h+var_50], esi */ v50 = esi_;
/* 0x0041B147 mov ax, [esi] */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)G16(esi_);
/* 0x0041B14A lea esi, [ecx+ecx] */ esi_ = (ecx_ + ecx_) & U32MASK;
/* 0x0041B14D mov [esp+68h+var_30], esi */ v30 = esi_;
/* 0x0041B151 mov edi, [esp+68h+var_30] */ edi_ = v30;
/* 0x0041B155 mov esi, dword_429A00 */ esi_ = M32(0x429A00u);
/* 0x0041B15B add edi, esi */ edi_ = (edi_ + esi_) & U32MASK;
/* 0x0041B15D mov si, [edi] */ esi_ = (esi_ & 0xFFFF0000UL) | (unsigned int)G16(edi_);
/* 0x0041B160 mov [esp+68h+var_20], esi */ v20 = esi_;
/* 0x0041B164 mov esi, dword_4441C4 */ esi_ = M32(0x4441C4u);
/* 0x0041B16A mov [esp+68h+var_28], edi */ v28 = edi_;
/* 0x0041B16E mov si, word_44428A[esi*2] */ esi_ = (esi_ & 0xFFFF0000UL) | (unsigned int)G16(0x44428Au + esi_ * 2u);
/* 0x0041B176 test ax, ax */
/* 0x0041B179 jnz short loc_41B1DB */ if ((eax_ & 0xFFFFu) != 0u) goto l_41b1db;
/* 0x0041B17B add edx, [esp+68h+var_64] */ edx_ = (edx_ + v64) & U32MASK;
/* 0x0041B17F mov eax, dword_4299F4 */ eax_ = M32(0x4299F4u);
/* 0x0041B184 add edx, edx */ edx_ = (edx_ + edx_) & U32MASK;
/* 0x0041B186 add edx, eax */ edx_ = (edx_ + eax_) & U32MASK;
/* 0x0041B188 xor eax, eax */ eax_ = 0u;
/* 0x0041B18A mov edi, [esp+68h+var_40] */ edi_ = v40;
/* 0x0041B18E mov ax, [edx] */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)G16(edx_);
/* 0x0041B191 mov edx, dword_4299F0 */ edx_ = M32(0x4299F0u);
/* 0x0041B197 add edx, edi */ edx_ = (edx_ + edi_) & U32MASK;
/* 0x0041B199 shl eax, 0Eh */ eax_ = (eax_ << 14) & U32MASK;
/* 0x0041B19C mov dx, [edx] */ edx_ = (edx_ & 0xFFFF0000UL) | (unsigned int)G16(edx_);
/* 0x0041B19F and edx, 0FFFFh */ edx_ &= 0xFFFFu;
/* 0x0041B1A5 mov [esp+68h+var_58], edx */ v58 = edx_;
/* 0x0041B1A9 mov edi, [esp+68h+var_58] */ edi_ = v58;
/* 0x0041B1AD xor edx, edx */ edx_ = 0u;
/* 0x0041B1AF div edi */ qv = eax_; eax_ = qv / edi_; edx_ = qv % edi_;
/* 0x0041B1B1 mov ax, word_42D274[eax*2] */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)G16(0x42D274u + eax_ * 2u);
/* 0x0041B1B9 and eax, 0FFFFh */ eax_ &= 0xFFFFu;
/* 0x0041B1BE mov edx, dword_429A04 */ edx_ = M32(0x429A04u);
/* 0x0041B1C4 add ebp, eax */ ebp_ = (ebp_ + eax_) & U32MASK;
/* 0x0041B1C6 add edx, [esp+68h+var_30] */ edx_ = (edx_ + v30) & U32MASK;
/* 0x0041B1CA mov ax, [edx] */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)G16(edx_);
/* 0x0041B1CD mov edi, eax */ edi_ = eax_;
/* 0x0041B1CF add edi, esi */ edi_ = (edi_ + esi_) & U32MASK;
/* 0x0041B1D1 mov [edx], di */ M16(edx_) = (unsigned short)(edi_ & 0xFFFFu);
/* 0x0041B1D4 mov edx, [esp+68h+var_28] */ edx_ = v28;
/* 0x0041B1D8 add [edx], si */ M16(edx_) = (unsigned short)((G16(edx_) + (esi_ & 0xFFFFu)) & 0xFFFFu);
l_41b1db:
/* 0x0041B1DB and eax, 0FFFFh */ eax_ &= 0xFFFFu;
/* 0x0041B1E0 xor edi, edi */ edi_ = 0u;
/* 0x0041B1E2 xor edx, edx */ edx_ = 0u;
/* 0x0041B1E4 mov di, word ptr [esp+68h+var_20] */ edi_ = (edi_ & 0xFFFF0000UL) | (v20 & 0xFFFFu);
/* 0x0041B1E9 shl eax, 0Eh */ eax_ = (eax_ << 14) & U32MASK;
/* 0x0041B1EC div edi */ qv = eax_; eax_ = qv / edi_; edx_ = qv % edi_;
/* 0x0041B1EE mov ax, word_42D274[eax*2] */ eax_ = (eax_ & 0xFFFF0000UL) | (unsigned int)G16(0x42D274u + eax_ * 2u);
/* 0x0041B1F6 and eax, 0FFFFh */ eax_ &= 0xFFFFu;
/* 0x0041B1FB mov edx, [esp+68h+var_3C] */ edx_ = v3c;
/* 0x0041B1FF add ebp, eax */ ebp_ = (ebp_ + eax_) & U32MASK;
/* 0x0041B201 cmp ebp, edx */
/* 0x0041B203 ja short loc_41B27E */ if (ebp_ > edx_) goto l_41b27e;
/* 0x0041B205 mov eax, [esp+68h+var_50] */ eax_ = v50;
/* 0x0041B209 add [eax], si */ M16(eax_) = (unsigned short)((G16(eax_) + (esi_ & 0xFFFFu)) & 0xFFFFu);
/* 0x0041B20C mov eax, dword_429A00 */ eax_ = M32(0x429A00u);
/* 0x0041B211 mov di, [eax+ecx*2] */ edi_ = (edi_ & 0xFFFF0000UL) | (unsigned int)G16(eax_ + ecx_ * 2u);
/* 0x0041B215 add edi, esi */ edi_ = (edi_ + esi_) & U32MASK;
/* 0x0041B217 mov [eax+ecx*2], di */ M16(eax_ + ecx_ * 2u) = (unsigned short)(edi_ & 0xFFFFu);
/* 0x0041B21B cmp di, 4000h */
/* 0x0041B220 jb short loc_41B229 */ if ((edi_ & 0xFFFFu) < 0x4000u) goto l_41b229;
/* 0x0041B222 mov eax, ecx */ eax_ = ecx_;
/* 0x0041B224 call sub_41A284 */ eax_ = d3u_41a284(c, eax_);
l_41b229:
/* 0x0041B229 xor edx, edx */ edx_ = 0u;
/* 0x0041B22B xor eax, eax */ eax_ = 0u;
/* 0x0041B22D mov dl, bl */ edx_ = (edx_ & 0xFFFFFF00UL) | (ebx_ & 0xFFu);
/* 0x0041B22F mov al, bh */ eax_ = (eax_ & 0xFFFFFF00UL) | ((ebx_ >> 8) & 0xFFu);
/* 0x0041B231 call sub_41B69C */ d3u_41b69c(c, eax_, edx_);
/* 0x0041B236 mov ebx, dword_4441C0 */ ebx_ = M32(0x4441C0u);
/* 0x0041B23C mov ecx, [esp+68h+var_68] */ ecx_ = v68;
/* 0x0041B23F inc ebx */ ebx_ = (ebx_ + 1u) & U32MASK;
/* 0x0041B240 xor edx, edx */ edx_ = 0u;
/* 0x0041B242 mov eax, ebx */ eax_ = ebx_;
/* 0x0041B244 div ecx */ qv = eax_; eax_ = qv / ecx_; edx_ = qv % ecx_;
/* 0x0041B246 mov esi, [esp+68h+var_2C] */ esi_ = v2c;
/* 0x0041B24A mov edi, [esp+68h+var_44] */ edi_ = v44;
/* 0x0041B24E inc esi */ esi_ = (esi_ + 1u) & U32MASK;
/* 0x0041B24F inc edi */ edi_ = (edi_ + 1u) & U32MASK;
/* 0x0041B250 mov [esp+68h+var_2C], esi */ v2c = esi_;
/* 0x0041B254 mov dword_4441C0, ebx */ M32(0x4441C0u) = ebx_;
/* 0x0041B25A mov dword_4441D0, edx */ M32(0x4441D0u) = edx_;
/* 0x0041B260 call sub_41B318 */ d3u_41b318(c);
/* 0x0041B265 mov eax, [esp+68h+var_24] */ eax_ = v24;
/* 0x0041B269 mov [esp+68h+var_44], edi */ v44 = edi_;
/* 0x0041B26D inc eax */ eax_ = (eax_ + 1u) & U32MASK;
/* 0x0041B26E mov edx, [esp+68h+var_38] */ edx_ = v38;
/* 0x0041B272 mov [esp+68h+var_24], eax */ v24 = eax_;
/* 0x0041B276 cmp eax, edx */
/* 0x0041B278 jb loc_41B0D2 */ if (eax_ < edx_) goto l_41b0d2;
l_41b27e:
/* 0x0041B27E mov eax, [esp+68h+var_54] */ eax_ = v54;
/* 0x0041B282 mov dword_4441C0, eax */ M32(0x4441C0u) = eax_;
/* 0x0041B287 mov eax, [esp+68h+var_48] */ eax_ = v48;
/* 0x0041B28B mov dword_4441D0, eax */ M32(0x4441D0u) = eax_;
/* 0x0041B290 mov eax, [esp+68h+var_4C] */ eax_ = v4c;
/* 0x0041B294 mov dword_4441C4, eax */ M32(0x4441C4u) = eax_;
/* 0x0041B299 mov al, [esp+68h+var_10] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v10 & 0xFFu);
/* 0x0041B29D xor bl, bl */ ebx_ = (ebx_ & 0xFFFFFF00UL);
/* 0x0041B29F mov byte_444EA8, al */ M8(0x444EA8u) = (unsigned char)(eax_ & 0xFFu);
/* 0x0041B2A4 mov al, [esp+68h+var_14] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v14 & 0xFFu);
/* 0x0041B2A8 mov byte_444EAB, bl */ M8(0x444EABu) = (unsigned char)(ebx_ & 0xFFu);
/* 0x0041B2AE mov byte_444EA9, al */ M8(0x444EA9u) = (unsigned char)(eax_ & 0xFFu);
/* 0x0041B2B3 mov al, [esp+68h+var_1C] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v1c & 0xFFu);
/* 0x0041B2B7 mov ebx, [esp+68h+var_3C] */ ebx_ = v3c;
/* 0x0041B2BB mov byte_444E9D, al */ M8(0x444E9Du) = (unsigned char)(eax_ & 0xFFu);
/* 0x0041B2C0 cmp ebp, ebx */
/* 0x0041B2C2 jnb short loc_41B2CA */ if (ebp_ >= ebx_) goto l_41b2ca;
/* 0x0041B2C4 mov al, [esp+68h+var_18] */ eax_ = (eax_ & 0xFFFFFF00UL) | (v18 & 0xFFu);
/* 0x0041B2C8 jmp short loc_41B2CC */ goto l_41b2cc;
l_41b2ca:
/* 0x0041B2CA xor al, al */ eax_ = (eax_ & 0xFFFFFF00UL);
l_41b2cc:
/* 0x0041B2CC add esp, 5Ch */
/* 0x0041B2CF pop ebp */
/* 0x0041B2D0 pop edi */
/* 0x0041B2D1 pop esi */
/* 0x0041B2D2 retn */ sv = 0; qv = 0; return (eax_ & 0xFFu);
}

/* ---- model leaves (from draft_k3.c) ---- */
/* sub_417D74 (0x00417D74-0x00417F41, ~191 asm lines). Frame: var_1C=v1c,
 * var_18(byte)=v18. */
unsigned int d3u_417d74(dec3w_ctx *c, unsigned int eax_in, unsigned int dl_in)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    unsigned long esi = 0;
    unsigned long edi = 0;
    unsigned long ebp = 0;
    unsigned long v1c = 0;
    unsigned long v18 = 0;
    unsigned long qv = 0;
    /* 0x00417D74 push ebx/push ecx/push esi/push edi/push ebp + 0x00417D79 sub esp,8: no C equivalent */
    eax = (unsigned long)eax_in;
    /* 0x00417D7C mov bh,byte_444EAA */ ebx = (ebx & 0xFFFF00FFUL) | ((((unsigned long)G8(0x444EAAUL)) & 0xFFUL) << 8);
    /* 0x00417D82 mov ecx,eax */ ecx = eax;
    /* 0x00417D84 mov [var_18],dl */ v18 = (unsigned long)(dl_in & 0xFFUL);
    /* 0x00417D88 mov edi,dword_4299F4 */ edi = (unsigned long)G32(0x4299F4UL);
    /* 0x00417D8E shl eax,9 */ eax = (eax << 9) & U32MASK;
    /* 0x00417D91 xor edx,edx */ edx = 0UL;
    /* 0x00417D93 add edi,eax */ edi = (edi + eax) & U32MASK;
    /* 0x00417D95 mov [var_1C],edx */ v1c = edx;
    /* 0x00417D98 mov dl,[var_18] */ edx = (edx & 0xFFFFFF00UL) | (v18 & 0xFFUL);
    /* 0x00417D9C cmp word ptr [edi+edx*2],0 */ /* 0x00417DA1 jnz loc_417E24 */
    if (M16((edi + (edx & 0xFFFFUL) * 2UL) & U32MASK) != 0) goto l_417e24;
    /* 0x00417DA7 mov edx,dword_4299F0 */ edx = (unsigned long)G32(0x4299F0UL);
    /* 0x00417DAD mov dx,[edx+eax] */ edx = (edx & 0xFFFF0000UL) | (unsigned long)M16((edx + eax) & U32MASK);
    /* 0x00417DB1 xor eax,eax */ eax = 0UL;
l_417db3:
    /* 0x00417DB3 movzx ebp,bh */ ebp = (ebx >> 8) & 0xFFUL;
    /* 0x00417DB6 cmp eax,ebp */ /* 0x00417DB8 jnb loc_417DCA */
    if (eax >= ebp) goto l_417dca;
    /* 0x00417DBA movzx ebp,byte_444E82[eax] */ ebp = (unsigned long)M8((0x444E82UL + eax) & U32MASK);
    /* 0x00417DC1 mov si,[edi+ebp*2] */ esi = (esi & 0xFFFF0000UL) | (unsigned long)M16((edi + (ebp & 0xFFFFUL) * 2UL) & U32MASK);
    /* 0x00417DC5 inc eax */ eax = (eax + 1UL) & U32MASK;
    /* 0x00417DC6 sub edx,esi */ edx = (edx - esi) & U32MASK;
    /* 0x00417DC8 jmp loc_417DB3 */ goto l_417db3;
l_417dca:
    /* 0x00417DCA test dx,dx */ /* 0x00417DCD jz loc_417E12 */
    if ((edx & 0xFFFFUL) == 0) goto l_417e12;
    /* 0x00417DCF mov eax,dword_4299FC */ eax = (unsigned long)G32(0x4299FCUL);
    /* 0x00417DD4 add ecx,ecx */ ecx = (ecx + ecx) & U32MASK;
    /* 0x00417DD6 add eax,ecx */ eax = (eax + ecx) & U32MASK;
    /* 0x00417DD8 xor ecx,ecx */ ecx = 0UL;
    /* 0x00417DDA mov [var_1C],edi */ v1c = edi;
    /* 0x00417DDD mov cx,[eax] */ ecx = (ecx & 0xFFFF0000UL) | (unsigned long)M16(eax & U32MASK);
    /* 0x00417DE0 and edx,0FFFFh */ edx &= 0xFFFFUL;
    /* 0x00417DE6 mov ax,[eax] */ eax = (eax & 0xFFFF0000UL) | (unsigned long)M16(eax & U32MASK);
    /* 0x00417DE9 add ecx,edx */ ecx = (ecx + edx) & U32MASK;
    /* 0x00417DEB and eax,0FFFFh */ eax &= 0xFFFFUL;
    /* 0x00417DF0 xor edx,edx */ edx = 0UL;
    /* 0x00417DF2 shl eax,0Eh */ eax = (eax << 14) & U32MASK;
    /* 0x00417DF5 div ecx */ qv = eax; eax = qv / ecx; edx = qv % ecx;
    /* 0x00417DF7 mov ax,word_42D274[eax*2] */ eax = (eax & 0xFFFF0000UL) | (unsigned long)M16((0x42D274UL + eax * 2UL) & U32MASK);
    /* 0x00417DFF mov ecx,dword_4441F4 */ ecx = (unsigned long)G32(0x4441F4UL);
    /* 0x00417E05 and eax,0FFFFh */ eax &= 0xFFFFUL;
    /* 0x00417E0A add ecx,eax */ ecx = (ecx + eax) & U32MASK;
    /* 0x00417E0C mov dword_4441F4,ecx */ G32(0x4441F4UL) = (unsigned int)ecx;
l_417e12:
    /* 0x00417E12 mov ecx,dword_44421C */ ecx = (unsigned long)G32(0x44421CUL);
    /* 0x00417E18 mov edi,ecx */ edi = ecx;
    /* 0x00417E1A mov eax,dword_4299F4 */ eax = (unsigned long)G32(0x4299F4UL);
    /* 0x00417E1F shl edi,9 */ edi = (edi << 9) & U32MASK;
    /* 0x00417E22 add edi,eax */ edi = (edi + eax) & U32MASK;
l_417e24:
    /* 0x00417E24 mov eax,ecx */ eax = ecx;
    /* 0x00417E26 mov edx,dword_4299F0 */ edx = (unsigned long)G32(0x4299F0UL);
    /* 0x00417E2C shl eax,9 */ eax = (eax << 9) & U32MASK;
    /* 0x00417E2F mov si,[edx+eax] */ esi = (esi & 0xFFFF0000UL) | (unsigned long)M16((edx + eax) & U32MASK);
    /* 0x00417E33 cmp ecx,2FDh */ /* 0x00417E39 jnb loc_417E64 */
    if (ecx >= 0x2FDUL) goto l_417e64;
    /* 0x00417E3B mov edx,dword_4299FC */ edx = (unsigned long)G32(0x4299FCUL);
    /* 0x00417E41 xor eax,eax */ eax = 0UL;
    /* 0x00417E43 add si,[edx+ecx*2] */ esi = (esi & 0xFFFF0000UL) | ((((esi & 0xFFFFUL) + (unsigned long)M16((edx + (ecx & 0xFFFFUL) * 2UL) & U32MASK)) & 0xFFFFUL));
l_417e47:
    /* 0x00417E47 xor edx,edx */ edx = 0UL;
    /* 0x00417E49 mov dl,bh */ edx = (edx & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL);
    /* 0x00417E4B cmp eax,edx */ /* 0x00417E4D jnb loc_417EFD */
    if (eax >= edx) goto l_417efd;
    /* 0x00417E53 xor edx,edx */ edx = 0UL;
    /* 0x00417E55 mov dl,byte_444E82[eax] */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8((0x444E82UL + eax) & U32MASK);
    /* 0x00417E5B mov cx,[edi+edx*2] */ ecx = (ecx & 0xFFFF0000UL) | (unsigned long)M16((edi + (edx & 0xFFFFUL) * 2UL) & U32MASK);
    /* 0x00417E5F inc eax */ eax = (eax + 1UL) & U32MASK;
    /* 0x00417E60 sub esi,ecx */ esi = (esi - ecx) & U32MASK;
    /* 0x00417E62 jmp loc_417E47 */ goto l_417e47;
l_417e64:
    /* 0x00417E64 test bh,bh */ /* 0x00417E66 jnz loc_417E92 */
    if (((ebx >> 8) & 0xFFUL) != 0) goto l_417e92;
    /* 0x00417E68 cmp [var_1C],0 */ /* 0x00417E6C jz loc_417EFD */
    if (v1c == 0UL) goto l_417efd;
    /* 0x00417E72 mov ebp,[var_1C] */ ebp = v1c;
    /* 0x00417E75 xor al,al */ eax &= 0xFFFFFF00UL;
l_417e77:
    /* 0x00417E77 xor edx,edx */ edx = 0UL;
    /* 0x00417E79 mov dl,al */ edx = (edx & 0xFFFFFF00UL) | (eax & 0xFFUL);
    /* 0x00417E7B add edx,edx */ edx = (edx + edx) & U32MASK;
    /* 0x00417E7D cmp word ptr [edx+ebp],0 */ /* 0x00417E82 jz loc_417E88 */
    if (M16((edx + ebp) & U32MASK) == 0) goto l_417e88;
    /* 0x00417E84 sub si,[edx+edi] */ esi = (esi & 0xFFFF0000UL) | ((((esi & 0xFFFFUL) - (unsigned long)M16((edx + edi) & U32MASK)) & 0xFFFFUL));
l_417e88:
    /* 0x00417E88 inc al */ eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFUL) + 1UL) & 0xFFUL);
    /* 0x00417E8A jz loc_417EFD */
    if ((eax & 0xFFUL) == 0UL) goto l_417efd;
    /* 0x00417E90 jmp loc_417E77 */ goto l_417e77;
l_417e92:
    /* 0x00417E92 cmp [var_1C],0 */ /* 0x00417E96 jz loc_417EE2 */
    if (v1c == 0UL) goto l_417ee2;
    /* 0x00417E98 xor bl,bl */ ebx = (ebx & 0xFFFFFF00UL) | 0UL;
l_417e9a:
    /* 0x00417E9A xor eax,eax */ eax = 0UL;
    /* 0x00417E9C mov al,bl */ eax = (eax & 0xFFFFFF00UL) | (ebx & 0xFFUL);
    /* 0x00417E9E mov edx,[var_1C] */ edx = v1c;
    /* 0x00417EA1 add eax,eax */ eax = (eax + eax) & U32MASK;
    /* 0x00417EA3 add eax,edx */ eax = (eax + edx) & U32MASK;
    /* 0x00417EA5 cmp word ptr [eax],0 */ /* 0x00417EA9 setnz dl */
    edx = (edx & 0xFFFFFF00UL) | ((M16(eax & U32MASK) != 0) ? 1UL : 0UL);
    /* 0x00417EAC xor eax,eax */ eax = 0UL;
    /* 0x00417EAE mov cl,dl */ ecx = (ecx & 0xFFFFFF00UL) | (edx & 0xFFUL);
    /* 0x00417EB0 test dl,dl */ /* 0x00417EB2 jnz loc_417ED0 */
    if ((edx & 0xFFUL) != 0UL) goto l_417ed0;
l_417eb4:
    /* 0x00417EB4 xor edx,edx */ edx = 0UL;
    /* 0x00417EB6 mov dl,bh */ edx = (edx & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL);
    /* 0x00417EB8 cmp eax,edx */ /* 0x00417EBA jnb loc_417ED0 */
    if (eax >= edx) goto l_417ed0;
    /* 0x00417EBC mov edx,eax */ edx = eax;
    /* 0x00417EBE mov cl,byte_444E82[edx] */ ecx = (ecx & 0xFFFFFF00UL) | (unsigned long)M8((0x444E82UL + edx) & U32MASK);
    /* 0x00417EC4 inc eax */ eax = (eax + 1UL) & U32MASK;
    /* 0x00417EC5 cmp bl,cl */ /* 0x00417EC7 setz dl */
    edx = (edx & 0xFFFFFF00UL) | ((((ebx & 0xFFUL) == (ecx & 0xFFUL)) ? 1UL : 0UL));
    /* 0x00417ECA mov cl,dl */ ecx = (ecx & 0xFFFFFF00UL) | (edx & 0xFFUL);
    /* 0x00417ECC test dl,dl */ /* 0x00417ECE jz loc_417EB4 */
    if ((edx & 0xFFUL) == 0UL) goto l_417eb4;
l_417ed0:
    /* 0x00417ED0 test cl,cl */ /* 0x00417ED2 jz loc_417EDC */
    if ((ecx & 0xFFUL) == 0UL) goto l_417edc;
    /* 0x00417ED4 xor eax,eax */ eax = 0UL;
    /* 0x00417ED6 mov al,bl */ eax = (eax & 0xFFFFFF00UL) | (ebx & 0xFFUL);
    /* 0x00417ED8 sub si,[edi+eax*2] */ esi = (esi & 0xFFFF0000UL) | ((((esi & 0xFFFFUL) - (unsigned long)M16((edi + (eax & 0xFFFFUL) * 2UL) & U32MASK)) & 0xFFFFUL));
l_417edc:
    /* 0x00417EDC inc bl; 0x00417EDE jz loc_417EFD.
     * NOTE: keep the wrap test on the isolated new-bl local (_bl). An
     * earlier form tested (ebx & 0xFFUL) after merging, which gcc -O2
     * fused with the merge OR into a full-register je: once bh (or upper
     * bits) are nonzero the exit never fires and bl cycles forever. */
    { unsigned long _bl = (((ebx & 0xFFUL) + 1UL) & 0xFFUL); ebx = (ebx & 0xFFFFFF00UL) | _bl; if (_bl == 0UL) goto l_417efd; }
    /* 0x00417EDE jz loc_417EFD (folded into the _bl test above) */
    /* 0x00417EE0 jmp loc_417E9A */ goto l_417e9a;
l_417ee2:
    /* 0x00417EE2 xor eax,eax */ eax = 0UL;
l_417ee4:
    /* 0x00417EE4 xor edx,edx */ edx = 0UL;
    /* 0x00417EE6 mov dl,bh */ edx = (edx & 0xFFFFFF00UL) | ((ebx >> 8) & 0xFFUL);
    /* 0x00417EE8 cmp eax,edx */ /* 0x00417EEA jnb loc_417EFD */
    if (eax >= edx) goto l_417efd;
    /* 0x00417EEC xor edx,edx */ edx = 0UL;
    /* 0x00417EEE mov dl,byte_444E82[eax] */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8((0x444E82UL + eax) & U32MASK);
    /* 0x00417EF4 mov cx,[edi+edx*2] */ ecx = (ecx & 0xFFFF0000UL) | (unsigned long)M16((edi + (edx & 0xFFFFUL) * 2UL) & U32MASK);
    /* 0x00417EF8 inc eax */ eax = (eax + 1UL) & U32MASK;
    /* 0x00417EF9 sub esi,ecx */ esi = (esi - ecx) & U32MASK;
    /* 0x00417EFB jmp loc_417EE4 */ goto l_417ee4;
l_417efd:
    /* 0x00417EFD xor eax,eax */ eax = 0UL;
    /* 0x00417EFF mov al,[var_18] */ eax = (eax & 0xFFFFFF00UL) | (v18 & 0xFFUL);
    /* 0x00417F03 mov ax,[edi+eax*2] */ eax = (eax & 0xFFFF0000UL) | (unsigned long)M16((edi + (eax & 0xFFFFUL) * 2UL) & U32MASK);
    /* 0x00417F07 and eax,0FFFFh */ eax &= 0xFFFFUL;
    /* 0x00417F0C xor ecx,ecx */ ecx = 0UL;
    /* 0x00417F0E xor edx,edx */ edx = 0UL;
    /* 0x00417F10 shl eax,0Eh */ eax = (eax << 14) & U32MASK;
    /* 0x00417F13 mov cx,si */ ecx = (ecx & 0xFFFF0000UL) | (esi & 0xFFFFUL);
    /* 0x00417F16 div ecx */ qv = eax; eax = qv / ecx; edx = qv % ecx;
    /* 0x00417F18 mov ax,word_42D274[eax*2] */ eax = (eax & 0xFFFF0000UL) | (unsigned long)M16((0x42D274UL + eax * 2UL) & U32MASK);
    /* 0x00417F20 mov ecx,dword_4441F4 */ ecx = (unsigned long)G32(0x4441F4UL);
    /* 0x00417F26 and eax,0FFFFh */ eax &= 0xFFFFUL;
    /* 0x00417F2B add ecx,eax */ ecx = (ecx + eax) & U32MASK;
    /* 0x00417F2D mov dword_4441F4,ecx */ G32(0x4441F4UL) = (unsigned int)ecx;
    /* 0x00417F33 mov byte_444EAA,bh */ G8(0x444EAAUL) = (unsigned char)((ebx >> 8) & 0xFFUL);
    /* 0x00417F39 add esp,8 + pops + 0x00417F41 retn: no C equivalent */
    return (unsigned int)eax;
}
/* sub_41A284 (0x0041A284-0x0041A338, ~66 asm lines). No stack frame. */
unsigned int d3u_41a284(dec3w_ctx *c, unsigned int eax_in)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    unsigned long esi = 0;
    unsigned long edi = 0;
    unsigned long ebp = 0;
    /* prologue pushes: no C equivalent */
    /* 0x0041A28A mov ebp,eax */ ebp = (unsigned long)eax_in;
    /* 0x0041A28C mov ebx,eax */ ebx = (unsigned long)eax_in;
    /* 0x0041A28E mov eax,dword_4299F8 */ eax = (unsigned long)G32(0x4299F8UL);
    /* 0x0041A293 shl ebx,9 */ ebx = (ebx << 9) & U32MASK;
    /* 0x0041A296 add ebx,eax */ ebx = (ebx + eax) & U32MASK;
    /* 0x0041A298 xor ecx,ecx */ ecx = 0UL;
    /* 0x0041A29A lea edi,[ebx+200h] */ edi = (ebx + 0x200UL) & U32MASK;
l_41a2a0:
    /* 0x0041A2A0 mov eax,[ebx] */ eax = (unsigned long)M32(ebx & U32MASK);
    /* 0x0041A2A2 mov edx,[ebx+4] */ edx = (unsigned long)M32((ebx + 4UL) & U32MASK);
    /* 0x0041A2A5 add eax,10001h */ eax = (eax + 0x10001UL) & U32MASK;
    /* 0x0041A2AA add edx,10001h */ edx = (edx + 0x10001UL) & U32MASK;
    /* 0x0041A2B0 and eax,0FFFEFFFEh */ eax &= 0xFFFEFFFEUL;
    /* 0x0041A2B5 and edx,0FFFEFFFEh */ edx &= 0xFFFEFFFEUL;
    /* 0x0041A2BB shr eax,1 */ eax = (eax >> 1) & U32MASK;
    /* 0x0041A2BD shr edx,1 */ edx = (edx >> 1) & U32MASK;
    /* 0x0041A2BF add ebx,10h */ ebx = (ebx + 0x10UL) & U32MASK;
    /* 0x0041A2C2 lea esi,[eax+edx] */ esi = (eax + edx) & U32MASK;
    /* 0x0041A2C5 mov [ebx-0Ch],edx */ M32((ebx - 0xCUL) & U32MASK) = (unsigned int)edx;
    /* 0x0041A2C8 mov [ebx-10h],eax */ M32((ebx - 0x10UL) & U32MASK) = (unsigned int)eax;
    /* 0x0041A2CB mov edx,[ebx-4] */ edx = (unsigned long)M32((ebx - 4UL) & U32MASK);
    /* 0x0041A2CE mov eax,[ebx-8] */ eax = (unsigned long)M32((ebx - 8UL) & U32MASK);
    /* 0x0041A2D1 add edx,10001h */ edx = (edx + 0x10001UL) & U32MASK;
    /* 0x0041A2D7 add eax,10001h */ eax = (eax + 0x10001UL) & U32MASK;
    /* 0x0041A2DC and edx,0FFFEFFFEh */ edx &= 0xFFFEFFFEUL;
    /* 0x0041A2E2 and eax,0FFFEFFFEh */ eax &= 0xFFFEFFFEUL;
    /* 0x0041A2E7 shr edx,1 */ edx = (edx >> 1) & U32MASK;
    /* 0x0041A2E9 shr eax,1 */ eax = (eax >> 1) & U32MASK;
    /* 0x0041A2EB add ecx,esi */ ecx = (ecx + esi) & U32MASK;
    /* 0x0041A2ED lea esi,[eax+edx] */ esi = (eax + edx) & U32MASK;
    /* 0x0041A2F0 mov [ebx-4],edx */ M32((ebx - 4UL) & U32MASK) = (unsigned int)edx;
    /* 0x0041A2F3 add ecx,esi */ ecx = (ecx + esi) & U32MASK;
    /* 0x0041A2F5 mov [ebx-8],eax */ M32((ebx - 8UL) & U32MASK) = (unsigned int)eax;
    /* 0x0041A2F8 cmp ebx,edi */ /* 0x0041A2FA jnz loc_41A2A0 */
    if (ebx != edi) goto l_41a2a0;
    /* 0x0041A2FC mov eax,dword_429A04 */ eax = (unsigned long)G32(0x429A04UL);
    /* 0x0041A301 mov dx,[eax+ebp*2] */ edx = (edx & 0xFFFF0000UL) | (unsigned long)M16((eax + (ebp & 0xFFFFUL) * 2UL) & U32MASK);
    /* 0x0041A305 shr dx,2 */ edx = (edx & 0xFFFF0000UL) | ((edx & 0xFFFFUL) >> 2);
    /* 0x0041A309 mov ebx,edx */ ebx = edx;
    /* 0x0041A30B mov [eax+ebp*2],dx */ M16((eax + (ebp & 0xFFFFUL) * 2UL) & U32MASK) = (unsigned short)(edx & 0xFFFFUL);
    /* 0x0041A30F inc ebx */ ebx = (ebx + 1UL) & U32MASK;
    /* 0x0041A310 mov [eax+ebp*2],bx */ M16((eax + (ebp & 0xFFFFUL) * 2UL) & U32MASK) = (unsigned short)(ebx & 0xFFFFUL);
    /* 0x0041A314 mov eax,ecx */ eax = ecx;
    /* 0x0041A316 xor edx,edx */ edx = 0UL;
    /* 0x0041A318 and eax,0FFFFh */ eax &= 0xFFFFUL;
    /* 0x0041A31D shr ecx,10h */ ecx = (ecx >> 16) & U32MASK;
    /* 0x0041A320 mov dx,bx */ edx = (edx & 0xFFFF0000UL) | (ebx & 0xFFFFUL);
    /* 0x0041A323 add ecx,eax */ ecx = (ecx + eax) & U32MASK;
    /* 0x0041A325 lea eax,[ecx+edx] */ eax = (ecx + edx) & U32MASK;
    /* 0x0041A328 mov edx,dword_429A00 */ edx = (unsigned long)G32(0x429A00UL);
    /* 0x0041A32E mov [edx+ebp*2],ax */ M16((edx + (ebp & 0xFFFFUL) * 2UL) & U32MASK) = (unsigned short)(eax & 0xFFFFUL);
    /* pops + 0x0041A338 retn: no C equivalent */
    return (unsigned int)eax;
}
/* sub_41B318 (0x0041B318-0x0041B64A, ~261 asm lines). No stack frame,
 * no entry regs, void return (see header). */
void d3u_41b318(dec3w_ctx *c)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    unsigned long esi = 0;
    unsigned long edi = 0;
    unsigned long ebp = 0;
    unsigned long cf = 0;
    /* prologue pushes: no C equivalent */
    /* 0x0041B31E mov esi,dword_4441B0 */ esi = (unsigned long)G32(0x4441B0UL);
    /* 0x0041B324 mov edi,dword_4441D0 */ edi = (unsigned long)G32(0x4441D0UL);
    /* 0x0041B32A cmp byte_444EAB,0 */ /* 0x0041B331 jnz loc_41B33F */
    if (G8(0x444EABUL) != 0) goto l_41b33f;
    /* 0x0041B333 cmp esi,dword_4441B8 */ /* 0x0041B339 jb loc_41B60F */
    if (esi < (unsigned long)G32(0x4441B8UL)) goto l_41b60f;
l_41b33f:
    /* 0x0041B33F cmp byte_444EA9,7 */ /* 0x0041B346 jnb loc_41B4CF */
    if (G8(0x444EA9UL) >= 7) goto l_41b4cf;
    /* 0x0041B34C mov edx,dword_43D8BC */ edx = (unsigned long)G32(0x43D8BCUL);
    /* 0x0041B352 xor ecx,ecx */ ecx = 0UL;
    /* 0x0041B354 mov ebp,dword_43D8B4 */ ebp = (unsigned long)G32(0x43D8B4UL);
    /* 0x0041B35A mov dword_43DCCC,ecx */ M32(0x43DCCCUL) = (unsigned int)ecx;
    /* 0x0041B360 mov dword_43DCD0,edx */ M32(0x43DCD0UL) = (unsigned int)edx;
    /* 0x0041B366 lea eax,[edx+edx] */ eax = (edx + edx) & U32MASK;
    /* 0x0041B369 mov ecx,dword_4441C0 */ ecx = (unsigned long)G32(0x4441C0UL);
    /* 0x0041B36F mov edx,dword_43D8C0 */ edx = (unsigned long)G32(0x43D8C0UL);
    /* 0x0041B375 sub ecx,ebp */ ecx = (ecx - ebp) & U32MASK;
    /* 0x0041B377 sub eax,edx */ eax = (eax - edx) & U32MASK;
    /* 0x0041B379 and ecx,7Fh */ ecx &= 0x7FUL;
    /* 0x0041B37C mov dword_43DCD4,eax */ M32(0x43DCD4UL) = (unsigned int)eax;
    /* 0x0041B381 mov edx,dword_43DACC[ecx*4] */ edx = (unsigned long)M32((0x43DACCUL + ecx * 4UL) & U32MASK);
    /* 0x0041B388 mov eax,dword_43F504 */ eax = (unsigned long)G32(0x43F504UL);
    /* 0x0041B38D mov dword_43F50C,edx */ M32(0x43F50CUL) = (unsigned int)edx;
    /* 0x0041B393 mov dword_43F508,eax */ M32(0x43F508UL) = (unsigned int)eax;
    /* 0x0041B398 mov eax,dword_43F500 */ eax = (unsigned long)G32(0x43F500UL);
    /* 0x0041B39D mov edx,dword_43F528 */ edx = (unsigned long)G32(0x43F528UL);
    /* 0x0041B3A3 mov dword_43F504,eax */ M32(0x43F504UL) = (unsigned int)eax;
    /* 0x0041B3A8 sub edx,dword_43F4FC */ edx = (edx - (unsigned long)M32(0x43F4FCUL)) & U32MASK;
    /* 0x0041B3AE mov eax,dword_43F510 */ eax = (unsigned long)G32(0x43F510UL);
    /* 0x0041B3B3 mov dword_43F500,edx */ M32(0x43F500UL) = (unsigned int)edx;
    /* 0x0041B3B9 mov edx,dword_43F528 */ edx = (unsigned long)G32(0x43F528UL);
    /* 0x0041B3BF imul eax,edx */ eax = (eax * edx) & U32MASK;
    /* 0x0041B3C2 mov dword_43F4FC,edx */ M32(0x43F4FCUL) = (unsigned int)edx;
    /* 0x0041B3C8 mov edx,dword_43F524 */ edx = (unsigned long)G32(0x43F524UL);
    /* 0x0041B3CE shl edx,4 */ edx = (edx << 4) & U32MASK;
    /* 0x0041B3D1 mov ebp,dword_43F500 */ ebp = (unsigned long)M32(0x43F500UL);
    /* 0x0041B3D7 add eax,edx */ eax = (eax + edx) & U32MASK;
    /* 0x0041B3D9 mov edx,dword_43F514 */ edx = (unsigned long)G32(0x43F514UL);
    /* 0x0041B3DF imul edx,ebp */ edx = (edx * ebp) & U32MASK;
    /* 0x0041B3E2 mov ebp,dword_43F504 */ ebp = (unsigned long)M32(0x43F504UL);
    /* 0x0041B3E8 add eax,edx */ eax = (eax + edx) & U32MASK;
    /* 0x0041B3EA mov edx,dword_43F518 */ edx = (unsigned long)G32(0x43F518UL);
    /* 0x0041B3F0 imul edx,ebp */ edx = (edx * ebp) & U32MASK;
    /* 0x0041B3F3 mov ebp,dword_43F508 */ ebp = (unsigned long)M32(0x43F508UL);
    /* 0x0041B3F9 add eax,edx */ eax = (eax + edx) & U32MASK;
    /* 0x0041B3FB mov edx,dword_43F51C */ edx = (unsigned long)G32(0x43F51CUL);
    /* 0x0041B401 imul edx,ebp */ edx = (edx * ebp) & U32MASK;
    /* 0x0041B404 mov ebp,dword_43F50C */ ebp = (unsigned long)M32(0x43F50CUL);
    /* 0x0041B40A add eax,edx */ eax = (eax + edx) & U32MASK;
    /* 0x0041B40C mov edx,dword_43F520 */ edx = (unsigned long)G32(0x43F520UL);
    /* 0x0041B412 imul edx,ebp */ edx = (edx * ebp) & U32MASK;
    /* 0x0041B415 add edx,eax */ edx = (edx + eax) & U32MASK;
    /* 0x0041B417 mov eax,edx */ eax = edx;
    /* 0x0041B419 sar edx,1Fh */ edx = (unsigned long)(((long)edx >> 31)) & U32MASK;
    /* 0x0041B41C shl edx,4 */ cf = (edx >> 28) & 1UL; edx = (edx << 4) & U32MASK;
    /* 0x0041B41F sbb eax,edx */ eax = (eax - edx - cf) & U32MASK;
    /* 0x0041B421 sar eax,4 */ eax = (unsigned long)(((long)eax >> 4)) & U32MASK;
    /* 0x0041B424 mov dword_43DCD8,eax */ M32(0x43DCD8UL) = (unsigned int)eax;
    /* 0x0041B429 mov eax,dword_43DACC[ecx*4] */ eax = (unsigned long)M32((0x43DACCUL + ecx * 4UL) & U32MASK);
    /* 0x0041B430 mov edx,dword_43D8B8 */ edx = (unsigned long)G32(0x43D8B8UL);
    /* 0x0041B436 mov dword_43DCDC,eax */ M32(0x43DCDCUL) = (unsigned int)eax;
    /* 0x0041B43B mov eax,dword_4441C0 */ eax = (unsigned long)G32(0x4441C0UL);
    /* 0x0041B440 sub eax,edx */ eax = (eax - edx) & U32MASK;
    /* 0x0041B442 and eax,7Fh */ eax &= 0x7FUL;
    /* 0x0041B445 mov eax,dword_43DACC[eax*4] */ eax = (unsigned long)M32((0x43DACCUL + eax * 4UL) & U32MASK);
    /* 0x0041B44C mov dword_43DCE0,eax */ M32(0x43DCE0UL) = (unsigned int)eax;
    /* 0x0041B451 mov eax,dword_43D8B0 */ eax = (unsigned long)G32(0x43D8B0UL);
    /* 0x0041B456 mov ebx,offset dword_43F4FC */ ebx = 0x43F4FCUL;
    /* 0x0041B45B mov al,byte ptr dword_43DCCC[eax*4] */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((0x43DCCCUL + eax * 4UL) & U32MASK);
    /* 0x0041B462 mov ecx,dword_43D8B0 */ ecx = (unsigned long)G32(0x43D8B0UL);
    /* 0x0041B468 mov byte_444E9D,al */ G8(0x444E9DUL) = (unsigned char)(eax & 0xFFUL);
    /* 0x0041B46D cmp ecx,4 */ /* 0x0041B470 jb loc_41B47F */
    if (ecx < 4UL) goto l_41b47f;
    /* 0x0041B472 mov dl,byte ptr dword_43D8BC */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8(0x43D8BCUL);
    /* 0x0041B478 add al,dl */ eax = (eax & 0xFFFFFF00UL) | (((eax & 0xFFUL) + (edx & 0xFFUL)) & 0xFFUL);
    /* 0x0041B47A mov byte_444E9D,al */ G8(0x444E9DUL) = (unsigned char)(eax & 0xFFUL);
l_41b47f:
    /* 0x0041B47F cmp byte_444EA9,5 */ /* 0x0041B486 jnz loc_41B49E */
    if (G8(0x444EA9UL) != 5) goto l_41b49e;
    /* 0x0041B488 mov dl,byte_444E9D */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)G8(0x444E9DUL);
    /* 0x0041B48E add dl,80h */ edx = (edx & 0xFFFFFF00UL) | (((edx & 0xFFUL) + 0x80UL) & 0xFFUL);
    /* 0x0041B491 mov ecx,2E3h */ ecx = 0x2E3UL;
    /* 0x0041B496 mov byte_444E9D,dl */ G8(0x444E9DUL) = (unsigned char)(edx & 0xFFUL);
    /* 0x0041B49C jmp loc_41B4A3 */ goto l_41b4a3;
l_41b49e:
    /* 0x0041B49E mov ecx,2E7h */ ecx = 0x2E7UL;
l_41b4a3:
    /* 0x0041B4A3 cmp dword ptr [ebx+70h],0 */ /* 0x0041B4A7 jnz loc_41B4AF */
    if (M32((ebx + 0x70UL) & U32MASK) != 0) goto l_41b4af;
    /* 0x0041B4A9 inc ecx */ ecx = (ecx + 1UL) & U32MASK;
    /* 0x0041B4AA jmp loc_41B5E1 */ goto l_41b5e1;
l_41b4af:
    /* 0x0041B4AF cmp dword ptr [ebx+74h],0 */ /* 0x0041B4B3 jnz loc_41B4BD */
    if (M32((ebx + 0x74UL) & U32MASK) != 0) goto l_41b4bd;
    /* 0x0041B4B5 add ecx,2 */ ecx = (ecx + 2UL) & U32MASK;
    /* 0x0041B4B8 jmp loc_41B5E1 */ goto l_41b5e1;
l_41b4bd:
    /* 0x0041B4BD cmp dword ptr [ebx+78h],0 */ /* 0x0041B4C1 jnz loc_41B5E1 */
    if (M32((ebx + 0x78UL) & U32MASK) != 0) goto l_41b5e1;
    /* 0x0041B4C7 add ecx,3 */ ecx = (ecx + 3UL) & U32MASK;
    /* 0x0041B4CA jmp loc_41B5E1 */ goto l_41b5e1;
l_41b4cf:
    /* 0x0041B4CF mov ebx,edi */ ebx = edi;
    /* 0x0041B4D1 shl ebx,5 */ ebx = (ebx << 5) & U32MASK;
    /* 0x0041B4D4 sub ebx,edi */ ebx = (ebx - edi) & U32MASK;
    /* 0x0041B4D6 shl ebx,2 */ ebx = (ebx << 2) & U32MASK;
    /* 0x0041B4D9 add ebx,offset dword_43F4FC */ ebx = (ebx + 0x43F4FCUL) & U32MASK;
    /* 0x0041B4DF mov edx,[ebx+8] */ edx = (unsigned long)M32((ebx + 8UL) & U32MASK);
    /* 0x0041B4E2 mov [ebx+0Ch],edx */ M32((ebx + 0xCUL) & U32MASK) = (unsigned int)edx;
    /* 0x0041B4E5 mov edx,[ebx+4] */ edx = (unsigned long)M32((ebx + 4UL) & U32MASK);
    /* 0x0041B4E8 mov [ebx+8],edx */ M32((ebx + 8UL) & U32MASK) = (unsigned int)edx;
    /* 0x0041B4EB mov ecx,[ebx] */ ecx = (unsigned long)M32(ebx & U32MASK);
    /* 0x0041B4ED mov edx,[ebx+2Ch] */ edx = (unsigned long)M32((ebx + 0x2CUL) & U32MASK);
    /* 0x0041B4F0 sub edx,ecx */ edx = (edx - ecx) & U32MASK;
    /* 0x0041B4F2 mov [ebx+4],edx */ M32((ebx + 4UL) & U32MASK) = (unsigned int)edx;
    /* 0x0041B4F5 mov eax,[ebx+2Ch] */ eax = (unsigned long)M32((ebx + 0x2CUL) & U32MASK);
    /* 0x0041B4F8 mov edx,[ebx+14h] */ edx = (unsigned long)M32((ebx + 0x14UL) & U32MASK);
    /* 0x0041B4FB imul edx,eax */ edx = (edx * eax) & U32MASK;
    /* 0x0041B4FE mov [ebx],eax */ M32(ebx & U32MASK) = (unsigned int)eax;
    /* 0x0041B500 mov eax,[ebx+28h] */ eax = (unsigned long)M32((ebx + 0x28UL) & U32MASK);
    /* 0x0041B503 shl eax,4 */ eax = (eax << 4) & U32MASK;
    /* 0x0041B506 mov ecx,[ebx+4] */ ecx = (unsigned long)M32((ebx + 4UL) & U32MASK);
    /* 0x0041B509 add eax,edx */ eax = (eax + edx) & U32MASK;
    /* 0x0041B50B mov edx,[ebx+18h] */ edx = (unsigned long)M32((ebx + 0x18UL) & U32MASK);
    /* 0x0041B50E imul edx,ecx */ edx = (edx * ecx) & U32MASK;
    /* 0x0041B511 mov ebp,[ebx+8] */ ebp = (unsigned long)M32((ebx + 8UL) & U32MASK);
    /* 0x0041B514 add eax,edx */ eax = (eax + edx) & U32MASK;
    /* 0x0041B516 mov edx,[ebx+1Ch] */ edx = (unsigned long)M32((ebx + 0x1CUL) & U32MASK);
    /* 0x0041B519 imul edx,ebp */ edx = (edx * ebp) & U32MASK;
    /* 0x0041B51C mov ecx,[ebx+0Ch] */ ecx = (unsigned long)M32((ebx + 0xCUL) & U32MASK);
    /* 0x0041B51F add eax,edx */ eax = (eax + edx) & U32MASK;
    /* 0x0041B521 mov edx,[ebx+20h] */ edx = (unsigned long)M32((ebx + 0x20UL) & U32MASK);
    /* 0x0041B524 imul edx,ecx */ edx = (edx * ecx) & U32MASK;
    /* 0x0041B527 mov ebp,dword_43D8C4 */ ebp = (unsigned long)G32(0x43D8C4UL);
    /* 0x0041B52D add eax,edx */ eax = (eax + edx) & U32MASK;
    /* 0x0041B52F mov edx,[ebx+24h] */ edx = (unsigned long)M32((ebx + 0x24UL) & U32MASK);
    /* 0x0041B532 imul edx,ebp */ edx = (edx * ebp) & U32MASK;
    /* 0x0041B535 add edx,eax */ edx = (edx + eax) & U32MASK;
    /* 0x0041B537 mov eax,edx */ eax = edx;
    /* 0x0041B539 sar edx,1Fh */ edx = (unsigned long)(((long)edx >> 31)) & U32MASK;
    /* 0x0041B53C shl edx,4 */ cf = (edx >> 28) & 1UL; edx = (edx << 4) & U32MASK;
    /* 0x0041B53F sbb eax,edx */ eax = (eax - edx - cf) & U32MASK;
    /* 0x0041B541 sar eax,4 */ eax = (unsigned long)(((long)eax >> 4)) & U32MASK;
    /* 0x0041B544 mov dword_43DCCC,eax */ M32(0x43DCCCUL) = (unsigned int)eax;
    /* 0x0041B549 mov edx,[ebx+5Ch] */ edx = (unsigned long)M32((ebx + 0x5CUL) & U32MASK);
    /* 0x0041B54C cmp edx,[ebx+60h] */ /* 0x0041B54F jbe loc_41B556 */
    if (edx <= (unsigned long)M32((ebx + 0x60UL) & U32MASK)) goto l_41b556;
    /* 0x0041B551 mov dl,[ebx+28h] */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8((ebx + 0x28UL) & U32MASK);
    /* 0x0041B554 jmp loc_41B55C */ goto l_41b55c;
l_41b556:
    /* 0x0041B556 mov dl,byte ptr dword_43DCCC */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8(0x43DCCCUL);
l_41b55c:
    /* 0x0041B55C mov byte_444E9D,dl */ G8(0x444E9DUL) = (unsigned char)(edx & 0xFFUL);
    /* 0x0041B562 mov dh,byte_444EA9 */ edx = (edx & 0xFFFF00FFUL) | ((((unsigned long)G8(0x444EA9UL)) & 0xFFUL) << 8);
    /* 0x0041B568 cmp dh,7 */ /* 0x0041B56B jnz loc_41B5A4 */
    if (((edx >> 8) & 0xFFUL) != 7UL) goto l_41b5a4;
    /* 0x0041B56D mov ch,byte_444E9D */ ecx = (ecx & 0xFFFF00FFUL) | ((((unsigned long)G8(0x444E9DUL)) & 0xFFUL) << 8);
    /* 0x0041B573 add ch,80h */ ecx = (ecx & 0xFFFF00FFUL) | (((((ecx >> 8) & 0xFFUL) + 0x80UL) & 0xFFUL) << 8);
    /* 0x0041B576 mov ebp,[ebx+70h] */ ebp = (unsigned long)M32((ebx + 0x70UL) & U32MASK);
    /* 0x0041B579 mov byte_444E9D,ch */ G8(0x444E9DUL) = (unsigned char)((ecx >> 8) & 0xFFUL);
    /* 0x0041B57F lea ecx,[edi+2EBh] */ ecx = (edi + 0x2EBUL) & U32MASK;
    /* 0x0041B585 test ebp,ebp */ /* 0x0041B587 jnz loc_41B58E */
    if (ebp != 0UL) goto l_41b58e;
    /* 0x0041B589 add ecx,2 */ ecx = (ecx + 2UL) & U32MASK;
    /* 0x0041B58C jmp loc_41B5D5 */ goto l_41b5d5;
l_41b58e:
    /* 0x0041B58E cmp dword ptr [ebx+74h],0 */ /* 0x0041B592 jnz loc_41B599 */
    if (M32((ebx + 0x74UL) & U32MASK) != 0) goto l_41b599;
    /* 0x0041B594 add ecx,4 */ ecx = (ecx + 4UL) & U32MASK;
    /* 0x0041B597 jmp loc_41B5D5 */ goto l_41b5d5;
l_41b599:
    /* 0x0041B599 cmp dword ptr [ebx+78h],0 */ /* 0x0041B59D jnz loc_41B5D5 */
    if (M32((ebx + 0x78UL) & U32MASK) != 0) goto l_41b5d5;
    /* 0x0041B59F add ecx,6 */ ecx = (ecx + 6UL) & U32MASK;
    /* 0x0041B5A2 jmp loc_41B5D5 */ goto l_41b5d5;
l_41b5a4:
    /* 0x0041B5A4 cmp dh,8 */ /* 0x0041B5A7 jnz loc_41B5C0 */
    if (((edx >> 8) & 0xFFUL) != 8UL) goto l_41b5c0;
    /* 0x0041B5A9 cmp dword ptr [ebx+70h],0 */ /* 0x0041B5AD jz loc_41B5B3 */
    if (M32((ebx + 0x70UL) & U32MASK) == 0) goto l_41b5b3;
    /* 0x0041B5AF mov ecx,edi */ ecx = edi;
    /* 0x0041B5B1 jmp loc_41B5B8 */ goto l_41b5b8;
l_41b5b3:
    /* 0x0041B5B3 mov ecx,2 */ ecx = 2UL;
l_41b5b8:
    /* 0x0041B5B8 add ecx,2F3h */ ecx = (ecx + 0x2F3UL) & U32MASK;
    /* 0x0041B5BE jmp loc_41B5D5 */ goto l_41b5d5;
l_41b5c0:
    /* 0x0041B5C0 cmp dword ptr [ebx+70h],0 */ /* 0x0041B5C4 jz loc_41B5CA */
    if (M32((ebx + 0x70UL) & U32MASK) == 0) goto l_41b5ca;
    /* 0x0041B5C6 mov ecx,edi */ ecx = edi;
    /* 0x0041B5C8 jmp loc_41B5CF */ goto l_41b5cf;
l_41b5ca:
    /* 0x0041B5CA mov ecx,4 */ ecx = 4UL;
l_41b5cf:
    /* 0x0041B5CF add ecx,2F6h */ ecx = (ecx + 0x2F6UL) & U32MASK;
l_41b5d5:
    /* 0x0041B5D5 mov dl,byte_43F6EC[edi] */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8((0x43F6ECUL + edi) & U32MASK);
    /* 0x0041B5DB mov byte_444EA8,dl */ G8(0x444EA8UL) = (unsigned char)(edx & 0xFFUL);
l_41b5e1:
    /* 0x0041B5E1 cmp esi,dword_4441B8 */ /* 0x0041B5E7 jnb loc_41B632 */
    if (esi >= (unsigned long)G32(0x4441B8UL)) goto l_41b632;
    /* 0x0041B5E9 mov edx,dword_4441BC */ edx = (unsigned long)G32(0x4441BCUL);
    /* 0x0041B5EF lea eax,ds:0[esi*8] */ eax = (esi * 8UL) & U32MASK;
    /* 0x0041B5F6 add edx,eax */ edx = (edx + eax) & U32MASK;
    /* 0x0041B5F8 mov bl,byte_444EA8 */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(0x444EA8UL);
    /* 0x0041B5FE mov [edx],ecx */ M32(edx & U32MASK) = (unsigned int)ecx;
    /* 0x0041B600 mov [edx+4],bl */ M8((edx + 4UL) & U32MASK) = (unsigned char)(ebx & 0xFFUL);
    /* 0x0041B603 mov bl,byte_444E9D */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(0x444E9DUL);
    /* 0x0041B609 inc esi */ esi = (esi + 1UL) & U32MASK;
    /* 0x0041B60A mov [edx+5],bl */ M8((edx + 5UL) & U32MASK) = (unsigned char)(ebx & 0xFFUL);
    /* 0x0041B60D jmp loc_41B632 */ goto l_41b632;
l_41b60f:
    /* 0x0041B60F lea edx,ds:0[esi*8] */ edx = (esi * 8UL) & U32MASK;
    /* 0x0041B616 mov eax,dword_4441BC */ eax = (unsigned long)G32(0x4441BCUL);
    /* 0x0041B61B inc esi */ esi = (esi + 1UL) & U32MASK;
    /* 0x0041B61C mov ecx,[edx+eax] */ ecx = (unsigned long)M32((edx + eax) & U32MASK);
    /* 0x0041B61F mov bl,[edx+eax+4] */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)M8((edx + eax + 4UL) & U32MASK);
    /* 0x0041B623 mov al,[edx+eax+5] */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((edx + eax + 5UL) & U32MASK);
    /* 0x0041B627 mov byte_444EA8,bl */ G8(0x444EA8UL) = (unsigned char)(ebx & 0xFFUL);
    /* 0x0041B62D mov byte_444E9D,al */ G8(0x444E9DUL) = (unsigned char)(eax & 0xFFUL);
l_41b632:
    /* 0x0041B632 mov dword_4441D0,edi */ G32(0x4441D0UL) = (unsigned int)edi;
    /* 0x0041B638 mov dword_4441B0,esi */ G32(0x4441B0UL) = (unsigned int)esi;
    /* 0x0041B63E mov dword_4441C4,ecx */ G32(0x4441C4UL) = (unsigned int)ecx;
    /* pops + 0x0041B64A retn: no C equivalent */
}
/* sub_41B69C (0x0041B69C-0x0041BEF5, ~699 asm lines). Entry al/dl live,
 * void return (see header). Frame (all zero-init): var_44=v44,
 * var_40=v40, var_3C=v3c, var_38=v38, var_34=v34, var_30=v30,
 * var_2C=v2c, var_28=v28, var_24=v24, var_20=v20, var_1C(byte)v18slot,
 * var_18(byte)=v18. */
void d3u_41b69c(dec3w_ctx *c, unsigned int al_in, unsigned int dl_in)
{
    unsigned long eax = 0;
    unsigned long ebx = 0;
    unsigned long ecx = 0;
    unsigned long edx = 0;
    unsigned long esi = 0;
    unsigned long edi = 0;
    unsigned long ebp = 0;
    unsigned long v44 = 0;
    unsigned long v40 = 0;
    unsigned long v3c = 0;
    unsigned long v38 = 0;
    unsigned long v34 = 0;
    unsigned long v30 = 0;
    unsigned long v2c = 0;
    unsigned long v28 = 0;
    unsigned long v24 = 0;
    unsigned long v20 = 0;
    unsigned long v1c = 0;
    unsigned long v18 = 0;
    /* prologue pushes + 0x0041B6A1 sub esp,30h: no C equivalent */
    /* 0x0041B6A4 mov bl,al */ ebx = (ebx & 0xFFFFFF00UL) | ((unsigned long)(al_in & 0xFFUL));
    /* 0x0041B6A6 mov [var_18],dl */ v18 = (unsigned long)(dl_in & 0xFFUL);
    /* 0x0041B6AA cmp byte_444EAB,0 */ /* 0x0041B6B1 jnz loc_41B6C4 */
    if (G8(0x444EABUL) != 0) goto l_41b6c4;
    /* 0x0041B6B3 mov eax,dword_4441B0 */ eax = (unsigned long)G32(0x4441B0UL);
    /* 0x0041B6B8 cmp eax,dword_4441B8 */ /* 0x0041B6BE jb loc_41BEED */
    if (eax < (unsigned long)G32(0x4441B8UL)) goto l_41beed;
l_41b6c4:
    /* 0x0041B6C4 mov dl,byte_444EA9 */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)G8(0x444EA9UL);
    /* 0x0041B6CA cmp dl,7 */ /* 0x0041B6CD jnb loc_41BBA8 */
    if ((edx & 0xFFUL) >= 7UL) goto l_41bba8;
    /* 0x0041B6D3 mov ecx,offset dword_43F4FC */ ecx = 0x43F4FCUL;
    /* 0x0041B6D8 cmp dl,6 */ /* 0x0041B6DB jnz loc_41B6E2 */
    if ((edx & 0xFFUL) != 6UL) goto l_41b6e2;
    /* 0x0041B6DD movsx eax,bl */ eax = (unsigned long)(((long)(signed char)(ebx & 0xFFUL))) & U32MASK;
    /* 0x0041B6E0 jmp loc_41B6EB */ goto l_41b6eb;
l_41b6e2:
    /* 0x0041B6E2 xor eax,eax */ eax = 0UL;
    /* 0x0041B6E4 mov al,bl */ eax = (eax & 0xFFFFFF00UL) | (ebx & 0xFFUL);
    /* 0x0041B6E6 sub eax,80h */ eax = (eax - 0x80UL) & U32MASK;
l_41b6eb:
    /* 0x0041B6EB mov [var_2C],eax */ v2c = eax;
    /* 0x0041B6EF mov eax,[var_2C] */ eax = v2c;
    /* 0x0041B6F3 mov ebp,dword_43D8BC */ ebp = (unsigned long)G32(0x43D8BCUL);
    /* 0x0041B6F9 sub eax,ebp */ eax = (eax - ebp) & U32MASK;
    /* 0x0041B6FB mov [var_20],eax */ v20 = eax;
    /* 0x0041B6FF mov al,byte ptr dword_43DCD8 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8(0x43DCD8UL);
    /* 0x0041B704 sub al,byte ptr [var_2C] */ eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFUL) - (v2c & 0xFFUL)) & 0xFFUL));
    /* 0x0041B708 movsx eax,al */ eax = (unsigned long)(((long)(signed char)(eax & 0xFFUL))) & U32MASK;
    /* 0x0041B70B shl eax,4 */ eax = (eax << 4) & U32MASK;
    /* 0x0041B70E mov edx,eax */ edx = eax;
    /* 0x0041B710 and edx,0FFFh */ edx &= 0xFFFUL;
    /* 0x0041B716 mov ebx,[ecx+30h] */ ebx = (unsigned long)M32((ecx + 0x30UL) & U32MASK);
    /* 0x0041B719 mov edx,dword_4398B0[edx*4] */ edx = (unsigned long)M32((0x4398B0UL + edx * 4UL) & U32MASK);
    /* 0x0041B720 mov esi,[ecx] */ esi = (unsigned long)M32(ecx & U32MASK);
    /* 0x0041B722 add ebx,edx */ ebx = (ebx + edx) & U32MASK;
    /* 0x0041B724 mov edx,eax */ edx = eax;
    /* 0x0041B726 sub edx,esi */ edx = (edx - esi) & U32MASK;
    /* 0x0041B728 mov [ecx+30h],ebx */ M32((ecx + 0x30UL) & U32MASK) = (unsigned int)ebx;
    /* 0x0041B72B and edx,0FFFh */ edx &= 0xFFFUL;
    /* 0x0041B731 mov edi,[ecx+34h] */ edi = (unsigned long)M32((ecx + 0x34UL) & U32MASK);
    /* 0x0041B734 add edi,dword_4398B0[edx*4] */ edi = (edi + (unsigned long)M32((0x4398B0UL + edx * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041B73B mov edx,[ecx] */ edx = (unsigned long)M32(ecx & U32MASK);
    /* 0x0041B73D add edx,eax */ edx = (edx + eax) & U32MASK;
    /* 0x0041B73F mov [ecx+34h],edi */ M32((ecx + 0x34UL) & U32MASK) = (unsigned int)edi;
    /* 0x0041B742 and edx,0FFFh */ edx &= 0xFFFUL;
    /* 0x0041B748 mov ebp,[ecx+38h] */ ebp = (unsigned long)M32((ecx + 0x38UL) & U32MASK);
    /* 0x0041B74B mov edx,dword_4398B0[edx*4] */ edx = (unsigned long)M32((0x4398B0UL + edx * 4UL) & U32MASK);
    /* 0x0041B752 mov ebx,[ecx+4] */ ebx = (unsigned long)M32((ecx + 4UL) & U32MASK);
    /* 0x0041B755 add ebp,edx */ ebp = (ebp + edx) & U32MASK;
    /* 0x0041B757 mov edx,eax */ edx = eax;
    /* 0x0041B759 sub edx,ebx */ edx = (edx - ebx) & U32MASK;
    /* 0x0041B75B mov [ecx+38h],ebp */ M32((ecx + 0x38UL) & U32MASK) = (unsigned int)ebp;
    /* 0x0041B75E and edx,0FFFh */ edx &= 0xFFFUL;
    /* 0x0041B764 mov esi,[ecx+3Ch] */ esi = (unsigned long)M32((ecx + 0x3CUL) & U32MASK);
    /* 0x0041B767 add esi,dword_4398B0[edx*4] */ esi = (esi + (unsigned long)M32((0x4398B0UL + edx * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041B76E mov edx,[ecx+4] */ edx = (unsigned long)M32((ecx + 4UL) & U32MASK);
    /* 0x0041B771 add edx,eax */ edx = (edx + eax) & U32MASK;
    /* 0x0041B773 mov [ecx+3Ch],esi */ M32((ecx + 0x3CUL) & U32MASK) = (unsigned int)esi;
    /* 0x0041B776 and edx,0FFFh */ edx &= 0xFFFUL;
    /* 0x0041B77C mov edi,[ecx+40h] */ edi = (unsigned long)M32((ecx + 0x40UL) & U32MASK);
    /* 0x0041B77F mov edx,dword_4398B0[edx*4] */ edx = (unsigned long)M32((0x4398B0UL + edx * 4UL) & U32MASK);
    /* 0x0041B786 mov ebp,[ecx+8] */ ebp = (unsigned long)M32((ecx + 8UL) & U32MASK);
    /* 0x0041B789 add edi,edx */ edi = (edi + edx) & U32MASK;
    /* 0x0041B78B mov edx,eax */ edx = eax;
    /* 0x0041B78D sub edx,ebp */ edx = (edx - ebp) & U32MASK;
    /* 0x0041B78F mov [ecx+40h],edi */ M32((ecx + 0x40UL) & U32MASK) = (unsigned int)edi;
    /* 0x0041B792 and edx,0FFFh */ edx &= 0xFFFUL;
    /* 0x0041B798 mov ebx,[ecx+44h] */ ebx = (unsigned long)M32((ecx + 0x44UL) & U32MASK);
    /* 0x0041B79B add ebx,dword_4398B0[edx*4] */ ebx = (ebx + (unsigned long)M32((0x4398B0UL + edx * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041B7A2 mov edx,[ecx+8] */ edx = (unsigned long)M32((ecx + 8UL) & U32MASK);
    /* 0x0041B7A5 add edx,eax */ edx = (edx + eax) & U32MASK;
    /* 0x0041B7A7 mov [ecx+44h],ebx */ M32((ecx + 0x44UL) & U32MASK) = (unsigned int)ebx;
    /* 0x0041B7AA and edx,0FFFh */ edx &= 0xFFFUL;
    /* 0x0041B7B0 mov esi,[ecx+48h] */ esi = (unsigned long)M32((ecx + 0x48UL) & U32MASK);
    /* 0x0041B7B3 mov edx,dword_4398B0[edx*4] */ edx = (unsigned long)M32((0x4398B0UL + edx * 4UL) & U32MASK);
    /* 0x0041B7BA mov edi,[ecx+0Ch] */ edi = (unsigned long)M32((ecx + 0xCUL) & U32MASK);
    /* 0x0041B7BD add esi,edx */ esi = (esi + edx) & U32MASK;
    /* 0x0041B7BF mov edx,eax */ edx = eax;
    /* 0x0041B7C1 sub edx,edi */ edx = (edx - edi) & U32MASK;
    /* 0x0041B7C3 mov [ecx+48h],esi */ M32((ecx + 0x48UL) & U32MASK) = (unsigned int)esi;
    /* 0x0041B7C6 and edx,0FFFh */ edx &= 0xFFFUL;
    /* 0x0041B7CC mov ebp,[ecx+4Ch] */ ebp = (unsigned long)M32((ecx + 0x4CUL) & U32MASK);
    /* 0x0041B7CF add ebp,dword_4398B0[edx*4] */ ebp = (ebp + (unsigned long)M32((0x4398B0UL + edx * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041B7D6 mov edx,[ecx+0Ch] */ edx = (unsigned long)M32((ecx + 0xCUL) & U32MASK);
    /* 0x0041B7D9 add edx,eax */ edx = (edx + eax) & U32MASK;
    /* 0x0041B7DB mov [ecx+4Ch],ebp */ M32((ecx + 0x4CUL) & U32MASK) = (unsigned int)ebp;
    /* 0x0041B7DE and edx,0FFFh */ edx &= 0xFFFUL;
    /* 0x0041B7E4 mov ebx,[ecx+50h] */ ebx = (unsigned long)M32((ecx + 0x50UL) & U32MASK);
    /* 0x0041B7E7 mov edx,dword_4398B0[edx*4] */ edx = (unsigned long)M32((0x4398B0UL + edx * 4UL) & U32MASK);
    /* 0x0041B7EE mov esi,[ecx+10h] */ esi = (unsigned long)M32((ecx + 0x10UL) & U32MASK);
    /* 0x0041B7F1 add ebx,edx */ ebx = (ebx + edx) & U32MASK;
    /* 0x0041B7F3 mov edx,eax */ edx = eax;
    /* 0x0041B7F5 sub edx,esi */ edx = (edx - esi) & U32MASK;
    /* 0x0041B7F7 mov [ecx+50h],ebx */ M32((ecx + 0x50UL) & U32MASK) = (unsigned int)ebx;
    /* 0x0041B7FA and edx,0FFFh */ edx &= 0xFFFUL;
    /* 0x0041B800 mov edi,[ecx+54h] */ edi = (unsigned long)M32((ecx + 0x54UL) & U32MASK);
    /* 0x0041B803 mov edx,dword_4398B0[edx*4] */ edx = (unsigned long)M32((0x4398B0UL + edx * 4UL) & U32MASK);
    /* 0x0041B80A mov ebp,[ecx+10h] */ ebp = (unsigned long)M32((ecx + 0x10UL) & U32MASK);
    /* 0x0041B80D add edi,edx */ edi = (edi + edx) & U32MASK;
    /* 0x0041B80F add eax,ebp */ eax = (eax + ebp) & U32MASK;
    /* 0x0041B811 mov [ecx+54h],edi */ M32((ecx + 0x54UL) & U32MASK) = (unsigned int)edi;
    /* 0x0041B814 and eax,0FFFh */ eax &= 0xFFFUL;
    /* 0x0041B819 mov edx,[ecx+58h] */ edx = (unsigned long)M32((ecx + 0x58UL) & U32MASK);
    /* 0x0041B81C add edx,dword_4398B0[eax*4] */ edx = (edx + (unsigned long)M32((0x4398B0UL + eax * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041B823 mov ebx,[ecx+6Ch] */ ebx = (unsigned long)M32((ecx + 0x6CUL) & U32MASK);
    /* 0x0041B826 mov [ecx+58h],edx */ M32((ecx + 0x58UL) & U32MASK) = (unsigned int)edx;
    /* 0x0041B829 mov al,byte ptr [var_2C] */ eax = (eax & 0xFFFFFF00UL) | (v2c & 0xFFUL);
    /* 0x0041B82D mov dl,[ecx+28h] */ edx = (edx & 0xFFFFFF00UL) | (unsigned long)M8((ecx + 0x28UL) & U32MASK);
    /* 0x0041B830 inc ebx */ ebx = (ebx + 1UL) & U32MASK;
    /* 0x0041B831 sub al,dl */ eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFUL) - (edx & 0xFFUL)) & 0xFFUL));
    /* 0x0041B833 mov [ecx+6Ch],ebx */ M32((ecx + 0x6CUL) & U32MASK) = (unsigned int)ebx;
    /* 0x0041B836 movsx eax,al */ eax = (unsigned long)(((long)(signed char)(eax & 0xFFUL))) & U32MASK;
    /* 0x0041B839 mov [ecx+2Ch],eax */ M32((ecx + 0x2CUL) & U32MASK) = (unsigned int)eax;
    /* 0x0041B83C mov eax,[var_2C] */ eax = v2c;
    /* 0x0041B840 mov dh,[ecx+6Ch] */ edx = (edx & 0xFFFF00FFUL) | ((((unsigned long)M8((ecx + 0x6CUL) & U32MASK)) & 0xFFUL) << 8);
    /* 0x0041B843 mov [ecx+28h],eax */ M32((ecx + 0x28UL) & U32MASK) = (unsigned int)eax;
    /* 0x0041B846 test dh,0Fh */ /* 0x0041B849 jnz loc_41B939 */
    if ((((edx >> 8) & 0xFFUL) & 0x0FUL) != 0) goto l_41b939;
    /* 0x0041B84F mov ebx,1 */ ebx = 1UL;
    /* 0x0041B854 lea eax,[ecx+4] */ eax = (ecx + 4UL) & U32MASK;
    /* 0x0041B857 xor esi,esi */ esi = 0UL;
    /* 0x0041B859 mov edx,[ecx+30h] */ edx = (unsigned long)M32((ecx + 0x30UL) & U32MASK);
    /* 0x0041B85C mov [ecx+30h],esi */ M32((ecx + 0x30UL) & U32MASK) = (unsigned int)esi;
l_41b85f:
    /* 0x0041B85F mov edi,[eax+30h] */ edi = (unsigned long)M32((eax + 0x30UL) & U32MASK);
    /* 0x0041B862 cmp edx,edi */ /* 0x0041B864 jbe loc_41B86A */
    if (edx <= edi) goto l_41b86a;
    /* 0x0041B866 mov edx,edi */ edx = edi;
    /* 0x0041B868 mov esi,ebx */ esi = ebx;
l_41b86a:
    /* 0x0041B86A add eax,4 */ eax = (eax + 4UL) & U32MASK;
    /* 0x0041B86D inc ebx */ ebx = (ebx + 1UL) & U32MASK;
    /* 0x0041B86E mov dword ptr [eax+2Ch],0 */ M32((eax + 0x2CUL) & U32MASK) = 0;
    /* 0x0041B875 cmp ebx,0Bh */ /* 0x0041B878 jl loc_41B85F */
    if (((long)ebx) < 11) goto l_41b85f;
    /* 0x0041B87A dec esi */ esi = (esi - 1UL) & U32MASK;
    /* 0x0041B87B cmp esi,9 */ /* 0x0041B87E ja loc_41B939 */
    if (esi > 9UL) goto l_41b939;
    /* 0x0041B884 jmp cs:off_41B64C[esi*4]: switch */
    switch (esi) {
    case 0:
        /* 0x0041B88C loc_41B88C */
        /* 0x0041B88C mov ebp,[ecx+14h] */ ebp = (unsigned long)M32((ecx + 0x14UL) & U32MASK);
        /* 0x0041B88F cmp ebp,0FFFFFFE0h */ /* 0x0041B892 jle loc_41B939 */
        if (((long)ebp) <= -32) goto l_41b939;
        /* 0x0041B898 lea eax,[ebp-1] */ eax = (ebp - 1UL) & U32MASK;
        /* 0x0041B89B mov [ecx+14h],eax */ M32((ecx + 0x14UL) & U32MASK) = (unsigned int)eax;
        /* 0x0041B89E jmp loc_41B939 */ goto l_41b939;
    case 1:
        /* 0x0041B8A3 loc_41B8A3 */
        /* 0x0041B8A3 mov esi,[ecx+14h] */ esi = (unsigned long)M32((ecx + 0x14UL) & U32MASK);
        /* 0x0041B8A6 cmp esi,20h */ /* 0x0041B8A9 jge loc_41B939 */
        if (((long)esi) >= 32) goto l_41b939;
        /* 0x0041B8AF lea edi,[esi+1] */ edi = (esi + 1UL) & U32MASK;
        /* 0x0041B8B2 mov [ecx+14h],edi */ M32((ecx + 0x14UL) & U32MASK) = (unsigned int)edi;
        /* 0x0041B8B5 jmp loc_41B939 */ goto l_41b939;
    case 2:
        /* 0x0041B8BA loc_41B8BA */
        /* 0x0041B8BA mov edx,[ecx+18h] */ edx = (unsigned long)M32((ecx + 0x18UL) & U32MASK);
        /* 0x0041B8BD cmp edx,0FFFFFFE0h */ /* 0x0041B8C0 jle loc_41B939 */
        if (((long)edx) <= -32) goto l_41b939;
        /* 0x0041B8C6 lea ebx,[edx-1] */ ebx = (edx - 1UL) & U32MASK;
        /* 0x0041B8C9 mov [ecx+18h],ebx */ M32((ecx + 0x18UL) & U32MASK) = (unsigned int)ebx;
        /* 0x0041B8CC jmp loc_41B939 */ goto l_41b939;
    case 3:
        /* 0x0041B8D1 loc_41B8D1 */
        /* 0x0041B8D1 mov ebp,[ecx+18h] */ ebp = (unsigned long)M32((ecx + 0x18UL) & U32MASK);
        /* 0x0041B8D4 cmp ebp,20h */ /* 0x0041B8D7 jge loc_41B939 */
        if (((long)ebp) >= 32) goto l_41b939;
        /* 0x0041B8DD lea eax,[ebp+1] */ eax = (ebp + 1UL) & U32MASK;
        /* 0x0041B8E0 mov [ecx+18h],eax */ M32((ecx + 0x18UL) & U32MASK) = (unsigned int)eax;
        /* 0x0041B8E3 jmp loc_41B939 */ goto l_41b939;
    case 4:
        /* 0x0041B8E5 loc_41B8E5 */
        /* 0x0041B8E5 mov esi,[ecx+1Ch] */ esi = (unsigned long)M32((ecx + 0x1CUL) & U32MASK);
        /* 0x0041B8E8 cmp esi,0FFFFFFE0h */ /* 0x0041B8EB jle loc_41B939 */
        if (((long)esi) <= -32) goto l_41b939;
        /* 0x0041B8ED lea edi,[esi-1] */ edi = (esi - 1UL) & U32MASK;
        /* 0x0041B8F0 mov [ecx+1Ch],edi */ M32((ecx + 0x1CUL) & U32MASK) = (unsigned int)edi;
        /* 0x0041B8F3 jmp loc_41B939 */ goto l_41b939;
    case 5:
        /* 0x0041B8F5 loc_41B8F5 */
        /* 0x0041B8F5 mov edx,[ecx+1Ch] */ edx = (unsigned long)M32((ecx + 0x1CUL) & U32MASK);
        /* 0x0041B8F8 cmp edx,20h */ /* 0x0041B8FB jge loc_41B939 */
        if (((long)edx) >= 32) goto l_41b939;
        /* 0x0041B8FD lea ebx,[edx+1] */ ebx = (edx + 1UL) & U32MASK;
        /* 0x0041B900 mov [ecx+1Ch],ebx */ M32((ecx + 0x1CUL) & U32MASK) = (unsigned int)ebx;
        /* 0x0041B903 jmp loc_41B939 */ goto l_41b939;
    case 6:
        /* 0x0041B905 loc_41B905 */
        /* 0x0041B905 mov ebp,[ecx+20h] */ ebp = (unsigned long)M32((ecx + 0x20UL) & U32MASK);
        /* 0x0041B908 cmp ebp,0FFFFFFE0h */ /* 0x0041B90B jle loc_41B939 */
        if (((long)ebp) <= -32) goto l_41b939;
        /* 0x0041B90D lea eax,[ebp-1] */ eax = (ebp - 1UL) & U32MASK;
        /* 0x0041B910 mov [ecx+20h],eax */ M32((ecx + 0x20UL) & U32MASK) = (unsigned int)eax;
        /* 0x0041B913 jmp loc_41B939 */ goto l_41b939;
    case 7:
        /* 0x0041B915 loc_41B915 */
        /* 0x0041B915 mov esi,[ecx+20h] */ esi = (unsigned long)M32((ecx + 0x20UL) & U32MASK);
        /* 0x0041B918 cmp esi,20h */ /* 0x0041B91B jge loc_41B939 */
        if (((long)esi) >= 32) goto l_41b939;
        /* 0x0041B91D lea edi,[esi+1] */ edi = (esi + 1UL) & U32MASK;
        /* 0x0041B920 mov [ecx+20h],edi */ M32((ecx + 0x20UL) & U32MASK) = (unsigned int)edi;
        /* 0x0041B923 jmp loc_41B939 */ goto l_41b939;
    case 8:
        /* 0x0041B925 loc_41B925 */
        /* 0x0041B925 cmp dword ptr [ecx+20h],0FFFFFFE0h */ /* 0x0041B929 jle loc_41B939 */
        if (((long)M32((ecx + 0x20UL) & U32MASK)) <= -32) goto l_41b939;
        /* 0x0041B92B dec dword ptr [ecx+24h] */ M32((ecx + 0x24UL) & U32MASK) = (unsigned int)(((unsigned long)M32((ecx + 0x24UL) & U32MASK) - 1UL) & U32MASK);
        /* 0x0041B92E jmp loc_41B939 */ goto l_41b939;
    case 9:
        /* 0x0041B930 loc_41B930 */
        /* 0x0041B930 cmp dword ptr [ecx+20h],20h */ /* 0x0041B934 jge loc_41B939 */
        if (((long)M32((ecx + 0x20UL) & U32MASK)) >= 32) goto l_41b939;
        /* 0x0041B936 inc dword ptr [ecx+24h] */ M32((ecx + 0x24UL) & U32MASK) = (unsigned int)(((unsigned long)M32((ecx + 0x24UL) & U32MASK) + 1UL) & U32MASK);
        /* fallthrough to default */
    default:
        goto l_41b939;
    }
l_41b939:
    /* 0x0041B939 mov eax,dword_4441C0 */ eax = (unsigned long)G32(0x4441C0UL);
    /* 0x0041B93E and eax,7Fh */ eax &= 0x7FUL;
    /* 0x0041B941 mov edx,[var_20] */ edx = v20;
    /* 0x0041B945 mov dword_43DACC[eax*4],edx */ M32((0x43DACCUL + eax * 4UL) & U32MASK) = (unsigned int)edx;
    /* 0x0041B94C mov eax,dword_43D8B0 */ eax = (unsigned long)G32(0x43D8B0UL);
    /* 0x0041B951 mov edx,dword_4441C0 */ edx = (unsigned long)G32(0x4441C0UL);
    /* 0x0041B957 test dword_42A054[eax*4],edx */ /* 0x0041B95E jnz loc_41BA81 */
    if ((((unsigned long)M32((0x42A054UL + eax * 4UL) & U32MASK)) & edx) != 0) goto l_41ba81;
    /* 0x0041B964 mov edi,0FFFFh */ edi = 0xFFFFUL;
    /* 0x0041B969 mov eax,2 */ eax = 2UL;
    /* 0x0041B96E mov edx,8 */ edx = 8UL;
    /* 0x0041B973 mov dword_43D8B4,eax */ M32(0x43D8B4UL) = (unsigned int)eax;
    /* 0x0041B978 mov dword_43D8B8,eax */ M32(0x43D8B8UL) = (unsigned int)eax;
l_41b97d:
    /* 0x0041B97D mov ebx,dword_4441C0 */ ebx = (unsigned long)G32(0x4441C0UL);
    /* 0x0041B983 lea esi,[ebx-1] */ esi = (ebx - 1UL) & U32MASK;
    /* 0x0041B986 mov ebp,esi */ ebp = esi;
    /* 0x0041B988 sub ebp,eax */ ebp = (ebp - eax) & U32MASK;
    /* 0x0041B98A and esi,7Fh */ esi &= 0x7FUL;
    /* 0x0041B98D and ebp,7Fh */ ebp &= 0x7FUL;
    /* 0x0041B990 mov esi,dword_43DACC[esi*4] */ esi = (unsigned long)M32((0x43DACCUL + esi * 4UL) & U32MASK);
    /* 0x0041B997 shl ebp,2 */ ebp = (ebp << 2) & U32MASK;
    /* 0x0041B99A mov [var_1C],esi */ v1c = esi;
    /* 0x0041B99E mov esi,ss:dword_43DACC[ebp] */ esi = (unsigned long)M32((0x43DACCUL + ebp) & U32MASK);
    /* 0x0041B9A4 mov ebp,[var_1C] */ ebp = v1c;
    /* 0x0041B9A8 sub ebp,esi */ ebp = (ebp - esi) & U32MASK;
    /* 0x0041B9AA mov esi,ebp */ esi = ebp;
    /* 0x0041B9AC and esi,0FFFh */ esi &= 0xFFFUL;
    /* 0x0041B9B2 shl esi,2 */ esi = (esi << 2) & U32MASK;
    /* 0x0041B9B5 mov [var_28],esi */ v28 = esi;
    /* 0x0041B9B9 mov esi,ebx */ esi = ebx;
    /* 0x0041B9BB sub esi,eax */ esi = (esi - eax) & U32MASK;
    /* 0x0041B9BD and esi,7Fh */ esi &= 0x7FUL;
    /* 0x0041B9C0 shl esi,2 */ esi = (esi << 2) & U32MASK;
    /* 0x0041B9C3 mov [var_24],esi */ v24 = esi;
    /* 0x0041B9C7 mov esi,ebx */ esi = ebx;
    /* 0x0041B9C9 and esi,7Fh */ esi &= 0x7FUL;
    /* 0x0041B9CC mov esi,dword_43DACC[esi*4] */ esi = (unsigned long)M32((0x43DACCUL + esi * 4UL) & U32MASK);
    /* 0x0041B9D3 mov [var_1C],esi */ v1c = esi;
    /* 0x0041B9D7 mov esi,[var_24] */ esi = v24;
    /* 0x0041B9DB mov ebp,[var_1C] */ ebp = v1c;
    /* 0x0041B9DF mov esi,dword_43DACC[esi] */ esi = (unsigned long)M32((0x43DACCUL + esi) & U32MASK);
    /* 0x0041B9E5 sub ebp,esi */ ebp = (ebp - esi) & U32MASK;
    /* 0x0041B9E7 mov esi,ebp */ esi = ebp;
    /* 0x0041B9E9 and esi,0FFFh */ esi &= 0xFFFUL;
    /* 0x0041B9EF mov esi,dword_4398B0[esi*4] */ esi = (unsigned long)M32((0x4398B0UL + esi * 4UL) & U32MASK);
    /* 0x0041B9F6 mov [var_1C],esi */ v1c = esi;
    /* 0x0041B9FA mov esi,[var_28] */ esi = v28;
    /* 0x0041B9FE mov ebp,[var_1C] */ ebp = v1c;
    /* 0x0041BA02 mov esi,dword_4398B0[esi] */ esi = (unsigned long)M32((0x4398B0UL + esi) & U32MASK);
    /* 0x0041BA08 sub ebx,2 */ ebx = (ebx - 2UL) & U32MASK;
    /* 0x0041BA0B add ebp,esi */ ebp = (ebp + esi) & U32MASK;
    /* 0x0041BA0D mov esi,ebx */ esi = ebx;
    /* 0x0041BA0F sub esi,eax */ esi = (esi - eax) & U32MASK;
    /* 0x0041BA11 and ebx,7Fh */ ebx &= 0x7FUL;
    /* 0x0041BA14 and esi,7Fh */ esi &= 0x7FUL;
    /* 0x0041BA17 mov [var_1C],ebp */ v1c = ebp;
    /* 0x0041BA1B mov ebx,dword_43DACC[ebx*4] */ ebx = (unsigned long)M32((0x43DACCUL + ebx * 4UL) & U32MASK);
    /* 0x0041BA22 sub ebx,dword_43DACC[esi*4] */ ebx = (ebx - (unsigned long)M32((0x43DACCUL + esi * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041BA29 and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x0041BA2F mov esi,[var_1C] */ esi = v1c;
    /* 0x0041BA33 mov ebp,dword_4398B0[ebx*4] */ ebp = (unsigned long)M32((0x4398B0UL + ebx * 4UL) & U32MASK);
    /* 0x0041BA3A lea ebx,[esi+ebp] */ ebx = (esi + ebp) & U32MASK;
    /* 0x0041BA3D imul esi,dword_43D8CC[edx],0Dh */ esi = (((unsigned long)M32((0x43D8CCUL + edx) & U32MASK) * 0xDUL)) & U32MASK;
    /* 0x0041BA44 shr esi,4 */ esi = (esi >> 4) & U32MASK;
    /* 0x0041BA47 add esi,ebx */ esi = (esi + ebx) & U32MASK;
    /* 0x0041BA49 mov dword_43D8CC[edx],esi */ M32((0x43D8CCUL + edx) & U32MASK) = (unsigned int)esi;
    /* 0x0041BA4F mov ebp,dword_43D8CC[edx] */ ebp = (unsigned long)M32((0x43D8CCUL + edx) & U32MASK);
    /* 0x0041BA55 mov esi,dword_43D8B4 */ esi = (unsigned long)G32(0x43D8B4UL);
    /* 0x0041BA5B cmp ebp,dword_43D8CC[esi*4] */ /* 0x0041BA62 jnb loc_41BA69 */
    if (ebp >= (unsigned long)M32((0x43D8CCUL + esi * 4UL) & U32MASK)) goto l_41ba69;
    /* 0x0041BA64 mov dword_43D8B4,eax */ M32(0x43D8B4UL) = (unsigned int)eax;
l_41ba69:
    /* 0x0041BA69 cmp ebx,edi */ /* 0x0041BA6B jnb loc_41BA74 */
    if (ebx >= edi) goto l_41ba74;
    /* 0x0041BA6D mov edi,ebx */ edi = ebx;
    /* 0x0041BA6F mov dword_43D8B8,eax */ M32(0x43D8B8UL) = (unsigned int)eax;
l_41ba74:
    /* 0x0041BA74 inc eax */ eax = (eax + 1UL) & U32MASK;
    /* 0x0041BA75 add edx,4 */ edx = (edx + 4UL) & U32MASK;
    /* 0x0041BA78 cmp eax,7Ch */ /* 0x0041BA7B jb loc_41B97D */
    if (eax < 0x7CUL) goto l_41b97d;
l_41ba81:
    /* 0x0041BA81 mov eax,[var_2C] */ eax = v2c;
    /* 0x0041BA85 sub eax,dword_43DCCC */ eax = (eax - (unsigned long)M32(0x43DCCCUL)) & U32MASK;
    /* 0x0041BA8B mov ebx,dword_43DCD0 */ ebx = (unsigned long)M32(0x43DCD0UL);
    /* 0x0041BA91 mov [var_44],eax */ v44 = eax;
    /* 0x0041BA94 mov eax,[var_2C] */ eax = v2c;
    /* 0x0041BA98 sub eax,ebx */ eax = (eax - ebx) & U32MASK;
    /* 0x0041BA9A mov esi,dword_43DCD4 */ esi = (unsigned long)M32(0x43DCD4UL);
    /* 0x0041BAA0 mov [var_40],eax */ v40 = eax;
    /* 0x0041BAA4 mov eax,[var_2C] */ eax = v2c;
    /* 0x0041BAA8 sub eax,esi */ eax = (eax - esi) & U32MASK;
    /* 0x0041BAAA mov edi,dword_43DCD8 */ edi = (unsigned long)M32(0x43DCD8UL);
    /* 0x0041BAB0 mov [var_3C],eax */ v3c = eax;
    /* 0x0041BAB4 mov eax,[var_2C] */ eax = v2c;
    /* 0x0041BAB8 sub eax,edi */ eax = (eax - edi) & U32MASK;
    /* 0x0041BABA mov ebp,dword_43DCDC */ ebp = (unsigned long)M32(0x43DCDCUL);
    /* 0x0041BAC0 mov [var_38],eax */ v38 = eax;
    /* 0x0041BAC4 mov eax,[var_20] */ eax = v20;
    /* 0x0041BAC8 mov edx,dword_43DCE0 */ edx = (unsigned long)M32(0x43DCE0UL);
    /* 0x0041BACE sub eax,ebp */ eax = (eax - ebp) & U32MASK;
    /* 0x0041BAD0 xor ebx,ebx */ ebx = 0UL;
    /* 0x0041BAD2 mov [var_34],eax */ v34 = eax;
    /* 0x0041BAD6 mov eax,[var_20] */ eax = v20;
    /* 0x0041BADA mov dword_43D8B0,ebx */ M32(0x43D8B0UL) = (unsigned int)ebx;
    /* 0x0041BAE0 sub eax,edx */ eax = (eax - edx) & U32MASK;
    /* 0x0041BAE2 xor esi,esi */ esi = 0UL;
    /* 0x0041BAE4 mov [var_30],eax */ v30 = eax;
    /* 0x0041BAE8 xor edx,edx */ edx = 0UL;
    /* 0x0041BAEA mov eax,offset dword_43DCE4 */ eax = 0x43DCE4UL;
l_41baef:
    /* 0x0041BAEF mov ebx,dword_4441C0 */ ebx = (unsigned long)G32(0x4441C0UL);
    /* 0x0041BAF5 and ebx,0FFh */ ebx &= 0xFFUL;
    /* 0x0041BAFB mov ebp,[eax+400h] */ ebp = (unsigned long)M32((eax + 0x400UL) & U32MASK);
    /* 0x0041BB01 mov edi,[eax+ebx*4] */ edi = (unsigned long)M32((eax + ebx * 4UL) & U32MASK);
    /* 0x0041BB04 sub ebp,edi */ ebp = (ebp - edi) & U32MASK;
    /* 0x0041BB06 mov [eax+400h],ebp */ M32((eax + 0x400UL) & U32MASK) = (unsigned int)ebp;
    /* 0x0041BB0C mov edi,[esp+esi+var_44] */ edi = (esi == 0UL) ? v44 : (esi == 4UL) ? v40 : (esi == 8UL) ? v3c : (esi == 12UL) ? v38 : (esi == 16UL) ? v34 : v30;
    /* 0x0041BB0F and edi,1FFh */ edi &= 0x1FFUL;
    /* 0x0041BB15 mov edi,dword_42A06C[edi*4] */ edi = (unsigned long)M32((0x42A06CUL + edi * 4UL) & U32MASK);
    /* 0x0041BB1C mov [eax+ebx*4],edi */ M32((eax + ebx * 4UL) & U32MASK) = (unsigned int)edi;
    /* 0x0041BB1F mov ebx,edi */ ebx = edi;
    /* 0x0041BB21 add [eax+400h],ebx */ M32((eax + 0x400UL) & U32MASK) = (unsigned int)(((unsigned long)M32((eax + 0x400UL) & U32MASK) + ebx) & U32MASK);
    /* 0x0041BB27 imul edi,dword_43D8B0,404h */ edi = (((unsigned long)M32(0x43D8B0UL) * 0x404UL)) & U32MASK;
    /* 0x0041BB31 mov ebx,[eax+400h] */ ebx = (unsigned long)M32((eax + 0x400UL) & U32MASK);
    /* 0x0041BB37 cmp ebx,dword_43E0E4[edi] */ /* 0x0041BB3D jge loc_41BB45 */
    if (ebx >= (unsigned long)M32((0x43E0E4UL + edi) & U32MASK)) goto l_41bb45;
    /* 0x0041BB3F mov dword_43D8B0,edx */ M32(0x43D8B0UL) = (unsigned int)edx;
l_41bb45:
    /* 0x0041BB45 add esi,4 */ esi = (esi + 4UL) & U32MASK;
    /* 0x0041BB48 inc edx */ edx = (edx + 1UL) & U32MASK;
    /* 0x0041BB49 add eax,404h */ eax = (eax + 0x404UL) & U32MASK;
    /* 0x0041BB4E cmp edx,6 */ /* 0x0041BB51 jl loc_41BAEF */
    if (((long)edx) < 6) goto l_41baef;
    /* 0x0041BB53 mov eax,dword_43D8BC */ eax = (unsigned long)G32(0x43D8BCUL);
    /* 0x0041BB58 mov dword_43D8C0,eax */ M32(0x43D8C0UL) = (unsigned int)eax;
    /* 0x0041BB5D mov eax,[var_2C] */ eax = v2c;
    /* 0x0041BB61 xor edx,edx */ edx = 0UL;
    /* 0x0041BB63 mov dword_43D8BC,eax */ M32(0x43D8BCUL) = (unsigned int)eax;
    /* 0x0041BB68 mov eax,[ecx+70h] */ eax = (unsigned long)M32((ecx + 0x70UL) & U32MASK);
    /* 0x0041BB6B mov dl,[var_18] */ edx = (edx & 0xFFFFFF00UL) | (v18 & 0xFFUL);
    /* 0x0041BB6F shl eax,8 */ eax = (eax << 8) & U32MASK;
    /* 0x0041BB72 add eax,edx */ eax = (eax + edx) & U32MASK;
    /* 0x0041BB74 mov [ecx+70h],eax */ M32((ecx + 0x70UL) & U32MASK) = (unsigned int)eax;
    /* 0x0041BB77 xor eax,eax */ eax = 0UL;
    /* 0x0041BB79 mov al,dl */ eax = (eax & 0xFFFFFF00UL) | (edx & 0xFFUL);
    /* 0x0041BB7B mov edx,[ecx+74h] */ edx = (unsigned long)M32((ecx + 0x74UL) & U32MASK);
    /* 0x0041BB7E mov ebx,dword_42A86C[eax*4] */ ebx = (unsigned long)M32((0x42A86CUL + (eax & 0xFFUL) * 4UL) & U32MASK);
    /* 0x0041BB85 shl edx,8 */ edx = (edx << 8) & U32MASK;
    /* 0x0041BB88 add edx,ebx */ edx = (edx + ebx) & U32MASK;
    /* 0x0041BB8A mov [ecx+74h],edx */ M32((ecx + 0x74UL) & U32MASK) = (unsigned int)edx;
    /* 0x0041BB8D mov edx,[ecx+78h] */ edx = (unsigned long)M32((ecx + 0x78UL) & U32MASK);
    /* 0x0041BB90 mov eax,dword_42B06C[eax*4] */ eax = (unsigned long)M32((0x42B06CUL + (eax & 0xFFUL) * 4UL) & U32MASK);
    /* 0x0041BB97 shl edx,8 */ edx = (edx << 8) & U32MASK;
    /* 0x0041BB9A add edx,eax */ edx = (edx + eax) & U32MASK;
    /* 0x0041BB9C mov [ecx+78h],edx */ M32((ecx + 0x78UL) & U32MASK) = (unsigned int)edx;
    /* 0x0041BB9F add esp,30h + pops + 0x0041BBA7 retn (<7 early return): no C equivalent */
    return;
l_41bba8:
    /* 0x0041BBA8 mov edx,dword_4441D0 */ edx = (unsigned long)G32(0x4441D0UL);
    /* 0x0041BBAE mov eax,edx */ eax = edx;
    /* 0x0041BBB0 shl eax,5 */ eax = (eax << 5) & U32MASK;
    /* 0x0041BBB3 sub eax,edx */ eax = (eax - edx) & U32MASK;
    /* 0x0041BBB5 mov edx,offset dword_43F4FC */ edx = 0x43F4FCUL;
    /* 0x0041BBBA shl eax,2 */ eax = (eax << 2) & U32MASK;
    /* 0x0041BBBD mov ecx,dword_4441D0 */ ecx = (unsigned long)G32(0x4441D0UL);
    /* 0x0041BBC3 add edx,eax */ edx = (edx + eax) & U32MASK;
    /* 0x0041BBC5 mov al,byte_444EA8 */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)G8(0x444EA8UL);
    /* 0x0041BBCA mov bh,byte_444EA9 */ ebx = (ebx & 0xFFFF00FFUL) | ((((unsigned long)G8(0x444EA9UL)) & 0xFFUL) << 8);
    /* 0x0041BBD0 mov byte_43F6EC[ecx],al */ M8((0x43F6ECUL + ecx) & U32MASK) = (unsigned char)(eax & 0xFFUL);
    /* 0x0041BBD6 cmp bh,8 */ /* 0x0041BBD9 jb loc_41BBE0 */
    if (((ebx >> 8) & 0xFFUL) < 8UL) goto l_41bbe0;
    /* 0x0041BBDB movsx eax,bl */ eax = (unsigned long)(((long)(signed char)(ebx & 0xFFUL))) & U32MASK;
    /* 0x0041BBDE jmp loc_41BBE9 */ goto l_41bbe9;
l_41bbe0:
    /* 0x0041BBE0 xor eax,eax */ eax = 0UL;
    /* 0x0041BBE2 mov al,bl */ eax = (eax & 0xFFFFFF00UL) | (ebx & 0xFFUL);
    /* 0x0041BBE4 sub eax,80h */ eax = (eax - 0x80UL) & U32MASK;
l_41bbe9:
    /* 0x0041BBE9 mov [var_2C],eax */ v2c = eax;
    /* 0x0041BBED mov cl,byte ptr [var_2C] */ ecx = (ecx & 0xFFFFFF00UL) | (v2c & 0xFFUL);
    /* 0x0041BBF1 mov al,byte ptr dword_43DCCC */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8(0x43DCCCUL);
    /* 0x0041BBF6 sub al,cl */ eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFUL) - (ecx & 0xFFUL)) & 0xFFUL));
    /* 0x0041BBF8 movsx eax,al */ eax = (unsigned long)(((long)(signed char)(eax & 0xFFUL))) & U32MASK;
    /* 0x0041BBFB shl eax,4 */ eax = (eax << 4) & U32MASK;
    /* 0x0041BBFE mov ebx,eax */ ebx = eax;
    /* 0x0041BC00 and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x0041BC06 mov ecx,[edx+30h] */ ecx = (unsigned long)M32((edx + 0x30UL) & U32MASK);
    /* 0x0041BC09 mov ebx,dword_4398B0[ebx*4] */ ebx = (unsigned long)M32((0x4398B0UL + ebx * 4UL) & U32MASK);
    /* 0x0041BC10 mov esi,[edx] */ esi = (unsigned long)M32(edx & U32MASK);
    /* 0x0041BC12 add ecx,ebx */ ecx = (ecx + ebx) & U32MASK;
    /* 0x0041BC14 mov ebx,eax */ ebx = eax;
    /* 0x0041BC16 sub ebx,esi */ ebx = (ebx - esi) & U32MASK;
    /* 0x0041BC18 mov [edx+30h],ecx */ M32((edx + 0x30UL) & U32MASK) = (unsigned int)ecx;
    /* 0x0041BC1B and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x0041BC21 mov edi,[edx+34h] */ edi = (unsigned long)M32((edx + 0x34UL) & U32MASK);
    /* 0x0041BC24 add edi,dword_4398B0[ebx*4] */ edi = (edi + (unsigned long)M32((0x4398B0UL + ebx * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041BC2B mov ebx,[edx] */ ebx = (unsigned long)M32(edx & U32MASK);
    /* 0x0041BC2D add ebx,eax */ ebx = (ebx + eax) & U32MASK;
    /* 0x0041BC2F mov [edx+34h],edi */ M32((edx + 0x34UL) & U32MASK) = (unsigned int)edi;
    /* 0x0041BC32 and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x0041BC38 mov ebp,[edx+38h] */ ebp = (unsigned long)M32((edx + 0x38UL) & U32MASK);
    /* 0x0041BC3B mov ebx,dword_4398B0[ebx*4] */ ebx = (unsigned long)M32((0x4398B0UL + ebx * 4UL) & U32MASK);
    /* 0x0041BC42 mov ecx,[edx+4] */ ecx = (unsigned long)M32((edx + 4UL) & U32MASK);
    /* 0x0041BC45 add ebp,ebx */ ebp = (ebp + ebx) & U32MASK;
    /* 0x0041BC47 mov ebx,eax */ ebx = eax;
    /* 0x0041BC49 sub ebx,ecx */ ebx = (ebx - ecx) & U32MASK;
    /* 0x0041BC4B mov [edx+38h],ebp */ M32((edx + 0x38UL) & U32MASK) = (unsigned int)ebp;
    /* 0x0041BC4E and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x0041BC54 mov esi,[edx+3Ch] */ esi = (unsigned long)M32((edx + 0x3CUL) & U32MASK);
    /* 0x0041BC57 add esi,dword_4398B0[ebx*4] */ esi = (esi + (unsigned long)M32((0x4398B0UL + ebx * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041BC5E mov ebx,[edx+4] */ ebx = (unsigned long)M32((edx + 4UL) & U32MASK);
    /* 0x0041BC61 add ebx,eax */ ebx = (ebx + eax) & U32MASK;
    /* 0x0041BC63 mov [edx+3Ch],esi */ M32((edx + 0x3CUL) & U32MASK) = (unsigned int)esi;
    /* 0x0041BC66 and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x0041BC6C mov edi,[edx+40h] */ edi = (unsigned long)M32((edx + 0x40UL) & U32MASK);
    /* 0x0041BC6F mov ebx,dword_4398B0[ebx*4] */ ebx = (unsigned long)M32((0x4398B0UL + ebx * 4UL) & U32MASK);
    /* 0x0041BC76 mov ebp,[edx+8] */ ebp = (unsigned long)M32((edx + 8UL) & U32MASK);
    /* 0x0041BC79 add edi,ebx */ edi = (edi + ebx) & U32MASK;
    /* 0x0041BC7B mov ebx,eax */ ebx = eax;
    /* 0x0041BC7D sub ebx,ebp */ ebx = (ebx - ebp) & U32MASK;
    /* 0x0041BC7F mov [edx+40h],edi */ M32((edx + 0x40UL) & U32MASK) = (unsigned int)edi;
    /* 0x0041BC82 and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x0041BC88 mov ecx,[edx+44h] */ ecx = (unsigned long)M32((edx + 0x44UL) & U32MASK);
    /* 0x0041BC8B add ecx,dword_4398B0[ebx*4] */ ecx = (ecx + (unsigned long)M32((0x4398B0UL + ebx * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041BC92 mov ebx,[edx+8] */ ebx = (unsigned long)M32((edx + 8UL) & U32MASK);
    /* 0x0041BC95 add ebx,eax */ ebx = (ebx + eax) & U32MASK;
    /* 0x0041BC97 mov [edx+44h],ecx */ M32((edx + 0x44UL) & U32MASK) = (unsigned int)ecx;
    /* 0x0041BC9A and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x0041BCA0 mov esi,[edx+48h] */ esi = (unsigned long)M32((edx + 0x48UL) & U32MASK);
    /* 0x0041BCA3 mov ebx,dword_4398B0[ebx*4] */ ebx = (unsigned long)M32((0x4398B0UL + ebx * 4UL) & U32MASK);
    /* 0x0041BCAA mov edi,[edx+0Ch] */ edi = (unsigned long)M32((edx + 0xCUL) & U32MASK);
    /* 0x0041BCAD add esi,ebx */ esi = (esi + ebx) & U32MASK;
    /* 0x0041BCAF mov ebx,eax */ ebx = eax;
    /* 0x0041BCB1 sub ebx,edi */ ebx = (ebx - edi) & U32MASK;
    /* 0x0041BCB3 mov [edx+48h],esi */ M32((edx + 0x48UL) & U32MASK) = (unsigned int)esi;
    /* 0x0041BCB6 and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x0041BCBC mov ebp,[edx+4Ch] */ ebp = (unsigned long)M32((edx + 0x4CUL) & U32MASK);
    /* 0x0041BCBF add ebp,dword_4398B0[ebx*4] */ ebp = (ebp + (unsigned long)M32((0x4398B0UL + ebx * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041BCC6 mov ebx,[edx+0Ch] */ ebx = (unsigned long)M32((edx + 0xCUL) & U32MASK);
    /* 0x0041BCC9 add ebx,eax */ ebx = (ebx + eax) & U32MASK;
    /* 0x0041BCCB mov [edx+4Ch],ebp */ M32((edx + 0x4CUL) & U32MASK) = (unsigned int)ebp;
    /* 0x0041BCCE and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x0041BCD4 mov ecx,[edx+50h] */ ecx = (unsigned long)M32((edx + 0x50UL) & U32MASK);
    /* 0x0041BCD7 mov ebx,dword_4398B0[ebx*4] */ ebx = (unsigned long)M32((0x4398B0UL + ebx * 4UL) & U32MASK);
    /* 0x0041BCDE add ecx,ebx */ ecx = (ecx + ebx) & U32MASK;
    /* 0x0041BCE0 mov [edx+50h],ecx */ M32((edx + 0x50UL) & U32MASK) = (unsigned int)ecx;
    /* 0x0041BCE3 mov ebx,eax */ ebx = eax;
    /* 0x0041BCE5 mov esi,dword_43D8C4 */ esi = (unsigned long)G32(0x43D8C4UL);
    /* 0x0041BCEB sub ebx,esi */ ebx = (ebx - esi) & U32MASK;
    /* 0x0041BCED and ebx,0FFFh */ ebx &= 0xFFFUL;
    /* 0x0041BCF3 mov edi,[edx+54h] */ edi = (unsigned long)M32((edx + 0x54UL) & U32MASK);
    /* 0x0041BCF6 add edi,dword_4398B0[ebx*4] */ edi = (edi + (unsigned long)M32((0x4398B0UL + ebx * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041BCFD add eax,esi */ eax = (eax + esi) & U32MASK;
    /* 0x0041BCFF mov [edx+54h],edi */ M32((edx + 0x54UL) & U32MASK) = (unsigned int)edi;
    /* 0x0041BD02 and eax,0FFFh */ eax &= 0xFFFUL;
    /* 0x0041BD07 mov ebx,[edx+58h] */ ebx = (unsigned long)M32((edx + 0x58UL) & U32MASK);
    /* 0x0041BD0A mov eax,dword_4398B0[eax*4] */ eax = (unsigned long)M32((0x4398B0UL + eax * 4UL) & U32MASK);
    /* 0x0041BD11 add ebx,eax */ ebx = (ebx + eax) & U32MASK;
    /* 0x0041BD13 mov [edx+58h],ebx */ M32((edx + 0x58UL) & U32MASK) = (unsigned int)ebx;
    /* 0x0041BD16 mov ch,byte ptr [var_2C] */ ecx = (ecx & 0xFFFF00FFUL) | ((v2c & 0xFFUL) << 8);
    /* 0x0041BD1A mov al,byte ptr dword_43DCCC */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8(0x43DCCCUL);
    /* 0x0041BD1F sub al,ch */ eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFUL) - ((ecx >> 8) & 0xFFUL)) & 0xFFUL));
    /* 0x0041BD21 and eax,0FFh */ eax &= 0xFFUL;
    /* 0x0041BD26 mov ecx,[edx+5Ch] */ ecx = (unsigned long)M32((edx + 0x5CUL) & U32MASK);
    /* 0x0041BD29 add ecx,dword_42B86C[eax*4] */ ecx = (ecx + (unsigned long)M32((0x42B86CUL + eax * 4UL) & U32MASK)) & U32MASK;
    /* 0x0041BD30 mov al,[edx+28h] */ eax = (eax & 0xFFFFFF00UL) | (unsigned long)M8((edx + 0x28UL) & U32MASK);
    /* 0x0041BD33 mov ah,byte ptr [var_2C] */ eax = (eax & 0xFFFF00FFUL) | ((v2c & 0xFFUL) << 8);
    /* 0x0041BD37 mov [edx+5Ch],ecx */ M32((edx + 0x5CUL) & U32MASK) = (unsigned int)ecx;
    /* 0x0041BD3A sub al,ah */ eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFUL) - ((eax >> 8) & 0xFFUL)) & 0xFFUL));
    /* 0x0041BD3C and eax,0FFh */ eax &= 0xFFUL;
    /* 0x0041BD41 mov esi,[edx+60h] */ esi = (unsigned long)M32((edx + 0x60UL) & U32MASK);
    /* 0x0041BD44 mov eax,dword_42B86C[eax*4] */ eax = (unsigned long)M32((0x42B86CUL + eax * 4UL) & U32MASK);
    /* 0x0041BD4B mov bl,[edx+28h] */ ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)M8((edx + 0x28UL) & U32MASK);
    /* 0x0041BD4E add esi,eax */ esi = (esi + eax) & U32MASK;
    /* 0x0041BD50 mov al,byte ptr [var_2C] */ eax = (eax & 0xFFFFFF00UL) | (v2c & 0xFFUL);
    /* 0x0041BD54 sub al,bl */ eax = (eax & 0xFFFFFF00UL) | ((((eax & 0xFFUL) - (ebx & 0xFFUL)) & 0xFFUL));
    /* 0x0041BD56 mov [edx+60h],esi */ M32((edx + 0x60UL) & U32MASK) = (unsigned int)esi;
    /* 0x0041BD59 movsx eax,al */ eax = (unsigned long)(((long)(signed char)(eax & 0xFFUL))) & U32MASK;
    /* 0x0041BD5C mov edi,[edx+6Ch] */ edi = (unsigned long)M32((edx + 0x6CUL) & U32MASK);
    /* 0x0041BD5F mov [edx+2Ch],eax */ M32((edx + 0x2CUL) & U32MASK) = (unsigned int)eax;
    /* 0x0041BD62 inc edi */ edi = (edi + 1UL) & U32MASK;
    /* 0x0041BD63 mov dword_43D8C4,eax */ M32(0x43D8C4UL) = (unsigned int)eax;
    /* 0x0041BD68 mov [edx+6Ch],edi */ M32((edx + 0x6CUL) & U32MASK) = (unsigned int)edi;
    /* 0x0041BD6B mov eax,[var_2C] */ eax = v2c;
    /* 0x0041BD6F mov bh,[edx+6Ch] */ ebx = (ebx & 0xFFFF00FFUL) | ((((unsigned long)M8((edx + 0x6CUL) & U32MASK)) & 0xFFUL) << 8);
    /* 0x0041BD72 mov [edx+28h],eax */ M32((edx + 0x28UL) & U32MASK) = (unsigned int)eax;
    /* 0x0041BD75 test bh,0Fh */ /* 0x0041BD78 jnz loc_41BE8A */
    if ((((ebx >> 8) & 0xFFUL) & 0x0FUL) != 0) goto l_41be8a;
    /* 0x0041BD7E mov ebx,1 */ ebx = 1UL;
    /* 0x0041BD83 lea eax,[edx+4] */ eax = (edx + 4UL) & U32MASK;
    /* 0x0041BD86 xor esi,esi */ esi = 0UL;
    /* 0x0041BD88 mov ecx,[edx+30h] */ ecx = (unsigned long)M32((edx + 0x30UL) & U32MASK);
    /* 0x0041BD8B mov [edx+30h],esi */ M32((edx + 0x30UL) & U32MASK) = (unsigned int)esi;
l_41bd8e:
    /* 0x0041BD8E mov edi,[eax+30h] */ edi = (unsigned long)M32((eax + 0x30UL) & U32MASK);
    /* 0x0041BD91 cmp ecx,edi */ /* 0x0041BD93 jbe loc_41BD99 */
    if (ecx <= edi) goto l_41bd99;
    /* 0x0041BD95 mov ecx,edi */ ecx = edi;
    /* 0x0041BD97 mov esi,ebx */ esi = ebx;
l_41bd99:
    /* 0x0041BD99 add eax,4 */ eax = (eax + 4UL) & U32MASK;
    /* 0x0041BD9C inc ebx */ ebx = (ebx + 1UL) & U32MASK;
    /* 0x0041BD9D mov dword ptr [eax+2Ch],0 */ M32((eax + 0x2CUL) & U32MASK) = 0;
    /* 0x0041BDA4 cmp ebx,0Bh */ /* 0x0041BDA7 jl loc_41BD8E */
    if (((long)ebx) < 11) goto l_41bd8e;
    /* 0x0041BDA9 dec esi */ esi = (esi - 1UL) & U32MASK;
    /* 0x0041BDAA cmp esi,9 */ /* 0x0041BDAD ja loc_41BE68 */
    if (esi > 9UL) goto l_41be68;
    /* 0x0041BDB3 jmp cs:off_41B674[esi*4]: switch */
    switch (esi) {
    case 0:
        /* 0x0041BDBB loc_41BDBB */
        /* 0x0041BDBB mov ecx,[edx+14h] */ ecx = (unsigned long)M32((edx + 0x14UL) & U32MASK);
        /* 0x0041BDBE cmp ecx,0FFFFFFE0h */ /* 0x0041BDC1 jle loc_41BE68 */
        if (((long)ecx) <= -32) goto l_41be68;
        /* 0x0041BDC7 lea esi,[ecx-1] */ esi = (ecx - 1UL) & U32MASK;
        /* 0x0041BDCA mov [edx+14h],esi */ M32((edx + 0x14UL) & U32MASK) = (unsigned int)esi;
        /* 0x0041BDCD jmp loc_41BE68 */ goto l_41be68;
    case 1:
        /* 0x0041BDD2 loc_41BDD2 */
        /* 0x0041BDD2 mov eax,[edx+14h] */ eax = (unsigned long)M32((edx + 0x14UL) & U32MASK);
        /* 0x0041BDD5 cmp eax,20h */ /* 0x0041BDD8 jge loc_41BE68 */
        if (((long)eax) >= 32) goto l_41be68;
        /* 0x0041BDDE lea ebx,[eax+1] */ ebx = (eax + 1UL) & U32MASK;
        /* 0x0041BDE1 mov [edx+14h],ebx */ M32((edx + 0x14UL) & U32MASK) = (unsigned int)ebx;
        /* 0x0041BDE4 jmp loc_41BE68 */ goto l_41be68;
    case 2:
        /* 0x0041BDE9 loc_41BDE9 */
        /* 0x0041BDE9 mov edi,[edx+18h] */ edi = (unsigned long)M32((edx + 0x18UL) & U32MASK);
        /* 0x0041BDEC cmp edi,0FFFFFFE0h */ /* 0x0041BDEF jle loc_41BE68 */
        if (((long)edi) <= -32) goto l_41be68;
        /* 0x0041BDF5 lea ebp,[edi-1] */ ebp = (edi - 1UL) & U32MASK;
        /* 0x0041BDF8 mov [edx+18h],ebp */ M32((edx + 0x18UL) & U32MASK) = (unsigned int)ebp;
        /* 0x0041BDFB jmp loc_41BE68 */ goto l_41be68;
    case 3:
        /* 0x0041BE00 loc_41BE00 */
        /* 0x0041BE00 mov ecx,[edx+18h] */ ecx = (unsigned long)M32((edx + 0x18UL) & U32MASK);
        /* 0x0041BE03 cmp ecx,20h */ /* 0x0041BE06 jge loc_41BE68 */
        if (((long)ecx) >= 32) goto l_41be68;
        /* 0x0041BE0C lea esi,[ecx+1] */ esi = (ecx + 1UL) & U32MASK;
        /* 0x0041BE0F mov [edx+18h],esi */ M32((edx + 0x18UL) & U32MASK) = (unsigned int)esi;
        /* 0x0041BE12 jmp loc_41BE68 */ goto l_41be68;
    case 4:
        /* 0x0041BE14 loc_41BE14 */
        /* 0x0041BE14 mov eax,[edx+1Ch] */ eax = (unsigned long)M32((edx + 0x1CUL) & U32MASK);
        /* 0x0041BE17 cmp eax,0FFFFFFE0h */ /* 0x0041BE1A jle loc_41BE68 */
        if (((long)eax) <= -32) goto l_41be68;
        /* 0x0041BE1C lea ebx,[eax-1] */ ebx = (eax - 1UL) & U32MASK;
        /* 0x0041BE1F mov [edx+1Ch],ebx */ M32((edx + 0x1CUL) & U32MASK) = (unsigned int)ebx;
        /* 0x0041BE22 jmp loc_41BE68 */ goto l_41be68;
    case 5:
        /* 0x0041BE24 loc_41BE24 */
        /* 0x0041BE24 mov edi,[edx+1Ch] */ edi = (unsigned long)M32((edx + 0x1CUL) & U32MASK);
        /* 0x0041BE27 cmp edi,20h */ /* 0x0041BE2A jge loc_41BE68 */
        if (((long)edi) >= 32) goto l_41be68;
        /* 0x0041BE2C lea ebp,[edi+1] */ ebp = (edi + 1UL) & U32MASK;
        /* 0x0041BE2F mov [edx+1Ch],ebp */ M32((edx + 0x1CUL) & U32MASK) = (unsigned int)ebp;
        /* 0x0041BE32 jmp loc_41BE68 */ goto l_41be68;
    case 6:
        /* 0x0041BE34 loc_41BE34 */
        /* 0x0041BE34 mov ecx,[edx+20h] */ ecx = (unsigned long)M32((edx + 0x20UL) & U32MASK);
        /* 0x0041BE37 cmp ecx,0FFFFFFE0h */ /* 0x0041BE3A jle loc_41BE68 */
        if (((long)ecx) <= -32) goto l_41be68;
        /* 0x0041BE3C lea esi,[ecx-1] */ esi = (ecx - 1UL) & U32MASK;
        /* 0x0041BE3F mov [edx+20h],esi */ M32((edx + 0x20UL) & U32MASK) = (unsigned int)esi;
        /* 0x0041BE42 jmp loc_41BE68 */ goto l_41be68;
    case 7:
        /* 0x0041BE44 loc_41BE44 */
        /* 0x0041BE44 mov eax,[edx+20h] */ eax = (unsigned long)M32((edx + 0x20UL) & U32MASK);
        /* 0x0041BE47 cmp eax,20h */ /* 0x0041BE4A jge loc_41BE68 */
        if (((long)eax) >= 32) goto l_41be68;
        /* 0x0041BE4C lea ebx,[eax+1] */ ebx = (eax + 1UL) & U32MASK;
        /* 0x0041BE4F mov [edx+20h],ebx */ M32((edx + 0x20UL) & U32MASK) = (unsigned int)ebx;
        /* 0x0041BE52 jmp loc_41BE68 */ goto l_41be68;
    case 8:
        /* 0x0041BE54 loc_41BE54 */
        /* 0x0041BE54 cmp dword ptr [edx+20h],0FFFFFFE0h */ /* 0x0041BE58 jle loc_41BE68 */
        if (((long)M32((edx + 0x20UL) & U32MASK)) <= -32) goto l_41be68;
        /* 0x0041BE5A dec dword ptr [edx+24h] */ M32((edx + 0x24UL) & U32MASK) = (unsigned int)(((unsigned long)M32((edx + 0x24UL) & U32MASK) - 1UL) & U32MASK);
        /* 0x0041BE5D jmp loc_41BE68 */ goto l_41be68;
    case 9:
        /* 0x0041BE5F loc_41BE5F */
        /* 0x0041BE5F cmp dword ptr [edx+20h],20h */ /* 0x0041BE63 jge loc_41BE68 */
        if (((long)M32((edx + 0x20UL) & U32MASK)) >= 32) goto l_41be68;
        /* 0x0041BE65 inc dword ptr [edx+24h] */ M32((edx + 0x24UL) & U32MASK) = (unsigned int)(((unsigned long)M32((edx + 0x24UL) & U32MASK) + 1UL) & U32MASK);
        /* fallthrough to default */
    default:
        goto l_41be68;
    }
l_41be68:
    /* 0x0041BE68 test byte ptr [edx+6Ch],0FFh */ /* 0x0041BE6C jnz loc_41BE8A */
    if (M8((edx + 0x6CUL) & U32MASK) != 0) goto l_41be8a;
    /* 0x0041BE6E mov eax,[edx+64h] */ eax = (unsigned long)M32((edx + 0x64UL) & U32MASK);
    /* 0x0041BE71 mov edi,[edx+5Ch] */ edi = (unsigned long)M32((edx + 0x5CUL) & U32MASK);
    /* 0x0041BE74 sub edi,eax */ edi = (edi - eax) & U32MASK;
    /* 0x0041BE76 mov [edx+5Ch],edi */ M32((edx + 0x5CUL) & U32MASK) = (unsigned int)edi;
    /* 0x0041BE79 mov ebp,[edx+60h] */ ebp = (unsigned long)M32((edx + 0x60UL) & U32MASK);
    /* 0x0041BE7C mov eax,[edx+68h] */ eax = (unsigned long)M32((edx + 0x68UL) & U32MASK);
    /* 0x0041BE7F mov [edx+64h],edi */ M32((edx + 0x64UL) & U32MASK) = (unsigned int)edi;
    /* 0x0041BE82 sub ebp,eax */ ebp = (ebp - eax) & U32MASK;
    /* 0x0041BE84 mov [edx+60h],ebp */ M32((edx + 0x60UL) & U32MASK) = (unsigned int)ebp;
    /* 0x0041BE87 mov [edx+68h],ebp */ M32((edx + 0x68UL) & U32MASK) = (unsigned int)ebp;
l_41be8a:
    /* 0x0041BE8A cmp byte_444EA9,9 */ /* 0x0041BE91 jnz loc_41BEA9 */
    if (G8(0x444EA9UL) != 9) goto l_41bea9;
    /* 0x0041BE93 mov eax,dword_43D8C8 */ eax = (unsigned long)G32(0x43D8C8UL);
    /* 0x0041BE98 mov ebx,dword_43D8C4 */ ebx = (unsigned long)G32(0x43D8C4UL);
    /* 0x0041BE9E mov dword_43D8C4,eax */ M32(0x43D8C4UL) = (unsigned int)eax;
    /* 0x0041BEA3 mov dword_43D8C8,ebx */ M32(0x43D8C8UL) = (unsigned int)ebx;
l_41bea9:
    /* 0x0041BEA9 xor ebx,ebx */ ebx = 0UL;
    /* 0x0041BEAB mov eax,[edx+70h] */ eax = (unsigned long)M32((edx + 0x70UL) & U32MASK);
    /* 0x0041BEAE mov bl,[var_18] */ ebx = (ebx & 0xFFFFFF00UL) | (v18 & 0xFFUL);
    /* 0x0041BEB2 shl eax,8 */ eax = (eax << 8) & U32MASK;
    /* 0x0041BEB5 add eax,ebx */ eax = (eax + ebx) & U32MASK;
    /* 0x0041BEB7 mov cl,byte_444EA9 */ ecx = (ecx & 0xFFFFFF00UL) | (unsigned long)G8(0x444EA9UL);
    /* 0x0041BEBD mov [edx+70h],eax */ M32((edx + 0x70UL) & U32MASK) = (unsigned int)eax;
    /* 0x0041BEC0 cmp cl,7 */ /* 0x0041BEC3 jnz loc_41BEED */
    if ((ecx & 0xFFUL) != 7UL) goto l_41beed;
    /* 0x0041BEC5 xor eax,eax */ eax = 0UL;
    /* 0x0041BEC7 mov ecx,[edx+74h] */ ecx = (unsigned long)M32((edx + 0x74UL) & U32MASK);
    /* 0x0041BECA mov al,bl */ eax = (eax & 0xFFFFFF00UL) | (ebx & 0xFFUL);
    /* 0x0041BECC shl ecx,8 */ ecx = (ecx << 8) & U32MASK;
    /* 0x0041BECF mov ebx,dword_42AC6C[eax*4] */ ebx = (unsigned long)M32((0x42AC6CUL + (eax & 0xFFUL) * 4UL) & U32MASK);
    /* 0x0041BED6 add ecx,ebx */ ecx = (ecx + ebx) & U32MASK;
    /* 0x0041BED8 mov ebx,[edx+78h] */ ebx = (unsigned long)M32((edx + 0x78UL) & U32MASK);
    /* 0x0041BEDB mov [edx+74h],ecx */ M32((edx + 0x74UL) & U32MASK) = (unsigned int)ecx;
    /* 0x0041BEDE shl ebx,8 */ ebx = (ebx << 8) & U32MASK;
    /* 0x0041BEE1 mov eax,dword_42B46C[eax*4] */ eax = (unsigned long)M32((0x42B46CUL + (eax & 0xFFUL) * 4UL) & U32MASK);
    /* 0x0041BEE8 add ebx,eax */ ebx = (ebx + eax) & U32MASK;
    /* 0x0041BEEA mov [edx+78h],ebx */ M32((edx + 0x78UL) & U32MASK) = (unsigned int)ebx;
l_41beed:
    /* 0x0041BEED add esp,30h + pops + 0x0041BEF5 retn: no C equivalent */
    ;
}

/* ---- price walker (from draft_n3.c) ---- */
unsigned int d3u_41a9c4(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in)
{
unsigned long eax = 0;
unsigned long ebx = 0;
unsigned long ecx = 0;
unsigned long edx = 0;
unsigned long esi = 0;
unsigned long edi = 0;
unsigned long ebp = 0;
unsigned long v34 = 0;
unsigned long v58 = 0;
unsigned long v54 = 0;
unsigned long v50 = 0;
unsigned long v4C = 0;
unsigned long v48 = 0;
unsigned long v44 = 0;
unsigned long v40 = 0;
unsigned long v38 = 0;
unsigned long v3C = 0;
unsigned long v30 = 0;
unsigned long v2C = 0;
unsigned long v28 = 0;
unsigned long v24 = 0;
unsigned long v20 = 0;
unsigned long v1C = 0;
unsigned long v18 = 0;
eax = (unsigned long)eax_in; /* entry live eax */
edx = (unsigned long)edx_in; /* entry live edx */
/* 0x0041A9C4 push ebx */
/* 0x0041A9C5 push ecx */
/* 0x0041A9C6 push esi */
/* 0x0041A9C7 push edi */
/* 0x0041A9C8 push ebp */
/* 0x0041A9C9 sub esp, 44h */
v34 = eax & U32MASK; /* 0x0041A9CC mov [var_34], eax */
v58 = edx & U32MASK; /* 0x0041A9D0 mov [var_58], edx */
eax = (eax & 0xFFFFFF00UL) | (unsigned long)G8(0x00444E9A); /* 0x0041A9D3 mov al, byte_444E9A */
v20 = eax & 0xFFUL; /* 0x0041A9D8 mov [var_20], al */
/* 0x0041A9DC cmp edx, 800h */
if (edx <= 0x800UL) goto L_41ADD9; /* 0x0041A9E2 jbe loc_41ADD9 */
ebx = 0x14UL; /* 0x0041A9E8 mov ebx, 14h */
eax = 0x00444188UL; /* 0x0041A9ED mov eax, offset dword_444188 */
edx = 0UL; /* 0x0041A9F2 xor edx, edx */
d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* 0x0041A9F4 call sub_40C9C0 */
ebx = 0x14UL; /* 0x0041A9F9 mov ebx, 14h */
eax = 0x0044419CUL; /* 0x0041A9FE mov eax, offset dword_44419C */
edx = 0UL; /* 0x0041AA03 xor edx, edx */
d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* 0x0041AA05 call sub_40C9C0 */
ebx = 0x1400UL; /* 0x0041AA0A mov ebx, 1400h */
eax = 0x0043F6F0UL; /* 0x0041AA0F mov eax, offset dword_43F6F0 */
edx = 0UL; /* 0x0041AA14 xor edx, edx */
d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* 0x0041AA16 call sub_40C9C0 */
ebx = 0x1400UL; /* 0x0041AA1B mov ebx, 1400h */
eax = 0x00440B08UL; /* 0x0041AA20 mov eax, offset dword_440B08 */
edx = 0UL; /* 0x0041AA25 xor edx, edx */
edi = 0UL; /* 0x0041AA27 xor edi, edi */
d3u_fill(c, (unsigned int)ebx, (unsigned int)eax, (unsigned int)edx); /* 0x0041AA29 call sub_40C9C0 */
ebx = 0x400UL; /* 0x0041AA2E mov ebx, 400h */
v48 = edi & U32MASK; /* 0x0041AA33 mov [var_48], edi */
v38 = ebx & U32MASK; /* 0x0041AA37 mov [var_38], ebx */
ebp = ebx & U32MASK; /* 0x0041AA3B mov ebp, ebx */
L_41AA3D: /* 0x0041AA3D loc_41AA3D: */
eax = edi & U32MASK; /* 0x0041AA3D mov eax, edi */
ebx = edi & U32MASK; /* 0x0041AA3F mov ebx, edi */
ecx = edi & U32MASK; /* 0x0041AA41 mov ecx, edi */
edx = 0UL; /* 0x0041AA43 xor edx, edx */
ebx = (ebx << 10) & U32MASK; /* 0x0041AA45 shl ebx, 0Ah */
ecx = (0UL - ecx) & U32MASK; /* 0x0041AA48 neg ecx */
v2C = ebx & U32MASK; /* 0x0041AA4A mov [var_2C], ebx */
ebx = v34 & U32MASK; /* 0x0041AA4E mov ebx, [var_34] */
eax = (eax << 11) & U32MASK; /* 0x0041AA52 shl eax, 0Bh */
esi = (ebx + ecx) & U32MASK; /* 0x0041AA55 lea esi, [ebx+ecx] */
L_41AA58: /* 0x0041AA58 loc_41AA58: */
/* 0x0041AA58 test edi, edi */
if (edi == 0UL) goto L_41AA68; /* 0x0041AA5A jz loc_41AA68 */
/* 0x0041AA5C cmp edx, edi */
if (edx < edi) goto L_41AA68; /* 0x0041AA5E jb loc_41AA68 */
ecx = (ecx & 0xFFFFFF00UL) | (unsigned long)G8(ebx); /* 0x0041AA60 mov cl, [ebx] */
ecx = (ecx & 0xFFFF00FFUL) | ((unsigned long)G8(esi) << 8); /* 0x0041AA62 mov ch, [esi] */
ecx = (ecx & 0xFFFFFF00UL) | (((ecx & 0xFFUL) - ((ecx >> 8) & 0xFFUL)) & 0xFFUL); /* 0x0041AA64 sub cl, ch */
goto L_41AA6A; /* 0x0041AA66 jmp loc_41AA6A */
L_41AA68: /* 0x0041AA68 loc_41AA68: */
ecx = (ecx & 0xFFFFFF00UL) | (unsigned long)G8(ebx); /* 0x0041AA68 mov cl, [ebx] */
L_41AA6A: /* 0x0041AA6A loc_41AA6A: */
M8(0x004370B0UL + eax) = (unsigned char)(ecx & 0xFFUL); /* 0x0041AA6A mov byte_4370B0[eax], cl */
ecx = 0UL; /* 0x0041AA70 xor ecx, ecx */
ecx = (ecx & 0xFFFFFF00UL) | (unsigned long)G8(0x004370B0UL + eax); /* 0x0041AA72 mov cl, byte_4370B0[eax] */
ebx = (ebx + 1UL) & U32MASK; /* 0x0041AA78 inc ebx */
ecx = (ecx << 2) & U32MASK; /* 0x0041AA79 shl ecx, 2 */
esi = (esi + 1UL) & U32MASK; /* 0x0041AA7C inc esi */
ecx = (ecx + v2C) & U32MASK; /* 0x0041AA7D add ecx, [var_2C] */
eax = (eax + 1UL) & U32MASK; /* 0x0041AA81 inc eax */
edx = (edx + 1UL) & U32MASK; /* 0x0041AA82 inc edx */
M32(0x0043F6F0UL + ecx) = (G32(0x0043F6F0UL + ecx) + 1UL) & U32MASK; /* 0x0041AA83 inc dword_43F6F0[ecx] */
/* 0x0041AA89 cmp edx, 400h */
if (edx < 0x400UL) goto L_41AA58; /* 0x0041AA8F jb loc_41AA58 */
eax = v48 & U32MASK; /* 0x0041AA91 mov eax, [var_48] */
edx = edi & U32MASK; /* 0x0041AA95 mov edx, edi */
ecx = v38 & U32MASK; /* 0x0041AA97 mov ecx, [var_38] */
edx = (edx << 10) & U32MASK; /* 0x0041AA9B shl edx, 0Ah */
L_41AA9E: /* 0x0041AA9E loc_41AA9E: */
ebx = G32(0x0043F6F0UL + edx) & U32MASK; /* 0x0041AA9E mov ebx, dword_43F6F0[edx] */
ebx = (ebx * ebx) & U32MASK; /* 0x0041AAA4 imul ebx, ebx */
esi = G32(0x00444188UL + eax) & U32MASK; /* 0x0041AAA7 mov esi, dword_444188[eax] */
esi = (esi + ebx) & U32MASK; /* 0x0041AAAD add esi, ebx */
edx = (edx + 4UL) & U32MASK; /* 0x0041AAAF add edx, 4 */
M32(0x00444188UL + eax) = esi & U32MASK; /* 0x0041AAB2 mov dword_444188[eax], esi */
/* 0x0041AAB8 cmp edx, ecx */
if (edx != ecx) goto L_41AA9E; /* 0x0041AABA jnz loc_41AA9E */
esi = v38 & U32MASK; /* 0x0041AABC mov esi, [var_38] */
eax = v48 & U32MASK; /* 0x0041AAC0 mov eax, [var_48] */
edi = (edi + 1UL) & U32MASK; /* 0x0041AAC4 inc edi */
esi = (esi + ebp) & U32MASK; /* 0x0041AAC5 add esi, ebp */
eax = (eax + 4UL) & U32MASK; /* 0x0041AAC7 add eax, 4 */
v38 = esi & U32MASK; /* 0x0041AACA mov [var_38], esi */
v48 = eax & U32MASK; /* 0x0041AACE mov [var_48], eax */
/* 0x0041AAD2 cmp edi, 5 */
if (edi < 5UL) goto L_41AA3D; /* 0x0041AAD5 jb loc_41AA3D */
edx = 0x400UL; /* 0x0041AADB mov edx, 400h */
eax = v34 & U32MASK; /* 0x0041AAE0 mov eax, [var_34] */
esi = 0UL; /* 0x0041AAE4 xor esi, esi */
v44 = edx & U32MASK; /* 0x0041AAE6 mov [var_44], edx */
eax = (eax + edx) & U32MASK; /* 0x0041AAEA add eax, edx */
v3C = esi & U32MASK; /* 0x0041AAEC mov [var_3C], esi */
v50 = eax & U32MASK; /* 0x0041AAF0 mov [var_50], eax */
ebp = 4UL; /* 0x0041AAF4 mov ebp, 4 */
L_41AAF9: /* 0x0041AAF9 loc_41AAF9: */
eax = esi & U32MASK; /* 0x0041AAF9 mov eax, esi */
eax = (eax << 10) & U32MASK; /* 0x0041AAFB shl eax, 0Ah */
edx = v50 & U32MASK; /* 0x0041AAFE mov edx, [var_50] */
v28 = eax & U32MASK; /* 0x0041AB02 mov [var_28], eax */
eax = esi & U32MASK; /* 0x0041AB06 mov eax, esi */
edi = esi & U32MASK; /* 0x0041AB08 mov edi, esi */
eax = (0UL - eax) & U32MASK; /* 0x0041AB0A neg eax */
edi = (edi << 11) & U32MASK; /* 0x0041AB0C shl edi, 0Bh */
ecx = (edx + eax) & U32MASK; /* 0x0041AB0F lea ecx, [edx+eax] */
eax = (edi + 0x400UL) & U32MASK; /* 0x0041AB12 lea eax, [edi+400h] */
edi = (edi + 0x800UL) & U32MASK; /* 0x0041AB18 add edi, 800h */
L_41AB1E: /* 0x0041AB1E loc_41AB1E: */
/* 0x0041AB1E test esi, esi */
if (esi == 0UL) goto L_41AB2A; /* 0x0041AB20 jz loc_41AB2A */
ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(edx); /* 0x0041AB22 mov bl, [edx] */
ebx = (ebx & 0xFFFF00FFUL) | ((unsigned long)G8(ecx) << 8); /* 0x0041AB24 mov bh, [ecx] */
ebx = (ebx & 0xFFFFFF00UL) | (((ebx & 0xFFUL) - ((ebx >> 8) & 0xFFUL)) & 0xFFUL); /* 0x0041AB26 sub bl, bh */
goto L_41AB2C; /* 0x0041AB28 jmp loc_41AB2C */
L_41AB2A: /* 0x0041AB2A loc_41AB2A: */
ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(edx); /* 0x0041AB2A mov bl, [edx] */
L_41AB2C: /* 0x0041AB2C loc_41AB2C: */
M8(0x004370B0UL + eax) = (unsigned char)(ebx & 0xFFUL); /* 0x0041AB2C mov byte_4370B0[eax], bl */
ebx = 0UL; /* 0x0041AB32 xor ebx, ebx */
ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(0x004370B0UL + eax); /* 0x0041AB34 mov bl, byte_4370B0[eax] */
ebx = (ebx << 2) & U32MASK; /* 0x0041AB3A shl ebx, 2 */
edx = (edx + 1UL) & U32MASK; /* 0x0041AB3D inc edx */
ebx = (ebx + v28) & U32MASK; /* 0x0041AB3E add ebx, [var_28] */
ecx = (ecx + 1UL) & U32MASK; /* 0x0041AB42 inc ecx */
eax = (eax + 1UL) & U32MASK; /* 0x0041AB43 inc eax */
M32(0x00440B08UL + ebx) = (G32(0x00440B08UL + ebx) + 1UL) & U32MASK; /* 0x0041AB44 inc dword_440B08[ebx] */
/* 0x0041AB4A cmp eax, edi */
if (eax != edi) goto L_41AB1E; /* 0x0041AB4C jnz loc_41AB1E */
edx = v3C & U32MASK; /* 0x0041AB4E mov edx, [var_3C] */
eax = esi & U32MASK; /* 0x0041AB52 mov eax, esi */
ecx = v44 & U32MASK; /* 0x0041AB54 mov ecx, [var_44] */
eax = (eax << 10) & U32MASK; /* 0x0041AB58 shl eax, 0Ah */
L_41AB5B: /* 0x0041AB5B loc_41AB5B: */
ebx = G32(0x00440B08UL + eax) & U32MASK; /* 0x0041AB5B mov ebx, dword_440B08[eax] */
ebx = (ebx * ebx) & U32MASK; /* 0x0041AB61 imul ebx, ebx */
edi = G32(0x0044419CUL + edx) & U32MASK; /* 0x0041AB64 mov edi, dword_44419C[edx] */
edi = (edi + ebx) & U32MASK; /* 0x0041AB6A add edi, ebx */
eax = (eax + 4UL) & U32MASK; /* 0x0041AB6C add eax, 4 */
M32(0x0044419CUL + edx) = edi & U32MASK; /* 0x0041AB6F mov dword_44419C[edx], edi */
/* 0x0041AB75 cmp eax, ecx */
if (eax != ecx) goto L_41AB5B; /* 0x0041AB77 jnz loc_41AB5B */
edi = v44 & U32MASK; /* 0x0041AB79 mov edi, [var_44] */
eax = v3C & U32MASK; /* 0x0041AB7D mov eax, [var_3C] */
esi = (esi + 1UL) & U32MASK; /* 0x0041AB81 inc esi */
edi = (edi + 0x400UL) & U32MASK; /* 0x0041AB82 add edi, 400h */
eax = (eax + ebp) & U32MASK; /* 0x0041AB88 add eax, ebp */
v44 = edi & U32MASK; /* 0x0041AB8A mov [var_44], edi */
v3C = eax & U32MASK; /* 0x0041AB8E mov [var_3C], eax */
/* 0x0041AB92 cmp esi, 5 */
if (esi < 5UL) goto L_41AAF9; /* 0x0041AB95 jb loc_41AAF9 */
edx = 0x800UL; /* 0x0041AB9B mov edx, 800h */
ebx = v58 & U32MASK; /* 0x0041ABA0 mov ebx, [var_58] */
v30 = edx & U32MASK; /* 0x0041ABA3 mov [var_30], edx */
/* 0x0041ABA7 cmp ebx, edx */
if (ebx <= edx) goto L_41ADD9; /* 0x0041ABA9 jbe loc_41ADD9 */
eax = v34 & U32MASK; /* 0x0041ABAF mov eax, [var_34] */
ecx = 0x0C00UL; /* 0x0041ABB3 mov ecx, 0C00h */
eax = (eax + edx) & U32MASK; /* 0x0041ABB8 add eax, edx */
v40 = ecx & U32MASK; /* 0x0041ABBA mov [var_40], ecx */
v4C = eax & U32MASK; /* 0x0041ABBE mov [var_4C], eax */
L_41ABC2: /* 0x0041ABC2 loc_41ABC2: */
esi = v30 & U32MASK; /* 0x0041ABC2 mov esi, [var_30] */
eax = v40 & U32MASK; /* 0x0041ABC6 mov eax, [var_40] */
ebx = v4C & U32MASK; /* 0x0041ABCA mov ebx, [var_4C] */
edi = v34 & U32MASK; /* 0x0041ABCE mov edi, [var_34] */
ebp = v30 & U32MASK; /* 0x0041ABD2 mov ebp, [var_30] */
ecx = (ecx & 0xFFFFFF00UL); /* 0x0041ABD6 xor cl, cl */
edx = 0UL; /* 0x0041ABD8 xor edx, edx */
v18 = ecx & 0xFFUL; /* 0x0041ABDA mov [var_18], cl */
v1C = ecx & 0xFFUL; /* 0x0041ABDE mov [var_1C], cl */
esi = esi & 0x7FFUL; /* 0x0041ABE2 and esi, 7FFh */
eax = eax & 0x7FFUL; /* 0x0041ABE8 and eax, 7FFh */
v54 = ebx & U32MASK; /* 0x0041ABED mov [var_54], ebx */
edi = (edi + ebp) & U32MASK; /* 0x0041ABF1 add edi, ebp */
v24 = eax & U32MASK; /* 0x0041ABF3 mov [var_24], eax */
ecx = 0UL; /* 0x0041ABF7 xor ecx, ecx */
eax = 0UL; /* 0x0041ABF9 xor eax, eax */
L_41ABFB: /* 0x0041ABFB loc_41ABFB: */
ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(0x004370B0UL + esi); /* 0x0041ABFB mov bl, byte_4370B0[esi] */
ebx = ebx & 0xFFUL; /* 0x0041AC01 and ebx, 0FFh */
ebp = G32(0x0043F6F0UL + ((ecx + ebx * 4UL) & U32MASK)) & U32MASK; /* 0x0041AC07 mov ebp, dword_43F6F0[ecx+ebx*4] */
M32(0x00444188UL + eax) = (G32(0x00444188UL + eax) - ebp) & U32MASK; /* 0x0041AC0E sub dword_444188[eax], ebp */
ebp = (ebp - 1UL) & U32MASK; /* 0x0041AC14 dec ebp */
M32(0x0043F6F0UL + ((ecx + ebx * 4UL) & U32MASK)) = ebp & U32MASK; /* 0x0041AC15 mov dword_43F6F0[ecx+ebx*4], ebp */
ebx = ebp & U32MASK; /* 0x0041AC1C mov ebx, ebp */
ebp = G32(0x00444188UL + eax) & U32MASK; /* 0x0041AC1E mov ebp, dword_444188[eax] */
ebp = (ebp - ebx) & U32MASK; /* 0x0041AC24 sub ebp, ebx */
ebx = v24 & U32MASK; /* 0x0041AC26 mov ebx, [var_24] */
M32(0x00444188UL + eax) = ebp & U32MASK; /* 0x0041AC2A mov dword_444188[eax], ebp */
ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(0x004370B0UL + (ebx & U32MASK)); /* 0x0041AC30 mov bl, byte_4370B0[ebx] */
ebx = ebx & 0xFFUL; /* 0x0041AC36 and ebx, 0FFh */
ebp = G32(0x0043F6F0UL + ((ecx + ebx * 4UL) & U32MASK)) & U32MASK; /* 0x0041AC3C mov ebp, dword_43F6F0[ecx+ebx*4] */
M32(0x00444188UL + eax) = (G32(0x00444188UL + eax) + ebp) & U32MASK; /* 0x0041AC43 add dword_444188[eax], ebp */
ebp = (ebp + 1UL) & U32MASK; /* 0x0041AC49 inc ebp */
M32(0x0043F6F0UL + ((ecx + ebx * 4UL) & U32MASK)) = ebp & U32MASK; /* 0x0041AC4A mov dword_43F6F0[ecx+ebx*4], ebp */
M32(0x00444188UL + eax) = (G32(0x00444188UL + eax) + ebp) & U32MASK; /* 0x0041AC51 add dword_444188[eax], ebp */
ebp = G32(0x00440B08UL + ((ecx + ebx * 4UL) & U32MASK)) & U32MASK; /* 0x0041AC57 mov ebp, dword_440B08[ecx+ebx*4] */
M32(0x0044419CUL + eax) = (G32(0x0044419CUL + eax) - ebp) & U32MASK; /* 0x0041AC5E sub dword_44419C[eax], ebp */
ebp = (ebp - 1UL) & U32MASK; /* 0x0041AC64 dec ebp */
M32(0x00440B08UL + ((ecx + ebx * 4UL) & U32MASK)) = ebp & U32MASK; /* 0x0041AC65 mov dword_440B08[ecx+ebx*4], ebp */
ebx = ebp & U32MASK; /* 0x0041AC6C mov ebx, ebp */
M32(0x0044419CUL + eax) = (G32(0x0044419CUL + eax) - ebx) & U32MASK; /* 0x0041AC6E sub dword_44419C[eax], ebx */
/* 0x0041AC74 test edx, edx */
if (edx == 0UL) goto L_41AC82; /* 0x0041AC76 jz loc_41AC82 */
ebx = v54 & U32MASK; /* 0x0041AC78 mov ebx, [var_54] */
ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(ebx & U32MASK); /* 0x0041AC7C mov bl, [ebx] */
ebx = (ebx & 0xFFFFFF00UL) | (((ebx & 0xFFUL) - (unsigned long)G8(edi)) & 0xFFUL); /* 0x0041AC7E sub bl, [edi] */
goto L_41AC88; /* 0x0041AC80 jmp loc_41AC88 */
L_41AC82: /* 0x0041AC82 loc_41AC82: */
ebx = v54 & U32MASK; /* 0x0041AC82 mov ebx, [var_54] */
ebx = (ebx & 0xFFFFFF00UL) | (unsigned long)G8(ebx & U32MASK); /* 0x0041AC86 mov bl, [ebx] */
L_41AC88: /* 0x0041AC88 loc_41AC88: */
M8(0x004370B0UL + esi) = (unsigned char)(ebx & 0xFFUL); /* 0x0041AC88 mov byte_4370B0[esi], bl */
ebx = ebx & 0xFFUL; /* 0x0041AC8E and ebx, 0FFh */
edi = (edi - 1UL) & U32MASK; /* 0x0041AC94 dec edi */
ebp = G32(0x00440B08UL + ((ecx + ebx * 4UL) & U32MASK)) & U32MASK; /* 0x0041AC95 mov ebp, dword_440B08[ecx+ebx*4] */
ecx = (ecx + 0x400UL) & U32MASK; /* 0x0041AC9C add ecx, 400h */
M32(0x0044419CUL + eax) = (G32(0x0044419CUL + eax) + ebp) & U32MASK; /* 0x0041ACA2 add dword_44419C[eax], ebp */
ebp = (ebp + 1UL) & U32MASK; /* 0x0041ACA8 inc ebp */
eax = (eax + 4UL) & U32MASK; /* 0x0041ACA9 add eax, 4 */
M32(0x00440708UL + ((ecx + ebx * 4UL) & U32MASK)) = ebp & U32MASK; /* 0x0041ACAC mov dword_440708[ecx+ebx*4], ebp */
ebx = ebp & U32MASK; /* 0x0041ACB3 mov ebx, ebp */
ebp = G32(0x00444198UL + eax) & U32MASK; /* 0x0041ACB5 mov ebp, dword_444198[eax] */
esi = (esi + 0x800UL) & U32MASK; /* 0x0041ACBB add esi, 800h */
ebp = (ebp + ebx) & U32MASK; /* 0x0041ACC1 add ebp, ebx */
ebx = v24 & U32MASK; /* 0x0041ACC3 mov ebx, [var_24] */
edx = (edx + 1UL) & U32MASK; /* 0x0041ACC7 inc edx */
ebx = (ebx + 0x800UL) & U32MASK; /* 0x0041ACC8 add ebx, 800h */
M32(0x00444198UL + eax) = ebp & U32MASK; /* 0x0041ACCE mov dword_444198[eax], ebp */
v24 = ebx & U32MASK; /* 0x0041ACD4 mov [var_24], ebx */
/* 0x0041ACD8 cmp edx, 5 */
if (edx < 5UL) goto L_41ABFB; /* 0x0041ACDB jb loc_41ABFB */
edx = 1UL; /* 0x0041ACE1 mov edx, 1 */
eax = 4UL; /* 0x0041ACE6 mov eax, 4 */
L_41ACEB: /* 0x0041ACEB loc_41ACEB: */
ebx = 0UL; /* 0x0041ACEB xor ebx, ebx */
ebx = (ebx & 0xFFFFFF00UL) | (v18 & 0xFFUL); /* 0x0041ACED mov bl, [var_18] */
ecx = (ebx * 4UL) & U32MASK; /* 0x0041ACF1 lea ecx, ds:0[ebx*4] */
ebx = G32(0x00444188UL + eax) & U32MASK; /* 0x0041ACF8 mov ebx, dword_444188[eax] */
/* 0x0041ACFE cmp ebx, dword_444188[ecx] */
if (ebx <= G32(0x00444188UL + ecx)) goto L_41AD0A; /* 0x0041AD04 jbe loc_41AD0A */
v18 = edx & 0xFFUL; /* 0x0041AD06 mov [var_18], dl */
L_41AD0A: /* 0x0041AD0A loc_41AD0A: */
ebx = 0UL; /* 0x0041AD0A xor ebx, ebx */
ebx = (ebx & 0xFFFFFF00UL) | (v1C & 0xFFUL); /* 0x0041AD0C mov bl, [var_1C] */
ecx = G32(0x0044419CUL + eax) & U32MASK; /* 0x0041AD10 mov ecx, dword_44419C[eax] */
/* 0x0041AD16 cmp ecx, dword_44419C[ebx*4] */
if (ecx <= G32(0x0044419CUL + ((ebx * 4UL) & U32MASK))) goto L_41AD23; /* 0x0041AD1D jbe loc_41AD23 */
v1C = edx & 0xFFUL; /* 0x0041AD1F mov [var_1C], dl */
L_41AD23: /* 0x0041AD23 loc_41AD23: */
edx = (edx + 1UL) & U32MASK; /* 0x0041AD23 inc edx */
eax = (eax + 4UL) & U32MASK; /* 0x0041AD24 add eax, 4 */
/* 0x0041AD27 cmp edx, 5 */
if (edx < 5UL) goto L_41ACEB; /* 0x0041AD2A jb loc_41ACEB */
eax = (eax & 0xFFFFFF00UL) | (v18 & 0xFFUL); /* 0x0041AD2C mov al, [var_18] */
ecx = (ecx & 0xFFFFFF00UL) | (v20 & 0xFFUL); /* 0x0041AD30 mov cl, [var_20] */
/* 0x0041AD34 cmp al, cl */
if ((eax & 0xFFUL) == (ecx & 0xFFUL)) goto L_41AD5E; /* 0x0041AD36 jz loc_41AD5E */
eax = 0UL; /* 0x0041AD38 xor eax, eax */
eax = (eax & 0xFFFFFF00UL) | (v18 & 0xFFUL); /* 0x0041AD3A mov al, [var_18] */
edx = G32(0x00444188UL + ((eax * 4UL) & U32MASK)) & U32MASK; /* 0x0041AD3E mov edx, dword_444188[eax*4] */
eax = 0UL; /* 0x0041AD45 xor eax, eax */
eax = (eax & 0xFFFFFF00UL) | (ecx & 0xFFUL); /* 0x0041AD47 mov al, cl */
eax = G32(0x00444188UL + ((eax * 4UL) & U32MASK)) & U32MASK; /* 0x0041AD49 mov eax, dword_444188[eax*4] */
edx = (edx + edx * 2UL) & U32MASK; /* 0x0041AD50 lea edx, [edx+edx*2] */
eax = (eax << 2) & U32MASK; /* 0x0041AD53 shl eax, 2 */
/* 0x0041AD56 cmp edx, eax */
if (edx >= eax) goto L_41AD5E; /* 0x0041AD58 jnb loc_41AD5E */
v18 = ecx & 0xFFUL; /* 0x0041AD5A mov [var_18], cl */
L_41AD5E: /* 0x0041AD5E loc_41AD5E: */
eax = (eax & 0xFFFFFF00UL) | (v1C & 0xFFUL); /* 0x0041AD5E mov al, [var_1C] */
ecx = (ecx & 0xFFFF00FFUL) | ((v20 & 0xFFUL) << 8); /* 0x0041AD62 mov ch, [var_20] */
/* 0x0041AD66 cmp al, ch */
if ((eax & 0xFFUL) == ((ecx >> 8) & 0xFFUL)) goto L_41AD90; /* 0x0041AD68 jz loc_41AD90 */
eax = 0UL; /* 0x0041AD6A xor eax, eax */
edx = 0UL; /* 0x0041AD6C xor edx, edx */
eax = (eax & 0xFFFFFF00UL) | (v1C & 0xFFUL); /* 0x0041AD6E mov al, [var_1C] */
edx = (edx & 0xFFFFFF00UL) | ((ecx >> 8) & 0xFFUL); /* 0x0041AD72 mov dl, ch */
eax = G32(0x0044419CUL + ((eax * 4UL) & U32MASK)) & U32MASK; /* 0x0041AD74 mov eax, dword_44419C[eax*4] */
edx = G32(0x0044419CUL + ((edx * 4UL) & U32MASK)) & U32MASK; /* 0x0041AD7B mov edx, dword_44419C[edx*4] */
eax = (eax + eax * 2UL) & U32MASK; /* 0x0041AD82 lea eax, [eax+eax*2] */
edx = (edx << 2) & U32MASK; /* 0x0041AD85 shl edx, 2 */
/* 0x0041AD88 cmp eax, edx */
if (eax >= edx) goto L_41AD90; /* 0x0041AD8A jnb loc_41AD90 */
v1C = (ecx >> 8) & 0xFFUL; /* 0x0041AD8C mov [var_1C], ch */
L_41AD90: /* 0x0041AD90 loc_41AD90: */
eax = (eax & 0xFFFFFF00UL) | (v18 & 0xFFUL); /* 0x0041AD90 mov al, [var_18] */
/* 0x0041AD94 cmp al, [var_1C] */
if ((eax & 0xFFUL) == (v1C & 0xFFUL)) goto L_41ADAB; /* 0x0041AD98 jz loc_41ADAB */
ebx = v30 & U32MASK; /* 0x0041AD9A mov ebx, [var_30] */
M8(0x00444E9AUL) = (unsigned char)(eax & 0xFFUL); /* 0x0041AD9E mov byte_444E9A, al */
ebx = (ebx - 0x200UL) & U32MASK; /* 0x0041ADA3 sub ebx, 200h */
goto L_41ADE5; /* 0x0041ADA9 jmp loc_41ADE5 */
L_41ADAB: /* 0x0041ADAB loc_41ADAB: */
eax = (eax & 0xFFFFFF00UL) | (v18 & 0xFFUL); /* 0x0041ADAB mov al, [var_18] */
ebp = v4C & U32MASK; /* 0x0041ADAF mov ebp, [var_4C] */
edx = v30 & U32MASK; /* 0x0041ADB3 mov edx, [var_30] */
ebx = v58 & U32MASK; /* 0x0041ADB7 mov ebx, [var_58] */
v20 = eax & 0xFFUL; /* 0x0041ADBA mov [var_20], al */
ebp = (ebp + 1UL) & U32MASK; /* 0x0041ADBE inc ebp */
edx = (edx + 1UL) & U32MASK; /* 0x0041ADBF inc edx */
eax = v40 & U32MASK; /* 0x0041ADC0 mov eax, [var_40] */
v4C = ebp & U32MASK; /* 0x0041ADC4 mov [var_4C], ebp */
eax = (eax + 1UL) & U32MASK; /* 0x0041ADC8 inc eax */
v30 = edx & U32MASK; /* 0x0041ADC9 mov [var_30], edx */
v40 = eax & U32MASK; /* 0x0041ADCD mov [var_40], eax */
/* 0x0041ADD1 cmp edx, ebx */
if (edx < ebx) goto L_41ABC2; /* 0x0041ADD3 jb loc_41ABC2 */
L_41ADD9: /* 0x0041ADD9 loc_41ADD9: */
eax = (eax & 0xFFFFFF00UL) | (v20 & 0xFFUL); /* 0x0041ADD9 mov al, [var_20] */
ebx = v58 & U32MASK; /* 0x0041ADDD mov ebx, [var_58] */
M8(0x00444E9AUL) = (unsigned char)(eax & 0xFFUL); /* 0x0041ADE0 mov byte_444E9A, al */
L_41ADE5: /* 0x0041ADE5 loc_41ADE5: */
eax = ebx & U32MASK; /* 0x0041ADE5 mov eax, ebx */
/* 0x0041ADE7 add esp, 44h */
/* 0x0041ADEA pop ebp */
/* 0x0041ADEB pop edi */
/* 0x0041ADEC pop esi */
/* 0x0041ADED pop ecx */
/* 0x0041ADEE pop ebx */
/* 0x0041ADEF retn */
return (unsigned int)(eax & U32MASK);
}
