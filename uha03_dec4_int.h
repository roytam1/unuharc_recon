#ifndef UHA03_DEC4_INT_H
#define UHA03_DEC4_INT_H
/* Shared internals for the typed UHARC 0.3 core-4 decoder (uha03_dec4_{clean,lzp,ppm,rc}.c). */
#include <stdlib.h>
#include <string.h>
#include "uha_types.h"
#include "uharc_dec.h"
#include "uha03_dec4_addr.h"
#include "uha03_dec4_state.h"

#define V3_ARENA 0x04000000U


typedef struct dec3w_ctx {
    unsigned char *mem; /* compat, unused */
    unsigned char fx_gap;   /* the arena's zero gap outside the globals */
    unsigned char *io_mem;
    unsigned char *model_mem;
    /* per-block pointers (aliases into model_mem) and bases */
    /* blk00_dict/blk00_base: MODEL-REGION alias, not just the dict block.
     * c->hi is set to D3U_MODEL_BASE before the model blocks are carved and
     * the dict block is the FIRST carve, so blk00_base == D3U_MODEL_BASE
     * and blk00_dict == model_mem.  Hence
     *     blk00_dict[X - blk00_base] == model_mem[X - MODEL_BASE]
     *     == d3u_ptr(c, X)
     * for every model-region address X.  All blk00_dict accesses in this
     * port have been checked to land inside the model region. */
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
    uint32_t hi;
    uharc_dec_read_cb rcb;
    void *rctx;
    uharc_dec_write_cb wcb;
    void *wctx;
    int err;
    int method;
    unsigned char flags2;
    uint32_t in_buf;
    uint32_t out_buf;
    d3u_state st;   /* typed model state */
    /* The raw accesses read and write words straight into the globals
     * array, so a word at its very last byte would run past it.  The
     * pad takes that read; the arena's bytes there are zero, and so
     * are these. */
    unsigned char fx_pad[8];
} dec3w_ctx;

static __attribute__((unused)) unsigned int d3w_o(unsigned int o)
{
    if (o > (unsigned)(V3_ARENA - 8)) o = (unsigned)(V3_ARENA - 8);
    return o;
}
#define V3O(o)  d3w_o((unsigned)(o))
#define G8(a)   (*(unsigned char  *)(void *)d3u_ptr(c, (uint32_t)(a)))
#define M8(o)   (*(unsigned char  *)(void *)d3u_ptr(c, (uint32_t)(o)))
#ifndef U32MASK
#define U32MASK 0xFFFFFFFFU
#endif

#define D3U_FIXED_BASE   0x428000U
#define D3U_FIXED_SIZE   0x40000U
#define D3U_GLOBALS_BASE 0x428000U /* the DGROUP slice */
#define D3U_GLOBALS_END  0x445100U /* end of the last named cell */
#define D3U_IO_BASE    0x3800000U
#define D3U_IO_SIZE    0x510000U
#define D3U_MODEL_BASE 0x00600000U
#define D3U_MODEL_SIZE 0x04000000U

static __attribute__((unused)) unsigned char *d3u_ptr(dec3w_ctx *c, uint32_t addr)
{
    if (addr >= D3U_MODEL_BASE && addr < D3U_MODEL_BASE + D3U_MODEL_SIZE)
        return c->model_mem + (addr - D3U_MODEL_BASE);
    /* The globals are one byte array the typed state owns: the DGROUP
     * slice the decoder loads at 0x428000 and the BSS above it. */
    if (addr >= D3U_GLOBALS_BASE && addr < D3U_GLOBALS_END)
        return c->st.fx_428000 + (addr - D3U_GLOBALS_BASE);
    if (addr >= D3U_IO_BASE && addr < D3U_IO_BASE + D3U_IO_SIZE)
        return c->io_mem + (addr - D3U_IO_BASE);
    /* The arena port clamps every address to the end of its buffer, so
     * an address outside the globals, the I/O buffers and the model
     * blocks reads the arena's zero gap.  fx_gap stands in for it. */
    return &c->fx_gap;
}

