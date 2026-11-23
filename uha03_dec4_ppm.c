#include "uha03_dec4_int.h"

void d3uc_ctx_model_update(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int t5 = 0;
    unsigned int t6 = 0;
    unsigned int v1c = 0;
    unsigned int v18 = 0;
    unsigned int adr = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    v1c = eax_v;
    t1 = 0;
    t0 = (eax_v << 8) & U32MASK;
    t1 = (t1 & 0xFFFFFF00U) | (edx_v & 0xFFU);
    t0 = (t0 + t1) & U32MASK;
    t6 = (uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t0 = (t0 + t0) & U32MASK;
    t2 = (t0 + t6) & U32MASK;
    t0 = v1c;
    t0 = (t0 + t0) & U32MASK;
    t5 = (t5 & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(t2));
    t4 = (t4 & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)((D3U_TBL_44428A + t0) & U32MASK));
    if ((t5 & 0xFFFFU) != 0) { _pc = 3; continue; }
    t5 = (t5 & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)((D3U_TBL_444884 + t0) & U32MASK));
    /* upper 16 = entry t5 (unknown); only the low 16 is read below */
    v18 = t5 & 0xFFFFU;
    t5 = (uint32_t)D3U_BLK17_SMALL_B_32(&c->st, 0);
    t0 = (t0 + t5) & U32MASK;
    d3u_st16(c, (uint32_t)(t0), (unsigned short)(((unsigned int)d3u_ld16(c, (uint32_t)(t0)) + (t4 & 0xFFFFU)) & 0xFFFFU));
    t0 = D3U_TBL_44421C_32(&c->st, 0);
    t0 = (t0 << 8) & U32MASK;
    t0 = (t0 + t1) & U32MASK;
    t1 = v18;
    adr = (t6 + (t0 << 1)) & U32MASK;
    d3u_st16(c, (uint32_t)(adr), (unsigned short)(((unsigned int)d3u_ld16(c, (uint32_t)(adr)) + (t1 & 0xFFFFU)) & 0xFFFFU));
    t6 = D3U_TBL_44421C_32(&c->st, 0);
    t0 = (uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    t6 = (t6 << 9) & U32MASK;
    t6 = (t6 + t0) & U32MASK;
    t0 = (t0 & 0xFFFFFF00U) | (edx_v & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | (((t0 & 0xFFU) + 1) & 0xFFU);
    if ((t0 & 0xFFU) == 0) { _pc = 2; continue; }
    t5 = v18;
    _pc = 1;
    continue;
        case 1:
    /* loc_414F3A: */
    t1 = 0;
    t1 = t0 & 0xFFU;
    t0 = (t0 & 0xFFFF00FFU) | ((t0 & 0xFFU) << 8);
    t0 = (t0 & 0xFFFF00FFU) | (((0U - ((t0 >> 8) & 0xFFU)) & 0xFFU) << 8);
    t0 = (t0 & 0xFFFF00FFU) | (((((t0 >> 8) & 0xFFU) & (t0 & 0xFFU))) << 8);
    adr = (t6 + (t1 << 1)) & U32MASK;
    d3u_st16(c, (uint32_t)(adr), (unsigned short)(((unsigned int)d3u_ld16(c, (uint32_t)(adr)) + (t5 & 0xFFFFU)) & 0xFFFFU));
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) + ((t0 >> 8) & 0xFFU))) & 0xFFU);
    if ((t0 & 0xFFU) != 0) { _pc = 1; continue; }
    _pc = 2;
    continue;
        case 2:
    /* loc_414F4D: */
    t0 = v18;
    t1 = (t1 & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(t6));
    t1 = (t1 + t0) & U32MASK;
    d3u_st16(c, (uint32_t)(t6), (unsigned short)(t1 & 0xFFFFU));
    if ((t1 & 0xFFFFU) < 0x4000U) { _pc = 3; continue; }
    t0 = D3U_TBL_44421C_32(&c->st, 0);
    d3uc_fold_ctx_block(c, t0);
    _pc = 3;
    continue;
        case 3:
    /* loc_414F6C: */
    t6 = v1c;
    if (t6 == 0x200U) { _pc = 6; continue; }
    d3u_st16(c, (uint32_t)(t2), (unsigned short)(((unsigned int)d3u_ld16(c, (uint32_t)(t2)) + (t4 & 0xFFFFU)) & 0xFFFFU));
    t0 = t6;
    t2 = (uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    t0 = (t0 << 9) & U32MASK;
    t2 = (t2 + t0) & U32MASK;
    t3 = edx_v;
    t3 = (t3 & 0xFFFFFF00U) | (((t3 & 0xFFU) + 1) & 0xFFU);
    if ((t3 & 0xFFU) == 0) { _pc = 5; continue; }
    _pc = 4;
    continue;
        case 4:
    /* loc_414F8B: */
    t0 = 0;
    t0 = t3 & 0xFFU;
    adr = (t2 + (t0 << 1)) & U32MASK;
    d3u_st16(c, (uint32_t)(adr), (unsigned short)(((unsigned int)d3u_ld16(c, (uint32_t)(adr)) + (t4 & 0xFFFFU)) & 0xFFFFU));
    t0 = (0U - (t0 & 0xFFU)) & 0xFFU;
    t0 = (t0 & (t3 & 0xFFU)) & 0xFFU;
    t3 = (t3 & 0xFFFFFF00U) | ((((t3 & 0xFFU) + (t0 & 0xFFU))) & 0xFFU);
    if ((t3 & 0xFFU) != 0) { _pc = 4; continue; }
    _pc = 5;
    continue;
        case 5:
    /* loc_414F9B: */
    t0 = v1c;
    t1 = (uint32_t)D3U_BLK17_SMALL_B_32(&c->st, 0);
    t0 = (t0 + t0) & U32MASK;
    t5 = (t5 & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(t2));
    t0 = (t0 + t1) & U32MASK;
    t1 = 0;
    t5 = (t5 + t4) & U32MASK;
    t1 = (unsigned int)d3u_ld16(c, (uint32_t)(t0));
    t0 = 0;
    t0 = t5 & 0xFFFFU;
    t0 = (t0 + t1) & U32MASK;
    d3u_st16(c, (uint32_t)(t2), (unsigned short)(t5 & 0xFFFFU));
    if ((int)t0 < 0x4000) { _pc = 6; continue; }
    t0 = v1c;
    d3uc_fold_ctx_block(c, t0);
    _pc = 6;
    continue;
        case 6:
    /* loc_414FCB: */
    /* retn */
    ;
        default:
            break;
        }
        break;
    }
}
/* sub_414FD4: entry al = index, dl = flag. */
void d3uc_update_len_model(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int adr = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t1 = 0;
    t1 = eax_v & 0xFFU;
    t3 = edx_v;
    t0 = 0;
    t0 = (unsigned int)D3U_FLAG_444EB6_8(&c->st, 0);
    t0 = (t0 + t1) & U32MASK;
    t0 = (t0 << 3) & U32MASK;
    t0 = (t0 + D3U_TBL_443B68) & U32MASK;
    if ((t3 & 0xFFU) != 1) { _pc = 1; continue; }
    t1 = 0;
    t1 = t3 & 0xFFU;
    adr = (t0 + (t1 << 1) + 2) & U32MASK;
    d3u_st16(c, (uint32_t)(adr), (unsigned short)(((unsigned int)d3u_ld16(c, (uint32_t)(adr)) + 0x40U) & 0xFFFFU));
    _pc = 1;
    continue;
        case 1:
    /* loc_414FFB: */
    /* t4 high = entry garbage; only low 16 observed below */
    t4 = (unsigned int)d3u_ld16(c, (uint32_t)((t0 + 6) & U32MASK));
    t4 = (t4 + 0x40U) & U32MASK;
    d3u_st16(c, (uint32_t)((t0 + 6) & U32MASK), (unsigned short)(t4 & 0xFFFFU));
    if ((t4 & 0xFFFFU) < 0x4000U) { _pc = 2; continue; }
    t1 = 0;
    t2 = 0;
    t1 = (unsigned int)d3u_ld16(c, (uint32_t)((t0 + 4) & U32MASK));
    t2 = (unsigned int)d3u_ld16(c, (uint32_t)((t0 + 2) & U32MASK));
    t4 = t1;
    t4 = (t4 - t2) & U32MASK;
    t2 = (t4 + 1) & U32MASK;
    t2 = (unsigned int)(((int)t2) >> 1);
    t4 = t2;
    t2 = 0;
    t2 = (unsigned int)d3u_ld16(c, (uint32_t)((t0 + 6) & U32MASK));
    t2 = (t2 - t1) & U32MASK;
    t2 = (t2 + 1) & U32MASK;
    t1 = (t1 & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)((t0 + 2) & U32MASK));
    t2 = (unsigned int)(((int)t2) >> 1);
    t4 = (t4 + t1) & U32MASK;
    t2 = (t2 + t4) & U32MASK;
    d3u_st16(c, (uint32_t)((t0 + 4) & U32MASK), (unsigned short)(t4 & 0xFFFFU));
    d3u_st16(c, (uint32_t)((t0 + 6) & U32MASK), (unsigned short)(t2 & 0xFFFFU));
    _pc = 2;
    continue;
        case 2:
    /* loc_41503F: */
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EB6_8(&c->st, 0);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) + (t0 & 0xFFU))) & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) + (t3 & 0xFFU))) & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | (((t0 & 0xFFU) - 1) & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 7U);
    D3U_FLAG_444EB6_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    /* retn */
        default:
            break;
        }
        break;
    }
}
/* sub_4153C0: entry al = symbol. Pure 8-way model update; no coder calls. */
void d3uc_update_model_8way(dec3w_ctx *c, unsigned int eax_v)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int t5 = 0;
    unsigned int t6 = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t4 = D3U_TBL_44427C_32(&c->st, 0);
    t6 = D3U_TBL_444204_32(&c->st, 0);
    /* t2 high = entry garbage; only cl/ch lanes observed below */
    t2 = eax_v & 0xFFU;
    _pc = 1;
    continue;
        case 1:
    /* loc_4153D6: */
    t0 = 0;
    t0 = (t2 >> 8) & 0xFFU;
    t5 = D3U_TBL_443CE8_32(&c->st, (t0 << 2));
    t1 = D3U_TBL_4299D0_32(&c->st, (t0 << 2));
    t5 = (t5 << 2) & U32MASK;
    t1 = (t1 + t5) & U32MASK;
    t0 = (t0 & 0xFFFF00FFU) | (((unsigned int)M8((t1 + 1) & U32MASK)) << 8);
    if (((t0 >> 8) & 0xFFU) == 0) { _pc = 14; continue; }
    if (((t2 >> 8) & 0xFFU) != (unsigned int)D3U_FLAG_444EA3_8(&c->st, 0)) { _pc = 8; continue; }
    if (D3U_FLAG_444EB2_8(&c->st, 0) == 0) { _pc = 4; continue; }
    if ((t2 & 0xFFU) != (unsigned int)M8((t1 + 2) & U32MASK)) { _pc = 2; continue; }
    t0 &= 0xFFFF00FFU;
    D3U_FLAG_444EAE_8(&c->st, 0) = 0;
    t0 = 0;
    t0 = (unsigned int)M8(t1);
    _pc = 3;
    continue;
        case 2:
    /* loc_415420: */
    t0 = 0;
    t0 = (unsigned int)M8((t1 + 1) & U32MASK);
    _pc = 3;
    continue;
        case 3:
    /* loc_415425: */
    t3 = 0;
    t5 = (unsigned int)M8(t1);
    t3 = (unsigned int)M8((t1 + 1) & U32MASK);
    t0 = (t0 << 14) & U32MASK;
    t5 = (t5 + t3) & U32MASK;
    t3 = 0;
    /* x86 div faults on div0 (both counts zero); direct / % below */
    t0 = t0 / t5;
    /* remainder in t3 discarded by the original */
    t0 = (t0 & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)((D3U_U16_CUM_DST + (t0 << 1)) & U32MASK));
    t0 &= 0xFFFFU;
    t6 = (t6 + t0) & U32MASK;
    /* loc_415445: falls through below */
    _pc = 4;
    continue;
        case 4:
    /* loc_415445: */
    if (t4 == 0) { _pc = 8; continue; }
    if ((t2 & 0xFFU) != (unsigned int)M8((t1 + 2) & U32MASK)) { _pc = 5; continue; }
    M8(t4) = (unsigned char)(((unsigned int)M8(t4) + 1) & 0xFFU);
    _pc = 6;
    continue;
        case 5:
    /* loc_415452: */
    M8((t4 + 1) & U32MASK) = (unsigned char)(((unsigned int)M8((t4 + 1) & U32MASK) + 1) & 0xFFU);
    _pc = 6;
    continue;
        case 6:
    /* loc_415455: */
    t3 = (t3 & 0xFFFF00FFU) | (((unsigned int)M8(t4)) << 8);
    t0 = t4;
    if (((t3 >> 8) & 0xFFU) >= 0xFEU) { _pc = 7; continue; }
    if ((unsigned int)M8((t4 + 1) & U32MASK) < 0xFEU) { _pc = 8; continue; }
    _pc = 7;
    continue;
        case 7:
    /* loc_415464: */
    t3 = (t3 & 0xFFFF00FFU) | (((unsigned int)M8(t4)) << 8);
    t3 = (t3 & 0xFFFF00FFU) | (((((t3 >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    M8(t4) = (unsigned char)((t3 >> 8) & 0xFFU);
    t3 = (t3 & 0xFFFFFF00U) | ((t3 >> 8) & 0xFFU);
    t3 = (t3 & 0xFFFFFF00U) | (((t3 & 0xFFU) >> 1) & 0xFFU);
    M8(t4) = (unsigned char)(t3 & 0xFFU);
    t3 = (t3 & 0xFFFF00FFU) | (((unsigned int)M8((t4 + 1) & U32MASK)) << 8);
    t3 = (t3 & 0xFFFF00FFU) | (((((t3 >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    M8((t4 + 1) & U32MASK) = (unsigned char)((t3 >> 8) & 0xFFU);
    t3 = (t3 & 0xFFFFFF00U) | ((t3 >> 8) & 0xFFU);
    t3 = (t3 & 0xFFFFFF00U) | (((t3 & 0xFFU) >> 1) & 0xFFU);
    M8((t4 + 1) & U32MASK) = (unsigned char)(t3 & 0xFFU);
    _pc = 8;
    continue;
        case 8:
    /* loc_41547F: */
    t3 = (t3 & 0xFFFF00FFU) | (((unsigned int)M8((t1 + 3) & U32MASK)) << 8);
    t3 = (t3 & 0xFFFF00FFU) | ((((t3 >> 8) & 0xFFU) & 0x7FU) << 8);
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)M8((t1 + 2) & U32MASK);
    M8((t1 + 3) & U32MASK) = (unsigned char)((t3 >> 8) & 0xFFU);
    if ((t2 & 0xFFU) != (t0 & 0xFFU)) { _pc = 10; continue; }
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8(t1);
    if ((t3 & 0xFFU) < 0xFEU) { _pc = 9; continue; }
    t3 = (t3 & 0xFFFF00FFU) | ((t3 & 0xFFU) << 8);
    t3 = (t3 & 0xFFFF00FFU) | (((((t3 >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    t0 = (t0 & 0xFFFFFF00U) | ((t3 >> 8) & 0xFFU);
    M8(t1) = (unsigned char)((t3 >> 8) & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | (((t0 & 0xFFU) >> 1) & 0xFFU);
    M8(t1) = (unsigned char)(t0 & 0xFFU);
    t0 = (t0 & 0xFFFF00FFU) | (((unsigned int)M8((t1 + 1) & U32MASK)) << 8);
    t0 = (t0 & 0xFFFF00FFU) | (((((t0 >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    t3 = (t3 & 0xFFFFFF00U) | ((t0 >> 8) & 0xFFU);
    M8((t1 + 1) & U32MASK) = (unsigned char)((t0 >> 8) & 0xFFU);
    t3 = (t3 & 0xFFFFFF00U) | (((t3 & 0xFFU) >> 1) & 0xFFU);
    M8((t1 + 1) & U32MASK) = (unsigned char)(t3 & 0xFFU);
    _pc = 9;
    continue;
        case 9:
    /* loc_4154B1: */
    t3 = (t3 & 0xFFFF00FFU) | (((unsigned int)M8(t1)) << 8);
    t3 = (t3 & 0xFFFF00FFU) | (((((t3 >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)M8((t1 + 3) & U32MASK);
    M8(t1) = (unsigned char)((t3 >> 8) & 0xFFU);
    if ((t0 & 0xFFU) >= 0x7FU) { _pc = 16; continue; }
    t0 = (t0 & 0xFFFF00FFU) | ((t0 & 0xFFU) << 8);
    t0 = (t0 & 0xFFFF00FFU) | (((((t0 >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    _pc = 15;
    continue;
        case 10:
    /* loc_4154C4: */
    if ((unsigned int)M8((t1 + 1) & U32MASK) < 0xFEU) { _pc = 11; continue; }
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8(t1);
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)M8((t1 + 1) & U32MASK);
    t3 = (t3 & 0xFFFFFF00U) | ((((t3 & 0xFFU) + 1)) & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) + 1)) & 0xFFU);
    M8(t1) = (unsigned char)(t3 & 0xFFU);
    M8((t1 + 1) & U32MASK) = (unsigned char)(t0 & 0xFFU);
    t3 = (t3 & 0xFFFF00FFU) | ((t3 & 0xFFU) << 8);
    t0 = (t0 & 0xFFFF00FFU) | ((t0 & 0xFFU) << 8);
    t3 = (t3 & 0xFFFF00FFU) | (((((t3 >> 8) & 0xFFU) >> 1)) << 8);
    t0 = (t0 & 0xFFFF00FFU) | (((((t0 >> 8) & 0xFFU) >> 1)) << 8);
    M8(t1) = (unsigned char)((t3 >> 8) & 0xFFU);
    M8((t1 + 1) & U32MASK) = (unsigned char)((t0 >> 8) & 0xFFU);
    _pc = 11;
    continue;
        case 11:
    /* loc_4154E5: */
    M8((t1 + 1) & U32MASK) = (unsigned char)(((unsigned int)M8((t1 + 1) & U32MASK) + 1) & 0xFFU);
    t3 = (t3 & 0xFFFF00FFU) | (((unsigned int)M8((t1 + 3) & U32MASK)) << 8);
    if (((t3 >> 8) & 0xFFU) >= 2U) { _pc = 12; continue; }
    M8((t1 + 3) & U32MASK) = 0;
    M8((t1 + 2) & U32MASK) = (unsigned char)(t2 & 0xFFU);
    _pc = 13;
    continue;
        case 12:
    /* loc_4154F9: */
    t0 = (t0 & 0xFFFFFF00U) | ((t3 >> 8) & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | (((t0 & 0xFFU) >> 1) & 0xFFU);
    M8((t1 + 3) & U32MASK) = (unsigned char)(t0 & 0xFFU);
    _pc = 13;
    continue;
        case 13:
    /* loc_415500: */
    M8((t1 + 3) & U32MASK) = (unsigned char)(((unsigned int)M8((t1 + 3) & U32MASK) + 0x80U) & 0xFFU);
    _pc = 16;
    continue;
        case 14:
    /* loc_415506: */
    M8(t1) = 2;
    M8((t1 + 1) & U32MASK) = 2;
    M8((t1 + 2) & U32MASK) = (unsigned char)(t2 & 0xFFU);
    _pc = 15;
    continue;
        case 15:
    /* loc_415510: */
    M8((t1 + 3) & U32MASK) = (unsigned char)((t0 >> 8) & 0xFFU);
    _pc = 16;
    continue;
        case 16:
    /* loc_415513: */
    t2 = (t2 & 0xFFFF00FFU) | (((((t2 >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    if (((t2 >> 8) & 0xFFU) < 8U) { _pc = 1; continue; }
    D3U_TBL_444204_32(&c->st, 0) = t6;
    D3U_TBL_44427C_32(&c->st, 0) = t4;
    /* retn */
        default:
            break;
        }
        break;
    }
}
/* sub_415534: entry al = symbol. Order-model update; calls sub_40C9C0. */
void d3uc_update_order_model_a(dec3w_ctx *c, unsigned int eax_v)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int t5 = 0;
    unsigned int t6 = 0;
    unsigned int adr = 0;
    /* t2 high = entry garbage; only cl/ch lanes observed below */
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t2 = (unsigned int)D3U_FLAG_444EB7_8(&c->st, 0);
    t2 = (t2 & 0xFFFF00FFU) | ((eax_v & 0xFFU) << 8);
    t3 = D3U_TBL_444224_32(&c->st, 0);
    if (t3 >= 0x40000U) { _pc = 19; continue; }
    t1 = t3;
    t0 = (uint32_t)D3U_BLK01_DICT_COPY_32(&c->st, 0);
    t1 = (t1 << 5) & U32MASK;
    t1 = (t1 + t0) & U32MASK;
    if ((t2 & 0xFFU) != 0xFFU) { _pc = 2; continue; }
    t2 &= 0xFFFFFF00U;
    _pc = 1;
    continue;
        case 1:
    /* loc_415567: */
    t0 = 0;
    t0 = t2 & 0xFFU;
    if (((t2 >> 8) & 0xFFU) == (unsigned int)M8((t1 + t0 + 0x0E) & U32MASK)) { _pc = 2; continue; }
    t2 = (t2 & 0xFFFFFF00U) | ((((t2 & 0xFFU) + 1)) & 0xFFU);
    if ((t2 & 0xFFU) < 0x0DU) { _pc = 1; continue; }
    _pc = 2;
    continue;
        case 2:
    /* loc_415578: */
    if (D3U_FLAG_444EB0_8(&c->st, 0) == 0) { _pc = 3; continue; }
    if ((t2 & 0xFFU) >= 0x0DU) { _pc = 3; continue; }
    t0 = 0;
    t0 = t2 & 0xFFU;
    t0 = (unsigned int)M8((t1 + t0 + 1) & U32MASK);
    t4 = t0;
    t4 &= 0xFFU;
    t0 = 0;
    t4 = (unsigned int)(((int)t4) >> 6);
    t0 = (unsigned int)M8(t1);
    if ((int)t4 <= (int)t0) { _pc = 3; continue; }
    D3U_TBL_444264_32(&c->st, 0) = (D3U_TBL_444264_32(&c->st, 0) + 4) & U32MASK;
    _pc = 3;
    continue;
        case 3:
    /* loc_4155A8: */
    if (D3U_FLAG_444EB1_8(&c->st, 0) == 0) { _pc = 6; continue; }
    if ((t2 & 0xFFU) >= 0x0DU) { _pc = 4; continue; }
    t3 &= 0xFFFFFF00U;
    t0 = 0;
    D3U_FLAG_444EB5_8(&c->st, 0) = 0;
    t0 = t2 & 0xFFU;
    D3U_FLAG_444EB2_8(&c->st, 0) = 0;
    D3U_FLAG_444EAE_8(&c->st, 0) = 0;
    t0 = (unsigned int)M8((t1 + t0 + 1) & U32MASK);
    t0 &= 0xFFU;
    _pc = 5;
    continue;
        case 4:
    /* loc_4155D9: */
    t0 = 0;
    t0 = (unsigned int)M8(t1);
    _pc = 5;
    continue;
        case 5:
    /* loc_4155DD: */
    t5 = D3U_TBL_444234_32(&c->st, 0);
    t0 = (t0 << 14) & U32MASK;
    t3 = 0;
    /* x86 div faults on div0; direct / % below; remainder discarded */
    t0 = t0 / t5;
    t0 = (t0 & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)((D3U_U16_CUM_DST + (t0 << 1)) & U32MASK));
    t6 = D3U_TBL_444204_32(&c->st, 0);
    t0 &= 0xFFFFU;
    t6 = (t6 + t0) & U32MASK;
    D3U_TBL_444204_32(&c->st, 0) = t6;
    _pc = 6;
    continue;
        case 6:
    /* loc_415605: */
    t0 = 0;
    t0 = (unsigned int)M8(t1);
    t0 = (unsigned int)d3u_t429BB8[(unsigned int)(t0 & 0xFFU)];
    t3 = D3U_TBL_444214_32(&c->st, 0);
    t0 &= 0xFFU;
    t0 = (t0 & 0xFFFFFF00U) | ((t0 < t3) ? 1U : 0U);
    t0 = (t0 & 0xFFFF00FFU) | ((t0 & 0xFFU) << 8);
    if ((t2 & 0xFFU) >= 0x0DU) { _pc = 9; continue; }
    t4 = t2 & 0xFFU;
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)M8((t1 + 0x1B) & U32MASK);
    t5 = (unsigned int)M8((t4 + t1 + 1) & U32MASK);
    t4 = (unsigned int)M8((t1 + 1) & U32MASK);
    t5 = (t5 + 1) & U32MASK;
    t4 = (unsigned int)(((int)t4) >> 1);
    t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xC0U);
    if ((int)t5 >= (int)t4) { _pc = 7; continue; }
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) + 0x40U)) & 0xFFU);
    if ((t0 & 0xFFU) != 0) { _pc = 8; continue; }
    t0 = (t0 & 0xFFFF00FFU) | (1U << 8);
    _pc = 8;
    continue;
        case 7:
    /* loc_415646: */
    t4 = t0 & 0xFFU;
    t3 = t4;
    t3 = (unsigned int)(((int)t3) >> 1);
    t3 &= 0xC0U;
    t0 = (t0 & 0xFFFFFF00U) | (t3 & 0xFFU);
    _pc = 8;
    continue;
        case 8:
    /* loc_415655: */
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8((t1 + 0x1B) & U32MASK);
    t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0x3FU);
    t3 = (t3 & 0xFFFFFF00U) | ((((t3 & 0xFFU) | (t0 & 0xFFU))) & 0xFFU);
    M8((t1 + 0x1B) & U32MASK) = (unsigned char)(t3 & 0xFFU);
    _pc = 9;
    continue;
        case 9:
    /* loc_415660: */
    if (((t0 >> 8) & 0xFFU) == 0) { _pc = 11; continue; }
    t0 = (t1 + 0x0D) & U32MASK;
    t4 = (t1 - 1) & U32MASK;
    _pc = 10;
    continue;
        case 10:
    /* loc_41566A: */
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8(t0);
    t3 = (t3 & 0xFFFFFF00U) | ((((t3 & 0xFFU) + 1)) & 0xFFU);
    M8(t0) = (unsigned char)(t3 & 0xFFU);
    t3 = (t3 & 0xFFFF00FFU) | ((t3 & 0xFFU) << 8);
    t0 = (t0 - 1) & U32MASK;
    t3 = (t3 & 0xFFFF00FFU) | (((((t3 >> 8) & 0xFFU) >> 1)) << 8);
    M8((t0 + 1) & U32MASK) = (unsigned char)((t3 >> 8) & 0xFFU);
    if (t0 != t4) { _pc = 10; continue; }
    _pc = 11;
    continue;
        case 11:
    /* loc_41567C: */
    if ((t2 & 0xFFU) >= 0x0DU) { _pc = 13; continue; }
    t0 = 0;
    t0 = t2 & 0xFFU;
    adr = (t1 + t0 + 1) & U32MASK;
    M8(adr) = (unsigned char)(((unsigned int)M8(adr) + 2) & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)M8((t1 + 0x1B) & U32MASK);
    t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0x3FU);
    if ((t0 & 0xFFU) >= 0x3FU) { _pc = 12; continue; }
    M8((t1 + 0x1B) & U32MASK) = (unsigned char)(((unsigned int)M8((t1 + 0x1B) & U32MASK) + 1) & 0xFFU);
    _pc = 12;
    continue;
        case 12:
    /* loc_415696: */
    D3U_TBL_444220_32(&c->st, 0) = (D3U_TBL_444220_32(&c->st, 0) + 5) & U32MASK;
    _pc = 17;
    continue;
        case 13:
    /* loc_41569F: */
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)M8(t1);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) + 2)) & 0xFFU);
    t2 &= 0xFFFFFF00U;
    M8(t1) = (unsigned char)(t0 & 0xFFU);
    _pc = 14;
    continue;
        case 14:
    /* loc_4156A7: */
    t0 = 0;
    t0 = t2 & 0xFFU;
    if (M8((t1 + t0 + 1) & U32MASK) == 0) { _pc = 15; continue; }
    t2 = (t2 & 0xFFFFFF00U) | ((((t2 & 0xFFU) + 1)) & 0xFFU);
    if ((t2 & 0xFFU) < 0x0CU) { _pc = 14; continue; }
    _pc = 15;
    continue;
        case 15:
    /* loc_4156B9: */
    t0 = 0;
    t0 = t2 & 0xFFU;
    t0 = (t0 + t1) & U32MASK;
    if ((unsigned int)M8((t0 + 1) & U32MASK) >= 6U) { _pc = 16; continue; }
    M8((t0 + 1) & U32MASK) = 2;
    M8((t0 + 0x0E) & U32MASK) = (unsigned char)((t2 >> 8) & 0xFFU);
    _pc = 16;
    continue;
        case 16:
    /* loc_4156CC: */
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)M8((t1 + 0x1B) & U32MASK);
    t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0x3FU);
    t4 = t0 & 0xFFU;
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)M8((t1 + 0x1B) & U32MASK);
    t4 = (unsigned int)(((int)t4) >> 1);
    t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xC0U);
    t0 &= 0xFFU;
    t0 = (t0 + t4) & U32MASK;
    M8((t1 + 0x1B) & U32MASK) = (unsigned char)(t0 & 0xFFU);
    _pc = 17;
    continue;
        case 17:
    /* loc_4156E5: */
    if ((t2 & 0xFFU) == 0) { _pc = 18; continue; }
    t0 = 0;
    t0 = t2 & 0xFFU;
    t0 = (t0 + t1) & U32MASK;
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8((t0 + 1) & U32MASK);
    t2 = (t2 & 0xFFFF00FFU) | (((unsigned int)M8(t0)) << 8);
    if ((t3 & 0xFFU) < ((t2 >> 8) & 0xFFU)) { _pc = 18; continue; }
    t3 = (t3 & 0xFFFF00FFU) | (((t2 >> 8) & 0xFFU) << 8);
    M8(t0) = (unsigned char)(t3 & 0xFFU);
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8((t0 + 0x0E) & U32MASK);
    t2 = (t2 & 0xFFFF00FFU) | (((unsigned int)M8((t0 + 0x0D) & U32MASK)) << 8);
    M8((t0 + 0x0D) & U32MASK) = (unsigned char)(t3 & 0xFFU);
    M8((t0 + 1) & U32MASK) = (unsigned char)((t3 >> 8) & 0xFFU);
    M8((t0 + 0x0E) & U32MASK) = (unsigned char)((t2 >> 8) & 0xFFU);
    t2 = (t2 & 0xFFFFFF00U) | ((((t2 & 0xFFU) - 1)) & 0xFFU);
    _pc = 17;
    continue;
        case 18:
    /* loc_41570F: */
    t5 = D3U_TBL_444250_32(&c->st, 0);
    t5 = (t5 + 2) & U32MASK;
    D3U_FLAG_444EB7_8(&c->st, 0) = (unsigned char)(t2 & 0xFFU);
    D3U_TBL_444250_32(&c->st, 0) = t5;
    if (t5 <= 0x800U) { _pc = 24; continue; }
    t3 = D3U_TBL_444220_32(&c->st, 0);
    t0 = t5;
    t1 = D3U_TBL_444264_32(&c->st, 0);
    t0 >>= 1;
    t3 >>= 1;
    t1 >>= 1;
    D3U_TBL_444250_32(&c->st, 0) = t0;
    D3U_TBL_444220_32(&c->st, 0) = t3;
    D3U_TBL_444264_32(&c->st, 0) = t1;
    t2 = (t2 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EB7_8(&c->st, 0);
    /* retn (early) */
    return;
        case 19:
    /* loc_415762: */
    t1 = (t3 - 0x40000U) & U32MASK;
    t4 = t1;
    t0 = (uint32_t)D3U_BLK01_DICT_COPY_32(&c->st, 0);
    t4 = (t4 << 5) & U32MASK;
    t4 = (t4 + t0) & U32MASK;
    t0 &= 0xFFFF00FFU;
    D3U_TBL_444224_32(&c->st, 0) = t1;
    t0 &= 0xFFFFFF00U;
    t3 = (t3 & 0xFFFF00FFU) | (0x3FU << 8);
    _pc = 20;
    continue;
        case 20:
    /* loc_415780: */
    t1 = 0;
    t1 = t0 & 0xFFU;
    t1 = (t1 << 20) & U32MASK;
    t1 = (t1 + t4) & U32MASK;
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8(t1);
    t2 = (t2 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EB7_8(&c->st, 0);
    if ((t3 & 0xFFU) != 0) { _pc = 21; continue; }
    t0 = (t0 & 0xFFFF00FFU) | ((t0 & 0xFFU) << 8);
    _pc = 23;
    continue;
        case 21:
    /* loc_415799: */
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8((t1 + 0x1B) & U32MASK);
    t1 = 0;
    t1 = (t0 >> 8) & 0xFFU;
    t3 = (t3 & 0xFFFFFF00U) | ((((t3 & 0xFFU) & ((t3 >> 8) & 0xFFU))) & 0xFFU);
    t1 = (t1 << 20) & U32MASK;
    t5 = t3 & 0xFFU;
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8((t4 + t1 + 0x1B) & U32MASK);
    t3 = (t3 & 0xFFFFFF00U) | ((((t3 & 0xFFU) & ((t3 >> 8) & 0xFFU))) & 0xFFU);
    t1 = 0;
    t1 = t3 & 0xFFU;
    if ((int)t5 >= (int)t1) { _pc = 22; continue; }
    t0 = (t0 & 0xFFFF00FFU) | ((t0 & 0xFFU) << 8);
    _pc = 22;
    continue;
        case 22:
    /* loc_4157B8: */
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) + 1)) & 0xFFU);
    D3U_FLAG_444EB7_8(&c->st, 0) = (unsigned char)(t2 & 0xFFU);
    if ((t0 & 0xFFU) < 8U) { _pc = 20; continue; }
    _pc = 23;
    continue;
        case 23:
    /* loc_4157C4: */
    t0 = (t0 & 0xFFFFFF00U) | ((t0 >> 8) & 0xFFU);
    t0 &= 0xFFU;
    t0 = (t0 << 20) & U32MASK;
    t1 = 0x20U;
    t4 = (t4 + t0) & U32MASK;
    t3 = 0;
    t0 = t4;
    d3uc_fill(c, t1, t0, t3);
    M8(t4) = 1;
    M8((t4 + 1) & U32MASK) = 1;
    t0 = D3U_TBL_444240_32(&c->st, 0);
    M8((t4 + 0x0E) & U32MASK) = (unsigned char)((t2 >> 8) & 0xFFU);
    d3u_st32(c, (uint32_t)((t4 + 0x1C) & U32MASK), t0);
    _pc = 24;
    continue;
        case 24:
    /* loc_4157F0: */
    t2 = (t2 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EB7_8(&c->st, 0);
    /* retn */
    ;
        default:
            break;
        }
        break;
    }
}
/* sub_415800: entry al = symbol. Order-model update; calls sub_40C9C0. */
void d3uc_update_order_model_b(dec3w_ctx *c, unsigned int eax_v)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int t5 = 0;
    unsigned int t6 = 0;
    unsigned int adr = 0;
    /* t2 high = entry garbage; only cl/ch lanes observed below */
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t2 = (unsigned int)D3U_FLAG_444EB8_8(&c->st, 0);
    t2 = (t2 & 0xFFFF00FFU) | ((eax_v & 0xFFU) << 8);
    t1 = D3U_TBL_444238_32(&c->st, 0);
    t0 = (t1 << 2) & U32MASK;
    t0 = (t0 + t1) & U32MASK;
    t4 = (uint32_t)D3U_BLK02_MODEL_A_32(&c->st, 0);
    t0 = (t0 << 2) & U32MASK;
    t4 = (t4 + t0) & U32MASK;
    if (M8(t4) == 0) { _pc = 13; continue; }
    if ((t2 & 0xFFU) != 0xFFU) { _pc = 2; continue; }
    t2 &= 0xFFFFFF00U;
    _pc = 1;
    continue;
        case 1:
    /* loc_415838: */
    t0 = 0;
    t0 = t2 & 0xFFU;
    if (((t2 >> 8) & 0xFFU) == (unsigned int)M8((t4 + t0 + 9) & U32MASK)) { _pc = 2; continue; }
    t2 = (t2 & 0xFFFFFF00U) | ((((t2 & 0xFFU) + 1)) & 0xFFU);
    if ((t2 & 0xFFU) < 8U) { _pc = 1; continue; }
    _pc = 2;
    continue;
        case 2:
    /* loc_415849: */
    if (D3U_FLAG_444EB5_8(&c->st, 0) == 0) { _pc = 5; continue; }
    if ((t2 & 0xFFU) >= 8U) { _pc = 3; continue; }
    t0 = 0;
    t1 &= 0xFFFFFF00U;
    t0 = t2 & 0xFFU;
    D3U_FLAG_444EB2_8(&c->st, 0) = (unsigned char)(t1 & 0xFFU);
    D3U_FLAG_444EAE_8(&c->st, 0) = (unsigned char)(t1 & 0xFFU);
    t0 = (unsigned int)M8((t4 + t0 + 1) & U32MASK);
    t0 &= 0xFFU;
    _pc = 4;
    continue;
        case 3:
    /* loc_415874: */
    t0 = 0;
    t0 = (unsigned int)M8(t4);
    _pc = 4;
    continue;
        case 4:
    /* loc_415878: */
    t1 = D3U_TBL_444248_32(&c->st, 0);
    t0 = (t0 << 14) & U32MASK;
    t3 = 0;
    /* x86 div faults on div0; direct / % below; remainder discarded */
    t0 = t0 / t1;
    t0 = (t0 & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)((D3U_U16_CUM_DST + (t0 << 1)) & U32MASK));
    t5 = D3U_TBL_444204_32(&c->st, 0);
    t0 &= 0xFFFFU;
    t5 = (t5 + t0) & U32MASK;
    D3U_TBL_444204_32(&c->st, 0) = t5;
    _pc = 5;
    continue;
        case 5:
    /* loc_4158A0: */
    t1 = 0;
    t1 = (unsigned int)M8(t4);
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)M8((t4 + 0x12) & U32MASK);
    if ((t0 & 0xFFU) <= (unsigned int)d3u_t429CB8[(unsigned int)(t1 & 0xFFU)]) { _pc = 7; continue; }
    t0 = (t4 + 8) & U32MASK;
    M8((t4 + 0x12) & U32MASK) = 0;
    t1 = (t4 - 1) & U32MASK;
    _pc = 6;
    continue;
        case 6:
    /* loc_4158B9: */
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8(t0);
    t3 = (t3 & 0xFFFFFF00U) | ((((t3 & 0xFFU) + 1)) & 0xFFU);
    M8(t0) = (unsigned char)(t3 & 0xFFU);
    t3 = (t3 & 0xFFFF00FFU) | ((t3 & 0xFFU) << 8);
    t3 = (t3 & 0xFFFF00FFU) | (((((t3 >> 8) & 0xFFU) >> 1)) << 8);
    M8(t0) = (unsigned char)((t3 >> 8) & 0xFFU);
    t3 = (t3 & 0xFFFFFF00U) | ((t3 >> 8) & 0xFFU);
    t3 = (t3 & 0xFFFF00FFU) | (((unsigned int)M8((t4 + 0x12) & U32MASK)) << 8);
    t3 = (t3 & 0xFFFF00FFU) | ((((((t3 >> 8) & 0xFFU) + (t3 & 0xFFU)) & 0xFFU)) << 8);
    t0 = (t0 - 1) & U32MASK;
    M8((t4 + 0x12) & U32MASK) = (unsigned char)((t3 >> 8) & 0xFFU);
    if (t0 != t1) { _pc = 6; continue; }
    _pc = 7;
    continue;
        case 7:
    /* loc_4158D4: */
    M8((t4 + 0x12) & U32MASK) = (unsigned char)(((unsigned int)M8((t4 + 0x12) & U32MASK) + 4) & 0xFFU);
    if ((t2 & 0xFFU) >= 8U) { _pc = 8; continue; }
    t1 = D3U_TBL_44424C_32(&c->st, 0);
    t0 = (t1 << 2) & U32MASK;
    t0 = (t0 - t1) & U32MASK;
    t0 >>= 2;
    D3U_TBL_44424C_32(&c->st, 0) = t0;
    t0 = 0;
    t0 = (unsigned int)M8((t4 + 0x11) & U32MASK);
    t0 = (unsigned int)(((int)t0) >> 1);
    M8((t4 + 0x11) & U32MASK) = (unsigned char)(t0 & 0xFFU);
    t0 = 0;
    t0 = t2 & 0xFFU;
    adr = (t4 + t0 + 1) & U32MASK;
    M8(adr) = (unsigned char)(((unsigned int)M8(adr) + 4) & 0xFFU);
    D3U_TBL_444220_32(&c->st, 0) = (D3U_TBL_444220_32(&c->st, 0) + 5) & U32MASK;
    _pc = 11;
    continue;
        case 8:
    /* loc_415910: */
    t6 = D3U_TBL_44424C_32(&c->st, 0);
    t1 = (t1 & 0xFFFF00FFU) | (((unsigned int)M8((t4 + 0x11) & U32MASK)) << 8);
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)M8(t4);
    t2 &= 0xFFFFFF00U;
    t6 = (t6 + 1) & U32MASK;
    t1 = (t1 & 0xFFFF00FFU) | (((((t1 >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) + 4)) & 0xFFU);
    M8((t4 + 0x11) & U32MASK) = (unsigned char)((t1 >> 8) & 0xFFU);
    D3U_TBL_44424C_32(&c->st, 0) = t6;
    M8(t4) = (unsigned char)(t0 & 0xFFU);
    _pc = 9;
    continue;
        case 9:
    /* loc_41592D: */
    t0 = 0;
    t0 = t2 & 0xFFU;
    if (M8((t4 + t0 + 1) & U32MASK) == 0) { _pc = 10; continue; }
    t2 = (t2 & 0xFFFFFF00U) | ((((t2 & 0xFFU) + 1)) & 0xFFU);
    if ((t2 & 0xFFU) < 7U) { _pc = 9; continue; }
    _pc = 10;
    continue;
        case 10:
    /* loc_41593F: */
    t0 = 0;
    t0 = t2 & 0xFFU;
    t0 = (t0 + t4) & U32MASK;
    t1 = (t1 & 0xFFFFFF00U) | (unsigned int)M8((t0 + 1) & U32MASK);
    if ((t1 & 0xFFU) >= 0x0CU) { _pc = 11; continue; }
    t3 = (t3 & 0xFFFFFF00U) | 4U;
    t3 = (t3 & 0xFFFFFF00U) | ((((t3 & 0xFFU) - (t1 & 0xFFU))) & 0xFFU);
    M8((t4 + 0x12) & U32MASK) = (unsigned char)(((unsigned int)M8((t4 + 0x12) & U32MASK) + (t3 & 0xFFU)) & 0xFFU);
    M8((t0 + 1) & U32MASK) = 4;
    M8((t0 + 9) & U32MASK) = (unsigned char)((t2 >> 8) & 0xFFU);
    _pc = 11;
    continue;
        case 11:
    /* loc_41595B: */
    if ((t2 & 0xFFU) == 0) { _pc = 12; continue; }
    t0 = 0;
    t0 = t2 & 0xFFU;
    t0 = (t0 + t4) & U32MASK;
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8((t0 + 1) & U32MASK);
    t1 = (t1 & 0xFFFFFF00U) | (unsigned int)M8(t0);
    if ((t3 & 0xFFU) < (t1 & 0xFFU)) { _pc = 12; continue; }
    t3 = (t3 & 0xFFFF00FFU) | ((t1 & 0xFFU) << 8);
    M8(t0) = (unsigned char)(t3 & 0xFFU);
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)M8((t0 + 9) & U32MASK);
    t1 = (t1 & 0xFFFFFF00U) | (unsigned int)M8((t0 + 8) & U32MASK);
    M8((t0 + 8) & U32MASK) = (unsigned char)(t3 & 0xFFU);
    M8((t0 + 1) & U32MASK) = (unsigned char)((t3 >> 8) & 0xFFU);
    t2 = (t2 & 0xFFFFFF00U) | ((((t2 & 0xFFU) - 1)) & 0xFFU);
    M8((t0 + 9) & U32MASK) = (unsigned char)(t1 & 0xFFU);
    _pc = 11;
    continue;
        case 12:
    /* loc_415985: */
    t3 = D3U_TBL_444250_32(&c->st, 0);
    t3 = (t3 + 2) & U32MASK;
    D3U_FLAG_444EB8_8(&c->st, 0) = (unsigned char)(t2 & 0xFFU);
    D3U_TBL_444250_32(&c->st, 0) = t3;
    if (t3 <= 0x800U) { _pc = 14; continue; }
    t5 = D3U_TBL_444220_32(&c->st, 0);
    t4 = t3;
    t6 = D3U_TBL_444264_32(&c->st, 0);
    t4 >>= 1;
    t5 >>= 1;
    t6 >>= 1;
    D3U_TBL_444250_32(&c->st, 0) = t4;
    D3U_TBL_444220_32(&c->st, 0) = t5;
    D3U_TBL_444264_32(&c->st, 0) = t6;
    _pc = 14;
    continue;
        case 13:
    /* loc_4159CA: */
    t1 = 0x14U;
    t0 = t4;
    t3 = 0;
    d3uc_fill(c, t1, t0, t3);
    M8(t4) = 3;
    M8((t4 + 1) & U32MASK) = 3;
    M8((t4 + 0x12) & U32MASK) = 6;
    M8((t4 + 9) & U32MASK) = (unsigned char)((t2 >> 8) & 0xFFU);
    _pc = 14;
    continue;
        case 14:
    /* loc_4159E6: */
    t2 = (t2 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EB8_8(&c->st, 0);
    /* retn */
    ;
        default:
            break;
        }
        break;
    }
}
/* sub_4159F4: entry al = symbol (bh), dl = flag. Calls sub_41B69C/41B318. */
void d3uc_update_match_chain(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int t5 = 0;
    unsigned int t6 = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t5 = D3U_TBL_444210_32(&c->st, 0);
    /* t1 high-16 = entry garbage; only bl/bh lanes observed below */
    t1 = (t1 & 0xFFFF00FFU) | ((eax_v & 0xFFU) << 8);
    t3 = edx_v;
    if ((t3 & 0xFFU) == 0) { _pc = 3; continue; }
    t1 = (t1 & 0xFFFFFF00U) | (unsigned int)D3U_TBL_4441C0_8(&c->st, 0);
    t0 = 0;
    t1 = (t1 & 0xFFFFFF00U) | ((t1 & 0xFFU) & 1U);
    t3 = D3U_FLAG_444E7E;
    t0 = t1 & 0xFFU;
    t3 = (t3 + t0) & U32MASK;
    t2 = (t2 & 0xFFFFFF00U) | (unsigned int)M8(t3);
    t2 = (t2 & 0xFFFFFF00U) | ((((t2 & 0xFFU) + (t2 & 0xFFU))) & 0xFFU);
    t2 = (t2 & 0xFFFFFF00U) | ((t2 & 0xFFU) & 0x0FU);
    t0 = (t0 + D3U_FLAG_444E80) & U32MASK;
    M8(t3) = (unsigned char)(t2 & 0xFFU);
    t2 = (t2 & 0xFFFFFF00U) | (unsigned int)M8(t0);
    t2 = (t2 & 0xFFFFFF00U) | ((((t2 & 0xFFU) + (t2 & 0xFFU))) & 0xFFU);
    t2 = (t2 & 0xFFFFFF00U) | ((t2 & 0xFFU) & 0x0FU);
    M8(t0) = (unsigned char)(t2 & 0xFFU);
    t2 &= 0xFFFFFF00U;
    D3U_FLAG_444EAF_8(&c->st, 0) = 0;
    if (((t1 >> 8) & 0xFFU) < 0x80U) { _pc = 3; continue; }
    if (((t1 >> 8) & 0xFFU) != 0xFFU) { _pc = 1; continue; }
    M8(t0) = (unsigned char)(((unsigned int)M8(t0) + 1) & 0xFFU);
    t3 = (t3 & 0xFFFFFF00U) | ((((t1 & 0xFFU) == 0)) ? 1U : 0U);
    t3 &= 0xFFU;
    t1 = (t1 & 0xFFFFFF00U) | (unsigned int)G8((D3U_FLAG_444E7E + t3) & U32MASK);
    if ((t1 & 0xFFU) != 0x0FU) { _pc = 3; continue; }
    if ((t1 & 0xFFU) != (unsigned int)M8(t0)) { _pc = 3; continue; }
    D3U_FLAG_444EAF_8(&c->st, 0) = 2;
    _pc = 3;
    continue;
        case 1:
    /* loc_415A6F: */
    if (((t1 >> 8) & 0xFFU) <= 0xDFU) { _pc = 2; continue; }
    if (((t1 >> 8) & 0xFFU) < 0xF2U) { _pc = 3; continue; }
    if (((t1 >> 8) & 0xFFU) > 0xF6U) { _pc = 3; continue; }
    _pc = 2;
    continue;
        case 2:
    /* loc_415A7E: */
    t2 = (t2 & 0xFFFF00FFU) | (((unsigned int)M8(t3)) << 8);
    t2 = (t2 & 0xFFFF00FFU) | (((((t2 >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    M8(t3) = (unsigned char)((t2 >> 8) & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | ((((t1 & 0xFFU) == 0)) ? 1U : 0U);
    t0 &= 0xFFU;
    t1 = (t1 & 0xFFFFFF00U) | (unsigned int)G8((D3U_FLAG_444E80 + t0) & U32MASK);
    if ((t1 & 0xFFU) != 0x0FU) { _pc = 3; continue; }
    if (((t2 >> 8) & 0xFFU) != (t1 & 0xFFU)) { _pc = 3; continue; }
    D3U_FLAG_444EAF_8(&c->st, 0) = 1;
    _pc = 3;
    continue;
        case 3:
    /* loc_415AA4: */
    if (((t1 >> 8) & 0xFFU) == 0) { _pc = 9; continue; }
    if (((t1 >> 8) & 0xFFU) == 0xFFU) { _pc = 9; continue; }
    t0 = 0;
    t0 = (t1 >> 8) & 0xFFU;
    t3 = (t0 << 2) & U32MASK;
    t0 = D3U_DICT_CURSOR_32(&c->st, 0);
    t2 = D3U_TBL_443360_32(&c->st, t3);
    t4 = D3U_DICT_MASK_32(&c->st, 0);
    t0 = (t0 - t2) & U32MASK;
    t0 &= t4;
    if (t0 < 0x100U) { _pc = 4; continue; }
    t0 = (t0 & 0xFFFFFF00U) | ((t1 >> 8) & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) - 1)) & 0xFFU);
    t0 &= 0xFFU;
    t3 = (t0 << 2) & U32MASK;
    t0 = D3U_DICT_CURSOR_32(&c->st, 0);
    t0 = (t0 - D3U_TBL_443360_32(&c->st, t3)) & U32MASK;
    t0 &= t4;
    if (t0 < 0x100U) { _pc = 4; continue; }
    t0 = (t0 & 0xFFFFFF00U) | ((t1 >> 8) & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) + 1)) & 0xFFU);
    t0 &= 0xFFU;
    t3 = (t0 << 2) & U32MASK;
    t0 = D3U_DICT_CURSOR_32(&c->st, 0);
    t0 = (t0 - D3U_TBL_443360_32(&c->st, t3)) & U32MASK;
    t0 &= t4;
    _pc = 4;
    continue;
        case 4:
    /* loc_415B1D: */
    if (t0 == 0) { _pc = 8; continue; }
    if (t0 >= 0x100U) { _pc = 8; continue; }
    t6 = (t0 << 2) & U32MASK;
    t2 = (uint32_t)D3U_TBL_442128_32(&c->st, t6);
    t3 = 0;
    t2 = (t2 + 1) & U32MASK;
    t3 = (unsigned int)D3U_FLAG_444EBA_8(&c->st, 0);
    D3U_TBL_442128_32(&c->st, t6) = t2;
    if (t0 == t3) { _pc = 6; continue; }
    t1 = (t1 & 0xFFFFFF00U) | (unsigned int)G8((D3U_TBL_442D58 + t0) & U32MASK);
    _pc = 5;
    continue;
        case 5:
    /* loc_415B56: */
    t3 = 0;
    t3 = t1 & 0xFFU;
    t2 = (uint32_t)D3U_TBL_442128_32(&c->st, t6);
    if (t2 < (uint32_t)D3U_TBL_442128_32(&c->st, (t3 << 2))) { _pc = 6; continue; }
    t2 = (t2 & 0xFFFFFF00U) | (unsigned int)G8((D3U_TBL_442D58 + t3) & U32MASK);
    G8((D3U_TBL_442D58 + t0) & U32MASK) = (unsigned char)(t2 & 0xFFU);
    t2 = (t2 & 0xFFFFFF00U) | (unsigned int)G8((D3U_TBL_443260 + t0) & U32MASK);
    G8((D3U_TBL_443260 + t3) & U32MASK) = (unsigned char)(t2 & 0xFFU);
    G8((D3U_TBL_442D58 + t3) & U32MASK) = (unsigned char)(t0 & 0xFFU);
    G8((D3U_TBL_443260 + t0) & U32MASK) = (unsigned char)(t1 & 0xFFU);
    t1 = (t1 & 0xFFFFFF00U) | (unsigned int)G8((D3U_TBL_442D58 + t0) & U32MASK);
    if ((t1 & 0xFFU) != 0) { _pc = 5; continue; }
    D3U_FLAG_444EBA_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    if ((t0 & 0xFFU) <= 1U) { _pc = 6; continue; }
    D3U_FLAG_444EB9_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    _pc = 6;
    continue;
        case 6:
    /* loc_415BA5: */
    t5 = (t5 + 1) & U32MASK;
    if (t5 <= 0x100U) { _pc = 8; continue; }
    t5 = 0;
    t0 = 0;
    _pc = 7;
    continue;
        case 7:
    /* loc_415BB2: */
    t6 = (uint32_t)D3U_TBL_442128_32(&c->st, t0);
    t6 >>= 1;
    t0 = (t0 + 4) & U32MASK;
    t5 = (t5 + t6) & U32MASK;
    D3U_TBL_442124_32(&c->st, t0) = t6;
    if (t0 != 0x400U) { _pc = 7; continue; }
    _pc = 8;
    continue;
        case 8:
    /* loc_415BCC: */
    t0 = 0;
    t0 = (t1 >> 8) & 0xFFU;
    t3 = (t0 << 2) & U32MASK;
    t0 = D3U_DICT_CURSOR_32(&c->st, 0);
    D3U_TBL_443360_32(&c->st, t3) = t0;
    _pc = 9;
    continue;
        case 9:
    /* loc_415BE2: */
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA9_8(&c->st, 0);
    D3U_TBL_444210_32(&c->st, 0) = t5;
    if ((t0 & 0xFFU) == 0) { _pc = 20; continue; }
    t0 = (t0 & 0xFFFFFF00U) | ((t1 >> 8) & 0xFFU);
    t0 = (t0 & 0xFFFF00FFU) | (((unsigned int)D3U_FLAG_444E9D_8(&c->st, 0)) << 8);
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA8_8(&c->st, 0);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) - ((t0 >> 8) & 0xFFU))) & 0xFFU);
    if ((t3 & 0xFFU) == 0) { _pc = 10; continue; }
    t0 = (t0 & 0xFFFFFF00U) | (((0U - (t0 & 0xFFU))) & 0xFFU);
    _pc = 10;
    continue;
        case 10:
    /* loc_415C0B: */
    if ((t0 & 0xFFU) < 0x80U) { _pc = 11; continue; }
    t3 = (t3 & 0xFFFF00FFU) | (((unsigned int)D3U_FLAG_444EA8_8(&c->st, 0)) << 8);
    t0 = (t0 & 0xFFFF00FFU) | ((((((t3 >> 8) & 0xFFU) == 0)) ? 1U : 0U) << 8);
    D3U_FLAG_444EA8_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFU);
    _pc = 11;
    continue;
        case 11:
    /* loc_415C20: */
    t1 = (t1 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA9_8(&c->st, 0);
    if ((t1 & 0xFFU) != 1U) { _pc = 12; continue; }
    t3 = D3U_TBL_4441C4_32(&c->st, 0);
    t3 = (t3 - 0x203U) & U32MASK;
    t3 &= 0x0FU;
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)d3u_t429E44[(unsigned int)(t3 & 0x0FU)];
    t3 &= 0xFFU;
    t3 = (t3 << 4) & U32MASK;
    t0 &= 0xFFU;
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)d3u_t429E54[(unsigned int)(t0 & 0xFFU)];
    t0 &= 0xFFU;
    t0 = (t0 + t3) & U32MASK;
    t0 = (t0 + 0x203U) & U32MASK;
    D3U_TBL_444210_32(&c->st, 0) = t5;
    D3U_TBL_4441C4_32(&c->st, 0) = t0;
    _pc = 20;
    continue;
        case 12:
    /* loc_415C70: */
    t3 = D3U_TBL_4441D0_32(&c->st, 0);
    t3 = (t3 << 4) & U32MASK;
    if ((t1 & 0xFFU) != 2U) { _pc = 13; continue; }
    t0 &= 0xFFU;
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)d3u_t429E54[(unsigned int)(t0 & 0xFFU)];
    t0 &= 0xFFU;
    t3 = (t3 + t0) & U32MASK;
    t3 = (t3 + 0x223U) & U32MASK;
    D3U_TBL_444210_32(&c->st, 0) = t5;
    D3U_TBL_4441C4_32(&c->st, 0) = t3;
    _pc = 20;
    continue;
        case 13:
    /* loc_415CA7: */
    if ((t1 & 0xFFU) != 3U) { _pc = 18; continue; }
    if (D3U_FLAG_444E9B_8(&c->st, 0) == 0) { _pc = 15; continue; }
    t3 = D3U_TBL_444258_32(&c->st, 0);
    t0 = 0;
    t3 >>= 16;
    t0 = (t1 >> 8) & 0xFFU;
    t0 = (t0 - t3) & U32MASK;
    t0 = (t0 & 0xFFFF00FFU) | (((unsigned int)D3U_FLAG_444E9F_8(&c->st, 0)) << 8);
    D3U_FLAG_444EA8_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFU);
    if (((t0 >> 8) & 0xFFU) == 0) { _pc = 14; continue; }
    t0 = (t0 & 0xFFFFFF00U) | (((0U - (t0 & 0xFFU))) & 0xFFU);
    _pc = 14;
    continue;
        case 14:
    /* loc_415CDA: */
    if ((t0 & 0xFFU) < 0x80U) { _pc = 15; continue; }
    t1 = (t1 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA8_8(&c->st, 0);
    t0 = (t0 & 0xFFFF00FFU) | (((((t1 & 0xFFU) == 0)) ? 1U : 0U) << 8);
    D3U_FLAG_444EA8_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFU);
    _pc = 15;
    continue;
        case 15:
    /* loc_415CEF: */
    t2 = 0;
    t2 = (unsigned int)D3U_FLAG_444E9C_8(&c->st, 0);
    t0 = (t0 & 0xFFFF00FFU) | (((unsigned int)G8((D3U_TBL_444284 + t2) & U32MASK)) << 8);
    t3 = (t2 << 2) & U32MASK;
    if (((t1 >> 8) & 0xFFU) != ((t0 >> 8) & 0xFFU)) { _pc = 16; continue; }
    D3U_TBL_440AF0_32(&c->st, t3) = ((uint32_t)D3U_TBL_440AF0_32(&c->st, t3) + 1) & U32MASK;
    _pc = 17;
    continue;
        case 16:
    /* loc_415D10: */
    D3U_TBL_440AF0_32(&c->st, t3) = (uint32_t)D3U_TBL_440AF0_32(&c->st, t3) >> 1;
    _pc = 17;
    continue;
        case 17:
    /* loc_415D16: */
    t0 &= 0xFFU;
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)d3u_t429F54[(unsigned int)(t0 & 0xFFU)];
    t3 = D3U_TBL_4441D0_32(&c->st, 0);
    t0 &= 0xFFU;
    t3 = (t3 << 5) & U32MASK;
    t0 = (t0 + t3) & U32MASK;
    t0 = (t0 + 0x243U) & U32MASK;
    D3U_TBL_444210_32(&c->st, 0) = t5;
    D3U_TBL_4441C4_32(&c->st, 0) = t0;
    _pc = 20;
    continue;
        case 18:
    /* loc_415D43: */
    D3U_TBL_444210_32(&c->st, 0) = t5;
    if ((t1 & 0xFFU) != 4U) { _pc = 19; continue; }
    t0 &= 0xFFU;
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)d3u_t429E54[(unsigned int)(t0 & 0xFFU)];
    t0 &= 0xFFU;
    t0 = (t0 + t3) & U32MASK;
    t0 = (t0 + 0x2A3U) & U32MASK;
    D3U_TBL_4441C4_32(&c->st, 0) = t0;
    _pc = 20;
    continue;
        case 19:
    /* loc_415D6C: */
    t3 = 0;
    t3 = t0 & 0xFFU;
    t0 = 0;
    t0 = (t1 >> 8) & 0xFFU;
    d3uc_walk_hash_chain(c, t0, t3);
    _pc = 20;
    continue;
        case 20:
    /* loc_415D79: */
    t6 = D3U_TBL_4441C0_32(&c->st, 0);
    t5 = D3U_TBL_444210_32(&c->st, 0);
    t6 = (t6 + 1) & U32MASK;
    t3 = (t3 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA0_8(&c->st, 0);
    D3U_TBL_4441C0_32(&c->st, 0) = t6;
    if ((t3 & 0xFFU) != 0) { _pc = 21; continue; }
    if (((t1 >> 8) & 0xFFU) < 4U) { _pc = 21; continue; }
    if (((t1 >> 8) & 0xFFU) > 6U) { _pc = 21; continue; }
    if (D3U_TBL_444258_8(&c->st, 0) == 0xDFU) { _pc = 22; continue; }
    t3 = D3U_TBL_444260_32(&c->st, 0);
    t0 = D3U_TBL_44425C_32(&c->st, 0);
    t3 = (t3 << 8) & U32MASK;
    t0 >>= 24;
    t3 = (t3 + t0) & U32MASK;
    t0 = D3U_TBL_44425C_32(&c->st, 0);
    D3U_TBL_444260_32(&c->st, 0) = t3;
    t3 = D3U_TBL_444258_32(&c->st, 0);
    t0 = (t0 << 8) & U32MASK;
    t3 >>= 24;
    t0 = (t0 + t3) & U32MASK;
    D3U_TBL_44425C_32(&c->st, 0) = t0;
    t0 = D3U_TBL_444258_32(&c->st, 0);
    t0 = (t0 << 8) & U32MASK;
    t0 = (t0 + 0xDFU) & U32MASK;
    D3U_TBL_444258_32(&c->st, 0) = t0;
    _pc = 22;
    continue;
        case 21:
    /* loc_415DF2: */
    t3 = D3U_TBL_444260_32(&c->st, 0);
    t0 = D3U_TBL_44425C_32(&c->st, 0);
    t3 = (t3 << 8) & U32MASK;
    t0 >>= 24;
    t3 = (t3 + t0) & U32MASK;
    t0 = D3U_TBL_44425C_32(&c->st, 0);
    D3U_TBL_444260_32(&c->st, 0) = t3;
    t3 = D3U_TBL_444258_32(&c->st, 0);
    t0 = (t0 << 8) & U32MASK;
    t3 >>= 24;
    t0 = (t0 + t3) & U32MASK;
    t3 = D3U_TBL_444258_32(&c->st, 0);
    D3U_TBL_44425C_32(&c->st, 0) = t0;
    t0 = 0;
    t3 = (t3 << 8) & U32MASK;
    t0 = (t1 >> 8) & 0xFFU;
    t3 = (t3 + t0) & U32MASK;
    D3U_TBL_444258_32(&c->st, 0) = t3;
    _pc = 22;
    continue;
        case 22:
    /* loc_415E38: */
    t1 = (t1 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA9_8(&c->st, 0);
    D3U_TBL_444210_32(&c->st, 0) = t5;
    if ((t1 & 0xFFU) == 0) { _pc = 28; continue; }
    t0 = D3U_TBL_4441C0_32(&c->st, 0);
    t1 = D3U_TBL_4441C8_32(&c->st, 0);
    t3 = 0;
    /* x86 div faults on div0; direct / % below */
    t3 = t0 % t1;
    t0 = t0 / t1;
    t6 = D3U_TBL_4441D0_32(&c->st, 0);
    t2 = (t2 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444E9E_8(&c->st, 0);
    t0 = D3U_TBL_444258_32(&c->st, 0);
    t1 = (t1 & 0xFFFF00FFU) | (((unsigned int)D3U_FLAG_444EA9_8(&c->st, 0)) << 8);
    /* x86 masks the count to 5 bits; direct shift below */
    t0 >>= ((t2 & 0xFFU) & 31U);
    D3U_TBL_4441D0_32(&c->st, 0) = t3;
    D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    if (((t1 >> 8) & 0xFFU) != 3U) { _pc = 27; continue; }
    t2 = 6;
    t0 = D3U_TBL_4441C0_32(&c->st, 0);
    t3 = 0;
    /* x86 div: divisor is 6 (nonzero); direct / % below */
    t3 = t0 % 6U;
    t0 = t0 / 6U;
    D3U_FLAG_444E9C_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
    t3 = 0;
    t3 = (unsigned int)D3U_FLAG_444E9C_8(&c->st, 0);
    t0 = (t0 & 0xFFFFFF00U) | ((D3U_TBL_440AF0_32(&c->st, (t3 << 2)) > 8U) ? 1U : 0U);
    t2 = D3U_TBL_444258_32(&c->st, 0);
    t4 = (unsigned int)D3U_TBL_444258_8(&c->st, 0);
    t2 >>= 24;
    t4 = (t4 - t2) & U32MASK;
    t2 = 0;
    t2 = (unsigned int)D3U_FLAG_444E9D_8(&c->st, 0);
    t2 = (t2 + t4) & U32MASK;
    t1 = (t1 & 0xFFFFFF00U) | (t0 & 0xFFU);
    G8((D3U_TBL_444284 + t3) & U32MASK) = (unsigned char)(t2 & 0xFFU);
    if ((t0 & 0xFFU) == 0) { _pc = 23; continue; }
    if (D3U_TBL_4441B4_32(&c->st, 0) >= 5U) { _pc = 23; continue; }
    t0 = (t0 & 0xFFFFFF00U) | 1U;
    _pc = 24;
    continue;
        case 23:
    /* loc_415EE4: */
    t0 &= 0xFFFFFF00U;
    _pc = 24;
    continue;
        case 24:
    /* loc_415EE6: */
    D3U_FLAG_444E9B_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    if ((t0 & 0xFFU) == 0) { _pc = 25; continue; }
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA8_8(&c->st, 0);
    D3U_FLAG_444E9F_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    t0 = t6;
    t0 = (t0 << 5) & U32MASK;
    t0 = (t0 + 0x253U) & U32MASK;
    D3U_TBL_4441C4_32(&c->st, 0) = t0;
    t0 = 0;
    t0 = (unsigned int)D3U_FLAG_444E9C_8(&c->st, 0);
    t2 &= 0xFFFF00FFU;
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)G8((D3U_TBL_444284 + (t0 & 0xFFU)) & U32MASK);
    D3U_FLAG_444EA8_8(&c->st, 0) = 0;
    D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    _pc = 25;
    continue;
        case 25:
    /* loc_415F22: */
    if ((t1 & 0xFFU) == 0) { _pc = 26; continue; }
    t0 = D3U_TBL_4441B4_32(&c->st, 0);
    t0 = (t0 + 1) & U32MASK;
    D3U_TBL_444210_32(&c->st, 0) = t5;
    D3U_TBL_4441B4_32(&c->st, 0) = t0;
    _pc = 28;
    continue;
        case 26:
    /* loc_415F39: */
    t6 = 0;
    D3U_TBL_444210_32(&c->st, 0) = t5;
    D3U_TBL_4441B4_32(&c->st, 0) = t6;
    t5 = D3U_TBL_444210_32(&c->st, 0);
    /* retn (early) */
    return;
        case 27:
    /* loc_415F53: */
    if (((t1 >> 8) & 0xFFU) < 5U) { _pc = 28; continue; }
    d3uc_update_hash_bucket(c);
    _pc = 28;
    continue;
        case 28:
    /* loc_415F5D: */
    t5 = D3U_TBL_444210_32(&c->st, 0);
    /* retn */
    ;
        default:
            break;
        }
        break;
    }
}
/* sub_41855C: entry t0 = count, t3 = ctx id, bl = symbol. Coder-agnostic. */
void d3uc_update_ctx_counter(dec3w_ctx *c, unsigned int eax_v, unsigned int edx_v, unsigned int ebx_v)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int t5 = 0;
    t4 = eax_v;
    t2 = edx_v;
    t3 = 0;
    D3U_TBL_4441F4_32(&c->st, 0) = t3;
    t3 = ebx_v & 0xFFU;
    t1 = ebx_v;
    D3U_TBL_44421C_32(&c->st, 0) = t2;
    d3uc_update_ctx_coder(c, t4, t3);
    t3 = 0;
    t3 = (unsigned int)D3U_FLAG_444EB3_8(&c->st, 0);
    t5 = D3U_TBL_44421C_32(&c->st, 0);
    /* t0 high lanes = entry garbage; only ah observed below */
    t0 = (t0 & 0xFFFF00FFU) | ((t3 & 0xFFU) << 8);
    D3U_TBL_441F08_32(&c->st, (t3 << 2)) = t5;
    D3U_TBL_442018_32(&c->st, (t3 << 2)) = t4;
    t0 = (t0 & 0xFFFF00FFU) | (((((t0 >> 8) & 0xFFU) + 1) & 0xFFU) << 8);
    G8((D3U_TBL_4441D8 + t3) & U32MASK) = (unsigned char)(t1 & 0xFFU);
    D3U_FLAG_444EB3_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | ((t2 == 0x2FDU) ? 1U : 0U);
    t0 &= 0xFFU;
    t3 = D3U_TBL_443258_32(&c->st, (t0 << 2));
    t0 = t3;
    t0 = (t0 << 5) & U32MASK;
    t0 = (t0 - t3) & U32MASK;
    t3 = D3U_TBL_4441F4_32(&c->st, 0);
    t0 >>= 5;
    t3 = (t3 + t0) & U32MASK;
    t0 = (t0 & 0xFFFFFF00U) | ((t2 == 0x2FDU) ? 1U : 0U);
    t0 &= 0xFFU;
    D3U_TBL_443258_32(&c->st, (t0 << 2)) = t3;
    t0 = (t0 & 0xFFFFFF00U) | ((t2 == 0x2FDU) ? 1U : 0U);
    t3 = 0;
    t3 = t0 & 0xFFU;
    t3 = D3U_TBL_443B60_32(&c->st, (t3 << 2));
    t0 = t3;
    t0 = (t0 << 5) & U32MASK;
    t0 = (t0 - t3) & U32MASK;
    t3 = D3U_TBL_4441F4_32(&c->st, 0);
    t0 >>= 5;
    t3 = (t3 + t0) & U32MASK;
    t0 = (t0 & 0xFFFFFF00U) | ((t2 == 0x2FDU) ? 1U : 0U);
    t0 &= 0xFFU;
    D3U_TBL_443B60_32(&c->st, (t0 << 2)) = t3;
    /* retn */
}
/* sub_418620: entry t3 = ctx id, bl = symbol. Coder-agnostic. */
void d3uc_update_ctx_ema(dec3w_ctx *c, unsigned int edx_v, unsigned int ebx_v)
{
    unsigned int t0 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    D3U_TBL_44421C_32(&c->st, 0) = edx_v;
    t3 = 0;
    t2 = 0;
    t3 = ebx_v & 0xFFU;
    D3U_TBL_4441F4_32(&c->st, 0) = t2;
    /* 0x00418633 call sub_417D74.
     * eax_in is the dispatcher-leftover d3u_ld32(c, (uint32_t)(0x4441C4)): native 418620 never
     * writes t0, so 417d74 receives the caller's t0 (set at 0x00417421).
     * Passing 0 here starved the db3 loop and overheated the E1E0 EMA. */
    d3uc_update_ctx_coder(c, (D3U_TBL_4441C4_32(&c->st, 0) & U32MASK), t3);
    t3 = D3U_TBL_4441E0_32(&c->st, 0);
    t0 = t3;
    t0 = (t0 << 5) & U32MASK;
    t0 = (t0 - t3) & U32MASK;
    t3 = t0;
    t0 = D3U_TBL_4441F4_32(&c->st, 0);
    t3 >>= 5;
    t3 = (t3 + t0) & U32MASK;
    D3U_TBL_4441E0_32(&c->st, 0) = t3;
    /* retn */
}

