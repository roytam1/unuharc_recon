#!/usr/bin/env python3
"""Differential fuzz: a typed core-4 port against its arena reference.

The typed ports are generated from the arena ports, so they must agree
byte for byte - not only on the corpus but on damaged input as well.
This mutates the archives of a corpus (bit flips, bursts, truncations)
and compares what the two binaries extract.

usage: diff_fuzz.py <corpus-dir> <typed-exe> <arena-exe> [rounds]
"""
import os
import random
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
        return ("TIMEOUT", b"")
    blob = b""
    for root, _dirs, files in os.walk(outdir):
        for f in sorted(files):
            with open(os.path.join(root, f), "rb") as fh:
                blob += fh.read()
    return (p.returncode, blob)


def main():
    corpus, typed, arena = sys.argv[1], sys.argv[2], sys.argv[3]
    rounds = int(sys.argv[4]) if len(sys.argv) > 4 else 2
    rnd = random.Random(0xC0FFEE)
    files = sorted(f for f in os.listdir(corpus) if f.endswith(".uha"))
    work = tempfile.mkdtemp(prefix="difffuzz_")
    trials = diffs = crashes = timeouts = 0
    try:
        for _r in range(rounds):
            for f in files:
                data = bytearray(open(os.path.join(corpus, f), "rb").read())
                if not data:
                    continue
                kind = rnd.randrange(3)
                if kind == 0:
                    i = rnd.randrange(len(data))
                    data[i] ^= 1 << rnd.randrange(8)
                elif kind == 1:
                    i = rnd.randrange(len(data))
                    for k in range(min(16, len(data) - i)):
                        data[i + k] ^= rnd.randrange(1, 256)
                else:
                    data = data[: rnd.randrange(1, len(data))]
                arc = os.path.join(work, f)
                with open(arc, "wb") as fh:
                    fh.write(bytes(data))
                ra, oa = extract(arena, arc, os.path.join(work, "a"))
                rt, ot = extract(typed, arc, os.path.join(work, "c"))
                shutil.rmtree(os.path.join(work, "a"), ignore_errors=True)
                shutil.rmtree(os.path.join(work, "c"), ignore_errors=True)
                trials += 1
                if ra == "TIMEOUT" or rt == "TIMEOUT":
                    timeouts += 1
                    continue
                if ra not in (0, 1) and ra != rt:
                    crashes += 1
                    continue
                if ra != rt or oa != ot:
                    diffs += 1
                    print("DIFF %s kind=%d arena=(%r,%d) typed=(%r,%d)"
                          % (f, kind, ra, len(oa), rt, len(ot)))
                    if diffs > 8:
                        print("... stopping")
                        return
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("diff fuzz done: %d trials, %d differences, %d arena faults, %d timeouts"
          % (trials, diffs, crashes, timeouts))


if __name__ == "__main__":
    main()
