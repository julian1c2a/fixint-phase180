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
DOCKER_DIR = PROJECT_ROOT / "docker"

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


def find_bash() -> str:
    """Encuentra un bash funcional en Windows (MSYS2 > Git Bash > sistema)."""
    if sys.platform == "win32":
        candidates = [
            r"C:\msys64\usr\bin\bash.exe",
            r"C:\msys64\bin\bash.exe",
            r"C:\Program Files\Git\usr\bin\bash.exe",
            r"C:\Program Files (x86)\Git\usr\bin\bash.exe",
        ]
        for path in candidates:
            if Path(path).exists():
                return path
    return "bash"


def _win_to_wsl_path(win_path: Path) -> str:
    """Convierte ruta Windows absoluta a formato WSL /mnt/<drive>/..."""
    p = str(win_path).replace("\\", "/")
    if len(p) >= 2 and p[1] == ':':
        drive = p[0].lower()
        rest = p[2:].lstrip('/')
        return f"/mnt/{drive}/{rest}"
    return p


def _wsl_available() -> bool:
    """Devuelve True si wsl.exe existe y responde."""
    try:
        r = subprocess.run(
            ["wsl", "--", "echo", "ok"],
            capture_output=True, text=True, timeout=15
        )
        return r.returncode == 0 and "ok" in r.stdout
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return False


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


def discover_tests() -> Dict[str, Path]:
    """Discover all test .cpp files in tests/ directory.
    Returns dict mapping feature_name -> source file path.
    """
    tests = {}

    # test_param_*.cpp -> feature name (parameterized feature tests)
    for f in sorted(TESTS_DIR.glob("test_param_*.cpp")):
        feature = f.stem[len("test_param_"):]
        tests[feature] = f

    # test_*.cpp (non-param) -> feature name (priority + standalone tests)
    for f in sorted(TESTS_DIR.glob("test_*.cpp")):
        if f.stem.startswith("test_param_"):
            continue
        feature = f.stem[len("test_"):]
        tests[feature] = f

    return tests


def discover_benchmarks() -> Dict[str, Path]:
    """Discover all benchmark .cpp files in benchs/ directory."""
    benchs = {}
    for f in sorted(BENCHS_DIR.glob("*.cpp")):
        if f.stem.startswith("benchmark_"):
            name = f.stem[len("benchmark_"):]
        else:
            name = f.stem
        benchs[name] = f
    return benchs


def get_all_combinations() -> List[Tuple[str, str]]:
    """Get all (type, feature) combinations for build/check commands.
    Type is always 'uint128' since Phase 1.75 tests are type-agnostic
    (the parameterized template handles all representations).
    """
    tests = discover_tests()
    return [("uint128", feature) for feature in sorted(tests.keys())]


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

    benchmarks = discover_benchmarks()
    combinations = [("uint128", feature) for feature in sorted(benchmarks.keys())]
    
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


# =============================================================================
# Docker cross-compilation configuration
# =============================================================================

DOCKER_ARCHES: List[str] = ['arm64', 'arm32', 'riscv64']
# riscv32 excluido de 'all': gcc-riscv64-linux-gnu no incluye sysroot ILP32.

DOCKER_CONFIG: Dict[str, dict] = {
    'arm64': {
        'platform':   'linux/arm64',
        'dockerfile': 'Dockerfile.crosstest',
        'image':      'fixint-crosstest:arm64',
        'privileged': False,
    },
    'arm32': {
        'platform':   'linux/arm/v7',
        'dockerfile': 'Dockerfile.crosstest',
        'image':      'fixint-crosstest:arm32',
        'privileged': False,
    },
    'riscv64': {
        'platform':   'linux/riscv64',
        'dockerfile': 'Dockerfile.crosstest',
        'image':      'fixint-crosstest:riscv64',
        'privileged': False,
    },
    'riscv32': {
        'platform':   None,   # sin imagen oficial linux/riscv32
        'dockerfile': 'Dockerfile.riscv32',
        'image':      'fixint-crosstest:riscv32',
        'privileged': True,   # necesario para binfmt_misc
    },
}


