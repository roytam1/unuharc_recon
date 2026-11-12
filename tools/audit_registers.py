#!/usr/bin/env python3
"""Audit the port sources for x86 register emulation."""
import os
import re
import sys

REGS = r'(?:e?[abcd]x|[abcd][lh]|[sd]i|[sb]p|sp)'
HEXRAYS = r'(?:v\d{1,3}|e_\w+|_\w+|\w+_)'
NAME = r'(?:%s|%s)' % (REGS, HEXRAYS)

TYPE = re.compile(
    r'^\s*(?:const\s+)?(?:unsigned\s+|signed\s+)?'
    r'(?:char|short|int|long|float|double|u?int\d+_t|UHI64|size_t|'
    r'unsigned\s+char|unsigned\s+short|unsigned\s+int|unsigned\s+long)\b')

DECLTOR = re.compile(
    r'\*?\s*\b(?P<name>' + NAME + r')\b\s*(?:\[[^\]]*\])?\s*(?:=\s*(?:\{[^}]*\}|[^,;]+))?')


def audit(path):
    hits = []
    try:
        lines = open(path, encoding="latin-1").read().split("\n")
    except OSError:
        return hits
    for i, line in enumerate(lines):
        s = line.lstrip()
        if s.startswith("*") or s.startswith("/*") or s.startswith("//"):
            continue
        if not TYPE.match(line):
            continue
        if ';' not in line:
            continue
        decl = line.rsplit(';', 1)[0]
        for m in DECLTOR.finditer(decl):
            nm = m.group('name')
            start = m.start('name')
            if start == 0:
                continue
            prev = decl[start - 1]
            if not (prev.isspace() or prev in ',*'):
                continue
            hits.append((i + 1, nm, line.strip()[:70]))
    return hits


def classify(name):
    if re.match(r'^(e?[abcd]x|[abcd][lh]|[sd]i|[sb]p|sp)$', name):
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
