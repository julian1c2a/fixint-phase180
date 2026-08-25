#!/usr/bin/env bash
# ==============================================================================
# run_generic.bash - Script genérico de ejecución de benchmarks y demos
# ==============================================================================
# Par con: build_benchs_generic.bash (para benchmarks)
#
# Uso:
#   # Para benchs:
#   run_generic.bash <feature> [compiler] [mode]
#   
#   # Para demos:
#   run_generic.bash demos <category> <demo_name> [compiler] [mode] [args...]
#
# Argumentos:
#   feature  : tt | bits | numeric | algorithm | iostreams | cmath | etc.
#   compiler : gcc | clang | intel | msvc | all (default: all)
#   mode     : debug | release | all (default: all)
#   args     : (solo demos) argumentos adicionales para pasar al demo
#
# Ejemplos Benchmarks:
#   run_generic.bash bits
#   run_generic.bash numeric gcc release
#   run_generic.bash algorithm all all
#
# Ejemplos Demos:
#   run_generic.bash demos tutorials 01_basic_operations gcc release
#   run_generic.bash demos examples ipv6_address clang debug
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

if [[ $# -lt 1 ]]; then
    echo "Error: Se requiere al menos 1 argumento"
    echo ""
    echo "Uso Benchmarks:"
    echo "  $0 <feature> [compiler] [mode]"
    echo "  $0 bits"
    echo "  $0 numeric gcc release"
    echo ""
    echo "Uso Demos:"
    echo "  $0 demos <category> <demo_name> [compiler] [mode]"
    exit 1
fi

# ========================= Detect Syntax =========================

IS_DEMO=false
IS_BENCH=false
DEMO_ARGS=()

if [[ "$1" == "demos" ]]; then
    IS_DEMO=true
    CATEGORY="${2:-}"
    DEMO_NAME="${3:-}"
    COMPILER="${4:-all}"
    MODE="${5:-all}"
    # Remaining arguments are extra args for the demo
    shift 5 2>/dev/null || true
    DEMO_ARGS=("$@")
    
    if [[ -z "$CATEGORY" ]]; then
        echo "Error: Se requiere la categoría del demo"
        exit 1
    fi
    
    if [[ -z "$DEMO_NAME" ]]; then
        echo "Error: Se requiere el nombre del demo"
        exit 1
    fi
    
    # Validate category
    VALID_CATEGORIES=("general" "tutorials" "examples" "showcase" "comparison" "performance" "integration")
    if [[ ! " ${VALID_CATEGORIES[*]} " =~ " ${CATEGORY} " ]]; then
        echo "Error: CATEGORY debe ser uno de: ${VALID_CATEGORIES[*]}"
        exit 1
    fi

else
    # Sintaxis benchmarks: <feature> [compiler] [mode]
    IS_BENCH=true
    FEATURE="$1"
    COMPILER="${2:-all}"
    MODE="${3:-all}"
    
    # Validate feature
    VALID_FEATURES=("all" "tt" "traits" "limits" "concepts" "algorithm" "iostreams" "bits" "cmath" "numeric" "ranges" "format" "safe" "thread_safety" "comparison_boost")
    if [[ ! " ${VALID_FEATURES[*]} " =~ " ${FEATURE} " ]]; then
        echo "Error: FEATURE debe ser uno de: ${VALID_FEATURES[*]}"
        exit 1
    fi
    
    # Handle FEATURE=all
    ALL_FEATURES=("tt" "traits" "limits" "concepts" "algorithm" "iostreams" "bits" "cmath" "numeric" "ranges" "format" "safe" "thread_safety" "comparison_boost")
    if [[ "$FEATURE" == "all" ]]; then
        echo "========================================="
        echo " Running ALL features (benchmarks)"
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

if [[ "$IS_DEMO" == true ]]; then
    BUILD_DIR="$PROJECT_ROOT/build/build_demos"
else
    BUILD_DIR="$PROJECT_ROOT/build/build_benchs"
fi

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

# ========================= Execution Functions =========================

run_benchmark() {
    local comp="$1"
    local mod="$2"
    
    local exe_name="int128_${FEATURE}_benchs_${comp}"
    
    # On Windows/MSYS2/Cygwin, ALL compilers produce .exe files
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "mingw"* ]]; then
        exe_name="${exe_name}.exe"
    fi
    
    local exe_path="$BUILD_DIR/$comp/$mod/$exe_name"
    
    if [[ ! -f "$exe_path" ]]; then
        echo_info "Skipping: $comp [$mod] (not found)"
        return 1
    fi
    
    echo_header "--------------------------------------------------------------------"
    echo_info "Running: $comp [$mod]"
    echo_header "--------------------------------------------------------------------"
    
    if "$exe_path"; then
        echo_success "  $comp [$mod]: completed"
        return 0
    else
        echo_error "  $comp [$mod]: failed"
        return 1
    fi
}

