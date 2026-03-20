#!/usr/bin/env bash
# ==============================================================================
# run_priority_tests_multicomp.bash
# Compila y ejecuta la suite de tests de prioridad con múltiples compiladores.
#
# Uso:
#   run_priority_tests_multicomp.bash [compiler] [mode]
#
# compiler : gcc | clang | msvc | msvc2022 | msvc2026-comm | msvc2026-ins |
#            intel | all   (default: all)
# mode     : release | debug | all             (default: release)
#
# Ejemplos:
#   run_priority_tests_multicomp.bash
#   run_priority_tests_multicomp.bash gcc release
#   run_priority_tests_multicomp.bash msvc2022 release
#   run_priority_tests_multicomp.bash all release
# ==============================================================================

set -euo pipefail

# ── Argumentos ────────────────────────────────────────────────────────────────

COMPILER="${1:-all}"
MODE="${2:-release}"

# ── Paths ─────────────────────────────────────────────────────────────────────

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

INCLUDE_DIR="include"
TEST_DIR="tests"
BUILD_DIR="build/priority_multicomp"

# ── Compiladores (rutas absolutas MSYS2) ──────────────────────────────────────

GCC_EXE="/c/msys64/ucrt64/bin/g++.exe"
CLANG_EXE="/c/msys64/clang64/bin/clang++.exe"

# vcvarsall.bat paths
VCVARS_2022="D:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat"
VCVARS_2026_COMM="C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvarsall.bat"
VCVARS_2026_INS="C:\\Program Files\\Microsoft Visual Studio\\18\\Insiders\\VC\\Auxiliary\\Build\\vcvarsall.bat"

ICX_EXE="C:\\Program Files (x86)\\Intel\\oneAPI\\compiler\\latest\\bin\\icpx.exe"
INTEL_SETVARS="C:\\Program Files (x86)\\Intel\\oneAPI\\setvars.bat"

# ── Lista de tests de prioridad ────────────────────────────────────────────────

TEST_FILES=(
    "test_priority1_constructors.cpp"
    "test_priority2_magnitude_sign.cpp"
    "test_priority3_representations_ms_ek.cpp"
    "test_priority4_arithmetic.cpp"
    "test_priority5_string_io.cpp"
    "test_priority6_bitwise.cpp"
    "test_priority7_shift.cpp"
    "test_priority8_bitops.cpp"
    "test_priority9_friends.cpp"
    "test_priority10_float.cpp"
    "test_priority11_array.cpp"
    "test_param_bits.cpp"
    "test_param_cmath.cpp"
    "test_param_limits.cpp"
    "test_param_numeric.cpp"
    "test_param_iostreams.cpp"
    "test_native_arithmetic.cpp"
    "test_excess_k_basic.cpp"
    "test_excess_k_comparison.cpp"
    "test_ms_storage.cpp"
    "test_representation_conversions.cpp"
    "test_ms_multiplication.cpp"
    "test_ek_operator_semantics.cpp"
)

# ── Colores ────────────────────────────────────────────────────────────────────

GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'
BLUE='\033[0;34m'; CYAN='\033[0;36m'; NC='\033[0m'

ok()     { echo -e "${GREEN}  [PASS]${NC} $*"; }
fail()   { echo -e "${RED}  [FAIL]${NC} $*"; }
info()   { echo -e "${YELLOW}  ->  ${NC}$*"; }
header() { echo -e "${BLUE}$*${NC}"; }

# ── Contadores globales ────────────────────────────────────────────────────────

declare -A RESULTS   # RESULTS["compiler_mode_testname"] = PASS|FAIL|SKIP
TOTAL_PASS=0
TOTAL_FAIL=0

# ── Función: compilar y ejecutar con GCC o Clang ──────────────────────────────

run_gcc_clang() {
    local comp_label="$1"     # "gcc" | "clang"
    local cxx_exe="$2"
    local mode="$3"
    local build_subdir="$BUILD_DIR/$comp_label/$mode"
    mkdir -p "$build_subdir"

    local cxxflags="-std=c++20 -I$INCLUDE_DIR -Wno-unknown-pragmas"
    case "$mode" in
        release) cxxflags="$cxxflags -O2 -DNDEBUG" ;;
        debug)   cxxflags="$cxxflags -O0 -g -DDEBUG" ;;
    esac

    # Prepend the compiler's own bin dir so its runtime DLLs (e.g. libc++.dll
    # for clang64) are found when executing the compiled binary on Windows.
    local comp_bin_dir
    comp_bin_dir="$(dirname "$cxx_exe")"

    header "  ── $comp_label [$mode] ──"

    local pass=0 fail=0

    for test_file in "${TEST_FILES[@]}"; do
        local test_name="${test_file%.cpp}"
        local src="$TEST_DIR/$test_file"
        local exe="$build_subdir/${test_name}.exe"

        if [[ ! -f "$src" ]]; then
            RESULTS["${comp_label}_${mode}_${test_name}"]="SKIP"
            continue
        fi

        if "$cxx_exe" $cxxflags "$src" -o "$exe" 2>/dev/null; then
            if PATH="${comp_bin_dir}:${PATH}" "$exe" >/dev/null 2>&1; then
                ok "$test_name"
                RESULTS["${comp_label}_${mode}_${test_name}"]="PASS"
                ((pass++)); ((TOTAL_PASS++))
            else
                fail "$test_name"
                RESULTS["${comp_label}_${mode}_${test_name}"]="FAIL"
                ((fail++)); ((TOTAL_FAIL++))
            fi
        else
            fail "$test_name  [compile error]"
            RESULTS["${comp_label}_${mode}_${test_name}"]="FAIL"
            ((fail++)); ((TOTAL_FAIL++))
        fi
    done

    echo -e "  ${CYAN}$comp_label [$mode]: $pass pass, $fail fail${NC}"
    echo ""
}

