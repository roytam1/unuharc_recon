# UHARC 0.3/0.4/0.6 unpacker (portable C89)

Derived from `UNUHARC06.EXE` (UnUHARC 0.6b, 01 Oct 2005) via
`UNUHARC06.lst`/`UNUHARC06.c`, `UHARC04` (UHARC 0.4 BETA,
28 Dec 2001) via `UHARC04.lst`/`UHARC04.c`, and `uharc_u`
(UHARC 0.3, full archiver) via `uharc_u.lst`/`uharc_u.c`.

## Status

0.6:
* Header parsing: DONE, verified on `D3X.UHA` and 06_m0/m1/mx/mz test vectors
  (magic `UHA\x06`, decrypt, CRC16, methods 0/43/27/11 for m0/m1/mx/mz).
* Directory: DONE, verified byte-exact (names/sizes/CRCs/total).
  Tables decrypt with default-password S-box (fresh per table, CRC over
  evolving S-box each 8 bytes). Tab1 CRC is cumulative tab0+tab1.
  Entry+3 sizes are deltas (prev_size - curr_size, 32-bit wrap).
  `D3X.UHA`: 45 files, 976594 bytes total, names/sizes match `UHARC06 l`.
  `06_m*` (6 files, 5840 bytes): names/sizes/CRCs match `l` + corpus.
* File extraction: STORE (method 0) DONE byte-exact (`06_m0.uha` 6/6 MATCH
  vs corpus, `x` + `t` exit 0). Compressed modes pending:
  ALZ file data (method 11, e.g. `06_mz`) needs core3 multi-block
  (single-block tables work, larger streams hit thresh/reset gap -> DATA);
  PPM/LZP (method 27, `06_mx`) needs core4 model; PPM2 (method 43,
  `06_m1`, `D3X` method 34) needs core5 model + exe/multimedia filters
  (e.g. D3X flags 00/01, `06_mz` flags 00/03). `06_m1` decodes but CRC
  fails (filter/model gap); `06_mx/mz` fail DATA; `D3X t` fails CRC.
* Container `uharc06.[ch]`: header + directory + solid-stream splitter
  with CRC per file (tab0 full32), empty-file + 0xFF-skip handling,
  `x/e/t/l` (l = list via directory, extension beyond original).

0.4:
* Header parsing: DONE, verified on `uha04.UHA` + 04_m0/m1/mx/mz vectors
  (magic `UHA\x04`, decrypt 36B key=raw42, CRC16, base=file packed len).
  Layout verified: 44 + base + tab0(4N) + cmp1+cmp2+cmp3 = filesize.
  `uha04.UHA`: 8 files, 43307 bytes, base 21470. Test corpus: 6 files,
  5840 bytes, base varies by mode (m0 5840 STORE, m1 1153, mx 1120, mz 1540).
* Directory: framework DONE (offsets/CRC slots), decode PENDING.
  Needs 04-specific core3 (330 contexts vs 06 319, order-0 at 328 vs 318)
  + core2 LSB-first (vs 06 MSB-first) ports (`uha04_dec3/2.c` TODO).
  Tables use same default-password crypt as 0.6 (byte-identical cipher).
  Ground truth via `UHARC04 l -d2` (names/sizes/dates/CRCs, e.g. text
  7125DAA3, repeat 51A02E01, binary B70B4C26, random 021E1879).
* File data: per-file methods (entry+2), solid helpers. PENDING (needs
  cores + filters). `uha04_extract` stub returns DATA.

0.3:
* Header parsing: DONE, verified on `uharc_u.uha`
  (magic `UHA\x03`, same 44B decrypt/CRC as 0.4, key=raw42).
  `uharc_u.uha`: 1 file, 1047398 bytes unpacked (= extracted size),
  base 151747, ndirs 1, names 11, cmp 15/4/8, layout
  44+151747+4+15+4+8=151822 verified. Header +10 = 0x3C (60, core5).
  Ground truth via extraction with `uharc_u.exe x -y+ -o+ -d0`
  (console silent, exit 0; file `uharc_u.c`, 1047398 bytes).
  Console output is suppressed when piped (likely WriteConsole vs pipe;
  winpty fails with size assert in headless env) - ground truth via
  filesystem effects instead. Updated `uharc_u.exe` (218112 bytes,
  2026-10-24; original 218592 silent no-op for `a`) creates correctly:
  test vectors `03_m0/m1/m2/m3/mx/mz.uha` (6-file corpus, 5840 bytes)
  via `a -d0 -y+ -o+ -p- -m*`, all round-trip byte-exact (6/6 MATCH).
  Header methods: m0->00 core0 STORE, m1->27 core4, m2/mx/mz->35 core4,
  m3->43 core5 (content-selected, not 1:1 with -m switch; ALZ CLI modes
  yield PPM/LZP methods on mixed corpus - same pattern as 0.4/0.6).
  Directory tables identical across modes (names 62, cmp 50/13/25,
  crc BD63/25CA/3CF2).
