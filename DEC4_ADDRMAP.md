# UHARC core-4 address → name audit (pre-refactor map)

**Status: audit only — no decoder code changed.**  
**Date:** 2026-11-07  
**Scope:** `uharc_dec4.c` (0.6), `uha04_dec4.c` (0.4), `uha03_dec4.c` (0.3).  
**Method:** static scan of every `G8/G16/G32/M8/M16/M32` literal + header/init comments, `D4TRACE` strings, and `UNUHARC06.c / UHARC04.c / uharc_u.c` Decompile comments in-file. No builds run for the audit — verification is reserved for post-map refactoring.

> This file is the contract for any future renaming/structification. Keep each original address in a comment on first use: `/* orig 0x45D0BC */`.

---

## 0. How the map was built (counts)

| File | Version / range | Lines / bytes | Distinct arena literals in `G/M(0x…)` | Arithmetic bases `M((base+idx))` | Non-`G/M` literals in DGROUP window |
|------|------|------|------|------|------|
| `uharc_dec4.c` | 0.6, methods 24–39 | 8,263 / 302 KB | **229** | 236 (with `M(A_*+…)`) | — (arena is `D4_ARENA 0x09000000`) |
| `uha04_dec4.c` | 0.4, methods 24–39 | 9,843 / 560 KB | **262 strict** | **+18** indexed-only (`M((base+idx))`) = 280 | **+16** = 296 total in `0x42E000–0x45DA00` |
| `uha03_dec4.c` | 0.3, methods 22–35 | 10,781 / 560 KB | **222 direct** (360 accesses) | **+174** arithmetic bases | — (const slice `0x428000–0x4354CC`) |

**What the counts mean**

* The `0x0041xxxx` hits that make the raw hex count look huge (e.g. 8,719 in the `0x00410000` page for `uha04_dec4.c`) are **instruction addresses in comments** (`/* 0x0041BF05 mov … */` in `uha03_dec4.c`, `L1094 415130` comments in `uha04_dec4.c`). They are **not** arena offsets and are excluded from the "distinct" column.
* `G` vs `M`: `G8/G16/G32(a)` is a literal global in source; `M8/M16/M32(o)` is a computed/heap-pointer deref. The same cell often appears as both across call sites. Size is inferred from the accessor used (`8/16/32`).
* Singleton vs array base is inferred from indexing (`[reg*4]`, `+2*i`, `+8*h`, `+idx<<9`, …) in nearby comments/code.

**Reading the tables:** `Original` is the literal as written in C (without `0x004` prefix variance). `Proposed` is the readable name from the audit sub-agents. It is a *proposal for review*, not yet applied. `Evidence` cites the function or asm comment that pins the role.

---

## 1. Naming convention (proposed, to be applied uniformly in Phase 1)

```
rc_code, rc_range, rc_total, rc_prob_lo, rc_prob_hi, rc_scale, rc_value, rc_carry_cnt, rc_pending_b
io_in_pos, io_in_avail, io_in_total, io_out_pos, io_out_total, io_out_len, io_err, io_out_cap
dict_size, dict_mask, dict_cursor, dict_buf
hash1, hash2, hash_head
fenwick1, fenwick2, ppm_tot
model_id, phase, ctx_w0..w2, ctx_key, tri_total, tri_low, tri_high
blk_ptr_XX_*  (0.4 block-pointer slots 0x433B40..) — keep index, add role suffix after review
g_blkXX_*     (0.3 carve slots 0x4299B8..)
cost_lut, cost_accum, cost_gate, cost_thresh
tbl_* / g_tbl_*  (DGROUP const tables — keep hex suffix until role is confirmed)
flag_e3xx / g_fxx  (single-byte flags — do not invent semantics until verified)
```

* Keep the original hex in a trailing comment.
* Singletons get a semantic name; array bases get `tbl_*`/`*_tab` + stride note (`stride 4` = `U32`).
* The `0x45E38x` (0.6) / `0x444E**` (0.3) / `0x45C65x–45C69x` (0.4) byte clusters are **flags/context bytes** — name them `flag_*`/`ctx_b_*` only after per-site review, otherwise retain `flag_e3A0`-style names.

---

## 2. Shared concepts across all three versions

The three coders are **version-specific**, but the same roles recur:

| Role | 0.6 (`uharc_dec4.c`) | 0.4 (`uha04_dec4.c`) | 0.3 (`uha03_dec4.c`) |
|------|------|------|------|
| Range-coder code | `0x4749D8` (`rc_code`) | `0x45D0B8` (`rc_code`) | `0x4441EC` + `0x4450F4` (`rc_prob_lo/value`) |
| Range-coder range | `0x4749DC` (`rc_range`) | `0x45D0BC` (`rc_range`) | `0x4450F8` (`rc_range`) + `0x4441E8` (`rc_total`) |
| Tri interval `[total, lo, hi]` | `0x45C67C/80/84` | `0x45AF68/6C/70` (`rslot_lo/mid/hi`) | `0x4441E8/EC/F0` (`rc_total/prob_lo/prob_hi`) |
| Dictionary size/mask/cursor | `0x45D10C/118/110` | `0x45BA30/3C/34` | `0x444270/278/274` |
| Input cursor / valid / total | `inbuf[4096]` + `rcb` (no DGROUP `45CD*`) | `0x45CD18/20/24` | `0x444EE8/F0/F4` |
| Output cursor / total / len | `0x434528/43451C/434514` | `0x45CD1C/28/14` | `0x444EF8/EE4` + staged `0x42C9AC/444EEC` |
| Block-end / next-block / file table | `0x45D12C/13C/128` + `p_prog` (`A_PROG`) | `0x45AFB0/B4/28/2C` | `0x444230/254/22C` (+ `0x444E99` header byte) |
| Hash/PPM Fenwick planes | `p_6bc/p_6c0/p_6c8` (`A_6BC/6C0/6C8`) | `blk_ptr_*` + price `0x437968/66` | `blk_4299F0/F4` (ctx coders, `0x61600`) |
| Cost LUT (folded once) | `0x429ED4` (`cost_lut`) | `0x437968` + `0x437966` (BSS-zero) | `0x42D274`/`0x42D272` rebuild |

