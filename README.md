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
* File extraction: STORE exact; core5 exact incl. multi-file solids,
  boundary resets and the EXE filter - `06_m1/m2/m3` and a 1.27 MB
  two-file archive (`BIG.uha`) are byte-identical to native
  extraction, plus generated 3-8 file solids across methods 49/50/
  51/65/81 with 1-3 boundary resets each.
  Still open: core4 (`06_mx`, `D3X` method 34) is unported; core3
  multi-block (`06_mz` method 11); a core5 defect for streams that
  select PPM group 6 (small EXE / random blob, e.g. method 44); and
  the multimedia filter.
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
* File data: EXTRACTION DONE and byte-exact for 04_m0 (STORE), 04_mz
  (core3) and 04_m1/m2/m3 (core5 via uha04_dec5.c). The 0.4 EXE filter
  (uharc_filter.c, shared with 0.3) is applied and verified against
  native extractions. GAPS in 0.4 core5: length-predictor groups 5..9
  are unimplemented, so uha04.UHA (method 62, first block picks such a
  group) still fails DATA; 04_mx needs the 0.4 core4 coder.

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
  uharc_u.uha (method 60, 1 MB dictionary) is now byte-exact too:
  1047398 bytes, CRC32 D9BA98B4, identical to a fresh official
  extraction by uharc_u.exe. The earlier failure was not a
  dictionary-size limit but four defects that the short 8 KB
  vectors never reached: a context index gated on an
  encoder-only flag, length tables truncated to 16 of 22
  entries, a Fenwick rescale off by one (fr[i]=raw[i-1]), and
  short-distance symbols polluting the wrong model.
  NOTE: 0.3 post-filters MZ/ZM/MQ files via sub_407D44 (E8/E9),
  which uha03.c does not implement yet - archives holding EXEs
  will extract with wrong CRC until that layer is added.
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
  - core 1 RLE/delta (method 1): DONE and VERIFIED in 0.2, 0.3 and
    0.6 (real archives made with the secret `-mr` switch; 0.2 also
    supplied as 02-R.UHA, 36522 bytes).
  - core 2 (methods 2..7): TWO different coders, both verified with
    real `-mw` (LZW) archives:
    * 0.6: Huffman/LZ with adaptive width, MSB-first bits
      (`uharc_dec2.c`) - verified on 06_lzw.uha.
    * 0.2/0.3/0.4: plain LZW, LSB-first with an optional
      complemented range code, first code emitted directly, code
      0x101 = 16-bit length RLE with pending-literal suppression,
      no table prune (`uharc_dec2_lsb.c`). Verified on 02-W.UHA,
      02_LZW.UHA, 03_lzw.uha and generated method 2..7 archives
      across dictionary sizes. NOTE: it is NOT a bit-order variant
      of the 0.6 coder - the earlier "same algorithm, LSB bits"
      reading only held for the tiny directory tables.
    0.6 FIXED (denominator +escw,
    fallback 0x13E, xsum, len `v51?v51:256`; verified 06_m1 tab1/tab2 +
    D3X tab1/tab2 CRCs + 45 names). Multi-block (thresh) stubbed to
    single-block: tables OK, larger ALZ file streams (e.g. 06_mz m11)
    fail DATA.
    0.4 core3: DONE (`uha04_dec3.c`) - byte-exact on 04_m*/uha04.UHA
    tab1 and a 5840-byte solid stream. 330 contexts (order-0 at
    328/329), lengths as ordinary symbols, 1-based Fenwick rescale.
    0.4 core5: DONE for groups 0-6 and 9+ (`uha04_dec5.c`), incl.
    the length predictors; uha04.UHA (method 62) now extracts all 8
    files. Open: groups 7/8 selected at block start.
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
    0.6: DONE (`uharc_dec5.c`) - byte-exact on 06_m1/m2/m3 (methods
    43/59/75) and on 16 generated 1 MB archives (dict 1 KB..32 MB).
    Nine defects fixed; three were aliased DGROUP arrays where
    `x[i]` is really `base[i+1]`, the rest were a 64/128 Fenwick
    rebuild bound, a shifted price table, incw[768]=6, escape
    pre-step routing, distance-node ordering, a copy-boundary
    snapshot and the group-3 predictor histogram.
    LIMIT: model resets are driven by the container cumulative
    file-size table, which our API does not pass in. Streams
    that stay under 128 KB or hold one file are exact; larger
    multi-file solids diverge at the first reset.
    0.4: DONE (`uha04_dec5.c`) incl. groups 5-9.
    0.3: DONE (`uha03_dec5.c`).
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
  0.4 core5 groups 7/8 at block start + 04_mx (core4), and 0.6 compressed
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
  per-file split, CRC verify, empty files, optional whole-file filter)
