/* UHARC 0.3 decompressor core 5 (PPM/RLE, methods 36..79).
 *
 * Clean portable C89 port of the UHARC 0.3 ("uharc_u.exe") core-5 decoder.
 * Every routine below was transliterated from the uharc_u.lst assembly
 * (the Hex-Rays output is wrong in several places; the listing was used).
 *
 * Original -> port mapping (uharc_u.lst):
 *   sub_40F6F0 (main loop)       -> uharc_dec5u_decode
 *   sub_40DC20 (init)            -> uharc_dec5u_init
 *   sub_40E4F4 (free)            -> uharc_dec5u_free
 *   sub_40FC2C (decision)        -> d5_decision
 *   sub_40F66C (decision upd)    -> d5_litm_update
 *   sub_40FCF0 (model switch)    -> d5_switch
 *   sub_40FEF8 (literal)         -> d5_literal
 *   sub_40FFB4 (match)           -> d5_match
 *   sub_410128 (long run)        -> d5_longrun
 *   sub_40F828 (distance sym)    -> d5_distsym
 *   sub_40F748 (length)          -> d5_length
 *   sub_40F520 (distance upd)    -> d5_dist_update
 *   sub_40F5CC (distance rescue) -> d5_dist_rescue
 *   sub_40F32C (length upd)      -> d5_len_update
 *   sub_40F3C0 (short dist upd)  -> d5_sdist_update
 *   sub_40FAEC (ctx symbol)      -> d5_sym
 *   sub_40FB88 (ctx symbol hi)   -> d5_sym_half
 *   sub_40F1F0 (model update)    -> d5_update
 *   sub_40F0DC (rescale)         -> d5_rescale
 *   sub_40F03C (rescale n)       -> d5_rescale_n
 *   sub_42335C (coder init)      -> d5_rc_init
 *   sub_423188 (decode point)    -> d5_point
 *   sub_423140 (narrow)          -> d5_narrow
 *   sub_41E2B0 (input byte)      -> d5_get
 *
 * The 0.3 core-5 model: a 3-way decision (switch model / match / literal)
 * with 8 three-bit history states; a PPM-style context model with six
 * context groupings (512/32/32/96/64/512 contexts), a 61-symbol distance
 * model, a 21/22-symbol length model, a short-distance model and a
 * 16-symbol long-run model.  Literals are delta-coded against a previous
 * byte selected by the current grouping.
 *
 * Return codes: 0 ok, 1 bad args, 2 nomem, 3 input short, 4 output short,
 * 7 data error (corrupt stream).
 */
#include <stdlib.h>
#include <string.h>

#include "uharc_dec.h"

int uharc_dec5u_init(void **pctx, int method);
int uharc_dec5u_decode(void *ctx, uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len);
void uharc_dec5u_free(void *ctx);

#define D5_M32 0xFFFFFFFFUL
#define D5_M16 0xFFFFU