def _docker_available() -> bool:
    """Comprueba si Docker está disponible y en ejecución."""
    try:
        r = subprocess.run(
            ['docker', 'info'],
            capture_output=True,
            timeout=10
        )
        return r.returncode == 0
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return False


def _docker_image_exists(image: str) -> bool:
    """Devuelve True si la imagen local existe."""
    r = subprocess.run(
        ['docker', 'image', 'inspect', image],
        capture_output=True
    )
    return r.returncode == 0


def _docker_build_image(arch: str, cfg: dict) -> bool:
    """Construye la imagen Docker para la arquitectura indicada."""
    image      = cfg['image']
    dockerfile = DOCKER_DIR / cfg['dockerfile']
    cmd = ['docker', 'build']
    if cfg['platform']:
        cmd += ['--platform', cfg['platform']]
    cmd += ['-t', image, '-f', str(dockerfile), str(PROJECT_ROOT)]
    echo_info(f"Construyendo imagen Docker {image} ...")
    r = subprocess.run(cmd, cwd=PROJECT_ROOT)
    return r.returncode == 0


def cmd_docker(args: argparse.Namespace) -> int:
    """Compila y ejecuta tests via Docker (cross-compilation multi-arch)."""
    echo_header("=" * 60)
    echo_header("  DOCKER — BUILD & TEST (Cross-compilation)")
    echo_header("=" * 60)
    print()

    if not _docker_available():
        echo_error("Docker no disponible. Inicia Docker Desktop.")
        echo_info("Descarga: https://www.docker.com/products/docker-desktop/")
        return 1

    arch         = getattr(args, 'arch',  None) or 'all'
    mode         = getattr(args, 'mode',  None) or 'release-O2'
    force_build  = getattr(args, 'build', False)

    if arch != 'all' and arch not in DOCKER_CONFIG:
        echo_error(f"Arquitectura desconocida: '{arch}'.")
        echo_info(f"Opciones: {', '.join(DOCKER_ARCHES)} | all")
        return 1

    arches = DOCKER_ARCHES if arch == 'all' else [arch]

    if 'riscv32' in arches:
        echo_error("riscv32 no soportado en este entorno:")
        echo_info("  gcc-riscv64-linux-gnu (Ubuntu 22.04) no incluye sysroot ILP32.")
        echo_info("  Se necesita un toolchain riscv32-linux-gnu con sysroot completo.")
        arches = [a for a in arches if a != 'riscv32']
        if not arches:
            return 1

    echo_info(f"Arquitectura(s): {', '.join(arches)}")
    echo_info(f"Modo:            {mode}")
    print()

    results: Dict[str, bool] = {}

    for a in arches:
        cfg   = DOCKER_CONFIG[a]
        image = cfg['image']

        echo_header(f"--- {a.upper()} ---")

        # Construir imagen si no existe o se fuerza rebuild
        if force_build or not _docker_image_exists(image):
            if not _docker_build_image(a, cfg):
                echo_error(f"{a}: FAIL (build de imagen falló)")
                results[a] = False
                print()
                continue

        # Named volume para build/: los binarios ARM/RISC-V se almacenan en
        # ext4 dentro de Docker, no en NTFS, para preservar el bit de ejecución.
        build_vol = f'fixint-{a}-build'
        if force_build:
            subprocess.run(['docker', 'volume', 'rm', '-f', build_vol],
                           capture_output=True)

        # Ejecutar tests en el contenedor
        cmd = ['docker', 'run', '--rm']
        if cfg['platform']:
            cmd += ['--platform', cfg['platform']]
        if cfg['privileged']:
            cmd += ['--privileged']
        cmd += ['-v', f'{str(PROJECT_ROOT).replace(chr(92), "/")}:/project']
        cmd += ['-v', f'{build_vol}:/project/build']  # ext4, no NTFS
        cmd += [image, 'python3', '/project/make.py', 'test', 'gcc', mode]

        echo_info(f"Ejecutando tests en {a} ({cfg.get('platform') or 'cross-compilation'}) ...")
        r = subprocess.run(cmd, cwd=PROJECT_ROOT)
        ok = (r.returncode == 0)
        results[a] = ok

        if ok:
            echo_success(f"{a}: PASS")
        else:
            echo_error(f"{a}: FAIL (código {r.returncode})")
        print()

    # Resumen
    passed = sum(1 for v in results.values() if v)
    failed = len(results) - passed

    echo_header("=" * 60)
    echo_header("  RESUMEN DOCKER")
    echo_header("=" * 60)
    echo_success(f"Pasaron: {passed}/{len(arches)} arquitecturas")
    if failed:
        echo_error(f"Fallaron: {failed}/{len(arches)} arquitecturas")
        for a, ok in results.items():
            if not ok:
                echo_error(f"  - {a}")

    return 0 if failed == 0 else 1


