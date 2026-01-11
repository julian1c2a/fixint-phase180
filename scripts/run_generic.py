#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
run_generic.py - Script genérico para ejecutar benchmarks y demos
=========================================================

Ejecuta benchmarks o demos compilados y muestra resultados

Uso:
    # Para benchs (sintaxis clásica):
    python run_generic.py <type> <feature> [compiler] [mode]
    
    # Para demos (nueva sintaxis):
    python run_generic.py demos <category> <demo_name> [compiler] [mode] [args...]

Argumentos:
    type     : uint128 | int128 | demos
    feature  : bits | numeric | algorithm | etc. (o <category> si type=demos)
    compiler : gcc | clang | intel | msvc | all (default: all)
    mode     : debug | release | all (default: all)
    args...  : argumentos adicionales para la demo (solo para demos)

Ejemplos Benchs:
    python run_generic.py uint128 bits
    python run_generic.py int128 numeric gcc release

Ejemplos Demos:
    python run_generic.py demos tutorials 01_basic_operations gcc release
    python run_generic.py demos examples uuid_generation gcc release 100
"""

import sys
import os
import subprocess
import time
from pathlib import Path
from typing import Dict, List, Optional

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


def run_demo(executable: Path, compiler: str, demo_args: List[str] = None) -> bool:
    """Ejecuta una demo y muestra su output"""
    
    if not executable.exists():
        return False
    
    try:
        # Obtener entorno aislado si está disponible
        if USE_COMPILER_ENV:
            comp_env = CompilerEnvironment(compiler)
            env = comp_env.get_env()
        else:
            env = os.environ.copy()
        
        # Preparar comando con argumentos
        cmd = [str(executable)]
        if demo_args:
            cmd.extend(demo_args)
        
        # Ejecutar demo mostrando output en tiempo real
        result = subprocess.run(
            cmd,
            text=True,
            timeout=300,  # 5 minutos máximo
            env=env
        )
        
        return (result.returncode == 0)
    
    except subprocess.TimeoutExpired:
        echo_error(f"Timeout ejecutando {executable.name}")
        return False
    except Exception as e:
        echo_error(f"Error ejecutando {executable.name}: {e}")
        return False


def run_benchmark(executable: Path, compiler: str) -> Optional[float]:
    """Ejecuta un benchmark y retorna el tiempo de ejecución en segundos"""
    
    if not executable.exists():
        return None
    
    try:
        # Obtener entorno aislado si está disponible
        if USE_COMPILER_ENV:
            comp_env = CompilerEnvironment(compiler)
            env = comp_env.get_env()
        else:
            env = os.environ.copy()
        
        start_time = time.time()
        
        result = subprocess.run(
            [str(executable)],
            capture_output=True,
            text=True,
            timeout=300,  # 5 minutos máximo
            env=env
        )
        
        elapsed = time.time() - start_time
        
        if result.returncode == 0:
            return elapsed
        else:
            return None
    
    except subprocess.TimeoutExpired:
        echo_error(f"Timeout ejecutando {executable.name}")
        return None
    except Exception as e:
        echo_error(f"Error ejecutando {executable.name}: {e}")
        return None


def format_time(seconds: Optional[float]) -> str:
    """Formatea el tiempo de ejecución"""
    if seconds is None:
        return "FAIL"
    
    if seconds < 1.0:
        return f"{seconds*1000:.2f} ms"
    else:
        return f"{seconds:.3f} s"


def main():
    if len(sys.argv) < 3:
        print("Error: Se requieren al menos 2 argumentos")
        print(f"Uso: {sys.argv[0]} <type> <feature> [compiler] [mode] [args...]")
        print()
        print("Benchs:")
        print(f"  python {sys.argv[0]} uint128 bits")
        print(f"  python {sys.argv[0]} int128 numeric gcc release")
        print()
        print("Demos:")
        print(f"  python {sys.argv[0]} demos tutorials 01_basic_operations gcc release")
        print(f"  python {sys.argv[0]} demos examples uuid_generation gcc release 100")
        sys.exit(1)
    
    type_name = sys.argv[1]
    
    # Determine if this is a demo execution
    is_demo = (type_name == "demos")
    
    # Configuration
    project_root = Path(__file__).parent.parent
    
    if is_demo:
        # Demos syntax: demos <category> <demo_name> [compiler] [mode] [args...]
        if len(sys.argv) < 4:
            print("Error: Demos requieren: demos <category> <demo_name> [compiler] [mode] [args...]")
            sys.exit(1)
        
        category = sys.argv[2]
        demo_name = sys.argv[3]
        compiler = sys.argv[4] if len(sys.argv) > 4 else "gcc"
        mode = sys.argv[5] if len(sys.argv) > 5 else "release"
        demo_args = sys.argv[6:] if len(sys.argv) > 6 else []
        
        build_dir = project_root / "build" / "build_demos"
        
        echo_header("=" * 70)
        echo_header(f"  DEMO EXECUTION: {category}/{demo_name}")
        echo_header("=" * 70)
        if demo_args:
            echo_info(f"Arguments: {' '.join(demo_args)}")
        print()
        
        # Build executable path
        exe_name = demo_name
        # In Windows, all compilers produce .exe
        if sys.platform == "win32":
            exe_name += ".exe"
        elif compiler in ["msvc", "intel"]:
            exe_name += ".exe"
        
        exe_path = build_dir / compiler / mode / exe_name
        
        echo_info(f"Compiler: {compiler} [{mode}]")
        echo_info(f"Executable: {exe_path}")
        print()
        echo_header("-" * 70)
        print()
        
        if exe_path.exists():
            success = run_demo(exe_path, compiler, demo_args)
            print()
            echo_header("-" * 70)
            
            if success:
                echo_success("Demo executed successfully!")
            else:
                echo_error("Demo failed!")
                sys.exit(1)
        else:
            echo_error(f"Executable not found: {exe_path}")
            echo_info("¿Necesitas compilarlo primero?")
            echo_info(f"  python scripts/build_generic.py demos {category} {demo_name} {compiler} {mode}")
            sys.exit(1)
    
    else:
        # Original benchmark execution code
        feature = sys.argv[2]
        compiler_arg = sys.argv[3] if len(sys.argv) > 3 else "all"
        mode_arg = sys.argv[4] if len(sys.argv) > 4 else "release"
        
        # Validation
        if type_name not in ["uint128", "int128"]:
            print("Error: TYPE debe ser 'uint128' o 'int128'")
            sys.exit(1)
        
        valid_features = ["t", "tt", "traits", "limits", "concepts", "algorithms", "iostreams",
                          "bits", "cmath", "numeric", "ranges", "format", "safe",
                          "thread_safety", "comparison_boost", "interop"]
        if feature not in valid_features:
            print(f"Error: FEATURE debe ser uno de: {', '.join(valid_features)}")
            sys.exit(1)
        
        build_dir = project_root / "build" / "build_benchs"
        
        # Determine compilers and modes to test
        compilers = ["gcc", "clang", "intel", "msvc"] if compiler_arg == "all" else [compiler_arg]
        modes = ["debug", "release"] if mode_arg == "all" else [mode_arg]
        
        echo_header("=" * 60)
        echo_header(f"  BENCHMARK EXECUTION: {type_name}_{feature}_benchs")
        echo_header("=" * 60)
        print()
        
        # Track results
        results: Dict[str, Dict[str, Optional[float]]] = {}
        
        for compiler in compilers:
            results[compiler] = {}
            
            for mode in modes:
                exe_name = f"{type_name}_{feature}_benchs_{compiler}"
                if compiler == "msvc":
                    exe_name += ".exe"
                
                exe_path = build_dir / compiler / mode / exe_name
                
                echo_info(f"Running {compiler} [{mode}]...")
                
                if exe_path.exists():
                    elapsed = run_benchmark(exe_path, compiler)
                    results[compiler][mode] = elapsed
                    
                    if elapsed is not None:
                        echo_success(f"  {compiler} [{mode}]: {format_time(elapsed)}")
                    else:
                        echo_error(f"  {compiler} [{mode}]: FAILED")
                else:
                    echo_error(f"  {compiler} [{mode}]: NOT FOUND")
                    echo_info(f"     Expected: {exe_path}")
                    results[compiler][mode] = None
        
        # Summary table
        print()
        echo_header("=" * 60)
        echo_header("  RESULTS SUMMARY")
        echo_header("=" * 60)
        print()
        print(f"{'Compiler':<12} | {'Debug':<15} | {'Release':<15}")
        print("-" * 47)
        
        for compiler in compilers:
            debug_time = format_time(results[compiler].get("debug"))
            release_time = format_time(results[compiler].get("release"))
            
            if mode_arg != "all":
                if mode_arg == "debug":
                    print(f"{compiler:<12} | {debug_time:<15} | {'- N/A':<15}")
                else:
                    print(f"{compiler:<12} | {'- N/A':<15} | {release_time:<15}")
            else:
                print(f"{compiler:<12} | {debug_time:<15} | {release_time:<15}")
        
        print()
        
        # Calculate success rate
        total_runs = len([r for comp in results.values() for r in comp.values()])
        successful_runs = len([r for comp in results.values() for r in comp.values() if r is not None])
        
        if total_runs > 0:
            success_rate = (successful_runs / total_runs) * 100
            echo_info(f"Success rate: {successful_runs}/{total_runs} ({success_rate:.1f}%)")
        
        sys.exit(0 if successful_runs == total_runs else 1)


if __name__ == "__main__":
    sys.exit(main())
