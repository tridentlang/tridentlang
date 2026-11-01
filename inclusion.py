#!/usr/bin/env python3
"""Check the Code Guide include rules for every .c/.h file under src/.

Definitions
  public subsystem    directory D containing D/<D's name>.h (that header is its API)
  public header       that matching header
  unit                the outermost public directory containing a file, otherwise
                      the file's own directory
  internal subsystem  a unit that is not public; usable only by its parent module's
                      own files and by files under that module (its siblings)

Rules checked
  1  Only a subsystem's public header may be included from outside it.
  2  Internal subsystems may be included only from under their parent module.
  3  The include graph between units must be acyclic (opt-in: --check-cycles).
  4  Public headers may include only public headers.

Suppressing a violation
  Put a comment with a reason on the line above the include (or at the end of it):

      // violation: bootstrap shim, remove once the new arena API lands
      #include "core/mem/arena_priv.h"

  The reason is required. A bare "// violation:" does not suppress anything.
  For rule 3 (cycles) the comment goes on the include that closes the cycle,
  which is the line the report points at.

Usage
  python3 check_includes.py [--root src] [--warn] [--show-suppressed] [--check-cycles] [--color auto|always|never]
  --warn             report violations but always exit 0 (the guide is a standard, not a gate)
  --show-suppressed  also list suppressed violations with their reasons
"""

import argparse
import os
import re
import sys
from pathlib import Path

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
VIOLATION_RE = re.compile(r"//\s*violation:\s*(.*)$")
REPORT_RE = re.compile(r"^(?P<loc>.+?:\d+): (?P<rule>rule \d+ violated): (?P<msg>.*)$")
EXTS = {".c", ".h"}

RESET, BOLD, DIM = "\033[0m", "\033[1m", "\033[2m"
RED, GREEN, YELLOW, CYAN = "\033[31m", "\033[32m", "\033[33m", "\033[36m"


def make_painter(enabled: bool):
    def paint(text: str, *styles: str) -> str:
        return f"{''.join(styles)}{text}{RESET}" if enabled else text

    return paint


