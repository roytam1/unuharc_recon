#!/usr/bin/env python3
"""Check the const tables that the core-4 ports lifted out of the DGROUP
image against the image itself.

The 0.4 port lifts read-only tables out of the loaded DGROUP image into
``static const`` arrays, and the 0.3 port does the same with the slice it
loads at 0x428000.  The lift has to be byte-exact: the array literal in
``uha04_dec4_dg.c`` sits behind a comment that contains the address
``0x0042E000``, so a scan that does not parse the array *body* shifts
every table by one byte - and a shifted table still decodes most
archives, so the corpus alone does not catch it.

This tool parses the array body of the image and the array bodies of the
lifted tables and compares them byte for byte.  It also checks that the
declared element count matches the size the comment claims.

Usage:
    python tools/check_lifted_tables.py            # both ports
    python tools/check_lifted_tables.py --port uha04_dec4_clean.c \\
        --image uha04_dec4_dg.c --base 0x42E000
"""

import argparse
import os
import re
import sys

ARRAY_RE = re.compile(r"\{\s*(.*?)\s*\}", re.S)
HEX_RE = re.compile(r"0[xX][0-9a-fA-F]+")

# (label, port, image, base)
PORTS = [
    ("0.4", "uha04_dec4_clean.c", "uha04_dec4_dg.c", 0x42E000),
    ("0.3", "uha03_dec4_clean.c", "uha03_dec4_dg.c", 0x428000),
]


def parse_body(text):
    """Return the list of integer literals in the first {...} of text."""
    m = ARRAY_RE.search(text)
    if not m:
        return None
    body = m.group(1)
    # drop comments: the image's header comment holds an address that must
    # not be mistaken for data
    body = re.sub(r"/\*.*?\*/", " ", body, flags=re.S)
    body = re.sub(r"//[^\n]*", " ", body)
    out = []
    for tok in body.split(","):
        tok = tok.strip()
        if not tok:
            continue
        if not HEX_RE.fullmatch(tok):
            return None
        out.append(int(tok, 16))
    return out


def load_image(path):
    with open(path, "r", encoding="latin-1") as fh:
        text = fh.read()
    m = re.search(r"const\s+unsigned\s+char\s+(\w+)\s*\[\s*(\d+)\s*\]\s*=", text)
    if not m:
        raise SystemExit("no image array in %s" % path)
    data = parse_body(text[m.end():])
    if data is None:
        raise SystemExit("cannot parse the image body in %s" % path)
    if len(data) != int(m.group(2)):
        raise SystemExit("image body has %d bytes, header says %s"
                         % (len(data), m.group(2)))
    return data


def find_tables(path):
    """Return [(name, address, count, values, unit)] for the lifted tables."""
    with open(path, "r", encoding="latin-1") as fh:
        text = fh.read()
    out = []
    pat = re.compile(
        r"/\*.*?Const table lifted from (?:the )?DGROUP (?:image|slice)\s+(0[xX][0-9a-fA-F]+)"
        r"\s*\((\d+)\s*(u32|byte).*?\*/\s*"
        r"static\s+const\s+unsigned\s+(int|char)\s+(\w+)\s*\[\s*(\d+)\s*\]\s*=",
        re.S)
    for m in pat.finditer(text):
        addr = int(m.group(1), 16)
        count = int(m.group(2))
        unit = m.group(3)
        ctype = m.group(4)
        name = m.group(5)
        decl = int(m.group(6))
        values = parse_body(text[m.end():])
        if values is None:
            raise SystemExit("cannot parse the body of %s" % name)
        width = 4 if ctype == "int" else 1
        if unit == "u32" and width != 4:
            raise SystemExit("%s: comment says u32 but the type is %s"
                             % (name, ctype))
        if unit == "byte" and width != 1:
            raise SystemExit("%s: comment says byte but the type is %s"
                             % (name, ctype))
        if decl != count:
            raise SystemExit("%s: header says %d entries, array has %d"
                             % (name, count, decl))
        if len(values) != decl:
            raise SystemExit("%s: array body has %d values, header says %d"
                             % (name, len(values), decl))
        out.append((name, addr, count, values, unit))
    return out


def check(label, port, image, base):
    img = load_image(image)
    tables = find_tables(port)
    if not tables:
        print("%s: no lifted tables found in %s" % (label, port))
        return 2
    bad = 0
    for name, addr, count, values, unit in sorted(tables, key=lambda t: t[1]):
        off = addr - base
        width = 4 if unit == "u32" else 1
        if off < 0 or off + count * width > len(img):
            print("FAIL %s %-16s 0x%08X is outside the image" % (label, name, addr))
            bad += 1
            continue
        want = []
        for v in values:
            if width == 4:
                want.extend([v & 0xFF, (v >> 8) & 0xFF,
                             (v >> 16) & 0xFF, (v >> 24) & 0xFF])
            else:
                want.append(v & 0xFF)
        got = img[off:off + len(want)]
        if got != want:
            first = next(i for i in range(min(len(got), len(want)))
                         if got[i] != want[i])
            print("FAIL %s %-16s 0x%08X differs at byte %d (image %02X, table %02X)"
                  % (label, name, addr, first, got[first], want[first]))
            bad += 1
        else:
            print("ok   %s %-16s 0x%08X %d %s" % (label, name, addr, count, unit))
    print("%s: %d/%d lifted tables match the image" % (label, len(tables) - bad, len(tables)))
    return 1 if bad else 0


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", help="port file to check (default: both ports)")
    ap.add_argument("--image", help="image file the port lifts from")
    ap.add_argument("--base", help="image base address (default: per port)")
    args = ap.parse_args()

    if args.port:
        port = args.port if os.path.isabs(args.port) else os.path.join(root, args.port)
        image = args.image or ""
        if not image:
            for _l, _p, _i, _b in PORTS:
                if _p == args.port:
                    image = os.path.join(root, _i)
                    break
        if not image:
            raise SystemExit("--image is required with --port")
        base = int(args.base, 16) if args.base else None
        if base is None:
            for _l, _p, _i, _b in PORTS:
                if _p == args.port:
                    base = _b
                    break
        if base is None:
            raise SystemExit("--base is required with --port")
        return check("custom", port, image, base)

    bad = 0
    for label, p, i, b in PORTS:
        bad |= check(label, os.path.join(root, p), os.path.join(root, i), b)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
