/* UHARC 0.4 core-4 (uha04_dec4.c) arena address names. See DEC4_ADDRMAP.md.
 * Phase 1 naming layer: each #define preserves the exact original
 * 32-bit DGROUP address so G/M macro expansions are unchanged. */
#ifndef UHA04_DEC4_ADDR_H
#define UHA04_DEC4_ADDR_H

#include "uha_types.h"

/* --- range coder, I/O, dict, block pointers (semantic) --- */
#define D4V_DGROUP_BASE ((uint32_t)0x42E000u) /* orig 0x42E000 -- DGROUP image base (memcpy) */
#define D4V_TAIL_GATE_FLAG ((uint32_t)0x431209u) /* orig 0x431209 -- tail-scan gate */
#define D4V_PREFIX_SUM_TMP ((uint32_t)0x43120Cu) /* orig 0x43120C -- running prefix sum -> BA50 */
#define D4V_BLK_PTR_00_DICT ((uint32_t)0x433B40u) /* orig 0x433B40 -- dict+0x100 */
#define D4V_BLK_PTR_01 ((uint32_t)0x433B44u) /* orig 0x433B44 -- 0x800000 fill 0x100000 */
#define D4V_BLK_PTR_02 ((uint32_t)0x433B48u) /* orig 0x433B48 -- 0x140000 */
#define D4V_BLK_PTR_03 ((uint32_t)0x433B4Cu) /* orig 0x433B4C -- 0x200000 fill FF */
#define D4V_BLK_PTR_04 ((uint32_t)0x433B50u) /* orig 0x433B50 -- 0x200000 fill FF */
#define D4V_BLK_PTR_05 ((uint32_t)0x433B54u) /* orig 0x433B54 -- 0x40000 PPM ctx */
#define D4V_BLK_PTR_06 ((uint32_t)0x433B58u) /* orig 0x433B58 -- order table, stride 4 */
#define D4V_BLK_PTR_07 ((uint32_t)0x433B5Cu) /* orig 0x433B5C -- PPM 0x80000 */
#define D4V_BLK_PTR_08 ((uint32_t)0x433B60u) /* orig 0x433B60 -- PPM 0x80000 */
#define D4V_BLK_PTR_09 ((uint32_t)0x433B64u) /* orig 0x433B64 -- PPM 0x40000 */
#define D4V_BLK_PTR_10 ((uint32_t)0x433B68u) /* orig 0x433B68 -- PPM 0x40000 */
#define D4V_BLK_PTR_11 ((uint32_t)0x433B6Cu) /* orig 0x433B6C -- PPM 0x40000 */
#define D4V_BLK_PTR_12 ((uint32_t)0x433B70u) /* orig 0x433B70 -- PPM 0x1C0000 */
#define D4V_BLK_PTR_13 ((uint32_t)0x433B74u) /* orig 0x433B74 -- PPM/emit 0x40000 */
#define D4V_BLK_PTR_14 ((uint32_t)0x433B78u) /* orig 0x433B78 -- PPM u16 model 0x61A00 */
#define D4V_BLK_PTR_15 ((uint32_t)0x433B7Cu) /* orig 0x433B7C -- PPM u16 model 0x61A00 */
#define D4V_BLK_PTR_16 ((uint32_t)0x433B80u) /* orig 0x433B80 -- S4 model 0x0C000 */
#define D4V_BLK_PTR_17 ((uint32_t)0x433B84u) /* orig 0x433B84 -- u16 counts 0x5FE */
#define D4V_BLK_PTR_18 ((uint32_t)0x433B88u) /* orig 0x433B88 -- S4 model 0x0C0 */
#define D4V_BLK_PTR_19 ((uint32_t)0x433B8Cu) /* orig 0x433B8C -- S4 model 0x0C0 */
#define D4V_IO_ERR ((uint32_t)0x436EC8u) /* orig 0x436EC8 -- sticky 3 EOF / 6 emit */
#define D4V_IO_IN_PTR ((uint32_t)0x436ED0u) /* orig 0x436ED0 -- in-buffer base (=p_inb) */
#define D4V_IO_OUT_PTR ((uint32_t)0x436ED4u) /* orig 0x436ED4 -- out-buffer base (=p_outb) */
#define D4V_TBL_PRICE_SRC ((uint32_t)0x437966u) /* orig 0x437966 -- price src (BSS-zero) */
#define D4V_TBL_PRICE_SUM ((uint32_t)0x437968u) /* orig 0x437968 -- price prefix sums */
#define D4V_PRICE_DONE ((uint32_t)0x43F968u) /* orig 0x43F968 -- one-time flag */
#define D4V_IO_SNAP_IN ((uint32_t)0x440E70u) /* orig 0x440E70 -- refill snapshot in */
#define D4V_IO_SNAP_OUT ((uint32_t)0x440E74u) /* orig 0x440E74 -- refill snapshot out */
#define D4V_ALLOC_CURSOR ((uint32_t)0x440E78u) /* orig 0x440E78 -- bump cursor */
#define D4V_ALLOC_HI16 ((uint32_t)0x440E7Au) /* orig 0x440E7A -- cursor hi (==440E78+2) */
#define D4V_TBL_NEG_44D920 ((uint32_t)0x44D920u) /* orig 0x44D920 -- neg/identity -x/x */
#define D4V_RSLOT_LO ((uint32_t)0x45AF68u) /* orig 0x45AF68 -- range slot lo */
#define D4V_RSLOT_MID ((uint32_t)0x45AF6Cu) /* orig 0x45AF6C -- range slot mid */
#define D4V_RSLOT_HI ((uint32_t)0x45AF70u) /* orig 0x45AF70 -- range slot hi */
#define D4V_FILETAB_PTR ((uint32_t)0x45AFB0u) /* orig 0x45AFB0 -- file-boundary tab */
#define D4V_SHIFT_R0 ((uint32_t)0x45BA18u) /* orig 0x45BA18 -- LZP shift reg 0 */
#define D4V_SHIFT_R1 ((uint32_t)0x45BA1Cu) /* orig 0x45BA1C -- LZP shift reg 1 */
#define D4V_SHIFT_R2 ((uint32_t)0x45BA20u) /* orig 0x45BA20 -- LZP shift reg 2 */
#define D4V_DICT_SIZE ((uint32_t)0x45BA30u) /* orig 0x45BA30 -- dict size 1<<(10+m%16) */
#define D4V_DICT_CURSOR ((uint32_t)0x45BA34u) /* orig 0x45BA34 -- dict cursor (36x in lzp) */
#define D4V_DICT_MASK ((uint32_t)0x45BA3Cu) /* orig 0x45BA3C -- dict mask size-1 */
#define D4V_LZP_TRIGGER ((uint32_t)0x45BA50u) /* orig 0x45BA50 -- 45CD28>= -> lzp (=43120C) */
#define D4V_IO_OUT_CAP ((uint32_t)0x45CD10u) /* orig 0x45CD10 -- out-cap 65536 flush thr */
#define D4V_IO_OUT_LEN ((uint32_t)0x45CD14u) /* orig 0x45CD14 -- out_len terminator */
#define D4V_IO_IN_POS ((uint32_t)0x45CD18u) /* orig 0x45CD18 -- in-buffer pos */
#define D4V_IO_OUT_POS ((uint32_t)0x45CD1Cu) /* orig 0x45CD1C -- out-buffer pos */
#define D4V_IO_IN_VALID ((uint32_t)0x45CD20u) /* orig 0x45CD20 -- bytes valid in inb */
#define D4V_IO_IN_TOTAL ((uint32_t)0x45CD24u) /* orig 0x45CD24 -- total consumed */
#define D4V_IO_OUT_TOTAL ((uint32_t)0x45CD28u) /* orig 0x45CD28 -- total produced */
#define D4V_IO_IN_CAP ((uint32_t)0x45CD2Cu) /* orig 0x45CD2C -- read size 65536 */
#define D4V_RC_CARRY_CNT ((uint32_t)0x45D0B0u) /* orig 0x45D0B0 -- FF run count (esi) */
#define D4V_RC_CODE_LO ((uint32_t)0x45D0B4u) /* orig 0x45D0B4 -- 64-bit code lo */
#define D4V_RC_CODE ((uint32_t)0x45D0B8u) /* orig 0x45D0B8 -- decode code reg */
#define D4V_RC_RANGE ((uint32_t)0x45D0BCu) /* orig 0x45D0BC -- decode range (0xFFFFFFFF) */
#define D4V_RC_PENDING_B ((uint32_t)0x45D0C0u) /* orig 0x45D0C0 -- pending byte >>24 */
#define D4V_RC_CODE_HI ((uint32_t)0x45D0C4u) /* orig 0x45D0C4 -- 64-bit code hi */

