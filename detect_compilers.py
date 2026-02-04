#!/usr/bin/env python3
"""
Setup MSVC and Intel paths, then run multi-compiler tests
"""

import os
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).parent.resolve()

# MSVC paths (Windows)
MSVC_BIN = r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64"
KIT_BIN = r"C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64"
INTEL_ROOT = r"C:\Program Files (x86)\Intel\oneAPI"

# Check what's available
print("=" * 70)
print("  Compiler Detection & Setup")
print("=" * 70)
print()

# Check GCC
print("[Checking] GCC...")
result = subprocess.run(["g++", "--version"], capture_output=True)
print(f"  {'[FOUND]' if result.returncode == 0 else '[NOT FOUND]'}")

# Check Clang
print("[Checking] Clang...")
result = subprocess.run(["clang++", "--version"], capture_output=True)
print(f"  {'[FOUND]' if result.returncode == 0 else '[NOT FOUND]'}")

# Check MSVC
print("[Checking] MSVC...")
msvc_exe = Path(MSVC_BIN) / "cl.exe"
if msvc_exe.exists():
    print(f"  [FOUND] {msvc_exe}")
else:
    print(f"  [NOT FOUND] {msvc_exe}")

# Check Intel
print("[Checking] Intel oneAPI...")
intel_exe = Path(INTEL_ROOT) / "compiler" / "latest" / "bin" / "icx.exe"
if intel_exe.exists():
    print(f"  [FOUND] {intel_exe}")
else:
    print(f"  [NOT FOUND] Checking alternative...")
    # Try to find icx in PATH or standard locations
    result = subprocess.run(["where", "icx.exe"], capture_output=True, text=True)
    if result.returncode == 0:
        print(f"  [FOUND] {result.stdout.strip()}")
    else:
        print(f"  [NOT FOUND]")

print()
print("=" * 70)
print("  Running Multi-Compiler Test Suite...")
print("=" * 70)
print()

# Change to project root and run tests
os.chdir(str(PROJECT_ROOT))
sys.exit(subprocess.call([sys.executable, "multi_compiler_test.py"]))
