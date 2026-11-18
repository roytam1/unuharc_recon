#ifndef UHA04_DEC4_INT_H
#define UHA04_DEC4_INT_H
/* Shared internals of the typed core-4 decoder. Included by
 * uha04_dec4_clean.c, uha04_dec4_lzp.c, uha04_dec4_ppm.c and
 * uha04_dec4_rc.c. The inline loaders live here so each file
 * sees the same memory model. */
#include <stdlib.h>
#include <string.h>
#include "uha_types.h"
#include "uharc_dec.h"
#include "uha04_dec4_addr.h"
#include "uha04_dec4_state.h"
#include <stddef.h>

#ifdef _MSC_VER
#define D4V_AINLINE __inline
#define D4V_INLINE  __inline
#else
/* always_inline belongs on helpers of a few instructions. The span
 * resolvers walk span[40] and page_span[4096]; forcing them inline
 * copies that walk into every load and blows the instruction cache. */
#define D4V_AINLINE __inline__ __attribute__((always_inline))
#define D4V_INLINE  __inline__
#endif

/* The globals are one byte array in the typed state: the DGROUP image
 * (0x42E000..0x440000) and the BSS above it (0x440000..0x45D0C8).  The
 * image is copied into it at init, so a cell inside the image keeps its
 * initial value from the image and a cell in the BSS starts at zero -
 * which is what the arena held. */

#define D4V_FIXED_BASE   0x420000UL
#define D4V_FIXED_SIZE   0x40000UL   /* ..0x460000 covers DGROUP/BSS/named cells */
#define D4V_GLOBALS_BASE 0x42E000UL /* the DGROUP image */
#define D4V_GLOBALS_END  0x45D0C8UL /* end of the last named cell */
#define D4V_IO_BASE    0x00000000UL
#define D4V_IO_SIZE    0x00040000UL /* covers in_buf/out_buf carved near 0x1000 */
#define D4V_MODEL_BASE 0x00600000UL
#define D4V_MODEL_SIZE 0x04000000UL /* 64MB for ~53MB carved footprint */

