#!/usr/bin/env python3
"""Cross-check <PKG>_DEPENDENCIES vs `select BR2_PACKAGE_<X>` in Config.in."""
import argparse
import os
import re
import sys
from pathlib import Path


REPO = Path(__file__).resolve().parents[2]
BASELINE_FILE = Path(__file__).resolve().parent / "lint-package-deps.baseline"

# Search order: Recalbox overrides win over Buildroot upstream.
CONFIG_SEARCH_ROOTS = [
    REPO / "package",
    REPO / "custom" / "package",
    REPO / "buildroot" / "package",
]

DEP_BLOCKLIST_PREFIX = ("host-",)
DEP_BLOCKLIST_EXACT = {
    "toolchain",
    "toolchain-buildroot",
    "skeleton",
    "skeleton-init-common",
    "skeleton-init-sysv",
    "skeleton-init-systemd",
    "skeleton-init-none",
    "linux",
    "linux-headers",
    "uclibc",
    "glibc",
    "musl",
    "gcc",
}


def _display(path: Path) -> Path:
    try:
        return path.relative_to(REPO)
    except ValueError:
        return path


def kconfig_symbol(dep_name: str) -> str:
    return "BR2_PACKAGE_" + dep_name.upper().replace("-", "_")


def build_symbol_index() -> tuple:
    symbols = {}
    has_symbols = set()
    for root in CONFIG_SEARCH_ROOTS:
        if not root.is_dir():
            continue
        for cfg in root.rglob("Config.in"):
            try:
                text = cfg.read_text(errors="replace")
            except OSError:
                continue
            for m in re.finditer(
                    r"^\s*(?:config|menuconfig)\s+(BR2_PACKAGE_\w+)\s*$",
                    text, flags=re.MULTILINE):
                sym = m.group(1)
                symbols.setdefault(sym, cfg)
                if sym.startswith("BR2_PACKAGE_HAS_"):
                    has_symbols.add(sym[len("BR2_PACKAGE_HAS_"):])

    virtuals = {
        x for x in has_symbols
        if f"BR2_PACKAGE_{x}" not in symbols
    }
    return symbols, virtuals


def extract_deps(mk_path: Path, pkg_var_prefix: str) -> dict:
    """Return {dep_name: set(guard_symbols)}.

    A guard is the BR2_PACKAGE_X mentioned in an `ifeq ($(BR2_PACKAGE_X),y)`
    enclosing the `DEPENDENCIES +=` line. The dep is only added when X is
    selected elsewhere, so Config.in does not need to `select` X itself.
    """
    try:
        text = mk_path.read_text(errors="replace")
    except OSError:
        return {}

    text = re.sub(r"\\\n", " ", text)

    dep_re = re.compile(
        rf"^[ \t]*{re.escape(pkg_var_prefix)}_DEPENDENCIES[ \t]*[+:?]?=[ \t]*([^\n]*)$"
    )
    # Matches `ifeq ($(BR2_PACKAGE_X),y)` and the canonical multi-symbol form
    # `ifeq ($(BR2_PACKAGE_A)$(BR2_PACKAGE_B),yy)` — N symbols concatenated
    # vs. N `y`s. When N matches, every symbol becomes a guard.
    if_re = re.compile(
        r"^[ \t]*(ifeq|ifneq)[ \t]*\([ \t]*"
        r"((?:\$\(BR2_PACKAGE_\w+\))+)[ \t]*,[ \t]*(y+)[ \t]*\)"
    )
    sym_re = re.compile(r"BR2_PACKAGE_\w+")
    else_re = re.compile(r"^[ \t]*else\b")
    endif_re = re.compile(r"^[ \t]*endif\b")

    deps: dict = {}
    stack: list = []  # list of (kind, frozenset(symbols))
    for line in text.splitlines():
        m = if_re.match(line)
        if m:
            syms = sym_re.findall(m.group(2))
            guard_syms = frozenset(syms) if len(syms) == len(m.group(3)) else frozenset()
            stack.append((m.group(1), guard_syms))
            continue
        if else_re.match(line):
            if stack:
                kind, syms = stack[-1]
                stack[-1] = ("ifneq" if kind == "ifeq" else "ifeq", syms)
            continue
        if endif_re.match(line):
            if stack:
                stack.pop()
            continue
        m = dep_re.match(line)
        if not m:
            continue
        guards = {sym for kind, syms in stack if kind == "ifeq" for sym in syms}
        for token in m.group(1).split():
            if token.startswith("$") or token.startswith("#"):
                break
            if token.startswith("-"):
                continue
            deps.setdefault(token, set()).update(guards)
    return deps


