#include "uha04_dec4_int.h"

int d4vc_ppm(dec4v_ctx *c)
{
    uint32_t t0 = 0, t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0;
    unsigned char s[11] = {0};
    unsigned int i = 0;

    /* prologue 0x00416220 push t1; 0x00416221 push t2;
     * 0x00416222 push t3; 0x00416223 push t4; 0x00416224 push t5;
     * 0x00416225 push t6; 0x00416226 sub esp,1Ch.
     * Callee-saved regs become C locals; the esp frame is s/w28..w1c. */

    t1 = 8;
    t3 = 1;
    t0 = D4V_CTX_B_45A278;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x20UL;
    t3 = 1;
    t0 = D4V_TBL_45A258;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x400UL;
    t3 = 1;
    t0 = D4V_TBL_45AA80;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x800UL;
    t3 = 1;
    t0 = D4V_TBL_45A280;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x80UL;
    t3 = 1;
    t0 = D4V_TBL_45AE80;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x40000UL;
    t0 = (uint32_t)D4V_BLK_PTR_06_32(&c->st, 0);
    t3 = 0;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x80000UL;
    t0 = (uint32_t)D4V_BLK_PTR_07_32(&c->st, 0);
    t3 = 0;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x80000UL;
    t0 = (uint32_t)D4V_BLK_PTR_08_32(&c->st, 0);
    t3 = 0;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x40000UL;
    t0 = (uint32_t)D4V_BLK_PTR_09_32(&c->st, 0);
    t3 = 0;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x40000UL;
    t0 = (uint32_t)D4V_BLK_PTR_10_32(&c->st, 0);
    t3 = 0;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x40000UL;
    t0 = (uint32_t)D4V_BLK_PTR_11_32(&c->st, 0);
    t3 = 0;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x1C0000UL;
    t0 = (uint32_t)D4V_BLK_PTR_12_32(&c->st, 0);
    t3 = 0;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x40000UL;
    t0 = (uint32_t)D4V_BLK_PTR_13_32(&c->st, 0);
    t3 = 0;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x40600UL;
    t0 = (uint32_t)D4V_BLK_PTR_14_32(&c->st, 0);
    t3 = 0;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x40600UL;
    t0 = (uint32_t)D4V_BLK_PTR_15_32(&c->st, 0);
    t3 = 0;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    {
        unsigned int base;
        unsigned int row;
        unsigned int k;
        unsigned int j;
        unsigned int addr;
        for (base = 0; base < 0x800U; base += 0x100U) {
            for (i = 0; i < 11; i++)
                s[i] = d4v_t4150d0[i];
            row = base;
            for (k = 0; k < 11U; k++) {
                for (j = 0; j < 8U; j++) {
                    if (j < k) {
                        addr = (D4V_TBL_45A280 + row + j * 2U) & U32MASK;
                        c->st.g.bytes[addr - D4V_GLOBALS_BASE] =
                            (unsigned char)((c->st.g.bytes[addr - D4V_GLOBALS_BASE]
                                + s[k - j]) & 0xFFU);
                    } else if (j > k) {
                        unsigned int curv;
                        unsigned int half;
                        half = (unsigned int)s[j - k];
                        half = d4v_sar((half - d4v_sar(half, 31)) & U32MASK, 1);
                        curv = (unsigned int)(*(unsigned char *)(void *)(d4v_fx(c, 238209UL + row + j * 2U)));
                        (*(unsigned char *)(void *)(d4v_fx(c, 238209UL + row + j * 2U))) =
                            (unsigned char)((curv + half + 1U) & 0xFFU);
                    }
                }
                row += 0x10U;
            }
        }
    }
    t0 = 0;
    t3 = 8;
    while (t0 != 0x200UL) {
        t1 = (uint32_t)D4V_TBL_45AA82_8(&c->st, t0);
        t0 = (t0 + 8) & U32MASK;
        ADD8_LO(t1, 2UL);
        t2 = (uint32_t)D4V_TBL_45AA7E_8(&c->st, t0);
        D4V_TBL_45AA7A_8(&c->st, t0) = (unsigned char)(t1 & 0xFFUL);
        ADD8_LO(t2, t3 & 0xFFUL);
        t1 = (uint32_t)D4V_TBL_45AA7C_8(&c->st, t0) << 8;
        D4V_TBL_45AA7E_8(&c->st, t0) = (unsigned char)(t2 & 0xFFUL);
        ADD8_HI(t1, 5UL);
        t2 = (uint32_t)D4V_TBL_45AC78_8(&c->st, t0) << 8;
        D4V_TBL_45AA7C_8(&c->st, t0) = (unsigned char)((t1 >> 8) & 0xFFUL);
        ADD8_HI(t2, 0xDUL);
        t1 = (uint32_t)D4V_TBL_45AC7A_8(&c->st, t0);
        D4V_TBL_45AC78_8(&c->st, t0) = (unsigned char)((t2 >> 8) & 0xFFUL);
        ADD8_LO(t1, 0x17UL);
        t2 = (uint32_t)D4V_TBL_45AC7E_8(&c->st, t0);
        D4V_TBL_45AC7A_8(&c->st, t0) = (unsigned char)(t1 & 0xFFUL);
        ADD8_LO(t2, 0x3FUL);
        t1 = (uint32_t)D4V_TBL_45AC7C_8(&c->st, t0) << 8;
        D4V_TBL_45AC7E_8(&c->st, t0) = (unsigned char)(t2 & 0xFFUL);
        ADD8_HI(t1, 0x27UL);
        D4V_TBL_45AC7C_8(&c->st, t0) = (unsigned char)((t1 >> 8) & 0xFFUL);
    }
    t2 = (uint32_t)D4V_CTX_B_45A25D_8(&c->st, 0) << 8;
    t0 = (uint32_t)D4V_CTX_B_45A25F_8(&c->st, 0);
    t3 = (uint32_t)D4V_CTX_B_45A268_8(&c->st, 0);
    t1 = (uint32_t)D4V_CTX_B_45A272_8(&c->st, 0);
    ADD8_HI(t2, 2UL);
    ADD8_LO(t0, 4UL);
    ADD8_LO(t3, 2UL);
    ADD8_LO(t1, 2UL);
    D4V_CTX_B_45A25D_8(&c->st, 0) = (unsigned char)((t2 >> 8) & 0xFFUL);
    D4V_CTX_B_45A25F_8(&c->st, 0) = (unsigned char)(t0 & 0xFFUL);
    D4V_CTX_B_45A268_8(&c->st, 0) = (unsigned char)(t3 & 0xFFUL);
    D4V_CTX_B_45A272_8(&c->st, 0) = (unsigned char)(t1 & 0xFFUL);
    t0 = (uint32_t)D4V_CTX_B_45A267_8(&c->st, 0) << 8;
    t3 = (uint32_t)D4V_CTX_B_45A270_8(&c->st, 0) << 8;
    t1 = (uint32_t)D4V_CTX_B_45A27A_8(&c->st, 0) << 8;
    t2 = (uint32_t)D4V_CTX_B_45A27C_8(&c->st, 0);
    ADD8_HI(t0, 2UL);
    ADD8_HI(t3, 4UL);
    ADD8_HI(t1, 2UL);
    ADD8_LO(t2, 5UL);
    D4V_CTX_B_45A267_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFUL);
    D4V_CTX_B_45A270_8(&c->st, 0) = (unsigned char)((t3 >> 8) & 0xFFUL);
    D4V_CTX_B_45A27C_8(&c->st, 0) = (unsigned char)(t2 & 0xFFUL);
    t2 = (uint32_t)D4V_CTX_B_45A27E_8(&c->st, 0) << 8;
    D4V_CTX_B_45A27A_8(&c->st, 0) = (unsigned char)((t1 >> 8) & 0xFFUL);
    ADD8_HI(t2, 8UL);
    D4V_CTX_B_45A27E_8(&c->st, 0) = (unsigned char)((t2 >> 8) & 0xFFUL);
    {
        unsigned int dest;
        dest = 0x5FE00UL;
        while (dest != 0x61A00UL) {
            unsigned int at;
            at = (D4V_BLK_PTR_14_32(&c->st, 0) + dest) & U32MASK;
            dest = (dest + 0x200UL) & U32MASK;
            memcpy(c->blk14 + (at - c->blk14_base), d4v_t436ED8, 0x200);
        }
    }
    t0 = 0;
    t1 = 0x200UL;
    while (t0 != 0x1C00UL) {
        t1 = (t1 & 0xFFFF0000UL) | d4v_u16_in(c, c->blk14, c->blk14_base, 0x61A00u,
            (uint32_t)((D4V_BLK_PTR_14_32(&c->st, 0) + t0 + 0x5FE00UL) & U32MASK));
        t0 = (t0 + 2) & U32MASK;
        t1 = (t1 << 3) & U32MASK;
        d4v_u16_st_in(c, c->blk14, c->blk14_base, 0x61A00u,
            (uint32_t)((D4V_BLK_PTR_14_32(&c->st, 0) + t0 + 0x5FDFEUL) & U32MASK),
            (unsigned short)(t1 & 0xFFFFUL));
    }
    d4vc_fill16(c, 0xE00UL,
        (D4V_BLK_PTR_15_32(&c->st, 0) + 0x5FE00UL) & U32MASK, 8U);
    d4vc_fill16(c, 0x203UL, D4V_BLK_PTR_17_32(&c->st, 0), 1U);
    memcpy(c->st.g.bytes + (D4V_TBL_459BA8 - D4V_GLOBALS_BASE), d4v_t433BE8, 0x290);
    for (t3 = 0x203UL; t3 < 0x2FFUL; t3++)
        d4vc_rescale_ppm_plane(c, t3);
    d4vc_rescale_blk02(c);
    D4V_TBL_45AFCC_32(&c->st, 0) = 0;
    D4V_TBL_45AFC0_32(&c->st, 0) = 0;
    D4V_TBL_45AFA0_32(&c->st, 0) = D4V_TBL_45B5F4;
    D4V_TBL_45AFD4_32(&c->st, 0) = 0x1FFUL;
    d4v_fill(c, 0x424UL, D4V_TBL_45B5F4, 0);
    D4V_TBL_45AF78_32(&c->st, 0) = D4V_TBL_45AFD8;
    d4v_fill(c, 0x61CUL, D4V_TBL_45AFD8, 0);
    d4v_fill(c, 0x400UL, D4V_TBL_4593A0, 0);
    d4v_fill(c, 0x400UL, D4V_TBL_458798, 0);
    D4V_TBL_45AF90_32(&c->st, 0) = 0;
    D4V_TBL_45AF98_32(&c->st, 0) = 1;
    D4V_TBL_45AF7C_32(&c->st, 0) = 1;
    D4V_TBL_45AF94_32(&c->st, 0) = 0;
    for (t0 = 4; t0 <= 0x400UL; t0 += 4)
        D4V_TBL_458F94_32(&c->st, t0) = 0x9240UL;
    D4V_TBL_45AF80_32(&c->st, 0) = 0xFFUL;
    D4V_TBL_45AF64_32(&c->st, 0) = 0x91ADC0UL;
    D4V_TBL_459398_32(&c->st, 0) = 0;
    D4V_TBL_45939C_32(&c->st, 0) = 0;
    D4V_TBL_459BA0_32(&c->st, 0) = 0;
    D4V_TBL_459BA4_32(&c->st, 0) = 0;
    D4V_TBL_45AF60_32(&c->st, 0) = 0;
    D4V_TBL_45AF84_32(&c->st, 0) = 0;
    if (D4V_FILETAB_PTR_32(&c->st, 0) == 0) {
        D4V_TBL_45AFB4_32(&c->st, 0) = D4V_IO_OUT_LEN_32(&c->st, 0);
    } else {
        unsigned int extra;
        unsigned int sum;
        unsigned int limit;
        unsigned int trigger;
        unsigned char *ft;
        extra = d4v_sar(0x40000UL,
            ((unsigned int)D4V_CTX_B_45C67F_8(&c->st, 0) * 2U) & 0xFFU);
        sum = (D4V_TBL_45AFB4_32(&c->st, 0) + extra) & U32MASK;
        limit = D4V_IO_OUT_LEN_32(&c->st, 0);
        if (sum > limit)
            sum = limit;
        D4V_TBL_45AFB4_32(&c->st, 0) = sum;
        trigger = D4V_LZP_TRIGGER_32(&c->st, 0);
        limit = D4V_TBL_45BA44_32(&c->st, 0);
        t0 = (D4V_TBL_45BA2C_32(&c->st, 0) << 2) & U32MASK;
        while (1) {
            unsigned int word;
            ft = d4v_at(c, D4V_FILETAB_PTR_32(&c->st, 0));
            word = ft ? d4v_gld32(ft + t0)
                : d4v_ld32(c, (D4V_FILETAB_PTR_32(&c->st, 0) + t0) & U32MASK);
            if (sum <= word)
                break;
            t0 = (t0 + 4) & U32MASK;
            D4V_TBL_45BA2C_32(&c->st, 0) =
                (D4V_TBL_45BA2C_32(&c->st, 0) + 1) & U32MASK;
            word = ft ? d4v_gld32(ft + t0)
                : d4v_ld32(c, (D4V_FILETAB_PTR_32(&c->st, 0) + t0) & U32MASK);
            if (!(word < trigger && word < limit))
                break;
        }
        t3 = D4V_TBL_45BA2C_32(&c->st, 0);
        ft = d4v_at(c, D4V_FILETAB_PTR_32(&c->st, 0));
        D4V_TBL_45AFB4_32(&c->st, 0) = ft ? d4v_gld32(ft + t3 * 4)
            : d4v_ld32(c, (D4V_FILETAB_PTR_32(&c->st, 0) + t3 * 4) & U32MASK);
    }
    if (D4V_TBL_45BA2C_32(&c->st, 0) != 0) {
        unsigned int slot;
        unsigned char *ft;
        int fill_both;
        slot = D4V_TBL_45BA2C_32(&c->st, 0);
        ft = d4v_at(c, D4V_FILETAB_PTR_32(&c->st, 0));
        t2 = ft ? d4v_gld32(ft + ((slot << 2) - 4))
            : d4v_ld32(c, (D4V_FILETAB_PTR_32(&c->st, 0) + (slot << 2) - 4) & U32MASK);
        t4 = (D4V_TBL_45AFB4_32(&c->st, 0) - t2) & U32MASK;
        if (slot > 1U) {
            unsigned int prev;
            prev = ft ? d4v_gld32(ft + ((slot << 2) - 8))
                : d4v_ld32(c, (D4V_FILETAB_PTR_32(&c->st, 0) + (slot << 2) - 8) & U32MASK);
            t2 = (t2 - prev) & U32MASK;
        }
        if (D4V_CTX_B_45C675_8(&c->st, 0) != 1)
            t5 = D4V_IO_OUT_TOTAL_32(&c->st, 0);
        else
            t5 = D4V_TBL_45BA28_32(&c->st, 0);
        if ((t2 == 0x75A45UL && t4 == 0x68302UL)
            || (t2 == 0x8F4D0UL && t4 == 0x2D9A22UL))
            d4v_fill(c, 0x100000UL, D4V_BLK_PTR_01_32(&c->st, 0), 0);
        if ((t2 == 0x17E5F8UL && t4 == 0x2CC640UL)
            || (t2 == 0x2CC640UL && t4 == 0x6AF80UL)) {
            d4vc_rescale_blk01(c);
            d4vc_rescale_blk02(c);
        }
        fill_both = 0;
        if ((t2 == 0x388DC2UL && t4 == 0x16DD2EUL)
            || (t2 == 0x7D4C0UL && t4 == 0x3C41EUL))
            fill_both = 1;
        else if (t2 != 0xFB670UL && t4 == 0xFB670UL
            && t5 >= D4V_TBL_45BA44_32(&c->st, 0))
            fill_both = 1;
        if (fill_both) {
            d4v_fill(c, 0x100000UL, D4V_BLK_PTR_01_32(&c->st, 0), 0);
            d4v_fill(c, 0x140000UL, D4V_BLK_PTR_02_32(&c->st, 0), 0);
            if (t5 >= D4V_TBL_45BA44_32(&c->st, 0))
                D4V_CTX_B_45C681_8(&c->st, 0) = 0x0A;
        }
    }
    if (D4V_CTX_B_45C675_8(&c->st, 0) == 1)
        return (int)d4vc_ppm_window_decode(c, 0xFFU);
    D4V_RSLOT_LO_32(&c->st, 0) = 0x0AU;
    t2 = 0x0AU;
    t0 = D4V_RC_RANGE_32(&c->st, 0) / t2;
    D4V_RC_RANGE_32(&c->st, 0) = t0;
    t1 = D4V_RC_CODE_32(&c->st, 0) / t0;
    D4V_RSLOT_MID_32(&c->st, 0) = t1 & 0xFFU;
    D4V_RSLOT_HI_32(&c->st, 0) = ((t1 & 0xFFU) + 1U) & U32MASK;
    t3 = d4v_slot32(c, 8u);
    t0 = d4v_slot32(c, 4u);
    t2 = D4V_RC_RANGE_32(&c->st, 0);
    t3 = (t3 - t0) & U32MASK;
    t0 = (t0 * t2) & U32MASK;
    t3 = (t3 * t2) & U32MASK;
    t2 = (D4V_RC_CODE_32(&c->st, 0) - t0) & U32MASK;
    d4v_rcrenorm_reg(c, &t2, &t3, &t0);
    D4V_RC_CODE_32(&c->st, 0) = t2;
    D4V_RC_RANGE_32(&c->st, 0) = t3;
    if ((t1 & 0xFFU) != (unsigned int)D4V_CTX_B_45C67D_8(&c->st, 0)) {
        unsigned int cls;
        cls = t1 & 0xFFU;
        D4V_CTX_B_45C67D_8(&c->st, 0) = (unsigned char)cls;
        D4V_TBL_45AF3C_32(&c->st, 0) = d4v_t4342C4[cls];
        D4V_TBL_45AF40_32(&c->st, 0) = d4v_t43429C[cls];
        D4V_CTX_B_45C67A_8(&c->st, 0) = d4v_t434314[cls];
        t0 = (cls == 5U || cls == 6U) ? 1U : 0U;
        D4V_CTX_B_45C688_8(&c->st, 0) = (unsigned char)t0;
        D4V_CTX_B_45C67B_8(&c->st, 0) = 0;
        D4V_CTX_B_45C679_8(&c->st, 0) = 0;
        D4V_TBL_45AF48_32(&c->st, 0) = 0;
        if (cls > 4U) {
            d4vc_s4_reset_tables(c);
            return (int)t0;
        }
    }
    return (int)(t0 & U32MASK);
}


/* ------------------------------------------------------------------ */
/* Role hypothesis: per-symbol PPM match decoder. Hashes the */
/* 45BA18/45BA1C/45BA20 shift registers into the [0x433B4C]/[0x433B50] */
/* chains, walks the two-entry chain at t3 (top = [var_34]) comparing */
/* the last 0x40 dict bytes (64-byte memcmp, 8-wide then byte tail) to */
/* find the longest match (var_20, entry t5), then length/flag */
/* classification (var_24/var_1C via 45BA38/45BA48/45C691/418590/ */
/* 416C80/419CA0/416B50 helpers) and either returns 0 (no PPM token */
/* caller falls through to sub_418830) or emits bl bytes through */
/* sub_4183D0/sub_4177A0 into the dict and returns 1. Called by */
/* sub_417DA0 per symbol; its al is tested (test al,al). */
/* Entry-live regs: none (t0/t1/t2/t3/t4/t5/t6 all derived */
/* from globals; no stack args). Returns unsigned int = al (0/1). */
/* Frame: 0x24 bytes + 6 pushed regs. Locals: v3c @ sp+0, v38 @ sp+4 */
/* v34 @ sp+8, v30 @ sp+0xC, v2c @ sp+0x10 (dword + byte lanes) */
/* v28 @ sp+0x14, v24 @ sp+0x18, v20 @ sp+0x1C, v1c @ sp+0x20 (bytes). */
/* Globals touched: 45BA18,45BA1C,45BA20,433B4C,433B50,45BA34,45BA3C */
/* 45BA30,433B40,433B54,45BA38,45BA48,45BA40,45C691,45AFB4,45AFA8 */
/* 45AFD0,45BA24,45C67F,45C68A,45C65E,45C68E,T43408C,T43407C,45AFA4 */
/* 45C689 (via helpers), helpers sub_418590/416C80/419CA0/416B50/ */
/* 4183D0/4177A0. */
/* ------------------------------------------------------------------ */
unsigned int d4vc_ppmdec(dec4v_ctx *c)
{
    uint32_t t0 = 0, t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0;
    uint32_t v3c = 0, v38 = 0, v34 = 0, v30 = 0, v2c = 0;
    uint32_t v28 = 0, v24 = 0, v20 = 0, v1c = 0; /* byte lanes, kept 0..255 */
    uint32_t dd_ = 0;

    /* prologue 0x00417E10 push t1; 0x00417E11 push t2;
     * 0x00417E12 push t3; 0x00417E13 push t4; 0x00417E14 push t5;
     * 0x00417E15 push t6; 0x00417E16 sub esp,24h.
     * Callee-saved regs become C locals; the esp frame is v3c..v1c. */

    t0 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
    t3 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
    t0 >>= 0xD;
    t0 ^= t3;
    t2 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
    t0 &= 0x3FFFFUL;
    t3 = (uint32_t)D4V_BLK_PTR_03_32(&c->st, 0);
    v30 = t0;
    t0 = (t0 << 3) & U32MASK;
    t1 = (uint32_t)D4V_SHIFT_R1_32(&c->st, 0);
    t3 = (t3 + t0) & U32MASK;
    t0 = (uint32_t)D4V_SHIFT_R1_32(&c->st, 0);
    v34 = t3;
    t3 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
    t0 >>= 0xF;
    t3 >>= 0xF;
    t0 ^= t1;
    t3 ^= t2;
    t0 = (t0 << 6) & U32MASK;
    t3 = (t3 << 4) & U32MASK;
    t3 ^= t0;
    t0 = (uint32_t)D4V_SHIFT_R2_32(&c->st, 0);
    t4 = (uint32_t)D4V_SHIFT_R2_32(&c->st, 0);
    t0 >>= 0xF;
    t0 ^= t4;
    t0 ^= t3;
    t0 &= 0x3FFFFUL;
    t3 = (t0 * 8) & U32MASK;
    t1 &= 0xFFFF00FFUL;
    t0 = (uint32_t)D4V_BLK_PTR_04_32(&c->st, 0);
    t3 = (t3 + 4) & U32MASK;
    t2 = (t2 & 0xFFFFFF00UL) | 2UL;
    t3 = (t3 + t0) & U32MASK;
    t0 &= 0xFFFF00FFUL;
    v20 = (t1 >> 8) & 0xFFUL; /* =0 */
    v24 = (t0 >> 8) & 0xFFUL; /* =0 */
    v1c = (t0 >> 8) & 0xFFUL; /* =0 */
    t0 = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
    v28 = t2 & 0xFFUL; /* =2 */
    v38 = t0;
    ;
    if (t0 < 0x40UL)
        v38 = (v38 + (uint32_t)D4V_DICT_SIZE_32(&c->st, 0)) & U32MASK;
    v3c = (v38 - 1) & U32MASK;
    while (v28 != 0) {
        unsigned int pos;
        unsigned int dist;
        unsigned int ptr;
        unsigned int rem;
        unsigned int src;
        unsigned int dst;
        unsigned int k;
        int stopped;
        v28 = (v28 - 1U) & 0xFFU;
        pos = *(unsigned int *)(void *)(c->blk04 + ((uint32_t)t3 - c->blk04_base));
        if (pos != 0xFFFFFFFFUL) {
            dist = (pos - (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0)) & U32MASK;
            dist &= (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
            if (dist >= 0x140UL) {
                if (pos < 0x40UL)
                    pos = (pos + (uint32_t)D4V_DICT_SIZE_32(&c->st, 0)) & U32MASK;
                ptr = (uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0);
                if (c->blk00_dict[((ptr + v38 - 0xCUL) & U32MASK) - c->blk00_base]
                    == c->blk00_dict[((ptr + pos - 0xCUL) & U32MASK) - c->blk00_base]) {
                    rem = 0x40U;
                    src = (ptr + v3c) & U32MASK;
                    dst = (ptr + ((pos - 1U) & U32MASK)) & U32MASK;
                    stopped = 0;
                    while (rem >= 8U) {
                        for (k = 0; k < 8U; k++) {
                            if (c->blk00_dict[((src - k) & U32MASK) - c->blk00_base]
                                != c->blk00_dict[((dst - k) & U32MASK) - c->blk00_base]) {
                                rem -= k;
                                stopped = 1;
                                break;
                            }
                        }
                        if (stopped)
                            break;
                        src = (src - 8U) & U32MASK;
                        dst = (dst - 8U) & U32MASK;
                        rem -= 8U;
                    }
                    while (rem != 0U && stopped == 0) {
                        if (c->blk00_dict[src - c->blk00_base]
                            != c->blk00_dict[dst - c->blk00_base])
                            break;
                        src = (src - 1U) & U32MASK;
                        dst = (dst - 1U) & U32MASK;
                        rem--;
                    }
                    k = (0x40U - rem) & 0xFFU;
                    if (k >= (v20 & 0xFFU)) {
                        v20 = k;
                        t5 = *(unsigned int *)(void *)(c->blk04 + ((uint32_t)t3 - c->blk04_base));
                    }
                }
            }
        }
        if (v28 == 0)
            break;
        t3 = (t3 - 4U) & U32MASK;
        pos = *(unsigned int *)(void *)(c->blk04 + ((uint32_t)t3 - c->blk04_base));
        *(unsigned int *)(void *)(c->blk04 + (((t3 + 4U) & U32MASK) - c->blk04_base)) = pos;
    }
    *(unsigned int *)(void *)(c->blk04 + ((uint32_t)t3 - c->blk04_base)) =
        (unsigned int)D4V_DICT_CURSOR_32(&c->st, 0);
    if ((v20 & 0xFFU) >= 0x0CU)
        v24 = 0x15UL;
    if (D4V_TBL_45BA38_32(&c->st, 0) != 0) {
        t0 = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
        t3 = (uint32_t)D4V_TBL_45BA48_32(&c->st, 0);
        t5 = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
        t0 = (t0 - t3) & U32MASK;
        t2 = (uint32_t)D4V_TBL_45BA38_32(&c->st, 0);
        t5 &= t0;
        t0 = (uint32_t)(t2 > 2UL ? 1 : 0);
        ADD8_LO(t0, 0x50UL);
        v1c = 1;
        v24 = t0 & 0xFFUL;
    }
    if (v24 == 0) {
        unsigned int mask;
        unsigned int rel;
        unsigned int cur;
        mask = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
        cur = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
        t5 = (cur - (uint32_t)D4V_TBL_45BA48_32(&c->st, 0)) & mask;
        rel = (t5 - cur) & mask;
        if (rel >= 0x104UL) {
            unsigned int at;
            unsigned int b0, b1, b2, b3;
            unsigned int r0;
            at = ((t5 - 4U) & U32MASK) & mask;
            at = (at + (uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0)) & U32MASK;
            b0 = c->blk00_dict[at - c->blk00_base];
            b1 = c->blk00_dict[((at + 1U) & U32MASK) - c->blk00_base];
            b2 = c->blk00_dict[((at + 2U) & U32MASK) - c->blk00_base];
            b3 = c->blk00_dict[((at + 3U) & U32MASK) - c->blk00_base];
            v2c = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
            r0 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
            if ((v2c & 0xFFFFUL) == (r0 & 0xFFFFUL)
                || (v2c & 0xFFFF00FFUL) == (r0 & 0xFFFF00FFUL)
                || (v2c & 0xFFFFFF00UL) == (r0 & 0xFFFFFF00UL)) {
                v24 = 0x0BUL;
                v1c = 1;
            }
        }
        if (v24 == 0) {
            unsigned int head;
            head = *(unsigned int *)(void *)(c->blk03 + (((v34 + 4U) & U32MASK) - c->blk03_base));
            if (head == (uint32_t)D4V_SHIFT_R0_32(&c->st, 0)) {
                t5 = *(unsigned int *)(void *)(c->blk03 + ((uint32_t)v34 - c->blk03_base));
                v24 = (t5 != 0xFFFFFFFFUL) ? 1U : 0U;
            } else if (t5 != 0xFFFFFFFFUL)
                v24 = 0x1FUL;
        }
    }
    *(unsigned int *)(void *)(c->blk03 + ((uint32_t)v34 - c->blk03_base)) =
        (unsigned int)D4V_DICT_CURSOR_32(&c->st, 0);
    *(unsigned int *)(void *)(c->blk03 + (((v34 + 4U) & U32MASK) - c->blk03_base)) =
        (unsigned int)D4V_SHIFT_R0_32(&c->st, 0);
    if (v24 == 0) {
        D4V_TBL_45BA38_32(&c->st, 0) = 0;
        return 0;
    }
    {
        unsigned int dist;
        dist = (t5 - (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0)) & U32MASK;
        dist &= (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
        if (dist < 0x100UL) {
            D4V_TBL_45BA38_32(&c->st, 0) = 0;
            return 0;
        }
    }
    if (D4V_TBL_45BA38_32(&c->st, 0) == 0) {
        unsigned int add;
        t0 = ((uint32_t)D4V_BLK_PTR_05_32(&c->st, 0) + v30) & U32MASK;
        if (c->blk05[t0 - c->blk05_base] == 0)
            add = 0x27UL;
        else
            add = 0xFFFFFFFFUL;
        v24 = (v24 + (uint32_t)D4V_CTX_B_45C691_8(&c->st, 0) + add) & 0xFFUL;
    }
    if (D4V_TBL_45BA40_32(&c->st, 0) != 0) {
        unsigned int ptr;
        unsigned int here;
        unsigned int there;
        ptr = (uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0);
        there = (uint32_t)D4V_TBL_45BA40_32(&c->st, 0);
        here = c->blk00_dict[((t5 + ptr) & U32MASK) - c->blk00_base];
        if (c->blk00_dict[there - c->blk00_base] == here) {
            d4vc_ppm_touch_len(c, (unsigned int)(v24 & 0xFFUL), 2U);
            t0 = ((uint32_t)D4V_BLK_PTR_05_32(&c->st, 0) + v30) & U32MASK;
            c->blk05[t0 - c->blk05_base] = 0;
            D4V_TBL_45BA38_32(&c->st, 0) = 0;
            return 0;
        }
    }
    t0 = d4vc_ppm_len_decode(c, (unsigned int)(v24 & 0xFFUL));
    if ((t0 & 0xFFUL) != 1U) {
        t0 = ((uint32_t)D4V_BLK_PTR_05_32(&c->st, 0) + v30) & U32MASK;
        c->blk05[t0 - c->blk05_base] = 0;
        D4V_TBL_45BA38_32(&c->st, 0) = 0;
        return 0;
    }
    D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)(v1c == 0 ? 1 : 0);
    dd_ = ((uint32_t)D4V_TBL_45AFA8_32(&c->st, 0) << 2) & U32MASK;
    if (D4V_CTX_B_45C68A_8(&c->st, 0) == 0) {
        t0 = (uint32_t)(dd_ / (uint32_t)D4V_TBL_45AFD0_32(&c->st, 0));
        t0 = (uint32_t)d4v_t43404C[0x30UL + t0];
    } else {
        unsigned int period;
        period = (uint32_t)D4V_TBL_45AFD0_32(&c->st, 0);
        t0 = (uint32_t)(dd_ / period);
        t0 = (uint32_t)d4v_t43404C[0x40UL + t0];
        SUB8_LO(t0, (uint32_t)D4V_CTX_B_45C67F_8(&c->st, 0));
        if ((uint32_t)D4V_TBL_45BA24_32(&c->st, 0) > period)
            t0 = (t0 & 0xFFFFFF00UL) | 0x1DUL;
        else if (D4V_CTX_B_45C68E_8(&c->st, 0) != 0)
            ADD8_LO(t0, t0 & 0xFFUL);
        D4V_CTX_B_45C65E_8(&c->st, 0) = 3;
    }
    while ((t0 & 0xFFUL) > 7UL) {
        unsigned int idx;
        unsigned int val;
        idx = D4V_CTX_B_45C68A_8(&c->st, 0);
        val = ((t0 & 0xFFUL) - 1U) & 0xFFU;
        D4V_CTX_B_45C65E_8(&c->st, idx) = (unsigned char)val;
        D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)(idx + 1U);
        t0 = (t0 & 0xFFFFFF00UL) | val;
    }
    t2 = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
    t0 = ((uint32_t)D4V_DICT_CURSOR_32(&c->st, 0) - t5) & U32MASK;
    t2 &= t0;
    if (v1c != 0)
        t3 = 0;
    else {
        t0 = ((uint32_t)D4V_BLK_PTR_05_32(&c->st, 0) + v30) & U32MASK;
        if (c->blk05[t0 - c->blk05_base] < 0x10)
            t3 = 0;
        else
            t3 = 1;
    }
    D4V_TBL_45AFA4_32(&c->st, 0) = 0x30CUL;
    t0 = (uint32_t)D4V_TBL_45BA38_32(&c->st, 0);
    if (t0 == 0)
        t3 = (t3 + 0x2FBUL) & U32MASK;
    else
        t3 = (((t0 > 2UL) ? 1UL : 0UL) + 0x2FDUL) & U32MASK;
    t0 = d4vc_ppm_prob_decode(c, (unsigned int)t3);
    t1 = ((t0 & 0xFFUL) << 8) | (t0 & 0xFFUL);
    d4vc_ppm_order_update(c, (unsigned int)(t0 & 0xFFUL), (unsigned int)t3);
    D4V_TBL_45BA48_32(&c->st, 0) = (unsigned int)t2;
    D4V_TBL_45BA40_32(&c->st, 0) = 0;
    t4 = ((uint32_t)D4V_BLK_PTR_05_32(&c->st, 0) + v30) & U32MASK;
    c->blk05[t4 - c->blk05_base] = (unsigned char)((t1 >> 8) & 0xFFUL);
    if (((t1 >> 8) & 0xFFUL) == 0xFFUL) {
        D4V_TBL_45BA38_32(&c->st, 0) =
            (D4V_TBL_45BA38_32(&c->st, 0) + 1U) & U32MASK;
    } else {
        if (((t1 >> 8) & 0xFFUL) >= 7UL) {
            unsigned int msk;
            unsigned int off;
            msk = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
            off = (((t1 >> 8) & 0xFFUL) + t5) & msk;
            D4V_TBL_45BA40_32(&c->st, 0) =
                ((uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0) + off) & U32MASK;
        }
        D4V_TBL_45BA38_32(&c->st, 0) = 0;
    }
    do {
        unsigned int ptr;
        unsigned int cur;
        unsigned int msk;
        unsigned int byte;
        ptr = (uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0);
        byte = c->blk00_dict[((t5 + ptr) & U32MASK) - c->blk00_base];
        d4vc_ppm_escape_emit(c, byte, 0);
        d4vc_ppm_mask_select(c, byte, ((t1 & 0xFFUL) <= 8UL) ? 1U : 0U);
        cur = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
        c->blk00_dict[((ptr + cur) & U32MASK) - c->blk00_base] = (unsigned char)byte;
        if (cur < 0x100UL) {
            unsigned int wrap;
            wrap = (cur + (uint32_t)D4V_DICT_SIZE_32(&c->st, 0)) & U32MASK;
            c->blk00_dict[((ptr + wrap) & U32MASK) - c->blk00_base] = (unsigned char)byte;
        }
        msk = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
        cur = ((uint32_t)D4V_DICT_CURSOR_32(&c->st, 0) + 1U) & msk;
        t5 = (t5 + 1U) & msk;
        D4V_DICT_CURSOR_32(&c->st, 0) = cur;
        DECB_LO(t1);
    } while ((t1 & 0xFFUL) != 0);
    return 1;
}