typedef struct dec4v_ctx {
    unsigned char *mem; /* compat, unused */
    unsigned char fx_gap[8]; /* the arena's zero gap past the fixed region */
    unsigned char *io_mem;
    unsigned char *model_mem; /* left NULL; the gap-fill test watches it */
    struct {
        uint32_t base;
        uint32_t size;
        unsigned char *p;
    } span[40];
    int nspan;
    int span_hint;
    unsigned short page_span[4096];
    uint32_t hi;
    uharc_dec_read_cb rcb;
    void *rctx;
    uharc_dec_write_cb wcb;
    void *wctx;
    /* Archive header byte +9. Bit 6 selects the kennedy.xls PPM case.
     * Owned by this context, so two decodes do not share it. */
    unsigned char flags2;
    unsigned in_pos;
    unsigned in_avail;
    unsigned out_fill;
    uint32_t out_tot;
    uint32_t out_len;
    int err;
    /* arena offsets of the 64KB stream buffers (dword_436ED0/436ED4) */
    uint32_t in_buf;
    uint32_t out_buf;
    /* per-block pointers (aliases into model_mem) and bases */
    /* blk00_dict/blk00_base: MODEL-REGION alias, not just the dict block.
     * c->hi is set to D4V_MODEL_BASE before the model blocks are carved and
     * the dict block is the FIRST carve, so blk00_base == D4V_MODEL_BASE
     * and blk00_dict == model_mem.  Hence
     *     blk00_dict[X - blk00_base] == model_mem[X - MODEL_BASE]
     *     == d4v_ptr(c, X)
     * for every model-region address X.  A blk00_dict[...] access whose
     * address comes from another block pointer (BLK_PTR_03/04/05/...)
     * therefore RESOLVES CORRECTLY but is misattributed; such sites use
     * the owning block pointer (c->blkNN) instead.  Every blk00_dict
     * access has been checked to land inside the model region. */
    unsigned char *blk00_dict; uint32_t blk00_base; uint32_t blk00_size;
    unsigned char *blk01; uint32_t blk01_base; uint32_t blk01_size;
    unsigned char *blk02; uint32_t blk02_base; uint32_t blk02_size;
    unsigned char *blk03; uint32_t blk03_base; uint32_t blk03_size;
    unsigned char *blk04; uint32_t blk04_base; uint32_t blk04_size;
    unsigned char *blk05; uint32_t blk05_base; uint32_t blk05_size;
    unsigned char *blk06; uint32_t blk06_base; uint32_t blk06_size;
    unsigned char *blk07; uint32_t blk07_base; uint32_t blk07_size;
    unsigned char *blk08; uint32_t blk08_base; uint32_t blk08_size;
    unsigned char *blk09; uint32_t blk09_base; uint32_t blk09_size;
    unsigned char *blk10; uint32_t blk10_base; uint32_t blk10_size;
    unsigned char *blk11; uint32_t blk11_base; uint32_t blk11_size;
    unsigned char *blk12; uint32_t blk12_base; uint32_t blk12_size;
    unsigned char *blk13; uint32_t blk13_base; uint32_t blk13_size;
    unsigned char *blk14; uint32_t blk14_base; uint32_t blk14_size;
    unsigned char *blk15; uint32_t blk15_base; uint32_t blk15_size;
    unsigned char *blk16; uint32_t blk16_base; uint32_t blk16_size;
    unsigned char *blk17; uint32_t blk17_base; uint32_t blk17_size;
    unsigned char *blk18; uint32_t blk18_base; uint32_t blk18_size;
    unsigned char *blk19; uint32_t blk19_base; uint32_t blk19_size;
    /* PPM per-order context blocks.  The original addresses the eight PPM
     * orders through the stride-4 cell run 0x433B58..0x433B74, which is
     * exactly the contiguous block-pointer array BLK_PTR_06..BLK_PTR_13;
     * order o therefore owns model block 06+o, and dword_459E38[o*4] is
     * the 4-byte slot index inside that block.  blkord[o] is that block's
     * typed pointer and blkord_base[o] its arena base address (the PPM
     * scorer hands that address to its caller, so it is still needed).
     * Verified over the whole core-4 corpus: the cell value always equals
     * the block base and the slot index always stays inside the block.
     *
     * The slot index itself is read through D4V_TBL_459E38_32, which is
     * still overridden to d4v_ptr(): the eight slots 0x459E38..0x459E54
     * carry two overlapping 16-bit hi-word views (0x459E3E, 0x459E42) and
     * therefore need a union, not eight struct members, before the
     * overrides can go. */
    unsigned char *blkord[8];
    uint32_t blkord_base[8];
    d4v_state st;   /* typed model state */
    /* The raw accesses read and write dwords straight into the globals
     * array, so a dword at its very last byte would run past it.  The
     * pad takes that read; the arena's bytes there are zero, and so
     * are these. */
    unsigned char fx_pad[8];
} dec4v_ctx;

/* The arena port reaches every cell through M8/M16/M32, which clamp the
 * address to the end of its 64 MB buffer (V4O).  The address itself is
 * computed in 32 bits, so an index that runs past the globals wraps
 * back into the arena - and into the model blocks, which hold live
 * data.  The typed port has to wrap and clamp the same way to read the
 * same bytes; c->fx_gap stands in for the arena's zero gap. */
static D4V_INLINE int d4v_span_idx(dec4v_ctx *c, uint32_t addr);
static D4V_INLINE int d4v_span_idx(dec4v_ctx *c, uint32_t addr)
{
    int i, n, h;
    n = c->nspan;
    h = c->span_hint;
    if (h >= 0 && h < n &&
        addr >= c->span[h].base &&
        addr < c->span[h].base + c->span[h].size)
        return h;
    for (i = 0; i < n; i++) {
        if (addr < c->span[i].base)
            break;
        if (addr < c->span[i].base + c->span[i].size) {
            c->span_hint = i;
            return i;
        }
    }
    return -1;
}