> **Do not merge across versions.** `0x45D0B8` in 0.4 is the range code; `0x45D0B8` does not exist in 0.6 (there it is `0x4749D8`). Names are per-version.

---

## 3. 0.6 — `uharc_dec4.c` (methods 24–39)

Arena `D4_ARENA 0x09000000`, `c->mem = calloc(1, D4_ARENA)`. `p_*` fields in `dec4_ctx` are the heap-block pointers. `A_*` defines are the arena base addresses.

### 3.1 Heap / arena blocks (`A_*` → `p_*`)

| Define / field | Addr | Size (init) | Proposed | Access | Role | Evidence |
|---|---|---|---|---|---|---|
| `A_DICTB / p_dict` | `0x01908000` | `dict+256`, zeroed | `dict_buf` | `p_dict` | sliding dict; `dict = 1<<((m-24)%16+10)` | `d4_40CDD1` memset; `p_dict+(mask&…)` in `40E885,40F9D0` |
| `A_680 / p_680` | `0x00610000` | `0x100000`, zeroed | `col_tab` | `p_680` | 32-byte column contexts (`32*idx`) | `40D604,40ECC7,41030F` |
| `A_684 / p_684` | `0x00E10000` | `0x280000`, zeroed | `row_tab` | `p_684` | 20-byte row contexts (`20*idx`) | `40F049,40F19A` |
| `A_H1 / p_h1` | `0x01350000` | `0x200000`, `0xFF` | `hash1` | `p_h1` | hash chain 1 (`8*hash`) | `40F9D0` |
| `A_H2 / p_h2` | `0x01550000` | `0x200000`, `0xFF` | `hash2` | `p_h2` | hash chain 2 (`8*hash2+4`) | `40F9D0` |
| `A_690 / p_690` | `0x01750000` | `0x40000`, zeroed | `hash_head` | `p_690` | per-hash best-length byte | `M8(v38+p_690)` in `40F9D0` |
| `A_694 / p_694` | `0x01790000` | `0x40000`, zeroed | `hash_byte` | `p_694` | current ctx byte; `p_148=p_694+v38` | `40F9D0` |
| `A_698 / p_698` | `0x017D0000` | `0x40000`, zeroed | `byte_hist` | `p_698` + `p_150..15c` | 4 planes for recent bytes (`+0x10000` strides) | `p_150=p_698` etc. in `40CDD1`; `40F39A` |
| `A_6BC / p_6bc` | `0x01810000` | `0x41200`, zeroed | `fenwick1` | `p_6bc` | Fenwick plane A (`(idx<<9)`, U16×512/ctx) | `40E672,412257,412A59` |
| `A_6C0 / p_6c0` | `0x01872000` | `0x41200`, zeroed | `fenwick2` | `p_6c0` | Fenwick plane B (`(a1<<9)`) | `40E3EB,40E4FF,40E672` |
| `A_6C4 / p_6c4` | `0x018D4000` | set only | `ppm_unused` | `p_6c4` | assigned, never dereferenced | 1 use in `40CDD1` |
| `A_6C8 / p_6c8` | `0x018E0000` | U16×521 | `ppm_tot` | `p_6c8` | per-ctx totals (`2*idx`) | `40E4FF,412257,412A59` |
| `A_6CC / p_6cc` | `0x018E0700` | `0x100` | `ppm_spare1` | `p_6cc` | assigned only | `40CDD1` |
| `A_6D0 / p_6d0` | `0x018E0800` | `0x100` | `ppm_spare2` | `p_6d0` | assigned only | `40CDD1` |
| `A_ADE8 / p_ade8` | `0x018E0900` | 8-byte records | `pred_cache` | `p_ade8` | `8*idx → {U32 val; U8 e38e; U8 e38b}` | `412FFA` |
| `A_PROG / p_prog` | `0x00500000` | `4*(ncum+1)` | `file_table` | `p_prog` | container cum file-end table (`dword_45D128`) | `M32(A_PROG+4*i)=cum[i]` in `decode_b`; `40DEB6` walk |
| `p_148` | dynamic | `U8*` | `ctx_byte_ptr` | `p_148` | `=p_694+v38` | `40F9D0` |
| `p_14c` | dynamic | `U8*` | `pair_ptr` | `p_14c` | `0x45AD64+2*…` | `40E885` when `M8==2/3` |
| `p_150,154,158,15c` | dynamic | `U8* ×4` | `hist_ptr_0..3` | `p_150..` | 4 window pointers into `p_698` | `40CDD1,40F39A,41030F` |
| `p_688` | `0x45C6B8` | U32 array | `cost_cnt_a` | via `p_688` | `=0x45C6B8+4*v4` | `41030F` |
| `p_68c` | `0x45CCD4` | U32 array | `cost_cnt_b` | via `p_68c` | `=0x45CCD4+4*v5` | `41030F` |
| `p_69C..p_6B8` | — | — | `ppm_unused2` | — | declared, never used | `dec4_ctx` |

