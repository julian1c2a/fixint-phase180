#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
make.py - Sistema de build centralizado para int128 project
============================================================

Reemplaza Makefile tradicional con lógica Python y entornos aislados.

COMANDOS PRINCIPALES:
    init                Detecta y configura compiladores
    build <args>        Compila tests, benchmarks o demos
    check <args>        Ejecuta tests o verifica demos
    run <args>          Ejecuta benchmarks o demos
    clean               Limpia directorios de build
    
TARGETS ESPECIALES:
    all                 Compila todo (tests + benchs) con todos los compiladores
    test                Compila y ejecuta todos los tests
    bench               Compila y ejecuta todos los benchmarks
    demo <args>         Compila y ejecuta un demo específico
    list                Lista todas las combinaciones disponibles

EJEMPLOS:
    python make.py init
    python make.py build uint128 bits tests gcc release
    python make.py check uint128 bits
    python make.py test
    python make.py all
    python make.py clean
    
    # Demos:
    python make.py build demos tutorials 01_basic_operations gcc release
    python make.py run demos tutorials 01_basic_operations gcc release
    python make.py check demos tutorials
    python make.py demo tutorials 01_basic_operations

ARGUMENTOS PARA BUILD/CHECK/RUN:
    <type>      uint128 | int128 | demos
    <feature>   bits | numeric | algorithm | etc. (o <category> si type=demos)
    <target>    tests | benchs | <demo_name> (si type=demos)
    <compiler>  gcc | clang | intel | msvc | all
    <mode>      debug | release | all
