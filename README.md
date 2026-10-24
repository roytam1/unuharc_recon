# UHARC 0.2/0.3/0.4/0.6 unpacker (portable C89)

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
* Directory: tab0/tab2/tab3 DONE (STORE + LSB-first core2). Only tab1
  (names, method 17 = core3) remains, needing a 0.4-specific core3
  (330 contexts vs 06 319; order-0 at 328 vs 318). tab3 verified:
  sizes 0/2280/1000/1024/1024/512 (sum 5840).
  Tables use default-password crypt, VERIFIED byte-identical to 0.6:
  04_m0 tab0 (STORE, 4*N) decrypts to exactly the six corpus CRCs in
  order [empty, text, repeat, binary, random, call] - same order as 0.3
  and as the m0 stream tiling.
  Ground truth via `UHARC04 l -d2` (names/sizes/dates/CRCs, e.g. text
  7125DAA3, repeat 51A02E01, binary B70B4C26, random 021E1879).
* File data: EXTRACTION DONE for 04_m0 (STORE) and 04_mz (core3 via
  uha04_dec3.c) - byte-exact vs corpus. 04_m1/m2/m3/mx still need the
  0.4 core5/core4 coders. Filters (exe/multimedia) not yet applied, so
  uha04.UHA (contains a 29 KB EXE) will fail its CRC.

0.3:
* Header parsing: DONE, verified on `uharc_u.uha`
  (magic `UHA\x03`, same 44B decrypt/CRC as 0.4, key=raw42).
  `uharc_u.uha`: 1 file, 1047398 bytes unpacked (= extracted size),
  base 151747, ndirs 1, names 11, cmp 15/4/8, layout
  44+151747+4+15+4+8=151822 verified. Header +10 = 0x3C (60;
  table maps 36..79 to core5 - see open question below).
  Ground truth via extraction with `uharc_u.exe x -y+ -o+ -d0`
  (console silent, exit 0; file `uharc_u.c`, 1047398 bytes).
  Console output IS capturable via pywinpty (needs absolute exe path;
  earlier `winpty` CLI failure was a headless-env/argv issue, not the
  archiver - see `pty_run.py` harness in temp). Captured ground truth:
  `l -d2 uharc_u.uha` = 1 file, 1047398B, 24-Oct-2026 10:29:04, ---A,
  crc32 D9BA98B4, packed 151747 `(ALZ:2-mode)`; `t` => `All files OK`.
  Updated `uharc_u.exe` (218112 bytes, 2026-10-24; original 218592
  silent no-op for `a`) creates correctly: test vectors
  `03_m0/m1/m2/m3/mx/mz.uha` (6-file corpus, 5840 bytes)
  via `a -d0 -y+ -o+ -p- -m*`, all round-trip byte-exact (6/6 MATCH);
  `t` re-confirms `All files OK` (uharc_u.uha, 03_m1.uha).
  Per-file ground truth (`l -d2 03_m0.uha`, display order):
  binary.bin 1024 B70B4C26, call.bin 512 6F3BDB8B, empty.bin 0 00000000,
  random.bin 1024 021E1879, repeat.txt 1000 51A02E01, text.txt 2280 7125DAA3
  (all 24-Oct-2026 11:49:40, ---A); m0 STORE stream tiles exactly with
  the corpus in non-empty order [text,repeat,binary,random,call]
  (+ empty.bin, 0 bytes: its slot is unobservable - directory order
  TBD by decoders).
  Header +10 bytes: m0 0x00, m1 0x27, m2/mx/mz 0x35, m3 0x43;
  `l` reports STORE / ALZ:1 / ALZ:2 / ALZ:2 / ALZ:2 / ALZ:3.
  OPEN QUESTION: header byte vs displayed mode vs codec table disagree
  (e.g. 0x27=39 falls in core5 range 36..79, yet `l` says ALZ:1).
  Either the byte is a mode tag (not the codec ID) or the table's ALZ
  range is wider than 8..21. Resolves once entry+2 methods decode
  (per-file ground truth, pending core2/3 ports).
  Directory tables byte-identical across all six modes (112B).
  NOTE: 03_m2, 03_mx, 03_mz are byte-identical (same MD5) - this
  "CLASS EDiTiON" build collapses x/z modes onto ALZ:2 (banner:
  "FOR CLASS/BACKSLASH RELEASES ONLY"), so vectors cover ALZ:1/2/3 only,
  NOT PPM/LZP. Header +10 follows 36+(k-1)*14+3 (k=switch, 3=dict idx):
  0x27/0x35/0x43 for k=1/2/3. 0.4 build does honour x/z (all four differ).