/* ---- top-ups (from draft_t3.c) ---- */
/* sub_414D78: entry-live reg is t0 only (t1,t2,t3,t4,t5,t6 pushed).
 * Halves 32 dwords at heapF4+a1*512 ((w+0x10001)&0xFFFEFFFE)>>1, sums into
 * t6, copies 510B heapF4-block to heapF0-block+2, stores t6-folded word,
 * prefix-sums the block, bumps heapFC word if a1<0x2FD. Return (t0) unused
 * in this span, hence void. */
void d3uc_fold_ctx_block(dec3w_ctx *c, unsigned int eax_in)
{
    uint32_t t0 = 0;
    uint32_t t1 = 0;
    uint32_t t2 = 0;
    uint32_t t3 = 0;
    uint32_t t4 = 0;
    uint32_t t5 = 0;
    uint32_t t6 = 0;
    uint32_t a1 = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    a1 = (uint32_t)eax_in;
    t3 = a1;
    t1 = a1;
    t0 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t1 = (t1 << 9) & U32MASK;
    t6 = 0;
    t1 = (t1 + t0) & U32MASK;
    t3 = (t3 << 8) & U32MASK;
    t5 = (t1 + 0x200U) & U32MASK;
    _pc = 1;
    continue;
        case 1:
    /* l_414d9d: */
    t2 = (uint32_t)d3u_ld32(c, (uint32_t)(t1));
    t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 4U) & U32MASK));
    t2 = (t2 + 0x10001U) & U32MASK;
    t0 = (t0 + 0x10001U) & U32MASK;
    t2 &= 0xFFFEFFFEU;
    t0 &= 0xFFFEFFFEU;
    t2 >>= 1;
    t0 >>= 1;
    t1 = (t1 + 0x10U) & U32MASK;
    t4 = (t2 + t0) & U32MASK;
    d3u_st32(c, (uint32_t)((t1 - 0xCU) & U32MASK), (unsigned int)t0);
    d3u_st32(c, (uint32_t)((t1 - 0x10U) & U32MASK), (unsigned int)t2);
    t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 - 4U) & U32MASK));
    t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 - 8U) & U32MASK));
    t0 = (t0 + 0x10001U) & U32MASK;
    t2 = (t2 + 0x10001U) & U32MASK;
    t0 &= 0xFFFEFFFEU;
    t2 &= 0xFFFEFFFEU;
    t0 >>= 1;
    t2 >>= 1;
    t6 = (t6 + t4) & U32MASK;
    t4 = (t2 + t0) & U32MASK;
    d3u_st32(c, (uint32_t)((t1 - 4U) & U32MASK), (unsigned int)t0);
    t6 = (t6 + t4) & U32MASK;
    d3u_st32(c, (uint32_t)((t1 - 8U) & U32MASK), (unsigned int)t2);
     if (t1 != t5) { _pc = 1; continue; }
    t2 = 0x1FEU;
    t1 = (t3 * 2U) & U32MASK;
    t0 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    t5 = (t1 + 2U) & U32MASK;
    t4 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t5 = (t5 + t0) & U32MASK;
    t4 = (t4 + t1) & U32MASK;
    ;
    t0 = t2;
    t2 = (t2 - t5) & U32MASK;
    t2 = (t2 - t0) & U32MASK;
    t2 &= 3U;
    t0 = (t0 - t2) & U32MASK;
    if ((int)(t0 & U32MASK) > 0) { /* signed */
        while (t2 != 0) { M8(t5) = M8(t4); t4 = (t4 + 1U) & U32MASK; t5 = (t5 + 1U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
        t2 = t0;
        t0 &= 3U;
        t2 >>= 2;
        while (t2 != 0) { d3u_st32(c, (uint32_t)(t5), d3u_ld32(c, (uint32_t)(t4))); t4 = (t4 + 4U) & U32MASK; t5 = (t5 + 4U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
    }
    _pc = 2;
    continue;
        case 2:
    /* l_414e31: */
    t2 = (t2 + t0) & U32MASK;
    while (t2 != 0) { M8(t5) = M8(t4); t4 = (t4 + 1U) & U32MASK; t5 = (t5 + 1U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
    t2 = t6;
    t0 = t6;
    t2 &= 0xFFFFU;
    t0 >>= 16;
    t0 = (t0 + t2) & U32MASK;
    t2 = t0;
    t0 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    d3u_st16(c, (uint32_t)((t1 + t0) & U32MASK), (unsigned short)(t2 & 0xFFFFU));
    t1 = (t1 & 0xFFFF00FFU) | 0x100U;
    _pc = 3;
    continue;
        case 3:
    /* l_414e51: */
    t1 = (t1 & 0xFFFFFF00U) | ((t1 >> 8) & 0xFFU);
    t3 &= 0xFFFFFF00U;
    t1 = (t1 & 0xFFFFFF00U) | ((((t1 & 0xFFU) + ((t1 >> 8) & 0xFFU))) & 0xFFU);
    t3 = (t3 & 0xFFFFFF00U) | (((t3 & 0xFFU) - (t1 & 0xFFU)) & 0xFFU);
    t2 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    t0 = (t3 * 2U) & U32MASK;
    t0 = (t0 + t2) & U32MASK;
    _pc = 4;
    continue;
        case 4:
    /* l_414e68: */
    t2 = 0;
    t2 = (t1 >> 8) & 0xFFU;
    t4 = t0;
    t2 = (t2 + t2) & U32MASK;
    t4 = (t4 - t2) & U32MASK;
    t2 = (t2 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)(t4));
    t4 = (t4 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)(t0));
    t4 = (t4 + t2) & U32MASK;
    t2 = 0;
    t2 = t1 & 0xFFU;
    t2 = (t2 + t2) & U32MASK;
    d3u_st16(c, (uint32_t)(t0), (unsigned short)(t4 & 0xFFFFU));
    t0 = (t0 - t2) & U32MASK;
    t3 = (t3 & 0xFFFFFF00U) | (((t3 & 0xFFU) - (t1 & 0xFFU)) & 0xFFU);
    if ((t3 & 0xFFU) != 0U) { _pc = 4; continue; }
    t1 = (t1 & 0xFFFF00FFU) | (((((t1 >> 8) & 0xFFU) * 2U) & 0xFFU) << 8);
     if (((t1 >> 8) & 0xFFU) < 0x80U) { _pc = 3; continue; } /* unsigned */
    t3 = a1;
     if (t3 >= 0x2FDU) { _pc = 5; continue; } /* unsigned */
    t0 = (uint32_t)(uint32_t)D3U_BLK17_SMALL_B_32(&c->st, 0);
    t5 = (t5 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)((t0 + t3 * 2U) & U32MASK));
    t5 = (t5 & 0xFFFF0000U) | ((t5 & 0xFFFFU) >> 2);
    t1 = t5;
    d3u_st16(c, (uint32_t)((t0 + t3 * 2U) & U32MASK), (unsigned short)(t5 & 0xFFFFU));
    t1 = (t1 + 1U) & U32MASK;
    d3u_st16(c, (uint32_t)((t0 + t3 * 2U) & U32MASK), (unsigned short)(t1 & 0xFFFFU));
    _pc = 5;
    continue;
        case 5:
    /* l_414eb3: */
    /* epilogue: add esp,4; pop t6,t5,t4,t3,t2,t1; retn */
    return;
        default:
            break;
        }
        break;
    }
}
/* sub_414290: input top-up / hash-table rebuild. No reg inputs (all pushed). */
void d3uc_topup_model_reinit(dec3w_ctx *c)
{
    uint32_t t0 = 0;
    uint32_t t1 = 0;
    uint32_t t2 = 0;
    uint32_t t3 = 0;
    uint32_t t4 = 0;
    uint32_t t5 = 0;
    uint32_t t6 = 0;
    uint32_t v1c = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t6 = (uint32_t)D3U_TBL_44426C_32(&c->st, 0);
    t1 = 0x20U;
    t3 = 1;
    t0 = D3U_TBL_444108;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=20h dst=444108h fill=1 */
    t1 = 0x20U;
    t3 = 1;
    t0 = D3U_TBL_444128;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=20h dst=444128h fill=1 */
    t1 = 0x40U;
    t3 = 1;
    t0 = D3U_TBL_444148;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=40h dst=444148h fill=1 */
    t1 = 0x40000U;
    t0 = (uint32_t)(uint32_t)D3U_TBL_4299D0_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=40000h dst=4299D0 fill=0 */
    t1 = 0x80000U;
    t0 = (uint32_t)(uint32_t)D3U_TBL_4299D4_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=80000h dst=4299D4 fill=0 */
    t1 = 0x80000U;
    t0 = (uint32_t)(uint32_t)D3U_TBL_4299D8_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=80000h dst=4299D8 fill=0 */
    t1 = 0x40000U;
    t0 = (uint32_t)(uint32_t)D3U_TBL_4299DC_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=40000h dst=4299DC fill=0 */
    t1 = 0x40000U;
    t0 = (uint32_t)(uint32_t)D3U_TBL_4299E0_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=40000h dst=4299E0 fill=0 */
    t1 = 0x40000U;
    t0 = (uint32_t)(uint32_t)D3U_TBL_4299E4_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=40000h dst=4299E4 fill=0 */
    t1 = 0x1C0000U;
    t0 = (uint32_t)(uint32_t)D3U_TBL_4299E8_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=1C0000h dst=4299E8 fill=0 */
    t1 = 0x40000U;
    t0 = (uint32_t)(uint32_t)D3U_TBL_4299EC_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=40000h dst=4299EC fill=0 */
    t1 = 0x40600U;
    t0 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=40600h dst=4299F0 fill=0 */
    t1 = 0x40600U;
    t0 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=40600h dst=4299F4 fill=0 */
    t3 = 0x5FA00U;
    _pc = 1;
    continue;
        case 1:
    /* l_414912: */
    t2 = 0x200U;
    t5 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    t4 = 0; /* index into d3u_t42C9B0 (was offset 42C9B0h) */
    t5 = (t5 + t3) & U32MASK;
    t3 = (t3 + 0x200U) & U32MASK;
    ;
    t0 = t2;
    t2 = (t2 - t5) & U32MASK;
    t2 = (t2 - t0) & U32MASK;
    t2 &= 3U;
    t0 = (t0 - t2) & U32MASK;
    if ((int)(t0 & U32MASK) > 0) { /* signed */
        while (t2 != 0) { M8(t5) = d3u_t42C9B0[t4]; t4 = (t4 + 1U) & U32MASK; t5 = (t5 + 1U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
        t2 = t0;
        t0 &= 3U;
        t2 >>= 2;
        while (t2 != 0) { d3u_st32(c, (uint32_t)(t5), d3u_tbl_ld32(d3u_t42C9B0, t4)); t4 = (t4 + 4U) & U32MASK; t5 = (t5 + 4U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
    }
    _pc = 2;
    continue;
        case 2:
    /* l_414944: */
    t2 = (t2 + t0) & U32MASK;
    while (t2 != 0) { M8(t5) = d3u_t42C9B0[t4]; t4 = (t4 + 1U) & U32MASK; t5 = (t5 + 1U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
     if (t3 != 0x61600U) { _pc = 1; continue; }
    t0 = 0;
    _pc = 3;
    continue;
        case 3:
    /* l_414952: */
    t3 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    t1 = (t1 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)((t3 + t0 + 0x5FA00U) & U32MASK));
    t0 = (t0 + 2U) & U32MASK;
    t1 = (t1 << 3) & U32MASK;
    d3u_st16(c, (uint32_t)((t3 + t0 + 0x5F9FEU) & U32MASK), (unsigned short)(t1 & 0xFFFFU));
     if (t0 != 0x1C00U) { _pc = 3; continue; }
    t3 = 0xE00U;
    t1 = 0xE00U;
    t0 = 8;
    v1c = t3;
    t3 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t2 = 0x180U;
    t3 = (t3 + 0x5FA00U) & U32MASK;
    t5 = D3U_TBL_443B68;
    d3uc_fill16(c, (unsigned int)t1, (unsigned int)t3, (unsigned int)t0, (unsigned int)t2, (unsigned int)t4, (unsigned int)t5);
    t1 = 0x203U;
    t0 = 1;
    t3 = (uint32_t)(uint32_t)D3U_BLK17_SMALL_B_32(&c->st, 0);
    t4 = 0; /* index into d3u_t429A08 (was offset 429A08h) */
    d3uc_fill16(c, (unsigned int)t1, (unsigned int)t3, (unsigned int)t0, (unsigned int)t2, (unsigned int)t4, (unsigned int)t5);
    t1 = 0x203U;
    ;
    t0 = t2;
    t2 = (t2 - t5) & U32MASK;
    t2 = (t2 - t0) & U32MASK;
    t2 &= 3U;
    t0 = (t0 - t2) & U32MASK;
    if ((int)(t0 & U32MASK) > 0) { /* signed */
        while (t2 != 0) { M8(t5) = d3u_t429A08[t4]; t4 = (t4 + 1U) & U32MASK; t5 = (t5 + 1U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
        t2 = t0;
        t0 &= 3U;
        t2 >>= 2;
        while (t2 != 0) { d3u_st32(c, (uint32_t)(t5), d3u_tbl_ld32(d3u_t429A08, t4)); t4 = (t4 + 4U) & U32MASK; t5 = (t5 + 4U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
    }
    _pc = 4;
    continue;
        case 4:
    /* l_4149db: */
    t2 = (t2 + t0) & U32MASK;
    while (t2 != 0) { M8(t5) = d3u_t429A08[t4]; t4 = (t4 + 1U) & U32MASK; t5 = (t5 + 1U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
    v1c = t1;
    _pc = 5;
    continue;
        case 5:
    /* l_4149e2: */
    t2 = v1c;
    t0 = v1c;
    t2 = (t2 + 1U) & U32MASK;
    d3uc_fold_ctx_block(c, (unsigned int)t0);
    v1c = t2;
     if (t2 < 0x2FDU) { _pc = 5; continue; } /* unsigned */
    d3uc_rescale_blk02_model(c);
    t1 = 0x1FFU;
    t2 = D3U_TBL_442528;
    t5 = 0;
    t3 = 0;
    D3U_TBL_444210_32(&c->st, 0) = (unsigned int)t5;
    D3U_TBL_44424C_32(&c->st, 0) = (unsigned int)t5;
    D3U_TBL_44423C_32(&c->st, 0) = (unsigned int)t5;
    D3U_TBL_444208_32(&c->st, 0) = (unsigned int)t1;
    t0 = t2;
    t1 = 0x430U;
    D3U_TBL_4441F8_32(&c->st, 0) = (unsigned int)t2;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=430h dst=442528h fill=0 */
    t1 = 0x400U;
    t3 = 0xFFFFFFFFU;
    t0 = D3U_TBL_443360;
    t4 = 1;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=400h dst=443360h fill=FFh */
    t1 = 0x400U;
    t0 = D3U_TBL_442128;
    t3 = 0;
    v1c = t4;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=400h dst=442128h fill=0 */
    t3 &= 0xFFFFFF00U;
    t0 &= 0xFFFF00FFU;
    D3U_TBL_442D58_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
    t3 = (t3 & 0xFFFF00FFU) | 0x100U;
    D3U_TBL_443260_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFU);
    D3U_FLAG_444EBA_8(&c->st, 0) = (unsigned char)((t3 >> 8) & 0xFFU);
    D3U_FLAG_444EB9_8(&c->st, 0) = (unsigned char)((t3 >> 8) & 0xFFU);
    _pc = 6;
    continue;
        case 6:
    /* l_414a81: */
    t3 = (t3 & 0xFFFFFF00U) | (v1c & 0xFFU);
    t0 = v1c;
    t3 = (t3 & 0xFFFFFF00U) | ((t3 - 1U) & 0xFFU);
    G8((D3U_TBL_442D58 + t0) & U32MASK) = (unsigned char)(t3 & 0xFFU);
    t3 = (t3 & 0xFFFFFF00U) | (v1c & 0xFFU);
    t5 = (t0 + 1U) & U32MASK;
    t3 = (t3 & 0xFFFFFF00U) | ((t3 + 1U) & 0xFFU);
    v1c = t5;
    G8((D3U_TBL_443260 + t0) & U32MASK) = (unsigned char)(t3 & 0xFFU);
     if (t5 < 0x100U) { _pc = 6; continue; } /* unsigned */
    t3 = 0x9240U;
    t6 = (uint32_t)D3U_TBL_44426C_32(&c->st, 0);
    t0 = 0;
    _pc = 7;
    continue;
        case 7:
    /* l_414ab5: */
    t0 = (t0 + 4U) & U32MASK;
    D3U_TBL_442E54_32(&c->st, t0) = (unsigned int)t3;
     if (t0 != 0x400U) { _pc = 7; continue; }
    t1 = 0xFFU;
    t2 = 0x91ADC0U;
    t4 = 0;
    D3U_TBL_4441FC_32(&c->st, 0) = (unsigned int)t1;
    D3U_TBL_443258_32(&c->st, 0) = (unsigned int)t4;
    D3U_TBL_44325C_32(&c->st, 0) = (unsigned int)t4;
    D3U_TBL_443B60_32(&c->st, 0) = (unsigned int)t4;
    D3U_TBL_443B64_32(&c->st, 0) = (unsigned int)t4;
    D3U_TBL_4441E0_32(&c->st, 0) = (unsigned int)t4;
    D3U_TBL_444200_32(&c->st, 0) = (unsigned int)t4;
    t4 = (uint32_t)D3U_CUM_TABLE_PTR_32(&c->st, 0);
    D3U_TBL_4441E4_32(&c->st, 0) = (unsigned int)t2;
     if (t4 == 0U) { _pc = 11; continue; }
    t2 = 0;
    t2 = (uint32_t)D3U_FLAG_444EA0_8(&c->st, 0);
    t0 = 0x40000U;
    t2 = (t2 + t2) & U32MASK;
    t5 = (uint32_t)D3U_TBL_444230_32(&c->st, 0);
    t0 = (uint32_t)(((int)0x40000) >> (t2 & 31U));
    t5 = (t5 + t0) & U32MASK;
    t3 = (uint32_t)D3U_OUT_TOTAL_LEN_32(&c->st, 0);
    D3U_TBL_444230_32(&c->st, 0) = (unsigned int)t5;
     if (t5 > t3) { /* unsigned */
    D3U_TBL_444230_32(&c->st, 0) = (unsigned int)t3;
    }
    _pc = 8;
    continue;
        case 8:
    /* l_414b3e: */
    t4 = (uint32_t)D3U_TBL_444228_32(&c->st, 0);
    t2 = (uint32_t)D3U_PRIMER_TOTAL_32(&c->st, 0);
    t1 = (uint32_t)D3U_TBL_444230_32(&c->st, 0);
    t0 = (t6 * 4U) & U32MASK;
    _pc = 9;
    continue;
        case 9:
    /* l_414b57: */
    t3 = (uint32_t)D3U_CUM_TABLE_PTR_32(&c->st, 0);
     if (t1 <= (uint32_t)d3u_ld32(c, (uint32_t)((t3 + t0) & U32MASK))) { _pc = 10; continue; } /* unsigned */
    t0 = (t0 + 4U) & U32MASK;
    t3 = (t3 + t0) & U32MASK;
    t5 = (uint32_t)d3u_ld32(c, (uint32_t)(t3));
    t6 = (t6 + 1U) & U32MASK;
     if (t5 >= t2) { _pc = 10; continue; } /* unsigned */
     if (t5 < t4) { _pc = 9; continue; } /* unsigned */
     _pc = 10;
     continue;
        case 10:
    /* l_414b72: */
    t3 = (uint32_t)D3U_CUM_TABLE_PTR_32(&c->st, 0);
    t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + ((t6 * 4U) & U32MASK)) & U32MASK));
    _pc = 12;
    continue;
        case 11:
    /* l_414b7d: */
    t0 = (uint32_t)D3U_OUT_TOTAL_LEN_32(&c->st, 0);
    _pc = 12;
    continue;
        case 12:
    /* l_414b82: */
    D3U_TBL_444230_32(&c->st, 0) = (unsigned int)t0;
    D3U_TBL_44426C_32(&c->st, 0) = (unsigned int)t6;
     if (t6 == 0U) { _pc = 25; continue; } /* unsigned jbe 0 */
    t2 = (uint32_t)D3U_CUM_TABLE_PTR_32(&c->st, 0);
    t0 = (t6 * 4U) & U32MASK;
    t0 = (t0 + t2) & U32MASK;
    t2 = (uint32_t)D3U_TBL_444230_32(&c->st, 0);
    t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t0 - 4U) & U32MASK));
    t2 = (t2 - t5) & U32MASK;
     if (t6 > 1U) { /* unsigned */
    t5 = (t5 - (uint32_t)d3u_ld32(c, (uint32_t)((t0 - 8U) & U32MASK))) & U32MASK;
    }
    _pc = 13;
    continue;
        case 13:
    /* l_414bb7: */
    if (D3U_FLAG_444E99_8(&c->st, 0) != 1) { _pc = 14; continue; }
    t0 = (uint32_t)D3U_TBL_444268_32(&c->st, 0);
    _pc = 15;
    continue;
        case 14:
    /* l_414bc7: */
    t0 = (uint32_t)D3U_OUT_TOTAL_POS_32(&c->st, 0);
    _pc = 15;
    continue;
        case 15:
    /* l_414bcc: */
    v1c = t0;
    D3U_TBL_44426C_32(&c->st, 0) = (unsigned int)t6;
     if (t5 != 0x75A45U) { _pc = 16; continue; }
     if (t2 == 0x68302U) { _pc = 17; continue; }
     _pc = 16;
     continue;
        case 16:
    /* l_414be5: */
     if (t5 != 0x8F4D0U) { _pc = 18; continue; }
     if (t2 != 0x2D9A22U) { _pc = 18; continue; }
     _pc = 17;
     continue;
        case 17:
    /* l_414bf5: */
    t1 = 0x800000U;
    t0 = (uint32_t)(uint32_t)D3U_BLK01_DICT_COPY_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=800000h dst=4299BC fill=0 */
    _pc = 18;
    continue;
        case 18:
    /* l_414c06: */
     if (t5 != 0x17E5F8U) { _pc = 19; continue; }
     if (t2 == 0x2CC640U) { _pc = 20; continue; }
     _pc = 19;
     continue;
        case 19:
    /* l_414c16: */
     if (t5 != 0x2CC640U) { _pc = 21; continue; }
     if (t2 != 0x6AF80U) { _pc = 21; continue; }
     _pc = 20;
     continue;
        case 20:
    /* l_414c26: */
    d3uc_rescale_blk01_model(c);
    d3uc_rescale_blk02_model(c);
    _pc = 21;
    continue;
        case 21:
    /* l_414c30: */
     if (t5 != 0x388DC2U) { _pc = 22; continue; }
     if (t2 == 0x16DD2EU) { _pc = 24; continue; }
     _pc = 22;
     continue;
        case 22:
    /* l_414c40: */
     if (t5 != 0x7D4C0U) { _pc = 23; continue; }
     if (t2 == 0x3C41EU) { _pc = 24; continue; }
     _pc = 23;
     continue;
        case 23:
    /* l_414c50: */
     if (t5 == 0xFB670U) { _pc = 25; continue; }
     if (t2 != 0xFB670U) { _pc = 25; continue; }
    t0 = v1c;
     if (t0 < (uint32_t)D3U_TBL_444228_32(&c->st, 0)) { _pc = 25; continue; } /* unsigned */
     _pc = 24;
     continue;
        case 24:
    /* l_414c6b: */
    t1 = 0x800000U;
    t0 = (uint32_t)(uint32_t)D3U_BLK01_DICT_COPY_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=800000h dst=4299BC fill=0 */
    t1 = 0x140000U;
    t0 = (uint32_t)(uint32_t)D3U_BLK02_MODEL_A_32(&c->st, 0);
    t3 = 0;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3); /* len=140000h dst=4299C0 fill=0 */
    t0 = v1c;
     if (t0 < (uint32_t)D3U_TBL_444228_32(&c->st, 0)) { _pc = 25; continue; } /* unsigned */
    D3U_FLAG_444EA2_8(&c->st, 0) = 0x0A;
    _pc = 25;
    continue;
        case 25:
    /* l_414c9f: */
    if (D3U_FLAG_444E99_8(&c->st, 0) != 1) { _pc = 26; continue; }
    t0 = 0xFFU;
    d3uc_update_match_price(c, (unsigned int)t0);
    _pc = 30;
    continue;
        case 26:
    /* l_414cb7: */
    t4 = 0x0AU;
    t0 = t4;
    D3U_RC_TOTAL_32(&c->st, 0) = (unsigned int)t4;
    t0 = (uint32_t)d3uc_rc_decode(c, (unsigned int)t0);
    t3 = t0;
    t0 = 0;
    t2 = 0;
    t0 = t3 & 0xFFU;
    t2 = t3 & 0xFFU;
    D3U_RC_PROB_LO_32(&c->st, 0) = (unsigned int)t0;
    t0 = (t2 + 1U) & U32MASK;
    D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)t0;
    t0 = D3U_RC_TOTAL;
    d3uc_rc_update(c, (unsigned int)t0);
    if ((t3 & 0xFFU) == (uint32_t)D3U_FLAG_444EA9_8(&c->st, 0)) { _pc = 30; continue; }
    t6 = (uint32_t)D3U_TBL_44426C_32(&c->st, 0);
    D3U_FLAG_444EA9_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
    t0 = (uint32_t)d3u_t429DE0[(unsigned int)(t2 & 0xFFU)];
    t5 = (uint32_t)d3u_t429DB8[(unsigned int)(t2 & 0xFFU)];
    D3U_TBL_4441C4_32(&c->st, 0) = (unsigned int)t0;
    t0 = (t0 & 0xFFFFFF00U) | (uint32_t)d3u_t429E30[(unsigned int)(t2 & 0xFFU)];
    D3U_TBL_4441C8_32(&c->st, 0) = (unsigned int)t5;
    D3U_FLAG_444E9E_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    if ((t3 & 0xFFU) == 5U) { _pc = 27; continue; }
    if ((t3 & 0xFFU) != 6U) { _pc = 28; continue; }
    _pc = 27;
    continue;
        case 27:
    /* l_414d30: */
    t0 = (t0 & 0xFFFFFF00U) | 1U;
    _pc = 29;
    continue;
        case 28:
    /* l_414d34: */
    t0 = (t0 & 0xFFFFFF00U);
    _pc = 29;
    continue;
        case 29:
    /* l_414d36: */
    t3 = (t3 & 0xFFFF00FFU) | ((uint32_t)D3U_FLAG_444EA9_8(&c->st, 0) << 8);
    D3U_FLAG_444EA6_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    D3U_TBL_44426C_32(&c->st, 0) = (unsigned int)t6;
    t5 = 0;
    t0 &= 0xFFFF00FFU;
    D3U_TBL_4441D0_32(&c->st, 0) = (unsigned int)t5;
    D3U_FLAG_444EA8_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFU);
    D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFU);
    if (((t3 >> 8) & 0xFFU) > 4U) { /* unsigned */
    d3uc_init_hash_buckets(c);
    }
    _pc = 30;
    continue;
        case 30:
    /* l_414d67: */
    t6 = (uint32_t)D3U_TBL_44426C_32(&c->st, 0);
    /* epilogue: add esp,4; pop t6,t5,t4,t3,t2,t1; retn */
        default:
            break;
        }
        break;
    }
}