# ── Función: compilar y ejecutar con MSVC vía cmd.exe ─────────────────────────

run_msvc() {
    local comp_label="$1"    # "msvc2022" | "msvc2026-comm" | "msvc2026-ins"
    local vcvars_path="$2"   # Windows path to vcvarsall.bat
    local mode="$3"
    local build_subdir="$BUILD_DIR/$comp_label/$mode"
    mkdir -p "$build_subdir"

    # MSVC flags
    local cl_flags="/std:c++20 /EHsc /W3 /I include /nologo"
    case "$mode" in
        release) cl_flags="$cl_flags /O2 /DNDEBUG" ;;
        debug)   cl_flags="$cl_flags /Od /Zi /DDEBUG" ;;
    esac

    header "  ── $comp_label [$mode] ──"

    local pass=0 fail=0

    for test_file in "${TEST_FILES[@]}"; do
        local test_name="${test_file%.cpp}"
        # Use absolute paths: vcvarsall.bat can change cmd.exe's working dir,
        # so relative paths would resolve incorrectly.
        local src_win
        src_win="$(cygpath -w "$PROJECT_ROOT/$TEST_DIR/$test_file" 2>/dev/null || echo "$PROJECT_ROOT\\$TEST_DIR\\$test_file")"
        local exe_win
        exe_win="$(cygpath -w "$PROJECT_ROOT/$build_subdir/${test_name}.exe" 2>/dev/null || echo "$PROJECT_ROOT\\$build_subdir\\${test_name}.exe")"
        local inc_win
        inc_win="$(cygpath -w "$PROJECT_ROOT/include" 2>/dev/null || echo "$PROJECT_ROOT\\include")"
        local exe_unix="$build_subdir/${test_name}.exe"

        if [[ ! -f "$TEST_DIR/$test_file" ]]; then
            RESULTS["${comp_label}_${mode}_${test_name}"]="SKIP"
            continue
        fi

        # Rebuild cl_flags with absolute include path
        local cl_flags_abs="${cl_flags/\/I include/\/I \"$inc_win\"}"

        local compile_ok=0
        # Use & (not &&) so cl runs even if vcvarsall.bat exits non-zero
        # (vcvarsall.bat can exit with error when vswhere.exe is missing but
        #  still initializes the environment correctly).
        cmd.exe /c "call \"$vcvars_path\" x64 >nul 2>&1 & cl $cl_flags_abs \"$src_win\" /Fe:\"$exe_win\" >nul 2>&1" \
            && compile_ok=1 || true

        if [[ $compile_ok -eq 1 && -f "$exe_unix" ]]; then
            if "$exe_unix" >/dev/null 2>&1; then
                ok "$test_name"
                RESULTS["${comp_label}_${mode}_${test_name}"]="PASS"
                ((pass++)); ((TOTAL_PASS++))
            else
                fail "$test_name"
                RESULTS["${comp_label}_${mode}_${test_name}"]="FAIL"
                ((fail++)); ((TOTAL_FAIL++))
            fi
        else
            fail "$test_name  [compile error]"
            RESULTS["${comp_label}_${mode}_${test_name}"]="FAIL"
            ((fail++)); ((TOTAL_FAIL++))
        fi
    done

    echo -e "  ${CYAN}$comp_label [$mode]: $pass pass, $fail fail${NC}"
    echo ""
}

# ── Función: compilar y ejecutar con Intel ICX vía cmd.exe ────────────────────
# Intel ICX necesita tanto setvars.bat (Intel) como vcvarsall.bat (MSVC linker).
# Aquí se usan ambos en cadena.