* Directory: framework DONE (mirrors 0.4: 4 tables, methods 0/17/2/7,
  19B entries, delta sizes). Decode PENDING: needs 0.3-specific
  core3 (14 methods 8..21, ALZ:1/2/3 split) + core2 ports
  (`uha03_dec3/2.c` TODO; do not reuse 0.6/0.4 verbatim - different
  models/ranges; 0.3 core table `0/1/2..7/8..21/22..35/36..79+80=>0`).
  Tables use default-password crypt (assumed byte-identical to 0.4/0.6,
  TODO verify via directory CRC once cores work).

Decompressors (method->core tables are version-specific:
0.6/0.4 `0->0,1->1,2..7->2,8..23->3,24..39->4,40..87->5`;
0.3 `0->0,1->1,2..7->2,8..21->3,22..35->4,36..79->5` + `80->0` alias):
  - core 0 STORE (method 0): DONE, byte-exact (`06_m0` 6/6 MATCH)
  - core 1 RLE/delta (method 1): DONE (from 0.6 sub_40B504), no test vector yet
  - core 2 Huffman/LZ (methods 2..7 all versions): 0.6 MSB-first port compiles;
    0.4 needs LSB-first port (`uha04_dec2.c` TODO, `sar` vs `shr` in lst);
    0.3 needs own port (same 2..7 range, verify bit order; `uha03_dec2.c` TODO)
  - core 3 ALZ (0.6/0.4 methods 8..23 incl. 17; 0.3 methods 8..21 incl. 17):
    0.6 FIXED (denominator +escw,
    fallback 0x13E, xsum, len `v51?v51:256`; verified 06_m1 tab1/tab2 +
    D3X tab1/tab2 CRCs + 45 names). Multi-block (thresh) stubbed to
    single-block: tables OK, larger ALZ file streams (e.g. 06_mz m11)
    fail DATA. 0.4 needs separate port (330 vs 319 contexts, order-0 at
    328 vs 318; `uha04_dec3.c` TODO). 0.3 needs separate port
    (14 methods, ALZ:1/2/3 split, different tables; `uha03_dec3.c` TODO).
  - core 4 (0.6 24..39 LZP/PPM; 0.4 24..39 PPM/LZP; 0.3 22..35 LZP/PPM):
    0.6 skeleton + 0.4 (`UHARC04 sub_41CE30/415130`) + 0.3
    (`uharc_u sub_419E9C/415F6C`) NOT ported bit-exact (different models).
    `06_mx` (m27) fails DATA; `D3X` (m34) decodes but CRC fails
    (model + missing exe filter).
  - core 5 (0.6 40..87 PPM2; 0.4 40..87 PPM2/LZW; 0.3 36..79 PPM2/LZW):
    0.6 ported (fuzz-clean, tables OK) but file-data model gap
    (`06_m1` m43 decodes but CRC fails) + 0.4 (`sub_412130/40E380`) +
    0.3 (`uharc_u sub_4103BC/40F6F0`) NOT ported bit-exact.
  - crypt `uhcrypt06/04`: default-password S-box, fresh per table,
    CRC over evolving S-box each 8 bytes (32 rounds). Byte-identical
    for 0.4/0.6; 0.3 reuses `uhcrypt04` (assumed identical,
    TODO verify via directory CRC once 03 cores work). Tables
    decrypt-then-decode; file data plain without -pw
    (password archives TODO).
* Tool `uharc06_unpack` (source `uharc06_main.c`) auto-detects
  `UHA 0.3` vs `UHA 0.4` vs `UHA 0.6`, supports `x/e/t/l` (l = list,
  extension beyond original 0.6 `x/e/t`). `l` + `t` work for 0.6 STORE;
  `x` byte-exact for 0.6 STORE (`06_m0`).

## Files

* `uharc06.h/.c` - 0.6 API, header decrypt/CRC/parse, directory
* `uha04.h/.c` - 0.4 API, 44-byte header decrypt/CRC/parse, directory
  (file names use `uha04*` to avoid clashing with `UHARC04.*`
  on case-insensitive filesystems)
* `uha03.h/.c` - 0.3 API, 44-byte header (magic 03) decrypt/CRC/parse,
  directory (mirrors 0.4; file names use `uha03*` to avoid clashing
  with `uharc_u.*`)
* `uharc_dec.h` - decoder interface (method -> core, version-specific tables)
* `uharc_dec0.c` - dispatch + core0/1 (shared logic, table differs per version)
* `uharc_dec2.c` - 0.6 core2 (0.4 needs LSB-first `uha04_dec2.c` TODO;
  0.3 needs own port `uha03_dec2.c` TODO)