/* Extracted from the uharc_u.lst DGROUP image. */
static const unsigned long d5_thresh[57] = {
    0xFFFFFFFFUL, 0x00780000UL, 0x00700000UL, 0x00680000UL, 0x00600000UL, 0x00580000UL, 0x00500000UL, 0x00480000UL,
    0x00400000UL, 0x003C0000UL, 0x00380000UL, 0x00340000UL, 0x00300000UL, 0x002C0000UL, 0x00280000UL, 0x00240000UL,
    0x00200000UL, 0x001E0000UL, 0x001C0000UL, 0x001A0000UL, 0x00180000UL, 0x00160000UL, 0x00140000UL, 0x00120000UL,
    0x00100000UL, 0x000E0000UL, 0x000C0000UL, 0x000A0000UL, 0x00080000UL, 0x00060000UL, 0x00040000UL, 0x00030000UL,
    0x00020000UL, 0x00018000UL, 0x00010000UL, 0x0000C000UL, 0x00008000UL, 0x00006000UL, 0x00004000UL, 0x00003000UL,
    0x00002000UL, 0x00001800UL, 0x00001000UL, 0x00000C00UL, 0x00000800UL, 0x00000600UL, 0x00000400UL, 0x00000300UL,
    0x00000200UL, 0x00000100UL, 0x00000080UL, 0x00000040UL, 0x00000020UL, 0x00000010UL, 0x00000008UL, 0x00000004UL,
    0x00000002UL,
};
static const unsigned long d5_dbase[60] = {
    0x00780000UL, 0x00700000UL, 0x00680000UL, 0x00600000UL, 0x00580000UL, 0x00500000UL, 0x00480000UL, 0x00400000UL,
    0x003C0000UL, 0x00380000UL, 0x00340000UL, 0x00300000UL, 0x002C0000UL, 0x00280000UL, 0x00240000UL, 0x00200000UL,
    0x001E0000UL, 0x001C0000UL, 0x001A0000UL, 0x00180000UL, 0x00160000UL, 0x00140000UL, 0x00120000UL, 0x00100000UL,
    0x000E0000UL, 0x000C0000UL, 0x000A0000UL, 0x00080000UL, 0x00060000UL, 0x00040000UL, 0x00030000UL, 0x00020000UL,
    0x00018000UL, 0x00010000UL, 0x0000C000UL, 0x00008000UL, 0x00006000UL, 0x00004000UL, 0x00003000UL, 0x00002000UL,
    0x00001800UL, 0x00001000UL, 0x00000C00UL, 0x00000800UL, 0x00000600UL, 0x00000400UL, 0x00000300UL, 0x00000200UL,
    0x00000100UL, 0x00000080UL, 0x00000040UL, 0x00000020UL, 0x00000010UL, 0x00000008UL, 0x00000004UL, 0x00000002UL,
    0x00000001UL, 0x00000003UL, 0x00000001UL, 0x00000000UL,
};
static const unsigned long d5_drange[60] = {
    0x00080000UL, 0x00080000UL, 0x00080000UL, 0x00080000UL, 0x00080000UL, 0x00080000UL, 0x00080000UL, 0x00080000UL,
    0x00040000UL, 0x00040000UL, 0x00040000UL, 0x00040000UL, 0x00040000UL, 0x00040000UL, 0x00040000UL, 0x00040000UL,
    0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL,
    0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00020000UL, 0x00010000UL, 0x00010000UL,
    0x00008000UL, 0x00008000UL, 0x00004000UL, 0x00004000UL, 0x00002000UL, 0x00002000UL, 0x00001000UL, 0x00001000UL,
    0x00000800UL, 0x00000800UL, 0x00000400UL, 0x00000400UL, 0x00000200UL, 0x00000200UL, 0x00000100UL, 0x00000100UL,
    0x00000100UL, 0x00000080UL, 0x00000040UL, 0x00000020UL, 0x00000010UL, 0x00000008UL, 0x00000004UL, 0x00000002UL,
    0x00000000UL, 0x00000000UL, 0x00000002UL, 0x00000000UL,
};
static const unsigned char d5_lit0[16] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
};
static const unsigned char d5_lit1[256] = {
    0x00, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0A, 0x0A,
    0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
    0x0A, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F,
};
static const unsigned char d5_lit3[256] = {
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
    0x15, 0x16, 0x16, 0x16, 0x17, 0x17, 0x18, 0x18, 0x19, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
};
static const unsigned char d5_subrescale[96] = {
    0x00, 0x00, 0x01, 0x02, 0x02, 0x00, 0x00, 0x01, 0x04, 0x02, 0x00, 0x00, 0x06, 0x04, 0x02, 0x00,
    0x00, 0x00, 0x01, 0x02, 0x02, 0x00, 0x00, 0x01, 0x04, 0x02, 0x00, 0x00, 0x06, 0x04, 0x02, 0x00,
    0x00, 0x00, 0x01, 0x02, 0x02, 0x00, 0x00, 0x01, 0x04, 0x02, 0x00, 0x00, 0x06, 0x04, 0x02, 0x00,
    0x00, 0x00, 0x01, 0x02, 0x02, 0x00, 0x00, 0x01, 0x04, 0x02, 0x00, 0x00, 0x06, 0x04, 0x02, 0x00,
    0x00, 0x00, 0x01, 0x02, 0x02, 0x00, 0x00, 0x01, 0x04, 0x02, 0x00, 0x00, 0x06, 0x04, 0x02, 0x00,
    0x00, 0x00, 0x01, 0x02, 0x02, 0x00, 0x00, 0x01, 0x04, 0x02, 0x00, 0x00, 0x06, 0x04, 0x02, 0x00,
};
static const unsigned char d5_lenbase[16] = {
    0xFF, 0xBF, 0x7F, 0x5F, 0x3F, 0x2F, 0x1F, 0x17, 0x13, 0x0F, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
};
static const unsigned char d5_lenbits[16] = {
    0x00, 0x40, 0x40, 0x20, 0x20, 0x10, 0x10, 0x08, 0x04, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const unsigned short d5_lenmodel[23] = {
    0x0000, 0x0010, 0x0018, 0x0028, 0x0038, 0x0058, 0x007C, 0x00CC, 0x0130, 0x018C, 0x0244, 0x02EF,
    0x0401, 0x04C9, 0x05CD, 0x071F, 0x08E1, 0x0B39, 0x0E59, 0x1241, 0x17B9, 0x2119, 0x2710,
};
static const unsigned short d5_shortdist[14] = {
    0x0000, 0x0190, 0x0352, 0x0546, 0x076C, 0x09C4, 0x0C4E, 0x0F0A, 0x11F8, 0x1518, 0x186A, 0x1BEE,
    0x1FA4, 0x238C,
};
static const unsigned short d5_smalldist[3] = {
    0x0000, 0x1194, 0x238C,
};
static const unsigned short d5_longmodel[16] = {
    0x0000, 0x0001, 0x0002, 0x0003, 0x0005, 0x0007, 0x0009, 0x000C, 0x0011, 0x0018, 0x0021, 0x002C,
    0x0039, 0x0049, 0x0059, 0x0063,
};
/* contexts per group (4296DC) */
static const unsigned short d5_nctx[6] = { 512, 32, 32, 96, 64, 512 };
/* count increment by sub-order (4296D4) */
static const unsigned short d5_inc[4] = { 2, 8, 32, 128 };
/* recent-distance slots by method family (42977C) */
static const unsigned short d5_mrun[4] = { 0, 5, 8, 16 };
/* Fenwick seed: total 256 then lowbit(i) (42C9B0) */
static const unsigned short d5_seed[256] = {
    0x0100, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001, 0x0008, 0x0001, 0x0002, 0x0001,
    0x0004, 0x0001, 0x0002, 0x0001, 0x0010, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001,
    0x0008, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001, 0x0020, 0x0001, 0x0002, 0x0001,
    0x0004, 0x0001, 0x0002, 0x0001, 0x0008, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001,
    0x0010, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001, 0x0008, 0x0001, 0x0002, 0x0001,
    0x0004, 0x0001, 0x0002, 0x0001, 0x0040, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001,
    0x0008, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001, 0x0010, 0x0001, 0x0002, 0x0001,
    0x0004, 0x0001, 0x0002, 0x0001, 0x0008, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001,
    0x0020, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001, 0x0008, 0x0001, 0x0002, 0x0001,
    0x0004, 0x0001, 0x0002, 0x0001, 0x0010, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001,
    0x0008, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001, 0x0080, 0x0001, 0x0002, 0x0001,
    0x0004, 0x0001, 0x0002, 0x0001, 0x0008, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001,
    0x0010, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001, 0x0008, 0x0001, 0x0002, 0x0001,
    0x0004, 0x0001, 0x0002, 0x0001, 0x0020, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001,
    0x0008, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001, 0x0010, 0x0001, 0x0002, 0x0001,
    0x0004, 0x0001, 0x0002, 0x0001, 0x0008, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001,
    0x0040, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001, 0x0008, 0x0001, 0x0002, 0x0001,
    0x0004, 0x0001, 0x0002, 0x0001, 0x0010, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001,
    0x0008, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001, 0x0020, 0x0001, 0x0002, 0x0001,
    0x0004, 0x0001, 0x0002, 0x0001, 0x0008, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001,
    0x0010, 0x0001, 0x0002, 0x0001, 0x0004, 0x0001, 0x0002, 0x0001, 0x0008, 0x0001, 0x0002, 0x0001,
    0x0004, 0x0001, 0x0002, 0x0001,
};
/* initial 61-symbol distance model (429324/42938C): only 52..60 */
static const unsigned short d5_distseed[62] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x01FF, 0x03FF, 0x05FF, 0x07FF, 0x09FF, 0x0BFF, 0x0DFF,
    0x0FFF, 0x1000,
};


