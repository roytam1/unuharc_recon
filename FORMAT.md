# UHARC archive formats — 0.2, 0.3, 0.4, 0.6

On-disk reference for the UHARC archive family, recovered from
`Uharc02.exe`, `uharc_u.exe`, `UHARC04.EXE` and `UNUHARC06.EXE`.

All multi-byte values are little-endian. `u8`, `u16`, `u24`, `u32` are
unsigned integers of 1, 2, 3 and 4 bytes. `u24` is stored as three
little-endian bytes.

**CRC32** always means the reflected IEEE polynomial `0xEDB88320`, initial
value `0xFFFFFFFF`, final complement (`~crc`), i.e. the same CRC as `zlib`.
In code snippets, `crc_update(crc, data, len)` is the raw register update
without the final complement; the finished CRC is
`~crc_update(0xFFFFFFFF, data, len)`.

## 1. Common structure

### 1.1 Archive layout

```
0.6    header(39) | packed solid stream | tab0 | tab1 | tab2
0.4    header(44) | packed solid stream | tab0 | tab1 | tab2 | tab3
0.3    header(44) | packed solid stream | tab0 | tab1 | tab2 | tab3
0.2    header(48) | packed solid stream | tab0 | tab1 | tab2 | tab3
```

The packed stream starts immediately after the header. The tables follow it,
in order. Every table is encrypted with the default-password cipher before it
is decompressed; `tab0` is a plain array (method 0) that is still encrypted.

### 1.2 Version comparison

| Property | 0.2 | 0.3 | 0.4 | 0.6 |
|----------|-----|-----|-----|-----|
| Magic | `UHA\x02` | `UHA\x03` | `UHA\x04` | `UHA\x06` |
| Header size | 48 | 44 | 44 | 39 |
| Encrypted range | 4..43 (40 B) | 4..39 (36 B) | 4..39 (36 B) | 4..34 (31 B) |
| Header key | `raw[44]` | `raw[42]` | `raw[42]` | `raw[37]` |
| Header CRC check | full 32-bit | low 16-bit | low 16-bit | low 16-bit |
| Packed stream length field | `+20` | `+17` | `+17` | `+17` |
| Table base | `48+len` | `44+len` | `44+len` | `39+len` |
| Tables | 4 | 4 | 4 | 3 |
| Table methods | 0 / 7 / 2 / 7 | 0 / 17 / 2 / 7 | 0 / 17 / 2 / 7 | 0 / 17 / 17 |
| Table cipher | own | 0.4/0.6 | 0.4/0.6 | 0.4/0.6 |
| Stream method field | `+9` | `+10` | `+10` | `+10` |
| Directory entries | 19 B | 19 B | 19 B | 19 B |

`tab0` holds file CRCs, `tab1` holds names, the remaining table(s) hold
date/time/size/method metadata. 0.6 folds the last two tables into one.

### 1.3 Directory entry (all versions)

The table data is merged into this 19-byte logical record:

```
+0   u16  directory index (also carries flags)
+2   u8   method / attribute byte
+3   u32  size, stored as a delta
+7   u32  file CRC32
+11  u16  DOS date
+13  u16  DOS time
+15  u32  name pointer (reconstructed in memory)
```

Sizes are stored backwards. With `size[-1] = 0`:

```
size[i] = size[i-1] - stored_delta[i]      (32-bit wrap)
offset[i] = offset[i-1] + size[i]
```

An empty file has size 0 and CRC 0. The stream demultiplexer uses the
cumulative offsets, so size reconstruction is the key step for extraction.

### 1.4 Solid stream

In the archives handled here, file data is one compressed block:

