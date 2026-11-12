# unuharc_recon — portable UHARC 0.2–0.6 unpacker

A dependency-free, C89, single-threaded reader for UHARC archives, plus a
small `x` / `e` / `t` / `l` command-line tool. It was recovered by reverse
engineering the original DOS and Windows archivers and is byte-exact against
them wherever the codec has been ported.

Originals used as reference:

| Version | Binary | Role |
|---------|--------|------|
| 0.2 | `Uharc02.exe` | DOS32 archiver (run under MS-DOS Player) |
| 0.3 | `uharc_u.exe` | full 0.3 archiver |
| 0.3 | `UHCLS.EXE` | CLASS edition, writes `CLS\xFF` archives |
| 0.4 | `UHARC04.EXE` | 0.4 BETA archiver |
| 0.6 | `UNUHARC06.EXE` | 0.6b unpacker |

The 0.6 unpacker is the primary reference. The corresponding `.lst`
disassembly is preferred over the Hex-Rays `.c` output; several codecs are
only correct when translated from the listing.

## Status

| Version | Container | core 0 STORE | core 1 RLE | core 2 Huffman/LZW | core 3 ALZ/PPM | core 4 LZP/PPM | core 5 PPM2/LZW | Extraction |
|---------|-----------|--------------|------------|--------------------|----------------|----------------|-------------|------------|
| **0.2** | complete | yes | yes | yes (LSB LZW) | yes (8–43) | — | — | **complete** |
| **0.3** | complete | yes | yes | yes (LSB LZW) | yes (8–21) | yes (22–35) | yes (36–79) | yes |
| **0.4** | complete | yes | yes | yes (LSB LZW) | yes (8–23) | **yes** | yes (40–87) | 24–39 yes |
| **0.6** | complete | yes | yes | yes (MSB) | yes (8–23) | **yes** | yes (40–87) | 24–39 yes |

"Container" means header decryption, CRC, directory tables, listing, solid
stream split and per-file CRC verification.

0.6 core 4 is byte-exact against the native oracle on every archived
sample present: method 27 (`06_mx`), method 34 (`D3X.UHA`, 45/45 files)
and method 39 (`gmud-06x.uha`, 44/44 files, 33 MB) all extract
byte-identical (`x` rc 0, `t` rc 0). The last defects were a missing `byte_433E36` EXE post-filter
pre-transform, gated per-archive by the header method
(`uharc_filter.c`, `f6` flag; `byte_42251F[method]==4`, i.e.
archive methods 24–39 — per-file gating corrupts e.g.
`gmud-06m3`/`-06z` members), and an off-by-one in the `d4_40CDD1` init
fills (bound `0xC04` = indices 1..769, was 1..768): the missing
`AE38[768] = 0x4000` let that accumulator decay below zero and fire
spurious `d4_40E4FF` rescales. 0.4 core 4 is byte-exact against the native oracle on the archived
sample present: method 39 (`gmud-04x.uha`, 44/44 files, 33 MB) extracts
byte-identical (`x` rc 0, `t` rc 0). The port is `uha04_dec4.c` (init
`415130`, driver `417DA0`, workers `418830`/`415C40`/`416220`/`417E10`,
range coder `41CD30`), wired in `uharc_dec0.c` for `ver == 4`, with the
72 KB DGROUP image (`uha04_dec4_dg.c`) copied 1:1 into the arena at init.
Two defects were found against the asm: an inverted `jb` at `41715A`
(the `41715C` saturation block runs when `dl >= 0xFE`, not below), and
uninitialised worker autos — every worker local is explicitly
zero-initialised, matching the original's deterministic (in practice
zero) caller leftovers.
0.3 core 4 is ported in `uha03_dec4.c` with its DGROUP image slice
(`uha03_dec4_dg.c`, `0x428000..0x4354CC`), wired as `dec4u` for `ver == 3`
in `uharc_dec0.c`. It is byte-exact against `uharc_u.exe` on
`gmud-03x.uha` (method 35, 44/44 files). 0.3 core 5 (`uha03_dec5.c`) covers
methods 36–79 byte-exact on `img.uha` (78), `snd.uha` (79) and the method
53/59/60/77 sweep (`03_mz`, `trig_image`, `uharc_u.uha`, `gmud-03m3`);
method 79 needs a per-context increment table with entry `[2] = 0x40`
(`word_4296D8`, written at `0x0040DC39`).
Known -O2 hazard: three functions (`d4_41030F`, `d4_411F8B`,
`d4_412257`) need zero-initialised autos (`-ftrivial-auto-var-init=zero`
via `#pragma GCC optimize`), otherwise the build miscompiles them and
`06_mx`/D3X file 1 fail. Eleven further locals that GCC flags
`maybe-uninitialized` are explicitly zero-initialised. Two genuine port
bugs found on the way: `d4_t42711F`/`d4_t427179` had 0 where the image
has 0xC8, and the `v206` gate read `v8 & (0x45C640) < X` (a precedence
trap that made the cost gate always true instead of comparing
`M32(0x45C640)`), both fixed against the asm.