/* ---- match model update (from draft_e3.c) ---- */
/* sub_419F44: match price/model update. Takes the length-code byte in al
 * (param sym), picks the best of 5 price slots, maybe refines via sub_41ADF0,
 * then runs the two sub_423228 range-model switches and refreshes the
 * 4441C4/C8/444E9E/444EA6/4441D0/444EA8/444E9D state when the code changed.
 * Returns (d3u_ld32(c, (uint32_t)(0x444268))+t6)&U32MASK. sub_41A33C is called x2 (0x0041A028,
 * 0x0041A053), sub_423228 x2. UNCERTAIN: d3uc_walk_price_slots/d3uc_walk_match_price/d3uc_refine_match_price
 * are void under this draft contract, so their register results (t6 update,
 * v-slot prices, bl refinement) are lost; the C locals keep pre-call values.
 * Entry-live: al=sym only. Frame: w40[5] dwords, w2c dword, w28/w24/w1c
 * bytes, w20 input byte. */
unsigned int d3uc_update_match_price(dec3w_ctx *c, unsigned int sym)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int t5 = 0;
    unsigned int t6 = 0;
    unsigned int w40[5] = {0, 0, 0, 0, 0};
    unsigned int w2c = 0;
    unsigned char w28 = 0;
    unsigned char w24 = 0;
    unsigned char w20 = 0;
    unsigned char w1c = 0;
    unsigned int i = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    w20 = (unsigned char)(sym & 0xFFU);
    t0 = 0U;
    t0 = (t0 & ~0xFFU) | (unsigned int)D3U_FLAG_444EA9_8(&c->st, 0);
    t6 = D3U_TBL_444230_32(&c->st, 0);
    t3 = D3U_TBL_444268_32(&c->st, 0);
    t0 = (t0 & ~0xFFU) | (unsigned int)d3u_t429E3A[(unsigned int)(t0 & 0xFFU)];
    t6 = (t6 - t3) & U32MASK;
    w24 = (unsigned char)(t0 & 0xFFU);
    w1c = (unsigned char)(t0 & 0xFFU);
    if (t6 < 0x4B00U) { _pc = 1; continue; }
    t6 = 0x3200U;
    _pc = 1;
    continue;
        case 1:
    /* L_419F81: */
    if (D3U_TBL_42BC6E_8(&c->st, 0) == 0) { _pc = 14; continue; }
    if (D3U_FLAG_444EA0_8(&c->st, 0) == 0) { _pc = 14; continue; }
    t0 = D3U_TBL_444268_32(&c->st, 0);
    if (t0 >= D3U_TBL_444228_32(&c->st, 0)) { _pc = 14; continue; }
    if (t6 <= 0x100U) { _pc = 2; continue; }
    t0 = (t6 - 0x100U) & U32MASK;
    d3uc_fill_input(c, t0);
    _pc = 2;
    continue;
        case 2:
    /* L_419FBF: */
    t2 = 0x14U;
    t5 = D3U_TBL_444188;
    t0 = (uint32_t)D3U_IN_BUFFER_BASE_32(&c->st, 0);
    t3 = D3U_IN_POS_32(&c->st, 0);
    t4 = D3U_TBL_44419C;
    t3 = (t3 + t0) & U32MASK;
    t0 = t2;
    t2 = (t2 - t5) & U32MASK;
    t2 = (t2 - t0) & U32MASK;
    t2 &= 3U;
    t0 = (t0 - t2) & U32MASK;
    if ((int)t0 <= 0) { _pc = 3; continue; }
    for (i = 0; i < 20U; i++) M8((D3U_TBL_444188 + i) & U32MASK) = M8((D3U_TBL_44419C + i) & U32MASK);
    _pc = 3;
    continue;
        case 3:
    /* L_419FF5: */
    t2 = (t2 + t0) & U32MASK;
    w2c = t3;
    if (t6 <= 0x100U) { _pc = 4; continue; }
    t0 = w2c;
    t3 = (t6 - 0x100U) & U32MASK;
    t6 = d3uc_walk_price_slots(c, (unsigned int)t0, (unsigned int)t3);
    _pc = 4;
    continue;
        case 4:
    /* L_41A016: */
    t4 = w1c;
    t1 = 0xFFFFFFFFU;
    t0 = w2c;
    t3 = t6;
    t2 = t4;
    t0 = d3uc_walk_match_price(c, (unsigned int)t0, (unsigned int)t3, (unsigned int)t2, (unsigned int)t1);
    t3 &= ~0xFF00U;
    w40[t4] = t0;
    w28 = 0;
    _pc = 5;
    continue;
        case 5:
    /* L_41A036: */
    t0 = (t0 & ~0xFFU) | (unsigned int)w28;
    if ((t0 & 0xFFU) == (unsigned int)w24) { _pc = 6; continue; }
    t5 = w1c;
    t4 = t0 & 0xFFU;
    t3 = t6;
    t0 = w2c;
    t2 = t4;
    t1 = w40[t5];
    t0 = d3uc_walk_match_price(c, (unsigned int)t0, (unsigned int)t3, (unsigned int)t2, (unsigned int)t1);
    w40[t4] = t0;
    if (t0 >= w40[t5]) { _pc = 6; continue; }
    t0 = (t0 & ~0xFFU) | (unsigned int)w28;
    w1c = (unsigned char)(t0 & 0xFFU);
    _pc = 6;
    continue;
        case 6:
    /* L_41A068: */
    t1 = (t1 & ~0xFFU) | (unsigned int)w28;
    t1 = (t1 & ~0xFFU) | (((t1 & 0xFFU) + 1U) & 0xFFU);
    w28 = (unsigned char)(t1 & 0xFFU);
    if ((t1 & 0xFFU) <= 4U) { _pc = 5; continue; }
    t2 = 0U;
    D3U_TBL_4441B8_32(&c->st, 0) = t6;
    D3U_TBL_4441B0_32(&c->st, 0) = t2;
    t2 = (t2 & ~0xFFU) | (unsigned int)w1c;
    t1 = (t1 & ~0xFFU) | 0U;
    if ((t2 & 0xFFU) == 0U) { _pc = 7; continue; }
    if ((t2 & 0xFFU) == 3U) { _pc = 7; continue; }
    t2 = 0U;
    t2 = (t2 & ~0xFFU) | (unsigned int)w1c;
    t3 = ((t2 & 0xFFU) * 4U) & U32MASK;
    t4 = (uint32_t)D3U_TBL_444188_32(&c->st, t3);
    t0 = (t4 * 4U) & U32MASK;
    t0 = (t0 + t4) & U32MASK;
    t4 = D3U_TBL_444188_32(&c->st, 0);
    t4 = (t4 << 2) & U32MASK;
    if (t0 <= t4) { _pc = 7; continue; }
    t0 = w2c;
    t1 = w40[(t3 >> 2) & 0xFFU];
    t3 = t6;
    _pc = 11;
    continue;
        case 7:
    /* L_41A0CB: */
    t0 = 1U;
    t3 = (t3 & ~0xFFU) | ((t6 < 0x800U) ? 1U : 0U);
    t4 = 5U;
    t3 &= 0xFFU;
    t5 = D3U_TBL_44418C_32(&c->st, 0);
    t4 = (t4 - t3) & U32MASK;
    if (t5 >= D3U_TBL_444190_32(&c->st, 0)) { _pc = 8; continue; }
    t0 = 2U;
    _pc = 8;
    continue;
        case 8:
    /* L_41A0F9: */
    t3 = D3U_TBL_444194_32(&c->st, 0);
    if (t3 <= D3U_TBL_444188_32(&c->st, (t0 * 4U))) { _pc = 9; continue; }
    t0 = 3U;
    _pc = 9;
    continue;
        case 9:
    /* L_41A10D: */
    t3 = D3U_TBL_444198_32(&c->st, 0);
    if (t3 <= D3U_TBL_444188_32(&c->st, (t0 * 4U))) { _pc = 10; continue; }
    t0 = 4U;
    _pc = 10;
    continue;
        case 10:
    /* L_41A121: */
    if (t0 == 3U) { _pc = 12; continue; }
    t3 = D3U_TBL_444188_32(&c->st, 0);
    t3 = (t3 * t4) & U32MASK;
    t2 = D3U_TBL_444188_32(&c->st, (t0 * 4U));
    t2 = (t2 << 2) & U32MASK;
    if (t2 <= t3) { _pc = 12; continue; }
    t2 = 0U;
    t2 = (t2 & ~0xFFU) | (t0 & 0xFFU);
    t0 = 0U;
    t0 = (t0 & ~0xFFU) | (unsigned int)w1c;
    t3 = t6;
    t1 = w40[t0 & 0xFFU];
    t0 = w2c;
    _pc = 11;
    continue;
        case 11:
    /* L_41A150: */
    t0 = (t0 & ~0xFFU) | d3uc_refine_match_price(c, (unsigned int)t0, (unsigned int)t3, (unsigned int)t2, (unsigned int)t1);
    t1 = (t1 & ~0xFFU) | (t0 & 0xFFU);
    _pc = 12;
    continue;
        case 12:
    /* L_41A157: */
    if ((t1 & 0xFFU) == 0U) { _pc = 13; continue; }
    w1c = (unsigned char)(t1 & 0xFFU);
    t1 = 0U;
    D3U_TBL_4441B8_32(&c->st, 0) = t6;
    D3U_TBL_4441B0_32(&c->st, 0) = t1;
    _pc = 14;
    continue;
        case 13:
    /* L_41A16F: */
    t0 = 0U;
    D3U_TBL_4441B0_32(&c->st, 0) = t0;
    D3U_TBL_4441B8_32(&c->st, 0) = t0;
    _pc = 14;
    continue;
        case 14:
    /* L_41A17B: */
    if (w20 == 0xFF) { _pc = 15; continue; }
    t0 = (t0 & ~0xFFU) | (unsigned int)w1c;
    if ((t0 & 0xFFU) == (unsigned int)D3U_FLAG_444EA9_8(&c->st, 0)) { _pc = 20; continue; }
    _pc = 15;
    continue;
        case 15:
    /* L_41A192: */
    t1 = (t1 & ~0xFFU) | (unsigned int)w20;
    if ((t1 & 0xFFU) == 0xFFU) { _pc = 16; continue; }
    t0 = 0U;
    t3 = 0U;
    t0 = (t0 & ~0xFFU) | (unsigned int)D3U_FLAG_444EB6_8(&c->st, 0);
    t3 = (t3 & ~0xFFU) | (t1 & 0xFFU);
    t0 = (t0 + (t3 & 0xFFU)) & U32MASK;
    t0 = (t0 << 3) & U32MASK;
    t0 = (t0 + D3U_TBL_443B68) & U32MASK;
    t3 = 0U;
    t3 = (t3 & ~0xFFFFU) | (unsigned int)d3u_ld16(c, (uint32_t)((t0 + 6U) & U32MASK));
    D3U_RC_TOTAL_32(&c->st, 0) = t3;
    t3 = 0U;
    t3 = (t3 & ~0xFFFFU) | (unsigned int)d3u_ld16(c, (uint32_t)(t0));
    D3U_RC_PROB_LO_32(&c->st, 0) = t3;
    t0 = (t0 & ~0xFFFFU) | (unsigned int)d3u_ld16(c, (uint32_t)((t0 + 2U) & U32MASK));
    t0 &= 0xFFFFU;
    D3U_RC_PROB_HI_32(&c->st, 0) = t0;
    t0 = D3U_RC_TOTAL;
    d3uc_rc_renorm_update(c, t0);
    _pc = 16;
    continue;
        case 16:
    /* L_41A1DF: */
    t0 = 0U;
    t1 = 0U;
    t0 = (t0 & ~0xFFU) | (unsigned int)w1c;
    t1 = (t1 & ~0xFFU) | (t0 & 0xFFU);
    D3U_RC_PROB_LO_32(&c->st, 0) = t0;
    t0 = (t1 + 1U) & U32MASK;
    t4 = 0x0AU;
    D3U_RC_PROB_HI_32(&c->st, 0) = t0;
    t0 = D3U_RC_TOTAL;
    D3U_RC_TOTAL_32(&c->st, 0) = t4;
    d3uc_rc_renorm_update(c, t0);
    t2 = (t2 & ~0xFFU) | (unsigned int)D3U_FLAG_444EA9_8(&c->st, 0);
    t0 = (t0 & ~0xFFU) | (t1 & 0xFFU);
    if ((t0 & 0xFFU) == (t2 & 0xFFU)) { _pc = 20; continue; }
    D3U_FLAG_444EA9_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    t2 = (t2 & ~0xFF00U) | ((t1 & 0xFFU) << 8);
    t0 = d3u_t429DE0[(unsigned int)(t1 & 0xFFU)];
    t3 = d3u_t429DB8[(unsigned int)(t1 & 0xFFU)];
    D3U_TBL_4441C4_32(&c->st, 0) = t0;
    t0 = (t0 & ~0xFFU) | (unsigned int)d3u_t429E30[(unsigned int)(t1 & 0xFFU)];
    D3U_TBL_4441C8_32(&c->st, 0) = t3;
    D3U_FLAG_444E9E_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    if (((t2 >> 8) & 0xFFU) == 5U) { _pc = 17; continue; }
    if ((t1 & 0xFFU) != 6U) { _pc = 18; continue; }
    _pc = 17;
    continue;
        case 17:
    /* L_41A24C: */
    t0 = (t0 & ~0xFFU) | 1U;
    _pc = 19;
    continue;
        case 18:
    /* L_41A250: */
    t0 &= ~0xFFU;
    _pc = 19;
    continue;
        case 19:
    /* L_41A252: */
    D3U_FLAG_444EA6_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    t5 = 0U;
    t0 &= ~0xFF00U;
    D3U_TBL_4441D0_32(&c->st, 0) = t5;
    D3U_FLAG_444EA8_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFU);
    D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFU);
    _pc = 20;
    continue;
        case 20:
    /* L_41A26D: */
    t0 = D3U_TBL_444268_32(&c->st, 0);
    t0 = (t0 + t6) & U32MASK;
    D3U_TBL_4441D4_32(&c->st, 0) = t0;
    return t0 & U32MASK; /* epilogue retn, t0=444268+t6 */
        default:
            break;
        }
        break;
    }
    return 0;
}

