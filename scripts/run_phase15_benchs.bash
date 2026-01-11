#!/usr/bin/env bash
# ==============================================================================
# run_phase15_benchs.bash - Ejecutar benchmarks de Fase 1.5
# ==============================================================================
# Script temporal para ejecutar benchmarks compilados con include_new/
# 
# Uso:
#   run_phase15_benchs.bash [type] [compiler] [mode]
#
# Argumentos:
#   type     : uint128 | int128 | both (default: both)
#   compiler : gcc | clang | intel | msvc (default: gcc)
#   mode     : debug | release (default: release)
#
# Ejemplos:
#   run_phase15_benchs.bash uint128 gcc release
#   run_phase15_benchs.bash both clang release
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

TYPE="${1:-both}"
COMPILER="${2:-gcc}"
MODE="${3:-release}"

# ========================= Validation =========================

if [[ "$TYPE" != "uint128" && "$TYPE" != "int128" && "$TYPE" != "both" ]]; then
    echo "Error: TYPE debe ser 'uint128', 'int128' o 'both'"
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

# ========================= Configuration =========================

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_BASE="${PROJECT_ROOT}/build/phase15_benchs"

# ========================= Functions =========================

run_bench() {
    local type="$1"
    local compiler="$2"
    local mode="$3"
    
    local output_exe="${OUTPUT_BASE}/${compiler}/${mode}/${type}_t_benchs_${compiler}.exe"
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "🚀 Ejecutando: ${type} | ${compiler} | ${mode}"
    echo "📂 Ejecutable: ${output_exe}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    if [[ ! -f "$output_exe" ]]; then
        echo "❌ Ejecutable no encontrado: $output_exe"
        echo "   Debe compilar primero con: build_phase15_benchs.bash $type $compiler $mode"
        return 1
    fi
    
    # Ejecutar el benchmark
    "$output_exe"
    local exit_code=$?
    
    echo ""
    if [[ $exit_code -eq 0 ]]; then
        echo "✅ Benchmark completado exitosamente"
        return 0
    else
        echo "❌ Benchmark falló (exit code: $exit_code)"
        return 1
    fi
}

# ========================= Main Logic =========================

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║   FASE 1.5 - RUN BENCHMARKS (Template Unificado)          ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Configuración:"
echo "  📦 Tipo: $TYPE"
echo "  🔧 Compilador: $COMPILER"
echo "  🎚️  Modo: $MODE"
echo ""

# Determinar qué tipos ejecutar
TYPES_TO_RUN=()
if [[ "$TYPE" == "both" ]]; then
    TYPES_TO_RUN=("uint128" "int128")
else
    TYPES_TO_RUN=("$TYPE")
fi

# Ejecutar benchmarks
SUCCESS_COUNT=0
FAIL_COUNT=0

for type in "${TYPES_TO_RUN[@]}"; do
    if run_bench "$type" "$COMPILER" "$MODE"; then
        ((SUCCESS_COUNT++))
    else
        ((FAIL_COUNT++))
    fi
    echo ""
done

# Resumen final
echo "╔════════════════════════════════════════════════════════════╗"
echo "║   RESUMEN DE BENCHMARKS                                    ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo "✅ Exitosos: $SUCCESS_COUNT"
echo "❌ Fallidos: $FAIL_COUNT"
echo ""

if [[ $FAIL_COUNT -gt 0 ]]; then
    exit 1
fi

exit 0
