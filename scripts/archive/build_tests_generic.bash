#!/usr/bin/env bash
# ==============================================================================
# build_tests_generic.bash - Script genérico para compilar tests
# ==============================================================================
# Compila tests del template unificado con todos los compiladores
# Par con: check_generic.bash
# 
# Uso:
#   build_tests_generic.bash <feature> [compiler] [mode] [print]
#
# Argumentos:
#   feature  : tt | t | bits | numeric | algorithm | traits | limits | concepts |
#              cmath | iostreams | format | ranges | safe | thread_safety |
#              comparison_boost | interop
#   compiler : gcc | clang | intel | msvc | all (default: all)
#   mode     : debug | release | all (default: all)
#   print    : yes | no (default: no) - Imprime comandos de compilación
#
# Ejemplos:
#   build_tests_generic.bash bits
#   build_tests_generic.bash numeric gcc release
#   build_tests_generic.bash algorithm all all yes
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

if [[ $# -lt 1 ]]; then
    echo "Error: Se requiere al menos 1 argumento (feature)"
    echo ""
    echo "Uso:"
    echo "  $0 <feature> [compiler] [mode] [print]"
    echo ""
    echo "Features válidas:"
    echo "  tt, t, bits, numeric, algorithm, traits, limits, concepts,"
    echo "  cmath, iostreams, format, ranges, safe, thread_safety,"
    echo "  comparison_boost, interop"
    echo ""
    echo "Ejemplos:"
    echo "  $0 bits"
    echo "  $0 numeric gcc release"
    echo "  $0 algorithm all all yes"
    echo "  $0 all gcc release      # Build ALL features"
    exit 1
fi

FEATURE="$1"
COMPILER="${2:-all}"
MODE="${3:-all}"
PRINT_COMMANDS="${4:-no}"

# ========================= Handle FEATURE=all =========================

ALL_FEATURES=("tt" "traits" "limits" "concepts" "algorithm" "iostreams" "bits" "cmath" "numeric" "ranges" "format" "safe" "thread_safety" "comparison_boost")

if [[ "$FEATURE" == "all" ]]; then
    echo "========================================="
    echo " Building ALL features (tests)"
    echo "========================================="
    for feat in "${ALL_FEATURES[@]}"; do
        echo ""
        echo ">>> Building feature: $feat"
        bash "$0" "$feat" "$COMPILER" "$MODE" "$PRINT_COMMANDS"
    done
    echo ""
    echo "========================================="
    echo " [OK] ALL features built!"
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

VALID_MODES=("debug" "debug-asan" "debug-ubsan" "release" "release-O1" "release-O2" "release-O3" "release-Ofast" "all")
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
NC='\033[0m'

echo_success() { echo -e "${GREEN}[OK] $1${NC}"; }
echo_error() { echo -e "${RED}[ERROR] $1${NC}"; }
echo_info() { echo -e "${YELLOW}-> $1${NC}"; }
echo_header() { echo -e "${BLUE}$1${NC}"; }

# ========================= Determine Paths =========================

# Tests usan template unificado (int128_*)
if [[ "$FEATURE" == "tt" ]]; then
    SOURCE_FILE="tests/int128_tt_extracted_tests.cpp"
else
    SOURCE_FILE="tests/int128_${FEATURE}_extracted_tests.cpp"
fi

BUILD_DIR="build/build_tests"

echo_info "Building tests for feature '$FEATURE' (template unificado)..."

# ========================= Check Source File =========================

if [[ ! -f "$SOURCE_FILE" ]]; then
    echo_error "Source file not found: $SOURCE_FILE"
    exit 1
fi

echo ""

# ========================= Compilation Function =========================

compile_with() {
    local compiler_name="$1"
    local compiler_cmd="$2"
    local skip_check="${3:-}"
    
    echo_info "Building with $compiler_name..."
    
    # Check if compiler exists (unless skip_check is set)
    if [[ -z "$skip_check" ]]; then
        if ! command -v "$compiler_cmd" &>/dev/null; then
            echo_error "$compiler_name not found ($compiler_cmd). Skipping..."
            echo ""
            return
        fi
    fi
    
    # Define modes to compile
    local modes_to_compile=()
    if [[ "$MODE" == "all" ]]; then
        # GCC en Windows/MSYS2 no tiene sanitizers disponibles
        if [[ "$compiler_name" == "gcc" && ("$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == mingw*) ]]; then
            modes_to_compile=("debug" "release" "release-O1" "release-O2" "release-O3" "release-Ofast")
            echo_info "  (Nota: sanitizers no disponibles para GCC en MSYS2)"
        else
            modes_to_compile=("debug" "debug-asan" "debug-ubsan" "release" "release-O1" "release-O2" "release-O3" "release-Ofast")
        fi
    else
        # Si el usuario pide específicamente un sanitizer con GCC en MSYS2, advertir
        if [[ "$compiler_name" == "gcc" && ("$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == mingw*) && ("$MODE" == "debug-asan" || "$MODE" == "debug-ubsan") ]]; then
            echo_error "Sanitizers no disponibles para GCC en MSYS2. Use Clang o Intel."
            return
        fi
        modes_to_compile=("$MODE")
    fi
    
    # Compile for each mode
    for mode in "${modes_to_compile[@]}"; do
        local output_dir="$BUILD_DIR/$compiler_name/$mode"
        mkdir -p "$output_dir"
        
        # Output: int128_<feature>_tests_<compiler>
        local output="$output_dir/int128_${FEATURE}_tests_${compiler_name}"
        
        # Add .exe extension on Windows/MSYS2/Cygwin (all compilers produce .exe)
        if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "mingw"* ]]; then
            output="${output}.exe"
        fi
        
        local common_flags=""
        local mode_flags=""
        local link_flags=""
        
        # Set compiler-specific flags
        if [[ "$compiler_name" == "msvc" ]]; then
            export MSYS2_ARG_CONV_EXCL="*"
            
            local win_source=$(cygpath -w "$SOURCE_FILE" 2>/dev/null || echo "$SOURCE_FILE" | sed 's|/|\\|g')
            local win_output=$(cygpath -w "$output" 2>/dev/null || echo "$output" | sed 's|/|\\|g')
            local win_include=$(cygpath -w "./include" 2>/dev/null || echo ".\\include")
            
            common_flags="/std:c++20 /W4 /EHsc /I\"$win_include\""
            
            case "$mode" in
                debug)        mode_flags="/Od /Zi /DDEBUG" ;;
                debug-asan)   mode_flags="/Od /Zi /DDEBUG /fsanitize=address" ;;
                debug-ubsan)  mode_flags="/Od /Zi /DDEBUG /RTC1" ;;
                release-O1)   mode_flags="/O1 /DNDEBUG" ;;
                release-O2)   mode_flags="/O2 /DNDEBUG" ;;
                release-O3)   mode_flags="/Ox /GL /DNDEBUG" ;;
                release-Ofast) mode_flags="/Ox /GL /fp:fast /DNDEBUG" ;;
                *)            mode_flags="/O2 /DNDEBUG" ;;
            esac
            
            local cmd="$compiler_cmd $common_flags $mode_flags \"$win_source\" /Fe:\"$win_output\""
            
        elif [[ "$compiler_name" == "intel" ]]; then
            local win_source=$(cygpath -w "$SOURCE_FILE" 2>/dev/null || echo "$SOURCE_FILE" | sed 's|/|\\|g')
            local win_output=$(cygpath -w "$output" 2>/dev/null || echo "$output" | sed 's|/|\\|g')
            local win_include=$(cygpath -w "./include" 2>/dev/null || echo ".\\include")
            
            common_flags="-Qstd:c++20 -W4 -EHsc -Xclang -fcxx-exceptions -I\"$win_include\""
            
            case "$mode" in
                debug)        mode_flags="-Od -Zi -DDEBUG" ;;
                debug-asan)   mode_flags="-Od -Zi -DDEBUG -fsanitize=address" ;;
                debug-ubsan)  mode_flags="-Od -Zi -DDEBUG -fsanitize=undefined" ;;
                release-O1)   mode_flags="-O1 -DNDEBUG" ;;
                release-O2)   mode_flags="-O2 -DNDEBUG" ;;
                release-O3)   mode_flags="-O3 -Qipo -DNDEBUG" ;;
                release-Ofast) mode_flags="-O3 -Qipo -fp:fast -DNDEBUG" ;;
                *)            mode_flags="-O2 -DNDEBUG" ;;
            esac
            
            local cmd="$compiler_cmd $common_flags $mode_flags \"$win_source\" -Fe:\"$win_output\""
            
        else
            # GCC/Clang
            common_flags="-std=c++20 -Wall -Wextra -pedantic -I./include"
            
            # Check if source file uses threading
            if grep -q -E '<thread>|std::thread|pthread|thread_safety\.hpp' "$SOURCE_FILE"; then
                common_flags="$common_flags -pthread"
                if grep -q -E '<atomic>|std::atomic|atomic_|thread_safety\.hpp' "$SOURCE_FILE"; then
                    link_flags="-latomic"
                fi
            fi
            
            case "$mode" in
                debug)        mode_flags="-O0 -g3 -DDEBUG" ;;
                debug-asan)   mode_flags="-O0 -g3 -DDEBUG -fsanitize=address -fno-omit-frame-pointer"
                              link_flags="$link_flags -fsanitize=address" ;;
                debug-ubsan)  mode_flags="-O0 -g3 -DDEBUG -fsanitize=undefined -fno-omit-frame-pointer"
                              link_flags="$link_flags -fsanitize=undefined" ;;
                release-O1)   mode_flags="-O1 -DNDEBUG" ;;
                release-O2)   mode_flags="-O2 -DNDEBUG" ;;
                release-O3)
                    if [[ "$compiler_name" == "gcc" ]]; then
                        mode_flags="-O3 -fexpensive-optimizations -funroll-loops -ftree-vectorize -march=native -DNDEBUG"
                    else
                        mode_flags="-O3 -funroll-loops -ftree-vectorize -march=native -DNDEBUG"
                    fi ;;
                release-Ofast)
                    if [[ "$compiler_name" == "gcc" ]]; then
                        mode_flags="-Ofast -fexpensive-optimizations -funroll-loops -ftree-vectorize -ffast-math -march=native -DNDEBUG"
                    else
                        mode_flags="-O3 -ffast-math -funroll-loops -ftree-vectorize -march=native -DNDEBUG"
                    fi ;;
                *)            mode_flags="-O3 -march=native -DNDEBUG" ;;
            esac
            
            local cmd="$compiler_cmd $common_flags $mode_flags \"$SOURCE_FILE\" -o \"$output\" $link_flags"
        fi
        
        echo_info "  Compiling [$mode]..."
        
        if [[ "$PRINT_COMMANDS" == "yes" ]]; then
            echo_header "    $ $cmd"
        fi
        
        if eval "$cmd" 2>&1; then
            if [[ -f "$output" ]]; then
                echo_success "  $compiler_name [$mode]: $output"
            else
                echo_error "  $compiler_name [$mode]: executable not created"
            fi
        else
            echo_error "  $compiler_name [$mode]: compilation failed"
        fi
    done
    
    echo ""
}

# ========================= Compile with Each Compiler =========================

if [[ "$COMPILER" == "gcc" || "$COMPILER" == "all" ]]; then
    GCC_CXX="${GCC_CXX:-g++}"
    compile_with "gcc" "$GCC_CXX" "skip_check"
fi

if [[ "$COMPILER" == "clang" || "$COMPILER" == "all" ]]; then
    CLANG_CXX="${CLANG_CXX:-clang++}"
    compile_with "clang" "$CLANG_CXX"
fi

if [[ "$COMPILER" == "intel" || "$COMPILER" == "all" ]]; then
    INTEL_CXX="${INTEL_CXX:-icx}"
    compile_with "intel" "$INTEL_CXX"
fi

if [[ "$COMPILER" == "msvc" || "$COMPILER" == "all" ]]; then
    MSVC_CXX="${MSVC_CXX:-cl.exe}"
    compile_with "msvc" "$MSVC_CXX"
fi

# ========================= Summary =========================

echo_success "Build tests complete for feature '$FEATURE'!"
