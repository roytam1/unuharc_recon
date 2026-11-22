#include "uha03_dec4_int.h"

unsigned int d3uc_decode_match(dec3w_ctx *c, unsigned int edi_in)
{
    uint32_t r_eax = 0;
    uint32_t r_ebx = 0;
    uint32_t r_ecx = 0;
    uint32_t r_edx = 0;
    uint32_t r_esi = 0;
    uint32_t r_edi = 0;
    uint32_t r_ebp = 0;
    uint32_t stk3c = 0;
    uint32_t stk38 = 0;
    uint32_t stk34 = 0;
    uint32_t stk30 = 0;
    uint32_t stk2c = 0;
    unsigned char stk28 = 0;
    unsigned char stk24 = 0;
    unsigned char stk20 = 0;
    unsigned char stk1c = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    r_edi = (uint32_t)edi_in;
     r_eax = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
     r_edx = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
     r_eax = (r_eax >> 13) & U32MASK;
     r_eax = (r_eax ^ r_edx) & U32MASK;
     r_eax &= 0x3FFFFU;
     r_edx = (uint32_t)(uint32_t)D3U_BLK03_MODEL_A_FF_32(&c->st, 0);
     stk34 = r_eax;
     r_eax = (r_eax << 3) & U32MASK;
     r_ebx = (uint32_t)D3U_TBL_44425C_32(&c->st, 0);
     r_edx = (r_edx + r_eax) & U32MASK;
     r_ecx = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
     stk38 = r_edx;
     r_edx = (uint32_t)D3U_TBL_44425C_32(&c->st, 0);
     r_eax = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
     r_edx = (r_edx >> 15) & U32MASK;
     r_eax = (r_eax >> 15) & U32MASK;
     r_edx = (r_edx ^ r_ebx) & U32MASK;
     r_eax = (r_eax ^ r_ecx) & U32MASK;
     r_edx = (r_edx << 4) & U32MASK;
     r_edx = (r_edx ^ r_eax) & U32MASK;
     r_eax = (uint32_t)D3U_TBL_444260_32(&c->st, 0);
     r_esi = (uint32_t)D3U_TBL_444260_32(&c->st, 0);
     r_eax = (r_eax >> 15) & U32MASK;
     r_eax = (r_eax ^ r_esi) & U32MASK;
     r_eax = (r_eax << 6) & U32MASK;
     r_eax = (r_eax ^ r_edx) & U32MASK;
     r_eax &= 0x3FFFFU;
    r_ecx = (r_ecx & ~0xFFU) | 2U;
     r_eax = (r_eax << 3) & U32MASK;
     r_edx = (uint32_t)(uint32_t)D3U_BLK04_MODEL_B_FF_32(&c->st, 0);
     r_eax = (r_eax + 4U) & U32MASK;
    stk1c = (unsigned char)(r_ecx & 0xFFU);
     r_edx = (r_edx + r_eax) & U32MASK;
    r_eax &= ~0xFF00U;
    r_ebx &= ~0xFF00U;
    stk28 = (unsigned char)((r_eax >> 8) & 0xFFU);
    stk24 = (unsigned char)((r_eax >> 8) & 0xFFU);
     r_eax = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
    stk20 = (unsigned char)((r_ebx >> 8) & 0xFFU);
     stk2c = r_eax;
    if (r_eax >= 0x40U) { _pc = 1; continue; }
     r_ebx = stk2c;
     r_eax = (uint32_t)D3U_DICT_SIZE_32(&c->st, 0);
     r_ebx = (r_ebx + r_eax) & U32MASK;
     stk2c = r_ebx;
            _pc = 1;
            continue;
        case 1:
    /* loc_416086: */
     r_eax = stk2c;
     r_eax = (r_eax - 1U) & U32MASK;
     stk3c = r_eax;
            _pc = 2;
            continue;
        case 2:
    /* loc_41608E: */
    r_ecx = (r_ecx & ~0xFF00U) | ((uint32_t)stk1c << 8);
    r_ecx = (r_ecx & ~0xFF00U) | (((((r_ecx >> 8) & 0xFFU) - 1U) & 0xFFU) << 8);
     r_eax = (uint32_t)d3u_ld32(c, (uint32_t)((r_edx) & U32MASK));
    stk1c = (unsigned char)((r_ecx >> 8) & 0xFFU);
    if (r_eax == 0xFFFFFFFFU) { _pc = 8; continue; }
     r_ecx = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
     r_ebx = r_eax;
     r_esi = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
     r_ebx = (r_ebx - r_ecx) & U32MASK;
     r_ebx = (r_ebx & r_esi) & U32MASK;
    if (r_ebx < 0x140U) { _pc = 8; continue; }
    if (r_eax >= 0x40U) { _pc = 3; continue; }
     r_eax = (r_eax + (uint32_t)D3U_DICT_SIZE_32(&c->st, 0)) & U32MASK;
            _pc = 3;
            continue;
        case 3:
    /* loc_4160CC: */
     r_esi = stk2c;
     r_ebx = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
     r_ecx = (r_ecx & ~0xFFU) | (uint32_t)c->blk00_dict[((r_esi + r_ebx - 0x0CU) & U32MASK) - c->blk00_base];
    if ((r_ecx & 0xFFU) != (uint32_t)c->blk00_dict[((r_ebx + r_eax - 0x0CU) & U32MASK) - c->blk00_base]) { _pc = 8; continue; }
     r_eax = (r_eax - 1U) & U32MASK;
     r_esi = (r_ebx + r_eax) & U32MASK;
     r_eax = stk3c;
     r_ecx = (r_ecx & ~0xFFU) | 0x40U;
     r_ebx = (r_ebx + r_eax) & U32MASK;
            _pc = 4;
            continue;
        case 4:
    /* loc_4160EF: */
    if ((r_ecx & 0xFFU) < 8U) { _pc = 5; continue; }
     r_eax = (r_eax & ~0xFFU) | (uint32_t)c->blk00_dict[((r_ebx) & U32MASK) - c->blk00_base];
    if ((r_eax & 0xFFU) != (uint32_t)c->blk00_dict[((r_esi) & U32MASK) - c->blk00_base]) { _pc = 7; continue; }
     r_eax = (r_eax & ~0xFFU) | (uint32_t)c->blk00_dict[((r_ebx - 1U) & U32MASK) - c->blk00_base];
    r_ecx = (r_ecx & ~0xFFU) | ((r_ecx - 1U) & 0xFFU);
    if ((r_eax & 0xFFU) != (uint32_t)c->blk00_dict[((r_esi - 1U) & U32MASK) - c->blk00_base]) { _pc = 7; continue; }
     r_eax = (r_eax & ~0xFFU) | (uint32_t)c->blk00_dict[((r_ebx - 2U) & U32MASK) - c->blk00_base];
    r_ecx = (r_ecx & ~0xFFU) | ((r_ecx - 1U) & 0xFFU);
    if ((r_eax & 0xFFU) != (uint32_t)c->blk00_dict[((r_esi - 2U) & U32MASK) - c->blk00_base]) { _pc = 7; continue; }
     r_eax = (r_eax & ~0xFFU) | (uint32_t)c->blk00_dict[((r_ebx - 3U) & U32MASK) - c->blk00_base];
    r_ecx = (r_ecx & ~0xFFU) | ((r_ecx - 1U) & 0xFFU);
    if ((r_eax & 0xFFU) != (uint32_t)c->blk00_dict[((r_esi - 3U) & U32MASK) - c->blk00_base]) { _pc = 7; continue; }
     r_eax = (r_eax & ~0xFFU) | (uint32_t)c->blk00_dict[((r_ebx - 4U) & U32MASK) - c->blk00_base];
    r_ecx = (r_ecx & ~0xFFU) | ((r_ecx - 1U) & 0xFFU);
    if ((r_eax & 0xFFU) != (uint32_t)c->blk00_dict[((r_esi - 4U) & U32MASK) - c->blk00_base]) { _pc = 7; continue; }
     r_eax = (r_eax & ~0xFFU) | (uint32_t)c->blk00_dict[((r_ebx - 5U) & U32MASK) - c->blk00_base];
    r_ecx = (r_ecx & ~0xFFU) | ((r_ecx - 1U) & 0xFFU);
    if ((r_eax & 0xFFU) != (uint32_t)c->blk00_dict[((r_esi - 5U) & U32MASK) - c->blk00_base]) { _pc = 7; continue; }
     r_eax = (r_eax & ~0xFFU) | (uint32_t)c->blk00_dict[((r_ebx - 6U) & U32MASK) - c->blk00_base];
    r_ecx = (r_ecx & ~0xFFU) | ((r_ecx - 1U) & 0xFFU);
    if ((r_eax & 0xFFU) != (uint32_t)c->blk00_dict[((r_esi - 6U) & U32MASK) - c->blk00_base]) { _pc = 7; continue; }
     r_eax = (r_eax & ~0xFFU) | (uint32_t)c->blk00_dict[((r_ebx - 7U) & U32MASK) - c->blk00_base];
    r_ecx = (r_ecx & ~0xFFU) | ((r_ecx - 1U) & 0xFFU);
    if ((r_eax & 0xFFU) != (uint32_t)c->blk00_dict[((r_esi - 7U) & U32MASK) - c->blk00_base]) { _pc = 7; continue; }
     r_ebx = (r_ebx - 8U) & U32MASK;
     r_esi = (r_esi - 8U) & U32MASK;
    r_ecx = (r_ecx & ~0xFFU) | ((r_ecx - 1U) & 0xFFU);
    if ((r_ecx & 0xFFU) != 0U) { _pc = 4; continue; }
    { _pc = 7; continue; }
        case 5:
    /* loc_41614C: */
     r_esi = (r_esi - r_ebx) & U32MASK;
     r_eax = (r_eax & ~0xFFU) | (uint32_t)c->blk00_dict[((r_ebx) & U32MASK) - c->blk00_base];
            _pc = 6;
            continue;
        case 6:
    /* loc_416150: */
    if ((r_eax & 0xFFU) != (uint32_t)c->blk00_dict[((r_ebx + r_esi) & U32MASK) - c->blk00_base]) { _pc = 7; continue; }
     r_eax = (r_eax & ~0xFFU) | (uint32_t)c->blk00_dict[((r_ebx - 1U) & U32MASK) - c->blk00_base];
     r_ebx = (r_ebx - 1U) & U32MASK;
    r_ecx = (r_ecx & ~0xFFU) | ((r_ecx - 1U) & 0xFFU);
    if ((r_ecx & 0xFFU) != 0U) { _pc = 6; continue; }
            _pc = 7;
            continue;
        case 7:
    /* loc_41615D: */
     r_eax = (r_eax & ~0xFFU) | 0x40U;
    r_eax = (r_eax & ~0xFFU) | (((r_eax & 0xFFU) - (r_ecx & 0xFFU)) & 0xFFU);
    if ((r_eax & 0xFFU) < (uint32_t)stk20) { _pc = 8; continue; }
    stk20 = (unsigned char)(r_eax & 0xFFU);
     r_edi = (uint32_t)d3u_ld32(c, (uint32_t)((r_edx) & U32MASK));
            _pc = 8;
            continue;
        case 8:
    /* loc_41616D: */
    if (stk1c == 0) { _pc = 9; continue; }
     r_edx = (r_edx - 4U) & U32MASK;
     r_eax = (uint32_t)d3u_ld32(c, (uint32_t)((r_edx) & U32MASK));
     d3u_st32(c, (uint32_t)((r_edx + 4U) & U32MASK), (unsigned int)r_eax);
    { _pc = 2; continue; }
        case 9:
    /* loc_416181: */
     r_eax = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
    r_ebx = (r_ebx & ~0xFF00U) | ((uint32_t)stk20 << 8);
     d3u_st32(c, (uint32_t)((r_edx) & U32MASK), (unsigned int)r_eax);
    if (((r_ebx >> 8) & 0xFFU) < 0x0CU) { _pc = 10; continue; }
     stk28 = 0x11;
            _pc = 10;
            continue;
        case 10:
    /* loc_416196: */
    if (stk28 != 0) { _pc = 14; continue; }
     r_eax = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
     r_edx = (uint32_t)D3U_TBL_444244_32(&c->st, 0);
     r_edi = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
     r_eax = (r_eax - r_edx) & U32MASK;
     r_edi = (r_edi & r_eax) & U32MASK;
     r_ebx = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
     r_eax = r_edi;
     r_ecx = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
     r_eax = (r_eax - r_ebx) & U32MASK;
     r_eax = (r_eax & r_ecx) & U32MASK;
    if (r_eax < 0x104U) { _pc = 12; continue; }
     r_eax = (r_edi - 4U) & U32MASK;
     r_edx = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
     r_eax = (r_eax & r_ecx) & U32MASK;
     r_eax = (r_eax + r_edx) & U32MASK;
     r_edx = (r_edx & ~0xFFU) | (uint32_t)c->blk00_dict[((r_eax) & U32MASK) - c->blk00_base];
    stk30 = (stk30 & ~0xFF000000U) | ((r_edx & 0xFFU) << 24);
     r_edx = (r_edx & ~0xFFU) | (uint32_t)c->blk00_dict[((r_eax + 1U) & U32MASK) - c->blk00_base];
    stk30 = (stk30 & ~0x00FF0000U) | ((r_edx & 0xFFU) << 16);
     r_edx = (r_edx & ~0xFFU) | (uint32_t)c->blk00_dict[((r_eax + 2U) & U32MASK) - c->blk00_base];
     r_eax = (r_eax & ~0xFFU) | (uint32_t)c->blk00_dict[((r_eax + 3U) & U32MASK) - c->blk00_base];
    stk30 = (stk30 & ~0x0000FF00U) | ((r_edx & 0xFFU) << 8);
    stk30 = (stk30 & ~0x000000FFU) | (r_eax & 0xFFU);
     r_edx = stk30;
     r_eax = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
     r_edx &= 0xFFFFU;
     r_eax &= 0xFFFFU;
    if (r_edx == r_eax) { _pc = 11; continue; }
     r_eax = stk30;
     r_edx = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    r_eax &= ~0xFF00U;
    r_edx &= ~0xFF00U;
    if (r_eax == r_edx) { _pc = 11; continue; }
     r_edx = stk30;
     r_eax = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    r_edx &= ~0xFFU;
    r_eax &= ~0xFFU;
    if (r_edx != r_eax) { _pc = 12; continue; }
            _pc = 11;
            continue;
        case 11:
    /* loc_416232: */
    stk28 = 9;
    stk24 = 1;
            _pc = 12;
            continue;
        case 12:
    /* loc_41623C: */
    if (stk28 != 0) { _pc = 14; continue; }
     r_eax = stk38;
     r_ebp = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    if (r_ebp != (uint32_t)d3u_ld32(c, (uint32_t)((r_eax + 4U) & U32MASK))) { _pc = 13; continue; }
     r_edi = stk38;
     r_edi = (uint32_t)d3u_ld32(c, (uint32_t)((r_edi) & U32MASK));
            _pc = 13;
            continue;
        case 13:
    /* loc_416258: */
    r_eax = (r_eax & ~0xFFU) | ((r_edi != 0xFFFFFFFFU) ? 1U : 0U);
    stk28 = (unsigned char)(r_eax & 0xFFU);
            _pc = 14;
            continue;
        case 14:
    /* loc_416262: */
     r_edx = stk38;
     r_eax = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
     d3u_st32(c, (uint32_t)((r_edx) & U32MASK), (unsigned int)r_eax);
     r_eax = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
     d3u_st32(c, (uint32_t)((r_edx + 4U) & U32MASK), (unsigned int)r_eax);
    if (stk28 == 0) { _pc = 29; continue; }
     r_edx = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
     r_eax = r_edi;
     r_ebx = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
     r_eax = (r_eax - r_edx) & U32MASK;
     r_eax = (r_eax & r_ebx) & U32MASK;
    if (r_eax < 0x100U) { _pc = 29; continue; }
    r_ebx = (r_ebx & ~0xFFU) | (uint32_t)stk28;
     r_ecx = stk34;
     r_eax = (uint32_t)(uint32_t)D3U_TBL_4299CC_32(&c->st, 0);
    r_ebx = (r_ebx & ~0xFFU) | ((r_ebx - 1U) & 0xFFU);
     r_eax = (r_eax + r_ecx) & U32MASK;
    stk28 = (unsigned char)(r_ebx & 0xFFU);
    if (M8((r_eax) & U32MASK) != 0) { _pc = 15; continue; }
    r_ecx = (r_ecx & ~0xFFU) | (r_ebx & 0xFFU);
     r_ecx = (r_ecx & ~0xFFU) | ((r_ecx + 0x18U) & 0xFFU);
    stk28 = (unsigned char)(r_ecx & 0xFFU);
            _pc = 15;
            continue;
        case 15:
    /* loc_4162C0: */
     r_esi = (uint32_t)D3U_TBL_444218_32(&c->st, 0);
    if (r_esi == 0) { _pc = 16; continue; }
     r_eax = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
     r_edx = (r_edx & ~0xFFU) | (uint32_t)c->blk00_dict[((r_esi) & U32MASK) - c->blk00_base];
    if ((r_edx & 0xFFU) == (uint32_t)c->blk00_dict[((r_edi + r_eax) & U32MASK) - c->blk00_base]) { _pc = 27; continue; }
            _pc = 16;
            continue;
        case 16:
    /* loc_4162DA: */
     r_eax = 0;
    r_eax = (r_eax & ~0xFFU) | (uint32_t)stk28;
    r_eax = (r_eax & ~0xFFU) | d3uc_decode_length_sym(c, (unsigned int)(r_eax & 0xFFU));
    if ((r_eax & 0xFFU) != 1U) { _pc = 28; continue; }
    r_eax = (r_eax & ~0xFF00U) | ((uint32_t)stk24 << 8);
     r_ebx = (uint32_t)D3U_TBL_444220_32(&c->st, 0);
    r_eax = (r_eax & ~0xFFU) | (((((r_eax >> 8) & 0xFFU) == 0) ? 1U : 0U));
     r_ebx = (r_ebx << 2) & U32MASK;
    D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)(r_eax & 0xFFU);
    if ((r_eax & 0xFFU) == 0U) { _pc = 20; continue; }
    /* The quotient (count<<2)/period indexes a 16-entry table.  Count
     * and period start equal, count grows by 5 and period by 2 in the
     * same list update, and both halve together past 0x800.  Enumerating
     * the reachable (count,period) states gives quotient <= 10, so the
     * two reads stay in 0x429B98..0x429BA7 and 0x429BA8..0x429BB7. */
     r_ecx = (uint32_t)D3U_TBL_444250_32(&c->st, 0);
    r_eax = (r_eax & ~0xFFU) | 7U;
    if (r_ecx == 0) { _pc = 17; continue; }
     r_eax = r_ebx;
     r_edx = 0;
     r_eax = (r_ebx / r_ecx) & U32MASK; r_edx = (r_ebx % r_ecx) & U32MASK; /* x86 div faults on div0/overflow: direct / % */
    r_edx = (r_edx & ~0xFFU) | (uint32_t)D3U_FLAG_444EA0_8(&c->st, 0);
     r_eax = (r_eax & ~0xFFU) | (uint32_t)d3u_t429B98[0x10U + (r_eax & U32MASK)];
    r_eax = (r_eax & ~0xFFU) | (((r_eax & 0xFFU) - (r_edx & 0xFFU)) & 0xFFU);
    r_eax = (r_eax & ~0xFFU) | (((r_eax & 0xFFU) + 7U) & 0xFFU);
            _pc = 17;
            continue;
        case 17:
    /* loc_41632A: */
     r_edx = (uint32_t)D3U_TBL_444264_32(&c->st, 0);
    if (r_edx <= (uint32_t)D3U_TBL_444250_32(&c->st, 0)) { _pc = 18; continue; }
    r_eax = (r_eax & ~0xFFU) | 0x1DU;
    { _pc = 19; continue; }
        case 18:
    /* loc_41633C: */
    if (D3U_FLAG_444EAF_8(&c->st, 0) == 0) { _pc = 19; continue; }
    r_eax = (r_eax & ~0xFFU) | (((r_eax & 0xFFU) + (r_eax & 0xFFU)) & 0xFFU);
            _pc = 19;
            continue;
        case 19:
    /* loc_416347: */
    D3U_FLAG_444E82_8(&c->st, 0) = 3;
    { _pc = 21; continue; }
        case 20:
    /* loc_416350: */
     r_ebp = (uint32_t)D3U_TBL_444250_32(&c->st, 0);
    r_eax = (r_eax & ~0xFFU) | 3U;
    if (r_ebp == 0) { _pc = 21; continue; }
     r_eax = r_ebx;
     r_edx = 0;
     r_eax = (r_ebx / r_ebp) & U32MASK; r_edx = (r_ebx % r_ebp) & U32MASK; /* x86 div faults on div0/overflow: direct / % */
     r_eax = (r_eax & ~0xFFU) | (uint32_t)d3u_t429B98[r_eax & U32MASK];
    r_eax = (r_eax & ~0xFFU) | (((r_eax & 0xFFU) + 3U) & 0xFFU);
            _pc = 21;
            continue;
        case 21:
    /* loc_41636A: */
    r_eax = (r_eax & ~0xFF00U) | ((r_eax & 0xFFU) << 8);
    r_eax = (r_eax & ~0xFFU) | ((r_eax - 1U) & 0xFFU);
    if (((r_eax >> 8) & 0xFFU) <= 7U) { _pc = 22; continue; }
    r_eax = (r_eax & ~0xFF00U) | ((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0) << 8);
     r_edx = 0;
    r_edx = (r_edx & ~0xFFU) | ((r_eax >> 8) & 0xFFU);
     M8((D3U_FLAG_444E82 + r_edx) & U32MASK) = (unsigned char)(r_eax & 0xFFU);
    r_eax = (r_eax & ~0xFF00U) | (((((r_eax >> 8) & 0xFFU) + 1U) & 0xFFU) << 8);
    D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)((r_eax >> 8) & 0xFFU);
    { _pc = 21; continue; }
        case 22:
    /* loc_41638D: */
    if (stk24 != 0) { _pc = 23; continue; }
     r_edx = stk34;
     r_eax = (uint32_t)(uint32_t)D3U_TBL_4299CC_32(&c->st, 0);
     r_eax = (r_eax + r_edx) & U32MASK;
    if (M8((r_eax) & U32MASK) < 0x10U) { _pc = 23; continue; }
     r_esi = 1;
    { _pc = 24; continue; }
        case 23:
    /* loc_4163AB: */
     r_esi = 0;
            _pc = 24;
            continue;
        case 24:
    /* loc_4163AD: */
     r_ebx = 0x30AU;
     r_esi = (r_esi + 0x2FBU) & U32MASK;
     r_edx = 0;
     r_eax = r_esi;
     D3U_TBL_44421C_32(&c->st, 0) = (unsigned int)r_ebx;
    r_eax = (r_eax & ~0xFFU) | d3uc_decode_offset_sym(c, (unsigned int)r_eax);
    r_ebx = (r_ebx & ~0xFFU) | (r_eax & 0xFFU);
    r_ecx = (r_ecx & ~0xFFU) | (r_eax & 0xFFU);
    r_edx = (r_edx & ~0xFFU) | (r_eax & 0xFFU);
     r_eax = r_esi;
    d3uc_ctx_model_update(c, (unsigned int)r_eax, (unsigned int)r_edx);
     r_esi = stk34;
     r_eax = (uint32_t)(uint32_t)D3U_TBL_4299CC_32(&c->st, 0);
     r_edx = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
     r_eax = (r_eax + r_esi) & U32MASK;
     r_edx = (r_edx - r_edi) & U32MASK;
     M8((r_eax) & U32MASK) = (unsigned char)(r_ebx & 0xFFU);
     r_eax = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
     r_ebp = 0;
     r_edx = (r_edx & r_eax) & U32MASK;
     D3U_TBL_444218_32(&c->st, 0) = (unsigned int)r_ebp;
     D3U_TBL_444244_32(&c->st, 0) = (unsigned int)r_edx;
    if ((r_ebx & 0xFFU) < 7U) { _pc = 25; continue; }
    if ((r_ebx & 0xFFU) >= 0xFFU) { _pc = 25; continue; }
     r_eax = 0;
    r_eax = (r_eax & ~0xFFU) | (r_ebx & 0xFFU);
     r_edx = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
     r_eax = (r_eax + r_edi) & U32MASK;
     r_eax = (r_eax & r_edx) & U32MASK;
     r_edx = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
     r_edx = (r_edx + r_eax) & U32MASK;
     D3U_TBL_444218_32(&c->st, 0) = (unsigned int)r_edx;
            _pc = 25;
            continue;
        case 25:
    /* loc_416424: */
     r_eax = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
     r_ebx = 0;
     r_ecx = (r_ecx & ~0xFF00U) | ((uint32_t)c->blk00_dict[((r_edi + r_eax) & U32MASK) - c->blk00_base] << 8);
    r_ebx = (r_ebx & ~0xFFU) | ((r_ecx >> 8) & 0xFFU);
     r_eax = r_ebx;
     r_edx = 0;
    d3uc_emit_literal(c, (unsigned int)(r_eax & 0xFFU));
    r_eax = (r_eax & ~0xFFU) | ((((r_ecx & 0xFFU) <= 8U) ? 1U : 0U));
    r_edx = (r_edx & ~0xFFU) | (r_eax & 0xFFU);
     r_eax = r_ebx;
    d3uc_update_match_chain(c, (unsigned int)r_eax, (unsigned int)r_edx);
     r_edx = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
     r_eax = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
     c->blk00_dict[((r_edx + r_eax) & U32MASK) - c->blk00_base] = (unsigned char)((r_ecx >> 8) & 0xFFU);
    if (r_eax >= 0x100U) { _pc = 26; continue; }
     r_eax = (r_eax + (uint32_t)D3U_DICT_SIZE_32(&c->st, 0)) & U32MASK;
     c->blk00_dict[((r_edx + r_eax) & U32MASK) - c->blk00_base] = (unsigned char)((r_ecx >> 8) & 0xFFU);
            _pc = 26;
            continue;
        case 26:
    /* loc_416466: */
     r_edx = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
     r_eax = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
     r_edx = (r_edx + 1U) & U32MASK;
     r_edi = (r_edi + 1U) & U32MASK;
     r_edx = (r_edx & r_eax) & U32MASK;
     r_edi = (r_edi & r_eax) & U32MASK;
     D3U_DICT_CURSOR_32(&c->st, 0) = (unsigned int)r_edx;
    r_ecx = (r_ecx & ~0xFFU) | ((r_ecx - 1U) & 0xFFU);
    if ((r_ecx & 0xFFU) != 0U) { _pc = 25; continue; }
    r_eax = (r_eax & ~0xFFU) | 1U;
    /* retn */ return (r_eax & 0xFFU);
        case 27:
    /* loc_41648D: */
     r_eax = 0;
     r_edx = 2;
    r_eax = (r_eax & ~0xFFU) | (uint32_t)stk28;
    (void)d3uc_update_len_model(c, (unsigned int)(r_eax & 0xFFU), (unsigned int)r_edx);
            _pc = 28;
            continue;
        case 28:
    /* loc_41649D: */
     r_edi = stk34;
     r_eax = (uint32_t)(uint32_t)D3U_TBL_4299CC_32(&c->st, 0);
     r_eax = (r_eax + r_edi) & U32MASK;
     M8((r_eax) & U32MASK) = 0;
            _pc = 29;
            continue;
        case 29:
    /* loc_4164AB: */
    r_eax = (r_eax & ~0xFFU) | 0U;
    /* retn */ return (r_eax & 0xFFU);
        default:
            break;
        }
        break;
    }
    return 0;
}