/* Boundary-safe byte-wise accessors: fixed-region data may later live in
 * separate layout-backed arrays (fx_*), so multi-byte reads/writes must
 * not assume a contiguous backing buffer at region boundaries. */
/* Lifted tables live in uha03_dec4_seed.c */
extern const unsigned char d3u_t429E30[256];
extern const unsigned char d3u_t429E44[16];
extern const unsigned char d3u_t429E54[256];
extern const unsigned char d3u_t429F54[256];
extern const unsigned char d3u_t42BF6C[256];
extern const unsigned char d3u_t42BD70[3061];
extern const unsigned char d3u_t42C438[256];
extern const unsigned char d3u_t429B98[32];
extern const unsigned char d3u_t429BB8[256];
extern const unsigned char d3u_t429CB8[256];
extern const unsigned int d3u_t429DB8[256];
extern const unsigned int d3u_t429DE0[256];
extern const unsigned int d3u_t429E08[256];
extern const unsigned int d3u_t42A86C[256];
extern const unsigned int d3u_t42AC6C[256];
extern const unsigned int d3u_t42B06C[256];
extern const unsigned int d3u_t42B46C[256];
extern const unsigned int d3u_t42B86C[256];
extern const unsigned int d3u_t42BDDC[256];
extern const unsigned char d3u_t429E3A[16];
extern const unsigned char d3u_t42C6A6[256];
extern const unsigned int d3u_t42A054[6];
extern const unsigned int d3u_t42A06C[512];

static __attribute__((unused)) unsigned int d3u_ld8(dec3w_ctx *c, uint32_t a)
{
    return *d3u_ptr(c, a);
}
static __attribute__((unused)) void d3u_st8(dec3w_ctx *c, uint32_t a, unsigned int v)
{
    *d3u_ptr(c, a) = (unsigned char)v;
}
static __attribute__((unused)) unsigned int d3u_ld16(dec3w_ctx *c, uint32_t a)
{
    return (unsigned int)(*d3u_ptr(c, a) | ((unsigned int)*d3u_ptr(c, a + 1U) << 8));
}
static __attribute__((unused)) void d3u_st16(dec3w_ctx *c, uint32_t a, unsigned int v)
{
    *d3u_ptr(c, a) = (unsigned char)(v & 0xFFU);
    *d3u_ptr(c, a + 1U) = (unsigned char)((v >> 8) & 0xFFU);
}
static __attribute__((unused)) unsigned int d3u_ld32(dec3w_ctx *c, uint32_t a)
{
    return (unsigned int)(*d3u_ptr(c, a)
        | ((unsigned int)*d3u_ptr(c, a + 1U) << 8)
        | ((unsigned int)*d3u_ptr(c, a + 2U) << 16)
        | ((unsigned int)*d3u_ptr(c, a + 3U) << 24));
}
static __attribute__((unused)) void d3u_st32(dec3w_ctx *c, uint32_t a, unsigned int v)
{
    *d3u_ptr(c, a) = (unsigned char)(v & 0xFFU);
    *d3u_ptr(c, a + 1U) = (unsigned char)((v >> 8) & 0xFFU);
    *d3u_ptr(c, a + 2U) = (unsigned char)((v >> 16) & 0xFFU);
    *d3u_ptr(c, a + 3U) = (unsigned char)((v >> 24) & 0xFFU);
}

/* Little-endian u32 read from a lifted const table.  The rep movsd copies
 * at 0x414942/0x4149D9 used to read the slice through d3u_ptr; the source
 * is a read-only table, so the index goes straight into the array. */
static __attribute__((unused)) unsigned int d3u_tbl_ld32(const unsigned char *t, uint32_t i)
{
    return (unsigned int)t[i]
        | ((unsigned int)t[i + 1] << 8)
        | ((unsigned int)t[i + 2] << 16)
        | ((unsigned int)t[i + 3] << 24);
}