### 3.2 Const tables (`.rodata`, never written)

| Orig | Proposed | Type | Role / consumer |
|---|---|---|---|
| `0x422BA0` | `ppm_pattern` | U8×512 | source of Fenwick 512B tiles in `40DEB6` |
| `0x42372C` | `cls_tree_src` | U8×656 | `40DEB6` `memcpy 0x445DB4` |
| `0x4239BB` | `cls_child` | U8×22 | `M8(0x4239BB+2*G8(45E3A2)+a2)` in `40E810` |
| `0x4236D4/0x4236E8` | `cnt_off_5/17` | U32×5/17 | `M32(…+4*G32(45C64C))` in `41030F` |
| `0x4239D0` | `cnt_off_8` | U32×8 | loaded, no consumer in dec4 (dangling) |
| `0x4239F0` | `bucket_lut` | U32×256 | `M32(0x4239F0+4*M8)` + `0x450464` addressing in `41030F` (asm `40CDA1`) |
| `0x423DF0` | `bucket_lut2` | U32×288 | loaded, unused in dec4 |
| `0x424270/280/290` | `len_lut_a/b/c` | U8 | `M8(0x424270+4*…/G32(45D138))` etc. in `40F9D0` |
| `0x4242A0` | `len_next` | U8×32 | `M8(0x4242A0+2*v34+…)` in `40F9D0` |
| `0x4244A0/0x4245A0` | `freq_thresh` | U8×256 | `M8(0x4244A0+M8(v1))` in `40ECC7,40F049,40F19A` |
| `0x4246A0/0x4246C8/0x424718` | `tri2_range/total/idx` | U32/U32/U8 | `40DEB6,41013C` → `G32(45AE20/45AE24)` |
| `0x42472C` | `sel_lut` | U32×256 | loaded; only via `4239F0` family |
| `0x424B2C/0x424B3C/0x424C3C` | `delta_lut/2/3` | U8 | `40F39A` → `AE20` chain |
| `0x424D3C/0x424D54` | `mask_lut/sel_hist` | U32 | `413347` |
| `0x425554/26554/25954/25D54/26154` | `byte_curve_0..4` | U32×256 | `413347` (paths `e390==6/<7/7/9`) |
| `0x426C53/0x426957/0x426AC3` | `sdi_len/sdi_tab/sdi_ptr` | U8/U8/U32 | `40FFFD` (SDI unpack) |
| `0x426954` | `init_flag` | G8×1 | `=1` in `40CDD1`, never read |
| `0x42711F/0x427179` | `decode_map/pending_tab` | U8×256 / U8×1024 | `40FFFD` |
| `0x42738D` | `hash_enable` | U8×256 | gating `45C69C` (`41030F,412257`) |
| `0x42748D..0x42764D` (9×) | `cost_coef_0..8` | U32 | denominators in `41030F,412257,412A59` |
| `0x429ED4/0x429ED2` | `cost_lut` | U16×16384 | `cost+=lut[((x<<14)/total)*2]` everywhere; fold-once in `419AC0` via `G8(0x431ED4)` |
| `0x40CDA1/0x40CDAD` | `mask13/9_src` | U8×13/10 | stack-mask sources (`vmask13/9` in `41030F`) |
| `0x42369C..0x4236B8` | `row_base_ptr` | U32×8 | `=p_684+1.25M…5.25M` in `40CDD1`; `M32(42369C+4*v)` in `40E885,40EBA7,40FFFD,412ED2` |

### 3.3 `.bss` globals (top by frequency)