typedef struct dec5u_s {
    /* callbacks / buffered I/O */
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
    unsigned long out_len;
    int err;

    /* range coder (4450F0..4450FC) */
    unsigned long unit;
    unsigned long code;
    unsigned long rng;
    unsigned cur;

    /* window / output state */
    unsigned char *dict;       /* 428F74 */
    unsigned long dict_sz;     /* 437010 */
    unsigned long dict_mask;   /* 437018 */
    unsigned long dict_pos;    /* 43700C */
    unsigned long hist;        /* 437014: usable history length */
    unsigned long hist_max;    /* 437008: dict_sz - 511 */

    /* the six context models (429698 / 4296B0) */
    unsigned short *fen[6];
    unsigned short *raw[6];
    unsigned short *cfen;      /* 429690 */
    unsigned short *craw;      /* 429694 */
    unsigned long ctx;         /* 436FC4 */

    /* context selection state */
    unsigned char group;       /* 437097 */
    unsigned char lag;         /* 437089 */
    unsigned char sub;         /* 43708C */
    unsigned char psub;        /* 43708F */
    unsigned char lit0mode;    /* 437094 */
    unsigned char lithmode;    /* 437096 */
    unsigned char sign;        /* 437098 */
    unsigned char b099;        /* 437099 */
    unsigned char dmode;       /* 4370A6 */
    unsigned char length;      /* 4370A7 */
    unsigned long lag4;        /* 436F40 */
    unsigned long fe4;         /* 436FE4 */
    unsigned long fe8;         /* 436FE8 */
    unsigned long dval;        /* 43701C */

    /* recent distance list (42968C) and its limits */
    unsigned long *mru;
    unsigned mru_n;            /* 43709B */
    unsigned mru_max;          /* 43709C */
    unsigned dstate;           /* 4293A0 */

    /* auxiliary models */
    unsigned short lenA[23];   /* 437058 */
    unsigned short lenB[23];   /* 43702A */
    unsigned short dist61[62]; /* 436F48 (+ total at [61]) */
    unsigned short sdist[14];  /* 436FC8 */
    unsigned short smalld[3];  /* 437024 */
    unsigned short longm[16];  /* 436EF0 */
    unsigned short litm[32];   /* 436EB0: 8 x 4 */
    unsigned char lhist;       /* 43708A */
} dec5u;

/* ---- buffered input (sub_41E2B0) ---- */
static int d5_get(dec5u *d)
{
    int n;
    if (d->err) return 0;
    if (d->in_pos >= d->in_avail) {
        n = d->rcb(d->rctx, d->inb, d->in_cap);
        if (n <= 0) {
            d->err = 3;
            return 0;
        }
        d->in_avail = (unsigned)n;
        d->in_pos = 0;
    }
    return d->inb[d->in_pos++];
}

/* ---- buffered output ---- */
static void d5_put1(dec5u *d, unsigned b)
{
    if (d->out_total < d->out_len) {
        d->outb[d->out_fill++] = (unsigned char)b;
        if (d->out_fill == d->out_cap) {
            if (d->wcb(d->wctx, d->outb, d->out_fill) != 0) d->err = 4;
            d->out_fill = 0;
        }
    }
    d->out_total++;
}

static void d5_putn(dec5u *d, unsigned b, unsigned long n)
{
    while (n--) d5_put1(d, b);
}

/* ---- range coder (sub_42335C / sub_423188 / sub_423140) ---- */
static void d5_rc_init(dec5u *d)
{
    int b;
    d5_get(d);                 /* discarded: ~out_len marker byte */
    b = d5_get(d);
    d->cur = (unsigned)b;
    d->rng = 0x80UL;
    d->code = (unsigned long)((unsigned)b >> 1);
}

static void d5_renorm(dec5u *d)
{
    while (d->rng <= 0x800000UL) {
        unsigned b = (unsigned)d5_get(d);
        d->code = ((d->code << 8) |
                   (unsigned long)((d->cur & 1u) << 7) |
                   (unsigned long)(b >> 1)) & D5_M32;
        d->rng = (d->rng << 8) & D5_M32;
        d->cur = b;
        if (d->err) return;
    }
}

static unsigned d5_point(dec5u *d, unsigned total)
{
    unsigned long unit, p;
    if (d->err) return 0;
    if (d->rng <= 0x800000UL) d5_renorm(d);
    if (d->err) return 0;
    total &= D5_M16;
    if (total == 0) { d->err = 7; return 0; }
    unit = d->rng / total;
    if (unit == 0) { d->err = 7; return 0; }
    d->unit = unit;
    p = d->code / unit;
    if (p >= (unsigned long)total) p = (unsigned long)total - 1;
    return (unsigned)p;
}

static void d5_narrow(dec5u *d, unsigned total, unsigned lo, unsigned hi)
{
    unsigned long u = d->unit;
    unsigned long lp = (u * (unsigned long)lo) & D5_M32;
    d->code = (d->code - lp) & D5_M32;
    if ((unsigned long)hi < (unsigned long)total)
        d->rng = (u * (unsigned long)(hi - lo)) & D5_M32;
    else
        d->rng = (d->rng - lp) & D5_M32;
}

/* ---- model helpers ---- */