* Directory: DONE. All four tables decode (tab1 via the new 0.3 core3
  port `uha03_dec3.c`, which is verified byte-exact against the known
  62-byte name table on 03_m0..m3 and against uharc_u.uha's 11-byte
  table). `l 03_m0.uha` -> empty.bin 0, text.txt 2280, repeat.txt 1000,
  binary.bin 1024, random.bin 1024, call.bin 512 (sum 5840);
  `l uharc_u.uha` -> uharc_u.c 1047398. Entry order matches tab0.
  0.3's coder is NOT 0.6's: 24-bit range/offset, 328 contexts,
  multi-exclusion escapes, discarded first byte = ~out_len
  (see uha03_dec3.c header). crc1 is cumulative over tab0+tab1.
  File data: EXTRACTION DONE for all six vectors (03_m0 STORE..03_mz,
  methods 0/39/67) via uha03_dec5.c - byte-exact vs corpus.
  Exception: uharc_u.uha (method 60 = 1 MB dictionary) still fails
  immediately (rc=7, out=0) - the 0.3 core5 port only covers the
  8 KB-dictionary methods exercised by the vectors.
  tab0 (STORE, 4*N) decrypts to exactly the six corpus CRCs
  (00000000, 7125DAA3, 51A02E01, B70B4C26, 021E1879, 6F3BDB8B), which
  also fixes the directory/stream order as
  [empty.bin, text.txt, repeat.txt, binary.bin, random.bin, call.bin]
  (matches the m0 STORE tiling - empty.bin contributes 0 bytes).
  Decoding 0.3 tab1/tab2 with the verified 0.6 core3/core2 returns
  rc=3 (input short) even though the cipher is proven correct, so the
  0.3/0.4 codecs genuinely differ from 0.6 (330-vs-319 context claim
  now evidence-backed): separate ports required.

0.2:
* Header parsing: DONE, verified on `uha02.UHA`
  (magic `UHA\x02`, 48B header, decrypt 40B key=raw44, full32 CRCs).
  `uha02.UHA`: 7 files, 31447 bytes total, base(data_len) 11656,
  ndirs 1, names_len 80, cmp 65/28/35, layout
  48+11656+28+65+28+35=11860 verified. Header +9 = 0x19 (25, core3).
  DOS32 executable cannot run here; test vectors via MAKE02.BAT
  (run on a DOS machine next to UHARC02.EXE): builds text corpus
  (EMPTY.BIN 0B, T1 362B + T2 353B lorem/pangram lines, REPEAT 528B =
  8x64 A + CRLF) + archives 02_m0/m1/m2/m3.uha (modes STORE/FAST/NORM/BEST).
  Vectors received in workarea root and verified here: headers parse rc=0,
  methods m0->00 core0 STORE, m1->09, m2->21, m3->33 (all core3);
  base m0 1243 (= total, STORE), m1 491, m2/m3 487;
  directory tables byte-identical across modes (names 36, cmp 32/11/18,
  77B total).
  m0 STORE file data is plain: it tiles exactly with the MAKE02.BAT corpus
  (non-empty order uniquely [REPEAT,T1,T2]; EMPTY.BIN contributes 0 bytes,
  so its slot is unobservable - directory order TBD by decoders).
  Ground truth travels as files (same pattern as 03/04/06 vectors);
  per-file names/CRCs await directory decoders. The batch runs `t`
  self-checks on DOS; here `l`/`t` output is uncapturable (DOS32 binary,
  headless env), so there is no per-file listing ground truth yet -
  unlike 0.3/0.4/0.6 (pywinpty / native console output).
