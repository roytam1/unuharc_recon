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

   **The table lifts are now checked by a tool.**
   `tools/check_lifted_tables.py` parses the array *body* of the image
   (0.4: `uha04_dec4_dg.c`, 73 728 bytes at 0x42E000; 0.3:
   `uha03_dec4_dg.c`, 54 476 bytes at 0x428000) and the array bodies of
   every lifted table, and compares them byte for byte.  It also checks
   that the element count in the table's comment matches the declared
   array.  It matters because the extraction has to be exact: the
   image's header comment holds the address `0x0042E000`, so a scan
   that does not parse the body shifts every table by one byte - and a
   shifted table still decodes most archives, so the corpus does not
   catch it.  The tool reports 27/27 (0.4) and 21/21 (0.3) tables
   matching.

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
   gives **35 regions covering 99 848 bytes**, with 169 named cells left
   as typed members.

   Each region's extent is the largest of four bounds, and all four are
   now available:

   | bound | source |
   |---|---|
   | the distance to the next named cell | the address map |
   | the highest offset the corpus reaches | the instrumented build |
   | the PPM plane the table lives in | the init's `d4v_fill` / `d4vc_fill16` lengths |
   | the end of the last named cell it covers | the cell's own type |

   The init's fills are what make the third bound work: they *define*
   the PPM planes (`d4v_fill(c, 0x800, 0x45A280, 0)` covers
   0x45A280..0x45AA80, `d4vc_fill16(c, 0x2FF, 0x45BA5E, 0x20)` covers
   0x45BA5E..0x45C05C), so every table inside a plane is bounded by
   the plane.  The three tables the corpus never reaches (the S4 path's
   0x45AF00, 0x45AF10 and 0x45AF14) come from the same code: the
   init copies 20 bytes from 0x45AF14 to 0x45AF00.

   With the extents known, the typed state can hold the regions as byte
   arrays, the named cells inside a region as indices into it, and the
   remaining 169 cells as typed members - 100 KB of state instead of the
   256 KB fixed buffer, and no address arithmetic over the globals.

   **What the conversion needs.**  Moving the tables into the typed
   state was prototyped (the tables become `fx_<region>` members of the
   context, one address -> storage mapper replaces the per-cell arena
   lookups) and it works, but it needs three things that the port alone
   cannot provide:

   * **The regions must cover every access**, not just the raw table
     accesses: the cell overrides inside a region reach further than the
     table does (the PPM context bytes 0x45C675..0x45C694 sit past the
     raw table's measured end), and the measured extent is only a lower
     bound, so a few tables still need their semantic bound.
   * **The init's fills have to be region-aware.**  A fill such as
     `d4v_fill(c, 0x1400, 0x455F60, 0)` spans three regions, and the
     mapper hands out one region per call, so a single memset only
     fills the first piece - the rest has to be split.
   * **The cells inside a region must become aliases into it.**  While
     they are separate struct members the region and the typed state
     hold two copies of the same bytes and disagree; the accessors have
     to index into the region instead.  That is the generator change.

   The 59 cells that are already typed are all outside the regions, so
   they do not collide with the tables.

   **The layout and the generator are ready.**  `uha04_dec4_layout.h`
   holds the layout and `tools/gen_dec4_state.py` takes it as an
   optional fifth argument: with it, the generator emits one byte array
   per region, turns the named cells inside a region into indices into
   that array and keeps the rest as typed members.  The port rewrite
   that goes with it is **done** now:

   * **The regions are one array.**  The layout's 21 regions had gaps
     between them, and a named cell in a gap would have lived in two
     places at once - the struct member the generated accessor reads
     and the `fixed_mem` byte the raw table accesses read.  The 21
     regions are merged into one contiguous span, and 0.3's nine into
     one.  One array means one branch in the address mapper and one
     boundary in the init's fills.
   * **The cells inside a region are reached with their offset.**  The
     generator's accessor for a cell inside a region takes the offset
     into the array, so the port's calls name it: 685 of the 0.4 port's
     calls now read `D4V_X_32(&c->st, 0)` rather than
     `D4V_X_32(&c->st)`.
   * **The multi-byte accesses are byte-wise.**  The fixed region is
     stored in more than one allocation now (the typed state's array
     and the arena's image copy), so a 16- or 32-bit access may
     straddle the boundary between them and must not read a contiguous
     block.  The 0.4 port's 238 raw multi-byte reads and 134 writes
     became `d4v_ld16`/`d4v_st16` and `d4v_ld32`/`d4v_st32`.
   * **The init's fills are split per region.**
   * **The image is in the array.**  The image is copied into the
     state's array at init, so a cell inside the image keeps its
     initial value from the image and a cell in the BSS starts at
     zero - which is what the arena held.

   **The image joins the array.**  The layout now covers the whole
   globals region, not just the BSS above the image: 0x42E000..0x45D0C8
   in 0.4 (192 712 bytes: the DGROUP image and the BSS above it) and
   0x428000..0x468000 in 0.3 (262 144 bytes: the DGROUP slice and the
   BSS above it).  So the globals are one byte array in the typed
   state, the image is copied into it at init, and every named cell -
   in the image or in the BSS - is an index into it.

   That is the end of the hand-written accessor overrides: the 0.4
   port's block is down from 136 entries to **none**, because the
   generated accessors are the ones the port calls (the 175 call sites
   that used the one-argument form now pass the offset).  The 0.3 port
   already reached every cell through the two-argument accessors, so
   no call site changed there.

   It also ends the second allocation for the globals: the ports kept
   the image in a separate 256 KB arena buffer, and that buffer is
   gone.  The 0.4 state grows by the image (73 728 bytes) and the
   buffer goes away (262 144), so the globals cost **188 KB less**; the
   0.3 state grows by 60 992 bytes and the buffer goes away, so its
   globals cost **201 KB less**.  The address mapper has one branch
   for the globals instead of two, and the fills have one boundary
   instead of three.

   The overlapping `D4V_ALLOC_CURSOR` (dword at 0x440E78) and
   `D4V_ALLOC_HI16` (word at 0x440E7A) are inside the region now, so
   the generator aliases them into the same array at their overlapping
   offsets - the union the plan asked for comes out of the layout
   rather than needing a hand-written one.

   The header and the port still reach a fixpoint: regenerating
   `uha04_dec4_state.h` or `uha03_dec4_state.h` after the change
   reproduces them byte for byte.

   **0.3 reaches its cells through the state too.**  The port used to
   address its named cells by absolute address - through the `G8`/`M8`
   macros and `d3u_ldNN`/`d3u_stNN` - so the state was only reached
   indirectly, through the address mapper.  **1457** of those accesses
   are now typed accessor calls, and the only ones left on the mapper
   are the table accesses whose index the code does not bound: 77 of
   them (the S4 tables, the range coder's delta and cumulative tables)
   and the seven reads of `D3U_TBL_4370B0`, whose base is a run-time
   value.  That is the same class as the 0.4 port's raw table accesses.

   Two things had to be got right for that:

   * **A two-argument accessor does not mean the cell is in the
     region.**  The generator gives a cell that is used at several
     widths a two-argument accessor too (it is a byte array), but
     before the image joined the array its bytes were the image copy in
     the arena buffer, so reading them through the state member and
     writing them through `d3u_stNN` would split each of them across
     two storages.  The filter is the cell's *address*, not the
     accessor's shape.  With the image in the array the split is gone,
     so the filter can go too.
   * **The generator has to see the typed port.**  It derived the
     widths of every cell from the arena port alone, so a cell the
     typed port reaches but the arena port never names got no accessor
     at all.  It now takes an optional sixth argument, a second
     source whose uses are merged in - but only for the cells inside a
     layout region, for the reason above.  The header and the port
     reach a fixpoint: regenerating the header from the converted port
     reproduces it byte for byte.
   * **The width of a cell is the width the code uses.**  The
     generator recorded a cell as used at all three widths when it saw
     it through one of the byte-wise helpers, whatever the helper's
     name said; it now takes the width from the helper's name, so the
     header carries the widths the port really uses.

   ```
   python tools/gen_dec4_state.py uha04_dec4_addr.h uha04_dec4.c \
       uha04_dec4_state.h d4v uha04_dec4_layout.h uha04_dec4_clean.c
   python tools/gen_dec4_state.py uha03_dec4_addr.h uha03_dec4.c \
       uha03_dec4_state.h d3u uha03_dec4_layout.h uha03_dec4_clean.c
   ```

   The group's byte array has to cover every byte the tables can
   reach, and the *measured* extent is only a lower bound: the index
   expressions are mostly unbounded loop counters (`+ eax * 4`), so the
   real bound is the table's semantic size - the loop bound or the data
   structure behind it.  That per-table reverse engineering is the
   remaining work; the measured extents (and the three hand-checked S4
   tables, 0x45AF00/0x45AF10/0x45AF14, which the corpus never
   reaches) are the starting point.

   **The group's size is also what keeps the memory down - and it
   cannot be shrunk yet.**  The plan says the two ways out are to size
   the array so it covers the whole arena range the group can reach
   ("gaps included"), or to bound the index expressions per table and
   collapse the gaps.  The first is what the ports do now, and it is
   why the state is not smaller than the buffer it replaced: the 0.3
   region is 0x428000..0x468000 (262 144 bytes) because the arena's
   globals - the DGROUP slice and the BSS above it - end at 0x468000,
   not because the decoder uses all of it.  Trimming it to the
   corpus-observed bound (0x4450FC) would save about 140 KB, but only
   if nothing writes above the bound, and the init's fills *do* write
   the whole fixed region - with zeros.  The bytes above the bound are
   zero, so trimming them would not change what the decoder reads; it
   would only change how much the state occupies.  It is worth doing
   once the extents are proven.

   **A fill that runs into the gap now stops there.**  The ports
   stand in for the arena's zero gap - the space between the globals
   and the model blocks - with a single byte, but a fill that ran into
   the gap handed its whole remaining length to `memset()` and wrote
   past that byte, over the context's own members and into the heap.
   `test_gapfill.c` drives a fill at each of the three gaps and
   checks that the members behind the gap byte still hold what they
   held; against the port as it was, it died with a heap corruption.
   A fill that reaches the gap writes the gap byte and stops: the
   arena's zero gap is one long run of zeros, so writing it once is
   the same as writing it many times.

   **What is left of the 0.4 image tables, and what it needs.**  The
   tables that are still on the image path are all read with an index
   the code does not bound.  One of them is now understood well enough
   to name the missing piece:

   * 0x43407C, 0x43408C and 0x43409C are three windows into *one*
     array, the PPM's byte probability list, 0x434000..0x43429B (668
     bytes): two flat runs of 0x09 and 0x0A, then a rising ramp, then
     a plateau of 0x7D (0x4341A4..0x43429B).  The 0x43409C access
     masks its index to a byte - it is a plain 256-entry byte lookup
     whose index is a byte read from the model - and is already lifted
     (it reads 0x43409C..0x43419B, inside the list).  The other two
     index the same list with the quotient `dd_ / D4V_TBL_45AFD0`,
     where `dd_` is `D4V_TBL_45AFA8 << 2` (a counter scaled by four)
     and `D4V_TBL_45AFD0` is the PPM's period.  So bounding that
     quotient to a byte would lift both, and the array's extent is
     already known.

     What is missing is the bound on the quotient.  The period is a
     run-time value - it is set to 1 at init (0x4157F7) and then from
     the model's counters (0x41616E, 0x417459, 0x41748B, 0x417729,
     0x417758) - so the source alone does not bound it.  The bound has
     to come from the model's semantics: how many symbols a context
     holds, and how the period relates to them.

     **The counters, measured.**  `D4V_TBL_45AFD0` (the period) is
     raised by 2 per escape (0x417450) and halved together with
     `D4V_TBL_45AFA8` and `D4V_TBL_45BA24` when it passes 0x800
     (0x41745F..0x417496), so the code bounds the *period* by 0x800.
     `D4V_TBL_45AFA8` (the count) is raised by 5 per modelled symbol
     (0x417441) and is halved with the period, so the *ratio* of the
     two - and with it the quotient - is not bounded by the code: it
     depends on how many symbols a context sees between two escapes.
     An instrumented build over the whole 0.4 corpus confirms the
     shape: the period never exceeds 0x800, the count never exceeds
     5121, and the quotient never exceeds **10** (the corpus's
     56 939 accesses see at most 10, and the access at 0x4181E4 sees
     at most 10 as well).  Ten is also what the two counters give if
     they are raised together (5/2 * 4 = 10), but the code does not
     force that, so the bound cannot be claimed.

     The consequence is that these two reads cannot become const
     tables: with an unbounded index the original reads whatever the
     image holds at `0x43407C + q`, which is not a fixed byte, so a
     typed table would have to cover the whole image to be faithful.
     They stay on the image path until the model's semantics give a
     bound.

   * 0x436976 is indexed by `11 * (byte_45C68C - 0xA6) + byte`, and
     the subtraction wraps when the byte is below 0xA6.
   * The price table at 0x437966/0x437968 is the one table that is
     *written* by the init - the prefix sum in `d4vc_prices` adds the
     source 0x437966 into the destination 0x437968 - so it is not a
     const table at all in 0.4.  (In 0.6, where there is no image, the
     source is BSS-zero and the sum is a no-op; that is the case the
     plan describes.)

   **Three more tables are lifted now - the symbol index is a byte.**
   The plan used to say that the symbol-index reads of 0x43429C,
   0x4342C4 and 0x434314 were unbounded.  They are not: at 0x41D23D
   the original stores `var_30` from `xor eax,eax` + `mov al,
   [var_24]`, so `var_30` is a byte, and the three reads at 0x41D30A,
   0x41D311 and 0x41D325 index with it.  The same holds for the
   `byte_434314` read at 0x41D5D0, whose index is the byte argument
   of sub_41D540.  All four accesses are now const-table reads.

   `dword_434538` is indexed by `D4V_TBL_451920`, the row counter of
   the 0x41ED8B loop: the loop starts the counter at 0 (0x41ED76) and
   stops it at 6 (0x41EDEA), so the index is in 0..5 and the table is
   the six dwords 0x434538..0x43454F - exactly the gap up to the next
   named cell 0x434550.  It is lifted as well.  The 0.3 port has the
   same table in the same shape: `dword_42A054`, indexed by the row
   counter `dword_43D8B0` of the 0x41BAEF loop (0x41BAD0 zeroes it,
   0x41BB4E stops it at 6), so the six dwords 0x42A054..0x42A06B
   are lifted too.

   **The image is not read-only, and the mutated ranges are known.**
   An instrumented build that diffs the live DGROUP image against the
   pristine copy at the end of every file decode finds exactly five
   mutated ranges over the 0.4 corpus:

   | range | what writes it |
   |---|---|
   | 0x43120C..0x43120F | the init's prefix-sum temporaries |
   | 0x433B42..0x433B90 | the block-pointer cells the init fills |
   | 0x436150 | the init |
   | 0x436ED1..0x436ED7 | the init |
   | 0x43796C..0x43F967 | the price table's prefix sum |

   Every lifted table lies outside these ranges, which is what makes
   the lift safe.

   **The tables that are read with a bounded index are already lifted.**
   The 27 0.4 tables and the 21 0.3 tables above are the ones whose
   extent the code proves; what the layout still holds is the rest of
   the BSS, addressed as raw tables with unbounded indices.

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

   **0.4: 27 tables lifted, 53 access sites.**  The port now reads them
   from `static const` arrays instead of `fixed_mem`.  Two facts had to
   be established first, and both were measured with instrumented
   builds rather than read off the source:

   * *The image is not read-only.*  A build that diffs the image region
     against the pristine DGROUP at the end of every decode finds
     **32 415 mutated bytes** over the 0.4 corpus: the PPM price table
     0x43796C..0x43F967 that `d4vc_prices` prefix-sums into, plus the
     cells the init writes (0x43120C..0x43120E, 0x433B42..0x433B90,
     0x436150, 0x436ED1..0x436ED7).  "There is no store at this base"
     is therefore *not* a proof - a lifted range has to be shown to be
     untouched, which is what the diff does.
   * *The bytes have to be extracted exactly.*  The array literal in
     `uha04_dec4_dg.c` sits behind a comment that contains the address
     `0x0042E000`; a regex that scans the whole file picks that `0x00`
     up and shifts every table by one byte.  The extraction must parse
     the array *body*.  (A shifted table still decodes most archives -
     it fails only where the shifted byte matters - so the corpus alone
     does not catch it; a build that compares the lifted regions
     against the live buffer does.)  `tools/check_lifted_tables.py`
     does the same comparison without an instrumented build.

   With those two settled, the remaining condition is a proven index
   bound, which the code usually supplies:

   | table | extent | bound |
   |---|---|---|
   | 0x433B90 | 5 u32 | branch at 0x418922: index <= 4; next cell 0x433BA4 |
   | 0x433BA4 | 17 u32 | branch at 0x418939: index <= 0x10; next cell 0x433BE8 |
   | 0x433E77 | 766 B | index = 2*byte_45C691 + dl, so <= 765 |
   | 0x433E8C | 256 u32 | index is a byte |
   | 0x433FCC, 0x43404C, 0x43409C, 0x43419C, 0x43431E, 0x434328, 0x434338, 0x434438, 0x436450, 0x43691C, 0x436B8A | 256 B | index masked to a byte |
   | 0x43429C, 0x4342C4, 0x4342EC, 0x434D50, 0x435150, 0x435550, 0x435950, 0x435D50, 0x4362C0 | 256 u32 | index a byte, or `esi = ebx & 0xFF` |
   | 0x434314 | 256 B | index a byte (`esi`, the symbol index `var_30`, or the byte argument of sub_41D540) |
   | 0x434538 | 6 u32 | index = the row counter `dword_451920`, in 0..5; next cell 0x434550 |
   | 0x434550 | 512 u32 | index masked to 0x1FF |

   What is left on the image path is exactly the set whose index the
   code does not bound: the quotient reads of 0x43407C and 0x43408C,
   the 0x436976 read and the price table 0x437966/0x437968 (whose
   destination the prefix sum writes, so it is one of the mutated
   ranges).  A build that records the largest index each lifted access
   sees reports 127 at most for the byte tables and 511 at most for
   the 512-entry one; a build that compares the live bytes against all
   27 tables at the end of every decode reports no mismatch over
   either corpus.

   **0.3: 21 tables lifted, 43 access sites.**  The same two checks
   apply to the slice in `uha03_dec4_dg.c` (0x428000, 54 476 bytes):
   the slice is mutated at run time too - the block-pointer cells the
   init fills, the accumulator at 0x428EDC, the range-coder tables
   above 0x42D272 - and the lifted ranges are the untouched ones:

   | table | extent | bound |
   |---|---|---|
   | 0x429E3A | 16 B | index masked to a byte; next cell 0x429E44 |
   | 0x429E44 | 16 B | index masked to 0x0F |
   | 0x429E30, 0x429E54, 0x429F54, 0x429BB8, 0x429CB8, 0x42BF6C, 0x42C438, 0x42C6A6 | 256 B | index a byte |
   | 0x429DB8, 0x429DE0, 0x429E08, 0x42A86C, 0x42AC6C, 0x42B06C, 0x42B46C, 0x42B86C, 0x42BDDC | 256 u32 | index a byte |
   | 0x42A054 | 6 u32 | index = the row counter `dword_43D8B0`, in 0..5; next cell 0x42A06C |
   | 0x42A06C | 512 u32 | index masked to 0x1FF; next cell 0x42A86C |

   The width of a lifted table follows from the width of the access,
   not from the width of the neighbouring tables: 0x429BB8 was lifted
   as a dword table first and the corpus caught it, because the access
   is a byte read.  What stays on the image path in 0.3 is the
   quotient-indexed reads (0x429B98, 0x429BA8), the wrapping read of
   0x42C492, the three single-byte cells the init writes or reads in
   place (0x428ED9, 0x42BC6C, 0x42BC6E), the block-pointer cells the
   init fills and the range coder's delta/cumulative tables.

3. **The port.** Move the arena port over function by function: the
   model blocks become typed pointers, the cells become struct members,
   the DGROUP reads become const-table reads. Keep the arena port as the
   reference implementation and select between them with a build switch
   the way 0.6 does (`-DDEC4_ARENA`).

4. **Verification.** Byte-exact against the arena port on the regenerated
   corpora (`tools/run_corpus.py --arc-dir uharc-rs-0X/arc`), then against
   the native tools. Both corpora are 103/103 with the current
   `uharc06_unpack.exe` (the 0.4 corpus's earlier 101/103 was core 5,
   not core 4, and is fixed).

## Risks

* The 77 (0.4) / 48 (0.3) dynamically indexed cells need arrays, so
  their sizes come from the address map and must match the original
  exactly.
* The 7/8 multi-width cells alias bytes inside dwords; they stay byte
  arrays, and the width is checked at the access site.
* The block pointers hold arena offsets today; in the typed port they
  become pointers, and the `D4V_ALLOC_CURSOR` bookkeeping disappears
  together with the arena.
