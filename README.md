# UHARC 0.6 unpacker (portable C89)

Derived from `UNUHARC06.EXE` (UnUHARC 0.6b, 01 Oct 2005) via
`UNUHARC06.lst` (IDA) and `UNUHARC06.c` (Hex-Rays).

## Status

* Header parsing: DONE, verified on `D3X.UHA`
  (magic `UHA\x06`, decrypt, CRC16, method 34, 4 dirs, 45 files,
  976594 bytes, data_off 110610, tab1 512/239 crc 5D4C,
  tab2 278 crc FD32, hdr_crc 23EC).
* Container: `uharc06.[ch]` implements header + directory rebuild
  (19-byte entries: dir_index/method/size/crc/date/time/path).
* Decompressors (`uharc_dec*`):
  - core 0 STORE (method 0): DONE
  - core 1 RLE/delta (method 1): DONE (from sub_40B504)
  - core 2 Huffman/LZ (methods 2..7): ported, compiles, NOT verified
  - core 3 ALZ (methods 8..23, incl. 17 for directory tables): ported,
    compiles, FAILS on D3X tables (out=7, range desync, see test_dec3)
  - core 4 big LZP/PPM (methods 24..39, incl. 34 for D3X solid stream):
    skeleton, NOT bit-exact (documented simplifications)
  - core 5 PPM2 (methods 40..87): ported, compiles, fuzzes clean,
    NOT verified on real archives
* Tool `uharc06_unpack` (source `uharc06_main.c`) matches `x/e/t` CLI subset.
* `D3X.UHA` extracts with original EXE (45 files, 976594 bytes);
  C port currently parses header but fails directory decode.

## Files

* `uharc06.h/.c` - public API, header decrypt/CRC/parse, directory
* `uharc_dec.h` - decoder interface (method 0..87 -> core 0..5)
* `uharc_dec0.c` - dispatch + core0/1
* `uharc_dec2.c` - core2 port
* `uharc_dec3.c` - core3 port (needs range-coder fix, see below)
* `uharc_dec4.c` - core4 skeleton (needs full PPM tree)
* `uharc_dec5.c` - core5 port (needs real-vector test)
* `uharc06_main.c` - CLI tool (named to avoid clashing with UNUHARC06.c on Windows) (x/e/t)
* `hdrtest.c` - header parse test (passes)
* `test_tab.c`, `test_dec3.c` - directory table tests (currently FAIL)
* `FORMAT.md` - reverse-engineered 0.6 format spec

## Build (MSYS2 mingw64)

  set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%
  gcc -std=c89 -Wall -Wextra -o uharc06_unpack.exe uharc06_main.c uharc06.c ^
    uharc_dec0.c uharc_dec2.c uharc_dec3.c uharc_dec4.c uharc_dec5.c

Or `mingw32-make` / `make` with provided Makefile.

## Format summary (see FORMAT.md)

39-byte header, bytes 4..34 encrypted with `b[i]^=(i*i+hdr[37])`,
CRC32 (IEEE) over 4..34, low16 == hdr[37..38].
Tables at `data_off+39`: table0 STORE (4*N file CRCs),
table1 ALZ (names), table2 ALZ (11*N metadata).
Solid file stream with method `hdr.method` (D3X: 34 -> core4).

## Method -> core (DGROUP:004228C2, 0042251F)

method 0->0 STORE, 1->1 RLE, 2..7->2 Huffman, 8..23->3 ALZ,
24..39->4 LZP/PPM, 40..87->5 PPM2.
D3X.UHA: 0 (table0), 17->3 (tables1/2), 34->4 (solid).

## Next steps to bit-exactness

1. core3: fix `d3_match` early-return vs range consumption.
   Trace shows `range=871` before `range/=total` (total=2160) at out=7.
   Previous `d3_renorm` left `range>=16M`, so `>>=12` + length loop
   must restore it. Compare `sub_40C805` length loop renewal
   (`range>>=12` + `renorm(0..4096/pv)`) exactly; check `e[2]` update
   timing and `tab_pair`/`tab_grp` tables from LST.
2. core4: replace compact order-1 model with full
   `4236BC/C0`, `42758D..42764D` price tables and `40E4FF/E3EB`
   rescale + `40D6D1` prefilter (`42711F/426AC3`).
3. core5: diff-test against EXE once container works.
4. Filters: port `sub_404665/40481E/404DCE/405054` (E8E9/exe, image)
   gated by `flags1/2` (`byte_42257F/582/581`).

## References

* `UNUHARC06.EXE` md5 2157E72055FB08516A55A8FF3134C08, ImageBase 0x400000
* `UNUHARC06.lst` 381373 lines, `UNUHARC06.c` 31112 lines, 475 funcs
* `D3X.UHA` 111346 bytes, `UHA\x06`, 45 files
