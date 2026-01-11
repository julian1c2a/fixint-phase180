#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_generic.py - Script genérico de compilación en Python
============================================================

Compila tests, benchmarks o demos con todos los compiladores (GCC, Clang, Intel, MSVC)

Uso:
    # Para tests y benchs (sintaxis clásica):
    python build_generic.py <type> <feature> <target> [compiler] [mode] [print]
    
    # Para demos (nueva sintaxis):
    python build_generic.py demos <category> <demo_name> [compiler] [mode] [print]

Argumentos:
    type     : uint128 | int128 | demos
    feature  : bits | numeric | algorithm | etc. (o <category> si type=demos)
    target   : tests | benchs | <demo_name> (si type=demos)
    compiler : gcc | clang | intel | msvc | all (default: all)
    mode     : debug | debug-asan | debug-ubsan | release | release-O1 | release-O2 | release-O3 | release-Ofast | all (default: all)
    print    : yes | no (default: no) - Imprime comandos de compilación

Ejemplos Tests/Benchs:
    python build_generic.py uint128 bits tests
    python build_generic.py int128 numeric benchs gcc release
    python build_generic.py uint128 algorithm tests all all yes

Ejemplos Demos:
    python build_generic.py demos tutorials 01_basic_operations gcc release
    python build_generic.py demos examples ipv6_address clang debug
    python build_generic.py demos showcase main gcc release