* `uharc_filter.h/.c` - 0.3/0.4 EXE post-filter (E8/E9 + MZ relocation
  table), identical for both versions; 0.2 uses its own simpler pass
* `uharc_dec.h` - decoder interface (method -> core, version-specific tables)
* `uharc_dec0.c` - dispatch + core0/1 (shared logic, table differs per version)
* `uharc_dec2.c` - 0.6 core2 (MSB-first bit order)
* `uharc_dec2_lsb.c` - core2 for 0.2/0.3/0.4: plain LZW, LSB-first (NOT a
  bit-order variant of the 0.6 coder). Selected via
  `uharc_dec_decode_v(method, core2_lsb, ver, ...)`. VERIFIED.
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

## Creating test vectors (secret switches)

`-mr` and `-mw` are undocumented switches that select RLE (method 1)
and LZW (method 5) instead of the normal ALZ/PPM modes. They work in
Uharc02.exe, uharc_u.exe and UHARC06.EXE, and were the only way to get
core1 and core2 coverage:

    UHARC06.EXE a -d0 -y+ -o+ -p- -mr out.uha files...   (RLE)
    UHARC06.EXE a -d0 -y+ -o+ -p- -mw out.uha files...   (LZW)

Vary dictionary size with -md1/-md8/-md64/-md1024; the LZW method id
becomes 2..7 accordingly.

## Next steps

1. core3 multi-block: tables are single-block (thresh=out_len works);
   larger ALZ file streams (e.g. 06_mz method 11) need block reset
   via progress table (`sub_40807F`/`dword_4396F4`) like original.
2. 0.3 is COMPLETE (listing + extraction, all dictionary sizes) and
   the 0.3/0.4 EXE filter is DONE and native-verified. 0.4 core5
   is complete for uha04.UHA/04_m1..m3; open: groups 7/8 at block start,
   04_mx needs the 0.4 core4 coder.
   Remaining core2 item: 0.6 MSB-first is unexercised (no 0.6
   archive uses methods 2..7).
   0.2 is COMPLETE: byte-exact against every oracle, including
   archives made by the real DOS archiver under MS-DOS Player.
3. 0.6 core4 is the last unported coder - now UNBLOCKED: the decode
   API accepts the cumulative file-size table (`uharc_dec_decode_b`),
   so core4 can reproduce D3X's five resets. A full translation
   sits in the temp dir (compiles, stream header exact) but still
   crashes in the Fenwick rebuild (sub_40E4FF). 0.4 core4 (04_mx)
   has a partial port only. Watch for aliased DGROUP arrays.
4. Remaining filter: the multimedia layer (uha02_mm.c, verified by
   encoder round-trip) is not wired for 0.3/0.4/0.6. The EXE
   filters are done for all four versions (0.2/0.3/0.4 shared
   algorithm, 0.6 confirmed identical on real archives).
   (`03_m*` via updated uharc_u.exe here) are byte-exact round-trips.
   0.2 vectors come from MAKE02.BAT on a DOS machine (m0..m3,
   text corpus; brings back CORPUS + 02_m*.uha). 0.4 vectors
   (`04_m*`) already exist from earlier work - same corpus pattern.
