#!/usr/bin/env bash
# ==============================================================================
# build_generic.bash - Script genérico para compilar demos
# ==============================================================================
# Compila demos con todos los compiladores (GCC, Clang, Intel, MSVC)
# 
# Para tests y benchmarks, usar:
#   - build_tests_generic.bash (par con check_generic.bash)
#   - build_benchs_generic.bash (par con run_generic.bash)
# 
# Uso:
#   build_generic.bash <category> <demo_name> [compiler] [mode] [print]
#
# Argumentos:
#   category  : general | tutorials | examples | showcase | comparison | performance | integration
#   demo_name : Nombre del demo (sin .cpp)
#   compiler  : gcc | clang | intel | msvc | all (default: all)
#   mode      : debug | release | all (default: all)
#   print     : yes | no (default: no) - Imprime comandos de compilación
#
# Ejemplos:
#   build_generic.bash tutorials 01_basic_operations gcc release
#   build_generic.bash examples ipv6_address clang debug
#   build_generic.bash showcase main gcc release
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

if [[ $# -lt 2 ]]; then
    echo "Uso: $0 <category> <demo_name> [compiler] [mode] [print]"
    echo ""
    echo "Categorías: general, tutorials, examples, showcase, comparison, performance, integration"
    echo ""
    echo "Ejemplos:"
    echo "  $0 tutorials 01_basic_operations gcc release"
    echo "  $0 examples ipv6_address clang debug"
    exit 1
fi

CATEGORY="$1"
DEMO_NAME="$2"
COMPILER="${3:-all}"
MODE="${4:-all}"
PRINT_COMMANDS="${5:-no}"

# ========================= Validation =========================

VALID_CATEGORIES=("general" "tutorials" "examples" "showcase" "comparison" "performance" "integration")
if [[ ! " ${VALID_CATEGORIES[*]} " =~ " ${CATEGORY} " ]]; then
    echo "Error: CATEGORY debe ser uno de: ${VALID_CATEGORIES[*]}"
    exit 1
fi

VALID_COMPILERS=("gcc" "clang" "intel" "msvc" "all")
if [[ ! " ${VALID_COMPILERS[*]} " =~ " ${COMPILER} " ]]; then
    echo "Error: COMPILER debe ser uno de: ${VALID_COMPILERS[*]}"
    exit 1
fi

VALID_MODES=("debug" "release" "release-O1" "release-O2" "release-O3" "release-Ofast" "all")
if [[ ! " ${VALID_MODES[*]} " =~ " ${MODE} " ]]; then
    echo "Error: MODE debe ser uno de: ${VALID_MODES[*]}"
    exit 1
fi

# ========================= Configuration =========================

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

SOURCE_FILE="demos/${CATEGORY}/${DEMO_NAME}.cpp"
BUILD_DIR="build/build_demos"

if [[ ! -f "$SOURCE_FILE" ]]; then
    echo "Error: Archivo fuente no encontrado: $SOURCE_FILE"
    exit 1
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

echo_info "Building demo: ${CATEGORY}/${DEMO_NAME}..."
echo ""

# ========================= Compilation Function =========================

compile_with() {
    local compiler_name="$1"
    local compiler_cmd="$2"
    local skip_check="${3:-}"
    
    echo_info "Building with $compiler_name..."
    
    # Check if compiler exists
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
        modes_to_compile=("debug" "release")
    else
        modes_to_compile=("$MODE")
    fi
    
    # Compile for each mode
    for mode in "${modes_to_compile[@]}"; do
        local output_dir="$BUILD_DIR/$compiler_name/$mode"
        mkdir -p "$output_dir"
        
        local output="$output_dir/${DEMO_NAME}"
        
        # Add .exe extension on Windows/MSYS2
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
            
            if [[ "$mode" == "debug" ]]; then
                mode_flags="/Od /Zi /DDEBUG"
            else
                mode_flags="/O2 /DNDEBUG"
            fi
            
            local cmd="$compiler_cmd $common_flags $mode_flags \"$win_source\" /Fe:\"$win_output\""
            
        elif [[ "$compiler_name" == "intel" ]]; then
            local win_source=$(cygpath -w "$SOURCE_FILE" 2>/dev/null || echo "$SOURCE_FILE" | sed 's|/|\\|g')
            local win_output=$(cygpath -w "$output" 2>/dev/null || echo "$output" | sed 's|/|\\|g')
            local win_include=$(cygpath -w "./include" 2>/dev/null || echo ".\\include")
            
            common_flags="-Qstd:c++20 -W4 -EHsc -Xclang -fcxx-exceptions -I\"$win_include\""
            
            if [[ "$mode" == "debug" ]]; then
                mode_flags="-Od -Zi -DDEBUG"
            else
                mode_flags="-O2 -DNDEBUG"
            fi
            
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
            
            if [[ "$mode" == "debug" ]]; then
                mode_flags="-O0 -g3 -DDEBUG"
            else
                mode_flags="-O2 -DNDEBUG"
            fi
            
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
    compile_with "gcc" "${GCC_CXX:-g++}" "skip_check"
fi

if [[ "$COMPILER" == "clang" || "$COMPILER" == "all" ]]; then
    compile_with "clang" "${CLANG_CXX:-clang++}"
fi

if [[ "$COMPILER" == "intel" || "$COMPILER" == "all" ]]; then
    compile_with "intel" "${INTEL_CXX:-icx}"
fi

if [[ "$COMPILER" == "msvc" || "$COMPILER" == "all" ]]; then
    compile_with "msvc" "${MSVC_CXX:-cl.exe}"
fi

echo_success "Build complete for demo ${CATEGORY}/${DEMO_NAME}!"
