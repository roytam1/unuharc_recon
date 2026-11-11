# Typed core-4 ports for 0.3 and 0.4

The 0.6 core-4 decoder was rewritten as a typed port
(`uharc_dec4_clean.c`): explicit state instead of a flat 32-bit address
arena, no DGROUP image, and the arena port kept as a reference. This file
records what the same job needs for the 0.3 and 0.4 core-4 decoders
(`uha03_dec4.c` + `uha03_dec4_dg.c`, `uha04_dec4.c` + `uha04_dec4_dg.c`).

## What the arena ports look like today

Both are hand-written ports of the original `.lst` (not Hex-Rays output),
so the code is readable; only the *state* is still arena-shaped:

* every model cell is reached through `G8/G16/G32/M8/M16/M32` macros that
  index `c->mem + <original 32-bit address>`;
* the original's loaded image (`.text`/`.data`) is copied into the arena
  as a byte blob and read from there.

## Measured facts (this repository, current sources)

| | 0.4 (`uha04_dec4.c`) | 0.3 (`uha03_dec4.c`) |
|---|---|---|
| port size | 9 743 lines | 10 801 lines |
| DGROUP image | 73 728 B (`uha04_dec4_dg.c`) | 54 476 B (`uha03_dec4_dg.c`) |
| arena cells referenced | 210 | 193 |
| macro references | 1 907 | - |
| cells accessed at more than one width | 7 | 8 |
| cells used as arrays (`M32(cell + k)`) | 77 | 48 |
| addresses inside the DGROUP window referenced by name | 3 | - |

The three DGROUP-resident names in 0.4 are `D4V_DGROUP_BASE` (the
memcpy), `D4V_TAIL_GATE_FLAG` (0x431209) and `D4V_PREFIX_SUM_TMP`
(0x43120C). Everything else is reached through the block pointers or
through computed offsets, so the blob cannot simply be dropped.

## The DGROUP overlap was a real hazard (now avoided)

In the *original* program the heap blocks are bumped from 0x18C4E upwards
while the DGROUP image is loaded at 0x42E000, so the allocations run over
the image:

```
DGROUP live bytes 41318/73728 (56.0%)   alloc cursor 0x18f54a4
```

Only 56 % of the image survives. The decoder still matches the native
tool on the corpus, which means the overwritten bytes are never read -
but it also means the arena's behaviour depends on the allocation order,
and a typed port (separate heap, separate const tables) must reproduce
the *surviving* bytes exactly. Poisoning the whole image except the two
live cells breaks the decode, so those bytes do matter.

Both C ports now avoid the overlap instead of reproducing it: the model
blocks are bumped from `hi = 0x600000`, above the globals window
(`uha04_dec4.c:135-138` and the 0.3 equivalent), so the DGROUP image
stays intact.  That is safe because the bytes the original's heap would
have overwritten are never read, and it is what makes a typed port
possible: the image can be lifted into const tables unchanged.

