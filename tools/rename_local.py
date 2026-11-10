#!/usr/bin/env python3
"""Rename a local variable inside one function.

The ports translated from the original disassemblies use x86 register
names and Hex-Rays spill names (v1, v2, ...) for ordinary locals.  This
tool renames one local at a time, inside the function that declares it,
so a name that also occurs elsewhere in the file is left alone.

usage: rename_local.py <file.c> <decl-line> <old-name> <new-name> [...]
"""
import re
import sys

FUNC = re.compile(r'^[A-Za-z_][\w \t\*]*\b(\w+)\s*\([^;]*$')


def function_range(lines, decl_line):
    """(first, last) 1-based line range of the function owning decl_line."""
    start = None
    for i in range(decl_line - 2, -1, -1):
        line = lines[i]
        if line.startswith("}"):
            break
        if line.startswith("{"):
            continue
        m = FUNC.match(line)
        if m and not line.rstrip().endswith(";"):
            start = i
            break
    if start is None:
        raise SystemExit("no function header found above line %d" % decl_line)
    # the closing brace of the function is the first line that starts with
    # "}" at column 0 after the declaration
    for j in range(decl_line - 1, len(lines)):
        if lines[j].startswith("}"):
            return start + 1, j + 1
    raise SystemExit("no function end found below line %d" % decl_line)


def main():
    path = sys.argv[1]
    raw = open(path, encoding="latin-1").read()
    lines = raw.split("\n")
    for i in range(2, len(sys.argv), 3):
        decl = int(sys.argv[i])
        old = sys.argv[i + 1]
        new = sys.argv[i + 2]
        lo, hi = function_range(lines, decl)
        pat = re.compile(r'\b%s\b' % re.escape(old))
        n = 0
        for j in range(lo - 1, hi):
            new_line, k = pat.subn(new, lines[j])
            if k:
                lines[j] = new_line
                n += k
        print("%-28s %-6s -> %-12s lines %d-%d, %d use(s)" % (path, old, new, lo, hi, n))
    open(path, "w", encoding="latin-1").write("\n".join(lines))


if __name__ == "__main__":
    main()