| Orig | Proposed | Type | Role | Evidence |
|---|---|---|---|---|
| `0x434528` | `out_pos` | U32 | output cursor; D4TRACE `out=%u`; loop bound `40F962` | 155× |
| `0x4749DC` | `rc_range` | U32 | range; `=div(range,total)`, renormalize by shifting `40B093` byte | 103× (`D4FFFDDBG code=%u range=%u`) |
| `0x45C67C` | `tri_total` | U32 | `[total]` (see tri below) | 82× |
| `0x43F378` | `row_region` | U32 base | 31-dword rows; `R=43F378+124*(AE2C-idx)` | 75× (`412FFA,413347`) |
| `0x4749D8` | `rc_code` | U32 | code; 5-byte init `419A4D` | 61× |
| `0x45D0F8` | `ctx_w0` | U32 | last-4-bytes shift reg (BE) | 61× |
| `0x45D110` | `dict_pos` | U32 | insert cursor, masked `45D118` | 50× |
| `0x45E3A5` | `row_slot` | U8 | `0…7` scan idx | 49× (`40F049,40F19A,41030F`) |
| `0x45AE1C` | `out_count` | U32 | `++` per ctx-update byte | 38× |
| `0x45E39C` | `hist_len` | U8 | count of `0x45DD6E` symbols | 38× |
| `0x45E3A3` | `col_slot` | U8 | `-1/0…11` scan idx | 37× |
| `0x45D118` | `dict_mask` | U32 | `dict_size-1` | 35× |
| `0x45E392` | `phase` | U8 | PPM phase; `0x20000>>phase` | 35× (D4TRACE `phase=%u`) |
| `0x45C684` | `tri_high` | U32 | `[high]` (see tri) | 32× |
| `0x45DD6E` | `hist_syms` | U8 list | pending hist symbols (`h0…h3` debug) | 32× (`40E885,40FFFD,412257`) |
| `0x45E390` | `model_id` | U8 | core model selector | 31× |
| `0x45C680` | `tri_low` | U32 | `[low]` | 30× |
| `0x45C6A4` | `ctx_copy` | U32 | copy of `45D0F8` (permuted if `45E3A0==2`) | 29× |
| `0x43972C` | `perm_tab` | U32×4096 | `i` / `2048-i` | 27× (`413347`) |
| `0x45C648` | `cost_tmp` | U32 | transient cost sum | 27× |
| `0x45C690` | `ppm_ctx` | U32 | current PPM ctx idx | 26× |
| `0x45D0FC` | `ctx_w1` | U32 | previous window | 23× |
| `0x45E39B` | `escape_flag` | U8 | `==1` pending-escape | 23× |
| `0x429ED4` | `cost_lut` | U16 | see 3.2 | 20× |
| `0x45E38E` | `pred_flag` | U8 | predictor flag | 19× |
| `0x45D138` | `freq_b` | U32 | adaptive weight B | 18× |
| `0x45AE2C` | `mod_cnt` | U32 | `=AE1C%AE24` | 17× |
| `0x45C64C` | `best_cnt` | U32 | dict-offset best count | 17× |
| `0x45E38B` | `pred_delta` | U8 | `=(U32)45D0F8>>G8(45E38C)` | 16× |
| `0x457964` | `last_seen` | U32×256 | last `AE1C` per byte value | 15× |
| `0x45C694` | `col_idx` | U32 | `(h^(h>>13))&0x7FFF` | 14× |
| `0x45D12C` | `blk_end` | U32 | current block end | 13× (D4TRACE `blk=%u`) |
| `0x45D104` | `freq_a` | U32 | adaptive weight A | 13× |
| `0x45C698/0x45C6A8` | `col_tot_a/b` | U32 | column total thresholds | 13× |
| `0x45D100` | `ctx_w2` | U32 | older window | 12× |
| `0x445DAC/0x45DD85/0x42738D` | `cost_acc_c / run_cnt / hash_enable` | — | see §3.2 | 11× each |
| `0x422B90` | `io_err` | U32 | `3`=read EOF, `4`=write err | 10× (`40B093,40B178,41013C`) |

**Tri record at `0x45C67C`:** `[total (0x45C67C), low (0x45C680), high (0x45C684)]`. All arithmetic decoders read `low=M32(0x45C67C+4)`, `high=M32(0x45C67C+8)`.

**Flag bytes `0x45E387–0x45E3A5`:** all `U8` (`mode_flag, slot_idx, model_id, phase, gate_*, escape_flag, hist_len, pending_sym, col_slot, row_slot`, …) — per-function evidence in audit; treat as a block, name individually only after review.

**Low-frequency leftovers (U16 tables, cost-model region):** `0x45D566,0x45D768,0x45D166,0x45D738,0x45DD5A,0x45DD6A/6C,0x45D160/164,0x450464/0x446464/0x453464/0x4558E4/0x45A564/0x458564/0x453064` (cost-model addressing in `41030F,412257,412A59`).

---

## 4. 0.4 — `uha04_dec4.c` (methods 24–39)

Arena `V4_ARENA 0x04000000` (64 MiB), `V4O` clamps to `ARENA-8`. DGROUP image `0x42E000–0x440000` (73,728 B `memcpy`), `.bss 0x440000–0x45DA00`. Model blocks bumped from `hi=0x600000` (above globals, ~53 MB footprint).

`dec4v_ctx`: `mem, hi, rcb/rctx, wcb/wctx, in_pos/in_avail/out_fill/out_tot/out_len, err, p_inb/p_outb` (copied to `436ED0/D4`). `p_3b40…p_3b8c, p_af34` are dead mirror declarations.

**Helpers:** `d4v_alloc` (bump-16), `d4v_fill16` (`M16(dst+2*i)`), `d4v_fill` (`memset`), `d4v_sar`, `d4v_memmove`, `d4v_4370d8` (indirect `[4370D8]` wrapper over `rcb`), `d4v_prices` (L105: if `M8(0x43F968)` then `M16(0x437968+2*a)+=M16(0x437966+2*a)` for `a=1..0x3FFF` — `0x437966` is BSS-zero/no-writers so this is a clear-flag no-op).

**Special clusters (from audit callouts):**
* **Range coder:** `45D0B8` code, `45D0BC` range; `45D0B0` carry-count, `45D0B4` code-lo, `45D0C4` code-hi, `45D0C0` pending byte (`>>24`). `45AF68/6C/70` = `[lo,mid,hi]` slot: `M32(eax+8)-M32(eax+4)`.
* **I/O pump:** `45CD10/14/18/1C/20/24/28/2C` + buffers `436ED0/D4`, sticky `436EC8`, snapshots `440E70/74`.
* **Stand-ins:** `D4V_SP 0x0F000000` (never dereferenced, only inc/compare).

### 4.1 Master table (262 strict literals, sorted — abridged to table in audit; full list in §7)