/* --- remaining arena cells (generic names; do NOT guess semantics) --- */
#define D4V_TBL_433B90 ((uint32_t)0x433B90u) /* orig 0x433B90 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_433BA4 ((uint32_t)0x433BA4u) /* orig 0x433BA4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_433BE8 ((uint32_t)0x433BE8u) /* orig 0x433BE8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_433E77 ((uint32_t)0x433E77u) /* orig 0x433E77 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_433E8C ((uint32_t)0x433E8Cu) /* orig 0x433E8C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_433FCC ((uint32_t)0x433FCCu) /* orig 0x433FCC -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_43404C ((uint32_t)0x43404Cu) /* orig 0x43404C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_43407C ((uint32_t)0x43407Cu) /* orig 0x43407C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_43408C ((uint32_t)0x43408Cu) /* orig 0x43408C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_43409C ((uint32_t)0x43409Cu) /* orig 0x43409C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_43419C ((uint32_t)0x43419Cu) /* orig 0x43419C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_43429C ((uint32_t)0x43429Cu) /* orig 0x43429C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_4342C4 ((uint32_t)0x4342C4u) /* orig 0x4342C4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_4342EC ((uint32_t)0x4342ECu) /* orig 0x4342EC -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_434314 ((uint32_t)0x434314u) /* orig 0x434314 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_43431E ((uint32_t)0x43431Eu) /* orig 0x43431E -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_434328 ((uint32_t)0x434328u) /* orig 0x434328 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_434338 ((uint32_t)0x434338u) /* orig 0x434338 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_434438 ((uint32_t)0x434438u) /* orig 0x434438 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_434538 ((uint32_t)0x434538u) /* orig 0x434538 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_434550 ((uint32_t)0x434550u) /* orig 0x434550 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_434D50 ((uint32_t)0x434D50u) /* orig 0x434D50 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_435150 ((uint32_t)0x435150u) /* orig 0x435150 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_435550 ((uint32_t)0x435550u) /* orig 0x435550 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_435950 ((uint32_t)0x435950u) /* orig 0x435950 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_435D50 ((uint32_t)0x435D50u) /* orig 0x435D50 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_436150 ((uint32_t)0x436150u) /* orig 0x436150 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_436152 ((uint32_t)0x436152u) /* orig 0x436152 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_4362C0 ((uint32_t)0x4362C0u) /* orig 0x4362C0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_436450 ((uint32_t)0x436450u) /* orig 0x436450 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_43691C ((uint32_t)0x43691Cu) /* orig 0x43691C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_436976 ((uint32_t)0x436976u) /* orig 0x436976 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_436B8A ((uint32_t)0x436B8Au) /* orig 0x436B8A -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_436ED8 ((uint32_t)0x436ED8u) /* orig 0x436ED8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451920 ((uint32_t)0x451920u) /* orig 0x451920 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451924 ((uint32_t)0x451924u) /* orig 0x451924 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451928 ((uint32_t)0x451928u) /* orig 0x451928 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45192C ((uint32_t)0x45192Cu) /* orig 0x45192C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451930 ((uint32_t)0x451930u) /* orig 0x451930 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451934 ((uint32_t)0x451934u) /* orig 0x451934 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451938 ((uint32_t)0x451938u) /* orig 0x451938 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45193C ((uint32_t)0x45193Cu) /* orig 0x45193C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451B3C ((uint32_t)0x451B3Cu) /* orig 0x451B3C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451D3C ((uint32_t)0x451D3Cu) /* orig 0x451D3C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451D40 ((uint32_t)0x451D40u) /* orig 0x451D40 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451D44 ((uint32_t)0x451D44u) /* orig 0x451D44 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451D48 ((uint32_t)0x451D48u) /* orig 0x451D48 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451D4C ((uint32_t)0x451D4Cu) /* orig 0x451D4C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451D50 ((uint32_t)0x451D50u) /* orig 0x451D50 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_451D54 ((uint32_t)0x451D54u) /* orig 0x451D54 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_452154 ((uint32_t)0x452154u) /* orig 0x452154 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453560 ((uint32_t)0x453560u) /* orig 0x453560 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453564 ((uint32_t)0x453564u) /* orig 0x453564 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453568 ((uint32_t)0x453568u) /* orig 0x453568 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45356C ((uint32_t)0x45356Cu) /* orig 0x45356C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453570 ((uint32_t)0x453570u) /* orig 0x453570 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453574 ((uint32_t)0x453574u) /* orig 0x453574 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453578 ((uint32_t)0x453578u) /* orig 0x453578 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45357C ((uint32_t)0x45357Cu) /* orig 0x45357C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453580 ((uint32_t)0x453580u) /* orig 0x453580 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453584 ((uint32_t)0x453584u) /* orig 0x453584 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453588 ((uint32_t)0x453588u) /* orig 0x453588 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45358C ((uint32_t)0x45358Cu) /* orig 0x45358C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453590 ((uint32_t)0x453590u) /* orig 0x453590 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453594 ((uint32_t)0x453594u) /* orig 0x453594 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453598 ((uint32_t)0x453598u) /* orig 0x453598 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45375C ((uint32_t)0x45375Cu) /* orig 0x45375C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_453760 ((uint32_t)0x453760u) /* orig 0x453760 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_455F60 ((uint32_t)0x455F60u) /* orig 0x455F60 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_456F78 ((uint32_t)0x456F78u) /* orig 0x456F78 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_457360 ((uint32_t)0x457360u) /* orig 0x457360 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_457364 ((uint32_t)0x457364u) /* orig 0x457364 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_457368 ((uint32_t)0x457368u) /* orig 0x457368 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45736C ((uint32_t)0x45736Cu) /* orig 0x45736C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_457370 ((uint32_t)0x457370u) /* orig 0x457370 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_457374 ((uint32_t)0x457374u) /* orig 0x457374 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_457378 ((uint32_t)0x457378u) /* orig 0x457378 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_458778 ((uint32_t)0x458778u) /* orig 0x458778 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_458788 ((uint32_t)0x458788u) /* orig 0x458788 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_458794 ((uint32_t)0x458794u) /* orig 0x458794 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_458798 ((uint32_t)0x458798u) /* orig 0x458798 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45879C ((uint32_t)0x45879Cu) /* orig 0x45879C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_458B94 ((uint32_t)0x458B94u) /* orig 0x458B94 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_458B98 ((uint32_t)0x458B98u) /* orig 0x458B98 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_458B9C ((uint32_t)0x458B9Cu) /* orig 0x458B9C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_458F94 ((uint32_t)0x458F94u) /* orig 0x458F94 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_458F98 ((uint32_t)0x458F98u) /* orig 0x458F98 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459398 ((uint32_t)0x459398u) /* orig 0x459398 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45939C ((uint32_t)0x45939Cu) /* orig 0x45939C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_4593A0 ((uint32_t)0x4593A0u) /* orig 0x4593A0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_4597A0 ((uint32_t)0x4597A0u) /* orig 0x4597A0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459BA0 ((uint32_t)0x459BA0u) /* orig 0x459BA0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459BA4 ((uint32_t)0x459BA4u) /* orig 0x459BA4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459BA8 ((uint32_t)0x459BA8u) /* orig 0x459BA8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459E38 ((uint32_t)0x459E38u) /* orig 0x459E38 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459E3C ((uint32_t)0x459E3Cu) /* orig 0x459E3C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459E3E ((uint32_t)0x459E3Eu) /* orig 0x459E3E -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459E40 ((uint32_t)0x459E40u) /* orig 0x459E40 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459E42 ((uint32_t)0x459E42u) /* orig 0x459E42 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459E44 ((uint32_t)0x459E44u) /* orig 0x459E44 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459E48 ((uint32_t)0x459E48u) /* orig 0x459E48 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459E4C ((uint32_t)0x459E4Cu) /* orig 0x459E4C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459E50 ((uint32_t)0x459E50u) /* orig 0x459E50 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459E54 ((uint32_t)0x459E54u) /* orig 0x459E54 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_459E58 ((uint32_t)0x459E58u) /* orig 0x459E58 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45A258 ((uint32_t)0x45A258u) /* orig 0x45A258 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45A25D ((uint32_t)0x45A25Du) /* orig 0x45A25D -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45A25F ((uint32_t)0x45A25Fu) /* orig 0x45A25F -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45A267 ((uint32_t)0x45A267u) /* orig 0x45A267 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45A268 ((uint32_t)0x45A268u) /* orig 0x45A268 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45A270 ((uint32_t)0x45A270u) /* orig 0x45A270 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45A272 ((uint32_t)0x45A272u) /* orig 0x45A272 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45A278 ((uint32_t)0x45A278u) /* orig 0x45A278 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45A27A ((uint32_t)0x45A27Au) /* orig 0x45A27A -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45A27C ((uint32_t)0x45A27Cu) /* orig 0x45A27C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45A27E ((uint32_t)0x45A27Eu) /* orig 0x45A27E -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45A280 ((uint32_t)0x45A280u) /* orig 0x45A280 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45A281 ((uint32_t)0x45A281u) /* orig 0x45A281 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AA7A ((uint32_t)0x45AA7Au) /* orig 0x45AA7A -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AA7C ((uint32_t)0x45AA7Cu) /* orig 0x45AA7C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AA7E ((uint32_t)0x45AA7Eu) /* orig 0x45AA7E -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AA80 ((uint32_t)0x45AA80u) /* orig 0x45AA80 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AA82 ((uint32_t)0x45AA82u) /* orig 0x45AA82 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AC78 ((uint32_t)0x45AC78u) /* orig 0x45AC78 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AC7A ((uint32_t)0x45AC7Au) /* orig 0x45AC7A -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AC7C ((uint32_t)0x45AC7Cu) /* orig 0x45AC7C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AC7E ((uint32_t)0x45AC7Eu) /* orig 0x45AC7E -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AE80 ((uint32_t)0x45AE80u) /* orig 0x45AE80 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF00 ((uint32_t)0x45AF00u) /* orig 0x45AF00 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF04 ((uint32_t)0x45AF04u) /* orig 0x45AF04 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF08 ((uint32_t)0x45AF08u) /* orig 0x45AF08 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF0C ((uint32_t)0x45AF0Cu) /* orig 0x45AF0C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF10 ((uint32_t)0x45AF10u) /* orig 0x45AF10 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF14 ((uint32_t)0x45AF14u) /* orig 0x45AF14 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF28 ((uint32_t)0x45AF28u) /* orig 0x45AF28 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF2C ((uint32_t)0x45AF2Cu) /* orig 0x45AF2C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF30 ((uint32_t)0x45AF30u) /* orig 0x45AF30 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF34 ((uint32_t)0x45AF34u) /* orig 0x45AF34 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF38 ((uint32_t)0x45AF38u) /* orig 0x45AF38 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF3C ((uint32_t)0x45AF3Cu) /* orig 0x45AF3C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF40 ((uint32_t)0x45AF40u) /* orig 0x45AF40 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF44 ((uint32_t)0x45AF44u) /* orig 0x45AF44 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF48 ((uint32_t)0x45AF48u) /* orig 0x45AF48 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF4C ((uint32_t)0x45AF4Cu) /* orig 0x45AF4C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF50 ((uint32_t)0x45AF50u) /* orig 0x45AF50 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF54 ((uint32_t)0x45AF54u) /* orig 0x45AF54 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF58 ((uint32_t)0x45AF58u) /* orig 0x45AF58 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF5C ((uint32_t)0x45AF5Cu) /* orig 0x45AF5C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF60 ((uint32_t)0x45AF60u) /* orig 0x45AF60 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF64 ((uint32_t)0x45AF64u) /* orig 0x45AF64 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF74 ((uint32_t)0x45AF74u) /* orig 0x45AF74 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF78 ((uint32_t)0x45AF78u) /* orig 0x45AF78 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF7C ((uint32_t)0x45AF7Cu) /* orig 0x45AF7C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF80 ((uint32_t)0x45AF80u) /* orig 0x45AF80 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF84 ((uint32_t)0x45AF84u) /* orig 0x45AF84 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF88 ((uint32_t)0x45AF88u) /* orig 0x45AF88 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF8C ((uint32_t)0x45AF8Cu) /* orig 0x45AF8C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF90 ((uint32_t)0x45AF90u) /* orig 0x45AF90 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF94 ((uint32_t)0x45AF94u) /* orig 0x45AF94 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF98 ((uint32_t)0x45AF98u) /* orig 0x45AF98 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AF9C ((uint32_t)0x45AF9Cu) /* orig 0x45AF9C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFA0 ((uint32_t)0x45AFA0u) /* orig 0x45AFA0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFA4 ((uint32_t)0x45AFA4u) /* orig 0x45AFA4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFA8 ((uint32_t)0x45AFA8u) /* orig 0x45AFA8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFAC ((uint32_t)0x45AFACu) /* orig 0x45AFAC -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFB4 ((uint32_t)0x45AFB4u) /* orig 0x45AFB4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFB8 ((uint32_t)0x45AFB8u) /* orig 0x45AFB8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFBC ((uint32_t)0x45AFBCu) /* orig 0x45AFBC -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFC0 ((uint32_t)0x45AFC0u) /* orig 0x45AFC0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFC4 ((uint32_t)0x45AFC4u) /* orig 0x45AFC4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFC8 ((uint32_t)0x45AFC8u) /* orig 0x45AFC8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFCC ((uint32_t)0x45AFCCu) /* orig 0x45AFCC -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFD0 ((uint32_t)0x45AFD0u) /* orig 0x45AFD0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFD4 ((uint32_t)0x45AFD4u) /* orig 0x45AFD4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45AFD8 ((uint32_t)0x45AFD8u) /* orig 0x45AFD8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45B5F4 ((uint32_t)0x45B5F4u) /* orig 0x45B5F4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA24 ((uint32_t)0x45BA24u) /* orig 0x45BA24 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA28 ((uint32_t)0x45BA28u) /* orig 0x45BA28 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA2C ((uint32_t)0x45BA2Cu) /* orig 0x45BA2C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA38 ((uint32_t)0x45BA38u) /* orig 0x45BA38 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA40 ((uint32_t)0x45BA40u) /* orig 0x45BA40 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA44 ((uint32_t)0x45BA44u) /* orig 0x45BA44 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA48 ((uint32_t)0x45BA48u) /* orig 0x45BA48 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA4C ((uint32_t)0x45BA4Cu) /* orig 0x45BA4C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA54 ((uint32_t)0x45BA54u) /* orig 0x45BA54 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA58 ((uint32_t)0x45BA58u) /* orig 0x45BA58 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA59 ((uint32_t)0x45BA59u) /* orig 0x45BA59 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA5A ((uint32_t)0x45BA5Au) /* orig 0x45BA5A -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA5B ((uint32_t)0x45BA5Bu) /* orig 0x45BA5B -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA5C ((uint32_t)0x45BA5Cu) /* orig 0x45BA5C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA5D ((uint32_t)0x45BA5Du) /* orig 0x45BA5D -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45BA5E ((uint32_t)0x45BA5Eu) /* orig 0x45BA5E -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45C044 ((uint32_t)0x45C044u) /* orig 0x45C044 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45C05C ((uint32_t)0x45C05Cu) /* orig 0x45C05C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45C45C ((uint32_t)0x45C45Cu) /* orig 0x45C45C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_TBL_45C622 ((uint32_t)0x45C622u) /* orig 0x45C622 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C65A ((uint32_t)0x45C65Au) /* orig 0x45C65A -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C65B ((uint32_t)0x45C65Bu) /* orig 0x45C65B -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C65C ((uint32_t)0x45C65Cu) /* orig 0x45C65C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C65D ((uint32_t)0x45C65Du) /* orig 0x45C65D -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C65E ((uint32_t)0x45C65Eu) /* orig 0x45C65E -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C675 ((uint32_t)0x45C675u) /* orig 0x45C675 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C676 ((uint32_t)0x45C676u) /* orig 0x45C676 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C677 ((uint32_t)0x45C677u) /* orig 0x45C677 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C678 ((uint32_t)0x45C678u) /* orig 0x45C678 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C679 ((uint32_t)0x45C679u) /* orig 0x45C679 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C67A ((uint32_t)0x45C67Au) /* orig 0x45C67A -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C67B ((uint32_t)0x45C67Bu) /* orig 0x45C67B -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C67C ((uint32_t)0x45C67Cu) /* orig 0x45C67C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C67D ((uint32_t)0x45C67Du) /* orig 0x45C67D -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C67E ((uint32_t)0x45C67Eu) /* orig 0x45C67E -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C67F ((uint32_t)0x45C67Fu) /* orig 0x45C67F -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C680 ((uint32_t)0x45C680u) /* orig 0x45C680 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C681 ((uint32_t)0x45C681u) /* orig 0x45C681 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C682 ((uint32_t)0x45C682u) /* orig 0x45C682 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C683 ((uint32_t)0x45C683u) /* orig 0x45C683 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C684 ((uint32_t)0x45C684u) /* orig 0x45C684 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C685 ((uint32_t)0x45C685u) /* orig 0x45C685 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C686 ((uint32_t)0x45C686u) /* orig 0x45C686 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C687 ((uint32_t)0x45C687u) /* orig 0x45C687 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C688 ((uint32_t)0x45C688u) /* orig 0x45C688 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C689 ((uint32_t)0x45C689u) /* orig 0x45C689 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C68A ((uint32_t)0x45C68Au) /* orig 0x45C68A -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C68B ((uint32_t)0x45C68Bu) /* orig 0x45C68B -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C68C ((uint32_t)0x45C68Cu) /* orig 0x45C68C -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C68D ((uint32_t)0x45C68Du) /* orig 0x45C68D -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C68E ((uint32_t)0x45C68Eu) /* orig 0x45C68E -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C68F ((uint32_t)0x45C68Fu) /* orig 0x45C68F -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C690 ((uint32_t)0x45C690u) /* orig 0x45C690 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C691 ((uint32_t)0x45C691u) /* orig 0x45C691 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C692 ((uint32_t)0x45C692u) /* orig 0x45C692 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C693 ((uint32_t)0x45C693u) /* orig 0x45C693 -- arena cell, see DEC4_ADDRMAP.md */
#define D4V_CTX_B_45C694 ((uint32_t)0x45C694u) /* orig 0x45C694 -- arena cell, see DEC4_ADDRMAP.md */

#endif /* UHA04_DEC4_ADDR_H */