"""

import sys
import os
import subprocess
import shutil
from pathlib import Path
from typing import List, Optional

# Add env_setup to path for importing compiler_env module
sys.path.insert(0, str(Path(__file__).parent / "env_setup"))

try:
    from compiler_env import CompilerEnvironment
    USE_COMPILER_ENV = True
except ImportError:
    USE_COMPILER_ENV = False
    print("[WARN] compiler_env module not available, using default environment")

# Force UTF-8 encoding for Windows
if sys.platform == "win32":
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')


class Colors:
    """ANSI color codes for terminal output"""
    GREEN = '\033[0;32m'
    RED = '\033[0;31m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'  # No Color


def echo_success(msg: str) -> None:
    print(f"{Colors.GREEN}OK {msg}{Colors.NC}")


def echo_error(msg: str) -> None:
    print(f"{Colors.RED}ERROR {msg}{Colors.NC}")


def echo_info(msg: str) -> None:
    print(f"{Colors.YELLOW}INFO {msg}{Colors.NC}")


def echo_header(msg: str) -> None:
    print(f"{Colors.BLUE}{msg}{Colors.NC}")


def find_compiler(compiler_cmd: str) -> Optional[str]:
    """Check if compiler exists in PATH"""
    return shutil.which(compiler_cmd)


def compile_with_compiler(
    compiler_name: str,
    compiler_cmd: str,
    source_file: str,
    build_dir: str,
    type_name: str,
    feature: str,
    output_suffix: str,
    modes: List[str],
    print_commands: bool,
    skip_check: bool = False,
    project_root: Path = None  # Add project_root parameter
) -> None:
    """Compile source file with specified compiler"""
    
    if project_root is None:
        project_root = Path.cwd()
    
    echo_info(f"Building with {compiler_name}...")
    
    # Check if compiler exists (unless skip_check is set)
    if not skip_check:
        if not find_compiler(compiler_cmd):
            echo_error(f"{compiler_name} not found ({compiler_cmd}). Skipping...")
            print()
            return
    
    # Get compiler environment (isolated, doesn't modify global environment)
    if USE_COMPILER_ENV:
        comp_env = CompilerEnvironment(compiler_name)
        env = comp_env.get_env()
        echo_info(f"  Using isolated environment for {compiler_name}")
    else:
        env = os.environ.copy()
    
    # Compile for each mode
    for mode in modes:
        output_dir = Path(build_dir) / compiler_name / mode
        output_dir.mkdir(parents=True, exist_ok=True)
        
        # Build output filename
        if type_name and feature:
            # Tests/benchs: <type>_<feature>_<suffix>_<compiler>
            output = output_dir / f"{type_name}_{feature}_{output_suffix}_{compiler_name}"
        else:
            # Demos: just <demo_name>
            output = output_dir / output_suffix
        
        # Add .exe extension for Windows compilers
        if compiler_name in ["msvc", "intel"]:
            output = output.with_suffix(".exe")
        
        # Check if source file uses threading (for pthread flag)
        needs_pthread = False
        needs_atomic = False
        try:
            with open(source_file, 'r', encoding='utf-8') as f:
                content = f.read(4000)  # Read first 4000 chars
                if '<thread>' in content or 'std::thread' in content or 'pthread' in content:
                    needs_pthread = True
                # Check for atomic usage or thread_safety headers
                if ('<atomic>' in content or 'std::atomic' in content or 'atomic_' in content or
                    'thread_safety.hpp' in content):
                    needs_atomic = True
        except:
            pass
        
        # Build command - convert paths to forward slashes for compatibility
        source_str = str(source_file).replace("\\", "/")
        output_str = str(output).replace("\\", "/")
        
        # Set compiler-specific flags
        if compiler_name == "msvc":
            # MSVC flags
            common_flags = ["/std:c++20", "/W4", "/EHsc", "/I./include"]
            
            if mode == "debug":
                mode_flags = ["/Od", "/Zi", "/DDEBUG"]
            elif mode == "debug-asan":
                mode_flags = ["/Od", "/Zi", "/DDEBUG", "/fsanitize=address"]
            elif mode == "debug-ubsan":
                # MSVC doesn't have UBSan, use RTC instead
                mode_flags = ["/Od", "/Zi", "/DDEBUG", "/RTC1"]
            elif mode == "release-O1":
                mode_flags = ["/O1", "/DNDEBUG"]
            elif mode == "release-O2":
                mode_flags = ["/O2", "/DNDEBUG"]
            elif mode == "release-O3":
                mode_flags = ["/Ox", "/GL", "/DNDEBUG"]
            elif mode == "release-Ofast":
                mode_flags = ["/Ox", "/GL", "/fp:fast", "/DNDEBUG"]
            else:
                # Default release
                mode_flags = ["/O2", "/DNDEBUG"]
            
            # MSVC uses /Fe: for output
            cmd = [compiler_cmd] + common_flags + mode_flags + [source_str, f"/Fe:{output_str}"]
        else:
            # GCC/Clang/Intel flags
            common_flags = ["-std=c++20", "-Wall", "-Wextra", "-pedantic", "-I./include"]
            
            if needs_pthread and compiler_name in ["gcc", "clang", "intel"]:
                common_flags.append("-pthread")
            
            if mode == "debug":
                mode_flags = ["-O0", "-g", "-DDEBUG"]
            elif mode == "debug-asan":
                mode_flags = ["-O0", "-g", "-DDEBUG", "-fsanitize=address", "-fno-omit-frame-pointer"]
            elif mode == "debug-ubsan":
                mode_flags = ["-O0", "-g", "-DDEBUG", "-fsanitize=undefined", "-fno-omit-frame-pointer"]
            elif mode == "release-O1":
                mode_flags = ["-O1", "-DNDEBUG"]
            elif mode == "release-O2":
                mode_flags = ["-O2", "-DNDEBUG"]
            elif mode == "release-O3":
                mode_flags = ["-O3", "-fexpensive-optimizations", "-funroll-loops", "-ftree-vectorize", "-march=native", "-DNDEBUG"]
            elif mode == "release-Ofast":
                mode_flags = ["-Ofast", "-fexpensive-optimizations", "-funroll-loops", "-ftree-vectorize", "-ffast-math", "-march=native", "-DNDEBUG"]
            else:
                # Default release
                mode_flags = ["-O3", "-DNDEBUG"]
            
            cmd = [compiler_cmd] + common_flags + mode_flags + [source_str, "-o", output_str]
            
            # Add linker flags after -o output
            if needs_atomic and compiler_name in ["gcc", "clang"]:
                cmd.append("-latomic")
            
            # Add sanitizer linker flags
            if mode == "debug-asan" and compiler_name in ["gcc", "clang"]:
                cmd.append("-fsanitize=address")
            elif mode == "debug-ubsan" and compiler_name in ["gcc", "clang"]:
                cmd.append("-fsanitize=undefined")
        
        echo_info(f"  Compiling [{mode}]...")
        
        # Print command if requested
        if print_commands:
            echo_header(f"    $ {' '.join(cmd)}")
        
        # Execute compilation
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                cwd=project_root,  # Use project root from global scope
                env=env  # Use isolated compiler environment
            )
            
            # Check if compilation succeeded
            output_check = Path(output_str)  # Use the string path for checking
            if result.returncode == 0 or output_check.exists():
                echo_success(f"  {compiler_name} [{mode}]: {output_str}")
            else:
                echo_error(f"  {compiler_name} [{mode}]: compilation failed")
                echo_error(f"    Return code: {result.returncode}")
                if result.stderr:
                    print(f"\nSTDERR:\n{result.stderr}")
                if result.stdout:
                    print(f"\nSTDOUT:\n{result.stdout}")
        except Exception as e:
            echo_error(f"  {compiler_name} [{mode}]: {str(e)}")
    
    print()


def main():
    """Main function"""
    
    # Parse arguments
    if len(sys.argv) < 4:
        print("Error: Se requieren al menos 3 argumentos")
        print(f"Uso: {sys.argv[0]} <type> <feature> <target> [compiler] [mode] [print]")
        print()
        print("Tests/Benchs:")
        print(f"  python {sys.argv[0]} uint128 bits tests")
        print(f"  python {sys.argv[0]} int128 numeric benchs gcc release")
        print()
        print("Demos:")
        print(f"  python {sys.argv[0]} demos tutorials 01_basic_operations gcc release")
        print(f"  python {sys.argv[0]} demos examples ipv6_address clang debug")
        sys.exit(1)
    
    type_name = sys.argv[1]
    feature = sys.argv[2]
    target = sys.argv[3]
    compiler = sys.argv[4] if len(sys.argv) > 4 else "all"
    mode = sys.argv[5] if len(sys.argv) > 5 else "all"
    print_commands = (sys.argv[6].lower() == "yes") if len(sys.argv) > 6 else False
    
    # Determine if this is a demo build or tests/benchs build
    is_demo = (type_name == "demos")
    
    if is_demo:
        # For demos: feature=category, target=demo_name
        category = feature
        demo_name = target
        
        # Validation for demos
        valid_categories = ["general", "tutorials", "examples", "showcase", 
                           "comparison", "performance", "integration"]
        if category not in valid_categories:
            print(f"Error: CATEGORY debe ser uno de: {', '.join(valid_categories)}")
            sys.exit(1)
    else:
        # Validation for tests/benchs
        if type_name not in ["uint128", "int128"]:
            print("Error: TYPE debe ser 'uint128' o 'int128'")
            sys.exit(1)
        
        valid_features = ["t", "tt", "traits", "limits", "concepts", "algorithm", "algorithms", "iostreams",
                          "bits", "cmath", "numeric", "ranges", "format", "safe",
                          "thread_safety", "comparison_boost", "interop"]
        if feature not in valid_features:
            print(f"Error: FEATURE debe ser uno de: {', '.join(valid_features)}")
            sys.exit(1)
        
        if target not in ["tests", "benchs"]:
            print("Error: TARGET debe ser 'tests' o 'benchs'")
            sys.exit(1)
    
    # Common validation
    valid_compilers = ["gcc", "clang", "intel", "msvc", "all"]
    if compiler not in valid_compilers:
        print(f"Error: COMPILER debe ser uno de: {', '.join(valid_compilers)}")
        sys.exit(1)
    
    valid_modes = ["debug", "debug-asan", "debug-ubsan", "release", "release-O1", "release-O2", "release-O3", "release-Ofast", "all"]
    if mode not in valid_modes:
        print(f"Error: MODE debe ser uno de: {', '.join(valid_modes)}")
        sys.exit(1)
    
    # Configuration
    project_root = Path(__file__).parent.parent
    os.chdir(project_root)
    
    # Determine source file and build directory
    if is_demo:
        source_file = f"demos/{category}/{demo_name}.cpp"
        build_dir = "build/build_demos"
        output_suffix = demo_name  # Output name is the demo name
        echo_info(f"Building demo: {category}/{demo_name}...")
    elif target == "tests":
        source_file = f"tests/{type_name}_{feature}_extracted_tests.cpp"
        build_dir = "build/build_tests"
        output_suffix = "tests"
        echo_info(f"Building {type_name} {feature} {target} for all compilers...")
    else:  # benchs
        source_file = f"benchs/{type_name}_{feature}_extracted_benchs.cpp"
        build_dir = "build/build_benchs"
        output_suffix = "benchs"
        echo_info(f"Building {type_name} {feature} {target} for all compilers...")
    
    # Check source file
    if not Path(source_file).exists():
        echo_error(f"Source file not found: {source_file}")
        sys.exit(1)
    
    print()
    
    # Define modes to compile
    modes_to_compile = ["debug", "debug-asan", "debug-ubsan", "release", "release-O1", "release-O2", "release-O3", "release-Ofast"] if mode == "all" else [mode]
    
    # Get compiler commands from environment or use defaults
    gcc_cmd = os.environ.get("GCC_CXX", "g++")
    clang_cmd = os.environ.get("CLANG_CXX", "clang++")
    intel_cmd = os.environ.get("INTEL_CXX", "icx")
    msvc_cmd = os.environ.get("MSVC_CXX", "cl.exe")
    
    # For demos, we need to pass empty strings for type_name and feature
    # since they don't apply
    if is_demo:
        type_name_arg = ""
        feature_arg = ""
    else:
        type_name_arg = type_name
        feature_arg = feature
    
    # Compile with each compiler
    if compiler in ["gcc", "all"]:
        compile_with_compiler(
            "gcc", gcc_cmd, source_file, build_dir,
            type_name_arg, feature_arg, output_suffix, modes_to_compile,
            print_commands, skip_check=True, project_root=project_root
        )
    
    if compiler in ["clang", "all"]:
        compile_with_compiler(
            "clang", clang_cmd, source_file, build_dir,
            type_name_arg, feature_arg, output_suffix, modes_to_compile,
            print_commands, project_root=project_root
        )
    
    if compiler in ["intel", "all"]:
        compile_with_compiler(
            "intel", intel_cmd, source_file, build_dir,
            type_name_arg, feature_arg, output_suffix, modes_to_compile,
            print_commands, project_root=project_root
        )
    
    if compiler in ["msvc", "all"]:
        compile_with_compiler(
            "msvc", msvc_cmd, source_file, build_dir,
            type_name_arg, feature_arg, output_suffix, modes_to_compile,
            print_commands, project_root=project_root
        )
    
    # Summary
    if is_demo:
        echo_success(f"Build complete for demo {category}/{demo_name}!")
    else:
        echo_success(f"Build complete for {type_name} {feature} {target}!")


if __name__ == "__main__":
    main()
