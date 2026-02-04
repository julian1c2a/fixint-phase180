#!/usr/bin/env python3
"""
Multi-compiler test suite for division algorithm
Soporta: GCC, Clang, MSVC, Intel oneAPI
"""

import os
import subprocess
import sys
from pathlib import Path
from datetime import datetime

# Setup paths
PROJECT_ROOT = Path(__file__).parent.resolve()
TEST_FILE = PROJECT_ROOT / "tests" / "test_divmod_final.cpp"
BUILD_DIR = PROJECT_ROOT / "build"
INCLUDE_DIR = PROJECT_ROOT / "include"
RESULTS_FILE = PROJECT_ROOT / "compiler_results.txt"

# Ensure build directory exists
BUILD_DIR.mkdir(exist_ok=True)

# Compiler paths
MSVC_PATH = r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
INTEL_PATH = r"C:\Program Files (x86)\Intel\oneAPI\compiler\latest\bin\icx.exe"

# Compiler configurations
COMPILERS = [
    {
        "name": "GCC -O0 (Baseline)",
        "cmd": ["g++", "-std=c++20", "-O0"],
        "output": BUILD_DIR / "test_gcc_o0.exe",
        "optional": False,
    },
    {
        "name": "Clang -O2 (Optimized)",
        "cmd": ["clang++", "-std=c++20", "-O2"],
        "output": BUILD_DIR / "test_clang_o2.exe",
        "optional": False,
    },
    {
        "name": "MSVC 2026 (Windows)",
        "cmd": [MSVC_PATH, "/std:c++latest", "/O2"],
        "output": BUILD_DIR / "test_msvc.exe",
        "optional": True,
    },
    {
        "name": "Intel ICX (oneAPI)",
        "cmd": [INTEL_PATH, "/std:c++20", "/O2"],
        "output": BUILD_DIR / "test_intel.exe",
        "optional": True,
    },
]


def run_command(cmd_list, cwd=None):
    """Run a command and return (returncode, stdout, stderr)"""
    try:
        result = subprocess.run(
            cmd_list,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=60,
        )
        return result.returncode, result.stdout, result.stderr
    except FileNotFoundError:
        return -1, "", f"Command not found: {cmd_list[0]}"
    except subprocess.TimeoutExpired:
        return -1, "", "Command timeout (60s)"
    except Exception as e:
        return -1, "", str(e)


def compile_and_test(compiler_config):
    """Compile and test with a single compiler"""
    name = compiler_config["name"]
    cmd = compiler_config["cmd"]
    output_exe = compiler_config["output"]
    
    # Check if compiler exists (use first element of cmd list)
    compiler_path = cmd[0]
    
    # If it's a full path, check if file exists
    if compiler_path.startswith("C:\\") or compiler_path.startswith("/c/"):
        if not Path(compiler_path).exists():
            if compiler_config["optional"]:
                return f"[SKIP] {name} - not found\n"
            else:
                return f"[FAIL] {name}\n  Compiler not found: {compiler_path}\n\n"
    else:
        # For PATH lookups, try running --version
        check_cmd = [compiler_path, "--version"]
        ret, _, _ = run_command(check_cmd)
        if ret != 0 and compiler_config["optional"]:
            return f"[SKIP] {name} - not found in PATH\n"
    
    # Build compile command
    compile_cmd = cmd + [f"-I{INCLUDE_DIR}", str(TEST_FILE)]
    
    # Handle output flag based on compiler
    if "cl.exe" in str(cmd[0]):
        # MSVC uses /Fe: flag
        compile_cmd.append(f"/Fe:{output_exe}")
    else:
        # GCC, Clang, Intel use -o flag
        compile_cmd.extend(["-o", str(output_exe)])
    
    # Compile
    ret, stdout, stderr = run_command(compile_cmd, cwd=str(PROJECT_ROOT))
    
    if ret != 0:
        return f"[FAIL] {name}\n  Compilation failed\n  Error: {stderr[:200]}\n\n"
    
    if not output_exe.exists():
        return f"[FAIL] {name}\n  No executable generated\n\n"
    
    # Run tests
    ret, stdout, stderr = run_command([str(output_exe)], cwd=str(PROJECT_ROOT))
    
    output = f"[OK] {name}\n"
    if "9 passed" in stdout or "9 PASS" in stdout:
        output += "  Result: 9/9 PASS [OK]\n"
    else:
        output += f"  Output: {stdout[:200]}\n"
    
    if stderr:
        output += f"  Stderr: {stderr[:200]}\n"
    
    output += "\n"
    return output


def main():
    """Main test suite"""
    print("=" * 70)
    print("  Multi-Compiler Division Test Suite")
    print("=" * 70)
    print(f"\nProject: {PROJECT_ROOT}")
    print(f"Test file: {TEST_FILE}")
    print(f"Build dir: {BUILD_DIR}")
    print(f"Date/Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("\n" + "=" * 70)
    print("  Compilation & Test Results")
    print("=" * 70 + "\n")
    
    results = []
    
    for i, compiler in enumerate(COMPILERS, 1):
        print(f"[{i}/{len(COMPILERS)}] Testing {compiler['name']}...", flush=True)
        result = compile_and_test(compiler)
        results.append(result)
        print(result, end="", flush=True)
    
    # Write results to file
    with open(RESULTS_FILE, "w", encoding="utf-8") as f:
        f.write("=" * 70 + "\n")
        f.write("  Multi-Compiler Division Test Results\n")
        f.write("=" * 70 + "\n")
        f.write(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        f.write("".join(results))
        f.write("\n" + "=" * 70 + "\n")
        f.write("Generated Executables:\n")
        for exe in sorted(BUILD_DIR.glob("test_*.exe")):
            size = exe.stat().st_size
            f.write(f"  {exe.name} ({size} bytes)\n")
    
    # Print summary
    print("\n" + "=" * 70)
    print("  Summary")
    print("=" * 70)
    print(f"Results saved to: {RESULTS_FILE}")
    print(f"Executables: {len(list(BUILD_DIR.glob('test_*.exe')))}")
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