/* Emit the symbol chosen in stk[0x24] and advance the dictionary. */
static void d4vc_sym_emit(dec4v_ctx *c, unsigned char *stk)
{
    unsigned int sym;
    sym = stk[0x24];
    d4vc_ppm_list_update_32(c, sym);
    if (D4V_CTX_B_45C68B_8(&c->st, 0) != 0)
        d4vc_ppm_list_update_20(c, sym);
    if (D4V_CTX_B_45C680_8(&c->st, 0) != 0)
        d4vc_ppm_rescale_orders(c, sym);
    if (stk[0x28] != 0) {
        unsigned int adj;
        unsigned int scaled;
        adj = (sym - (unsigned)D4V_CTX_B_45C679_8(&c->st, 0)) & 0xFFu;
        if (D4V_CTX_B_45C67B_8(&c->st, 0) != 0)
            adj = (0u - adj) & 0xFFu;
        D4V_CTX_B_45C68A_8(&c->st, 0) = 0;
        d4vc_freq_adapt_45af60(c, D4V_TBL_45AF3C_32(&c->st, 0),
            (unsigned)D4V_CTX_B_45C67D_8(&c->st, 0) + 0x302u, adj);
        if (D4V_CTX_B_45C68D_8(&c->st, 0) != 0)
            D4V_TBL_45AF88_32(&c->st, 0) += D4V_TBL_45AF8C_32(&c->st, 0);
        scaled = D4V_TBL_45AF84_32(&c->st, 0);
        scaled = ((scaled << 5) - scaled) >> 5;
        D4V_TBL_45AF84_32(&c->st, 0) = scaled + D4V_TBL_45AF88_32(&c->st, 0);
    }
    while (1) {
        unsigned int b;
        unsigned int slot;
        b = (D4V_CTX_B_45C690_8(&c->st, 0) - 1u) & 0xFFu;
        D4V_CTX_B_45C690_8(&c->st, 0) = (unsigned char)b;
        if (b == 0xFFu)
            break;
        slot = (uint32_t)D4V_TBL_45AF50_8(&c->st, b) & 0xFFu;
        D4V_TBL_45AFA4_32(&c->st, 0) = (uint32_t)D4V_TBL_458778_32(&c->st, b * 4u);
        d4vc_ppm_order_update(c, slot, (uint32_t)D4V_TBL_458788_32(&c->st, b * 4u));
    }
    d4vc_ppm_escape_emit(c, sym, 1u);
    d4vc_ppm_mask_select(c, sym, 1u);
    {
        unsigned int ptr;
        unsigned int cur;
        ptr = D4V_BLK_PTR_00_DICT_32(&c->st, 0);
        cur = D4V_DICT_CURSOR_32(&c->st, 0);
        c->blk00_dict[(ptr + cur) - c->blk00_base] = (unsigned char)sym;
        if (cur < 0x100u)
            c->blk00_dict[(cur + D4V_DICT_SIZE_32(&c->st, 0) + ptr) - c->blk00_base] =
                (unsigned char)sym;
        cur = D4V_DICT_CURSOR_32(&c->st, 0);
        D4V_DICT_CURSOR_32(&c->st, 0) =
            (cur + 1u) & D4V_DICT_MASK_32(&c->st, 0);
    }
}

/* ---- worker: per-symbol PPM (from draft_418830.c) ---- */
void d4vc_sym(dec4v_ctx *c, unsigned int a_eax)
{
    unsigned int e_eax = 0, e_ebx = 0, e_ecx = 0, e_edx = 0, e_esi = 0, e_edi = 0, e_ebp = 0;
    unsigned int f_cf = 0, f_zf = 0, f_sf = 0, f_of = 0;
    unsigned UHI64 d4v_q = 0;
    UHI64 d4v_n = 0;
    unsigned char stk[0x50] = {0};
    unsigned char *p2 = 0; /* 20-byte blk02 entry while e_ebp holds it */
    unsigned char *n1 = 0; /* 32-byte blk01 node while e_ebx holds it */
    unsigned char *sc = 0; /* blkord slot returned by d4vc_ppm_score_orders */
    (void)a_eax; /* dead at entry: overwritten at / */
    f_cf = f_zf = f_sf = f_of = 0u;
    /* prologue: push t1,t2,t3,t4,t5,t6; sub esp,0x50 (frame 0x68) */
    stk[0x30] = (unsigned char)D4V_CTX_B_45C67F_8(&c->st, 0);
    stk[0x20] = 1;
    if (D4V_CTX_B_45C68E_8(&c->st, 0) == 2) {
        unsigned int r0;
        unsigned int r1;
        r0 = D4V_SHIFT_R0_32(&c->st, 0);
        r1 = D4V_SHIFT_R1_32(&c->st, 0);
        D4V_TBL_45AFC4_32(&c->st, 0) = ((r0 >> 8) & 0xFFu)
            + ((r0 >> 16) & 0xFF00u)
            + ((r1 << 8) & 0xFF0000u)
            + (r1 & 0xFF000000u);
    } else
        D4V_TBL_45AFC4_32(&c->st, 0) = D4V_SHIFT_R0_32(&c->st, 0);
    {
        unsigned int word;
        unsigned int low;
        unsigned int slot;
        word = D4V_TBL_45AFC4_32(&c->st, 0);
        D4V_TBL_45AFAC_32(&c->st, 0) = ((word >> 13) ^ word) & 0x7FFFu;
        low = word & 0xFFFFu;
        D4V_TBL_45AFBC_32(&c->st, 0) = low;
        D4V_CTX_B_45C68F_8(&c->st, 0) = 0;
        D4V_CTX_B_45C683_8(&c->st, 0) = 0;
        D4V_CTX_B_45C687_8(&c->st, 0) = 0;
        D4V_CTX_B_45C685_8(&c->st, 0) = 0;
        D4V_CTX_B_45C68D_8(&c->st, 0) = 0;
        D4V_CTX_B_45C68B_8(&c->st, 0) = 0;
        D4V_CTX_B_45C680_8(&c->st, 0) = 0;
        D4V_CTX_B_45C684_8(&c->st, 0) = 0;
        D4V_CTX_B_45C68A_8(&c->st, 0) = 0;
        D4V_CTX_B_45C690_8(&c->st, 0) = 0;
        D4V_CTX_B_45C693_8(&c->st, 0) = 0;
        D4V_TBL_45AF88_32(&c->st, 0) = 0;
        D4V_TBL_45AF8C_32(&c->st, 0) = 0;
        slot = low * 20u;
        e_ebp = D4V_BLK_PTR_02_32(&c->st, 0) + slot;
        p2 = c->blk02 + (e_ebp - c->blk02_base);
        e_esi = D4V_TBL_45BA40_32(&c->st, 0);
        if (e_esi != 0) {
            D4V_TBL_45BA40_32(&c->st, 0) = 0;
            D4V_CTX_B_45C65E_8(&c->st, 0) = c->blk00_dict[e_esi - c->blk00_base];
            D4V_CTX_B_45C68A_8(&c->st, 0) = 1;
        }
        if (D4V_TBL_45AF98_32(&c->st, 0) > 1u
            || ((D4V_TBL_45AF7C_8(&c->st, 0) - 2u) & 0xFFu) <= 2u) {
            unsigned int rem;
            e_ecx = D4V_TBL_45AF7C_32(&c->st, 0);
            e_ebx = e_ecx;
            if (e_ecx <= 4u) {
                rem = D4V_TBL_45AF38_32(&c->st, 0) % e_ecx;
                e_ebx = d4v_t433B90[e_ecx] + rem;
            }
            if (e_ecx <= 0x10u) {
                rem = D4V_TBL_45AF38_32(&c->st, 0) % e_ecx;
                e_ecx = d4v_t433BA4[e_ecx] + rem;
            }
            D4V_TBL_45AFA0_32(&c->st, 0) = D4V_TBL_45B5F4 + (e_ebx << 2);
            D4V_TBL_45AF78_32(&c->st, 0) = D4V_TBL_45AFD8 + (e_ecx << 2);
        }
    }
    if (stk[0x30] != 0) {
        unsigned int a;
        unsigned int b;
        a = D4V_TBL_459BA0_32(&c->st, 0);
        b = D4V_TBL_459BA4_32(&c->st, 0);
        if (D4V_CTX_B_45C688_8(&c->st, 0) == 0)
            stk[0x30] = (unsigned char)(a < b);
        else
            stk[0x30] = (unsigned char)((a * 31u) < (b << 5));
    }
    {
        unsigned int bit;
        if (D4V_CTX_B_45C67D_8(&c->st, 0) == 0)
            bit = 0;
        else if (D4V_TBL_45AF98_32(&c->st, 0) <= 4u)
            bit = 1;
        else if (d4v_gld32(c->st.g.bytes + (D4V_TBL_45AFA0_32(&c->st, 0) - D4V_GLOBALS_BASE)) <= 0x3FEu)
            bit = 1;
        else
            bit = 0;
        stk[0x4C] = (unsigned char)bit;
        if (stk[0x30] == 0 || bit == 0)
            bit = 0;
        else
            bit = 1;
        stk[0x28] = (unsigned char)bit;
        if (bit != 0 && D4V_TBL_45AF60_32(&c->st, 0) < D4V_TBL_45AF84_32(&c->st, 0))
            stk[0x1C] = 1;
        else
            stk[0x1C] = 0;
    }
    e_eax = D4V_TBL_45AFAC_32(&c->st, 0) << 5;
    e_edi = D4V_TBL_45AFC4_32(&c->st, 0);
    {
        unsigned int found;
        found = 0;
        while (1) {
            unsigned int key;
            e_ebx = D4V_BLK_PTR_01_32(&c->st, 0) + e_eax;
            n1 = c->blk01 + (e_ebx - c->blk01_base);
            if (n1[0] == 0) {
                D4V_TBL_45AFAC_16(&c->st, 0x2u) =
                    (unsigned)((D4V_TBL_45AFAC_16(&c->st, 0x2u) + 4u) & 0xFFFFu);
                break;
            }
            key = n1[0x1C] | ((unsigned)n1[0x1D] << 8)
                | ((unsigned)n1[0x1E] << 16) | ((unsigned)n1[0x1F] << 24);
            if (key != e_edi) {
                e_eax += 0x100000u;
                D4V_TBL_45AFAC_32(&c->st, 0) += 0x8000u;
                if (D4V_TBL_45AFAC_32(&c->st, 0) < 0x40000u)
                    continue;
                break;
            }
            D4V_CTX_B_45C692_8(&c->st, 0) = 0xFFu;
            D4V_TBL_45AF9C_32(&c->st, 0) = (unsigned)n1[0] + (unsigned)n1[1];
            e_eax = 1;
            while (1) {
                unsigned int b;
                e_ecx = e_eax;
                b = n1[e_ecx + 1];
                e_eax += 1u;
                e_edx = b;
                if (b == 0)
                    break;
                D4V_TBL_45AF9C_32(&c->st, 0) += b;
                if (e_eax >= 0xDu)
                    break;
            }
            found = 1;
            break;
        }
        if (found && (D4V_TBL_45AFC0_32(&c->st, 0) != 0u
            || D4V_TBL_45BA24_32(&c->st, 0) > D4V_TBL_45AFD0_32(&c->st, 0)
            || D4V_TBL_45AF98_32(&c->st, 0) <= 1u
            || d4v_gld32(c->st.g.bytes + (D4V_TBL_45AFA0_32(&c->st, 0) - D4V_GLOBALS_BASE)) <= 0x3FEu
            || d4v_gld32(c->st.g.bytes + (D4V_TBL_45AF78_32(&c->st, 0) - D4V_GLOBALS_BASE)) <= 0u))
            {
        unsigned int go_on;
        go_on = 1;
 e_eax = ((e_eax & ~0xFFu) | (n1[0x1B])); e_eax = ((e_eax & ~0xFFu) | ((((((e_eax & 0xFFu)) & (0x3Fu))) & 0xFFu)));
    ;
    if (((e_eax & 0xFFu) < 0x8u) && (p2[0] != 0u)) {
        e_eax = ((e_eax) & (0xFFu)); e_edx = (0x3E0u); e_eax = ((e_eax) << 5); e_edx = ((e_edx) - (e_eax)); e_eax = (e_edx); e_edx = (0u);
        e_edx = ((e_edx & ~0xFFu) | (p2[1]));
        d4v_q = ((UHI64)(int)(e_edx)) * ((UHI64)(int)(e_eax));
        e_edx = (unsigned)d4v_q  ;
        e_ecx = (0u); e_ecx = ((e_ecx & ~0xFFu) | (p2[0x12])); e_eax = (e_edx); e_edx = ((unsigned)((int)(e_edx) >> 31));
        d4v_n = (((UHI64)(int)e_edx << 32) + (UHI64)e_eax);
        e_eax = (unsigned)(int)(d4v_n / ((UHI64)(int)(e_ecx)));
        e_edx = (unsigned)(int)(d4v_n % ((UHI64)(int)(e_ecx))) ;
        e_ecx = (e_eax); e_eax = (0u); e_esi = (D4V_TBL_45AF9C_32(&c->st, 0)); e_eax = ((e_eax & ~0xFFu) | (n1[1])); e_edx = (0u); e_eax = ((e_eax) << 10);
        d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) ;
        e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_esi)));
        e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_esi))) ;
        if ((e_ecx) > (e_eax))
            go_on = 0;
    }
    if (go_on) {
    e_eax = ((e_eax & ~0xFFu) | (0x1u)); e_edi = (D4V_SP); D4V_CTX_B_45C68F_8(&c->st, 0) = ((e_eax & 0xFFu)); e_eax = (D4V_TBL_45AF9C_32(&c->st, 0)); e_esi = (0x4150DBu);
    D4V_TBL_45AFB8_32(&c->st, 0) = (e_eax); e_eax = ((e_eax & ~(0xFFu << 8)) | ((D4V_CTX_B_45C68A_8(&c->st, 0)) << 8));
     stk[0x00] = 1u; stk[0x01] = 1u; stk[0x02] = 1u; stk[0x03] = 1u; stk[0x04] = 1u; stk[0x05] = 1u; stk[0x06] = 1u; stk[0x07] = 1u; stk[0x08] = 1u; stk[0x09] = 1u; stk[0x0A] = 1u; stk[0x0B] = 1u; stk[0x0C] = 1u;
    e_esi += 13u; e_edi += 13u;
    /* above: movsd x3 + movsb: 13 code-const 0x01 bytes (e_esi was ) to stk[0x00..0x0C]; UNCERTAIN(src-bytes): inferred from 01 01 opcode display */
    ;
    if (D4V_CTX_B_45C68A_8(&c->st, 0) != 0) {
        unsigned int i;
        stk[0x34] = D4V_CTX_B_45C65E_8(&c->st, 0);
        i = 0;
        while (i < 0xDu) {
            unsigned int lenb;
            e_ecx = i;
            lenb = n1[i + 1];
            if (lenb == 0u)
                break;
            if (n1[i + 0xEu] == stk[0x34]) {
                stk[i] = 0;
                D4V_TBL_45AFB8_32(&c->st, 0) -= lenb;
                break;
            }
            i++;
        }
    }
    {
        unsigned int to_aa7;
        to_aa7 = 1;
        if ((unsigned)n1[0] < D4V_TBL_45AFB8_32(&c->st, 0)) {
            e_eax = ((e_eax & ~0xFFu) | (stk[0x28])); D4V_CTX_B_45C683_8(&c->st, 0) = ((e_eax & 0xFFu)); e_eax = ((e_eax & ~(0xFFu << 8)) | ((stk[0x1C]) << 8));
            e_eax = ((e_eax & ~0xFFu) | (((((((((((e_eax >> 8) & 0xFFu)) & (((e_eax >> 8) & 0xFFu)))) & 0xFFu) == 0u) )) & 0xFFu))); D4V_CTX_B_45C68F_8(&c->st, 0) = ((e_eax & 0xFFu));
            if ((e_eax & 0xFFu) != 0u) {
            if (n1[0] == 0x1u
        && (n1[1] | ((unsigned)n1[2] << 8) | ((unsigned)n1[3] << 16) | ((unsigned)n1[4] << 24)) < 0x50u) {
        unsigned int idx;
        unsigned int add;
        idx = d4v_t433E8C[n1[1]] + (D4V_TBL_45AFC4_32(&c->st, 0) & 0xF0u);
        if (D4V_CTX_B_45C67F_8(&c->st, 0) == 0) {
            unsigned int hi;
            unsigned int lo;
            hi = (unsigned int)d4v_t436B8A[(D4V_TBL_45AFC4_32(&c->st, 0) >> 8) & 0xFFu];
            lo = (unsigned int)d4v_t436B8A[D4V_TBL_45AFC4_8(&c->st, 0)];
            add = (lo << 3) + (hi << 2);
        } else
            add = (D4V_TBL_45AFC4_32(&c->st, 0) >> 11) & 0xCu;
        e_eax = idx + add;
        e_eax = e_eax + e_eax + D4V_TBL_45AA80;
        e_eax = (unsigned)d4vc_ppm_pair_price(c, e_eax);
        if ((e_eax & 0xFFu) == 0u) {
            unsigned int n;
            n = D4V_CTX_B_45C68A_8(&c->st, 0);
            D4V_CTX_B_45C692_8(&c->st, 0) = 0xD;
            D4V_CTX_B_45C65E_8(&c->st, n) = n1[0xE];
            D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)((n + 1u) & 0xFFu);
        } else {
            stk[0x24] = n1[0xE];
            e_ebx = e_ebx & ~0xFFu;
            stk[0x20] = 0;
            D4V_CTX_B_45C692_8(&c->st, 0) = 0;
        }
        to_aa7 = 0;
    } else
{
 e_edi = (0xFFFFFFFFu); e_eax = (D4V_TBL_45AFB8_32(&c->st, 0)); e_ecx = ((e_ecx & ~0xFFu) | (n1[0])); e_esi = (0u); e_edx = ((e_edx & ~(0xFFu << 8)) | ((0u) << 8));
    D4V_RSLOT_LO_32(&c->st, 0) = (e_eax); D4V_RSLOT_MID_32(&c->st, 0) = (e_esi); D4V_CTX_B_45C692_8(&c->st, 0) = (((e_edx >> 8) & 0xFFu));
    ;
    if (((e_ecx & 0xFFu)) < (0x20u)) {
        unsigned int diff;
        diff = e_eax - (e_ecx & 0xFFu);
        e_eax = diff;
        e_edx = diff;
        if (diff < 0x80u) {
            unsigned int adj;
            e_ecx = (unsigned int)d4v_t433FCC[diff & 0xFFu] << 3;
            e_edi = (unsigned int)d4v_t43404C[n1[0]] + e_ecx;
            if (D4V_CTX_B_45C67F_8(&c->st, 0) == 0) {
                unsigned int hi;
                unsigned int lo;
                hi = (unsigned int)d4v_t436B8A[(D4V_TBL_45AFC4_32(&c->st, 0) >> 8) & 0xFFu];
                lo = (unsigned int)d4v_t436B8A[D4V_TBL_45AFC4_8(&c->st, 0)];
                adj = (hi << 8) + (lo << 7);
                adj += ((D4V_TBL_45AFC4_8(&c->st, 0) == 0xDFu) ? 1u : 0u) << 9;
            } else {
                adj = D4V_TBL_45AFC4_32(&c->st, 0) & 0xC0u;
                adj = adj + adj;
            }
            e_edi = e_edi + adj;
            e_eax = (unsigned)d4vc_freq_scale_step(c, e_edi + e_edi + D4V_TBL_45A280, e_edx);
            D4V_RSLOT_LO_32(&c->st, 0) = e_eax;
        }
    }
    e_ecx = (D4V_RSLOT_LO_32(&c->st, 0)); e_eax = (D4V_RC_RANGE_32(&c->st, 0)); e_edx = (0u);
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) ;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) ;
    e_edx = (0u); D4V_RC_RANGE_32(&c->st, 0) = (e_eax); e_ecx = (e_eax); e_eax = (D4V_RC_CODE_32(&c->st, 0));
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) ;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) ;
    e_esi = (e_eax);
    {
        unsigned int via0ea;
        via0ea = 1;
        while (1) {
            unsigned int i;
            unsigned int lenb;
            i = D4V_CTX_B_45C692_8(&c->st, 0);
            if (stk[i] != 0) {
                lenb = n1[i + 1];
                if (lenb == 0) {
                    D4V_CTX_B_45C692_8(&c->st, 0) = 0xD;
                    break;
                }
                {
                    unsigned int hi;
                    hi = D4V_RSLOT_MID_32(&c->st, 0) + lenb;
                    D4V_RSLOT_HI_32(&c->st, 0) = hi;
                    if ((e_esi & 0xFFFFu) >= hi) {
                        unsigned int n;
                        n = D4V_CTX_B_45C68A_8(&c->st, 0);
                        D4V_CTX_B_45C65E_8(&c->st, n) = n1[i + 0xE];
                        D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)((n + 1u) & 0xFFu);
                        D4V_RSLOT_MID_32(&c->st, 0) = hi;
                    } else {
                        stk[0x24] = n1[i + 0xE];
                        stk[0x20] = 0;
                        if (D4V_CTX_B_45C692_8(&c->st, 0) != 0xD)
                            via0ea = 0;
                        break;
                    }
                }
            }
            i = (D4V_CTX_B_45C692_8(&c->st, 0) + 1u) & 0xFFu;
            D4V_CTX_B_45C692_8(&c->st, 0) = (unsigned char)i;
            if (i >= 0xDu) {
                if (i != 0xDu)
                    via0ea = 0;
                break;
            }
        }
        if (via0ea)
            D4V_RSLOT_HI_32(&c->st, 0) = D4V_RSLOT_LO_32(&c->st, 0);
    }
    if (e_edi != 0xFFFFFFFFu) {
        e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((stk[0x20]) << 8)); e_edx = (0u);
        e_edx = ((e_edx & ~0xFFu) | (((((((((((e_ebx >> 8) & 0xFFu)) & (((e_ebx >> 8) & 0xFFu)))) & 0xFFu) == 0u) )) & 0xFFu))); e_eax = (e_edi + e_edi);
        e_eax = ((e_eax) + (D4V_TBL_45A280));
        d4vc_counter_bump_halve(c, e_eax, e_edx);
    }
    e_eax = (D4V_RSLOT_LO); e_edx = (d4v_slot32(c, 8u)); e_eax = (d4v_slot32(c, 4u)); e_ecx = (D4V_RC_RANGE_32(&c->st, 0)); e_edx = ((e_edx) - (e_eax));
    d4v_q = ((UHI64)(int)(e_eax)) * ((UHI64)(int)(e_ecx));
    e_eax = (unsigned)d4v_q  ;
    d4v_q = ((UHI64)(int)(e_edx)) * ((UHI64)(int)(e_ecx));
    e_edx = (unsigned)d4v_q  ;
    e_ecx = (D4V_RC_CODE_32(&c->st, 0)); e_ecx = ((e_ecx) - (e_eax)); ;
    d4v_rcrenorm_reg(c, &e_ecx, &e_edx, &e_eax);