/* ---- small helpers (from draft_h3.c) ---- */
/* exit al=boolean (setl at survives; ah/bh lanes clobbered). */
unsigned int d3uc_decode_bit_update(dec3w_ctx *c, unsigned int p)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int ecx_ = 0;
unsigned int edx_ = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
 edx_ = p;
 ebx_ = 0u;
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)G8(p + 1u);
 eax_ = 0u;
 eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)G8(edx_);
 eax_ = (eax_ + ebx_) & U32MASK;
 D3U_RC_TOTAL_32(&c->st, 0) = eax_;
eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)D3U_RC_TOTAL_16(&c->st, 0);
 eax_ &= 0xFFFFu;
 ebx_ = 0u;
eax_ = d3uc_rc_decode(c, eax_);
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)G8(edx_);
 eax_ &= 0xFFFFu;
 /* flags for setl */
eax_ = (eax_ & 0xFFFFFF00U) | ((((int)eax_ < (int)ebx_) ? 1u : 0u));
if ((eax_ & 0xFFu) == 0u) { _pc = 1; continue; }
 ebx_ = 0u;
 D3U_RC_PROB_LO_32(&c->st, 0) = ebx_;
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)G8(edx_);
 D3U_RC_PROB_HI_32(&c->st, 0) = ebx_;
ebx_ = (ebx_ & 0xFFFFFF00U) | ((ebx_ + 1u) & 0xFFu);
 M8(edx_) = (unsigned char)(ebx_ & 0xFFu);
_pc = 2;
continue;
        case 1:
    /* l_4167c1: */
 ebx_ = 0u;
 eax_ = (eax_ & 0xFFFF00FFU) | (((unsigned int)G8(edx_ + 1u) << 8) & 0xFF00u);
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)G8(edx_);
eax_ = (eax_ & 0xFFFF00FFU) | (((((eax_ >> 8) & 0xFFu) + 1u) & 0xFFu) << 8);
 D3U_RC_PROB_LO_32(&c->st, 0) = ebx_;
 ebx_ = D3U_RC_TOTAL_32(&c->st, 0);
 M8(edx_ + 1u) = (unsigned char)((eax_ >> 8) & 0xFFu);
 D3U_RC_PROB_HI_32(&c->st, 0) = ebx_;
            _pc = 2;
            continue;
        case 2:
    /* l_4167df: */
if (G8(edx_) >= 0xFEu) { _pc = 3; continue; }
if (G8(edx_ + 1u) < 0xFEu) { _pc = 4; continue; }
            _pc = 3;
            continue;
        case 3:
    /* l_4167ea: */
 ecx_ = (ecx_ & 0xFFFF00FFU) | (((unsigned int)G8(edx_) << 8) & 0xFF00u);
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)G8(edx_ + 1u);
ecx_ = (ecx_ & 0xFFFF00FFU) | (((((ecx_ >> 8) & 0xFFu) + 1u) & 0xFFu) << 8);
ebx_ = (ebx_ & 0xFFFFFF00U) | ((ebx_ + 1u) & 0xFFu);
 M8(edx_) = (unsigned char)((ecx_ >> 8) & 0xFFu);
eax_ = (eax_ & 0xFFFF00FFU) | (ecx_ & 0xFF00u);
 M8(edx_ + 1u) = (unsigned char)(ebx_ & 0xFFu);
ebx_ = (ebx_ & 0xFFFF00FFU) | ((ebx_ & 0xFFu) << 8);
eax_ = (eax_ & 0xFFFF00FFU) | (((((eax_ >> 8) & 0xFFu) >> 1) & 0xFFu) << 8);
ebx_ = (ebx_ & 0xFFFF00FFU) | (((((ebx_ >> 8) & 0xFFu) >> 1) & 0xFFu) << 8);
 M8(edx_) = (unsigned char)((eax_ >> 8) & 0xFFu);
 M8(edx_ + 1u) = (unsigned char)((ebx_ >> 8) & 0xFFu);
            _pc = 4;
            continue;
        case 4:
    /* l_416805: */
/* retn */ return (eax_ & 0xFFu);
        default:
            break;
        }
        break;
    }
    return 0;
}
/* write is xor at ). Exit t0 = table-entry pointer, or 0. */
unsigned int d3uc_lookup_model_entry(dec3w_ctx *c)
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
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
ebx_ = (ebx_ & 0xFFFF00FFU) | (8u << 8);
 edx_ = D3U_TBL_444258_32(&c->st, 0);
 esi_ = edx_;
 edi_ = edx_;
 ecx_ = 0u;
 eax_ = 0u;
 D3U_TBL_44427C_32(&c->st, 0) = ecx_;
_pc = 1;
continue;
        case 1:
    /* l_41507f: */
 edx_ >>= 8;
 esi_ >>= 16;
edi_ &= 0xFFFF0000U;
ebx_ &= 0xFFFFFF00U;
if (edi_ != 0xFFFF0000U) { _pc = 2; continue; }
 eax_ = 0x10000u;
_pc = 7;
continue;
        case 2:
    /* l_41509c: */
 edi_ = D3U_TBL_444258_32(&c->st, 0);
 edi_ &= 0xFFFFu;
if (edi_ != 0xFFFFu) { _pc = 3; continue; }
 eax_ = 0x20000u;
_pc = 7;
continue;
        case 3:
    /* l_4150ba: */
 edi_ = D3U_TBL_444258_32(&c->st, 0);
 edi_ &= 0xFF000000U;
if (edi_ != 0xFF000000U) { _pc = 4; continue; }
 eax_ = 0x30000u;
_pc = 7;
continue;
        case 4:
    /* l_4150d5: */
 edi_ = D3U_TBL_444258_32(&c->st, 0);
 edi_ &= 0xFF0000u;
if (edi_ != 0xFF0000u) { _pc = 5; continue; }
 eax_ = 0x40000u;
_pc = 7;
continue;
        case 5:
    /* l_4150f0: */
 edi_ = D3U_TBL_444258_32(&c->st, 0);
 edi_ &= 0xFF00u;
if (edi_ != 0xFF00u) { _pc = 6; continue; }
 eax_ = 0x50000u;
_pc = 7;
continue;
        case 6:
    /* l_41510b: */
 edi_ = D3U_TBL_444258_32(&c->st, 0);
 edi_ &= 0xFFu;
if (edi_ != 0xFFu) { _pc = 7; continue; }
 eax_ = 0x60000u;
            _pc = 7;
            continue;
        case 7:
    /* l_415124: */
 edi_ = esi_;
 ebp_ = D3U_TBL_44425C_32(&c->st, 0);
 edi_ &= 0xFF00u;
 ebp_ >>= 24;
 esi_ &= 0xFFFFu;
 D3U_TBL_443CF0_32(&c->st, 0) = esi_;
 ebp_ = (ebp_ + edi_) & U32MASK;
 esi_ = edx_;
 D3U_TBL_443CE8_32(&c->st, 0) = ebp_;
 esi_ &= 0xFF00u;
 ebp_ = edx_;
 v24 = esi_;
 ebp_ &= 0xFFFFu;
 esi_ = D3U_TBL_444258_32(&c->st, 0);
 D3U_TBL_443CEC_32(&c->st, 0) = ebp_;
 esi_ &= 0xFFu;
 ebp_ = v24;
 ebp_ = (ebp_ + esi_) & U32MASK;
 edx_ &= 0xFFu;
 D3U_TBL_443CF4_32(&c->st, 0) = ebp_;
 ebp_ = (edi_ + edx_) & U32MASK;
 edx_ = D3U_TBL_44425C_32(&c->st, 0);
 D3U_TBL_443CF8_32(&c->st, 0) = ebp_;
 edx_ &= 0xFFFFu;
 edi_ = (edi_ + esi_) & U32MASK;
 edx_ = (edx_ + eax_) & U32MASK;
 D3U_TBL_443CFC_32(&c->st, 0) = edi_;
 D3U_TBL_443D00_32(&c->st, 0) = edx_;
if (esi_ != 0xFFu) { _pc = 8; continue; }
edx_ = (edx_ & 0xFFFF0000U) | (unsigned int)D3U_TBL_443CEC_16(&c->st, 2u);
esi_ = (esi_ & 0xFFFF0000U) | (unsigned int)D3U_TBL_443CF0_16(&c->st, 2u);
 edx_ = (edx_ + 1u) & U32MASK;
 esi_ = (esi_ + 1u) & U32MASK;
D3U_TBL_443CEC_16(&c->st, 2u) = (unsigned short)(edx_ & 0xFFFFu);
D3U_TBL_443CF0_16(&c->st, 2u) = (unsigned short)(esi_ & 0xFFFFu);
            _pc = 8;
            continue;
        case 8:
    /* l_4151ca: */
 esi_ = (unsigned int)D3U_FLAG_444EB9_8(&c->st, 0);
 edx_ = D3U_DICT_CURSOR_32(&c->st, 0);
 edi_ = D3U_DICT_MASK_32(&c->st, 0);
 edx_ = (edx_ - esi_) & U32MASK;
 eax_ = (uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
 edx_ &= edi_;
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)c->blk00_dict[(edx_ + eax_) - c->blk00_base];
 edx_ &= 0xFFu;
 v1c = edx_;
 edx_ = 0u;
edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EB9_8(&c->st, 0);
 edi_ = D3U_DICT_CURSOR_32(&c->st, 0);
 edx_ = (edx_ + edx_) & U32MASK;
 edi_ = (edi_ - edx_) & U32MASK;
 ebp_ = D3U_DICT_MASK_32(&c->st, 0);
 edx_ = edi_;
 edx_ &= ebp_;
 eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)c->blk00_dict[(edx_ + eax_) - c->blk00_base];
 eax_ &= 0xFFu;
 v20 = eax_;
eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EB9_8(&c->st, 0);
if ((eax_ & 0xFFu) != (unsigned int)D3U_FLAG_444EA2_8(&c->st, 0)) { _pc = 10; continue; }
if (D3U_TBL_444258_8(&c->st, 0) != 1u) { _pc = 9; continue; }
ebx_ = (ebx_ & 0xFFFFFF00U) | 8u;
            _pc = 9;
            continue;
        case 9:
    /* l_415233: */
 eax_ = D3U_TBL_444258_32(&c->st, 0);
 eax_ >>= 16;
eax_ = (eax_ & 0xFFFFFF00U) | ((eax_ == 0xF6FFu) ? 1u : 0u);
 eax_ &= 0xFFu;
 eax_ = (eax_ << 8) & U32MASK;
 D3U_TBL_443D04_32(&c->st, 0) = eax_;
_pc = 11;
continue;
        case 10:
    /* l_415252: */
 eax_ = D3U_TBL_4441C0_32(&c->st, 0);
 edx_ = 0u;
 qv = eax_; eax_ = qv / esi_; edx_ = qv % esi_;
 edx_ = (edx_ << 8) & U32MASK;
 D3U_TBL_443D04_32(&c->st, 0) = edx_;
            _pc = 11;
            continue;
        case 11:
    /* l_415264: */
 edx_ = (edx_ & 0xFFFFFF00U) | (v1c & 0xFFu);
edx_ = (edx_ & 0xFFFFFF00U) | (((edx_ & 0xFFu) + (edx_ & 0xFFu)) & 0xFFu);
 edx_ = (edx_ & 0xFFFF00FFU) | ((v20 & 0xFFu) << 8);
 eax_ = (eax_ & 0xFFFFFF00U) | (v1c & 0xFFu);
edx_ = (edx_ & 0xFFFFFF00U) | (((edx_ & 0xFFu) - ((edx_ >> 8) & 0xFFu)) & 0xFFu);
eax_ = (eax_ & 0xFFFFFF00U) | (((eax_ & 0xFFu) - ((edx_ >> 8) & 0xFFu)) & 0xFFu);
 esi_ = D3U_TBL_443D04_32(&c->st, 0);
 eax_ &= 0xFFu;
 esi_ = (esi_ + eax_) & U32MASK;
 eax_ = esi_;
 D3U_TBL_443D04_32(&c->st, 0) = esi_;
 esi_ = (uint32_t)D3U_TBL_4299EC_32(&c->st, 0);
 M8(esi_ + eax_ * 4u + 2u) = (unsigned char)(edx_ & 0xFFu);
if ((ebx_ & 0xFFu) >= 8u) { _pc = 14; continue; }
            _pc = 12;
            continue;
        case 12:
    /* l_41529a: */
 eax_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
 edx_ = D3U_TBL_443CE8_32(&c->st, (eax_ & 0xFFu) * 4u);
 edx_ = (edx_ << 2) & U32MASK;
 eax_ = D3U_TBL_4299D0_32(&c->st, (eax_ & 0xFFu) * 4u);
 eax_ = (eax_ + edx_) & U32MASK;
if (G8(eax_ + 1u) == 0u) { _pc = 13; continue; }
if (G8(eax_ + 3u) >= 0x80u) { _pc = 13; continue; }
 esi_ = (unsigned int)G8(eax_);
 edx_ = 0u;
 esi_ = (esi_ << 8) & U32MASK;
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)G8(eax_ + 3u);
 edx_ = (edx_ * esi_) & U32MASK;
 esi_ = (unsigned int)G8(eax_ + 1u);
 eax_ = edx_;
 edx_ = (unsigned int)(((int)edx_) >> 31) & U32MASK;
 qv = eax_; eax_ = (unsigned int)(((int)qv) / ((int)esi_)); edx_ = (unsigned int)(((int)qv) % ((int)esi_));
if (eax_ <= ecx_) { _pc = 13; continue; }
 ecx_ = eax_;
ebx_ = (ebx_ & 0xFFFF00FFU) | ((ebx_ & 0xFFu) << 8);
            _pc = 13;
            continue;
        case 13:
    /* l_4152de: */
ebx_ = (ebx_ & 0xFFFFFF00U) | ((ebx_ + 1u) & 0xFFu);
if ((ebx_ & 0xFFu) < 8u) { _pc = 12; continue; }
            _pc = 14;
            continue;
        case 14:
    /* l_4152e5: */
if (((ebx_ >> 8) & 0xFFu) >= 8u) { _pc = 19; continue; }
 eax_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | ((ebx_ >> 8) & 0xFFu);
 edx_ = D3U_TBL_443CE8_32(&c->st, (eax_ & 0xFFu) * 4u);
 edx_ = (edx_ << 2) & U32MASK;
 eax_ = D3U_TBL_4299D0_32(&c->st, (eax_ & 0xFFu) * 4u);
 eax_ = (eax_ + edx_) & U32MASK;
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)G8(eax_);
if ((edx_ & 0xFFu) > (unsigned int)G8(eax_ + 1u)) { _pc = 15; continue; }
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)G8(eax_ + 3u);
 edx_ = (edx_ & 0xFFFFFF00U) | ((edx_ & 0xFFu) & 0x7Fu);
if ((edx_ & 0xFFu) <= 1u) { _pc = 19; continue; }
            _pc = 15;
            continue;
        case 15:
    /* l_41531b: */
if ((unsigned int)D3U_FLAG_444EB9_8(&c->st, 0) <= 1u) { _pc = 16; continue; }
edx_ = (edx_ & 0xFFFFFF00U) | (((((ebx_ >> 8) & 0xFFu) == 7u) ? 1u : 0u));
D3U_FLAG_444EA4_8(&c->st, 0) = (unsigned char)(edx_ & 0xFFu);
            _pc = 16;
            continue;
        case 16:
    /* l_415330: */
 ecx_ = (ecx_ & 0xFFFF00FFU) | (((unsigned int)G8(eax_ + 1u) << 8) & 0xFF00u);
if (((ecx_ >> 8) & 0xFFu) != 2u) { _pc = 17; continue; }
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)G8(eax_);
edx_ = (edx_ & 0xFFFFFF00U) | (((edx_ & 0xFFu) - 3u) & 0xFFu);
if ((edx_ & 0xFFu) >= ((ecx_ >> 8) & 0xFFu)) { _pc = 17; continue; }
 edx_ = 0u;
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)G8(eax_);
 ecx_ = 0u;
 edx_ = (edx_ << 3) & U32MASK;
ecx_ = (ecx_ & 0xFFFFFF00U) | ((ebx_ >> 8) & 0xFFu);
 edx_ = (edx_ - 0x18u) & U32MASK;
 edx_ = (edx_ + ecx_) & U32MASK;
 ecx_ = D3U_TBL_444148;
 edx_ = (edx_ + edx_) & U32MASK;
 ecx_ = (ecx_ + edx_) & U32MASK;
 D3U_TBL_44427C_32(&c->st, 0) = ecx_;
            _pc = 17;
            continue;
        case 17:
    /* l_415360: */
if (G8(eax_ + 1u) != 3u) { _pc = 18; continue; }
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)G8(eax_);
edx_ = (edx_ & 0xFFFFFF00U) | (((edx_ & 0xFFu) - 4u) & 0xFFu);
if ((edx_ & 0xFFu) >= 2u) { _pc = 18; continue; }
 edx_ = 0u;
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)G8(eax_);
 ecx_ = 0u;
 edx_ = (edx_ << 3) & U32MASK;
ecx_ = (ecx_ & 0xFFFFFF00U) | ((ebx_ >> 8) & 0xFFu);
 edx_ = (edx_ - 0x20u) & U32MASK;
 edx_ = (edx_ + ecx_) & U32MASK;
 edx_ = (edx_ + edx_) & U32MASK;
 ecx_ = D3U_TBL_444148;
 edx_ = (edx_ + 0x20u) & U32MASK;
 ecx_ = (ecx_ + edx_) & U32MASK;
 D3U_TBL_44427C_32(&c->st, 0) = ecx_;
            _pc = 18;
            continue;
        case 18:
    /* l_415392: */
D3U_FLAG_444EA3_8(&c->st, 0) = (unsigned char)((ebx_ >> 8) & 0xFFu);
/* retn */ return eax_;
            _pc = 19;
            continue;
        case 19:
    /* l_4153a2: */
if ((unsigned int)D3U_FLAG_444EBA_8(&c->st, 0) <= 1u) { _pc = 20; continue; }
 eax_ = D3U_TBL_4441F8_32(&c->st, 0);
 d3u_st32(c, (uint32_t)(eax_), d3u_ld32(c, (uint32_t)(eax_)) >> (1));
            _pc = 20;
            continue;
        case 20:
    /* l_4153b2: */
 eax_ = 0u;
/* retn */ return eax_;
        default:
            break;
        }
        break;
    }
    return 0;
}
/* t1 access is write mov bh,al at . Exit al (mov al,bl tail). */
unsigned int d3uc_decode_offset_ctx(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in)
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
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
 ecx_ = eax_in;
 esi_ = edx_in;
 edi_ = D3U_TBL_4441E4_32(&c->st, 0);
eax_ = (eax_ & 0xFFFFFF00U) | ((edx_in == 0x2FDu) ? 1u : 0u);
 D3U_TBL_44421C_32(&c->st, 0) = esi_;
 edx_ = D3U_TBL_4441FC_32(&c->st, 0);
 ebp_ = 0u;
 edx_ = (edx_ + 1u) & U32MASK;
ebx_ = (ebx_ & 0xFFFF00FFU) | ((eax_ & 0xFFu) << 8);
 edx_ &= 0xFFu;
 D3U_TBL_4441F4_32(&c->st, 0) = ebp_;
 D3U_TBL_4441FC_32(&c->st, 0) = edx_;
 edx_ = D3U_TBL_442E58_32(&c->st, edx_ * 4u);
eax_ = (eax_ & 0xFFFF00FFU) | (1u << 8);
 edi_ = (edi_ - edx_) & U32MASK;
 stk_b = (eax_ >> 8) & 0xFFu;
 D3U_TBL_4441E4_32(&c->st, 0) = edi_;
if (edi_ < 0x6D4250u) { _pc = 7; continue; }
if (edi_ >= 0xFEFF01u) { _pc = 7; continue; }
if (edi_ < 0x91ADC0u) { _pc = 2; continue; }
 esi_ = 0x100u;
 eax_ = 0u;
 D3U_RC_TOTAL_32(&c->st, 0) = esi_;
eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)D3U_RC_TOTAL_16(&c->st, 0);
eax_ = d3uc_rc_decode(c, eax_);
 edx_ = eax_;
ebx_ = (ebx_ & 0xFFFFFF00U) | (eax_ & 0xFFu);
 eax_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | (edx_ & 0xFFu);
 D3U_RC_PROB_LO_32(&c->st, 0) = eax_;
 eax_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | (edx_ & 0xFFu);
 eax_ = (eax_ + 1u) & U32MASK;
 D3U_RC_PROB_HI_32(&c->st, 0) = eax_;
 eax_ = D3U_RC_TOTAL;
d3uc_rc_update(c, eax_);
if (D3U_FLAG_444EAE_8(&c->st, 0) == 0u) { _pc = 1; continue; }
 D3U_TBL_44420C_32(&c->st, 0) = (D3U_TBL_44420C_32(&c->st, 0) + 0x9240u) & U32MASK;
            _pc = 1;
            continue;
        case 1:
    /* l_4175bb: */
 edx_ = 0u;
 eax_ = ecx_;
edx_ = (edx_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
eax_ = d3uc_update_ctx_coder(c, eax_, edx_);
_pc = 8;
continue;
        case 2:
    /* l_4175cb: */
 edx_ = 0u;
edx_ = (edx_ & 0xFFFFFF00U) | (eax_ & 0xFFu);
 edi_ = D3U_TBL_443258_32(&c->st, (edx_ & 0xFFu) * 4u);
if (edi_ < D3U_TBL_443B60_32(&c->st, (edx_ & 0xFFu) * 4u)) { _pc = 4; continue; }
 eax_ = ecx_;
eax_ = d3uc_decode_offset_sym(c, eax_);
ebx_ = (ebx_ & 0xFFFFFF00U) | (eax_ & 0xFFu);
eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EAE_8(&c->st, 0);
 ebp_ = D3U_TBL_4441F4_32(&c->st, 0);
if ((eax_ & 0xFFu) == 0u) { _pc = 3; continue; }
 D3U_TBL_44420C_32(&c->st, 0) = (D3U_TBL_44420C_32(&c->st, 0) + ebp_) & U32MASK;
            _pc = 3;
            continue;
        case 3:
    /* l_4175fd: */
 eax_ = 0u;
 D3U_TBL_4441F4_32(&c->st, 0) = eax_;
eax_ = (eax_ & 0xFFFFFF00U) | ((ebx_ >> 8) & 0xFFu);
 edx_ = 0u;
 eax_ = (eax_ + 0x2FEu) & U32MASK;
edx_ = (edx_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
 D3U_TBL_44421C_32(&c->st, 0) = eax_;
 eax_ = 0x200u;
eax_ = d3uc_update_ctx_coder(c, eax_, edx_);
 edi_ = D3U_TBL_4441F4_32(&c->st, 0);
_pc = 6;
continue;
        case 4:
    /* l_417626: */
 edx_ = (edx_ + 0x2FEu) & U32MASK;
 eax_ = 0x200u;
 D3U_TBL_44421C_32(&c->st, 0) = edx_;
eax_ = d3uc_decode_offset_sym(c, eax_);
 edi_ = D3U_TBL_4441F4_32(&c->st, 0);
edx_ = (edx_ & 0xFFFF00FFU) | (((unsigned int)D3U_FLAG_444EAE_8(&c->st, 0) << 8) & 0xFF00u);
ebx_ = (ebx_ & 0xFFFFFF00U) | (eax_ & 0xFFu);
if (((edx_ >> 8) & 0xFFu) == 0u) { _pc = 5; continue; }
 D3U_TBL_44420C_32(&c->st, 0) = (D3U_TBL_44420C_32(&c->st, 0) + edi_) & U32MASK;
            _pc = 5;
            continue;
        case 5:
    /* l_417654: */
 edx_ = 0u;
 eax_ = ecx_;
 D3U_TBL_4441F4_32(&c->st, 0) = edx_;
edx_ = (edx_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
 D3U_TBL_44421C_32(&c->st, 0) = esi_;
eax_ = d3uc_update_ctx_coder(c, eax_, edx_);
 ebp_ = D3U_TBL_4441F4_32(&c->st, 0);
            _pc = 6;
            continue;
        case 6:
    /* l_417671: */
 eax_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | ((ebx_ >> 8) & 0xFFu);
 stk_d = eax_;
 eax_ = (eax_ + 0x2FEu) & U32MASK;
 D3U_TBL_44421C_32(&c->st, 0) = eax_;
 eax_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EB3_8(&c->st, 0);
 edx_ = D3U_TBL_44421C_32(&c->st, 0);
 D3U_TBL_441F08_32(&c->st, (eax_ & 0xFFu) * 4u) = edx_;
 D3U_TBL_442018_32(&c->st, (eax_ & 0xFFu) * 4u) = 0x200u;
D3U_FLAG_444EB3_8(&c->st, 0) = (unsigned char)((D3U_FLAG_444EB3_8(&c->st, 0) + 1u) & 0xFFu);
 edx_ = D3U_TBL_4441E4_32(&c->st, 0);
 edx_ = (edx_ + edi_) & U32MASK;
 D3U_TBL_4441D8_8(&c->st, (eax_ & 0xFFu)) = (unsigned char)(ebx_ & 0xFFu);
 D3U_TBL_4441E4_32(&c->st, 0) = edx_;
 edx_ = stk_d;
 eax_ = D3U_TBL_4441FC_32(&c->st, 0);
 edx_ = (edx_ << 2) & U32MASK;
 D3U_TBL_442E58_32(&c->st, eax_ * 4u) = edi_;
 eax_ = D3U_TBL_443258_32(&c->st, edx_);
 stk_d = eax_;
 eax_ = (eax_ << 5) & U32MASK;
 eax_ = (eax_ - stk_d) & U32MASK;
 eax_ >>= 5;
 eax_ = (eax_ + edi_) & U32MASK;
 D3U_TBL_4441F4_32(&c->st, 0) = ebp_;
 D3U_TBL_443258_32(&c->st, edx_) = eax_;
edx_ = (edx_ & 0xFFFF00FFU);
 D3U_TBL_44421C_32(&c->st, 0) = esi_;
 stk_b = (edx_ >> 8) & 0xFFu;
_pc = 8;
continue;
        case 7:
    /* l_4176fb: */
 eax_ = ecx_;
eax_ = d3uc_decode_offset_sym(c, eax_);
edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EAE_8(&c->st, 0);
ebx_ = (ebx_ & 0xFFFFFF00U) | (eax_ & 0xFFu);
if ((edx_ & 0xFFu) == 0u) { _pc = 8; continue; }
 eax_ = D3U_TBL_4441F4_32(&c->st, 0);
 D3U_TBL_44420C_32(&c->st, 0) = (D3U_TBL_44420C_32(&c->st, 0) + eax_) & U32MASK;
            _pc = 8;
            continue;
        case 8:
    /* l_417719: */
 eax_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EB3_8(&c->st, 0);
 esi_ = D3U_TBL_44421C_32(&c->st, 0);
 D3U_TBL_441F08_32(&c->st, (eax_ & 0xFFu) * 4u) = esi_;
 D3U_TBL_442018_32(&c->st, (eax_ & 0xFFu) * 4u) = ecx_;
 D3U_TBL_4441D8_8(&c->st, (eax_ & 0xFFu)) = (unsigned char)(ebx_ & 0xFFu);
eax_ = (eax_ & 0xFFFF00FFU) | ((eax_ & 0xFFu) << 8);
eax_ = (eax_ & 0xFFFF00FFU) | (((((eax_ >> 8) & 0xFFu) + 1u) & 0xFFu) << 8);
 edx_ = (edx_ & 0xFFFFFF00U) | (stk_b & 0xFFu);
D3U_FLAG_444EB3_8(&c->st, 0) = (unsigned char)((eax_ >> 8) & 0xFFu);
if ((edx_ & 0xFFu) == 0u) { _pc = 9; continue; }
 edx_ = D3U_TBL_4441F4_32(&c->st, 0);
 ebp_ = D3U_TBL_4441E4_32(&c->st, 0);
 eax_ = D3U_TBL_4441FC_32(&c->st, 0);
 ebp_ = (ebp_ + edx_) & U32MASK;
 D3U_TBL_442E58_32(&c->st, eax_ * 4u) = edx_;
 edx_ = 0u;
edx_ = (edx_ & 0xFFFFFF00U) | ((ebx_ >> 8) & 0xFFu);
 ecx_ = D3U_TBL_443258_32(&c->st, (edx_ & 0xFFu) * 4u);
 eax_ = ecx_;
 eax_ = (eax_ << 5) & U32MASK;
 eax_ = (eax_ - ecx_) & U32MASK;
 ecx_ = D3U_TBL_4441F4_32(&c->st, 0);
 eax_ >>= 5;
 eax_ = (eax_ + ecx_) & U32MASK;
 D3U_TBL_4441E4_32(&c->st, 0) = ebp_;
 D3U_TBL_443258_32(&c->st, (edx_ & 0xFFu) * 4u) = eax_;
            _pc = 9;
            continue;
        case 9:
    /* l_417790: */
 ecx_ = 0u;
ecx_ = (ecx_ & 0xFFFFFF00U) | ((ebx_ >> 8) & 0xFFu);
 edx_ = D3U_TBL_443B60_32(&c->st, (ecx_ & 0xFFu) * 4u);
 eax_ = edx_;
 eax_ = (eax_ << 5) & U32MASK;
 eax_ = (eax_ - edx_) & U32MASK;
 esi_ = D3U_TBL_4441F4_32(&c->st, 0);
 eax_ >>= 5;
 eax_ = (eax_ + esi_) & U32MASK;
 D3U_TBL_443B60_32(&c->st, (ecx_ & 0xFFu) * 4u) = eax_;
eax_ = (eax_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
/* retn */ return (eax_ & 0xFFu);
        default:
            break;
        }
        break;
    }
    return 0;
}
/* ignore t0 (next instr overwrites or calls). Void. */
void d3uc_rescale_blk01_model(dec3w_ctx *c)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int ecx_ = 0;
unsigned int edx_ = 0;
unsigned int esi_ = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
 esi_ = (uint32_t)D3U_BLK01_DICT_COPY_32(&c->st, 0);
 ecx_ = 0u;
            _pc = 1;
            continue;
        case 1:
    /* l_4141e0: */
 ebx_ = (esi_ + ecx_) & U32MASK;
if (G8(ebx_) == 0u) { _pc = 5; continue; }
eax_ = (eax_ & 0xFFFFFF00U) | 0x0Du;
eax_ = (eax_ & 0xFFFF00FFU);
            _pc = 2;
            continue;
        case 2:
    /* l_4141ec: */
eax_ = (eax_ & 0xFFFFFF00U) | (((eax_ & 0xFFu) - 1u) & 0xFFu);
 edx_ = 0u;
edx_ = (edx_ & 0xFFFFFF00U) | (eax_ & 0xFFu);
 M8(edx_ + ebx_ + 1u) = (unsigned char)(M8(edx_ + ebx_ + 1u) >> 2);
 eax_ = (eax_ & 0xFFFF00FFU) | (((((eax_ >> 8) & 0xFFu) + (unsigned int)G8(edx_ + ebx_ + 1u)) & 0xFFu) << 8);
if ((eax_ & 0xFFu) != 0u) { _pc = 2; continue; }
if (((eax_ >> 8) & 0xFFu) == 0u) { _pc = 3; continue; }
 edx_ = 0u;
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)G8(ebx_);
 edx_ = (unsigned int)(((int)edx_) >> 2) & U32MASK;
 edx_ = (edx_ + 1u) & U32MASK;
