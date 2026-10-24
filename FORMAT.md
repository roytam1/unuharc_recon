# UHARC 0.2/0.3/0.4/0.6 archive formats (from Uharc02/uharc_u/UHARC04/UNUHARC06)

All integers little-endian. CRC32 IEEE ED pennies (init FFFFFFFF).

## 0.2 summary (Uharc02.c / Uharc02.lst, DOS32 full archiver, not runnable here)

48-byte header (sub_12848), magic 0x02414855 UHA02.
Decrypt 40 bytes 4..43 key=raw44: raw[4+i] ^= (key+i*i)&FF.
CRC32 over decrypted 4..43, full32 == raw44..47 (header) - unlike later low16.
Fields: +0 magic, +4 time u16, +6 date u16, +8 reserved u8 (0),
+9 method u8 0..43 (sample 25 core3), +10 ndirs u16, +12 nfiles u32,
+16 total u32, +20 data_len u32 (tables at 48+len), +24 names_len u32,
+28 cmp1 u32, +32 cmp2 u32, +36 cmp3 u32, +40 contents_crc full32,
+44 hdr_crc full32. Sample uha02.UHA 11860 bytes: 7 files, 31447 total,
base(data_len) 11656, names 80, cmp 65/28/35, layout
48+11656+28+65+28+35=11860 verified.
Tables: tab0 STORE 4*N file CRCs, tab1 method7 names, tab2 method2 dates,
tab3 method7 dir/size/method (note method7 for names vs 17 in 0.3/0.4/0.6).
19B entries like later versions (dir/method/size/crc/date/time/ptr).
Solid stream like 0.6 (header method, demux by delta sizes).
Method->core: 0->0,1->1,2..7->2,8..43->3 (44 methods, 4 cores, no PPM).
DOS32 executable cannot run here - no console/listing ground truth;
header layout verified arithmetically + decrypt/CRC self-consistent.
Table cipher (verified 2026-10-24): S[i]=pwd[i%22]; 32 rounds of
crc=CRC32(S,256) then S[k..k+3]^=crc, S[k+4..k+7]^=~crc (32-bit LE),
k+=8 (asm sub_10840); stream a=S[p++]; S[p]+=a; out=S[p]^in (sub_12AE0).
Proof: 02_M0 tab0 decrypts to 00000000 9166788F 3CD931D7 5FC97467,
which equal CRCs measured from the archive's own plaintext STORE data
(EMPTY.BIN, REPEAT.TXT, T1.TXT, T2.TXT) - so entry order == stream order.
Note: XOR is 32-bit per 8-byte chunk, not byte-wide (decompiler narrowed
this; byte-wide versions fail).
Test vectors (MAKE02.BAT, DOS side; files in workarea root): text corpus
(EMPTY 0B, T1 362B, T2 353B, REPEAT 528B) + 02_m0/m1/m2/m3.uha, all `t`
self-tested on DOS. Headers: m0->00 core0 STORE base 1243 (= total),
m1->09, m2->21, m3->33 (all core3); directory identical across modes
(names 36, cmp 32/11/18, contents CRC identical, tables byte-identical
77B). m0 STORE file data is plain concatenation in directory order
(EMPTY,REPEAT,T1,T2): byte-exact MATCH vs batch definitions.

## 0.3 summary (uharc_u.c / uharc_u.lst, UHARC 0.3 full archiver)