0.6 core 2 was verified against a generated `-mw` archive (`06_lzw.uha`) that
is not in the tree; none of the 0.6 samples present uses methods 2–7.

### Verified end to end

All results below mean: output is byte-identical to a native extraction and
every per-file CRC matches.

* **0.2** — `uha02.UHA` (7/7), `02_M0..M3` (STORE and three compressed
  modes, methods 9/21/33), `02-R.UHA` (RLE), `02-W.UHA` (LZW),
  `UH02MMP.UHA` (9/9, `-mm+`), `UHA02EXE.UHA` (rebuilt EXE has the same MD5
  as `Uharc02.exe`), `V_MMOFF.UHA` (10/10, including a 1.15 MB executable).
* **0.3** — `03_m0/m1/m2/m3/mx/mz`, `uharc_u.uha` (method 60, 1 MB
  dictionary), `gmud-03m3` and `gmud-03z` (44 files, 33 MB),
  `gmud-03x.uha` (method 35, core 4, 44 files), `img.uha` / `snd.uha`
  (methods 78/79, core 5).
* **0.4** — `04_m0/m1/m2/m3`, `uha04.UHA` (8 files, method 62), `gmud-04m3`,
  `gmud-04x` (44 files, 33 MB) and `gmud-04z` (44 files, 33 MB).
* **0.6** — `06_m0/m1/m2/m3/mz`, `BIG.uha` and generated multi-file solids
  (including model-reset boundaries), `gmud-06m3` and `gmud-06z`
  (44 files, 33 MB), plus `D3X.UHA` listing (45 files). Also all 103
  `uharc-rs/testdata/arc/*.uha` (STORE/RLE/LZW/LZP/PPM/ALZ, `-md1`…
  `-md32m`, `-mm±`, `-pwsecret`/`-ph`, EXE/fax/media filters, kennedy.xls);
  `x` output compared byte-identical to `uharc-rs/testdata/src`. Run it
  with `python tools/run_corpus.py` (integrity check + extraction +
  byte-for-byte comparison, non-zero exit on any difference). Core 4 of
  0.6 uses the clean typed port; it agrees with the arena reference
  byte-for-byte on the corpus, and a mutation fuzz (bit flips, bursts and
  truncations of every corpus archive) shows the same extracted bytes on
  damaged input — the arena reference faults on some of those, the clean
  port reports a data error instead.

The three `gmud-*m3` / `gmud-*z` sets extract to the same 44 files
(33,705,622 bytes), which cross-checks the 0.3, 0.4 and 0.6 ports against
each other.

### Known failures

No known failures on the samples listed above.

## Build

Built with GCC or Clang in C89 mode (`-std=c89`); the core 4 translation uses
a few GNU extensions, so a strict C89 compiler is not enough. On Windows the
project is built with MSYS2:

```bat
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%
mingw32-make
```

or compile the CLI directly:

```bat
gcc -std=c89 -Wall -Wextra -O2 -o uharc06_unpack.exe ^
  uharc06_main.c uharc06.c uha04.c uha03.c uha02.c ^
  uhcrypt06.c uhcrypt04.c uharc_split.c uharc_filter.c uha02_mm.c ^
  uharc_dec0.c uharc_dec2.c uharc_dec2_lsb.c uharc_dec3.c ^
  uha02_dec3.c uha03_dec3.c uha04_dec3.c ^
  uharc_dec4.c uharc_dec5.c uha03_dec5.c uha04_dec5.c
```

