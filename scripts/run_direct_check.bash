#!/bin/bash
# =============================================================================
# run_direct_check.bash - Ejecuta directamente el binario de tests
# Muestra los 137 tests individuales en lugar de solo la matriz resumen
# =============================================================================

set -e

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo_info()    { echo -e "${CYAN}[INFO]${NC} $1"; }
echo_success() { echo -e "${GREEN}[OK]${NC} $1"; }
echo_error()   { echo -e "${RED}[ERROR]${NC} $1"; }
echo_header()  { echo -e "${YELLOW}$1${NC}"; }

# Uso
if [[ $# -lt 2 ]]; then
    echo "Uso: $0 <type> <feature> [compiler] [mode]"
    echo ""
    echo "Ejemplos:"
    echo "  $0 int128 tt                    # gcc release por defecto"
    echo "  $0 int128 tt gcc release"
    echo "  $0 uint128 bits clang debug"
    echo "  $0 int128 tt all all            # Ejecuta todas las combinaciones"
    exit 1
fi

TYPE="$1"
FEATURE="$2"
COMPILER="${3:-gcc}"
MODE="${4:-release}"

# Directorio del proyecto
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build/build_tests"

# Expandir "all"
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

echo_header "============================================================"
echo_header "  DIRECT TEST EXECUTION: ${TYPE}_${FEATURE}"
echo_header "============================================================"
echo ""

TOTAL_RUNS=0
SUCCESSFUL_RUNS=0

for comp in "${COMPILERS[@]}"; do
    for m in "${MODES[@]}"; do
        EXE_NAME="${TYPE}_${FEATURE}_tests_${comp}"
        EXE_PATH="$BUILD_DIR/$comp/$m/$EXE_NAME"
        
        # En Windows, añadir .exe si no existe
        if [[ ! -f "$EXE_PATH" && -f "${EXE_PATH}.exe" ]]; then
            EXE_PATH="${EXE_PATH}.exe"
        fi
        
        echo_header "------------------------------------------------------------"
        echo_info "Compiler: $comp | Mode: $m"
        echo_info "Executable: $EXE_PATH"
        echo_header "------------------------------------------------------------"
        echo ""
        
        if [[ -f "$EXE_PATH" ]]; then
            TOTAL_RUNS=$((TOTAL_RUNS + 1))
            
            # Ejecutar y mostrar salida completa
            if "$EXE_PATH"; then
                SUCCESSFUL_RUNS=$((SUCCESSFUL_RUNS + 1))
                echo ""
                echo_success "Tests passed for $comp [$m]"
            else
                echo ""
                echo_error "Tests FAILED for $comp [$m]"
            fi
        else
            echo_error "Executable not found: $EXE_PATH"
            echo_info "Build it first with:"
            echo_info "  scripts/build_generic.bash $TYPE $FEATURE tests $comp $m"
        fi
        
        echo ""
    done
done

echo_header "============================================================"
echo_header "  SUMMARY"
echo_header "============================================================"
echo_info "Total runs: $TOTAL_RUNS"
echo_success "Successful: $SUCCESSFUL_RUNS"
if [[ $TOTAL_RUNS -gt $SUCCESSFUL_RUNS ]]; then
    echo_error "Failed: $((TOTAL_RUNS - SUCCESSFUL_RUNS))"
fi
echo ""

# Exit code
if [[ $SUCCESSFUL_RUNS -eq $TOTAL_RUNS && $TOTAL_RUNS -gt 0 ]]; then
    exit 0
else
    exit 1
fi