Same 44-byte layout as 0.4 (sub_404B14), magic 0x03414855 UHA03.
Decrypt 36 bytes 4..39 key=raw42, CRC32 over 4..39 low16==raw42..43.
Fields: same offsets as 0.4 (+0 magic, +4 time, +6 date, +8 pw, +9 filt,
+10 unk/method byte 0x3C=60 (sample; meaning disputed - see below),
+11 ndirs, +13 unk, +17 base, +21 nfiles u24,
+24 names_len, +27 cmp1, +30 cmp2, +33 cmp3, +36 crc1, +38 crc2, +40 crc3,
+42 hdr_crc). Sample uharc_u.uha 151822 bytes: ndirs 1, nfiles 1,
names 11, cmp 15/4/8, layout 44+151747+4+15+4+8=151822 verified.
Extracted ground truth (uharc_u.exe x -y+ -o+ -d0, silent console):
single file `uharc_u.c`, 1047398 bytes (= unk_dfd total).
Test vectors (updated uharc_u.exe 218112 bytes creates correctly;
original 218592 silent no-op): 6-file corpus (5840 bytes), modes
m0/m1/m2/m3/mx/mz -> 03_m*.uha (5996/1598/1598/1597/1598/1598 bytes).
Headers +10: m0 0x00, m1 0x27, m2/mx/mz 0x35, m3 0x43;
`l` reports STORE / ALZ:1 / ALZ:2 / ALZ:2 / ALZ:2 / ALZ:3.
OPEN QUESTION: header byte vs displayed mode vs codec table disagree
(e.g. 0x27=39 falls in core5 range 36..79, yet `l` says ALZ:1) -
either the byte is a mode tag, not the codec ID, or the ALZ range is
wider than 8..21. Resolves once entry+2 methods decode.
Directory tables byte-identical across all six modes (112B).
Table cipher VERIFIED identical to 0.4/0.6 (not assumed): 03_m0 tab0
decrypts to exactly the corpus CRCs in order
[empty, text, repeat, binary, random, call] - same as the m0 tiling.
Feeding correctly decrypted 0.3 tab1 (method 17) / tab2 (method 2) to
the verified 0.6 core3/core2 gives rc=3 (input short): the codecs
differ, so separate 0.3 ports are required (cipher is not the cause).
NOTE: 03_m2, 03_mx, 03_mz are byte-identical (CLASS build collapses
x/z onto ALZ:2) - vectors cover ALZ:1/2/3 only, not PPM/LZP.
Header +10 = 36+(k-1)*14+3 (k=switch, 3=dict index): 0x27/0x35/0x43.
m0 STORE stream tiles the corpus in non-empty order
[text,repeat,binary,random,call] (+ empty.bin, 0 bytes).
Per-file ground truth (`l -d2 03_m0.uha`, display order):
binary.bin 1024 B70B4C26, call.bin 512 6F3BDB8B, empty.bin 0 00000000,
random.bin 1024 021E1879, repeat.txt 1000 51A02E01, text.txt 2280 7125DAA3
(all 24-Oct-2026 11:49:40, ---A). Console capturable via pywinpty
(needs absolute exe path; plain-pipe runs stay silent).
Tables: tab0 STORE 4*N, tab1 method17 names, tab2 method2 4*N,
tab3 method7 7*N (same methods as 0.4 directory). Per-file methods.
Method->core table differs: 0->0,1->1,2..7->2,8..21->3,22..35->4,
36..79->5 (+80->0). Cores differ from 0.4/0.6 (different models/tables).

## 0.4 summary (UHARC04.c / UHARC04.lst, UHARC 0.4 BETA)

44-byte header (sub_4051B0), magic 0x04414855 UHA04.
Decrypt 36 bytes 4..39 with key=raw42: raw[4+i] ^= (key+i*i)&FF.
CRC32 over 4..39, low16 == raw42..43.
Fields: +0 magic, +4 time, +6 date, +8 flags_pw bit1,
+9 flags_filt, +10 unk, +11 ndirs u16, +13 unk u32,
+17 base_e01 u32 (tables at base+44), +21 nfiles u24,
+24 names_len u24, +27 cmp1 u24, +30 cmp2 u24, +33 cmp3 u24,
+36 crc1, +38 crc2, +40 crc3, +42 hdr_crc.
Sample uha04.UHA 21687 bytes: ndirs 1, nfiles 8, base 21470,
names 88, cmp 69/32/40, layout 44+21426+32+69+32+40=21687 verified.
Tables: tab0 STORE 4*N CRCs, tab1 method17 names,
tab2 method2 dates, tab3 method7 dir/size/method.
Per-file methods via sub_4091E0. Method->core same as 0.6.