`make` also builds the header and decoder test programs (`hdrtest*.exe`,
`test_tab.exe`, `test_dec3.exe`) and the two core-4 self-tests:
`test_dec4state.exe` checks the generated state headers, and
`test_gapfill.exe` drives the 0.3 and 0.4 fills into the stand-in for
the arena's zero gap and checks that they stop there.

### MSVC6

The tree also builds with MSVC6 `cl`/`link` (C89 only; no C99 features).
`uharc_dec.h` maps `long long` to `__int64` under `_MSC_VER` (`UHI64` /
`U64LL` / `I64LL`), and trace output uses a plain `D4TRACE` function since
MSVC6 has no variadic macros:

```bat
"C:\Program Files (x86)\Microsoft Visual Studio\VC98\Bin\VCVARS32.BAT"
nmake -f Makefile.msvc
```

This produces `uharc06_msvc.exe` from the same sources as the GNU
Makefile's `uharc06_unpack.exe`. Run from a local path or mapped drive —
CMD.EXE cannot use a UNC working directory. Verified: `t D3X.UHA`,
`gmud-06x`, `gmud-06m3` and `gmud-06z` all pass with `/O2`.

One MSVC6 `/O2` miscompile is worked around in-tree: the nested
`do/while` cost-table setup loop at the top of `d4_40DBC5`
(`uharc_dec4.c`) silently leaves tables zeroed (cell `0x457D68` stays 0
instead of `0x1680`, breaking `D3X.UHA` at output byte 694), so it is
written as plain `for`-loops. Behaviour is identical under GCC (D3X
extraction is byte-identical before and after the rewrite).

## Usage

```
uharc06_unpack.exe <x|e|t|l> [-pwpassword] <archive[.uha]> [outdir]
```

| Command | Meaning |
|---------|---------|
| `x` | extract, recreating directory names |
| `e` | extract flat (no directory names) |
| `t` | test archive integrity, write nothing |
| `l` | list directory only |

The version is auto-detected from the magic (`UHA\x02`, `\x03`, `\x04`,
`\x06`). If the name has no extension, `.uha` is appended. Output defaults to
the current directory.

```bat
uharc06_unpack.exe l gmud-06m3.uha
uharc06_unpack.exe t gmud-06z.uha
uharc06_unpack.exe x uharc_u.uha out
```

The tool accepts only the four commands above; original UHARC switches
(`-d`, `-y`, `-o`, `-p`, `-m`) are not implemented, except `-pwpassword`
(joined, or `-pw password` separate), which supplies the password for
`-pw` protected archives (0.3/0.4/0.6). Without it they fail with
`Password required`; with a wrong one the table CRC check fails.

## Source layout

```
uharc06_main.c                 CLI: version detect + dispatch
│
├─ uha02.c   uha03.c   uha04.c   uharc06.c
│  container per version: header, crypto, CRC,
│  directory tables, solid-stream split, file output
│     ├─ uharc_split.c         shared 0.3/0.4 splitter + whole-file filter hook
│     ├─ uharc_filter.c        shared 0.3/0.4/0.6 EXE post-filter
│     └─ uhcrypt04.c / uhcrypt06.c
│                              default-password table cipher
│                              (0.3 reuses the 0.4 cipher)
│
└─ uharc_dec0.c                method → core dispatch, core 0/1
   ├─ uharc_dec2.c             0.6 core 2, MSB-first
   ├─ uharc_dec2_lsb.c         0.2/0.3/0.4 core 2, LSB-first LZW
   ├─ uharc_dec3.c             0.6 core 3
   ├─ uha03_dec3.c             0.3 core 3
   ├─ uha04_dec3.c             0.4 core 3
   ├─ uha02_dec3.c             0.2 core 3
   ├─ uharc_dec4_clean.c       0.6 core 4 — production decoder (typed port)
   ├─ uharc_dec4.c             0.6 core 4 — arena reference port
   ├─ uha04_dec4_clean.c       0.4 core 4 — production decoder (typed port)
   ├─ uha04_dec4.c             0.4 core 4 — arena reference port
   ├─ uha03_dec4_clean.c       0.3 core 4 — production decoder (typed port)
   ├─ uha03_dec4.c             0.3 core 4 — arena reference port
   ├─ uha04_dec4.c             0.4 core 4 (methods 24–39) + DGROUP image
   │                            (`uha04_dec4_dg.c`, copied into the arena)
   ├─ uharc_dec5.c             0.6 core 5
   ├─ uha03_dec5.c             0.3 core 5
   └─ uha04_dec5.c             0.4 core 5
```