static D4V_INLINE unsigned char *d4v_at(dec4v_ctx *c, uint32_t addr);
static D4V_INLINE unsigned char *d4v_at(dec4v_ctx *c, uint32_t addr)
{
    int i;
    uint32_t off, idx;
    unsigned si;
    i = c->span_hint;
    if (i >= 0 && i < c->nspan &&
        addr >= c->span[i].base &&
        addr < c->span[i].base + c->span[i].size)
        return c->span[i].p + (addr - c->span[i].base);
    if (addr >= D4V_MODEL_BASE) {
        off = addr - D4V_MODEL_BASE;
        idx = off >> 12;
        if (idx < 4096u) {
            si = c->page_span[idx];
            if (si < (unsigned)c->nspan &&
                addr >= c->span[si].base &&
                addr < c->span[si].base + c->span[si].size) {
                c->span_hint = (int)si;
                return c->span[si].p + (addr - c->span[si].base);
            }
        }
    }
    i = d4v_span_idx(c, addr);
    if (i < 0)
        return 0;
    return c->span[i].p + (addr - c->span[i].base);
}

unsigned char *d4v_fx(dec4v_ctx *c, uint32_t off);

static D4V_INLINE unsigned char *d4v_ptr(dec4v_ctx *c, uint32_t addr);
static D4V_INLINE unsigned char *d4v_ptr(dec4v_ctx *c, uint32_t addr)
{
    if (addr >= D4V_MODEL_BASE) {
        unsigned char *p = d4v_at(c, addr);
        if (p)
            return p;
        return c->fx_gap;
    }
    /* The globals are one byte array the typed state owns. */
    if (addr >= D4V_GLOBALS_BASE && addr < D4V_GLOBALS_END)
        return c->st.g.bytes + (addr - D4V_GLOBALS_BASE);
    if (addr < D4V_IO_SIZE)
        return c->io_mem + addr;
    /* The arena port clamps every address to the end of its buffer, so
     * an address outside the globals, the I/O buffers and the model
     * blocks reads the arena's zero gap.  fx_gap stands in for it. */
    return c->fx_gap;
}

/* Boundary-safe byte-wise accessors: the fixed region is stored in more
 * than one allocation, so a multi-byte access may straddle the boundary
 * between them and must not read a contiguous block of memory. */
#ifdef D4V_AUDIT
#include <stdio.h>
static unsigned char d4v_aud_pristine[73728];
static unsigned char d4v_aud_wrote[192712];
static uint32_t d4v_aud_model_max;
static void d4v_aud_read(uint32_t a, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        uint32_t x = a + (uint32_t)i;
        if (x >= D4V_GLOBALS_BASE && x < D4V_GLOBALS_BASE + 73728u) {
            uint32_t off = x - D4V_GLOBALS_BASE;
            if (!d4v_aud_wrote[off])
                d4v_aud_pristine[off] = 1;
        }
        if (x >= D4V_MODEL_BASE && x >= d4v_aud_model_max)
            d4v_aud_model_max = x + 1u;
    }
}
static void d4v_aud_write(uint32_t a, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        uint32_t x = a + (uint32_t)i;
        if (x >= D4V_GLOBALS_BASE && x < D4V_GLOBALS_END)
            d4v_aud_wrote[x - D4V_GLOBALS_BASE] = 1;
    }
}
static void d4v_aud_dump(void)
{
    uint32_t i, n = 0, runs = 0, run = 0;
    for (i = 0; i < 73728u; i++) {
        if (d4v_aud_pristine[i]) {
            n++;
            if (!run) runs++;
            run = 1;
        } else {
            run = 0;
        }
    }
    fprintf(stderr, "AUDIT pristine_image_bytes %u runs %u model_max 0x%08x\n",
            n, runs, d4v_aud_model_max);
}
#else
static D4V_AINLINE void d4v_aud_read(uint32_t a, int n) { (void)a; (void)n; }
static D4V_AINLINE void d4v_aud_write(uint32_t a, int n) { (void)a; (void)n; }
static D4V_AINLINE void d4v_aud_dump(void) {}
#endif