/* halve every raw count once, with the original packed-word arithmetic */
static unsigned long d5_halve(unsigned short *raw)
{
    unsigned long total = 0;
    unsigned i;
    for (i = 0; i < 256; i += 8) {
        unsigned long d0 = (unsigned long)raw[i] | ((unsigned long)raw[i + 1] << 16);
        unsigned long d1 = (unsigned long)raw[i + 2] | ((unsigned long)raw[i + 3] << 16);
        unsigned long d2 = (unsigned long)raw[i + 4] | ((unsigned long)raw[i + 5] << 16);
        unsigned long d3 = (unsigned long)raw[i + 6] | ((unsigned long)raw[i + 7] << 16);
        unsigned long h0 = ((d0 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h1 = ((d1 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h2 = ((d2 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        unsigned long h3 = ((d3 + 0x10001UL) & 0xFFFEFFFEUL) >> 1;
        raw[i]     = (unsigned short)(h0 & D5_M16);
        raw[i + 1] = (unsigned short)((h0 >> 16) & D5_M16);
        raw[i + 2] = (unsigned short)(h1 & D5_M16);
        raw[i + 3] = (unsigned short)((h1 >> 16) & D5_M16);
        raw[i + 4] = (unsigned short)(h2 & D5_M16);
        raw[i + 5] = (unsigned short)((h2 >> 16) & D5_M16);
        raw[i + 6] = (unsigned short)(h3 & D5_M16);
        raw[i + 7] = (unsigned short)((h3 >> 16) & D5_M16);
        total = (total + h0 + h1) & D5_M32;
        total = (total + h2 + h3) & D5_M32;
    }
    return total;
}

/* sub_40F0DC: halve one context and rebuild its Fenwick tree */
static void d5_rescale(dec5u *d, unsigned ctx)
{
    unsigned short *raw = d->craw + ctx * 256u;
    unsigned short *fr = d->cfen + ctx * 256u;
    unsigned long total;
    unsigned i, k, j;
    total = d5_halve(raw);
    for (i = 1; i < 256; i++) fr[i] = raw[i];
    fr[0] = (unsigned short)(((total & D5_M16) + ((total >> 16) & D5_M16)) & D5_M16);
    for (k = 1; k <= 64; k *= 2) {
        j = 256 - 2 * k;
        for (;;) {
            fr[j] = (unsigned short)(fr[j] + fr[j - k]);
            if (j == 2 * k) break;
            j -= 2 * k;
        }
    }
}

/* sub_40F03C: halve a context (times-1) additional times, then rebuild */
static void d5_rescale_n(dec5u *d, unsigned ctx, unsigned times)
{
    unsigned short *raw = d->craw + ctx * 256u;
    unsigned char t = (unsigned char)times;
    for (;;) {
        t = (unsigned char)(t - 1);
        if (t == 0) break;
        d5_halve(raw);
    }
    d5_rescale(d, ctx);
}

/* sub_40F1F0: update one context with symbol sym, select the next context */
static void d5_update(dec5u *d, unsigned sym)
{
    unsigned ctx = (unsigned)d->ctx;
    unsigned short *fr = d->cfen + ctx * 256u;
    unsigned short *raw = d->craw + ctx * 256u;
    unsigned inc = d5_inc[d->sub];
    unsigned v;
    raw[sym] = (unsigned short)(raw[sym] + inc);
    v = (sym + 1u) & 0xFFu;
    if (v) {
        for (;;) {
            fr[v] = (unsigned short)(fr[v] + inc);
            v = (v + (v & ((0u - v) & 0xFFu))) & 0xFFu;
            if (v == 0) break;
        }
    }
    fr[0] = (unsigned short)(fr[0] + inc);
    if (fr[0] >= 0x4000u) d5_rescale(d, ctx);

    if (d->group == 0 || d->group == 5) {
        if (d->b099)
            ctx = ((((ctx & 0xC0u) == (sym & 0xC0u)) ? 1u : 0u) << 8) | sym;
        else
            ctx = sym;
    } else if (d->group == 1) {
        ctx = (unsigned)d5_lit0[ctx & 0xFu] * 16u + (unsigned)d5_lit1[sym];
    } else if (d->group == 3) {
        ctx = (unsigned)(d->fe8 % 3u) * 32u + (unsigned)d5_lit3[sym];
    } else {
        ctx = ((unsigned)(d->lag & d->fe8) & 0xFFu) * 16u + (unsigned)d5_lit1[sym];
    }
    d->fe8 = (d->fe8 + 1UL) & D5_M32;
    d->ctx = ctx;
}

/* ---- symbol decoders ---- */

/* sub_40FAEC: decode a symbol in the current PPM context */
static unsigned d5_sym(dec5u *d, unsigned ctx)
{
    unsigned short *base = d->cfen + ctx * 256u;
    unsigned short *p = base;
    unsigned total = base[0];
    unsigned point, rem, sym, dl, di, lo, hi;
    point = d5_point(d, total);
    if (d->err) return 0;
    rem = (total - point) & D5_M16;
    dl = 0x80u;
    for (;;) {
        di = p[dl];
        if (rem > di) {
            p += dl;
            rem = (rem - di) & D5_M16;
        }
        dl >>= 1;
        if (dl == 0) break;
    }
    sym = (unsigned)(p - base);
    hi = (point + rem) & D5_M16;
    lo = (hi - d->craw[ctx * 256u + sym]) & D5_M16;
    d5_narrow(d, total, lo, hi);
    return sym;
}

/* sub_40FB88: decode a symbol in the upper half (128..255) of a context */
static unsigned d5_sym_half(dec5u *d, unsigned ctx)
{
    unsigned short *base = d->cfen + ctx * 256u;
    unsigned short *half = base + 128;
    unsigned short *p = half;
    unsigned total = (unsigned)((base[0] - base[128]) & D5_M16);
    unsigned point, rem, sym, dl, di, lo, hi;
    point = d5_point(d, total);
    if (d->err) return 0;
    rem = (total - point) & D5_M16;
    dl = 0x40u;
    for (;;) {
        di = p[dl];
        if (rem > di) {
            p += dl;
            rem = (rem - di) & D5_M16;
        }
        dl >>= 1;
        if (dl == 0) break;
    }
    sym = (unsigned)(p - half) + 0x80u;
    hi = (point + rem) & D5_M16;
    lo = (hi - d->craw[ctx * 256u + sym]) & D5_M16;
    d5_narrow(d, total, lo, hi);
    return sym;
}

/* sub_40F66C: update the 3-way decision model */
static void d5_litm_update(dec5u *d, unsigned sym)
{
    unsigned short *p = d->litm + 4u * (unsigned)d->lhist;
    unsigned v3, v4, v5, v6;
    if (sym == 1) p[2] = (unsigned short)(p[2] + 64);
    p[3] = (unsigned short)(p[3] + 64);
    if (p[3] >= 0x4000u) {
        v3 = p[2];
        v5 = (unsigned)((v3 - p[1] + 1) & D5_M16);
        v6 = (unsigned)(((unsigned)(p[3] - v3) + 1) >> 1);
        v4 = (unsigned)(p[1] + (v5 >> 1));
        p[2] = (unsigned short)v4;
        p[3] = (unsigned short)((v4 + v6) & D5_M16);
    }
    d->lhist = (unsigned char)((sym + 2u * (unsigned)d->lhist - 1u) & 7u);
}

/* sub_40FC2C: decode the 3-way decision */
static unsigned d5_decision(dec5u *d)
{
    unsigned short *e = d->litm + 4u * (unsigned)d->lhist;
    unsigned total = e[3];
    unsigned point, sym, dl, lo, hi;
    point = d5_point(d, total);
    if (d->err) return 0;
    dl = 3;
    for (;;) {
        dl--;
        sym = e[dl];
        if (point >= sym || dl == 0) break;
    }
    lo = e[dl];
    hi = e[dl + 1];
    d5_narrow(d, total, lo, hi);
    if (dl == 0) return 0;
    if (dl <= 2) {
        d5_litm_update(d, dl);
        return dl;
    }
    d->err = 7;
    return 0;
}

/* ---- literals ---- */

/* sub_40EEFC: output one literal symbol (delta transform + dict write) */
static unsigned d5_out_sym(dec5u *d, unsigned sym)
{
    unsigned v = (~sym) & 0xFFu;
    unsigned x = v;
    if (d->group) {
        if (!d->sign) x = (0u - v) & 0xFFu;
        if (v >= 0x80u) d->sign = (unsigned char)(d->sign ? 0 : 1);
        x = (x + ((unsigned)((d->lag4 >> (8u * (unsigned)d->lag)) & 0xFFu))) & 0xFFu;
    }
    d->lag4 = ((d->lag4 << 8) | (unsigned long)x) & D5_M32;
    d5_put1(d, x);
    return x;
}

/* sub_40EF98: emit one raw byte from a match and derive its model symbol */
static void d5_out_byte(dec5u *d, unsigned b)
{
    unsigned v;
    if (d->group) {
        unsigned lagb = (unsigned)((d->lag4 >> (8u * (unsigned)d->lag)) & 0xFFu);
        v = (b - lagb) & 0xFFu;
        if (!d->sign) v = (0u - v) & 0xFFu;
        if (v >= 0x80u) d->sign = (unsigned char)(d->sign ? 0 : 1);
        d->dmode = (unsigned char)((~v) & 0xFFu);
    } else {
        d->dmode = (unsigned char)((~b) & 0xFFu);
    }
    d->lag4 = ((d->lag4 << 8) | (unsigned long)b) & D5_M32;
    d5_put1(d, b);
}

/* sub_40FEF8: decode and emit one literal */
static void d5_literal(dec5u *d)
{
    unsigned sym;
    if (d->lit0mode) {
        unsigned long p = d5_point(d, 256);
        if (d->err) return;
        sym = (unsigned)p;
        d5_narrow(d, 256, sym, sym + 1);
    } else if (d->lithmode) {
        sym = d5_sym_half(d, (unsigned)d->ctx);
        if (d->err) return;
    } else {
        sym = d5_sym(d, (unsigned)d->ctx);
        if (d->err) return;
    }
    {
        unsigned ob;
        d5_update(d, sym);
        ob = d5_out_sym(d, sym);
        d->dict[d->dict_pos] = (unsigned char)ob;
    }
    if (d->hist < d->hist_max) d->hist++;
    d->dict_pos = (d->dict_pos + 1UL) & d->dict_mask;
}

/* ---- distance model ---- */

/* sub_40F520: update the 61-symbol distance model */
static void d5_dist_update(dec5u *d, unsigned sym)
{
    unsigned short *p = d->dist61;
    unsigned i, total;
    unsigned inc;
    if (sym == 60) {
        inc = 4;
    } else {
        i = sym + 1u;
        while (i < 61u) { p[i] = (unsigned short)(p[i] + 32); i++; }
        inc = 32;
    }
    total = (unsigned)(p[61] + inc) & D5_M16;
    p[61] = (unsigned short)total;
    if (total >= 0x4000u) {
        unsigned char dl = 61;
        while (dl) {
            unsigned short a = p[dl - 1];
            unsigned diff = (unsigned)((p[dl] - a) + 1);
            p[dl] = (unsigned short)((diff >> 1) & D5_M16);
            dl--;
        }
        for (i = 2; i <= 61u; i++)
            p[i] = (unsigned short)(p[i] + p[i - 1]);
    }
}

/* sub_40F5CC: make symbol sym available as history grows */
static void d5_dist_rescue(dec5u *d, unsigned sym)
{
    unsigned short *p = d->dist61;
    unsigned i;
    if (sym <= 61u) {
        unsigned v = (unsigned)(((unsigned)p[sym + 1] + 1) >> 1);
        i = sym;
        while (i <= 61u) { p[i] = (unsigned short)(p[i] + v); i++; }
    }
    {
        unsigned char dl = 61;
        while (dl) {
            unsigned short a = p[dl - 1];
            unsigned diff = (unsigned)((p[dl] - a) + 1);
            p[dl] = (unsigned short)((diff >> 1) & D5_M16);
            dl--;
        }
    }
    for (i = 2; i <= 61u; i++)
        p[i] = (unsigned short)(p[i] + p[i - 1]);
}

/* sub_40F828: decode the distance symbol; returns 0..60 */
static unsigned d5_distsym(dec5u *d)
{
    unsigned short *p = d->dist61;
    unsigned total = p[61];
    unsigned point, dl, lo, hi;
    point = d5_point(d, total);
    if (d->err) return 0;
    dl = 61;
    for (;;) {
        dl--;
        if (point >= p[dl] || dl == 0) break;
    }
    lo = p[dl];
    hi = p[dl + 1];
    d5_narrow(d, total, lo, hi);
    d->dval = (dl < 60u) ? d5_dbase[dl] : 0UL;
    if (dl == 60u) return 60;

    if (dl == 56u) { d->dmode = 0; return 56; }
    if (dl == 57u) {
        unsigned m = d->mru_max;
        unsigned t2 = d->sdist[m];
        unsigned p2 = d5_point(d, t2);
        unsigned idx;
        if (d->err) return 0;
        idx = m;
        for (;;) {
            idx--;
            if (p2 >= d->sdist[idx] || idx == 0) break;
        }
        d5_narrow(d, t2, d->sdist[idx], d->sdist[idx + 1]);
        d->dval += (unsigned long)(m - 1u - idx);
        d->dmode = 1;
        return 57;
    }
    if (dl == 58u) {
        unsigned t2 = d->smalld[2];
        unsigned p2 = d5_point(d, t2);
        unsigned idx;
        if (d->err) return 0;
        idx = 2;
        for (;;) {
            idx--;
            if (p2 >= d->smalld[idx] || idx == 0) break;
        }
        d5_narrow(d, t2, d->smalld[idx], d->smalld[idx + 1]);
        d->dval += (unsigned long)(1u - idx);
        d->dmode = 1;
        return 58;
    }
    if (dl == 59u) { d->dmode = 1; return 59; }

    {
        unsigned long n = d5_drange[dl];
        if (n + d->dval - 1UL > d->hist)
            n = d->hist - d->dval + 1UL;
        if (n >= 2UL) {
            if (n > 0x1000UL) {
                unsigned long t2 = ((n - 1UL) >> 12) + 1UL;
                unsigned p2 = d5_point(d, (unsigned)t2);
                if (d->err) return 0;
                d5_narrow(d, (unsigned)t2, p2, p2 + 1);
                d->dval = (d->dval + ((unsigned long)p2 << 12)) & D5_M32;
                if ((unsigned long)(p2 + 1) == t2) {
                    n = (n - ((unsigned long)p2 << 12)) & D5_M32;
                } else {
                    n = 0x1000UL;
                }
            }
            if (n >= 2UL) {
                unsigned p2 = d5_point(d, (unsigned)n);
                if (d->err) return 0;
                d5_narrow(d, (unsigned)n, p2, p2 + 1);
                d->dval = (d->dval + p2) & D5_M32;
            }
        }
    }
    d->dmode = 0;
    return dl;
}

/* sub_40F748: decode the match length symbol */
static unsigned d5_length(dec5u *d)
{
    unsigned short *p = (d->dmode == 0) ? d->lenA : d->lenB;
    unsigned c = (d->dmode == 0) ? 21u : 22u;
    unsigned total = p[c];
    unsigned point, dl, lo, hi, bits, off;
    point = d5_point(d, total);
    if (d->err) return 0;
    dl = c;
    for (;;) {
        dl--;
        if (point >= p[dl] || dl == 0) break;
    }
    lo = p[dl];
    hi = p[dl + 1];
    d5_narrow(d, total, lo, hi);
    bits = d5_lenbits[dl];
    off = 0;
    if (bits) {
        off = d5_point(d, bits);
        if (d->err) return 0;
        d5_narrow(d, bits, off, off + 1);
    }
    d->length = (unsigned char)((unsigned)d5_lenbase[dl] + off);
    return dl;
}

/* sub_40F32C: update the length model */
static void d5_len_update(dec5u *d, unsigned sym)
{
    unsigned short *p = (d->dmode == 0) ? d->lenA : d->lenB;
    unsigned c = (d->dmode == 0) ? 21u : 22u;
    unsigned i;
    i = sym + 1u;
    while (i < c) { p[i] = (unsigned short)(p[i] + 64); i++; }
    p[c] = (unsigned short)(p[c] + 64);
    if (p[c] >= 0x4000u) {
        unsigned char dl = (unsigned char)c;
        while (dl) {
            unsigned short a = p[dl - 1];
            unsigned diff = (unsigned)((p[dl] - a) + 1);
            p[dl] = (unsigned short)((diff >> 1) & D5_M16);
            dl--;
        }
        for (i = 2; i <= c; i++)
            p[i] = (unsigned short)(p[i] + p[i - 1]);
    }
}

/* sub_40F3C0: update the short-distance model */
static void d5_sdist_update(dec5u *d, unsigned sym)
{
    unsigned short *p = d->sdist;
    unsigned c = d->mru_max;
    unsigned i;
    if (sym < 3u) {
        if (sym == 2u) p[1] = (unsigned short)(p[1] + 32);
        p[2] = (unsigned short)(p[2] + 32);
        if (p[2] >= 0x4000u) {
            unsigned d1 = (unsigned)(((unsigned)p[1] - p[0] + 1) >> 1);
            unsigned d2 = (unsigned)(((unsigned)p[2] - p[1] + 1) >> 1);
            if (d1 < 0x400u) d1 = 0x400u;
            if (d2 < 0x400u) d2 = 0x400u;
            p[1] = (unsigned short)(p[0] + d1);
            p[2] = (unsigned short)(p[1] + d2);
        }
    } else {
        unsigned a = (unsigned)d->mru_n - 1u - sym;
        for (;;) {
            a++;
            if (a >= c) break;
            p[a] = (unsigned short)(p[a] + 32);
        }
        p[c] = (unsigned short)(p[c] + 32);
        if (p[c] >= 0x4000u) {
            unsigned char dl = (unsigned char)c;
            while (dl) {
                unsigned short prev = p[dl - 1];
                unsigned diff = (unsigned)((p[dl] - prev) + 1);
                unsigned v = diff >> 1;
                if (v < 0x100u) v = 0x100u;
                p[dl] = (unsigned short)v;
                dl--;
            }
            for (i = 2; i <= c; i++)
                p[i] = (unsigned short)(p[i] + p[i - 1]);
        }
    }
}

/* ---- model switch ---- */

/* sub_40FCF0: decode and apply a model-switch symbol (uniform 0..11) */
static void d5_switch(dec5u *d)
{
    unsigned total = 12;
    unsigned point, v, flag1 = 0, flag2 = 0;
    unsigned long i;
    point = d5_point(d, total);
    if (d->err) return;
    v = point;
    d5_narrow(d, total, v, v + 1);
    if (v > 11u) { d->err = 7; return; }
    switch (v) {
    case 0: case 1: case 2: case 3: case 4: case 5:
        flag1 = 1;
        break;
    case 6: case 7: case 8: case 9:
        flag2 = 1;
        break;
    case 10:
        d->lit0mode = (unsigned char)(d->lit0mode ? 0 : 1);
        break;
    case 11:
        d->lithmode = (unsigned char)(d->lithmode ? 0 : 1);
        break;
    default:
        break;
    }

    if (flag1) {
        d->group = (unsigned char)v;
        d->lag = (unsigned char)((v - 1u) & 3u);
        d->sub = 1;
        d->cfen = d->fen[v];
        d->craw = d->raw[v];
        d->ctx = 0;
        if (d->out_total != 0 && (v == 0 || v == 5)) {
            unsigned n = d5_nctx[v];
            for (i = 0; i < n; i++) {
                unsigned short *fr = d->cfen + i * 256u;
                unsigned short *rw = d->craw + i * 256u;
                unsigned j;
                memcpy(fr, d5_seed, 512);
                for (j = 0; j < 256u; j++) rw[j] = 1;
            }
        }
    }

    if (flag2) {
        d->psub = d->sub;
        d->sub = (unsigned char)(v - 6u);
        if (d->out_total != 0) {
            unsigned idx = 16u * (unsigned)d->group + 4u * (unsigned)d->psub
                         + (unsigned)d->sub;
            unsigned times = d5_subrescale[idx];
            if (times) {
                unsigned n = d5_nctx[d->group];
                for (i = 0; i < n; i++) d5_rescale_n(d, (unsigned)i, times);
            }
        }
    }
}

/* ---- long run (symbol 60) ---- */

/* sub_410128: decode and copy a run of the previous byte */
static void d5_longrun(dec5u *d)
{
    unsigned long esi = 0;
    unsigned total = d->longm[15];
    unsigned point, dl, lo, hi;
    unsigned long range;
    unsigned b;
    unsigned long n;

    point = d5_point(d, total);
    if (d->err) return;
    dl = 15;
    for (;;) {
        dl--;
        if (point >= d->longm[dl] || dl == 0) break;
    }
    lo = d->longm[dl];
    hi = d->longm[dl + 1];
    d5_narrow(d, total, lo, hi);

    if (dl == 14) {
        esi = d->fe4;
    } else {
        range = 0x80UL;
        if (dl < 13u) {
            range = 1UL << (19u - dl);
            esi = (esi + range) & D5_M32;
        }
        if (range > 0x2000UL) {
            unsigned long t2 = range >> 12;
            unsigned p2 = d5_point(d, (unsigned)t2);
            if (d->err) return;
            d5_narrow(d, (unsigned)t2, p2, p2 + 1);
            esi = (esi + ((unsigned long)p2 << 12)) & D5_M32;
            range = 0x1000UL;
        }
        {
            unsigned p2 = d5_point(d, (unsigned)range);
            if (d->err) return;
            d5_narrow(d, (unsigned)range, p2, p2 + 1);
            esi = (esi + p2) & D5_M32;
        }
        d->fe4 = esi;
    }

    n = (esi + 0x200UL) & D5_M32;   /* total run length */
    b = (unsigned)d->dict[(d->dict_pos - 1UL) & d->dict_mask];
    if (d->dict_pos + 0x1FFUL < d->dict_sz) {
        memset(d->dict + d->dict_pos, (int)b, 0x1FFu);
        if (d->hist + 0x1FFUL > d->hist_max) d->hist = d->hist_max;
        else d->hist += 0x1FFUL;
        d->dict_pos = (d->dict_pos + 0x1FFUL) & D5_M32;
    } else {
        unsigned long k;
        for (k = 0; k < 0x1FFUL; k++) {
            d->dict[d->dict_pos] = (unsigned char)b;
            if (d->hist < d->hist_max) d->hist++;
            d->dict_pos = (d->dict_pos + 1UL) & d->dict_mask;
        }
    }
    {
        unsigned j;
        for (j = 0; j < 5u; j++) {
            d5_out_byte(d, b);
            d5_update(d, (unsigned)d->dmode);
        }
    }
    n -= 5UL;
    d5_putn(d, b, n);
    {
        unsigned long k;
        for (k = 0; k < n; k++) d5_update(d, (unsigned)d->dmode);
    }
}

/* ---- match (decision 1) ---- */

/* sub_40FFB4: decode and copy a match */
static void d5_match(dec5u *d)
{
    unsigned sym, cls, len, b;
    unsigned long dist, src;
    unsigned i;

    while (d->hist >= d5_thresh[d->dstate]) {
        unsigned old = d->dstate;
        d->dstate--;
        d5_dist_rescue(d, old);
    }

    sym = d5_distsym(d);
    if (d->err) return;
    d5_dist_update(d, sym);
    if (sym == 60u) { d5_longrun(d); return; }

    if (d->dmode == 0) {
        i = (unsigned)d->mru_n - 1u;
        dist = d->dval;
    } else {
        i = (unsigned)(d->dval & 0xFFu);
        if (i >= d->mru_n) { d->err = 7; return; }
        dist = d->mru[i];
        if (i) d5_sdist_update(d, i);
    }
    if (i && dist > 1UL) {
        unsigned j;
        for (j = i; j > 0; j--) d->mru[j] = d->mru[j - 1];
        d->mru[0] = dist;
    }

    src = (d->dict_pos - dist) & d->dict_mask;
    cls = d5_length(d);
    if (d->err) return;
    d5_len_update(d, cls);
    len = (unsigned)d->length;
    if (len < 2u) { d->err = 7; return; }

    for (;;) {
        b = (unsigned)d->dict[src];
        src = (src + 1UL) & d->dict_mask;
        d->dict[d->dict_pos] = (unsigned char)b;
        if (d->hist < d->hist_max) d->hist++;
        d->dict_pos = (d->dict_pos + 1UL) & d->dict_mask;
        d5_out_byte(d, b);
        d5_update(d, (unsigned)d->dmode);
        len--;
        if (len == 0) break;
    }
}

/* ---- public API ---- */

int uharc_dec5u_init(void **pctx, int method)
{
    dec5u *d;
    unsigned long dictsz;
    unsigned g, i;
    if (!pctx) return 1;
    *pctx = 0;
    /* the original folds methods 78/79 onto 36 before sizing the model */
    if (method == 78 || method == 79) method = 36;
    if (method < 36 || method > 79) return 1;
    dictsz = 1UL << (((method - 36) % 14) + 10);

    d = (dec5u *)malloc(sizeof(*d));
    if (!d) return 2;
    memset(d, 0, sizeof(*d));
    for (g = 0; g < 6; g++) {
        d->fen[g] = 0;
        d->raw[g] = 0;
    }
    d->dict = (unsigned char *)malloc((size_t)(dictsz + 512UL));
    if (!d->dict) { uharc_dec5u_free(d); return 2; }
    memset(d->dict, 0, (size_t)(dictsz + 512UL));
    for (g = 0; g < 6; g++) {
        unsigned n = d5_nctx[g];
        d->fen[g] = (unsigned short *)malloc((size_t)n * 512UL);
        d->raw[g] = (unsigned short *)malloc((size_t)n * 512UL);
        if (!d->fen[g] || !d->raw[g]) { uharc_dec5u_free(d); return 2; }
        for (i = 0; i < n; i++) {
            memcpy(d->fen[g] + i * 256u, d5_seed, 512);
            {
                unsigned j;
                for (j = 0; j < 256u; j++) d->raw[g][i * 256u + j] = 1;
            }
        }
    }
    d->mru = (unsigned long *)malloc(16UL * sizeof(unsigned long));
    if (!d->mru) { uharc_dec5u_free(d); return 2; }
    for (i = 0; i < 16; i++) d->mru[i] = D5_M32;

    d->dict_sz = dictsz;
    d->dict_mask = dictsz - 1UL;
    d->dict_pos = 0;
    d->hist = 31;
    d->hist_max = dictsz - 511UL;
    d->cfen = d->fen[0];
    d->craw = d->raw[0];
    d->ctx = 0xFF;
    d->group = 0;
    d->lag = 0;
    d->sub = 3;
    d->psub = 3;
    d->lit0mode = 0;
    d->lithmode = 0;
    d->sign = 1;
    d->b099 = 0;
    d->dmode = 0;
    d->length = 0;
    d->lag4 = D5_M32;
    d->fe4 = 0;
    d->fe8 = 0;
    d->dval = D5_M32;
    d->dstate = 52;
    d->lhist = 7;

    /* mru limits come from the method family */
    {
        unsigned fam;
        if (method <= 49) fam = 1;
        else if (method <= 63) fam = 2;
        else fam = 3;
        d->mru_n = d5_mrun[fam];
        d->mru_max = d->mru_n - 3u;
    }

    memcpy(d->lenA, d5_lenmodel, sizeof(d->lenA));
    memcpy(d->lenB, d5_lenmodel, sizeof(d->lenB));
    memcpy(d->sdist, d5_shortdist, sizeof(d->sdist));
    memcpy(d->smalld, d5_smalldist, sizeof(d->smalld));
    memcpy(d->longm, d5_longmodel, sizeof(d->longm));
    memcpy(d->dist61, d5_distseed, sizeof(d->dist61));
    for (i = 0; i < 8u; i++) {
        d->litm[i * 4u + 0] = 0;
        d->litm[i * 4u + 1] = 1;
        d->litm[i * 4u + 2] = 0x0C00;
        d->litm[i * 4u + 3] = 0x2000;
    }
    *pctx = d;
    return 0;
}

void uharc_dec5u_free(void *ctx)
{
    dec5u *d = (dec5u *)ctx;
    unsigned g;
    if (!d) return;
    for (g = 0; g < 6; g++) {
        free(d->fen[g]);
        free(d->raw[g]);
    }
    free(d->dict);
    free(d->mru);
    free(d);
}

int uharc_dec5u_decode(void *ctx,
                       uharc_dec_read_cb rcb, void *rctx,
                       uharc_dec_write_cb wcb, void *wctx,
                       unsigned long out_len)
{
    dec5u *d = (dec5u *)ctx;
    int err;
    if (!d || !rcb || !wcb) return 1;
    if (out_len == 0) return 0;
    d->inb = (unsigned char *)malloc(65536UL);
    d->outb = (unsigned char *)malloc(65536UL);
    if (!d->inb || !d->outb) {
        free(d->inb);
        free(d->outb);
        d->inb = 0;
        d->outb = 0;
        return 2;
    }
    d->rcb = rcb;
    d->rctx = rctx;
    d->wcb = wcb;
    d->wctx = wctx;
    d->in_cap = 65536;
    d->out_cap = 65536;
    d->in_pos = 0;
    d->in_avail = 0;
    d->out_fill = 0;
    d->out_total = 0;
    d->out_len = out_len;
    d->err = 0;
    d5_rc_init(d);
    while (d->out_total < d->out_len && !d->err) {
        unsigned s = d5_decision(d);
        if (d->err) break;
        if (s == 0) d5_switch(d);
        else if (s == 1) d5_match(d);
        else if (s == 2) d5_literal(d);
        else d->err = 7;
    }
    err = d->err;
    if (err == 0 && d->out_fill) {
        if (wcb(wctx, d->outb, d->out_fill) != 0) err = 4;
    }
    free(d->inb);
    free(d->outb);
    d->inb = 0;
    d->outb = 0;
    return err;
}