D4V_RC_CODE_32(&c->st, 0) = (e_ecx); D4V_RC_RANGE_32(&c->st, 0) = (e_edx);
        }

            }
        }
        if (!to_aa7) {
    e_eax = (D4V_RSLOT_LO); e_edx = (d4v_slot32(c, 8u)); e_eax = (d4v_slot32(c, 4u)); e_ecx = (D4V_RC_RANGE_32(&c->st, 0)); e_edx = ((e_edx) - (e_eax));
    d4v_q = ((UHI64)(int)(e_eax)) * ((UHI64)(int)(e_ecx));
    e_eax = (unsigned)d4v_q  ;
    d4v_q = ((UHI64)(int)(e_edx)) * ((UHI64)(int)(e_ecx));
    e_edx = (unsigned)d4v_q  ;
    e_ecx = (D4V_RC_CODE_32(&c->st, 0)); e_ecx = ((e_ecx) - (e_eax)); ;
    d4v_rcrenorm_reg(c, &e_ecx, &e_edx, &e_eax);
D4V_RC_CODE_32(&c->st, 0) = (e_ecx); D4V_RC_RANGE_32(&c->st, 0) = (e_edx);
        }
    }
    }
}

    }
    if (stk[0x20] == 0) {
        d4vc_sym_emit(c, stk);
        return;
    }
    if (D4V_CTX_B_45C67F_8(&c->st, 0) == 0)
        e_eax = 0;
    else {
        e_eax = (unsigned)d4vc_ppm_score_orders(c, e_ebx);
        if (e_eax != 0u) {
            unsigned int sord;
            sord = (unsigned int)D4V_CTX_B_45C682_8(&c->st, 0);
            sc = c->blkord[sord] + (e_eax - c->blkord_base[sord]);
        }
    }
    stk[0x38] = 1u;
    e_edx = e_edx & ~(0xFFu << 8);
    stk[0x18] = (unsigned char)e_eax;
    stk[0x19] = (unsigned char)(e_eax >> 8);
    stk[0x1A] = (unsigned char)(e_eax >> 16);
    stk[0x1B] = (unsigned char)(e_eax >> 24);
    stk[0x3C] = 0;
    if (e_eax != 0u)
        D4V_TBL_45AFC0_32(&c->st, 0) = D4V_TBL_45AFC0_32(&c->st, 0) + 2u;
    e_esi = e_eax;
    if (e_esi != 0u) {
        unsigned int left;
        unsigned int right;
        left = (unsigned)sc[0] * 3u;
        right = (unsigned)sc[1] * 5u;
        if ((int)left < (int)right && (sc[3] & 0x7Fu) <= 2u)
            e_eax = 1u;
        else
            e_eax = 0u;
    } else
        e_eax = 1u;
    D4V_CTX_B_45C68B_8(&c->st, 0) = (unsigned char)e_eax;
    do {
    {
        unsigned int run;
        unsigned int do_renorm;
        run = 1;
        do_renorm = 0;
        if (D4V_CTX_B_45C68B_8(&c->st, 0) == 0 && stk[0x3C] == 0)
            run = 0;
        else {
            unsigned int keep;
            e_ecx = e_ecx & ~0xFFu;
            e_edi = D4V_TBL_45AFCC_32(&c->st, 0);
            stk[0x38] = 0;
            if (e_edi == 0u || p2[0x11] == 0)
                keep = 1;
            else if (D4V_TBL_45AFD4_32(&c->st, 0) < 0xFFFFu
                && d4v_gld32(c->st.g.bytes + (D4V_TBL_45AFA0_32(&c->st, 0) - D4V_GLOBALS_BASE)) <= 0x3FEu)
                keep = 1;
            else
                keep = 0;
            e_eax = keep;
            D4V_CTX_B_45C68B_8(&c->st, 0) = (unsigned char)keep;
            if (keep == 0 || p2[0] == 0)
                run = 0;
        }
        if (run) {
    e_edi = (D4V_SP + 0x10u); e_eax = ((e_eax & ~(0xFFu << 8)) | ((0u) << 8)); e_esi = (0x4150E8u); stk[0x48] = (((e_eax >> 8) & 0xFFu));
     stk[0x10] = 1u; stk[0x11] = 1u; stk[0x12] = 1u; stk[0x13] = 1u; stk[0x14] = 1u; stk[0x15] = 1u; stk[0x16] = 1u; stk[0x17] = 1u;
    e_esi += 8u; e_edi += 8u;
    /* above: movsd pair: 8 code-const 0x01 bytes (e_esi was ) to stk[0x10..0x17]; UNCERTAIN(src-bytes): inferred from 01 01 opcode display */
    stk[0x40] = (((e_eax >> 8) & 0xFFu)); e_eax = ((e_eax & ~0xFFu) | ((p2[0x12]))); stk[0x44] = ((e_eax & 0xFFu));
    {
        unsigned int k;
        k = stk[0x48];
        while (1) {
            unsigned int i;
            unsigned int limit;
            if (p2[k + 1] == 0)
                break;
            stk[0x40] = (unsigned char)(stk[0x40] + 1u);
            limit = D4V_CTX_B_45C68A_8(&c->st, 0);
            i = 0;
            while (i < limit) {
                if (D4V_CTX_B_45C65E_8(&c->st, i) == p2[k + 9]) {
                    stk[k + 0x10] = 0;
                    stk[0x44] = (unsigned char)(stk[0x44] - p2[k + 1]);
                    break;
                }
                i++;
            }
            k = (k + 1u) & 0xFFu;
            stk[0x48] = (unsigned char)k;
            if (k >= 8u)
                break;
        }
    }
    e_ebx = (e_ebx & ~0xFFu) | 0xFFu;
    e_eax = stk[0x44];
    e_ecx = p2[0];
    D4V_TBL_45AFC8_32(&c->st, 0) = e_eax;
    e_edx = e_eax;
    e_ecx = e_ecx + e_ecx * 4u;
    e_eax = e_edx * 4u;
    D4V_CTX_B_45C694_8(&c->st, 0) = 0xFFu;
    {
        unsigned int skip_renorm;
        skip_renorm = 1;
        if ((int)e_eax > (int)e_ecx) {
            e_eax = ((unsigned)p2[0x11] << 2) + D4V_TBL_45AFCC_32(&c->st, 0);
            if (e_eax < 0x10u) {
                D4V_CTX_B_45C687_8(&c->st, 0) = stk[0x28];
                e_ebx = (e_ebx & ~(0xFFu << 8)) | ((unsigned)stk[0x1C] << 8);
                if (stk[0x1C] == 0) {
                    if (p2[0] != 3u
                        || (p2[1] | ((unsigned)p2[2] << 8) | ((unsigned)p2[3] << 16) | ((unsigned)p2[4] << 24)) >= 0x10u) {
                        e_ecx = ((e_ecx & ~(0xFFu << 8)) | ((stk[0x40]) << 8)); e_esi = (0xFFFFFFFFu);
    if ((((e_ecx >> 8) & 0xFFu)) <= (0x4u)) {
        unsigned int diff;
        diff = (unsigned int)stk[0x44] - (unsigned int)p2[0];
        if (diff < 0x10u) {
            e_edx = diff;
            e_esi = (((diff & 0xCu) + (unsigned int)stk[0x40]) - 1u);
            e_eax = (unsigned)d4vc_freq_scale_step(c, e_esi + e_esi + D4V_TBL_45A258, e_edx);
            stk[0x44] = (unsigned char)(e_eax & 0xFFu);
        }
    }
    e_eax = (0u); e_eax = ((e_eax & ~0xFFu) | (stk[0x44])); e_edi = (0u); e_ecx = (e_eax); D4V_RSLOT_LO_32(&c->st, 0) = (e_eax); e_eax = (D4V_RC_RANGE_32(&c->st, 0)); e_edx = (0u);
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) ;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) ;
    e_edx = (0u); D4V_RC_RANGE_32(&c->st, 0) = (e_eax); e_ecx = (e_eax); e_eax = (D4V_RC_CODE_32(&c->st, 0));
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) ;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) ;
    e_ebx = (e_eax); e_eax = ((e_eax & ~0xFFu) | (0u)); D4V_RSLOT_MID_32(&c->st, 0) = (e_edi); D4V_CTX_B_45C694_8(&c->st, 0) = ((e_eax & 0xFFu));
    {
        unsigned int via373;
        via373 = 1;
        while (1) {
            unsigned int i;
            unsigned int lenb;
            i = D4V_CTX_B_45C694_8(&c->st, 0);
            if (stk[i + 0x10] != 0) {
                lenb = p2[i + 1];
                if (lenb == 0) {
                    D4V_CTX_B_45C694_8(&c->st, 0) = 8;
                    break;
                }
                {
                    unsigned int hi;
                    hi = D4V_RSLOT_MID_32(&c->st, 0) + lenb;
                    D4V_RSLOT_HI_32(&c->st, 0) = hi;
                    if ((e_ebx & 0xFFFFu) >= hi) {
                        unsigned int n;
                        n = D4V_CTX_B_45C68A_8(&c->st, 0);
                        D4V_RSLOT_MID_32(&c->st, 0) = hi;
                        D4V_CTX_B_45C65E_8(&c->st, n) = p2[i + 9];
                        D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)((n + 1u) & 0xFFu);
                    } else {
                        unsigned int scaled;
                        stk[0x24] = p2[i + 9];
                        stk[0x20] = 0;
                        scaled = lenb << 2;
                        if (scaled >= stk[0x44]) {
                            unsigned int flag;
                            flag = stk[0x44] > 0x10u;
                            D4V_CTX_B_45C693_8(&c->st, 0) = (unsigned char)(flag + 1u);
                        }
                        if (D4V_CTX_B_45C694_8(&c->st, 0) != 8)
                            via373 = 0;
                        break;
                    }
                }
            }
            i = (D4V_CTX_B_45C694_8(&c->st, 0) + 1u) & 0xFFu;
            D4V_CTX_B_45C694_8(&c->st, 0) = (unsigned char)i;
            if (i >= 8u) {
                if (i != 8u)
                    via373 = 0;
                break;
            }
        }
        if (via373)
            D4V_RSLOT_HI_32(&c->st, 0) = D4V_RSLOT_LO_32(&c->st, 0);
    }
    if (e_esi != 0xFFFFFFFFu) {
        e_ebx = ((e_ebx & ~0xFFu) | (stk[0x20])); e_edx = (0u);
        e_edx = ((e_edx & ~0xFFu) | ((((((((((e_ebx & 0xFFu)) & ((e_ebx & 0xFFu)))) & 0xFFu) == 0u) )) & 0xFFu))); e_eax = (e_esi + e_esi);
        e_eax = ((e_eax) + (D4V_TBL_45A258));
        d4vc_counter_bump_halve(c, e_eax, e_edx);
    }
                        do_renorm = 1;
                    } else
                    skip_renorm = 0;
                }
            }
        }
        if (!skip_renorm) {
    e_eax = (unsigned)((int)p2[1] >> 2);
    e_eax = e_eax + e_eax + D4V_CTX_B_45A278;
    e_eax = (unsigned)d4vc_ppm_pair_price(c, e_eax);
    if ((e_eax & 0xFFu) == 0) {
        unsigned int n;
        n = D4V_CTX_B_45C68A_8(&c->st, 0);
        D4V_CTX_B_45C694_8(&c->st, 0) = 8;
        D4V_CTX_B_45C65E_8(&c->st, n) = p2[9];
        D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)(n + 1u);
    } else {
        stk[0x24] = p2[9];
        e_eax = e_edx;
        e_edx = (unsigned)((int)e_edx >> 31);
        e_edx = e_edx << 2;
        f_cf = (e_edx >> 30) & 1u;
        f_zf = (e_edx << 2) == 0u;
        f_sf = ((e_edx << 2) >> 31) & 1u;
        d4v_q = (unsigned UHI64)e_edx + f_cf;
        e_eax = e_eax - (unsigned)d4v_q;
        e_eax = (unsigned)((int)e_eax >> 2);
        e_ebx = e_ebx & ~0xFFu;
        e_ecx = p2[1];
        stk[0x20] = 0;
        D4V_CTX_B_45C694_8(&c->st, 0) = 0;
        if ((int)e_ecx > (int)e_eax)
            D4V_CTX_B_45C693_8(&c->st, 0) =
                (unsigned char)((D4V_CTX_B_45C693_8(&c->st, 0) + 1u) & 0xFFu);
    }
            do_renorm = 1;
        }
    }
    if (do_renorm) {
    e_eax = (D4V_RSLOT_LO); e_edx = (d4v_slot32(c, 8u)); e_eax = (d4v_slot32(c, 4u)); e_ecx = (D4V_RC_RANGE_32(&c->st, 0)); e_edx = ((e_edx) - (e_eax));
    d4v_q = ((UHI64)(int)(e_eax)) * ((UHI64)(int)(e_ecx));
    e_eax = (unsigned)d4v_q  ;
    d4v_q = ((UHI64)(int)(e_edx)) * ((UHI64)(int)(e_ecx));
    e_edx = (unsigned)d4v_q  ;
    e_ecx = (D4V_RC_CODE_32(&c->st, 0)); e_ecx = ((e_ecx) - (e_eax)); ;
    d4v_rcrenorm_reg(c, &e_ecx, &e_edx, &e_eax);
D4V_RC_CODE_32(&c->st, 0) = (e_ecx); D4V_RC_RANGE_32(&c->st, 0) = (e_edx);
    }
        }
    }
    e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((stk[0x3C]) << 8)); ;
    if ((((((((e_ebx >> 8) & 0xFFu)) & (((e_ebx >> 8) & 0xFFu)))) & 0xFFu) == 0u)) {
 e_eax = ((e_eax & ~0xFFu) | (stk[0x20])); D4V_CTX_B_45C680_8(&c->st, 0) = ((e_eax & 0xFFu));
    ;
    if ((e_eax & 0xFFu) != 0u) {
    e_ecx = ((e_ecx & ~(0xFFu << 8)) | ((0x1u) << 8)); e_eax = ((e_eax & ~0xFFu) | (D4V_CTX_B_45C67F_8(&c->st, 0))); stk[0x3C] = (((e_ecx >> 8) & 0xFFu));
    e_ebx = ((e_ebx & ~0xFFu) | (((e_ecx >> 8) & 0xFFu))); ;
    if ((e_eax & 0xFFu) == 0u)
        D4V_CTX_B_45C680_8(&c->st, 0) = (((e_ebx >> 8) & 0xFFu));
    if ((stk[0x18] | ((unsigned)stk[0x19] << 8) | ((unsigned)stk[0x1A] << 16) | ((unsigned)stk[0x1B] << 24)) != 0u) {
    e_edx = (D4V_TBL_45AFC0_32(&c->st, 0)); e_ecx = (stk[0x18] | ((unsigned)stk[0x19] << 8) | ((unsigned)stk[0x1A] << 16) | ((unsigned)stk[0x1B] << 24)); e_edx = ((e_edx) >> 2);
    e_eax = (0u); D4V_TBL_45AFC0_32(&c->st, 0) = (e_edx);
    while (e_eax < (unsigned)D4V_CTX_B_45C68A_8(&c->st, 0)) {
        unsigned int idx;
        unsigned int b;
        unsigned int mark;
        idx = e_eax;
        b = D4V_CTX_B_45C65E_8(&c->st, idx);
        e_eax += 1u;
        if (b != sc[2])
            continue;
        mark = sc[1];
        e_ebx = (e_ebx & ~0xFFu);
        e_ebx = (e_ebx & ~(0xFFu << 8)) | (mark << 8);
        if (mark <= 3u) {
            unsigned int top;
            top = (unsigned)((int)(*(unsigned char *)d4v_ptr(c, (uint32_t)e_ecx)) >> 6);
            e_eax = mark;
            if ((int)top > (int)e_eax) {
                e_ecx = e_ecx & ~0xFFu;
                D4V_CTX_B_45C680_8(&c->st, 0) = 0;
            }
        } else {
            e_ecx = e_ecx & ~0xFFu;
            D4V_CTX_B_45C680_8(&c->st, 0) = 0;
        }
        break;
    }

    if ((e_ebx & 0xFFu) != 0u) {
    e_eax = ((e_eax & ~0xFFu) | (stk[0x28])); e_ecx = ((e_ecx & ~(0xFFu << 8)) | ((stk[0x1C]) << 8)); D4V_CTX_B_45C685_8(&c->st, 0) = ((e_eax & 0xFFu));
    if ((((e_ecx >> 8) & 0xFFu)) == 0u) {
    e_ecx = (D4V_TBL_45BA4C_32(&c->st, 0)); ;
    if (e_ecx == 0u) {
        e_ebx = (e_ebx & ~0xFFu) | sc[0];
        e_eax = (e_eax & ~0xFFu) | sc[1];
    } else {
        unsigned char *p4;
        p4 = d4v_span(c, e_ecx, 2);
        if (p4) {
            e_ebx = ((e_ebx & ~0xFFu) | (p4[0]));
            e_eax = ((e_eax & ~0xFFu) | (p4[1]));
        } else {
            e_ebx = ((e_ebx & ~0xFFu) | ((*(unsigned char *)d4v_ptr(c, (uint32_t)(e_ecx)))));
            e_eax = ((e_eax & ~0xFFu) | ((*(unsigned char *)d4v_ptr(c, (uint32_t)(e_ecx + 0x1u)))));
        }
    }
e_esi = (e_ebx & 0xFFu); e_eax = ((e_eax) & (0xFFu)); e_edx = (e_esi + e_eax); e_ecx = (e_edx); D4V_RSLOT_LO_32(&c->st, 0) = (e_edx); e_eax = (D4V_RC_RANGE_32(&c->st, 0)); e_edx = (0u);
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) ;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) ;
    e_edx = (0u); D4V_RC_RANGE_32(&c->st, 0) = (e_eax); e_ecx = (e_eax); e_eax = (D4V_RC_CODE_32(&c->st, 0));
    d4v_q = (((unsigned UHI64)e_edx << 32) | e_eax) ;
    e_eax = (unsigned)(d4v_q / ((unsigned UHI64)(e_ecx)));
    e_edx = (unsigned)(d4v_q % ((unsigned UHI64)(e_ecx))) ;
    e_eax = ((e_eax) & (0xFFFFu)); ;
    if ((int)e_eax >= (int)e_esi) {
        if (D4V_CTX_B_45C684_8(&c->st, 0) != 0) {
            e_eax = (D4V_TBL_45AFA0_32(&c->st, 0));
            d4v_gst32(c->st.g.bytes + (e_eax - D4V_GLOBALS_BASE), d4v_gld32(c->st.g.bytes + (e_eax - D4V_GLOBALS_BASE)) >> 1);
            e_eax = (D4V_TBL_45AF78_32(&c->st, 0));
            d4v_gst32(c->st.g.bytes + (e_eax - D4V_GLOBALS_BASE), d4v_gld32(c->st.g.bytes + (e_eax - D4V_GLOBALS_BASE)) >> 1);
            e_eax = (D4V_TBL_45AFD4_32(&c->st, 0));
            e_eax = (e_eax >> 1) + 0xFFu;
            D4V_TBL_45AFD4_32(&c->st, 0) = (e_eax);
        }
        {
            unsigned int n;
            n = D4V_CTX_B_45C68A_8(&c->st, 0);
            D4V_RSLOT_MID_32(&c->st, 0) = (e_ebx & 0xFFu);
            D4V_RSLOT_HI_32(&c->st, 0) = D4V_RSLOT_LO_32(&c->st, 0);
            D4V_CTX_B_45C65E_8(&c->st, n) = sc[2];
            D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)((n + 1u) & 0xFFu);
        }
    } else {
        if (D4V_CTX_B_45C684_8(&c->st, 0) != 0) {
            e_eax = (D4V_TBL_45AFA0_32(&c->st, 0));
            e_edx = (D4V_TBL_45AFD4_32(&c->st, 0));
            d4v_gst32(c->st.g.bytes + (e_eax - D4V_GLOBALS_BASE), d4v_gld32(c->st.g.bytes + (e_eax - D4V_GLOBALS_BASE)) + (e_edx));
            e_eax = (D4V_TBL_45AF78_32(&c->st, 0));
            e_esi = (d4v_gld32(c->st.g.bytes + (e_eax - D4V_GLOBALS_BASE)));
            e_esi = e_esi + e_edx;
            e_edi = e_edx + e_edx;
            d4v_gst32(c->st.g.bytes + (e_eax - D4V_GLOBALS_BASE), e_esi);
            D4V_TBL_45AFD4_32(&c->st, 0) = e_edi;
            if (e_edi > 0xFFFFu)
                D4V_TBL_45AFD4_32(&c->st, 0) = 0xFFFFu;
        }
        D4V_RSLOT_HI_32(&c->st, 0) = (e_ebx & 0xFFu);
        D4V_RSLOT_MID_32(&c->st, 0) = 0;
        stk[0x20] = 0;
        stk[0x24] = sc[2];
    }
e_eax = (D4V_RSLOT_LO); e_edx = (d4v_slot32(c, 8u)); e_eax = (d4v_slot32(c, 4u)); e_ecx = (D4V_RC_RANGE_32(&c->st, 0)); e_edx = ((e_edx) - (e_eax));
    d4v_q = ((UHI64)(int)(e_eax)) * ((UHI64)(int)(e_ecx));
    e_eax = (unsigned)d4v_q  ;
    d4v_q = ((UHI64)(int)(e_edx)) * ((UHI64)(int)(e_ecx));
    e_edx = (unsigned)d4v_q  ;
    e_ecx = (D4V_RC_CODE_32(&c->st, 0)); e_ecx = ((e_ecx) - (e_eax)); ;
    d4v_rcrenorm_reg(c, &e_ecx, &e_edx, &e_eax);
D4V_RC_CODE_32(&c->st, 0) = (e_ecx); D4V_RC_RANGE_32(&c->st, 0) = (e_edx);
    }
    }
    }
    }
    if (stk[0x20] != 0 && stk[0x38] != 0)
        continue;
    }
    break;
    } while (1);

    if ((stk[0x20]) == (0x0u)) { d4vc_sym_emit(c, stk); return; }
    e_eax = ((e_eax & ~0xFFu) | (stk[0x28])); e_ecx = (0x201u); D4V_CTX_B_45C68D_8(&c->st, 0) = ((e_eax & 0xFFu)); e_eax = ((e_eax & ~0xFFu) | (D4V_CTX_B_45C67F_8(&c->st, 0))); e_edi = (0x302u);
    ;
    if ((e_eax & 0xFFu) == 0u
        || D4V_TBL_45AF98_32(&c->st, 0) <= 1u
        || d4v_gld32(c->st.g.bytes + (D4V_TBL_45AFA0_32(&c->st, 0) - D4V_GLOBALS_BASE)) == 0u)
    {
        e_eax = (D4V_TBL_45AFC4_32(&c->st, 0));
        e_ebx = ((e_ebx & ~0xFFu) | (D4V_TBL_45AFC4_8(&c->st, 0)));
        e_eax = ((e_eax) >> 8);
        e_esi = (e_ebx & 0xFFu);
        if (((e_ebx & 0xFFu)) == ((e_eax & 0xFFu)))
            e_ecx = ((e_ecx) + 1u);
        if (D4V_CTX_B_45C67F_8(&c->st, 0) == 0
            && d4v_t436B8A[(D4V_TBL_45AFC4_32(&c->st, 0) >> 8) & 0xFFu] != 0)
            e_esi = ((e_esi) + (0x100u));
    } else {
    e_ebx = (D4V_TBL_45AF98_32(&c->st, 0));
    e_eax = (D4V_DICT_CURSOR_32(&c->st, 0)); e_edx = (D4V_DICT_MASK_32(&c->st, 0)); e_eax = ((e_eax) - (e_ebx)); e_edx = ((e_edx) & (e_eax)); e_eax = (D4V_BLK_PTR_00_DICT_32(&c->st, 0)); e_ebp = (e_ebx);
    e_ebx = ((e_ebx & ~0xFFu) | (c->blk00_dict[(e_edx + e_eax) - c->blk00_base])); e_edx = (e_ebp); e_ebp = (D4V_DICT_CURSOR_32(&c->st, 0)); e_edx = ((e_edx) + (e_edx)); e_ebp = ((e_ebp) - (e_edx));
    e_edx = (e_ebp); e_edx = ((e_edx) & (D4V_DICT_MASK_32(&c->st, 0))); e_esi = (e_ebx & 0xFFu); e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((c->blk00_dict[(e_edx + e_eax) - c->blk00_base]) << 8));
    e_esi = ((e_esi) + (0x100u)); ;
    if (((e_ebx & 0xFFu)) == (((e_ebx >> 8) & 0xFFu)))
        e_ecx = (0x202u);
    }
    if ((stk[0x4C]) != (0x0u)) {
        e_ecx = (D4V_TBL_45AF3C_32(&c->st, 0));
        e_edi = (D4V_CTX_B_45C67D_8(&c->st, 0));
        e_ebx = ((e_ebx & ~0xFFu) | (D4V_CTX_B_45C679_8(&c->st, 0)));
        e_edi = ((e_edi) + (0x302u));
    }
    if ((stk[0x4C]) != (0x0u) && (D4V_CTX_B_45C67B_8(&c->st, 0)) != (0x0u))
        e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((0x1u) << 8));
    else
        e_ebx = ((e_ebx & ~(0xFFu << 8)) | ((0u) << 8));
    e_edx = ((e_edx & ~(0xFFu << 8)) | ((stk[0x30]) << 8)); stk[0x2C] = (((e_ebx >> 8) & 0xFFu));
    ;
    if ((((e_edx >> 8) & 0xFFu)) == 0u) {
        e_edx = (0x2FFu); e_eax = (e_esi);
        e_eax = ((e_eax & ~0xFFu) | ((unsigned)(d4vc_ppm_sym_split(c, e_eax, e_edx, e_ebx)) & 0xFFu));
        stk[0x24] = ((e_eax & 0xFFu));
        if ((D4V_CTX_B_45C67F_8(&c->st, 0)) == (0x0u)) { d4vc_sym_emit(c, stk); return; }
        if (((((e_ebx >> 8) & 0xFFu)) == 0u)) {
            unsigned int gap;
            gap = ((e_eax & 0xFFu) - (e_ebx & 0xFFu)) & 0xFFu;
            e_ebx = (e_ebx & ~(0xFFu << 8)) | (gap << 8);
        } else {
            unsigned int gap;
            gap = ((e_ebx & 0xFFu) - (e_eax & 0xFFu)) & 0xFFu;
            e_ebx = (e_ebx & ~(0xFFu << 8)) | (gap << 8);
        }
        e_eax = 0u;
        if (stk[0x2C] != 0) {
            while (e_eax < (unsigned)D4V_CTX_B_45C68A_8(&c->st, 0)) {
                unsigned int diff;
                diff = ((e_ebx & 0xFFu) - (unsigned)D4V_CTX_B_45C65E_8(&c->st, e_eax)) & 0xFFu;
                e_eax += 1u;
                D4V_CTX_B_45C65D_8(&c->st, e_eax) = (unsigned char)diff;
            }
        } else {
            while (e_eax < (unsigned)D4V_CTX_B_45C68A_8(&c->st, 0)) {
                unsigned int idx;
                idx = e_eax;
                e_eax += 1u;
                D4V_CTX_B_45C65E_8(&c->st, idx) = (unsigned char)(
                    ((unsigned)D4V_CTX_B_45C65E_8(&c->st, idx) - (e_ebx & 0xFFu)) & 0xFFu);
            }
        }
        e_ebx = (e_ebx & ~0xFFu) | ((e_ebx >> 8) & 0xFFu);
        e_ebx = e_ebx & 0xFFu;
        e_edx = e_edi;
        e_eax = e_ecx;
        d4vc_freq_adapt_459398(c, e_eax, e_edx, e_ebx);
        d4vc_sym_emit(c, stk);
        return;
    }
    e_eax = 0u;
    if ((((e_ebx >> 8) & 0xFFu)) == 0u) {
        while (e_eax < (unsigned)D4V_CTX_B_45C68A_8(&c->st, 0)) {
            unsigned int idx;
            unsigned int diff;
            idx = e_eax;
            diff = ((unsigned)D4V_CTX_B_45C65E_8(&c->st, idx) - (e_ebx & 0xFFu)) & 0xFFu;
            e_ebx = (e_ebx & ~(0xFFu << 8)) | (diff << 8);
            e_eax += 1u;
            D4V_CTX_B_45C65E_8(&c->st, idx) = (unsigned char)diff;
        }
    } else {
        while (e_eax < (unsigned)D4V_CTX_B_45C68A_8(&c->st, 0)) {
            unsigned int diff;
            diff = ((e_ebx & 0xFFu) - (unsigned)D4V_CTX_B_45C65E_8(&c->st, e_eax)) & 0xFFu;
            e_eax += 1u;
            D4V_CTX_B_45C65D_8(&c->st, e_eax) = (unsigned char)diff;
        }
    }
    e_edx = e_edi;
    e_eax = e_ecx;
    e_ebx = (e_ebx & ~(0xFFu << 8)) | ((unsigned)stk[0x2C] << 8);
    e_eax = (unsigned)d4vc_ppm_sym_split(c, e_eax, e_edx, e_ebx) & 0xFFu;
    if (((e_ebx >> 8) & 0xFFu) == 0) {
        e_eax = (e_eax + (e_ebx & 0xFFu)) & 0xFFu;
        stk[0x24] = (unsigned char)e_eax;
    } else {
        unsigned int gap;
        gap = ((e_ebx & 0xFFu) - e_eax) & 0xFFu;
        e_eax = (e_eax & ~(0xFFu << 8)) | (gap << 8);
        stk[0x24] = (unsigned char)gap;
    }
    e_ecx = (e_ecx & ~0xFFu) | (unsigned)stk[0x2C];
    e_eax = 0u;
    if (stk[0x2C] == 0) {
        while (e_eax < (unsigned)D4V_CTX_B_45C68A_8(&c->st, 0)) {
            unsigned int idx;
            unsigned int sum;
            idx = e_eax;
            sum = ((unsigned)D4V_CTX_B_45C65E_8(&c->st, idx) + (e_ebx & 0xFFu)) & 0xFFu;
            e_eax += 1u;
            D4V_CTX_B_45C65E_8(&c->st, idx) = (unsigned char)sum;
        }
    } else {
        while (e_eax < (unsigned)D4V_CTX_B_45C68A_8(&c->st, 0)) {
            unsigned int diff;
            diff = ((e_ebx & 0xFFu) - (unsigned)D4V_CTX_B_45C65E_8(&c->st, e_eax)) & 0xFFu;
            e_eax += 1u;
            D4V_CTX_B_45C65D_8(&c->st, e_eax) = (unsigned char)diff;
        }
    }
    e_edx = (0x2FFu); e_ebx = (0u); e_eax = (e_esi); e_ebx = ((e_ebx & ~0xFFu) | (stk[0x24]));
    d4vc_freq_adapt_459398(c, e_eax, e_edx, e_ebx);
    d4vc_sym_emit(c, stk);
    return;

}

/* ---- workers: range-splitter cluster (from draft_s1.c) ---- */
unsigned int d4vc_ppm_sym_split(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in);
unsigned int d4vc_ppm_prob_decode(dec4v_ctx *c, unsigned int eax_in);
unsigned int d4vc_freq_divide_price(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in);
void d4vc_freq_adapt_459398(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in);
void d4vc_freq_adapt_45af60(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in);
unsigned int d4vc_freq_scale_step(dec4v_ctx *c, unsigned int tab, unsigned int edx_in);
void d4vc_counter_bump_halve(dec4v_ctx *c, unsigned int tab, unsigned int flag_in);

/* sub_419970: range/PPM symbol splitter.
 * Role hypothesis: consumes one range-coded decision (45AF64 cursor minus
 * 458F98[45AF80] history), picks one of three bands: out-of-range
 * (<0x6D4250 or >=0xFEFF01) -> estimator only; mid (<0x91ADC0) -> estimator
 * + freq-divide with 459398/459BA0 adaptation; high -> direct freq-divide.
 * Logs (45AFA4/symbol/bl) into the 458778/458788/45AF50 ring at 45C690.
 * Entry: t0, t3 (t1 dummy, dead). Frame: 8 (var_1C dword, var_18 byte).
 * Returns al (=bl, decoded symbol). Calls: sub_4220B0, sub_41A360,
 * sub_419CA0.
 */
unsigned int d4vc_ppm_sym_split(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in)
{
    unsigned int saved_eax;
    unsigned int saved_edx;
    unsigned int flag;
    unsigned int sym;
    unsigned int ctx;
    unsigned int af64;
    unsigned int idx;
    unsigned int price;
    unsigned int held;
    unsigned int old;
    unsigned int range;
    unsigned int code;
    unsigned int midv;
    unsigned int hiv;
    unsigned int scratch;
    int via_b51;

    (void)ebx_in;
    saved_eax = eax_in;
    saved_edx = edx_in;
    flag = (edx_in == 0x2FFU) ? 1U : 0U;
    sym = 0;
    price = 0;
    held = 0;
    via_b51 = 0;
    D4V_TBL_45AFA4_32(&c->st, 0) = saved_edx;
    idx = (D4V_TBL_45AF80_32(&c->st, 0) + 1U) & 0xFFU;
    D4V_TBL_45AF74_32(&c->st, 0) = 0;
    D4V_TBL_45AF80_32(&c->st, 0) = idx;
    af64 = (D4V_TBL_45AF64_32(&c->st, 0) - D4V_TBL_458F98_32(&c->st, idx * 4U)) & U32MASK;
    D4V_TBL_45AF64_32(&c->st, 0) = af64;
    if (af64 < 0x6D4250UL || af64 >= 0xFEFF01UL) {
        sym = d4vc_ppm_prob_decode(c, saved_eax) & 0xFFU;
        if (D4V_CTX_B_45C68D_8(&c->st, 0) != 0)
            D4V_TBL_45AF8C_32(&c->st, 0) =
                (D4V_TBL_45AF8C_32(&c->st, 0) + D4V_TBL_45AF74_32(&c->st, 0)) & U32MASK;
    } else if (af64 < 0x91ADC0UL) {
        if (D4V_TBL_459398_32(&c->st, flag * 4U)
            < D4V_TBL_459BA0_32(&c->st, flag * 4U)) {
            D4V_TBL_45AFA4_32(&c->st, 0) = (flag + 0x300U) & U32MASK;
            sym = d4vc_ppm_prob_decode(c, 0x200U) & 0xFFU;
            price = D4V_TBL_45AF74_32(&c->st, 0);
            if (D4V_CTX_B_45C68D_8(&c->st, 0) != 0)
                D4V_TBL_45AF8C_32(&c->st, 0) =
                    (D4V_TBL_45AF8C_32(&c->st, 0) + price) & U32MASK;
            D4V_TBL_45AF74_32(&c->st, 0) = 0;
            D4V_TBL_45AFA4_32(&c->st, 0) = saved_edx;
            (void)d4vc_freq_divide_price(c, saved_eax, sym);
            held = D4V_TBL_45AF74_32(&c->st, 0);
        } else {
            sym = d4vc_ppm_prob_decode(c, saved_eax) & 0xFFU;
            held = D4V_TBL_45AF74_32(&c->st, 0);
            if (D4V_CTX_B_45C68D_8(&c->st, 0) != 0)
                D4V_TBL_45AF8C_32(&c->st, 0) =
                    (D4V_TBL_45AF8C_32(&c->st, 0) + held) & U32MASK;
            D4V_TBL_45AF74_32(&c->st, 0) = 0;
            D4V_TBL_45AFA4_32(&c->st, 0) = (flag + 0x300U) & U32MASK;
            (void)d4vc_freq_divide_price(c, 0x200U, sym);
            price = D4V_TBL_45AF74_32(&c->st, 0);
        }
        via_b51 = 1;
    } else {
        D4V_RSLOT_LO_32(&c->st, 0) = 0x100U;
        range = D4V_RC_RANGE_32(&c->st, 0) / 0x100U;
        D4V_RC_RANGE_32(&c->st, 0) = range;
        sym = (D4V_RC_CODE_32(&c->st, 0) / range) & 0xFFU;
        D4V_RSLOT_MID_32(&c->st, 0) = sym;
        D4V_RSLOT_HI_32(&c->st, 0) = (sym + 1U) & U32MASK;
        hiv = d4v_slot32(c, 8u);
        midv = d4v_slot32(c, 4u);
        scratch = (midv * range) & U32MASK;
        code = (D4V_RC_CODE_32(&c->st, 0) - scratch) & U32MASK;
        range = ((hiv - midv) * range) & U32MASK;
        d4v_rcrenorm_reg(c, &code, &range, &scratch);
        D4V_RC_CODE_32(&c->st, 0) = code;
        D4V_RC_RANGE_32(&c->st, 0) = range;
        if (D4V_CTX_B_45C68D_8(&c->st, 0) != 0)
            D4V_TBL_45AF8C_32(&c->st, 0) =
                (D4V_TBL_45AF8C_32(&c->st, 0) + 0x9240UL) & U32MASK;
        (void)d4vc_freq_divide_price(c, saved_eax, sym);
    }
    if (via_b51) {
        ctx = D4V_CTX_B_45C690_8(&c->st, 0);
        D4V_TBL_45AFA4_32(&c->st, 0) = (flag + 0x300U) & U32MASK;
        D4V_TBL_458778_32(&c->st, ctx * 4U) = D4V_TBL_45AFA4_32(&c->st, 0);
        D4V_TBL_458788_32(&c->st, ctx * 4U) = 0x200U;
        D4V_CTX_B_45C690_8(&c->st, 0) = (unsigned char)(ctx + 1U);
        D4V_TBL_45AF64_32(&c->st, 0) =
            (D4V_TBL_45AF64_32(&c->st, 0) + price) & U32MASK;
        D4V_TBL_45AF50_8(&c->st, ctx) = (unsigned char)sym;
        old = D4V_TBL_459398_32(&c->st, flag * 4U);
        D4V_TBL_458F98_32(&c->st, (D4V_TBL_45AF80_32(&c->st, 0) & 0xFFU) * 4U) = price;
        D4V_TBL_459398_32(&c->st, flag * 4U) = (((old << 5) - old) >> 5) + price;
        D4V_TBL_45AF74_32(&c->st, 0) = held;
        D4V_TBL_45AFA4_32(&c->st, 0) = saved_edx;
    }
    ctx = D4V_CTX_B_45C690_8(&c->st, 0);
    D4V_TBL_458778_32(&c->st, ctx * 4U) = D4V_TBL_45AFA4_32(&c->st, 0);
    D4V_TBL_458788_32(&c->st, ctx * 4U) = saved_eax;
    D4V_TBL_45AF50_8(&c->st, ctx) = (unsigned char)sym;
    D4V_CTX_B_45C690_8(&c->st, 0) = (unsigned char)(ctx + 1U);
    if (!via_b51) {
        price = D4V_TBL_45AF74_32(&c->st, 0);
        old = D4V_TBL_459398_32(&c->st, flag * 4U);
        D4V_TBL_458F98_32(&c->st, (D4V_TBL_45AF80_32(&c->st, 0) & 0xFFU) * 4U) = price;
        D4V_TBL_45AF64_32(&c->st, 0) =
            (D4V_TBL_45AF64_32(&c->st, 0) + price) & U32MASK;
        D4V_TBL_459398_32(&c->st, flag * 4U) = (((old << 5) - old) >> 5) + price;
    }
    old = D4V_TBL_459BA0_32(&c->st, flag * 4U);
    D4V_TBL_459BA0_32(&c->st, flag * 4U) =
        (((old << 5) - old) >> 5) + D4V_TBL_45AF74_32(&c->st, 0);
    return sym & 0xFFU;
}


/* Decode one PPM symbol from context eax_in. Returns the symbol byte. */
static void d4v_rc_renorm_slot(dec4v_ctx *c)
{
    unsigned int mid;
    unsigned int hi;
    unsigned int range;
    unsigned int code;
    unsigned int scratch;

    hi = d4v_slot32(c, 8u);
    mid = d4v_slot32(c, 4u);
    range = D4V_RC_RANGE_32(&c->st, 0);
    scratch = mid * range;
    range = (hi - mid) * range;
    code = D4V_RC_CODE_32(&c->st, 0) - scratch;
    d4v_rcrenorm_reg(c, &code, &range, &scratch);
    D4V_RC_CODE_32(&c->st, 0) = code;
    D4V_RC_RANGE_32(&c->st, 0) = range;
}

static void d4v_rc_price_slots(dec4v_ctx *c)
{
    unsigned int hi;
    unsigned int mid;
    unsigned int lo;
    unsigned int price;

    hi = D4V_RSLOT_HI_32(&c->st, 0);
    mid = D4V_RSLOT_MID_32(&c->st, 0);
    lo = D4V_RSLOT_LO_32(&c->st, 0);
    price = ((hi - mid) << 14) / lo;
    price = (unsigned int)D4V_TBL_PRICE_SUM_16(&c->st, (price & 0xFFFFU) * 2U) & 0xFFFFU;
    D4V_TBL_45AF74_32(&c->st, 0) += price;
    d4v_rc_renorm_slot(c);
}

unsigned int d4vc_ppm_prob_decode(dec4v_ctx *c, unsigned int eax_in)
{
    unsigned int t1;
    unsigned int t4;
    unsigned int t5;
    unsigned int v30;
    unsigned int v28;
    unsigned int v2c;
    unsigned int v34;
    unsigned int v38;
    unsigned int i;
    unsigned int sym;
    unsigned int word;
    unsigned int total;
    unsigned int scaled;
    unsigned int count;
    unsigned char v1C;
    unsigned char *b14;
    unsigned char *b15;
    unsigned char *b17;
    uint32_t b14b;
    uint32_t b15b;
    uint32_t b17b;
    unsigned int c4[256];
    unsigned short cw[257];

    for (i = 0; i < 256U; i++)
        c4[i] = 0U;
    for (i = 0; i < 257U; i++)
        cw[i] = 0U;
    b14 = c->blk14;
    b14b = c->blk14_base;
    b15 = c->blk15;
    b15b = c->blk15_base;
    b17 = c->blk17;
    b17b = c->blk17_base;
    t1 = D4V_CTX_B_45C68A_8(&c->st, 0);
    count = t1 & 0xFFU;
    v30 = eax_in;
    t4 = D4V_BLK_PTR_14_32(&c->st, 0) + (eax_in << 9);
    v28 = (D4V_BLK_PTR_14_32(&c->st, 0) & 0xFFFF0000UL)
        | d4v_blk_u16(b14, b14b, (uint32_t)t4);
    scaled = eax_in << 8;
    v1C = 0;
    for (i = 0; i < count; i++) {
        sym = D4V_CTX_B_45C65E_8(&c->st, i);
        sym = sym + scaled;
        word = (D4V_BLK_PTR_15_32(&c->st, 0) & 0xFFFF0000UL)
            | d4v_blk_u16(b15, b15b,
                (uint32_t)((sym + sym) + D4V_BLK_PTR_15_32(&c->st, 0)));
        v28 = v28 - word;
    }
    D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)count;
    if ((v28 & 0xFFFFU) == 0U) {
        v30 = D4V_TBL_45AFA4_32(&c->st, 0);
        t4 = D4V_BLK_PTR_14_32(&c->st, 0) + (v30 << 9);
        scaled = v30 << 8;
        v28 = (D4V_BLK_PTR_14_32(&c->st, 0) & 0xFFFF0000UL)
            | d4v_blk_u16(b14, b14b, (uint32_t)t4);
        for (i = 0; i < count; i++) {
            sym = D4V_CTX_B_45C65E_8(&c->st, i);
            sym = sym + scaled;
            word = (D4V_BLK_PTR_15_32(&c->st, 0) & 0xFFFF0000UL)
                | d4v_blk_u16(b15, b15b,
                    (uint32_t)((sym + sym) + D4V_BLK_PTR_15_32(&c->st, 0)));
            v28 = v28 - word;
        }
        total = v28 & 0xFFFFU;
        D4V_RSLOT_LO_32(&c->st, 0) = total;
        word = D4V_RC_RANGE_32(&c->st, 0) / total;
        D4V_RC_RANGE_32(&c->st, 0) = word;
        v2c = D4V_RC_CODE_32(&c->st, 0) / word;
    } else {
        word = d4v_blk_u16(b17, b17b,
            (uint32_t)(D4V_BLK_PTR_17_32(&c->st, 0) + v30 * 2U));
        total = (v28 & 0xFFFFU) + word;
        D4V_RSLOT_LO_32(&c->st, 0) = total;
        sym = D4V_RC_RANGE_32(&c->st, 0) / total;
        D4V_RC_RANGE_32(&c->st, 0) = sym;
        v2c = D4V_RC_CODE_32(&c->st, 0) / sym;
        if ((v2c & 0xFFFFU) >= (v28 & 0xFFFFU)) {
            unsigned int idx;
            unsigned int plane15;
            unsigned int acc;
            D4V_RSLOT_MID_32(&c->st, 0) = v28 & 0xFFFFU;
            D4V_RSLOT_HI_32(&c->st, 0) = D4V_RSLOT_LO_32(&c->st, 0);
            d4v_rc_price_slots(c);
            plane15 = D4V_BLK_PTR_15_32(&c->st, 0) + (v30 << 9);
            total = d4v_blk_u16(b14, b14b,
                (uint32_t)(D4V_BLK_PTR_14_32(&c->st, 0)
                    + (D4V_TBL_45AFA4_32(&c->st, 0) << 9))) & 0xFFFFU;
            D4V_RSLOT_LO_32(&c->st, 0) = total;
            scaled = D4V_TBL_45AFA4_32(&c->st, 0) << 8;
            t1 = count;
            for (idx = 0; idx < 256U; idx++) {
                int take;
                D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)count;
                take = d4v_blk_u16(b15, b15b, (uint32_t)(plane15 + idx * 2U)) != 0U;
                if (!take) {
                    take = 0;
                    for (i = 0; i < count; i++) {
                        if (idx == (unsigned int)D4V_CTX_B_45C65E_8(&c->st, i)) {
                            take = 1;
                            break;
                        }
                    }
                }
                if (take) {
                    word = d4v_blk_u16(b15, b15b,
                        (uint32_t)(D4V_BLK_PTR_15_32(&c->st, 0) + (idx + scaled) * 2U))
                        & 0xFFFFU;
                    D4V_RSLOT_LO_32(&c->st, 0) -= word;
                }
            }
            total = D4V_RSLOT_LO_32(&c->st, 0);
            word = D4V_RC_RANGE_32(&c->st, 0) / total;
            D4V_RC_RANGE_32(&c->st, 0) = word;
            v2c = D4V_RC_CODE_32(&c->st, 0) / word;
            acc = 0;
            v1C = 0;
            do {
                int empty;
                int add;
                v1C = (unsigned char)(v1C - 1U);
                count = D4V_CTX_B_45C68A_8(&c->st, 0);
                empty = d4v_blk_u16(b15, b15b,
                    (uint32_t)(plane15 + (unsigned int)v1C * 2U)) == 0U;
                add = 0;
                if (empty) {
                    add = 1;
                    for (i = 0; i < count; i++) {
                        if ((unsigned int)D4V_CTX_B_45C65E_8(&c->st, i) == (unsigned int)v1C) {
                            add = 0;
                            break;
                        }
                    }
                }
                if (add) {
                    word = d4v_blk_u16(b15, b15b,
                        (uint32_t)(D4V_BLK_PTR_15_32(&c->st, 0)
                            + ((unsigned int)v1C + scaled) * 2U));
                    acc = ((acc & 0xFFFFU) + word) & 0xFFFFU;
                }
                D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)count;
            } while ((acc & 0xFFFFU) <= (v2c & 0xFFFFU));
            word = d4v_blk_u16(b15, b15b,
                (uint32_t)(D4V_BLK_PTR_15_32(&c->st, 0)
                    + ((D4V_TBL_45AFA4_32(&c->st, 0) << 8) + (unsigned int)v1C) * 2U));
            D4V_RSLOT_HI_32(&c->st, 0) = acc & 0xFFFFU;
            D4V_RSLOT_MID_32(&c->st, 0) = (acc & 0xFFFFU) - (word & 0xFFFFU);
            d4v_rc_price_slots(c);
            return (unsigned int)v1C;
        }
    }
    count = D4V_CTX_B_45C68A_8(&c->st, 0);
    t1 = count;
    if (count == 0U) {
        unsigned int bit;
        unsigned int origin;
        unsigned int probe;
        origin = t4;
        v28 = v28 - v2c;
        bit = 0x80U;
        while (bit != 0U) {
            probe = bit * 2U + t4;
            word = d4v_blk_u16(b14, b14b, (uint32_t)probe);
            if ((word & 0xFFFFU) < (v28 & 0xFFFFU)) {
                t4 = probe;
                v28 = v28 - word;
            }
            bit >>= 1;
        }
        v28 = v28 + v2c;
        D4V_CTX_B_45C68A_8(&c->st, 0) = 0;
        sym = (unsigned int)(((int)(t4 - origin)) >> 1);
    } else {
        unsigned int bit;
        unsigned int seen;
        v34 = t4;
        v38 = v30 << 8;
        i = 0;
        t5 = 0;
        sym = 0;
        while (i < count) {
            word = D4V_CTX_B_45C65E_8(&c->st, i);
            c4[sym >> 2] = word + word + t4;
            word = D4V_CTX_B_45C65E_8(&c->st, i) + v38;
            t5 += 2U;
            sym += 4U;
            i++;
            cw[t5 >> 1] = (unsigned short)d4v_blk_u16(b15, b15b,
                (uint32_t)(word + word + D4V_BLK_PTR_15_32(&c->st, 0)));
        }
        v28 = v28 - v2c;
        bit = 0x80U;
        while (bit != 0U) {
            seen = 0;
            i = 0;
            sym = 0;
            word = 0;
            while (word < count) {
                if ((bit + bit + t4) > c4[i >> 2])
                    seen += (unsigned int)cw[(sym + 2U) >> 1];
                i += 4U;
                sym += 2U;
                word++;
            }
            total = d4v_blk_u16(b14, b14b, (uint32_t)(t4 + bit * 2U));
            if (total < ((v28 & 0xFFFFU) + seen)) {
                i = 0;
                sym = 0;
                while (sym < count) {
                    if ((bit + bit + t4) > c4[i >> 2])
                        c4[i >> 2] = 0xFFFFFFFFUL;
                    i += 4U;
                    sym++;
                }
                t4 = t4 + bit * 2U;
                word = d4v_blk_u16(b14, b14b, (uint32_t)t4);
                v28 = v28 - (word - seen);
            }
            bit >>= 1;
        }
        v28 = v28 + v2c;
        D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)count;
        sym = (unsigned int)(((int)(t4 - v34)) >> 1);
    }
    D4V_RSLOT_HI_32(&c->st, 0) = v28 & 0xFFFFU;
    word = d4v_blk_u16(b15, b15b,
        (uint32_t)(D4V_BLK_PTR_15_32(&c->st, 0) + ((v30 << 8) + (sym & 0xFFU)) * 2U))
        & 0xFFFFU;
    D4V_RSLOT_MID_32(&c->st, 0) = (v28 & 0xFFFFU) - word;
    d4v_rc_price_slots(c);
    return sym & 0xFFU;
}


