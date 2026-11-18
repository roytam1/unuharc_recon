#include "uha04_dec4_int.h"

void d4vc_lzp(dec4v_ctx *c)
{
    uint32_t t0 = 0, t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0, t6 = 0;
    unsigned char buf[256] = {0};
    uint32_t t2c = 0, t28 = 0, t24 = 0, t20 = 0, t1c = 0;

    /* prologue 0x00415C40 push t1; 0x00415C41 push t2;
     * 0x00415C42 push t3; 0x00415C43 push t4; 0x00415C44 push t5;
     * 0x00415C45 push t6; 0x00415C46 sub esp,114h.
     * Callee-saved regs become C locals; the esp frame is buf/t2c..t1c. */

    t6 = (uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0);
    t0 = (uint32_t)D4V_TBL_45AFD0_32(&c->st, 0);
    t24 = t0;
    t0 = (uint32_t)D4V_TBL_45AFA8_32(&c->st, 0);
    t3 = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
    t20 = t0;
    t0 = (uint32_t)D4V_TBL_45BA24_32(&c->st, 0);
    t2 = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
    t28 = t0;
    t0 = 0;
    while (t0 < 0x100UL) {
        t3 = (t3 + 1) & U32MASK;
        t0 = (t0 + 1) & U32MASK;
        buf[(t0 - 1) & 0xFF] = c->blk00_dict[((t6 + t3 - 1) & U32MASK) - c->blk00_base];
        t3 &= t2;
    }
    t3 = (uint32_t)D4V_DICT_SIZE_32(&c->st, 0);
    t0 = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
    t3 = (t3 - 0x140UL) & U32MASK;
    t0 = (t0 + 0x140UL) & U32MASK;
    t3 >>= 1;
    t0 &= t2;
    t5 = (t0 + t3) & U32MASK;
    t5 &= t2;
    ;
    if (t3 > (uint32_t)D4V_TBL_45AF38_32(&c->st, 0))
        t5 = 0;
    t1 = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
    t2c = t0;
    D4V_CTX_B_45C689_8(&c->st, 0) = 0;
    if (t5 != t1) {
        unsigned int stop;
        stop = t1;
        while (1) {
            unsigned int mask;
            unsigned int sym;
            unsigned int mode;
            unsigned int next;
            unsigned int dest;
            unsigned int cls;
            mask = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
            sym = (~(unsigned int)c->blk00_dict[((t5 + t6) & U32MASK) - c->blk00_base]) & 0xFFU;
            mode = D4V_CTX_B_45C689_8(&c->st, 0);
            next = (t0 + 1U) & mask;
            dest = (t0 + t6) & U32MASK;
            if (mode == 1U) {
                unsigned int idx;
                unsigned int val;
                idx = (D4V_CTX_B_45C68C_8(&c->st, 0) - 0xA6UL) & U32MASK;
                idx = (idx * 0x0BUL) & U32MASK;
                D4V_CTX_B_45C689_8(&c->st, 0) = 0;
                val = d4v_t436254[(0x722UL + idx + sym) & U32MASK];
                c->blk00_dict[dest - c->blk00_base] = (unsigned char)((~val) & 0xFFU);
                t0 = next;
            } else if (sym < 0x80U && mode == 0U) {
                c->blk00_dict[dest - c->blk00_base] = (unsigned char)((~sym) & 0xFFU);
                t0 = next;
            } else {
                cls = d4v_t43691C[sym];
                if (cls >= 0xC8U) {
                    unsigned int outb;
                    outb = (sym - mode) & 0xFFU;
                    c->blk00_dict[dest - c->blk00_base] =
                        (unsigned char)((~outb) & 0xFFU);
                    D4V_CTX_B_45C689_8(&c->st, 0) = 0;
                    t0 = next;
                } else if (cls == 0x64U) {
                    D4V_CTX_B_45C689_8(&c->st, 0) = 1;
                    D4V_CTX_B_45C68C_8(&c->st, 0) = (unsigned char)sym;
                } else if (cls == 0x63U) {
                    D4V_CTX_B_45C689_8(&c->st, 0) = 0x20;
                } else {
                    unsigned int sp;
                    unsigned int nleft;
                    sp = (uint32_t)d4v_t4362C0[cls];
                    nleft = (uint32_t)d4v_t436450[cls];
                    if (mode != 0U) {
                        unsigned int b;
                        b = d4v_ld8g(c, sp);
                        c->blk00_dict[dest - c->blk00_base] =
                            (unsigned char)((b + 0x20U) & 0xFFU);
                        sp = (sp + 1U) & U32MASK;
                        nleft = (nleft - 1U) & 0xFFU;
                        D4V_CTX_B_45C689_8(&c->st, 0) = 0;
                        t0 = next;
                    }
                    while (1) {
                        unsigned int b;
                        b = d4v_ld8g(c, sp);
                        t0 = (t0 + 1U) & U32MASK;
                        c->blk00_dict[((t6 + t0 - 1U) & U32MASK) - c->blk00_base] =
                            (unsigned char)b;
                        t0 &= mask;
                        sp = (sp + 1U) & U32MASK;
                        if (t5 == t0)
                            break;
                        nleft = (nleft - 1U) & 0xFFU;
                        if (nleft == 0U)
                            break;
                    }
                }
            }
            if (t5 == t0)
                break;
            t5 = (t5 + 1U) & mask;
            if (t5 == stop)
                break;
        }
    }
    t3 &= 0xFFFFFF00UL;
    D4V_CTX_B_45C689_8(&c->st, 0) = 0;
    t3 = (uint32_t)D4V_TBL_45BA54_32(&c->st, 0);
    t1 = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
    t3 = (t3 + t0) & U32MASK;
    t2 = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
    t3 = (t3 - t1) & U32MASK;
    t4 = 1;
    t3 &= t2;
    D4V_DICT_CURSOR_32(&c->st, 0) = (unsigned int)t0;
    D4V_TBL_45BA54_32(&c->st, 0) = (unsigned int)t3;
    t3 = 0;
    while (t3 < 0x100UL) {
        D4V_TBL_459E58_32(&c->st, (t0 & 0xFFUL) * 4) = (unsigned int)t4;
        c->blk00_dict[((t6 + t0) & U32MASK) - c->blk00_base] = buf[t3 & 0xFF];
        t5 = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
        t0 = (t0 + t4) & t5;
        t3 = (t3 + t4) & U32MASK;
    }
    t3 = t2c;
    t0 = 0x40UL;
    D4V_BLK_PTR_00_DICT_32(&c->st, 0) = (unsigned int)t6;
    while (t0 != 0) {
        t6 = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
        t3 = (t3 - 1) & t6;
        t6 = (uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0);
        t4 = t6;
        t2 = (t3 + t6) & U32MASK;
        c->blk00_dict[(t2 - c->blk00_base) & U32MASK] = 0;
        t0 = (t0 - 1) & U32MASK;
    }
    t2 = 0x100UL;
    t5 = (uint32_t)D4V_DICT_SIZE_32(&c->st, 0);
    t1 = 0x200000UL;
    t5 = (t5 + t6) & U32MASK;
    t3 = 0xFFUL;
    t0 = t2;
    t2 = (t2 - t5) & U32MASK;
    t2 = (t2 - t0) & U32MASK;
    t2 &= 3UL;
    t0 = (t0 - t2) & U32MASK;
    if ((int)(t0 & U32MASK) > 0) {
        while (t2 != 0) {
            c->blk00_dict[t5 - c->blk00_base] = c->blk00_dict[t4 - c->blk00_base];
            t4 = (t4 + 1) & U32MASK;
            t5 = (t5 + 1) & U32MASK;
            t2 = (t2 - 1) & U32MASK;
        }
        t2 = t0;
        t0 &= 3UL;
        t2 >>= 2;
        while (t2 != 0) {
            (*(unsigned int *)(void *)(c->blk00_dict + (t5 - c->blk00_base))) =
                (*(unsigned int *)(void *)(c->blk00_dict + (t4 - c->blk00_base)));
            t4 = (t4 + 4) & U32MASK;
            t5 = (t5 + 4) & U32MASK;
            t2 = (t2 - 1) & U32MASK;
        }
    }
    t2 = (t2 + t0) & U32MASK;
    while (t2 != 0) {
        c->blk00_dict[t5 - c->blk00_base] = c->blk00_dict[t4 - c->blk00_base];
        t4 = (t4 + 1) & U32MASK;
        t5 = (t5 + 1) & U32MASK;
        t2 = (t2 - 1) & U32MASK;
    }
    d4vc_rescale_blk01(c);
    d4vc_rescale_blk02(c);
    t0 = (uint32_t)D4V_BLK_PTR_03_32(&c->st, 0);
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)(t3 & 0xFFUL));
    t1 = 0x200000UL;
    t3 = 0xFFUL;
    t0 = (uint32_t)D4V_BLK_PTR_04_32(&c->st, 0);
    t4 = t2c;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)(t3 & 0xFFUL));
    t0 = 0;
    t3 = 0;
    t1 &= 0xFFFFFF00UL;
    D4V_SHIFT_R0_32(&c->st, 0) = 0;
    D4V_SHIFT_R1_32(&c->st, 0) = 0;
    D4V_SHIFT_R2_32(&c->st, 0) = 0;
    D4V_CTX_B_45C683_8(&c->st, 0) = (unsigned char)(t1 & 0xFFUL);
    t3 &= 0xFFFF00FFUL;
    t0 = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
    t1 = (uint32_t)D4V_DICT_SIZE_32(&c->st, 0);
    D4V_CTX_B_45C68F_8(&c->st, 0) = (unsigned char)((t3 >> 8) & 0xFFUL);
    t0 = (t0 + t1) & U32MASK;
    t3 = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
    t0 = (t0 - t4) & U32MASK;
    t0 &= t3;
    if (t0 <= 0x8000UL) {
        t2 = 0xFFFFFFFFUL;
        t6 = (uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0);
        t1c = t2;
    } else {
        t0 = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
        t0 = (t0 + 0xFFFF8000UL) & U32MASK;
        t3 &= t0;
        t6 = (uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0);
        t1c = t3;
    }
    t5 = 0x40000UL;
    while (1) {
        t1 = (uint32_t)D4V_DICT_CURSOR_32(&c->st, 0);
        D4V_BLK_PTR_00_DICT_32(&c->st, 0) = (unsigned int)t6;
        if (t4 == t1)
            break;
        t0 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
        t3 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
        t0 >>= 0xD;
        t0 ^= t3;
        t0 &= 0x3FFFFUL;
        t3 = (uint32_t)D4V_BLK_PTR_03_32(&c->st, 0);
        t0 = (t0 << 3) & U32MASK;
        t3 = (t3 + t0) & U32MASK;
        t0 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
        t2 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
        t0 >>= 0xF;
        t2 ^= t0;
        t0 = (uint32_t)D4V_SHIFT_R1_32(&c->st, 0);
        t1 = (uint32_t)D4V_SHIFT_R1_32(&c->st, 0);
        t0 >>= 0xF;
        t0 ^= t1;
        t2 = (t2 << 4) & U32MASK;
        t0 = (t0 << 6) & U32MASK;
        t2 ^= t0;
        t0 = (uint32_t)D4V_SHIFT_R2_32(&c->st, 0);
        t1 = (uint32_t)D4V_SHIFT_R2_32(&c->st, 0);
        t0 >>= 0xF;
        t0 ^= t1;
        t0 ^= t2;
        t0 &= 0x3FFFFUL;
        t0 = (t0 << 3) & U32MASK;
        t2 = (uint32_t)D4V_BLK_PTR_04_32(&c->st, 0);
        t0 = (t0 + 4) & U32MASK;
        t0 = (t0 + t2) & U32MASK;
        t2 = d4v_gld32(c->blk04 + (((t0 - 4) & U32MASK) - c->blk04_base));
        d4v_gst32(c->blk04 + (t0 - c->blk04_base), t2);
        d4v_gst32(c->blk04 + (((t0 - 4) & U32MASK) - c->blk04_base), t4);
        t0 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
        d4v_gst32(c->blk03 + (((t3 + 4) & U32MASK) - c->blk03_base), t0);
        t0 = t1c;
        d4v_gst32(c->blk03 + (t3 - c->blk03_base), t4);
        if (t4 != t0) {
            if (t0 != 0xFFFFFFFFUL) {
                t3 = (uint32_t)D4V_SHIFT_R2_32(&c->st, 0);
                t0 = (uint32_t)D4V_SHIFT_R1_32(&c->st, 0);
                t3 = (t3 << 8) & U32MASK;
                t0 >>= 0x18;
                t3 = (t3 + t0) & U32MASK;
                t0 = (uint32_t)D4V_SHIFT_R1_32(&c->st, 0);
                D4V_SHIFT_R2_32(&c->st, 0) = (unsigned int)t3;
                t3 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
                t0 = (t0 << 8) & U32MASK;
                t3 >>= 0x18;
                t0 = (t0 + t3) & U32MASK;
                D4V_SHIFT_R1_32(&c->st, 0) = (unsigned int)t0;
                t0 = t6;
                t0 = c->blk00_dict[((t4 + t0) & U32MASK) - c->blk00_base];
                t3 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
                t0 &= 0xFFUL;
                t3 = (t3 << 8) & U32MASK;
                t4 = (t4 + 1) & U32MASK;
                t3 = (t3 + t0) & U32MASK;
                t0 = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
                D4V_SHIFT_R0_32(&c->st, 0) = (unsigned int)t3;
                t4 &= t0;
                continue;
            }
        } else
            t1c = 0xFFFFFFFFUL;
        t0 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
        t3 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
        D4V_TBL_45AFC4_32(&c->st, 0) = (unsigned int)t0;
        t0 >>= 0xD;
        t0 ^= t3;
        D4V_TBL_45AFC4_32(&c->st, 0) = (unsigned int)t3;
        t0 &= 0x7FFFUL;
        t1 = (uint32_t)D4V_TBL_45AFC4_32(&c->st, 0);
        t3 = t0;
        D4V_TBL_45AFAC_32(&c->st, 0) = (unsigned int)t0;
        t3 = (t3 << 5) & U32MASK;
        while (1) {
            t0 = ((uint32_t)D4V_BLK_PTR_01_32(&c->st, 0) + t3) & U32MASK;
            if (c->blk01[t0 - c->blk01_base] == 0) {
                D4V_TBL_45AFAC_32(&c->st, 0) =
                    (D4V_TBL_45AFAC_32(&c->st, 0) + t5) & U32MASK;
                break;
            }
            if (t1 != d4v_gld32(c->blk01 + (((t0 + 0x1C) & U32MASK) - c->blk01_base))) {
                D4V_TBL_45AFAC_32(&c->st, 0) =
                    (D4V_TBL_45AFAC_32(&c->st, 0) + 0x8000UL) & U32MASK;
                t3 = (t3 + 0x100000UL) & U32MASK;
                if (t5 > D4V_TBL_45AFAC_32(&c->st, 0))
                    continue;
                break;
            }
            D4V_CTX_B_45C692_8(&c->st, 0) = 0xFF;
            t2 = 0;
            t3 = c->blk01[t0 - c->blk01_base];
            t2 = c->blk01[((t0 + 1) & U32MASK) - c->blk01_base];
            D4V_TBL_45AF9C_32(&c->st, 0) = (t3 + t2) & U32MASK;
            t3 = 1;
            while (1) {
                t2 = c->blk01[((t3 + t0 + 1) & U32MASK) - c->blk01_base];
                t3 = (t3 + 1) & U32MASK;
                if (t2 == 0)
                    break;
                D4V_TBL_45AF9C_32(&c->st, 0) =
                    (D4V_TBL_45AF9C_32(&c->st, 0) + t2) & U32MASK;
                if (t3 >= 0x0DUL)
                    break;
            }
            break;
        }
        t0 = c->blk00_dict[((t4 + t6) & U32MASK) - c->blk00_base];
        D4V_BLK_PTR_00_DICT_32(&c->st, 0) = (unsigned int)t6;
        d4vc_ppm_list_update_32(c, (unsigned int)(t0 & 0xFFUL));
        t6 = (uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0);
        t3 = (uint32_t)D4V_SHIFT_R2_32(&c->st, 0);
        t0 = (uint32_t)D4V_SHIFT_R1_32(&c->st, 0);
        t3 = (t3 << 8) & U32MASK;
        t0 >>= 0x18;
        t3 = (t3 + t0) & U32MASK;
        t0 = (uint32_t)D4V_SHIFT_R1_32(&c->st, 0);
        D4V_SHIFT_R2_32(&c->st, 0) = (unsigned int)t3;
        t3 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
        t0 = (t0 << 8) & U32MASK;
        t3 >>= 0x18;
        t0 = (t0 + t3) & U32MASK;
        D4V_SHIFT_R1_32(&c->st, 0) = (unsigned int)t0;
        t0 = c->blk00_dict[((t4 + t6) & U32MASK) - c->blk00_base];
        t3 = (uint32_t)D4V_SHIFT_R0_32(&c->st, 0);
        t0 &= 0xFFUL;
        t3 = (t3 << 8) & U32MASK;
        t4 = (t4 + 1) & U32MASK;
        t3 = (t3 + t0) & U32MASK;
        t0 = (uint32_t)D4V_DICT_MASK_32(&c->st, 0);
        D4V_SHIFT_R0_32(&c->st, 0) = (unsigned int)t3;
        t4 &= t0;
    }
    t0 = t24;
    D4V_TBL_45AFD0_32(&c->st, 0) = (unsigned int)t0;
    t0 = t20;
    D4V_TBL_45AFA8_32(&c->st, 0) = (unsigned int)t0;
    t0 = t28;
    t1 = 0x800UL;
    D4V_TBL_45BA24_32(&c->st, 0) = (unsigned int)t0;
    t0 = (uint32_t)D4V_BLK_PTR_14_32(&c->st, 0);
    t3 = 0;
    t0 = (t0 + 0x5F600UL) & U32MASK;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t1 = 0x800UL;
    t0 = (uint32_t)D4V_BLK_PTR_15_32(&c->st, 0);
    t3 = 0;
    t0 = (t0 + 0x5F600UL) & U32MASK;
    d4v_fill(c, (unsigned int)t1, (unsigned int)t0,
        (unsigned int)(t3 & 0xFFUL));
    t0 = (uint32_t)D4V_BLK_PTR_17_32(&c->st, 0);
    d4v_gst32(c->blk17 + (((t0 + 0x5F6UL) & U32MASK) - c->blk17_base), 0x10001UL);
    d4v_u16_st_in(c, c->blk17, c->blk17_base, 0x5FEu,
        (uint32_t)((t0 + 0x5FAUL) & U32MASK), 1);
    t1 = (t1 & 0xFFFF00FFUL) |
        ((uint32_t)D4V_CTX_B_45C675_8(&c->st, 0) << 8);
    d4v_u16_st_in(c, c->blk17, c->blk17_base, 0x5FEu,
        (uint32_t)((t0 + 0x5FCUL) & U32MASK), 1);
    ;
    if (((t1 >> 8) & 0xFFUL) == 1)
        t0 = (uint32_t)D4V_TBL_45BA28_32(&c->st, 0);
    else
        t0 = (uint32_t)D4V_IO_OUT_TOTAL_32(&c->st, 0);
    t6 = (uint32_t)D4V_BLK_PTR_00_DICT_32(&c->st, 0);
    D4V_TBL_45AFB4_32(&c->st, 0) = (unsigned int)t0;
    t3 = 0xFFFFFFFFUL;
    t0 = (t0 & 0xFFFFFF00UL) | 1UL;
    D4V_LZP_TRIGGER_32(&c->st, 0) = (unsigned int)t3;
    D4V_CTX_B_45C67F_8(&c->st, 0) = (unsigned char)(t0 & 0xFFUL);
    D4V_BLK_PTR_00_DICT_32(&c->st, 0) = (unsigned int)t6;
    /* epilogue 0x0041620B add esp,114h; 0x00416211 pop t6;
     * 0x00416212 pop t5; 0x00416213 pop t4; 0x00416214 pop t3;
     * 0x00416215 pop t2; 0x00416216 pop t1; 0x00416217 retn. */
}