/* ---- fallback decoder (from draft_f3.c) ---- */
void d3uc_decode_fallback(dec3w_ctx *c, unsigned int edi_in)
{
uint32_t t0 = 0;
uint32_t t1 = 0;
uint32_t t2 = 0;
uint32_t t3 = 0;
uint32_t t4 = 0;
uint32_t t5 = 0;
uint32_t t6 = 0;
uint32_t var_1C = 0;
uint32_t var_4C = 0;
uint32_t var_50 = 0;
uint32_t var_3C = 0;
uint32_t var_48 = 0;
uint32_t var_2C = 0;
uint32_t var_30 = 0;
uint32_t var_40 = 0;
uint32_t var_28 = 0;
uint32_t var_34 = 0;
uint32_t var_38 = 0;
uint32_t var_44 = 0;
uint32_t var_24 = 0;
uint32_t var_20 = 0;
unsigned char v68[13] = {0};
unsigned char v58[8] = {0};
uint32_t tmp = 0;
uint32_t q = 0;
uint32_t r = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
t5 = (uint32_t)edi_in; /* entry t5 live */
t0 = (t0 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EA0_8(&c->st, 0)) & 0xFFU);
var_1C = (t0 & 0xFFU) & 0xFFU;
t3 = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
t3 &= U32MASK; /* lane init */
var_4C = 1U;
t0 = t3 & U32MASK;
D3U_TBL_444240_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
if ((uint32_t)D3U_FLAG_444EAF_8(&c->st, 0) != 2U) { _pc = 1; continue; }
t3 = ((t3 >> 8) & U32MASK);
t0 = ((t0 >> 16) & U32MASK);
t3 &= 0xFFU;
t0 &= 0xFF00U;
t3 = (t3 + t0) & U32MASK;
t0 = (uint32_t)D3U_TBL_44425C_32(&c->st, 0);
t0 = ((t0 << 8) & U32MASK);
t0 &= 0xFF0000U;
t3 = (t3 + t0) & U32MASK;
t0 = (uint32_t)D3U_TBL_44425C_32(&c->st, 0);
t0 &= 0xFF000000U;
t3 = (t3 + t0) & U32MASK;
D3U_TBL_444240_32(&c->st, 0) = (unsigned int)(t3 & U32MASK);
t0 = t3 & U32MASK;
            _pc = 1;
            continue;
        case 1:
t0 = ((t0 >> 13) & U32MASK);
t0 ^= t3;
t0 &= 0x7FFFU;
t6 = (uint32_t)(uint32_t)D3U_BLK02_MODEL_A_32(&c->st, 0);
D3U_TBL_444224_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
t0 = t3 & U32MASK;
t4 = (uint32_t)D3U_TBL_444218_32(&c->st, 0);
t0 &= 0xFFFFU;
t3 &= 0xFFFF00FFU;
t3 = (t3 & ~0xFF00U) | (((0U) & 0xFFU) << 8);
D3U_TBL_444238_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
D3U_FLAG_444EB0_32(&c->st, 0) = 0;
D3U_FLAG_444EA0_8(&c->st, 0x15) = 0;
D3U_FLAG_444EA0_8(&c->st, 0x0E) = 0;
D3U_FLAG_444EA0_8(&c->st, 0x0C) = 0;
D3U_FLAG_444EA0_8(&c->st, 0x01) = 0;
D3U_FLAG_444EA0_8(&c->st, 0x04) = 0;
D3U_FLAG_444EA0_8(&c->st, 0x0A) = 0;
t0 = (uint32_t)D3U_TBL_444238_32(&c->st, 0);
{ _pc = 2; continue; }
        case 2:
t3 = t0 & U32MASK;
t0 = ((t0 << 2) & U32MASK);
t1 = 0U;
t0 = (t0 + t3) & U32MASK;
D3U_TBL_444204_32(&c->st, 0) = 0;
t0 = ((t0 << 2) & U32MASK);
D3U_TBL_44420C_32(&c->st, 0) = 0;
t6 = (t6 + t0) & U32MASK;
if (t4 == 0U) { _pc = 3; continue; }
D3U_TBL_444218_32(&c->st, 0) = 0;
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)t4)) & 0xFFU);
t1 = (t1 & 0xFFFF00FFU) | (1U << 8);
t1 = (t1 & ~0xFF00U) | (((1U) & 0xFFU) << 8);
D3U_FLAG_444E82_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)(((t1 >> 8) & 0xFFU) & 0xFFU);
            _pc = 3;
            continue;
        case 3:
if (((uint32_t)D3U_FLAG_444EBA_8(&c->st, 0) > 1U)) { _pc = 4; continue; }
t0 = (t0 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EB9_8(&c->st, 0)) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((((t0 & 0xFFU) - 2) & 0xFFU)) & 0xFFU);
if (((t0 & 0xFFU) & 0xFFU) > 2U) { _pc = 6; continue; }
            _pc = 4;
            continue;
        case 4:
t1 &= 0xFFFFFF00U;
t1 = 0U;
t1 = (t1 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EB9_8(&c->st, 0)) & 0xFFU);
t1 &= 0xFFU;
if ((t1 & U32MASK) > 4U) { _pc = 5; continue; }
t0 = (uint32_t)D3U_TBL_4441C0_32(&c->st, 0);
t3 = 0U;
q = t1 ? (t0 / t1) : 0U; r = t1 ? (t0 % t1) : 0U;
t0 = q & U32MASK; t3 = r & U32MASK;
t1 = ((t1 << 2) & U32MASK);
t3 = (t3 + t1) & U32MASK;
t1 = (t3 + 0xF8U) & U32MASK;
            _pc = 5;
            continue;
        case 5:
t0 = D3U_TBL_442528;
t1 = ((t1 << 2) & U32MASK);
t0 = (t0 + t1) & U32MASK;
D3U_TBL_4441F8_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
            _pc = 6;
            continue;
        case 6:
if ((var_1C & 0xFFU) == 0U) { _pc = 12; continue; }
if ((uint32_t)D3U_FLAG_444EA6_8(&c->st, 0) == 0U) { _pc = 7; continue; }
t3 = (uint32_t)D3U_TBL_443B60_32(&c->st, 0);
t0 = t3 & U32MASK;
t1 = (uint32_t)D3U_TBL_443B64_32(&c->st, 0);
t0 = ((t0 << 5) & U32MASK);
t1 = ((t1 << 5) & U32MASK);
t0 = (t0 - t3) & U32MASK;
if (!((t0 & U32MASK) < (t1 & U32MASK))) { _pc = 8; continue; }
{ _pc = 9; continue; }
        case 7:
t0 = (uint32_t)D3U_TBL_443B60_32(&c->st, 0);
t3 = (uint32_t)D3U_TBL_443B64_32(&c->st, 0);
tmp = ((t0 & U32MASK) < (t3 & U32MASK)) ? 1U : 0U;
{ _pc = 10; continue; }
        case 8:
tmp = 0U;
{ _pc = 10; continue; }
        case 9:
tmp = 1U;
{ _pc = 10; continue; }
        case 10:
        case 11:
t0 = (t0 & ~0xFFU) | ((tmp & 0xFFU) & 0xFFU);
var_1C = (t0 & 0xFFU) & 0xFFU;
            _pc = 12;
            continue;
        case 12:
if ((uint32_t)D3U_FLAG_444EA9_8(&c->st, 0) == 0U) { _pc = 16; continue; }
if (!((uint32_t)D3U_FLAG_444EBA_8(&c->st, 0) > 4U)) { _pc = 13; continue; }
t0 = (uint32_t)D3U_TBL_4441F8_32(&c->st, 0);
if (!((uint32_t)d3u_ld32(c, (uint32_t)((uint32_t)t0)) > 0x3FEU)) { _pc = 13; continue; }
{ _pc = 14; continue; }
        case 13:
t0 = 1U;
{ _pc = 15; continue; }
        case 14:
t0 = 0U;
            _pc = 15;
            continue;
        case 15:
if (t0 == 0U) { _pc = 16; continue; }
t0 = (t0 & ~0xFFU) | ((1U) & 0xFFU);
{ _pc = 17; continue; }
        case 16:
t0 = (t0 & ~0xFFU) | ((0U) & 0xFFU);
            _pc = 17;
            continue;
        case 17:
t3 = (t3 & ~0xFF00U) | ((((var_1C & 0xFFU)) & 0xFFU) << 8);
var_3C = (t0 & 0xFFU) & 0xFFU;
if ((((t3 >> 8) & 0xFFU) & 0xFFU) == 0U) { _pc = 18; continue; }
if (((t0 & 0xFFU) & 0xFFU) == 0U) { _pc = 18; continue; }
t0 = (t0 & ~0xFFU) | ((1U) & 0xFFU);
{ _pc = 19; continue; }
        case 18:
t0 = (t0 & ~0xFFU) | ((0U) & 0xFFU);
            _pc = 19;
            continue;
        case 19:
var_48 = (t0 & 0xFFU) & 0xFFU;
if (((t0 & 0xFFU) & 0xFFU) == 0U) { _pc = 20; continue; }
t0 = (uint32_t)D3U_TBL_4441E0_32(&c->st, 0);
if (!((t0 & U32MASK) < (uint32_t)D3U_TBL_444200_32(&c->st, 0))) { _pc = 20; continue; }
t0 = (t0 & ~0xFFU) | ((1U) & 0xFFU);
{ _pc = 21; continue; }
        case 20:
t0 = (t0 & ~0xFFU) | ((0U) & 0xFFU);
            _pc = 21;
            continue;
        case 21:
t4 = (uint32_t)D3U_TBL_444224_32(&c->st, 0);
var_2C = (t0 & 0xFFU) & 0xFFU;
if (!((t4 & U32MASK) < 0x40000U)) { _pc = 40; continue; }
t0 = t4 & U32MASK;
t0 = ((t0 << 5) & U32MASK);
            _pc = 22;
            continue;
        case 22:
t1 = (uint32_t)(uint32_t)D3U_BLK01_DICT_COPY_32(&c->st, 0);
t1 = (t1 + t0) & U32MASK;
if ((uint32_t)M8((uint32_t)t1) == 0U) { _pc = 39; continue; }
t2 = (uint32_t)D3U_TBL_444240_32(&c->st, 0);
if (t2 != (uint32_t)d3u_ld32(c, (uint32_t)((uint32_t)(t1 + 0x1C)))) { _pc = 39; continue; }
t0 = 0U;
t2 = (t2 & 0xFFFFFF00U) | 0xFFU;
t2 = (t2 & ~0xFF00U) | (((0xFFU) & 0xFFU) << 8);
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)t1)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
D3U_FLAG_444EB7_8(&c->st, 0) = (unsigned char)(((t2 >> 8) & 0xFFU) & 0xFFU);
D3U_TBL_444214_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
t0 = t1 & U32MASK;
t2 = (t1 + 0x0DU) & U32MASK;
            _pc = 23;
            continue;
        case 23:
t3 = 0U;
t4 = (uint32_t)D3U_TBL_444214_32(&c->st, 0);
t3 = (t3 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t0 + 1))) & 0xFFU);
t3 &= 0xFFU;
t4 = (t4 + t3) & U32MASK;
t0 = (t0 + 1) & U32MASK;
D3U_TBL_444214_32(&c->st, 0) = (unsigned int)(t4 & U32MASK);
if (t0 != t2) { _pc = 23; continue; }
if ((uint32_t)D3U_TBL_44423C_32(&c->st, 0) != 0U) { _pc = 24; continue; }
t0 = (uint32_t)D3U_TBL_444264_32(&c->st, 0);
if (!((t0 & U32MASK) <= (uint32_t)D3U_TBL_444250_32(&c->st, 0))) { _pc = 24; continue; }
if (!((uint32_t)D3U_FLAG_444EBA_8(&c->st, 0) > 1U)) { _pc = 24; continue; }
t0 = (uint32_t)D3U_TBL_4441F8_32(&c->st, 0);
if ((uint32_t)d3u_ld32(c, (uint32_t)((uint32_t)t0)) > 0x3FEU) { _pc = 40; continue; }
            _pc = 24;
            continue;
        case 24:
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t1 + 0x1B))) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((t0 & 0xFFU) & 0x3FU) & 0xFFU);
if (!(((t0 & 0xFFU) & 0xFFU) < 8U)) { _pc = 25; continue; }
if ((uint32_t)M8((uint32_t)t6) == 0U) { _pc = 25; continue; }
t0 = (t0 & 0xFFU) & 0xFFU;
t3 = 0x3E0U;
t0 = ((t0 << 5) & U32MASK);
t3 = (t3 - t0) & U32MASK;
t0 = 0U;
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t6 + 1))) & 0xFFU);
t0 &= 0xFFU;
t3 = (t3 * t0) & U32MASK;
t2 = 0U;
t2 = (t2 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t6 + 0x12))) & 0xFFU);
t2 &= 0xFFU;
t0 = t3 & U32MASK;
t3 = ((((int)t0) < 0) ? U32MASK : 0U);
if (t2 == 0U) { t0 = 0U; t3 = 0U; } else { q = (uint32_t)(((int)t0) / (int)(int)t2); r = (uint32_t)(((int)t0) % (int)(int)t2); t0 = q & U32MASK; t3 = r & U32MASK; }
t2 = t0 & U32MASK;
t0 = 0U;
t4 = (uint32_t)D3U_TBL_444214_32(&c->st, 0);
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t1 + 1))) & 0xFFU);
t0 &= 0xFFU;
t3 = 0U;
t0 = ((t0 << 10) & U32MASK);
if (t4 == 0U) { t0 = 0U; } else { q = (t0 / t4) & U32MASK; r = (t0 % t4) & U32MASK; t0 = q; t3 = r; }
if (!((t2 & U32MASK) > (t0 & U32MASK))) { _pc = 25; continue; }
{ _pc = 40; continue; }
        case 25:
t3 = (t3 & ~0xFFU) | ((1U) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | 0x01U;
t5 = 0U;
t4 = 0x4137F0U;
t0 = (uint32_t)D3U_TBL_444214_32(&c->st, 0);
D3U_FLAG_444EB0_8(&c->st, 0) = (unsigned char)((t3 & 0xFFU) & 0xFFU);
D3U_TBL_444234_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
t3 = (t3 & ~0xFF00U) | ((((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU) << 8);
t3 = (t3 & 0xFFFF00FFU) | ((((t3 >> 8) & 0xFFU) & 0xFFU) << 8);
    { unsigned int _k; static const unsigned char _b13[13] = {1,1,1,1,1,1,1,1,1,1,1,1,1};
      for (_k = 0; _k < 13; _k++) v68[_k] = _b13[_k]; }
if (((((t3 >> 8) & 0xFFU)) & 0xFFU) == 0U) { _pc = 28; continue; }
t0 = (t0 & ~0xFFU) | ((0U) & 0xFFU);
t0 = (t0 & ~0xFF00U) | ((((uint32_t)D3U_FLAG_444E82_8(&c->st, 0)) & 0xFFU) << 8);
var_34 = (((t0 >> 8) & 0xFFU)) & 0xFFU;
            _pc = 26;
            continue;
        case 26:
t2 = 0U;
t2 = (t2 & ~0xFFU) | (((t0 & 0xFFU) & 0xFFU) & 0xFFU);
t2 &= 0xFFU;
t3 = (t1 + t2) & U32MASK;
if ((uint32_t)M8((uint32_t)(t3 + 1)) == 0U) { _pc = 28; continue; }
t0 = (t0 & ~0xFF00U) | ((((uint32_t)M8((uint32_t)(t3 + 0x0E))) & 0xFFU) << 8);
t0 = (t0 & 0xFFFF00FFU) | ((((t0 >> 8) & 0xFFU) & 0xFFU) << 8);
if (((((t0 >> 8) & 0xFFU)) & 0xFFU) != ((var_34) & 0xFFU)) { _pc = 27; continue; }
t0 = (t0 & ~0xFFU) | ((0U) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | 0U;
if (t2 < 13U) v68[t2] = 0;
t0 = 0U;
t5 = (uint32_t)D3U_TBL_444234_32(&c->st, 0);
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t3 + 1))) & 0xFFU);
t0 &= 0xFFU;
t5 = (t5 - t0) & U32MASK;
D3U_TBL_444234_32(&c->st, 0) = (unsigned int)(t5 & U32MASK);
{ _pc = 28; continue; }
        case 27:
t0 = (t0 & ~0xFFU) | (((((t0 & 0xFFU) + 1) & 0xFFU)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
if (((t0 & 0xFFU) & 0xFFU) < 0x0DU) { _pc = 26; continue; }
            _pc = 28;
            continue;
        case 28:
t0 = 0U;
t3 = (uint32_t)D3U_TBL_444234_32(&c->st, 0);
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)t1)) & 0xFFU);
t0 &= 0xFFU;
if (!((t0 & U32MASK) < (t3 & U32MASK))) { _pc = 40; continue; }
t0 = (t0 & ~0xFFU) | (((var_48 & 0xFFU)) & 0xFFU);
D3U_FLAG_444EB1_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
t0 = (t0 & ~0xFF00U) | ((((var_2C & 0xFFU)) & 0xFFU) << 8);
t0 = (t0 & 0xFFFF00FFU) | ((((t0 >> 8) & 0xFFU) & 0xFFU) << 8);
if (((((t0 >> 8) & 0xFFU)) & 0xFFU) == 0U) { _pc = 29; continue; }
t0 = (t0 & ~0xFFU) | ((0U) & 0xFFU); { _pc = 30; continue; }
            _pc = 29;
            continue;
        case 29:
t0 = (t0 & ~0xFFU) | ((1U) & 0xFFU);
            _pc = 30;
            continue;
        case 30:
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
D3U_FLAG_444EB0_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
if (((t0 & 0xFFU) & 0xFFU) == 0U) { _pc = 40; continue; }
t2 = (uint32_t)D3U_TBL_444214_32(&c->st, 0);
if (!((t2 & U32MASK) <= 8U)) { _pc = 32; continue; }
t0 = 0U;
t4 = (uint32_t)M8((uint32_t)t1);
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t1 + 1))) & 0xFFU);
t0 &= 0xFFU;
t3 = (t4 + t0) & U32MASK;
if (t3 != t2) { _pc = 32; continue; }
t4 = (((int)t4) >> 1) & U32MASK;
t0 = (((int)(t0 & U32MASK)) >> 1) & U32MASK;
t4 = ((t4 << 2) & U32MASK);
t0 = (t0 + t4) & U32MASK;
t0 = (t0 + t0) & U32MASK;
t0 = (t0 + D3U_TBL_444128) & U32MASK;
t0 = (t0 & ~0xFFU) | (d3uc_decode_bit_update(c, (unsigned int)t0) & 0xFFU);
if (((t0 & 0xFFU) & 0xFFU) == 0U) { _pc = 31; continue; }
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t1 + 0x0E))) & 0xFFU);
t1 &= 0xFFFF00FFU;
t1 = (t1 & ~0xFFU) | ((0U) & 0xFFU); t1 &= 0xFFFFFF00U;
var_28 = (t0 & 0xFFU) & 0xFFU;
var_4C = ((t1 & 0xFFU)) & 0xFFU;
D3U_FLAG_444EB7_8(&c->st, 0) = (unsigned char)((t1 & 0xFFU) & 0xFFU);
{ _pc = 38; continue; }
        case 31:
t3 = (t3 & ~0xFF00U) | (((0x0DU) & 0xFFU) << 8);
t3 = (t3 & 0xFFFF00FFU) | (0x0DU << 8);
t0 = (t0 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
D3U_FLAG_444EB7_8(&c->st, 0) = (unsigned char)(((t3 >> 8) & 0xFFU) & 0xFFU);
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((t0 & 0xFFU) & 0xFFU) & 0xFFU);
t3 &= 0xFFU;
t0 = (t0 & ~0xFF00U) | ((((uint32_t)M8((uint32_t)(t1 + 0x0E))) & 0xFFU) << 8);
t0 = (t0 & 0xFFFF00FFU) | ((((t0 >> 8) & 0xFFU) & 0xFFU) << 8);
M8((uint32_t)(D3U_FLAG_444E82 + (t3 & 0xFFU))) = (unsigned char)(((t0 >> 8) & 0xFFU) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((((t0 & 0xFFU) + 1) & 0xFFU)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
{ _pc = 38; continue; }
        case 32:
t0 = (uint32_t)D3U_TBL_444234_32(&c->st, 0);
D3U_RC_TOTAL_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
t0 &= 0xFFFFU;
t4 = (t4 & 0xFFFF0000U) | (d3uc_rc_decode(c, (unsigned int)(t0 & 0xFFFFU)) & 0xFFFFU);
t3 = (t3 & ~0xFFU) | ((0U) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | 0U;
t0 = 0U;
D3U_FLAG_444EB7_8(&c->st, 0) = (unsigned char)((t3 & 0xFFU) & 0xFFU);
D3U_RC_PROB_LO_32(&c->st, 0) = 0;
            _pc = 33;
            continue;
        case 33:
t0 = 0U;
t0 = (t0 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EB7_8(&c->st, 0)) & 0xFFU);
t0 &= 0xFFU;
    if (((uint32_t)v68[t0 & 0xFFU]) == 0U) { _pc = 36; continue; }
t0 = (t0 + t1) & U32MASK;
if ((uint32_t)M8((uint32_t)(t0 + 1)) != 0U) { _pc = 34; continue; }
D3U_FLAG_444EB7_8(&c->st, 0) = 0x0D;
{ _pc = 37; continue; }
        case 34:
t2 = 0U;
t3 = (uint32_t)D3U_RC_PROB_LO_32(&c->st, 0);
t2 = (t2 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t0 + 1))) & 0xFFU);
t2 &= 0xFFU;
t3 = (t3 + t2) & U32MASK;
D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)(t3 & U32MASK);
t3 = 0U;
t2 = (uint32_t)D3U_RC_PROB_HI_32(&c->st, 0);
t3 = (t4 & 0xFFFFU);
t3 &= 0xFFFFU;
if (!((t3 & U32MASK) < (t2 & U32MASK))) { _pc = 35; continue; }
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t0 + 0x0E))) & 0xFFU);
t3 = (t3 & ~0xFFU) | ((0U) & 0xFFU);
var_28 = (t0 & 0xFFU) & 0xFFU;
var_4C = ((t3 & 0xFFU)) & 0xFFU;
{ _pc = 37; continue; }
        case 35:
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU);
t2 = 0U;
t2 = (t2 & ~0xFFU) | (((t3 & 0xFFU) & 0xFFU) & 0xFFU);
t2 &= 0xFFU;
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t0 + 0x0E))) & 0xFFU);
t0 &= 0xFFU;
t3 = (t3 & ~0xFFU) | (((((t3 & 0xFFU) + 1) & 0xFFU)) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU);
M8((uint32_t)(D3U_FLAG_444E82 + (t2 & 0xFFU))) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
t0 = (uint32_t)D3U_RC_PROB_HI_32(&c->st, 0);
D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)((t3 & 0xFFU) & 0xFFU);
D3U_RC_PROB_LO_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
            _pc = 36;
            continue;
        case 36:
t0 = (t0 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EB7_8(&c->st, 0)) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((((t0 & 0xFFU) + 1) & 0xFFU)) & 0xFFU);
D3U_FLAG_444EB7_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
if (((t0 & 0xFFU) & 0xFFU) < 0x0DU) { _pc = 33; continue; }
            _pc = 37;
            continue;
        case 37:
if ((uint32_t)D3U_FLAG_444EB7_8(&c->st, 0) != 0x0DU) { _pc = 38; continue; }
t0 = (uint32_t)D3U_RC_TOTAL_32(&c->st, 0);
D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
            _pc = 38;
            continue;
        case 38:
t0 = D3U_RC_TOTAL;
d3uc_rc_update(c, D3U_RC_TOTAL);
{ _pc = 40; continue; }
        case 39:
t5 = (uint32_t)D3U_TBL_444224_32(&c->st, 0);
t5 = (t5 + 0x8000U) & U32MASK;
t0 = (t0 + 0x100000U) & U32MASK;
D3U_TBL_444224_32(&c->st, 0) = (unsigned int)(t5 & U32MASK);
if ((t5 & U32MASK) < 0x40000U) { _pc = 22; continue; }
            _pc = 40;
            continue;
        case 40:
if ((var_4C & 0xFFU) == 0U) { _pc = 104; continue; }
if ((uint32_t)D3U_FLAG_444EA0_8(&c->st, 0) == 0U) { _pc = 41; continue; }
t0 = d3uc_lookup_model_entry(c);
{ _pc = 42; continue; }
        case 41:
t0 = 0U;
            _pc = 42;
            continue;
        case 42:
t2 = (t2 & ~0xFF00U) | (((1U) & 0xFFU) << 8);
t2 = (t2 & 0xFFFF00FFU) | (1U << 8);
var_50 = t0 & U32MASK;
t3 = (t3 & ~0xFFU) | ((0U) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | 0U;
var_30 = (((t2 >> 8) & 0xFFU)) & 0xFFU;
var_40 = ((t3 & 0xFFU)) & 0xFFU;
if (t0 == 0U) { _pc = 43; continue; }
D3U_TBL_44423C_32(&c->st, 0) = (unsigned int)(((uint32_t)D3U_TBL_44423C_32(&c->st, 0) + 2U) & U32MASK);
            _pc = 43;
            continue;
        case 43:
t5 = var_50 & U32MASK;
if (t5 == 0U) { _pc = 46; continue; }
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t5 + 1))) & 0xFFU);
t3 &= 0xFFU;
t1 = (t3 * 4U) & U32MASK;
t1 = (t1 + t3) & U32MASK;
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)M8((uint32_t)t5)) & 0xFFU);
t3 &= 0xFFU;
t0 = (t3 * 4U) & U32MASK;
t0 = (t0 - t3) & U32MASK;
if (!((int)(t0 & U32MASK) < (int)(t1 & U32MASK))) { _pc = 44; continue; }
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t5 + 3))) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((t0 & 0xFFU) & 0x7FU) & 0xFFU);
if (((t0 & 0xFFU) & 0xFFU) > 2U) { _pc = 44; continue; }
t0 = 1U;
{ _pc = 45; continue; }
        case 44:
t0 = 0U;
            _pc = 45;
            continue;
        case 45:
if (t0 == 0U) { _pc = 47; continue; }
            _pc = 46;
            continue;
        case 46:
t0 = (t0 & ~0xFFU) | ((1U) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
{ _pc = 48; continue; }
        case 47:
t0 = (t0 & ~0xFFU) | ((0U) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | 0U;
            _pc = 48;
            continue;
        case 48:
D3U_FLAG_444EAC_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
            _pc = 49;
            continue;
        case 49:
if ((uint32_t)D3U_FLAG_444EAC_8(&c->st, 0) == 0U) { _pc = 50; continue; }
{ _pc = 51; continue; }
        case 50:
if ((var_40 & 0xFFU) == 0U) { _pc = 70; continue; }
            _pc = 51;
            continue;
        case 51:
t1 = (t1 & ~0xFF00U) | (((0U) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | 0U;
t0 = (uint32_t)D3U_TBL_44424C_32(&c->st, 0);
var_30 = (((t1 >> 8) & 0xFFU)) & 0xFFU;
if (t0 == 0U) { _pc = 52; continue; }
if ((uint32_t)M8((uint32_t)(t6 + 0x11)) == 0U) { _pc = 52; continue; }
if (!((uint32_t)D3U_TBL_444208_32(&c->st, 0) < 0xFFFFU)) { _pc = 53; continue; }
t0 = (uint32_t)D3U_TBL_4441F8_32(&c->st, 0);
if ((uint32_t)d3u_ld32(c, (uint32_t)((uint32_t)t0)) > 0x3FEU) { _pc = 53; continue; }
            _pc = 52;
            continue;
        case 52:
t0 = 1U;
{ _pc = 54; continue; }
        case 53:
t0 = 0U;
            _pc = 54;
            continue;
        case 54:
if (t0 == 0U) { _pc = 55; continue; }
t0 = (t0 & ~0xFFU) | ((1U) & 0xFFU); { _pc = 56; continue; }
            _pc = 55;
            continue;
        case 55:
t0 = (t0 & ~0xFFU) | ((0U) & 0xFFU);
            _pc = 56;
            continue;
        case 56:
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
D3U_FLAG_444EAC_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
if (((t0 & 0xFFU) & 0xFFU) == 0U) { _pc = 70; continue; }
if ((uint32_t)M8((uint32_t)t6) == 0U) { _pc = 70; continue; }
t5 = 0U;
t0 = (t0 & ~0xFFU) | ((0U) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | 0U;
t4 = 0x4137FDU;
var_38 = (t0 & 0xFFU) & 0xFFU;
    { unsigned int _k; static const unsigned char _b8[8] = {0xA5,0xA5,0x88,0x44,0x24,0x24,0x8A,0x45};
      for (_k = 0; _k < 8; _k++) v58[_k] = _b8[_k]; }
var_44 = (t0 & 0xFFU) & 0xFFU;
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t6 + 0x12))) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
var_24 = (t0 & 0xFFU) & 0xFFU;
            _pc = 57;
            continue;
        case 57:
t0 = 0U;
t0 = (t0 & ~0xFFU) | (((var_38 & 0xFFU)) & 0xFFU);
t0 &= 0xFFU;
if ((uint32_t)M8((uint32_t)(t0 + t6 + 1)) == 0U) { _pc = 60; continue; }
t3 = (t3 & ~0xFF00U) | ((((var_44 & 0xFFU)) & 0xFFU) << 8);
t3 = (t3 & ~0xFF00U) | (((((((t3 >> 8) & 0xFFU) + 1) & 0xFFU)) & 0xFFU) << 8);
t0 = 0U;
var_44 = ((t3 >> 8) & 0xFFU) & 0xFFU;
            _pc = 58;
            continue;
        case 58:
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t3 &= 0xFFU;
if (!((t0 & U32MASK) < (t3 & U32MASK))) { _pc = 59; continue; }
t1 = 0U;
t1 = (t1 & ~0xFFU) | (((var_38 & 0xFFU)) & 0xFFU);
t1 &= 0xFFU;
t2 = t0 & U32MASK;
t3 = (t1 + t6) & U32MASK;
t2 = (t2 & ~0xFFU) | (((uint32_t)M8((uint32_t)(D3U_FLAG_444E82 + (t2 & 0xFFU)))) & 0xFFU);
t2 = (t2 & 0xFFFFFF00U) | ((t2 & 0xFFU) & 0xFFU);
t2 = (t2 & ~0xFF00U) | ((((uint32_t)M8((uint32_t)(t3 + 9))) & 0xFFU) << 8);
t2 = (t2 & 0xFFFF00FFU) | ((((t2 >> 8) & 0xFFU) & 0xFFU) << 8);
t0 = (t0 + 1) & U32MASK;
if ((((t2 & 0xFFU)) & 0xFFU) != ((((t2 >> 8) & 0xFFU)) & 0xFFU)) { _pc = 58; continue; }
t0 = (t0 & ~0xFFU) | ((0U) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | 0U;
if (t1 < 8U) v58[t1] = 0;
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t3 + 1))) & 0xFFU);
t0 &= 0xFFU;
var_24 = ((var_24 - ((t0 & 0xFFU) & 0xFFU)) & 0xFFU);
            _pc = 59;
            continue;
        case 59:
t3 = (t3 & ~0xFFU) | (((var_38 & 0xFFU)) & 0xFFU);
t3 = (t3 & ~0xFFU) | (((((t3 & 0xFFU) + 1) & 0xFFU)) & 0xFFU);
var_38 = (t3 & 0xFFU) & 0xFFU;
if (((t3 & 0xFFU) & 0xFFU) < 8U) { _pc = 57; continue; }
            _pc = 60;
            continue;
        case 60:
t1 = (t1 & ~0xFFU) | ((0xFFU) & 0xFFU);
t1 = (t1 & 0xFFFFFF00U) | 0xFFU;
t0 = 0U;
t3 = 0U;
t0 = (t0 & ~0xFFU) | (((var_24 & 0xFFU)) & 0xFFU);
t0 &= 0xFFU;
t3 = (t3 & ~0xFFU) | (((var_24 & 0xFFU)) & 0xFFU);
t3 &= 0xFFU;
D3U_TBL_444248_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
t0 = 0U;
D3U_FLAG_444EB8_8(&c->st, 0) = (unsigned char)((t1 & 0xFFU) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)t6)) & 0xFFU);
t0 &= 0xFFU;
t3 = ((t3 << 2) & U32MASK);
t1 = (t0 + t0 * 4U) & U32MASK;
if (!((t3 & U32MASK) > (t1 & U32MASK))) { _pc = 70; continue; }
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t6 + 0x11))) & 0xFFU);
t3 &= 0xFFU;
t2 = (uint32_t)D3U_TBL_44424C_32(&c->st, 0);
t3 = ((t3 << 2) & U32MASK);
t3 = (t3 + t2) & U32MASK;
if (!((t3 & U32MASK) < 0x10U)) { _pc = 70; continue; }
t3 = (t3 & ~0xFFU) | (((var_48 & 0xFFU)) & 0xFFU);
t1 = (t1 & ~0xFF00U) | ((((var_2C & 0xFFU)) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | ((((t1 >> 8) & 0xFFU) & 0xFFU) << 8);
D3U_FLAG_444EB5_8(&c->st, 0) = (unsigned char)((t3 & 0xFFU) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU);
if (((((t1 >> 8) & 0xFFU)) & 0xFFU) != 0U) { _pc = 70; continue; }
if (!((var_24 & 0xFFU) > 0x10U)) { _pc = 61; continue; }
{ _pc = 63; continue; }
        case 61:
t2 = (t2 & ~0xFF00U) | ((((var_44 & 0xFFU)) & 0xFFU) << 8);
if (((((t2 >> 8) & 0xFFU)) & 0xFFU) != 1U) { _pc = 63; continue; }
t0 = (((int)(t0 & U32MASK)) >> 2) & U32MASK;
t3 = (t0 * 4U) & U32MASK;
t0 = 0U;
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t6 + 1))) & 0xFFU);
t0 &= 0xFFU;
t0 = (((int)(t0 & U32MASK)) >> 2) & U32MASK;
t0 = (t0 + t3) & U32MASK;
t0 = (t0 + t0) & U32MASK;
t0 = (t0 + D3U_TBL_444108) & U32MASK;
t0 = (t0 & ~0xFFU) | (d3uc_decode_bit_update(c, (unsigned int)t0) & 0xFFU);
if (((t0 & 0xFFU) & 0xFFU) == 0U) { _pc = 62; continue; }
var_4C = (((t1 >> 8) & 0xFFU)) & 0xFFU;
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t6 + 9))) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
D3U_FLAG_444EB8_8(&c->st, 0) = (unsigned char)(((t1 >> 8) & 0xFFU) & 0xFFU);
var_28 = (t0 & 0xFFU) & 0xFFU;
{ _pc = 69; continue; }
        case 62:
t0 = (t0 & ~0xFF00U) | (((8U) & 0xFFU) << 8);
t0 = (t0 & 0xFFFF00FFU) | (8U << 8);
t3 = 0U;
D3U_FLAG_444EB8_8(&c->st, 0) = (unsigned char)(((t0 >> 8) & 0xFFU) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
t3 = (t3 & ~0xFFU) | (((t0 & 0xFFU) & 0xFFU) & 0xFFU);
t3 &= 0xFFU;
t0 = (t0 & ~0xFF00U) | ((((uint32_t)M8((uint32_t)(t6 + 9))) & 0xFFU) << 8);
t0 = (t0 & 0xFFFF00FFU) | ((((t0 >> 8) & 0xFFU) & 0xFFU) << 8);
M8((uint32_t)(D3U_FLAG_444E82 + (t3 & 0xFFU))) = (unsigned char)(((t0 >> 8) & 0xFFU) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((((t0 & 0xFFU) + (((t2 >> 8) & 0xFFU) & 0xFFU)) & 0xFFU)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
{ _pc = 69; continue; }
        case 63:
t0 = 0U;
t0 = (t0 & ~0xFFU) | (((var_24 & 0xFFU)) & 0xFFU);
t0 &= 0xFFU;
D3U_RC_TOTAL_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
t0 = 0U;
t0 = (t0 & ~0xFFU) | (((var_24 & 0xFFU)) & 0xFFU);
t0 &= 0xFFU;
t4 = 0U;
t2 = (t2 & 0xFFFF0000U) | (d3uc_rc_decode(c, (unsigned int)(t0 & 0xFFFFU)) & 0xFFFFU);
t0 = (t0 & ~0xFFU) | ((0U) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | 0U;
D3U_RC_PROB_LO_32(&c->st, 0) = (unsigned int)(t4 & U32MASK);
D3U_FLAG_444EB8_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
            _pc = 64;
            continue;
        case 64:
t0 = 0U;
t0 = (t0 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EB8_8(&c->st, 0)) & 0xFFU);
t0 &= 0xFFU;
    if (((uint32_t)v58[t0 & 0xFFU]) == 0U) { _pc = 67; continue; }
t0 = (t0 + t6) & U32MASK;
if ((uint32_t)M8((uint32_t)(t0 + 1)) != 0U) { _pc = 65; continue; }
D3U_FLAG_444EB8_8(&c->st, 0) = 8;
{ _pc = 68; continue; }
        case 65:
t1 = 0U;
t3 = (uint32_t)D3U_RC_PROB_LO_32(&c->st, 0);
t1 = (t1 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t0 + 1))) & 0xFFU);
t1 &= 0xFFU;
t3 = (t3 + t1) & U32MASK;
D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)(t3 & U32MASK);
t3 = 0U;
t5 = (uint32_t)D3U_RC_PROB_HI_32(&c->st, 0);
t3 = (t2 & 0xFFFFU);
t3 &= 0xFFFFU;
if (!((t3 & U32MASK) < (t5 & U32MASK))) { _pc = 66; continue; }
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t0 + 9))) & 0xFFU);
t0 &= 0xFFU; /* lane-keep al (dropped spurious t3 zeroing: no native insn between and ; it broke xor dl,cl) */
t3 = (t3 & ~0xFFU) | (((((t3 & 0xFFU) ^ (t2 & 0xFFU)) & 0xFFU)) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU);
var_28 = (t0 & 0xFFU) & 0xFFU;
var_4C = ((t3 & 0xFFU)) & 0xFFU;
{ _pc = 68; continue; }
        case 66:
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU);
t1 = 0U;
D3U_RC_PROB_LO_32(&c->st, 0) = (unsigned int)(t5 & U32MASK);
t1 = (t1 & ~0xFFU) | (((t3 & 0xFFU) & 0xFFU) & 0xFFU);
t1 &= 0xFFU;
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t0 + 9))) & 0xFFU);
t0 &= 0xFFU;
t3 = (t3 & ~0xFFU) | (((((t3 & 0xFFU) + 1) & 0xFFU)) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU);
M8((uint32_t)(D3U_FLAG_444E82 + (t1 & 0xFFU))) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)((t3 & 0xFFU) & 0xFFU);
            _pc = 67;
            continue;
        case 67:
t0 = (t0 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EB8_8(&c->st, 0)) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((((t0 & 0xFFU) + 1) & 0xFFU)) & 0xFFU);
D3U_FLAG_444EB8_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
if (((t0 & 0xFFU) & 0xFFU) < 8U) { _pc = 64; continue; }
            _pc = 68;
            continue;
        case 68:
if ((uint32_t)D3U_FLAG_444EB8_8(&c->st, 0) != 8U) { _pc = 69; continue; }
t0 = (uint32_t)D3U_RC_TOTAL_32(&c->st, 0);
D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
            _pc = 69;
            continue;
        case 69:
t0 = D3U_RC_TOTAL;
d3uc_rc_update(c, D3U_RC_TOTAL);
            _pc = 70;
            continue;
        case 70:
t1 = (t1 & ~0xFF00U) | ((((var_40 & 0xFFU)) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | ((((t1 >> 8) & 0xFFU) & 0xFFU) << 8);
if (((((t1 >> 8) & 0xFFU)) & 0xFFU) != 0U) { _pc = 81; continue; }
t0 = (t0 & ~0xFFU) | (((var_4C & 0xFFU)) & 0xFFU);
D3U_FLAG_444EA1_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
if (((t0 & 0xFFU) & 0xFFU) == 0U) { _pc = 80; continue; }
if ((uint32_t)D3U_FLAG_444EA0_8(&c->st, 0) != 0U) { _pc = 71; continue; }
D3U_FLAG_444EA1_8(&c->st, 0) = (unsigned char)(((t1 >> 8) & 0xFFU) & 0xFFU);
            _pc = 71;
            continue;
        case 71:
var_40 = 1U;
if (var_50 == 0U) { _pc = 80; continue; }
t3 = (uint32_t)D3U_TBL_44423C_32(&c->st, 0);
t1 = var_50 & U32MASK;
t3 = ((t3 >> 2) & U32MASK);
t0 = 0U;
D3U_TBL_44423C_32(&c->st, 0) = (unsigned int)(t3 & U32MASK);
            _pc = 72;
            continue;
        case 72:
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t3 &= 0xFFU;
if (!((t0 & U32MASK) < (t3 & U32MASK))) { _pc = 73; continue; }
t3 = t0 & U32MASK;
t3 = (t3 & ~0xFFU) | (((uint32_t)M8((uint32_t)(D3U_FLAG_444E82 + (t3 & 0xFFU)))) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU);
t3 = (t3 & ~0xFF00U) | ((((uint32_t)M8((uint32_t)(t1 + 2))) & 0xFFU) << 8);
t3 = (t3 & 0xFFFF00FFU) | ((((t3 >> 8) & 0xFFU) & 0xFFU) << 8);
t0 = (t0 + 1) & U32MASK;
if ((((t3 & 0xFFU)) & 0xFFU) != ((((t3 >> 8) & 0xFFU)) & 0xFFU)) { _pc = 72; continue; }
t1 = (t1 & ~0xFFU) | ((0U) & 0xFFU);
t1 = (t1 & 0xFFFFFF00U) | 0U;
D3U_FLAG_444EA1_8(&c->st, 0) = (unsigned char)((t1 & 0xFFU) & 0xFFU);
            _pc = 73;
            continue;
        case 73:
if ((uint32_t)D3U_FLAG_444EA1_8(&c->st, 0) == 0U) { _pc = 80; continue; }
t0 = (t0 & ~0xFFU) | (((var_48 & 0xFFU)) & 0xFFU);
t2 = (t2 & ~0xFFU) | (((var_2C & 0xFFU)) & 0xFFU);
D3U_FLAG_444EB2_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
if ((((t2 & 0xFFU)) & 0xFFU) != 0U) { _pc = 80; continue; }
t1 = (uint32_t)D3U_TBL_44427C_32(&c->st, 0);
if (t1 == 0U) { _pc = 74; continue; }
t3 = (t3 & ~0xFFU) | (((uint32_t)M8((uint32_t)t1)) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t1 + 1))) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU); t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
{ _pc = 75; continue; }
        case 74:
t3 = var_50 & U32MASK;
t0 = var_50 & U32MASK;
t3 = (t3 & ~0xFFU) | (((uint32_t)M8((uint32_t)t3)) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)t0 + 1)) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU); t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
            _pc = 75;
            continue;
        case 75:
t1 = 0U;
t1 = (t1 & ~0xFFU) | ((((t3 & 0xFFU) & 0xFFU)) & 0xFFU);
t1 &= 0xFFU;
t0 &= 0xFFU;
t2 = (t1 + t0) & U32MASK;
D3U_RC_TOTAL_32(&c->st, 0) = (unsigned int)(t2 & U32MASK);
t0 = 0U;
t0 = (D3U_RC_TOTAL_32(&c->st, 0) & 0xFFFFU);
t0 = d3uc_rc_decode(c, (unsigned int)(t0 & 0xFFFFU));
t0 &= 0xFFFFU;
if (!((t0 & U32MASK) < (t1 & U32MASK))) { _pc = 77; continue; }
if ((uint32_t)D3U_FLAG_444EA4_8(&c->st, 0) == 0U) { _pc = 76; continue; }
t0 = (uint32_t)D3U_TBL_4441F8_32(&c->st, 0);
t1 = (uint32_t)D3U_TBL_444208_32(&c->st, 0);
t4 = (uint32_t)d3u_ld32(c, (uint32_t)((uint32_t)t0));
t4 = (t4 + t1) & U32MASK;
t5 = (t1 + t1) & U32MASK;
d3u_st32(c, (uint32_t)((uint32_t)t0), (unsigned int)(t4 & U32MASK));
D3U_TBL_444208_32(&c->st, 0) = (unsigned int)(t5 & U32MASK);
if (!((t5 & U32MASK) > 0xFFFFU)) { _pc = 76; continue; }
D3U_TBL_444208_32(&c->st, 0) = 0xFFFF;
            _pc = 76;
            continue;
        case 76:
t0 = 0U;
t0 = (t0 & ~0xFFU) | ((((t3 & 0xFFU) & 0xFFU)) & 0xFFU);
t0 &= 0xFFU;
D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
t0 = var_50 & U32MASK;
t0 = (t0 & ~0xFFU) | (((uint32_t)M8((uint32_t)(t0 + 2))) & 0xFFU);
t0 &= 0xFFU;
t2 = 0U;
var_28 = (t0 & 0xFFU) & 0xFFU;
t0 = (t0 & ~0xFF00U) | (((0U) & 0xFFU) << 8);
t0 = (t0 & 0xFFFF00FFU) | 0U;
D3U_RC_PROB_LO_32(&c->st, 0) = 0;
var_4C = (((t0 >> 8) & 0xFFU)) & 0xFFU;
{ _pc = 79; continue; }
        case 77:
if ((uint32_t)D3U_FLAG_444EA4_8(&c->st, 0) == 0U) { _pc = 78; continue; }
t0 = (uint32_t)D3U_TBL_4441F8_32(&c->st, 0);
d3u_st32(c, (uint32_t)((uint32_t)t0), (unsigned int)(((uint32_t)d3u_ld32(c, (uint32_t)((uint32_t)t0)) >> 1) & U32MASK));
t0 = (uint32_t)D3U_TBL_444208_32(&c->st, 0);
t0 = ((t0 >> 1) & U32MASK);
t0 = (t0 + 0xFFU) & U32MASK;
D3U_TBL_444208_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
            _pc = 78;
            continue;
        case 78:
t0 = 0U;
t0 = (t0 & ~0xFFU) | ((((t3 & 0xFFU) & 0xFFU)) & 0xFFU);
t0 &= 0xFFU;
D3U_RC_PROB_LO_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
t0 = (uint32_t)D3U_RC_TOTAL_32(&c->st, 0);
t3 = 0U;
D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
t0 = (t0 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
t1 = var_50 & U32MASK;
t3 = (t3 & ~0xFFU) | (((t0 & 0xFFU) & 0xFFU) & 0xFFU);
t3 &= 0xFFU;
t0 = (t0 & ~0xFF00U) | ((((uint32_t)M8((uint32_t)(t1 + 2))) & 0xFFU) << 8);
t0 = (t0 & 0xFFFF00FFU) | ((((t0 >> 8) & 0xFFU) & 0xFFU) << 8);
M8((uint32_t)(D3U_FLAG_444E82 + (t3 & 0xFFU))) = (unsigned char)(((t0 >> 8) & 0xFFU) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((((t0 & 0xFFU) + 1) & 0xFFU)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
            _pc = 79;
            continue;
        case 79:
t0 = D3U_RC_TOTAL;
d3uc_rc_update(c, D3U_RC_TOTAL);
            _pc = 80;
            continue;
        case 80:
if ((var_4C & 0xFFU) == 0U) { _pc = 81; continue; }
if ((var_30 & 0xFFU) != 0U) { _pc = 49; continue; }
{ _pc = 81; continue; }
        case 81:
if ((var_4C & 0xFFU) == 0U) { _pc = 104; continue; }
t2 = 0x201U;
t4 = 0x300U;
t0 = (t0 & ~0xFFU) | (((var_48 & 0xFFU)) & 0xFFU);
t1 = (t1 & ~0xFF00U) | ((((uint32_t)D3U_FLAG_444EA0_8(&c->st, 0)) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | ((((t1 >> 8) & 0xFFU) & 0xFFU) << 8);
D3U_FLAG_444EAE_8(&c->st, 0) = (unsigned char)((t0 & 0xFFU) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
if (((((t1 >> 8) & 0xFFU)) & 0xFFU) == 0U) { _pc = 82; continue; }
if (!((uint32_t)D3U_FLAG_444EBA_8(&c->st, 0) > 1U)) { _pc = 82; continue; }
t0 = (uint32_t)D3U_TBL_4441F8_32(&c->st, 0);
if ((uint32_t)d3u_ld32(c, (uint32_t)((uint32_t)t0)) == 0U) { _pc = 82; continue; }
t0 = 0U;
t3 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
t0 = (t0 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EBA_8(&c->st, 0)) & 0xFFU);
t0 &= 0xFFU;
t6 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
t3 = (t3 - t0) & U32MASK;
t0 = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
t3 &= t6;
t1 = (t1 & ~0xFFU) | (((uint32_t)c->blk00_dict[((uint32_t)(t3 + t0)) - c->blk00_base]) & 0xFFU);
t1 = (t1 & 0xFFFFFF00U) | ((t1 & 0xFFU) & 0xFFU);
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EBA_8(&c->st, 0)) & 0xFFU);
t3 &= 0xFFU;
t5 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
t3 = (t3 + t3) & U32MASK;
t5 = (t5 - t3) & U32MASK;
t3 = t5 & U32MASK;
t3 &= (t6 & U32MASK);
t6 = ((t1 & 0xFFU) & 0xFFU);
t1 = (t1 & ~0xFF00U) | ((((uint32_t)c->blk00_dict[((uint32_t)(t3 + t0)) - c->blk00_base]) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | ((((t1 >> 8) & 0xFFU) & 0xFFU) << 8);
t6 = (t6 + 0x100U) & U32MASK;
if ((((t1 & 0xFFU)) & 0xFFU) != ((((t1 >> 8) & 0xFFU)) & 0xFFU)) { _pc = 85; continue; }
t2 = 0x202U;
{ _pc = 85; continue; }
        case 82:
t0 = (uint32_t)D3U_TBL_444240_32(&c->st, 0);
t1 = (t1 & ~0xFFU) | (((t0 & 0xFFU)) & 0xFFU);
t1 = (t1 & 0xFFFFFF00U) | ((t1 & 0xFFU) & 0xFFU);
t0 = ((t0 >> 8) & U32MASK);
t6 = ((t1 & 0xFFU) & 0xFFU);
if ((((t1 & 0xFFU)) & 0xFFU) == ((t0) & 0xFFU)) { _pc = 83; continue; }
{ _pc = 84; continue; }
        case 83:
t2 = (t2 + 1) & U32MASK;
            _pc = 84;
            continue;
        case 84:
if ((uint32_t)D3U_FLAG_444EA0_8(&c->st, 0) != 0U) { _pc = 85; continue; }
t0 = (uint32_t)D3U_TBL_444240_32(&c->st, 0);
t0 = ((t0 >> 8) & U32MASK);
t0 &= 0xFFU;
if ((uint32_t)d3u_t42C6A6[(unsigned int)(t0 & 0xFFU)] == 0U) { _pc = 85; continue; }
t6 = (t6 + 0x100U) & U32MASK;
            _pc = 85;
            continue;
        case 85:
if ((var_3C & 0xFFU) == 0U) { _pc = 86; continue; }
t2 = (uint32_t)D3U_TBL_4441C4_32(&c->st, 0);
t4 = (uint32_t)D3U_FLAG_444EA9_8(&c->st, 0);
t1 = (t1 & ~0xFFU) | (((uint32_t)D3U_FLAG_444E9D_8(&c->st, 0)) & 0xFFU);
t1 = (t1 & 0xFFFFFF00U) | ((t1 & 0xFFU) & 0xFFU);
t4 = (t4 + 0x300U) & U32MASK;
            _pc = 86;
            continue;
        case 86:
if ((var_3C & 0xFFU) == 0U) { _pc = 87; continue; }
if ((uint32_t)D3U_FLAG_444EA8_8(&c->st, 0) == 0U) { _pc = 87; continue; }
t1 = (t1 & ~0xFF00U) | (((1U) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | (1U << 8);
{ _pc = 88; continue; }
        case 87:
t1 = (t1 & ~0xFF00U) | (((0U) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | 0U;
            _pc = 88;
            continue;
        case 88:
t3 = (t3 & ~0xFF00U) | ((((var_1C & 0xFFU)) & 0xFFU) << 8);
var_20 = (((t1 >> 8) & 0xFFU)) & 0xFFU;
if (((((t3 >> 8) & 0xFFU)) & 0xFFU) == 0U) { _pc = 97; continue; }
t0 = 0U;
if (((((t1 >> 8) & 0xFFU)) & 0xFFU) == 0U) { _pc = 90; continue; }
            _pc = 89;
            continue;
        case 89:
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t3 &= 0xFFU;
if (!((t0 & U32MASK) < (t3 & U32MASK))) { _pc = 91; continue; }
t3 = (t3 & ~0xFFU) | ((((t1 & 0xFFU) & 0xFFU)) & 0xFFU);
t3 = (t3 & ~0xFF00U) | ((((uint32_t)M8((uint32_t)(D3U_FLAG_444E82 + (t0 & 0xFFU)))) & 0xFFU) << 8);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU); t3 = (t3 & 0xFFFF00FFU) | ((((t3 >> 8) & 0xFFU) & 0xFFU) << 8);
t0 = (t0 + 1) & U32MASK;
t3 = (t3 & ~0xFFU) | (((((t3 & 0xFFU) - ((t3 >> 8) & 0xFFU)) & 0xFFU)) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU);
M8((uint32_t)(D3U_FLAG_444E81 + (t0 & 0xFFU))) = (unsigned char)((t3 & 0xFFU) & 0xFFU);
{ _pc = 89; continue; }
        case 90:
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t3 &= 0xFFU;
if (!((t0 & U32MASK) < (t3 & U32MASK))) { _pc = 91; continue; }
t3 = t0 & U32MASK;
t1 = (t1 & ~0xFF00U) | ((((uint32_t)M8((uint32_t)(D3U_FLAG_444E82 + (t3 & 0xFFU)))) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | ((((t1 >> 8) & 0xFFU) & 0xFFU) << 8);
t1 = (t1 & ~0xFF00U) | (((((((t1 >> 8) & 0xFFU) - (t1 & 0xFFU)) & 0xFFU)) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | ((((t1 >> 8) & 0xFFU) & 0xFFU) << 8);
t0 = (t0 + 1) & U32MASK;
M8((uint32_t)(D3U_FLAG_444E82 + (t3 & 0xFFU))) = (unsigned char)(((t1 >> 8) & 0xFFU) & 0xFFU);
{ _pc = 90; continue; }
        case 91:
t3 = t4 & U32MASK;
t0 = t2 & U32MASK;
t1 = (t1 & ~0xFF00U) | ((((var_20 & 0xFFU)) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | ((((t1 >> 8) & 0xFFU) & 0xFFU) << 8);
t0 = (t0 & ~0xFFU) | (d3uc_decode_offset_ctx(c, (unsigned int)t0, (unsigned int)t3) & 0xFFU);
if (((((t1 >> 8) & 0xFFU)) & 0xFFU) == 0U) { _pc = 92; continue; }
t0 = (t0 & ~0xFF00U) | (((((t1 & 0xFFU) & 0xFFU)) & 0xFFU) << 8);
t0 = (t0 & ~0xFF00U) | (((((((t0 >> 8) & 0xFFU) - ((t0 & 0xFFU) & 0xFFU)) & 0xFFU)) & 0xFFU) << 8);
t0 = (t0 & 0xFFFF00FFU) | ((((t0 >> 8) & 0xFFU) & 0xFFU) << 8);
var_28 = ((t0 >> 8) & 0xFFU) & 0xFFU;
{ _pc = 93; continue; }
        case 92:
t0 = (t0 & ~0xFFU) | (((((t0 & 0xFFU) + (t1 & 0xFFU)) & 0xFFU)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
var_28 = (t0 & 0xFFU) & 0xFFU;
            _pc = 93;
            continue;
        case 93:
t2 = (t2 & ~0xFFU) | (((var_20 & 0xFFU)) & 0xFFU);
t2 = (t2 & 0xFFFFFF00U) | ((t2 & 0xFFU) & 0xFFU);
t0 = 0U;
if ((((t2 & 0xFFU)) & 0xFFU) == 0U) { _pc = 95; continue; }
            _pc = 94;
            continue;
        case 94:
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t3 &= 0xFFU;
if (!((t0 & U32MASK) < (t3 & U32MASK))) { _pc = 96; continue; }
t3 = (t3 & ~0xFFU) | ((((t1 & 0xFFU) & 0xFFU)) & 0xFFU);
t3 = (t3 & ~0xFF00U) | ((((uint32_t)M8((uint32_t)(D3U_FLAG_444E82 + (t0 & 0xFFU)))) & 0xFFU) << 8);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU); t3 = (t3 & 0xFFFF00FFU) | ((((t3 >> 8) & 0xFFU) & 0xFFU) << 8);
t0 = (t0 + 1) & U32MASK;
t3 = (t3 & ~0xFFU) | (((((t3 & 0xFFU) - ((t3 >> 8) & 0xFFU)) & 0xFFU)) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU);
M8((uint32_t)(D3U_FLAG_444E81 + (t0 & 0xFFU))) = (unsigned char)((t3 & 0xFFU) & 0xFFU);
{ _pc = 94; continue; }
        case 95:
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t3 &= 0xFFU;
if (!((t0 & U32MASK) < (t3 & U32MASK))) { _pc = 96; continue; }
t3 = t0 & U32MASK;
t2 = (t2 & ~0xFF00U) | ((((uint32_t)M8((uint32_t)(D3U_FLAG_444E82 + (t3 & 0xFFU)))) & 0xFFU) << 8);
t2 = (t2 & 0xFFFF00FFU) | ((((t2 >> 8) & 0xFFU) & 0xFFU) << 8);
t2 = (t2 & ~0xFF00U) | (((((((t2 >> 8) & 0xFFU) + (t1 & 0xFFU)) & 0xFFU)) & 0xFFU) << 8);
t2 = (t2 & 0xFFFF00FFU) | ((((t2 >> 8) & 0xFFU) & 0xFFU) << 8);
t0 = (t0 + 1) & U32MASK;
M8((uint32_t)(D3U_FLAG_444E82 + (t3 & 0xFFU))) = (unsigned char)(((t2 >> 8) & 0xFFU) & 0xFFU);
{ _pc = 95; continue; }
        case 96:
t3 = 0x2FDU;
t1 = 0U;
t0 = t6 & U32MASK;
t1 = (t1 & ~0xFFU) | (((var_28 & 0xFFU)) & 0xFFU);
t1 = (t1 & 0xFFFFFF00U) | ((t1 & 0xFFU) & 0xFFU); /* lane-merge (split bl) */
t1 &= 0xFFU;
{ _pc = 103; continue; }
        case 97:
t3 = 0x2FDU;
t0 = t6 & U32MASK;
t0 = (t0 & ~0xFFU) | (d3uc_decode_offset_ctx(c, (unsigned int)t0, (unsigned int)t3) & 0xFFU);
var_28 = (t0 & 0xFFU) & 0xFFU;
if ((uint32_t)D3U_FLAG_444EA0_8(&c->st, 0) == 0U) { _pc = 104; continue; }
if (((((t1 >> 8) & 0xFFU)) & 0xFFU) == 0U) { _pc = 98; continue; }
t1 = (t1 & ~0xFF00U) | (((((t1 & 0xFFU) & 0xFFU)) & 0xFFU) << 8);
t1 = (t1 & ~0xFF00U) | (((((((t1 >> 8) & 0xFFU) - ((t0 & 0xFFU) & 0xFFU)) & 0xFFU)) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | ((((t1 >> 8) & 0xFFU) & 0xFFU) << 8);
{ _pc = 99; continue; }
        case 98:
t1 = (t1 & ~0xFF00U) | ((((t0 & 0xFFU) & 0xFFU) & 0xFFU) << 8);
t1 = (t1 & ~0xFF00U) | (((((((t1 >> 8) & 0xFFU) - ((t1 & 0xFFU) & 0xFFU)) & 0xFFU)) & 0xFFU) << 8);
t1 = (t1 & 0xFFFF00FFU) | ((((t1 >> 8) & 0xFFU) & 0xFFU) << 8);
            _pc = 99;
            continue;
        case 99:
t3 = (t3 & ~0xFFU) | (((var_20 & 0xFFU)) & 0xFFU);
t0 = 0U;
if ((((t3 & 0xFFU)) & 0xFFU) == 0U) { _pc = 101; continue; }
            _pc = 100;
            continue;
        case 100:
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t3 &= 0xFFU;
if (!((t0 & U32MASK) < (t3 & U32MASK))) { _pc = 102; continue; }
t3 = (t3 & ~0xFFU) | ((((t1 & 0xFFU) & 0xFFU)) & 0xFFU);
t3 = (t3 & ~0xFF00U) | ((((uint32_t)M8((uint32_t)(D3U_FLAG_444E82 + (t0 & 0xFFU)))) & 0xFFU) << 8);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU); t3 = (t3 & 0xFFFF00FFU) | ((((t3 >> 8) & 0xFFU) & 0xFFU) << 8);
t0 = (t0 + 1) & U32MASK;
t3 = (t3 & ~0xFFU) | (((((t3 & 0xFFU) - ((t3 >> 8) & 0xFFU)) & 0xFFU)) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU);
M8((uint32_t)(D3U_FLAG_444E81 + (t0 & 0xFFU))) = (unsigned char)((t3 & 0xFFU) & 0xFFU);
{ _pc = 100; continue; }
        case 101:
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0)) & 0xFFU);
t3 &= 0xFFU;
if (!((t0 & U32MASK) < (t3 & U32MASK))) { _pc = 102; continue; }
t3 = t0 & U32MASK;
t0 = (t0 + 1) & U32MASK;
tmp = (uint32_t)M8((uint32_t)(D3U_FLAG_444E82 + (t3 & 0xFFU)));
tmp = ((tmp - ((t1 & 0xFFU) & 0xFFU)) & 0xFFU);
M8((uint32_t)(D3U_FLAG_444E82 + (t3 & 0xFFU))) = (unsigned char)tmp;
{ _pc = 101; continue; }
        case 102:
t1 = (t1 & ~0xFFU) | (((((t1 >> 8) & 0xFFU) & 0xFFU)) & 0xFFU);
t1 = (t1 & 0xFFFFFF00U) | ((t1 & 0xFFU) & 0xFFU);
t1 &= 0xFFU;
t3 = t4 & U32MASK;
t0 = t2 & U32MASK;
            _pc = 103;
            continue;
        case 103:
d3uc_update_ctx_counter(c, (unsigned int)t0, (unsigned int)t3, (unsigned int)t1);
            _pc = 104;
            continue;
        case 104:
t3 = 0U;
t3 = (t3 & ~0xFFU) | (((var_28 & 0xFFU)) & 0xFFU);
t3 = (t3 & 0xFFFFFF00U) | ((t3 & 0xFFU) & 0xFFU); /* lane-merge (split dl) */
t3 &= 0xFFU;
t0 = t3 & U32MASK;
d3uc_update_order_model_a(c, (unsigned int)t0);
if ((uint32_t)D3U_FLAG_444EAC_8(&c->st, 0) == 0U) { _pc = 105; continue; }
t0 = t3 & U32MASK;
d3uc_update_order_model_b(c, (unsigned int)t0);
            _pc = 105;
            continue;
        case 105:
if ((uint32_t)D3U_FLAG_444EA1_8(&c->st, 0) == 0U) { _pc = 106; continue; }
t0 = 0U;
t0 = (t0 & ~0xFFU) | (((var_28 & 0xFFU)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU); /* lane-merge (split al) */
t0 &= 0xFFU;
d3uc_update_model_8way(c, (unsigned int)t0);
            _pc = 106;
            continue;
        case 106:
if ((var_48 & 0xFFU) == 0U) { _pc = 109; continue; }
t0 = (t0 & ~0xFFU) | (((var_28 & 0xFFU)) & 0xFFU);
t0 = (t0 & ~0xFFU) | (((((t0 & 0xFFU) - (uint32_t)D3U_FLAG_444E9D_8(&c->st, 0)) & 0xFFU)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
if ((uint32_t)D3U_FLAG_444EA8_8(&c->st, 0) == 0U) { _pc = 107; continue; }
t0 = (t0 & ~0xFFU) | ((((0U - (t0 & 0xFFU)) & 0xFFU)) & 0xFFU);
t0 = (t0 & 0xFFFFFF00U) | ((t0 & 0xFFU) & 0xFFU);
            _pc = 107;
            continue;
        case 107:
t3 = (t3 & ~0xFFU) | ((0U) & 0xFFU);
t1 = 0U;
D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)((t3 & 0xFFU) & 0xFFU);
t3 = 0U;
t1 = (t1 & ~0xFFU) | (((t0 & 0xFFU) & 0xFFU) & 0xFFU);
t1 &= 0xFFU;
t3 = (t3 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EA9_8(&c->st, 0)) & 0xFFU);
t3 &= 0xFFU;
t0 = (uint32_t)D3U_TBL_4441C4_32(&c->st, 0);
t3 = (t3 + 0x300U) & U32MASK;
d3uc_update_ctx_ema(c, (unsigned int)t3, (unsigned int)t1);
if ((uint32_t)D3U_FLAG_444EAE_8(&c->st, 0) == 0U) { _pc = 108; continue; }
t0 = (uint32_t)D3U_TBL_44420C_32(&c->st, 0);
D3U_TBL_444204_32(&c->st, 0) = (unsigned int)(((uint32_t)D3U_TBL_444204_32(&c->st, 0) + t0) & U32MASK);
            _pc = 108;
            continue;
        case 108:
t3 = (uint32_t)D3U_TBL_444200_32(&c->st, 0);
t0 = t3 & U32MASK;
t0 = ((t0 << 5) & U32MASK);
t0 = (t0 - t3) & U32MASK;
t3 = t0 & U32MASK;
t0 = (uint32_t)D3U_TBL_444204_32(&c->st, 0);
t3 = ((t3 >> 5) & U32MASK);
t3 = (t3 + t0) & U32MASK;
D3U_TBL_444200_32(&c->st, 0) = (unsigned int)(t3 & U32MASK);
            _pc = 109;
            continue;
        case 109:
t1 = (t1 & ~0xFFU) | (((uint32_t)D3U_FLAG_444EB3_8(&c->st, 0)) & 0xFFU);
t1 = (t1 & ~0xFFU) | (((((t1 & 0xFFU) - 1) & 0xFFU)) & 0xFFU);
D3U_FLAG_444EB3_8(&c->st, 0) = (unsigned char)((t1 & 0xFFU) & 0xFFU);
if ((((t1 & 0xFFU)) & 0xFFU) == 0xFFU) { _pc = 110; continue; }
t0 = 0U;
t0 = (t0 & ~0xFFU) | (((t1 & 0xFFU) & 0xFFU) & 0xFFU);
t0 &= 0xFFU;
t1 = (uint32_t)d3u_ld32(c, (uint32_t)((uint32_t)(D3U_TBL_441F08 + (t0 & 0xFFU) * 4U)));
D3U_TBL_44421C_32(&c->st, 0) = (unsigned int)(t1 & U32MASK);
t1 = 0U;
t1 = (t1 & ~0xFFU) | (((uint32_t)M8((uint32_t)(D3U_TBL_4441D8 + (t0 & 0xFFU)))) & 0xFFU);
t1 &= 0xFFU;
t3 = t1 & U32MASK;
t0 = (uint32_t)d3u_ld32(c, (uint32_t)((uint32_t)(D3U_TBL_442018 + (t0 & 0xFFU) * 4U)));
d3uc_ctx_model_update(c, (unsigned int)(t0 & U32MASK), (unsigned int)(t3 & U32MASK));
{ _pc = 109; continue; }
        case 110:
t1 = 0U;
t1 = (t1 & ~0xFFU) | (((var_28 & 0xFFU)) & 0xFFU);
t1 = (t1 & 0xFFFFFF00U) | ((t1 & 0xFFU) & 0xFFU); /* lane-merge (split bl) */
t1 &= 0xFFU;
t0 = t1 & U32MASK;
t3 = 1U;
d3uc_emit_literal(c, (unsigned int)(t0 & U32MASK));
t0 = t1 & U32MASK;
d3uc_update_match_chain(c, (unsigned int)(t0 & U32MASK), 1);
t3 = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
t0 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
c->blk00_dict[((uint32_t)(t3 + (t0 & U32MASK))) - c->blk00_base] = (unsigned char)((t1 & 0xFFU) & 0xFFU);
if (!((t0 & U32MASK) < 0x100U)) { _pc = 111; continue; }
t0 = (t0 + (uint32_t)D3U_DICT_SIZE_32(&c->st, 0)) & U32MASK;
t3 = (t3 + t0) & U32MASK;
t0 = (t0 & ~0xFFU) | (((var_28 & 0xFFU)) & 0xFFU);
c->blk00_dict[((uint32_t)t3) - c->blk00_base] = (unsigned char)((t0 & 0xFFU) & 0xFFU);
            _pc = 111;
            continue;
        case 111:
t0 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
t3 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
t0 = (t0 + 1) & U32MASK;
t0 &= t3;
D3U_DICT_CURSOR_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
        default:
            break;
        }
        break;
    }
}

/* ---- length/offset coders (from draft_l3.c) ---- */
/* sub_4164B8: entry al=sym live (dl=al, dl=~al). Escape flag byte_444EA7.
 * Tables: 42C492 (mapped bytes), 42C438 (class), 42BDDC (dword ptrs),
 * 42BF6C (lengths). Calls sub_41E3BC. Void return.
 */
void d3uc_emit_literal(dec3w_ctx *c, unsigned int sym)
{
    uint32_t t0 = 0;
    uint32_t t1 = 0;
    uint32_t t2 = 0;
    uint32_t t3 = 0;
    uint32_t t4 = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t3 = (t3 & ~0xFFU) | (sym & 0xFFU);
    t0 = (sym & 0xFFU);
    t0 = (t0 & ~0xFF00U) | (((uint32_t)D3U_FLAG_444EA7_8(&c->st, 0) & 0xFFU) << 8);
    t3 = (t3 & ~0xFFU) | ((~(t3 & 0xFFU)) & 0xFFU);
    if (((t0 >> 8) & 0xFFU) != 1U) { _pc = 4; continue; }
    t0 = (uint32_t)D3U_OUT_TOTAL_POS_32(&c->st, 0);
    t3 = t3 & ~0xFF00U;
    t4 = (uint32_t)D3U_TBL_444228_32(&c->st, 0);
    D3U_FLAG_444EA7_8(&c->st, 0) = (unsigned char)((t3 >> 8) & 0xFFU);
    if (t0 < t4) { _pc = 3; continue; }
    if ((t3 & 0xFFU) != 0xFEU) { _pc = 1; continue; }
    t0 = 0xFEU;
    d3uc_put_byte(c, (unsigned int)(t0 & U32MASK));
    /* retn */
    return;
        case 1:
    t0 = 0;
    d3uc_put_byte(c, (unsigned int)(t0 & U32MASK));
    t0 = 0;
    d3uc_put_byte(c, (unsigned int)(t0 & U32MASK));
    t0 = 3;
    d3uc_put_byte(c, (unsigned int)(t0 & U32MASK));
    t0 = 3;
    d3uc_put_byte(c, (unsigned int)(t0 & U32MASK));
            _pc = 2;
            continue;
        case 2:
    t0 = 0;
    t0 = (t0 & ~0xFFU) | (t3 & 0xFFU);
    d3uc_put_byte(c, (unsigned int)(t0 & U32MASK));
    /* retn */
    return;
        case 3:
    t1 = 0;
    t1 = (t1 & ~0xFFU) | ((uint32_t)D3U_FLAG_444EAD_8(&c->st, 0) & 0xFFU);
    t1 = (t1 - 0xA6U) & U32MASK;
    t0 = (t1 * 4U) & U32MASK;
    t0 = (t0 - t1) & U32MASK;
    t0 = (t0 << 2) & U32MASK;
    t0 = (t0 - t1) & U32MASK;
    t1 = t0;
    t0 = 0;
    t0 = (t0 & ~0xFFU) | (t3 & 0xFFU);
    t0 = (t0 & ~0xFFU) | ((uint32_t)d3u_t42BD70[(1826U + t1 + (t0 & 0xFFU)) & U32MASK] & 0xFFU);
    t0 &= 0xFFU;
    d3uc_put_byte(c, (unsigned int)(t0 & U32MASK));
    /* retn */
    return;
        case 4:
    t0 = (uint32_t)D3U_OUT_TOTAL_POS_32(&c->st, 0);
    if (t0 < (uint32_t)D3U_TBL_444228_32(&c->st, 0)) { _pc = 5; continue; }
    if ((t3 & 0xFFU) != 0xFEU) { _pc = 5; continue; }
    D3U_FLAG_444EA7_8(&c->st, 0) = 1;
    /* retn */
    return;
        case 5:
    t0 = (uint32_t)D3U_OUT_TOTAL_POS_32(&c->st, 0);
    if (t0 >= (uint32_t)(uint32_t)D3U_PRIMER_ACCUMULATOR_32(&c->st, 0)) { _pc = 2; continue; }
    if ((t3 & 0xFFU) >= 0x80U) { _pc = 6; continue; }
    if (D3U_FLAG_444EA7_8(&c->st, 0) == 0) { _pc = 2; continue; }
            _pc = 6;
            continue;
        case 6:
    t0 = 0;
    t0 = (t0 & ~0xFFU) | (t3 & 0xFFU);
    t0 = (t0 & ~0xFFU) | ((uint32_t)d3u_t42C438[(unsigned int)(t0 & 0xFFU)] & 0xFFU);
    if ((t0 & 0xFFU) >= 0xC8U) { _pc = 10; continue; }
    if ((t0 & 0xFFU) != 0x64U) { _pc = 7; continue; }
    t0 = (t0 & ~0xFF00U) | (1U << 8);
    D3U_FLAG_444EAD_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
    D3U_FLAG_444EA7_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFU);
    /* retn */
    return;
        case 7:
    if ((t0 & 0xFFU) != 0x63U) { _pc = 8; continue; }
    D3U_FLAG_444EA7_8(&c->st, 0) = 0x20;
    /* retn */
    return;
        case 8:
    t1 = 0;
    t1 = (t1 & ~0xFFU) | (t0 & 0xFFU);
    t2 = (t2 & ~0xFFU) | ((uint32_t)D3U_FLAG_444EA7_8(&c->st, 0) & 0xFFU);
    t3 = (uint32_t)d3u_t42BDDC[(unsigned int)(t1 & 0xFFU)];
    t1 = (t1 & ~0xFFU) | ((uint32_t)d3u_t42BF6C[(unsigned int)(t1 & 0xFFU)] & 0xFFU);
    if ((t2 & 0xFFU) == 0) { _pc = 9; continue; }
    t0 = (t0 & ~0xFFU) | ((uint32_t)M8(t3 & U32MASK) & 0xFFU);
    t0 = (t0 & ~0xFFU) | ((~(t0 & 0xFFU)) & 0xFFU);
    t3 = (t3 + 1U) & U32MASK;
    t0 = (t0 & ~0xFFU) | (((t0 & 0xFFU) - 0x20U) & 0xFFU);
    t0 &= 0xFFU;
    d3uc_put_byte(c, (unsigned int)(t0 & U32MASK));
    t2 = t2 & ~0xFF00U;
    t1 = (t1 & ~0xFFU) | (((t1 & 0xFFU) - 1U) & 0xFFU);
    D3U_FLAG_444EA7_8(&c->st, 0) = (unsigned char)((t2 >> 8) & 0xFFU);
            _pc = 9;
            continue;
        case 9:
    t0 = (t0 & ~0xFFU) | ((uint32_t)M8(t3 & U32MASK) & 0xFFU);
    t0 = (t0 & ~0xFFU) | ((~(t0 & 0xFFU)) & 0xFFU);
    t0 &= 0xFFU;
    t3 = (t3 + 1U) & U32MASK;
    d3uc_put_byte(c, (unsigned int)(t0 & U32MASK));
    t1 = (t1 & ~0xFFU) | (((t1 & 0xFFU) - 1U) & 0xFFU);
    if ((t1 & 0xFFU) == 0) { _pc = 11; continue; }
    { _pc = 9; continue; }
        case 10:
    t1 = (t1 & ~0xFFU) | ((uint32_t)D3U_FLAG_444EA7_8(&c->st, 0) & 0xFFU);
    t0 = (t0 & ~0xFFU) | (t3 & 0xFFU);
    t0 = (t0 & ~0xFFU) | (((t0 & 0xFFU) - (t1 & 0xFFU)) & 0xFFU);
    t0 &= 0xFFU;
    t1 = t1 & ~0xFF00U;
    d3uc_put_byte(c, (unsigned int)(t0 & U32MASK));
    D3U_FLAG_444EA7_8(&c->st, 0) = (unsigned char)((t1 >> 8) & 0xFFU);
            _pc = 11;
            continue;
        case 11:
    /* retn */
    return;
        default:
            break;
        }
        break;
    }
}
/* sub_416640: length-symbol coder. Model dword_443B68 (8-byte entries).
 * Calls sub_423188/sub_423140, escape reloads 429DB8/429DE0/429E30,
 * calls sub_41B2D4 + sub_414FD4. Entry al (cl) live but signature is
 * void(c) per unit contract: cl zero-inits (UNCERTAIN 0041664A, see
 * summary). Exit al=bl stashed in c->hi (signature void, see summary).
 */
unsigned int d3uc_decode_length_sym(dec3w_ctx *c, unsigned int al_in)
{
    uint32_t t0 = 0;
    uint32_t t1 = 0;
    uint32_t t2 = 0;
    uint32_t t3 = 0;
    uint32_t t4 = 0;
    uint32_t t6 = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t6 = D3U_RC_TOTAL;
    t0 = (t0 & ~0xFFU) | (al_in & 0xFFU); /* entry al live */
    t2 = (t2 & ~0xFFU) | (t0 & 0xFFU);
            _pc = 1;
            continue;
        case 1:
    t0 = 0;
    t3 = 0;
    t0 = (t0 & ~0xFFU) | ((uint32_t)D3U_FLAG_444EB6_8(&c->st, 0) & 0xFFU);
    t3 = (t3 & ~0xFFU) | (t2 & 0xFFU);
    t0 = (t0 + t3) & U32MASK;
    t0 = (t0 << 3) & U32MASK;
    t0 = (t0 + D3U_TBL_443B68) & U32MASK;
    t3 = (t0 + 6U) & U32MASK;
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)(t3 & U32MASK)) & 0xFFFFU);
    D3U_RC_TOTAL_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)D3U_RC_TOTAL_16(&c->st, 0) & 0xFFFFU);
    t1 = (t1 & ~0xFFU) | 3U;
    t0 = ((unsigned int (*)(dec3w_ctx *, unsigned int))d3uc_rc_decode)(c, (unsigned int)(t0 & 0xFFFFU));
            _pc = 2;
            continue;
        case 2:
    t1 = (t1 & ~0xFFU) | (((t1 & 0xFFU) - 1U) & 0xFFU);
    t4 = (t4 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((t3 - 2U) & U32MASK)) & 0xFFFFU);
    t3 = (t3 - 2U) & U32MASK;
    if ((t0 & 0xFFFFU) >= (t4 & 0xFFFFU)) { _pc = 3; continue; }
    if ((t1 & 0xFFU) != 0) { _pc = 2; continue; }
            _pc = 3;
            continue;
        case 3:
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)(t3 & U32MASK)) & 0xFFFFU);
    D3U_RC_PROB_LO_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((t3 + 2U) & U32MASK)) & 0xFFFFU);
    D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = t6;
    d3uc_rc_update(c, (unsigned int)(t0 & U32MASK));
    if ((t1 & 0xFFU) != 0) { _pc = 7; continue; }
    t3 = 0x0AU;
    t0 = t3;
    D3U_RC_TOTAL_32(&c->st, 0) = (unsigned int)(t3 & U32MASK);
    t0 = ((unsigned int (*)(dec3w_ctx *, unsigned int))d3uc_rc_decode)(c, (unsigned int)(t0 & 0xFFFFU));
    t3 = t0;
    t0 = 0;
    t1 = 0;
    t0 = (t0 & ~0xFFU) | (t3 & 0xFFU);
    t1 = (t1 & ~0xFFU) | (t3 & 0xFFU);
    D3U_RC_PROB_LO_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = (t1 + 1U) & U32MASK;
    D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = t6;
    d3uc_rc_update(c, (unsigned int)(t0 & U32MASK));
    if ((t3 & 0xFFU) == ((uint32_t)D3U_FLAG_444EA9_8(&c->st, 0) & 0xFFU)) { _pc = 1; continue; }
    D3U_FLAG_444EA9_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
    t0 = (uint32_t)d3u_t429DE0[(unsigned int)(t1 & 0xFFU)];
    t4 = (uint32_t)d3u_t429DB8[(unsigned int)(t1 & 0xFFU)];
    D3U_TBL_4441C4_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = (t0 & ~0xFFU) | ((uint32_t)d3u_t429E30[(unsigned int)(t1 & 0xFFU)] & 0xFFU);
    D3U_TBL_4441C8_32(&c->st, 0) = (unsigned int)(t4 & U32MASK);
    D3U_FLAG_444E9E_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    if ((t3 & 0xFFU) == 5U) { _pc = 4; continue; }
    if ((t3 & 0xFFU) != 6U) { _pc = 5; continue; }
            _pc = 4;
            continue;
        case 4:
    t0 = (t0 & ~0xFFU) | 1U;
    { _pc = 6; continue; }
        case 5:
    t0 = t0 & ~0xFFU;
            _pc = 6;
            continue;
        case 6:
    D3U_FLAG_444EA6_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    t3 = t3 & ~0xFFU;
    t1 = 0;
    D3U_FLAG_444EA8_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
    D3U_TBL_4441D0_32(&c->st, 0) = (unsigned int)(t1 & U32MASK);
    t1 = (t1 & ~0xFFU) | ((uint32_t)D3U_FLAG_444EA9_8(&c->st, 0) & 0xFFU);
    D3U_FLAG_444E9D_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
    if ((t1 & 0xFFU) <= 4U) { _pc = 1; continue; }
    d3uc_init_hash_buckets(c);
    { _pc = 1; continue; }
        case 7:
    t3 = 0;
    t0 = 0;
    t3 = (t3 & ~0xFFU) | (t1 & 0xFFU);
    t0 = (t0 & ~0xFFU) | (t2 & 0xFFU);
    d3uc_update_len_model(c, (unsigned int)(t0 & U32MASK), (unsigned int)(t3 & U32MASK));
    t0 = (t0 & ~0xFFU) | (t1 & 0xFFU);
    /* retn */
    return (t0 & 0xFFU);
        default:
            break;
        }
        break;
    }
    return 0;
}
/* sub_4177C0: offset/selector coder. Tables dword_4299F0/4299F4/4299FC,
 * const word_42D274. Calls sub_423188/sub_423140. Entry t0 live, taken
 * as x (UNCERTAIN frame sizes: var_C4 22 dwords / var_68 words modelled
 * as 256-entry arrays, zero-init). Exit al (cl or bh) stashed in c->hi
 * because the unit signature is void (see summary).
 */