run_demo() {
    local comp="$1"
    local mod="$2"
    shift 2
    local extra_args=("$@")
    
    local exe_name="${DEMO_NAME}"
    
    # On Windows/MSYS2/Cygwin, ALL compilers produce .exe files
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "mingw"* ]]; then
        exe_name="${exe_name}.exe"
    fi
    
    local exe_path="$BUILD_DIR/$comp/$mod/$exe_name"
    
    if [[ ! -f "$exe_path" ]]; then
        echo_info "Skipping: $comp [$mod] (not found)"
        return 1
    fi
    
    echo_header "--------------------------------------------------------------------"
    echo_info "Running demo: ${CATEGORY}/${DEMO_NAME} with $comp [$mod]"
    if [[ ${#extra_args[@]} -gt 0 ]]; then
        echo_info "Arguments: ${extra_args[*]}"
    fi
    echo_header "--------------------------------------------------------------------"
    echo ""
    
    # Run demo with arguments and show output
    "$exe_path" "${extra_args[@]}"
    local exit_code=$?
    
    echo ""
    if [[ $exit_code -eq 0 ]]; then
        echo_success "  $comp [$mod]: completed (exit code: $exit_code)"
        return 0
    else
        echo_error "  $comp [$mod]: failed (exit code: $exit_code)"
        return 1
    fi
}

# ========================= Execute Benchmarks or Demos =========================

if [[ "$IS_DEMO" == true ]]; then
    echo_header "========================================================================"
    echo_header "  DEMO EXECUTION: ${CATEGORY}/${DEMO_NAME}"
    echo_header "========================================================================"
    echo ""
else
    echo_header "========================================================================"
    echo_header "  INT128 ${FEATURE^^} BENCHMARKS - Execution"
    echo_header "========================================================================"
    echo ""
fi

executed=0
failed=0
skipped=0

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

# Execute benchmarks or demos
for comp in "${compilers_to_test[@]}"; do
    for mod in "${modes_to_test[@]}"; do
        if [[ "$IS_DEMO" == true ]]; then
            if run_demo "$comp" "$mod" "${DEMO_ARGS[@]}"; then
                executed=$((executed + 1))
            else
                # Check if file exists to determine if skipped or failed
                local check_exe="${DEMO_NAME}"
                # On Windows/MSYS2/Cygwin, ALL compilers produce .exe files
                if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "mingw"* ]]; then
                    check_exe="${check_exe}.exe"
                fi
                if [[ -f "$BUILD_DIR/$comp/$mod/$check_exe" ]]; then
                    failed=$((failed + 1))
                else
                    skipped=$((skipped + 1))
                fi
            fi
        else
            if run_benchmark "$comp" "$mod"; then
                executed=$((executed + 1))
            else
                if [[ -f "$BUILD_DIR/$comp/$mod/int128_${FEATURE}_benchs_${comp}"* ]]; then
                    failed=$((failed + 1))
                else
                    skipped=$((skipped + 1))
                fi
            fi
        fi
        echo ""
    done
done

# ========================= Summary =========================

echo_header "========================================================================"
if [[ "$IS_DEMO" == true ]]; then
    echo_header "  DEMO EXECUTION SUMMARY"
else
    echo_header "  BENCHMARK EXECUTION SUMMARY"
fi
echo_header "========================================================================"
echo ""
echo "Executed: $executed"
echo "Failed: $failed"
echo "Skipped: $skipped"
echo ""

if [[ $failed -gt 0 ]]; then
    exit 1
fi