| Orig | Proposed | Type | Role | Evidence |
|---|---|---|---|---|
| `0x431209` | `g_tail_gate_flag` | U8 | tail-scan gate | `415130` `cmp [431209],0; jz` |
| `0x43120C` | `g_prefix_sum_tmp` | U32 | running prefix sum → `45BA50` | `41f2a0,4183d0` (`dword_43120C`) |
| `0x431280` | `g_heap_base_unused` | U32 | native heap base (comment only) | `415130` `sub_40DA80` |
| `0x433B40` | `blk_ptr_00_dict` | U32 ptr | `dict+0x100` | `415130` `mov [433B40]` L702 |
| `0x433B44` | `blk_ptr_01` | U32 ptr | `0x800000`, fill `0x100000` | `415130` L714; `415b50` |
| `0x433B48` | `blk_ptr_02` | U32 ptr | `0x140000` | `415130` L726 |
| `0x433B4C` | `blk_ptr_03` | U32 ptr | `0x200000` fill `0xFF` | `415130` L738; `lzp,ppmdec` |
| `0x433B50` | `blk_ptr_04` | U32 ptr | `0x200000` fill `0xFF` | `415130` L750 |
| `0x433B54` | `blk_ptr_05` | U32 ptr | `0x40000` (PPM ctx) | `415130` L760 |
| `0x433B58` | `blk_ptr_06` | U32 base stride 4 | order table | `417090` `dword_433B58[eax*4]` |
| `0x433B5C–0x433B70` | `blk_ptr_07..12` | U32 ptr | PPM (`0x80000/0x40000/0x1C0000`) | `415130` L784–842 |
| `0x433B74/78/7C` | `blk_ptr_13/14/15` | U32 ptr | `0x40000` / `0x61A00` ×2 | `415130` L854–876 |
| `0x433B80–0x433B8C` | `blk_ptr_16..19` | U32 ptr | S4 model (`0x0C000/0x5FE/0x0C0/0x0C0`) | `415130` L888–924 |
| `0x433B90/0x433BA4` | `tbl_433B90/A4` | U32 stride 4 | sym lookup | `418830` `41892D/418944` |
| `0x433E77/8C,0x433FCC,0x43404C,0x43409C,0x43419C,0x43429C,0x4342C4,0x4342EC,0x434314…` | `tbl_*` (byte/U32) | U8/U32 | sym/pair tables (stride noted) | `sym,416c80,418590,41d540,41cf60` |
| `0x434328/38/438/538…0x435950` | `tbl_*` | U8/U32 | byte/dispatch tables | `41d540,41e9f0` (`[434538+eax*4]` etc.) |
| `0x436150/52,0x436450,0x43691C,0x436B8A` | `g_cfg_ah / g_flag_* / tbl_*` | U8 | init flag / LZP byte tables | `415130,41cf60` |
| `0x436EC8` | `io_err` | U32 | sticky `3` EOF / `6` emit | `refill,emit,417da0` |
| `0x436ED0` | `io_in_ptr` | U32 ptr | in-buffer base (`=p_inb`) | `417da0` L613 |
| `0x436ED4` | `io_out_ptr` | U32 ptr | out-buffer base (`=p_outb`) | `417da0` L614 |
| `0x437966/0x437968` | `tbl_price_src/sum` | U16 stride 2 | price src (zero) / prefix sums | `prices`, `419ca0` `word_437968[eax*2]` |
| `0x43F968` | `g_price_done` | U8 | one-time flag | `prices` |
| `0x440E70/74` | `io_snap_in/out` | U32 | refill snapshots | `refill` (`=45CD24/28`) |
| `0x440E78` | `g_alloc_cursor` | U32 | bump cursor | `415130` |
| `0x440E7A` | `g_alloc_hi16` | U16 view of `+2` | cursor hi (overlaps `440E78`) | `[440E78+2] 4151F1` (26×) |
| `0x44D920` | `tbl_neg_44D920` | U32 stride 4 | `-x/x` (`(edx&0xFFF)*4`) | `415130`, `41e9f0` |
| `0x451920–0x451D50` | `s4_cursor/state/save_*` | U32 (+ U8 views) | S4 model state (`*0x404`) | `41e5f0,41e640,41e9f0` |
| `0x452154,0x453560–0x45375C` | `s4_row_lim / tbl_* / e640_*` | U32/U8 | row limits / init stores / `e640` scalars (`45356C` base) | `41e9f0,41e5f0,41e640` |
| `0x455F60/0x456F78/0x457360…0x458F98` | `cnt_*/tbl_*/s_*` | U32 | counters / history tables | `41dbc0,4177a0,419970,ppm` |
| `0x459E38–0x459E58` | `ctx_*` | U32 (+ U8/U16 views) | 8-order ctx ptrs (`459E38`); `459E3E/42` = hi-word views of `459E3C/40`; `459E58` emit table | `416d10,417090` |
| `0x45A25D–0x45A27E` | `ppm_b_25D…27E` | U8 | PPM bytes (hi-byte-swap lvalues) | `ppm` (`mov ch,[45A25D]` …) |
| `0x45AF00–0x45AFD4` | `af*_*` | U32 | range/file-tab scalars (`AF00` array, `AF68/6C/70` = `rslot_*`, `AFB0/B4` = `filetab_ptr/pos`, `AFB8`…`AFD4` values) | `415130,41cf60,41dbc0,ppm,sym` |
| `0x45BA18–0x45BA50` | `shift_r*/cnt_*/dict_*` | U32 | LZP shift regs `BA18/1C/20`; `dict_size/mask/cursor` = `BA30/3C/34`; `lzp_trigger` = `BA50` (`=43120C`) | `415130,lzp,417da0` |
| `0x45BA58–0x45BA5E` | `ctx_b_58…5E` | U8/U16 | PPM ctx bytes; `45BA5E` = U16 seed table stride 2 (`fill16 0x2FF,0x20`) | `415130,41e0a0` |
| `0x45C45C` | `g_u16_45C45C` | U16 | `=6` | `415130` |
| `0x45C65A–0x45C694` | `ctx_b_65A…694` | U8 | PPM ctx byte cluster (flags `==0/!=0`, `61=isNewline`, `69=LZP byte`, …) — per-byte evidence in audit | 80+ refs (`sym,416d10,418590,417220…`) |
| `0x45CD10–0x45CD2C` | `io_out_cap/out_len/in_pos/out_pos/in_valid/in_total/out_total/in_cap` | U32 | `10=65536` flush thr.; `28` produced total; `24` consumed | `get_byte/put_byte/417da0` |
| `0x45D0B0–0x45D0C4` | `rc_carry_cnt/code_lo/code/range/pending_b/code_hi` | U32 | 64-bit code (`B4/C4`), `B0=esi` FF count, `C0>>24` pending | `426f70,rc` |
| `0x45D0B8/BC` | `rc_code/range` | U32 | decode code/range; `BC=0xFFFFFFFF` init | 43× / 62× |