* `uharc_dec3.c` - 0.6 core3 FIXED for tables (0.4 needs `uha04_dec3.c`,
  0.3 needs `uha03_dec3.c` TODO)
* `uharc_dec4.c` - 0.6 core4 skeleton (0.4/0.3 cores separate TODO)
* `uharc_dec5.c` - 0.6 core5 (0.4/0.3 cores separate TODO)
* `uhcrypt06.h/.c`, `uhcrypt04.h/.c` - default-password table cipher
  (0.3 reuses 04 cipher pending verification)
* `uharc06_main.c` - unified CLI tool x/e/t/l with version auto-detect
* `hdrtest.c` - 0.6 header test (passes)
* `hdrtest04.c` - 0.4 header test (passes)
* `hdrtest03.c` - 0.3 header test (passes)
* `test_tab.c`, `test_dec3.c` - 0.6 directory tests (tab1/tab2 PASS now)
* `FORMAT.md` - reverse-engineered 0.4 + 0.6 format spec

## Build (MSYS2 mingw64)

  set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%
  gcc -std=c89 -Wall -Wextra -o uharc06_unpack.exe uharc06_main.c uharc06.c uha04.c uha03.c uhcrypt06.c uhcrypt04.c ^
    uharc_dec0.c uharc_dec2.c uharc_dec3.c uharc_dec4.c uharc_dec5.c

Or `mingw32-make` / `make` with provided Makefile.

## Format summary (see FORMAT.md)

0.6: 39-byte header, bytes 4..34 `b[i]^=(i*i+hdr[37])`,
CRC32 over 4..34, low16 == hdr[37..38].
Tables at `data_off+39`: tab0 STORE, tab1/2 ALZ-17.
Solid stream with `hdr.method` (D3X: 34 -> core4).

0.4: 44-byte header, bytes 4..39 `b[i]^=(key+i*i)`, key=raw[42],
CRC32 over 4..39, low16 == hdr[42..43].
Tables at `base_e01+44`: tab0 STORE 4*N, tab1 method17 names_len,
tab2 method2 4*N dates, tab3 method7 7*N dir/size/method.
Per-file methods (entry+2), solid helpers; file data at 44..base_e01.

0.3: same 44-byte layout as 0.4 (magic `UHA\x03`, decrypt/CRC identical).
Tables at `base+44`: tab0 STORE 4*N, tab1 method17 names, tab2 method2
4*N, tab3 method7 7*N. Per-file methods. Sample `uharc_u.uha`: 1 file
(`uharc_u.c`, 1047398 bytes), base 151747, names 11, cmp 15/4/8.

## Method -> core (version-specific tables)

0.6/0.4: method 0->0 STORE, 1->1 RLE, 2..7->2 Huffman, 8..23->3 ALZ,
24..39->4 (0.6 LZP/PPM vs 0.4 PPM/LZP - different models),
40..87->5 (0.6 PPM2 vs 0.4 PPM2/LZW - different models).
0.3: method 0->0 STORE, 1->1 RLE, 2..7->2 Huffman, 8..21->3 ALZ,
22..35->4 LZP/PPM, 36..79->5 PPM2/LZW (+80->0 alias) - different
models/ranges from 0.4/0.6, do not reuse verbatim.

## Next steps

1. core3 multi-block: tables are single-block (thresh=out_len works);
   larger ALZ file streams (e.g. 06_mz method 11) need block reset
   via progress table (`sub_40807F`/`dword_4396F4`) like original.
2. core2: 0.6 MSB-first works? (0.6 directory needs only core3, so core2
   unverified for 0.6 file data). 0.4 needs LSB-first `uha04_dec2.c`
   for tab2/tab3 (methods 2/7) + `uha04_dec3.c` (330 contexts).
   0.3 needs `uha03_dec2.c` + `uha03_dec3.c` (14-method ALZ,
   different tables; 0.3 core table above).
3. core4/5 file data: 0.6 `06_mx` (m27 core4) DATA, `06_m1` (m43 core5)
   + `D3X` (m34 core4) CRC fail (model gap). Port full PPM trees +
   `40D6D1` prefilter. 0.4 cores differ, need `41CE30/415130`
   and `412130/40E380` ports; 0.3 cores differ, need `419E9C/415F6C`
   and `4103BC/40F6F0` ports (do not reuse 0.6 verbatim).
4. Filters: 0.6 `sub_404665/40481E/404DCE` (exe E8E9/multimedia) for
   flags like D3X 00/01; 0.4 `sub_4085A0/408380/408D20`;
   0.3 `sub_407B44/407D44` + `sub_408EB8/4090A0` (same pairs).
   `06_m0` (flags 00/00) needs none - already byte-exact.