* Directory: DONE. All four tables decode (0.2 uses only STORE + core2,
  so no core3 is needed here). VERIFIED:
  `l 02_M0.uha` -> EMPTY.BIN 0, REPEAT.TXT 528, T1.TXT 362, T2.TXT 353
  (sum 1243 = header total); `l uha02.UHA` -> 7 real names
  (BOOTLOG.TXT 621, BIG5-2.TXT 265, BIG5.TXT 49, BOOT.INI 185,
  BAK-KA~1.EXE 29696, BOOKMA~1.BMK 10, BOOTLOG.PRV 621; sum 31447).
  tab3 layout (VERIFIED): [0..4N) size deltas as consecutive 32-bit LE
  words, [4N..6N) dir index, [6N..7N) method/attr byte
  (0x20/0x22/0x27 in uha02.UHA, 0x20 in the corpus vectors).
  Sizes are deltas: size_i = size_{i-1} - var_i (size_{-1} = 0).
  File data: EXTRACTION WORKS for the compressed modes. `x 02_M1/M2/M3.uha`
  all byte-exact vs the 02_M0 plaintext stream (1243 bytes), per-file
  CRCs verified (new `uha02_dec3.c`: 16-bit-init range coder, token/
  literal/match/run models, methods 9/21/33 = dict sizes).
  0.2 is now COMPLETE and byte-exact against every oracle, including
  archives produced by the real DOS archiver (runnable through MS-DOS
  Player, msdos.exe):
  - uha02.UHA (`a uha02.uha b*.*`)               7/7 files MATCH
  - UH02MMP.UHA (`a -mm+ ... b*.*`)              9/9 files MATCH (WAV+BMP)
  - UHA02EXE.UHA (`a -mm+ ... Uha02d3x.exe`)     extracted EXE MD5 == Uharc02.exe
  - V_MMOFF.UHA (`a -mm- -m2 ...`)               10/10 files MATCH (incl. 1.15 MB msdos.exe)
  - 02_M1/M2/M3                                  byte-exact
  The earlier 10-byte BAK-KA~1.EXE gap was the missing E8/E9 executable
  filter (sub_150A4/sub_15AD0); 0.2 stores no filter flag, so it is
  applied unconditionally, gated by an MZ/ZM header and len >= 0x400
  (no-op otherwise). Implemented as uha02_unfilter() in uha02.c.
  Two genuine core3 defects were also found and fixed in uha02_dec3.c:
  (1) the symbol->nibble table byte_3ADF8 is 256 entries (was truncated
  to 16, breaking every non-zero -mm predictor mode), and (2) the
  small-distance rescale (sub_1BAC4) halves (m2-m1)/(m3-m2) around m1
  with a 0x400 floor and rewrites m2, not (m1-m0)/(m3-m1)/m1.
  NOTE: 0.2's -mm+ gain is a codec-internal literal predictor, NOT a
  post-decode multimedia filter - output is already original data.
  Crypt: 0.2-specific S-box/stream (NOT byte-identical to 04/06;
  implemented as static in-file functions per binary,
  `sub_108C8`/`sub_12AE0`), fresh per table like later versions.
  VERIFIED via tab0: decrypting the STORE CRC table with this cipher
  yields exactly the archive's own measured file CRCs
  (00000000 EMPTY.BIN, 9166788F REPEAT.TXT, 3CD931D7 T1.TXT,
  5FC97467 T2.TXT), i.e. entry order == stream order. Key schedule
  (asm sub_10840): S[i]=pwd[i%22], then 32 rounds of
  crc=CRC32(S,256); S[k..k+3]^=crc, S[k+4..k+7]^=~crc (32-bit LE),
  k+=8. Stream (sub_12AE0): a=S[p++]; S[p]+=a; out=S[p]^in.