def cmd_wsl(args: argparse.Namespace) -> int:
    """Compila y ejecuta tests en WSL con GCC, Clang e Intel."""
    echo_header("=" * 60)
    echo_header("  WSL — BUILD & TEST (Linux: GCC / Clang / Intel)")
    echo_header("=" * 60)
    print()

    if not _wsl_available():
        echo_error("WSL no disponible (servicio detenido o no instalado).")
        echo_info("Inicia WSL desde PowerShell:  wsl")
        echo_info("O habilita el servicio:       wsl --install")
        return 1

    wsl_root = _win_to_wsl_path(PROJECT_ROOT)
    compiler = getattr(args, 'compiler', None) or 'all'
    mode     = getattr(args, 'mode',     None) or 'release-O2'

    echo_info(f"Proyecto en WSL:  {wsl_root}")
    echo_info(f"Compilador(es):   {compiler}")
    echo_info(f"Modo:             {mode}")
    print()

    families = ['gcc', 'clang', 'intel'] if compiler == 'all' else [compiler]

    results: Dict[str, bool] = {}

    for family in families:
        echo_header(f"--- {family.upper()} ---")
        bash_cmd = f"cd '{wsl_root}' && python3 make.py test {family} {mode}"
        r = subprocess.run(["wsl", "--", "bash", "-c", bash_cmd], cwd=PROJECT_ROOT)
        ok = (r.returncode == 0)
        results[family] = ok
        if ok:
            echo_success(f"{family}: PASS")
        else:
            echo_error(f"{family}: FAIL (código {r.returncode})")
        print()

    passed = sum(1 for v in results.values() if v)
    failed = len(results) - passed

    echo_header("=" * 60)
    echo_header("  RESUMEN WSL")
    echo_header("=" * 60)
    echo_success(f"Pasaron: {passed}/{len(families)} familias")
    if failed:
        echo_error(f"Fallaron: {failed}/{len(families)} familias")
        for fam, ok in results.items():
            if not ok:
                echo_error(f"  - {fam}")

    return 0 if failed == 0 else 1


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
    
    cmd = [find_bash(), str(script_path), type_name, feature, target, compiler, sanitizer]
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
    
    cmd = [find_bash(), str(script_path), tool, target]
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
    
    # Resolver ruta real del compilador para evitar colisiones de nombre en PATH
    sys.path.insert(0, str(SCRIPTS_DIR))
    from env_setup.compiler_env import CompilerEnvironment
    cxx = CompilerEnvironment(compiler).get_compiler_cmd()

    # Convertir ruta Windows a formato MSYS2 para que bash la ejecute correctamente
    if sys.platform == "win32" and cxx and '\\' in cxx:
        cygpath = Path(r"C:\msys64\usr\bin\cygpath.exe")
        if cygpath.exists():
            cp = subprocess.run([str(cygpath), "-u", cxx], capture_output=True, text=True)
            if cp.returncode == 0:
                cxx = cp.stdout.strip()
        else:
            # Conversión manual: C:\foo\bar → /c/foo/bar
            p = Path(cxx)
            parts = list(p.parts)
            if parts and len(parts[0]) >= 2 and parts[0][1] == ':':
                drive = parts[0][0].lower()
                rest = '/'.join(parts[1:]).replace('\\', '/')
                cxx = f'/{drive}/{rest}'

    # Ejecutar script bash
    script_path = SCRIPTS_DIR / "benchmark_comparison.bash"
    if not script_path.exists():
        echo_error(f"Script no encontrado: {script_path}")
        return 1

    cmd = [find_bash(), str(script_path), compiler, mode, iterations, cxx]
    result = subprocess.run(cmd, cwd=PROJECT_ROOT)
    return result.returncode