/* A word that sits entirely inside one allocation is a native
 * little-endian load.  Linux and Windows are little-endian, and the
 * typed accessors already store host-endian words into the same bytes.
 * A word that crosses two allocations stays byte-wise: that is the
 * case the four d4v_ptr calls existed for. */
static D4V_INLINE unsigned char *d4v_span(dec4v_ctx *c, uint32_t a, int n);
static D4V_INLINE unsigned char *d4v_span(dec4v_ctx *c, uint32_t a, int n)
{
    uint32_t last;
    if (n <= 1)
        return d4v_ptr(c, a);
    last = a + (uint32_t)(n - 1);
    if (last < a)
        return 0;
    if (a >= D4V_GLOBALS_BASE && last < D4V_GLOBALS_END)
        return c->st.g.bytes + (a - D4V_GLOBALS_BASE);
    if (a < D4V_IO_SIZE && last < D4V_IO_SIZE)
        return c->io_mem + a;
    {
        int i;
        i = d4v_span_idx(c, a);
        if (i >= 0 && last < c->span[i].base + c->span[i].size)
            return c->span[i].p + (a - c->span[i].base);
    }
    return 0;
}

static D4V_AINLINE unsigned int d4v_ld8(dec4v_ctx *c, uint32_t a);
static D4V_AINLINE unsigned int d4v_ld8(dec4v_ctx *c, uint32_t a)
{
    d4v_aud_read(a, 1);
    return *d4v_ptr(c, a);
}
static D4V_AINLINE void d4v_st8(dec4v_ctx *c, uint32_t a, unsigned int v);
static D4V_AINLINE void d4v_st8(dec4v_ctx *c, uint32_t a, unsigned int v)
{
    d4v_aud_write(a, 1);
    *d4v_ptr(c, a) = (unsigned char)v;
}
/* Two bytes inside one model block. addr is the arena address, base the
 * block's arena base, b its native pointer. */
static D4V_AINLINE unsigned int d4v_ld8g(dec4v_ctx *c, uint32_t a);
static D4V_AINLINE unsigned int d4v_ld8g(dec4v_ctx *c, uint32_t a)
{
    if (a >= D4V_GLOBALS_BASE && a < D4V_GLOBALS_END)
        return c->st.g.bytes[a - D4V_GLOBALS_BASE];
    return (unsigned int)(*(unsigned char *)d4v_ptr(c, a));
}
static D4V_AINLINE unsigned int d4v_slot32(dec4v_ctx *c, unsigned off);
static D4V_AINLINE unsigned int d4v_slot32(dec4v_ctx *c, unsigned off)
{
    unsigned int v;
    memcpy(&v, c->st.g.bytes + ((D4V_RSLOT_LO + off) - D4V_GLOBALS_BASE), 4);
    return v;
}
static D4V_AINLINE unsigned int d4v_gld32(const unsigned char *p);
static D4V_AINLINE unsigned int d4v_gld32(const unsigned char *p)
{
    unsigned int v;
    memcpy(&v, p, 4);
    return v;
}
static D4V_AINLINE void d4v_gst32(unsigned char *p, unsigned int v);
static D4V_AINLINE void d4v_gst32(unsigned char *p, unsigned int v)
{
    memcpy(p, &v, 4);
}
static D4V_AINLINE unsigned int d4v_blk_u16(unsigned char *b, uint32_t base, uint32_t addr);
static D4V_AINLINE unsigned int d4v_blk_u16(unsigned char *b, uint32_t base, uint32_t addr)
{
    unsigned short s;
    s = 0;
    memcpy(&s, b + (addr - base), 2);
    return (unsigned int)s;
}
/* Same read when addr is inside the block; otherwise the general loader. */
static D4V_INLINE unsigned int d4v_ld16(dec4v_ctx *c, uint32_t a);
static D4V_INLINE void d4v_st16(dec4v_ctx *c, uint32_t a, unsigned int v);
static D4V_INLINE unsigned int d4v_u16_in(dec4v_ctx *c, unsigned char *b,
    uint32_t base, uint32_t size, uint32_t addr);
