#!/usr/bin/env python3
"""Classify d4v_ld/st/ptr/fx call sites in uha04_dec4_clean.c.

Static only: a constant or D4V_* address is placed in io, globals, model
or gap.  Anything else is dynamic.  A constant access whose bytes cross
a region edge is reported as straddling.  The runtime bounds come from
a -DD4V_AUDIT build (see d4v_audit_dump).
"""
import re
import sys

IO_END = 0x40000
GLOB_LO = 0x42E000
GLOB_HI = 0x45D0C8
MODEL_LO = 0x600000
MODEL_HI = 0x600000 + 0x4000000
FIXED_BASE = 0x420000

SRC = sys.argv[1] if len(sys.argv) > 1 else "uha04_dec4_clean.c"


def region(addr, nbytes):
    def one(a):
        if a < IO_END:
            return "io"
        if GLOB_LO <= a < GLOB_HI:
            return "globals"
        if MODEL_LO <= a < MODEL_HI:
            return "model"
        return "gap"
    first = one(addr)
    last = one(addr + nbytes - 1)
    if first != last:
        return "straddle"
    return first


def parse_args(text, i):
    """Return the argument list starting at text[i] == '('."""
    depth = 0
    start = i
    args = []
    cur = []
    j = i
    while j < len(text):
        ch = text[j]
        if ch == "(":
            depth += 1
            if depth > 1:
                cur.append(ch)
        elif ch == ")":
            depth -= 1
            if depth == 0:
                args.append("".join(cur).strip())
                return args, j + 1
            cur.append(ch)
        elif ch == "," and depth == 1:
            args.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
        j += 1
    return args, j


def const_value(expr, names):
    expr = expr.strip()
    expr = re.sub(r'^\(uint32_t\)', '', expr).strip()
    expr = re.sub(r'^\((?:unsigned(?: int| long)?)\)', '', expr).strip()
    if expr.startswith("(") and expr.endswith(")"):
        inner = expr[1:-1].strip()
        if inner.count("(") == inner.count(")"):
            expr = inner
    m = re.fullmatch(r'0x[0-9A-Fa-f]+u?', expr)
    if m:
        return int(expr.rstrip("u"), 16)
    m = re.fullmatch(r'\d+u?', expr)
    if m:
        return int(expr.rstrip("u"))
    m = re.fullmatch(r'(D4V_[A-Z0-9_]+)', expr)
    if m and m.group(1) in names:
        return names[m.group(1)]
    m = re.fullmatch(r'(D4V_[A-Z0-9_]+)\s*\+\s*(0x[0-9A-Fa-f]+|\d+)u?', expr)
    if m and m.group(1) in names:
        return names[m.group(1)] + int(m.group(2).rstrip("u"), 0)
    return None


def main():
    src = open(SRC, encoding="latin-1").read()
    names = {}
    addr_h = open("uha04_dec4_addr.h", encoding="latin-1").read()
    for m in re.finditer(r'#define\s+(D4V_[A-Z0-9_]+)\s+\(\(uint32_t\)(0x[0-9A-Fa-f]+)u\)', addr_h):
        names[m.group(1)] = int(m.group(2), 16)

    call = re.compile(r'\bd4v_(ld8|ld16|ld32|st8|st16|st32|ptr|fx|fill|fill16)\s*\(')
    width = {"ld8": 1, "st8": 1, "ld16": 2, "st16": 2, "ld32": 4, "st32": 4,
             "ptr": 1, "fx": 1, "fill": 1, "fill16": 2}
    buckets = {}
    dynamic = []
    i = 0
    while True:
        m = call.search(src, i)
        if not m:
            break
        kind = m.group(1)
        args, nxt = parse_args(src, m.end() - 1)
        i = nxt
        line = src.count("\n", 0, m.start()) + 1
        # Definitions and comments are not calls.  A call's first
        # argument is the context `c`.
        if not args or args[0] != "c":
            continue
        if kind in ("ld8", "ld16", "ld32", "st8", "st16", "st32", "ptr", "fx"):
            if len(args) < 2:
                dynamic.append((line, kind, "?", "missing-args"))
                continue
            expr = args[1]
            if kind == "fx":
                off = const_value(expr, names)
                addr = None if off is None else (FIXED_BASE + off) & 0xFFFFFFFF
            else:
                addr = const_value(expr, names)
            if addr is None:
                dynamic.append((line, kind, expr[:80], "dynamic"))
                key = "dynamic"
            else:
                key = region(addr, width[kind])
            buckets[key] = buckets.get(key, 0) + 1
        else:
            buckets["fill"] = buckets.get("fill", 0) + 1

    print("static classification of %s" % SRC)
    for key in ("globals", "model", "io", "gap", "straddle", "dynamic", "fill"):
        print("  %-10s %d" % (key, buckets.get(key, 0)))
    print("dynamic samples (first 25):")
    for line, kind, expr, _why in dynamic[:25]:
        print("  %s:%d %s %s" % (SRC, line, kind, expr.replace("\n", " ")))
    print("dynamic total", len(dynamic))


if __name__ == "__main__":
    main()