/* sub_41A360: freq-divide helper.
 * Role hypothesis: given a table selector in t0 and a symbol byte in dl,
 * subtracts the mapped word frequencies ([433B7C] via byte_45C65E order,
 * plus the 256-slot scan when the direct slot is empty and bh!=0), does
 * the 0x437968 price lookup ((freq<<14)/total), and accumulates into
 * 45AF74. Used by 419970/41AD60/41AE30 after they set 45AFA4/45AF74.
 * Entry: t0, dl. Frame: 8 (var_1C dword, var_18 byte). Returns leftover
 * t0 (price remnant; callers ignore -> effectively void).
 */
unsigned int d4vc_freq_divide_price(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in)
{
    unsigned int count;
    unsigned int index;
    unsigned int sym;
    unsigned int plane;
    unsigned int old_plane;
    unsigned int total;
    unsigned int i;
    unsigned int excl;
    unsigned int word;
    unsigned int den;
    unsigned int quot;
    unsigned int price;
    unsigned char *b14;
    unsigned char *b15;
    unsigned char *b17;
    uint32_t b14b;
    uint32_t b15b;
    uint32_t b17b;

    b14 = c->blk14; b14b = c->blk14_base;
    b15 = c->blk15; b15b = c->blk15_base;
    b17 = c->blk17; b17b = c->blk17_base;
    count = D4V_CTX_B_45C68A_8(&c->st, 0);
    index = eax_in;
    sym = edx_in & 0xFFU;
    plane = (D4V_BLK_PTR_15_32(&c->st, 0) + (eax_in << 9)) & U32MASK;
    old_plane = 0;
    if (d4v_u16_in(c, b15, b15b, 0x61A00u, (uint32_t)((plane + sym * 2U) & U32MASK)) == 0U) {
        total = d4v_u16_in(c, b14, b14b, 0x61A00u,
            (uint32_t)((D4V_BLK_PTR_14_32(&c->st, 0) + (eax_in << 9)) & U32MASK));
        for (i = 0; i < count; i++) {
            excl = D4V_CTX_B_45C65E_8(&c->st, i);
            word = d4v_u16_in(c, b15, b15b, 0x61A00u,
                (uint32_t)((plane + excl * 2U) & U32MASK));
            total = (total - word) & U32MASK;
        }
        if ((total & 0xFFFFU) != 0U) {
            unsigned int slot;
            slot = d4v_u16_in(c, b17, b17b, 0x5FEu,
                (uint32_t)((D4V_BLK_PTR_17_32(&c->st, 0) + ((index + index) & U32MASK)) & U32MASK));
            den = (slot + (total & 0xFFFFU)) & U32MASK;
            quot = ((slot & 0xFFFFU) << 14) / den;
            price = D4V_TBL_PRICE_SUM_16(&c->st, (quot & 0xFFFFU) * 2U) & 0xFFFFU;
            D4V_TBL_45AF74_32(&c->st, 0) =
                (D4V_TBL_45AF74_32(&c->st, 0) + price) & U32MASK;
            old_plane = plane;
        }
        index = D4V_TBL_45AFA4_32(&c->st, 0);
        plane = (D4V_BLK_PTR_15_32(&c->st, 0) + (index << 9)) & U32MASK;
    }
    total = d4v_u16_in(c, b14, b14b, 0x61A00u,
        (uint32_t)((D4V_BLK_PTR_14_32(&c->st, 0) + (index << 9)) & U32MASK));
    if (index < 0x2FFU) {
        word = d4v_u16_in(c, b17, b17b, 0x5FEu,
            (uint32_t)((D4V_BLK_PTR_17_32(&c->st, 0) + ((index + index) & U32MASK)) & U32MASK));
        total = ((total & 0xFFFFU) + word) & 0xFFFFU;
        for (i = 0; i < count; i++) {
            excl = D4V_CTX_B_45C65E_8(&c->st, i);
            word = d4v_u16_in(c, b15, b15b, 0x61A00u,
                (uint32_t)((plane + excl * 2U) & U32MASK));
            total = (total - word) & U32MASK;
        }
    } else if (count == 0U) {
        if (old_plane != 0U) {
            for (i = 0; i < 256U; i++) {
                if (d4v_u16_in(c, b15, b15b, 0x61A00u,
                        (uint32_t)((old_plane + i * 2U) & U32MASK)) != 0U) {
                    word = d4v_u16_in(c, b15, b15b, 0x61A00u,
                        (uint32_t)((plane + i * 2U) & U32MASK));
                    total = (total - word) & U32MASK;
                }
            }
        }
    } else if (old_plane == 0U) {
        for (i = 0; i < count; i++) {
            excl = D4V_CTX_B_45C65E_8(&c->st, i);
            word = d4v_u16_in(c, b15, b15b, 0x61A00u,
                (uint32_t)((plane + excl * 2U) & U32MASK));
            total = (total - word) & U32MASK;
        }
    } else {
        for (i = 0; i < 256U; i++) {
            int take;
            take = d4v_u16_in(c, b15, b15b, 0x61A00u,
                (uint32_t)((old_plane + i * 2U) & U32MASK)) != 0U;
            if (!take) {
                unsigned int k;
                take = 0;
                for (k = 0; k < count; k++) {
                    if (i == (unsigned int)D4V_CTX_B_45C65E_8(&c->st, k)) {
                        take = 1;
                        break;
                    }
                }
            }
            if (take) {
                word = d4v_u16_in(c, b15, b15b, 0x61A00u,
                    (uint32_t)((plane + i * 2U) & U32MASK));
                total = (total - word) & U32MASK;
            }
        }
    }
    word = d4v_u16_in(c, b15, b15b, 0x61A00u,
        (uint32_t)((plane + sym * 2U) & U32MASK)) & 0xFFFFU;
    den = total & 0xFFFFU;
    quot = (word << 14) / den;
    price = D4V_TBL_PRICE_SUM_16(&c->st, (quot & 0xFFFFU) * 2U) & 0xFFFFU;
    D4V_TBL_45AF74_32(&c->st, 0) =
        (D4V_TBL_45AF74_32(&c->st, 0) + price) & U32MASK;
    D4V_CTX_B_45C68A_8(&c->st, 0) = (unsigned char)count;
    return price;
}


/* sub_41AD60: tiny dispatch to 41A360.
 * Role hypothesis: zeroes 45AF74, runs the freq-divide helper with
 * (t0, bl) after stashing t3 into 45AFA4, logs (45AFA4/symbol/bl) into
 * the 458778/458788/45AF50 ring, then adapts the 459398 + 459BA0 slots
 * selected by (entry_edx == 0x2FF).
 * Entry: t0, t3, bl. Void. No frame.
 */
/* Blend one frequency counter: old*31/32 plus the price just stored
 * in 45AF74. slot is 1 when the caller passed the 0x2FF sentinel. */
static unsigned int d4v_blend31(dec4v_ctx *c, unsigned int slot)
{
    unsigned int old;
    unsigned int mixed;
    old = D4V_TBL_459398_32(&c->st, (slot & 0xFFUL) * 4UL);
    mixed = ((old << 5) - old) >> 5;
    return mixed + D4V_TBL_45AF74_32(&c->st, 0);
}

void d4vc_freq_adapt_459398(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in)
{
    unsigned int ctx;
    unsigned int slot;
    unsigned int price;
    unsigned int old;

    D4V_TBL_45AF74_32(&c->st, 0) = 0;
    D4V_TBL_45AFA4_32(&c->st, 0) = edx_in;
    (void)d4vc_freq_divide_price(c, eax_in, ebx_in & 0xFFUL);
    ctx = (unsigned int)D4V_CTX_B_45C690_8(&c->st, 0);
    price = D4V_TBL_45AFA4_32(&c->st, 0);
    D4V_TBL_458778_32(&c->st, (ctx & 0xFFUL) * 4UL) = price;
    D4V_TBL_458788_32(&c->st, (ctx & 0xFFUL) * 4UL) = eax_in;
    D4V_TBL_45AF50_8(&c->st, ctx & 0xFFUL) = (unsigned char)(ebx_in & 0xFFUL);
    D4V_CTX_B_45C690_8(&c->st, 0) = (unsigned char)((ctx + 1UL) & 0xFFUL);
    slot = (edx_in == 0x2FFUL) ? 1UL : 0UL;
    D4V_TBL_459398_32(&c->st, (slot & 0xFFUL) * 4UL) = d4v_blend31(c, slot);
    old = D4V_TBL_459BA0_32(&c->st, (slot & 0xFFUL) * 4UL);
    D4V_TBL_459BA0_32(&c->st, (slot & 0xFFUL) * 4UL) =
        (((old << 5) - old) >> 5) + D4V_TBL_45AF74_32(&c->st, 0);
}

/* sub_41AE30: tiny context-index shim.
 * Role hypothesis: stashes t3 into 45AFA4, zeroes 45AF74, runs the
 * freq-divide helper with passthrough t0 + bl, then adapts the 45AF60
 * counter (old*31/32 + 45AF74).
 * Entry: t0 (passthrough), t3, bl. Void. No frame.
 */
void d4vc_freq_adapt_45af60(dec4v_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ebx_in)
{
    unsigned int old;

    D4V_TBL_45AFA4_32(&c->st, 0) = edx_in;
    D4V_TBL_45AF74_32(&c->st, 0) = 0;
    (void)d4vc_freq_divide_price(c, eax_in, ebx_in & 0xFFUL);
    old = D4V_TBL_45AF60_32(&c->st, 0);
    D4V_TBL_45AF60_32(&c->st, 0) =
        (((old << 5) - old) >> 5) + D4V_TBL_45AF74_32(&c->st, 0);
}

/* sub_41AE70: tiny shim (freq scale).
 * Role hypothesis: scaled frequency step: t1 = tab[1]*t3; t4 = tab[0];
 * t0 = t1/t4 (min 1); t0 += t3; saturate at 0xFF.
 * Entry: t0 = tab (arena pointer: tab[0], tab[1] bytes), t3. Frame: none.
 * Returns t0.
 */
unsigned int d4vc_freq_scale_step(dec4v_ctx *c, unsigned int tab, unsigned int edx_in)
{
    unsigned char *p;
    unsigned int lo;
    unsigned int hi;
    unsigned int step;

    p = d4v_span(c, (uint32_t)tab, 2);
    if (p) {
        lo = p[0];
        hi = p[1];
    } else {
        lo = (unsigned int)(*(unsigned char *)d4v_ptr(c, (uint32_t)tab));
        hi = (unsigned int)(*(unsigned char *)d4v_ptr(c, (uint32_t)(tab + 1UL)));
    }
    step = (hi * edx_in) / lo;
    if (step == 0U)
        step = 1U;
    step += edx_in;
    if (step > 0xFFUL)
        step = 0xFFUL;
    return step;
}

/* sub_41AEB0: tiny shim (2-byte counter with halve on saturate).
 * Role hypothesis: increments tab[1] (flag==0) or tab[0] (flag!=0); if
 * either reaches >=0xFE, stores (x+1)>>1 back into both bytes.
 * Entry: t0 = tab (arena pointer), dl = flag. Void. No frame.
 */
void d4vc_counter_bump_halve(dec4v_ctx *c, unsigned int tab, unsigned int flag_in)
{
    unsigned char *p;
    unsigned char tmp[2];
    int local;
    p = d4v_span(c, (uint32_t)tab, 2);
    local = 0;
    if (!p) {
        tmp[0] = *(unsigned char *)d4v_ptr(c, (uint32_t)tab);
        tmp[1] = *(unsigned char *)d4v_ptr(c, (uint32_t)(tab + 1UL));
        p = tmp;
        local = 1;
    }
    if ((flag_in & 0xFFUL) != 0U)
        p[0] = (unsigned char)(p[0] + 1U);
    else
        p[1] = (unsigned char)(p[1] + 1U);
    if (p[0] >= 0xFEU || p[1] >= 0xFEU) {
        p[0] = (unsigned char)(((unsigned int)p[0] + 1U) >> 1);
        p[1] = (unsigned char)(((unsigned int)p[1] + 1U) >> 1);
    }
    if (local) {
        *(unsigned char *)d4v_ptr(c, (uint32_t)tab) = tmp[0];
        *(unsigned char *)d4v_ptr(c, (uint32_t)(tab + 1UL)) = tmp[1];
    }
}

/* ---- workers: PPM helpers batch S2 (from draft_s2.c) ---- */
/* Halve the eight count bytes of every live 0x14-byte entry in blk02.
 * A zero sum clears the head byte; otherwise the head is (head>>2)+1
 * and the folded total lands in the byte at +0x12. */
void d4vc_rescale_blk02(dec4v_ctx *c)
{
    unsigned int base;
    unsigned int i;
    unsigned int k;
    unsigned int sum;
    unsigned int head;
    unsigned char *e2;

    base = (unsigned int)D4V_BLK_PTR_02_32(&c->st, 0);
    e2 = c->blk02 + (base - c->blk02_base);
    for (i = 0; i != 0x140000UL; i = (i + 0x14UL) & U32MASK) {
        if (e2[i] == 0)
            continue;
        sum = 0;
        k = 8;
        do {
            k--;
            e2[i + k + 1] = (unsigned char)(e2[i + k + 1] >> 2);
            sum = (sum + e2[i + k + 1]) & 0xFFU;
        } while (k != 0);
        if (sum == 0)
            head = 0;
        else
            head = (d4v_sar((unsigned int)e2[i], 2) + 1U) & 0xFFU;
        e2[i] = (unsigned char)head;
        e2[i + 0x11UL] = (unsigned char)(e2[i + 0x11UL] >> 1);
        e2[i + 0x12UL] = (unsigned char)((sum + head) & 0xFFU);
    }
    D4V_BLK_PTR_02_32(&c->st, 0) = base;
}

/* ------------------------------------------------------------------ */
/* Role: rescale/decay the table at [0x433B44] (0x20-byte entries over */
/* banks 0..3 at +0x8000 index / +0x100000 address steps; halves 13 */
/* count bytes; on zero sum shifts one bank down onto the entry, else */
/* clears it; folds [t5+1Bh]/[t5] the same way as sub_415AE0). */
/* Entry-live regs: none (t3 zeroed and var_1C initialised before any */
/* read; no stack args). Returns void. */
/* Frame: 4 bytes (var_1C @ sp+0) + 6 pushed regs. */
/* Globals touched: 433B44. */
/* ------------------------------------------------------------------ */
/* Decay every live 32-byte node of blk01. Thirteen count bytes are
 * shifted down by two. A zero sum pulls the next bank (steps of
 * 0x100000) onto this node and clears the donor; otherwise the head
 * byte becomes (head>>2)+1 and the byte at +0x1B is halved. The index
 * then steps by 0x8000 through the eight banks before the next slot. */
void d4vc_rescale_blk01(dec4v_ctx *c)
{
    unsigned int idx;
    unsigned int base;
    unsigned int off;
    unsigned int k;
    unsigned int sum;
    unsigned char *b1;
    unsigned int b1b;

    b1 = c->blk01;
    b1b = c->blk01_base;
    base = (unsigned int)D4V_BLK_PTR_01_32(&c->st, 0);
    idx = 0;
    for (;;) {
        off = ((base + ((idx << 5) & U32MASK)) & U32MASK) - b1b;
        if (b1[off] == 0) {
            idx = ((idx & 0x7FFFUL) + 1U) & U32MASK;
            if (idx < 0x8000UL)
                continue;
            return;
        }
        sum = 0;
        k = 0x0DU;
        do {
            k = (k - 1U) & 0xFFU;
            b1[off + k + 1U] = (unsigned char)(b1[off + k + 1U] >> 2);
            sum = (sum + b1[off + k + 1U]) & 0xFFU;
        } while (k != 0);
        if (sum == 0) {
            unsigned int bank;
            unsigned int src;
            unsigned int dst;
            bank = idx;
            src = (idx << 5) & U32MASK;
            dst = off;
            for (;;) {
                unsigned int src_off;
                bank = (bank + 0x8000UL) & U32MASK;
                src = (src + 0x100000UL) & U32MASK;
                src_off = ((base + src) & U32MASK) - b1b;
                if (bank >= 0x40000UL || b1[src_off] == 0) {
                    b1[dst] = 0;
                    idx = (idx + 0xFFFF8000UL) & U32MASK;
                    break;
                }
                memcpy(b1 + dst, b1 + src_off, 0x20);
                dst = src_off;
            }
        } else {
            unsigned int head;
            b1[off + 0x1BU] = (unsigned char)(b1[off + 0x1BU] >> 1);
            head = (d4v_sar((unsigned int)b1[off], 2) + 1U) & U32MASK;
            b1[off] = (unsigned char)head;
        }
        idx = (idx + 0x8000UL) & U32MASK;
        if (idx >= 0x40000UL) {
            idx = ((idx & 0x7FFFUL) + 1U) & U32MASK;
            if (idx >= 0x8000UL)
                return;
        }
    }
}
/* ------------------------------------------------------------------ */
/* Role: small PPM-context helper. Halves (round-half-up per u16) the */
/* 0x200-byte plane at [0x433B7C]+idx*0x200 into itself while summing */
/* all halved dwords into t6; copies 0x1FE bytes [0x433B7C]+idx*0x200 */
/* to [0x433B78]+idx*0x200+2; folds the sum (lo16+hi16) as a u16 at */
/* [0x433B78]+idx*0x200; rebuilds a doubling prefix structure over the */
/* [0x433B78] bytes; for idx < 0x2FF touches the u16 at */
/* [0x433B84]+idx*2 (((old >> 2) + 1)). */
/* Entry-live regs: t0 = idx (saved to var_1C, then t3=t0, t1=t0 */
/* no stack args). Returns void (callers at 4165AD/416C65/416C71 ignore */
/* t0; both exits leave callee residue in t0). */
/* Frame: 4 bytes (var_1C @ sp+0) + 6 pushed regs. */
/* Globals touched: 433B78, 433B7C, 433B84. */
/* ------------------------------------------------------------------ */
void d4vc_rescale_ppm_plane(dec4v_ctx *c, unsigned int idx)
{
    unsigned int cursor;
    unsigned int end;
    unsigned int sum;
    unsigned int plane;
    unsigned int step;
    unsigned int at;
    unsigned int a;
    unsigned int b;
    unsigned int pair;
    unsigned int folded;
    unsigned char *b14;
    unsigned char *b15;
    unsigned char *b17;
    unsigned char *q;
    uint32_t b14b;
    uint32_t b15b;
    uint32_t b17b;
    unsigned short s16;
    unsigned int base14;
    unsigned int base15;

    b14 = c->blk14;
    b14b = c->blk14_base;
    b15 = c->blk15;
    b15b = c->blk15_base;
    b17 = c->blk17;
    b17b = c->blk17_base;
    base15 = (unsigned int)D4V_BLK_PTR_15_32(&c->st, 0);
    base14 = (unsigned int)D4V_BLK_PTR_14_32(&c->st, 0);
    cursor = (base15 + ((idx << 9) & U32MASK)) & U32MASK;
    end = (cursor + 0x200UL) & U32MASK;
    plane = (idx << 8) & U32MASK;
    sum = 0;
    while (cursor != end) {
        q = b15 + (cursor - b15b);
        memcpy(&b, q, 4);
        memcpy(&a, q + 4, 4);
        b = (b + 0x10001UL) & U32MASK;
        a = (a + 0x10001UL) & U32MASK;
        b &= 0xFFFEFFFEUL;
        a &= 0xFFFEFFFEUL;
        b >>= 1;
        a >>= 1;
        cursor = (cursor + 0x10UL) & U32MASK;
        q = b15 + (cursor - b15b);
        pair = (b + a) & U32MASK;
        memcpy(q - 0x0C, &a, 4);
        memcpy(q - 0x10, &b, 4);
        memcpy(&a, q - 4, 4);
        memcpy(&b, q - 8, 4);
        a = (a + 0x10001UL) & U32MASK;
        b = (b + 0x10001UL) & U32MASK;
        a &= 0xFFFEFFFEUL;
        b &= 0xFFFEFFFEUL;
        a >>= 1;
        b >>= 1;
        sum = (sum + pair) & U32MASK;
        pair = (b + a) & U32MASK;
        memcpy(q - 4, &a, 4);
        sum = (sum + pair) & U32MASK;
        memcpy(q - 8, &b, 4);
    }
    /* idx<<9 is the plane offset. Copy the halved plane into blk14
     * starting two bytes in, and park the folded sum in front of it. */
    cursor = (plane + plane) & U32MASK;
    memcpy(b14 + (((cursor + 2U + base14) & U32MASK) - b14b),
        b15 + (((base15 + cursor) & U32MASK) - b15b), 0x1FE);
    folded = ((sum & 0xFFFFUL) + (sum >> 16)) & U32MASK;
    s16 = (unsigned short)(folded & 0xFFFFUL);
    memcpy(b14 + (((cursor + base14) & U32MASK) - b14b), &s16, 2);
    /* Doubling prefix over the blk14 plane. step lives in bits 8..15
     * and starts at 1; plane's low byte counts the inner walk. */
    step = (cursor & 0xFFFF00FFUL) | (1UL << 8);
    for (;;) {
        step = (step & 0xFFFFFF00UL) | ((step >> 8) & 0xFFUL);
        plane &= 0xFFFFFF00UL;
        step = (step & 0xFFFFFF00UL)
            | (((step & 0xFFUL) + ((step >> 8) & 0xFFUL)) & 0xFFUL);
        plane = (plane & 0xFFFFFF00UL) | ((0UL - (step & 0xFFUL)) & 0xFFUL);
        at = (plane + plane) & U32MASK;
        at = (at + base14) & U32MASK;
        for (;;) {
            unsigned int stride;
            unsigned int left;
            unsigned int acc;
            unsigned int back;
            stride = (((step >> 8) & 0xFFUL) * 2U) & U32MASK;
            left = (at - stride) & U32MASK;
            s16 = 0;
            memcpy(&s16, b14 + (left - b14b), 2);
            stride = s16;
            s16 = 0;
            memcpy(&s16, b14 + (at - b14b), 2);
            acc = (stride + s16) & U32MASK;
            back = ((step & 0xFFUL) * 2U) & U32MASK;
            s16 = (unsigned short)(acc & 0xFFFFUL);
            memcpy(b14 + (at - b14b), &s16, 2);
            at = (at - back) & U32MASK;
            plane = (plane & 0xFFFFFF00UL)
                | (((plane & 0xFFUL) - (step & 0xFFUL)) & 0xFFUL);
            if ((plane & 0xFFUL) == 0)
                break;
        }
        step = (step & 0xFFFF00FFUL)
            | (((((step >> 8) & 0xFFUL) * 2U) & 0xFFUL) << 8);
        if (((step >> 8) & 0xFFUL) >= 0x80UL)
            break;
    }
    if (idx < 0x2FFUL) {
        unsigned int word;
        q = b17 + (((D4V_BLK_PTR_17_32(&c->st, 0) + idx * 2U) & U32MASK) - b17b);
        s16 = 0;
        memcpy(&s16, q, 2);
        word = ((unsigned int)s16 >> 2) & 0xFFFFUL;
        word = (word + 1U) & U32MASK;
        s16 = (unsigned short)(word & 0xFFFFUL);
        memcpy(q, &s16, 2);
    }
}


/* ------------------------------------------------------------------ */
/* Role: PPM order-N update helper. Indexes the u16 plane at [0x433B7C] */
/* by (arg*0x100+sym), prices against word_45BA5E/word_45C05C side */
/* tables, accumulates into the [0x433B78] prefix plane (lowest-set-bit */
/* stepping) and the [0x433B84] totals; rescales via d4vc_rescale_ppm_plane when a */
/* total reaches 0x4000. */
/* Entry-live regs: al = sym (mov bl,al), t3 = arg (mov t6,t3 and */
/* (4182FB/419815/41C183/41CC66: al=sym byte, t3=dword arg). Returns */
/* void (callers ignore t0; exits leave callee residue). */
/* Frame: 8 bytes (var_1C @ sp+0, var_18 @ sp+4) + 5 pushed regs. */
/* Globals touched: 433B78, 433B7C, 433B84, 45AFA4, 45BA5E, 45C05C. */
/* Calls sub_416A00 as d4vc_rescale_ppm_plane (t0 = rescale index). */
/* ------------------------------------------------------------------ */
/* Walk a byte index through its lowest set bit, adding price to
 * each u16 of the blk14 plane. A starting index of 0 does nothing. */
static void d4v_plane_add(unsigned char *base, unsigned int index, unsigned int price)
{
    unsigned short s;
    unsigned int lsb;
    while (index != 0) {
        s = 0;
        memcpy(&s, base + index * 2U, 2);
        s = (unsigned short)((s + (price & 0xFFFFU)) & 0xFFFFU);
        memcpy(base + index * 2U, &s, 2);
        lsb = ((0U - index) & index) & 0xFFU;
        index = (index + lsb) & 0xFFU;
    }
}

void d4vc_ppm_order_update(dec4v_ctx *c, unsigned int sym, unsigned int arg)
{
    unsigned int symbol;
    unsigned int price;
    unsigned int total;
    unsigned int head;
    unsigned int prev;
    unsigned char *b14;
    unsigned char *b15;
    unsigned char *b17;
    unsigned char *cell;
    unsigned char *plane;
    uint32_t b14b;
    uint32_t b15b;
    uint32_t b17b;
    unsigned int base14;
    unsigned int base15;
    unsigned int base17;
    unsigned int cell_at;
    unsigned short s;

    symbol = sym & 0xFFU;
    b14 = c->blk14;
    b14b = c->blk14_base;
    b15 = c->blk15;
    b15b = c->blk15_base;
    b17 = c->blk17;
    b17b = c->blk17_base;
    base14 = (unsigned int)D4V_BLK_PTR_14_32(&c->st, 0);
    base15 = (unsigned int)D4V_BLK_PTR_15_32(&c->st, 0);
    base17 = (unsigned int)D4V_BLK_PTR_17_32(&c->st, 0);
    cell_at = (base15 + ((((arg << 8) + symbol) * 2U) & U32MASK)) & U32MASK;
    price = *(unsigned short *)(void *)d4v_fx(c, 244318UL + arg * 2U);
    cell = b15 + (cell_at - b15b);
    s = 0;
    memcpy(&s, cell, 2);
    if (s == 0) {
        unsigned int price2;
        unsigned int slot;
        unsigned int sum;
        price2 = *(unsigned short *)(void *)d4v_fx(c, 245852UL + arg * 2U);
        cell = b17 + (((base17 + arg * 2U) & U32MASK) - b17b);
        s = 0;
        memcpy(&s, cell, 2);
        s = (unsigned short)((s + (price & 0xFFFFU)) & 0xFFFFU);
        memcpy(cell, &s, 2);
        slot = (unsigned int)D4V_TBL_45AFA4_32(&c->st, 0);
        cell = b15 + (((base15 + ((((slot << 8) + symbol) * 2U) & U32MASK)) & U32MASK) - b15b);
        s = 0;
        memcpy(&s, cell, 2);
        s = (unsigned short)((s + (price2 & 0xFFFFU)) & 0xFFFFU);
        memcpy(cell, &s, 2);
        plane = b14 + (((base14 + ((slot << 9) & U32MASK)) & U32MASK) - b14b);
        d4v_plane_add(plane, (symbol + 1U) & 0xFFU, price2);
        s = 0;
        memcpy(&s, plane, 2);
        sum = ((unsigned int)s + (price2 & 0xFFFFU)) & U32MASK;
        s = (unsigned short)sum;
        memcpy(plane, &s, 2);
        if ((sum & 0xFFFFU) >= 0x4000U)
            d4vc_rescale_ppm_plane(c, slot);
        cell = b15 + (cell_at - b15b);
    }
    if (arg == 0x200UL)
        return;
    s = 0;
    memcpy(&s, cell, 2);
    s = (unsigned short)((s + (price & 0xFFFFU)) & 0xFFFFU);
    memcpy(cell, &s, 2);
    plane = b14 + (((base14 + ((arg << 9) & U32MASK)) & U32MASK) - b14b);
    d4v_plane_add(plane, (symbol + 1U) & 0xFFU, price);
    s = 0;
    memcpy(&s, plane, 2);
    head = ((unsigned int)s + (price & 0xFFFFU)) & U32MASK;
    s = (unsigned short)head;
    memcpy(plane, &s, 2);
    cell = b17 + (((base17 + arg * 2U) & U32MASK) - b17b);
    s = 0;
    memcpy(&s, cell, 2);
    prev = s;
    total = ((head & 0xFFFFU) + prev) & U32MASK;
    if ((long)total >= 0x4000L)
        d4vc_rescale_ppm_plane(c, arg);
}

/* ------------------------------------------------------------------ */
/* Role: tiny PPM param setter. Treats entry t0 as a 2-byte object */
/* (counts at [obj]/[obj+1]); prices sum=[0]+[1] against 45D0BC/45D0B8 */
/* quotients, records the al outcome into 45AF6C/45AF68/45AF70, bumps */
/* one lane, and halves both bytes when either reaches 0xFE. */
/* Entry-live regs: t0 = obj, an arena offset (deref [t0]/[t0+1] */
/* verified at both call sites: 418C3A builds an arena pointer, 418F78 */
/* builds one from unk_45AA80). Returns al in t0 (0/1 compare result */
/* consumed via test al,al at 418C3F/418F7D). */
/* Frame: no locals; 3 pushed regs (t1/t2/t3). */
/* Globals touched: 45AF68, 45AF6C, 45AF70, 45D0B8, 45D0BC. */
/* ------------------------------------------------------------------ */
/* Price the two-byte counter at obj. Returns 1 when the range-coder
 * code falls below the first count. That arm bumps count 0 and sets
 * the bin to [0, count0); the other arm bumps count 1 and sets the
 * bin to [count0, sum). Either count reaching 0xFE halves both. */
unsigned int d4vc_ppm_pair_price(dec4v_ctx *c, unsigned int obj)
{
    unsigned char *p;
    unsigned char tmp[2];
    int local;
    unsigned int sum;
    unsigned int range;
    unsigned int code;
    unsigned int below;

    p = d4v_span(c, (uint32_t)obj, 2);
    local = 0;
    if (!p) {
        tmp[0] = *(unsigned char *)d4v_ptr(c, (uint32_t)obj);
        tmp[1] = *(unsigned char *)d4v_ptr(c, (uint32_t)((obj + 1) & U32MASK));
        p = tmp;
        local = 1;
    }
    sum = ((unsigned int)p[0] + (unsigned int)p[1]) & U32MASK;
    D4V_RSLOT_LO_32(&c->st, 0) = sum;
    range = D4V_RC_RANGE_32(&c->st, 0) / sum;
    D4V_RC_RANGE_32(&c->st, 0) = range;
    code = (D4V_RC_CODE_32(&c->st, 0) / range) & 0xFFFFUL;
    below = ((long)code < (long)p[0]) ? 1U : 0U;
    if (below) {
        D4V_RSLOT_MID_32(&c->st, 0) = 0;
        D4V_RSLOT_HI_32(&c->st, 0) = p[0];
        p[0] = (unsigned char)(p[0] + 1U);
    } else {
        D4V_RSLOT_MID_32(&c->st, 0) = p[0];
        D4V_RSLOT_HI_32(&c->st, 0) = D4V_RSLOT_LO_32(&c->st, 0);
        p[1] = (unsigned char)(p[1] + 1U);
    }
    if (p[0] >= 0xFEU || p[1] >= 0xFEU) {
        p[0] = (unsigned char)(((unsigned int)p[0] + 1U) >> 1);
        p[1] = (unsigned char)(((unsigned int)p[1] + 1U) >> 1);
    }
    if (local) {
        *(unsigned char *)d4v_ptr(c, (uint32_t)obj) = tmp[0];
        *(unsigned char *)d4v_ptr(c, (uint32_t)((obj + 1) & U32MASK)) = tmp[1];
    }
    return below;
}
/* ------------------------------------------------------------------ */
/* Scores PPM orders 0..7 from the shift registers and returns the
 * winning slot address, or 0 when no order is accepted. */