static D4V_INLINE unsigned int d4v_u16_in(dec4v_ctx *c, unsigned char *b,
    uint32_t base, uint32_t size, uint32_t addr)
{
    uint32_t off;
    if (addr >= base) {
        off = addr - base;
        if (off + 1u < size)
            return d4v_blk_u16(b, base, addr);
    }
    return d4v_ld16(c, addr);
}
static D4V_INLINE void d4v_u16_st_in(dec4v_ctx *c, unsigned char *b,
    uint32_t base, uint32_t size, uint32_t addr, unsigned int v);
static D4V_INLINE void d4v_u16_st_in(dec4v_ctx *c, unsigned char *b,
    uint32_t base, uint32_t size, uint32_t addr, unsigned int v)
{
    uint32_t off;
    unsigned short s;
    if (addr >= base) {
        off = addr - base;
        if (off + 1u < size) {
            s = (unsigned short)v;
            memcpy(b + off, &s, 2);
            return;
        }
    }
    d4v_st16(c, addr, v);
}
static D4V_INLINE unsigned int d4v_ld16(dec4v_ctx *c, uint32_t a)
{
    unsigned char *p;
    unsigned int v;
    d4v_aud_read(a, 2);
    p = d4v_span(c, a, 2);
    if (p) {
        v = 0;
        memcpy(&v, p, 2);
        return v;
    }
    return (unsigned int)(*d4v_ptr(c, a)
        | ((unsigned int)*d4v_ptr(c, a + 1UL) << 8));
}
static D4V_INLINE void d4v_st16(dec4v_ctx *c, uint32_t a, unsigned int v)
{
    unsigned char *p;
    unsigned short s;
    d4v_aud_write(a, 2);
    p = d4v_span(c, a, 2);
    if (p) {
        s = (unsigned short)v;
        memcpy(p, &s, 2);
        return;
    }
    *d4v_ptr(c, a) = (unsigned char)(v & 0xFFU);
    *d4v_ptr(c, a + 1UL) = (unsigned char)((v >> 8) & 0xFFU);
}
static D4V_INLINE unsigned int d4v_ld32(dec4v_ctx *c, uint32_t a);
static D4V_INLINE unsigned int d4v_ld32(dec4v_ctx *c, uint32_t a)
{
    unsigned char *p;
    unsigned int v;
    d4v_aud_read(a, 4);
    p = d4v_span(c, a, 4);
    if (p) {
        memcpy(&v, p, 4);
        return v;
    }
    return (unsigned int)(*d4v_ptr(c, a)
        | ((unsigned int)*d4v_ptr(c, a + 1UL) << 8)
        | ((unsigned int)*d4v_ptr(c, a + 2UL) << 16)
        | ((unsigned int)*d4v_ptr(c, a + 3UL) << 24));
}
static D4V_INLINE void d4v_st32(dec4v_ctx *c, uint32_t a, unsigned int v);
static D4V_INLINE void d4v_st32(dec4v_ctx *c, uint32_t a, unsigned int v)
{
    unsigned char *p;
    d4v_aud_write(a, 4);
    p = d4v_span(c, a, 4);
    if (p) {
        memcpy(p, &v, 4);
        return;
    }
    *d4v_ptr(c, a) = (unsigned char)(v & 0xFFU);
    *d4v_ptr(c, a + 1UL) = (unsigned char)((v >> 8) & 0xFFU);
    *d4v_ptr(c, a + 2UL) = (unsigned char)((v >> 16) & 0xFFU);
    *d4v_ptr(c, a + 3UL) = (unsigned char)((v >> 24) & 0xFFU);
}
/* Record pointer when the whole record sits in the globals; otherwise
 * the general loader. base is the record's arena address. */