---

# UHARC 0.6 archive format (reverse engineered from UNUHARC06.EXE)

All integers little-endian. All CRC32 are IEEE (EDB88320, init FFFFFFFF,
table at `00421EB3`), matching `sub_403035`.

## 1. SFX overlay

`sub_4031D7`: if first 2 bytes `MZ` (`5A 4D`), read `e_lfanew` at `0x3C`,
check `PE`, return overlay offset in `dword_422631`. Subsequent seeks are
`base+offset`. Plain `UHA` archives have base 0.

## 2. 39-byte header (`dword_433E08`, `sub_4032B8`)

Raw offsets, then decrypt, then parse:

```
+0  u32 magic 0x06414855 "UHA\x06" (else "No UHARC 0.6 archive",
     or "use UHARC version %X.%X" if low24 == loc_414853+2)
+4  u16 dostime (HH:MM:SS/2, via sub_401488 "%02u:%02u")
+6  u16 dosdate (DD-Mon-YYYY, via sub_401515, month names table)
+8  u8  flags1 (byte_433E10: bit2 -> need unpack? bit4? )
+9  u8  flags2 (byte_433E11: bit0 -> 0x8000 bufs + exe filter on,
                bit1 -> image filter, bit7 -> big filter, bit6 -> ?)
+10 u8  method (byte_433E12, 0..87 solid-stream method)
+11 u16 ndirs (word_433E13 -> dword_4225E1)
+13 u32 total_unpacked (dword_433E15, D3X: 976594)
+17 u32 data_off (dword_433E19, D3X: 110610; tables at +39)
+21 u24 nfiles (dword_433E1D & 0xFFFFFF, D3X: 45)
+24 u24 tab1_dec_size (*(E1D+3) & 0xFFFFFF, D3X: 512)
+27 u24 tab1_cmp_size (dword_433E23 & 0xFFFFFF, D3X: 239)
+30 u24 tab2_cmp_size (*(E23+3) & 0xFFFFFF, D3X: 278)
+33 u16 tab1_crc (word_433E29, low16 of crc32(decoded tab1))
+35 u16 tab2_crc (word_433E2B, low16 of crc32(decoded tab2))
+37 u16 hdr_crc (word_433E2D, low16 of crc32(decrypted 4..34))
```

Decrypt (`sub_4032B8`): `key=raw[37]`, for `i=0..30`:
`raw[4+i] ^= (i*i+key) & 0xFF`.

CRC (`sub_403035`): `crc=FFFFFFFF`, update over `4..34` (31 bytes),
check `(~crc & 0xFFFF)==hdr_crc`, else "CRC-failure (archive header)".

`created:` display combines time+date via `sub_401488/401515/401565`:
`Processing archive "%s" (created: %s, %s).`

D3X decrypted (hex):
`55 48 41 06 59 AE CD 54 00 01 22 04 00 D2 E6 0E 00 12 B0 01 00
 2D 00 00 00 02 00 EF 00 00 16 01 00 4C 5D 32 FD EC 23`
-> time AE59=21:50, date 54CD=13-Jun-2022, flags 00/01, method 22h=34,
ndirs 4, total 0EE6D2, data_off 01B012, nfiles 45, tab1dec 512,
tab1cmp 239, tab2cmp 278, tab1crc 5D4C, tab2crc FD32, hdrcrc 23EC.

## 3. Tables (`sub_403840`, at data_off+39)

```
tab0_off = data_off+39, len 4*N, method 0 STORE (file CRCs, full32)
tab1_off = tab0_off+4*N, cmp tab1_cmp, dec tab1_dec, method 17 (core3 ALZ)
tab2_off = tab1_off+tab1_cmp, cmp tab2_cmp, dec 11*N, method 17
data_off_stream = tab2_off+tab2_cmp (D3X: 110649+180+239+278=111346)
```