"""

import sys
import os
import subprocess
import shutil
import argparse
from pathlib import Path
from typing import List, Dict, Optional, Tuple
import json
import time

# Force UTF-8 encoding for Windows
if sys.platform == "win32":
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

# Paths
PROJECT_ROOT = Path(__file__).parent
SCRIPTS_DIR = PROJECT_ROOT / "scripts"
BUILD_DIR = PROJECT_ROOT / "build"
TESTS_DIR = PROJECT_ROOT / "tests"
BENCHS_DIR = PROJECT_ROOT / "benchs"
DEMOS_DIR = PROJECT_ROOT / "demos"
COMPILER_ENVS_DIR = BUILD_DIR / "compiler_envs"

# Colors
class Colors:
    GREEN = '\033[0;32m'
    RED = '\033[0;31m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    CYAN = '\033[0;36m'
    MAGENTA = '\033[0;35m'
    NC = '\033[0m'

def echo_success(msg: str):
    print(f"{Colors.GREEN}[OK] {msg}{Colors.NC}")

def echo_error(msg: str):
    print(f"{Colors.RED}[ERROR] {msg}{Colors.NC}")

def echo_info(msg: str):
    print(f"{Colors.CYAN}[INFO] {msg}{Colors.NC}")

def echo_header(msg: str):
    print(f"{Colors.BLUE}{msg}{Colors.NC}")

def echo_warning(msg: str):
    print(f"{Colors.YELLOW}[WARN] {msg}{Colors.NC}")


def run_script(script_name: str, args: List[str]) -> int:
    """Ejecuta un script Python de scripts/"""
    script_path = SCRIPTS_DIR / script_name
    
    if not script_path.exists():
        echo_error(f"Script no encontrado: {script_path}")
        return 1
    
    cmd = [sys.executable, str(script_path)] + args
    result = subprocess.run(cmd, cwd=PROJECT_ROOT)
    return result.returncode


def cmd_init(args: argparse.Namespace) -> int:
    """Inicializa el proyecto detectando compiladores"""
    echo_header("=" * 60)
    echo_header("  INICIALIZACIÓN DEL PROYECTO")
    echo_header("=" * 60)
    print()
    
    return run_script("init_project.py", [])


def cmd_build(args: argparse.Namespace) -> int:
    """Compila tests, benchmarks o demos"""
    # Detectar si es demo (type=demos)
    is_demo = (args.type == "demos")
    
    if is_demo:
        # Para demos: type=demos, feature=category, target=demo_name
        if not args.type or not args.feature or not args.target:
            echo_error("build demos requiere: demos <category> <demo_name> [compiler] [mode]")
            return 1
    else:
        # Para tests/benchs: type=uint128/int128, feature=bits/etc, target=tests/benchs
        if not args.type or not args.feature or not args.target:
            echo_error("build requiere: <type> <feature> <target> [compiler] [mode]")
            return 1
    
    build_args = [
        args.type,
        args.feature,
        args.target,
        args.compiler or "all",
        args.mode or "all"
    ]
    
    if args.verbose:
        build_args.append("yes")
    
    return run_script("build_generic.py", build_args)


def cmd_check(args: argparse.Namespace) -> int:
    """Ejecuta tests o verifica compilación de demos"""
    is_demo = (args.type == "demos")
    
    if is_demo:
        # Para demos: verifica que compilen correctamente
        if not args.type or not args.feature:
            echo_error("check demos requiere: demos <category|all> [compiler] [mode]")
            return 1
    else:
        # Para tests: ejecuta los tests
        if not args.type or not args.feature:
            echo_error("check requiere: <type> <feature> [compiler] [mode]")
            return 1
    
    check_args = [
        args.type,
        args.feature,
        args.compiler or "all",
        args.mode or "all"
    ]
    
    return run_script("check_generic.py", check_args)


def cmd_run(args: argparse.Namespace) -> int:
    """Ejecuta benchmarks o demos"""
    is_demo = (args.type == "demos")
    
    if is_demo:
        # Para demos: type=demos, feature=category, target=demo_name (en extra_args)
        if not args.type or not args.feature:
            echo_error("run demos requiere: demos <category> <demo_name> [compiler] [mode] [args...]")
            return 1
        
        # El demo_name estará en extra_args[0] si se proporcionó
        if not hasattr(args, 'extra_args') or not args.extra_args:
            echo_error("run demos requiere el nombre del demo")
            return 1
        
        demo_name = args.extra_args[0]
        demo_args = args.extra_args[1:] if len(args.extra_args) > 1 else []
        
        run_args = [
            args.type,           # demos
            args.feature,        # category
            demo_name,          # demo_name
            args.compiler or "all",
            args.mode or "release"
        ] + demo_args
    else:
        # Para benchmarks: type=uint128/int128, feature=bits/etc
        if not args.type or not args.feature:
            echo_error("run requiere: <type> <feature> [compiler] [mode]")
            return 1
        
        run_args = [
            args.type,
            args.feature,
            args.compiler or "all",
            args.mode or "release"  # Default to release for benchmarks
        ]
    
    return run_script("run_generic.py", run_args)


def cmd_clean(args: argparse.Namespace) -> int:
    """Limpia directorios de build"""
    echo_info("Limpiando directorios de build...")
    
    # Limpiar build_tests, build_benchs y build_demos
    cleaned = []
    
    build_tests = BUILD_DIR / "build_tests"
    if build_tests.exists():
        shutil.rmtree(build_tests)
        cleaned.append("build_tests/")
    
    build_benchs = BUILD_DIR / "build_benchs"
    if build_benchs.exists():
        shutil.rmtree(build_benchs)
        cleaned.append("build_benchs/")
    
    build_demos = BUILD_DIR / "build_demos"
    if build_demos.exists():
        shutil.rmtree(build_demos)
        cleaned.append("build_demos/")
    
    # Opcional: limpiar compiler_envs si se pide
    if args.all:
        if COMPILER_ENVS_DIR.exists():
            shutil.rmtree(COMPILER_ENVS_DIR)
            cleaned.append("compiler_envs/")
    
    if cleaned:
        echo_success(f"Limpiado: {', '.join(cleaned)}")
    else:
        echo_info("Nada que limpiar")
    
    return 0


def get_all_combinations() -> List[Tuple[str, str]]:
    """Obtiene todas las combinaciones type-feature disponibles"""
    combinations = []
    
    for tests_file in TESTS_DIR.glob("*_*_extracted_tests.cpp"):
        name = tests_file.stem
        parts = name.replace("_extracted_tests", "").split("_", 1)
        if len(parts) == 2:
            combinations.append((parts[0], parts[1]))
    
    return sorted(set(combinations))


def cmd_all(args: argparse.Namespace) -> int:
    """Compila todo (tests + benchs) con todos los compiladores"""
    echo_header("=" * 60)
    echo_header("  COMPILACIÓN COMPLETA DEL PROYECTO")
    echo_header("=" * 60)
    print()
    
    combinations = get_all_combinations()
    
    if not combinations:
        echo_error("No se encontraron archivos de tests/benchs")
        return 1
    
    echo_info(f"Compilando {len(combinations)} combinaciones × 2 targets × 4 compiladores × 2 modos")
    echo_info("Esto puede tomar varios minutos...")
    print()
    
    total = len(combinations) * 2  # tests + benchs
    success = 0
    failed = []
    
    start_time = time.time()
    
    for i, (type_name, feature) in enumerate(combinations, 1):
        echo_header(f"\n[{i}/{len(combinations)}] {type_name}_{feature}")
        
        # Build tests
        echo_info(f"  Compilando tests...")
        ret = run_script("build_generic.py", [type_name, feature, "tests", "all", "all"])
        if ret == 0:
            success += 1
        else:
            failed.append(f"{type_name}_{feature}_tests")
        
        # Build benchs
        echo_info(f"  Compilando benchs...")
        ret = run_script("build_generic.py", [type_name, feature, "benchs", "all", "all"])
        if ret == 0:
            success += 1
        else:
            failed.append(f"{type_name}_{feature}_benchs")
    
    elapsed = time.time() - start_time
    
    # Resumen
    print()
    echo_header("=" * 60)
    echo_header("  RESUMEN DE COMPILACIÓN")
    echo_header("=" * 60)
    echo_success(f"Exitosas: {success}/{total}")
    
    if failed:
        echo_error(f"Fallidas: {len(failed)}")
        for item in failed:
            echo_error(f"  - {item}")
    
    echo_info(f"Tiempo total: {elapsed:.1f}s")
    
    return 0 if len(failed) == 0 else 1


def cmd_test(args: argparse.Namespace) -> int:
    """Compila y ejecuta todos los tests"""
    echo_header("=" * 60)
    echo_header("  EJECUCIÓN COMPLETA DE TESTS")
    echo_header("=" * 60)
    print()
    
    combinations = get_all_combinations()
    
    if not combinations:
        echo_error("No se encontraron archivos de tests")
        return 1
    
    total_tests = 0
    passed_tests = 0
    failed = []
    
    start_time = time.time()
    
    for i, (type_name, feature) in enumerate(combinations, 1):
        echo_header(f"\n[{i}/{len(combinations)}] {type_name}_{feature}")
        
        # Build
        echo_info("  Compilando...")
        ret = run_script("build_generic.py", [type_name, feature, "tests", args.compiler or "all", args.mode or "all"])
        
        if ret != 0:
            failed.append(f"{type_name}_{feature} (build failed)")
            continue
        
        # Check
        echo_info("  Ejecutando...")
        ret = run_script("check_generic.py", [type_name, feature, args.compiler or "all", args.mode or "all"])
        
        total_tests += 1
        if ret == 0:
            passed_tests += 1
        else:
            failed.append(f"{type_name}_{feature} (test failed)")
    
    elapsed = time.time() - start_time
    
    # Resumen
    print()
    echo_header("=" * 60)
    echo_header("  RESUMEN DE TESTS")
    echo_header("=" * 60)
    echo_success(f"Pasaron: {passed_tests}/{total_tests}")
    
    if failed:
        echo_error(f"Fallaron: {len(failed)}")
        for item in failed:
            echo_error(f"  - {item}")
    
    echo_info(f"Tiempo total: {elapsed:.1f}s")
    
    return 0 if len(failed) == 0 else 1


def cmd_bench(args: argparse.Namespace) -> int:
    """Compila y ejecuta todos los benchmarks"""
    echo_header("=" * 60)
    echo_header("  EJECUCIÓN COMPLETA DE BENCHMARKS")
    echo_header("=" * 60)
    print()
    
    combinations = get_all_combinations()
    
    if not combinations:
        echo_error("No se encontraron archivos de benchmarks")
        return 1
    
    total_benchs = 0
    success_benchs = 0
    failed = []
    
    start_time = time.time()
    
    for i, (type_name, feature) in enumerate(combinations, 1):
        echo_header(f"\n[{i}/{len(combinations)}] {type_name}_{feature}")
        
        # Build
        echo_info("  Compilando...")
        ret = run_script("build_generic.py", [type_name, feature, "benchs", args.compiler or "all", "release"])
        
        if ret != 0:
            failed.append(f"{type_name}_{feature} (build failed)")
            continue
        
        # Run
        echo_info("  Ejecutando...")
        ret = run_script("run_generic.py", [type_name, feature, args.compiler or "all", "release"])
        
        total_benchs += 1
        if ret == 0:
            success_benchs += 1
        else:
            failed.append(f"{type_name}_{feature} (benchmark failed)")
    
    elapsed = time.time() - start_time
    
    # Resumen
    print()
    echo_header("=" * 60)
    echo_header("  RESUMEN DE BENCHMARKS")
    echo_header("=" * 60)
    echo_success(f"Exitosos: {success_benchs}/{total_benchs}")
    
    if failed:
        echo_error(f"Fallaron: {len(failed)}")
        for item in failed:
            echo_error(f"  - {item}")
    
    echo_info(f"Tiempo total: {elapsed:.1f}s")
    
    return 0 if len(failed) == 0 else 1


def cmd_demo(args: argparse.Namespace) -> int:
    """Compila y ejecuta un demo específico"""
    if not args.category or not args.demo_name:
        echo_error("demo requiere: <category> <demo_name> [compiler] [mode] [args...]")
        return 1
    
    compiler = args.compiler or "gcc"
    mode = args.mode or "release"
    demo_args = args.extra_args if hasattr(args, 'extra_args') else []
    
    echo_header("=" * 60)
    echo_header(f"  DEMO: {args.category}/{args.demo_name}")
    echo_header("=" * 60)
    print()
    
    # Build
    echo_info("Compilando...")
    ret = run_script("build_generic.py", [
        "demos",
        args.category,
        args.demo_name,
        compiler,
        mode
    ])
    
    if ret != 0:
        echo_error("Compilación fallida")
        return 1
    
    print()
    
    # Run
    echo_info("Ejecutando...")
    run_args = [
        "demos",
        args.category,
        args.demo_name,
        compiler,
        mode
    ] + demo_args
    
    ret = run_script("run_generic.py", run_args)
    
    return ret


def cmd_sanitize(args: argparse.Namespace) -> int:
    """Compila con sanitizers"""
    echo_header("=" * 60)
    echo_header("  COMPILACIÓN CON SANITIZER")
    echo_header("=" * 60)
    print()
    
    type_name = args.type
    feature = args.feature
    sanitizer = args.sanitizer or 'asan'
    compiler = args.compiler or 'gcc'
    target = args.target or 'tests'
    
    echo_info(f"Tipo:       {type_name}")
    echo_info(f"Feature:    {feature}")
    echo_info(f"Sanitizer:  {sanitizer}")
    echo_info(f"Compilador: {compiler}")
    echo_info(f"Target:     {target}")
    print()
    
    # Ejecutar script bash
    script_path = SCRIPTS_DIR / "build_with_sanitizers.bash"
    if not script_path.exists():
        echo_error(f"Script no encontrado: {script_path}")
        return 1
    
    cmd = ["bash", str(script_path), type_name, feature, target, compiler, sanitizer]
    result = subprocess.run(cmd, cwd=PROJECT_ROOT)
    return result.returncode


def cmd_analyze(args: argparse.Namespace) -> int:
    """Ejecuta análisis estático"""
    echo_header("=" * 60)
    echo_header("  ANÁLISIS ESTÁTICO")
    echo_header("=" * 60)
    print()
    
    tool = args.tool or 'cppcheck'
    target = args.target or 'headers'
    
    echo_info(f"Herramienta: {tool}")
    echo_info(f"Target:      {target}")
    print()
    
    # Ejecutar script bash
    script_path = SCRIPTS_DIR / "run_static_analysis.bash"
    if not script_path.exists():
        echo_error(f"Script no encontrado: {script_path}")
        return 1
    
    cmd = ["bash", str(script_path), tool, target]
    result = subprocess.run(cmd, cwd=PROJECT_ROOT)
    return result.returncode


def cmd_compare(args: argparse.Namespace) -> int:
    """Ejecuta benchmark comparativo"""
    echo_header("=" * 60)
    echo_header("  BENCHMARK COMPARATIVO")
    echo_header("=" * 60)
    print()
    
    compiler = args.compiler or 'gcc'
    mode = args.mode or 'release-O3'
    iterations = args.iterations or '100000'
    
    echo_info(f"Compilador:   {compiler}")
    echo_info(f"Modo:         {mode}")
    echo_info(f"Iteraciones:  {iterations}")
    print()
    
    # Ejecutar script bash
    script_path = SCRIPTS_DIR / "benchmark_comparison.bash"
    if not script_path.exists():
        echo_error(f"Script no encontrado: {script_path}")
        return 1
    
    cmd = ["bash", str(script_path), compiler, mode, iterations]
    result = subprocess.run(cmd, cwd=PROJECT_ROOT)
    return result.returncode


def cmd_list(args: argparse.Namespace) -> int:
    """Lista todas las combinaciones disponibles"""
    echo_header("=" * 60)
    echo_header("  TESTS/BENCHMARKS DISPONIBLES")
    echo_header("=" * 60)
    print()
    
    combinations = get_all_combinations()
    
    if combinations:
        echo_info(f"Encontradas {len(combinations)} combinaciones:")
        for type_name, feature in combinations:
            print(f"  • {type_name}_{feature}")
    else:
        echo_warning("No se encontraron archivos de tests/benchs")
    
    # Listar demos
    print()
    echo_header("=" * 60)
    echo_header("  DEMOS DISPONIBLES")
    echo_header("=" * 60)
    print()
    
    categories = ["general", "tutorials", "examples", "showcase", "comparison", "performance", "integration"]
    total_demos = 0
    
    for category in categories:
        category_dir = DEMOS_DIR / category
        if category_dir.exists():
            demos = list(category_dir.glob("*.cpp"))
            if demos:
                echo_info(f"{category}/ ({len(demos)} demos):")
                for demo in sorted(demos):
                    print(f"  • {demo.stem}")
                total_demos += len(demos)
    
    print()
    echo_success(f"Total: {len(combinations)} tests/benchs + {total_demos} demos")
    
    # Mostrar compiladores disponibles
    if COMPILER_ENVS_DIR.exists():
        configs = list(COMPILER_ENVS_DIR.glob("*_env.json"))
        if configs:
            print()
            echo_info("Compiladores configurados:")
            for config in sorted(configs):
                compiler = config.stem.replace("_env", "")
                print(f"  • {compiler}")
    
    return 0


def main():
    """Main entry point"""
    
    # Parser principal
    parser = argparse.ArgumentParser(
        description="Sistema de build centralizado para int128 project",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Ejemplos:
  python make.py init
  python make.py build uint128 bits tests gcc release
  python make.py check uint128 bits
  python make.py run uint128 cmath all release
  python make.py test
  python make.py all
  python make.py clean
  python make.py list
  
  # Demos:
  python make.py build demos tutorials 01_basic_operations gcc release
  python make.py demo tutorials 01_basic_operations gcc release
  
  # Sanitizers:
  python make.py sanitize uint128 bits asan gcc tests
  python make.py sanitize uint128 thread_safety tsan gcc tests
  python make.py sanitize int128 algorithm ubsan clang tests
  
  # Análisis estático:
  python make.py analyze cppcheck headers
  python make.py analyze clang-tidy tests
  python make.py analyze all all
  
  # Benchmark comparativo (builtin, __int128, boost):
  python make.py compare gcc release-O3 100000
  python make.py compare clang release-Ofast 1000000
  python make.py run demos tutorials 01_basic_operations gcc release
  python make.py check demos tutorials
  python make.py demo tutorials 01_basic_operations
        """
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Comando a ejecutar')
    
    # init
    subparsers.add_parser('init', help='Detecta y configura compiladores')
    
    # build
    build_parser = subparsers.add_parser('build', help='Compila tests, benchmarks o demos')
    build_parser.add_argument('type', nargs='?', help='uint128 | int128 | demos')
    build_parser.add_argument('feature', nargs='?', help='bits | numeric | algorithm | <category> (si demos)')
    build_parser.add_argument('target', nargs='?', help='tests | benchs | <demo_name> (si demos)')
    build_parser.add_argument('compiler', nargs='?', help='gcc | clang | intel | msvc | all')
    build_parser.add_argument('mode', nargs='?', help='debug | release | release-O1 | release-O2 | release-O3 | release-Ofast | all')
    build_parser.add_argument('-v', '--verbose', action='store_true', help='Modo verbose')
    
    # check
    check_parser = subparsers.add_parser('check', help='Ejecuta tests o verifica demos')
    check_parser.add_argument('type', nargs='?', help='uint128 | int128 | demos')
    check_parser.add_argument('feature', nargs='?', help='bits | numeric | algorithm | <category> (si demos)')
    check_parser.add_argument('compiler', nargs='?', help='gcc | clang | intel | msvc | all')
    check_parser.add_argument('mode', nargs='?', help='debug | release | release-O1 | release-O2 | release-O3 | release-Ofast | all')
    
    # run
    run_parser = subparsers.add_parser('run', help='Ejecuta benchmarks o demos')
    run_parser.add_argument('type', nargs='?', help='uint128 | int128 | demos')
    run_parser.add_argument('feature', nargs='?', help='bits | numeric | algorithm | <category> (si demos)')
    run_parser.add_argument('compiler', nargs='?', help='gcc | clang | intel | msvc | all')
    run_parser.add_argument('mode', nargs='?', help='debug | release | release-O1 | release-O2 | release-O3 | release-Ofast | all')
    run_parser.add_argument('extra_args', nargs='*', help='<demo_name> y args adicionales (solo demos)')
    
    # clean
    clean_parser = subparsers.add_parser('clean', help='Limpia directorios de build')
    clean_parser.add_argument('--all', action='store_true', help='Limpia también compiler_envs')
    
    # all
    all_parser = subparsers.add_parser('all', help='Compila todo (tests + benchs)')
    
    # test
    test_parser = subparsers.add_parser('test', help='Compila y ejecuta todos los tests')
    test_parser.add_argument('compiler', nargs='?', help='gcc | clang | intel | msvc | all')
    test_parser.add_argument('mode', nargs='?', help='debug | release | all')
    
    # bench
    bench_parser = subparsers.add_parser('bench', help='Compila y ejecuta todos los benchmarks')
    bench_parser.add_argument('compiler', nargs='?', help='gcc | clang | intel | msvc | all')
    
    # demo
    demo_parser = subparsers.add_parser('demo', help='Compila y ejecuta un demo específico')
    demo_parser.add_argument('category', nargs='?', help='tutorials | examples | showcase | etc.')
    demo_parser.add_argument('demo_name', nargs='?', help='Nombre del demo sin .cpp')
    demo_parser.add_argument('compiler', nargs='?', help='gcc | clang | intel | msvc (default: gcc)')
    demo_parser.add_argument('mode', nargs='?', help='debug | release (default: release)')
    demo_parser.add_argument('extra_args', nargs='*', help='Argumentos adicionales para el demo')
    
    # list
    subparsers.add_parser('list', help='Lista todas las combinaciones disponibles')
    
    # sanitize
    sanitize_parser = subparsers.add_parser('sanitize', help='Compila con sanitizers (asan, ubsan, tsan)')
    sanitize_parser.add_argument('type', help='uint128 | int128')
    sanitize_parser.add_argument('feature', help='bits | numeric | algorithm | etc.')
    sanitize_parser.add_argument('sanitizer', nargs='?', default='asan', 
                                 help='asan | ubsan | tsan | msan | all (default: asan)')
    sanitize_parser.add_argument('compiler', nargs='?', default='gcc',
                                 help='gcc | clang | intel | msvc (default: gcc)')
    sanitize_parser.add_argument('target', nargs='?', default='tests',
                                 help='tests | benchs (default: tests)')
    
    # static-analysis
    analysis_parser = subparsers.add_parser('analyze', help='Análisis estático del código')
    analysis_parser.add_argument('tool', nargs='?', default='cppcheck',
                                 help='cppcheck | clang-tidy | infer | all (default: cppcheck)')
    analysis_parser.add_argument('target', nargs='?', default='headers',
                                 help='headers | tests | benchs | demos | all (default: headers)')
    
    # benchmark-compare
    compare_parser = subparsers.add_parser('compare', help='Benchmark comparativo (builtin, __int128, boost)')
    compare_parser.add_argument('compiler', nargs='?', default='gcc',
                                help='gcc | clang (default: gcc)')
    compare_parser.add_argument('mode', nargs='?', default='release-O3',
                                help='release-O1 | release-O2 | release-O3 | release-Ofast (default: release-O3)')
    compare_parser.add_argument('iterations', nargs='?', default='100000',
                                help='Número de iteraciones (default: 100000)')
    
    # Parse argumentos
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    # Ejecutar comando
    commands = {
        'init': cmd_init,
        'build': cmd_build,
        'check': cmd_check,
        'run': cmd_run,
        'clean': cmd_clean,
        'all': cmd_all,
        'test': cmd_test,
        'bench': cmd_bench,
        'demo': cmd_demo,
        'list': cmd_list,
        'sanitize': cmd_sanitize,
        'analyze': cmd_analyze,
        'compare': cmd_compare,
    }
    
    return commands[args.command](args)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        echo_warning("\nInterrumpido por el usuario")
        sys.exit(130)
    except Exception as e:
        echo_error(f"Error inesperado: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