static D4V_AINLINE unsigned int d4v_mld32(dec4v_ctx *c, unsigned char *m,
    uint32_t base, uint32_t addr);
static D4V_AINLINE unsigned int d4v_mld32(dec4v_ctx *c, unsigned char *m,
    uint32_t base, uint32_t addr)
{
    if (m)
        return d4v_gld32(m + (addr - base));
    return d4v_ld32(c, addr);
}
static D4V_AINLINE void d4v_mst32(dec4v_ctx *c, unsigned char *m,
    uint32_t base, uint32_t addr, unsigned int v);
static D4V_AINLINE void d4v_mst32(dec4v_ctx *c, unsigned char *m,
    uint32_t base, uint32_t addr, unsigned int v)
{
    if (m) {
        d4v_gst32(m + (addr - base), v);
        return;
    }
    d4v_st32(c, addr, v);
}


#define U32MASK 0xFFFFFFFFUL
#define ADD8_LO(r,v) ((r) = ((r) & 0xFFFFFF00UL) | (((r) + (v)) & 0xFFUL))
#define SUB8_LO(r,v) ((r) = ((r) & 0xFFFFFF00UL) | (((r) - (v)) & 0xFFUL))
#define DECB_LO(r) ((r) = ((r) & 0xFFFFFF00UL) | (((r) - 1) & 0xFFUL))
#define ADD8_HI(r,v) ((r) = ((r) & 0xFFFF00FFUL) | ((((((r) >> 8) & 0xFFUL) + (v)) & 0xFFUL) << 8))
#define D4V_SP 0x0F000000u /* stand-in stack base; never dereferenced */

void d4v_fill(dec4v_ctx *c, unsigned int len, unsigned int dst, unsigned int val);
uint32_t d4v_sar(uint32_t v, unsigned int n);
void d4v_put_byte(dec4v_ctx *c, unsigned int b);
void d4v_rcrenorm_reg(dec4v_ctx *c, unsigned int *pcode, unsigned int *prange, unsigned int *pbyte);
void d4v_memmove(dec4v_ctx *c, unsigned int dst, unsigned int src, unsigned int n);
unsigned int d4v_read_stream(dec4v_ctx *c, unsigned int dst, unsigned int n, unsigned int aux);
void d4v_rc_init(dec4v_ctx *c);
void d4v_rc_primer(dec4v_ctx *c);

/* ---- worker entry points (bodies appended below) ---- */
void d4vc_lzp(dec4v_ctx *c);
int d4vc_ppm(dec4v_ctx *c);
unsigned int d4vc_ppmdec(dec4v_ctx *c);
void d4vc_sym(dec4v_ctx *c, unsigned int a_eax);
unsigned int d4vc_ppm_sym_split(dec4v_ctx *c, unsigned int eax_in,
                        unsigned int edx_in, unsigned int ebx_in);
unsigned int d4vc_ppm_prob_decode(dec4v_ctx *c, unsigned int eax_in);
unsigned int d4vc_freq_divide_price(dec4v_ctx *c, unsigned int eax_in,
                        unsigned int edx_in);
void d4vc_freq_adapt_459398(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in,
                unsigned int ebx_in);
void d4vc_freq_adapt_45af60(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in,
                unsigned int ebx_in);
unsigned int d4vc_freq_scale_step(dec4v_ctx *c, unsigned int tab,
                        unsigned int edx_in);
