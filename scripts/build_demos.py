#!/usr/bin/env python3
"""
build_demos.py - Automated demo compilation script for int128 Library

Usage:
    python scripts/build_demos.py [category] [compiler] [mode]

Arguments:
    category  - tutorials|examples|showcase|general|all (default: all)
    compiler  - gcc|clang|all (default: gcc)
    mode      - debug|release (default: release)

Examples:
    python scripts/build_demos.py                      # Build all demos with gcc release
    python scripts/build_demos.py tutorials            # Build tutorials only
    python scripts/build_demos.py all clang release    # Build all with clang release
    python scripts/build_demos.py examples gcc debug   # Build examples with gcc debug
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path
from typing import List, Tuple, Optional

# =============================================================================
# Configuration
# =============================================================================

PROJECT_ROOT = Path(__file__).parent.parent.resolve()
INCLUDE_DIR = PROJECT_ROOT / "include"
BUILD_BASE = PROJECT_ROOT / "build" / "build_demos"

DEMO_CATEGORIES = ["tutorials", "examples", "showcase", "general"]

# Compiler paths (Windows native)
COMPILERS = {
    "gcc": "C:/msys64/ucrt64/bin/g++.exe",
    "clang": "C:/msys64/clang64/bin/clang++.exe",
}

# Compiler flags by mode
FLAGS = {
    "common": ["-std=c++20", f"-I{INCLUDE_DIR}"],
    "debug": ["-O0", "-g", "-DDEBUG"],
    "release": ["-O2", "-DNDEBUG"],
    "warnings": ["-Wall", "-Wextra", "-Wpedantic"],
}


# =============================================================================
# Helper functions
# =============================================================================

def needs_threading(demo_file: Path) -> bool:
    """Check if demo needs threading support."""
    try:
        content = demo_file.read_text(encoding='utf-8', errors='replace')
        return any(x in content for x in ['<thread>', '<atomic>', 'thread_safety'])
    except Exception:
        return False


def get_compiler_cmd(compiler: str) -> str:
    """Get compiler executable path."""
    if compiler not in COMPILERS:
        raise ValueError(f"Unknown compiler: {compiler}")
    return COMPILERS[compiler]


def get_flags(compiler: str, mode: str) -> List[str]:
    """Get compiler flags for given compiler and mode."""
    flags = FLAGS["common"].copy()
    flags.extend(FLAGS.get(mode, FLAGS["release"]))
    flags.extend(FLAGS["warnings"])
    return flags


def build_demo(demo_file: Path, compiler: str, mode: str) -> Tuple[bool, str]:
    """
    Build a single demo file.
    
    Returns: (success, message)
    """
    demo_name = demo_file.stem
    category = demo_file.parent.name
    
    output_dir = BUILD_BASE / compiler / mode / category
    output_dir.mkdir(parents=True, exist_ok=True)
    
    output_file = output_dir / demo_name
    if sys.platform == "win32" or os.name == "nt":
        output_file = output_file.with_suffix(".exe")
    
    compiler_cmd = get_compiler_cmd(compiler)
    flags = get_flags(compiler, mode)
    
    # Add threading flags if needed
    if needs_threading(demo_file):
        flags.append("-pthread")
        if compiler == "gcc":
            flags.append("-latomic")
    
    # Build command
    cmd = [compiler_cmd] + flags + ["-o", str(output_file), str(demo_file)]
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=60
        )
        
        if result.returncode == 0:
            return True, f"[OK] {category}/{demo_name}"
        else:
            error_msg = result.stderr[:200] if result.stderr else "Unknown error"
            return False, f"[FAIL] {category}/{demo_name}: {error_msg}"
            
    except subprocess.TimeoutExpired:
        return False, f"[TIMEOUT] {category}/{demo_name}"
    except FileNotFoundError:
        return False, f"[ERROR] Compiler not found: {compiler_cmd}"
    except Exception as e:
        return False, f"[ERROR] {category}/{demo_name}: {e}"


def build_category(category: str, compiler: str, mode: str) -> Tuple[int, int]:
    """
    Build all demos in a category.
    
    Returns: (ok_count, fail_count)
    """
    demo_dir = PROJECT_ROOT / "demos" / category
    
    if not demo_dir.is_dir():
        print(f"[SKIP] Category not found: {category}")
        return 0, 0
    
    ok_count = 0
    fail_count = 0
    
    for demo_file in sorted(demo_dir.glob("*.cpp")):
        success, message = build_demo(demo_file, compiler, mode)
        print(message)
        
        if success:
            ok_count += 1
        else:
            fail_count += 1
    
    print(f"  Subtotal {category}: {ok_count} OK, {fail_count} FAIL")
    return ok_count, fail_count


# =============================================================================
# Main
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Build int128 library demos",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "category",
        nargs="?",
        default="all",
        choices=DEMO_CATEGORIES + ["all"],
        help="Demo category to build (default: all)"
    )
    parser.add_argument(
        "compiler",
        nargs="?",
        default="gcc",
        choices=["gcc", "clang", "all"],
        help="Compiler to use (default: gcc)"
    )
    parser.add_argument(
        "mode",
        nargs="?",
        default="release",
        choices=["debug", "release"],
        help="Build mode (default: release)"
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Show compilation commands"
    )
    
    args = parser.parse_args()
    
    print("=" * 60)
    print("  int128 Library - Demo Build System")
    print("=" * 60)
    print()
    print(f"Configuration:")
    print(f"  Category: {args.category}")
    print(f"  Compiler: {args.compiler}")
    print(f"  Mode:     {args.mode}")
    print()
    print("-" * 60)
    
    # Determine compilers and categories
    compilers = ["gcc", "clang"] if args.compiler == "all" else [args.compiler]
    categories = DEMO_CATEGORIES if args.category == "all" else [args.category]
    
    total_ok = 0
    total_fail = 0
    
    for compiler in compilers:
        print()
        print(f">>> Building with {compiler} ({args.mode})")
        print()
        
        for category in categories:
            ok, fail = build_category(category, compiler, args.mode)
            total_ok += ok
            total_fail += fail
    
    print()
    print("=" * 60)
    print("  Build Summary")
    print("=" * 60)
    print()
    print(f"  Total: {total_ok} OK, {total_fail} FAIL")
    print()
    print(f"Output directory: {BUILD_BASE}/")
    print()
    
    if total_fail > 0:
        print("Some demos failed to compile. Run fix_demos.py first:")
        print("  python fix_demos.py")
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
