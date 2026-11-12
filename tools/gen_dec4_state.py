#!/usr/bin/env python3
"""Derive a typed state description from a core-4 arena address header.

The 0.3/0.4 core-4 decoders keep their whole model state in a flat 32-bit
address arena (`c->mem + 0x45BA3C` and friends).  A typed port needs the
same state as ordinary struct fields.  This tool reads the address
header, works out for every cell

  * its size (the distance to the next cell in the same arena),
  * the widths it is accessed with in the port source,

and emits a C header with

  * one struct member per cell,
  * accessor macros that keep the original G8/G16/G32/M8/M16/M32 spelling
    so the ported code can be moved over mechanically.

Cells that are accessed at more than one width (the original aliases
bytes inside dwords) are emitted as byte arrays with explicit width
accessors; everything else gets a plain typed field.

usage: gen_dec4_state.py <addr-header.h> <source.c> <out.h> [prefix]
"""
import re
import sys
from collections import Counter

HDR = re.compile(r'#define\s+(\w+)\s+(0x[0-9A-Fa-f]+)UL')
USE = re.compile(r'\b(G|M)(8|16|32)\s*\(\s*(\w+)\s*\)')


def parse_header(path):
    defs = {}
    for line in open(path, encoding="latin-1"):
        m = HDR.match(line)
        if m:
            defs[m.group(1)] = int(m.group(2), 16)
    return defs


def parse_uses(path, defs=None, regions=None):
    """address -> Counter of access widths, and the set of names used."""
    widths = {}
    dynamic = set()
    src = open(path, encoding="latin-1").read()
    for m in USE.finditer(src):
        kind, width, name = m.group(1), int(m.group(2)), m.group(3)
        widths.setdefault(name, Counter())[(kind, width)] += 1
    # accesses of the form M32(D4V_X + 2*i) are dynamic: the cell is used
    # as an array, so its size matters more than its width.
    for m in re.finditer(r'\b[GM](?:8|16|32)\s*\(\s*(\w+)\s*[+\-]', src):
        dynamic.add(m.group(1))
    if defs:
        # The typed port spells an access two more ways: the accessors the
        # generator itself emits (D4V_X_32(&c->st, off)) and the
        # byte-wise helpers that stand in for the arena's macros
        # (d3u_ld32(c, D4V_X)).  A cell the arena port never names can
        # still be used by the typed port, so both spellings count.
        #
        # Only the cells inside a layout region are taken from the typed
        # port: there the accessor's byte array and the port's own
        # storage are the same bytes.  A cell in the image keeps the
        # arena port's widths, because its bytes are the image copy in
        # the arena buffer and not the state's member.
        for name in sorted(defs, key=len, reverse=True):
            a = defs[name]
            if not any(b <= a < e for b, e in regions):
                continue
            for w in (8, 16, 32):
                if re.search(r'\b%s_%d\s*\(\s*&c->st' % (re.escape(name), w), src):
                    widths.setdefault(name, Counter())[("t", w)] += 1
            # the byte-wise helpers spell the cell as an absolute address,
            # and the width is the one in the helper's name
            for m2 in re.finditer(r'\bd[34][a-z]*u_(?:ld|st)(8|16|32)\s*\(\s*c\s*,\s*'
                                  r'\(unsigned long\)\(\s*%s\b' % re.escape(name), src):
                widths.setdefault(name, Counter())[("t", int(m2.group(1)))] += 1
    return widths, dynamic


def parse_layout(path):
    """The layout file lists the fixed-region ranges the port addresses
    as arrays: one "0xBASE 0xEND" pair per line.  Everything inside a
    range becomes a byte array of the typed state, and the named cells
    in it become indices into that array."""
    regions = []
    if not path:
        return regions
    for line in open(path, encoding="latin-1"):
        m = re.match(r"\s*(0x[0-9A-Fa-f]+)\s+(0x[0-9A-Fa-f]+)", line)
        if m:
            b = int(m.group(1), 16)
            e = int(m.group(2), 16)
            if e > b:
                regions.append((b, e))
    regions.sort()
    return regions


