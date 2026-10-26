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
| **0.3** | complete | yes | yes | yes (LSB LZW) | yes (8–21) | **missing** | yes (36–79) | 22–35 missing |
| **0.4** | complete | yes | yes | yes (LSB LZW) | yes (8–23) | **missing** | yes (40–87) | 24–39 missing |
| **0.6** | complete | yes | yes | yes (MSB) | yes (8–23) | **partial** | yes (40–87) | 24–39 partial |

"Container" means header decryption, CRC, directory tables, listing, solid
stream split and per-file CRC verification.

0.6 core 4 is the only partially ported coder. Method 27 (`06_mx`) now
decodes end to end (`t` = All files OK). Method 34 (`D3X.UHA`) runs without
crashing; `BIN/FILES.TXT` (498 B) is byte-exact and `BIN/D3XSTUB.ASM`
diverges at its byte 569 (stream offset 1067) - within one `d4_41030F`
invocation the range/code pair leaves in step with the native although the
entry state, hash tables, contexts and MTF rank arrays all match, so the
remaining defect is an internal branch of that function. Method 39
(`gmud-06x`, 32 MB dict) diverges at output byte 3. There is no 0.3- or
0.4-specific core 4 port; the dispatcher routes those methods to the 0.6
coder, which does not decode them.
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
  dictionary), `gmud-03m3` and `gmud-03z` (44 files, 33 MB).
* **0.4** — `04_m0/m1/m2/m3`, `uha04.UHA` (8 files, method 62), `gmud-04m3`
  and `gmud-04z` (44 files, 33 MB).
* **0.6** — `06_m0/m1/m2/m3/mz`, `BIG.uha` and generated multi-file solids
  (including model-reset boundaries), `gmud-06m3` and `gmud-06z`
  (44 files, 33 MB), plus `D3X.UHA` listing (45 files).

The three `gmud-*m3` / `gmud-*z` sets extract to the same 44 files
(33,705,622 bytes), which cross-checks the 0.3, 0.4 and 0.6 ports against
each other.

### Known failures

| Archive | Version | Reason |
|---------|---------|--------|
| `gmud-03x.uha` | 0.3 | core 4 (methods 22–35) not ported |
| `gmud-04x.uha` | 0.4 | core 4 (methods 24–39) not ported |
| `gmud-06x.uha` | 0.6 | core 4 not yet byte-exact |
| `D3X.UHA` (extract) | 0.6 | core 4 method 34: CRC failure on the first file |

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
  uhcrypt06.c uhcrypt04.c uharc_split.c uharc_filter.c ^
  uharc_dec0.c uharc_dec2.c uharc_dec2_lsb.c uharc_dec3.c ^
  uha02_dec3.c uha03_dec3.c uha04_dec3.c ^
  uharc_dec4.c uharc_dec5.c uha03_dec5.c uha04_dec5.c
```

`make` also builds the header and decoder test programs (`hdrtest*.exe`,
`test_tab.exe`, `test_dec3.exe`).

## Usage

```
uharc06_unpack.exe <x|e|t|l> <archive[.uha]> [outdir]
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
(`-d`, `-y`, `-o`, `-p`, `-m`) are not implemented.

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
   ├─ uharc_dec4.c             0.6 core 4 (partial)
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
| `uha02_mm.c` | 0.3/0.4/0.6 multimedia filter, verified by round-trip, **not wired in** |
| `hdrtest*.c` | per-version header/crypto tests |
| `test_tab.c`, `test_dec3.c` | 0.6 directory/table tests |
| `MAKE02.BAT` | builds the 0.2 corpus and archives on a DOS machine |
| `tools/pty_run.py` | captures console output from the original archivers via `pywinpty` |
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
| 0.3 | `base 8, span 14` | not ported | `base 36, span 14` |
| 0.4 | `base 8, span 16` | not ported | `base 40, span 16` |
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

1. **0.6 core 4** — finish the 0.6 coder. The full translation is in
   `uharc_dec4.c`; method 27 now decodes end to end, but method 34 and 39
   still diverge. The remaining defects are in the LZP candidate pass and the
   PPM escape path.
2. **0.4 core 4** — port from `UHARC04` `sub_41CE30` / `415130`.
3. **0.3 core 4** — port from `uharc_u` `sub_419E9C` / `415F6C`.
4. **Multimedia filter** — `uha02_mm.c` is implemented and round-trip tested
   but not wired into 0.3/0.4/0.6 extraction.
5. **Password-protected archives** — tables currently always use the default
   password; user-password archives are not handled.
6. **Non-solid archives** — extraction assumes one solid stream per archive
   and uses the header method. Per-file method bytes (`entry+2`) are stored but
   not used; mixed-codec archives are untested.
7. **0.6 skip markers** — `method == 0xFF` entries with non-zero size are
   rejected rather than skipped.
8. **CLI and test automation** — no switches, no recursive output, no
   scripted regression runner yet.

### Core 4 implementation notes

`uharc_dec4.c` is a statement-by-statement translation of the 0.6 dispatcher
case `sub_40F962`, using a flat 32-bit address arena so that the original
DGROUP aliasing is preserved. The common Hex-Rays pitfalls in this unit are
truncated 32-bit pointer values, dropped pointer destinations, signed-`char`
comparisons that miss bytes `>= 0x80`, table values used as addresses, and
`BYTE1(x) |= N` rendered as a no-op. When continuing, compare against the
native decoder from the first divergent output byte and keep the `cum[]`
file-end table in sync with `dword_45D128`.

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
