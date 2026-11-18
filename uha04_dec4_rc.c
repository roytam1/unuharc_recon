#include "uha04_dec4_int.h"

/* sub_41CD30. One range-coder step for symbol `sym` in bin `sel`.
 * The 8-byte length record at 0x459BA8 holds the cumulative counts;
 * the step scales the range by the chosen bin and shifts bytes out
 * while the range stays below 2^24. */
void d4vc_rc(dec4v_ctx *c, unsigned int sym, unsigned int sel)
{
    unsigned char *g;
    unsigned short s;
    unsigned int symbol;
    unsigned int which;
    unsigned int entry;
    unsigned int mid;
    unsigned int hi;
    unsigned int range;
    unsigned int width;
    unsigned int code_lo;
    unsigned int code_hi;
    unsigned int carry;
    unsigned int pending;
    unsigned int scaled;

    g = c->st.g.bytes;
    symbol = sym & 0xFFU;
    which = sel & 0xFFU;
    entry = D4V_TBL_459BA8 + symbol * 8U;
    s = 0;
    memcpy(&s, g + ((entry + 6U) - D4V_GLOBALS_BASE), 2);
    D4V_RSLOT_LO_32(&c->st, 0) = s;
    entry += which * 2U;
    s = 0;
    memcpy(&s, g + (entry - D4V_GLOBALS_BASE), 2);
    D4V_RSLOT_MID_32(&c->st, 0) = s;
    s = 0;
    memcpy(&s, g + ((entry + 2U) - D4V_GLOBALS_BASE), 2);
    D4V_RSLOT_HI_32(&c->st, 0) = s;

    memcpy(&mid, g + ((D4V_RSLOT_LO + 4U) - D4V_GLOBALS_BASE), 4);
    memcpy(&hi, g + ((D4V_RSLOT_LO + 8U) - D4V_GLOBALS_BASE), 4);
    range = D4V_RC_RANGE_32(&c->st, 0);
    width = hi - mid;
    scaled = mid * range;
    range *= width;
    code_lo = scaled + D4V_RC_CODE_LO_32(&c->st, 0);
    code_hi = D4V_RC_CODE_HI_32(&c->st, 0) + (code_lo < scaled ? 1U : 0U);
    carry = D4V_RC_CARRY_CNT_32(&c->st, 0);

    while (range < 0x1000000UL) {
        /* A high byte of 0xFF with no outstanding carry is deferred.
         * Anything else is emitted, followed by the run of carries. */
        if (code_hi != 0 || code_lo < 0xFF000000UL) {
            pending = D4V_RC_PENDING_B_32(&c->st, 0);
            d4v_put_byte(c, (pending + code_hi) & 0xFFU);
            if (carry != 0) {
                unsigned int fill;
                fill = (code_hi + 0xFFU) & 0xFFU;
                do {
                    d4v_put_byte(c, fill);
                    carry--;
                } while (carry != 0);
            }
            carry--;
            D4V_RC_PENDING_B_32(&c->st, 0) = code_lo >> 24;
        }
        range <<= 8;
        carry++;
        code_lo <<= 8;
        code_hi = 0;
    }

    D4V_RC_RANGE_32(&c->st, 0) = range;
    D4V_RC_CODE_LO_32(&c->st, 0) = code_lo;
    D4V_RC_CODE_HI_32(&c->st, 0) = code_hi;
    D4V_RC_CARRY_CNT_32(&c->st, 0) = carry;
    d4vc_ppm_touch_len(c, symbol, which);
}
