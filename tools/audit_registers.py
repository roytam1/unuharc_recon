#!/usr/bin/env python3
"""Audit the port sources for x86 register emulation.

The ports that were translated from the original disassemblies model the
x86 machine: the register names (eax, ebx, esi, ...) and the Hex-Rays
spills (v1, v2, e_eax, eax_) are used as ordinary variable names.  That
makes the code hard to read and hides what a value actually means.

This tool reports, per file, how many register-named locals it declares
and where, so the naming can be cleaned up.

usage: audit_registers.py [--list] [file.c ...]
"""
import os
import re
import sys

# the register names a port may use as a variable
REGS = r'(?:e?[abcd]x|[abcd][lh]|[sd]i|[sb]p|sp)'
HEXRAYS = r'(?:v\d{1,3}|e_\w+|_\w+|\w+_)'

DECL = re.compile(
    r'^\s*(?:const\s+)?(?:unsigned\s+|signed\s+)?'
    r'(?:char|short|int|long|float|double|u?int\d+_t|UHI64|size_t|'
    r'unsigned\s+char|unsigned\s+short|unsigned\s+int|unsigned\s+long)\s*'
    r'(?:\*\s*)?(%s|%s)\s*(?:\[[^\]]*\])?\s*(?:,[^;]*)?;'
    % (REGS, HEXRAYS))


def audit(path):
    hits = []
    try:
        lines = open(path, encoding="latin-1").read().split("\n")
    except OSError:
        return hits
    for i, line in enumerate(lines):
        if line.lstrip().startswith("*") or line.lstrip().startswith("/*"):
            continue
        m = DECL.match(line)
        if m:
            hits.append((i + 1, m.group(1), line.strip()[:70]))
    return hits


def classify(name):
    """'reg' for an x86 register name, 'spill' for a Hex-Rays temp."""
    if re.match(r'^(e?[abcd]x|[abcd][lh]|[sd]i|[sb]p|sp)(_)?$', name):
        return "reg"
    if re.match(r'^(v\d{1,3}|e_\w+|_\w+|\w+_)$', name):
        return "spill"
    return "other"


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    show = "--list" in sys.argv
    if args:
        files = args
    else:
        files = []
        for f in sorted(os.listdir(".")):
            if f.endswith(".c") and not f.endswith("_dg.c"):
                files.append(f)
    total = 0
    spills = 0
    for f in files:
        hits = audit(f)
        if not hits:
            continue
        kinds = {}
        for _ln, nm, _txt in hits:
            kinds[nm] = kinds.get(nm, 0) + 1
        regs = sorted(n for n in kinds if classify(n) == "reg")
        nspill = sum(kinds[n] for n in kinds if classify(n) == "spill")
        total += len(regs)
        spills += nspill
        print("%-24s %4d locals: %d register-named, %d hex-rays spills"
              % (f, len(hits), len(regs), nspill))
        if regs:
            print("      registers: %s" % " ".join(regs[:20]))
        if show:
            for ln, nm, txt in hits[:40]:
                print("      %5d  %-10s %s" % (ln, nm, txt))
    print("total: %d register-named locals, %d hex-rays spills" % (total, spills))


if __name__ == "__main__":
    main()