unsigned int d3uc_decode_offset_sym(dec3w_ctx *c, unsigned int x)
{
    uint32_t t0 = 0;
    uint32_t t1 = 0;
    uint32_t t2 = 0;
    uint32_t t3 = 0;
    uint32_t t4 = 0;
    uint32_t t5 = 0;
    uint32_t t6 = 0;
    uint32_t v34 = 0;
    uint32_t v20 = 0;
    uint32_t v24 = 0;
    uint32_t v38 = 0;
    uint32_t v30 = 0;
    uint32_t v28 = 0;
    uint32_t v2c = 0;
    uint32_t v1c = 0;
    uint32_t c4[256] = {0};
    uint32_t cw[256] = {0};
    uint32_t ta = 0;
    uint32_t ti = 0;
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t1 = (t1 & ~0xFFU) | ((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0) & 0xFFU);
    v34 = (uint32_t)x & U32MASK;
    t4 = (uint32_t)x & U32MASK;
    t0 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    t4 = (t4 << 9) & U32MASK;
    t4 = (t4 + t0) & U32MASK;
    t6 = v34;
    t2 = t2 & ~0xFFU;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)(t4 & U32MASK)) & 0xFFFFU);
    t6 = (t6 << 8) & U32MASK;
    v20 = t0;
    t0 = 0;
            _pc = 1;
            continue;
        case 1:
    t3 = 0;
    t3 = (t3 & ~0xFFU) | (t1 & 0xFFU);
    if (t0 >= t3) { _pc = 2; continue; }
    t3 = 0;
    t3 = (t3 & ~0xFFU) | ((uint32_t)M8((D3U_FLAG_444E82 + (t0 & U32MASK)) & U32MASK) & 0xFFU);
    t3 = (t3 + t6) & U32MASK;
    t5 = (t3 * 2U) & U32MASK;
    t3 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t3 = (t3 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((t3 + t5) & U32MASK)) & 0xFFFFU);
    t5 = v20;
    t5 = (t5 - t3) & U32MASK;
    t0 = (t0 + 1U) & U32MASK;
    v20 = t5;
    { _pc = 1; continue; }
        case 2:
    t3 = v20;
    D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)(t1 & 0xFFU);
    if ((t3 & 0xFFFFU) != 0) { _pc = 7; continue; }
    t0 = (uint32_t)D3U_TBL_44421C_32(&c->st, 0);
    v34 = t0;
    t4 = t0;
    t0 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    t4 = (t4 << 9) & U32MASK;
    t4 = (t4 + t0) & U32MASK;
    t6 = v34;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)(t4 & U32MASK)) & 0xFFFFU);
    t6 = (t6 << 8) & U32MASK;
    v20 = t0;
    t0 = 0;
            _pc = 3;
            continue;
        case 3:
    t3 = 0;
    t3 = (t3 & ~0xFFU) | (t1 & 0xFFU);
    D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)(t1 & 0xFFU);
    if (t0 >= t3) { _pc = 4; continue; }
    t3 = 0;
    t3 = (t3 & ~0xFFU) | ((uint32_t)M8((D3U_FLAG_444E82 + (t0 & U32MASK)) & U32MASK) & 0xFFU);
    t3 = (t3 + t6) & U32MASK;
    t2 = (t3 * 2U) & U32MASK;
    t3 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t3 = (t3 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((t2 + t3) & U32MASK)) & 0xFFFFU);
    t2 = v20;
    t2 = (t2 - t3) & U32MASK;
    t0 = (t0 + 1U) & U32MASK;
    v20 = t2;
    { _pc = 3; continue; }
        case 4:
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | (v20 & 0xFFFFU);
    D3U_RC_TOTAL_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)D3U_RC_TOTAL_16(&c->st, 0) & 0xFFFFU);
    t0 = ((unsigned int (*)(dec3w_ctx *, unsigned int))d3uc_rc_decode)(c, (unsigned int)(t0 & 0xFFFFU));
    v24 = t0;
            _pc = 5;
            continue;
        case 5:
    t1 = (t1 & ~0xFFU) | ((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0) & 0xFFU);
    if ((t1 & 0xFFU) == 0) { _pc = 25; continue; }
    t1 = (t1 & ~0xFF00U) | (0x80U << 8);
    t3 = v34;
    v38 = t4;
    t0 = 0;
    t3 = (t3 << 8) & U32MASK;
    t2 = 0;
    v30 = t3;
    t3 = 0;
            _pc = 6;
            continue;
        case 6:
    t5 = t1 & 0xFFU;
    if (t0 >= t5) { _pc = 16; continue; }
    t5 = (uint32_t)M8((D3U_FLAG_444E82 + (t0 & U32MASK)) & U32MASK) & 0xFFU;
    t5 = (t5 + t5) & U32MASK;
    t5 = (t5 + t4) & U32MASK;
    t6 = v30;
    ta = (t3 >> 2) & U32MASK;
    ti = 0;
    c4[ta & 0xFFU] = t5;
    t5 = (uint32_t)M8((D3U_FLAG_444E82 + (t0 & U32MASK)) & U32MASK) & 0xFFU;
    t5 = (t5 + t6) & U32MASK;
    t6 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t5 = (t5 + t5) & U32MASK;
    t2 = (t2 + 2U) & U32MASK;
    t5 = (t5 + t6) & U32MASK;
    t3 = (t3 + 4U) & U32MASK;
    t0 = (t0 + 1U) & U32MASK;
    t5 = (t5 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)(t5 & U32MASK)) & 0xFFFFU);
    ti = (t2 >> 1) & U32MASK;
    cw[(ti - 1U) & 0xFFU] = t5 & 0xFFFFU;
    { _pc = 6; continue; }
        case 7:
    t0 = v34;
    t3 = (uint32_t)(uint32_t)D3U_BLK17_SMALL_B_32(&c->st, 0);
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((t3 + (t0 & U32MASK) * 2U) & U32MASK)) & 0xFFFFU);
    t3 = 0;
    t0 &= 0xFFFFU;
    t3 = (t3 & ~0xFFFFU) | (v20 & 0xFFFFU);
    t3 = (t3 + t0) & U32MASK;
    D3U_RC_TOTAL_32(&c->st, 0) = (unsigned int)(t3 & U32MASK);
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)D3U_RC_TOTAL_16(&c->st, 0) & 0xFFFFU);
    t5 = v20;
    t0 = ((unsigned int (*)(dec3w_ctx *, unsigned int))d3uc_rc_decode)(c, (unsigned int)(t0 & 0xFFFFU));
    v24 = t0;
    if ((t0 & 0xFFFFU) < (t5 & 0xFFFFU)) { _pc = 5; continue; }
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | (t5 & 0xFFFFU);
    D3U_RC_PROB_LO_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = (uint32_t)D3U_RC_TOTAL_32(&c->st, 0);
    t3 = (uint32_t)D3U_RC_PROB_LO_32(&c->st, 0);
    D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = (t0 - t3) & U32MASK;
    t4 = (uint32_t)D3U_RC_TOTAL_32(&c->st, 0);
    t0 = (t0 << 14) & U32MASK;
    t3 = 0;
    t0 = (t4 == 0) ? 0 : (t0 / t4) & U32MASK;
    t3 = 0;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((D3U_U16_CUM_DST + (t0 & U32MASK) * 2U) & U32MASK)) & 0xFFFFU);
    t5 = (uint32_t)D3U_TBL_4441F4_32(&c->st, 0);
    t0 &= 0xFFFFU;
    t5 = (t5 + t0) & U32MASK;
    t0 = D3U_RC_TOTAL;
    D3U_TBL_4441F4_32(&c->st, 0) = (unsigned int)(t5 & U32MASK);
    d3uc_rc_update(c, (unsigned int)(t0 & U32MASK));
    t0 = v34;
    t3 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t0 = (t0 << 9) & U32MASK;
    t3 = (t3 + t0) & U32MASK;
    t0 = (uint32_t)D3U_TBL_44421C_32(&c->st, 0);
    v28 = t3;
    t3 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    t0 = (t0 << 9) & U32MASK;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((t3 + t0) & U32MASK)) & 0xFFFFU);
    t4 = (uint32_t)D3U_TBL_44421C_32(&c->st, 0);
    t0 &= 0xFFFFU;
    t1 = t1 & ~0xFF00U;
    D3U_RC_TOTAL_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t4 = (t4 << 8) & U32MASK;
            _pc = 8;
            continue;
        case 8:
    t1 = (t1 & ~0xFFU) | ((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0) & 0xFFU);
    t0 = 0;
    t0 = (t0 & ~0xFFU) | ((t1 >> 8) & 0xFFU);
    t3 = v28;
    t0 = (t0 + t0) & U32MASK;
    t0 = (t0 + t3) & U32MASK;
    t3 = (t3 & ~0xFFU) | ((d3u_ld16(c, (uint32_t)(t0 & U32MASK)) != 0) ? 1U : 0U);
    t0 = 0;
    v1c = t3 & 0xFFU;
    if ((t3 & 0xFFU) != 0) { _pc = 10; continue; }
            _pc = 9;
            continue;
        case 9:
    t3 = 0;
    t3 = (t3 & ~0xFFU) | (t1 & 0xFFU);
    if (t0 >= t3) { _pc = 10; continue; }
    t3 = t0;
    t2 = (t2 & ~0xFF00U) | ((((uint32_t)M8((D3U_FLAG_444E82 + (t3 & U32MASK)) & U32MASK) & 0xFFU)) << 8);
    t0 = (t0 + 1U) & U32MASK;
    t3 = (t3 & ~0xFFU) | ((((t1 >> 8) & 0xFFU) == ((t2 >> 8) & 0xFFU)) ? 1U : 0U);
    v1c = t3 & 0xFFU;
    if ((t3 & 0xFFU) == 0) { _pc = 9; continue; }
            _pc = 10;
            continue;
        case 10:
    if ((v1c & 0xFFU) == 0) { _pc = 11; continue; }
    t0 = 0;
    t0 = (t0 & ~0xFFU) | ((t1 >> 8) & 0xFFU);
    t3 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t0 = (t0 + t4) & U32MASK;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((t3 + (t0 & U32MASK) * 2U) & U32MASK)) & 0xFFFFU);
    t5 = (uint32_t)D3U_RC_TOTAL_32(&c->st, 0);
    t0 &= 0xFFFFU;
    t5 = (t5 - t0) & U32MASK;
    D3U_RC_TOTAL_32(&c->st, 0) = (unsigned int)(t5 & U32MASK);
            _pc = 11;
            continue;
        case 11:
    t1 = (t1 & ~0xFF00U) | (((((t1 >> 8) & 0xFFU) + 1U) & 0xFFU) << 8);
    D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)(t1 & 0xFFU);
    if (((t1 >> 8) & 0xFFU) != 0) { _pc = 8; continue; }
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)D3U_RC_TOTAL_16(&c->st, 0) & 0xFFFFU);
    t0 = ((unsigned int (*)(dec3w_ctx *, unsigned int))d3uc_rc_decode)(c, (unsigned int)(t0 & 0xFFFFU));
    t5 = t0;
    t0 = (uint32_t)D3U_TBL_44421C_32(&c->st, 0);
    t6 = v28;
    t0 = (t0 << 8) & U32MASK;
    t4 = 0;
    v2c = t0;
            _pc = 12;
            continue;
        case 12:
    t2 = (t2 & ~0xFFU) | (((t2 & 0xFFU) - 1U) & 0xFFU);
    t0 = 0;
    t0 = (t0 & ~0xFFU) | (t2 & 0xFFU);
    t0 = (t0 + t0) & U32MASK;
    t0 = (t0 + t6) & U32MASK;
    t1 = (t1 & ~0xFFU) | ((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0) & 0xFFU);
    t3 = (t3 & ~0xFFU) | ((d3u_ld16(c, (uint32_t)(t0 & U32MASK)) == 0) ? 1U : 0U);
    t1 = (t1 & ~0xFF00U) | ((t3 & 0xFFU) << 8);
    t0 = 0;
    if ((t3 & 0xFFU) == 0) { _pc = 14; continue; }
            _pc = 13;
            continue;
        case 13:
    t3 = 0;
    t3 = (t3 & ~0xFFU) | (t1 & 0xFFU);
    if (t0 >= t3) { _pc = 14; continue; }
    t3 = t0;
    t1 = (t1 & ~0xFF00U) | ((((uint32_t)M8((D3U_FLAG_444E82 + (t3 & U32MASK)) & U32MASK) & 0xFFU)) << 8);
    t0 = (t0 + 1U) & U32MASK;
    t3 = (t3 & ~0xFFU) | (((t2 & 0xFFU) != ((t1 >> 8) & 0xFFU)) ? 1U : 0U);
    t1 = (t1 & ~0xFF00U) | ((t3 & 0xFFU) << 8);
    if ((t3 & 0xFFU) != 0) { _pc = 13; continue; }
            _pc = 14;
            continue;
        case 14:
    if (((t1 >> 8) & 0xFFU) == 0) { _pc = 15; continue; }
    t0 = 0;
    t3 = v2c;
    t0 = (t0 & ~0xFFU) | (t2 & 0xFFU);
    t0 = (t0 + t3) & U32MASK;
    t3 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t4 = (t4 + ((uint32_t)d3u_ld16(c, (uint32_t)((t3 + (t0 & U32MASK) * 2U) & U32MASK)) & 0xFFFFU)) & U32MASK;
            _pc = 15;
            continue;
        case 15:
    D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)(t1 & 0xFFU);
    if ((t4 & 0xFFFFU) <= (t5 & 0xFFFFU)) { _pc = 12; continue; }
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | (t4 & 0xFFFFU);
    t3 = 0;
    D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = (uint32_t)D3U_TBL_44421C_32(&c->st, 0);
    t3 = (t3 & ~0xFFU) | (t2 & 0xFFU);
    t0 = (t0 << 8) & U32MASK;
    t0 = (t0 + t3) & U32MASK;
    t3 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t0 = (t0 + t0) & U32MASK;
    t0 = (t0 + t3) & U32MASK;
    t3 = 0;
    t3 = (t3 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)(t0 & U32MASK)) & 0xFFFFU);
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | (t4 & 0xFFFFU);
    t0 = (t0 - t3) & U32MASK;
    D3U_RC_PROB_LO_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = (uint32_t)D3U_RC_PROB_HI_32(&c->st, 0);
    t4 = (uint32_t)D3U_RC_PROB_LO_32(&c->st, 0);
    t5 = (uint32_t)D3U_RC_TOTAL_32(&c->st, 0);
    t0 = (t0 - t4) & U32MASK;
    t3 = 0;
    t0 = (t0 << 14) & U32MASK;
    t0 = (t5 == 0) ? 0 : (t0 / t5) & U32MASK;
    t3 = 0;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((D3U_U16_CUM_DST + (t0 & U32MASK) * 2U) & U32MASK)) & 0xFFFFU);
    t6 = (uint32_t)D3U_TBL_4441F4_32(&c->st, 0);
    t0 &= 0xFFFFU;
    t6 = (t6 + t0) & U32MASK;
    t0 = D3U_RC_TOTAL;
    D3U_TBL_4441F4_32(&c->st, 0) = (unsigned int)(t6 & U32MASK);
    d3uc_rc_update(c, (unsigned int)(t0 & U32MASK));
    t0 = (t0 & ~0xFFU) | (t2 & 0xFFU);
    t1 = (t1 & ~0xFFU) | ((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0) & 0xFFU);
    /* retn */
    return (t0 & 0xFFU);
        case 16:
    t0 = v24;
    v20 = (v20 - t0) & U32MASK;
            _pc = 17;
            continue;
        case 17:
    t5 = 0;
    t0 = 0;
    t2 = 0;
    t3 = 0;
            _pc = 18;
            continue;
        case 18:
    t6 = t1 & 0xFFU;
    if (t0 >= t6) { _pc = 20; continue; }
    t6 = (t1 >> 8) & 0xFFU;
    t6 = (t6 + t6) & U32MASK;
    t6 = (t6 + t4) & U32MASK;
    ta = (t2 >> 2) & U32MASK;
    if (t6 <= c4[ta & 0xFFU]) { _pc = 19; continue; }
    ti = (t3 >> 1) & U32MASK;
    t6 = cw[ti & 0xFFU] & 0xFFFFU;
    t5 = (t5 + t6) & U32MASK;
            _pc = 19;
            continue;
        case 19:
    t2 = (t2 + 4U) & U32MASK;
    t3 = (t3 + 2U) & U32MASK;
    t0 = (t0 + 1U) & U32MASK;
    { _pc = 18; continue; }
        case 20:
    t0 = 0;
    t0 = (t0 & ~0xFFU) | ((t1 >> 8) & 0xFFU);
    t3 = 0;
    t3 = (t3 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((t4 + (t0 & 0xFFU) * 2U) & U32MASK)) & 0xFFFFU);
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | (v20 & 0xFFFFU);
    t0 = (t0 + t5) & U32MASK;
    if ((t3 & 0xFFFFU) >= (t0 & 0xFFFFU)) { _pc = 24; continue; }
    t3 = 0;
    t0 = 0;
            _pc = 21;
            continue;
        case 21:
    t2 = 0;
    t2 = (t2 & ~0xFFU) | (t1 & 0xFFU);
    if (t3 >= t2) { _pc = 23; continue; }
    t2 = 0;
    t2 = (t2 & ~0xFFU) | ((t1 >> 8) & 0xFFU);
    t2 = (t2 + t2) & U32MASK;
    ta = (t0 >> 2) & U32MASK;
    t6 = c4[ta & 0xFFU];
    t2 = (t2 + t4) & U32MASK;
    if (t2 <= t6) { _pc = 22; continue; }
    c4[ta & 0xFFU] = 0xFFFFFFFFU;
            _pc = 22;
            continue;
        case 22:
    t0 = (t0 + 4U) & U32MASK;
    t3 = (t3 + 1U) & U32MASK;
    { _pc = 21; continue; }
        case 23:
    t0 = 0;
    t0 = (t0 & ~0xFFU) | ((t1 >> 8) & 0xFFU);
    t0 = (t0 + t0) & U32MASK;
    t4 = (t4 + t0) & U32MASK;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)(t4 & U32MASK)) & 0xFFFFU);
    t3 = v20;
    t0 = (t0 - t5) & U32MASK;
    t3 = (t3 - t0) & U32MASK;
    v20 = t3;
            _pc = 24;
            continue;
        case 24:
    t1 = (t1 & ~0xFF00U) | ((((t1 >> 8) & 0xFFU) >> 1) << 8);
    if (((t1 >> 8) & 0xFFU) != 0) { _pc = 17; continue; }
    t0 = v24;
    t2 = v20;
    t5 = v38;
    t2 = (t2 + t0) & U32MASK;
    t0 = t4;
    t0 = (t0 - t5) & U32MASK;
    t0 = ((uint32_t)(((int)(t0 & U32MASK)) >> 1)) & U32MASK;
    t1 = (t1 & ~0xFF00U) | ((t0 & 0xFFU) << 8);
    v20 = t2;
    { _pc = 28; continue; }
        case 25:
    t3 = v24;
    t5 = v20;
    t0 = (t0 & ~0xFFU) | 0x80U;
    t5 = (t5 - t3) & U32MASK;
    t2 = t4;
    v20 = t5;
            _pc = 26;
            continue;
        case 26:
    t3 = 0;
    t3 = (t3 & ~0xFFU) | (t0 & 0xFFU);
    t3 = (t3 + t3) & U32MASK;
    t3 = (t3 + t4) & U32MASK;
    t5 = (t5 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)(t3 & U32MASK)) & 0xFFFFU);
    if ((t5 & 0xFFFFU) >= (v20 & 0xFFFFU)) { _pc = 27; continue; }
    t4 = t3;
    t3 = t5;
    v20 = (v20 - t3) & U32MASK;
            _pc = 27;
            continue;
        case 27:
    t0 = (t0 & ~0xFFU) | (((t0 & 0xFFU) >> 1) & 0xFFU);
    if ((t0 & 0xFFU) != 0) { _pc = 26; continue; }
    t0 = v24;
    t3 = v20;
    t3 = (t3 + t0) & U32MASK;
    t0 = t4;
    t0 = (t0 - t2) & U32MASK;
    t0 = ((uint32_t)(((int)(t0 & U32MASK)) >> 1)) & U32MASK;
    t1 = (t1 & ~0xFF00U) | ((t0 & 0xFFU) << 8);
    v20 = t3;
            _pc = 28;
            continue;
        case 28:
    D3U_FLAG_444EAA_8(&c->st, 0) = (unsigned char)(t1 & 0xFFU);
    t0 = 0;
    t0 = (t0 & ~0xFFFFU) | (v20 & 0xFFFFU);
    t3 = 0;
    D3U_RC_PROB_HI_32(&c->st, 0) = (unsigned int)(t0 & U32MASK);
    t0 = v34;
    t3 = (t3 & ~0xFFU) | ((t1 >> 8) & 0xFFU);
    t0 = (t0 << 8) & U32MASK;
    t0 = (t0 + t3) & U32MASK;
    t3 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((t3 + (t0 & U32MASK) * 2U) & U32MASK)) & 0xFFFFU);
    t3 = 0;
    t0 &= 0xFFFFU;
    t3 = (t3 & ~0xFFFFU) | (v20 & 0xFFFFU);
    t3 = (t3 - t0) & U32MASK;
    t0 = (uint32_t)D3U_RC_PROB_HI_32(&c->st, 0);
    t2 = (uint32_t)D3U_RC_TOTAL_32(&c->st, 0);
    t0 = (t0 - t3) & U32MASK;
    D3U_RC_PROB_LO_32(&c->st, 0) = (unsigned int)(t3 & U32MASK);
    t0 = (t0 << 14) & U32MASK;
    t3 = 0;
    t0 = (t2 == 0) ? 0 : (t0 / t2) & U32MASK;
    t3 = 0;
    t0 = (t0 & ~0xFFFFU) | ((uint32_t)d3u_ld16(c, (uint32_t)((D3U_U16_CUM_DST + (t0 & U32MASK) * 2U) & U32MASK)) & 0xFFFFU);
    t4 = (uint32_t)D3U_TBL_4441F4_32(&c->st, 0);
    t0 &= 0xFFFFU;
    t4 = (t4 + t0) & U32MASK;
    t0 = D3U_RC_TOTAL;
    D3U_TBL_4441F4_32(&c->st, 0) = (unsigned int)(t4 & U32MASK);
    d3uc_rc_update(c, (unsigned int)(t0 & U32MASK));
    t0 = (t0 & ~0xFFU) | ((t1 >> 8) & 0xFFU);
    t1 = (t1 & ~0xFFU) | ((uint32_t)D3U_FLAG_444EAA_8(&c->st, 0) & 0xFFU);
    /* retn */
    return (t0 & 0xFFU);
        default:
            break;
        }
        break;
    }
    return 0;
}

