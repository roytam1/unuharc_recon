# UHARC 0.2 analogues (8.3)

Generated from 8.3 copies of `uharc-rs/testdata/src` in `src83/`
(uppercase, see mapping below) with the original DOS32 `UHARC02.EXE`
via the MS-DOS Player (`msdos.exe UHARC02.EXE a …`, see `MAKE02.BAT`
for the DOS-only constraint). Archive stems are 8-char `R02*`
(e.g. `R02TEXT0.UHA`); the 0.6 long stems (`single_text_m0` …)
would truncate to `SINGLE_T` and collide in one directory.

8.3 mapping (`src83/` → `uharc-rs/testdata/src/`):

* dir `alz_media` → `ALZMEDIA`
* `dosrel_bad.exe` → `DOSREL~1.EXE`, `dosrel_big.exe` → `DOSREL~2.EXE`,
  `dosrel_few.exe` → `DOSREL~3.EXE`, `dosrel_min.exe` → `DOSREL~4.EXE`,
  `fax_wrong.img` → `FAX_WRON.IMG`, `cp.html` → `CP.HTM`
* everything else uppercased verbatim (already 8.3)

`arc/` holds 48 archives: 6 singles (`TEXT/ZERO/ONE/RAND/SOUN/IMAG`)
× 6 modes (`-m0/-m1/-m2/-m3/-mr/-mw`), `R02MED0-5` (9 media files) and
`R02EXE0-5` (9 EXE files). Notes for 0.2:

* no `-pw`/`-ph`, `-md ≤ 2048`, no PPM/LZP split (all core 3),
  `-mm+` is an in-stream predictor, not a post-filter.
* `-ed+` is rejected (`RC 255`); `-pr`/`-p-` only.
* `STORE` (`-m0`) bypasses the MZ filter (stored bytes verbatim);
  decoder honours `hdr.method != 0` (see `uha02.c`).
* multi-file needs `@LIST02.TXT` (DOS 127-char command line,
  `LIST02.TXT` itself is not part of the corpus).

Verification with `uharc06_unpack.exe`: **48/48** `t` pass,
`x` output byte-identical to `src83/` (names uppercase by DOS).

Generation script: `tools/gen02.py` (needs `msdos.exe` +
`UHARC02.EXE` in `C:\Users\Roy\AppData\Local\Temp\opencode\`).