Decompressors (method->core tables are version-specific:
0.6/0.4 `0->0,1->1,2..7->2,8..23->3,24..39->4,40..87->5`;
0.3 `0->0,1->1,2..7->2,8..21->3,22..35->4,36..79->5` + `80->0` alias;
0.2 `0->0,1->1,2..7->2,8..43->3` (44 methods, 4 cores, no PPM):
  - core 0 STORE (method 0): DONE, byte-exact (`06_m0` 6/6 MATCH)
  - core 1 RLE/delta (method 1): DONE (from 0.6 sub_40B504), no test vector yet
  - core 2 Huffman/LZ (methods 2..7 all versions): SOLVED for 0.2/0.3/0.4.
    Identical algorithm to the 0.6 port with an LSB-first bit reader
    (`uharc_dec2_lsb.c`, symbols uharc_dec2l_*). 0.6 stays MSB-first
    (`uharc_dec2.c`). Select with `uharc_dec_decode_v(method, lsb, ...)`.
    VERIFIED: 0.2 tab1 names (EMPTY.BIN/REPEAT.TXT/T1.TXT/T2.TXT),
    0.2 tab2 dates 5D58/963B, 0.2 tab3 sizes, 0.3+0.4 tab2 crc16 25CA
    exact and tab3 sizes summing to 5840. MSB-first fails all of these.
  - core 3 ALZ (0.6/0.4 methods 8..23 incl. 17; 0.3 methods 8..21 incl. 17):
    0.6 FIXED (denominator +escw,
    fallback 0x13E, xsum, len `v51?v51:256`; verified 06_m1 tab1/tab2 +
    D3X tab1/tab2 CRCs + 45 names). Multi-block (thresh) stubbed to
    single-block: tables OK, larger ALZ file streams (e.g. 06_mz m11)
    fail DATA.
    0.4: DONE (`uha04_dec3.c`) - byte-exact on 04_m*/uha04.UHA tab1 and a
    5840-byte solid stream. 330 contexts (order-0 at 328/329), lengths as
    ordinary symbols, 1-based Fenwick rescale fill.
    0.3 core5: DONE (`uha03_dec5.c`) - byte-exact on 03_m1/m2/m3/mx/mz file
    data (methods 39 and 67, all per-file CRCs). 3-way decision model,
    PPM/Fenwick contexts, delta literals, 61-symbol distance + MRU.
    0.3: DONE (`uha03_dec3.c`) - byte-exact on 03_m* and uharc_u tab1.
    Different coder: 24-bit range/offset, 328 contexts, multi-exclusion
    escapes, first stream byte = ~out_len.
    0.2: DONE (`uha02_dec3.c`) - byte-exact on 02_M1/M2/M3 streams
    (methods 9/21/33) and on uha02.UHA (method 25, all 7 files).
    0.2, 0.3, 0.4 and 0.6 core3 are all distinct coders, each now ported.
    EVIDENCE: correctly decrypted 0.3 tab1/tab2 fed to the 0.6
    core3/core2 returned rc=3, so the gap is codec-level, not cipher
    (cipher proven by recovering tab0 CRCs).
  - core 4 (0.6 24..39 LZP/PPM; 0.4 24..39 PPM/LZP; 0.3 22..35 LZP/PPM;
    0.2 has no core4 - max method 43->core3):
    0.6 skeleton + 0.4 (`UHARC04 sub_41CE30/415130`) + 0.3
    (`uharc_u sub_419E9C/415F6C`) NOT ported bit-exact (different models).
    `06_mx` (m27) fails DATA; `D3X` (m34) decodes but CRC fails
    (model + missing exe filter).
  - core 5 (0.6 40..87 PPM2; 0.4 40..87 PPM2/LZW; 0.3 36..79 PPM2/LZW;
    0.2 has no core5):
    0.6 ported (fuzz-clean, tables OK) but file-data model gap
    (`06_m1` m43 decodes but CRC fails) + 0.4 (`sub_412130/40E380`) +
    0.3 (`uharc_u sub_4103BC/40F6F0`) NOT ported bit-exact.
  - crypt `uhcrypt06/04`: default-password S-box, fresh per table,
    CRC over evolving S-box each 8 bytes (32 rounds).
    VERIFIED for 0.3, 0.4, 0.6: their tab0 (STORE CRC table) decrypts to
    the archive's own measured file CRCs (0.3/0.4 corpus: 00000000,
    7125DAA3, 51A02E01, B70B4C26, 021E1879, 6F3BDB8B; uharc_u.uha:
    D9BA98B4 == its `l -d2` CRC). 0.3 can therefore drop the "assumed"
    caveat and use `uhcrypt04` outright.
    0.2 has a distinct in-file cipher, ALSO VERIFIED the same way
    (tab0 -> 00000000, 9166788F, 3CD931D7, 5FC97467 from 02_M0's own
    plaintext STORE stream). Tables
    decrypt-then-decode; file data plain without -pw
    (password archives TODO).