unsigned int d4vc_ppm_score_orders(dec4v_ctx *c, unsigned int ebx_in)
{
    unsigned int r0;
    unsigned int r1;
    unsigned int tag;
    unsigned int cursor;
    unsigned int dist;
    unsigned int mask;
    unsigned int ptr;
    unsigned int addr;
    unsigned int byte_a;
    unsigned int byte_b;
    unsigned int seed;
    unsigned int al;
    unsigned int hi;
    unsigned int lo;
    unsigned int best;
    unsigned int win;
    unsigned int ord;
    unsigned int off;
    unsigned int b0;
    unsigned int b1;
    unsigned int slot;
    unsigned char *cell;

    (void)ebx_in;
    D4V_TBL_45BA4C_32(&c->st, 0) = 0;
    r0 = D4V_SHIFT_R0_32(&c->st, 0);
    r1 = D4V_SHIFT_R1_32(&c->st, 0);
    if ((r0 & 0xFFFF0000UL) == 0xFFFF0000UL)
        tag = 0x10000UL;
    else if ((r0 & 0xFFFFUL) == 0xFFFFUL)
        tag = 0x20000UL;
    else if ((r0 & 0xFF000000UL) == 0xFF000000UL)
        tag = 0x30000UL;
    else if ((r0 & 0xFF0000UL) == 0xFF0000UL)
        tag = 0x40000UL;
    else if ((r0 & 0xFF00UL) == 0xFF00UL)
        tag = 0x50000UL;
    else if ((r0 & 0xFFUL) == 0xFFUL)
        tag = 0x60000UL;
    else
        tag = 0;
    D4V_TBL_459E40_32(&c->st, 0) = (r0 >> 16) & 0xFFFFU;
    D4V_TBL_459E38_32(&c->st, 0) =
        ((r1 >> 24) + ((r0 >> 16) & 0xFF00U)) & U32MASK;
    D4V_TBL_459E3C_32(&c->st, 0) = (r0 >> 8) & 0xFFFFU;
    D4V_TBL_459E44_32(&c->st, 0) =
        (((r0 >> 8) & 0xFF00U) + (r0 & 0xFFU)) & U32MASK;
    D4V_TBL_459E48_32(&c->st, 0) =
        (((r0 >> 16) & 0xFF00U) + ((r0 >> 8) & 0xFFU)) & U32MASK;
    D4V_TBL_459E4C_32(&c->st, 0) =
        (((r0 >> 16) & 0xFF00U) + (r0 & 0xFFU)) & U32MASK;
    D4V_TBL_459E50_32(&c->st, 0) = ((r1 & 0xFFFFU) + tag) & U32MASK;
    if ((r0 & 0xFFU) == 0xFFU) {
        D4V_TBL_459E3E_16(&c->st, 0) =
            (unsigned short)(D4V_TBL_459E3E_16(&c->st, 0) + 1U);
        D4V_TBL_459E42_16(&c->st, 0) =
            (unsigned short)(D4V_TBL_459E42_16(&c->st, 0) + 1U);
    }
    cursor = D4V_DICT_CURSOR_32(&c->st, 0);
    dist = D4V_TBL_45AF7C_32(&c->st, 0);
    mask = D4V_DICT_MASK_32(&c->st, 0);
    ptr = D4V_BLK_PTR_00_DICT_32(&c->st, 0);
    addr = (mask & ((cursor - dist) & U32MASK)) & U32MASK;
    byte_a = c->blk00_dict[((addr + ptr) & U32MASK) - c->blk00_base];
    addr = (mask & ((cursor - ((dist + dist) & U32MASK)) & U32MASK)) & U32MASK;
    byte_b = c->blk00_dict[((addr + ptr) & U32MASK) - c->blk00_base];
    al = 0;
    if ((unsigned int)D4V_CTX_B_45C681_8(&c->st, 0) == dist) {
        if ((r0 & 0xFFU) == 1U)
            al = 8;
        seed = ((r0 >> 16) == 0xF6FFU) ? 1U : 0U;
    } else {
        /* unsigned; a zero distance faults like x86 DIV */
        seed = D4V_TBL_45AF38_32(&c->st, 0) % dist;
    }
    D4V_TBL_459E54_32(&c->st, 0) = (seed << 8) & U32MASK;
    hi = (byte_a - byte_b) & 0xFFU;
    lo = ((byte_a + byte_a) - byte_b) & 0xFFU;
    D4V_TBL_459E54_32(&c->st, 0) =
        (D4V_TBL_459E54_32(&c->st, 0) + hi) & U32MASK;
    c->blk13[D4V_TBL_459E54_32(&c->st, 0) * 4U + 2U] = (unsigned char)lo;
    best = 0;
    win = 8;
    if (al < 8U) {
        for (ord = 0; ord < 8U; ord++) {
            unsigned int prod;
            unsigned int den;
            unsigned int quot;
            off = (D4V_TBL_459E38_32(&c->st, ord * 4U) << 2) & U32MASK;
            cell = c->blkord[ord];
            if (cell[off + 1] == 0)
                continue;
            if (cell[off + 3] >= 0x80U)
                continue;
            prod = (unsigned int)cell[off + 3] * ((unsigned int)cell[off] << 8);
            den = (unsigned int)cell[off + 1];
            /* signed; a zero denominator faults like x86 IDIV */
            quot = (unsigned int)((int)prod / (int)den);
            if (quot > best) {
                best = quot;
                win = ord;
            }
        }
    }
    if (win < 8U) {
        off = (D4V_TBL_459E38_32(&c->st, win * 4U) << 2) & U32MASK;
        cell = c->blkord[win];
        slot = (c->blkord_base[win] + off) & U32MASK;
        b0 = cell[off];
        b1 = cell[off + 1];
        if (b0 > b1 || (cell[off + 3] & 0x7FU) > 1U) {
            unsigned int rel;
            if (D4V_TBL_45AF7C_32(&c->st, 0) > 1U)
                D4V_CTX_B_45C684_8(&c->st, 0) = (unsigned char)(win == 7U);
            if (b1 == 2U && ((b0 - 3U) & 0xFFU) < 4U) {
                rel = ((b0 << 3) - 0x18U + win) & U32MASK;
                rel = (rel + rel) & U32MASK;
                D4V_TBL_45BA4C_32(&c->st, 0) = (D4V_TBL_45AE80 + rel) & U32MASK;
            }
            if (b1 == 3U && ((b0 - 4U) & 0xFFU) < 4U) {
                rel = ((b0 << 3) - 0x20U + win) & U32MASK;
                rel = (rel + rel) & U32MASK;
                rel = (rel + 0x40U) & U32MASK;
                D4V_TBL_45BA4C_32(&c->st, 0) = (D4V_TBL_45AE80 + rel) & U32MASK;
            }
            D4V_CTX_B_45C682_8(&c->st, 0) = (unsigned char)win;
            return slot;
        }
    }
    if (D4V_TBL_45AF98_32(&c->st, 0) > 1U) {
        unsigned int at;
        unsigned char *p;
        at = D4V_TBL_45AFA0_32(&c->st, 0);
        p = c->st.g.bytes + (at - D4V_GLOBALS_BASE);
        d4v_gst32(p, d4v_gld32(p) >> 1);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Role: PPM mask/order selector. Prices the symbol (entry al) against */
/* the 4593A0 last-pos table and the 458798 frequency table (with */
/* periodic halving into 458794), maintains the 45AF94/45AF98/45AF7C */
/* best-order state, bumps 45AF38/45AF90, then drives one of several */
/* order/mask update paths keyed on 45C67D/45C67F/symbol range (shift */
/* registers 45BA18/45BA1C/45BA20, tables 434328/434338/434438 */
/* 457360/45BA58, or the 41E9F0/41E640 helpers). With nonzero entry */
/* dl it first runs the 45C65A/45C65C nibble-tickler path. */
/* Entry-live regs: al = sym (mov cl,al), dl = flag (test dl,dl). */
/* Only the low bytes are live; high bytes are dead on every path */
/* (no full-width read precedes an overwrite; verified by inspection). */
/* Callers pass high-zeroed t0 (418366/41991B mov t0,t2 after */
/* (callers ignore t0). */
/* Frame: 4 bytes (var_18 byte @ sp+0) + 5 pushed regs (t1/t2/si/ */
/* di/bp; t3 is the flag input and is NOT preserved). */
/* Globals touched: 433B40, 434328, 434338, 434438, 457360, 458794 */
/* 458798, 45879C, 4593A0, 45AF2C, 45AF38, 45AF3C, 45AF40, 45AF48 */
/* 45AF90, 45AF94, 45AF98, 45AF7C, 45BA18, 45BA1C, 45BA20, 45BA58 */
/* 45BA34, 45BA3C, 45C65A, 45C65C, 45C677, 45C678, 45C679, 45C67A */
/* 45C67B, 45C67C, 45C67D, 45C67F, 45C68E. */
/* Calls d4vc_s4_update_model (t0=sym, t3=derived byte) and d4vc_s4_pick_predictor. */
/* Deviations: div @417A38 (t3 pre-zeroed) and @417CB3 (divisor is */
/* const 6) are direct C / with comments (zero divisor faults like */
/* x86); shr t0,cl @417A51 masks the count to 5 bits per x86. */
/* ------------------------------------------------------------------ */
void d4vc_ppm_mask_select(dec4v_ctx *c, unsigned int sym, unsigned int flag)
{
    uint32_t t0 = 0, t1 = 0, t2 = 0, t3 = 0, t5 = 0, t6 = 0;
    /* prologue 0x004177A0 push t1; 0x004177A1 push t2;
     * 0x004177A2 push t4; 0x004177A3 push t5; 0x004177A4 push t6;
     * 0x004177A5 sub esp,4.
     * Callee-saved regs become C locals; the esp frame is t_18b. */

    t1 = (uint32_t)D4V_TBL_45AF90_32(&c->st, 0);
    t2 = (uint32_t)sym & 0xFFUL;
    t3 = (uint32_t)flag; /* entry dl (t3 high dead: xored before any full read) */
    ;
    if ((t3 & 0xFFUL) != 0) {
        unsigned int parity;
        unsigned int flip;
        unsigned char n0;
        unsigned char n1;
        unsigned char low;
        unsigned char probe;
        low = (unsigned char)t2;
        parity = (unsigned int)D4V_TBL_45AF38_8(&c->st, 0) & 1U;
        n0 = D4V_CTX_B_45C65A_8(&c->st, parity);
        n0 = (unsigned char)((n0 + n0) & 0x0FU);
        D4V_CTX_B_45C65A_8(&c->st, parity) = n0;
        n1 = D4V_CTX_B_45C65C_8(&c->st, parity);
        n1 = (unsigned char)((n1 + n1) & 0x0FU);
        D4V_CTX_B_45C65C_8(&c->st, parity) = n1;
        D4V_CTX_B_45C68E_8(&c->st, 0) = 0;
        if (low >= 0x80U) {
            if (low == 0xFFU) {
                n1 = (unsigned char)(n1 + 1U);
                D4V_CTX_B_45C65C_8(&c->st, parity) = n1;
                flip = parity == 0U ? 1U : 0U;
                probe = D4V_CTX_B_45C65A_8(&c->st, flip);
                if (probe == 0x0FU && probe == n1)
                    D4V_CTX_B_45C68E_8(&c->st, 0) = 2;
            } else if (low <= 0xDFU || (low >= 0xF2U && low <= 0xF6U)) {
                n0 = (unsigned char)(n0 + 1U);
                D4V_CTX_B_45C65A_8(&c->st, parity) = n0;
                flip = parity == 0U ? 1U : 0U;
                probe = D4V_CTX_B_45C65C_8(&c->st, flip);
                if (probe == 0x0FU && probe == n0)
                    D4V_CTX_B_45C68E_8(&c->st, 0) = 1;
            }
        }
    }
    if ((t2 & 0xFFUL) != 0 && (t2 & 0xFFUL) != 0xFFUL) {
    {
        uint32_t cursor;
        uint32_t mask;
        uint32_t dist;
        uint32_t slot;
        cursor = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
        mask = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
        dist = (cursor - (uint32_t)D4V_TBL_4593A0_32(&c->st, (t2 & 0xFFUL) * 4U)) & mask;
        if (dist >= 0x100UL) {
            dist = (cursor - (uint32_t)D4V_TBL_4593A0_32(&c->st, ((t2 - 1UL) & 0xFFUL) * 4U)) & mask;
            if (dist >= 0x100UL)
                dist = (cursor - (uint32_t)D4V_TBL_4593A0_32(&c->st, ((t2 + 1UL) & 0xFFUL) * 4U)) & mask;
        }
        if (dist != 0 && dist < 0x100UL) {
            uint32_t best;
            D4V_TBL_458798_32(&c->st, dist * 4U) =
                (uint32_t)D4V_TBL_458798_32(&c->st, dist * 4U) + 1U;
            best = (uint32_t)D4V_TBL_45AF98_32(&c->st, 0);
            if (dist != best) {
                uint32_t at;
                uint32_t ptr;
                at = (cursor - best) & mask;
                ptr = (uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0);
                if ((t2 & 0xFFUL) != (uint32_t)c->blk00_dict[((at + ptr) & U32MASK) - c->blk00_base])
                    t1 = (t1 + 1U) & U32MASK;
                else
                    D4V_TBL_458798_32(&c->st, best * 4U) =
                        (uint32_t)D4V_TBL_458798_32(&c->st, best * 4U) + 1U;
                slot = (uint32_t)D4V_TBL_45AF94_32(&c->st, 0);
                if (dist != slot)
                    D4V_TBL_45AF94_32(&c->st, 0) = (unsigned int)dist;
                else if ((uint32_t)D4V_TBL_458798_32(&c->st, slot * 4U)
                    >= (uint32_t)D4V_TBL_458798_32(&c->st, best * 4U)) {
                    D4V_TBL_45AF94_32(&c->st, 0) = (unsigned int)best;
                    D4V_TBL_45AF98_32(&c->st, 0) = (unsigned int)dist;
                    if (dist > 1UL)
                        D4V_TBL_45AF7C_32(&c->st, 0) = (unsigned int)dist;
                }
            }
        }
    }
        if (t1 < 0x40UL)
            t1 = (t1 + 1) & U32MASK;
        else {
            t6 = (uint32_t)D4V_TBL_45879C_32(&c->st, 0);
            t1 = 0;
            t6 = (t6 + 1) & U32MASK;
            D4V_TBL_45879C_32(&c->st, 0) = (unsigned int)t6;
            for (t0 = 0; t0 != 0x400UL; ) {
                t3 = (uint32_t)D4V_TBL_458798_32(&c->st, t0);
                t3 >>= 1;
                t0 = (t0 + 4) & U32MASK;
                t1 = (t1 + t3) & U32MASK;
                D4V_TBL_458794_32(&c->st, t0) = (unsigned int)t3;
            }
        }
        t0 = 0;
        t0 |= (t2 & 0xFFUL);
        t3 = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
        D4V_TBL_4593A0_32(&c->st, (t0 & 0xFFUL) * 4) = (unsigned int)t3;
    }
    {
        uint32_t cls;
        unsigned char adj;
        unsigned char nb;
        cls = (uint32_t)D4V_CTX_B_45C67D_8(&c->st, 0);
        t0 = (t0 & 0xFFFFFF00UL) | cls;
        D4V_TBL_45AF90_32(&c->st, 0) = (unsigned int)t1;
        if (cls != 0) {
        adj = (unsigned char)((t2 & 0xFFUL) - (uint32_t)D4V_CTX_B_45C679_8(&c->st, 0));
        nb = D4V_CTX_B_45C67B_8(&c->st, 0);
        if (nb != 0)
            adj = (unsigned char)(0U - adj);
        if (adj >= 0x80U) {
            nb = (unsigned char)(nb == 0);
            D4V_CTX_B_45C67B_8(&c->st, 0) = nb;
        }
        t0 = (t0 & 0xFFFFFF00UL) | adj;
        t2 = (t2 & 0xFFFF00FFUL) | (cls << 8);
        if (cls == 1) {
            t3 = ((uint32_t)D4V_TBL_45AF3C_32(&c->st, 0) - 0x203UL) & 0xFUL;
            t3 = (uint32_t)d4v_t434328[t3] << 4;
            t0 = (uint32_t)d4v_t434338[t0 & 0xFFUL];
            t3 = (t3 + t0 + 0x203UL) & U32MASK;
            D4V_TBL_45AF90_32(&c->st, 0) = (unsigned int)t1;
            D4V_TBL_45AF3C_32(&c->st, 0) = (unsigned int)t3;
        } else {
            uint32_t base;
            unsigned char idx_b;
            idx_b = (unsigned char)t0;
            base = ((uint32_t)D4V_TBL_45AF48_32(&c->st, 0) << 4) & U32MASK;
            t3 = base;
            D4V_TBL_45AF90_32(&c->st, 0) = (unsigned int)t1;
            if (cls == 2) {
                t0 = ((uint32_t)d4v_t434338[idx_b] + base + 0x223UL) & U32MASK;
                D4V_TBL_45AF3C_32(&c->st, 0) = (unsigned int)t0;
            } else if (cls == 3) {
                unsigned char idx;
                uint32_t slot;
                idx = idx_b;
                if (D4V_CTX_B_45C677_8(&c->st, 0) != 0) {
                    unsigned char nb;
                    idx = (unsigned char)((t2 & 0xFFUL) - ((uint32_t)D4V_SHIFT_R0_32(&c->st, 0) >> 16));
                    nb = D4V_CTX_B_45C67C_8(&c->st, 0);
                    D4V_CTX_B_45C67B_8(&c->st, 0) = nb;
                    if (nb != 0)
                        idx = (unsigned char)(0U - idx);
                    if (idx >= 0x80U)
                        D4V_CTX_B_45C67B_8(&c->st, 0) = (unsigned char)(nb == 0);
                }
                slot = (uint32_t)D4V_CTX_B_45C678_8(&c->st, 0);
                if ((t2 & 0xFFUL) == (uint32_t)D4V_TBL_45BA58_8(&c->st, slot & 0xFFUL))
                    D4V_TBL_457360_32(&c->st, slot * 4U) =
                        (uint32_t)D4V_TBL_457360_32(&c->st, slot * 4U) + 1U;
                else
                    D4V_TBL_457360_32(&c->st, slot * 4U) =
                        (uint32_t)D4V_TBL_457360_32(&c->st, slot * 4U) >> 1;
                t3 = (uint32_t)d4v_t434438[idx];
                t0 = (((uint32_t)D4V_TBL_45AF48_32(&c->st, 0) << 5) + t3 + 0x243UL) & U32MASK;
                D4V_TBL_45AF3C_32(&c->st, 0) = (unsigned int)t0;
            } else if (cls == 4) {
                t0 = (uint32_t)d4v_t434338[idx_b];
                t3 = (base + t0 + 0x2A3UL) & U32MASK;
                D4V_TBL_45AF3C_32(&c->st, 0) = (unsigned int)t3;
            } else {
                t3 = idx_b;
                d4vc_s4_update_model(c, t2 & 0xFFUL, t3);
            }
        }
        }
    }
    {
        uint32_t flag67;
        uint32_t sym;
        flag67 = (uint32_t)D4V_CTX_B_45C67F_8(&c->st, 0);
        sym = t2 & 0xFFUL;
        t1 = (uint32_t)D4V_TBL_45AF90_32(&c->st, 0);
        D4V_TBL_45AF38_32(&c->st, 0) =
            (uint32_t)D4V_TBL_45AF38_32(&c->st, 0) + 1U;
        if (flag67 != 0 || sym < 4UL || sym > 6UL
            || (uint32_t)D4V_SHIFT_R0_8(&c->st, 0) != 0xDFU) {
            uint32_t r0;
            uint32_t r1;
            uint32_t r2;
            uint32_t byte;
            r0 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
            r1 = (uint32_t)D4V_SHIFT_R1_32(&c->st, 0);
            r2 = (uint32_t)D4V_SHIFT_R2_32(&c->st, 0);
            if (flag67 == 0 && sym >= 4UL && sym <= 6UL)
                byte = 0xDFUL;
            else
                byte = sym;
            D4V_SHIFT_R2_32(&c->st, 0) = (unsigned int)((r2 << 8) + (r1 >> 24));
            D4V_SHIFT_R1_32(&c->st, 0) = (unsigned int)((r1 << 8) + (r0 >> 24));
            D4V_SHIFT_R0_32(&c->st, 0) = (unsigned int)((r0 << 8) + byte);
        }
        {
            uint32_t cls;
            uint32_t width;
            cls = (uint32_t)D4V_CTX_B_45C67D_8(&c->st, 0);
            t2 = (t2 & 0xFFFFFF00UL) | cls;
            D4V_TBL_45AF90_32(&c->st, 0) = (unsigned int)t1;
            if (cls == 0)
                return;
            t5 = (uint32_t)D4V_TBL_45AF48_32(&c->st, 0);
            width = (uint32_t)D4V_TBL_45AF40_32(&c->st, 0);
            t3 = (uint32_t)D4V_TBL_45AF38_32(&c->st, 0) % width;
            D4V_TBL_45AF48_32(&c->st, 0) = (unsigned int)t3;
            D4V_CTX_B_45C679_8(&c->st, 0) = (unsigned char)(
                (uint32_t)D4V_SHIFT_R0_32(&c->st, 0)
                >> ((uint32_t)D4V_CTX_B_45C67A_8(&c->st, 0) & 31UL));
            t2 = (t2 & 0xFFFF00FFUL) | (cls << 8);
            if (cls == 3) {
                uint32_t slot;
                uint32_t r0;
                uint32_t mix;
                uint32_t streak;
                unsigned int hot;
                r0 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
                slot = (uint32_t)D4V_TBL_45AF38_32(&c->st, 0) % 6U;
                D4V_CTX_B_45C678_8(&c->st, 0) = (unsigned char)slot;
                hot = (uint32_t)D4V_TBL_457360_32(&c->st, slot * 4U) > 8UL;
                mix = (uint32_t)D4V_CTX_B_45C679_8(&c->st, 0)
                    + ((r0 & 0xFFUL) - (r0 >> 24));
                D4V_TBL_45BA58_8(&c->st, slot) = (unsigned char)mix;
                streak = (uint32_t)D4V_TBL_45AF2C_32(&c->st, 0);
                if (hot && streak < 5UL) {
                    D4V_CTX_B_45C677_8(&c->st, 0) = 1;
                    D4V_CTX_B_45C67C_8(&c->st, 0) = D4V_CTX_B_45C67B_8(&c->st, 0);
                    D4V_CTX_B_45C67B_8(&c->st, 0) = 0;
                    D4V_TBL_45AF3C_32(&c->st, 0) =
                        (unsigned int)(((t5 << 5) + 0x253UL) & U32MASK);
                    D4V_CTX_B_45C679_8(&c->st, 0) =
                        D4V_TBL_45BA58_8(&c->st, slot);
                } else
                    D4V_CTX_B_45C677_8(&c->st, 0) = 0;
                if (hot)
                    D4V_TBL_45AF2C_32(&c->st, 0) = (unsigned int)(streak + 1U);
                else
                    D4V_TBL_45AF2C_32(&c->st, 0) = 0;
                D4V_TBL_45AF90_32(&c->st, 0) = (unsigned int)t1;
                return;
            }
            if (cls >= 5)
                d4vc_s4_pick_predictor(c);
            return;
        }
    }

}

/* ------------------------------------------------------------------ */
/* Role: PPM child-table allocator/init. Folds the symbol (entry al */
/* complemented into dl) with the 45C689 escape-state machine and the */
/* 45CD28/45BA44/43120C position bounds: either plants an escape flag */
/* (45C689=1/0x20), emits the raw byte via sub_4221F0, runs the */
/* 43691C-classified multi-byte emit loop (tables off_4362C0/436450 */
/* 436976/45C68C-derived index), or the differenced single emit. */
/* Entry-live regs: al = sym ONLY (first use mov dl,al). Deviation: */
/* entry t3 is destroyed by that same mov (callers set it anyway: 0 */
/* at 418357 via xor t3,t3, 1 at 419911 via mov t3,1), so the flag */
/* parameter is currently dead; entry t2 is likewise never read */
/* before overwrite (first read test cl,cl follows mov cl,[45C689]). */
/* Kept as (c, sym, flag) per caller discipline, with flag documented */
/* dead in this build. */
/* UNCERTAIN(004183D4): whether the t3 slot is a vestigial flag or */
/* padding is unknowable from two call sites; the translation keeps */
/* it verbatim (dl overwritten on entry). Returns void (both callers */
/* ignore t0; all exits leave call residue). Frame: no locals */
/* 4 pushed regs (t1/t2/t3/t4). */
/* Globals touched: 43120C, 436976, 43691C, 4362C0, 436450, 45BA44 */
/* 45CD28, 45C689, 45C68C. */
/* Calls d4v_4221f0 (t0 = emit item). */
/* ------------------------------------------------------------------ */
/* Emit one escaped literal. sym is stored complemented. The context
 * byte 45C689 selects a run of literal bytes, a class change, or a
 * single translated byte from the 436254 table. */
void d4vc_ppm_escape_emit(dec4v_ctx *c, unsigned int sym, unsigned int flag)
{
    unsigned int symbol;
    unsigned int mode;
    unsigned int produced;
    unsigned int cls;
    unsigned int len;
    unsigned int ptr;
    unsigned int byte;
    (void)flag;

    symbol = (~(sym & 0xFFU)) & 0xFFU;
    mode = (unsigned int)D4V_CTX_B_45C689_8(&c->st, 0);
    produced = (unsigned int)D4V_IO_OUT_TOTAL_32(&c->st, 0);
    if (mode == 1U) {
        unsigned int mark;
        D4V_CTX_B_45C689_8(&c->st, 0) = 0;
        if (produced < (unsigned int)D4V_TBL_45BA44_32(&c->st, 0)) {
            mark = ((unsigned int)D4V_CTX_B_45C68C_8(&c->st, 0) - 0xA6UL) & U32MASK;
            mark = (mark * 11U) & U32MASK;
            byte = d4v_t436254[(1826UL + mark + symbol) & U32MASK];
            d4v_put_byte(c, byte & 0xFFU);
            return;
        }
        if (symbol == 0xFEU) {
            d4v_put_byte(c, 0xFEU);
            return;
        }
        d4v_put_byte(c, 0);
        d4v_put_byte(c, 0);
        d4v_put_byte(c, 3);
        d4v_put_byte(c, 3);
        d4v_put_byte(c, symbol);
        return;
    }
    if (produced >= (unsigned int)D4V_TBL_45BA44_32(&c->st, 0) && symbol == 0xFEU) {
        D4V_CTX_B_45C689_8(&c->st, 0) = 1;
        return;
    }
    if (produced >= (unsigned int)D4V_PREFIX_SUM_TMP_32(&c->st, 0)
        || (symbol < 0x80U && mode == 0U)) {
        d4v_put_byte(c, symbol);
        return;
    }
    cls = d4v_t43691C[symbol];
    if (cls >= 0xC8U) {
        d4v_put_byte(c, (symbol - mode) & 0xFFU);
        D4V_CTX_B_45C689_8(&c->st, 0) = 0;
        return;
    }
    if (cls == 0x64U) {
        D4V_CTX_B_45C68C_8(&c->st, 0) = (unsigned char)symbol;
        D4V_CTX_B_45C689_8(&c->st, 0) = 1;
        return;
    }
    if (cls == 0x63U) {
        D4V_CTX_B_45C689_8(&c->st, 0) = 0x20;
        return;
    }
    ptr = d4v_t4362C0[cls];
    len = d4v_t436450[cls];
    if (mode != 0U) {
        byte = (~d4v_ld8g(c, ptr) - 0x20U) & 0xFFU;
        d4v_put_byte(c, byte);
        ptr = (ptr + 1U) & U32MASK;
        len = (len - 1U) & 0xFFU;
        D4V_CTX_B_45C689_8(&c->st, 0) = 0;
    }
    while (len != 0U) {
        byte = (~d4v_ld8g(c, ptr)) & 0xFFU;
        d4v_put_byte(c, byte);
        ptr = (ptr + 1U) & U32MASK;
        len = (len - 1U) & 0xFFU;
    }
}

/* ---- workers: PPM helpers batch S3 (from draft_s3.c) ---- */
void d4vc_ppm_touch_len(dec4v_ctx *c, unsigned int len, unsigned int flag);
void d4vc_ppm_rescale_orders(dec4v_ctx *c, unsigned int sym)
{
    unsigned int symbol;
    unsigned int ord;
    unsigned int acc;
    unsigned int addr;
    unsigned char *c4;
    unsigned char c4tmp[2];
    int c4copy;

    symbol = sym & 0xFFU;
    addr = (unsigned int)D4V_TBL_45BA4C_32(&c->st, 0);
    c4 = d4v_span(c, (uint32_t)addr, 2);
    c4copy = 0;
    if (!c4) {
        c4tmp[0] = *(unsigned char *)d4v_ptr(c, (uint32_t)addr);
        c4tmp[1] = *(unsigned char *)d4v_ptr(c, (uint32_t)(addr + 1U));
        c4 = c4tmp;
        c4copy = 1;
    }
    acc = (unsigned int)D4V_TBL_45AF88_32(&c->st, 0);
    for (ord = 0; ord < 8U; ord++) {
        unsigned int cnt1;
        unsigned int num;
        unsigned int den;
        unsigned int quot;
        unsigned char *slot;
        unsigned int off;
        unsigned int ctx;

        slot = c->blkord[ord];
        off = ((unsigned int)D4V_TBL_459E38_32(&c->st, ord * 4U) << 2) & U32MASK;
        cnt1 = slot[off + 1U];
        if (cnt1 == 0U) {
            slot[off] = 2;
            slot[off + 1U] = 2;
            slot[off + 2U] = (unsigned char)symbol;
            slot[off + 3U] = 0;
            continue;
        }
        ctx = (unsigned int)D4V_CTX_B_45C682_8(&c->st, 0);
        if (ord == ctx) {
            if (D4V_CTX_B_45C685_8(&c->st, 0) != 0) {
                if (symbol == slot[off + 2U]) {
                    D4V_CTX_B_45C68D_8(&c->st, 0) = 0;
                    num = slot[off];
                } else
                    num = slot[off + 1U];
                den = ((unsigned int)slot[off] + (unsigned int)slot[off + 1U]) & U32MASK;
                quot = (num << 14) / den;
                quot = *(unsigned short *)(void *)d4v_fx(c, 96616UL + (quot & U32MASK) * 2U);
                acc = (acc + (quot & 0xFFFFU)) & U32MASK;
            }
            if (addr != 0U) {
                if (symbol == slot[off + 2U])
                    c4[0] = (unsigned char)(c4[0] + 1U);
                else
                    c4[1] = (unsigned char)(c4[1] + 1U);
                if (c4[0] >= 0xFEU || c4[1] >= 0xFEU) {
                    c4[0] = (unsigned char)(((unsigned int)c4[0] + 1U) >> 1);
                    c4[1] = (unsigned char)(((unsigned int)c4[1] + 1U) >> 1);
                }
            }
        }
        slot[off + 3U] = (unsigned char)(slot[off + 3U] & 0x7FU);
        if (symbol == slot[off + 2U]) {
            if (slot[off] >= 0xFEU) {
                slot[off] = (unsigned char)(((unsigned int)slot[off] + 1U) >> 1);
                slot[off + 1U] = (unsigned char)(((unsigned int)slot[off + 1U] + 1U) >> 1);
            }
            slot[off] = (unsigned char)(slot[off] + 1U);
            if (slot[off + 3U] < 0x7FU)
                slot[off + 3U] = (unsigned char)(slot[off + 3U] + 1U);
        } else {
            if (slot[off + 1U] >= 0xFEU) {
                slot[off] = (unsigned char)(((unsigned int)slot[off] + 1U) >> 1);
                slot[off + 1U] = (unsigned char)(((unsigned int)slot[off + 1U] + 1U) >> 1);
            }
            slot[off + 1U] = (unsigned char)(slot[off + 1U] + 1U);
            if (slot[off + 3U] < 2U) {
                slot[off + 2U] = (unsigned char)symbol;
                slot[off + 3U] = 0x80U;
            } else
                slot[off + 3U] = (unsigned char)((slot[off + 3U] >> 1) + 0x80U);
        }
    }
    if (c4copy) {
        *(unsigned char *)d4v_ptr(c, (uint32_t)addr) = c4tmp[0];
        *(unsigned char *)d4v_ptr(c, (uint32_t)(addr + 1U)) = c4tmp[1];
    }
    D4V_TBL_45AF88_32(&c->st, 0) = acc;
    D4V_TBL_45BA4C_32(&c->st, 0) = addr;
}

void d4vc_ppm_list_update_32(dec4v_ctx *c, unsigned int sym);
void d4vc_ppm_list_update_20(dec4v_ctx *c, unsigned int sym);
unsigned int d4vc_ppm_len_decode(dec4v_ctx *c, unsigned int len);
int d4vc_ppm_window_decode(dec4v_ctx *c, unsigned int a);
void d4vc_s4_reset_tables(dec4v_ctx *c);

/* sub_416C80 (UHARC04.lst:33289-33346).
 * Role: PPM per-length touch/rescale on the 8-byte entry at
 * unk_459BA8 + (len&0xFF)*8 (words at +2/+4/+6), then rotate
 * byte_45C691 through byte_433E77[2*old + dl].
 * Entry-live regs (first use): t0 (ANDed to 0xFF at 0x416C83),
 * t3 (dl compared at 0x416C90). t1/t4 are scratch (xor-clear
 * before use at 0x416CD1-0x416CD3); t2 is scratch.
 * Frame: no locals; pushes t1/t2/t4 only.
 * Deviation: none (void return; original leaves garbage in t0).
 */
/* Bump one cumulative count in the 8-byte length record of `len`.
 * flag 1 also bumps the middle bin. Past 0x4000 the two upper bins
 * are halved back onto the low bin, then the context byte is stepped
 * through the 433E77 map. */
void d4vc_ppm_touch_len(dec4v_ctx *c, unsigned int len, unsigned int flag)
{
    unsigned int flag8;
    unsigned int total;
    unsigned int upper;
    unsigned int lower;
    unsigned int ctx;
    unsigned char *e;
    unsigned short s;

    flag8 = flag & 0xFFU;
    e = c->st.g.bytes + ((D4V_TBL_459BA8 + (len & 0xFFU) * 8U) - D4V_GLOBALS_BASE);
    if (flag8 == 1) {
        s = 0;
        memcpy(&s, e + 2 + flag8 * 2, 2);
        s = (unsigned short)(s + 0x20U);
        memcpy(e + 2 + flag8 * 2, &s, 2);
    }
    s = 0;
    memcpy(&s, e + 6, 2);
    total = ((unsigned int)s + 0x20U) & U32MASK;
    s = (unsigned short)total;
    memcpy(e + 6, &s, 2);
    if ((total & 0xFFFFU) >= 0x4000U) {
        s = 0; memcpy(&s, e + 4, 2); upper = s;
        s = 0; memcpy(&s, e + 2, 2); lower = s;
        lower = d4v_sar(((upper - lower) + 1U) & U32MASK, 1);
        s = 0; memcpy(&s, e + 6, 2); total = s;
        upper = d4v_sar(((total - upper) + 1U) & U32MASK, 1);
        s = 0; memcpy(&s, e + 2, 2);
        lower = (lower + s) & U32MASK;
        upper = (upper + lower) & U32MASK;
        s = (unsigned short)lower; memcpy(e + 4, &s, 2);
        s = (unsigned short)upper; memcpy(e + 6, &s, 2);
    }
    ctx = (unsigned int)D4V_CTX_B_45C691_8(&c->st, 0);
    ctx = (unsigned int)d4v_t433E77[(ctx + ctx + flag8) & U32MASK];
    D4V_CTX_B_45C691_8(&c->st, 0) = (unsigned char)ctx;
}

/* sub_41E5F0 (UHARC04.lst:44053-44077).
 * Role: reset the small model tables: fill 0x1E40 bytes at 0x451920
 * with 0, set dword_451928 = 2, and store -1 into the three dwords of
 * each of 4 entries stepping 0x7C over 0x453560+{0x7C..0x1F0}.
 * Entry-live regs: none (t1/t0/t3/t2 all set from immediates
 * before first use at 0x41E5F3-0x41E5FF).
 * Frame: no locals; pushes t1/t2/t3 only.
 * Deviation: void return; the original retn leaves garbage t0 = 0x1F0.
 */
void d4vc_s4_reset_tables(dec4v_ctx *c)
{
    unsigned int off;

    d4v_fill(c, 0x1E40U, D4V_TBL_451920, 0U);
    D4V_TBL_451928_32(&c->st, 0) = 2;
    off = 0;
    do {
        off = (off + 0x7CU) & U32MASK;
        D4V_TBL_453560_32(&c->st, off) = 0xFFFFFFFFUL;
        D4V_TBL_453564_32(&c->st, off) = 0xFFFFFFFFUL;
        D4V_TBL_453568_32(&c->st, off) = 0xFFFFFFFFUL;
    } while (off != 0x1F0U);
}

/* sub_418590 (UHARC04.lst:35592-35759).
 * Role: PPM seen/escape-table range-decode for one length class:
 * stage 1 range-decodes an item of class (al) over the 8-byte entry at
 * unk_459BA8 + (al&0xFF)*8 (returns the surviving stage index 1..3 in
 * al after touching the model via sub_416C80); stage 2 (index 0)
 * range-decodes a 10-way symbol, updates byte_45C67D + the
 * dword_4342C4/43429C/byte_434314 tables, and loops until stage 1
 * yields nonzero (calling sub_41E5F0 when the new class exceeds 4).
 * Entry-live regs (first use): t0 (only al saved at 0x418598).
 * Frame: 8 bytes locals (var_18 byte=len, var_1C byte=stage) over
 * pushes t1/t2/t3/t4/t6 (esp+1Ch addressing).
 * Deviations: div-by-zero: direct `/` with comment (4 sites: the two
 * stage-1 divs and the two stage-2 divs); x86 div would also #DE on
 * quotient overflow, which the model keeps in range.
 */
/* Scale the range by the bin currently stored at 45AF68+4/+8 and
 * renorm. The third renorm argument is only a scratch byte. */
static void d4v_rc_take_bin(dec4v_ctx *c)
{
    unsigned int mid;
    unsigned int hi;
    unsigned int range;
    unsigned int code;
    unsigned int scratch;

    hi = d4v_slot32(c, 8u);
    mid = d4v_slot32(c, 4u);
    range = D4V_RC_RANGE_32(&c->st, 0);
    hi = (hi - mid) & U32MASK;
    mid = (mid * range) & U32MASK;
    hi = (hi * range) & U32MASK;
    code = D4V_RC_CODE_32(&c->st, 0);
    code = (code - mid) & U32MASK;
    scratch = 0;
    d4v_rcrenorm_reg(c, &code, &hi, &scratch);
    D4V_RC_CODE_32(&c->st, 0) = code;
    D4V_RC_RANGE_32(&c->st, 0) = hi;
}

unsigned int d4vc_ppm_len_decode(dec4v_ctx *c, unsigned int len)
{
    unsigned int len_class;
    unsigned int stage;
    unsigned int cursor;
    unsigned int total;
    unsigned int range;
    unsigned int code;
    unsigned int sym;
    unsigned char *e;
    unsigned short s;

    len_class = len & 0xFFU;
    for (;;) {
        e = c->st.g.bytes + ((D4V_TBL_459BA8 + len_class * 8U) - D4V_GLOBALS_BASE);
        s = 0;
        memcpy(&s, e + 6, 2);
        total = s;
        D4V_RSLOT_LO_32(&c->st, 0) = total;
        range = D4V_RC_RANGE_32(&c->st, 0) / total;
        D4V_RC_RANGE_32(&c->st, 0) = range;
        code = D4V_RC_CODE_32(&c->st, 0) / range;
        stage = 3;
        cursor = 6;
        do {
            stage = (stage - 1U) & 0xFFU;
            s = 0;
            memcpy(&s, e + cursor - 2, 2);
            cursor -= 2;
            if ((code & 0xFFFFU) >= s)
                break;
        } while (stage != 0);
        s = 0; memcpy(&s, e + cursor, 2);
        D4V_RSLOT_MID_32(&c->st, 0) = s;
        s = 0; memcpy(&s, e + cursor + 2, 2);
        D4V_RSLOT_HI_32(&c->st, 0) = s;
        d4v_rc_take_bin(c);
        if (stage != 0) {
            d4vc_ppm_touch_len(c, len_class, stage);
            return stage;
        }
        D4V_RSLOT_LO_32(&c->st, 0) = 10;
        range = D4V_RC_RANGE_32(&c->st, 0) / 10U;
        D4V_RC_RANGE_32(&c->st, 0) = range;
        sym = D4V_RC_CODE_32(&c->st, 0) / range;
        sym &= 0xFFU;
        D4V_RSLOT_MID_32(&c->st, 0) = sym;
        D4V_RSLOT_HI_32(&c->st, 0) = (sym + 1U) & U32MASK;
        d4v_rc_take_bin(c);
        if (sym == (unsigned int)D4V_CTX_B_45C67D_8(&c->st, 0))
            continue;
        D4V_CTX_B_45C67D_8(&c->st, 0) = (unsigned char)sym;
        D4V_TBL_45AF3C_32(&c->st, 0) = d4v_t4342C4[sym];
        D4V_TBL_45AF40_32(&c->st, 0) = d4v_t43429C[sym];
        D4V_CTX_B_45C67A_8(&c->st, 0) = d4v_t434314[sym];
        D4V_CTX_B_45C688_8(&c->st, 0) = (unsigned char)(sym == 5U || sym == 6U);
        D4V_TBL_45AF48_32(&c->st, 0) = 0;
        D4V_CTX_B_45C67B_8(&c->st, 0) = 0;
        D4V_CTX_B_45C679_8(&c->st, 0) = 0;
        if (sym > 4U)
            d4vc_s4_reset_tables(c);
    }
}
/* sub_417220 (UHARC04.lst:33828-34157).
 * Role: PPM per-literal escape/list walker over the 0x20-byte entry at
 * dword_433B44 + idx*32 (idx = dword_45AFAC, or idx&0x7FFF banks when
 * idx >= 0x40000): linear-search the symbol slot (byte_45C692 cursor,
 * 0xFF = unknown), price one context via word_437968, halve the
 * 14 count bytes on escape, insert/sort the new symbol, and account
 * dword_45AFD0/dword_45AFA8/dword_45BA24 (halved past 0x800).
 * Entry-live regs (first use): t0 (only al saved at 0x41722F).
 * Frame: 8 bytes locals (var_1C byte=symbol, var_20 byte=bank score)
 * over pushes t1/t2/t3/t4/t5/t6 (esp+20h addressing).
 * Deviations: div-by-zero: direct `/` with comment (0x4172D5 site);
 * x86 div would also #DE on quotient overflow, model-kept in range.
 */
void d4vc_ppm_list_update_32(dec4v_ctx *c, unsigned int sym)
{
    unsigned int cursor;
    unsigned int symbol;
    unsigned int idx;
    unsigned int num;
    unsigned int quot;
    unsigned int flag;
    unsigned int base;
    unsigned int node_at;
    unsigned char *node;
    unsigned char *b1;
    unsigned int b1b;

    cursor = (unsigned int)D4V_CTX_B_45C692_8(&c->st, 0);
    symbol = sym & 0xFFU;
    idx = (unsigned int)D4V_TBL_45AFAC_32(&c->st, 0);
    b1 = c->blk01;
    b1b = c->blk01_base;
    base = (unsigned int)D4V_BLK_PTR_01_32(&c->st, 0);
    if (idx >= 0x40000UL) {
        unsigned int slot;
        unsigned int first;
        unsigned int best;
        unsigned int step;
        unsigned int walk;
        unsigned int chosen;
        slot = idx & 0x7FFFUL;
        first = (base + ((slot << 5) & U32MASK)) & U32MASK;
        D4V_TBL_45AFAC_32(&c->st, 0) = slot;
        best = 0;
        step = 0;
        walk = first;
        if (b1[first - b1b] != 0) {
            for (;;) {
                unsigned int here;
                unsigned int kept;
                here = b1[(walk + 0x1BU) - b1b] & 0x3FU;
                kept = b1[((((best << 5) + first) & U32MASK) + 0x1BU) - b1b] & 0x3FU;
                if (kept > here)
                    best = step;
                walk = (walk + 0x100000UL) & U32MASK;
                step = (step + 0x8000UL) & U32MASK;
                D4V_CTX_B_45C692_8(&c->st, 0) = (unsigned char)cursor;
                if (step >= 0x40000UL)
                    break;
                if (b1[walk - b1b] == 0) {
                    best = step;
                    break;
                }
            }
        }
        chosen = (first + ((best << 5) & U32MASK)) & U32MASK;
        d4v_fill(c, 0x20U, chosen, 0);
        b1[chosen - b1b] = 1;
        b1[(chosen + 1U) - b1b] =
            (unsigned char)((D4V_CTX_B_45C693_8(&c->st, 0) + 1U) & 0xFFU);
        b1[(chosen + 0x0EU) - b1b] = (unsigned char)symbol;
        d4v_gst32(b1 + ((chosen + 0x1CU) - b1b),
            (unsigned int)D4V_TBL_45AFC4_32(&c->st, 0));
        if (step < 0x38000UL)
            b1[((chosen + 0x100000UL) & U32MASK) - b1b] = 0;
        return;
    }
    node_at = (base + ((idx << 5) & U32MASK)) & U32MASK;
    node = b1 + (node_at - b1b);
    if (cursor == 0xFFU) {
        cursor = 0;
        while (cursor < 0x0DU && node[cursor + 0x0EU] != symbol)
            cursor++;
    }
    if (D4V_CTX_B_45C68F_8(&c->st, 0) != 0 && cursor < 0x0DU) {
        if ((unsigned int)d4v_sar(node[cursor + 1U], 6) > node[0])
            D4V_TBL_45BA24_32(&c->st, 0) =
                (D4V_TBL_45BA24_32(&c->st, 0) + 4U) & U32MASK;
    }
    if (D4V_CTX_B_45C683_8(&c->st, 0) != 0) {
        if (cursor >= 0x0DU)
            num = node[0];
        else {
            D4V_CTX_B_45C687_8(&c->st, 0) = 0;
            D4V_CTX_B_45C685_8(&c->st, 0) = 0;
            D4V_CTX_B_45C68D_8(&c->st, 0) = 0;
            num = node[cursor + 1U];
        }
        quot = (num << 14) / (unsigned int)D4V_TBL_45AFB8_32(&c->st, 0);
        quot = *(unsigned short *)(void *)d4v_fx(c, 96616UL + (quot & U32MASK) * 2U);
        D4V_TBL_45AF88_32(&c->st, 0) =
            (D4V_TBL_45AF88_32(&c->st, 0) + (quot & 0xFFFFU)) & U32MASK;
    }
    flag = ((unsigned int)d4v_t43409C[node[0]]
        < (unsigned int)D4V_TBL_45AF9C_32(&c->st, 0)) ? 1U : 0U;
    if (cursor < 0x0DU) {
        unsigned int top;
        unsigned int grown;
        top = node[0x1BU] & 0xC0U;
        grown = ((unsigned int)node[cursor + 1U] + 1U) & U32MASK;
        if (grown >= (unsigned int)d4v_sar(node[1], 1))
            top = d4v_sar(top, 1) & 0xC0U;
        else {
            top = (top + 0x40U) & 0xFFU;
            if (top == 0U)
                flag = 1U;
        }
        node[0x1BU] = (unsigned char)((node[0x1BU] & 0x3FU) | top);
    }
    if (flag != 0U) {
        int k;
        for (k = 0x0D; k >= 0; k--)
            node[k] = (unsigned char)(((unsigned int)node[k] + 1U) >> 1);
    }
    if (cursor < 0x0DU) {
        unsigned int low;
        node[cursor + 1U] = (unsigned char)(node[cursor + 1U] + 2U);
        low = node[0x1BU] & 0x3FU;
        if (low < 0x3FU)
            node[0x1BU] = (unsigned char)(node[0x1BU] + 1U);
        D4V_TBL_45AFA8_32(&c->st, 0) =
            (D4V_TBL_45AFA8_32(&c->st, 0) + 5U) & U32MASK;
    } else {
        unsigned int bump;
        unsigned int low;
        unsigned int high;
        bump = 2U - ((D4V_CTX_B_45C693_8(&c->st, 0) != 0) ? 1U : 0U);
        node[0] = (unsigned char)(node[0] + bump);
        cursor = 1U;
        while (node[cursor + 1U] != 0) {
            cursor = (cursor + 1U) & 0xFFU;
            if (cursor == 0x0CU)
                break;
        }
        if (node[cursor + 1U] < 6U) {
            node[cursor + 1U] =
                (unsigned char)(2U + ((D4V_CTX_B_45C693_8(&c->st, 0) == 2) ? 1U : 0U));
            node[cursor + 0x0EU] = (unsigned char)symbol;
        }
        low = node[0x1BU] & 0x3FU;
        high = node[0x1BU] & 0xC0U;
        node[0x1BU] = (unsigned char)(high + (unsigned int)d4v_sar(low, 1));
    }
    while (cursor != 0U) {
        unsigned int cur;
        unsigned int prev;
        unsigned int sym_cur;
        unsigned int sym_prev;
        if (node[cursor + 1U] < node[cursor])
            break;
        cur = node[cursor + 1U];
        prev = node[cursor];
        sym_cur = node[cursor + 0x0EU];
        sym_prev = node[cursor + 0x0DU];
        node[cursor] = (unsigned char)cur;
        node[cursor + 0x0DU] = (unsigned char)sym_cur;
        node[cursor + 1U] = (unsigned char)prev;
        node[cursor + 0x0EU] = (unsigned char)sym_prev;
        cursor = (cursor - 1U) & 0xFFU;
    }
    idx = (D4V_TBL_45AFD0_32(&c->st, 0) + 2U) & U32MASK;
    D4V_CTX_B_45C692_8(&c->st, 0) = (unsigned char)cursor;
    if (idx > 0x800U) {
        D4V_TBL_45AFD0_32(&c->st, 0) = idx >> 1;
        D4V_TBL_45AFA8_32(&c->st, 0) = D4V_TBL_45AFA8_32(&c->st, 0) >> 1;
        D4V_TBL_45BA24_32(&c->st, 0) = D4V_TBL_45BA24_32(&c->st, 0) >> 1;
    } else
        D4V_TBL_45AFD0_32(&c->st, 0) = idx;
}


/* sub_417570 (UHARC04.lst:34163-34377).
 * Role: PPM suffix/context builder over the 0x14-byte entry at
 * dword_433B48 + dword_45AFBC*20 (5 dwords + count/symbol lanes):
 * linear-search the symbol slot (byte_45C694 cursor, 0xFF = unknown),
 * price one context via word_437968, halve the 9 count bytes on
 * escape, insert/sort the new symbol, and account
 * dword_45AFD0/dword_45AFA8/dword_45BA24 (halved past 0x800).
 * Entry-live regs (first use): t0 (only al saved to ch at 0x41757C).
 * Frame: no locals; pushes t1/t2/t3/t4/t5/t6 only.
 * Deviations: div-by-zero: direct `/` with comment (0x4175EE site);
 * x86 div would also #DE on quotient overflow, model-kept in range.
 */
void d4vc_ppm_list_update_20(dec4v_ctx *c, unsigned int sym)
{
    unsigned int cursor;
    unsigned int symbol;
    unsigned int idx;
    unsigned int addr;
    unsigned int quot;
    unsigned int num;
    unsigned char *ent;

    cursor = (unsigned int)D4V_CTX_B_45C694_8(&c->st, 0);
    symbol = sym & 0xFFU;
    idx = (unsigned int)D4V_TBL_45AFBC_32(&c->st, 0);
    addr = (D4V_BLK_PTR_02_32(&c->st, 0) + ((((idx << 2) + idx) << 2) & U32MASK)) & U32MASK;
    ent = c->blk02 + (addr - c->blk02_base);
    if (ent[0] == 0) {
        d4v_fill(c, 0x14U, addr, 0);
        d4v_st16(c, addr, 0x0303U);
        ent[0x12] = 6;
        ent[9] = (unsigned char)symbol;
        return;
    }
    if (cursor == 0xFFU) {
        cursor = 0;
        while (cursor < 8U && symbol != ent[cursor + 9])
            cursor++;
    }
    if (D4V_CTX_B_45C687_8(&c->st, 0) != 0) {
        if (cursor >= 8U)
            num = ent[0];
        else {
            D4V_CTX_B_45C685_8(&c->st, 0) = 0;
            D4V_CTX_B_45C68D_8(&c->st, 0) = 0;
            num = ent[cursor + 1];
        }
        quot = (num << 14) / (unsigned int)D4V_TBL_45AFC8_32(&c->st, 0);
        quot = *(unsigned short *)(void *)d4v_fx(c, 96616UL + (quot & U32MASK) * 2U);
        D4V_TBL_45AF88_32(&c->st, 0) =
            (D4V_TBL_45AF88_32(&c->st, 0) + (quot & 0xFFFFU)) & U32MASK;
    }
    if (ent[0x12] > d4v_t43419C[ent[0]]) {
        unsigned int acc;
        int k;
        acc = 0;
        ent[0x12] = 0;
        for (k = 8; k >= 0; k--) {
            unsigned int halved;
            halved = ((unsigned int)ent[k] + 1U) >> 1;
            ent[k] = (unsigned char)halved;
            acc = (acc + halved) & 0xFFU;
            ent[0x12] = (unsigned char)acc;
        }
    }
    ent[0x12] = (unsigned char)(ent[0x12] + 4U);
    if (cursor < 8U) {
        unsigned int cc;
        cc = (unsigned int)D4V_TBL_45AFCC_32(&c->st, 0);
        D4V_TBL_45AFCC_32(&c->st, 0) = ((cc << 2) - cc) >> 2;
        ent[0x11] = (unsigned char)d4v_sar((unsigned int)ent[0x11], 1);
        ent[cursor + 1] = (unsigned char)(ent[cursor + 1] + 4U);
        D4V_TBL_45AFA8_32(&c->st, 0) =
            (D4V_TBL_45AFA8_32(&c->st, 0) + 5U) & U32MASK;
    } else {
        cursor = 1;
        D4V_TBL_45AFCC_32(&c->st, 0) =
            (D4V_TBL_45AFCC_32(&c->st, 0) + 1U) & U32MASK;
        ent[0x11] = (unsigned char)(ent[0x11] + 1U);
        ent[0] = (unsigned char)(ent[0] + 4U);
        while (ent[cursor + 1] != 0) {
            cursor = (cursor + 1U) & 0xFFU;
            if (cursor == 7U)
                break;
        }
        if (ent[cursor + 1] < 0x0CU) {
            ent[0x12] = (unsigned char)(ent[0x12] + (4U - ent[cursor + 1]));
            ent[cursor + 1] = 4;
            ent[cursor + 9] = (unsigned char)symbol;
        }
    }
    while (cursor != 0) {
        unsigned int cur;
        unsigned int prev;
        unsigned int sym_cur;
        unsigned int sym_prev;
        if (ent[cursor + 1] < ent[cursor])
            break;
        cur = ent[cursor + 1];
        prev = ent[cursor];
        sym_cur = ent[cursor + 9];
        sym_prev = ent[cursor + 8];
        ent[cursor] = (unsigned char)cur;
        ent[cursor + 8] = (unsigned char)sym_cur;
        ent[cursor + 1] = (unsigned char)prev;
        ent[cursor + 9] = (unsigned char)sym_prev;
        cursor = (cursor - 1U) & 0xFFU;
    }
    idx = (D4V_TBL_45AFD0_32(&c->st, 0) + 2U) & U32MASK;
    D4V_CTX_B_45C694_8(&c->st, 0) = (unsigned char)cursor;
    if (idx > 0x800U) {
        D4V_TBL_45AFD0_32(&c->st, 0) = idx >> 1;
        D4V_TBL_45AFA8_32(&c->st, 0) = D4V_TBL_45AFA8_32(&c->st, 0) >> 1;
        D4V_TBL_45BA24_32(&c->st, 0) = D4V_TBL_45BA24_32(&c->st, 0) >> 1;
    } else
        D4V_TBL_45AFD0_32(&c->st, 0) = idx;
}


/* sub_41CF60 (UHARC04.lst:41992-42443).
 * Role: PPM rescale/escape worker: clamp the window (t6 =
 * dword_45AFB4 - dword_45BA28, capped at 0x3200), snapshot 20 bytes of
 * the model, score candidates 0..4 via sub_41D540 (or sub_41DBC0 for
 * windows over 0x100), pick the class in var_24, then range-decode
 * the stage-1 item and the 10-way stage-2 symbol like sub_418590
 * (via sub_4221F0 output bytes), refresh byte_45C67D + the
 * dword_4342C4/43429C/byte_434314 tables, and return
 * dword_45BA28 + t6 in t0.
 * Entry-live regs (first use): t0 (only al saved at 0x41CF69).
 * Frame: 0x2C bytes locals (var_1C/var_24/var_28/var_20 bytes,
 * var_2C/var_30 dwords, var_44 = 5-dword score array) over pushes
 * t1/t2/t3/t4/t5/t6 (esp+44h addressing).
 * Deviations: div-by-zero: direct `/` with comment (0x41D091,
 * 0x41D09A, 0x41D19F, 0x41D268 sites); x86 div would also #DE on
 * quotient overflow, model-kept in range. The 0x41CFDB rep-movsb/
 * movsd sequence is folded to a 20-byte loop (see UNCERTAIN).
 */
static void d4v_rc_shift_out(dec4v_ctx *c, unsigned int range,
    unsigned int code_lo, unsigned int code_hi, unsigned int carry)
{
    while (range < 0x1000000UL) {
        if (code_hi != 0U || code_lo < 0xFF000000UL) {
            unsigned int fill;
            d4v_put_byte(c, (D4V_RC_PENDING_B_32(&c->st, 0) + code_hi) & 0xFFU);
            if (carry != 0U) {
                fill = (code_hi + 0xFFU) & 0xFFU;
                do {
                    d4v_put_byte(c, fill);
                    carry--;
                } while (carry != 0U);
            }
            carry--;
            D4V_RC_PENDING_B_32(&c->st, 0) = code_lo >> 24;
        }
        range = (range << 8) & U32MASK;
        carry = (carry + 1U) & U32MASK;
        code_lo = (code_lo << 8) & U32MASK;
        code_hi = 0;
    }
    D4V_RC_RANGE_32(&c->st, 0) = range;
    D4V_RC_CODE_LO_32(&c->st, 0) = code_lo;
    D4V_RC_CODE_HI_32(&c->st, 0) = code_hi;
    D4V_RC_CARRY_CNT_32(&c->st, 0) = carry;
}

static void d4v_rc_apply_counts(dec4v_ctx *c)
{
    unsigned int total;
    unsigned int mid;
    unsigned int hi;
    unsigned int range;
    unsigned int width;
    unsigned int scaled;
    unsigned int code_lo;
    unsigned int code_hi;
    unsigned int carry;

    total = d4v_slot32(c, 0u);
    mid = d4v_slot32(c, 4u);
    hi = d4v_slot32(c, 8u);
    range = D4V_RC_RANGE_32(&c->st, 0);
    width = (hi - mid) & U32MASK;
    range = range / total;
    scaled = (mid * range) & U32MASK;
    range = (range * width) & U32MASK;
    code_lo = D4V_RC_CODE_LO_32(&c->st, 0);
    code_hi = D4V_RC_CODE_HI_32(&c->st, 0);
    {
        unsigned int sum;
        sum = (scaled + code_lo) & U32MASK;
        code_hi = (code_hi + ((sum < scaled) ? 1U : 0U)) & U32MASK;
        code_lo = sum;
    }
    carry = D4V_RC_CARRY_CNT_32(&c->st, 0);
    d4v_rc_shift_out(c, range, code_lo, code_hi, carry);
}

int d4vc_ppm_window_decode(dec4v_ctx *c, unsigned int a)
{
    unsigned int symbol;
    unsigned int win;
    unsigned int initial;
    unsigned int cls;
    unsigned int stream;
    unsigned int scores[5];
    unsigned int k;
    unsigned int best;
    unsigned int accepted;
    unsigned int i;
    unsigned char *e;
    unsigned short ws;

    symbol = a & 0xFFU;
    initial = d4v_t43431E[(unsigned int)D4V_CTX_B_45C67D_8(&c->st, 0)];
    cls = initial;
    win = (D4V_TBL_45AFB4_32(&c->st, 0) - D4V_TBL_45BA28_32(&c->st, 0)) & U32MASK;
    if (win >= 0x4B00UL)
        win = 0x3200U;
    if (D4V_TBL_436152_8(&c->st, 0) != 0
        && D4V_CTX_B_45C67F_8(&c->st, 0) != 0
        && D4V_TBL_45BA28_32(&c->st, 0) < D4V_TBL_45BA44_32(&c->st, 0)) {
        unsigned int quota;
        unsigned int gap;
        unsigned int scale;
        unsigned int bias;
        if (win > 0x100U)
            d4vc_ensure_input(c, (win - 0x100U) & U32MASK);
        for (i = 0; i < 0x14U; i++)
            D4V_TBL_45AF00_8(&c->st, i) = (uint32_t)D4V_TBL_45AF14_8(&c->st, i);
        stream = (D4V_IO_IN_PTR_32(&c->st, 0) + D4V_IO_IN_POS_32(&c->st, 0)) & U32MASK;
        if (win > 0x100U)
            win = d4vc_pick_model_order(c, stream, (win - 0x100U) & U32MASK);
        scores[cls] = d4vc_s4_score_candidate(c, stream, win, cls, 0xFFFFFFFFUL);
        for (k = 0; k <= 4U; k++) {
            unsigned int got;
            if (k == initial)
                continue;
            got = d4vc_s4_score_candidate(c, stream, win, k, scores[cls]);
            scores[k] = got;
            if (got < scores[cls])
                cls = k;
        }
        quota = (scores[cls] / win) / 0x249U;
        if (quota >= 0x0AU)
            quota = 0x0AU;
        gap = (0x0AU - quota) & U32MASK;
        scale = (gap * 4U) & U32MASK;
        bias = (cls == 1U) ? 0x50U : 0x4BU;
        D4V_TBL_45AF30_32(&c->st, 0) = win;
        D4V_TBL_45AF28_32(&c->st, 0) = 0;
        scale = (bias - scale) & U32MASK;
        accepted = 0;
        if (cls != 0U && cls != 3U
            && (scale * (unsigned int)D4V_TBL_45AF00_32(&c->st, cls * 4U))
                > ((unsigned int)D4V_TBL_45AF00_32(&c->st, 0) << 6)) {
            accepted = d4vc_score_adaptive_order(c, stream, win, scores[cls], cls) & 0xFFU;
        } else {
            unsigned int span;
            unsigned int add;
            unsigned int thresh;
            int run_thresh;
            best = 1U;
            if (D4V_TBL_45AF08_32(&c->st, 0) > D4V_TBL_45AF04_32(&c->st, 0))
                best = 2U;
            if (D4V_TBL_45AF0C_32(&c->st, 0)
                > (unsigned int)D4V_TBL_45AF00_32(&c->st, best * 4U))
                best = 3U;
            run_thresh = 1;
            if (D4V_TBL_45AF10_32(&c->st, 0)
                > (unsigned int)D4V_TBL_45AF00_32(&c->st, best * 4U))
                best = 4U;
            else if (best == 3U)
                run_thresh = 0;
            if (run_thresh) {
                span = ((0x0AU - quota) * 4U) & U32MASK;
                add = (win >= 0x800U) ? 0x50U : 0x41U;
                thresh = ((span + add) * (unsigned int)D4V_TBL_45AF00_32(&c->st, 0)) & U32MASK;
                if (((unsigned int)D4V_TBL_45AF00_32(&c->st, best * 4U) << 6) > thresh)
                    accepted = d4vc_score_adaptive_order(c, stream, win,
                        scores[cls], best) & 0xFFU;
            }
        }
        if (accepted == 0U) {
            D4V_TBL_45AF28_32(&c->st, 0) = 0;
            D4V_TBL_45AF30_32(&c->st, 0) = 0;
        } else {
            cls = accepted;
            D4V_TBL_45AF30_32(&c->st, 0) = win;
            D4V_TBL_45AF28_32(&c->st, 0) = 0;
        }
    }
    if (symbol != 0xFFU && cls != (unsigned int)D4V_CTX_B_45C67D_8(&c->st, 0)) {
        e = c->st.g.bytes + ((D4V_TBL_459BA8 + symbol * 8U) - D4V_GLOBALS_BASE);
        ws = 0; memcpy(&ws, e + 6, 2);
        D4V_RSLOT_LO_32(&c->st, 0) = ws;
        ws = 0; memcpy(&ws, e, 2);
        D4V_RSLOT_MID_32(&c->st, 0) = ws;
        ws = 0; memcpy(&ws, e + 2, 2);
        D4V_RSLOT_HI_32(&c->st, 0) = ws;
        d4v_rc_apply_counts(c);
    }
    if (!(symbol != 0xFFU && cls == (unsigned int)D4V_CTX_B_45C67D_8(&c->st, 0))) {
        unsigned int decoded;
        decoded = cls & 0xFFU;
        D4V_RSLOT_MID_32(&c->st, 0) = decoded;
        D4V_RSLOT_HI_32(&c->st, 0) = (decoded + 1U) & U32MASK;
        D4V_RSLOT_LO_32(&c->st, 0) = 0x0AU;
        d4v_rc_apply_counts(c);
        if (decoded != (unsigned int)D4V_CTX_B_45C67D_8(&c->st, 0)) {
            D4V_CTX_B_45C67D_8(&c->st, 0) = (unsigned char)decoded;
            D4V_TBL_45AF3C_32(&c->st, 0) = d4v_t4342C4[decoded];
            D4V_TBL_45AF40_32(&c->st, 0) = d4v_t43429C[decoded];
            D4V_CTX_B_45C67A_8(&c->st, 0) = d4v_t434314[decoded];
            D4V_CTX_B_45C688_8(&c->st, 0) =
                (unsigned char)(decoded == 5U || decoded == 6U);
            D4V_TBL_45AF48_32(&c->st, 0) = 0;
            D4V_CTX_B_45C67B_8(&c->st, 0) = 0;
            D4V_CTX_B_45C679_8(&c->st, 0) = 0;
        }
    }
    win = (D4V_TBL_45BA28_32(&c->st, 0) + win) & U32MASK;
    D4V_TBL_45AF4C_32(&c->st, 0) = win;
    return (int)win;
}


/* APPEND */

/* ---- workers: S4b leaves (from draft_s4b.c) ---- */
void d4vc_s4_pick_predictor(dec4v_ctx *c)
{
    unsigned int t4 = 0;
    unsigned int t5 = 0;
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t6 = 0;
    unsigned char *m;
    unsigned char cls;
    t4 = D4V_TBL_45AF28_32(&c->st, 0);
    t5 = D4V_TBL_45AF48_32(&c->st, 0);
    m = 0;
    if (D4V_CTX_B_45C67E_8(&c->st, 0) == 0
        && t4 < D4V_TBL_45AF30_32(&c->st, 0)) {
        unsigned int base;
        unsigned char *rec;
        t3 = t4 * 8U;
        t0 = D4V_TBL_45AF34_32(&c->st, 0);
        t4++;
        base = t0 + t3;
        rec = d4v_at(c, (uint32_t)base);
        if (rec) {
            t1 = d4v_gld32(rec);
            t2 = rec[4];
            t3 = rec[5];
        } else {
            t1 = d4v_ld32(c, (uint32_t)base);
            t2 = (unsigned int)(*(unsigned char *)d4v_ptr(c, (uint32_t)(base + 4)));
            t3 = (unsigned int)(*(unsigned char *)d4v_ptr(c, (uint32_t)(base + 5)));
        }
        D4V_CTX_B_45C67B_8(&c->st, 0) = (unsigned char)t2;
        D4V_CTX_B_45C679_8(&c->st, 0) = (unsigned char)t3;
    } else {
        if (D4V_CTX_B_45C67D_8(&c->st, 0) < 7) {
            t3 = D4V_TBL_45192C_32(&c->st, 0);
            t1 = 0;
            t6 = D4V_TBL_451924_32(&c->st, 0);
            D4V_TBL_451D3C_32(&c->st, 0) = t1;
            D4V_TBL_451D40_32(&c->st, 0) = t3;
            t0 = t3 + t3;
            t1 = D4V_TBL_45AF38_32(&c->st, 0);
            t3 = D4V_TBL_451930_32(&c->st, 0);
            t1 -= t6;
            t0 -= t3;
            t1 &= 0x7FU;
            D4V_TBL_451D44_32(&c->st, 0) = t0;
            t3 = (uint32_t)D4V_TBL_451B3C_32(&c->st, t1 * 4);
            t0 = D4V_TBL_453574_32(&c->st, 0);
            D4V_TBL_45357C_32(&c->st, 0) = t3;
            D4V_TBL_453578_32(&c->st, 0) = t0;
            t0 = D4V_TBL_453570_32(&c->st, 0);
            t3 = D4V_TBL_453598_32(&c->st, 0);
            D4V_TBL_453574_32(&c->st, 0) = t0;
            t3 -= D4V_TBL_45356C_32(&c->st, 0);
            t0 = D4V_TBL_453580_32(&c->st, 0);
            D4V_TBL_453570_32(&c->st, 0) = t3;
            t3 = D4V_TBL_453598_32(&c->st, 0);
            t0 *= t3;
            D4V_TBL_45356C_32(&c->st, 0) = t3;
            t3 = D4V_TBL_453594_32(&c->st, 0);
            t3 <<= 4;
            t6 = D4V_TBL_453570_32(&c->st, 0);
            t0 += t3;
            t3 = D4V_TBL_453584_32(&c->st, 0);
            t3 *= t6;
            t6 = D4V_TBL_453574_32(&c->st, 0);
            t0 += t3;
            t3 = D4V_TBL_453588_32(&c->st, 0);
            t3 *= t6;
            t6 = D4V_TBL_453578_32(&c->st, 0);
            t0 += t3;
            t3 = D4V_TBL_45358C_32(&c->st, 0);
            t3 *= t6;
            t6 = D4V_TBL_45357C_32(&c->st, 0);
            t0 += t3;
            t3 = D4V_TBL_453590_32(&c->st, 0);
            t3 *= t6;
            t3 += t0;
            {
                unsigned int cf = 0;
                t0 = t3; /* 0x41E739a mov t0,t3 */
                t3 = (unsigned int)((int)t3 >> 31); /* 0x41E739b sar t3,1Fh */
                cf = t3 & 1U; /* CF out of shl t3,4 (0 or 1) */
                t3 <<= 4;
                t0 = t0 - t3 - cf;
                t0 = (unsigned int)((int)t0 >> 4);
            }
            D4V_TBL_451D48_32(&c->st, 0) = t0;
            t0 = (uint32_t)D4V_TBL_451B3C_32(&c->st, t1 * 4);
            t3 = D4V_TBL_451928_32(&c->st, 0);
            D4V_TBL_451D4C_32(&c->st, 0) = t0;
            t0 = D4V_TBL_45AF38_32(&c->st, 0);
            t0 -= t3;
            t0 &= 0x7FU;
            t0 = (uint32_t)D4V_TBL_451B3C_32(&c->st, t0 * 4);
            D4V_TBL_451D50_32(&c->st, 0) = t0;
            t0 = D4V_TBL_451920_32(&c->st, 0);
            t2 = D4V_TBL_45356C;
            m = c->st.g.bytes + (t2 - D4V_GLOBALS_BASE);
            t0 = (unsigned int)(uint32_t)D4V_TBL_451D3C_8(&c->st, t0 * 4UL);
            t1 = D4V_TBL_451920_32(&c->st, 0);
            D4V_CTX_B_45C679_8(&c->st, 0) = (unsigned char)t0;
            if (t1 >= 4) {
                t3 = (unsigned int)D4V_TBL_45192C_8(&c->st, 0);
                t0 = (t0 + t3) & 0xFFU;
                D4V_CTX_B_45C679_8(&c->st, 0) = (unsigned char)t0;
            }
            /* loc_41E79F */
            if (D4V_CTX_B_45C67D_8(&c->st, 0) != 5) {
                t1 = 0x2E7U; /* loc_41E81A */
            } else {
                t3 = (unsigned int)D4V_CTX_B_45C679_8(&c->st, 0);
                t3 = (t3 + 0x80U) & 0xFFU;
                t1 = 0x2E3U;
                D4V_CTX_B_45C679_8(&c->st, 0) = (unsigned char)t3;
            }
            if (d4v_gld32(m + 0x70) == 0)
                t1++;
            else if (d4v_gld32(m + 0x74) == 0)
                t1 += 2U;
            else if (d4v_gld32(m + 0x78) == 0)
                t1 += 3U;
        } else {
            t2 = t5 * 4U;
            t3 = t2;
            t2 <<= 5;
            t2 -= t3;
            t2 += D4V_TBL_45356C;
            m = 0;
            if (t2 >= D4V_GLOBALS_BASE && t2 <= D4V_GLOBALS_END - 0x7Cu)
                m = c->st.g.bytes + (t2 - D4V_GLOBALS_BASE);
            t3 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 4));
            t0 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 8));
            d4v_mst32(c, m, t2, (uint32_t)(t2 + 8), t3);
            t1 = d4v_mld32(c, m, t2, (uint32_t)t2);
            t3 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x2C));
            d4v_mst32(c, m, t2, (uint32_t)(t2 + 0x0C), t0);
            t3 -= t1;
            d4v_mst32(c, m, t2, (uint32_t)(t2 + 4), t3);
            t0 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x2C));
            t3 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x14));
            t3 *= t0;
            d4v_mst32(c, m, t2, (uint32_t)t2, t0);
            t0 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x28));
            t0 <<= 4;
            t1 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 4));
            t0 += t3;
            t3 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x18));
            t3 *= t1;
            t6 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 8));
            t0 += t3;
            t3 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x1C));
            t3 *= t6;
            t1 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x0C));
            t0 += t3;
            t3 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x20));
            t3 *= t1;
            t6 = D4V_TBL_451934_32(&c->st, 0);
            t0 += t3;
            t3 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x24));
            t3 *= t6;
            t3 += t0;
            {
                unsigned int cf = 0;
                t0 = t3;
                t3 = (unsigned int)((int)t3 >> 31);
                cf = t3 & 1U; /* CF out of shl t3,4 */
                t3 <<= 4;
                t0 = t0 - t3 - cf;
                t0 = (unsigned int)((int)t0 >> 4);
            }
            D4V_TBL_451D3C_32(&c->st, 0) = t0;
            t3 = d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x5C));
            if (t3 <= d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x60))) {
                t3 = (unsigned int)D4V_TBL_451D3C_8(&c->st, 0); /* loc_41E901: low byte of dword just stored */
            } else {
                t3 = (unsigned int)(m ? m[0x28] : (*(unsigned char *)d4v_ptr(c, (uint32_t)(t2 + 0x28))));
            }
            /* loc_41E8C0 */
            D4V_CTX_B_45C679_8(&c->st, 0) = (unsigned char)t3;
            cls = D4V_CTX_B_45C67D_8(&c->st, 0);
            if (cls == 7) {
                t1 = (unsigned int)D4V_CTX_B_45C679_8(&c->st, 0);
                D4V_CTX_B_45C679_8(&c->st, 0) = (unsigned char)((t1 + 0x80U) & 0xFFU);
                t1 = (t5 + 0x2EBU) & U32MASK;
                if (d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x70U)) == 0)
                    t1 += 2U;
                else if (d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x74U)) == 0)
                    t1 += 4U;
                else if (d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x78U)) == 0)
                    t1 += 6U;
            } else if (cls == 8) {
                if (d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x70U)) != 0)
                    t1 = t5;
                else
                    t1 = 2U;
                t1 = (t1 + 0x2F3U) & U32MASK;
            } else {
                if (d4v_mld32(c, m, t2, (uint32_t)(t2 + 0x70U)) == 0)
                    t1 = 4U;
                else
                    t1 = t5;
                t1 = (t1 + 0x2F6U) & U32MASK;
            }
            D4V_CTX_B_45C67B_8(&c->st, 0) =
                (unsigned char)(*(unsigned char *)(void *)(d4v_fx(c, 210780UL + t5)));
        }
        if (t4 < D4V_TBL_45AF30_32(&c->st, 0)) {
            unsigned char *rec;
            t3 = D4V_TBL_45AF34_32(&c->st, 0);
            t0 = t4 * 8U;
            t3 += t0;
            t2 = (unsigned int)D4V_CTX_B_45C67B_8(&c->st, 0);
            rec = d4v_at(c, (uint32_t)t3);
            if (rec) {
                d4v_gst32(rec, t1);
                rec[4] = (unsigned char)t2;
                t2 = (unsigned int)D4V_CTX_B_45C679_8(&c->st, 0);
                t4++;
                rec[5] = (unsigned char)t2;
            } else {
                d4v_st32(c, (uint32_t)t3, t1);
                (*(unsigned char *)d4v_ptr(c, (uint32_t)(t3 + 4))) = (unsigned char)t2;
                t2 = (unsigned int)D4V_CTX_B_45C679_8(&c->st, 0);
                t4++;
                (*(unsigned char *)d4v_ptr(c, (uint32_t)(t3 + 5))) = (unsigned char)t2;
            }
        }
    }
    D4V_TBL_45AF48_32(&c->st, 0) = t5;
    D4V_TBL_45AF28_32(&c->st, 0) = t4;
    D4V_TBL_45AF3C_32(&c->st, 0) = t1;
}