| File | Purpose |
|------|---------|
| `uha02.[ch]` | 0.2 container: 48-byte header, 4 tables, solid stream, own table cipher, own E8/E9 filter |
| `uha03.[ch]` | 0.3 container: 44-byte header, 4 tables, solid stream |
| `uha04.[ch]` | 0.4 container: 44-byte header, 4 tables, solid stream |
| `uharc06.[ch]` | 0.6 container: 39-byte header, 3 tables, solid stream, EXE filter |
| `uharc_split.[ch]` | split one decoded stream into files, CRC-check, optional whole-file post-filter |
| `uharc_filter.[ch]` | 0.3/0.4/0.6 EXE filter (E8/E9 plus MZ relocation table) |
| `uhcrypt06.[ch]` | 0.6 table cipher (default password) |
| `uhcrypt04.[ch]` | 0.4/0.3 table cipher (same algorithm) |
| `uharc_dec.h` | decoder API: `decode`, `decode_v`, `decode_b` |
| `uharc_dec0.c` | method/core tables per version, core 0 STORE and core 1 RLE |
| `uha02_mm.[ch]` | 0.3/0.4/0.6 multimedia filter, wired in (header +9 bit1) |
| `hdrtest*.c` | per-version header/crypto tests |
| `test_tab.c`, `test_dec3.c` | 0.6 directory/table tests |
| `MAKE02.BAT` | builds the 0.2 corpus and archives on a DOS machine |
| `tools/pty_run.py` | captures console output from the original archivers via `pywinpty` |
| `tools/run_corpus.py` | regression runner: per-archive integrity check, extraction and byte-for-byte comparison against a reference tree |
| `tools/check_lifted_tables.py` | checks the const tables the 0.3/0.4 core-4 ports lift out of the DGROUP image against the image, byte for byte |
| `UHCLS_CREATE.md` | notes and patcher for the CLASS edition (`CLS\xFF` archives) |

## Codec model

Each method ID selects one of six codecs. The ranges are **version-specific**;
a method number from one version must not be reused for another.

| Version | 0 STORE | 1 RLE | 2 Huffman/LZW | 3 ALZ/PPM | 4 LZP/PPM | 5 PPM2/LZW |
|---------|---------|-------|---------------|-----------|-----------|--------|
| 0.2 | 0 | 1 | 2–7 | 8–43 | — | — |
| 0.3 | 0 | 1 | 2–7 | 8–21 | 22–35 | 36–79 (`80` → STORE) |
| 0.4 | 0 | 1 | 2–7 | 8–23 | 24–39 | 40–87 |
| 0.6 | 0 | 1 | 2–7 | 8–23 | 24–39 | 40–87 |

Cores 3, 4 and 5 are version-specific coders. The 0.2 core 3 is a range coder
with PPM/LZ models; 0.3, 0.4 and 0.6 each have their own core 3. The 0.6
core 5 is a descendant of the 0.4 core 5 but not identical.

Dictionary size is `1 << (((method - base) % span) + 10)`:

| Version | core 3 | core 4 | core 5 |
|---------|--------|--------|--------|
| 0.2 | `base 8, span 12` | — | — |
| 0.3 | `base 8, span 14` | `base 22, span 14` | `base 36, span 14` |
| 0.4 | `base 8, span 16` | `base 24, span 16` | `base 40, span 16` |
| 0.6 | `base 8, span 16` | `base 24, span 16` | `base 40, span 16` |

