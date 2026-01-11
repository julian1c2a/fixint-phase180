#!/usr/bin/env bash
# ==============================================================================
# check_generic.bash - Script genérico de ejecución de tests
# ==============================================================================
# Ejecuta tests compilados con todos los compiladores y modos
# Muestra resultados en una matriz resumen
# 
# Uso:
#   check_generic.bash <feature> [compiler] [mode]
#
# Argumentos:
#   feature  : tt | bits | numeric | algorithm | iostreams | cmath | etc.
#   compiler : gcc | clang | intel | msvc | all (default: all)
#   mode     : debug | release | all (default: all)
#
# Ejemplos:
#   check_generic.bash bits
#   check_generic.bash numeric gcc release
#   check_generic.bash algorithm all all
#
# NOTA: El tipo "int128" se usa implícitamente (template unificado int128_base_t<S>)
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

if [[ $# -lt 1 ]]; then
    echo "Error: Se requiere al menos 1 argumento"
    echo "Uso: $0 <feature> [compiler] [mode]"
    echo ""
    echo "Ejemplos:"
    echo "  $0 bits"
    echo "  $0 numeric gcc release"
    echo "  $0 algorithm all all"
    echo "  $0 all gcc release      # Check ALL features"
    exit 1
fi

FEATURE="$1"
COMPILER="${2:-all}"
MODE="${3:-all}"

# ========================= Handle FEATURE=all =========================

ALL_FEATURES=("tt" "traits" "limits" "concepts" "algorithm" "iostreams" "bits" "cmath" "numeric" "ranges" "format" "safe" "thread_safety" "comparison_boost")

if [[ "$FEATURE" == "all" ]]; then
    echo "========================================="
    echo " Checking ALL features (tests)"
    echo "========================================="
    for feat in "${ALL_FEATURES[@]}"; do
        echo ""
        echo ">>> Checking feature: $feat"
        bash "$0" "$feat" "$COMPILER" "$MODE"
    done
    echo ""
    echo "========================================="
    echo " [OK] ALL features checked!"
    echo "========================================="
    exit 0
fi

# ========================= Validation =========================

VALID_FEATURES=("all" "tt" "traits" "limits" "concepts" "algorithm" "iostreams" "bits" "cmath" "numeric" "ranges" "format" "safe" "thread_safety" "comparison_boost")
if [[ ! " ${VALID_FEATURES[*]} " =~ " ${FEATURE} " ]]; then
    echo "Error: FEATURE debe ser uno de: ${VALID_FEATURES[*]}"
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

BUILD_DIR="$PROJECT_ROOT/build/build_tests"

# ========================= Color Output =========================

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo_success() { echo -e "${GREEN}[OK] $1${NC}"; }
echo_error() { echo -e "${RED}[ERROR] $1${NC}"; }
echo_info() { echo -e "${YELLOW}-> $1${NC}"; }
echo_header() { echo -e "${BLUE}$1${NC}"; }

# ========================= Test Results Tracking =========================

declare -A test_results

run_test() {
    local comp="$1"
    local mod="$2"
    
    local exe_name="int128_${FEATURE}_tests_${comp}"
    
    # On Windows/MSYS2/Cygwin, ALL compilers produce .exe files
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "mingw"* ]]; then
        exe_name="${exe_name}.exe"
    fi
    
    local exe_path="$BUILD_DIR/$comp/$mod/$exe_name"
    local key="${comp}_${mod}"
    
    if [[ ! -f "$exe_path" ]]; then
        test_results["$key"]="NOT_FOUND"
        return
    fi
    
    echo_info "Running: $comp [$mod]"
    
    if "$exe_path" &>/dev/null; then
        test_results["$key"]="PASS"
        echo_success "  $comp [$mod]: PASSED"
    else
        test_results["$key"]="FAIL"
        echo_error "  $comp [$mod]: FAILED"
    fi
}

# ========================= Execute Tests =========================

echo_header "========================================================================"
echo_header "  INT128 ${FEATURE^^} TESTS - Execution"
echo_header "========================================================================"
echo ""

# Define compilers to test
compilers_to_test=()
if [[ "$COMPILER" == "all" ]]; then
    compilers_to_test=("gcc" "clang" "intel" "msvc")
else
    compilers_to_test=("$COMPILER")
fi

# Define modes to test
modes_to_test=()
if [[ "$MODE" == "all" ]]; then
    modes_to_test=("debug" "release")
else
    modes_to_test=("$MODE")
fi

# Execute tests
for comp in "${compilers_to_test[@]}"; do
    for mod in "${modes_to_test[@]}"; do
        run_test "$comp" "$mod"
    done
    echo ""
done

# ========================= Display Summary =========================

echo_header "========================================================================"
echo_header "  TEST RESULTS SUMMARY"
echo_header "========================================================================"
echo ""

# Print header
printf "%-12s | %-10s | %-10s\n" "Compiler" "Debug" "Release"
echo "------------------------------------------------------------------------"

# Print results for each compiler
for comp in "${compilers_to_test[@]}"; do
    debug_result="N/A"
    release_result="N/A"
    
    # Check if we tested debug mode
    if [[ " ${modes_to_test[*]} " =~ " debug " ]] || [[ "$MODE" == "all" ]]; then
        case "${test_results[${comp}_debug]:-NOT_FOUND}" in
            PASS) debug_result="[OK] PASS" ;;
            FAIL) debug_result="[ERROR] FAIL" ;;
            NOT_FOUND) debug_result="- N/A" ;;
        esac
    fi
    
    # Check if we tested release mode
    if [[ " ${modes_to_test[*]} " =~ " release " ]] || [[ "$MODE" == "all" ]]; then
        case "${test_results[${comp}_release]:-NOT_FOUND}" in
            PASS) release_result="[OK] PASS" ;;
            FAIL) release_result="[ERROR] FAIL" ;;
            NOT_FOUND) release_result="- N/A" ;;
        esac
    fi
    
    printf "%-12s | %-10s | %-10s\n" "$comp" "$debug_result" "$release_result"
done

echo ""

# ========================= Exit Status =========================

# Check if any tests failed
failed=0
for key in "${!test_results[@]}"; do
    if [[ "${test_results[$key]}" == "FAIL" ]]; then
        failed=1
        break
    fi
done

if [[ $failed -eq 1 ]]; then
    echo_error "Some tests failed!"
    exit 1
else
    echo_success "All tests passed!"
    exit 0
fi