/* ---- sub_41E9F0 (lst 44378..45083, 706 lines) ----
 * Role hypothesis: predictor/model updater called per decoded symbol.
 * Two variants share one shape: order<=6 path (fixed 0x45356C model
 * base, ACCT-style [t1+30h..58h] accumulators via 44D920 weights,
 * min-rebalance of slots +34h..58h with 10-case switch on
 * [t1+14h..24h]) and order>=7 path (per-entry t3=45356C+AF48*124,
 * extra 435D50 [5Ch/60h] accumulators, zero-generation reset,
 * 451934/38 swap at order 9, order-7 [74h/78h] lane store).
 * Entry-live regs (first-use): al=sym (mov cl,al), dl=arg (stored to
 * var_18). Callers confirm: 0x417C52 (al=cl-sym, dl=old-al) and
 * 0x41E58A (al=bh-sym, dl=bl). High bytes of t2 on entry are garbage
 * (only cl is read before full-width overwrite); t3 callers zero it.
 * Frame: 5 pushes + sub esp,30h (var_44..var_18 locals).
 */
void d4vc_s4_update_model(dec4v_ctx *c, unsigned int sym, unsigned int arg)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int t5 = 0;
    unsigned int t6 = 0;
    unsigned int v20 = 0; /* var_20: signed symbol */
    unsigned int v24 = 0; /* var_24: sym - 45192C */
    unsigned int varg = 0; /* var_18: saved dl byte */
    unsigned int v1c = 0; /* var_1C temp */
    unsigned int v28 = 0; /* var_28 temp */
    unsigned int v2c = 0; /* var_2C temp */
    unsigned int va[6] = {0}; /* var_44,var_40,var_3C,var_38,var_34,var_30 */
    unsigned char *mlo; /* order<7 record at 0x45356C */
    unsigned char *mhi; /* order>=7 record, 124-byte stride */
    unsigned char *row; /* six rows at 0x451D54, stride 0x404 */
    t2 = sym & 0xFFU;
    varg = arg & 0xFFU;
    if (D4V_CTX_B_45C67E_8(&c->st, 0) == 0) {
        t0 = D4V_TBL_45AF28_32(&c->st, 0); /* loc_41EE46 */
        if (t0 < D4V_TBL_45AF30_32(&c->st, 0)) return;
    }
    /* loc_41EA0B */
    t3 = (unsigned int)D4V_CTX_B_45C67D_8(&c->st, 0);
    if (t3 < 7) {
    t1 = D4V_TBL_45356C;
    mlo = c->st.g.bytes + (t1 - D4V_GLOBALS_BASE);
    if (t3 != 6)
        t0 = (t2 & 0xFFU) - 0x80U;
    else
        t0 = (unsigned int)(int)(signed char)(t2 & 0xFFU);
    v20 = t0;
    t0 = v20;
    t6 = D4V_TBL_45192C_32(&c->st, 0);
    t0 -= t6;
    v24 = t0;
    t0 = (unsigned int)D4V_TBL_451D48_8(&c->st, 0);
    t0 = (t0 - (v20 & 0xFFU)) & 0xFFU;
    t0 = (unsigned int)(int)(signed char)(t0 & 0xFFU);
    t0 <<= 4;
    t3 = t0 & 0xFFFU;
    t2 = d4v_gld32(mlo + 0x30);
    t3 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t3 * 4);
    t4 = d4v_gld32(mlo);
    t2 += t3;
    t3 = (t0 - t4) & 0xFFFU;
    d4v_gst32(mlo + 0x30, t2);
    t5 = d4v_gld32(mlo + 0x34);
    t5 += (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t3 * 4);
    t3 = d4v_gld32(mlo);
    t3 += t0;
    d4v_gst32(mlo + 0x34, t5);
    t3 &= 0xFFFU;
    t6 = d4v_gld32(mlo + 0x38);
    t3 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t3 * 4);
    t2 = d4v_gld32(mlo + 4);
    t6 += t3;
    t3 = (t0 - t2) & 0xFFFU;
    d4v_gst32(mlo + 0x38, t6);
    t4 = d4v_gld32(mlo + 0x3C);
    t4 += (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t3 * 4);
    t3 = d4v_gld32(mlo + 4);
    t3 += t0;
    d4v_gst32(mlo + 0x3C, t4);
    t3 &= 0xFFFU;
    t5 = d4v_gld32(mlo + 0x40);
    t3 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t3 * 4);
    t6 = d4v_gld32(mlo + 8);
    t5 += t3;
    t3 = (t0 - t6) & 0xFFFU;
    d4v_gst32(mlo + 0x40, t5);
    t2 = d4v_gld32(mlo + 0x44);
    t2 += (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t3 * 4);
    t3 = d4v_gld32(mlo + 8);
    t3 += t0;
    d4v_gst32(mlo + 0x44, t2);
    t3 &= 0xFFFU;
    t4 = d4v_gld32(mlo + 0x48);
    t3 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t3 * 4);
    t5 = d4v_gld32(mlo + 0x0C);
    t4 += t3;
    t3 = (t0 - t5) & 0xFFFU;
    d4v_gst32(mlo + 0x48, t4);
    t6 = d4v_gld32(mlo + 0x4C);
    t6 += (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t3 * 4);
    t3 = d4v_gld32(mlo + 0x0C);
    t3 += t0;
    d4v_gst32(mlo + 0x4C, t6);
    t3 &= 0xFFFU;
    t2 = d4v_gld32(mlo + 0x50);
    t3 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t3 * 4);
    t4 = d4v_gld32(mlo + 0x10);
    t2 += t3;
    t3 = (t0 - t4) & 0xFFFU;
    d4v_gst32(mlo + 0x50, t2);
    t5 = d4v_gld32(mlo + 0x54);
    t3 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t3 * 4);
    t6 = d4v_gld32(mlo + 0x10);
    t5 += t3;
    t0 += t6;
    d4v_gst32(mlo + 0x54, t5);
    t0 &= 0xFFFU;
    t3 = d4v_gld32(mlo + 0x58);
    t3 += (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t0 * 4);
    t2 = d4v_gld32(mlo + 0x6C);
    d4v_gst32(mlo + 0x58, t3);
    t0 = v20 & 0xFFU;
    t3 = (unsigned int)mlo[0x28];
    t2++;
    t0 = (t0 - (t3 & 0xFFU)) & 0xFFU;
    d4v_gst32(mlo + 0x6C, t2);
    t0 = (unsigned int)(int)(signed char)(t0 & 0xFFU);
    d4v_gst32(mlo + 0x2C, t0);
    t0 = v20;
    t3 = (t3 & ~0xFF00U) | ((unsigned int)mlo[0x6C] << 8);
    d4v_gst32(mlo + 0x28, t0);
    if (((t3 >> 8) & 0xFU) == 0) {
    t2 = 1;
    t0 = t1 + 4;
    t4 = 0;
    t3 = d4v_gld32(mlo + 0x30);
    d4v_gst32(mlo + 0x30, t4);
    for (;;) { /* loc_41EB9B */
        t5 = d4v_gld32(mlo + (t0 - t1) + 0x30);
        if (t3 > t5) {
            t3 = t5;
            t4 = t2;
        }
        /* loc_41EBA6 */
        t0 += 4;
        t2++;
        d4v_gst32(mlo + (t0 - t1) + 0x2C, 0);
        if ((int)t2 < 0x0B) continue;
        break;
    }
    t4--;
    if (t4 <= 9) {
        switch (t4) {
        case 0: /* loc_41EBC3 */
            t6 = d4v_gld32(mlo + 0x14);
            if ((int)t6 <= (int)0xFFFFFFE0U) break;
            t0 = t6 - 1;
            d4v_gst32(mlo + 0x14, t0);
            break;
        case 1: /* loc_41EE6E */
            t4 = d4v_gld32(mlo + 0x14);
            if ((int)t4 >= 0x20) break;
            t5 = t4 + 1;
            d4v_gst32(mlo + 0x14, t5);
            break;
        case 2: /* loc_41EE85 */
            t3 = d4v_gld32(mlo + 0x18);
            if ((int)t3 <= (int)0xFFFFFFE0U) break;
            t2 = t3 - 1;
            d4v_gst32(mlo + 0x18, t2);
            break;
        case 3: /* loc_41EE9C */
            t6 = d4v_gld32(mlo + 0x18);
            if ((int)t6 >= 0x20) break;
            t0 = t6 + 1;
            d4v_gst32(mlo + 0x18, t0);
            break;
        case 4: /* loc_41EEB3 */
            t4 = d4v_gld32(mlo + 0x1C);
            if ((int)t4 <= (int)0xFFFFFFE0U) break;
            t5 = t4 - 1;
            d4v_gst32(mlo + 0x1C, t5);
            break;
        case 5: /* loc_41EECA */
            t3 = d4v_gld32(mlo + 0x1C);
            if ((int)t3 >= 0x20) break;
            t2 = t3 + 1;
            d4v_gst32(mlo + 0x1C, t2);
            break;
        case 6: /* loc_41EEE1 */
            t6 = d4v_gld32(mlo + 0x20);
            if ((int)t6 <= (int)0xFFFFFFE0U) break;
            t0 = t6 - 1;
            d4v_gst32(mlo + 0x20, t0);
            break;
        case 7: /* loc_41EEF8 */
            t4 = d4v_gld32(mlo + 0x20);
            if ((int)t4 >= 0x20) break;
            t5 = t4 + 1;
            d4v_gst32(mlo + 0x20, t5);
            break;
        case 8: /* loc_41EF0F */
            if ((int)d4v_gld32(mlo + 0x20) <= (int)0xFFFFFFE0U) break;
            d4v_gst32(mlo + 0x24, d4v_gld32(mlo + 0x24) - 1);
            break;
        case 9: /* loc_41EF21 */
            if ((int)d4v_gld32(mlo + 0x20) >= 0x20) break;
            d4v_gst32(mlo + 0x24, d4v_gld32(mlo + 0x24) + 1);
            break;
        }
    }
    }
    t0 = D4V_TBL_45AF38_32(&c->st, 0) & 0x7FU;
    t3 = v24;
    D4V_TBL_451B3C_32(&c->st, t0 * 4) = t3;
    t0 = D4V_TBL_451920_32(&c->st, 0);
    t3 = D4V_TBL_45AF38_32(&c->st, 0);
    if (((d4v_t434538[(unsigned int)(t0) & 0xFFU]) & t3) == 0) {
    t5 = 0xFFFFU;
    t4 = 2;
    t3 = 8;
    D4V_TBL_451924_32(&c->st, 0) = t4;
    D4V_TBL_451928_32(&c->st, 0) = t4;
    t0 = t4;
    for (;;) { /* loc_41EC19 */
        t2 = D4V_TBL_45AF38_32(&c->st, 0);
        t4 = t2 - 1;
        t6 = t4 - t0;
        t4 &= 0x7FU;
        t6 &= 0x7FU;
        t4 = (uint32_t)D4V_TBL_451B3C_32(&c->st, t4 * 4);
        t6 <<= 2;
        v1c = t4;
        t4 = (uint32_t)D4V_TBL_451B3C_32(&c->st, t6);
        t6 = v1c;
        t6 -= t4;
        t4 = t6 & 0xFFFU;
        t4 <<= 2;
        v2c = t4;
        t4 = t2 - t0;
        t4 &= 0x7FU;
        t4 <<= 2;
        v28 = t4;
        t4 = t2 & 0x7FU;
        t4 = (uint32_t)D4V_TBL_451B3C_32(&c->st, t4 * 4);
        v1c = t4;
        t4 = v28;
        t6 = v1c;
        t4 = (uint32_t)D4V_TBL_451B3C_32(&c->st, t4);
        t6 -= t4;
        t4 = t6 & 0xFFFU;
        t4 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t4 * 4);
        v1c = t4;
        t4 = v2c;
        t6 = v1c;
        t4 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t4);
        t2 -= 2;
        t6 += t4;
        t4 = t2 - t0;
        t2 &= 0x7FU;
        t4 &= 0x7FU;
        v1c = t6;
        t2 = (uint32_t)D4V_TBL_451B3C_32(&c->st, t2 * 4);
        t2 -= (uint32_t)D4V_TBL_451B3C_32(&c->st, t4 * 4);
        t2 &= 0xFFFU;
        t4 = v1c;
        t6 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t2 * 4);
        t2 = t4 + t6;
        t4 = (unsigned int)((uint32_t)(uint32_t)D4V_TBL_45193C_32(&c->st, t3) * 13UL & 0xFFFFFFFFUL);
        t4 >>= 4;
        t4 += t2;
        D4V_TBL_45193C_32(&c->st, t3) = t4;
        t6 = (uint32_t)D4V_TBL_45193C_32(&c->st, t3);
        t4 = D4V_TBL_451924_32(&c->st, 0);
        if (t6 < (unsigned int)(uint32_t)D4V_TBL_45193C_32(&c->st, t4 * 4))
            D4V_TBL_451924_32(&c->st, 0) = t0;
        /* loc_41ED05 */
        if (t2 < t5) {
            t5 = t2;
            D4V_TBL_451928_32(&c->st, 0) = t0;
        }
        /* loc_41ED10 */
        t0++;
        t3 += 4;
        if (t0 < 0x7CU) continue;
        break;
    }
    }
    t0 = v20;
    t0 -= D4V_TBL_451D3C_32(&c->st, 0);
    t2 = D4V_TBL_451D40_32(&c->st, 0);
    va[0] = t0;
    t0 = v20;
    t0 -= t2;
    t4 = D4V_TBL_451D44_32(&c->st, 0);
    va[1] = t0;
    t0 = v20;
    t0 -= t4;
    t5 = D4V_TBL_451D48_32(&c->st, 0);
    va[2] = t0;
    t0 = v20;
    t0 -= t5;
    t6 = D4V_TBL_451D4C_32(&c->st, 0);
    va[3] = t0;
    t0 = v24;
    t3 = D4V_TBL_451D50_32(&c->st, 0);
    t0 -= t6;
    t2 = 0;
    va[4] = t0;
    t0 = v24;
    D4V_TBL_451920_32(&c->st, 0) = t2;
    t0 -= t3;
    t4 = 0;
    va[5] = t0;
    t3 = 0;
    t0 = D4V_TBL_451D54;
    row = c->st.g.bytes + (t0 - D4V_GLOBALS_BASE);
    for (;;) { /* loc_41ED8B: 6 rows, stride 0x404 */
        t2 = D4V_TBL_45AF38_32(&c->st, 0) & 0xFFU;
        t6 = d4v_gld32(row + 0x400);
        t5 = d4v_gld32(row + t2 * 4);
        t6 -= t5;
        d4v_gst32(row + 0x400, t6);
        t5 = va[t4 >> 2];
        t5 &= 0x1FFU;
        t5 = d4v_t434550[(unsigned int)(t5) & 0x1FFU];
        d4v_gst32(row + t2 * 4, t5);
        t2 = t5;
        d4v_gst32(row + 0x400, d4v_gld32(row + 0x400) + t2);
        t5 = (unsigned int)((uint32_t)D4V_TBL_451920_32(&c->st, 0) * 0x404UL & 0xFFFFFFFFUL);
        t2 = d4v_gld32(row + 0x400);
        if ((int)t2 < (int)(uint32_t)D4V_TBL_452154_32(&c->st, t5))
            D4V_TBL_451920_32(&c->st, 0) = t3;
        t4 += 4;
        t3++;
        t0 += 0x404U;
        row += 0x404U;
        if ((int)t3 < 6) continue;
        break;
    }
    t0 = D4V_TBL_45192C_32(&c->st, 0);
    D4V_TBL_451930_32(&c->st, 0) = t0;
    t0 = v20;
    t3 = d4v_gld32(mlo + 0x70);
    D4V_TBL_45192C_32(&c->st, 0) = t0;
    t0 = 0;
    t3 <<= 8;
    t0 = varg;
    t2 = d4v_gld32(mlo + 0x74);
    t3 += t0;
    t0 = 0;
    d4v_gst32(mlo + 0x70, t3);
    t0 = varg;
    t2 <<= 8;
    t3 = d4v_t434D50[(unsigned int)(t0) & 0xFFU];
    t2 += t3;
    t3 = d4v_gld32(mlo + 0x78);
    d4v_gst32(mlo + 0x74, t2);
    t3 <<= 8;
    t0 = d4v_t435550[(unsigned int)(t0) & 0xFFU];
    t3 += t0;
    d4v_gst32(mlo + 0x78, t3);
    } else {
    t0 = D4V_TBL_45AF48_32(&c->st, 0);
    t0 <<= 2;
    t3 = t0;
    t0 <<= 5;
    t0 -= t3;
    t3 = D4V_TBL_45356C;
    t1 = D4V_TBL_45AF48_32(&c->st, 0);
    t3 += t0;
    t0 = (unsigned int)D4V_CTX_B_45C67B_8(&c->st, 0);
    (*(unsigned char *)(void *)(d4v_fx(c, 210780UL + t1))) = (unsigned char)t0;
    if (D4V_CTX_B_45C67D_8(&c->st, 0) < 8)
        t0 = (t2 & 0xFFU) - 0x80U;
    else
        t0 = (unsigned int)(int)(signed char)(t2 & 0xFFU);
    mhi = 0;
    if (t3 >= D4V_GLOBALS_BASE && t3 <= D4V_GLOBALS_END - 0x7Cu)
        mhi = c->st.g.bytes + (t3 - D4V_GLOBALS_BASE);
    v20 = t0;
    t1 = v20 & 0xFFU;
    t0 = (unsigned int)D4V_TBL_451D3C_8(&c->st, 0);
    t0 = (t0 - (t1 & 0xFFU)) & 0xFFU;
    t0 = (unsigned int)(int)(signed char)(t0 & 0xFFU);
    t0 <<= 4;
    t2 = t0 & 0xFFFU;
    t1 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x30));
    t2 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t2 * 4);
    t4 = d4v_mld32(c, mhi, t3, (uint32_t)t3);
    t1 += t2;
    t2 = (t0 - t4) & 0xFFFU;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x30), t1);
    t5 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x34));
    t5 += (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t2 * 4);
    t2 = d4v_mld32(c, mhi, t3, (uint32_t)t3);
    t2 += t0;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x34), t5);
    t2 &= 0xFFFU;
    t6 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x38));
    t2 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t2 * 4);
    t1 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 4));
    t6 += t2;
    t2 = (t0 - t1) & 0xFFFU;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x38), t6);
    t4 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x3C));
    t4 += (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t2 * 4);
    t2 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 4));
    t2 += t0;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x3C), t4);
    t2 &= 0xFFFU;
    t5 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x40));
    t2 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t2 * 4);
    t6 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 8));
    t5 += t2;
    t2 = (t0 - t6) & 0xFFFU;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x40), t5);
    t1 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x44));
    t1 += (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t2 * 4);
    t2 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 8));
    t2 += t0;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x44), t1);
    t2 &= 0xFFFU;
    t4 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x48));
    t2 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t2 * 4);
    t5 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x0C));
    t4 += t2;
    t2 = (t0 - t5) & 0xFFFU;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x48), t4);
    t6 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x4C));
    t6 += (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t2 * 4);
    t2 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x0C));
    t2 += t0;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x4C), t6);
    t2 &= 0xFFFU;
    t1 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x50));
    t2 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t2 * 4);
    t1 += t2;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x50), t1);
    t2 = t0;
    t4 = D4V_TBL_451934_32(&c->st, 0);
    t2 = (t2 - t4) & 0xFFFU;
    t5 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x54));
    t5 += (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t2 * 4);
    t0 += t4;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x54), t5);
    t0 &= 0xFFFU;
    t2 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x58));
    t0 = (uint32_t)D4V_TBL_NEG_44D920_32(&c->st, t0 * 4);
    t2 += t0;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x58), t2);
    t2 = v20 & 0xFFU;
    t0 = (unsigned int)D4V_TBL_451D3C_8(&c->st, 0);
    t0 = (t0 - (t2 & 0xFFU)) & 0xFFU;
    t0 &= 0xFFU;
    t1 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x5C));
    t1 += d4v_t435D50[(unsigned int)(t0) & 0xFFU];
    t0 = (unsigned int)(mhi ? mhi[0x28] : (*(unsigned char *)d4v_ptr(c, (uint32_t)(t3 + 0x28))));
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x5C), t1);
    t0 = (t0 - (t2 & 0xFFU)) & 0xFFU;
    t0 &= 0xFFU;
    t4 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x60));
    t4 += d4v_t435D50[(unsigned int)(t0) & 0xFFU];
    t0 = t2 & 0xFFU;
    t0 = (t0 - ((unsigned int)(mhi ? mhi[0x28] : (*(unsigned char *)d4v_ptr(c, (uint32_t)(t3 + 0x28)))) & 0xFFU)) & 0xFFU;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x60), t4);
    t0 = (unsigned int)(int)(signed char)(t0 & 0xFFU);
    t5 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x6C));
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x2C), t0);
    t5++;
    D4V_TBL_451934_32(&c->st, 0) = t0;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x6C), t5);
    t0 = v20;
    t1 = (unsigned int)(mhi ? mhi[0x6C] : (*(unsigned char *)d4v_ptr(c, (uint32_t)(t3 + 0x6C))));
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x28), t0);
    if ((t1 & 0xFU) == 0) {
    t2 = 1;
    t0 = t3 + 4;
    t4 = 0;
    t1 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x30));
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x30), t4);
    for (;;) { /* loc_41F101 */
        t5 = d4v_mld32(c, mhi, t3, (uint32_t)(t0 + 0x30));
        if (t1 > t5) {
            t1 = t5;
            t4 = t2;
        }
        /* loc_41F10C */
        t0 += 4;
        t2++;
        d4v_mst32(c, mhi, t3, (uint32_t)(t0 + 0x2C), 0);
        if ((int)t2 < 0x0B) continue;
        break;
    }
    t4--;
    if (t4 <= 9) {
        switch (t4) {
        case 0: /* loc_41F129 */
            t6 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x14));
            if ((int)t6 <= (int)0xFFFFFFE0U) break;
            t0 = t6 - 1;
            d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x14), t0);
            break;
        case 1: /* loc_41F1D9 */
            t4 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x14));
            if ((int)t4 >= 0x20) break;
            t5 = t4 + 1;
            d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x14), t5);
            break;
        case 2: /* loc_41F1F0 */
            t2 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x18));
            if ((int)t2 <= (int)0xFFFFFFE0U) break;
            t1 = t2 - 1;
            d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x18), t1);
            break;
        case 3: /* loc_41F207 */
            t6 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x18));
            if ((int)t6 >= 0x20) break;
            t0 = t6 + 1;
            d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x18), t0);
            break;
        case 4: /* loc_41F21E */
            t4 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x1C));
            if ((int)t4 <= (int)0xFFFFFFE0U) break;
            t5 = t4 - 1;
            d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x1C), t5);
            break;
        case 5: /* loc_41F235 */
            t2 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x1C));
            if ((int)t2 >= 0x20) break;
            t1 = t2 + 1;
            d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x1C), t1);
            break;
        case 6: /* loc_41F24C */
            t6 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x20));
            if ((int)t6 <= (int)0xFFFFFFE0U) break;
            t0 = t6 - 1;
            d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x20), t0);
            break;
        case 7: /* loc_41F263 */
            t4 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x20));
            if ((int)t4 >= 0x20) break;
            t5 = t4 + 1;
            d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x20), t5);
            break;
        case 8: /* loc_41F27A */
            if ((int)d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x20)) <= (int)0xFFFFFFE0U) break;
            d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x24), d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x24)) - 1);
            break;
        case 9: /* loc_41F28C */
            if ((int)d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x20)) >= 0x20) break;
            d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x24), d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x24)) + 1);
            break;
        }
    }
    /* loc_41F137 (switch default: zero-generation reset) */
    if (((unsigned int)(mhi ? mhi[0x6C] : (*(unsigned char *)d4v_ptr(c, (uint32_t)(t3 + 0x6C)))) & 0xFFU) == 0) {
    t0 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x64));
    t2 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x5C));
    t2 -= t0;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x5C), t2);
    t1 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x60));
    t0 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x68));
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x64), t2);
    t1 -= t0;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x60), t1);
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x68), t1);
        }
    }
    if (D4V_CTX_B_45C67D_8(&c->st, 0) == 9) {
        t0 = D4V_TBL_451938_32(&c->st, 0);
        t2 = D4V_TBL_451934_32(&c->st, 0);
        D4V_TBL_451934_32(&c->st, 0) = t0;
        D4V_TBL_451938_32(&c->st, 0) = t2;
    }
    t2 = 0;
    t0 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x70));
    t2 = varg;
    t0 <<= 8;
    t0 += t2;
    t2 = (unsigned int)D4V_CTX_B_45C67D_8(&c->st, 0);
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x70), t0);
    if ((t2 & 0xFFU) != 7) return;
    t0 = 0;
    t1 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x74));
    t0 = varg;
    t1 <<= 8;
    t2 = d4v_t435150[(unsigned int)(t0) & 0xFFU];
    t1 += t2;
    t2 = d4v_mld32(c, mhi, t3, (uint32_t)(t3 + 0x78));
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x74), t1);
    t2 <<= 8;
    t0 = d4v_t435950[(unsigned int)(t0) & 0xFFU];
    t2 += t0;
    d4v_mst32(c, mhi, t3, (uint32_t)(t3 + 0x78), t2);
    return; /* : add esp,30h; pops; retn */
    }
}

