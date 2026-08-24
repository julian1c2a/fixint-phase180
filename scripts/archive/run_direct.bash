#!/usr/bin/env bash
# ==============================================================================
# run_direct.bash - Ejecutar benchmark directamente en consola
# ==============================================================================
# Ejecuta un benchmark compilado mostrando toda la salida en consola
# Sin formateo de colores, sin resumen - output directo
# 
# Par con: build_benchs_generic.bash
# 
# Uso:
#   run_direct.bash <feature> [compiler] [mode]
#
# Argumentos:
#   feature  : tt | bits | numeric | algorithm | cmath | etc.
#   compiler : gcc | clang | intel | msvc (default: gcc)
#   mode     : debug | release (default: release)
#
# Ejemplos:
#   run_direct.bash tt
#   run_direct.bash bits gcc release
#   run_direct.bash cmath clang debug
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

if [[ $# -lt 1 ]]; then
    echo "Uso: $0 <feature> [compiler] [mode]"
    echo ""
    echo "Ejemplos:"
    echo "  $0 tt"
    echo "  $0 bits gcc release"
    echo "  $0 cmath clang debug"
    exit 1
fi

FEATURE="$1"
COMPILER="${2:-gcc}"
MODE="${3:-release}"

# ========================= Handle FEATURE=all =========================

ALL_FEATURES=("tt" "traits" "limits" "concepts" "algorithm" "iostreams" "bits" "cmath" "numeric" "ranges" "format" "safe" "thread_safety" "comparison_boost")

if [[ "$FEATURE" == "all" ]]; then
    echo "========================================="
    echo " Running ALL features (benchmarks direct)"
    echo "========================================="
    for feat in "${ALL_FEATURES[@]}"; do
        echo ""
        echo ">>> Running feature: $feat"
        bash "$0" "$feat" "$COMPILER" "$MODE"
    done
    echo ""
    echo "========================================="
    echo " [OK] ALL features run!"
    echo "========================================="
    exit 0
fi

# ========================= Validation =========================

VALID_FEATURES=("all" "tt" "traits" "limits" "concepts" "algorithm" "iostreams" "bits" "cmath" "numeric" "ranges" "format" "safe" "thread_safety" "comparison_boost")
if [[ ! " ${VALID_FEATURES[*]} " =~ " ${FEATURE} " ]]; then
    echo "Error: FEATURE debe ser uno de: ${VALID_FEATURES[*]}"
    exit 1
fi

VALID_COMPILERS=("gcc" "clang" "intel" "msvc")
if [[ ! " ${VALID_COMPILERS[*]} " =~ " ${COMPILER} " ]]; then
    echo "Error: COMPILER debe ser uno de: ${VALID_COMPILERS[*]}"
    exit 1
fi

VALID_MODES=("debug" "release")
if [[ ! " ${VALID_MODES[*]} " =~ " ${MODE} " ]]; then
    echo "Error: MODE debe ser uno de: ${VALID_MODES[*]}"
    exit 1
fi

# ========================= Find Executable =========================

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

EXE_NAME="int128_${FEATURE}_benchs_${COMPILER}"

# On Windows/MSYS2, add .exe
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "mingw"* ]]; then
    EXE_NAME="${EXE_NAME}.exe"
fi

EXE_PATH="build/build_benchs/${COMPILER}/${MODE}/${EXE_NAME}"

if [[ ! -f "$EXE_PATH" ]]; then
    echo "Error: Ejecutable no encontrado: $EXE_PATH"
    echo ""
    echo "Primero compila con:"
    echo "  bash scripts/build_benchs_generic.bash ${FEATURE} ${COMPILER} ${MODE}"
    exit 1
fi

# ========================= Execute =========================

echo "========================================================================"
echo "  EJECUTANDO: ${EXE_PATH}"
echo "========================================================================"
echo ""

# Execute directly - all output goes to console
exec "$EXE_PATH"