Each table: `sub_40705C(method, sub_4036E2, out_len, sub_403731)` where
`sub_4036E2` is bounded file read (`dword_433DDC` limit, optional
`sub_40367A` crypt via `byte_422589`+`byte_433150`), `sub_403731` is
CRC-accumulate + memcopy (`dword_433DD4` init FFFFFFFF).
Afterwards check `(~crc&0xFFFF)==tab*_crc`, else `sub_40376E`.

* tab0: `N` u32 file CRCs (full32, checked in `sub_4051A2` as
  `~crc==entry+7`).
* tab1: `ndirs` + `N` pascal strings (`u8 len + bytes`):
  dir names then file names. Rebuilt with `dword_4225B5/B9` pools.
* tab2: `11*N` bytes split as planes (from `lst:403B84..403C91`):
  - `0..2N-1`: N u16 -> entry+13 (time) then entry+11 (date)
  - `4N..6N-1`: 2N bytes -> entry+0 word (dir_index+flags:
    low=`buf[4N+i]`, high=`buf[5N+i]`)
  - `6N..10N-1`: 4N bytes -> assembled u32 per file, accumulated
    `var-=dword` -> entry+3 (size; on-disk cumulative? `sub_4051A2`
    uses `[esi+3]` directly as remaining bytes)
  - `10N..11N-1`: N bytes -> entry+2 method (0xFF=separator/skip)

19-byte entry (`dword_4225BD`, `sub_403840` tail):
```
+0  u16 dir_index/flags
+2  u8  method (per-file? 0xFF=skip; solid stream uses header method)
+3  u32 size (unpacked, also progress table c5 from cumulative?)
+7  u32 crc (from tab0)
+11 u16 dosdate
+13 u16 dostime
+15 u32 name_ptr (into strpool)
```

Filenames via `sub_403CFB` (dir + `\` + name, `sub_401565` truncate).

## 4. Solid stream (`sub_405858`, method hdr.method)

Single `sub_40705C(hdr.method, sub_4036E2, total_unpacked, sub_4051A2)`:
input is archive at `39` (`sub_403628(39)`), output demuxes by
`entry+3` remaining (`dword_433DE4`), creating dirs (`sub_408930`),
opening `fopen(path,"wb")` (`sub_407794`), filtering via
`sub_404DCE` (multimedia: `sub_404665` exe-E8E9 detect,
`sub_40481E` image, `byte_433E36=(tbl_42251F[method]==4)` PPM flag,
`dword_433DD8/DB0` windows gated by `flags2`), CRC via `dword_433DD4`,
and progress via `sub_403EA0` (console `Extracting %-64s Total:%3lu%%`).

Method->core (`DGROUP:004228C2`, `0042251F`):
```
0->0 STORE (sub_40B4BB), 1->1 RLE (sub_40B504),
2..7->2 Huffman/LZ (sub_40B8DF, B=1<<(m+8)),
8..23->3 ALZ (sub_40C305, dict=1<<(((m-8)%16)+10)),
24..39->4 LZP/PPM (sub_40F962, dict=1<<(((m-24)%16)+10)),
40..87->5 PPM2 (sub_414BF3, dict=1<<(((m-40)%16)+10), group 1/2/3).
D3X: 0, 17->3, 34->4 (1M dict).
```

Range coder (`dword_4749D8=code`, `4749DC=range`):
init `sub_419A4D` (`code=0,range=-1`, 5 bytes BE),
renorm `while(range<0x1000000){code=(code<<8)|getbyte();range<<=8;}`.

## 5. CLI (`sub_402713/405EE2`)

`x/X` extract with dirs (`byte_42259D=1`), `e/E` flat (`=0`),
`t/T` test (`byte_42258B=1`), all set `byte_422577=2`.
`-?`/`/?` help, else `Invalid command "%c"`, `No archive specified`,
`Invalid switch "%s"`. Password via `-pw` (`sub_4017E8`,
`UHARC Default Password`), `byte_422589` crypt, `byte_433150` table.
