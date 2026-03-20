#!/usr/bin/env bash
# ==============================================================================
# check_phase15_tests.bash - Ejecutar tests con include_new/ (Fase 1.5)
# ==============================================================================
# Script temporal para ejecutar tests compilados con el template unificado
# 
# Uso:
#   check_phase15_tests.bash [type] [compiler] [mode]
#
# Argumentos:
#   type     : uint128 | int128 | both (default: both)
#   compiler : gcc | clang | intel | msvc | all (default: gcc)
#   mode     : debug | release | all (default: debug)
#
# Ejemplos:
#   check_phase15_tests.bash uint128 gcc debug
#   check_phase15_tests.bash int128 clang release
#   check_phase15_tests.bash both all all
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

TYPE="${1:-both}"
COMPILER="${2:-gcc}"
MODE="${3:-debug}"

# ========================= Validation =========================

if [[ "$TYPE" != "uint128" && "$TYPE" != "int128" && "$TYPE" != "both" ]]; then
    echo "Error: TYPE debe ser 'uint128', 'int128' o 'both'"
    exit 1
fi

VALID_COMPILERS=("gcc" "clang" "intel" "msvc" "all")
if [[ ! " ${VALID_COMPILERS[*]} " =~ " ${COMPILER} " ]]; then
    echo "Error: COMPILER debe ser uno de: ${VALID_COMPILERS[*]}"
    exit 1
fi

VALID_MODES=("debug" "release" "all")
if [[ ! " ${VALID_MODES[*]} " =~ " ${MODE} " ]]; then
    echo "Error: MODE debe ser uno de: ${VALID_MODES[*]}"
    exit 1
fi

# ========================= Configuration =========================

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_BASE="${PROJECT_ROOT}/build/phase15_tests"

# ========================= Functions =========================

run_test() {
    local type="$1"
    local compiler="$2"
    local mode="$3"
    
    local output_exe="${OUTPUT_BASE}/${compiler}/${mode}/phase15_${type}_tests_${compiler}.exe"
    
    # Verificar que el ejecutable existe
    if [[ ! -f "$output_exe" ]]; then
        echo "❌ Ejecutable no encontrado: $output_exe"
        echo "🔧 Compilar primero: bash scripts/build_phase15_tests.bash $type $compiler $mode"
        return 1
    fi
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "▶️  Ejecutando: ${type} | ${compiler} | ${mode}"
    echo "📂 Ejecutable: ${output_exe}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    # Ejecutar test y capturar resultado
    if "$output_exe"; then
        echo "✅ Tests pasaron correctamente"
        return 0
    else
        echo "❌ Tests fallaron"
        return 1
    fi
}

# ========================= Main Logic =========================

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║   FASE 1.5 - CHECK TESTS (Template Unificado)             ║"
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

# Determinar qué compiladores usar
COMPILERS_TO_USE=()
if [[ "$COMPILER" == "all" ]]; then
    COMPILERS_TO_USE=("gcc" "clang" "intel" "msvc")
else
    COMPILERS_TO_USE=("$COMPILER")
fi

# Determinar qué modos usar
MODES_TO_USE=()
if [[ "$MODE" == "all" ]]; then
    MODES_TO_USE=("debug" "release")
else
    MODES_TO_USE=("$MODE")
fi

# Ejecutar todas las combinaciones
SUCCESS_COUNT=0
FAIL_COUNT=0

for type in "${TYPES_TO_RUN[@]}"; do
    for compiler in "${COMPILERS_TO_USE[@]}"; do
        for mode in "${MODES_TO_USE[@]}"; do
            if run_test "$type" "$compiler" "$mode"; then
                ((SUCCESS_COUNT++))
            else
                ((FAIL_COUNT++))
            fi
            echo ""
        done
    done
done

# Resumen final
echo "╔════════════════════════════════════════════════════════════╗"
echo "║   RESUMEN DE TESTS                                         ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo "✅ Pasados: $SUCCESS_COUNT"
echo "❌ Fallidos: $FAIL_COUNT"
echo ""

if [[ $FAIL_COUNT -gt 0 ]]; then
    exit 1
fi

exit 0
