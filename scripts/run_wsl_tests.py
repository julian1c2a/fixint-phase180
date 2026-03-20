#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
# int128 Library - 128-bit Integer Types for C++20
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
# Copyright (c) 2024-2026 Julián Calderón Almendros
#
# WSL Test Runner - Windows to WSL bridge
# =============================================================================

"""
WSL Test Runner - Bridge for running tests from Windows in WSL.

This script provides a Windows-friendly interface to run the unified
int128_base_t<S> template tests in WSL (Ubuntu).

Usage (from Windows PowerShell/CMD):
    python scripts/run_wsl_tests.py <feature> [compiler] [mode]
    python scripts/run_wsl_tests.py tt all all
    python scripts/run_wsl_tests.py bits gcc-15 release

Features: tt, algorithm, bits, cmath, concepts, format, iostreams,
          limits, numeric, ranges, safe, thread_safety, traits
Compilers: gcc-13, gcc-14, gcc-15, clang-18, clang-19, clang-20, clang-21, icpx, all
Modes: debug, release, all
"""

import subprocess
import sys
import os
import shutil
from pathlib import Path

# Force UTF-8 encoding for Windows
if sys.platform == "win32":
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

# =============================================================================
# Valid values (must match wsl/common.py and wsl_build_and_test.bash)
# =============================================================================
VALID_FEATURES = [
    "tt", "t", "algorithm", "bits", "cmath", "concepts", 
    "format", "iostreams", "limits", "numeric", "ranges", 
    "safe", "thread_safety", "traits"
]

VALID_COMPILERS = [
    "gcc-13", "gcc-14", "gcc-15",
    "clang-18", "clang-19", "clang-20", "clang-21",
    "icpx", "all"
]

VALID_MODES = ["debug", "release", "all"]


def fix_line_endings(directory):
    """
    Convert CRLF to LF in shell and python scripts recursively.
    Avoids "$'\\r': command not found" errors in Linux/WSL.
    """
    print(f"🔧 Normalizing line endings (CRLF -> LF) in {directory}...")
    count = 0
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.bash', '.sh', '.py')):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, 'rb') as f:
                        content = f.read()
                    
                    if b'\r\n' in content:
                        content = content.replace(b'\r\n', b'\n')
                        with open(filepath, 'wb') as f:
                            f.write(content)
                        count += 1
                except Exception as e:
                    print(f"  ⚠️ Error processing {file}: {e}")
    
    if count > 0:
        print(f"  ✅ Fixed {count} files.")
    else:
        print("  ✅ All files already have LF endings.")
    return count


def get_wsl_path(windows_path):
    """Convert Windows path to WSL path (/mnt/c/...)."""
    drive, path = os.path.splitdrive(windows_path)
    
    if drive:
        # Standard Windows path (C:\...)
        drive_letter = drive[0].lower()
        return f"/mnt/{drive_letter}{path.replace(os.sep, '/')}"
    else:
        # POSIX/MSYS2 style path (/c/...)
        if windows_path.startswith('/') and len(windows_path) >= 3:
            if windows_path[2] == '/' and windows_path[1].isalpha():
                return f"/mnt/{windows_path[1].lower()}{windows_path[2:]}"
        return windows_path


def show_usage():
    """Print usage information."""
    print(f"""
WSL Test Runner - Windows to WSL Bridge
=======================================

Usage: python {sys.argv[0]} <feature> [compiler] [mode]

Features:  {' '.join(VALID_FEATURES)}
Compilers: {' '.join(VALID_COMPILERS)}
Modes:     {' '.join(VALID_MODES)}

Examples:
  python {sys.argv[0]} tt all all           # All compilers, all modes
  python {sys.argv[0]} bits gcc-15 release  # Just GCC 15 release
  python {sys.argv[0]} cmath clang-21 all   # Clang 21 debug+release
""")


def main():
    """Main entry point."""
    print("🐧 WSL Test Runner - Windows to WSL Bridge")
    print("=" * 50)
    
    # Check WSL is available
    if shutil.which("wsl") is None:
        print("❌ Error: 'wsl' command not found. Please install WSL.")
        sys.exit(1)
    
    # Parse arguments
    if len(sys.argv) < 2:
        show_usage()
        sys.exit(1)
    
    feature = sys.argv[1]
    compiler = sys.argv[2] if len(sys.argv) > 2 else "all"
    mode = sys.argv[3] if len(sys.argv) > 3 else "all"
    
    # Validate feature
    if feature not in VALID_FEATURES:
        print(f"❌ Invalid feature: {feature}")
        print(f"Valid: {' '.join(VALID_FEATURES)}")
        sys.exit(1)
    
    # Validate compiler
    if compiler not in VALID_COMPILERS:
        print(f"❌ Invalid compiler: {compiler}")
        print(f"Valid: {' '.join(VALID_COMPILERS)}")
        sys.exit(1)
    
    # Validate mode
    if mode not in VALID_MODES:
        print(f"❌ Invalid mode: {mode}")
        print(f"Valid: {' '.join(VALID_MODES)}")
        sys.exit(1)
    
    # Get paths
    cwd = os.getcwd()
    wsl_path = get_wsl_path(cwd)
    
    print(f"📂 Windows path: {cwd}")
    print(f"📂 WSL path: {wsl_path}")
    print(f"🎯 Feature: {feature}")
    print(f"🔧 Compiler: {compiler}")
    print(f"⚙️  Mode: {mode}")
    print()
    
    # Fix line endings in scripts directory
    scripts_dir = os.path.join(cwd, "scripts")
    if os.path.exists(scripts_dir):
        fix_line_endings(scripts_dir)
    
    print()
    
    # Build the WSL command using the bash script (which is already validated)
    intel_env_cmd = "source /opt/intel/oneapi/setvars.sh --force > /dev/null 2>&1 || true"
    
    bash_cmd = (
        f"{intel_env_cmd} && cd '{wsl_path}' && "
        f"bash scripts/wsl_build_and_test.bash {feature} {compiler} {mode}"
    )
    
    print("🚀 Running tests in WSL...")
    print("-" * 50)
    
    try:
        result = subprocess.run(["wsl", "bash", "-c", bash_cmd], check=False)
        
        print("-" * 50)
        
        if result.returncode == 0:
            print("\n✅ [Windows] WSL tests completed successfully!")
            sys.exit(0)
        else:
            print(f"\n❌ [Windows] WSL tests failed (exit code: {result.returncode})")
            sys.exit(result.returncode)
            
    except Exception as e:
        print(f"\n❌ [Windows] Error running WSL: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()