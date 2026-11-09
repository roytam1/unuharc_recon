#!/usr/bin/env python3
"""Generate 0.2 analogues (8.3 short names) under uharc-rs-02/."""
import os, subprocess, shutil

ROOT = r"\\roy-xp\c\devel\UNUHARC"
SRC = os.path.join(ROOT, "uharc-rs-02", "src83")
ARC = os.path.join(ROOT, "uharc-rs-02", "arc")
MSDOS = r"C:\Users\Roy\AppData\Local\Temp\opencode\msdos.exe"
ARCH = r"C:\Users\Roy\AppData\Local\Temp\opencode\UHARC02.EXE"
UNPACK = r"C:\Users\Roy\AppData\Local\Temp\opencode\uharc06_unpack.exe"

os.makedirs(ARC, exist_ok=True)

singles = {
    "TEXT": ["TEXT.TXT"],
    "ZERO": ["ZEROS.BIN"],
    "ONE": ["ONE.BIN"],
    "RAND": ["RANDOM.BIN"],
    "SOUN": ["SOUND.WAV"],
    "IMAG": ["IMAGE.BMP"],
}
modes = ["-m0", "-m1", "-m2", "-m3", "-mr", "-mw"]

def run(arcname, mode, files):
    out = os.path.join(ARC, arcname)
    if os.path.exists(out):
        os.remove(out)
    listpath = os.path.join(SRC, "LIST02.TXT")
    with open(listpath, "w") as f:
        f.write(" ".join(files))
    # use -pr for subpaths to preserve dirs, -p- otherwise
    pflag = "-pr" if any("\\" in f for f in files) else "-p-"
    cmd = [MSDOS, ARCH, "a", "-d0", "-y+", "-o+", pflag, mode, out, "@LIST02.TXT"]
    r = subprocess.run(cmd, cwd=SRC, capture_output=True)
    ok = os.path.exists(out)
    if ok:
        t = subprocess.run([UNPACK, "t", out], capture_output=True, text=True)
        stat = "OK" if t.returncode == 0 else "FAIL"
        print(f"{arcname:12s} {mode} {stat} t_rc={t.returncode}", flush=True)
    else:
        # DOS truncates long arc names to 8.3; check SRC for truncated
        print(f"{arcname:12s} {mode} NOFILE rc={r.returncode}", flush=True)

for stem, files in singles.items():
    for i, mode in enumerate(modes):
        arcname = f"R02{stem[:4]}{i}.UHA"  # 3+4+1=8 chars
        run(arcname, mode, files)

media = [r"EXTRA\MEDIA\SOUND.WAV", r"EXTRA\MEDIA\C3.WAV", r"EXTRA\MEDIA\M16.WAV",
         r"EXTRA\MEDIA\S8.WAV", r"EXTRA\MEDIA\B24.WAV", r"EXTRA\MEDIA\SMALL.WAV",
         r"EXTRA\MEDIA\SND.VOC", r"EXTRA\MEDIA\PIC24.BMP", r"EXTRA\MEDIA\PIC8.BMP"]
for i, mode in enumerate(modes):
    run(f"R02MED{i}.UHA", mode, media)

exe = [r"EXTRA\EXE\PROG.BIN", r"EXTRA\EXE\ELF32.BIN", r"EXTRA\EXE\FBOV.BIN",
       r"EXTRA\EXE\ELF64.BIN", r"EXTRA\EXE\E8STRESS.EXE", r"EXTRA\EXE\DOSREL~2.EXE",
       r"EXTRA\EXE\DOSREL.EXE", r"EXTRA\EXE\ZM.EXE", r"EXTRA\EXE\DOSREL~1.EXE"]
for i, mode in enumerate(modes):
    run(f"R02EXE{i}.UHA", mode, exe)
