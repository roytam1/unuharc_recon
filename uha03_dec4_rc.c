#include "uha03_dec4_int.h"

void d3uc_refill_input(dec3w_ctx *c)
{
    unsigned int t0 = 0;
    unsigned int t3 = 0;
    unsigned int cb = 0;
    t0 = D3U_IN_ABS_POS_32(&c->st, 0);
    t3 = 0;
    D3U_TBL_436E40_32(&c->st, 0) = t0;
    D3U_IN_POS_32(&c->st, 0) = t3;
    t0 = D3U_OUT_TOTAL_POS_32(&c->st, 0);
    t3 = D3U_TBL_444EFC_32(&c->st, 0);
    D3U_TBL_436E44_32(&c->st, 0) = t0;
    t0 = (uint32_t)D3U_IN_BUFFER_BASE_32(&c->st, 0);
    cb = (uint32_t)D3U_TBL_42CBB0_32(&c->st, 0);
    (void)cb;
    {
        unsigned int _room = ((t0 & U32MASK) >= (uint32_t)D3U_IN_BUFFER_BASE_32(&c->st, 0))
            ? 0x100000U - ((t0 & U32MASK) - (uint32_t)D3U_IN_BUFFER_BASE_32(&c->st, 0))
            : 0U;
        unsigned int _n = (t3 & U32MASK) < _room ? (t3 & U32MASK) : _room;
        t0 = (c->rcb != 0 && _n != 0)
            ? (unsigned int)c->rcb(c->rctx, d3u_ptr(c, (t0 & U32MASK)), _n) : 0U;
    }
    D3U_IN_AVAIL_32(&c->st, 0) = t0;
    if (t0 == 0)
        D3U_STATUS_ERR_32(&c->st, 0) = 3;
}

unsigned int d3uc_get_byte(dec3w_ctx *c)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t3 = 0;
    t0 = D3U_IN_POS_32(&c->st, 0);
    if (t0 == D3U_IN_AVAIL_32(&c->st, 0))
        d3uc_refill_input(c);
    t1 = D3U_IN_ABS_POS_32(&c->st, 0);
    t1 = (t1 + 1) & U32MASK;
    t0 = D3U_IN_POS_32(&c->st, 0);
    D3U_IN_ABS_POS_32(&c->st, 0) = t1;
    t1 = (t0 + 1) & U32MASK;
    t3 = (uint32_t)D3U_IN_BUFFER_BASE_32(&c->st, 0);
    D3U_IN_POS_32(&c->st, 0) = t1;
    t0 = (t0 & 0xFFFFFF00U) | (uint32_t)M8((t3 + t0) & U32MASK);
    return t0 & U32MASK;
}

void d3uc_fill_input(dec3w_ctx *c, unsigned int eax_v)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int t5 = 0;
    unsigned int cb = 0;
    t2 = D3U_IN_AVAIL_32(&c->st, 0);
    t4 = eax_v;
    t0 = eax_v;
    t3 = D3U_IN_POS_32(&c->st, 0);
    t5 = t2;
    t5 = (t5 - t3) & U32MASK;
    if (t5 < t0) {
        t0 = (uint32_t)D3U_IN_BUFFER_BASE_32(&c->st, 0);
        t1 = t5;
        t2 = t5;
        t3 = (t3 + t0) & U32MASK;
        t5 = 3;
        d3uc_memmove(c, t0, t3, t1);
        t0 = D3U_IN_ABS_POS_32(&c->st, 0);
        t1 = 0;
        D3U_TBL_436E40_32(&c->st, 0) = t0;
        t0 = D3U_OUT_TOTAL_POS_32(&c->st, 0);
        D3U_IN_POS_32(&c->st, 0) = t1;
        D3U_TBL_436E44_32(&c->st, 0) = t0;
        for (;;) {
            t3 = D3U_TBL_444EFC_32(&c->st, 0);
            t0 = (uint32_t)D3U_IN_BUFFER_BASE_32(&c->st, 0);
            t1 = t2;
            t3 = (t3 - t2) & U32MASK;
            t0 = (t0 + t2) & U32MASK;
            D3U_IN_AVAIL_32(&c->st, 0) = t2;
            cb = (uint32_t)D3U_TBL_42CBB0_32(&c->st, 0);
            (void)cb;
            {
                unsigned int _room = ((t0 & U32MASK) >= (uint32_t)D3U_IN_BUFFER_BASE_32(&c->st, 0))
                    ? 0x100000U - ((t0 & U32MASK) - (uint32_t)D3U_IN_BUFFER_BASE_32(&c->st, 0))
                    : 0U;
                unsigned int _n = (t3 & U32MASK) < _room ? (t3 & U32MASK) : _room;
                t0 = (c->rcb != 0 && _n != 0)
                    ? (unsigned int)c->rcb(c->rctx, d3u_ptr(c, (t0 & U32MASK)), _n) : 0U;
            }
            t2 = t0;
            if (t0 == 0)
                D3U_STATUS_ERR_32(&c->st, 0) = t5;
            t2 = (t2 + t1) & U32MASK;
            if (t2 >= t4)
                break;
        }
    }
    D3U_IN_AVAIL_32(&c->st, 0) = t2;
}