_pc = 4;
continue;
        case 3:
    /* l_41420d: */
 edx_ = 0u;
            _pc = 4;
            continue;
        case 4:
    /* l_41420f: */
 M8(ebx_) = (unsigned char)(edx_ & 0xFFu);
 M8(ebx_ + 0x1Bu) = (unsigned char)(M8(ebx_ + 0x1Bu) >> 1);
            _pc = 5;
            continue;
        case 5:
    /* l_414214: */
 ecx_ = (ecx_ + 0x20u) & U32MASK;
if (ecx_ != 0x800000u) { _pc = 1; continue; }
 D3U_BLK01_DICT_COPY_32(&c->st, 0) = esi_;
/* retn */ return;
        default:
            break;
        }
        break;
    }
}
/* return; callers overwrite t0 next. Void. */
void d3uc_rescale_blk02_model(dec3w_ctx *c)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int ecx_ = 0;
unsigned int edx_ = 0;
unsigned int esi_ = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
 esi_ = (uint32_t)D3U_BLK02_MODEL_A_32(&c->st, 0);
 ecx_ = 0u;
            _pc = 1;
            continue;
        case 1:
    /* l_414238: */
 ebx_ = (esi_ + ecx_) & U32MASK;
if (G8(ebx_) == 0u) { _pc = 5; continue; }
eax_ = (eax_ & 0xFFFFFF00U) | 8u;
eax_ = (eax_ & 0xFFFF00FFU);
            _pc = 2;
            continue;
        case 2:
    /* l_414244: */
eax_ = (eax_ & 0xFFFFFF00U) | (((eax_ & 0xFFu) - 1u) & 0xFFu);
 edx_ = 0u;
edx_ = (edx_ & 0xFFFFFF00U) | (eax_ & 0xFFu);
 M8(edx_ + ebx_ + 1u) = (unsigned char)(M8(edx_ + ebx_ + 1u) >> 2);
 eax_ = (eax_ & 0xFFFF00FFU) | (((((eax_ >> 8) & 0xFFu) + (unsigned int)G8(edx_ + ebx_ + 1u)) & 0xFFu) << 8);
if ((eax_ & 0xFFu) != 0u) { _pc = 2; continue; }
if (((eax_ >> 8) & 0xFFu) == 0u) { _pc = 3; continue; }
 edx_ = 0u;
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)G8(ebx_);
 edx_ = (unsigned int)(((int)edx_) >> 2) & U32MASK;
 edx_ = (edx_ + 1u) & U32MASK;
_pc = 4;
continue;
        case 3:
    /* l_414265: */
 edx_ = 0u;
            _pc = 4;
            continue;
        case 4:
    /* l_414267: */
 M8(ebx_) = (unsigned char)(edx_ & 0xFFu);
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)G8(ebx_ + 0x11u);
 eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)G8(ebx_);
edx_ = (edx_ & 0xFFFFFF00U) | (((edx_ & 0xFFu) >> 1) & 0xFFu);
eax_ = (eax_ & 0xFFFF00FFU) | (((((eax_ >> 8) & 0xFFu) + (eax_ & 0xFFu)) & 0xFFu) << 8);
 M8(ebx_ + 0x11u) = (unsigned char)(edx_ & 0xFFu);
 M8(ebx_ + 0x12u) = (unsigned char)((eax_ >> 8) & 0xFFu);
            _pc = 5;
            continue;
        case 5:
    /* l_414278: */
 ecx_ = (ecx_ + 0x14u) & U32MASK;
if (ecx_ != 0x140000u) { _pc = 1; continue; }
 D3U_BLK02_MODEL_A_32(&c->st, 0) = esi_;
/* retn */ return;
        default:
            break;
        }
        break;
    }
}
/* written before use). Tail sets no return. Void. */
void d3uc_init_hash_buckets(dec3w_ctx *c)
{
unsigned int eax_ = 0;
unsigned int ebx_ = 0;
unsigned int edx_ = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
 ebx_ = 0x1E40u;
 eax_ = D3U_TBL_43D8B0;
 edx_ = 0u;
d3uc_fill(c, ebx_, eax_, edx_);
 edx_ = 2u;
 ebx_ = 0xFFFFFFFFU;
 eax_ = 0u;
 D3U_TBL_43D8B8_32(&c->st, 0) = edx_;
            _pc = 1;
            continue;
        case 1:
    /* l_41b2f9: */
 eax_ = (eax_ + 0x7Cu) & U32MASK;
 D3U_TBL_43F4F0_32(&c->st, eax_) = ebx_;
 D3U_TBL_43F4F4_32(&c->st, eax_) = ebx_;
 D3U_TBL_43F4F8_32(&c->st, eax_) = ebx_;
if (eax_ != 0x1F0u) { _pc = 1; continue; }
/* retn */ return;
        default:
            break;
        }
        break;
    }
}
/* caller overwrites t0 with mov t0,2 next). Void. */
unsigned int d3uc_walk_match_price(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ecx_in, unsigned int ebx_in)
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
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
 ebp_ = D3U_TBL_4441DC_32(&c->st, 0);
 v54 = eax_in;
 v5c = edx_in;
 v44 = ebx_in;
 v1c = ecx_in & 0xFFu;
 eax_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | (ecx_in & 0xFFu);
 ebx_ = d3u_ld32(c, (uint32_t)(d3u_t429DE0[(unsigned int)(eax_ & 0xFFU)]));
 edx_ = d3u_t429E08[(unsigned int)(eax_ & 0xFFU)];
 v34 = ebx_;
 v3c = edx_;
 eax_ = (eax_ << 8) & U32MASK;
 edx_ = 0u;
 eax_ = (eax_ + 0x30000u) & U32MASK;
 v28 = edx_;
 v2c = edx_;
 v58 = eax_;
 edx_ = D3U_DICT_CURSOR_32(&c->st, 0);
eax_ = (eax_ & 0xFFFF00FFU) | (((unsigned int)D3U_FLAG_444EA9_8(&c->st, 0) << 8) & 0xFF00u);
 v24 = edx_;
if ((ecx_in & 0xFFu) != ((eax_ >> 8) & 0xFFu)) { _pc = 1; continue; }
 eax_ = D3U_TBL_4441C4_32(&c->st, 0);
 eax_ = (eax_ - ebx_) & U32MASK;
_pc = 2;
continue;
        case 1:
    /* l_41a3a5: */
 eax_ = 0u;
            _pc = 2;
            continue;
        case 2:
    /* l_41a3a7: */
 v40 = eax_;
 eax_ = D3U_TBL_4441C0_32(&c->st, 0);
 v48 = eax_;
eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA8_8(&c->st, 0);
 v14 = eax_ & 0xFFu;
 eax_ = 0u;
 eax_ = (eax_ & 0xFFFFFF00U) | (v1c & 0xFFu);
 edx_ = (edx_ & 0xFFFFFF00U) | (v1c & 0xFFu);
 eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)d3u_t429E30[(unsigned int)(eax_ & 0xFFU)];
 D3U_TBL_4441DC_32(&c->st, 0) = ebp_;
 v18 = eax_ & 0xFFu;
if ((edx_ & 0xFFu) != 0u) { _pc = 7; continue; }
 ecx_ = 0x400u;
 edi_ = D3U_TBL_443760;
 esi_ = D3U_TBL_443360;
 eax_ = ecx_;
 ecx_ = (ecx_ - edi_) & U32MASK;
 ecx_ = (ecx_ - eax_) & U32MASK;
 ecx_ &= 3u;
 eax_ = (eax_ - ecx_) & U32MASK;
if ((int)eax_ <= 0) { _pc = 3; continue; }
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 ecx_ = eax_;
 eax_ &= 3u;
 ecx_ >>= 2;
 while (ecx_ != 0u) { d3u_st32(c, (uint32_t)(edi_), d3u_ld32(c, (uint32_t)(esi_))); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
            _pc = 3;
            continue;
        case 3:
    /* l_41a408: */
 ecx_ = (ecx_ + eax_) & U32MASK;
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 ecx_ = 0x400u;
 edi_ = D3U_TBL_442958;
 esi_ = D3U_TBL_442128;
 eax_ = ecx_;
 ecx_ = (ecx_ - edi_) & U32MASK;
 ecx_ = (ecx_ - eax_) & U32MASK;
 ecx_ &= 3u;
 eax_ = (eax_ - ecx_) & U32MASK;
if ((int)eax_ <= 0) { _pc = 4; continue; }
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 ecx_ = eax_;
 eax_ &= 3u;
 ecx_ >>= 2;
 while (ecx_ != 0u) { d3u_st32(c, (uint32_t)(edi_), d3u_ld32(c, (uint32_t)(esi_))); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
            _pc = 4;
            continue;
        case 4:
    /* l_41a435: */
 ecx_ = (ecx_ + eax_) & U32MASK;
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 ecx_ = 0x100u;
 edi_ = D3U_TBL_441F18;
 esi_ = D3U_TBL_442D58;
 eax_ = ecx_;
 ecx_ = (ecx_ - edi_) & U32MASK;
 ecx_ = (ecx_ - eax_) & U32MASK;
 ecx_ &= 3u;
 eax_ = (eax_ - ecx_) & U32MASK;
if ((int)eax_ <= 0) { _pc = 5; continue; }
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 ecx_ = eax_;
 eax_ &= 3u;
 ecx_ >>= 2;
 while (ecx_ != 0u) { d3u_st32(c, (uint32_t)(edi_), d3u_ld32(c, (uint32_t)(esi_))); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
            _pc = 5;
            continue;
        case 5:
    /* l_41a462: */
 ecx_ = (ecx_ + eax_) & U32MASK;
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 ecx_ = 0x100u;
 edi_ = D3U_TBL_442028;
 esi_ = D3U_TBL_443260;
 eax_ = ecx_;
 ecx_ = (ecx_ - edi_) & U32MASK;
 ecx_ = (ecx_ - eax_) & U32MASK;
 ecx_ &= 3u;
 eax_ = (eax_ - ecx_) & U32MASK;
if ((int)eax_ <= 0) { _pc = 6; continue; }
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 ecx_ = eax_;
 eax_ &= 3u;
 ecx_ >>= 2;
 while (ecx_ != 0u) { d3u_st32(c, (uint32_t)(edi_), d3u_ld32(c, (uint32_t)(esi_))); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
            _pc = 6;
            continue;
        case 6:
    /* l_41a48f: */
 ecx_ = (ecx_ + eax_) & U32MASK;
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 esi_ = 0x201u;
 ebp_ = D3U_TBL_444210_32(&c->st, 0);
eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EBA_8(&c->st, 0);
 D3U_TBL_4441DC_32(&c->st, 0) = ebp_;
D3U_FLAG_444EB4_8(&c->st, 0) = (unsigned char)(eax_ & 0xFFu);
 v34 = esi_;
            _pc = 7;
            continue;
        case 7:
    /* l_41a4b2: */
 edx_ = v3c;
 ebx_ = v34;
 edi_ = (uint32_t)D3U_TBL_4299F8_32(&c->st, 0);
 esi_ = (uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
 edx_ = (edx_ << 9) & U32MASK;
 ebx_ = (ebx_ << 9) & U32MASK;
 ecx_ = edx_;
 esi_ = (esi_ + ebx_) & U32MASK;
 eax_ = ecx_;
 ecx_ = (ecx_ - edi_) & U32MASK;
 ecx_ = (ecx_ - eax_) & U32MASK;
 ecx_ &= 3u;
 eax_ = (eax_ - ecx_) & U32MASK;
if ((int)eax_ <= 0) { _pc = 8; continue; }
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 ecx_ = eax_;
 eax_ &= 3u;
 ecx_ >>= 2;
 while (ecx_ != 0u) { d3u_st32(c, (uint32_t)(edi_), d3u_ld32(c, (uint32_t)(esi_))); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
            _pc = 8;
            continue;
        case 8:
    /* l_41a4ea: */
 ecx_ = (ecx_ + eax_) & U32MASK;
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 ecx_ = (edx_ + ebx_) & U32MASK;
 edx_ = v3c;
 ebx_ = v34;
 edx_ = (edx_ + edx_) & U32MASK;
 ebx_ = (ebx_ + ebx_) & U32MASK;
 eax_ = edx_;
 edx_ = (edx_ + ebx_) & U32MASK;
            _pc = 9;
            continue;
        case 9:
    /* l_41a501: */
 edi_ = v3c;
 ebp_ = D3U_TBL_4441DC_32(&c->st, 0);
 ecx_ = (ecx_ - 0x200u) & U32MASK;
 eax_ = (eax_ - 2u) & U32MASK;
 edi_ = (edi_ - 1u) & U32MASK;
 edx_ = (edx_ - 2u) & U32MASK;
 v3c = edi_;
if (eax_ == 0xFFFFFFFEU) { _pc = 10; continue; }
 ebx_ = (uint32_t)D3U_BLK17_SMALL_B_32(&c->st, 0);
 esi_ = (uint32_t)D3U_TBL_429A04_32(&c->st, 0);
 ebx_ = (ebx_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(edx_ + ebx_));
 edi_ = 0u;
 d3u_st16(c, (uint32_t)(esi_ + eax_), (unsigned short)(ebx_ & 0xFFFFu));
 esi_ = (uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
edi_ = (edi_ & 0xFFFF0000U) | (ebx_ & 0xFFFFu);
 esi_ = (esi_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(ecx_ + esi_));
 esi_ &= 0xFFFFu;
 esi_ = (esi_ + edi_) & U32MASK;
 ebx_ = esi_;
 esi_ = (uint32_t)D3U_TBL_429A00_32(&c->st, 0);
 d3u_st16(c, (uint32_t)(esi_ + eax_), (unsigned short)(ebx_ & 0xFFFFu));
_pc = 9;
continue;
        case 10:
    /* l_41a55a: */
 eax_ = D3U_TBL_444258_32(&c->st, 0);
 v34 = eax_;
 eax_ = v58;
 eax_ = (eax_ + eax_) & U32MASK;
 edx_ = v54;
 v60 = eax_;
 eax_ = v28;
 eax_ = (eax_ + edx_) & U32MASK;
 v30 = eax_;
            _pc = 11;
            continue;
        case 11:
    /* l_41a57a: */
if (v28 >= 0x100u) { _pc = 12; continue; }
 ecx_ = v24;
 eax_ = (uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
 eax_ = (eax_ + ecx_) & U32MASK;
 eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)c->blk00_dict[eax_ - c->blk00_base];
 v10 = eax_ & 0xFFu;
_pc = 13;
continue;
        case 12:
    /* l_41a597: */
 eax_ = v30;
 eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)G8(eax_ - 0x100u);
edx_ = (edx_ & 0xFFFF00FFU) | ((eax_ & 0xFFu) << 8);
 v10 = eax_ & 0xFFu;
edx_ = (edx_ & 0xFFFF00FFU) | (((~((edx_ >> 8) & 0xFFu)) & 0xFFu) << 8);
 v10 = (edx_ >> 8) & 0xFFu;
            _pc = 13;
            continue;
        case 13:
    /* l_41a5ad: */
if ((v1c & 0xFFu) == 0u) { _pc = 14; continue; }
 ecx_ = (ecx_ & 0xFFFFFF00U) | (v18 & 0xFFu);
 eax_ = v34;
 eax_ >>= (ecx_ & 0xFFu);
if ((v14 & 0xFFu) == 0u) { _pc = 20; continue; }
 eax_ = (eax_ & 0xFFFF00FFU) | ((v10 & 0xFFu) << 8);
ebx_ = (ebx_ & 0xFFFFFF00U) | (eax_ & 0xFFu);
ebx_ = (ebx_ & 0xFFFFFF00U) | (((ebx_ & 0xFFu) - ((eax_ >> 8) & 0xFFu)) & 0xFFu);
_pc = 21;
continue;
        case 14:
    /* l_41a5d6: */
 esi_ = 0u;
ebx_ = (ebx_ & 0xFFFF00FFU) | (((unsigned int)D3U_FLAG_444EB4_8(&c->st, 0)) << 8);
 v40 = esi_;
if (((ebx_ >> 8) & 0xFFu) <= 1u) { _pc = 19; continue; }
 edx_ = 0u;
 eax_ = v28;
edx_ = (edx_ & 0xFFFFFF00U) | ((ebx_ >> 8) & 0xFFu);
 eax_ = (eax_ - edx_) & U32MASK;
if ((int)eax_ >= 0) { _pc = 15; continue; }
 eax_ = v24;
 eax_ = (eax_ - edx_) & U32MASK;
 edx_ = D3U_DICT_MASK_32(&c->st, 0);
 edx_ &= eax_;
 eax_ = (uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
 eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)c->blk00_dict[(edx_ + eax_) - c->blk00_base];
_pc = 16;
continue;
        case 15:
    /* l_41a611: */
 eax_ = (eax_ + v54) & U32MASK;
 eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)G8(eax_);
eax_ = (eax_ & 0xFFFFFF00U) | ((~(eax_ & 0xFFu)) & 0xFFu);
            _pc = 16;
            continue;
        case 16:
    /* l_41a619: */
 ecx_ = 0u;
ecx_ = (ecx_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EB4_8(&c->st, 0);
 edx_ = v28;
 ecx_ = (ecx_ + ecx_) & U32MASK;
 edx_ = (edx_ - ecx_) & U32MASK;
if ((int)edx_ >= 0) { _pc = 17; continue; }
 edx_ = v24;
 edx_ = (edx_ - ecx_) & U32MASK;
 edx_ &= D3U_DICT_MASK_32(&c->st, 0);
 ecx_ = (uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
 eax_ = (eax_ & 0xFFFF00FFU) | (((unsigned int)c->blk00_dict[(ecx_ + edx_) - c->blk00_base] << 8) & 0xFF00u);
_pc = 18;
continue;
        case 17:
    /* l_41a644: */
 edx_ = (edx_ + v54) & U32MASK;
 eax_ = (eax_ & 0xFFFF00FFU) | (((unsigned int)G8(edx_) << 8) & 0xFF00u);
eax_ = (eax_ & 0xFFFF00FFU) | (((~((eax_ >> 8) & 0xFFu)) & 0xFFu) << 8);
            _pc = 18;
            continue;
        case 18:
    /* l_41a64c: */
if ((eax_ & 0xFFu) != ((eax_ >> 8) & 0xFFu)) { _pc = 20; continue; }
 v40 = (v40 + 1u) & U32MASK;
_pc = 20;
continue;
        case 19:
    /* l_41a656: */
 edx_ = v34;
 eax_ = (eax_ & 0xFFFFFF00U) | (v34 & 0xFFu);
 edx_ >>= 8;
if ((edx_ & 0xFFu) != (eax_ & 0xFFu)) { _pc = 20; continue; }
 v40 = 1u;
            _pc = 20;
            continue;
        case 20:
    /* l_41a66d: */
 ebx_ = (ebx_ & 0xFFFFFF00U) | (v10 & 0xFFu);
ebx_ = (ebx_ & 0xFFFFFF00U) | (((ebx_ & 0xFFu) - (eax_ & 0xFFu)) & 0xFFu);
            _pc = 21;
            continue;
        case 21:
    /* l_41a673: */
 eax_ = v40;
 edx_ = 0u;
 eax_ = (eax_ << 8) & U32MASK;
edx_ = (edx_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
 edi_ = v40;
 eax_ = (eax_ + edx_) & U32MASK;
 ecx_ = (uint32_t)D3U_TBL_429A00_32(&c->st, 0);
 esi_ = (eax_ * 2u) & U32MASK;
 edi_ = (edi_ + edi_) & U32MASK;
 eax_ = (uint32_t)D3U_TBL_4299F8_32(&c->st, 0);
 ecx_ = (ecx_ + edi_) & U32MASK;
 esi_ = (esi_ + eax_) & U32MASK;
 v38 = edx_;
 edx_ = (edx_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(ecx_));
 eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(esi_));
 v20 = edx_;
if ((eax_ & 0xFFFFu) != 0u) { _pc = 22; continue; }
 eax_ = v58;
 eax_ = (eax_ + v38) & U32MASK;
 edx_ = (uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
 eax_ = (eax_ + eax_) & U32MASK;
 eax_ = (eax_ + edx_) & U32MASK;
 eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(eax_));
 eax_ &= 0xFFFFu;
 eax_ = (eax_ << 14) & U32MASK;
 edx_ = v60;
 v50 = eax_;
 eax_ = (uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
 eax_ = (eax_ + edx_) & U32MASK;
 eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(eax_));
 eax_ &= 0xFFFFu;
 edx_ = 0u;
 v4c = eax_;
 eax_ = v50;
 qv = eax_; eax_ = qv / v4c; edx_ = qv % v4c;
 eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(D3U_U16_CUM_DST + eax_ * 2u));
 edx_ = v2c;
 eax_ &= 0xFFFFu;
 edx_ = (edx_ + eax_) & U32MASK;
 eax_ = (uint32_t)D3U_TBL_429A04_32(&c->st, 0);
 edi_ = (edi_ + eax_) & U32MASK;
 eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(edi_));
 v2c = edx_;
 edx_ = eax_;
 edx_ = (edx_ + 0x20u) & U32MASK;
 d3u_st16(c, (uint32_t)(edi_), (unsigned short)(edx_ & 0xFFFFu));
 d3u_st16(c, (uint32_t)(ecx_), (unsigned short)((d3u_ld16(c, (uint32_t)(ecx_)) + 0x20u) & 0xFFFFu));
            _pc = 22;
            continue;
        case 22:
    /* l_41a71d: */
 eax_ &= 0xFFFFu;
 ecx_ = 0u;
 edx_ = 0u;
 ecx_ = (ecx_ & 0xFFFF0000U) | (v20 & 0xFFFFu);
 eax_ = (eax_ << 14) & U32MASK;
 qv = eax_; eax_ = qv / ecx_; edx_ = qv % ecx_;
 eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(D3U_U16_CUM_DST + eax_ * 2u));
 ecx_ = v2c;
 eax_ &= 0xFFFFu;
 ecx_ = (ecx_ + eax_) & U32MASK;
 edi_ = v44;
 v2c = ecx_;
if (ecx_ > edi_) { _pc = 37; continue; }
 ecx_ = (ecx_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(esi_));
 eax_ = v40;
 ecx_ = (ecx_ + 0x20u) & U32MASK;
 edx_ = (uint32_t)D3U_TBL_429A00_32(&c->st, 0);
 d3u_st16(c, (uint32_t)(esi_), (unsigned short)(ecx_ & 0xFFFFu));
 esi_ = (esi_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(edx_ + eax_ * 2u));
 esi_ = (esi_ + 0x20u) & U32MASK;
 D3U_TBL_4441DC_32(&c->st, 0) = ebp_;
 d3u_st16(c, (uint32_t)(edx_ + eax_ * 2u), (unsigned short)(esi_ & 0xFFFFu));
if ((esi_ & 0xFFFFu) < 0x4000u) { _pc = 23; continue; }
eax_ = d3uc_fold_small_ctx_block(c, eax_);
            _pc = 23;
            continue;
        case 23:
    /* l_41a783: */
 edx_ = (edx_ & 0xFFFFFF00U) | (v1c & 0xFFu);
 ebp_ = D3U_TBL_4441DC_32(&c->st, 0);
if ((edx_ & 0xFFu) == 0u) { _pc = 30; continue; }
if ((ebx_ & 0xFFu) < 0x80u) { _pc = 24; continue; }
 ebx_ = (ebx_ & 0xFFFF00FFU) | ((v14 & 0xFFu) << 8);
eax_ = (eax_ & 0xFFFFFF00U) | (((((ebx_ >> 8) & 0xFFu) == 0u) ? 1u : 0u));
 v14 = eax_ & 0xFFu;
            _pc = 24;
            continue;
        case 24:
    /* l_41a7a7: */
 ecx_ = (ecx_ & 0xFFFFFF00U) | (v1c & 0xFFu);
if ((ecx_ & 0xFFu) != 1u) { _pc = 25; continue; }
 eax_ = v40;
 eax_ &= 0xFu;
 eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)d3u_t429E44[(unsigned int)(eax_ & 0x0FU)];
 edx_ = eax_;
 eax_ = 0u;
 edx_ &= 0xFFu;
eax_ = (eax_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
 edx_ = (edx_ << 4) & U32MASK;
 eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)d3u_t429E54[(unsigned int)(eax_ & 0xFFU)];
 eax_ &= 0xFFu;
 edx_ = (edx_ + eax_) & U32MASK;
 v40 = edx_;
_pc = 29;
continue;
        case 25:
    /* l_41a7df: */
if ((ecx_ & 0xFFu) != 2u) { _pc = 26; continue; }
 eax_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
 edx_ = 0u;
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)d3u_t429E54[(unsigned int)(eax_ & 0xFFU)];
 eax_ = v48;
 eax_ &= 1u;
_pc = 28;
continue;
        case 26:
    /* l_41a7f9: */
if ((ecx_ & 0xFFu) != 3u) { _pc = 27; continue; }
 eax_ = 0u;
 ecx_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
 edx_ = 0u;
 ebx_ = 3u;
 ecx_ = (ecx_ & 0xFFFFFF00U) | (unsigned int)d3u_t429F54[(unsigned int)(eax_ & 0xFFU)];
 eax_ = v48;
 qv = eax_; eax_ = qv / ebx_; edx_ = qv % ebx_;
 edx_ = (edx_ << 5) & U32MASK;
 edx_ = (edx_ + ecx_) & U32MASK;
 v40 = edx_;
 v48 = (v48 + 1u) & U32MASK;
_pc = 36;
continue;
        case 27:
    /* l_41a829: */
 edx_ = 0u;
edx_ = (edx_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
 edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)d3u_t429E54[(unsigned int)(edx_ & 0xFFU)];
 eax_ = v48;
 eax_ &= 3u;
 edx_ &= 0xFFu;
            _pc = 28;
            continue;
        case 28:
    /* l_41a840: */
 eax_ = (eax_ << 4) & U32MASK;
 eax_ = (eax_ + edx_) & U32MASK;
 v40 = eax_;
            _pc = 29;
            continue;
        case 29:
    /* l_41a849: */
 v48 = (v48 + 1u) & U32MASK;
_pc = 36;
continue;
        case 30:
    /* l_41a852: */
 edx_ = (edx_ & 0xFFFF00FFU) | ((v10 & 0xFFu) << 8);
if (((edx_ >> 8) & 0xFFu) == 0u) { _pc = 36; continue; }
if (((edx_ >> 8) & 0xFFu) == 0xFFu) { _pc = 36; continue; }
 eax_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | ((edx_ >> 8) & 0xFFu);
 edx_ = v24;
 ebx_ = D3U_TBL_443760_32(&c->st, (eax_ & 0xFFu) * 4u);
 eax_ = edx_;
 ecx_ = D3U_DICT_MASK_32(&c->st, 0);
 eax_ = (eax_ - ebx_) & U32MASK;
 eax_ &= ecx_;
if (eax_ < 0x100u) { _pc = 31; continue; }
 eax_ = (eax_ & 0xFFFFFF00U) | (v10 & 0xFFu);
eax_ = (eax_ & 0xFFFFFF00U) | (((eax_ & 0xFFu) - 1u) & 0xFFu);
 eax_ &= 0xFFu;
 esi_ = D3U_TBL_443760_32(&c->st, eax_ * 4u);
 eax_ = edx_;
 eax_ = (eax_ - esi_) & U32MASK;
 eax_ &= ecx_;
if (eax_ < 0x100u) { _pc = 31; continue; }
 eax_ = (eax_ & 0xFFFFFF00U) | (v10 & 0xFFu);
eax_ = (eax_ & 0xFFFFFF00U) | (((eax_ & 0xFFu) + 1u) & 0xFFu);
 eax_ &= 0xFFu;
 ebx_ = D3U_TBL_443760_32(&c->st, eax_ * 4u);
 eax_ = edx_;
 eax_ = (eax_ - ebx_) & U32MASK;
 eax_ &= ecx_;
            _pc = 31;
            continue;
        case 31:
    /* l_41a8c0: */
if (eax_ == 0u) { _pc = 35; continue; }
if (eax_ >= 0x100u) { _pc = 35; continue; }
 edi_ = (eax_ * 4u) & U32MASK;
 esi_ = D3U_TBL_442958_32(&c->st, edi_);
 edx_ = 0u;
 esi_ = (esi_ + 1u) & U32MASK;
edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EB4_8(&c->st, 0);
 D3U_TBL_442958_32(&c->st, edi_) = esi_;
if (eax_ == edx_) { _pc = 33; continue; }
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)D3U_TBL_441F18_8(&c->st, eax_);
            _pc = 32;
            continue;
        case 32:
    /* l_41a8f9: */
 edx_ = 0u;
edx_ = (edx_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
 esi_ = D3U_TBL_442958_32(&c->st, edi_);
if (esi_ < D3U_TBL_442958_32(&c->st, (edx_ & 0xFFu) * 4u)) { _pc = 33; continue; }
 ebx_ = (ebx_ & 0xFFFF00FFU) | (((unsigned int)D3U_TBL_441F18_8(&c->st, (edx_ & 0xFFu))) << 8);
 D3U_TBL_441F18_8(&c->st, eax_) = (unsigned char)((ebx_ >> 8) & 0xFFu);
 ebx_ = (ebx_ & 0xFFFF00FFU) | (((unsigned int)D3U_TBL_442028_8(&c->st, eax_)) << 8);
 D3U_TBL_442028_8(&c->st, (edx_ & 0xFFu)) = (unsigned char)((ebx_ >> 8) & 0xFFu);
 D3U_TBL_441F18_8(&c->st, (edx_ & 0xFFu)) = (unsigned char)(eax_ & 0xFFu);
 D3U_TBL_442028_8(&c->st, eax_) = (unsigned char)(ebx_ & 0xFFu);
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)D3U_TBL_441F18_8(&c->st, eax_);
if ((ebx_ & 0xFFu) != 0u) { _pc = 32; continue; }
D3U_FLAG_444EB4_8(&c->st, 0) = (unsigned char)(eax_ & 0xFFu);
            _pc = 33;
            continue;
        case 33:
    /* l_41a93f: */
 ebp_ = (ebp_ + 1u) & U32MASK;
if (ebp_ <= 0x100u) { _pc = 35; continue; }
 ebp_ = 0u;
 eax_ = 0u;
            _pc = 34;
            continue;
        case 34:
    /* l_41a94c: */
 esi_ = D3U_TBL_442958_32(&c->st, eax_);
 esi_ >>= 1;
 eax_ = (eax_ + 4u) & U32MASK;
 ebp_ = (ebp_ + esi_) & U32MASK;
 D3U_TBL_442954_32(&c->st, eax_) = esi_;
if (eax_ != 0x400u) { _pc = 34; continue; }
            _pc = 35;
            continue;
        case 35:
    /* l_41a966: */
 eax_ = 0u;
 eax_ = (eax_ & 0xFFFFFF00U) | (v10 & 0xFFu);
 edx_ = v24;
 D3U_TBL_443760_32(&c->st, (eax_ & 0xFFu) * 4u) = edx_;
            _pc = 36;
            continue;
        case 36:
    /* l_41a977: */
 eax_ = v34;
 edi_ = v24;
 edx_ = 0u;
 ebx_ = v5c;
 edx_ = (edx_ & 0xFFFFFF00U) | (v10 & 0xFFu);
 edi_ = (edi_ + 1u) & U32MASK;
 eax_ = (eax_ << 8) & U32MASK;
 v24 = edi_;
 eax_ = (eax_ + edx_) & U32MASK;
 edx_ = v28;
 v34 = eax_;
 eax_ = v30;
 edx_ = (edx_ + 1u) & U32MASK;
 eax_ = (eax_ + 1u) & U32MASK;
 v28 = edx_;
 v30 = eax_;
if (edx_ < ebx_) { _pc = 11; continue; }
            _pc = 37;
            continue;
        case 37:
    /* l_41a9b1: */
 eax_ = v2c;
 D3U_TBL_4441DC_32(&c->st, 0) = ebp_;
/* retn */ qv = 0; (void)sv; return eax_;
        default:
            break;
        }
        break;
    }
    return 0;
}
/* agrees: (t0,t3,cl,t1)). Exit al (0 or var_18 byte). */
unsigned int d3uc_refine_match_price(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in, unsigned int ecx_in, unsigned int ebx_in)
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
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
 v5c = eax_in;
 v38 = edx_in;
 v3c = ebx_in;
 eax_ = D3U_DICT_CURSOR_32(&c->st, 0);
 v2c = eax_;
 eax_ = D3U_TBL_4441C0_32(&c->st, 0);
 v54 = eax_;
 eax_ = D3U_TBL_4441D0_32(&c->st, 0);
 v48 = eax_;
 eax_ = D3U_TBL_4441C4_32(&c->st, 0);
 v4c = eax_;
eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA8_8(&c->st, 0);
 v10 = eax_ & 0xFFu;
eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA9_8(&c->st, 0);
 edx_ = 0u;
 v14 = eax_ & 0xFFu;
eax_ = (eax_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444E9D_8(&c->st, 0);
 ebp_ = 0u;
 v1c = eax_ & 0xFFu;
eax_ = (eax_ & 0xFFFF00FFU) | (1u << 8);
 v24 = edx_;
D3U_FLAG_444EAB_8(&c->st, 0) = (unsigned char)((eax_ >> 8) & 0xFFu);
if ((ecx_in & 0xFFu) != ((eax_ >> 8) & 0xFFu)) { _pc = 11; continue; }
 ecx_ = 0x100u;
 ebx_ = v2c;
 eax_ = (uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
 esi_ = v38;
 eax_ = (eax_ + ebx_) & U32MASK;
if (esi_ >= ecx_) { _pc = 1; continue; }
 ecx_ = esi_;
            _pc = 1;
            continue;
        case 1:
    /* l_41ae73: */
 edi_ = 0xFFFFFFFFU;
            _pc = 2;
            continue;
        case 2:
    /* l_41ae78: */
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)c->blk00_dict[eax_ - c->blk00_base];
if ((ebx_ & 0xFFu) < 0x40u) { _pc = 3; continue; }
if ((ebx_ & 0xFFu) < 0xC0u) { _pc = 4; continue; }
            _pc = 3;
            continue;
        case 3:
    /* l_41ae84: */
 esi_ = 1u;
_pc = 5;
continue;
        case 4:
    /* l_41ae8b: */
 esi_ = edi_;
            _pc = 5;
            continue;
        case 5:
    /* l_41ae8d: */
 eax_ = (eax_ + 1u) & U32MASK;
 edx_ = (edx_ + esi_) & U32MASK;
 ecx_ = (ecx_ - 1u) & U32MASK;
if (ecx_ != 0u) { _pc = 2; continue; }
 eax_ = v38;
if (eax_ <= 0x100u) { _pc = 10; continue; }
 ecx_ = (eax_ - 0x100u) & U32MASK;
 esi_ = 0xFFFFFFFFU;
 eax_ = v5c;
            _pc = 6;
            continue;
        case 6:
    /* l_41aead: */
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)G8(eax_);
ebx_ = (ebx_ & 0xFFFFFF00U) | ((~(ebx_ & 0xFFu)) & 0xFFu);
if ((ebx_ & 0xFFu) < 0x40u) { _pc = 7; continue; }
if ((ebx_ & 0xFFu) < 0xC0u) { _pc = 8; continue; }
            _pc = 7;
            continue;
        case 7:
    /* l_41aebb: */
 ebx_ = 1u;
_pc = 9;
continue;
        case 8:
    /* l_41aec2: */
 ebx_ = esi_;
            _pc = 9;
            continue;
        case 9:
    /* l_41aec4: */
 eax_ = (eax_ + 1u) & U32MASK;
 edx_ = (edx_ + ebx_) & U32MASK;
 ecx_ = (ecx_ - 1u) & U32MASK;
if (ecx_ != 0u) { _pc = 6; continue; }
            _pc = 10;
            continue;
        case 10:
    /* l_41aeca: */
 eax_ = (eax_ & 0xFFFFFF00U) | ((((int)edx_ > 0) ? 1u : 0u));
eax_ = (eax_ & 0xFFFFFF00U) | (((eax_ & 0xFFu) + 5u) & 0xFFu);
 v18 = eax_ & 0xFFu;
_pc = 33;
continue;
        case 11:
    /* l_41aeda: */
if ((ecx_in & 0xFFu) != 2u) { _pc = 32; continue; }
 edx_ = 0x100u;
 ebx_ = v2c;
 edi_ = v38;
 eax_ = (uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
 esi_ = 0u;
 ecx_ = 0u;
 eax_ = (eax_ + ebx_) & U32MASK;
if (edi_ >= edx_) { _pc = 12; continue; }
 edx_ = edi_;
            _pc = 12;
            continue;
        case 12:
    /* l_41af01: */
 edi_ = 0xFFFFFFFFU;
            _pc = 13;
            continue;
        case 13:
    /* l_41af06: */
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)c->blk00_dict[eax_ - c->blk00_base];
if ((ebx_ & 0xFFu) < 0x40u) { _pc = 14; continue; }
if ((ebx_ & 0xFFu) < 0xC0u) { _pc = 15; continue; }
            _pc = 14;
            continue;
        case 14:
    /* l_41af12: */
 ebx_ = 1u;
_pc = 16;
continue;
        case 15:
    /* l_41af19: */
 ebx_ = edi_;
            _pc = 16;
            continue;
        case 16:
    /* l_41af1b: */
 eax_ = (eax_ + 1u) & U32MASK;
 esi_ = (esi_ + ebx_) & U32MASK;
 edx_ = (edx_ - 1u) & U32MASK;
if (edx_ == 0u) { _pc = 20; continue; }
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)c->blk00_dict[eax_ - c->blk00_base];
if ((ebx_ & 0xFFu) < 0x40u) { _pc = 17; continue; }
if ((ebx_ & 0xFFu) < 0xC0u) { _pc = 18; continue; }
            _pc = 17;
            continue;
        case 17:
    /* l_41af2d: */
 ebx_ = 1u;
_pc = 19;
continue;
        case 18:
    /* l_41af34: */
 ebx_ = edi_;
            _pc = 19;
            continue;
        case 19:
    /* l_41af36: */
 eax_ = (eax_ + 1u) & U32MASK;
 ecx_ = (ecx_ + ebx_) & U32MASK;
 edx_ = (edx_ - 1u) & U32MASK;
if (edx_ != 0u) { _pc = 13; continue; }
            _pc = 20;
            continue;
        case 20:
    /* l_41af3c: */
 edi_ = v38;
if (edi_ <= 0x100u) { _pc = 28; continue; }
 edx_ = (edi_ - 0x100u) & U32MASK;
 eax_ = v5c;
 edi_ = 0xFFFFFFFFU;
            _pc = 21;
            continue;
        case 21:
    /* l_41af57: */
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)G8(eax_);
ebx_ = (ebx_ & 0xFFFFFF00U) | ((~(ebx_ & 0xFFu)) & 0xFFu);
if ((ebx_ & 0xFFu) < 0x40u) { _pc = 22; continue; }
if ((ebx_ & 0xFFu) < 0xC0u) { _pc = 23; continue; }
            _pc = 22;
            continue;
        case 22:
    /* l_41af65: */
 ebx_ = 1u;
_pc = 24;
continue;
        case 23:
    /* l_41af6c: */
 ebx_ = edi_;
            _pc = 24;
            continue;
        case 24:
    /* l_41af6e: */
 eax_ = (eax_ + 1u) & U32MASK;
 esi_ = (esi_ + ebx_) & U32MASK;
 edx_ = (edx_ - 1u) & U32MASK;
if (edx_ == 0u) { _pc = 28; continue; }
 ebx_ = (ebx_ & 0xFFFFFF00U) | (unsigned int)G8(eax_);
ebx_ = (ebx_ & 0xFFFFFF00U) | ((~(ebx_ & 0xFFu)) & 0xFFu);
if ((ebx_ & 0xFFu) < 0x40u) { _pc = 25; continue; }
if ((ebx_ & 0xFFu) < 0xC0u) { _pc = 26; continue; }
            _pc = 25;
            continue;
        case 25:
    /* l_41af82: */
 ebx_ = 1u;
_pc = 27;
continue;
        case 26:
    /* l_41af89: */
 ebx_ = edi_;
            _pc = 27;
            continue;
        case 27:
    /* l_41af8b: */
 eax_ = (eax_ + 1u) & U32MASK;
 ecx_ = (ecx_ + ebx_) & U32MASK;
 edx_ = (edx_ - 1u) & U32MASK;
if (edx_ != 0u) { _pc = 21; continue; }
            _pc = 28;
            continue;
        case 28:
    /* l_41af91: */
if ((int)esi_ > 0) { _pc = 29; continue; }
if ((int)ecx_ <= 0) { _pc = 30; continue; }
            _pc = 29;
            continue;
        case 29:
    /* l_41af99: */
eax_ = (eax_ & 0xFFFFFF00U) | 1u;
_pc = 31;
continue;
        case 30:
    /* l_41af9d: */
eax_ = (eax_ & 0xFFFFFF00U);
            _pc = 31;
            continue;
        case 31:
    /* l_41af9f: */
eax_ = (eax_ & 0xFFFFFF00U) | (((eax_ & 0xFFu) + 7u) & 0xFFu);
 v18 = eax_ & 0xFFu;
_pc = 33;
continue;
        case 32:
    /* l_41afa7: */
 v18 = 9u;
            _pc = 33;
            continue;
        case 33:
    /* l_41afac: */
 edx_ = 0u;
 edx_ = (edx_ & 0xFFFFFF00U) | (v18 & 0xFFu);
ecx_ = (ecx_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA9_8(&c->st, 0);
 ebx_ = d3u_ld32(c, (uint32_t)(d3u_t429DB8[(unsigned int)(edx_ & 0xFFU)]));
 eax_ = d3u_t429E08[(unsigned int)(edx_ & 0xFFU)];
 v68 = ebx_;
 v34 = eax_;
 ebx_ = d3u_ld32(c, (uint32_t)(d3u_t429DE0[(unsigned int)(edx_ & 0xFFU)]));
 edx_ = (edx_ << 8) & U32MASK;
 eax_ = (eax_ & 0xFFFFFF00U) | (v18 & 0xFFu);
 edx_ = (edx_ + 0x30000u) & U32MASK;
 v60 = ebx_;
 v64 = edx_;
if ((eax_ & 0xFFu) == (ecx_ & 0xFFu)) { _pc = 34; continue; }
 eax_ = ebx_;
 D3U_TBL_4441C4_32(&c->st, 0) = ebx_;
ecx_ = (ecx_ & 0xFFFF00FFU);
 edx_ = 0u;
eax_ = (eax_ & 0xFFFFFF00U) | (((eax_ & 0xFFu) ^ (ebx_ & 0xFFu)) & 0xFFu);
D3U_FLAG_444EA8_8(&c->st, 0) = (unsigned char)((ecx_ >> 8) & 0xFFu);
D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)(eax_ & 0xFFu);
 eax_ = (eax_ & 0xFFFFFF00U) | (v18 & 0xFFu);
 D3U_TBL_4441D0_32(&c->st, 0) = edx_;
D3U_FLAG_444EA9_8(&c->st, 0) = (unsigned char)(eax_ & 0xFFu);
d3uc_init_hash_buckets(c);
            _pc = 34;
            continue;
        case 34:
    /* l_41b01a: */
 edx_ = v34;
 ebx_ = v60;
 edi_ = (uint32_t)D3U_TBL_4299F8_32(&c->st, 0);
 esi_ = (uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
 edx_ = (edx_ << 9) & U32MASK;
 ebx_ = (ebx_ << 9) & U32MASK;
 ecx_ = edx_;
 esi_ = (esi_ + ebx_) & U32MASK;
 eax_ = ecx_;
 ecx_ = (ecx_ - edi_) & U32MASK;
 ecx_ = (ecx_ - eax_) & U32MASK;
 ecx_ &= 3u;
 eax_ = (eax_ - ecx_) & U32MASK;
if ((int)eax_ <= 0) { _pc = 35; continue; }
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 ecx_ = eax_;
 eax_ &= 3u;
 ecx_ >>= 2;
 while (ecx_ != 0u) { d3u_st32(c, (uint32_t)(edi_), d3u_ld32(c, (uint32_t)(esi_))); edi_ = (edi_ + 4u) & U32MASK; esi_ = (esi_ + 4u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
            _pc = 35;
            continue;
        case 35:
    /* l_41b052: */
 ecx_ = (ecx_ + eax_) & U32MASK;
 while (ecx_ != 0u) { M8(edi_) = G8(esi_); edi_ = (edi_ + 1u) & U32MASK; esi_ = (esi_ + 1u) & U32MASK; ecx_ = (ecx_ - 1u) & U32MASK; }
 edx_ = (edx_ + ebx_) & U32MASK;
 ebx_ = v34;
 ecx_ = v60;
 ebx_ = (ebx_ + ebx_) & U32MASK;
 ecx_ = (ecx_ + ecx_) & U32MASK;
 eax_ = ebx_;
 ecx_ = (ecx_ + ebx_) & U32MASK;
            _pc = 36;
            continue;
        case 36:
    /* l_41b068: */
 ebx_ = v34;
 edx_ = (edx_ - 0x200u) & U32MASK;
 eax_ = (eax_ - 2u) & U32MASK;
 ebx_ = (ebx_ - 1u) & U32MASK;
 ecx_ = (ecx_ - 2u) & U32MASK;
 v34 = ebx_;
if (eax_ == 0xFFFFFFFEU) { _pc = 37; continue; }
 ebx_ = (uint32_t)D3U_BLK17_SMALL_B_32(&c->st, 0);
 esi_ = (uint32_t)D3U_TBL_429A04_32(&c->st, 0);
 ebx_ = (ebx_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(ecx_ + ebx_));
 d3u_st16(c, (uint32_t)(esi_ + eax_), (unsigned short)(ebx_ & 0xFFFFu));
 esi_ = (uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
 ebx_ &= 0xFFFFu;
 esi_ = (esi_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(edx_ + esi_));
 esi_ &= 0xFFFFu;
 ebx_ = (ebx_ + esi_) & U32MASK;
 esi_ = (uint32_t)D3U_TBL_429A00_32(&c->st, 0);
 d3u_st16(c, (uint32_t)(esi_ + eax_), (unsigned short)(ebx_ & 0xFFFFu));
_pc = 36;
continue;
        case 37:
    /* l_41b0ba: */
 eax_ = v64;
 eax_ = (eax_ + eax_) & U32MASK;
 ecx_ = v5c;
 v40 = eax_;
 eax_ = v24;
 eax_ = (eax_ + ecx_) & U32MASK;
 v44 = eax_;
            _pc = 38;
            continue;
        case 38:
    /* l_41b0d2: */
if (v24 >= 0x100u) { _pc = 39; continue; }
 edi_ = v2c;
 eax_ = (uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
 eax_ = (eax_ + edi_) & U32MASK;
 ebx_ = (ebx_ & 0xFFFF00FFU) | (((unsigned int)c->blk00_dict[eax_ - c->blk00_base]) << 8);
_pc = 40;
continue;
        case 39:
    /* l_41b0eb: */
 ebx_ = v44;
 ebx_ = (ebx_ & 0xFFFF00FFU) | (((unsigned int)G8(ebx_ - 0x100u)) << 8);
ebx_ = (ebx_ & 0xFFFF00FFU) | (((~((ebx_ >> 8) & 0xFFu)) & 0xFFu) << 8);
            _pc = 40;
            continue;
        case 40:
    /* l_41b0f7: */
eax_ = (eax_ & 0xFFFF00FFU) | (((unsigned int)D3U_FLAG_444E9D_8(&c->st, 0) << 8) & 0xFF00u);
ebx_ = (ebx_ & 0xFFFFFF00U) | ((ebx_ >> 8) & 0xFFu);
edx_ = (edx_ & 0xFFFFFF00U) | (unsigned int)D3U_FLAG_444EA8_8(&c->st, 0);
ebx_ = (ebx_ & 0xFFFFFF00U) | (((ebx_ & 0xFFu) - ((eax_ >> 8) & 0xFFu)) & 0xFFu);
if ((edx_ & 0xFFu) == 0u) { _pc = 41; continue; }
ebx_ = (ebx_ & 0xFFFFFF00U) | (((-(int)(ebx_ & 0xFFu)) & 0xFFu));
            _pc = 41;
            continue;
        case 41:
    /* l_41b10d: */
if ((ebx_ & 0xFFu) < 0x80u) { _pc = 42; continue; }
edx_ = (edx_ & 0xFFFF00FFU) | (((unsigned int)D3U_FLAG_444EA8_8(&c->st, 0) << 8) & 0xFF00u);
eax_ = (eax_ & 0xFFFFFF00U) | (((((edx_ >> 8) & 0xFFu) == 0u) ? 1u : 0u));
D3U_FLAG_444EA8_8(&c->st, 0) = (unsigned char)(eax_ & 0xFFu);
            _pc = 42;
            continue;
        case 42:
    /* l_41b122: */
 eax_ = v60;
 ecx_ = D3U_TBL_4441C4_32(&c->st, 0);
 ecx_ = (ecx_ - eax_) & U32MASK;
 edx_ = 0u;
 eax_ = ecx_;
edx_ = (edx_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
 eax_ = (eax_ << 8) & U32MASK;
 eax_ = (eax_ + edx_) & U32MASK;
 esi_ = (uint32_t)D3U_TBL_4299F8_32(&c->st, 0);
 eax_ = (eax_ + eax_) & U32MASK;
 esi_ = (esi_ + eax_) & U32MASK;
 v50 = esi_;
 eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(esi_));
 esi_ = (ecx_ + ecx_) & U32MASK;
 v30 = esi_;
 edi_ = v30;
 esi_ = (uint32_t)D3U_TBL_429A00_32(&c->st, 0);
 edi_ = (edi_ + esi_) & U32MASK;
 esi_ = (esi_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(edi_));
 v20 = esi_;
 esi_ = D3U_TBL_4441C4_32(&c->st, 0);
 v28 = edi_;
 esi_ = (esi_ & 0xFFFF0000U) | (unsigned int)D3U_TBL_44428A_16(&c->st, esi_ * 2u);
if ((eax_ & 0xFFFFu) != 0u) { _pc = 43; continue; }
 edx_ = (edx_ + v64) & U32MASK;
 eax_ = (uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
 edx_ = (edx_ + edx_) & U32MASK;
 edx_ = (edx_ + eax_) & U32MASK;
 eax_ = 0u;
 edi_ = v40;
 eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(edx_));
 edx_ = (uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
 edx_ = (edx_ + edi_) & U32MASK;
 eax_ = (eax_ << 14) & U32MASK;
 edx_ = (edx_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(edx_));
 edx_ &= 0xFFFFu;
 v58 = edx_;
 edi_ = v58;
 edx_ = 0u;
 qv = eax_; eax_ = qv / edi_; edx_ = qv % edi_;
 eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(D3U_U16_CUM_DST + eax_ * 2u));
 eax_ &= 0xFFFFu;
 edx_ = (uint32_t)D3U_TBL_429A04_32(&c->st, 0);
 ebp_ = (ebp_ + eax_) & U32MASK;
 edx_ = (edx_ + v30) & U32MASK;
 eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(edx_));
 edi_ = eax_;
 edi_ = (edi_ + esi_) & U32MASK;
 d3u_st16(c, (uint32_t)(edx_), (unsigned short)(edi_ & 0xFFFFu));
 edx_ = v28;
 d3u_st16(c, (uint32_t)(edx_), (unsigned short)((d3u_ld16(c, (uint32_t)(edx_)) + (esi_ & 0xFFFFu)) & 0xFFFFu));
            _pc = 43;
            continue;
        case 43:
    /* l_41b1db: */
 eax_ &= 0xFFFFu;
 edi_ = 0u;
 edx_ = 0u;
 edi_ = (edi_ & 0xFFFF0000U) | (v20 & 0xFFFFu);
 eax_ = (eax_ << 14) & U32MASK;
 qv = eax_; eax_ = qv / edi_; edx_ = qv % edi_;
 eax_ = (eax_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(D3U_U16_CUM_DST + eax_ * 2u));
 eax_ &= 0xFFFFu;
 edx_ = v3c;
 ebp_ = (ebp_ + eax_) & U32MASK;
if (ebp_ > edx_) { _pc = 45; continue; }
 eax_ = v50;
 d3u_st16(c, (uint32_t)(eax_), (unsigned short)((d3u_ld16(c, (uint32_t)(eax_)) + (esi_ & 0xFFFFu)) & 0xFFFFu));
 eax_ = (uint32_t)D3U_TBL_429A00_32(&c->st, 0);
 edi_ = (edi_ & 0xFFFF0000U) | (unsigned int)d3u_ld16(c, (uint32_t)(eax_ + ecx_ * 2u));
 edi_ = (edi_ + esi_) & U32MASK;
 d3u_st16(c, (uint32_t)(eax_ + ecx_ * 2u), (unsigned short)(edi_ & 0xFFFFu));
if ((edi_ & 0xFFFFu) < 0x4000u) { _pc = 44; continue; }
 eax_ = ecx_;
eax_ = d3uc_fold_small_ctx_block(c, eax_);
            _pc = 44;
            continue;
        case 44:
    /* l_41b229: */
 edx_ = 0u;
 eax_ = 0u;
edx_ = (edx_ & 0xFFFFFF00U) | (ebx_ & 0xFFu);
eax_ = (eax_ & 0xFFFFFF00U) | ((ebx_ >> 8) & 0xFFu);
d3uc_walk_hash_chain(c, eax_, edx_);
 ebx_ = D3U_TBL_4441C0_32(&c->st, 0);
 ecx_ = v68;
 ebx_ = (ebx_ + 1u) & U32MASK;
 edx_ = 0u;
 eax_ = ebx_;
 qv = eax_; eax_ = qv / ecx_; edx_ = qv % ecx_;
 esi_ = v2c;
 edi_ = v44;
 esi_ = (esi_ + 1u) & U32MASK;
 edi_ = (edi_ + 1u) & U32MASK;
 v2c = esi_;
 D3U_TBL_4441C0_32(&c->st, 0) = ebx_;
 D3U_TBL_4441D0_32(&c->st, 0) = edx_;
d3uc_update_hash_bucket(c);
 eax_ = v24;
 v44 = edi_;
 eax_ = (eax_ + 1u) & U32MASK;
 edx_ = v38;
 v24 = eax_;
if (eax_ < edx_) { _pc = 38; continue; }
            _pc = 45;
            continue;
        case 45:
    /* l_41b27e: */
 eax_ = v54;
 D3U_TBL_4441C0_32(&c->st, 0) = eax_;
 eax_ = v48;
 D3U_TBL_4441D0_32(&c->st, 0) = eax_;
 eax_ = v4c;
 D3U_TBL_4441C4_32(&c->st, 0) = eax_;
 eax_ = (eax_ & 0xFFFFFF00U) | (v10 & 0xFFu);
ebx_ = (ebx_ & 0xFFFFFF00U);
D3U_FLAG_444EA8_8(&c->st, 0) = (unsigned char)(eax_ & 0xFFu);
 eax_ = (eax_ & 0xFFFFFF00U) | (v14 & 0xFFu);
D3U_FLAG_444EAB_8(&c->st, 0) = (unsigned char)(ebx_ & 0xFFu);
D3U_FLAG_444EA9_8(&c->st, 0) = (unsigned char)(eax_ & 0xFFu);
 eax_ = (eax_ & 0xFFFFFF00U) | (v1c & 0xFFu);
 ebx_ = v3c;
D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)(eax_ & 0xFFu);
if (ebp_ >= ebx_) { _pc = 46; continue; }
 eax_ = (eax_ & 0xFFFFFF00U) | (v18 & 0xFFu);
_pc = 47;
continue;
        case 46:
    /* l_41b2ca: */
eax_ = (eax_ & 0xFFFFFF00U);
            _pc = 47;
            continue;
        case 47:
    /* l_41b2cc: */
/* retn */ qv = 0; (void)sv; return (eax_ & 0xFFu);
        default:
            break;
        }
        break;
    }
    return 0;
}

/* ---- model leaves (from draft_k3.c) ---- */
/* sub_417D74 (0x00417D74-0x00417F41, ~191 asm lines). Frame: var_1C=v1c,
 * var_18(byte)=v18. */
unsigned int d3uc_update_ctx_coder(dec3w_ctx *c, unsigned int eax_in, unsigned int dl_in)
{
    uint32_t t0 = 0;
    uint32_t t1 = 0;
    uint32_t t2 = 0;
    uint32_t t3 = 0;
    uint32_t t4 = 0;
    uint32_t t5 = 0;
    uint32_t t6 = 0;
    uint32_t v1c = 0;
    uint32_t v18 = 0;
    uint32_t qv = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t0 = (uint32_t)eax_in;
    t1 = (t1 & 0xFFFF00FFU) | ((((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU) << 8);
     t2 = t0;
    v18 = (uint32_t)(dl_in & 0xFFU);
     t5 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
     t0 = (t0 << 9) & U32MASK;
     t3 = 0U;
     t5 = (t5 + t0) & U32MASK;
     v1c = t3;
    t3 = (t3 & 0xFFFFFF00U) | (v18 & 0xFFU);
    if (d3u_ld16(c, (uint32_t)((t5 + (t3 & 0xFFFFU) * 2U) & U32MASK)) != 0) { _pc = 4; continue; }
     t3 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
     t3 = (t3 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)((t3 + t0) & U32MASK));
     t0 = 0U;
     _pc = 1;
     continue;
        case 1:
    /* l_417db3: */
     t6 = (t1 >> 8) & 0xFFU;
    if (t0 >= t6) { _pc = 2; continue; }
     t6 = (uint32_t)M8((D3U_FLAG_444E82 + t0) & U32MASK);
     t4 = (t4 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)((t5 + (t6 & 0xFFFFU) * 2U) & U32MASK));
     t0 = (t0 + 1U) & U32MASK;
     t3 = (t3 - t4) & U32MASK;
    _pc = 1;
    continue;
        case 2:
    /* l_417dca: */
    if ((t3 & 0xFFFFU) == 0) { _pc = 3; continue; }
     t0 = (uint32_t)(uint32_t)D3U_BLK17_SMALL_B_32(&c->st, 0);
     t2 = (t2 + t2) & U32MASK;
     t0 = (t0 + t2) & U32MASK;
     t2 = 0U;
     v1c = t5;
     t2 = (t2 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)(t0 & U32MASK));
     t3 &= 0xFFFFU;
     t0 = (t0 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)(t0 & U32MASK));
     t2 = (t2 + t3) & U32MASK;
     t0 &= 0xFFFFU;
     t3 = 0U;
     t0 = (t0 << 14) & U32MASK;
     qv = t0; t0 = qv / t2; t3 = qv % t2;
     t0 = (t0 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)((D3U_U16_CUM_DST + t0 * 2U) & U32MASK));
     t2 = (uint32_t)D3U_TBL_4441F4_32(&c->st, 0);
     t0 &= 0xFFFFU;
     t2 = (t2 + t0) & U32MASK;
     D3U_TBL_4441F4_32(&c->st, 0) = (unsigned int)t2;
     _pc = 3;
     continue;
        case 3:
    /* l_417e12: */
     t2 = (uint32_t)D3U_TBL_44421C_32(&c->st, 0);
     t5 = t2;
     t0 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
     t5 = (t5 << 9) & U32MASK;
     t5 = (t5 + t0) & U32MASK;
     _pc = 4;
     continue;
        case 4:
    /* l_417e24: */
     t0 = t2;
     t3 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
     t0 = (t0 << 9) & U32MASK;
     t4 = (t4 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)((t3 + t0) & U32MASK));
    if (t2 >= 0x2FDU) { _pc = 6; continue; }
     t3 = (uint32_t)(uint32_t)D3U_BLK17_SMALL_B_32(&c->st, 0);
     t0 = 0U;
     t4 = (t4 & 0xFFFF0000U) | ((((t4 & 0xFFFFU) + (uint32_t)d3u_ld16(c, (uint32_t)((t3 + (t2 & 0xFFFFU) * 2U) & U32MASK))) & 0xFFFFU));
     _pc = 5;
     continue;
        case 5:
    /* l_417e47: */
     t3 = 0U;
    t3 = (t3 & 0xFFFFFF00U) | ((t1 >> 8) & 0xFFU);
    if (t0 >= t3) { _pc = 16; continue; }
     t3 = 0U;
     t3 = (t3 & 0xFFFFFF00U) | (uint32_t)M8((D3U_FLAG_444E82 + t0) & U32MASK);
     t2 = (t2 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)((t5 + (t3 & 0xFFFFU) * 2U) & U32MASK));
     t0 = (t0 + 1U) & U32MASK;
     t4 = (t4 - t2) & U32MASK;
    _pc = 5;
    continue;
        case 6:
    /* l_417e64: */
    if (((t1 >> 8) & 0xFFU) != 0) { _pc = 9; continue; }
    if (v1c == 0U) { _pc = 16; continue; }
     t6 = v1c;
    t0 &= 0xFFFFFF00U;
    _pc = 7;
    continue;
        case 7:
    /* l_417e77: */
     t3 = 0U;
    t3 = (t3 & 0xFFFFFF00U) | (t0 & 0xFFU);
     t3 = (t3 + t3) & U32MASK;
    if (d3u_ld16(c, (uint32_t)((t3 + t6) & U32MASK)) == 0) { _pc = 8; continue; }
     t4 = (t4 & 0xFFFF0000U) | ((((t4 & 0xFFFFU) - (uint32_t)d3u_ld16(c, (uint32_t)((t3 + t5) & U32MASK))) & 0xFFFFU));
     _pc = 8;
     continue;
        case 8:
    /* l_417e88: */
    t0 = (t0 & 0xFFFFFF00U) | (((t0 & 0xFFU) + 1U) & 0xFFU);
    if ((t0 & 0xFFU) == 0U) { _pc = 16; continue; }
    _pc = 7;
    continue;
        case 9:
    /* l_417e92: */
    if (v1c == 0U) { _pc = 14; continue; }
    t1 = (t1 & 0xFFFFFF00U) | 0U;
    _pc = 10;
    continue;
        case 10:
    /* l_417e9a: */
     t0 = 0U;
    t0 = (t0 & 0xFFFFFF00U) | (t1 & 0xFFU);
     t3 = v1c;
     t0 = (t0 + t0) & U32MASK;
     t0 = (t0 + t3) & U32MASK;
    t3 = (t3 & 0xFFFFFF00U) | ((d3u_ld16(c, (uint32_t)(t0 & U32MASK)) != 0) ? 1U : 0U);
     t0 = 0U;
    t2 = (t2 & 0xFFFFFF00U) | (t3 & 0xFFU);
    if ((t3 & 0xFFU) != 0U) { _pc = 12; continue; }
    _pc = 11;
    continue;
        case 11:
    /* l_417eb4: */
     t3 = 0U;
    t3 = (t3 & 0xFFFFFF00U) | ((t1 >> 8) & 0xFFU);
    if (t0 >= t3) { _pc = 12; continue; }
     t3 = t0;
     t2 = (t2 & 0xFFFFFF00U) | (uint32_t)M8((D3U_FLAG_444E82 + t3) & U32MASK);
     t0 = (t0 + 1U) & U32MASK;
    t3 = (t3 & 0xFFFFFF00U) | ((((t1 & 0xFFU) == (t2 & 0xFFU)) ? 1U : 0U));
    t2 = (t2 & 0xFFFFFF00U) | (t3 & 0xFFU);
    if ((t3 & 0xFFU) == 0U) { _pc = 11; continue; }
    _pc = 12;
    continue;
        case 12:
    /* l_417ed0: */
    if ((t2 & 0xFFU) == 0U) { _pc = 13; continue; }
     t0 = 0U;
    t0 = (t0 & 0xFFFFFF00U) | (t1 & 0xFFU);
     t4 = (t4 & 0xFFFF0000U) | ((((t4 & 0xFFFFU) - (uint32_t)d3u_ld16(c, (uint32_t)((t5 + (t0 & 0xFFFFU) * 2U) & U32MASK))) & 0xFFFFU));
     _pc = 13;
     continue;
        case 13:
    /* l_417edc: */
    /* 0x00417EDC inc bl; 0x00417EDE jz loc_417EFD.
     * NOTE: keep the wrap test on the isolated new-bl local (_bl). An
     * earlier form tested (t1 & 0xFFU) after merging, which gcc -O2
     * fused with the merge OR into a full-register je: once bh (or upper
     * bits) are nonzero the exit never fires and bl cycles forever. */
    { uint32_t _bl = (((t1 & 0xFFU) + 1U) & 0xFFU); t1 = (t1 & 0xFFFFFF00U) | _bl; if (_bl == 0U) { _pc = 16; continue; } }
    _pc = 10;
    continue;
        case 14:
    /* l_417ee2: */
     t0 = 0U;
     _pc = 15;
     continue;
        case 15:
    /* l_417ee4: */
     t3 = 0U;
    t3 = (t3 & 0xFFFFFF00U) | ((t1 >> 8) & 0xFFU);
    if (t0 >= t3) { _pc = 16; continue; }
     t3 = 0U;
     t3 = (t3 & 0xFFFFFF00U) | (uint32_t)M8((D3U_FLAG_444E82 + t0) & U32MASK);
     t2 = (t2 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)((t5 + (t3 & 0xFFFFU) * 2U) & U32MASK));
     t0 = (t0 + 1U) & U32MASK;
     t4 = (t4 - t2) & U32MASK;
    _pc = 15;
    continue;
        case 16:
    /* l_417efd: */
     t0 = 0U;
    t0 = (t0 & 0xFFFFFF00U) | (v18 & 0xFFU);
     t0 = (t0 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)((t5 + (t0 & 0xFFFFU) * 2U) & U32MASK));
     t0 &= 0xFFFFU;
     t2 = 0U;
     t3 = 0U;
     t0 = (t0 << 14) & U32MASK;
    t2 = (t2 & 0xFFFF0000U) | (t4 & 0xFFFFU);
     qv = t0; t0 = qv / t2; t3 = qv % t2;
     t0 = (t0 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)((D3U_U16_CUM_DST + t0 * 2U) & U32MASK));
     t2 = (uint32_t)D3U_TBL_4441F4_32(&c->st, 0);
     t0 &= 0xFFFFU;
     t2 = (t2 + t0) & U32MASK;
     D3U_TBL_4441F4_32(&c->st, 0) = (unsigned int)t2;
    D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)((t1 >> 8) & 0xFFU);
    return (unsigned int)t0;
        default:
            break;
        }
        break;
    }
    return 0;
}
/* sub_41A284 (-, ~66 asm lines). No stack frame. */
unsigned int d3uc_fold_small_ctx_block(dec3w_ctx *c, unsigned int eax_in)
{
    uint32_t t0 = 0;
    uint32_t t1 = 0;
    uint32_t t2 = 0;
    uint32_t t3 = 0;
    uint32_t t4 = 0;
    uint32_t t5 = 0;
    uint32_t t6 = 0;
    /* prologue pushes: no C equivalent */
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
     t6 = (uint32_t)eax_in;
     t1 = (uint32_t)eax_in;
     t0 = (uint32_t)(uint32_t)D3U_TBL_4299F8_32(&c->st, 0);
     t1 = (t1 << 9) & U32MASK;
     t1 = (t1 + t0) & U32MASK;
     t2 = 0U;
     t5 = (t1 + 0x200U) & U32MASK;
     _pc = 1;
     continue;
        case 1:
    /* l_41a2a0: */
     t0 = (uint32_t)d3u_ld32(c, (uint32_t)(t1 & U32MASK));
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 4U) & U32MASK));
     t0 = (t0 + 0x10001U) & U32MASK;
     t3 = (t3 + 0x10001U) & U32MASK;
     t0 &= 0xFFFEFFFEU;
     t3 &= 0xFFFEFFFEU;
     t0 = (t0 >> 1) & U32MASK;
     t3 = (t3 >> 1) & U32MASK;
     t1 = (t1 + 0x10U) & U32MASK;
     t4 = (t0 + t3) & U32MASK;
     d3u_st32(c, (uint32_t)((t1 - 0xCU) & U32MASK), (unsigned int)t3);
     d3u_st32(c, (uint32_t)((t1 - 0x10U) & U32MASK), (unsigned int)t0);
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 - 4U) & U32MASK));
     t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 - 8U) & U32MASK));
     t3 = (t3 + 0x10001U) & U32MASK;
     t0 = (t0 + 0x10001U) & U32MASK;
     t3 &= 0xFFFEFFFEU;
     t0 &= 0xFFFEFFFEU;
     t3 = (t3 >> 1) & U32MASK;
     t0 = (t0 >> 1) & U32MASK;
     t2 = (t2 + t4) & U32MASK;
     t4 = (t0 + t3) & U32MASK;
     d3u_st32(c, (uint32_t)((t1 - 4U) & U32MASK), (unsigned int)t3);
     t2 = (t2 + t4) & U32MASK;
     d3u_st32(c, (uint32_t)((t1 - 8U) & U32MASK), (unsigned int)t0);
    if (t1 != t5) { _pc = 1; continue; }
     t0 = (uint32_t)(uint32_t)D3U_TBL_429A04_32(&c->st, 0);
     t3 = (t3 & 0xFFFF0000U) | (uint32_t)d3u_ld16(c, (uint32_t)((t0 + (t6 & 0xFFFFU) * 2U) & U32MASK));
    t3 = (t3 & 0xFFFF0000U) | ((t3 & 0xFFFFU) >> 2);
     t1 = t3;
     d3u_st16(c, (uint32_t)((t0 + (t6 & 0xFFFFU) * 2U) & U32MASK), (unsigned short)(t3 & 0xFFFFU));
     t1 = (t1 + 1U) & U32MASK;
     d3u_st16(c, (uint32_t)((t0 + (t6 & 0xFFFFU) * 2U) & U32MASK), (unsigned short)(t1 & 0xFFFFU));
     t0 = t2;
     t3 = 0U;
     t0 &= 0xFFFFU;
     t2 = (t2 >> 16) & U32MASK;
    t3 = (t3 & 0xFFFF0000U) | (t1 & 0xFFFFU);
     t2 = (t2 + t0) & U32MASK;
     t0 = (t2 + t3) & U32MASK;
     t3 = (uint32_t)(uint32_t)D3U_TBL_429A00_32(&c->st, 0);
     d3u_st16(c, (uint32_t)((t3 + (t6 & 0xFFFFU) * 2U) & U32MASK), (unsigned short)(t0 & 0xFFFFU));
    /* pops + retn: no C equivalent */
    return (unsigned int)t0;
        default:
            break;
        }
        break;
    }
    return 0;
}
/* sub_41B318 (0x0041B318-0x0041B64A, ~261 asm lines). No stack frame,
 * no entry regs, void return (see header). */
