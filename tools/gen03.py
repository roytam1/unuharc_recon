#!/usr/bin/env python3
"""Generate 0.3/0.4 analogues of uharc-rs 0.6 test archives."""
import os, sys, glob, subprocess, re

ROOT = r"\\roy-xp\c\devel\UNUHARC"
SRC = os.path.join(ROOT, "uharc-rs", "testdata", "src")
ARC06 = os.path.join(ROOT, "uharc-rs", "testdata", "arc")
UNPACK = r"C:\Users\Roy\AppData\Local\Temp\opencode\uharc06_unpack.exe"
ARCH = {
    "03": r"C:\Users\Roy\AppData\Local\Temp\opencode\uharc_u.exe",
    "04": r"C:\Users\Roy\AppData\Local\Temp\opencode\UHARC04.EXE",
}
OUT = {
    "03": os.path.join(ROOT, "uharc-rs-03", "arc"),
    "04": os.path.join(ROOT, "uharc-rs-04", "arc"),
}

def method_of(name):
    raw = open(os.path.join(ARC06, name), "rb").read()
    seed = raw[37]
    dec = bytearray(raw[:39])
    for i in range(31):
        dec[4+i] = raw[4+i] ^ ((seed + i*i) & 0xFF)
    return dec[10], dec[8], dec[9]

def mode_switch(method):
    if method == 0: return "-m0"
    if method == 1: return "-mr"
    if 2 <= method <= 7: return "-mw"
    if 8 <= method <= 23: return "-mz"
    if 24 <= method <= 39: return "-mx"
    if 40 <= method <= 55: return "-m1"
    if 56 <= method <= 71: return "-m2"
    return "-m3"

def md_switch(name, ver):
    # 0.3 max -md is 8192 (help); 0.4/0.6 max 32768. Cap for 03.
    if "_md1.uha" in name or name == "md1_m1.uha": return "-md1"
    if "md4" in name: return "-md4"
    if "md64" in name: return "-md64"
    if "md1024" in name: return "-md1024"
    if "md32m" in name: return "-md8192" if ver == "03" else "-md32768"
    return None

def list_files(arc, name):
    args = [UNPACK, "l"]
    if name.startswith("pw"):
        args.append("-pwsecret")
    args.append(arc)
    out = subprocess.run(args, capture_output=True, text=True, errors="replace")
    files = []
    for line in (out.stdout + out.stderr).splitlines():
        m = re.match(r"^\s+(.+?)\s+\(\d+ bytes\)", line)
        if m:
            files.append(m.group(1).strip())
    return files

def main(which):
    vers = ["03", "04"] if which == "both" else [which]
    for ver in vers:
        os.makedirs(OUT[ver], exist_ok=True)
    arcs = sorted(glob.glob(os.path.join(ARC06, "*.uha")))
    for arc in arcs:
        name = os.path.basename(arc)
        files = list_files(arc, name)
        method, f1, f2 = method_of(name)
        mode = mode_switch(method)
        pw = name.startswith("pw")
        mm = None
        if "mmon" in name: mm = "-mm+"
        elif "mmoff" in name: mm = "-mm-"
        has_slash = any("/" in f or "\\" in f for f in files)
        psw = "-pr" if has_slash else "-p-"
        fargs = [f.replace("/", os.sep) for f in files]
        for ver in vers:
            out = os.path.join(OUT[ver], name)
            if os.path.exists(out):
                # verify it has files (pw archives generated empty before fix)
                if pw:
                    chk = subprocess.run([UNPACK, "l", "-pwsecret", out], capture_output=True, text=True, errors="replace")
                    if "files" in chk.stdout and "0 files" in chk.stdout:
                        os.remove(out)
                    else:
                        continue
                else:
                    continue
            md = md_switch(name, ver)
            cmd = [ARCH[ver], "a", "-d0", "-y+", "-o+", psw, "-ed+", mode]
            if md: cmd.append(md)
            if mm: cmd.append(mm)
            if pw: cmd.append("-pwsecret")
            cmd.append(out)
            cmd.extend(fargs)
            r = subprocess.run(cmd, cwd=SRC, capture_output=True, text=True, errors="replace")
            print(f"{ver} {name:28s} {'OK' if os.path.exists(out) else 'FAIL'} mode={mode} md={md} nfiles={len(files)}")
            if not os.path.exists(out):
                print("  cmd:", " ".join(cmd))
                print("  err:", (r.stdout + r.stderr)[-500:])

if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "both")