unsigned int d3uc_rc_decode(dec3w_ctx *c, unsigned int total)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int t5 = 0;
    t1 = D3U_RC_VALUE_32(&c->st, 0);
    t2 = D3U_RC_RANGE_32(&c->st, 0);
    t4 = total;
    while (t2 <= 0x800000U) {
        t0 = 0;
        t0 = (unsigned int)D3U_RC_CARRY_BYTE_8(&c->st, 0);
        t3 = t0;
        t3 = (t3 << 7) & U32MASK;
        t1 = (t1 << 8) & U32MASK;
        t3 &= 0xFFU;
        t1 |= t3;
        D3U_RC_RANGE_32(&c->st, 0) = t2;
        D3U_RC_VALUE_32(&c->st, 0) = t1;
        t0 = d3uc_get_byte(c);
        t2 = D3U_RC_RANGE_32(&c->st, 0);
        t1 = D3U_RC_VALUE_32(&c->st, 0);
        D3U_RC_CARRY_BYTE_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
        t0 &= 0xFFU;
        t0 = (unsigned int)(((int)t0) >> 1);
        t2 = (t2 << 8) & U32MASK;
        t1 |= t0;
    }
    t4 &= 0xFFFFU;
    t0 = t2;
    t3 = 0;
    t0 = t2 / t4;
    t3 = t2 % t4;
    D3U_RC_SCALE_32(&c->st, 0) = t0;
    t3 = 0;
    t5 = D3U_RC_SCALE_32(&c->st, 0);
    t0 = t1;
    t0 = t1 / t5;
    if (t0 >= t4)
        t0 = (t4 - 1) & U32MASK;
    D3U_RC_RANGE_32(&c->st, 0) = t2;
    D3U_RC_VALUE_32(&c->st, 0) = t1;
    return t0 & U32MASK;
}

void d3uc_rc_update(dec3w_ctx *c, unsigned int p)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t2 = 0;
    unsigned int t3 = 0;
    unsigned int t4 = 0;
    unsigned int t6 = 0;
    t3 = D3U_RC_SCALE_32(&c->st, 0);
    t3 = (t3 * d3u_ld32(c, (uint32_t)(p + 4))) & U32MASK;
    t2 = D3U_RC_VALUE_32(&c->st, 0);
    t4 = d3u_ld32(c, (uint32_t)(p + 0));
    t2 = (t2 - t3) & U32MASK;
    t1 = d3u_ld32(c, (uint32_t)(p + 8));
    D3U_RC_VALUE_32(&c->st, 0) = t2;
    if (t1 < t4) {
        t3 = t1;
        t6 = d3u_ld32(c, (uint32_t)(p + 4));
        t0 = D3U_RC_SCALE_32(&c->st, 0);
        t3 = (t3 - t6) & U32MASK;
        t0 = (t0 * t3) & U32MASK;
        D3U_RC_RANGE_32(&c->st, 0) = t0;
    } else {
        D3U_RC_RANGE_32(&c->st, 0) = (D3U_RC_RANGE_32(&c->st, 0) - t3) & U32MASK;
    }
}

void d3uc_rc_init(dec3w_ctx *c)
{
    unsigned int t0 = 0;
    unsigned int t3 = 0;
    (void)d3uc_get_byte(c);
    t0 = d3uc_get_byte(c);
    D3U_RC_CARRY_BYTE_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    t0 &= 0xFFU;
    t3 = 0x80U;
    t0 = (unsigned int)(((int)t0) >> 1);
    D3U_RC_RANGE_32(&c->st, 0) = t3;
    D3U_RC_VALUE_32(&c->st, 0) = t0;
}

void d3uc_rc_reset(dec3w_ctx *c)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;
    unsigned int t3 = 0;
    t1 = 0x80000000U;
    t3 = 0;
    t0 = (t0 & 0xFFFFFF00U) | (unsigned int)D3U_OUT_TOTAL_LEN_8(&c->st, 0);
    D3U_RC_VALUE_32(&c->st, 0) = t3;
    D3U_RC_CARRY_BYTE_8(&c->st, 0) = (unsigned char)(t0 & 0xFFU);
    t0 = (t0 & 0xFFFF00FFU) | ((t0 & 0xFFU) << 8);
    D3U_RC_RANGE_32(&c->st, 0) = t1;
    t0 = (t0 & 0xFFFF00FFU) | (((~(t0 >> 8)) & 0xFFU) << 8);
    D3U_RC_SCALE_32(&c->st, 0) = t3;
    D3U_RC_CARRY_BYTE_8(&c->st, 0) = (unsigned char)((t0 >> 8) & 0xFFU);
}