def format_report(text: str, paint, suppressed: bool = False) -> str:
    """Color one report line: location, rule name, and quoted paths/names."""
    m = REPORT_RE.match(text)
    if not m:
        return text
    msg = re.sub(r'"[^"]*"', lambda q: paint(q.group(0), YELLOW), m["msg"])
    msg = re.sub(
        r"\[suppression comment has no reason\]",
        lambda q: paint(q.group(0), BOLD, RED),
        msg,
    )
    rule_style = (DIM, GREEN) if suppressed else (BOLD, RED)
    return f"{paint(m['loc'], BOLD, CYAN)}: {paint(m['rule'], *rule_style)}: {msg}"


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    ap.add_argument("--root", default="src", help="source root (default: src)")
    ap.add_argument("--warn", action="store_true", help="always exit 0")
    ap.add_argument(
        "--check-cycles",
        action="store_true",
        help="also report include cycles between subsystems (rule 3, off by default)",
    )
    ap.add_argument(
        "--show-suppressed",
        action="store_true",
        help="list suppressed violations with their reasons",
    )
    ap.add_argument(
        "--color",
        choices=["auto", "always", "never"],
        default="auto",
        help="colored output (default: auto, off when piped or NO_COLOR is set)",
    )
    args = ap.parse_args()

    use_color = args.color == "always" or (
        args.color == "auto" and sys.stdout.isatty() and not os.environ.get("NO_COLOR")
    )
    paint = make_painter(use_color)

    root = Path(args.root).resolve()
    if not root.is_dir():
        print(f"error: {args.root} is not a directory", file=sys.stderr)
        return 2

    files = sorted(p for p in root.rglob("*") if p.is_file() and p.suffix in EXTS)

    # directories that expose a public header
    public_dirs = {
        d
        for d in {p.parent for p in files}
        if d != root and (d / f"{d.name}.h").is_file()
    }
    # a public dir may also be an ancestor of files in nested subdirectories
    for p in files:
        for d in p.parents:
            if d == root:
                break
            if (d / f"{d.name}.h").is_file():
                public_dirs.add(d)

    def is_public_header(p: Path) -> bool:
        return p.parent in public_dirs and p.name == f"{p.parent.name}.h"

    def unit_of(p: Path) -> Path:
        unit = p.parent
        for d in p.parents:
            if d == root:
                break
            if d in public_dirs:
                unit = d  # keep climbing: outermost public ancestor wins
        return unit

    def resolve(src: Path, inc: str):
        for base in (root, src.parent):
            cand = (base / inc).resolve()
            if cand.is_file() and (cand == root or root in cand.parents):
                return cand
        return None  # system or external header

    def rel(p: Path) -> str:
        return (Path(args.root) / p.relative_to(root)).as_posix()

    def unit_name(u: Path) -> str:
        return "." if u == root else u.relative_to(root).as_posix()

    violations = []
    suppressed = []
    edges = {}  # unit -> {unit: (file, line)}
    source_lines = {p: p.read_text(errors="replace").splitlines() for p in files}

    def suppression(src, lineno):
        """Reason text from a `// violation:` comment on this line, or on a
        comment-only line directly above. None if there is no such comment."""
        lines = source_lines[src]
        same = lines[lineno - 1]
        m = VIOLATION_RE.search(same)
        if m:
            return m.group(1).strip()
        if lineno >= 2:
            above = lines[lineno - 2]
            if above.lstrip().startswith("//"):
                m = VIOLATION_RE.search(above)
                if m:
                    return m.group(1).strip()
        return None

    def report(src, line, rule, msg):
        text = f"{rel(src)}:{line}: rule {rule} violated: {msg}"
        reason = suppression(src, line)
        if reason is None:
            violations.append(text)
        elif not reason:
            violations.append(text + "  [suppression comment has no reason]")
        else:
            suppressed.append(f"{text}\n    reason: {reason}")

    for src in files:
        u_s = unit_of(src)
        in_block_comment = False
        for lineno, text in enumerate(source_lines[src], 1):
            # skip block comments (simple handling, good enough for includes)
            if in_block_comment:
                if "*/" in text:
                    in_block_comment = False
                continue
            if text.lstrip().startswith("/*") and "*/" not in text:
                in_block_comment = True
                continue

            m = INCLUDE_RE.match(text)
            if not m:
                continue
            tgt = resolve(src, m.group(1))
            if tgt is None:
                continue

            u_t = unit_of(tgt)
            inc = m.group(1)

            # Rule 4: public headers include only public headers.
            # Exception: data files of the same subsystem (X-macro lists such as
            # token.def, .inc, ...) are part of the header's own implementation.
            own_data_file = tgt.suffix not in EXTS and u_s == u_t
            if (
                is_public_header(src)
                and not is_public_header(tgt)
                and not own_data_file
            ):
                report(src, lineno, 4, f'public header includes non-public "{inc}"')

            if u_s == u_t:
                continue

            edges.setdefault(u_s, {}).setdefault(u_t, (src, lineno))

            if is_public_header(tgt):
                continue

            if u_t in public_dirs:
                # Rule 1: reaching past the public header
                report(
                    src,
                    lineno,
                    1,
                    f'includes "{inc}", which is not the public header of '
                    f"{unit_name(u_t)} ({u_t.name}/{u_t.name}.h)",
                )
            else:
                # Rule 2: internal subsystem, siblings only
                module = u_t.parent
                # the module's own files, and anything under the module (siblings)
                allowed = u_s == module or module in u_s.parents
                if not allowed:
                    report(
                        src,
                        lineno,
                        2,
                        f'includes "{inc}" (internal to module {unit_name(module)})',
                    )

    # Rule 3: cycles between units (DFS with a path stack)
    seen_cycles = set()
    color = {}

    def dfs(u, path):
        color[u] = 1
        path.append(u)
        for v in edges.get(u, {}):
            if color.get(v, 0) == 0:
                dfs(v, path)
            elif color.get(v) == 1:
                cyc = path[path.index(v) :]
                key = frozenset(cyc)
                if key not in seen_cycles:
                    seen_cycles.add(key)
                    f, ln = edges[u][v]
                    chain = " -> ".join(unit_name(x) for x in cyc + [v])
                    report(f, ln, 3, f"include cycle between subsystems: {chain}")
        path.pop()
        color[u] = 2

    if args.check_cycles:
        for u in list(edges):
            if color.get(u, 0) == 0:
                dfs(u, [])

    for v in violations:
        print(format_report(v, paint))
    if args.show_suppressed:
        for s in suppressed:
            report_line, _, reason = s.partition("\n    reason: ")
            print(
                f"{paint('suppressed', DIM, GREEN)} {format_report(report_line, paint, True)}"
            )
            print(f"    {paint('reason:', DIM)} {paint(reason, GREEN)}")

    n = len(violations)
    verdict = paint(f"{n} violation(s)", BOLD, RED if n else GREEN)
    print(
        f"\n{len(files)} files checked, {verdict}, "
        f"{paint(f'{len(suppressed)} suppressed', DIM)}"
    )

    return 0 if (args.warn or not violations) else 1


if __name__ == "__main__":
    sys.exit(main())