The image itself carries real data - 47 090 of its 73 728 bytes are
non-zero.  Of the 63 named cells inside the window, 33 are zero
(a typed struct's calloc-zeroing reproduces them) and 30 hold const
table bytes that a typed port has to carry over verbatim.

## Status

Done:

1. **Typed state.** `tools/gen_dec4_state.py` derives the state from the
   address header and `uha03_dec4_state.h` / `uha04_dec4_state.h` hold
   it; `test_dec4state.c` checks the accessors.
2. **Typed ports.** `tools/port_to_typed.py` rewrites a port so that the
   cells outside the loaded image become typed struct members.
   `uha03_dec4_clean.c` (145 cells moved) and `uha04_dec4_clean.c`
   (167 cells) are generated from the arena ports and are byte-identical
   to them on the regenerated corpora (0.3: 103/103, 0.4: 101/103 - the
   two failures are core 5). The dispatcher selects them by default and
   the arena ports with `-DDEC4_ARENA`.

   Both clean ports are now **maintained by hand on top of the generated
   version** - the `D4V_*` accessor overrides that keep the arena cells
   in the arena, the per-block model pointers in the context and the PPM
   order table are hand-written.  Re-running `tools/port_to_typed.py`
   would drop them, so the banner in the two files now says so.  The
   tool itself is unchanged apart from that banner.

Checked with `tools/diff_fuzz.py`: the typed ports and their arena
references agree byte for byte on mutated archives as well (103 trials
per version, bit flips, bursts and truncations, 0 differences).

The full corpora are compared against the arena references as well: the
0.4 clean port is byte-identical on all 103 archives of `uharc-rs-04/arc`
and the 0.3 clean port on all 103 archives of `uharc-rs-03/arc` (both
built with `-DDEC4_ARENA` for the reference).

Left:

3. **Const tables.** The cells inside the loaded image and the model
   tables (addressed through computed arena offsets) still use the arena.
   The inventory for 0.4 is now measured (see below), so what is left is
   the mechanical move once the table extents are settled.

   The first step is done: **413 of the 630 `D4V_*` overrides in the
   0.4 clean port were never used at all** and have been deleted (the
   generated `d4v_state` accessors take over).  A further **59** of the
   used overrides were moved into the typed state as well, so the
   override block is down to **159** entries (from 630) and the
   `d4v_state` struct is live for the cells it can hold.

   The remaining 159 are the ones that carry state, and they fall into
   these groups:

   | group | why it stays |
   |---|---|
   | image cell (`< 0x440000`) | its initial value comes from the DGROUP image |
   | the cell has views of another width | the narrow view stays in the arena, so the bytes are shared |
   | table-backed | a raw or indexed table reaches into the cell |
   | overlapping view | two named cells alias (0x440E78/0x440E7A, 0x459E3C/0x459E3E, 0x459E40/0x459E42) |
   | different accessor signature | the state header's accessor takes an offset the port does not pass |
   | init writes a non-zero value | `d4v_fill` / `d4vc_fill16` / plain stores put the value in the arena |
   | read through a second path | 36 cells, see below |

   The **shadow build** is the tool that settles the last group: it
   mirrors every moved cell's write into the arena copy, so a cell whose
   arena copy goes stale shows up as a decode difference, and a funnel
   check in `d4v_ptr` reports every arena access that lands inside a
   moved cell.  The two together give the 36 cells that cannot move:
   the PPM price slots 0x459E44..0x459E54 (read through the indexed
   `D4V_TBL_459E38_32(s, ord*4)` accessor), the range coder's code and
   range 0x45D0B8/0x45D0BC, the LZP shift registers 0x45BA1C/0x45BA20,
   the I/O counters 0x45CD10/0x45CD14/0x45CD1C/0x45CD24/0x45CD28, the
   PPM context bytes 0x45C679..0x45C692, the PPM order state
   0x45AF90..0x45AFA0, 0x45AFB4/0x45AFB8/0x45AFC0, 0x45AF3C/0x45AF40
   and 0x45AFCC.

   The image tables cannot simply be read out of the const ROM: they are
   indexed with values that are not bounded by the mask the code
   suggests (the price table `word_437968` is read at `(eax & 0xFFFF) * 2`,
   i.e. up to 128 KB past the image), so they rely on the arena's
   zero-filled tail.  Turning them into const tables therefore needs
   their extents first - the same blocker as everything else.

   **0.3 is a different shape.**  Its clean port has no override block:
   it addresses the fixed region through `G8/G16/G32(D3U_X)` (named) and
   `M8/M16/M32(...)` (computed) macros.  Measured over the 0.3 corpus
   (29 348 fixed-region bytes touched, only two cells with a non-zero
   post-init value: 0x42BC6E and 0x435274), **none** of its 159 named
   cells can move into the typed state: 114 are the base of a computed
   access (they are tables), 42 have a byte-array member (the arena port
   used more than one width on them), and 3 are read at two widths.  So
   0.3's typed state stays empty until its tables are typed; the 0.4
   port is the one that can make progress cell by cell.

   **Why the tables are the blocker.**  In the arena the tables and the
   named cells are *interleaved* in one contiguous address space:
   `dword_45AF00[eax*4]` walks straight over the named cells
   0x45AF04, 0x45AF08, 0x45AF0C and 0x45AF10, and the address map's
   "cell = distance to the next name" extents are systematically too
   small (the measured extents run past them by up to a kilobyte).  So a
   table cannot become a standalone array unless the cells it walks
   over are folded into the same array - and the cells' accessors have
   to become indices into it.  That means the typed state needs a
   *unified* layout (tables as arrays, cells as aliases into them),
   which is a change to `tools/gen_dec4_state.py` rather than a
   mechanical rewrite of the port.  The measured extents are the input
   that layout needs.

   **The layout that falls out.**  The tables are not independent:
   several of them are *views of the same array* at different offsets
   (the arena accesses `0x453560+eax`, `0x453564+eax` and `0x453568+eax`
   with the same index).  Merging the tables whose ranges overlap
   gives **36 groups** covering 96 635 bytes and 128 named cells:

   | group | base | end | bytes | groups merged |
   |---|---|---|---|---|
   | 1 | 0x433B90 | 0x433BE9 | 89 | 433B90, 433BA4 |
   | 2 | 0x433E77 | 0x43406D | 502 | 433E77, 433E8C, 433FCC, 43404C |
   | 3-6 | 0x43407C-0x43419C | | 12-103 | one table each |
   | 9 | 0x434328 | 0x436151 | 7 721 | 434328, 434338, 434438, 434538, 434550, 434D50, 435150, 435550, 435950, 435D50 |
   | 10-13 | 0x4362C0-0x436B8A | | 100-622 | 436450, 43691C+436976, 436B8A |
   | 14 | 0x437966 | 0x43F96B | 32 773 | PRICE_SRC, PRICE_SUM |
   | 15 | 0x44D920 | 0x451921 | 16 385 | NEG_44D920 |
   | 17 | 0x451B3C | 0x451D52 | 534 | 451B3C, 451D3C (+5 cells) |
   | 19 | 0x453560 | 0x455F60 | 10 752 | 453560, 453564, 453568, ... |
   | 22 | 0x457360 | 0x458778 | 5 144 | 457360, 457378 (+6 cells) |
   | 24 | 0x458788 | 0x458B9C | 1 044 | 458788, 458794, 458798, ... |
   | 25 | 0x458F94 | 0x459BA0 | 3 084 | 458F94, 458F98, 459398, ... |
   | 30 | 0x45AA7A | 0x45AE80 | 1 030 | 45AA7A, 45AA7C, 45AA7E, 45AA80, 45AA82, 45AC78, ... |
   | 31-32 | 0x45AF00 / 0x45AF14 | | 20 / 20 | 45AF00 (+4 cells), 45AF14 |
   | 35 | 0x45BA58 | 0x45C676 | 3 102 | 45BA58, 45BA5E, 45C05C, 45C044, ... |

   The group's byte array has to cover every byte the tables can
   reach, and the *measured* extent is only a lower bound: the index
   expressions are mostly unbounded loop counters (`+ eax * 4`), so the
   real bound is the table's semantic size - the loop bound or the data
   structure behind it.  That per-table reverse engineering is the
   remaining work; the measured extents (and the three hand-checked S4
   tables, 0x45AF00/0x45AF10/0x45AF14, which the corpus never
   reaches) are the starting point.

   **A latent deviation in the raw accesses - now fixed.**  The arena
   port reaches every cell through `M8/M16/M32`, which clamp the
   address: `V4O(o)` caps it at `V4_ARENA - 8`, so an out-of-range
   index reads the arena's last byte.  `tools/port_to_typed.py` rewrote
   the computed accesses into plain `c->fixed_mem + N + index`
   arithmetic and that clamp was lost, so an index past the end of the
   256 KB fixed region read out of bounds in the clean port where the
   arena port would have read a zero byte of its 64 MB buffer.  The
   unbounded expressions are the price table's
   (`word_437968[(eax & 0xFFFF) * 2]` and one `* 2` with a full 32-bit
   index), so a damaged archive can reach it.  The 281 raw accesses now
   go through `d4v_fx()`, which clamps the offset the same way and
   lands on a per-context gap byte - the clean port's stand-in for the
   arena's zero gap between the globals and the model blocks.  That
   matches the arena exactly for every offset that stays below the
   model blocks; only the absurd ones that reach the model differ
   (the arena would read model bytes there).

   **The layout has to stay byte-identical to the arena's.**  A read
   past a table lands on whatever follows it in the arena - the next
   table's bytes, or the zero tail of the fixed region - so a typed
   array that stops at the table's real end would hand back the wrong
   bytes for the index expressions that are not bounded (the price
   table is read up to 128 KB past the image).  Two ways out: size each
   group's array so that it covers the *whole* arena range the group
   can reach, gaps included, which keeps the layout byte-identical and
   the memory the same size as today; or bound the index expressions
   per table and collapse the gaps, which needs the semantic extents
   above and would shrink the fixed region from 256 KB to about 96 KB.
   The first is mechanical and can be done now; the second is the one
   that pays off, and it is the per-table reverse engineering.

   Measured on `uha04_dec4_clean.c`:

   * the port overrides **630** `D4V_*` accessors back to `d4v_ptr()`
     (546 of them address a BSS cell `>= 0x440000`, 84 an image cell
     `< 0x440000`), so the generated `d4v_state` struct is dead weight;
   * **80** distinct raw `c->fixed_mem + N` targets (**281**
     references) address tables that the typed struct does not cover;
     32 targets (67 references) are inside the DGROUP image and are
     read-only, 48 targets (214 references) are BSS tables that the
     decoder writes at run time;
   * the 13 raw targets that land *inside* a named cell are the
     table bases that the address map names (`d4v_tbl_45af00_raw`,
     `d4v_tbl_459398_raw`, `d4v_tbl_457360`, `d4v_ctx_b_45c65d_raw`,
     ...).

   The blocker is the **table extents**.  `tools/gen_dec4_state.py`
   sizes a member from the distance to the next named cell, but the
   named cells are table *bases*: `dword_45AF00[eax*4]`,
   `dword_459398[esi*4]` and `dword_457360[(eax&0xFF)*4]` are tables
   that run far past the next name, so the generated member sizes are
   not the table sizes and the generated struct cannot be used as-is
   for them.  The real extents have to be recovered per table from the
   data layout and the index bounds before a typed array can be
   declared; that is the work item 3 still needs.

   Two special cases are already clear:

   * the PPM price table `word_437968` is a **const** table: the
     prefix-sum loop in `d4vc_prices` adds the BSS-zero source
     `word_437966`, so it is a no-op and the table keeps the image's
     bytes.  Its read index is a full 32-bit value at some sites, so the
     typed array needs a defined tail (the image ends at 0x43F968);
   * the PPM context table `0x459E38..0x459E54` is a run of eight
     4-byte slots with two overlapping 16-bit hi-word views
     (`0x459E3E`, `0x459E42`); it needs a union, not eight members.

4. **Model-block attribution.** `blk00_dict` / `blk00_base` are named
   as if they were the dictionary block, but they alias the *whole* model
   region: `c->hi` is set to `<PORT>_MODEL_BASE` before the model blocks
   are carved and the dict block is the FIRST carve, so
   `blk00_base == MODEL_BASE` and `blk00_dict == model_mem` (verified at
   runtime in both ports).  Consequently
   `blk00_dict[X - blk00_base] == d4v_ptr(c, X)` for every model-region
   address `X`, and a `blk00_dict[...]` site whose address comes from
   another block pointer resolves correctly but is misattributed.  Such
   sites are being re-expressed with the owning block pointer
   (`c->blkNN`).  The `blk00_dict` spelling is only valid for
   model-region addresses; an instrumented build confirms that every
   `blk00_dict` access stays inside the model region on the whole
   corpus (58 in 0.3; 81 in 0.4 before the PPM order table was
   converted, 67 after).

   The PPM **order table** is the last dynamic (order-indexed) model
   access and is now converted in 0.4.  The stride-4 cell run
   `0x433B58..0x433B74` is exactly the contiguous block-pointer array
   `BLK_PTR_06..BLK_PTR_13`, so PPM order `o` owns model block `06+o`,
   and `dword_459E38[o*4]` is a 4-byte slot index inside that block
   (byte offset `price << 2`).  An instrumented build over all 17
   core-4 archives of the 0.4 corpus (9.7 M order probes) confirms that
   the cell value always equals the block base and that the slot index
   always stays inside the owning block - the highest byte touched per
   order is 262143 / 524287 / 524287 / 262143 / 262143 / 262143 /
   1835007 / 260887 against block sizes 0x40000 / 0x80000 / 0x80000 /
   0x40000 / 0x40000 / 0x40000 / 0x1C0000 / 0x40000.  The context
   therefore carries `blkord[8]` (typed block pointers) and
   `blkord_base[8]` (the arena bases, still needed because
   `d4vc_ppm_score_orders` returns the slot's arena address to its
   caller, which dereferences it).  `d4vc_ppm_score_orders` and
   `d4vc_ppm_rescale_orders` now address the slot block-relative.


1. **Typed state.** `tools/gen_dec4_state.py` derives the state from the
   address header: one struct member per cell, sized from the distance to
   the next cell, typed by the width the port uses. Cells aliased at
   several widths become byte arrays with width-checked accessors.

   ```
   python tools/gen_dec4_state.py uha04_dec4_addr.h uha04_dec4.c uha04_dec4_state.h d4v
   ```

2. **Const tables.** Extract the DGROUP bytes the decoder actually reads
   into named `const` tables (the 0.6 port does the same through
   `dec4c_rom2.h`), so the port no longer depends on a blob or on the
   heap layout.

3. **The port.** Move the arena port over function by function: the
   model blocks become typed pointers, the cells become struct members,
   the DGROUP reads become const-table reads. Keep the arena port as the
   reference implementation and select between them with a build switch
   the way 0.6 does (`-DDEC4_ARENA`).

4. **Verification.** Byte-exact against the arena port on the regenerated
   corpora (`tools/run_corpus.py --arc-dir uharc-rs-0X/arc`), then against
   the native tools. The 0.4 corpus is currently 101/103 (the two
   failures are core 5, not core 4) and the 0.3 corpus is 103/103.

## Risks

* The 77 (0.4) / 48 (0.3) dynamically indexed cells need arrays, so
  their sizes come from the address map and must match the original
  exactly.
* The 7/8 multi-width cells alias bytes inside dwords; they stay byte
  arrays, and the width is checked at the access site.
* The block pointers hold arena offsets today; in the typed port they
  become pointers, and the `D4V_ALLOC_CURSOR` bookkeeping disappears
  together with the arena.