**Indexed-only bases (+18):** `0x43407C/8C,0x4362C0,0x436976,0x458794/98,0x4593A0,0x45A281,0x45AA7A/7C/7E/82,0x45AC78/7A/7C/7E,0x45AFD8,0x45C05C` — U8/U32/U16 tables (see audit for stride).

**Non-`G/M` literals (+16):** `0x42E000/0x440000/0x45DA00` (image base / `.bss` bounds), `0x433BE8/0x436ED8/0x4370D8/0x451D54/0x459BA8` (heap-offset / reader slots), `0x45A258/78/80/AA80`, `0x45AE80`, `0x45B5F4`, `0x45C044/622` (offset args).

---

## 5. 0.3 — `uha03_dec4.c` (methods 22–35)

Arena `V3_ARENA 0x04000000` (64 MiB), `V3O` clamps to `ARENA-8`. Const slice `uha03_dec4_dg.c` copied 1:1 over `0x428000–0x4354CC`; `.bss` stays zero. Model blocks bumped from `hi=0x600000`. `dec3w_ctx`: `mem, hi, rcb/rctx, wcb/wctx, err, method, p_inb (0x3800000), p_outb (0x3D00000)`.

> `uha03_dec4.c` is the most "register-emulation" of the three: every worker uses `eax/ebx/ecx/edx/esi/edi/ebp` locals + `goto` labels, with `/* addr mov … */` comments. The names below are therefore *inferred from those comments*, not from C identifiers.

### 5.1 Pool / carve block pointers `0x4299B8…0x4441BC` (U32 singletons)

| Orig | Proposed | Size | Role | Evidence |
|---|---|---|---|---|
| `0x436E48` | `g_pool_cursor` | U32 + U16 `@+2` | bump hi-water cursor; hi word = size-class tag | `415130` `mov ebx,dword_436E48` (41 refs) |
| `0x4299B8` | `g_blk00_dict_master` | `dict+0x100` | dict window | `carve 444270+0x100` |
| `0x4299BC` | `g_blk01_dict_copy` | `0x800000` | dict copy | `carve 0x800000` |
| `0x4299C0` | `g_blk02_modelA` | `0x140000` | model | `carve 0x140000` |
| `0x4299C4` | `g_blk03_modelA_ff` | `0x200000` fill `0xFF` | model | `carve 0x200000` |
| `0x4299C8` | `g_blk04_modelB_ff` | `0x200000` fill `0xFF` | model | `carve 0x200000` |
| `0x4299CC–0x4299E8` | `g_blk05..11_modelC..I` | `0x40000/0x80000/0x1C0000` | model chain (7 carves) | `carve` sequence |
| `0x4299EC` | `g_blk13_modelK` | `0x40000` | model | `carve 0x40000` |
| `0x4299F0` | `g_blk14_ctx_coderA` | `0x61600` + tail `0x40600` | ctx coder (`414ec0` base) | `carve 0x61600` |
| `0x4299F4` | `g_blk15_ctx_coderB` | `0x61600` + tail | ctx coder | `carve 0x61600` |
| `0x4299F8/FC,0x429A00/04` | `g_blk16..19_small_A..D` | `0xC000/0x5FA/0xC0/0xC0` | small (`5FA` word-filled `1` @`+0x406`) | `carve 0xC000/0x5FA/0xC0/0xC0` |
| `0x4441BC` | `g_blk20_big_tail` | `0x25800` | big tail (only non-`4299` carve name) | `carve 0x25800` |

### 5.2 Const tables & primer accumulators

| Orig | Proposed | Type | Role |
|---|---|---|---|
| `0x428ED9` | `g_const_flag_428ED9` | U8 | file-boundary gate |
| `0x428EDC` | `g_primer_accumulator` | U32 | primer total → `444254` |
| `0x428F48` | `g_const_428F48` | U32 | const via `40cecc` → `44422C` |

### 5.3 Range coder triple + I/O