/* MORE */

/* ---- sub_41D480 (UHARC04.lst:42448-42514) -> d4vc_rescale_s4_pair ----
 * Rescale the model pair: halve 32 dwords at 433B80[a<<9] (packed
 * 16-bit lanes: +0x10001, mask 0xFFFEFFFE, shr 1, cross-sum into t1),
 * then bump the 433B8C[a] word (shr 2, inc) and publish
 * 433B88[a] = (t1 + (word&0xFFFF)) low word. Entry-live: t0 only. */
/* Halve the 512-byte plane a<<9 in blk16, rounding each 16-bit lane up.
 * blk19[a] becomes (old>>2)+1. blk18[a] receives that value plus the
 * sum of the halved dwords, folded from its two halves. */
void d4vc_rescale_s4_pair(dec4v_ctx *c, unsigned int a)
{
    unsigned int cursor;
    unsigned int end;
    unsigned int sum;
    unsigned int lo;
    unsigned int hi;
    unsigned int pair;
    unsigned int total;
    unsigned int folded;
    unsigned char *m;
    unsigned int plane;
    unsigned int at19;
    unsigned int at18;

    plane = (D4V_BLK_PTR_16_32(&c->st, 0) + ((a << 9) & U32MASK)) & U32MASK;
    m = 0;
    if (a < 96u)
        m = c->blk16 + (plane - c->blk16_base);
    cursor = plane;
    end = (plane + 0x200U) & U32MASK;
    sum = 0;
    while (cursor != end) {
        lo = d4v_mld32(c, m, plane, (uint32_t)cursor);
        hi = d4v_mld32(c, m, plane, (uint32_t)(cursor + 4));
        lo = (lo + 0x10001U) & U32MASK;
        hi = (hi + 0x10001U) & U32MASK;
        lo &= 0xFFFEFFFEUL;
        hi &= 0xFFFEFFFEUL;
        lo >>= 1;
        hi >>= 1;
        cursor = (cursor + 0x10U) & U32MASK;
        pair = (lo + hi) & U32MASK;
        d4v_mst32(c, m, plane, (uint32_t)(cursor - 0x0CU), hi);
        d4v_mst32(c, m, plane, (uint32_t)(cursor - 0x10U), lo);
        hi = d4v_mld32(c, m, plane, (uint32_t)(cursor - 4));
        lo = d4v_mld32(c, m, plane, (uint32_t)(cursor - 8));
        hi = (hi + 0x10001U) & U32MASK;
        lo = (lo + 0x10001U) & U32MASK;
        hi &= 0xFFFEFFFEUL;
        lo &= 0xFFFEFFFEUL;
        hi >>= 1;
        lo >>= 1;
        sum = (sum + pair) & U32MASK;
        pair = (lo + hi) & U32MASK;
        d4v_mst32(c, m, plane, (uint32_t)(cursor - 4), hi);
        sum = (sum + pair) & U32MASK;
        d4v_mst32(c, m, plane, (uint32_t)(cursor - 8), lo);
    }
    at19 = (D4V_BLK_PTR_19_32(&c->st, 0) + a * 2U) & U32MASK;
    total = d4v_u16_in(c, c->blk19, c->blk19_base, 0xC0u, at19);
    total = ((total >> 2) + 1U) & U32MASK;
    d4v_u16_st_in(c, c->blk19, c->blk19_base, 0xC0u, at19, total & 0xFFFFUL);
    folded = ((sum & 0xFFFFUL) + (sum >> 16) + (total & 0xFFFFUL)) & U32MASK;
    at18 = (D4V_BLK_PTR_18_32(&c->st, 0) + a * 2U) & U32MASK;
    d4v_u16_st_in(c, c->blk18, c->blk18_base, 0xC0u, at18, folded & 0xFFFFUL);
}

