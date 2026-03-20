#!/usr/bin/env bash
# ==============================================================================
# build_demo.bash - Script para compilar demos individuales
# ==============================================================================
# Compila una demo específica con el compilador y modo seleccionados
# 
# Uso:
#   build_demo.bash <category> <demo_name> [compiler] [mode]
#
# Argumentos:
#   category  : general | tutorials | examples | showcase | comparison | performance | integration
#   demo_name : nombre del archivo sin extensión .cpp
#   compiler  : gcc | clang | intel | msvc (default: gcc)
#   mode      : debug | release (default: release)
#
# Ejemplos:
#   build_demo.bash tutorials 01_basic_operations
#   build_demo.bash examples ipv6_address gcc release
#   build_demo.bash showcase main clang debug
#   build_demo.bash general bit_analysis gcc release
#   build_demo.bash comparison boost_vs_int128 intel release
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

if [[ $# -lt 2 ]]; then
    echo "Error: Se requieren al menos 2 argumentos"
    echo "Uso: $0 <category> <demo_name> [compiler] [mode]"
    echo ""
    echo "Categorías disponibles: general, tutorials, examples, showcase, comparison, performance, integration"
    echo ""
    echo "Ejemplos:"
    echo "  $0 tutorials 01_basic_operations"
    echo "  $0 examples ipv6_address gcc release"
    echo "  $0 showcase main clang debug"
    exit 1
fi

CATEGORY="$1"
DEMO_NAME="$2"
COMPILER="${3:-gcc}"
MODE="${4:-release}"

# ========================= Validation =========================

VALID_CATEGORIES=("general" "tutorials" "examples" "showcase" "comparison" "performance" "integration")
if [[ ! " ${VALID_CATEGORIES[*]} " =~ " ${CATEGORY} " ]]; then
    echo "Error: CATEGORY debe ser uno de: ${VALID_CATEGORIES[*]}"
    exit 1
fi

VALID_COMPILERS=("gcc" "clang" "intel" "msvc")
if [[ ! " ${VALID_COMPILERS[*]} " =~ " ${COMPILER} " ]]; then
    echo "Error: COMPILER debe ser uno de: ${VALID_COMPILERS[*]}"
    exit 1
fi

VALID_MODES=("debug" "release")
if [[ ! " ${VALID_MODES[*]} " =~ " ${MODE} " ]]; then
    echo "Error: MODE debe ser uno de: ${VALID_MODES[*]}"
    exit 1
fi

# ========================= Configuration =========================

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

SOURCE_FILE="demos/${CATEGORY}/${DEMO_NAME}.cpp"
BUILD_DIR="build/build_demos/${COMPILER}/${MODE}"
INCLUDE_DIR="$PROJECT_ROOT/include"

# Determinar extensión ejecutable según compilador
if [[ "$COMPILER" == "msvc" || "$COMPILER" == "intel" ]]; then
    OUTPUT_FILE="$BUILD_DIR/${DEMO_NAME}.exe"
else
    OUTPUT_FILE="$BUILD_DIR/${DEMO_NAME}"
fi

# ========================= Color Output =========================

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo_success() { echo -e "${GREEN}[✓] $1${NC}"; }
echo_error() { echo -e "${RED}[✗] $1${NC}"; }
echo_info() { echo -e "${YELLOW}→ $1${NC}"; }
echo_header() { echo -e "${BLUE}═══ $1 ═══${NC}"; }

# ========================= Check Source File =========================

if [[ ! -f "$SOURCE_FILE" ]]; then
    echo_error "No existe: $SOURCE_FILE"
    echo_info "Demos disponibles en $CATEGORY/:"
    if [[ -d "demos/$CATEGORY" ]]; then
        ls -1 "demos/$CATEGORY"/*.cpp 2>/dev/null | sed 's|.*/||' | sed 's|\.cpp$||' || echo "  (ninguno)"
    fi
    exit 1
fi

# ========================= Create Build Directory =========================

mkdir -p "$BUILD_DIR"

# ========================= Build Command Setup =========================

echo_header "Compilando Demo"
echo_info "Categoría: $CATEGORY"
echo_info "Demo: $DEMO_NAME"
echo_info "Compilador: $COMPILER"
echo_info "Modo: $MODE"
echo ""

# Flags comunes
COMMON_FLAGS="-std=c++20 -I${INCLUDE_DIR}"
DEBUG_FLAGS="-g -O0 -DDEBUG"
RELEASE_FLAGS="-O3 -DNDEBUG"

# Flags específicos por modo
if [[ "$MODE" == "debug" ]]; then
    MODE_FLAGS="$DEBUG_FLAGS"
else
    MODE_FLAGS="$RELEASE_FLAGS"
fi

# ========================= Compiler-Specific Compilation =========================

compile_gcc() {
    echo_info "Compilando con GCC..."
    g++ $COMMON_FLAGS $MODE_FLAGS \
        -Wall -Wextra -Wpedantic \
        -pthread \
        "$SOURCE_FILE" \
        -o "$OUTPUT_FILE"
}

compile_clang() {
    echo_info "Compilando con Clang..."
    clang++ $COMMON_FLAGS $MODE_FLAGS \
        -Wall -Wextra -Wpedantic \
        -pthread \
        "$SOURCE_FILE" \
        -o "$OUTPUT_FILE"
}

compile_intel() {
    echo_info "Compilando con Intel ICX..."
    
    # Cargar entorno Intel si existe
    if [[ -f "/opt/intel/oneapi/setvars.sh" ]]; then
        source /opt/intel/oneapi/setvars.sh --force > /dev/null 2>&1
    fi
    
    icpx $COMMON_FLAGS $MODE_FLAGS \
        -Wall -Wextra \
        -pthread \
        "$SOURCE_FILE" \
        -o "$OUTPUT_FILE"
}

compile_msvc() {
    echo_info "Compilando con MSVC..."
    
    # MSVC requiere entorno específico
    if [[ "$MODE" == "debug" ]]; then
        MSVC_FLAGS="/MDd /Zi /Od /DDEBUG"
    else
        MSVC_FLAGS="/MD /O2 /DNDEBUG"
    fi
    
    cl.exe /std:c++20 /EHsc \
        /I"${INCLUDE_DIR}" \
        $MSVC_FLAGS \
        /W4 \
        "$SOURCE_FILE" \
        /Fe:"${OUTPUT_FILE}" \
        > /dev/null 2>&1 || {
            echo_error "MSVC no disponible o error en compilación"
            exit 1
        }
}

# ========================= Execute Compilation =========================

case "$COMPILER" in
    gcc)
        compile_gcc
        ;;
    clang)
        compile_clang
        ;;
    intel)
        compile_intel
        ;;
    msvc)
        compile_msvc
        ;;
esac

# ========================= Verify Success =========================

if [[ -f "$OUTPUT_FILE" ]]; then
    echo ""
    echo_success "Compilación exitosa!"
    echo_info "Ejecutable: $OUTPUT_FILE"
    echo ""
    echo_info "Para ejecutar:"
    echo "  bash scripts/run_demo.bash $CATEGORY $DEMO_NAME"
    echo "  o directamente:"
    echo "  $OUTPUT_FILE"
    exit 0
else
    echo ""
    echo_error "Compilación fallida"
    exit 1
fi