void d4vc_counter_bump_halve(dec4v_ctx *c, unsigned int tab, unsigned int flag_in);
void d4vc_rescale_blk02(dec4v_ctx *c);
void d4vc_rescale_blk01(dec4v_ctx *c);
void d4vc_rescale_ppm_plane(dec4v_ctx *c, unsigned int idx);
void d4vc_ppm_order_update(dec4v_ctx *c, unsigned int sym, unsigned int arg);
unsigned int d4vc_ppm_score_orders(dec4v_ctx *c, unsigned int ebx_in);
void d4vc_ppm_mask_select(dec4v_ctx *c, unsigned int sym, unsigned int flag);
void d4vc_ppm_escape_emit(dec4v_ctx *c, unsigned int sym, unsigned int flag);
unsigned int d4vc_ppm_pair_price(dec4v_ctx *c, unsigned int obj);
void d4vc_ppm_touch_len(dec4v_ctx *c, unsigned int len, unsigned int flag);
void d4vc_ppm_rescale_orders(dec4v_ctx *c, unsigned int sym);
void d4vc_ppm_list_update_32(dec4v_ctx *c, unsigned int sym);
void d4vc_ppm_list_update_20(dec4v_ctx *c, unsigned int sym);
unsigned int d4vc_ppm_len_decode(dec4v_ctx *c, unsigned int len);
int d4vc_ppm_window_decode(dec4v_ctx *c, unsigned int a);
void d4vc_s4_reset_tables(dec4v_ctx *c);
/* S4 leaves (stubs below until draft_s4a/s4b land) */
unsigned int d4vc_s4_score_candidate(dec4v_ctx *c, unsigned int a, unsigned int d,
                        unsigned int e, unsigned int b);
unsigned int d4vc_pick_model_order(dec4v_ctx *c, unsigned int base, unsigned int count);
unsigned int d4vc_score_adaptive_order(dec4v_ctx *c, unsigned int a, unsigned int d,
                        unsigned int b, unsigned int e);
void d4vc_s4_pick_predictor(dec4v_ctx *c);
void d4vc_s4_update_model(dec4v_ctx *c, unsigned int sym, unsigned int arg);
void d4vc_ensure_input(dec4v_ctx *c, unsigned int n);
void d4vc_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4vc_rescale_s4_pair(dec4v_ctx *c, unsigned int a);
/* init helpers (defined in base, called by the 415130 draft) */
uint32_t d4vc_alloc(dec4v_ctx *c, uint32_t size);
void d4vc_prices(dec4v_ctx *c);
void d4vc_fill16(dec4v_ctx *c, unsigned int count, unsigned int dst,
                unsigned int val);
int d4vc_init_file(dec4v_ctx *c, int method, unsigned int al_in);
void d4vc_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel);
void d4vc_rescale_s4_pair(dec4v_ctx *c, unsigned int a);

extern const unsigned char d4v_t43409C[256];
extern const unsigned char d4v_t43419C[256];
extern const unsigned char d4v_t43431E[256];
extern const unsigned char d4v_t434328[256];
extern const unsigned char d4v_t434338[256];
extern const unsigned char d4v_t434438[256];
extern const unsigned char d4v_t436450[256];
extern const unsigned char d4v_t43691C[256];
extern const unsigned char d4v_t433FCC[256];
extern const unsigned char d4v_t43404C[256];
extern const unsigned char d4v_t436254[3061];
extern const unsigned char d4v_t436B8A[256];
extern const unsigned int d4v_t4342EC[256];
extern const unsigned int d4v_t43429C[256];
extern const unsigned int d4v_t4342C4[256];
extern const unsigned int d4v_t434D50[256];
extern const unsigned int d4v_t435150[256];
extern const unsigned int d4v_t435550[256];
extern const unsigned int d4v_t435950[256];
extern const unsigned int d4v_t435D50[256];
extern const unsigned int d4v_t4362C0[256];
extern const unsigned int d4v_t434538[6];
extern const unsigned int d4v_t434550[512];
extern const unsigned int d4v_t433B90[5];
extern const unsigned int d4v_t433BA4[17];
extern const unsigned int d4v_t433E8C[256];
extern const unsigned char d4v_t434314[256];
extern const unsigned char d4v_t433E77[766];
extern const unsigned char d4v_t4150d0[11];
extern const unsigned char d4v_t436ED8[512];
extern const unsigned char d4v_t433BE8[656];

#endif