/* ---- workers: S4a cluster (from draft_s4a.c) ---- */
unsigned int d4vc_s4_score_candidate(dec4v_ctx *c, unsigned int a, unsigned int d,
    unsigned int e, unsigned int b)
{
    unsigned int t0 = 0, t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0, t6 = 0;
    unsigned int v20 = 0, v24 = 0, v28 = 0, v2c = 0, v30 = 0, v34 = 0, v40 = 0;
    unsigned int v44 = 0, v50 = 0, v54 = 0, v58 = 0, v5c = 0;
    unsigned char v10 = 0, v14 = 0, v18 = 0;
    unsigned char alb = 0, ahb = 0, clb = 0, chb = 0, dlb = 0;
    /* prologue: 0x41D540 push t4; 0x41D541 push t5; 0x41D542 push t6;
     * 0x41D543 sub esp,50h */
    t6 = D4V_TBL_45AF54_32(&c->st, 0);
    v30 = a;
    v58 = d;
    v40 = b;
    v14 = (unsigned char)(e & 0xFFU);
    t3 = 0;
    clb = v14; /* entry cl */
    t3 = clb;
    t2 = d4v_t4342C4[(unsigned int)(t3) & 0xFFU];
    t0 = d4v_t4342EC[(unsigned int)(t3) & 0xFFU];
    v24 = t2;
    v50 = t0;
    t3 <<= 8;
    t0 = D4V_DICT_CURSOR_32(&c->st, 0);
    t2 = 0;
    t3 += 0x30200U;
    v44 = t2;
    v20 = t0;
    alb = v14;
    v28 = t2;
    ahb = D4V_CTX_B_45C67D_8(&c->st, 0);
    v54 = t3;
    if (alb == ahb)
        t1 = D4V_TBL_45AF3C_32(&c->st, 0) - v24;
    else
        t1 = 0;
    t0 = D4V_TBL_45AF38_32(&c->st, 0);
    v2c = t0;
    alb = D4V_CTX_B_45C67B_8(&c->st, 0);
    v18 = alb;
    t0 = 0;
    t0 = v14;
    dlb = v14;
    alb = d4v_t434314[(unsigned int)(t0) & 0xFFU];
    D4V_TBL_45AF54_32(&c->st, 0) = t6;
    v10 = alb;
    if (dlb == 0) {
    /* 0x400 bytes, globals 0x4593A0 -> 0x4597A0 and 0x458798 -> 0x458B98.
     * The two pairs are adjacent and do not overlap. */
    memcpy(c->st.g.bytes + (D4V_TBL_4597A0 - D4V_GLOBALS_BASE),
        c->st.g.bytes + (D4V_TBL_4593A0 - D4V_GLOBALS_BASE), 0x400U);
    memcpy(c->st.g.bytes + (D4V_TBL_458B98 - D4V_GLOBALS_BASE),
        c->st.g.bytes + (D4V_TBL_458798 - D4V_GLOBALS_BASE), 0x400U);
    t5 = 0x201U;
    t6 = D4V_TBL_45AF90_32(&c->st, 0);
    t0 = D4V_TBL_45AF98_32(&c->st, 0);
    D4V_TBL_45AF54_32(&c->st, 0) = t6;
    D4V_TBL_45AF5C_32(&c->st, 0) = t0;
    t0 = D4V_TBL_45AF94_32(&c->st, 0);
    v24 = t5;
    D4V_TBL_45AF58_32(&c->st, 0) = t0;
    }
    t3 = v50;
    t0 = v24;
    t3 <<= 9;
    t0 <<= 9;
    {
        unsigned int n;
        unsigned int off;
        n = t3;
        off = t0;
        /* The movsb/movsd prologue copies n bytes from blk15+off to the
         * start of blk16. Both ranges fit for the orders this path uses. */
        if (off <= 0x61A00u && n <= 0x61A00u - off && n <= 0xC000u) {
            memcpy(c->blk16 + (D4V_BLK_PTR_16_32(&c->st, 0) - c->blk16_base),
                c->blk15 + ((D4V_BLK_PTR_15_32(&c->st, 0) + off) - c->blk15_base),
                n);
        } else {
            t5 = D4V_BLK_PTR_16_32(&c->st, 0);
            t4 = D4V_BLK_PTR_15_32(&c->st, 0);
            t2 = t3;
            t4 += t0;
            t0 = t2;
            t2 -= t5;
            t2 -= t0;
            t2 &= 3U;
            t0 -= t2;
            if ((int)t0 > 0) {
                while (t2) { (*(unsigned char *)d4v_ptr(c, (uint32_t)(t5))) = (*(unsigned char *)d4v_ptr(c, (uint32_t)(t4))); t4++; t5++; t2--; }
                t2 = t0;
                t0 &= 3U;
                t2 >>= 2;
                while (t2) { d4v_st32(c, (uint32_t)(t5), (*(unsigned int *)(void *)d4v_ptr(c, (uint32_t)(t4)))); t4 += 4; t5 += 4; t2--; }
            }
            t2 += t0;
            while (t2) { (*(unsigned char *)d4v_ptr(c, (uint32_t)(t5))) = (*(unsigned char *)d4v_ptr(c, (uint32_t)(t4))); t4++; t5++; t2--; }
        }
    }
    {
        unsigned int k;
        unsigned int idx;
        unsigned int src;
        unsigned int a17;
        unsigned int a14;
        k = v50;
        while (k != 0U) {
            k--;
            idx = k << 1;
            src = (v24 + k) << 9;
            a17 = d4v_u16_in(c, c->blk17, c->blk17_base, 0x5FEu,
                (uint32_t)(D4V_BLK_PTR_17_32(&c->st, 0) + idx + (v24 << 1)));
            d4v_u16_st_in(c, c->blk19, c->blk19_base, 0xC0u,
                (uint32_t)(D4V_BLK_PTR_19_32(&c->st, 0) + idx), a17 & 0xFFFFU);
            a14 = d4v_u16_in(c, c->blk14, c->blk14_base, 0x61A00u,
                (uint32_t)(D4V_BLK_PTR_14_32(&c->st, 0) + src));
            d4v_u16_st_in(c, c->blk18, c->blk18_base, 0xC0u,
                (uint32_t)(D4V_BLK_PTR_18_32(&c->st, 0) + idx), (a17 + a14) & 0xFFFFU);
        }
    }
    v24 = D4V_SHIFT_R0_32(&c->st, 0);
    v5c = v54 << 1;
    v34 = v44 + v30;
    do {
        unsigned int slot;
        unsigned int off;
        unsigned int freq;
        unsigned int pair18;
        unsigned int at16;
        unsigned int at18;
        unsigned int quot;
        unsigned int price;
        if (v44 >= 0x100U)
            chb = (unsigned char)~c->blk00_dict[((uint32_t)(v34 - 0x100U) - c->blk00_base)];
        else
            chb = c->blk00_dict[(D4V_BLK_PTR_00_DICT_32(&c->st, 0) + v20) - c->blk00_base];
        if (v14 == 0) {
            unsigned int gap;
            gap = D4V_TBL_45AF5C_32(&c->st, 0);
            t1 = 0;
            if (gap <= 1U) {
                alb = (unsigned char)(v24 & 0xFFU);
                if ((unsigned char)(v24 >> 8) == alb)
                    t1 = 1;
            } else {
                unsigned int mask;
                unsigned int span;
                mask = D4V_DICT_MASK_32(&c->st, 0);
                if ((int)(v44 - gap) < 0)
                    alb = c->blk00_dict[(D4V_BLK_PTR_00_DICT_32(&c->st, 0) + ((v20 - gap) & mask)) - c->blk00_base];
                else
                    alb = (unsigned char)~c->blk00_dict[(v30 + (v44 - gap)) - c->blk00_base];
                span = gap + gap;
                if ((int)(v44 - span) < 0)
                    ahb = c->blk00_dict[(D4V_BLK_PTR_00_DICT_32(&c->st, 0) + ((v20 - span) & mask)) - c->blk00_base];
                else
                    ahb = (unsigned char)~c->blk00_dict[(v30 + (v44 - span)) - c->blk00_base];
                if (alb == ahb)
                    t1 = 1;
            }
            clb = (unsigned char)(chb - alb);
        } else {
            unsigned int shifted;
            shifted = v24 >> (v10 & 31U);
            if (v18 == 0)
                clb = (unsigned char)(chb - (unsigned char)shifted);
            else
                clb = (unsigned char)((unsigned char)shifted - chb);
        }
        slot = t1;
        off = ((slot << 8) + clb) << 1;
        at16 = D4V_BLK_PTR_16_32(&c->st, 0) + off;
        freq = d4v_u16_in(c, c->blk16, c->blk16_base, 0xC000u, (uint32_t)at16);
        at18 = D4V_BLK_PTR_18_32(&c->st, 0) + (slot << 1);
        pair18 = d4v_u16_in(c, c->blk18, c->blk18_base, 0xC0u, (uint32_t)at18);
        if ((freq & 0xFFFFU) == 0U) {
            unsigned int f15;
            unsigned int tot;
            unsigned int old19;
            unsigned int p19;
            f15 = d4v_u16_in(c, c->blk15, c->blk15_base, 0x61A00u,
                (uint32_t)(D4V_BLK_PTR_15_32(&c->st, 0) + ((v54 + clb) << 1)));
            tot = d4v_u16_in(c, c->blk14, c->blk14_base, 0x61A00u,
                (uint32_t)(D4V_BLK_PTR_14_32(&c->st, 0) + v5c));
            quot = ((f15 & 0xFFFFU) << 14) / (tot & 0xFFFFU);
            price = *(unsigned short *)(void *)(d4v_fx(c, 96616UL + quot * 2U));
            v28 += price & 0xFFFFU;
            p19 = D4V_BLK_PTR_19_32(&c->st, 0) + (slot << 1);
            old19 = d4v_u16_in(c, c->blk19, c->blk19_base, 0xC0u, (uint32_t)p19);
            d4v_u16_st_in(c, c->blk19, c->blk19_base, 0xC0u, (uint32_t)p19,
                (old19 + 0x20U) & 0xFFFFU);
            d4v_u16_st_in(c, c->blk18, c->blk18_base, 0xC0u, (uint32_t)at18,
                (d4v_u16_in(c, c->blk18, c->blk18_base, 0xC0u, (uint32_t)at18) + 0x20U) & 0xFFFFU);
            freq = old19;
        }
        quot = ((freq & 0xFFFFU) << 14) / (pair18 & 0xFFFFU);
        price = *(unsigned short *)(void *)(d4v_fx(c, 96616UL + quot * 2U));
        v28 += price & 0xFFFFU;
        if (v28 > v40)
            break;
        d4v_u16_st_in(c, c->blk16, c->blk16_base, 0xC000u, (uint32_t)at16,
            (d4v_u16_in(c, c->blk16, c->blk16_base, 0xC000u, (uint32_t)at16) + 0x20U) & 0xFFFFU);
        t4 = (d4v_u16_in(c, c->blk18, c->blk18_base, 0xC0u,
            (uint32_t)(D4V_BLK_PTR_18_32(&c->st, 0) + (slot << 1))) + 0x20U) & 0xFFFFU;
        D4V_TBL_45AF54_32(&c->st, 0) = t6;
        d4v_u16_st_in(c, c->blk18, c->blk18_base, 0xC0u,
            (uint32_t)(D4V_BLK_PTR_18_32(&c->st, 0) + (slot << 1)), t4);
        if (t4 >= 0x4000U)
            d4vc_rescale_s4_pair(c, slot);
        t6 = D4V_TBL_45AF54_32(&c->st, 0);
        if (v14 == 0) {
            if (chb != 0 && chb != 0xFFU) {
                unsigned int mask;
                unsigned int dist;
                unsigned int gap;
                mask = D4V_DICT_MASK_32(&c->st, 0);
                dist = (v20 - D4V_TBL_458F98_32(&c->st, (unsigned int)chb * 4U)) & mask;
                if (dist >= 0x100U) {
                    dist = (v20 - D4V_TBL_458F98_32(&c->st,
                        (unsigned int)(unsigned char)(chb - 1) * 4U)) & mask;
                    if (dist >= 0x100U)
                        dist = (v20 - D4V_TBL_458F98_32(&c->st,
                            (unsigned int)(unsigned char)(chb + 1) * 4U)) & mask;
                }
                if (dist != 0U && dist < 0x100U) {
                    D4V_TBL_458B98_32(&c->st, dist * 4U)++;
                    gap = D4V_TBL_45AF5C_32(&c->st, 0);
                    if (dist != gap) {
                        if ((int)(v44 - gap) < 0)
                            dlb = c->blk00_dict[(D4V_BLK_PTR_00_DICT_32(&c->st, 0) + ((v20 - gap) & mask)) - c->blk00_base];
                        else
                            dlb = (unsigned char)~c->blk00_dict[(v30 + (v44 - gap)) - c->blk00_base];
                        if (dlb != chb)
                            t6++;
                        else
                            D4V_TBL_458B98_32(&c->st, gap * 4U)++;
                        if (dist == D4V_TBL_45AF58_32(&c->st, 0)) {
                            if (D4V_TBL_458B98_32(&c->st, D4V_TBL_45AF58_32(&c->st, 0) * 4U)
                                >= D4V_TBL_458B98_32(&c->st, gap * 4U)) {
                                D4V_TBL_45AF58_32(&c->st, 0) = gap;
                                D4V_TBL_45AF5C_32(&c->st, 0) = dist;
                            }
                        } else
                            D4V_TBL_45AF58_32(&c->st, 0) = dist;
                    }
                }
                if (t6 >= 0x40U) {
                    unsigned int i;
                    unsigned int half;
                    D4V_TBL_458B9C_32(&c->st, 0)++;
                    t6 = 0;
                    i = 0;
                    do {
                        half = D4V_TBL_458B98_32(&c->st, i) >> 1;
                        i += 4U;
                        t6 += half;
                        D4V_TBL_458B94_32(&c->st, i) = half;
                    } while (i != 0x400U);
                } else
                    t6++;
                D4V_TBL_458F98_32(&c->st, (unsigned int)chb * 4U) = v20;
            }
        } else {
            if (clb >= 0x80U)
                v18 = (unsigned char)(v18 == 0);
            if (v14 == 1)
                t1 = ((t1 & 0xFU) << 4) + d4v_t434338[clb];
            else if (v14 == 2)
                t1 = (unsigned int)d4v_t434338[clb] + ((v2c & 1U) << 4);
            else if (v14 == 3)
                t1 = (unsigned int)d4v_t434438[clb] + ((v2c % 3U) << 5);
            else
                t1 = (unsigned int)d4v_t434338[clb] + ((v2c & 3U) << 4);
            v2c++;
        }
        v24 = (v24 << 8) + chb;
        v20++;
        v34++;
        v44++;
    } while (v44 < v58);
    D4V_TBL_45AF54_32(&c->st, 0) = t6;
    return v28;
    /* sub_41D540 endp */
}

/* ---- sub_41DBC0 (lst 43104..43513, 409 lines) ----
 * Role hypothesis: block model-order selector feeding sub_41CF60's t6
 * (the sole caller at lst 42350..42353 passes t0=src ptr, t3=t6-0x100
 * and takes back t0). For short inputs (t3 <= 0x800) it returns the
 * entry count untouched. Otherwise it zeroes the 45AF00/45AF14 score
 * tables and the 455F60/457378 histograms (via sub_40D570), builds five
 * lagged byte-difference lanes over 0x400 windows (diff bytes into the
 * 453760 area, counts into 455F60, squared-count scores into 45AF00;
 * then the same shape over the second half into 457378/45AF14), and
 * finally walks positions, maintaining per-lane 455F60/457378 costs and
 * picking per position the cheaper of two predictors (45AF00 minimum
 * with 3x-vs-4x hysteresis, then 45AF14 minimum); positions where both
 * agree advance, otherwise it records byte_45C676 and returns pos-0x200.
 * Entry-live regs (first-use): t0 (saved 0x41DBC8, reloaded 0x41DC69),
 * t3 (saved 0x41DBCC, compared 0x41DBD9). t1/t2/t4/t5/t6 are
 * pushed and written before any read (t1 0x41DBF7, t2 0x41DC19,
 * t4 0x41DC03, t5 0x41DC6F, t6 0x41DC6D). DEVIATION: none from the
 * 2-arg call-site shape. Frame: 5 pushes + sub esp,60h = 0x74
 * (var_74..var_18).
 * NOTE: `mov t4,400h` (0x41DC03) is read much later (0x41DC34) across
 * two d4v_40d570 calls, and `xor t2,t2` (0x41DC19) across one; the
 * draft reads the C locals, matching the original's register survival.
 */
unsigned int d4vc_pick_model_order(dec4v_ctx *c, unsigned int base, unsigned int count)
{
    unsigned int t0;
    unsigned int t1;
    unsigned int t2;
    unsigned int t3;
    unsigned int t4;
    unsigned int t5;
    unsigned int t6;
    unsigned int k;
    unsigned int v24;
    unsigned int v28;
    unsigned int v30;
    unsigned int v34;
    unsigned int v38;
    unsigned int v3c;
    unsigned int v40;
    unsigned int v44;
    unsigned int v48;
    unsigned int v4c;
    unsigned int v50;
    unsigned int v54;
    unsigned int v58;
    unsigned int v5c;
    unsigned int v60;
    unsigned int v68;
    unsigned int v6c;
    unsigned int v74;
    unsigned char v18;
    unsigned char v20;
    unsigned char v1c;
    unsigned char blb;
    unsigned char bhb;
    unsigned char clb;
    int iofast;

    v40 = base;
    iofast = (base >= 4u && base < D4V_IO_SIZE - 0x800u);
    v6c = count;
    v18 = D4V_CTX_B_45C676_8(&c->st, 0);
    if (v6c <= 0x800U) {
        D4V_CTX_B_45C676_8(&c->st, 0) = v18;
        return v6c;
    }
    d4v_fill(c, 0x14U, D4V_TBL_45AF00, 0);
    d4v_fill(c, 0x14U, D4V_TBL_45AF14, 0);
    d4v_fill(c, 0x1400U, D4V_TBL_455F60, 0);
    d4v_fill(c, 0x1400U, D4V_TBL_457378, 0);
    v3c = 0;
    v54 = 0x400U;
    v5c = 0;
    for (k = 0; k < 5U; k++) {
        v34 = k << 10;
        t0 = v40;
        t4 = v3c;
        t6 = t4;
        t5 = t0 - k;
        t2 = t4;
        t3 = 0;
        while (t3 < 0x400U) {
            if (k == 0U || t3 < k) {
                if (iofast)
                    blb = c->io_mem[t0];
                else
                    blb = *(unsigned char *)d4v_ptr(c, (uint32_t)t0);
                D4V_TBL_453760_8(&c->st, t4) = blb;
            } else {
                if (iofast) {
                    blb = c->io_mem[t0];
                    bhb = c->io_mem[t5];
                } else {
                    blb = *(unsigned char *)d4v_ptr(c, (uint32_t)t0);
                    bhb = *(unsigned char *)d4v_ptr(c, (uint32_t)t5);
                }
                blb = (unsigned char)(blb - bhb);
                D4V_TBL_453760_8(&c->st, t6) = blb;
            }
            t4++;
            t1 = (uint32_t)D4V_TBL_453760_8(&c->st, t2);
            t6++;
            t0++;
            t1 <<= 2;
            t5++;
            t1 += v34;
            t2++;
            t3++;
            D4V_TBL_455F60_32(&c->st, t1)++;
        }
        t3 = v5c;
        t0 = k << 10;
        t1 = v54;
        while (t0 != t1) {
            t2 = (uint32_t)D4V_TBL_455F60_32(&c->st, t0);
            t2 *= t2;
            t5 = (uint32_t)D4V_TBL_45AF00_32(&c->st, t3);
            t5 += t2;
            t0 += 4;
            D4V_TBL_45AF00_32(&c->st, t3) = t5;
        }
        v3c += 0x800U;
        v54 += 0x400U;
        v5c += 4;
    }
    v44 = 0x800U;
    v50 = 0x400U;
    v48 = 0;
    v60 = 0;
    v68 = v40 + 0x400U;
    for (k = 0; k < 5U; k++) {
        v30 = k << 10;
        t2 = v68;
        t4 = t2 - k;
        t0 = v48 + 0x400U;
        t1 = v44;
        t3 = t0;
        while (t0 != t1) {
            if (k == 0U) {
                if (iofast)
                    blb = c->io_mem[t2];
                else
                    blb = *(unsigned char *)d4v_ptr(c, (uint32_t)t2);
            } else {
                if (iofast) {
                    blb = c->io_mem[t2];
                    bhb = c->io_mem[t4];
                } else {
                    blb = *(unsigned char *)d4v_ptr(c, (uint32_t)t2);
                    bhb = *(unsigned char *)d4v_ptr(c, (uint32_t)t4);
                }
                blb = (unsigned char)(blb - bhb);
            }
            D4V_TBL_453760_8(&c->st, t3) = blb;
            t6 = (uint32_t)D4V_TBL_453760_8(&c->st, t0);
            t6 <<= 2;
            t3++;
            t6 += v30;
            t2++;
            t0++;
            D4V_TBL_457378_32(&c->st, t6)++;
            t4++;
        }
        t3 = v60;
        t0 = k << 10;
        t1 = v50;
        while (t0 != t1) {
            t2 = (uint32_t)D4V_TBL_457378_32(&c->st, t0);
            t2 *= t2;
            t4 = (uint32_t)D4V_TBL_45AF14_32(&c->st, t3);
            t4 += t2;
            t0 += 4;
            D4V_TBL_45AF14_32(&c->st, t3) = t4;
        }
        v44 += 0x800U;
        v48 += 0x800U;
        v50 += 0x400U;
        v60 += 4;
    }
    if (v6c <= 0x800U) {
        D4V_CTX_B_45C676_8(&c->st, 0) = v18;
        return v6c;
    }
    v38 = 0x800U;
    v4c = 0xC00U;
    v58 = v40 + 0x800U;
    while (1) {
        t5 = v38 & 0x7FFU;
        t0 = v4c & 0x7FFU;
        t4 = v40 + v38;
        t1 = 0;
        t3 = 0;
        v20 = 0;
        v1c = 0;
        v74 = t0;
        v24 = t5;
        v28 = v58;
        t0 = 0;
        while (t3 < 5U) {
            t2 = v24;
            clb = (unsigned char)D4V_TBL_453760_8(&c->st, t2);
            t2 = clb;
            t6 = (uint32_t)D4V_TBL_455F60_32(&c->st, t1 + t2 * 4U);
            D4V_TBL_45AF00_32(&c->st, t0) -= t6;
            t6--;
            D4V_TBL_455F60_32(&c->st, t1 + t2 * 4U) = t6;
            t2 = t6;
            t6 = (uint32_t)D4V_TBL_45AF00_32(&c->st, t0);
            t6 -= t2;
            t2 = v74;
            D4V_TBL_45AF00_32(&c->st, t0) = t6;
            clb = (unsigned char)D4V_TBL_453760_8(&c->st, t2);
            t2 = clb;
            t6 = (uint32_t)D4V_TBL_455F60_32(&c->st, t1 + t2 * 4U);
            D4V_TBL_45AF00_32(&c->st, t0) += t6;
            t6++;
            D4V_TBL_455F60_32(&c->st, t1 + t2 * 4U) = t6;
            D4V_TBL_45AF00_32(&c->st, t0) += t6;
            t6 = (uint32_t)D4V_TBL_457378_32(&c->st, t1 + t2 * 4U);
            D4V_TBL_45AF14_32(&c->st, t0) -= t6;
            t6--;
            D4V_TBL_457378_32(&c->st, t1 + t2 * 4U) = t6;
            t2 = t6;
            D4V_TBL_45AF14_32(&c->st, t0) -= t2;
            if (t3 == 0U)
                clb = *(unsigned char *)d4v_ptr(c, (uint32_t)v28);
            else {
                clb = *(unsigned char *)d4v_ptr(c, (uint32_t)v28);
                clb = (unsigned char)(clb - *(unsigned char *)d4v_ptr(c, (uint32_t)t4));
            }
            D4V_TBL_453760_8(&c->st, t5) = clb;
            t2 = clb;
            t4--;
            t0 += 4;
            t6 = (uint32_t)D4V_TBL_457378_32(&c->st, t1 + t2 * 4U);
            t1 += 0x400U;
            D4V_TBL_45AF10_32(&c->st, t0) += t6;
            t6++;
            t5 += 0x800U;
            D4V_TBL_456F78_32(&c->st, t1 + t2 * 4U) = t6;
            t2 = t6;
            t6 = (uint32_t)D4V_TBL_45AF10_32(&c->st, t0);
            t3++;
            t6 += t2;
            t2 = v74;
            D4V_TBL_45AF10_32(&c->st, t0) = t6;
            t2 += 0x800U;
            v24 += 0x800U;
            v74 = t2;
        }
        t3 = 1;
        t0 = 4;
        while (t3 < 5U) {
            if ((uint32_t)D4V_TBL_45AF00_32(&c->st, t0)
                > (uint32_t)D4V_TBL_45AF00_32(&c->st, (unsigned int)v20 * 4U))
                v20 = (unsigned char)t3;
            if ((uint32_t)D4V_TBL_45AF14_32(&c->st, t0)
                > (uint32_t)D4V_TBL_45AF14_32(&c->st, (unsigned int)v1c * 4U))
                v1c = (unsigned char)t3;
            t3++;
            t0 += 4;
        }
        if (v20 != v18) {
            t3 = (uint32_t)D4V_TBL_45AF00_32(&c->st, (unsigned int)v20 * 4U);
            t0 = (uint32_t)D4V_TBL_45AF00_32(&c->st, (unsigned int)v18 * 4U);
            t3 = t3 * 2U + t3;
            t0 <<= 2;
            if (t3 < t0)
                v20 = v18;
        }
        if (v1c != v18) {
            t0 = (uint32_t)D4V_TBL_45AF14_32(&c->st, (unsigned int)v1c * 4U);
            t3 = (uint32_t)D4V_TBL_45AF14_32(&c->st, (unsigned int)v18 * 4U);
            t0 = t0 * 2U + t0;
            t3 <<= 2;
            if (t0 < t3)
                v1c = v18;
        }
        if (v20 != v1c) {
            D4V_CTX_B_45C676_8(&c->st, 0) = v20;
            return v38 - 0x200U;
        }
        v18 = v20;
        v58++;
        v38++;
        v4c++;
        if (v38 >= v6c)
            break;
    }
    D4V_CTX_B_45C676_8(&c->st, 0) = v18;
    return v6c;
}


/* ---- sub_41E0A0 (lst 43519..44047, 528 lines) ----
 * Role hypothesis: adaptive order-N scorer with online model update, the
 * second candidate scorer called from sub_41CF60's loc_41D11F (lst 42137..
 * 42143: t0=src ptr, t3=t6=len, t1=slot score, t2=old value with the
 * order byte live in cl; the second entry (lst 42404..42411) passes the
 * same shape: t3=len, t1=slot score indexed by the order byte,
 * cl=selected class). It first estimates an order class v14 in
 * {5..9} by counting history bytes <0x40 (weight 1) versus escape-like
 * bytes >=0xC0 (weight = running index, -1 sentinel logic), over the
 * 0x100 history window plus the inverted recent buffer; on an order-class
 * change it resets the model (stores 45AF3C/45C67B/45C679/45AF48/45C67D,
 * calls sub_41E5F0). The main loop (while v24 < entry len) prices one
 * symbol per iteration into t6 (starting 0), bailing to the restore path
 * once t6 exceeds the entry score; accepted symbols update the model via
 * sub_41E9F0(al=bh-sym, dl=bl), a div-based renormalization and sub_41E640.
 * Returns the order-class byte in al, or 0 when the price loses.
 * Entry-live regs (first-use): t0 (saved 0x41E0A6, reloaded 0x41E155),
 * t3 (saved 0x41E0AA, reloaded 0x41E116), t1 (saved 0x41E0AE, reloaded
 * 0x41E4EE), cl only of t2 (compared 0x41E100; full t2 first written
 * 0x41E108/0x41E29C). t4/t5/t6 written before read (t4 0x41E116,
 * t5 0x41E2A5/0x41E1CA, t6 0x41E0EE xor). DEVIATION: none from the 4-arg
 * call-site shape; only CL of the 4th arg is live. Frame: 3 pushes +
 * sub esp,5Ch = 0x68 (var_68..var_10).
 * UNCERTAIN(0x41E58A): d4vc_s4_update_model/d4vc_s4_pick_predictor take the callees' own entry
 * footage on trust (al=bh, dl=bl here matches draft_s4b.c's 0x41E58A
 * caller note; E640 takes no regs per its first-use prologue there).
 */
unsigned int d4vc_score_adaptive_order(dec4v_ctx *c, unsigned int a, unsigned int d,
    unsigned int b, unsigned int e)
{
    unsigned int t0 = 0, t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0, t6 = 0;
    unsigned int v24 = 0, v28 = 0, v2c = 0, v30 = 0, v34 = 0, v38 = 0, v3c = 0, v40 = 0, v44 = 0, v48 = 0;
    unsigned int v4c = 0, v54 = 0, v58 = 0, v5c = 0, v60 = 0, v64 = 0, v68 = 0;
    unsigned char v10 = 0, v14 = 0, v18 = 0, v1c = 0;
    unsigned char alb = 0, ahb = 0, clb = 0, blb = 0, bhb = 0, dlb = 0;
    /* prologue: 0x41E0A0 push t4; 0x41E0A1 push t5; 0x41E0A2 push t6;
     * 0x41E0A3 sub esp,5Ch */
    v64 = a;
    v34 = d;
    v3c = b;
    t0 = D4V_DICT_CURSOR_32(&c->st, 0);
    v2c = t0;
    t0 = D4V_TBL_45AF38_32(&c->st, 0);
    v44 = t0;
    t0 = D4V_TBL_45AF48_32(&c->st, 0);
    v68 = t0;
    t0 = D4V_TBL_45AF3C_32(&c->st, 0);
    v4c = t0;
    alb = D4V_CTX_B_45C67B_8(&c->st, 0);
    v10 = alb;
    alb = D4V_CTX_B_45C67D_8(&c->st, 0);
    t3 = 0;
    v1c = alb;
    alb = D4V_CTX_B_45C679_8(&c->st, 0);
    t6 = 0;
    v18 = alb;
    ahb = 1;
    clb = (unsigned char)(e & 0xFFU); /* entry cl */
    v24 = t3;
    D4V_CTX_B_45C67E_8(&c->st, 0) = ahb;
    if (clb == ahb) {
        unsigned int n;
        unsigned int p;
        unsigned int acc;
        unsigned char b;
        n = (v34 < 0x100U) ? v34 : 0x100U;
        p = D4V_BLK_PTR_00_DICT_32(&c->st, 0) + v2c;
        acc = 0;
        do {
            b = c->blk00_dict[(p - c->blk00_base)];
            acc += (b < 0x40U || b >= 0xC0U) ? 1U : 0xFFFFFFFFUL;
            p++;
            n--;
        } while (n != 0U);
        if (v34 > 0x100U) {
            n = v34 - 0x100U;
            p = v64;
            do {
                b = (unsigned char)~c->blk00_dict[(p - c->blk00_base)];
                acc += (b < 0x40U || b >= 0xC0U) ? 1U : 0xFFFFFFFFUL;
                p++;
                n--;
            } while (n != 0U);
        }
        alb = (unsigned char)(((int)acc > 0) ? 6 : 5);
    } else if (clb == 2) {
        unsigned int n;
        unsigned int p;
        unsigned int acc4;
        unsigned int acc2;
        unsigned char b;
        unsigned int w;
        n = (v34 < 0x100U) ? v34 : 0x100U;
        p = D4V_BLK_PTR_00_DICT_32(&c->st, 0) + v2c;
        acc4 = 0;
        acc2 = 0;
        do {
            b = c->blk00_dict[(p - c->blk00_base)];
            w = (b < 0x40U || b >= 0xC0U) ? 1U : 0xFFFFFFFFUL;
            p++;
            acc4 += w;
            n--;
            if (n == 0U)
                break;
            b = c->blk00_dict[(p - c->blk00_base)];
            w = (b < 0x40U || b >= 0xC0U) ? 1U : 0xFFFFFFFFUL;
            p++;
            acc2 += w;
            n--;
        } while (n != 0U);
        if (v34 > 0x100U) {
            n = v34 - 0x100U;
            p = v64;
            do {
                b = (unsigned char)~c->blk00_dict[(p - c->blk00_base)];
                w = (b < 0x40U || b >= 0xC0U) ? 1U : 0xFFFFFFFFUL;
                p++;
                acc4 += w;
                n--;
                if (n == 0U)
                    break;
                b = (unsigned char)~c->blk00_dict[(p - c->blk00_base)];
                w = (b < 0x40U || b >= 0xC0U) ? 1U : 0xFFFFFFFFUL;
                p++;
                acc2 += w;
                n--;
            } while (n != 0U);
        }
        alb = (unsigned char)(((int)acc4 > 0 || (int)acc2 > 0) ? 8 : 7);
    } else
        alb = 9;
    v14 = alb;
    t3 = v14;
    t2 = d4v_t43429C[(unsigned int)(t3) & 0xFFU];
    t0 = d4v_t4342EC[(unsigned int)(t3) & 0xFFU];
    v54 = t2;
    v30 = t0;
    alb = v14;
    t2 = d4v_t4342C4[(unsigned int)(t3) & 0xFFU];
    t3 <<= 8;
    v5c = t2;
    t3 += 0x30200U;
    clb = D4V_CTX_B_45C67D_8(&c->st, 0);
    v38 = t3;
    if (alb != clb) {
        D4V_TBL_45AF3C_32(&c->st, 0) = v5c;
        D4V_CTX_B_45C67B_8(&c->st, 0) = 0;
        D4V_CTX_B_45C679_8(&c->st, 0) = 0;
        D4V_TBL_45AF48_32(&c->st, 0) = 0;
        alb = v14;
        D4V_CTX_B_45C67D_8(&c->st, 0) = alb;
        d4vc_s4_reset_tables(c);
    }
    t3 = v30;
    t1 = v5c;
    t5 = D4V_BLK_PTR_16_32(&c->st, 0);
    t4 = D4V_BLK_PTR_15_32(&c->st, 0);
    t3 <<= 9;
    t1 <<= 9;
    t4 += t1;
    {
        unsigned int n;
        unsigned int off;
        n = t3;
        off = t1;
        /* Same plane copy as d4vc_s4_score_candidate: blk15+off to blk16. */
        if (off <= 0x61A00u && n <= 0x61A00u - off && n <= 0xC000u)
            memcpy(c->blk16, c->blk15 + off, n);
        else {
            unsigned int left;
            left = n;
            while (left) {
                *(unsigned char *)d4v_ptr(c, (uint32_t)t5) =
                    *(unsigned char *)d4v_ptr(c, (uint32_t)t4);
                t4++;
                t5++;
                left--;
            }
        }
    }
    {
        unsigned int k;
        unsigned int idx;
        unsigned int src;
        unsigned int a17;
        unsigned int a14;
        k = v30;
        while (k != 0U) {
            k--;
            idx = k << 1;
            src = (v5c + k) << 9;
            a17 = d4v_u16_in(c, c->blk17, c->blk17_base, 0x5FEu,
                (uint32_t)(D4V_BLK_PTR_17_32(&c->st, 0) + idx + (v5c << 1)));
            d4v_u16_st_in(c, c->blk19, c->blk19_base, 0xC0u,
                (uint32_t)(D4V_BLK_PTR_19_32(&c->st, 0) + idx), a17 & 0xFFFFU);
            a14 = d4v_u16_in(c, c->blk14, c->blk14_base, 0x61A00u,
                (uint32_t)(D4V_BLK_PTR_14_32(&c->st, 0) + src));
            d4v_u16_st_in(c, c->blk18, c->blk18_base, 0xC0u,
                (uint32_t)(D4V_BLK_PTR_18_32(&c->st, 0) + idx), (a17 + a14) & 0xFFFFU);
        }
    }
    v60 = v38 << 1;
    v40 = v64 + v24;
    do {
        unsigned int slot;
        unsigned int off;
        unsigned int freq;
        unsigned int pair18;
        unsigned int bump;
        unsigned int price;
        unsigned int quot;
        if (v24 >= 0x100U)
            bhb = (unsigned char)~c->blk00_dict[((uint32_t)(v40 - 0x100U) - c->blk00_base)];
        else {
            t0 = D4V_BLK_PTR_00_DICT_32(&c->st, 0) + v2c;
            bhb = c->blk00_dict[((uint32_t)t0 - c->blk00_base)];
        }
        ahb = D4V_CTX_B_45C679_8(&c->st, 0);
        blb = bhb;
        dlb = D4V_CTX_B_45C67B_8(&c->st, 0);
        blb = (unsigned char)(blb - ahb);
        if (dlb != 0)
            blb = (unsigned char)(0U - blb);
        if (blb >= 0x80U) {
            alb = (D4V_CTX_B_45C67B_8(&c->st, 0) == 0) ? 1 : 0;
            D4V_CTX_B_45C67B_8(&c->st, 0) = alb;
        }
        slot = D4V_TBL_45AF3C_32(&c->st, 0) - v5c;
        off = ((slot << 8) + blb) << 1;
        v48 = D4V_BLK_PTR_16_32(&c->st, 0) + off;
        freq = d4v_u16_in(c, c->blk16, c->blk16_base, 0xC000u, (uint32_t)v48);
        v58 = slot << 1;
        v28 = D4V_BLK_PTR_18_32(&c->st, 0) + v58;
        pair18 = d4v_u16_in(c, c->blk18, c->blk18_base, 0xC0u, (uint32_t)v28);
        bump = *(unsigned short *)(void *)(d4v_fx(c, 244318UL + D4V_TBL_45AF3C_32(&c->st, 0) * 2U));
        if ((freq & 0xFFFFU) == 0U) {
            unsigned int f15;
            unsigned int tot;
            unsigned int old19;
            unsigned int p19;
            f15 = d4v_u16_in(c, c->blk15, c->blk15_base, 0x61A00u,
                (uint32_t)(D4V_BLK_PTR_15_32(&c->st, 0) + ((blb + v38) << 1)));
            tot = d4v_u16_in(c, c->blk14, c->blk14_base, 0x61A00u,
                (uint32_t)(D4V_BLK_PTR_14_32(&c->st, 0) + v60));
            quot = ((f15 & 0xFFFFU) << 14) / (tot & 0xFFFFU);
            price = *(unsigned short *)(void *)(d4v_fx(c, 96616UL + quot * 2U));
            t6 += price & 0xFFFFU;
            p19 = D4V_BLK_PTR_19_32(&c->st, 0) + v58;
            old19 = d4v_u16_in(c, c->blk19, c->blk19_base, 0xC0u, (uint32_t)p19);
            d4v_u16_st_in(c, c->blk19, c->blk19_base, 0xC0u, (uint32_t)p19,
                (old19 + (bump & 0xFFFFU)) & 0xFFFFU);
            d4v_u16_st_in(c, c->blk18, c->blk18_base, 0xC0u, (uint32_t)v28,
                (d4v_u16_in(c, c->blk18, c->blk18_base, 0xC0u, (uint32_t)v28) + (bump & 0xFFFFU)) & 0xFFFFU);
            freq = old19;
        }
        quot = ((freq & 0xFFFFU) << 14) / (pair18 & 0xFFFFU);
        price = *(unsigned short *)(void *)(d4v_fx(c, 96616UL + quot * 2U));
        t6 += price & 0xFFFFU;
        if (t6 > v3c)
            break;
        d4v_u16_st_in(c, c->blk16, c->blk16_base, 0xC000u, (uint32_t)v48,
            (d4v_u16_in(c, c->blk16, c->blk16_base, 0xC000u, (uint32_t)v48) + (bump & 0xFFFFU)) & 0xFFFFU);
        t5 = d4v_u16_in(c, c->blk18, c->blk18_base, 0xC0u,
            (uint32_t)(D4V_BLK_PTR_18_32(&c->st, 0) + (slot << 1)));
        t5 = (t5 + bump) & 0xFFFFU;
        d4v_u16_st_in(c, c->blk18, c->blk18_base, 0xC0u,
            (uint32_t)(D4V_BLK_PTR_18_32(&c->st, 0) + (slot << 1)), t5);
        if (t5 >= 0x4000U)
            d4vc_rescale_s4_pair(c, slot);
        d4vc_s4_update_model(c, bhb, blb);
        t2 = D4V_TBL_45AF38_32(&c->st, 0) + 1U;
        v2c++;
        v40++;
        D4V_TBL_45AF38_32(&c->st, 0) = t2;
        D4V_TBL_45AF48_32(&c->st, 0) = t2 % v54;
        d4vc_s4_pick_predictor(c);
        v24++;
    } while (v24 < v34);
    D4V_TBL_45AF38_32(&c->st, 0) = v44;
    D4V_TBL_45AF48_32(&c->st, 0) = v68;
    D4V_TBL_45AF3C_32(&c->st, 0) = v4c;
    D4V_CTX_B_45C67B_8(&c->st, 0) = v10;
    D4V_CTX_B_45C67D_8(&c->st, 0) = v1c;
    D4V_CTX_B_45C67E_8(&c->st, 0) = 0;
    D4V_CTX_B_45C679_8(&c->st, 0) = v18;
    if (t6 >= v3c)
        return 0;
    return v14;
    /* sub_41E0A0 endp */
}

/* END-S4A */