/* ------------------------------------------------------------------ */
/* Role hypothesis: PPM block path / model (re)initialisation. Zeroes */
/* the small static tables, builds the 45A280/45A281 seed tables from */
/* the dword_4150D0 template, biases the 45AAxx/45ACxx tables, copies */
/* the static 0x436ED8 data into the [0x433B78] model area, rebuilds */
/* its u16 prefix sums, runs sub_416A00 over 0x203..0x2FE, rescales */
/* (sub_415AE0), allocates/clears the 45B5F4/45AFD8/4593A0/458798 */
/* tables, then walks the 45AFB0/45BA2C extent list against 45AFB4 to */
/* pick the block-tail magic (75A45/8F4D0/17E5F8/2CC640/388DC2/7D4C0/ */
/* FB670), re-clearing the LZP tables on some transitions, and finally */
/* range-decodes the method selector (45D0B8/45D0BC + sub_4220B0) to */
/* choose between the early return, the sub_41CF60 tail call (45C675 */
/* set path) and the sub_41E5F0 tail call. */
/* Entry-live regs: none (t1/t3/t0 set up the first fill call; no */
/* stack args). Returns int = final t0 (tail-call results of */
/* sub_41CF60/sub_41E5F0, else the range-quotient leftover); both */
/* in-tree callers (417DC5, 41CE8C) discard it. */
/* Frame: 0x1C bytes + 6 pushed regs. Locals: s[11] template @ sp+0 */
/* (var_34), w28 @ sp+0xC (var_28), w24 @ sp+0x10 (var_24, outer */
/* 0..0x700 step 0x100), w20 @ sp+0x14 (var_20), w1c @ sp+0x18 */
/* (var_1C, 0..0xA). */
/* Globals touched: 45A278,45A258,45AA80,45A280,45AE80,433B58..433B7C */
/* 45A25D..45A27E,433B84,433BE8,459BA8,45AFCC,45AFC0,45AFA0,45AFD4 */
/* 45B5F4,45AF78,45AFD8,4593A0,458798,458F94,45AF80,45AF64,459398 */
/* 45939C,459BA0,459BA4,45AF60,45AFB0,45AF84,45C67F,45AFB4,45CD14 */
/* 45BA2C,45BA44,45BA50,45C675,45BA28,45AFA8,45AFD0,45BA24,45BA34 */
/* 45BA3C,45BA30,433B44,433B48,45C681,45D0BC,45D0B8,45AF68,45AF6C */
/* 45AF70,45C67D,T4342C4,T43429C,T434314,45AF3C,45AF40,45C67A,45C688 */
/* 45C67B,45C679,45AF48 plus static tables T4150D0/T436ED8 (arena). */
/* ------------------------------------------------------------------ */