def cmd_list(args: argparse.Namespace) -> int:
    """Lista todas las combinaciones disponibles"""
    tests = discover_tests()
    benchs = discover_benchmarks()

    # Categorize tests
    param_tests = {}
    priority_tests = {}
    standalone_tests = {}
    for name, path in tests.items():
        if (TESTS_DIR / f"test_param_{name}.cpp").exists():
            param_tests[name] = path
        elif name.startswith("priority"):
            priority_tests[name] = path
        else:
            standalone_tests[name] = path

    echo_header("=" * 60)
    echo_header("  PARAMETERIZED FEATURE TESTS")
    echo_header("=" * 60)
    print()
    if param_tests:
        echo_info(f"{len(param_tests)} tests:")
        for name in sorted(param_tests):
            print(f"  - {name:30s} ({param_tests[name].name})")
    print()

    echo_header("=" * 60)
    echo_header("  PRIORITY TESTS")
    echo_header("=" * 60)
    print()
    if priority_tests:
        echo_info(f"{len(priority_tests)} tests:")
        for name in sorted(priority_tests, key=lambda x: (len(x), x)):
            print(f"  - {name:30s} ({priority_tests[name].name})")
    print()

    echo_header("=" * 60)
    echo_header("  STANDALONE TESTS")
    echo_header("=" * 60)
    print()
    if standalone_tests:
        echo_info(f"{len(standalone_tests)} tests:")
        for name in sorted(standalone_tests):
            print(f"  - {name:30s} ({standalone_tests[name].name})")
    print()

    echo_header("=" * 60)
    echo_header("  BENCHMARKS")
    echo_header("=" * 60)
    print()
    if benchs:
        echo_info(f"{len(benchs)} benchmarks:")
        for name in sorted(benchs):
            print(f"  - {name:30s} ({benchs[name].name})")
    else:
        echo_warning("No benchmarks found")
    print()

    # Listar demos
    echo_header("=" * 60)
    echo_header("  DEMOS")
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
                    print(f"  - {demo.stem}")
                total_demos += len(demos)

    if total_demos == 0:
        echo_warning("No demos found")
    print()

    echo_success(f"Total: {len(tests)} tests + {len(benchs)} benchmarks + {total_demos} demos")

    # Mostrar compiladores disponibles
    if COMPILER_ENVS_DIR.exists():
        configs = list(COMPILER_ENVS_DIR.glob("*_env.json"))
        if configs:
            print()
            echo_info("Compiladores configurados:")
            for config in sorted(configs):
                compiler = config.stem.replace("_env", "")
                print(f"  - {compiler}")

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

  # WSL (compiladores Linux: GCC, Clang, Intel):
  python make.py wsl                      # todos los compiladores, release-O2
  python make.py wsl gcc                  # solo GCC
  python make.py wsl clang release-O2     # solo Clang
  python make.py wsl intel release-O3     # solo Intel ICX
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

    # wsl
    wsl_parser = subparsers.add_parser('wsl', help='Compila y ejecuta tests en WSL (GCC/Clang/Intel)')
    wsl_parser.add_argument('compiler', nargs='?', help='gcc | clang | intel | all (default: all)')
    wsl_parser.add_argument('mode', nargs='?', help='release-O2 | debug | release-O3 (default: release-O2)')

    # docker
    docker_parser = subparsers.add_parser(
        'docker',
        help='Cross-compilation tests via Docker (arm64, arm32, riscv64, riscv32)'
    )
    docker_parser.add_argument(
        'arch', nargs='?',
        help='arm64 | arm32 | riscv64 | riscv32 | all (default: all)'
    )
    docker_parser.add_argument(
        'mode', nargs='?',
        help='release-O2 | debug | release-O3 (default: release-O2)'
    )
    docker_parser.add_argument(
        '--build', action='store_true',
        help='Forzar reconstrucción de imágenes Docker'
    )

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
        'wsl': cmd_wsl,
        'docker': cmd_docker,
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