def extract_selects_and_main_symbol(cfg_path: Path) -> tuple:
    try:
        text = cfg_path.read_text(errors="replace")
    except OSError:
        return (None, set())

    main = None
    selects = set()
    for line in text.splitlines():
        s = line.strip()
        if main is None:
            m = re.match(r"(?:config|menuconfig)\s+(BR2_PACKAGE_\w+)\s*$", s)
            if m:
                main = m.group(1)
        # Capture every BR2_PACKAGE_X on a select/depends-on line, including
        # symbols in `if COND` clauses or `||`/`&&` expressions.
        if re.match(r"^(?:select|depends on)\s+", s):
            selects.update(re.findall(r"BR2_PACKAGE_\w+", s))
    return main, selects


def derive_pkg_var_prefix(pkg_dir: Path, main_symbol: str | None) -> str:
    if main_symbol:
        return main_symbol.removeprefix("BR2_PACKAGE_")
    return pkg_dir.name.upper().replace("-", "_")


def lint_package(pkg_dir: Path, symbols: dict, virtuals: set) -> list:
    cfg = pkg_dir / "Config.in"
    if not cfg.is_file():
        return []

    main_symbol, selects = extract_selects_and_main_symbol(cfg)
    var_prefix = derive_pkg_var_prefix(pkg_dir, main_symbol)

    mk_candidates = list(pkg_dir.glob("*.mk"))
    if not mk_candidates:
        return []
    mk = pkg_dir / f"{pkg_dir.name}.mk"
    if not mk.is_file():
        mk = mk_candidates[0]

    deps = extract_deps(mk, var_prefix)
    violations = []
    rel = _display(pkg_dir)
    for dep, guards in sorted(deps.items()):
        if dep.startswith(DEP_BLOCKLIST_PREFIX) or dep in DEP_BLOCKLIST_EXACT:
            continue
        sym_name = dep.upper().replace("-", "_")
        if sym_name in virtuals:
            continue
        sym = kconfig_symbol(dep)
        if sym in guards:
            # Dep only added when this symbol is already selected elsewhere.
            continue
        key = f"{rel}:{dep}"
        if sym not in symbols:
            violations.append((
                f"{rel}:unknown:{dep}",
                f"{rel}: dep '{dep}' has no Kconfig "
                f"symbol {sym} (typo or removed?)",
            ))
            continue
        if sym not in selects:
            decl = _display(symbols[sym])
            violations.append((
                key,
                f"{rel}: {mk.name} depends on '{dep}' "
                f"but Config.in does not `select {sym}` (defined in {decl})",
            ))
    return violations


def discover_packages(args_pkgs):
    if args_pkgs:
        return [Path(p) if Path(p).is_absolute() else REPO / p for p in args_pkgs]
    pkgs = []
    for root in (REPO / "package", REPO / "custom" / "package"):
        if not root.is_dir():
            continue
        for d in sorted(root.iterdir()):
            if d.is_dir():
                pkgs.append(d)
    return pkgs


def load_baseline(path: Path) -> set:
    if not path.is_file():
        return set()
    keys = set()
    for line in path.read_text().splitlines():
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        keys.add(s)
    return keys


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--ci", action="store_true",
                    help="exit non-zero on any *new* violation (not in baseline)")
    ap.add_argument("--baseline-out", metavar="FILE",
                    help="write current violations as a baseline file and exit 0")
    ap.add_argument("--no-baseline", action="store_true",
                    help="ignore the baseline file (show all violations)")
    ap.add_argument("pkgs", nargs="*",
                    help="package dirs to check (default: all Recalbox pkgs)")
    args = ap.parse_args()

    symbols, virtuals = build_symbol_index()
    if not symbols:
        print("ERROR: no BR2_PACKAGE_ symbols found — "
              "are buildroot/ and package/ present?", file=sys.stderr)
        return 2

    pkgs = discover_packages(args.pkgs)
    all_violations = []
    for pkg in pkgs:
        all_violations.extend(lint_package(pkg, symbols, virtuals))

    if args.baseline_out:
        path = Path(args.baseline_out)
        path.write_text(
            "# Generated by scripts/ci/lint-package-deps.py --baseline-out\n"
            "# Each line: <pkg_dir>:<dep> for already-known violations.\n"
            "# Fix entries when you can — never add new ones.\n"
            + "\n".join(sorted(k for k, _ in all_violations))
            + "\n"
        )
        print(f"Wrote {len(all_violations)} entries to {path}", file=sys.stderr)
        return 0

    baseline = set() if args.no_baseline else load_baseline(BASELINE_FILE)
    new_violations = [(k, m) for (k, m) in all_violations if k not in baseline]
    stale_baseline = baseline - {k for k, _ in all_violations}

    for _, msg in new_violations:
        print(msg)

    print(
        f"\nScanned {len(pkgs)} packages: "
        f"{len(all_violations)} total violation(s), "
        f"{len(baseline)} baselined, "
        f"{len(new_violations)} NEW.",
        file=sys.stderr,
    )
    if stale_baseline:
        print(
            f"NOTE: {len(stale_baseline)} baseline entries no longer match — "
            f"consider regenerating with --baseline-out.",
            file=sys.stderr,
        )

    if args.ci and new_violations:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