## Test corpus

The archive corpus is not tracked in the repository; the files below are the
oracles used during development. `MAKE02.BAT` regenerates the 0.2 set on a DOS
machine; `uharc_u.exe`, `UHARC04.EXE` and `UHARC06.EXE` regenerate the other
sets on Windows.

| Oracle | Version | Covers |
|--------|---------|--------|
| `uha02.UHA` | 0.2 | 7 real files, method 25 |
| `02_M0..M3` | 0.2 | corpus of 5,840 bytes in STORE/FAST/NORM/BEST |
| `02-R.UHA`, `02-W.UHA` | 0.2 | secret `-mr` (RLE) and `-mw` (LZW) switches |
| `UH02MMP.UHA`, `UHA02EXE.UHA` | 0.2 | `-mm+` media predictor, EXE filter |
| `uha04.UHA` | 0.4 | 8 files, method 62 |
| `03_m0..mz`, `uharc_u.uha` | 0.3 | methods 0/39/53/67/60 |
| `gmud-03*`, `gmud-04*`, `gmud-06*` | all | 44 files, 33 MB, multi-block, real-world paths |
| `D3X.UHA` | 0.6 | 45 files, method 34, directory only |
| `06_m0..mz` | 0.6 | 5,840-byte corpus, STORE + core 3 + core 5 |
| `BIG.uha` + synthetic solids | 0.6 | model resets, 1–3 block boundaries |

`-mr` and `-mw` are undocumented switches accepted by `Uharc02.exe`,
`uharc_u.exe` and `UHARC06.EXE`; they were the only way to obtain core 1 and
core 2 test vectors:

```bat
UHARC06.EXE a -d0 -y+ -o+ -p- -mr rle.uha files...
UHARC06.EXE a -d0 -y+ -o+ -p- -mw lzw.uha files...
```

Vary the dictionary with `-md1`, `-md8`, `-md64`, `-md1024` to move the LZW
method ID across 2–7.

## Remaining work

1. **0.6 core 4** — done and byte-exact on all samples present
   (methods 27/34/39). The full translation is in `uharc_dec4.c`.
2. **0.4 core 4** — done and byte-exact on the sample present
   (method 39). The full translation is in `uha04_dec4.c`.
3. **0.3 core 4** — done and byte-exact on the sample present
   (method 35, `gmud-03x.uha` 44/44). The port is `uha03_dec4.c` plus the
   DGROUP slice `uha03_dec4_dg.c`.
4. **Multimedia filter** — wired into 0.3/0.4/0.6 extraction (header +9
   bit1, size ≥ 0x400; decode order EXE then multimedia). Verified with
   0.6 `-mz` / `-mw -mm+` archives (`BUNTITE.BMP`, `B261024.WAV`) and
   all 103 `uharc-rs/testdata/arc` archives (incl. EXE/fax/media filters
   and the kennedy.xls PPM special case, header +9 bit6/bit7).
5. **Password-protected archives** — 0.3/0.4/0.6 `-pw` archives open with
   `-pwpassword` (tables and packed stream use the password cipher; a
   missing password fails fast, a wrong one fails the table CRC). 0.2
   password archives are not handled.
6. **Non-solid archives** — extraction assumes one solid stream per archive
   and uses the header method. Per-file method bytes (`entry+2`) are stored but
   not used; mixed-codec archives are untested.
7. **0.6 skip markers** — `method == 0xFF` entries with non-zero size are
   rejected rather than skipped.
8. **CLI and test automation** — no switches, no recursive output. A
   scripted regression runner exists (`tools/run_corpus.py`): it runs the
   container's own integrity check on every archive of a corpus directory,
   extracts each one and compares the result byte for byte with a
   reference tree, and exits non-zero when anything differs.
9. **Damaged archives** — the clean 0.6 core-4 decoder and the arena
   reference diverge on corrupt input (they walk different model paths),
   so they do not necessarily stop at the same byte. On one fuzz sample
   the arena reports a data error after 296 KB while the clean decoder
   keeps going and, because the decoded bytes turn into long matches,
   spends tens of milliseconds per symbol; valid archives are unaffected.
