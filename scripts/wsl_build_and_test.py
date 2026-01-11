#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
# int128 Library - 128-bit Integer Types for C++20
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
# Copyright (c) 2024-2026 Julián Calderón Almendros
#
# WSL Build and Test - Master Script (Python version)
# =============================================================================

"""
WSL Build and Test - Master Script for unified int128_base_t<S> template.

Tests the unified template across all WSL compilers (GCC, Clang, Intel).

Usage:
    python wsl_build_and_test.py <feature> [compiler] [mode]

Features: tt, algorithm, bits, cmath, concepts, format, iostreams,
          limits, numeric, ranges, safe, thread_safety, traits
Compilers: gcc-13, gcc-14, gcc-15, clang-18, clang-19, clang-20, clang-21, icpx, all
Modes: debug, release, all

Examples:
    python wsl_build_and_test.py tt all all           # All compilers
    python wsl_build_and_test.py bits gcc-15 release
    python wsl_build_and_test.py cmath clang-21 all
"""

import sys
import os
import subprocess
from pathlib import Path
from typing import Dict, List, Tuple, Optional

# Add script directory to path for imports
SCRIPT_DIR = Path(__file__).parent.resolve()
sys.path.insert(0, str(SCRIPT_DIR / "wsl"))

# Import common module
try:
    from wsl.common import (
        Colors, echo_info, echo_success, echo_error, echo_warn, echo_header,
        VALID_FEATURES, VALID_MODES, VALID_COMPILERS, COMPILER_COMMANDS,
        PROJECT_ROOT, INCLUDE_DIR, BUILD_BASE,
        check_compiler, get_compiler_version, detect_available_compilers,
        get_source_file, build_and_test, print_summary
    )
except ImportError:
    # Fallback: try direct import if running from scripts directory
    sys.path.insert(0, str(SCRIPT_DIR))
    from wsl.common import (
        Colors, echo_info, echo_success, echo_error, echo_warn, echo_header,
        VALID_FEATURES, VALID_MODES, VALID_COMPILERS, COMPILER_COMMANDS,
        PROJECT_ROOT, INCLUDE_DIR, BUILD_BASE,
        check_compiler, get_compiler_version, detect_available_compilers,
        get_source_file, build_and_test, print_summary
    )


def show_usage():
    """Print usage information."""
    print(f"""
Usage: {sys.argv[0]} <feature> [compiler] [mode]

Tests the unified int128_base_t<S> template.

Features:  {' '.join(VALID_FEATURES)}
Compilers: {' '.join(VALID_COMPILERS)}
Modes:     {' '.join(VALID_MODES)}

Examples:
  {sys.argv[0]} tt all all           # All compilers, all modes
  {sys.argv[0]} bits gcc-15 release  # Just GCC 15 release
  {sys.argv[0]} cmath clang-21 all   # Clang 21 debug+release
""")


def load_intel_environment() -> bool:
    """Try to load Intel oneAPI environment."""
    setvars_path = Path("/opt/intel/oneapi/setvars.sh")
    if setvars_path.exists():
        try:
            # Source setvars.sh and capture environment
            result = subprocess.run(
                f"source {setvars_path} --force > /dev/null 2>&1 && env",
                shell=True,
                executable="/bin/bash",
                capture_output=True,
                text=True,
                timeout=30
            )
            if result.returncode == 0:
                # Parse and update environment
                for line in result.stdout.strip().split('\n'):
                    if '=' in line:
                        key, _, value = line.partition('=')
                        os.environ[key] = value
                return True
        except Exception:
            pass
    return False


def main() -> int:
    """Main entry point."""
    
    # Parse arguments
    if len(sys.argv) < 2:
        show_usage()
        return 1
    
    feature = sys.argv[1]
    compiler = sys.argv[2] if len(sys.argv) > 2 else "all"
    mode = sys.argv[3] if len(sys.argv) > 3 else "all"
    
    # Validate feature
    if feature not in VALID_FEATURES:
        echo_error(f"Invalid feature: {feature}")
        print(f"Valid: {' '.join(VALID_FEATURES)}")
        return 1
    
    # Validate compiler
    if compiler not in VALID_COMPILERS:
        echo_error(f"Invalid compiler: {compiler}")
        print(f"Valid: {' '.join(VALID_COMPILERS)}")
        return 1
    
    # Validate mode
    if mode not in VALID_MODES:
        echo_error(f"Invalid mode: {mode}")
        print(f"Valid: {' '.join(VALID_MODES)}")
        return 1
    
    # Check source file exists
    source_file = get_source_file(feature)
    if not source_file.exists():
        echo_error(f"Source file not found: {source_file}")
        return 1
    
    # Print header
    echo_header("=" * 60)
    echo_header(f"  WSL Build & Test - int128_{feature}")
    echo_header(f"  Compiler: {compiler} | Mode: {mode}")
    echo_header("=" * 60)
    print()
    
    # Try to load Intel environment
    echo_info("Loading Intel oneAPI environment (if available)...")
    load_intel_environment()
    
    # Detect available compilers
    echo_info("Detecting available compilers...")
    available_compilers = detect_available_compilers()
    
    if not available_compilers:
        echo_error("No compilers found!")
        return 1
    
    print()
    
    # Build list of compilers to run
    compilers_to_run: List[str] = []
    
    if compiler == "all":
        compilers_to_run = list(available_compilers.keys())
    else:
        if compiler in available_compilers:
            compilers_to_run = [compiler]
        else:
            echo_error(f"Compiler '{compiler}' not available")
            return 1
    
    # Execute builds and tests
    results: Dict[str, str] = {}
    total_passed = 0
    total_failed = 0
    
    for comp in compilers_to_run:
        cmd = available_compilers[comp]
        
        passed, failed = build_and_test(comp, cmd, feature, mode)
        
        # Record results
        if mode == "all":
            for m in ["debug", "release"]:
                key = f"{comp}_{m}"
                results[key] = "PASS" if passed > failed else "FAIL"
        else:
            key = f"{comp}_{mode}"
            results[key] = "PASS" if passed > 0 else "FAIL"
        
        total_passed += passed
        total_failed += failed
    
    # Print summary
    passed_count, total_count = print_summary(results, feature)
    
    # Final status
    print()
    if total_failed == 0:
        echo_success("All tests passed!")
        return 0
    else:
        echo_error(f"{total_failed} test(s) failed")
        return 1


if __name__ == "__main__":
    sys.exit(main())
