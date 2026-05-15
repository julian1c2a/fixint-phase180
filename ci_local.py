#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ci_local.py — Local CI pipeline for int128-phase175
====================================================

Espejo del .github/workflows/ci.yml pero solo con compiladores locales.
Corre make.py test para cada combinación compilador/modo y muestra un
dashboard resumen al final.

Uso:
    python ci_local.py                   # GCC + Clang + MSVC + Intel, release-O2
    python ci_local.py --quick           # solo GCC + Clang (más rápido)
    python ci_local.py --with-debug      # añade GCC debug al conjunto completo
    python ci_local.py --compiler gcc clang   # compiladores específicos
    python ci_local.py --mode release-O2 debug
    python ci_local.py --quiet           # solo muestra resumen (suprime output de make.py)
"""

import sys
import os
import subprocess
import time
import argparse
from pathlib import Path
from datetime import datetime
import io

# Force UTF-8 on Windows
if sys.platform == "win32":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

PROJECT_ROOT = Path(__file__).parent

# ============================================================================
# Colors
# ============================================================================

class C:
    GREEN   = "\033[0;32m"
    RED     = "\033[0;31m"
    YELLOW  = "\033[1;33m"
    BLUE    = "\033[0;34m"
    CYAN    = "\033[0;36m"
    BOLD    = "\033[1m"
    NC      = "\033[0m"


def green(s):  return f"{C.GREEN}{s}{C.NC}"
def red(s):    return f"{C.RED}{s}{C.NC}"
def yellow(s): return f"{C.YELLOW}{s}{C.NC}"
def blue(s):   return f"{C.BLUE}{s}{C.NC}"
def bold(s):   return f"{C.BOLD}{s}{C.NC}"


# ============================================================================
# Compiler detection
# ============================================================================

COMPILER_BINS = {
    "gcc":   Path(r"C:\msys64\ucrt64\bin\g++.exe"),
    "clang": Path(r"C:\msys64\ucrt64\bin\clang++.exe"),
    "msvc":  Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"),
    "intel": None,  # checked separately
}

def _intel_available() -> bool:
    for p in [
        Path(r"C:\Program Files (x86)\Intel\oneAPI\compiler\2025.3\bin\icpx.exe"),
        Path(r"C:\Program Files (x86)\Intel\oneAPI\compiler\latest\bin\icpx.exe"),
    ]:
        if p.exists():
            return True
    return False

def compiler_available(name: str) -> bool:
    if name == "intel":
        return _intel_available()
    p = COMPILER_BINS.get(name)
    if p is None:
        return False
    return p.exists()


# ============================================================================
# CI matrix definition
# ============================================================================
#
# Each entry:  (label, compiler, mode, required)
#   required=True  → failure makes the whole CI fail
#   required=False → failure is reported as WARN, CI can still pass

MATRIX_FULL = [
    ("GCC   release-O2", "gcc",   "release-O2", True),
    ("Clang release-O2", "clang", "release-O2", True),
    ("MSVC  release-O2", "msvc",  "release-O2", False),
    ("Intel release-O2", "intel", "release-O2", False),
]

MATRIX_QUICK = [
    ("GCC   release-O2", "gcc",   "release-O2", True),
    ("Clang release-O2", "clang", "release-O2", True),
]

MATRIX_DEBUG_EXTRA = [
    ("GCC   debug",      "gcc",   "debug",      False),
]


# ============================================================================
# Run one CI job
# ============================================================================

def run_job(label: str, compiler: str, mode: str, required: bool,
            quiet: bool) -> dict:
    """
    Run 'python make.py test <compiler> <mode>'.
    Returns dict with status, duration, return_code.
    """
    if not compiler_available(compiler):
        return {"status": "SKIP", "duration": 0.0, "rc": 0, "required": required}

    cmd = [sys.executable, str(PROJECT_ROOT / "make.py"), "test", compiler, mode]

    print(f"\n{blue('='*62)}")
    print(f"{blue('  ')} {bold(label)}  [{compiler} {mode}]")
    print(f"{blue('='*62)}")

    start = time.time()

    if quiet:
        result = subprocess.run(
            cmd,
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        # On failure, show captured output
        if result.returncode != 0:
            print(result.stdout)
            print(result.stderr, file=sys.stderr)
    else:
        result = subprocess.run(cmd, cwd=PROJECT_ROOT)

    duration = time.time() - start
    rc = result.returncode

    status = "PASS" if rc == 0 else ("FAIL" if required else "WARN")
    return {"status": status, "duration": duration, "rc": rc, "required": required}


# ============================================================================
# Dashboard
# ============================================================================

STATUS_COLORS = {
    "PASS": green,
    "FAIL": red,
    "WARN": yellow,
    "SKIP": lambda s: f"\033[2m{s}\033[0m",  # dim
}

def fmt_status(s: str) -> str:
    return STATUS_COLORS.get(s, str)(s)

def fmt_duration(secs: float) -> str:
    if secs < 60:
        return f"{secs:.1f}s"
    m, s = divmod(secs, 60)
    return f"{int(m)}m{int(s):02d}s"


def print_dashboard(jobs: list, total_secs: float) -> int:
    """Print the summary table. Returns suggested exit code."""
    print(f"\n{blue('='*62)}")
    print(f"{blue('  LOCAL CI DASHBOARD — int128-phase175')}")
    print(f"{blue('='*62)}")

    col_w = max(len(j[0]) for j in jobs) + 2

    n_pass = n_fail = n_warn = n_skip = 0
    for label, result in jobs:
        s = result["status"]
        d = result["duration"]
        required_tag = "" if result["required"] else "  (optional)"

        line = f"  {label:<{col_w}}  {fmt_status(s):<20}  {fmt_duration(d)}{required_tag}"
        print(line)

        if s == "PASS": n_pass += 1
        elif s == "FAIL": n_fail += 1
        elif s == "WARN": n_warn += 1
        elif s == "SKIP": n_skip += 1

    print(f"{blue('-'*62)}")
    parts = [green(f"{n_pass} PASS")]
    if n_fail: parts.append(red(f"{n_fail} FAIL"))
    if n_warn: parts.append(yellow(f"{n_warn} WARN"))
    if n_skip: parts.append(f"\033[2m{n_skip} SKIP\033[0m")
    print(f"  {' | '.join(parts)}  — total {fmt_duration(total_secs)}")
    print(f"{blue('='*62)}\n")

    return 1 if n_fail > 0 else 0


# ============================================================================
# Main
# ============================================================================

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Local CI pipeline — int128-phase175",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--quick",       action="store_true",
                        help="Solo GCC + Clang (sin MSVC, Intel, debug)")
    parser.add_argument("--with-debug",  action="store_true",
                        help="Añadir GCC debug al conjunto de jobs")
    parser.add_argument("--compiler",    nargs="+",
                        choices=["gcc", "clang", "msvc", "intel"],
                        metavar="COMPILER",
                        help="Compiladores específicos (gcc clang msvc intel)")
    parser.add_argument("--mode",        nargs="+",
                        metavar="MODE",
                        help="Modos específicos (release-O2 debug ...)")
    parser.add_argument("--quiet", "-q", action="store_true",
                        help="Suprime output de make.py (solo muestra resumen)")

    args = parser.parse_args()

    # Resolve matrix
    if args.compiler or args.mode:
        compilers = args.compiler or ["gcc", "clang", "msvc", "intel"]
        modes     = args.mode     or ["release-O2"]
        matrix = []
        for comp in compilers:
            for mode in modes:
                required = comp in ("gcc", "clang") and mode == "release-O2"
                matrix.append((f"{comp.upper():<5} {mode}", comp, mode, required))
    elif args.quick:
        matrix = list(MATRIX_QUICK)
    else:
        matrix = list(MATRIX_FULL)
        if args.with_debug:
            matrix += MATRIX_DEBUG_EXTRA

    # Header
    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    tag = "quick" if args.quick else "full"
    if args.with_debug:
        tag += "+debug"
    print(f"\n{blue('='*62)}")
    print(f"{blue('  LOCAL CI — int128-phase175')}")
    print(f"{blue('  ')} {now}  |  mode: {bold(tag)}")
    print(f"{blue('='*62)}")

    # Compiler availability summary
    all_compilers = ["gcc", "clang", "msvc", "intel"]
    for comp in all_compilers:
        avail = compiler_available(comp)
        tag_str = green("found") if avail else yellow("not found — jobs will be SKIP")
        print(f"  {comp:<8} {tag_str}")

    # Run jobs
    wall_start = time.time()
    results = []
    for i, (label, compiler, mode, required) in enumerate(matrix, 1):
        print(f"\n[{i}/{len(matrix)}]", end="")
        result = run_job(label, compiler, mode, required, args.quiet)
        results.append((label, result))

    wall_total = time.time() - wall_start

    # Dashboard
    return print_dashboard(results, wall_total)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print(yellow("\n\n  Interrupted by user"))
        sys.exit(130)
