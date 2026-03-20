#!/usr/bin/env python3
"""
Script de inicialización del proyecto
Detecta y configura todos los compiladores disponibles
"""

import sys
from pathlib import Path

# Add env_setup to path
sys.path.insert(0, str(Path(__file__).parent / "env_setup"))

from compiler_env import detect_all_compilers

if __name__ == "__main__":
    print("""
╔════════════════════════════════════════════════════════════╗
║        INT128 PROJECT - COMPILER CONFIGURATION             ║
╚════════════════════════════════════════════════════════════╝
    """)
    
    results = detect_all_compilers()
    
    detected = sum(1 for success in results.values() if success)
    total = len(results)
    
    print(f"""
╔════════════════════════════════════════════════════════════╗
║  Compiladores detectados: {detected}/{total}                            ║
╚════════════════════════════════════════════════════════════╝

Ahora puedes usar los scripts genéricos Python:

  Compilar:
    python scripts/build_generic.py uint128 bits tests gcc release
    python scripts/build_generic.py int128 numeric benchs all all

  Ejecutar tests:
    python scripts/check_generic.py uint128 bits
    python scripts/check_generic.py int128 algorithm gcc release

  Ejecutar benchmarks:
    python scripts/run_generic.py uint128 cmath
    python scripts/run_generic.py int128 algorithm all release

Los entornos de compiladores están aislados y no afectan
el entorno global del sistema.
    """)
