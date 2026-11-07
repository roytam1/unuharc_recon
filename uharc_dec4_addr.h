/* UHARC 0.6 core-4 (uharc_dec4.c) arena address names. See DEC4_ADDRMAP.md.
 * Phase 1 naming layer: each #define preserves the exact original
 * 32-bit DGROUP address so G/M macro expansions are unchanged. */
#ifndef UHARC_DEC4_ADDR_H
#define UHARC_DEC4_ADDR_H

/* --- range coder, dictionary, context, I/O --- */
#define D4A_MASK13_SRC 0x40CDA1UL /* orig 0x40CDA1 -- stack mask src (13B) */
#define D4A_MASK9_SRC 0x40CDADUL /* orig 0x40CDAD -- stack mask src (10B) */
#define D4A_HDR_SIZE 0x4228BEUL /* orig 0x4228BE -- stream header length */
#define D4A_IO_ERR 0x422B90UL /* orig 0x422B90 -- 3=EOF 4=write err */
#define D4A_PPM_PATTERN 0x422BA0UL /* orig 0x422BA0 -- const: Fenwick 512B tiles src */
#define D4A_CNT_OFF_5 0x4236D4UL /* orig 0x4236D4 -- const: cnt off x5 */
#define D4A_CNT_OFF_17 0x4236E8UL /* orig 0x4236E8 -- const: cnt off x17 */
#define D4A_CLS_TREE_SRC 0x42372CUL /* orig 0x42372C -- const: cls tree src */
#define D4A_CLS_CHILD 0x4239BBUL /* orig 0x4239BB -- const: cls child */
#define D4A_CNT_OFF_8 0x4239D0UL /* orig 0x4239D0 -- const: cnt off x8 (dangling) */
#define D4A_BUCKET_LUT 0x4239F0UL /* orig 0x4239F0 -- const: bucket LUT */
#define D4A_BUCKET_LUT2 0x423DF0UL /* orig 0x423DF0 -- const: bucket LUT2 (dangling) */
#define D4A_LEN_LUT_A 0x424270UL /* orig 0x424270 -- const: len LUT A */
#define D4A_LEN_LUT_B 0x424280UL /* orig 0x424280 -- const: len LUT B */
#define D4A_LEN_LUT_C 0x424290UL /* orig 0x424290 -- const: len LUT C */
#define D4A_LEN_NEXT 0x4242A0UL /* orig 0x4242A0 -- const: len next */
#define D4A_FREQ_THRESH1 0x4244A0UL /* orig 0x4244A0 -- const: freq thresh 1 */
#define D4A_FREQ_THRESH2 0x4245A0UL /* orig 0x4245A0 -- const: freq thresh 2 */
#define D4A_TRI2_RANGE 0x4246A0UL /* orig 0x4246A0 -- const: tri2 range */
#define D4A_TRI2_TOTAL 0x4246C8UL /* orig 0x4246C8 -- const: tri2 total */
#define D4A_TRI2_IDX 0x424718UL /* orig 0x424718 -- const: tri2 idx */
#define D4A_SEL_LUT 0x42472CUL /* orig 0x42472C -- const: sel LUT (dangling) */
#define D4A_DELTA_LUT 0x424B2CUL /* orig 0x424B2C -- const: delta LUT */
#define D4A_DELTA_LUT2 0x424B3CUL /* orig 0x424B3C -- const: delta LUT2 */
#define D4A_DELTA_LUT3 0x424C3CUL /* orig 0x424C3C -- const: delta LUT3 */
#define D4A_MASK_LUT 0x424D3CUL /* orig 0x424D3C -- const: mask LUT */
#define D4A_SEL_HIST 0x424D54UL /* orig 0x424D54 -- const: sel hist */
#define D4A_BYTE_CURVE_0 0x425554UL /* orig 0x425554 -- const: byte curve 0 */
#define D4A_BYTE_CURVE_1 0x425954UL /* orig 0x425954 -- const: byte curve 1 */
#define D4A_BYTE_CURVE_2 0x425D54UL /* orig 0x425D54 -- const: byte curve 2 */
#define D4A_BYTE_CURVE_3 0x426154UL /* orig 0x426154 -- const: byte curve 3 */
#define D4A_BYTE_CURVE_4 0x426554UL /* orig 0x426554 -- const: byte curve 4 */
#define D4A_INIT_FLAG 0x426954UL /* orig 0x426954 -- const: init flag (unread) */
#define D4A_SDI_TAB 0x426957UL /* orig 0x426957 -- const: sdi tab */
#define D4A_SDI_PTR 0x426AC3UL /* orig 0x426AC3 -- const: sdi ptr x100 */
#define D4A_SDI_LEN 0x426C53UL /* orig 0x426C53 -- const: sdi len */
#define D4A_DECODE_MAP 0x42711FUL /* orig 0x42711F -- const: decode map */
#define D4A_PENDING_TAB 0x427179UL /* orig 0x427179 -- const: pending tab */
#define D4A_HASH_ENABLE 0x42738DUL /* orig 0x42738D -- const: hash enable */
#define D4A_COST_LUT_SRC 0x429ED2UL /* orig 0x429ED2 -- cost LUT preimage */
#define D4A_COST_LUT 0x429ED4UL /* orig 0x429ED4 -- cost LUT (folded once) */
#define D4A_COST_DIRTY 0x431ED4UL /* orig 0x431ED4 -- fold-once flag */
#define D4A_OUT_TOTAL 0x434514UL /* orig 0x434514 -- total out_len */
#define D4A_OUTBUF_LEN 0x43451CUL /* orig 0x43451C -- outbuf fill / flush size */
#define D4A_IN_CHUNKS 0x434524UL /* orig 0x434524 -- input refill count */
#define D4A_OUT_POS 0x434528UL /* orig 0x434528 -- output cursor */
#define D4A_PERM_TAB 0x43972CUL /* orig 0x43972C -- perm table i/2048-i */
#define D4A_ROW_REGION 0x43F378UL /* orig 0x43F378 -- predictor row region base */
#define D4A_LAST_SEEN 0x457964UL /* orig 0x457964 -- last AE1C per byte */
#define D4A_OUT_COUNT 0x45AE1CUL /* orig 0x45AE1C -- vs byte count */
#define D4A_TRI2_TOTAL_NEXT 0x45AE20UL /* orig 0x45AE20 -- tri2 total next */
#define D4A_TRI2_RANGE_NEXT 0x45AE24UL /* orig 0x45AE24 -- tri2 range next */
#define D4A_MOD_CNT 0x45AE2CUL /* orig 0x45AE2C -- mod counter */
#define D4A_COST_ACCUM 0x45C640UL /* orig 0x45C640 -- smoothed cost */
#define D4A_COST_TMP 0x45C648UL /* orig 0x45C648 -- transient cost sum */
#define D4A_BEST_CNT 0x45C64CUL /* orig 0x45C64C -- dict-offset best count */
#define D4A_COST_GATE 0x45C654UL /* orig 0x45C654 -- cost comparator */
#define D4A_COST_SUM 0x45C658UL /* orig 0x45C658 -- cost sum */
#define D4A_BEST_OFF 0x45C674UL /* orig 0x45C674 -- dict-offset best */
#define D4A_TRI_TOTAL 0x45C67CUL /* orig 0x45C67C -- tri [total] */
#define D4A_TRI_LOW 0x45C680UL /* orig 0x45C680 -- tri [low] = total+4 */
#define D4A_TRI_HIGH 0x45C684UL /* orig 0x45C684 -- tri [high] = total+8 */
#define D4A_PPM_CTX 0x45C690UL /* orig 0x45C690 -- current PPM ctx idx */
#define D4A_COL_IDX 0x45C694UL /* orig 0x45C694 -- column idx */
#define D4A_COL_TOT_A 0x45C698UL /* orig 0x45C698 -- column total threshold A */
#define D4A_ROW_CTX 0x45C69CUL /* orig 0x45C69C -- row ctx = hash */
#define D4A_CTX_COPY 0x45C6A4UL /* orig 0x45C6A4 -- copy of ctx_w0 */
#define D4A_COL_TOT_B 0x45C6A8UL /* orig 0x45C6A8 -- column total threshold B */
#define D4A_CTX_W0 0x45D0F8UL /* orig 0x45D0F8 -- last-4-bytes shift reg (BE) */
#define D4A_CTX_W1 0x45D0FCUL /* orig 0x45D0FC -- previous window */
#define D4A_CTX_W2 0x45D100UL /* orig 0x45D100 -- older window */
#define D4A_FREQ_A 0x45D104UL /* orig 0x45D104 -- adaptive weight A */
#define D4A_DICT_SIZE 0x45D10CUL /* orig 0x45D10C -- dict size 1<<((m-24)%16+10) */
#define D4A_DICT_POS 0x45D110UL /* orig 0x45D110 -- dict insert cursor */
#define D4A_DICT_MASK 0x45D118UL /* orig 0x45D118 -- dict mask size-1 */
#define D4A_FREQ_C 0x45D120UL /* orig 0x45D120 -- adaptive weight C */
#define D4A_FILE_TAB_PTR 0x45D128UL /* orig 0x45D128 -- cum file-end table ptr (=p_prog) */
#define D4A_BLK_END 0x45D12CUL /* orig 0x45D12C -- current block end */
#define D4A_FREQ_B 0x45D138UL /* orig 0x45D138 -- adaptive weight B */
#define D4A_NEXT_BLK 0x45D13CUL /* orig 0x45D13C -- next-block threshold */
#define D4A_HIST_SYMS 0x45DD6EUL /* orig 0x45DD6E -- pending hist symbols */
#define D4A_RUN_CNT 0x45DD85UL /* orig 0x45DD85 -- run counter base */
#define D4A_PRED_FLAG 0x45E38EUL /* orig 0x45E38E -- predictor flag */
#define D4A_MODEL_ID 0x45E390UL /* orig 0x45E390 -- core model selector */
#define D4A_PHASE 0x45E392UL /* orig 0x45E392 -- PPM phase */
#define D4A_ESCAPE_FLAG 0x45E39BUL /* orig 0x45E39B -- pending-escape flag */
#define D4A_HIST_LEN 0x45E39CUL /* orig 0x45E39C -- hist sym count */
#define D4A_COL_SLOT 0x45E3A3UL /* orig 0x45E3A3 -- col scan idx -1..11 */
#define D4A_ROW_SLOT 0x45E3A5UL /* orig 0x45E3A5 -- row scan idx 0..7 */
#define D4A_RC_CODE 0x4749D8UL /* orig 0x4749D8 -- range-coder code */
#define D4A_RC_RANGE 0x4749DCUL /* orig 0x4749DC -- range-coder range */