* The codec comes from the header method byte (0.2 `+9`, 0.3/0.4/0.6 `+10`).
* Decoding produces `sum(file sizes)` bytes (the header's total field).
* The splitter cuts the output at the cumulative file ends, applies the
  optional whole-file post-filter, and checks each file's CRC.
* The 0.3/0.4/0.6 decoders may reset their models at file boundaries. The
  reset points come from the cumulative file-end table, not from the stream.

`entry+2` also contains a per-file byte. For 0.6 it is the method byte
(`0xFF` = skip marker). For 0.2/0.3/0.4 it is displayed as `m=` by the tool
but is **not** used to select the stream codec; the verified archives are
solid and use the header method. Non-solid archives with mixed per-file
codecs are not implemented.

### 1.5 DOS date and time

Standard FAT format:

* time: `hour << 11 | minute << 5 | second/2`
* date: `(year - 1980) << 9 | month << 5 | day`

## 2. UHARC 0.6

### 2.1 Header (39 bytes)

| Offset | Type | Field | Meaning |
|--------|------|-------|---------|
| +0 | u32 | magic | `0x06414855` = `UHA\x06` |
| +4 | u16 | time | DOS time |
| +6 | u16 | date | DOS date |
| +8 | u8 | flags1 | bit 2: tables encrypted with `-pw` password, bit 1: packed stream encrypted (see 6.6); other bits unknown |
| +9 | u8 | flags2 | bit 0: EXE filter, bit 1: multimedia filter, bit 7: "big" filter |
| +10 | u8 | method | solid-stream method, 0–87 |
| +11 | u16 | ndirs | number of directory strings |
| +13 | u32 | total_unpacked | total decoded bytes |
| +17 | u32 | data_off | packed stream length |
| +21 | u24 | nfiles | number of entries |
| +24 | u24 | tab1_dec_size | decoded size of tab1 |
| +27 | u24 | tab1_cmp_size | compressed size of tab1 |
| +30 | u24 | tab2_cmp_size | compressed size of tab2 |
| +33 | u16 | tab1_crc | low 16 of CRC(tab0 + tab1) |
| +35 | u16 | tab2_crc | low 16 of CRC(tab2) |
| +37 | u16 | hdr_crc | low 16 of CRC(decrypted 4..34) |

Note that `tab1_crc` at +33 lies inside both the encrypted range and the
header-CRC range. `hdr_crc` at +37 is outside the encrypted range and supplies
the key byte.

### 2.2 Header decryption and CRC

```c
key = raw[37];
for (i = 0; i < 31; i++)
    raw[4 + i] ^= (key + i*i) & 0xFF;
```

Update a raw CRC from `0xFFFFFFFF` over decrypted bytes 4..34; the stored
field is the final value `(~crc) & 0xFFFF`:

```
(~crc_update(0xFFFFFFFF, decrypted[4..34]) & 0xFFFF) == u16(raw + 37)
```

### 2.3 Tables

```
tab0_off = 39 + data_off
tab1_off = tab0_off + 4*N
tab2_off = tab1_off + tab1_cmp_size
stream   = tab2_off + tab2_cmp_size
```

| Table | Decoded size | Method | Content |
|-------|--------------|--------|---------|
| tab0 | `4*N` | 0 (STORE) | N file CRCs, full 32-bit |
| tab1 | `tab1_dec_size` | 17 (core 3) | `ndirs` Pascal strings, then N Pascal strings |
| tab2 | `11*N` | 17 (core 3) | date/time/size/method metadata |

`tab2` is laid out as byte planes:

```
0..2N      N u16: DOS time      -> entry+13
2N..4N     N u16: DOS date      -> entry+11
4N..6N     N u16: dir index     -> entry+0  (low byte at 4N+i, high at 5N+i)
6N..10N    N u32: size delta    -> entry+3  (byte planes)
10N..11N   N u8 : method        -> entry+2
```

CRC chaining:

```
crc = 0xFFFFFFFF
crc = crc_update(crc, decoded_tab0)
crc = crc_update(crc, decoded_tab1)
check (~crc & 0xFFFF) == tab1_crc

crc = 0xFFFFFFFF
crc = crc_update(crc, decoded_tab2)
check (~crc & 0xFFFF) == tab2_crc
```

### 2.4 Solid stream and filters

The stream is decoded with the header method and split by the cumulative
sizes. After each file is complete:

1. If `flags2 & 1` and the file is at least `0x400` bytes, apply the EXE
   post-filter over the whole file.
2. Check `CRC32(file)` against `tab0[i]`.
3. Write the file (skip in test mode).

`flags2 & 2` selects the multimedia filter (BMP/WAV/VOC), which is not wired
in yet. Entries with `method == 0xFF` and size 0 are empty-file markers.

### 2.5 Codecs

| Method | Core | Codec |
|--------|------|-------|
| 0 | 0 | STORE |
| 1 | 1 | RLE / delta |
| 2–7 | 2 | Huffman/LZ, MSB-first bits |
| 8–23 | 3 | ALZ, dictionary `1 << (((m-8) % 16) + 10)` |
| 24–39 | 4 | LZP + PPM, dictionary `1 << (((m-24) % 16) + 10)` |
| 40–87 | 5 | PPM2, dictionary `1 << (((m-40) % 16) + 10)` |

Core 5 is split into groups: 40–55 = group 1, 56–71 = group 2, 72–87 = group 3.

The range coder used by cores 3–5:

```
init:  code = 0, range = 0xFFFFFFFF, read 5 bytes big-endian
renorm: while (range < 0x1000000) { code = (code << 8) | getbyte(); range <<= 8; }
```

### 2.6 SFX overlay

If the file starts with `MZ`, read `e_lfanew` at `+0x3C`, check for `PE`, and
add the resulting overlay offset to every seek. Plain `UHA` archives have
offset 0.

### 2.7 Original CLI

`x/X` extract with paths, `e/E` extract flat, `t/T` test; `-?`/`/?` help;
`-pw` sets a password. The port implements the four data commands only.

## 3. UHARC 0.4

### 3.1 Header (44 bytes)

| Offset | Type | Field | Meaning |
|--------|------|-------|---------|
| +0 | u32 | magic | `0x04414855` = `UHA\x04` |
| +4 | u16 | time | DOS time |
| +6 | u16 | date | DOS date |
| +8 | u8 | flags_pw | bit 1: `-pw` password archive (tables and packed stream encrypted, see 6.6) |
| +9 | u8 | flags_filt | bit 0: EXE filter enabled |
| +10 | u8 | method | solid-stream method |
| +11 | u16 | ndirs | number of directory strings |
| +13 | u32 | total_unpacked | total decoded bytes |
| +17 | u32 | base_e01 | packed stream length |
| +21 | u24 | nfiles | number of entries |
| +24 | u24 | names_len | decoded size of tab1 |
| +27 | u24 | cmp1 | compressed size of tab1 |
| +30 | u24 | cmp2 | compressed size of tab2 |
| +33 | u24 | cmp3 | compressed size of tab3 |
| +36 | u16 | crc1 | low 16 of CRC(tab0 + tab1) |
| +38 | u16 | crc2 | low 16 of CRC(tab2) |
| +40 | u16 | crc3 | low 16 of CRC(tab3) |
| +42 | u16 | hdr_crc | low 16 of CRC(decrypted 4..39) |

### 3.2 Decryption and CRC

```c
key = raw[42];
for (i = 0; i < 36; i++)
    raw[4 + i] ^= (key + i*i) & 0xFF;
```

The check uses the final (complemented) CRC:

```
(~crc_update(0xFFFFFFFF, decrypted[4..39]) & 0xFFFF) == u16(raw + 42)
```

### 3.3 Tables and layout

```
tab0_off = 44 + base_e01
tab1_off = tab0_off + 4*N
tab2_off = tab1_off + cmp1
tab3_off = tab2_off + cmp2
end      = tab3_off + cmp3
```

| Table | Decoded size | Method | Content |
|-------|--------------|--------|---------|
| tab0 | `4*N` | 0 (STORE) | N file CRCs |
| tab1 | `names_len` | 17 (core 3) | `ndirs` Pascal strings + N Pascal strings |
| tab2 | `4*N` | 2 (core 2, LSB) | N u16 dates, then N u16 times |
| tab3 | `7*N` | 7 (core 2, LSB) | dir index, size, method as byte planes |

`tab3` planes:

```
0..N     dir index low byte
N..2N    dir index high byte
2N..3N   size byte 0
3N..4N   size byte 1
4N..5N   size byte 2
5N..6N   size byte 3
6N..7N   method / attribute byte
```

CRC chaining is the same as 0.3: `crc1` covers tab0+tab1, `crc2` covers tab2,
`crc3` covers tab3.

Worked example, `uha04.UHA`:

```
file size      21687
ndirs          1
nfiles         8
total_unpacked 43307
base_e01       21470
names_len      88
cmp            69 / 32 / 40
layout         44 + 21470 + 32 + 69 + 32 + 40 = 21687
```

### 3.4 File data and codecs

Packed data occupies `44 .. 44+base_e01`. It is decoded as one solid stream
with the header method and split by cumulative sizes. The EXE post-filter is
applied per file when `flags_filt & 1` and the file is at least `0x400` bytes.

Method ranges are identical to 0.6. The coders are not: 0.4 core 3 uses 330
contexts (0.6 uses 319), and 0.4 core 5 is an earlier PPM2/LZW variant.

Bit-exact status: all six cores are ported, including core 4 (methods
24–39, `uha04_dec4.c`). `uha04.UHA` (method 62), the 33 MB `gmud-04m3` /
`gmud-04z` and `gmud-04x` (44/44 files, core 4) archives pass.

## 4. UHARC 0.3

### 4.1 Header

Identical to 0.4 with magic `UHA\x03` (`0x03414855`). The same 44-byte layout,
the same 36-byte decryption, the same low-16 CRC check and the same table
offsets apply.

The `+10` byte is the solid-stream method. For the test corpus it is:

| Mode | Header +10 | Codec reached |
|------|-----------|---------------|
| m0 | `0x00` | STORE |
| m1 | `0x27` = 39 | core 5 |
| m2 / mx / mz | `0x35` = 53 | core 5 |
| m3 | `0x43` = 67 | core 5 |

The `l` command of the original displays `STORE / ALZ:1 / ALZ:2 / ALZ:3`.
Those labels do not match the method ranges; extraction confirms that `+10`
is the codec ID, not a mode label.

### 4.2 Tables

Same four tables and methods as 0.4:

```
tab0  4*N        method 0   CRCs
tab1  names_len  method 17  Pascal strings
tab2  4*N        method 2   N dates, then N times
tab3  7*N        method 7   dir/size/method planes
```

Tabs 2 and 3 use the same date-first / byte-plane layout as 0.4. `crc1`
covers tab0+tab1, `crc2` covers tab2, `crc3` covers tab3.

Worked example, `uharc_u.uha`:

```
file size      151822
ndirs          1
nfiles         1
total_unpacked 1047398
base_e01       151747
names_len      11
cmp            15 / 4 / 8
layout         44 + 151747 + 4 + 15 + 4 + 8 = 151822
decoded        uharc_u.c, 1047398 bytes, CRC32 D9BA98B4
```

### 4.3 Codecs

| Method | Core | Codec |
|--------|------|-------|
| 0 | 0 | STORE |
| 1 | 1 | RLE |
| 2–7 | 2 | LZW, LSB-first |
| 8–21 | 3 | ALZ, dictionary `1 << (((m-8) % 14) + 10)` |
| 22–35 | 4 | LZP / PPM (`uha03_dec4.c`), dictionary `1 << (((m-22) % 14) + 10)` |
| 36–79 | 5 | PPM2/LZW, dictionary `1 << (((m-36) % 14) + 10)` |
| 80 | 0 | alias for STORE |

Core 3 is a separate coder from 0.4/0.6: 24-bit range/offset, 328 contexts,
multi-exclusion escapes, and the first stream byte is `~out_len`. Cores 4 and
5 are also version-specific.

### 4.4 Notes

* `03_m2`, `03_mx` and `03_mz` are byte-identical (same MD5): all three store
  method 53 and the original displays them as `ALZ:2`. The corpus therefore
  covers one core-5 configuration, not distinct PPM/LZP modes. The 0.4 build
  distinguishes the `x`/`z` modes.
* 0.3 applies the EXE post-filter (`sub_407D44`), gated by `flags_filt & 1`
  and size `>= 0x400`. The port implements it.
* `UHCLS.EXE` is the same format with magic `CLS\xFF` (`0xFF534C43`). See
  `UHCLS_CREATE.md`.

## 5. UHARC 0.2

### 5.1 Header (48 bytes)

| Offset | Type | Field | Meaning |
|--------|------|-------|---------|
| +0 | u32 | magic | `0x02414855` = `UHA\x02` |
| +4 | u16 | time | DOS time |
| +6 | u16 | date | DOS date |
| +8 | u8 | reserved | 0 |
| +9 | u8 | method | solid-stream method, 0–43 |
| +10 | u16 | ndirs | number of directory strings |
| +12 | u32 | nfiles | number of entries |
| +16 | u32 | total_unpacked | total decoded bytes |
| +20 | u32 | data_len | packed stream length |
| +24 | u32 | names_len | decoded size of tab1 |
| +28 | u32 | cmp1 | compressed size of tab1 |
| +32 | u32 | cmp2 | compressed size of tab2 |
| +36 | u32 | cmp3 | compressed size of tab3 |
| +40 | u32 | contents_crc | full CRC32 over all four decoded tables |
| +44 | u32 | hdr_crc | full CRC32 over decrypted 4..43 |

### 5.2 Decryption and CRC

```c
key = raw[44];
for (i = 0; i < 40; i++)
    raw[4 + i] ^= (key + i*i) & 0xFF;
```

The key byte is the low byte of the stored full-32-bit header CRC. Unlike
later versions, the check is not truncated:

```
(~crc_update(0xFFFFFFFF, decrypted[4..43])) == u32(raw + 44)
```

### 5.3 Tables

```
tab0_off = 48 + data_len
tab1_off = tab0_off + 4*N
tab2_off = tab1_off + cmp1
tab3_off = tab2_off + cmp2
end      = tab3_off + cmp3
```

| Table | Decoded size | Method | Content |
|-------|--------------|--------|---------|
| tab0 | `4*N` | 0 (STORE) | N file CRCs |
| tab1 | `names_len` | 7 (core 2) | `ndirs` Pascal strings + N Pascal strings |
| tab2 | `4*N` | 2 (core 2) | N u16 **times**, then N u16 **dates** |
| tab3 | `7*N` | 7 (core 2) | sizes as dword array, then dir/method planes |

`tab3` is the odd one out:

```
0..4N    N u32 size deltas, consecutive little-endian dwords
4N..5N   dir index low byte
5N..6N   dir index high byte
6N..7N   method / attribute byte
```

All four tables are CRC-chained and compared once:

```
crc = 0xFFFFFFFF
crc = crc_update(crc, tab0); crc = crc_update(crc, tab1)
crc = crc_update(crc, tab2); crc = crc_update(crc, tab3)
check (~crc) == contents_crc
```

Worked example, `uha02.UHA`:

```
file size      11860
ndirs          1
nfiles         7
total_unpacked 31447
data_len       11656
names_len      80
cmp            65 / 28 / 35
layout         48 + 11656 + 28 + 65 + 28 + 35 = 11860
```

### 5.4 File data and filters

The packed stream at `48 .. 48+data_len` is one solid block decoded with the
header method and split by cumulative sizes. Method 0 is a plain concatenation
of the non-empty files; the empty file contributes no bytes.

Method ranges:

| Method | Core | Codec |
|--------|------|-------|
| 0 | 0 | STORE |
| 1 | 1 | RLE |
| 2–7 | 2 | LZW, LSB-first |
| 8–43 | 3 | PPM/LZ + range coder |

Core 3 uses 16-bit init, a token/literal/match/run model, and groups
8–19 / 20–31 / 32–43 with dictionary `1 << (((m-8) % 12) + 10)`.

0.2 stores no filter flag. The EXE filter is applied unconditionally, gated
only by an `MZ`/`ZM` header and `len >= 0x400`; it is the simpler 0.2 E8/E9
pass, not the 0.3/0.4/0.6 relocation-table filter.

`-mm+` in 0.2 is **not** a post-decode multimedia filter. It selects an
encoder-side literal-predictor mode (in-stream control token 0–5, delta at
byte distance `((mode-1) & 3) + 1`). The decoded output is already the
original data.

## 6. Shared algorithms

### 6.1 Header cipher

One bytewise XOR, identical in all versions but with a different range and key
position:

```c
raw[4 + i] ^= (key + i*i) & 0xFF;
```

`key` is a byte taken from the header CRC field (outside the encrypted range):

| Version | Range | Key |
|---------|-------|-----|
| 0.2 | `i = 0..39` | `raw[44]` |
| 0.3 | `i = 0..35` | `raw[42]` |
| 0.4 | `i = 0..35` | `raw[42]` |
| 0.6 | `i = 0..30` | `raw[37]` |

### 6.2 Table cipher — 0.3, 0.4, 0.6

Default password: `"UHARC Default Password"` (22 bytes).

S-box initialisation:

```c
sb[0] = 22;
for (k = 0; k < 255; k++)
    sb[1 + k] = pwd[k % 22];

for (off = 0; off < 256; off += 8) {
    crc = crc_update(0xFFFFFFFF, sb, 256);
    for (k = 0; k < 4; k++) {
        sb[off + k]     ^= (crc  >> (8*k)) & 0xFF;
        sb[off + 4 + k] ^= (~crc >> (8*k)) & 0xFF;
    }
}
```

The S-box is reset to this state before every table. Stream state:
`pos = 0`, `mix = 0xFE`.

```c
a = S[pos]; pos++;
b = S[pos];
mix++;
S[pos] = b + a;
out    = in ^ (b + a);
S[mix] = S[mix] - b;
```

All arithmetic is byte-wide (`uint8_t` wrap). 0.3 uses the same cipher as 0.4;
this is verified, not assumed.

With `-pw<password>` the same construction uses the given password
instead of the default one: `sb[0]` is the password length and
`sb[1+k]` cycles over the password bytes. The 32-round schedule and the
per-byte stream transform are unchanged, with a fresh S-box per table.
See 6.6 for which parts of an archive this covers.

### 6.3 Table cipher — 0.2

Same password, different S-box fill and a simpler stream:

```c
for (i = 0; i < 256; i++)
    sb[i] = pwd[i % 22];          /* no length prefix */

/* same 32-round CRC/XOR key schedule as 6.2 */

a = S[pos]; pos++;
S[pos] = S[pos] + a;
out = S[pos] ^ in;
```

There is no `mix` index. 0.2's cipher is **not** byte-identical to 0.4/0.6.

### 6.4 EXE post-filter

* **0.2**: plain E8/E9 operand rewrite. Gate: `MZ` or `ZM`, length `>= 0x400`,
  no stored flag. The scan advances 4 bytes per position, or 5 after rewriting
  an `E8`/`E9`.
* **0.3/0.4/0.6**: shared algorithm. It rewrites `E8`/`E9` operands through a
  128-entry move-to-front address table **and** rebuilds the MZ relocation
  table. Gate: header filter flag and size `>= 0x400`; internally also accepts
  `MZ`/`ZM`/`MQ`, ELF and `FBOV`.
* **0.6** uses the same algorithm as 0.3/0.4, selected by `flags2 & 1`.

### 6.5 Multimedia filter

`uha02_mm.c` implements the BMP/WAV/VOC predictor for 0.3/0.4/0.6 and is
verified by encoder-inverse round-trip. It is not yet wired into extraction.
0.2 needs no such pass.

### 6.6 Password-protected archives (`-pw`)

With `-pw<password>`, table decryption (§6.2) uses the given password,
and the packed stream is transformed the same way with a fresh S-box
over the whole packed buffer.

Flag bits (verified against `-pwpassword` samples with password
`"password"`, plus the `06-lic.uha` control built from the same source
without a password):

* 0.6 `flags1` bit 2: tables encrypted; bit 1: packed stream encrypted.
  `06-password.uha` has both and extracts byte-identical to the control;
  `06-phneg-password.uha` has only the stream bit (default-cipher
  tables, encrypted stream).
* 0.4 / 0.3 `flags_pw` (byte +8) bit 1: tables and packed stream
  encrypted (`04-password.uha`, `03-password.uha`).
* The header always uses the stored-key byte cipher (§2.2 etc.) with a
  random per-archive key, independent of `-pw`.

A missing password fails fast (`Password required`); a wrong password
decrypts the tables to garbage and fails their CRC check.

## Appendix A — Verification data

Header dumps after decryption:

```
D3X.UHA (0.6), decrypted, 16 bytes per line
55 48 41 06 59 AE CD 54 00 01 22 04 00 D2 E6 0E
00 12 B0 01 00 2D 00 00 00 02 00 EF 00 00 16 01
00 4C 5D 32 FD EC 23

time AE59 = 21:50, date 54CD = 13-Jun-2022
flags 00/01, method 0x22 = 34, ndirs 4
total 0x0EE6D2 = 976594, data_off 0x01B012 = 110610
nfiles 45, tab1_dec 512, tab1_cmp 239, tab2_cmp 278
tab1_crc 5D4C, tab2_crc FD32, hdr_crc 23EC
```

```
uha02.UHA (0.2)
7 files, 31447 bytes unpacked, data_len 11656, names_len 80
cmp 65/28/35, layout 48+11656+28+65+28+35 = 11860
```

```
uha04.UHA (0.4)
8 files, 43307 bytes unpacked, archive 21687 bytes
base_e01 21470, names_len 88, cmp 69/32/40
```

```
uharc_u.uha (0.3)
1 file, 1047398 bytes, base_e01 151747, names_len 11
cmp 15/4/8, layout 44+151747+4+15+4+8 = 151822
```

Table decryption is proven by recovering the file CRCs from tab0. For the
0.3/0.4/0.6 test corpus the decrypted tab0 is:

```
00000000 7125DAA3 51A02E01 B70B4C26 021E1879 6F3BDB8B
```

For `uharc_u.uha` it is `D9BA98B4`, which matches the CRC reported by
`uharc_u.exe l -d2`. For 0.2 the cipher recovers
`00000000 9166788F 3CD931D7 5FC97467`, exactly the CRCs measured from the
archive's own plaintext STORE stream.

## Appendix B — Open questions

* **0.6 `flags1` / `flags2`**: only the filter and password bits are used.
  The remaining bits are unidentified.
* **0.6 method `0xFF`**: treated as an empty-file marker. A non-zero size is
  rejected as a data error rather than decoded.
* **0.3 / 0.4 non-solid archives**: `entry+2` may select per-file codecs in
  archives created with solid mode off. The verified archives are solid, so
  this path is untested.
* **0.2 `total_size` vs. filtered sizes**: the directory sum may differ from
  the header total when filters/padding are involved; the reader stays
  permissive on this mismatch.
* **UHCLS `CLS\xFF`**: recognized by the patcher but not by this reader.