10. **0.4 core 5 with a small dictionary** - the 0.4 archives that the
    native `UHARC04.EXE` produced with `-md1`/`-md64` (`md1_m1.uha`,
    `alz2_md64.uha` of a regenerated 0.4 corpus) previously failed to
    decode in `uha04_dec5.c` (2/103 of the 0.4 corpus). The native tool
    extracts both archives correctly, so the stream is good and the
    port was at fault. The 0.3 and 0.6 equivalents of the same data are
    fine. Fixed: the 0.4 core-5 corpus is now **103/103**.

    What is known: the dictionary is *not* the problem. Dumping the
    dictionary and the output history at the same moment and comparing
    them byte for byte shows the dictionary mirrors the output exactly
    (64 KB, no mismatch), so the match copy is faithful and the
    divergence is upstream of it. In `alz2_md64.uha` the first wrong
    byte is at output 196 620, where both decoders take a match but
    decode different distances: the native copies from 58 576 bytes
    back (`text fox compres`), we copy from 9 386 (`lazy dog`). The
    distance is decoded in three range-coded stages (`d5v_dbase` /
    `d5v_drange`, the 4096-scale `dprob` branch, the 0x800-scale stage
    and the final stage).

    The two distances even belong to different *classes*: the native's
    58 576 is class 26 (`d5v_dbase[26]` = 49 152, range 16 384) and
    ours 9 386 is class 21 (base 8 192, range 4 096). The class is the
    symbol decoded from the distance PPM model, so the range coder and
    the offset stages are not the problem either - the *symbol model*
    (the per-context frequencies `cnt`/`fen` and the escape counts
    `ext` of the distance contexts) had already drifted before this
    symbol. That puts the fault in the model update path
    (`d5v_update` / `d5v_fenadd` / `d5v_rescale`), the same class of
    defect as the two 0.6 core-4 bugs.

    Checked against `UHARC04.lst` and found faithful: the count update
    (`sub_40EFE0`, including the escape path), the context rescale
    (`sub_40EE90` - the per-lane halving, the tree rebuild and the
    escape decay), the increment table `word_44D0F4` (33, with 10 for
    contexts 1009..1016 and 44 for 1017..1026) and the escape weight
    table `word_44C8EE` (15, with 8 for contexts 993..1016 and 6 for
    context 768). The symbol decode (`sub_410440`) and its price
    (`sub_410990`) were checked too and match: the `s913` adjustment
    subtracts `counts[ctx][s914]`, the escape price is
    `LUT[(ext << 14) / (ext + total)]`, the symbol price is
    `LUT[(count << 14) / (total + ext - counts[ctx][s914])]`, and
    `ext` is only added for contexts below 0x403.

    The match-copy path had a real bug (sub_40FB80): when the copy
    wrapped the dictionary ring, `s914` was read as
    `dict[(src + 2*len) & mask]` instead of `dict[(src + len) & mask]`
    because `src` had been advanced inside the byte-by-byte loop. The
    end position is now saved before the copy. This caused the
    small-dictionary failures (both affected archives use a 64 KB /
    128 KB dictionary and wrap frequently): wrong `s914` fed the wrong
    exclusion into the next symbol decode's `s913` adjustment, drifting
    the distance-model state until a different class was decoded.
    Verified: `uharc-rs-04/arc` is now **103/103**.



### Naming conventions

The ports are written from the original disassemblies, so the audit in
`tools/audit_registers.py` keeps them honest: it reports the x86
register names used as ordinary locals (there must be none) separately
from the Hex-Rays spill names (`v1`, `v2`, ...). Every file this project
ships is free of register-named locals; the spill names survive only in
the 0.6 arena reference, which is kept as the oracle for differential
debugging. `tools/rename_local.py` renames one local at a time inside
the function that declares it.

### Core 4 implementation notes

All three versions have two core-4 decoders:

| version | typed port (default) | arena reference |
|---|---|---|
| 0.6 | `uharc_dec4_clean.c` | `uharc_dec4.c` |
| 0.4 | `uha04_dec4_clean.c` | `uha04_dec4.c` |
| 0.3 | `uha03_dec4_clean.c` | `uha03_dec4.c` |