def main():
    hdr_path, src_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
    prefix = sys.argv[4] if len(sys.argv) > 4 else "d4v"
    layout_path = sys.argv[5] if len(sys.argv) > 5 else None
    extra_path = sys.argv[6] if len(sys.argv) > 6 else None
    regions = parse_layout(layout_path)

    defs = parse_header(hdr_path)
    widths, dynamic = parse_uses(src_path, defs, regions)
    if extra_path:
        # The typed port reaches cells the arena port never names, so its
        # uses are merged in: a cell only the typed port uses still needs
        # an accessor.
        w2, d2 = parse_uses(extra_path, defs, regions)
        for name, c in w2.items():
            widths.setdefault(name, Counter()).update(c)
        dynamic |= d2
    used = {n for n in widths if n in defs}

    # size of each cell: the distance to the next higher used address
    addrs = sorted({defs[n] for n in used})
    size = {}
    for i, a in enumerate(addrs):
        nxt = addrs[i + 1] if i + 1 < len(addrs) else a + 4
        size[a] = max(4, nxt - a)

    lines = []
    lines.append("/* Generated by tools/gen_dec4_state.py - do not hand-edit.")
    lines.append(" * Source: %s" % hdr_path)
    lines.append(" *")
    lines.append(" * Typed state for the %s core-4 port: one member per arena" % prefix)
    lines.append(" * cell, sized from the original address map.  The accessor")
    lines.append(" * macros keep the arena spelling so the ported code reads")
    lines.append(" * the same as the original.")
    lines.append(" */")
    lines.append("#ifndef %s_STATE_H" % prefix.upper())
    lines.append("#define %s_STATE_H" % prefix.upper())
    lines.append("")
    lines.append("typedef struct %s_state {" % prefix)
    lines.append("    unsigned char *mem;")
    raws = []
    aliases = {}
    for b, e in regions:
        lines.append("    unsigned char fx_%06X[%d];   /* 0x%06X..0x%06X */"
                     % (b, e - b, b, e))
    for name in sorted(used, key=lambda n: defs[n]):
        a = defs[name]
        ws = sorted(set(w for (_k, w) in widths[name]))
        reg = None
        for b, e in regions:
            if b <= a < e:
                reg = (b, e)
                break
        if reg is not None:
            # the cell lives inside a byte array: it is an index into it
            aliases[name] = (reg[0], a - reg[0], name in dynamic)
            continue
        if len(ws) > 1 or name in dynamic:
            raws.append(name)
            lines.append("    unsigned char %s_raw[%d];   /* 0x%06X, %s */"
                         % (name.lower(), size[a], a,
                            "+".join(str(w) for w in ws) + " bit"))
        else:
            ctype = {8: "unsigned char", 16: "unsigned short",
                     32: "unsigned int"}[ws[0]]
            lines.append("    %s %s;   /* 0x%06X */" % (ctype, name.lower(), a))
    lines.append("} %s_state;" % prefix)
    lines.append("")
    lines.append("/* Arena-spelling accessors over the typed state.  The")
    lines.append(" * multi-width cells are byte arrays, so the width is")
    lines.append(" * checked at the access site instead of being assumed. */")
    for name in sorted(used, key=lambda n: defs[n]):
        if name in aliases:
            base, off, isdyn = aliases[name]
            arr = "((unsigned char *)(s)->fx_%06X + %dUL)" % (base, off)
            ws = sorted(set(w for (_k, w) in widths[name]))
            for w in ws:
                ctype = {8: "unsigned char", 16: "unsigned short",
                         32: "unsigned int"}[w]
                if True:
                    lines.append("#define %s_%d(s, a) (*(%s *)(void *)(%s + (a)))"
                                 % (name, w, ctype, arr))
                else:
                    lines.append("#define %s_%d(s) (*(%s *)(void *)(%s))"
                                 % (name, w, ctype, arr))
            continue
        if name in raws:
            base = "(s)->%s_raw" % name.lower()
            lines.append("#define %s_8(s, a)  ((unsigned char *)%s)[(a)]" % (name, base))
            lines.append("#define %s_16(s, a) (*(unsigned short *)(void *)((unsigned char *)%s + (a)))"
                         % (name, base))
            lines.append("#define %s_32(s, a) (*(unsigned int *)(void *)((unsigned char *)%s + (a)))"
                         % (name, base))
        else:
            w = sorted(set(w for (_k, w) in widths[name]))[0]
            ctype = {8: "unsigned char", 16: "unsigned short",
                     32: "unsigned int"}[w]
            # the field keeps the dominant width; the narrower views are
            # read-only narrowings (the original never writes them).
            lines.append("#define %s_%d(s) ((s)->%s)" % (name, w, name.lower()))
            for w2 in (8, 16, 32):
                if w2 == w:
                    continue
                lines.append("#define %s_%d(s) ((%s)(s)->%s)"
                             % (name, w2, ctype, name.lower()))
    lines.append("")
    lines.append("#endif")
    lines.append("")

    open(out_path, "w", encoding="latin-1").write("\n".join(lines))
    multi = [n for n in used if len(set(w for (_k, w) in widths[n])) > 1]
    print("%s: %d cells used, %d multi-width, %d dynamic"
          % (prefix, len(used), len(multi), len(dynamic)))
    for n in sorted(multi)[:12]:
        print("   multi-width %-26s widths=%s"
              % (n, sorted(set(w for (_k, w) in widths[n]))))
    print("   wrote", out_path)


if __name__ == "__main__":
    main()