* Tool `uharc06_unpack` (source `uharc06_main.c`) auto-detects
  `UHA 0.2` vs `UHA 0.3` vs `UHA 0.4` vs `UHA 0.6`, supports `x/e/t/l`.
  Working end to end: 0.2 list + extract
  (every oracle byte-exact, incl. real DOS-made -mm+ archives);
  0.6 STORE extract (byte-exact); 0.3 + 0.4 list (all vectors/samples).
  0.3 extract (all six vectors, methods 0/39/67) and 0.4 extract
  (04_m0 STORE, 04_mz core3) are now byte-exact too.
  Remaining: 0.3 uharc_u.uha (method 60, 1 MB dict path),
  0.4 m1/m2/m3/mx (need 0.4 core5/core4), and 0.6 compressed
  data (core4/5 + filters).

## Files

* `uharc06.h/.c` - 0.6 API, header decrypt/CRC/parse, directory
* `uha04.h/.c` - 0.4 API, 44-byte header decrypt/CRC/parse, directory
  (file names use `uha04*` to avoid clashing with `UHARC04.*`
  on case-insensitive filesystems)
* `uha03.h/.c` - 0.3 API, 44-byte header (magic 03) decrypt/CRC/parse,
  directory (mirrors 0.4; file names use `uha03*` to avoid clashing
  with `uharc_u.*`)
* `uha02.h/.c` - 0.2 API, 48-byte header (magic 02, full32 CRCs)
  decrypt/CRC/parse, directory (solid like 0.6; `uha02*` avoids
  clashing with `Uharc02.*`)
* `uharc_split.h/.c` - shared solid-stream splitter for 0.3/0.4 (decode,
  per-file split, CRC verify, empty files)
* `uharc_dec.h` - decoder interface (method -> core, version-specific tables)
* `uharc_dec0.c` - dispatch + core0/1 (shared logic, table differs per version)
* `uharc_dec2.c` - 0.6 core2 (MSB-first bit order)
* `uharc_dec2_lsb.c` - core2 for 0.2/0.3/0.4 (LSB-first; same algorithm).
  Selected via `uharc_dec_decode_v(method, core2_lsb, ...)`. VERIFIED.
* `uharc_dec3.c` - 0.6 core3 (tables verified)
* `uha03_dec3.c` - 0.3 core3, verified byte-exact (own 24-bit coder)
* `uha02_dec3.c` - 0.2 core3, verified byte-exact on 02_M1/M2/M3
* `uha04_dec3.c` - 0.4 core3, verified byte-exact (330 contexts)
* `uha03_dec5.c` - 0.3 core5, verified byte-exact on 03_m* file data
* `uha02_mm.c` - 0.3/0.4/0.6 multimedia filter (BMP/WAV/VOC), verified by
  encoder-inverse round-trip. NOT wired in: 0.2 needs no post-filter
  (its -mm+ gain is a codec-internal predictor), staged for later versions.
* `uharc_dec4.c` - 0.6 core4 skeleton (0.4/0.3 cores separate TODO;
  0.2 has no core4)
* `uharc_dec5.c` - 0.6 core5 (0.4/0.3 cores separate TODO;
  0.2 has no core5)
* `uhcrypt06.h/.c`, `uhcrypt04.h/.c` - default-password table cipher
  (0.3 reuses 04 cipher pending verification; 0.2 has own in-file
  cipher, NOT byte-identical)
* `uharc06_main.c` - unified CLI tool x/e/t/l with version auto-detect
* `hdrtest.c` - 0.6 header test (passes)
* `hdrtest04.c` - 0.4 header test (passes)
* `hdrtest03.c` - 0.3 header test (passes)
* `hdrtest02.c` - 0.2 header test (passes)
* `test_tab.c`, `test_dec3.c` - 0.6 directory tests (tab1/tab2 PASS now)
* `FORMAT.md` - reverse-engineered 0.4 + 0.6 format spec

## Build (MSYS2 mingw64)

  set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%
  gcc -std=c89 -Wall -Wextra -o uharc06_unpack.exe uharc06_main.c uharc06.c uha04.c uha03.c uha02.c uhcrypt06.c uhcrypt04.c ^
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

