#!/usr/bin/env bash
# =============================================================================
# WSL Build and Test - Master Script (Template Unificado)
# =============================================================================
#
# Tests the unified int128_base_t<S> template across all WSL compilers.
#
# Usage:
#   ./scripts/wsl_build_and_test.bash <feature> [compiler] [mode]
#
# Features: tt, algorithm, bits, cmath, concepts, format, iostreams,
#           limits, numeric, ranges, safe, thread_safety, traits
# Compilers: gcc-13, gcc-14, gcc-15, clang-18, clang-19, clang-20, clang-21, icpx, all
# Modes: debug, release, all
#
# Examples:
#   ./scripts/wsl_build_and_test.bash tt all all           # All compilers
#   ./scripts/wsl_build_and_test.bash bits gcc-15 release
#   ./scripts/wsl_build_and_test.bash cmath clang-21 all
# =============================================================================

set -e

# ========================= Colors =========================
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo_info()    { echo -e "${CYAN}[INFO]${NC} $1"; }
echo_success() { echo -e "${GREEN}[OK]${NC} $1"; }
echo_error()   { echo -e "${RED}[FAIL]${NC} $1"; }
echo_header()  { echo -e "${BLUE}$1${NC}"; }

# ========================= Paths =========================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WSL_SCRIPTS_DIR="$SCRIPT_DIR/wsl"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# ========================= Valid Values =========================
VALID_FEATURES=("all" "tt" "algorithm" "bits" "cmath" "concepts" "format" "iostreams" "limits" "numeric" "ranges" "safe" "thread_safety" "traits" "comparison_boost")
VALID_COMPILERS=("gcc-13" "gcc-14" "gcc-15" "clang-18" "clang-19" "clang-20" "clang-21" "icpx" "all")
VALID_MODES=("debug" "release" "all")

# ========================= Compiler Map =========================
declare -A COMPILER_SCRIPTS=(
    ["gcc-13"]="build_gcc13.bash"
    ["gcc-14"]="build_gcc14.bash"
    ["gcc-15"]="build_gcc15.bash"
    ["clang-18"]="build_clang18.bash"
    ["clang-19"]="build_clang19.bash"
    ["clang-20"]="build_clang20.bash"
    ["clang-21"]="build_clang21.bash"
    ["icpx"]="build_icpx.bash"
)

# ========================= Usage =========================
show_usage() {
    echo "Usage: $0 <feature> [compiler] [mode]"
    echo ""
    echo "Tests the unified int128_base_t<S> template."
    echo ""
    echo "Features:  ${VALID_FEATURES[*]}"
    echo "Compilers: ${VALID_COMPILERS[*]}"
    echo "Modes:     ${VALID_MODES[*]}"
    echo ""
    echo "Examples:"
    echo "  $0 tt all all           # All compilers, all modes"
    echo "  $0 bits gcc-15 release  # Just GCC 15 release"
    echo "  $0 cmath clang-21 all   # Clang 21 debug+release"
}

# ========================= Arguments =========================
if [[ $# -lt 1 ]]; then
    show_usage
    exit 1
fi

FEATURE="$1"
COMPILER="${2:-all}"
MODE="${3:-all}"

# Validate feature
valid=false
for f in "${VALID_FEATURES[@]}"; do
    [[ "$FEATURE" == "$f" ]] && valid=true && break
done
if [[ "$valid" == false ]]; then
    echo_error "Invalid feature: $FEATURE"
    echo "Valid: ${VALID_FEATURES[*]}"
    exit 1
fi

# Validate compiler
valid=false
for c in "${VALID_COMPILERS[@]}"; do
    [[ "$COMPILER" == "$c" ]] && valid=true && break
done
if [[ "$valid" == false ]]; then
    echo_error "Invalid compiler: $COMPILER"
    echo "Valid: ${VALID_COMPILERS[*]}"
    exit 1
fi

# Validate mode
valid=false
for m in "${VALID_MODES[@]}"; do
    [[ "$MODE" == "$m" ]] && valid=true && break
done
if [[ "$valid" == false ]]; then
    echo_error "Invalid mode: $MODE"
    echo "Valid: ${VALID_MODES[*]}"
    exit 1
fi

# Check source file exists
SOURCE_FILE="$PROJECT_ROOT/tests/int128_${FEATURE}_extracted_tests.cpp"
if [[ ! -f "$SOURCE_FILE" ]]; then
    echo_error "Source file not found: $SOURCE_FILE"
    exit 1
fi

# ========================= Header =========================
echo_header "============================================================"
echo_header "  WSL Build & Test - int128_${FEATURE}"
echo_header "  Compiler: $COMPILER | Mode: $MODE"
echo_header "============================================================"
echo ""

# ========================= Detect Available Compilers =========================
echo_info "Detecting available compilers..."

declare -A AVAILABLE_COMPILERS
for name in "${!COMPILER_SCRIPTS[@]}"; do
    script="$WSL_SCRIPTS_DIR/${COMPILER_SCRIPTS[$name]}"
    if [[ -f "$script" ]]; then
        # Check if the actual compiler binary exists
        case "$name" in
            gcc-*)   cmd="g++-${name#gcc-}" ;;
            clang-*) cmd="clang++-${name#clang-}" ;;
            icpx)    
                # Try to load oneAPI first
                [[ -f /opt/intel/oneapi/setvars.sh ]] && source /opt/intel/oneapi/setvars.sh --force > /dev/null 2>&1
                cmd="icpx" 
                ;;
        esac
        
        if command -v "$cmd" &> /dev/null; then
            AVAILABLE_COMPILERS["$name"]="$script"
            echo_success "  $name: $($cmd --version 2>&1 | head -1)"
        fi
    fi
