#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_generic.py - Script genérico para ejecutar tests y verificar demos
=========================================================================

Ejecuta tests compilados o verifica compilación de demos

Uso:
    # Para tests (sintaxis clásica):
    python check_generic.py <type> <feature> [compiler] [mode]
    
    # Para demos (verifica compilación):
    python check_generic.py demos <category> [compiler] [mode]

Argumentos:
    type     : uint128 | int128 | demos
    feature  : bits | numeric | algorithm | etc. (o <category> si type=demos)
    compiler : gcc | clang | intel | msvc | all (default: all)
    mode     : debug | release | all (default: all)

Ejemplos Tests:
    python check_generic.py uint128 bits
    python check_generic.py int128 numeric gcc release

Ejemplos Demos:
    python check_generic.py demos tutorials gcc release
    python check_generic.py demos all
"""

import sys
import os
import subprocess
from pathlib import Path
from typing import Dict, List

# Force UTF-8 encoding for Windows
if sys.platform == "win32":
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

# Add env_setup to path
sys.path.insert(0, str(Path(__file__).parent / "env_setup"))

try:
    from compiler_env import CompilerEnvironment
    USE_COMPILER_ENV = True
except ImportError:
    USE_COMPILER_ENV = False


class Colors:
    GREEN = '\033[0;32m'
    RED = '\033[0;31m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'


def echo_success(msg: str):
    print(f"{Colors.GREEN}[OK] {msg}{Colors.NC}")

def echo_error(msg: str):
    print(f"{Colors.RED}[ERROR] {msg}{Colors.NC}")

def echo_info(msg: str):
    print(f"{Colors.YELLOW}-> {msg}{Colors.NC}")

def echo_header(msg: str):
    print(f"{Colors.BLUE}{msg}{Colors.NC}")


def run_test(executable: Path, compiler: str) -> bool:
    """Ejecuta un test y retorna True si pasa"""
    
    if not executable.exists():
        return False
    
    try:
        # Obtener entorno aislado si está disponible
        if USE_COMPILER_ENV:
            comp_env = CompilerEnvironment(compiler)
            env = comp_env.get_env()
        else:
            env = os.environ.copy()
        
        result = subprocess.run(
            [str(executable)],
            capture_output=True,
            text=True,
            timeout=30,
            env=env
        )
        
        return result.returncode == 0
    
    except subprocess.TimeoutExpired:
        echo_error(f"Timeout ejecutando {executable.name}")
        return False
    except Exception as e:
        echo_error(f"Error ejecutando {executable.name}: {e}")
        return False


def check_demo_compilation(category: str, demo_file: Path, compiler: str, mode: str, project_root: Path) -> bool:
    """Verifica que una demo compile correctamente"""
    
    demo_name = demo_file.stem  # nombre sin .cpp
    
    # Intentar compilar
    try:
        # Obtener entorno aislado si está disponible
        if USE_COMPILER_ENV:
            comp_env = CompilerEnvironment(compiler)
            env = comp_env.get_env()
        else:
            env = os.environ.copy()
        
        # Build directory
        build_dir = project_root / "build" / "build_demos" / compiler / mode
        build_dir.mkdir(parents=True, exist_ok=True)
        
        # Output executable
        output = build_dir / demo_name
        if sys.platform == "win32":
            output = output.with_suffix(".exe")
        
        # Compiler command
        compiler_cmd = compiler + "++" if compiler in ["g", "clang"] else compiler
        if compiler == "gcc":
            compiler_cmd = os.environ.get("GCC_CXX", "g++")
        elif compiler == "clang":
            compiler_cmd = os.environ.get("CLANG_CXX", "clang++")
        elif compiler == "intel":
            compiler_cmd = os.environ.get("INTEL_CXX", "icx")
        elif compiler == "msvc":
            compiler_cmd = os.environ.get("MSVC_CXX", "cl.exe")
        
        # Flags
        common_flags = ["-std=c++20", f"-I{project_root}/include"]
        mode_flags = ["-O0", "-g"] if mode == "debug" else ["-O3", "-DNDEBUG"]
        link_flags = []
        
        # Check if source file uses threading (for pthread and atomic flags)
        try:
            with open(demo_file, 'r', encoding='utf-8') as f:
                content = f.read(4000)
                if ('<thread>' in content or 'std::thread' in content or 'pthread' in content):
                    common_flags.append("-pthread")
                if ('<atomic>' in content or 'std::atomic' in content or 
                    'atomic_' in content or 'thread_safety.hpp' in content):
                    if compiler in ["gcc", "clang"]:
                        link_flags.append("-latomic")
        except:
            pass
        
        cmd = [compiler_cmd] + common_flags + mode_flags + [str(demo_file), "-o", str(output)] + link_flags
        
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=60,
            cwd=project_root,
            env=env
        )
        
        # Check if compilation succeeded
        return result.returncode == 0 and output.exists()
    
    except subprocess.TimeoutExpired:
        return False
    except Exception as e:
        return False


def main():
    if len(sys.argv) < 2:
        print("Error: Se requieren al menos 1 argumento")
        print(f"Uso: {sys.argv[0]} <type> <feature> [compiler] [mode]")
        print()
        print("Tests:")
        print(f"  python {sys.argv[0]} uint128 bits")
        print(f"  python {sys.argv[0]} int128 numeric gcc release")
        print()
        print("Demos (verifica compilación):")
        print(f"  python {sys.argv[0]} demos tutorials gcc release")
        print(f"  python {sys.argv[0]} demos all")
        sys.exit(1)
    
    type_name = sys.argv[1]
    
    # Determine if this is demo verification
    is_demo = (type_name == "demos")
    
    # Configuration
    project_root = Path(__file__).parent.parent
    
    if is_demo:
        # Demos verification: demos <category> [compiler] [mode]
        category = sys.argv[2] if len(sys.argv) > 2 else "all"
        compiler_arg = sys.argv[3] if len(sys.argv) > 3 else "all"
        mode_arg = sys.argv[4] if len(sys.argv) > 4 else "release"
        
        # Valid categories
        all_categories = ["general", "tutorials", "examples", "showcase", 
                         "comparison", "performance", "integration"]
        
        if category == "all":
            categories = all_categories
        elif category in all_categories:
            categories = [category]
        else:
            print(f"Error: CATEGORY debe ser 'all' o uno de: {', '.join(all_categories)}")
            sys.exit(1)
        
        # Determine compilers and modes
        compilers = ["gcc", "clang", "intel", "msvc"] if compiler_arg == "all" else [compiler_arg]
        modes = ["debug", "release"] if mode_arg == "all" else [mode_arg]
        
        echo_header("=" * 70)
        echo_header("  DEMOS COMPILATION CHECK")
        echo_header("=" * 70)
        print()
        
        # Track results: {category: {demo_name: {compiler: {mode: bool}}}}
        total_checks = 0
        passed_checks = 0
        failed_demos = []
        
        for cat in categories:
            demos_dir = project_root / "demos" / cat
            
            if not demos_dir.exists():
                echo_info(f"Category '{cat}' not found, skipping...")
                continue
            
            # Get all .cpp files in category
            demo_files = list(demos_dir.glob("*.cpp"))
            
            if not demo_files:
                echo_info(f"No demos in category '{cat}', skipping...")
                continue
            
            echo_header(f"--- Category: {cat} ({len(demo_files)} demos) ---")
            print()
            
            for demo_file in sorted(demo_files):
                demo_name = demo_file.stem
                
                for compiler in compilers:
                    for mode in modes:
                        total_checks += 1
                        
                        echo_info(f"  Checking {cat}/{demo_name} [{compiler}, {mode}]...")
                        
                        if check_demo_compilation(cat, demo_file, compiler, mode, project_root):
                            echo_success(f"    ✓ OK")
                            passed_checks += 1
                        else:
                            echo_error(f"    ✗ FAIL")
                            failed_demos.append(f"{cat}/{demo_name} [{compiler}, {mode}]")
            
            print()
        
        # Summary
        echo_header("=" * 70)
        echo_header("  SUMMARY")
        echo_header("=" * 70)
        print()
        echo_info(f"Total checks: {total_checks}")
        echo_success(f"Passed: {passed_checks}")
        
        if failed_demos:
            echo_error(f"Failed: {len(failed_demos)}")
            print()
            echo_header("Failed demos:")
            for failed in failed_demos:
                print(f"  - {failed}")
        else:
            echo_success("All demos compiled successfully!")
        
        print()
        
        return 0 if passed_checks == total_checks else 1
    
    else:
        # Original test execution code
        if len(sys.argv) < 3:
            print("Error: Tests requieren al menos 2 argumentos")
            print(f"Uso: {sys.argv[0]} <type> <feature> [compiler] [mode]")
            sys.exit(1)
        
        feature = sys.argv[2]
        compiler_arg = sys.argv[3] if len(sys.argv) > 3 else "all"
        mode_arg = sys.argv[4] if len(sys.argv) > 4 else "all"
        
        # Validation
        if type_name not in ["uint128", "int128"]:
            print("Error: TYPE debe ser 'uint128' o 'int128'")
            sys.exit(1)
        
        build_dir = project_root / "build" / "build_tests"
        
        # Determine compilers and modes to test
        compilers = ["gcc", "clang", "intel", "msvc"] if compiler_arg == "all" else [compiler_arg]
        modes = ["debug", "release"] if mode_arg == "all" else [mode_arg]
        
        echo_header("=" * 60)
        echo_header(f"  TEST EXECUTION: {type_name}_{feature}_tests")
        echo_header("=" * 60)
        print()
        
        # Track results
        results: Dict[str, Dict[str, bool]] = {}
        
        for compiler in compilers:
            results[compiler] = {}
            
            for mode in modes:
                exe_name = f"{type_name}_{feature}_tests_{compiler}"
                if compiler == "msvc":
                    exe_name += ".exe"
                
                exe_path = build_dir / compiler / mode / exe_name
                
                # On Windows, GCC/Clang may generate .exe
                if not exe_path.exists() and sys.platform == "win32":
                    exe_path_with_ext = exe_path.with_suffix(".exe")
                    if exe_path_with_ext.exists():
                        exe_path = exe_path_with_ext
                
                echo_info(f"Testing {compiler} [{mode}]...")
                
                if exe_path.exists():
                    passed = run_test(exe_path, compiler)
                    results[compiler][mode] = passed
                    
                    if passed:
                        echo_success(f"  {compiler} [{mode}]: PASS")
                    else:
                        echo_error(f"  {compiler} [{mode}]: FAIL")
                else:
                    echo_error(f"  {compiler} [{mode}]: NOT FOUND")
                    echo_info(f"     Expected: {exe_path}")
                    results[compiler][mode] = False
        
        # Summary table
        print()
        echo_header("=" * 60)
        echo_header("  RESULTS SUMMARY")
        echo_header("=" * 60)
        print()
        print(f"{'Compiler':<12} | {'Debug':<12} | {'Release':<12}")
        print("-" * 42)
        
        for compiler in compilers:
            debug_result = "[OK] PASS" if results[compiler].get("debug", False) else "[ERROR] FAIL"
            release_result = "[OK] PASS" if results[compiler].get("release", False) else "[ERROR] FAIL"
            
            if mode_arg != "all":
                if mode_arg == "debug":
                    print(f"{compiler:<12} | {debug_result:<12} | {'- N/A':<12}")
                else:
                    print(f"{compiler:<12} | {'- N/A':<12} | {release_result:<12}")
            else:
                print(f"{compiler:<12} | {debug_result:<12} | {release_result:<12}")
        
        print()
        
        # Calculate pass rate
        total_tests = len([r for comp in results.values() for r in comp.values()])
        passed_tests = len([r for comp in results.values() for r in comp.values() if r])
        
        echo_header(f"Pass rate: {passed_tests}/{total_tests}")
        
        return 0 if passed_tests == total_tests else 1


if __name__ == "__main__":
    sys.exit(main())