0.3: same 44-byte layout as 0.4 (magic 03, decrypt/CRC identical).
Tables at `base+44`: tab0 STORE 4*N, tab1 method17 names, tab2 method2
4*N, tab3 method7 7*N. Per-file methods. Sample `uharc_u.uha`: 1 file
(`uharc_u.c`, 1047398 bytes), base 151747, names 11, cmp 15/4/8.

0.2: 48-byte header, magic 02, decrypt 40 bytes 4..43 key=raw44,
CRC32 full32 (header + contents, not low16). Fields: +0 magic,
+4 time, +6 date, +8 reserved, +9 method (sample 25 core3),
+10 ndirs u16, +12 nfiles u32, +16 total u32, +20 data_len u32,
+24 names_len u32, +28/+32/+36 cmp1/2/3 u32, +40 contents_crc full32,
+44 hdr_crc full32. Sample `uha02.UHA` 11860 bytes: 7 files,
31447 total, base 11656, names 80, cmp 65/28/35, layout
48+11656+28+65+28+35=11860. Tables: tab0 STORE 4*N, tab1 method7 names,
tab2 method2 dates, tab3 method7 dir/size/method. Solid stream like 0.6
(header method, demux by delta sizes). DOS32 exe not runnable here.

## Method -> core (version-specific tables)

0.6/0.4: method 0->0 STORE, 1->1 RLE, 2..7->2 Huffman, 8..23->3 ALZ,
24..39->4 (0.6 LZP/PPM vs 0.4 PPM/LZP - different models),
40..87->5 (0.6 PPM2 vs 0.4 PPM2/LZW - different models).
0.3: method 0->0 STORE, 1->1 RLE, 2..7->2 Huffman, 8..21->3 ALZ,
22..35->4 LZP/PPM, 36..79->5 PPM2/LZW (+80->0 alias) - different
models/ranges from 0.4/0.6, do not reuse verbatim.
0.2: method 0->0 STORE, 1->1 RLE, 2..7->2 Huffman, 8..43->3 PPM/LZ+range
(44 methods, 4 cores, no PPM) - different models/ranges, do not reuse.

## Next steps

1. core3 multi-block: tables are single-block (thresh=out_len works);
   larger ALZ file streams (e.g. 06_mz method 11) need block reset
   via progress table (`sub_40807F`/`dword_4396F4`) like original.
2. core3 is now DONE for all four versions (0.6 covers tables only).
   Next: wire 0.3/0.4 extraction (uha03_extract/uha04_extract are
   still stubs) using the verified core3/core5 decoders, then add
   their exe/multimedia filters (uha02_mm.c is the verified base).
   Remaining core2 item: 0.6 MSB-first is still unexercised (no 0.6
   archive uses methods 2..7).
   0.2 is COMPLETE: byte-exact against every oracle, including
   archives made by the real DOS archiver under MS-DOS Player.
3. core4/5 file data: 0.6 `06_mx` (m27 core4) DATA, `06_m1` (m43 core5)
   + `D3X` (m34 core4) CRC fail (model gap). Port full PPM trees +
   `40D6D1` prefilter. 0.4 needs `41CE30/415130` + `412130/40E380`;
   0.3 core5 is DONE (uha03_dec5.c) and works as a reference for 0.4.
   0.2 has no core4/5 (max method 43->core3).
4. Filters: 0.6 `sub_404665/40481E/404DCE` (exe E8E9/multimedia) for
   flags like D3X 00/01; 0.4 `sub_4085A0/408380/408D20`;
   0.3 `sub_407B44/407D44` + `sub_408EB8/4090A0` (same pairs);
   0.2 `sub_150A4/15AD0` (+ wrappers `sub_151B0/15BC0`), gate `byte_32222&&len>=0x400`.
   `06_m0` (flags 00/00) needs none - already byte-exact.
5. Test vectors: 0.6 (`06_m*` via UHARC06.EXE here) and 0.3
   (`03_m*` via updated uharc_u.exe here) are byte-exact round-trips.
   0.2 vectors come from MAKE02.BAT on a DOS machine (m0..m3,
   text corpus; brings back CORPUS + 02_m*.uha). 0.4 vectors
   (`04_m*`) already exist from earlier work - same corpus pattern.
