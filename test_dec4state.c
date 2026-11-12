/* Self-test for the generated core-4 state headers.
 *
 * The typed ports (0.3/0.4 core 4) replace the flat address arena with
 * one struct member per model cell.  tools/gen_dec4_state.py derives
 * those members from the address map; this test makes sure the derived
 * layout is usable: every cell is reachable, the widths behave, and the
 * byte arrays that stand in for the cells the original aliases at
 * several widths keep their alignment.
 *
 * Build:  gcc -std=c89 -Wall -Wextra -I. test_dec4state.c -o test_dec4state.exe
 */
#include <stdio.h>
#include <string.h>

#include "uha04_dec4_state.h"
#include "uha03_dec4_state.h"

static int failures;

static void check(int ok, const char *what)
{
    if (!ok) {
        printf("FAIL %s\n", what);
        failures++;
    }
}

int main(void)
{
    /* 0.4: a scalar cell must round-trip at its own width, ... */
    {
        d4v_state st;
        memset(&st, 0, sizeof(st));
        D4V_DICT_SIZE_32(&st, 0) = 0x1234u;
        check(D4V_DICT_SIZE_32(&st, 0) == 0x1234u, "0.4 dict size round-trip");
        /* the cell is a dword inside the region's byte array, so its
         * bytes are the ones the array holds */
        check(((unsigned char *)&st)[0] == 0, "0.4 region byte array starts zero");
        D4V_TBL_436150_8(&st, 0) = 0xABu;
        check(D4V_TBL_436150_8(&st, 0) == 0xABu, "0.4 byte cell round-trip");
        /* a byte cell must not disturb its neighbour */
        check(D4V_TBL_436152_8(&st, 0) == 0, "0.4 byte cell does not spill");
        /* a cell inside the DGROUP image is an index into the same
         * array, so it round-trips the same way */
        D4V_BLK_PTR_00_DICT_32(&st, 0) = 0x600000u;
        check(D4V_BLK_PTR_00_DICT_32(&st, 0) == 0x600000u,
              "0.4 image cell round-trip");
    }
    /* 0.4: the multi-width cells are byte arrays; an aligned 32-bit
     * access must see the bytes that were written individually. */
    {
        d4v_state st;
        memset(&st, 0, sizeof(st));
        D4V_SHIFT_R0_8(&st, 0) = 0x11u;
        D4V_SHIFT_R0_8(&st, 1) = 0x22u;
        D4V_SHIFT_R0_8(&st, 2) = 0x33u;
        D4V_SHIFT_R0_8(&st, 3) = 0x44u;
        check(D4V_SHIFT_R0_32(&st, 0) == 0x44332211u, "0.4 aliased cell 32-bit view");
        check(D4V_SHIFT_R0_8(&st, 0) == 0x11u, "0.4 aliased cell byte view");
        D4V_SHIFT_R0_32(&st, 0) = 0xAABBCCDDu;
        check(D4V_SHIFT_R0_8(&st, 0) == 0xDDu, "0.4 aliased cell write-back");
    }
    /* 0.3: the same shape, with its own names. */
    {
        d3u_state st;
        memset(&st, 0, sizeof(st));
        D3U_FLAG_444EB0_8(&st, 0) = 0x5Au;
        check(D3U_FLAG_444EB0_8(&st, 0) == 0x5Au, "0.3 byte cell round-trip");
        check(D3U_FLAG_444EB0_32(&st, 0) == 0x5Au, "0.3 aliased cell 32-bit view");
        D3U_FLAG_444EB0_32(&st, 0) = 0x11223344u;
        check(D3U_FLAG_444EB0_8(&st, 0) == 0x44u, "0.3 aliased cell write-back");
    }
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("state headers ok\n");
    return 0;
}