run_intel() {
    local mode="$1"
    local build_subdir="$BUILD_DIR/intel/$mode"
    mkdir -p "$build_subdir"

    local icx_flags="-std=c++20 -EHsc -W3 -I include"
    case "$mode" in
        release) icx_flags="$icx_flags -O2 -DNDEBUG" ;;
        debug)   icx_flags="$icx_flags -Od -DDEBUG" ;;
    esac

    header "  ── intel ICX [$mode] ──"

    local pass=0 fail=0

    for test_file in "${TEST_FILES[@]}"; do
        local test_name="${test_file%.cpp}"
        local src_win
        src_win="$(cygpath -w "$TEST_DIR/$test_file" 2>/dev/null || echo "$TEST_DIR\\$test_file")"
        local exe_win
        exe_win="$(cygpath -w "$build_subdir/${test_name}.exe" 2>/dev/null || echo "$build_subdir\\${test_name}.exe")"
        local exe_unix="$build_subdir/${test_name}.exe"

        if [[ ! -f "$TEST_DIR/$test_file" ]]; then
            RESULTS["intel_${mode}_${test_name}"]="SKIP"
            continue
        fi

        local compile_ok=0
        # Use & (not &&) — setvars.bat can exit non-zero but still sets env.
        cmd.exe /c "call \"$INTEL_SETVARS\" --force >nul 2>&1 & \"$ICX_EXE\" $icx_flags \"$src_win\" -o \"$exe_win\" >nul 2>&1" \
            && compile_ok=1 || true

        if [[ $compile_ok -eq 1 && -f "$exe_unix" ]]; then
            if "$exe_unix" >/dev/null 2>&1; then
                ok "$test_name"
                RESULTS["intel_${mode}_${test_name}"]="PASS"
                ((pass++)); ((TOTAL_PASS++))
            else
                fail "$test_name"
                RESULTS["intel_${mode}_${test_name}"]="FAIL"
                ((fail++)); ((TOTAL_FAIL++))
            fi
        else
            fail "$test_name  [compile error]"
            RESULTS["intel_${mode}_${test_name}"]="FAIL"
            ((fail++)); ((TOTAL_FAIL++))
        fi
    done

    echo -e "  ${CYAN}intel [$mode]: $pass pass, $fail fail${NC}"
    echo ""
}

# ── Función: resumen por compilador ───────────────────────────────────────────

summary_line() {
    local comp="$1"
    local mode="$2"
    local pass=0 fail=0 skip=0
    for key in "${!RESULTS[@]}"; do
        if [[ "$key" == "${comp}_${mode}_"* ]]; then
            case "${RESULTS[$key]}" in
                PASS) ((pass++)) ;;
                FAIL) ((fail++)) ;;
                SKIP) ((skip++)) ;;
            esac
        fi
    done
    if [[ $((pass + fail)) -eq 0 ]]; then
        printf "  %-24s | %-6s | %s\n" "$comp [$mode]" "N/A" "-"
    elif [[ $fail -eq 0 ]]; then
        printf "  %-24s | ${GREEN}%-6s${NC} | %d/%d pass\n" "$comp [$mode]" "OK" "$pass" "$((pass+fail))"
    else
        printf "  %-24s | ${RED}%-6s${NC} | %d pass, %d FAIL\n" "$comp [$mode]" "FAIL" "$pass" "$fail"
    fi
}

# ── Main ──────────────────────────────────────────────────────────────────────

header "$(printf '=%.0s' {1..70})"
header " Priority Test Suite — Multi-Compiler Runner"
header "$(printf '=%.0s' {1..70})"
echo ""

MODES=()
case "$MODE" in
    all) MODES=("release" "debug") ;;
    *)   MODES=("$MODE") ;;
esac

COMPILERS=()
case "$COMPILER" in
    all) COMPILERS=("gcc" "clang" "msvc2022" "msvc2026-comm" "msvc2026-ins" "intel") ;;
    *)   COMPILERS=("$COMPILER") ;;
esac

for comp in "${COMPILERS[@]}"; do
    for mode in "${MODES[@]}"; do
        case "$comp" in
            gcc)
                [[ -x "$GCC_EXE" ]] && run_gcc_clang "gcc" "$GCC_EXE" "$mode" \
                    || info "GCC not found at $GCC_EXE — skipping"
                ;;
            clang)
                [[ -x "$CLANG_EXE" ]] && run_gcc_clang "clang" "$CLANG_EXE" "$mode" \
                    || info "Clang not found at $CLANG_EXE — skipping"
                ;;
            msvc2022)
                run_msvc "msvc2022" "$VCVARS_2022" "$mode"
                ;;
            msvc2026-comm)
                run_msvc "msvc2026-comm" "$VCVARS_2026_COMM" "$mode"
                ;;
            msvc2026-ins)
                run_msvc "msvc2026-ins" "$VCVARS_2026_INS" "$mode"
                ;;
            intel)
                run_intel "$mode"
                ;;
            *)
                echo "Unknown compiler: $comp  (valid: gcc clang msvc2022 msvc2026-comm msvc2026-ins intel all)"
                exit 1
                ;;
        esac
    done
done

# ── Summary matrix ────────────────────────────────────────────────────────────

header "$(printf '=%.0s' {1..70})"
header " SUMMARY"
header "$(printf '=%.0s' {1..70})"
echo ""
printf "  %-24s | %-6s | %s\n" "Compiler [mode]" "Status" "Details"
printf "  %.0s─" {1..68}; echo ""

for comp in "${COMPILERS[@]}"; do
    for mode in "${MODES[@]}"; do
        summary_line "$comp" "$mode"
    done
done

echo ""
echo -e "  ${CYAN}Grand total: ${TOTAL_PASS} pass, ${TOTAL_FAIL} fail${NC}"
echo ""

[[ $TOTAL_FAIL -eq 0 ]] && exit 0 || exit 1
