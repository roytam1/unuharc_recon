#!/usr/bin/env python3
"""Byte-compare two unpackers on one corpus directory.

Extracts every archive with both binaries and compares the concatenated
file bytes.  Password archives (name starts with "pw") use -pwsecret.
Exit status is 0 only when every archive agrees.
"""
import os
import shutil
import subprocess
import sys
import tempfile

PASSWORD_PREFIXES = ("pw",)
PASSWORD = "secret"


def extract(exe, arc, outdir):
    os.makedirs(outdir, exist_ok=True)
    cmd = [exe, "e", arc, outdir]
    if os.path.basename(arc).startswith(PASSWORD_PREFIXES):
        cmd.insert(2, "-pw" + PASSWORD)
    try:
        p = subprocess.run(cmd, capture_output=True, timeout=300)
    except subprocess.TimeoutExpired:
        return "TIMEOUT", b""
    blob = b""
    for root, _dirs, files in os.walk(outdir):
        for name in sorted(files):
            with open(os.path.join(root, name), "rb") as fh:
                blob += fh.read()
    return p.returncode, blob


def main():
    if len(sys.argv) != 4:
        print("usage: diff_corpus.py <exe-a> <exe-b> <arc-dir>")
        return 2
    exe_a, exe_b, arc_dir = sys.argv[1], sys.argv[2], sys.argv[3]
    files = sorted(f for f in os.listdir(arc_dir) if f.lower().endswith(".uha"))
    work = tempfile.mkdtemp(prefix="diffcorp_")
    bad = 0
    try:
        for name in files:
            arc = os.path.join(arc_dir, name)
            ra, oa = extract(exe_a, arc, os.path.join(work, "a"))
            rb, ob = extract(exe_b, arc, os.path.join(work, "b"))
            shutil.rmtree(os.path.join(work, "a"), ignore_errors=True)
            shutil.rmtree(os.path.join(work, "b"), ignore_errors=True)
            if ra != rb or oa != ob:
                bad += 1
                print("DIFF %-24s rc %s/%s bytes %d/%d" % (name, ra, rb, len(oa), len(ob)))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("diff_corpus: %d/%d agree" % (len(files) - bad, len(files)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
