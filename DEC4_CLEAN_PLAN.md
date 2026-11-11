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

## The DGROUP overlap is a real hazard

In 0.4 the heap blocks are bumped from 0x18C4E upwards while the DGROUP
image is copied to 0x42E000, so the allocations overwrite the image:

```
DGROUP live bytes 41318/73728 (56.0%)   alloc cursor 0x18f54a4
```

Only 56 % of the image survives the allocations. The decoder still
matches the native tool on the corpus, which means the overwritten bytes
are never read - but it also means the arena's behaviour depends on the
allocation order, and a typed port (separate heap, separate const tables)
must reproduce the *surviving* bytes exactly. Poisoning the whole image
except the two live cells breaks the decode, so those bytes do matter.

0.3 avoids this: its comment at `uha03_dec4.c:291` says the model blocks
must live above the fixed DGROUP window, and the init honours that.

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

Checked with `tools/diff_fuzz.py`: the typed ports and their arena
references agree byte for byte on mutated archives as well (103 trials
per version, bit flips, bursts and truncations, 0 differences).

Left:

3. **Const tables.** The cells inside the loaded image and the model
   tables (addressed through computed arena offsets) still use the arena.
   Moving the tables to typed arrays needs the per-site work described
   under Risks; the arena's 0.4 heap overwriting 44 % of the image has to
   be reproduced exactly while that happens.
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
   `blk00_dict` access (84 in 0.4, 59 in 0.3) stays inside the model
   region on the whole corpus.


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