/* --- remaining arena cells (generic names; do NOT guess semantics) --- */
#define D4A_TBL_42369C 0x42369CUL /* orig 0x42369C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4236A0 0x4236A0UL /* orig 0x4236A0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4236A4 0x4236A4UL /* orig 0x4236A4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4236A8 0x4236A8UL /* orig 0x4236A8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4236AC 0x4236ACUL /* orig 0x4236AC -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4236B0 0x4236B0UL /* orig 0x4236B0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4236B4 0x4236B4UL /* orig 0x4236B4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4236B8 0x4236B8UL /* orig 0x4236B8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_42748D 0x42748DUL /* orig 0x42748D -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4274CD 0x4274CDUL /* orig 0x4274CD -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_42750D 0x42750DUL /* orig 0x42750D -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_42754D 0x42754DUL /* orig 0x42754D -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_42758D 0x42758DUL /* orig 0x42758D -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4275CD 0x4275CDUL /* orig 0x4275CD -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_42760D 0x42760DUL /* orig 0x42760D -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_42762D 0x42762DUL /* orig 0x42762D -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_42764D 0x42764DUL /* orig 0x42764D -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43D72C 0x43D72CUL /* orig 0x43D72C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43D730 0x43D730UL /* orig 0x43D730 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43D734 0x43D734UL /* orig 0x43D734 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43D738 0x43D738UL /* orig 0x43D738 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43D73C 0x43D73CUL /* orig 0x43D73C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43D740 0x43D740UL /* orig 0x43D740 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43D744 0x43D744UL /* orig 0x43D744 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43D748 0x43D748UL /* orig 0x43D748 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43D948 0x43D948UL /* orig 0x43D948 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43DB48 0x43DB48UL /* orig 0x43DB48 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43DB4C 0x43DB4CUL /* orig 0x43DB4C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43DB50 0x43DB50UL /* orig 0x43DB50 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43DB54 0x43DB54UL /* orig 0x43DB54 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43DB58 0x43DB58UL /* orig 0x43DB58 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43DB5C 0x43DB5CUL /* orig 0x43DB5C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43DB60 0x43DB60UL /* orig 0x43DB60 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43DF60 0x43DF60UL /* orig 0x43DF60 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F36C 0x43F36CUL /* orig 0x43F36C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F370 0x43F370UL /* orig 0x43F370 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F374 0x43F374UL /* orig 0x43F374 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F37C 0x43F37CUL /* orig 0x43F37C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F380 0x43F380UL /* orig 0x43F380 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F384 0x43F384UL /* orig 0x43F384 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F388 0x43F388UL /* orig 0x43F388 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F38C 0x43F38CUL /* orig 0x43F38C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F390 0x43F390UL /* orig 0x43F390 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F394 0x43F394UL /* orig 0x43F394 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F398 0x43F398UL /* orig 0x43F398 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F39C 0x43F39CUL /* orig 0x43F39C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F3A0 0x43F3A0UL /* orig 0x43F3A0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F3A4 0x43F3A4UL /* orig 0x43F3A4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_43F568 0x43F568UL /* orig 0x43F568 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_44316C 0x44316CUL /* orig 0x44316C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_444584 0x444584UL /* orig 0x444584 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_444594 0x444594UL /* orig 0x444594 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4445A0 0x4445A0UL /* orig 0x4445A0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4445A4 0x4445A4UL /* orig 0x4445A4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4445A8 0x4445A8UL /* orig 0x4445A8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_444DA4 0x444DA4UL /* orig 0x444DA4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4451A0 0x4451A0UL /* orig 0x4451A0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4451A4 0x4451A4UL /* orig 0x4451A4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4455A4 0x4455A4UL /* orig 0x4455A4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4455A8 0x4455A8UL /* orig 0x4455A8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4455AC 0x4455ACUL /* orig 0x4455AC -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_445DAC 0x445DACUL /* orig 0x445DAC -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_445DB0 0x445DB0UL /* orig 0x445DB0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_445DB4 0x445DB4UL /* orig 0x445DB4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_446044 0x446044UL /* orig 0x446044 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_446048 0x446048UL /* orig 0x446048 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_44604C 0x44604CUL /* orig 0x44604C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_446050 0x446050UL /* orig 0x446050 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_446054 0x446054UL /* orig 0x446054 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_446058 0x446058UL /* orig 0x446058 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_44605C 0x44605CUL /* orig 0x44605C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_446060 0x446060UL /* orig 0x446060 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_446064 0x446064UL /* orig 0x446064 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_446460 0x446460UL /* orig 0x446460 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_446464 0x446464UL /* orig 0x446464 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_448460 0x448460UL /* orig 0x448460 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_448464 0x448464UL /* orig 0x448464 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_450464 0x450464UL /* orig 0x450464 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_452460 0x452460UL /* orig 0x452460 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_452464 0x452464UL /* orig 0x452464 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_452860 0x452860UL /* orig 0x452860 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_452864 0x452864UL /* orig 0x452864 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_453064 0x453064UL /* orig 0x453064 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_453460 0x453460UL /* orig 0x453460 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_453464 0x453464UL /* orig 0x453464 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4534E4 0x4534E4UL /* orig 0x4534E4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4554E4 0x4554E4UL /* orig 0x4554E4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4558E4 0x4558E4UL /* orig 0x4558E4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4578E0 0x4578E0UL /* orig 0x4578E0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_4578E4 0x4578E4UL /* orig 0x4578E4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_457D60 0x457D60UL /* orig 0x457D60 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_457D64 0x457D64UL /* orig 0x457D64 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_458564 0x458564UL /* orig 0x458564 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45A564 0x45A564UL /* orig 0x45A564 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45AD64 0x45AD64UL /* orig 0x45AD64 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45ADE4 0x45ADE4UL /* orig 0x45ADE4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45AE14 0x45AE14UL /* orig 0x45AE14 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45AE18 0x45AE18UL /* orig 0x45AE18 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45AE28 0x45AE28UL /* orig 0x45AE28 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45AE30 0x45AE30UL /* orig 0x45AE30 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45AE34 0x45AE34UL /* orig 0x45AE34 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45AE38 0x45AE38UL /* orig 0x45AE38 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45BA38 0x45BA38UL /* orig 0x45BA38 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45BA3C 0x45BA3CUL /* orig 0x45BA3C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45C644 0x45C644UL /* orig 0x45C644 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45C650 0x45C650UL /* orig 0x45C650 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45C668 0x45C668UL /* orig 0x45C668 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45C66C 0x45C66CUL /* orig 0x45C66C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45C670 0x45C670UL /* orig 0x45C670 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45C678 0x45C678UL /* orig 0x45C678 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45C6A0 0x45C6A0UL /* orig 0x45C6A0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45C6AC 0x45C6ACUL /* orig 0x45C6AC -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45C6B0 0x45C6B0UL /* orig 0x45C6B0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45C6B4 0x45C6B4UL /* orig 0x45C6B4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45C6B8 0x45C6B8UL /* orig 0x45C6B8 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45CCD4 0x45CCD4UL /* orig 0x45CCD4 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D108 0x45D108UL /* orig 0x45D108 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D114 0x45D114UL /* orig 0x45D114 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D11C 0x45D11CUL /* orig 0x45D11C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D124 0x45D124UL /* orig 0x45D124 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D130 0x45D130UL /* orig 0x45D130 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D134 0x45D134UL /* orig 0x45D134 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D140 0x45D140UL /* orig 0x45D140 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D144 0x45D144UL /* orig 0x45D144 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D160 0x45D160UL /* orig 0x45D160 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D164 0x45D164UL /* orig 0x45D164 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D166 0x45D166UL /* orig 0x45D166 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D566 0x45D566UL /* orig 0x45D566 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D738 0x45D738UL /* orig 0x45D738 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45D768 0x45D768UL /* orig 0x45D768 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45DD5A 0x45DD5AUL /* orig 0x45DD5A -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45DD6A 0x45DD6AUL /* orig 0x45DD6A -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45DD6C 0x45DD6CUL /* orig 0x45DD6C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45DD84 0x45DD84UL /* orig 0x45DD84 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45E085 0x45E085UL /* orig 0x45E085 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_TBL_45E086 0x45E086UL /* orig 0x45E086 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E387 0x45E387UL /* orig 0x45E387 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E388 0x45E388UL /* orig 0x45E388 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E389 0x45E389UL /* orig 0x45E389 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E38A 0x45E38AUL /* orig 0x45E38A -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E38B 0x45E38BUL /* orig 0x45E38B -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E38C 0x45E38CUL /* orig 0x45E38C -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E38D 0x45E38DUL /* orig 0x45E38D -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E38F 0x45E38FUL /* orig 0x45E38F -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E391 0x45E391UL /* orig 0x45E391 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E393 0x45E393UL /* orig 0x45E393 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E394 0x45E394UL /* orig 0x45E394 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E395 0x45E395UL /* orig 0x45E395 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E396 0x45E396UL /* orig 0x45E396 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E397 0x45E397UL /* orig 0x45E397 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E398 0x45E398UL /* orig 0x45E398 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E399 0x45E399UL /* orig 0x45E399 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E39A 0x45E39AUL /* orig 0x45E39A -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E39D 0x45E39DUL /* orig 0x45E39D -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E39E 0x45E39EUL /* orig 0x45E39E -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E39F 0x45E39FUL /* orig 0x45E39F -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E3A0 0x45E3A0UL /* orig 0x45E3A0 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E3A1 0x45E3A1UL /* orig 0x45E3A1 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E3A2 0x45E3A2UL /* orig 0x45E3A2 -- arena cell, see DEC4_ADDRMAP.md */
#define D4A_FLAG_45E3A4 0x45E3A4UL /* orig 0x45E3A4 -- arena cell, see DEC4_ADDRMAP.md */

#endif /* UHARC_DEC4_ADDR_H */
