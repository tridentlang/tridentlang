import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(os.environ.get("ROOT", ".")).resolve()
BIN = os.environ.get("BIN", "./trident")
DIR = os.environ.get("DIR", "test/golden")
FILTER = os.environ.get("FILTER", "")
UPDATE = bool(os.environ.get("UPDATE"))
TIMEOUT = float(os.environ.get("TIMEOUT", "10"))
JOBS = int(os.environ.get("JOBS", os.cpu_count() or 1))

USE_COLOR = sys.stdout.isatty() and "NO_COLOR" not in os.environ


def paint(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if USE_COLOR else text


def red(t):
    return paint("1;31", t)


def green(t):
    return paint("1;32", t)


def yellow(t):
    return paint("1;33", t)


def dim(t):
    return paint("2", t)


def bold(t):
    return paint("1", t)


ANSI = re.compile(r"\x1b\[[0-9;]*m")


def run(src: Path) -> str:
    try:
        p = subprocess.run(
            [BIN, str(src.relative_to(ROOT))],
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=TIMEOUT,
            check=False,
        )
        out = p.stdout.decode("utf-8", errors="replace")
        out += f"exit: {p.returncode}\n"
    except subprocess.TimeoutExpired as e:
        partial = (e.stdout or b"").decode("utf-8", errors="replace")
        out = partial + f"\n<timed out after {TIMEOUT:g}s>\n"
    return ANSI.sub("", out)


def trunc(s: str, n: int = 110) -> str:
    return s if len(s) <= n else s[: n - 1] + "…"


def failure_report(expected: str, actual: str) -> list[str]:
    exp = expected.split("\n")
    act = actual.split("\n")
    n = max(len(exp), len(act))
    first = next(
        (i for i in range(n) if i >= len(exp) or i >= len(act) or exp[i] != act[i]),
        None,
    )
    if first is None:
        return [f"        {dim('differs only in trailing newline/whitespace')}"]

    e = "<end of expected>" if first >= len(exp) else exp[first]
    a = "<end of output>" if first >= len(act) else act[first]
    nexp = expected.count("\n")
    nact = actual.count("\n")
    return [
        f"        {dim(f'first difference at line {first + 1}')}  "
        f"{dim(f'(expected {nexp} lines, got {nact})')}",
        f"        {green('expected')} | {trunc(e)}",
        f"        {red('actual  ')} | {trunc(a)}",
    ]


def check(src: Path):
    name = src.relative_to(ROOT / DIR).with_suffix("").as_posix()
    exp_path = src.with_suffix(".expect")
    actual = run(src)

    if UPDATE:
        exp_path.write_text(actual, encoding="utf-8")
        return name, "updated", []

    if not exp_path.exists():
        return name, "missing", []

    expected = exp_path.read_text(encoding="utf-8")
    if expected == actual:
        return name, "ok", []
    return name, "fail", failure_report(expected, actual)


def main() -> int:
    base = ROOT / DIR
    sources = sorted(p for p in base.rglob("*.tri") if FILTER in p.name)

    print(bold("snapshot tests"))
    if not sources:
        print(dim(f"  no tests found in {DIR}"))
        return 0

    counts = {"ok": 0, "fail": 0, "missing": 0, "updated": 0}

    with ThreadPoolExecutor(max_workers=JOBS) as pool:
        for name, status, detail in pool.map(check, sources):
            counts[status] += 1
            if status == "ok":
                print(f"  {green('ok')}       {name}")
            elif status == "updated":
                print(f"  {yellow('updated')}  {name}")
            elif status == "missing":
                print(f"  {yellow('MISSING')}  {name}  {dim('(run with UPDATE=1)')}")
            else:
                print(f"  {red('FAIL')}     {name}")
                print("\n".join(detail))

    if UPDATE:
        print(bold(f"{counts['updated']} updated"))
        return 0

    total = len(sources)
    bad = counts["fail"] + counts["missing"]
    if bad == 0:
        print(green(f"{counts['ok']}/{total} passed"))
        return 0

    print(
        f"{bold(f'{counts['ok']}/{total} passed')}, "
        f"{red(f'{counts['fail']} failed')}, "
        f"{yellow(f'{counts['missing']} missing')}"
    )
    print(dim("accept intended changes with: make snapshot-update"))
    return 1


if __name__ == "__main__":
    sys.exit(main())