| Orig | Proposed | Type | Role | Evidence |
|---|---|---|---|---|
| `0x4441E8` | `g_rc_total` | U32 | range `total` (seed `2`/`0x800` in primer) | `41bef8` `mov dword_4441E8,edx` (33×) |
| `0x4441EC` | `g_rc_prob_lo` | U32 | interval low | `41bef8` `mov dword_4441EC,eax` |
| `0x4441F0` | `g_rc_prob_hi` | U32 | interval high (`lo+1`) | `41bef8` `mov dword_4441F0,eax` |
| `0x4450F0` | `g_rc_scale` | U32 | `range/total` | `423188` |
| `0x4450F4` | `g_rc_value` | U32 | value/state (byte `@+3` = top flag) | `423188` |
| `0x4450F8` | `g_rc_range` | U32 | range | `423188` |
| `0x4450FC` | `g_rc_carry_byte` | U8 | carry/normalization byte | `42335c` `mov byte_4450FC,al` |
| `0x444EE8` | `g_in_pos` | U32 | input cursor | `41e17c` |
| `0x444EF0` | `g_in_avail` | U32 | buffered avail | `41e17c` |
| `0x444EF4` | `g_in_abs_pos` | U32 | absolute input offset | `41e300` snapshot |
| `0x444EFC` | `g_in_size_cap` | U32 | `0x40000` cookie | `decode_b` |
| `0x42C9A8` | `g_in_buffer_base` | U32 ptr | input arena `0x03800000` (1 MiB) | `decode_b` `=p_inb` |
| `0x42CBB0` | `g_in_cb_slot` | U32 cb | indirect `call [42CBB0]` | `41e17c` (stubbed as `rcb`) |
| `0x42C9A0` | `g_status_err` | U32 | `3` EOF / `4` write / `6` cancel | `41e17c,41e300,40d010` |
| `0x42C9AC` | `g_out_buffer_base` | U32 ptr | output arena `0x03D00000` (64 KiB) | `decode_b` `=p_outb` |
| `0x444EEC` | `g_out_staged_len` | U32 | staged len ; `0` after flush | `41e300,41e3bc` |
| `0x444EE0` | `g_out_flush_thresh` | U32 | `0xFF00` | `decode_b`; `41e3bc` |
| `0x444EF8` | `g_out_total_pos` | U32 | total produced; loop `<444EE4` | `415f6c` |
| `0x444EE4` | `g_out_total_len` | U32 | expected `out_len` | `decode_b,415f6c` |
| `0x436E40/0x436E44` | `g_snap_in/out_pos` | U32 | flush snapshots | `41e300` |

### 5.4 Model init globals `0x4441B0…0x444280` (zeroed by `d3u_startup` after `0x00413893`)

Key singletons (full per-address table in §7):

* `0x444270/78/74/80` = `g_dict_size/mask/cursor/lo_cursor` (`dict_size = 1<<((method-22)%14+10)`, `mask = size-1`)
* `0x444258/25C/60/64/68/6C` = `g_ctx_key/val_A/val_B/ptr/top_ptr/state_ptr` (ctx key + walker pointers)
* `0x444254` = `g_primer_total` (first top-up threshold; `FFFFFFFFh` until primer runs)
* `0x444230` = `g_topup_thresh_B` (second top-up threshold)
* `0x44422C` = `g_cum_table_ptr` (container cum file-end table)
* `0x4441C0/C4/C8/CC/D0/D4/D8/DC/E0/E4/F4/F8/FC` and `0x444200…0x444250` = `g_model_*` scalars/tables (zeroed counts, pointer slots) — see §7 for each hex.
* `0x444284–0x444289` = `g_pad_284[6]` (U8 pad), `0x44428A` = `g_word_tbl_init_28A` (U16 array, `word_44428A[esi*2]`), `0x444C84` = `g_const_word_C84` (`6`).

### 5.5 Flag cluster `0x444E7E…0x444EBA` (BSS, all U8)

Zeroed by `d3u_startup` `0x00413EF7–0x00413FEA` (special inits: `444EA0`, `444EB6=7`, `444E7E/7F=ecx^eax`, `444E9A=bh`, …). Names from audit: `g_f7E…g_fBA` with semantic hints — `444E7A/48/9D` = match/escape flags (`g_escape_flag 444EA7`, `g_mode_state 444E82`, `g_primer_done 444EA0`, `g_header_low 444E99`), `444EBA` = window size, `444EB6=7`, `444EB7/EB8` = tests `0x0D`/`8`, `444EB9` = window flag. Full table in §7.

### 5.6 Scatter/hash/dict tables `0x42BCxx…0x43Fxxx`

`0x42D272/74` = `g_u16_delta_src/cum_dst` (0x4000-entry rebuild by `4230f0`), `0x435274` = `g_rebuild_flag`, `0x4370B0` = `g_dict_byte_tbl`, `0x4398B0` = `g_lin_extend_tbl` (U32 16 KiB, `-2048…2048`), `0x43D8B0…E0` = `g_hash_*` pointers/values, `0x43F4F0…528` = `g_bucket_*` (sparse stride `0x7C`), `0x43F6F0` = `g_chain_tbl_ptr`, `0x440708` = `g_chain_bulk_tbl`, `0x440AF0…B08` = `g_state_*`, `0x441F08…0x443D04` = `g_ref_*/g_model_*/g_len_*` tables, `0x444188…19C` = `g_match_ctx_*`, `0x444870/84/E4A` = `g_fill_dst_*` (`421284` targets).

