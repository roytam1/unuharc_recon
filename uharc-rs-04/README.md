# UHARC 0.4 analogues of the uharc-rs 0.6 corpus

Generated from `uharc-rs/testdata/src` (shared ground truth) with the
original `UHARC04.EXE` (0.4). Each `arc/*.uha` mirrors an
`uharc-rs/testdata/arc/*.uha` stem and stored file list.

Generation (`tools/gen03.py`, ver `04`):

* mode from 0.6 method ranges (`-m0/-m1/-m2/-m3/-mx/-mz/-mw/-mr`),
  0.4 ranges match 0.6.
* `-md` from name, `-pwsecret` for `pw*` (no `-ph` on 0.4).
* `-mm+/-` only for `full_*mmon/mmoff`.

Verification with `uharc06_unpack.exe`: **101/103** `t` pass
(`-pwsecret` for `pw*`). Failures are pre-existing `uha04_dec5`
small-dict bugs, unrelated to filters: `alz2_md64` (method 62,
`-m2 -md64`) and `md1_m1` (method 40, `-m1 -md1`) fail with
`Data error` at byte ~2788; `-md256+` (method 48) passes. All other
archives, including `filt_fax_*` (fax, bit7) and `ppm_kennedy/cant`
(bit6), pass.
