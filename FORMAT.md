# UHARC 0.3/0.4/0.6 archive formats (from uharc_u/UHARC04/UNUHARC06)

All integers little-endian. CRC32 IEEE ED pennies (init FFFFFFFF).

## 0.3 summary (uharc_u.c / uharc_u.lst, UHARC 0.3 full archiver)

Same 44-byte layout as 0.4 (sub_404B14), magic 0x03414855 UHA03.
Decrypt 36 bytes 4..39 key=raw42, CRC32 over 4..39 low16==raw42..43.
Fields: same offsets as 0.4 (+0 magic, +4 time, +6 date, +8 pw, +9 filt,
+10 unk/method 0x3C=60 core5, +11 ndirs, +13 unk, +17 base, +21 nfiles u24,
+24 names_len, +27 cmp1, +30 cmp2, +33 cmp3, +36 crc1, +38 crc2, +40 crc3,
+42 hdr_crc). Sample uharc_u.uha 151822 bytes: ndirs 1, nfiles 1,
names 11, cmp 15/4/8, layout 44+151747+4+15+4+8=151822 verified.
Extracted ground truth (uharc_u.exe x -y+ -o+ -d0, silent console):
single file `uharc_u.c`, 1047398 bytes (= unk_dfd total).
Test vectors (updated uharc_u.exe 218112 bytes creates correctly;
original 218592 silent no-op): 6-file corpus (5840 bytes), modes
m0/m1/m2/m3/mx/mz -> 03_m*.uha (5996/1598/1598/1597/1598/1598 bytes).
Headers: unk_dfa m0 00, m1 27, m2/mx/mz 35, m3 43; base m0 5840 STORE,
others 1441-1442; names 62, cmp 50/13/25, crc BD63/25CA/3CF2 identical
across modes (directory method-independent). All round-trip byte-exact
via original (6/6 MATCH per mode).
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