void d3uc_rc_renorm_update(dec3w_ctx *c, unsigned int p)
{
    unsigned int eax_ = 0;
    unsigned int ebx_ = 0;
    unsigned int ecx_ = 0;
    unsigned int edx_ = 0;
    unsigned int esi_ = 0;
    unsigned int edi_ = 0;
    unsigned int qv = 0;

    ebx_ = D3U_RC_SCALE_32(&c->st, 0);
    edx_ = D3U_RC_RANGE_32(&c->st, 0);
    ecx_ = p;
    while (edx_ <= 0x800000u) {
        esi_ = 0xFFu;
        for (;;) {
            if (D3U_RC_VALUE_32(&c->st, 0) < 0x7F800000U) {
                eax_ = (unsigned int)D3U_RC_CARRY_BYTE_8(&c->st, 0);
                D3U_RC_SCALE_32(&c->st, 0) = ebx_;
                d3uc_put_byte(c, eax_);
                ebx_ = D3U_RC_SCALE_32(&c->st, 0);
                while (ebx_ != 0u) {
                    eax_ = esi_;
                    D3U_RC_SCALE_32(&c->st, 0) = ebx_;
                    d3uc_put_byte(c, eax_);
                    ebx_ = D3U_RC_SCALE_32(&c->st, 0);
                    ebx_ = (ebx_ - 1u) & U32MASK;
                }
                eax_ = D3U_RC_VALUE_32(&c->st, 0);
                eax_ >>= 23;
                D3U_RC_CARRY_BYTE_8(&c->st, 0) = (unsigned char)(eax_ & 0xFFu);
            } else if ((D3U_RC_VALUE_8(&c->st, 3u) & 0x80u) != 0u) {
                eax_ = (unsigned int)D3U_RC_CARRY_BYTE_8(&c->st, 0);
                eax_ = (eax_ + 1u) & 0xFFu;
                D3U_RC_SCALE_32(&c->st, 0) = ebx_;
                d3uc_put_byte(c, eax_);
                ebx_ = D3U_RC_SCALE_32(&c->st, 0);
                edx_ = 0u;
                while (ebx_ != 0u) {
                    eax_ = edx_;
                    D3U_RC_SCALE_32(&c->st, 0) = ebx_;
                    d3uc_put_byte(c, eax_);
                    ebx_ = D3U_RC_SCALE_32(&c->st, 0);
                    ebx_ = (ebx_ - 1u) & U32MASK;
                }
                eax_ = D3U_RC_VALUE_32(&c->st, 0);
                eax_ >>= 23;
                D3U_RC_CARRY_BYTE_8(&c->st, 0) = (unsigned char)(eax_ & 0xFFu);
            } else {
                ebx_ = (ebx_ + 1u) & U32MASK;
            }
            eax_ = D3U_RC_VALUE_32(&c->st, 0);
            edi_ = D3U_RC_RANGE_32(&c->st, 0);
            eax_ = (eax_ << 8) & U32MASK;
            edi_ = (edi_ << 8) & U32MASK;
            eax_ &= 0x7FFFFFFFU;
            D3U_RC_RANGE_32(&c->st, 0) = edi_;
            D3U_RC_VALUE_32(&c->st, 0) = eax_;
            if (edi_ > 0x800000u)
                break;
        }
        edx_ = D3U_RC_RANGE_32(&c->st, 0);
        if (edx_ > 0x800000u)
            break;
    }
    eax_ = D3U_RC_RANGE_32(&c->st, 0);
    esi_ = d3u_ld32(c, (uint32_t)(ecx_));
    edx_ = 0u;
    qv = eax_;
    eax_ = qv / esi_;
    edx_ = qv % esi_;
    esi_ = d3u_ld32(c, (uint32_t)(ecx_ + 4u));
    esi_ = (esi_ * eax_) & U32MASK;
    edi_ = d3u_ld32(c, (uint32_t)(ecx_));
    edx_ = eax_;
    eax_ = d3u_ld32(c, (uint32_t)(ecx_ + 8u));
    if (eax_ < edi_) {
        eax_ = (eax_ - d3u_ld32(c, (uint32_t)(ecx_ + 4u))) & U32MASK;
        edx_ = (edx_ * eax_) & U32MASK;
        D3U_RC_RANGE_32(&c->st, 0) = edx_;
    } else {
        D3U_RC_RANGE_32(&c->st, 0) = (D3U_RC_RANGE_32(&c->st, 0) - esi_) & U32MASK;
    }
    D3U_RC_VALUE_32(&c->st, 0) = (D3U_RC_VALUE_32(&c->st, 0) + esi_) & U32MASK;
    D3U_RC_SCALE_32(&c->st, 0) = ebx_;
}