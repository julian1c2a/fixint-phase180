#!/bin/bash
# =============================================================================
# build_demos.bash - Automated demo compilation script
# Part of int128 Library - https://github.com/[repo]
# License: BSL-1.0
# =============================================================================
#
# Usage: bash scripts/build_demos.bash [category] [compiler] [mode]
#
# Arguments:
#   category  - tutorials|examples|showcase|general|all (default: all)
#   compiler  - gcc|clang|all (default: gcc)
#   mode      - debug|release (default: release)
#
# Examples:
#   bash scripts/build_demos.bash                      # Build all demos with gcc release
#   bash scripts/build_demos.bash tutorials            # Build tutorials only
#   bash scripts/build_demos.bash all clang release    # Build all with clang release
#   bash scripts/build_demos.bash examples gcc debug   # Build examples with gcc debug

set -e

# =============================================================================
# Configuration
# =============================================================================

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INCLUDE_DIR="${PROJECT_ROOT}/include"
BUILD_BASE="${PROJECT_ROOT}/build/build_demos"

DEMO_CATEGORIES=("tutorials" "examples" "showcase" "general")

# Default arguments
CATEGORY="${1:-all}"
COMPILER="${2:-gcc}"
MODE="${3:-release}"

# Compiler paths
GCC_PATH="/c/msys64/ucrt64/bin/g++"
CLANG_PATH="/c/msys64/clang64/bin/clang++"

# =============================================================================
# Compiler flags
# =============================================================================

get_compiler_flags() {
    local compiler="$1"
    local mode="$2"
    
    local common_flags="-std=c++20 -I${INCLUDE_DIR}"
    local mode_flags=""
    local compiler_flags=""
    
    # Mode-specific flags
    case "$mode" in
        debug)
            mode_flags="-O0 -g -DDEBUG"
            ;;
        release)
            mode_flags="-O2 -DNDEBUG"
            ;;
        *)
            echo "Unknown mode: $mode" >&2
            exit 1
            ;;
    esac
    
    # Compiler-specific flags
    case "$compiler" in
        gcc)
            compiler_flags="-Wall -Wextra -Wpedantic"
            ;;
        clang)
            compiler_flags="-Wall -Wextra -Wpedantic"
            ;;
        *)
            echo "Unknown compiler: $compiler" >&2
            exit 1
            ;;
    esac
    
    echo "${common_flags} ${mode_flags} ${compiler_flags}"
}

get_compiler_path() {
    local compiler="$1"
    
    case "$compiler" in
        gcc)
            echo "$GCC_PATH"
            ;;
        clang)
            echo "$CLANG_PATH"
            ;;
        *)
            echo "Unknown compiler: $compiler" >&2
            exit 1
            ;;
    esac
}

# =============================================================================
# Build functions
# =============================================================================

build_demo() {
    local demo_file="$1"
    local compiler="$2"
    local mode="$3"
    
    local demo_name
    demo_name=$(basename "${demo_file}" .cpp)
    local category
    category=$(basename "$(dirname "${demo_file}")")
    
    local output_dir="${BUILD_BASE}/${compiler}/${mode}/${category}"
    local output_file="${output_dir}/${demo_name}"
    
    # Add .exe extension on Windows
    if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
        output_file="${output_file}.exe"
    fi
    
    # Create output directory
    mkdir -p "$output_dir"
    
    local compiler_path
    compiler_path=$(get_compiler_path "$compiler")
    
    local flags
    flags=$(get_compiler_flags "$compiler" "$mode")
    
    # Check if demo needs threading support
    local thread_flags=""
    if grep -qE '#include\s*<thread>|#include\s*<atomic>|thread_safety' "$demo_file" 2>/dev/null; then
        thread_flags="-pthread"
        if [[ "$compiler" == "gcc" ]]; then
            thread_flags="${thread_flags} -latomic"
        fi
    fi
    
    # Build command
    local cmd="${compiler_path} ${flags} ${thread_flags} -o ${output_file} ${demo_file}"
    
    # Execute compilation
    if $cmd 2>/dev/null; then
        echo "[OK] ${category}/${demo_name}"
        return 0
    else
        echo "[FAIL] ${category}/${demo_name}"
        return 1
    fi
}

build_category() {
    local category="$1"
    local compiler="$2"
    local mode="$3"
    
    local demo_dir="${PROJECT_ROOT}/demos/${category}"
    
    if [[ ! -d "$demo_dir" ]]; then
        echo "[SKIP] Category not found: ${category}"
        return 0
    fi
    
    local ok_count=0
    local fail_count=0
    
    for demo_file in "${demo_dir}"/*.cpp; do
        if [[ -f "$demo_file" ]]; then
            if build_demo "$demo_file" "$compiler" "$mode"; then
                ((ok_count++)) || true
            else
                ((fail_count++)) || true
            fi
        fi
    done
    
    echo "  Subtotal ${category}: ${ok_count} OK, ${fail_count} FAIL"
    
    return $fail_count
}

# =============================================================================
# Main
# =============================================================================

main() {
    echo "============================================================"
    echo "  int128 Library - Demo Build System"
    echo "============================================================"
    echo ""
    echo "Configuration:"
    echo "  Category: ${CATEGORY}"
    echo "  Compiler: ${COMPILER}"
    echo "  Mode:     ${MODE}"
    echo ""
    echo "------------------------------------------------------------"
    
    local total_ok=0
    local total_fail=0
    local compilers_to_use=()
    
    # Determine compilers to use
    if [[ "$COMPILER" == "all" ]]; then
        compilers_to_use=("gcc" "clang")
    else
        compilers_to_use=("$COMPILER")
    fi
    
    # Determine categories to build
    local categories_to_build=()
    if [[ "$CATEGORY" == "all" ]]; then
        categories_to_build=("${DEMO_CATEGORIES[@]}")
    else
        categories_to_build=("$CATEGORY")
    fi
    
    # Build loop
    for comp in "${compilers_to_use[@]}"; do
        echo ""
        echo ">>> Building with ${comp} (${MODE})"
        echo ""
        
        for cat in "${categories_to_build[@]}"; do
            build_category "$cat" "$comp" "$MODE"
        done
    done
    
    echo ""
    echo "============================================================"
    echo "  Build Complete"
    echo "============================================================"
    echo ""
    echo "Output directory: ${BUILD_BASE}/"
    echo ""
    echo "To run a demo:"
    echo "  ${BUILD_BASE}/gcc/release/tutorials/01_basic_operations"
    echo ""
}

main "$@"