The typed ports keep the globals in the typed state: one byte array
that holds the loaded DGROUP image and the BSS above it, so every
named cell is an index into it (`uha03_dec4_state.h`,
`uha04_dec4_state.h`, generated by `tools/gen_dec4_state.py` with the
layout in `uha03_dec4_layout.h` / `uha04_dec4_layout.h`).  Only the
model blocks and the I/O buffers are separate allocations, and the
ports address them by typed pointer, not by absolute address.  The
arena ports stay in the tree as the reference implementation and are
not linked into the production binary: `make` builds the typed ports,
and `make arena` builds `uharc06_arena.exe` with `-DDEC4_ARENA` for
differential testing (`tools/diff_fuzz.py`).  All three versions produce
byte-identical output either way; the regenerated corpora are checked
with `tools/run_corpus.py`.

The read-only tables that the decoder reads out of the loaded image are
lifted into `static const` arrays - 30 of them in 0.4 and 25 in 0.3.
`tools/check_lifted_tables.py` compares every one of them with the
image byte for byte and checks the element counts, because the
extraction has to be exact: the image's header comment holds the
address `0x0042E000`, so a scan that does not parse the array *body*
shifts every table by one byte - and a shifted table still decodes most
archives, so the corpus alone does not catch it. The tables that are
left on the image path are the ones whose index the code does not
bound; `DEC4_CLEAN_PLAN.md` says which they are and why.

`uharc_dec4.c` is a statement-by-statement translation of the 0.6 dispatcher
case `sub_40F962`, using a flat 32-bit address arena so that the original
DGROUP aliasing is preserved.

The common Hex-Rays pitfalls in the arena unit are truncated 32-bit pointer
values, dropped pointer destinations, signed-`char` comparisons that miss
bytes `>= 0x80`, table values used as addresses, and `BYTE1(x) |= N` rendered
as a no-op. When continuing, compare against the native decoder from the
first divergent output byte and keep the `cum[]` file-end table in sync with
`dword_45D128`.

Two behaviours of the 0.6 core-4 model are easy to miss and both change the
decoded bytes:

* The multimedia **predicted byte** (`0x45e38b`) and the prediction flag
  (`0x45e38f`) are cleared whenever the model id changes
  (`0x40deb6`/`0x41013c` type switch). A stale predicted byte makes the
  following plain segment add the wrong context byte to the order-1 symbol.
* The order-1 MRU SEE index compares the **raw unsigned** distance
  `dict_pos - last_pos[s]` against 64. Masking it with the dictionary size
  turns a stale entry from before a dictionary wrap into a false "recent"
  hit, which changes the SEE index, the accumulated cost and the symbol.

Arena address notes for these tables: the order-1 cumulative/frequency
planes are `c->fenwick1`/`c->fenwick2` (784 contexts each), the escape
counts live in `c->ppm_tot`, the per-symbol "last position" table is
`0x4455AC`, and the "recent position" table is `0x444DA4`.

## Provenance

* Everything here is derived from static analysis of the four reference
  binaries: `.lst` disassembly plus Hex-Rays `.c` output. Function addresses
  in source comments refer to those listings.
* The library is plain C89 with no external dependencies and no threads.
  Decoder state lives in per-instance contexts; only the container modules
  keep a small CRC table cache. The only non-libc include is the platform
  `mkdir` header (`<direct.h>` on Windows, `<sys/stat.h>` elsewhere).
* "Verified" means the extracted bytes and CRC32 of every file match the
  original tool's output, not merely that decoding finishes.
* Test vectors for the DOS-only 0.2 build come from `MAKE02.BAT`, run on a
  DOS machine (or MS-DOS Player, `msdos.exe`).
* Console output of the original tools is captured with `tools/pty_run.py`
  (`pywinpty`); ordinary pipes stay silent because the archivers write to the
  console directly.

## See also

* [`FORMAT.md`](FORMAT.md) — on-disk format reference for all four versions.
* [`UHCLS_CREATE.md`](UHCLS_CREATE.md) — the 0.3 CLASS edition, its
  `CLS\xFF` signature, and `patch_uhcls.py`.
