# UHARC 0.3 analogues of the uharc-rs 0.6 corpus

Generated from `uharc-rs/testdata/src` (shared ground truth) with the
original `uharc_u.exe` (0.3). Each `arc/*.uha` mirrors an
`uharc-rs/testdata/arc/*.uha` stem and stored file list.

Generation (`tools/gen03.py`):

* mode from 0.6 method ranges (`-m0/-m1/-m2/-m3/-mx/-mz/-mw/-mr`),
  archiver picks the 0.3 method (0.3 ranges differ: core 3 `8–21`,
  core 4 `22–35`, core 5 `36–79`).
* `-md` from name (`_md1/_md4/_md64/_md1024/_md32m`), capped at
  `-md8192` (0.3 max; 0.4/0.6 allow 32768).
* `-pwsecret` for `pw*` (`-ph` does not exist on 0.3 — `pwph_*`
  is generated as plain `pw`).
* `-mm+/-` only for `full_*mmon/mmoff`, otherwise default.
* `-pr -ed+` when stored paths contain `/`, else `-p-`.

Verification with `uharc06_unpack.exe`: **103/103** `t` pass
(`-pwsecret` for `pw*`).