/* sub_40C9C0-style byte fill (t1=len, t0=dst, t3=val).
 * Must be storage-aware: a fill can run past the end of the globals
 * array into the arena's zero gap, which the port stands in for with
 * one byte, so it stops there. */

/* Split modules */
void d3uc_fill(dec3w_ctx *c, unsigned int len, unsigned int dst, unsigned int val);
void d3uc_fill16(dec3w_ctx *c, unsigned int t1, unsigned int t3, unsigned int t0,
                 unsigned int t2, unsigned int t4, unsigned int t5);
void d3uc_memmove(dec3w_ctx *c, unsigned int dst, unsigned int src, unsigned int n);
void d3uc_put_byte(dec3w_ctx *c, unsigned int b);
void d3uc_flush_out(dec3w_ctx *c);
extern const unsigned char d3u_t42C9B0[512];
extern const unsigned char d3u_t429A08[384];
unsigned int d3uc_decode_bit_update(dec3w_ctx *c, unsigned int p);
unsigned int d3uc_decode_length_sym(dec3w_ctx *c, unsigned int al_in);
unsigned int d3uc_decode_match(dec3w_ctx *c, unsigned int edi_in);
unsigned int d3uc_decode_offset_ctx(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in);
unsigned int d3uc_decode_offset_sym(dec3w_ctx *c, unsigned int x);
unsigned int d3uc_fold_small_ctx_block(dec3w_ctx *c, unsigned int eax_in);
unsigned int d3uc_get_byte(dec3w_ctx *c);
unsigned int d3uc_lookup_model_entry(dec3w_ctx *c);
unsigned int d3uc_rc_decode(dec3w_ctx *c, unsigned int total);
unsigned int d3uc_refine_match_price(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ecx_in, unsigned int ebx_in);
unsigned int d3uc_update_ctx_coder(dec3w_ctx *c, unsigned int eax_in, unsigned int dl_in);
unsigned int d3uc_update_match_price(dec3w_ctx *c, unsigned int sym);
unsigned int d3uc_walk_match_price(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ecx_in, unsigned int ebx_in);
unsigned int d3uc_walk_price_slots(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in);
void d3uc_ctx_model_update(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v);
void d3uc_decode_fallback(dec3w_ctx *c, unsigned int edi_in);
void d3uc_emit_literal(dec3w_ctx *c, unsigned int sym);
void d3uc_fill_input(dec3w_ctx *c, unsigned int eax_v);
void d3uc_fold_ctx_block(dec3w_ctx *c, unsigned int eax_in);
void d3uc_init_hash_buckets(dec3w_ctx *c);
void d3uc_rc_init(dec3w_ctx *c);
void d3uc_rc_renorm_update(dec3w_ctx *c, unsigned int p);
void d3uc_rc_reset(dec3w_ctx *c);
void d3uc_rc_update(dec3w_ctx *c, unsigned int p);
void d3uc_refill_input(dec3w_ctx *c);
void d3uc_rescale_blk01_model(dec3w_ctx *c);
void d3uc_rescale_blk02_model(dec3w_ctx *c);
void d3uc_topup_dict_rebuild(dec3w_ctx *c);
void d3uc_topup_model_reinit(dec3w_ctx *c);
void d3uc_update_ctx_counter(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v, unsigned int ebx_v);
void d3uc_update_ctx_ema(dec3w_ctx *c, unsigned int edx_v, unsigned int ebx_v);
void d3uc_update_hash_bucket(dec3w_ctx *c);
void d3uc_update_len_model(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v);
void d3uc_update_match_chain(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v);
void d3uc_update_model_8way(dec3w_ctx *c, unsigned int eax_v);
void d3uc_update_order_model_a(dec3w_ctx *c, unsigned int eax_v);
void d3uc_update_order_model_b(dec3w_ctx *c, unsigned int eax_v);
void d3uc_walk_hash_chain(dec3w_ctx *c, unsigned int al_in, unsigned int dl_in);

#endif /* UHA03_DEC4_INT_H */
