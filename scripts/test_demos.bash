#!/usr/bin/env bash
# ==============================================================================
# test_demos.bash - Script de verificación de compilación de demos
# ==============================================================================
# Compila todas las demos con GCC release para verificar que no hay errores.
# Ideal para CI/CD, pre-commit hooks, o verificación rápida.
# 
# Uso:
#   test_demos.bash [category] [compiler] [mode]
#
# Argumentos:
#   category : general | tutorials | examples | showcase | comparison | 
#              performance | integration | all (default: all)
#   compiler : gcc | clang | intel | msvc | all (default: gcc)
#   mode     : debug | release | all (default: release)
#
# Ejemplos:
#   test_demos.bash
#   test_demos.bash all gcc release
#   test_demos.bash tutorials
#   test_demos.bash examples clang debug
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

CATEGORY="${1:-all}"
COMPILER="${2:-gcc}"
MODE="${3:-release}"

# ========================= Validation =========================

VALID_CATEGORIES=("general" "tutorials" "examples" "showcase" "comparison" "performance" "integration" "all")
if [[ ! " ${VALID_CATEGORIES[*]} " =~ " ${CATEGORY} " ]]; then
    echo "Error: CATEGORY debe ser uno de: ${VALID_CATEGORIES[*]}"
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
cd "$PROJECT_ROOT"

# ========================= Color Output =========================

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m'

echo_success() { echo -e "${GREEN}[✓] $1${NC}"; }
echo_error() { echo -e "${RED}[✗] $1${NC}"; }
echo_info() { echo -e "${YELLOW}→ $1${NC}"; }
echo_header() { echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"; }
echo_title() { echo -e "${MAGENTA}$1${NC}"; }
echo_detail() { echo -e "${CYAN}  $1${NC}"; }

# ========================= Counters =========================

TOTAL_TESTS=0
PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

declare -a FAILED_TESTS
declare -a SKIPPED_TESTS

# ========================= Test Start =========================

START_TIME=$(date +%s)

echo_header
echo_title "  🧪 Test Suite - Verificación de Demos"
echo_header
echo ""
echo_info "Categoría: $CATEGORY"
echo_info "Compilador: $COMPILER"
echo_info "Modo: $MODE"
echo ""

# ========================= Determine Scope =========================

if [[ "$CATEGORY" == "all" ]]; then
    CATEGORIES=("general" "tutorials" "examples" "showcase" "comparison" "performance" "integration")
else
    CATEGORIES=("$CATEGORY")
fi

if [[ "$COMPILER" == "all" ]]; then
    COMPILERS=("gcc" "clang" "intel" "msvc")
else
    COMPILERS=("$COMPILER")
fi

if [[ "$MODE" == "all" ]]; then
    MODES=("debug" "release")
else
    MODES=("$MODE")
fi

# ========================= Test Function =========================

test_demo() {
    local cat="$1"
    local demo="$2"
    local comp="$3"
    local mod="$4"
    
    local demo_name=$(basename "$demo" .cpp)
    local test_name="$cat/$demo_name [$comp, $mod]"
    
    ((TOTAL_TESTS++))
    
    # Verificar que el archivo fuente existe
    if [[ ! -f "demos/$cat/$demo_name.cpp" ]]; then
        echo_error "SKIP: $test_name (archivo no encontrado)"
        SKIPPED_TESTS+=("$test_name - Archivo no encontrado")
        ((SKIP_COUNT++))
        return 0
    fi
    
    # Intentar compilar
    if bash scripts/build_demo.bash "$cat" "$demo_name" "$comp" "$mod" > /dev/null 2>&1; then
        echo_success "PASS: $test_name"
        ((PASS_COUNT++))
        return 0
    else
        echo_error "FAIL: $test_name"
        FAILED_TESTS+=("$test_name")
        ((FAIL_COUNT++))
        return 1
    fi
}

# ========================= Main Test Loop =========================

for cat in "${CATEGORIES[@]}"; do
    DEMO_DIR="demos/$cat"
    
    if [[ ! -d "$DEMO_DIR" ]]; then
        echo_info "Categoría no existe: $cat (omitida)"
        continue
    fi
    
    # Obtener lista de demos usando glob
    shopt -s nullglob
    DEMO_FILES=("$DEMO_DIR"/*.cpp)
    shopt -u nullglob
    
    if [[ ${#DEMO_FILES[@]} -eq 0 ]]; then
        echo_info "Sin demos en: $cat (omitida)"
        continue
    fi
    
    echo_title "📂 Testing: $cat (${#DEMO_FILES[@]} demos)"
    echo ""
    
    for demo_file in "${DEMO_FILES[@]}"; do
        for comp in "${COMPILERS[@]}"; do
            for mod in "${MODES[@]}"; do
                test_demo "$cat" "$demo_file" "$comp" "$mod"
            done
        done
    done
    
    echo ""
done

# ========================= Calculate Duration =========================

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

# ========================= Summary Report =========================

echo_header
echo_title "  📊 Resumen de Tests"
echo_header
echo ""

# Estadísticas generales
echo_detail "Tests ejecutados: $TOTAL_TESTS"
echo_detail "Duración: ${DURATION}s"
echo ""

# Resultados
echo_success "✓ Pasaron: $PASS_COUNT"
echo_error "✗ Fallaron: $FAIL_COUNT"

if [[ $SKIP_COUNT -gt 0 ]]; then
    echo_info "⊘ Omitidos: $SKIP_COUNT"
fi

# Tasa de éxito
if [[ $TOTAL_TESTS -gt 0 ]]; then
    SUCCESS_RATE=$(( (PASS_COUNT * 100) / TOTAL_TESTS ))
    echo ""
    echo_detail "Tasa de éxito: ${SUCCESS_RATE}%"
fi

# Detalles de fallos
if [[ ${#FAILED_TESTS[@]} -gt 0 ]]; then
    echo ""
    echo_title "❌ Tests fallidos:"
    for failed in "${FAILED_TESTS[@]}"; do
        echo "  - $failed"
    done
fi

# Detalles de omitidos
if [[ ${#SKIPPED_TESTS[@]} -gt 0 ]]; then
    echo ""
    echo_title "⊘ Tests omitidos:"
    for skipped in "${SKIPPED_TESTS[@]}"; do
        echo "  - $skipped"
    done
fi

echo ""
echo_header

# ========================= Exit Status =========================

if [[ $FAIL_COUNT -gt 0 ]]; then
    echo ""
    echo_error "❌ Algunos tests fallaron"
    exit 1
elif [[ $PASS_COUNT -eq 0 ]]; then
    echo ""
    echo_error "⚠️  No se ejecutaron tests"
    exit 1
else
    echo ""
    echo_success "✅ Todos los tests pasaron exitosamente!"
    exit 0
fi