/* ---- range coder + stream + models (from draft_r3.c) ---- */
/* ---- compressed-input stream (sub_41E17C / sub_41E2B0 / sub_41E1C0) ---- */

void d3uc_topup_dict_rebuild(dec3w_ctx *c)
{
    uint32_t t0 = 0;
    uint32_t t1 = 0;
    uint32_t t2 = 0;
    uint32_t t3 = 0;
    uint32_t t4 = 0;
    uint32_t t5 = 0;
    uint32_t t6 = 0;
    uint32_t v1c = 0;
    uint32_t v20 = 0;
    uint32_t v24 = 0;
    uint32_t v28 = 0;
    uint32_t v2c = 0;
    unsigned char stk[256] = {0};
    unsigned int _pc;
    _pc = 0;
    for (;;) {
        switch (_pc) {
        case 0:
    t6 = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
    t0 = (uint32_t)D3U_TBL_444250_32(&c->st, 0);
    v24 = t0;
    t0 = (uint32_t)D3U_TBL_444220_32(&c->st, 0);
    v2c = t0;
    t0 = (uint32_t)D3U_TBL_444264_32(&c->st, 0);
    t3 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
    v20 = t0;
    t0 = 0;
            _pc = 1;
            continue;
        case 1:
    t1 = t6;
    t1 = (t1 & 0xFFFFFF00U) | (uint32_t)c->blk00_dict[((t1 + t3) & U32MASK) - c->blk00_base];
    t3 = (t3 + 1U) & U32MASK;
    stk[t0] = (unsigned char)(t1 & 0xFFU);
    t1 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
    t0 = (t0 + 1U) & U32MASK;
    t3 &= t1;
     if (t0 < 0x100U) { _pc = 1; continue; } /* unsigned */
    t3 = (uint32_t)D3U_DICT_SIZE_32(&c->st, 0);
    t0 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
    t3 = (t3 - 0x140U) & U32MASK;
    t0 = (t0 + 0x140U) & U32MASK;
    t3 >>= 1;
    t0 &= t1;
    t5 = (t0 + t3) & U32MASK;
    t5 &= t1;
     if (t3 > (uint32_t)D3U_TBL_4441C0_32(&c->st, 0)) { /* unsigned */
    t5 = 0;
    }
            _pc = 2;
            continue;
        case 2:
    t2 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
    t3 &= 0xFFFFFF00U;
    v28 = t0;
    D3U_FLAG_444EA7_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
     if (t5 == t2) { _pc = 14; continue; }
            _pc = 3;
            continue;
        case 3:
    t1 = t6;
    t2 = (t0 + 1U) & U32MASK;
    t3 = (t3 & 0xFFFFFF00U) | (uint32_t)c->blk00_dict[((t5 + t1) & U32MASK) - c->blk00_base];
    t1 = (t0 + t6) & U32MASK;
    t3 = (t3 & 0xFFFFFF00U) | ((~t3) & 0xFFU);
    v1c = t1;
    t1 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
    t3 = (t3 & 0xFFFF00FFU) | ((uint32_t)D3U_FLAG_444EA7_8(&c->st, 0) << 8);
    t2 &= t1;
    if (((t3 >> 8) & 0xFFU) != 1U) { _pc = 4; continue; }
    t1 &= 0xFFFFFF00U;
    D3U_FLAG_444EA7_8(&c->st, 0) = (unsigned char)(t1 & 0xFFU);
    t1 = 0;
    t1 = (uint32_t)D3U_FLAG_444EAD_8(&c->st, 0);
    t1 = (t1 - 0xA6U) & U32MASK;
    t1 = (t1 * 11U) & U32MASK;
    t0 = 0;
    t0 = t3 & 0xFFU;
    t0 = (uint32_t)d3u_t42BD70[(1826U + t1 + t0) & U32MASK];
    { _pc = 12; continue; }
        case 4:
     if ((t3 & 0xFFU) >= 0x80U) { _pc = 5; continue; } /* unsigned */
    if (((t3 >> 8) & 0xFFU) == 0U) { _pc = 11; continue; }
            _pc = 5;
            continue;
        case 5:
    t1 = 0;
    t1 = t3 & 0xFFU;
    t4 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
    t2 = (t0 + 1U) & U32MASK;
    t3 = (t3 & 0xFFFF00FFU) | ((uint32_t)d3u_t42C438[(unsigned int)(t1 & 0xFFU)] << 8);
    t2 &= t4;
     if (((t3 >> 8) & 0xFFU) >= 0xC8U) { _pc = 10; continue; } /* unsigned */
     if (((t3 >> 8) & 0xFFU) != 0x64U) { _pc = 6; continue; }
    D3U_FLAG_444EA7_8(&c->st, 0) = 1;
    D3U_FLAG_444EAD_8(&c->st, 0) = (unsigned char)(t3 & 0xFFU);
    { _pc = 13; continue; }
        case 6:
     if (((t3 >> 8) & 0xFFU) != 0x63U) { _pc = 7; continue; }
    D3U_FLAG_444EA7_8(&c->st, 0) = 0x20;
    { _pc = 13; continue; }
        case 7:
    t1 = 0;
    t1 = (t3 >> 8) & 0xFFU;
    t3 = (uint32_t)d3u_t42BDDC[(unsigned int)(t1 & 0xFFU)];
    t1 = (t1 & 0xFFFFFF00U) | (uint32_t)d3u_t42BF6C[(unsigned int)(t1 & 0xFFU)];
    if (D3U_FLAG_444EA7_8(&c->st, 0) == 0) { _pc = 8; continue; }
    t4 = t3;
    t1 = (t1 & 0xFFFF00FFU) | ((uint32_t)M8(t4) << 8);
    t0 = (t0 + t6) & U32MASK;
    t1 = (t1 & 0xFFFF00FFU) | (((((t1 >> 8) & 0xFFU) + 0x20U) & 0xFFU) << 8);
    t3 = (t3 + 1U) & U32MASK;
    c->blk00_dict[t0 - c->blk00_base] = (unsigned char)((t1 >> 8) & 0xFFU);
    t0 = t2;
    t2 &= 0xFFFFFF00U;
    t1 = (t1 & 0xFFFFFF00U) | ((t1 - 1U) & 0xFFU);
    D3U_FLAG_444EA7_8(&c->st, 0) = (unsigned char)(t2 & 0xFFU);
            _pc = 8;
            continue;
        case 8:
    t4 = t6;
            _pc = 9;
            continue;
        case 9:
    t2 = t3;
    t0 = (t0 + 1U) & U32MASK;
    t1 = (t1 & 0xFFFF00FFU) | ((uint32_t)M8(t2) << 8);
    t2 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
    c->blk00_dict[((t4 + t0 - 1U) & U32MASK) - c->blk00_base] = (unsigned char)((t1 >> 8) & 0xFFU);
    t0 &= t2;
    t3 = (t3 + 1U) & U32MASK;
     if (t5 == t0) { _pc = 13; continue; }
    t1 = (t1 & 0xFFFFFF00U) | ((t1 - 1U) & 0xFFU);
    if ((t1 & 0xFFU) != 0U) { _pc = 9; continue; }
    { _pc = 13; continue; }
        case 10:
    t1 = (t1 & 0xFFFFFF00U) | ((t1 - (uint32_t)D3U_FLAG_444EA7_8(&c->st, 0)) & 0xFFU);
    t3 = t6;
    t1 = (t1 & 0xFFFFFF00U) | ((~t1) & 0xFFU);
    c->blk00_dict[((t3 + t0) & U32MASK) - c->blk00_base] = (unsigned char)(t1 & 0xFFU);
    t0 = t2;
    t2 &= 0xFFFFFF00U;
    D3U_FLAG_444EA7_8(&c->st, 0) = (unsigned char)(t2 & 0xFFU);
    { _pc = 13; continue; }
        case 11:
    t0 = (t0 & 0xFFFFFF00U) | (t3 & 0xFFU);
            _pc = 12;
            continue;
        case 12:
    t3 = v1c;
    t0 = (t0 & 0xFFFFFF00U) | ((~t0) & 0xFFU);
    c->blk00_dict[t3 - c->blk00_base] = (unsigned char)(t0 & 0xFFU);
    t0 = t2;
            _pc = 13;
            continue;
        case 13:
     if (t5 == t0) { _pc = 14; continue; }
    t4 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
    t5 = (t5 + 1U) & U32MASK;
    t3 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
    t5 &= t4;
     if (t5 != t3) { _pc = 3; continue; }
            _pc = 14;
            continue;
        case 14:
    t3 = (uint32_t)D3U_TBL_444280_32(&c->st, 0);
    t1 &= 0xFFFF00FFU;
    t4 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
    D3U_FLAG_444EA7_8(&c->st, 0) = (unsigned char)((t1 >> 8) & 0xFFU);
    t3 = (t3 + t0) & U32MASK;
    t1 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
    t3 = (t3 - t4) & U32MASK;
    t5 = 1;
    t3 &= t1;
    D3U_DICT_CURSOR_32(&c->st, 0) = (unsigned int)t0;
    D3U_TBL_444280_32(&c->st, 0) = (unsigned int)t3;
    t3 = 0;
            _pc = 15;
            continue;
        case 15:
    t1 = t0;
    t1 &= 0xFFU;
    t2 = (t0 + t6) & U32MASK;
    D3U_TBL_443D08_32(&c->st, t1 * 4U) = (unsigned int)t5;
    t1 = (t1 & 0xFFFFFF00U) | (uint32_t)stk[t3];
    t0 = (t0 + t5) & U32MASK;
    c->blk00_dict[t2 - c->blk00_base] = (unsigned char)(t1 & 0xFFU);
    t1 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
    t3 = (t3 + t5) & U32MASK;
    t0 &= t1;
     if (t3 < 0x100U) { _pc = 15; continue; } /* unsigned */
    t0 = v28;
    t3 = 0x40U;
    D3U_BLK00_DICT_MASTER_32(&c->st, 0) = (unsigned int)t6;
            _pc = 16;
            continue;
        case 16:
    t2 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
    t6 = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
    t0 = (t0 - 1U) & U32MASK;
    t4 = t6;
    t0 &= t2;
    c->blk00_dict[((t4 + t0) & U32MASK) - c->blk00_base] = 0;
    t3 = (t3 - 1U) & U32MASK;
    if (t3 != 0U) { _pc = 16; continue; }
    t2 = 0x100U;
    t5 = (uint32_t)D3U_DICT_SIZE_32(&c->st, 0);
    t1 = 0x200000U;
    t5 = (t5 + t6) & U32MASK;
    t3 = 0xFFU;
    ;
    /* t4 is stale = t6 here (no t4 write since zero loop); 256B copy t4->t5 */
    t0 = t2;
    t2 = (t2 - t5) & U32MASK;
    t2 = (t2 - t0) & U32MASK;
    t2 &= 3U;
    t0 = (t0 - t2) & U32MASK;
    if ((int)(t0 & U32MASK) > 0) { /* signed */
        while (t2 != 0) { c->blk00_dict[t5 - c->blk00_base] = c->blk00_dict[t4 - c->blk00_base]; t4 = (t4 + 1U) & U32MASK; t5 = (t5 + 1U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
        t2 = t0;
        t0 &= 3U;
        t2 >>= 2;
        while (t2 != 0) { (*(unsigned int *)(void *)(c->blk00_dict + (t5 - c->blk00_base))) = (*(unsigned int *)(void *)(c->blk00_dict + (t4 - c->blk00_base))); t4 = (t4 + 4U) & U32MASK; t5 = (t5 + 4U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
    }
            _pc = 17;
            continue;
        case 17:
    t2 = (t2 + t0) & U32MASK;
    while (t2 != 0) { c->blk00_dict[t5 - c->blk00_base] = c->blk00_dict[t4 - c->blk00_base]; t4 = (t4 + 1U) & U32MASK; t5 = (t5 + 1U) & U32MASK; t2 = (t2 - 1U) & U32MASK; }
    d3uc_rescale_blk01_model(c);
    d3uc_rescale_blk02_model(c);
    t0 = (uint32_t)(uint32_t)D3U_BLK03_MODEL_A_FF_32(&c->st, 0);
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3);
    t4 = 0;
    t1 = 0x200000U;
    t3 = 0xFFU;
    t0 = (uint32_t)(uint32_t)D3U_BLK04_MODEL_B_FF_32(&c->st, 0);
    t2 &= 0xFFFFFF00U;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3);
    t0 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
    t3 = (uint32_t)D3U_DICT_SIZE_32(&c->st, 0);
    D3U_TBL_444258_32(&c->st, 0) = (unsigned int)t4;
    D3U_TBL_44425C_32(&c->st, 0) = (unsigned int)t4;
    D3U_TBL_444260_32(&c->st, 0) = (unsigned int)t4;
    D3U_FLAG_444EB0_8(&c->st, 0) = (unsigned char)(t2 & 0xFFU);
    t2 &= 0xFFFF00FFU;
    t0 = (t0 + t3) & U32MASK;
    D3U_FLAG_444EB1_8(&c->st, 0) = (unsigned char)((t2 >> 8) & 0xFFU);
    t2 = v28;
    t4 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
    t0 = (t0 - t2) & U32MASK;
    t0 &= t4;
     if (t0 > 0x8000U) { /* unsigned */
    t4 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
    t5 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
    t4 = (t4 + 0xFFFF8000U) & U32MASK;
    t6 = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
    t4 &= t5;
    { _pc = 18; continue; }
    }
    t4 = 0xFFFFFFFFU;
    t6 = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
            _pc = 18;
            continue;
        case 18:
    t0 = (uint32_t)D3U_DICT_CURSOR_32(&c->st, 0);
    D3U_BLK00_DICT_MASTER_32(&c->st, 0) = (unsigned int)t6;
     if (t2 == t0) { _pc = 25; continue; }
    t0 = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    t3 = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    t0 = (t0 >> 13) & U32MASK;
    t0 ^= t3;
    t0 &= 0x3FFFFU;
    t3 = (uint32_t)(uint32_t)D3U_BLK03_MODEL_A_FF_32(&c->st, 0);
    t0 = (t0 << 3) & U32MASK;
    t3 = (t3 + t0) & U32MASK;
    t0 = (uint32_t)D3U_TBL_44425C_32(&c->st, 0);
    t1 = (uint32_t)D3U_TBL_44425C_32(&c->st, 0);
    t0 = (t0 >> 15) & U32MASK;
    t1 ^= t0;
    t0 = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    t5 = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    t0 = (t0 >> 15) & U32MASK;
    t1 = (t1 << 4) & U32MASK;
    t0 ^= t5;
    t1 ^= t0;
    t0 = (uint32_t)D3U_TBL_444260_32(&c->st, 0);
    t5 = (uint32_t)D3U_TBL_444260_32(&c->st, 0);
    t0 = (t0 >> 15) & U32MASK;
    t0 ^= t5;
    t0 = (t0 << 6) & U32MASK;
    t1 ^= t0;
    t1 &= 0x3FFFFU;
    t0 = (t1 * 8U) & U32MASK;
    t1 = (uint32_t)(uint32_t)D3U_BLK04_MODEL_B_FF_32(&c->st, 0);
    t0 = (t0 + 4U) & U32MASK;
    t0 = (t0 + t1) & U32MASK;
    t1 = (uint32_t)d3u_ld32(c, (uint32_t)((t0 - 4U) & U32MASK));
    d3u_st32(c, (uint32_t)(t0), (unsigned int)t1);
    d3u_st32(c, (uint32_t)((t0 - 4U) & U32MASK), (unsigned int)t2);
    t0 = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    d3u_st32(c, (uint32_t)(t3), (unsigned int)t2);
    d3u_st32(c, (uint32_t)((t3 + 4U) & U32MASK), (unsigned int)t0);
     if (t2 != t4) { _pc = 19; continue; }
    t4 = 0xFFFFFFFFU;
            _pc = 19;
            continue;
        case 19:
     if (t4 != 0xFFFFFFFFU) { _pc = 24; continue; }
    t0 = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    t3 = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    D3U_TBL_444240_32(&c->st, 0) = (unsigned int)t0;
    t0 = (t0 >> 13) & U32MASK;
    t0 ^= t3;
    t0 &= 0x7FFFU;
    D3U_TBL_444240_32(&c->st, 0) = (unsigned int)t3;
    D3U_TBL_444224_32(&c->st, 0) = (unsigned int)t0;
     if (t0 >= 0x40000U) { _pc = 23; continue; } /* unsigned; DEAD: t0<=7FFFh after and */
    t0 = (t0 << 5) & U32MASK;
            _pc = 20;
            continue;
        case 20:
    t3 = (uint32_t)(uint32_t)D3U_BLK01_DICT_COPY_32(&c->st, 0);
    t3 = (t3 + t0) & U32MASK;
     if (M8(t3) == 0) { _pc = 22; continue; }
    t5 = (uint32_t)D3U_TBL_444240_32(&c->st, 0);
     if (t5 != (uint32_t)d3u_ld32(c, (uint32_t)((t3 + 0x1CU) & U32MASK))) { _pc = 22; continue; }
    t0 = 0;
    t1 = (t1 & 0xFFFF00FFU) | 0xFF00U;
    t0 = (t0 & 0xFFFFFF00U) | (uint32_t)M8(t3);
    D3U_FLAG_444EB7_8(&c->st, 0) = (unsigned char)((t1 >> 8) & 0xFFU);
    D3U_TBL_444214_32(&c->st, 0) = (unsigned int)t0;
    t0 = t3;
    t1 = (t3 + 0xDU) & U32MASK;
            _pc = 21;
            continue;
        case 21:
    t3 = 0;
    t5 = (uint32_t)D3U_TBL_444214_32(&c->st, 0);
    t3 = (uint32_t)M8((t0 + 1U) & U32MASK);
    t5 = (t5 + t3) & U32MASK;
    t0 = (t0 + 1U) & U32MASK;
    D3U_TBL_444214_32(&c->st, 0) = (unsigned int)t5;
     if (t0 == t1) { _pc = 23; continue; }
    { _pc = 21; continue; }
        case 22:
    t5 = (uint32_t)D3U_TBL_444224_32(&c->st, 0);
    t5 = (t5 + 0x8000U) & U32MASK;
    t0 = (t0 + 0x100000U) & U32MASK;
    D3U_TBL_444224_32(&c->st, 0) = (unsigned int)t5;
     if (t5 < 0x40000U) { _pc = 20; continue; } /* unsigned */
            _pc = 23;
            continue;
        case 23:
    t0 = t6;
    t0 = (t0 & 0xFFFFFF00U) | (uint32_t)c->blk00_dict[((t2 + t0) & U32MASK) - c->blk00_base];
    t0 &= 0xFFU;
    D3U_BLK00_DICT_MASTER_32(&c->st, 0) = (unsigned int)t6;
    d3uc_update_order_model_a(c, (unsigned int)t0);
            _pc = 24;
            continue;
        case 24:
    t0 = (uint32_t)D3U_TBL_444260_32(&c->st, 0);
    t3 = (uint32_t)D3U_TBL_44425C_32(&c->st, 0);
    t0 = (t0 << 8) & U32MASK;
    t3 = (t3 >> 24) & U32MASK;
    t0 = (t0 + t3) & U32MASK;
    t3 = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    D3U_TBL_444260_32(&c->st, 0) = (unsigned int)t0;
    t0 = (uint32_t)D3U_TBL_44425C_32(&c->st, 0);
    t3 = (t3 >> 24) & U32MASK;
    t0 = (t0 << 8) & U32MASK;
    t0 = (t0 + t3) & U32MASK;
    D3U_TBL_44425C_32(&c->st, 0) = (unsigned int)t0;
    t0 = t6;
    t0 = (t0 & 0xFFFFFF00U) | (uint32_t)c->blk00_dict[((t2 + t0) & U32MASK) - c->blk00_base];
    t3 = (uint32_t)D3U_TBL_444258_32(&c->st, 0);
    t0 &= 0xFFU;
    t3 = (t3 << 8) & U32MASK;
    t2 = (t2 + 1U) & U32MASK;
    t3 = (t3 + t0) & U32MASK;
    t0 = (uint32_t)D3U_DICT_MASK_32(&c->st, 0);
    D3U_TBL_444258_32(&c->st, 0) = (unsigned int)t3;
    t2 &= t0;
    { _pc = 18; continue; }
        case 25:
    t0 = v24;
    D3U_TBL_444250_32(&c->st, 0) = (unsigned int)t0;
    t0 = v2c;
    D3U_TBL_444220_32(&c->st, 0) = (unsigned int)t0;
    t0 = v20;
    t1 = 0x400U;
    D3U_TBL_444264_32(&c->st, 0) = (unsigned int)t0;
    t0 = (uint32_t)(uint32_t)D3U_BLK14_CTX_CODER_A_32(&c->st, 0);
    t3 = 0;
    t0 = (t0 + 0x5F600U) & U32MASK;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3);
    t1 = 0x400U;
    t0 = (uint32_t)(uint32_t)D3U_BLK15_CTX_CODER_B_32(&c->st, 0);
    t3 = 0;
    t0 = (t0 + 0x5F600U) & U32MASK;
    d3uc_fill(c, (unsigned int)t1, (unsigned int)t0, (unsigned int)t3);
    t0 = (uint32_t)(uint32_t)D3U_BLK17_SMALL_B_32(&c->st, 0);
    d3u_st16(c, (uint32_t)((t0 + 0x5F6U) & U32MASK), (unsigned short)1);
    d3u_st16(c, (uint32_t)((t0 + 0x5F8U) & U32MASK), (unsigned short)1);
    if (D3U_FLAG_444E99_8(&c->st, 0) != 1) { _pc = 26; continue; }
    t0 = (uint32_t)D3U_TBL_444268_32(&c->st, 0);
    { _pc = 27; continue; }
        case 26:
    t0 = (uint32_t)D3U_OUT_TOTAL_POS_32(&c->st, 0);
            _pc = 27;
            continue;
        case 27:
    t6 = (uint32_t)(uint32_t)D3U_BLK00_DICT_MASTER_32(&c->st, 0);
    D3U_TBL_444230_32(&c->st, 0) = (unsigned int)t0;
    t3 = 0xFFFFFFFFU;
    t2 = (t2 & 0xFFFFFF00U) | 1U;
    D3U_PRIMER_TOTAL_32(&c->st, 0) = (unsigned int)t3;
    D3U_FLAG_444EA0_8(&c->st, 0) = (unsigned char)(t2 & 0xFFU);
    D3U_BLK00_DICT_MASTER_32(&c->st, 0) = (unsigned int)t6;
    /* epilogue: add esp,114h; pop t6,t5,t4,t3,t2,t1; retn */
        default:
            break;
        }
        break;
    }
}
/* Const table lifted from the DGROUP slice 0x42C9B0 (512 bytes): the
 * source of the rep movsb/movsd copy at 0x41491D.  The code only reads
 * it and copies exactly 0x200 bytes from the table base. */
const unsigned char d3u_t42C9B0[512] = {
    0x00,0x01,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x10,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x20,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x10,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x40,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x10,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x20,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x10,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x80,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x10,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x20,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x10,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x40,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x10,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x20,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x10,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00,
    0x08,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x04,0x00,0x01,0x00,0x02,0x00,0x01,0x00
};
/* Const table lifted from the DGROUP slice 0x429A08 (384 bytes): the
 * source of the rep movsb/movsd copy at 0x4149B2.  The code only reads
 * it and copies exactly 0x180 bytes from the table base. */
const unsigned char d3u_t429A08[384] = {
    0x00,0x00,0x01,0x00,0x00,0x04,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x10,
    0x00,0x00,0x01,0x00,0x80,0x02,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x10,
    0x00,0x00,0x01,0x00,0x00,0x03,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x10,
    0x00,0x00,0x01,0x00,0x80,0x02,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x10,
    0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x10,
    0x00,0x00,0x01,0x00,0x40,0x01,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x10,
    0x00,0x00,0x01,0x00,0x80,0x01,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x10,
    0x00,0x00,0x01,0x00,0x40,0x01,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x10,
    0x00,0x00,0x01,0x00,0x00,0x08,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x04,0x00,0x10,
    0x00,0x00,0x01,0x00,0x00,0x05,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x04,0x00,0x10,
    0x00,0x00,0x01,0x00,0x00,0x06,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x04,0x00,0x10,
    0x00,0x00,0x01,0x00,0x00,0x05,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x04,0x00,0x10,
    0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x10,
    0x00,0x00,0x01,0x00,0x40,0x01,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x10,
    0x00,0x00,0x01,0x00,0x80,0x01,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x10,
    0x00,0x00,0x01,0x00,0x40,0x01,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x10,
    0x00,0x00,0x01,0x00,0x00,0x01,0x00,0x10,0x00,0x00,0x01,0x00,0x80,0x00,0x00,0x10,
    0x00,0x00,0x01,0x00,0xA0,0x00,0x00,0x10,0x00,0x00,0x01,0x00,0x80,0x00,0x00,0x10,
    0x00,0x00,0x01,0x00,0xC0,0x00,0x00,0x10,0x00,0x00,0x01,0x00,0x80,0x00,0x00,0x10,
    0x00,0x00,0x01,0x00,0xA0,0x00,0x00,0x10,0x00,0x00,0x01,0x00,0x80,0x00,0x00,0x10,
    0x00,0x00,0x01,0x00,0x00,0x04,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x10,
    0x00,0x00,0x01,0x00,0x80,0x02,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x10,
    0x00,0x00,0x01,0x00,0x00,0x03,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x10,
    0x00,0x00,0x01,0x00,0x80,0x02,0x00,0x10,0x00,0x00,0x01,0x00,0x00,0x02,0x00,0x10
};
