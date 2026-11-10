#!/usr/bin/env python3
"""Regression runner for the UHARC unpacker.

For every archive in a corpus directory:

  * run ``<exe> t <arc>``          - the container's own integrity check
                                     (per-file CRC32, whole-stream CRC)
  * run ``<exe> x <arc> <tmpdir>`` - extract
  * compare every extracted file byte for byte with the reference tree
    (when one is given), and check its size against the archive listing

Usage:
    python tools/run_corpus.py [--exe uharc06_unpack.exe]
                               [--arc-dir uharc-rs/testdata/arc]
                               [--src-dir uharc-rs/testdata/src]
                               [--label NAME] [-v] [--jobs N]

Exit status is 0 only when every archive passes.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

# Archives whose name starts with one of these need a password; the corpus
# in uharc-rs/testdata uses "secret".
PASSWORD_PREFIXES = ("pw",)
PASSWORD = "secret"

LIST_RE = re.compile(r"^\s{2}(\S.*?) \((\d+) bytes\)$")


def parse_listing(text):
    """Return [(path, size)] from the output of the `l` command."""
    entries = []
    for line in text.splitlines():
        m = LIST_RE.match(line)
        if m:
            entries.append((m.group(1).replace("\\", "/"), int(m.group(2))))
    return entries


def run(exe, args, timeout=900):
    return subprocess.run([exe] + args, capture_output=True, timeout=timeout)


def check_one(exe, arc, src_dir, verbose):
    """Return (ok, message) for one archive."""
    name = os.path.basename(arc)
    pw = [name.startswith(p) for p in PASSWORD_PREFIXES]
    pw = pw[0]
    try:
        t = run(exe, ["t"] + (["-pw" + PASSWORD] if pw else []) + [arc])
    except subprocess.TimeoutExpired:
        return False, "timeout in `t`"
    if t.returncode != 0:
        return False, "t: exit %d" % t.returncode
    if b"Completed" not in t.stdout + t.stderr:
        return False, "t: no 'Completed'"

    try:
        l = run(exe, ["l"] + (["-pw" + PASSWORD] if pw else []) + [arc])
    except subprocess.TimeoutExpired:
        return False, "timeout in `l`"
    listing = parse_listing((l.stdout + l.stderr).decode("latin-1"))
    if not listing:
        return False, "l: no file list"

    tmp = tempfile.mkdtemp(prefix="uharc_corpus_")
    try:
        x = run(exe, ["x"] + (["-pw" + PASSWORD] if pw else []) + [arc, tmp])
        if x.returncode != 0:
            return False, "x: exit %d" % x.returncode
        for path, size in listing:
            got = os.path.join(tmp, path.replace("/", os.sep))
            if not os.path.isfile(got):
                return False, "x: missing %s" % path
            if os.path.getsize(got) != size:
                return False, "x: %s size %d != %d" % (path, os.path.getsize(got), size)
            if src_dir:
                ref = os.path.join(src_dir, path.replace("/", os.sep))
                if not os.path.isfile(ref):
                    return False, "no reference for %s" % path
                with open(got, "rb") as fh:
                    data = fh.read()
                with open(ref, "rb") as fh:
                    if fh.read() != data:
                        return False, "content differs: %s" % path
        return True, "%d files" % len(listing)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", default=os.path.join(root, "uharc06_unpack.exe"),
                    help="unpacker to test (default: %(default)s)")
    ap.add_argument("--arc-dir", default=os.path.join(root, "uharc-rs", "testdata", "arc"),
                    help="directory with .uha archives (default: %(default)s)")
    ap.add_argument("--src-dir", default=os.path.join(root, "uharc-rs", "testdata", "src"),
                    help="reference file tree (default: %(default)s)")
    ap.add_argument("--no-src", action="store_true",
                    help="skip the byte-for-byte comparison with --src-dir")
    ap.add_argument("--label", default="", help="name used in the report")
    ap.add_argument("--jobs", type=int, default=4, help="parallel archives (default: 4)")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    if not os.path.isdir(args.arc_dir):
        print("no such archive directory: %s" % args.arc_dir)
        return 2
    if not os.path.isfile(args.exe):
        print("no such executable: %s" % args.exe)
        return 2
    src_dir = None if args.no_src else args.src_dir
    if src_dir and not os.path.isdir(src_dir):
        print("no such reference directory: %s (use --no-src)" % src_dir)
        return 2

    arcs = sorted(f for f in os.listdir(args.arc_dir) if f.lower().endswith(".uha"))
    label = args.label or os.path.basename(args.exe)
    print("%s: %d archives" % (label, len(arcs)))

    results = {}
    with ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        futures = {pool.submit(check_one, args.exe, os.path.join(args.arc_dir, f), src_dir,
                               args.verbose): f for f in arcs}
        for fut in futures:
            pass
        for fut, f in futures.items():
            try:
                results[f] = fut.result()
            except Exception as exc:  # noqa: BLE001
                results[f] = (False, "exception: %s" % exc)

    bad = 0
    for f in arcs:
        ok, msg = results[f]
        if not ok:
            bad += 1
            print("FAIL %-24s %s" % (f, msg))
        elif args.verbose:
            print("ok   %-24s %s" % (f, msg))
    print("%s: %d/%d passed%s" % (label, len(arcs) - bad, len(arcs),
                                  "" if src_dir else "  (contents not compared)"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