void d3uc_update_hash_bucket(dec3w_ctx *c)
{
    uint32_t t0 = 0;
    uint32_t t1 = 0;
    uint32_t t2 = 0;
    uint32_t t3 = 0;
    uint32_t t4 = 0;
    uint32_t t5 = 0;
    uint32_t t6 = 0;
    uint32_t cf = 0;
    /* prologue pushes: no C equivalent */
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
     t4 = (uint32_t)D3U_TBL_4441B0_32(&c->st, 0);
     t5 = (uint32_t)D3U_TBL_4441D0_32(&c->st, 0);
    if (D3U_FLAG_444EAB_8(&c->st, 0) != 0) { _pc = 1; continue; }
    if (t4 < (uint32_t)D3U_TBL_4441B8_32(&c->st, 0)) { _pc = 20; continue; }
    _pc = 1;
    continue;
        case 1:
    /* l_41b33f: */
    if (D3U_FLAG_444EA9_8(&c->st, 0) >= 7) { _pc = 7; continue; }
     t3 = (uint32_t)D3U_TBL_43D8BC_32(&c->st, 0);
     t2 = 0U;
     t6 = (uint32_t)D3U_TBL_43D8B4_32(&c->st, 0);
     D3U_TBL_43DCCC_32(&c->st, 0) = (unsigned int)t2;
     D3U_TBL_43DCD0_32(&c->st, 0) = (unsigned int)t3;
     t0 = (t3 + t3) & U32MASK;
     t2 = (uint32_t)D3U_TBL_4441C0_32(&c->st, 0);
     t3 = (uint32_t)D3U_TBL_43D8C0_32(&c->st, 0);
     t2 = (t2 - t6) & U32MASK;
     t0 = (t0 - t3) & U32MASK;
     t2 &= 0x7FU;
     D3U_TBL_43DCD4_32(&c->st, 0) = (unsigned int)t0;
     t3 = (uint32_t)(uint32_t)D3U_TBL_43DACC_32(&c->st, t2 * 4U);
     t0 = (uint32_t)D3U_TBL_43F504_32(&c->st, 0);
     D3U_TBL_43F50C_32(&c->st, 0) = (unsigned int)t3;
     D3U_TBL_43F508_32(&c->st, 0) = (unsigned int)t0;
     t0 = (uint32_t)D3U_TBL_43F500_32(&c->st, 0);
     t3 = (uint32_t)D3U_TBL_43F528_32(&c->st, 0);
     D3U_TBL_43F504_32(&c->st, 0) = (unsigned int)t0;
     t3 = (t3 - (uint32_t)D3U_TBL_43F4FC_32(&c->st, 0)) & U32MASK;
     t0 = (uint32_t)D3U_TBL_43F510_32(&c->st, 0);
     D3U_TBL_43F500_32(&c->st, 0) = (unsigned int)t3;
     t3 = (uint32_t)D3U_TBL_43F528_32(&c->st, 0);
     t0 = (t0 * t3) & U32MASK;
     D3U_TBL_43F4FC_32(&c->st, 0) = (unsigned int)t3;
     t3 = (uint32_t)D3U_TBL_43F524_32(&c->st, 0);
     t3 = (t3 << 4) & U32MASK;
     t6 = (uint32_t)D3U_TBL_43F500_32(&c->st, 0);
     t0 = (t0 + t3) & U32MASK;
     t3 = (uint32_t)D3U_TBL_43F514_32(&c->st, 0);
     t3 = (t3 * t6) & U32MASK;
     t6 = (uint32_t)D3U_TBL_43F504_32(&c->st, 0);
     t0 = (t0 + t3) & U32MASK;
     t3 = (uint32_t)D3U_TBL_43F518_32(&c->st, 0);
     t3 = (t3 * t6) & U32MASK;
     t6 = (uint32_t)D3U_TBL_43F508_32(&c->st, 0);
     t0 = (t0 + t3) & U32MASK;
     t3 = (uint32_t)D3U_TBL_43F51C_32(&c->st, 0);
     t3 = (t3 * t6) & U32MASK;
     t6 = (uint32_t)D3U_TBL_43F50C_32(&c->st, 0);
     t0 = (t0 + t3) & U32MASK;
     t3 = (uint32_t)D3U_TBL_43F520_32(&c->st, 0);
     t3 = (t3 * t6) & U32MASK;
     t3 = (t3 + t0) & U32MASK;
     t0 = t3;
     t3 = (uint32_t)(((int)t3 >> 31)) & U32MASK;
     cf = (t3 >> 28) & 1U; t3 = (t3 << 4) & U32MASK;
     t0 = (t0 - t3 - cf) & U32MASK;
     t0 = (uint32_t)(((int)t0 >> 4)) & U32MASK;
     D3U_TBL_43DCD8_32(&c->st, 0) = (unsigned int)t0;
     t0 = (uint32_t)(uint32_t)D3U_TBL_43DACC_32(&c->st, t2 * 4U);
     t3 = (uint32_t)D3U_TBL_43D8B8_32(&c->st, 0);
     D3U_TBL_43DCDC_32(&c->st, 0) = (unsigned int)t0;
     t0 = (uint32_t)D3U_TBL_4441C0_32(&c->st, 0);
     t0 = (t0 - t3) & U32MASK;
     t0 &= 0x7FU;
     t0 = (uint32_t)(uint32_t)D3U_TBL_43DACC_32(&c->st, t0 * 4U);
     D3U_TBL_43DCE0_32(&c->st, 0) = (unsigned int)t0;
     t0 = (uint32_t)D3U_TBL_43D8B0_32(&c->st, 0);
     t1 = D3U_TBL_43F4FC;
     t0 = (t0 & 0xFFFFFF00U) | (uint32_t)M8((D3U_TBL_43DCCC + t0 * 4U) & U32MASK);
     t2 = (uint32_t)D3U_TBL_43D8B0_32(&c->st, 0);
    D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    if (t2 < 4U) { _pc = 2; continue; }
    t3 = (t3 & 0xFFFFFF00U) | (uint32_t)D3U_TBL_43D8BC_8(&c->st, 0);
    t0 = (t0 & 0xFFFFFF00U) | (((t0 & 0xFFU) + (t3 & 0xFFU)) & 0xFFU);
    D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    _pc = 2;
    continue;
        case 2:
    /* l_41b47f: */
    if (D3U_FLAG_444EA9_8(&c->st, 0) != 5) { _pc = 3; continue; }
    t3 = (t3 & 0xFFFFFF00U) | (uint32_t)D3U_FLAG_444E9D_8(&c->st, 0);
     t3 = (t3 & 0xFFFFFF00U) | (((t3 & 0xFFU) + 0x80U) & 0xFFU);
     t2 = 0x2E3U;
    D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
    _pc = 4;
    continue;
        case 3:
    /* l_41b49e: */
     t2 = 0x2E7U;
     _pc = 4;
     continue;
        case 4:
    /* l_41b4a3: */
    if (d3u_ld32(c, (uint32_t)((t1 + 0x70U) & U32MASK)) != 0) { _pc = 5; continue; }
     t2 = (t2 + 1U) & U32MASK;
    _pc = 19;
    continue;
        case 5:
    /* l_41b4af: */
    if (d3u_ld32(c, (uint32_t)((t1 + 0x74U) & U32MASK)) != 0) { _pc = 6; continue; }
     t2 = (t2 + 2U) & U32MASK;
    _pc = 19;
    continue;
        case 6:
    /* l_41b4bd: */
    if (d3u_ld32(c, (uint32_t)((t1 + 0x78U) & U32MASK)) != 0) { _pc = 19; continue; }
     t2 = (t2 + 3U) & U32MASK;
    _pc = 19;
    continue;
        case 7:
    /* l_41b4cf: */
     t1 = t5;
     t1 = (t1 << 5) & U32MASK;
     t1 = (t1 - t5) & U32MASK;
     t1 = (t1 << 2) & U32MASK;
     t1 = (t1 + D3U_TBL_43F4FC) & U32MASK;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 8U) & U32MASK));
     d3u_st32(c, (uint32_t)((t1 + 0xCU) & U32MASK), (unsigned int)t3);
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 4U) & U32MASK));
     d3u_st32(c, (uint32_t)((t1 + 8U) & U32MASK), (unsigned int)t3);
     t2 = (uint32_t)d3u_ld32(c, (uint32_t)(t1 & U32MASK));
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0x2CU) & U32MASK));
     t3 = (t3 - t2) & U32MASK;
     d3u_st32(c, (uint32_t)((t1 + 4U) & U32MASK), (unsigned int)t3);
     t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0x2CU) & U32MASK));
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0x14U) & U32MASK));
     t3 = (t3 * t0) & U32MASK;
     d3u_st32(c, (uint32_t)(t1 & U32MASK), (unsigned int)t0);
     t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0x28U) & U32MASK));
     t0 = (t0 << 4) & U32MASK;
     t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 4U) & U32MASK));
     t0 = (t0 + t3) & U32MASK;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0x18U) & U32MASK));
     t3 = (t3 * t2) & U32MASK;
     t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 8U) & U32MASK));
     t0 = (t0 + t3) & U32MASK;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0x1CU) & U32MASK));
     t3 = (t3 * t6) & U32MASK;
     t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0xCU) & U32MASK));
     t0 = (t0 + t3) & U32MASK;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0x20U) & U32MASK));
     t3 = (t3 * t2) & U32MASK;
     t6 = (uint32_t)D3U_TBL_43D8C4_32(&c->st, 0);
     t0 = (t0 + t3) & U32MASK;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0x24U) & U32MASK));
     t3 = (t3 * t6) & U32MASK;
     t3 = (t3 + t0) & U32MASK;
     t0 = t3;
     t3 = (uint32_t)(((int)t3 >> 31)) & U32MASK;
     cf = (t3 >> 28) & 1U; t3 = (t3 << 4) & U32MASK;
     t0 = (t0 - t3 - cf) & U32MASK;
     t0 = (uint32_t)(((int)t0 >> 4)) & U32MASK;
     D3U_TBL_43DCCC_32(&c->st, 0) = (unsigned int)t0;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0x5CU) & U32MASK));
    if (t3 <= (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0x60U) & U32MASK))) { _pc = 8; continue; }
     t3 = (t3 & 0xFFFFFF00U) | (uint32_t)M8((t1 + 0x28U) & U32MASK);
    _pc = 9;
    continue;
        case 8:
    /* l_41b556: */
    t3 = (t3 & 0xFFFFFF00U) | (uint32_t)D3U_TBL_43DCCC_8(&c->st, 0);
    _pc = 9;
    continue;
        case 9:
    /* l_41b55c: */
    D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
    t3 = (t3 & 0xFFFF00FFU) | ((((uint32_t)D3U_FLAG_444EA9_8(&c->st, 0)) & 0xFFU) << 8);
    if (((t3 >> 8) & 0xFFU) != 7U) { _pc = 12; continue; }
    t2 = (t2 & 0xFFFF00FFU) | ((((uint32_t)D3U_FLAG_444E9D_8(&c->st, 0)) & 0xFFU) << 8);
     t2 = (t2 & 0xFFFF00FFU) | (((((t2 >> 8) & 0xFFU) + 0x80U) & 0xFFU) << 8);
     t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t1 + 0x70U) & U32MASK));
    D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)((t2 >> 8) & 0xFFU);
     t2 = (t5 + 0x2EBU) & U32MASK;
    if (t6 != 0U) { _pc = 10; continue; }
     t2 = (t2 + 2U) & U32MASK;
    _pc = 18;
    continue;
        case 10:
    /* l_41b58e: */
    if (d3u_ld32(c, (uint32_t)((t1 + 0x74U) & U32MASK)) != 0) { _pc = 11; continue; }
     t2 = (t2 + 4U) & U32MASK;
    _pc = 18;
    continue;
        case 11:
    /* l_41b599: */
    if (d3u_ld32(c, (uint32_t)((t1 + 0x78U) & U32MASK)) != 0) { _pc = 18; continue; }
     t2 = (t2 + 6U) & U32MASK;
    _pc = 18;
    continue;
        case 12:
    /* l_41b5a4: */
    if (((t3 >> 8) & 0xFFU) != 8U) { _pc = 15; continue; }
    if (d3u_ld32(c, (uint32_t)((t1 + 0x70U) & U32MASK)) == 0) { _pc = 13; continue; }
     t2 = t5;
    _pc = 14;
    continue;
        case 13:
    /* l_41b5b3: */
     t2 = 2U;
     _pc = 14;
     continue;
        case 14:
    /* l_41b5b8: */
     t2 = (t2 + 0x2F3U) & U32MASK;
    _pc = 18;
    continue;
        case 15:
    /* l_41b5c0: */
    if (d3u_ld32(c, (uint32_t)((t1 + 0x70U) & U32MASK)) == 0) { _pc = 16; continue; }
     t2 = t5;
    _pc = 17;
    continue;
        case 16:
    /* l_41b5ca: */
     t2 = 4U;
     _pc = 17;
     continue;
        case 17:
    /* l_41b5cf: */
     t2 = (t2 + 0x2F6U) & U32MASK;
     _pc = 18;
     continue;
        case 18:
    /* l_41b5d5: */
     t3 = (t3 & 0xFFFFFF00U) | (uint32_t)M8((D3U_TBL_43F6EC + t5) & U32MASK);
    D3U_FLAG_444EA8_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
    _pc = 19;
    continue;
        case 19:
    /* l_41b5e1: */
    if (t4 >= (uint32_t)D3U_TBL_4441B8_32(&c->st, 0)) { _pc = 21; continue; }
     t3 = (uint32_t)D3U_TBL_4441BC_32(&c->st, 0);
     t0 = (t4 * 8U) & U32MASK;
     t3 = (t3 + t0) & U32MASK;
    t1 = (t1 & 0xFFFFFF00U) | (uint32_t)D3U_FLAG_444EA8_8(&c->st, 0);
     d3u_st32(c, (uint32_t)(t3 & U32MASK), (unsigned int)t2);
     M8((t3 + 4U) & U32MASK) = (unsigned char)(t1 & 0xFFU);
    t1 = (t1 & 0xFFFFFF00U) | (uint32_t)D3U_FLAG_444E9D_8(&c->st, 0);
     t4 = (t4 + 1U) & U32MASK;
     M8((t3 + 5U) & U32MASK) = (unsigned char)(t1 & 0xFFU);
    _pc = 21;
    continue;
        case 20:
    /* l_41b60f: */
     t3 = (t4 * 8U) & U32MASK;
     t0 = (uint32_t)D3U_TBL_4441BC_32(&c->st, 0);
     t4 = (t4 + 1U) & U32MASK;
     t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + t0) & U32MASK));
     t1 = (t1 & 0xFFFFFF00U) | (uint32_t)M8((t3 + t0 + 4U) & U32MASK);
     t0 = (t0 & 0xFFFFFF00U) | (uint32_t)M8((t3 + t0 + 5U) & U32MASK);
    D3U_FLAG_444EA8_8(&c->st, 0) = (unsigned char)(t1 & 0xFFU);
    D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    _pc = 21;
    continue;
        case 21:
    /* l_41b632: */
     D3U_TBL_4441D0_32(&c->st, 0) = (unsigned int)t5;
     D3U_TBL_4441B0_32(&c->st, 0) = (unsigned int)t4;
     D3U_TBL_4441C4_32(&c->st, 0) = (unsigned int)t2;
    /* pops + retn: no C equivalent */
        default:
            break;
        }
        break;
    }
}
/* sub_41B69C (0x0041B69C-0x0041BEF5, ~699 asm lines). Entry al/dl live,
 * void return (see header). Frame (all zero-init): var_44=v44,
 * var_40=v40, var_3C=v3c, var_38=v38, var_34=v34, var_30=v30,
 * var_2C=v2c, var_28=v28, var_24=v24, var_20=v20, var_1C(byte)v18slot,
 * var_18(byte)=v18. */