done

if [[ ${#AVAILABLE_COMPILERS[@]} -eq 0 ]]; then
    echo_error "No compilers found!"
    exit 1
fi

echo ""

# ========================= Build List =========================
declare -a COMPILERS_TO_RUN

if [[ "$COMPILER" == "all" ]]; then
    for c in "${!AVAILABLE_COMPILERS[@]}"; do
        COMPILERS_TO_RUN+=("$c")
    done
else
    if [[ -n "${AVAILABLE_COMPILERS[$COMPILER]}" ]]; then
        COMPILERS_TO_RUN+=("$COMPILER")
    else
        echo_error "Compiler '$COMPILER' not available"
        exit 1
    fi
fi

# ========================= Execute =========================
declare -A RESULTS
TOTAL=0
PASSED=0

for compiler in "${COMPILERS_TO_RUN[@]}"; do
    script="${AVAILABLE_COMPILERS[$compiler]}"
    
    # Pass feature and mode to the individual script
    if bash "$script" "$FEATURE" "$MODE"; then
        if [[ "$MODE" == "all" ]]; then
            RESULTS["${compiler}_debug"]="PASS"
            RESULTS["${compiler}_release"]="PASS"
            TOTAL=$((TOTAL + 2))
            PASSED=$((PASSED + 2))
        else
            RESULTS["${compiler}_${MODE}"]="PASS"
            TOTAL=$((TOTAL + 1))
            PASSED=$((PASSED + 1))
        fi
    else
        if [[ "$MODE" == "all" ]]; then
            RESULTS["${compiler}_debug"]="FAIL"
            RESULTS["${compiler}_release"]="FAIL"
            TOTAL=$((TOTAL + 2))
        else
            RESULTS["${compiler}_${MODE}"]="FAIL"
            TOTAL=$((TOTAL + 1))
        fi
    fi
done

# ========================= Summary =========================
echo_header "============================================================"
echo_header "  SUMMARY - int128_${FEATURE}"
echo_header "============================================================"
echo ""

# Determine modes to show
modes_to_show=()
if [[ "$MODE" == "all" ]]; then
    modes_to_show=("debug" "release")
else
    modes_to_show=("$MODE")
fi

# Print header
printf "%-15s" "Compiler"
for m in "${modes_to_show[@]}"; do
    printf "%-12s" "$m"
done
echo ""
printf "%-15s" "--------"
for m in "${modes_to_show[@]}"; do
    printf "%-12s" "------"
done
echo ""

# Print results
for compiler in "${COMPILERS_TO_RUN[@]}"; do
    printf "%-15s" "$compiler"
    for m in "${modes_to_show[@]}"; do
        key="${compiler}_${m}"
        result="${RESULTS[$key]:-N/A}"
        if [[ "$result" == "PASS" ]]; then
            printf "${GREEN}%-12s${NC}" "PASS"
        else
            printf "${RED}%-12s${NC}" "FAIL"
        fi
    done
    echo ""
done

echo ""
echo_header "Total: $PASSED/$TOTAL passed"
echo ""

if [[ "$PASSED" -eq "$TOTAL" ]]; then
    echo_success "All tests passed!"
    exit 0
else
    echo_error "Some tests failed"
    exit 1
fi