---

## 6. What is *not* in the map (intentionally excluded)

* **Immediates/masks:** `0xFF, 0xFF00, 0xFFFF, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF, 0x1000000, 0x40000` (sizes), `0x800, 0x20000, 0x4000` (model constants). These are not state.
* **Instruction addresses in comments** (`0x004xxxxx` in `/* … */`) — asm provenance, not arena data. Keep one comment per function after renaming; delete per-instruction noise.
* **Generated tables:** `uha04_dec4_dg.c` (6,151 lines, one `0x0042E000` literal) and `uha03_dec4_dg.c` (3,414 lines, two `0x00428000` literals) are opaque const images. **Isolate them** — do not rename their contents. Map them as `DGROUP_04 / DGROUP_03` and copy via `memcpy`.

---

## 7. Full per-address lists (audit as returned)

> These are the raw sub-agent tables, reproduced so a reviewer can tick off coverage. Abridged comments are preserved; the table order is by hex address. **Not yet applied to code.**

### 7.1 0.6 (`uharc_dec4.c`)

*(229 distinct — full table as in §3; top 60 by frequency expanded in audit; remainder in sub-agent report. Copy the §3 tables + §3.3 remainder list covers all.)*

### 7.2 0.4 (`uha04_dec4.c`)

*(262 strict + 18 indexed-only + 16 non-G/M — full table in §4.1; each row includes stride where known. Triplicated prototype block at lines 430–550 contains no new names — integration artifact.)*

### 7.3 0.3 (`uha03_dec4.c`)

*(222 direct + 174 arithmetic bases — full table in §5.1–5.6. Arithmetic bases not in the 222, e.g. `0x429B98, 0x429BA8, 0x429BB8, 0x429CB8, 0x429E3A, 0x42A054/6C, 0x42A86C, 0x42AC6C, 0x42B06C/46C/86C/BDDC, 0x42BF6C, 0x42C492/6A6, 0x43F6EC, 0x42E54/128, 0x443360/B68, 0x444108/128/148`.)*

---

## 8. Suggested first struct layout (to review, not implemented)

```c
/* 0.6 example — sketch only */
typedef struct dec4_state {
    unsigned long rc_code, rc_range;               /* 4749D8/DC */
    unsigned long tri_total, tri_low, tri_high;    /* 45C67C/80/84 */
    unsigned long dict_size, dict_mask, dict_pos;  /* 45D10C/118/110 */
    unsigned long ctx_w0, ctx_w1, ctx_w2;          /* 45D0F8/FC/100 */
    unsigned long out_pos, out_len, out_total;     /* 434528/434514/43451C */
    unsigned long blk_end, next_blk, file_idx;     /* 45D12C/13C/144 */
    unsigned char *dict_buf;                       /* p_dict */
    unsigned char *fenwick1, *fenwick2;            /* p_6bc/p_6c0 */
    unsigned char *cost_lut;                       /* 429ED4 */
    unsigned char phase, model_id;                 /* 45E392/45E390 */
    /* … flag bytes, hash1/2, row/col tabs … */
} dec4_state;
```

The same sketch generalizes to `dec4v_ctx` (0.4) and `dec3w_ctx` (0.3) with their respective `blk_ptr_*` / `g_blk*` arrays. Fields that are not yet semantically certain stay `flag_e3xx`/`ctx_b_xx` — **do not guess**.

---

## 9. Risks & open questions for reviewer

1. **Aliasing:** `dword_45D150 = dword_423698 + 0x10000`, `la50[i]==la4c[i+1]`-style overlaps (core-5 precedent) — any struct packing must be union-aware. Three `p_69C…p_6B8` and `A_6C4/6CC/6D0` blocks are dead in 0.6; confirm before deleting.
2. **Overlapping views:** e.g. 0.4 `440E78` (U32) vs `440E7A` (U16 @+2), `459E3C` (U32) vs `459E3E` (U16 @+2), 0.3 `4441C0` (U32+U8 union), `444EB0` (dword+byte). Flag these in the struct with `union`.
3. **Signedness traps:** `BYTE1(x)|=N` no-ops, `char` vs `unsigned char` for `>=0x80`, `sar` vs truncating div (`d5_asr4` precedent). Names must record signed intent, not just address.
4. **Arithmetic bases** (174 in 0.3) are not yet classified as array vs singleton — they need the same stride analysis before Phase 1.
5. **Unused consts** (`0x4239D0, 0x423DF0, 0x42472C` in 0.6) — leave in place; do not prune.

---

## 10. Verification plan for the follow-on refactor

1. `make` (GCC) + `nmake -f Makefile.msvc` (MSVC6) — especially `d4_40DBC5` `for`-loop form (README §MSVC6).
2. `M32(0x45C640) < M32(0x45C654)` precedence fix already in tree — renaming must not regress it.
3. Oracle sweep: `D3X.UHA` (45 files), `gmud-06x` (44 files, 33 MB), `06_mx`, `gmud-04x`, `gmud-03x` (44 files), `gmud-04m3/z/03m3/z/06m3/z` (cross-version 33 MB).
4. No changes to `d4v_prices` semantics (BSS-zero src) or to `D4V_SP` stand-in.

---

## 11. Next step

Review this map. Once approved, Phase 1 is mechanical: introduce `#define`/`enum` names per table (or a `dec4_addr.h`), replace literals one file at a time, rebuild + oracle-sweep after each file. No code has been changed for this audit.