void d3uc_walk_hash_chain(dec3w_ctx *c, unsigned int al_in, unsigned int dl_in)
{
    uint32_t t0 = 0;
    uint32_t t1 = 0;
    uint32_t t2 = 0;
    uint32_t t3 = 0;
    uint32_t t4 = 0;
    uint32_t t5 = 0;
    uint32_t t6 = 0;
    uint32_t v44 = 0;
    uint32_t v40 = 0;
    uint32_t v3c = 0;
    uint32_t v38 = 0;
    uint32_t v34 = 0;
    uint32_t v30 = 0;
    uint32_t v2c = 0;
    uint32_t v28 = 0;
    uint32_t v24 = 0;
    uint32_t v20 = 0;
    uint32_t v1c = 0;
    uint32_t v18 = 0;
    /* prologue pushes + sub esp,30h: no C equivalent */
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t1 = (t1 & 0xFFFFFF00U) | ((uint32_t)(al_in & 0xFFU));
    v18 = (uint32_t)(dl_in & 0xFFU);
    if (D3U_FLAG_444EAB_8(&c->st, 0) != 0) { _pc = 1; continue; }
     t0 = (uint32_t)D3U_TBL_4441B0_32(&c->st, 0);
    if (t0 < (uint32_t)D3U_TBL_4441B8_32(&c->st, 0)) { _pc = 21; continue; }
    _pc = 1;
    continue;
        case 1:
    /* l_41b6c4: */
    t3 = (t3 & 0xFFFFFF00U) | (uint32_t)D3U_FLAG_444EA9_8(&c->st, 0);
    if ((t3 & 0xFFU) >= 7U) { _pc = 13; continue; }
     t2 = D3U_TBL_43F4FC;
    if ((t3 & 0xFFU) != 6U) { _pc = 2; continue; }
     t0 = (uint32_t)(((int)(signed char)(t1 & 0xFFU))) & U32MASK;
    _pc = 3;
    continue;
        case 2:
    /* l_41b6e2: */
     t0 = 0U;
    t0 = (t0 & 0xFFFFFF00U) | (t1 & 0xFFU);
     t0 = (t0 - 0x80U) & U32MASK;
     _pc = 3;
     continue;
        case 3:
    /* l_41b6eb: */
     v2c = t0;
     t0 = v2c;
     t6 = (uint32_t)D3U_TBL_43D8BC_32(&c->st, 0);
     t0 = (t0 - t6) & U32MASK;
     v20 = t0;
    t0 = (t0 & 0xFFFFFF00U) | (uint32_t)D3U_TBL_43DCD8_8(&c->st, 0);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) - (v2c & 0xFFU)) & 0xFFU));
     t0 = (uint32_t)(((int)(signed char)(t0 & 0xFFU))) & U32MASK;
     t0 = (t0 << 4) & U32MASK;
     t3 = t0;
     t3 &= 0xFFFU;
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x30U) & U32MASK));
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((D3U_LIN_EXTEND_TBL + t3 * 4U) & U32MASK));
     t4 = (uint32_t)d3u_ld32(c, (uint32_t)(t2 & U32MASK));
     t1 = (t1 + t3) & U32MASK;
     t3 = t0;
     t3 = (t3 - t4) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x30U) & U32MASK), (unsigned int)t1);
     t3 &= 0xFFFU;
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x34U) & U32MASK));
     t5 = (t5 + (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t3 * 4U)) & U32MASK;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)(t2 & U32MASK));
     t3 = (t3 + t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x34U) & U32MASK), (unsigned int)t5);
     t3 &= 0xFFFU;
     t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x38U) & U32MASK));
     t3 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t3 * 4U);
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 4U) & U32MASK));
     t6 = (t6 + t3) & U32MASK;
     t3 = t0;
     t3 = (t3 - t1) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x38U) & U32MASK), (unsigned int)t6);
     t3 &= 0xFFFU;
     t4 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x3CU) & U32MASK));
     t4 = (t4 + (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t3 * 4U)) & U32MASK;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 4U) & U32MASK));
     t3 = (t3 + t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x3CU) & U32MASK), (unsigned int)t4);
     t3 &= 0xFFFU;
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x40U) & U32MASK));
     t3 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t3 * 4U);
     t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 8U) & U32MASK));
     t5 = (t5 + t3) & U32MASK;
     t3 = t0;
     t3 = (t3 - t6) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x40U) & U32MASK), (unsigned int)t5);
     t3 &= 0xFFFU;
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x44U) & U32MASK));
     t1 = (t1 + (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t3 * 4U)) & U32MASK;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 8U) & U32MASK));
     t3 = (t3 + t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x44U) & U32MASK), (unsigned int)t1);
     t3 &= 0xFFFU;
     t4 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x48U) & U32MASK));
     t3 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t3 * 4U);
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0xCU) & U32MASK));
     t4 = (t4 + t3) & U32MASK;
     t3 = t0;
     t3 = (t3 - t5) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x48U) & U32MASK), (unsigned int)t4);
     t3 &= 0xFFFU;
     t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x4CU) & U32MASK));
     t6 = (t6 + (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t3 * 4U)) & U32MASK;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0xCU) & U32MASK));
     t3 = (t3 + t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x4CU) & U32MASK), (unsigned int)t6);
     t3 &= 0xFFFU;
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x50U) & U32MASK));
     t3 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t3 * 4U);
     t4 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x10U) & U32MASK));
     t1 = (t1 + t3) & U32MASK;
     t3 = t0;
     t3 = (t3 - t4) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x50U) & U32MASK), (unsigned int)t1);
     t3 &= 0xFFFU;
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x54U) & U32MASK));
     t3 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t3 * 4U);
     t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x10U) & U32MASK));
     t5 = (t5 + t3) & U32MASK;
     t0 = (t0 + t6) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x54U) & U32MASK), (unsigned int)t5);
     t0 &= 0xFFFU;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x58U) & U32MASK));
     t3 = (t3 + (uint32_t)d3u_ld32(c, (uint32_t)((D3U_LIN_EXTEND_TBL + t0 * 4U) & U32MASK))) & U32MASK;
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x6CU) & U32MASK));
     d3u_st32(c, (uint32_t)((t2 + 0x58U) & U32MASK), (unsigned int)t3);
    t0 = (t0 & 0xFFFFFF00U) | (v2c & 0xFFU);
     t3 = (t3 & 0xFFFFFF00U) | (uint32_t)M8((t2 + 0x28U) & U32MASK);
     t1 = (t1 + 1U) & U32MASK;
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) - (t3 & 0xFFU)) & 0xFFU));
     d3u_st32(c, (uint32_t)((t2 + 0x6CU) & U32MASK), (unsigned int)t1);
     t0 = (uint32_t)(((int)(signed char)(t0 & 0xFFU))) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x2CU) & U32MASK), (unsigned int)t0);
     t0 = v2c;
     t3 = (t3 & 0xFFFF00FFU) | ((((uint32_t)M8((t2 + 0x6CU) & U32MASK)) & 0xFFU) << 8);
     d3u_st32(c, (uint32_t)((t2 + 0x28U) & U32MASK), (unsigned int)t0);
    if ((((t3 >> 8) & 0xFFU) & 0x0FU) != 0) { _pc = 6; continue; }
     t1 = 1U;
     t0 = (t2 + 4U) & U32MASK;
     t4 = 0U;
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x30U) & U32MASK));
     d3u_st32(c, (uint32_t)((t2 + 0x30U) & U32MASK), (unsigned int)t4);
     _pc = 4;
     continue;
        case 4:
    /* l_41b85f: */
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t0 + 0x30U) & U32MASK));
    if (t3 <= t5) { _pc = 5; continue; }
     t3 = t5;
     t4 = t1;
     _pc = 5;
     continue;
        case 5:
    /* l_41b86a: */
     t0 = (t0 + 4U) & U32MASK;
     t1 = (t1 + 1U) & U32MASK;
     d3u_st32(c, (uint32_t)((t0 + 0x2CU) & U32MASK), 0);
    if (((int)t1) < 11) { _pc = 4; continue; }
     t4 = (t4 - 1U) & U32MASK;
    if (t4 > 9U) { _pc = 6; continue; }
    switch (t4) {
    case 0:
        /* loc_41B88C */
         t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x14U) & U32MASK));
        if (((int)t6) <= -32) { _pc = 6; continue; }
         t0 = (t6 - 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t2 + 0x14U) & U32MASK), (unsigned int)t0);
        _pc = 6;
        continue;
    case 1:
        /* loc_41B8A3 */
         t4 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x14U) & U32MASK));
        if (((int)t4) >= 32) { _pc = 6; continue; }
         t5 = (t4 + 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t2 + 0x14U) & U32MASK), (unsigned int)t5);
        _pc = 6;
        continue;
    case 2:
        /* loc_41B8BA */
         t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x18U) & U32MASK));
        if (((int)t3) <= -32) { _pc = 6; continue; }
         t1 = (t3 - 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t2 + 0x18U) & U32MASK), (unsigned int)t1);
        _pc = 6;
        continue;
    case 3:
        /* loc_41B8D1 */
         t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x18U) & U32MASK));
        if (((int)t6) >= 32) { _pc = 6; continue; }
         t0 = (t6 + 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t2 + 0x18U) & U32MASK), (unsigned int)t0);
        _pc = 6;
        continue;
    case 4:
        /* loc_41B8E5 */
         t4 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x1CU) & U32MASK));
        if (((int)t4) <= -32) { _pc = 6; continue; }
         t5 = (t4 - 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t2 + 0x1CU) & U32MASK), (unsigned int)t5);
        _pc = 6;
        continue;
    case 5:
        /* loc_41B8F5 */
         t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x1CU) & U32MASK));
        if (((int)t3) >= 32) { _pc = 6; continue; }
         t1 = (t3 + 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t2 + 0x1CU) & U32MASK), (unsigned int)t1);
        _pc = 6;
        continue;
    case 6:
        /* loc_41B905 */
         t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x20U) & U32MASK));
        if (((int)t6) <= -32) { _pc = 6; continue; }
         t0 = (t6 - 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t2 + 0x20U) & U32MASK), (unsigned int)t0);
        _pc = 6;
        continue;
    case 7:
        /* loc_41B915 */
         t4 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x20U) & U32MASK));
        if (((int)t4) >= 32) { _pc = 6; continue; }
         t5 = (t4 + 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t2 + 0x20U) & U32MASK), (unsigned int)t5);
        _pc = 6;
        continue;
    case 8:
        /* loc_41B925 */
        if (((int)d3u_ld32(c, (uint32_t)((t2 + 0x20U) & U32MASK))) <= -32) { _pc = 6; continue; }
         d3u_st32(c, (uint32_t)((t2 + 0x24U) & U32MASK), (unsigned int)(((uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x24U) & U32MASK)) - 1U) & U32MASK));
        _pc = 6;
        continue;
    case 9:
        /* loc_41B930 */
        if (((int)d3u_ld32(c, (uint32_t)((t2 + 0x20U) & U32MASK))) >= 32) { _pc = 6; continue; }
         d3u_st32(c, (uint32_t)((t2 + 0x24U) & U32MASK), (unsigned int)(((uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x24U) & U32MASK)) + 1U) & U32MASK));
        _pc = 6;
        continue;
    default:
        _pc = 6;
        continue;
    }
    _pc = 6;
    continue;
        case 6:
    /* l_41b939: */
     t0 = (uint32_t)D3U_TBL_4441C0_32(&c->st, 0);
     t0 &= 0x7FU;
     t3 = v20;
     D3U_TBL_43DACC_32(&c->st, t0 * 4U) = (unsigned int)t3;
     t0 = (uint32_t)D3U_TBL_43D8B0_32(&c->st, 0);
     t3 = (uint32_t)D3U_TBL_4441C0_32(&c->st, 0);
    if ((((uint32_t)d3u_t42A054[(unsigned int)(t0) & 0xFFU]) & t3) != 0) { _pc = 10; continue; }
     t5 = 0xFFFFU;
     t0 = 2U;
     t3 = 8U;
     D3U_TBL_43D8B4_32(&c->st, 0) = (unsigned int)t0;
     D3U_TBL_43D8B8_32(&c->st, 0) = (unsigned int)t0;
     _pc = 7;
     continue;
        case 7:
    /* l_41b97d: */
     t1 = (uint32_t)D3U_TBL_4441C0_32(&c->st, 0);
     t4 = (t1 - 1U) & U32MASK;
     t6 = t4;
     t6 = (t6 - t0) & U32MASK;
     t4 &= 0x7FU;
     t6 &= 0x7FU;
     t4 = (uint32_t)(uint32_t)D3U_TBL_43DACC_32(&c->st, t4 * 4U);
     t6 = (t6 << 2) & U32MASK;
     v1c = t4;
     t4 = (uint32_t)(uint32_t)D3U_TBL_43DACC_32(&c->st, t6);
     t6 = v1c;
     t6 = (t6 - t4) & U32MASK;
     t4 = t6;
     t4 &= 0xFFFU;
     t4 = (t4 << 2) & U32MASK;
     v28 = t4;
     t4 = t1;
     t4 = (t4 - t0) & U32MASK;
     t4 &= 0x7FU;
     t4 = (t4 << 2) & U32MASK;
     v24 = t4;
     t4 = t1;
     t4 &= 0x7FU;
     t4 = (uint32_t)(uint32_t)D3U_TBL_43DACC_32(&c->st, t4 * 4U);
     v1c = t4;
     t4 = v24;
     t6 = v1c;
     t4 = (uint32_t)(uint32_t)D3U_TBL_43DACC_32(&c->st, t4);
     t6 = (t6 - t4) & U32MASK;
     t4 = t6;
     t4 &= 0xFFFU;
     t4 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t4 * 4U);
     v1c = t4;
     t4 = v28;
     t6 = v1c;
     t4 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t4);
     t1 = (t1 - 2U) & U32MASK;
     t6 = (t6 + t4) & U32MASK;
     t4 = t1;
     t4 = (t4 - t0) & U32MASK;
     t1 &= 0x7FU;
     t4 &= 0x7FU;
     v1c = t6;
     t1 = (uint32_t)(uint32_t)D3U_TBL_43DACC_32(&c->st, t1 * 4U);
     t1 = (t1 - (uint32_t)(uint32_t)D3U_TBL_43DACC_32(&c->st, t4 * 4U)) & U32MASK;
     t1 &= 0xFFFU;
     t4 = v1c;
     t6 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t1 * 4U);
     t1 = (t4 + t6) & U32MASK;
     t4 = (((uint32_t)(uint32_t)D3U_TBL_43D8CC_32(&c->st, t3) * 0xDU)) & U32MASK;
     t4 = (t4 >> 4) & U32MASK;
     t4 = (t4 + t1) & U32MASK;
     D3U_TBL_43D8CC_32(&c->st, t3) = (unsigned int)t4;
     t6 = (uint32_t)(uint32_t)D3U_TBL_43D8CC_32(&c->st, t3);
     t4 = (uint32_t)D3U_TBL_43D8B4_32(&c->st, 0);
    if (t6 >= (uint32_t)(uint32_t)D3U_TBL_43D8CC_32(&c->st, t4 * 4U)) { _pc = 8; continue; }
     D3U_TBL_43D8B4_32(&c->st, 0) = (unsigned int)t0;
     _pc = 8;
     continue;
        case 8:
    /* l_41ba69: */
    if (t1 >= t5) { _pc = 9; continue; }
     t5 = t1;
     D3U_TBL_43D8B8_32(&c->st, 0) = (unsigned int)t0;
     _pc = 9;
     continue;
        case 9:
    /* l_41ba74: */
     t0 = (t0 + 1U) & U32MASK;
     t3 = (t3 + 4U) & U32MASK;
    if (t0 < 0x7CU) { _pc = 7; continue; }
    _pc = 10;
    continue;
        case 10:
    /* l_41ba81: */
     t0 = v2c;
     t0 = (t0 - (uint32_t)D3U_TBL_43DCCC_32(&c->st, 0)) & U32MASK;
     t1 = (uint32_t)D3U_TBL_43DCD0_32(&c->st, 0);
     v44 = t0;
     t0 = v2c;
     t0 = (t0 - t1) & U32MASK;
     t4 = (uint32_t)D3U_TBL_43DCD4_32(&c->st, 0);
     v40 = t0;
     t0 = v2c;
     t0 = (t0 - t4) & U32MASK;
     t5 = (uint32_t)D3U_TBL_43DCD8_32(&c->st, 0);
     v3c = t0;
     t0 = v2c;
     t0 = (t0 - t5) & U32MASK;
     t6 = (uint32_t)D3U_TBL_43DCDC_32(&c->st, 0);
     v38 = t0;
     t0 = v20;
     t3 = (uint32_t)D3U_TBL_43DCE0_32(&c->st, 0);
     t0 = (t0 - t6) & U32MASK;
     t1 = 0U;
     v34 = t0;
     t0 = v20;
     D3U_TBL_43D8B0_32(&c->st, 0) = (unsigned int)t1;
     t0 = (t0 - t3) & U32MASK;
     t4 = 0U;
     v30 = t0;
     t3 = 0U;
     t0 = D3U_TBL_43DCE4;
     _pc = 11;
     continue;
        case 11:
    /* l_41baef: */
     t1 = (uint32_t)D3U_TBL_4441C0_32(&c->st, 0);
     t1 &= 0xFFU;
     t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t0 + 0x400U) & U32MASK));
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t0 + t1 * 4U) & U32MASK));
     t6 = (t6 - t5) & U32MASK;
     d3u_st32(c, (uint32_t)((t0 + 0x400U) & U32MASK), (unsigned int)t6);
     t5 = (t4 == 0U) ? v44 : (t4 == 4U) ? v40 : (t4 == 8U) ? v3c : (t4 == 12U) ? v38 : (t4 == 16U) ? v34 : v30;
     t5 &= 0x1FFU;
     t5 = (uint32_t)d3u_t42A06C[(unsigned int)(t5 & 0x1FFU)];
     d3u_st32(c, (uint32_t)((t0 + t1 * 4U) & U32MASK), (unsigned int)t5);
     t1 = t5;
     d3u_st32(c, (uint32_t)((t0 + 0x400U) & U32MASK), (unsigned int)(((uint32_t)d3u_ld32(c, (uint32_t)((t0 + 0x400U) & U32MASK)) + t1) & U32MASK));
     t5 = (((uint32_t)D3U_TBL_43D8B0_32(&c->st, 0) * 0x404U)) & U32MASK;
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t0 + 0x400U) & U32MASK));
    if (t1 >= (uint32_t)(uint32_t)D3U_TBL_43E0E4_32(&c->st, t5)) { _pc = 12; continue; }
     D3U_TBL_43D8B0_32(&c->st, 0) = (unsigned int)t3;
     _pc = 12;
     continue;
        case 12:
    /* l_41bb45: */
     t4 = (t4 + 4U) & U32MASK;
     t3 = (t3 + 1U) & U32MASK;
     t0 = (t0 + 0x404U) & U32MASK;
    if (((int)t3) < 6) { _pc = 11; continue; }
     t0 = (uint32_t)D3U_TBL_43D8BC_32(&c->st, 0);
     D3U_TBL_43D8C0_32(&c->st, 0) = (unsigned int)t0;
     t0 = v2c;
     t3 = 0U;
     D3U_TBL_43D8BC_32(&c->st, 0) = (unsigned int)t0;
     t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x70U) & U32MASK));
    t3 = (t3 & 0xFFFFFF00U) | (v18 & 0xFFU);
     t0 = (t0 << 8) & U32MASK;
     t0 = (t0 + t3) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x70U) & U32MASK), (unsigned int)t0);
     t0 = 0U;
    t0 = (t0 & 0xFFFFFF00U) | (t3 & 0xFFU);
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x74U) & U32MASK));
     t1 = (uint32_t)d3u_t42A86C[(unsigned int)(t0 & 0xFFU)];
     t3 = (t3 << 8) & U32MASK;
     t3 = (t3 + t1) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x74U) & U32MASK), (unsigned int)t3);
     t3 = (uint32_t)d3u_ld32(c, (uint32_t)((t2 + 0x78U) & U32MASK));
     t0 = (uint32_t)d3u_t42B06C[(unsigned int)(t0 & 0xFFU)];
     t3 = (t3 << 8) & U32MASK;
     t3 = (t3 + t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t2 + 0x78U) & U32MASK), (unsigned int)t3);
    return;
        case 13:
    /* l_41bba8: */
     t3 = (uint32_t)D3U_TBL_4441D0_32(&c->st, 0);
     t0 = t3;
     t0 = (t0 << 5) & U32MASK;
     t0 = (t0 - t3) & U32MASK;
     t3 = D3U_TBL_43F4FC;
     t0 = (t0 << 2) & U32MASK;
     t2 = (uint32_t)D3U_TBL_4441D0_32(&c->st, 0);
     t3 = (t3 + t0) & U32MASK;
    t0 = (t0 & 0xFFFFFF00U) | (uint32_t)D3U_FLAG_444EA8_8(&c->st, 0);
    t1 = (t1 & 0xFFFF00FFU) | ((((uint32_t)D3U_FLAG_444EA9_8(&c->st, 0)) & 0xFFU) << 8);
     M8((D3U_TBL_43F6EC + t2) & U32MASK) = (unsigned char)(t0 & 0xFFU);
    if (((t1 >> 8) & 0xFFU) < 8U) { _pc = 14; continue; }
     t0 = (uint32_t)(((int)(signed char)(t1 & 0xFFU))) & U32MASK;
    _pc = 15;
    continue;
        case 14:
    /* l_41bbe0: */
     t0 = 0U;
    t0 = (t0 & 0xFFFFFF00U) | (t1 & 0xFFU);
     t0 = (t0 - 0x80U) & U32MASK;
     _pc = 15;
     continue;
        case 15:
    /* l_41bbe9: */
     v2c = t0;
    t2 = (t2 & 0xFFFFFF00U) | (v2c & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | (uint32_t)D3U_TBL_43DCCC_8(&c->st, 0);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) - (t2 & 0xFFU)) & 0xFFU));
     t0 = (uint32_t)(((int)(signed char)(t0 & 0xFFU))) & U32MASK;
     t0 = (t0 << 4) & U32MASK;
     t1 = t0;
     t1 &= 0xFFFU;
     t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x30U) & U32MASK));
     t1 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t1 * 4U);
     t4 = (uint32_t)d3u_ld32(c, (uint32_t)(t3 & U32MASK));
     t2 = (t2 + t1) & U32MASK;
     t1 = t0;
     t1 = (t1 - t4) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x30U) & U32MASK), (unsigned int)t2);
     t1 &= 0xFFFU;
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x34U) & U32MASK));
     t5 = (t5 + (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t1 * 4U)) & U32MASK;
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)(t3 & U32MASK));
     t1 = (t1 + t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x34U) & U32MASK), (unsigned int)t5);
     t1 &= 0xFFFU;
     t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x38U) & U32MASK));
     t1 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t1 * 4U);
     t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 4U) & U32MASK));
     t6 = (t6 + t1) & U32MASK;
     t1 = t0;
     t1 = (t1 - t2) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x38U) & U32MASK), (unsigned int)t6);
     t1 &= 0xFFFU;
     t4 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x3CU) & U32MASK));
     t4 = (t4 + (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t1 * 4U)) & U32MASK;
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 4U) & U32MASK));
     t1 = (t1 + t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x3CU) & U32MASK), (unsigned int)t4);
     t1 &= 0xFFFU;
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x40U) & U32MASK));
     t1 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t1 * 4U);
     t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 8U) & U32MASK));
     t5 = (t5 + t1) & U32MASK;
     t1 = t0;
     t1 = (t1 - t6) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x40U) & U32MASK), (unsigned int)t5);
     t1 &= 0xFFFU;
     t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x44U) & U32MASK));
     t2 = (t2 + (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t1 * 4U)) & U32MASK;
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 8U) & U32MASK));
     t1 = (t1 + t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x44U) & U32MASK), (unsigned int)t2);
     t1 &= 0xFFFU;
     t4 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x48U) & U32MASK));
     t1 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t1 * 4U);
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0xCU) & U32MASK));
     t4 = (t4 + t1) & U32MASK;
     t1 = t0;
     t1 = (t1 - t5) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x48U) & U32MASK), (unsigned int)t4);
     t1 &= 0xFFFU;
     t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x4CU) & U32MASK));
     t6 = (t6 + (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t1 * 4U)) & U32MASK;
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0xCU) & U32MASK));
     t1 = (t1 + t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x4CU) & U32MASK), (unsigned int)t6);
     t1 &= 0xFFFU;
     t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x50U) & U32MASK));
     t1 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t1 * 4U);
     t2 = (t2 + t1) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x50U) & U32MASK), (unsigned int)t2);
     t1 = t0;
     t4 = (uint32_t)D3U_TBL_43D8C4_32(&c->st, 0);
     t1 = (t1 - t4) & U32MASK;
     t1 &= 0xFFFU;
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x54U) & U32MASK));
     t5 = (t5 + (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t1 * 4U)) & U32MASK;
     t0 = (t0 + t4) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x54U) & U32MASK), (unsigned int)t5);
     t0 &= 0xFFFU;
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x58U) & U32MASK));
     t0 = (uint32_t)(uint32_t)D3U_LIN_EXTEND_TBL_32(&c->st, t0 * 4U);
     t1 = (t1 + t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x58U) & U32MASK), (unsigned int)t1);
    t2 = (t2 & 0xFFFF00FFU) | ((v2c & 0xFFU) << 8);
    t0 = (t0 & 0xFFFFFF00U) | (uint32_t)D3U_TBL_43DCCC_8(&c->st, 0);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) - ((t2 >> 8) & 0xFFU)) & 0xFFU));
     t0 &= 0xFFU;
     t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x5CU) & U32MASK));
     t2 = (t2 + (uint32_t)d3u_t42B86C[(unsigned int)(t0 & 0xFFU)]) & U32MASK;
     t0 = (t0 & 0xFFFFFF00U) | (uint32_t)M8((t3 + 0x28U) & U32MASK);
    t0 = (t0 & 0xFFFF00FFU) | ((v2c & 0xFFU) << 8);
     d3u_st32(c, (uint32_t)((t3 + 0x5CU) & U32MASK), (unsigned int)t2);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) - ((t0 >> 8) & 0xFFU)) & 0xFFU));
     t0 &= 0xFFU;
     t4 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x60U) & U32MASK));
     t0 = (uint32_t)d3u_t42B86C[(unsigned int)(t0 & 0xFFU)];
     t1 = (t1 & 0xFFFFFF00U) | (uint32_t)M8((t3 + 0x28U) & U32MASK);
     t4 = (t4 + t0) & U32MASK;
    t0 = (t0 & 0xFFFFFF00U) | (v2c & 0xFFU);
    t0 = (t0 & 0xFFFFFF00U) | ((((t0 & 0xFFU) - (t1 & 0xFFU)) & 0xFFU));
     d3u_st32(c, (uint32_t)((t3 + 0x60U) & U32MASK), (unsigned int)t4);
     t0 = (uint32_t)(((int)(signed char)(t0 & 0xFFU))) & U32MASK;
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x6CU) & U32MASK));
     d3u_st32(c, (uint32_t)((t3 + 0x2CU) & U32MASK), (unsigned int)t0);
     t5 = (t5 + 1U) & U32MASK;
     D3U_TBL_43D8C4_32(&c->st, 0) = (unsigned int)t0;
     d3u_st32(c, (uint32_t)((t3 + 0x6CU) & U32MASK), (unsigned int)t5);
     t0 = v2c;
     t1 = (t1 & 0xFFFF00FFU) | ((((uint32_t)M8((t3 + 0x6CU) & U32MASK)) & 0xFFU) << 8);
     d3u_st32(c, (uint32_t)((t3 + 0x28U) & U32MASK), (unsigned int)t0);
    if ((((t1 >> 8) & 0xFFU) & 0x0FU) != 0) { _pc = 19; continue; }
     t1 = 1U;
     t0 = (t3 + 4U) & U32MASK;
     t4 = 0U;
     t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x30U) & U32MASK));
     d3u_st32(c, (uint32_t)((t3 + 0x30U) & U32MASK), (unsigned int)t4);
     _pc = 16;
     continue;
        case 16:
    /* l_41bd8e: */
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t0 + 0x30U) & U32MASK));
    if (t2 <= t5) { _pc = 17; continue; }
     t2 = t5;
     t4 = t1;
     _pc = 17;
     continue;
        case 17:
    /* l_41bd99: */
     t0 = (t0 + 4U) & U32MASK;
     t1 = (t1 + 1U) & U32MASK;
     d3u_st32(c, (uint32_t)((t0 + 0x2CU) & U32MASK), 0);
    if (((int)t1) < 11) { _pc = 16; continue; }
     t4 = (t4 - 1U) & U32MASK;
    if (t4 > 9U) { _pc = 18; continue; }
    switch (t4) {
    case 0:
        /* loc_41BDBB */
         t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x14U) & U32MASK));
        if (((int)t2) <= -32) { _pc = 18; continue; }
         t4 = (t2 - 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t3 + 0x14U) & U32MASK), (unsigned int)t4);
        _pc = 18;
        continue;
    case 1:
        /* loc_41BDD2 */
         t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x14U) & U32MASK));
        if (((int)t0) >= 32) { _pc = 18; continue; }
         t1 = (t0 + 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t3 + 0x14U) & U32MASK), (unsigned int)t1);
        _pc = 18;
        continue;
    case 2:
        /* loc_41BDE9 */
         t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x18U) & U32MASK));
        if (((int)t5) <= -32) { _pc = 18; continue; }
         t6 = (t5 - 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t3 + 0x18U) & U32MASK), (unsigned int)t6);
        _pc = 18;
        continue;
    case 3:
        /* loc_41BE00 */
         t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x18U) & U32MASK));
        if (((int)t2) >= 32) { _pc = 18; continue; }
         t4 = (t2 + 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t3 + 0x18U) & U32MASK), (unsigned int)t4);
        _pc = 18;
        continue;
    case 4:
        /* loc_41BE14 */
         t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x1CU) & U32MASK));
        if (((int)t0) <= -32) { _pc = 18; continue; }
         t1 = (t0 - 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t3 + 0x1CU) & U32MASK), (unsigned int)t1);
        _pc = 18;
        continue;
    case 5:
        /* loc_41BE24 */
         t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x1CU) & U32MASK));
        if (((int)t5) >= 32) { _pc = 18; continue; }
         t6 = (t5 + 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t3 + 0x1CU) & U32MASK), (unsigned int)t6);
        _pc = 18;
        continue;
    case 6:
        /* loc_41BE34 */
         t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x20U) & U32MASK));
        if (((int)t2) <= -32) { _pc = 18; continue; }
         t4 = (t2 - 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t3 + 0x20U) & U32MASK), (unsigned int)t4);
        _pc = 18;
        continue;
    case 7:
        /* loc_41BE44 */
         t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x20U) & U32MASK));
        if (((int)t0) >= 32) { _pc = 18; continue; }
         t1 = (t0 + 1U) & U32MASK;
         d3u_st32(c, (uint32_t)((t3 + 0x20U) & U32MASK), (unsigned int)t1);
        _pc = 18;
        continue;
    case 8:
        /* loc_41BE54 */
        if (((int)d3u_ld32(c, (uint32_t)((t3 + 0x20U) & U32MASK))) <= -32) { _pc = 18; continue; }
         d3u_st32(c, (uint32_t)((t3 + 0x24U) & U32MASK), (unsigned int)(((uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x24U) & U32MASK)) - 1U) & U32MASK));
        _pc = 18;
        continue;
    case 9:
        /* loc_41BE5F */
        if (((int)d3u_ld32(c, (uint32_t)((t3 + 0x20U) & U32MASK))) >= 32) { _pc = 18; continue; }
         d3u_st32(c, (uint32_t)((t3 + 0x24U) & U32MASK), (unsigned int)(((uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x24U) & U32MASK)) + 1U) & U32MASK));
        _pc = 18;
        continue;
    default:
        _pc = 18;
        continue;
    }
    _pc = 18;
    continue;
        case 18:
    /* l_41be68: */
    if (M8((t3 + 0x6CU) & U32MASK) != 0) { _pc = 19; continue; }
     t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x64U) & U32MASK));
     t5 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x5CU) & U32MASK));
     t5 = (t5 - t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x5CU) & U32MASK), (unsigned int)t5);
     t6 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x60U) & U32MASK));
     t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x68U) & U32MASK));
     d3u_st32(c, (uint32_t)((t3 + 0x64U) & U32MASK), (unsigned int)t5);
     t6 = (t6 - t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x60U) & U32MASK), (unsigned int)t6);
     d3u_st32(c, (uint32_t)((t3 + 0x68U) & U32MASK), (unsigned int)t6);
     _pc = 19;
     continue;
        case 19:
    /* l_41be8a: */
    if (D3U_FLAG_444EA9_8(&c->st, 0) != 9) { _pc = 20; continue; }
     t0 = (uint32_t)D3U_TBL_43D8C8_32(&c->st, 0);
     t1 = (uint32_t)D3U_TBL_43D8C4_32(&c->st, 0);
     D3U_TBL_43D8C4_32(&c->st, 0) = (unsigned int)t0;
     D3U_TBL_43D8C8_32(&c->st, 0) = (unsigned int)t1;
     _pc = 20;
     continue;
        case 20:
    /* l_41bea9: */
     t1 = 0U;
     t0 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x70U) & U32MASK));
    t1 = (t1 & 0xFFFFFF00U) | (v18 & 0xFFU);
     t0 = (t0 << 8) & U32MASK;
     t0 = (t0 + t1) & U32MASK;
    t2 = (t2 & 0xFFFFFF00U) | (uint32_t)D3U_FLAG_444EA9_8(&c->st, 0);
     d3u_st32(c, (uint32_t)((t3 + 0x70U) & U32MASK), (unsigned int)t0);
    if ((t2 & 0xFFU) != 7U) { _pc = 21; continue; }
     t0 = 0U;
     t2 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x74U) & U32MASK));
    t0 = (t0 & 0xFFFFFF00U) | (t1 & 0xFFU);
     t2 = (t2 << 8) & U32MASK;
     t1 = (uint32_t)d3u_t42AC6C[(unsigned int)(t0 & 0xFFU)];
     t2 = (t2 + t1) & U32MASK;
     t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x78U) & U32MASK));
     d3u_st32(c, (uint32_t)((t3 + 0x74U) & U32MASK), (unsigned int)t2);
     t1 = (t1 << 8) & U32MASK;
     t0 = (uint32_t)d3u_t42B46C[(unsigned int)(t0 & 0xFFU)];
     t1 = (t1 + t0) & U32MASK;
     d3u_st32(c, (uint32_t)((t3 + 0x78U) & U32MASK), (unsigned int)t1);
     _pc = 21;
     continue;
        case 21:
    /* l_41beed: */
    ;
        default:
            break;
        }
        break;
    }
}

/* ---- price walker (from draft_n3.c) ---- */
unsigned int d3uc_walk_price_slots(dec3w_ctx *c, unsigned int eax_in, unsigned int edx_in)
{
uint32_t t0 = 0;
uint32_t t1 = 0;
uint32_t t2 = 0;
uint32_t t3 = 0;
uint32_t t4 = 0;
uint32_t t5 = 0;
uint32_t t6 = 0;
uint32_t v34 = 0;
uint32_t v58 = 0;
uint32_t v54 = 0;
uint32_t v50 = 0;
uint32_t v4C = 0;
uint32_t v48 = 0;
uint32_t v44 = 0;
uint32_t v40 = 0;
uint32_t v38 = 0;
uint32_t v3C = 0;
uint32_t v30 = 0;
uint32_t v2C = 0;
uint32_t v28 = 0;
uint32_t v24 = 0;
uint32_t v20 = 0;
uint32_t v1C = 0;
uint32_t v18 = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
t0 = (uint32_t)eax_in; /* entry live t0 */
t3 = (uint32_t)edx_in; /* entry live t3 */
v34 = t0 & U32MASK;
v58 = t3 & U32MASK;
t0 = (t0 & 0xFFFFFF00U) | (uint32_t)D3U_FLAG_444E9A_8(&c->st, 0);
v20 = t0 & 0xFFU;
if (t3 <= 0x800U) { _pc = 21; continue; }
t1 = 0x14U;
t0 = D3U_TBL_444188;
t3 = 0U;
d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3);
t1 = 0x14U;
t0 = D3U_TBL_44419C;
t3 = 0U;
d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3);
t1 = 0x1400U;
t0 = D3U_TBL_43F6F0;
t3 = 0U;
d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3);
t1 = 0x1400U;
t0 = D3U_TBL_440B08;
t3 = 0U;
t5 = 0U;
d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3);
t1 = 0x400U;
v48 = t5 & U32MASK;
v38 = t1 & U32MASK;
t6 = t1 & U32MASK;
            _pc = 1;
            continue;
        case 1:
    /* L_41AA3D: */
t0 = t5 & U32MASK;
t1 = t5 & U32MASK;
t2 = t5 & U32MASK;
t3 = 0U;
t1 = (t1 << 10) & U32MASK;
t2 = (0U - t2) & U32MASK;
v2C = t1 & U32MASK;
t1 = v34 & U32MASK;
t0 = (t0 << 11) & U32MASK;
t4 = (t1 + t2) & U32MASK;
            _pc = 2;
            continue;
        case 2:
    /* L_41AA58: */
if (t5 == 0U) { _pc = 3; continue; }
if (t3 < t5) { _pc = 3; continue; }
t2 = (t2 & 0xFFFFFF00U) | (uint32_t)G8(t1);
t2 = (t2 & 0xFFFF00FFU) | ((uint32_t)G8(t4) << 8);
t2 = (t2 & 0xFFFFFF00U) | (((t2 & 0xFFU) - ((t2 >> 8) & 0xFFU)) & 0xFFU);
_pc = 4;
continue;
        case 3:
    /* L_41AA68: */
t2 = (t2 & 0xFFFFFF00U) | (uint32_t)G8(t1);
            _pc = 4;
            continue;
        case 4:
    /* L_41AA6A: */
M8(D3U_TBL_4370B0 + t0) = (unsigned char)(t2 & 0xFFU);
t2 = 0U;
t2 = (t2 & 0xFFFFFF00U) | (uint32_t)G8(D3U_TBL_4370B0 + t0);
t1 = (t1 + 1U) & U32MASK;
t2 = (t2 << 2) & U32MASK;
t4 = (t4 + 1U) & U32MASK;
t2 = (t2 + v2C) & U32MASK;
t0 = (t0 + 1U) & U32MASK;
t3 = (t3 + 1U) & U32MASK;
D3U_TBL_43F6F0_32(&c->st, t2) = (D3U_TBL_43F6F0_32(&c->st, t2) + 1U) & U32MASK;
if (t3 < 0x400U) { _pc = 2; continue; }
t0 = v48 & U32MASK;
t3 = t5 & U32MASK;
t2 = v38 & U32MASK;
t3 = (t3 << 10) & U32MASK;
            _pc = 5;
            continue;
        case 5:
    /* L_41AA9E: */
t1 = D3U_TBL_43F6F0_32(&c->st, t3) & U32MASK;
t1 = (t1 * t1) & U32MASK;
t4 = D3U_TBL_444188_32(&c->st, t0) & U32MASK;
t4 = (t4 + t1) & U32MASK;
t3 = (t3 + 4U) & U32MASK;
D3U_TBL_444188_32(&c->st, t0) = t4 & U32MASK;
if (t3 != t2) { _pc = 5; continue; }
t4 = v38 & U32MASK;
t0 = v48 & U32MASK;
t5 = (t5 + 1U) & U32MASK;
t4 = (t4 + t6) & U32MASK;
t0 = (t0 + 4U) & U32MASK;
v38 = t4 & U32MASK;
v48 = t0 & U32MASK;
if (t5 < 5U) { _pc = 1; continue; }
t3 = 0x400U;
t0 = v34 & U32MASK;
t4 = 0U;
v44 = t3 & U32MASK;
t0 = (t0 + t3) & U32MASK;
v3C = t4 & U32MASK;
v50 = t0 & U32MASK;
t6 = 4U;
            _pc = 6;
            continue;
        case 6:
    /* L_41AAF9: */
t0 = t4 & U32MASK;
t0 = (t0 << 10) & U32MASK;
t3 = v50 & U32MASK;
v28 = t0 & U32MASK;
t0 = t4 & U32MASK;
t5 = t4 & U32MASK;
t0 = (0U - t0) & U32MASK;
t5 = (t5 << 11) & U32MASK;
t2 = (t3 + t0) & U32MASK;
t0 = (t5 + 0x400U) & U32MASK;
t5 = (t5 + 0x800U) & U32MASK;
            _pc = 7;
            continue;
        case 7:
    /* L_41AB1E: */
if (t4 == 0U) { _pc = 8; continue; }
t1 = (t1 & 0xFFFFFF00U) | (uint32_t)G8(t3);
t1 = (t1 & 0xFFFF00FFU) | ((uint32_t)G8(t2) << 8);
t1 = (t1 & 0xFFFFFF00U) | (((t1 & 0xFFU) - ((t1 >> 8) & 0xFFU)) & 0xFFU);
_pc = 9;
continue;
        case 8:
    /* L_41AB2A: */
t1 = (t1 & 0xFFFFFF00U) | (uint32_t)G8(t3);
            _pc = 9;
            continue;
        case 9:
    /* L_41AB2C: */
M8(D3U_TBL_4370B0 + t0) = (unsigned char)(t1 & 0xFFU);
t1 = 0U;
t1 = (t1 & 0xFFFFFF00U) | (uint32_t)G8(D3U_TBL_4370B0 + t0);
t1 = (t1 << 2) & U32MASK;
t3 = (t3 + 1U) & U32MASK;
t1 = (t1 + v28) & U32MASK;
t2 = (t2 + 1U) & U32MASK;
t0 = (t0 + 1U) & U32MASK;
D3U_TBL_440B08_32(&c->st, t1) = (D3U_TBL_440B08_32(&c->st, t1) + 1U) & U32MASK;
if (t0 != t5) { _pc = 7; continue; }
t3 = v3C & U32MASK;
t0 = t4 & U32MASK;
t2 = v44 & U32MASK;
t0 = (t0 << 10) & U32MASK;
            _pc = 10;
            continue;
        case 10:
    /* L_41AB5B: */
t1 = D3U_TBL_440B08_32(&c->st, t0) & U32MASK;
t1 = (t1 * t1) & U32MASK;
t5 = D3U_TBL_44419C_32(&c->st, t3) & U32MASK;
t5 = (t5 + t1) & U32MASK;
t0 = (t0 + 4U) & U32MASK;
D3U_TBL_44419C_32(&c->st, t3) = t5 & U32MASK;
if (t0 != t2) { _pc = 10; continue; }
t5 = v44 & U32MASK;
t0 = v3C & U32MASK;
t4 = (t4 + 1U) & U32MASK;
t5 = (t5 + 0x400U) & U32MASK;
t0 = (t0 + t6) & U32MASK;
v44 = t5 & U32MASK;
v3C = t0 & U32MASK;
if (t4 < 5U) { _pc = 6; continue; }
t3 = 0x800U;
t1 = v58 & U32MASK;
v30 = t3 & U32MASK;
if (t1 <= t3) { _pc = 21; continue; }
t0 = v34 & U32MASK;
t2 = 0x0C00U;
t0 = (t0 + t3) & U32MASK;
v40 = t2 & U32MASK;
v4C = t0 & U32MASK;
            _pc = 11;
            continue;
        case 11:
    /* L_41ABC2: */
t4 = v30 & U32MASK;
t0 = v40 & U32MASK;
t1 = v4C & U32MASK;
t5 = v34 & U32MASK;
t6 = v30 & U32MASK;
t2 = (t2 & 0xFFFFFF00U);
t3 = 0U;
v18 = t2 & 0xFFU;
v1C = t2 & 0xFFU;
t4 = t4 & 0x7FFU;
t0 = t0 & 0x7FFU;
v54 = t1 & U32MASK;
t5 = (t5 + t6) & U32MASK;
v24 = t0 & U32MASK;
t2 = 0U;
t0 = 0U;
            _pc = 12;
            continue;
        case 12:
    /* L_41ABFB: */
t1 = (t1 & 0xFFFFFF00U) | (uint32_t)G8(D3U_TBL_4370B0 + t4);
t1 = t1 & 0xFFU;
t6 = D3U_TBL_43F6F0_32(&c->st, ((t2 + t1 * 4U) & U32MASK)) & U32MASK;
D3U_TBL_444188_32(&c->st, t0) = (D3U_TBL_444188_32(&c->st, t0) - t6) & U32MASK;
t6 = (t6 - 1U) & U32MASK;
D3U_TBL_43F6F0_32(&c->st, ((t2 + t1 * 4U) & U32MASK)) = t6 & U32MASK;
t1 = t6 & U32MASK;
t6 = D3U_TBL_444188_32(&c->st, t0) & U32MASK;
t6 = (t6 - t1) & U32MASK;
t1 = v24 & U32MASK;
D3U_TBL_444188_32(&c->st, t0) = t6 & U32MASK;
t1 = (t1 & 0xFFFFFF00U) | (uint32_t)G8(D3U_TBL_4370B0 + (t1 & U32MASK));
t1 = t1 & 0xFFU;
t6 = D3U_TBL_43F6F0_32(&c->st, ((t2 + t1 * 4U) & U32MASK)) & U32MASK;
D3U_TBL_444188_32(&c->st, t0) = (D3U_TBL_444188_32(&c->st, t0) + t6) & U32MASK;
t6 = (t6 + 1U) & U32MASK;
D3U_TBL_43F6F0_32(&c->st, ((t2 + t1 * 4U) & U32MASK)) = t6 & U32MASK;
D3U_TBL_444188_32(&c->st, t0) = (D3U_TBL_444188_32(&c->st, t0) + t6) & U32MASK;
t6 = D3U_TBL_440B08_32(&c->st, ((t2 + t1 * 4U) & U32MASK)) & U32MASK;
D3U_TBL_44419C_32(&c->st, t0) = (D3U_TBL_44419C_32(&c->st, t0) - t6) & U32MASK;
t6 = (t6 - 1U) & U32MASK;
D3U_TBL_440B08_32(&c->st, ((t2 + t1 * 4U) & U32MASK)) = t6 & U32MASK;
t1 = t6 & U32MASK;
D3U_TBL_44419C_32(&c->st, t0) = (D3U_TBL_44419C_32(&c->st, t0) - t1) & U32MASK;
if (t3 == 0U) { _pc = 13; continue; }
t1 = v54 & U32MASK;
t1 = (t1 & 0xFFFFFF00U) | (uint32_t)G8(t1 & U32MASK);
t1 = (t1 & 0xFFFFFF00U) | (((t1 & 0xFFU) - (uint32_t)G8(t5)) & 0xFFU);
_pc = 14;
continue;
        case 13:
    /* L_41AC82: */
t1 = v54 & U32MASK;
t1 = (t1 & 0xFFFFFF00U) | (uint32_t)G8(t1 & U32MASK);
            _pc = 14;
            continue;
        case 14:
    /* L_41AC88: */
M8(D3U_TBL_4370B0 + t4) = (unsigned char)(t1 & 0xFFU);
t1 = t1 & 0xFFU;
t5 = (t5 - 1U) & U32MASK;
t6 = D3U_TBL_440B08_32(&c->st, ((t2 + t1 * 4U) & U32MASK)) & U32MASK;
t2 = (t2 + 0x400U) & U32MASK;
D3U_TBL_44419C_32(&c->st, t0) = (D3U_TBL_44419C_32(&c->st, t0) + t6) & U32MASK;
t6 = (t6 + 1U) & U32MASK;
t0 = (t0 + 4U) & U32MASK;
D3U_TBL_440708_32(&c->st, ((t2 + t1 * 4U) & U32MASK)) = t6 & U32MASK;
t1 = t6 & U32MASK;
t6 = D3U_TBL_444198_32(&c->st, t0) & U32MASK;
t4 = (t4 + 0x800U) & U32MASK;
t6 = (t6 + t1) & U32MASK;
t1 = v24 & U32MASK;
t3 = (t3 + 1U) & U32MASK;
t1 = (t1 + 0x800U) & U32MASK;
D3U_TBL_444198_32(&c->st, t0) = t6 & U32MASK;
v24 = t1 & U32MASK;
if (t3 < 5U) { _pc = 12; continue; }
t3 = 1U;
t0 = 4U;
            _pc = 15;
            continue;
        case 15:
    /* L_41ACEB: */
t1 = 0U;
t1 = (t1 & 0xFFFFFF00U) | (v18 & 0xFFU);
t2 = (t1 * 4U) & U32MASK;
t1 = D3U_TBL_444188_32(&c->st, t0) & U32MASK;
if (t1 <= D3U_TBL_444188_32(&c->st, t2)) { _pc = 16; continue; }
v18 = t3 & 0xFFU;
            _pc = 16;
            continue;
        case 16:
    /* L_41AD0A: */
t1 = 0U;
t1 = (t1 & 0xFFFFFF00U) | (v1C & 0xFFU);
t2 = D3U_TBL_44419C_32(&c->st, t0) & U32MASK;
if (t2 <= D3U_TBL_44419C_32(&c->st, ((t1 * 4U) & U32MASK))) { _pc = 17; continue; }
v1C = t3 & 0xFFU;
            _pc = 17;
            continue;
        case 17:
    /* L_41AD23: */
t3 = (t3 + 1U) & U32MASK;
t0 = (t0 + 4U) & U32MASK;
if (t3 < 5U) { _pc = 15; continue; }
t0 = (t0 & 0xFFFFFF00U) | (v18 & 0xFFU);
t2 = (t2 & 0xFFFFFF00U) | (v20 & 0xFFU);
if ((t0 & 0xFFU) == (t2 & 0xFFU)) { _pc = 18; continue; }
t0 = 0U;
t0 = (t0 & 0xFFFFFF00U) | (v18 & 0xFFU);
t3 = D3U_TBL_444188_32(&c->st, ((t0 * 4U) & U32MASK)) & U32MASK;
t0 = 0U;
t0 = (t0 & 0xFFFFFF00U) | (t2 & 0xFFU);
t0 = D3U_TBL_444188_32(&c->st, ((t0 * 4U) & U32MASK)) & U32MASK;
t3 = (t3 + t3 * 2U) & U32MASK;
t0 = (t0 << 2) & U32MASK;
if (t3 >= t0) { _pc = 18; continue; }
v18 = t2 & 0xFFU;
            _pc = 18;
            continue;
        case 18:
    /* L_41AD5E: */
t0 = (t0 & 0xFFFFFF00U) | (v1C & 0xFFU);
t2 = (t2 & 0xFFFF00FFU) | ((v20 & 0xFFU) << 8);
if ((t0 & 0xFFU) == ((t2 >> 8) & 0xFFU)) { _pc = 19; continue; }
t0 = 0U;
t3 = 0U;
t0 = (t0 & 0xFFFFFF00U) | (v1C & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t2 >> 8) & 0xFFU);
t0 = D3U_TBL_44419C_32(&c->st, ((t0 * 4U) & U32MASK)) & U32MASK;
t3 = D3U_TBL_44419C_32(&c->st, ((t3 * 4U) & U32MASK)) & U32MASK;
t0 = (t0 + t0 * 2U) & U32MASK;
t3 = (t3 << 2) & U32MASK;
if (t0 >= t3) { _pc = 19; continue; }
v1C = (t2 >> 8) & 0xFFU;
            _pc = 19;
            continue;
        case 19:
    /* L_41AD90: */
t0 = (t0 & 0xFFFFFF00U) | (v18 & 0xFFU);
if ((t0 & 0xFFU) == (v1C & 0xFFU)) { _pc = 20; continue; }
t1 = v30 & U32MASK;
D3U_FLAG_444E9A_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
t1 = (t1 - 0x200U) & U32MASK;
_pc = 22;
continue;
        case 20:
    /* L_41ADAB: */
t0 = (t0 & 0xFFFFFF00U) | (v18 & 0xFFU);
t6 = v4C & U32MASK;
t3 = v30 & U32MASK;
t1 = v58 & U32MASK;
v20 = t0 & 0xFFU;
t6 = (t6 + 1U) & U32MASK;
t3 = (t3 + 1U) & U32MASK;
t0 = v40 & U32MASK;
v4C = t6 & U32MASK;
t0 = (t0 + 1U) & U32MASK;
v30 = t3 & U32MASK;
v40 = t0 & U32MASK;
if (t3 < t1) { _pc = 11; continue; }
            _pc = 21;
            continue;
        case 21:
    /* L_41ADD9: */
t0 = (t0 & 0xFFFFFF00U) | (v20 & 0xFFU);
t1 = v58 & U32MASK;
D3U_FLAG_444E9A_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
            _pc = 22;
            continue;
        case 22:
    /* L_41ADE5: */
t0 = t1 & U32MASK;
/* retn */
return (unsigned int)(t0 & U32MASK);
        default:
            break;
        }
        break;
    }
    return 0;
}
