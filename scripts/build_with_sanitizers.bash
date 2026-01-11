#!/bin/bash
# =============================================================================
# build_with_sanitizers.bash - Compilación con sanitizadores
# =============================================================================
# Uso: bash scripts/build_with_sanitizers.bash <type> <feature> <target> <compiler> <sanitizer>
#
# Sanitizers disponibles:
#   asan   - AddressSanitizer (buffer overflow, use-after-free)
#   ubsan  - UndefinedBehaviorSanitizer
#   tsan   - ThreadSanitizer (data races) - incompatible con asan
#   msan   - MemorySanitizer (solo Clang Linux)
#   all    - asan + ubsan combinados
#
# Ejemplos:
#   bash scripts/build_with_sanitizers.bash uint128 bits tests gcc asan
#   bash scripts/build_with_sanitizers.bash uint128 algorithm tests clang ubsan
#   bash scripts/build_with_sanitizers.bash uint128 thread_safety tests gcc tsan
# =============================================================================

set -e

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Parámetros
TYPE="${1:-uint128}"
FEATURE="${2:-t}"
TARGET="${3:-tests}"
COMPILER="${4:-gcc}"
SANITIZER="${5:-asan}"

# Validación
if [[ ! "$TYPE" =~ ^(uint128|int128)$ ]]; then
    echo -e "${RED}[ERROR] TYPE debe ser uint128 o int128${NC}"
    exit 1
fi

if [[ ! "$COMPILER" =~ ^(gcc|clang|intel|msvc)$ ]]; then
    echo -e "${RED}[ERROR] COMPILER debe ser gcc, clang, intel o msvc${NC}"
    exit 1
fi

if [[ ! "$SANITIZER" =~ ^(asan|ubsan|tsan|msan|all)$ ]]; then
    echo -e "${RED}[ERROR] SANITIZER debe ser asan, ubsan, tsan, msan o all${NC}"
    exit 1
fi

# Directorio del proyecto
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

# Archivo fuente
if [[ "$TARGET" == "tests" ]]; then
    SOURCE_FILE="tests/${TYPE}_${FEATURE}_extracted_tests.cpp"
elif [[ "$TARGET" == "benchs" ]]; then
    SOURCE_FILE="benchs/${TYPE}_${FEATURE}_extracted_benchs.cpp"
else
    echo -e "${RED}[ERROR] TARGET debe ser tests o benchs${NC}"
    exit 1
fi

if [[ ! -f "$SOURCE_FILE" ]]; then
    echo -e "${RED}[ERROR] No existe: $SOURCE_FILE${NC}"
    exit 1
fi

# Directorio de salida
BUILD_DIR="build/build_${TARGET}_sanitized/${COMPILER}/${SANITIZER}"
mkdir -p "$BUILD_DIR"

# Configurar flags de sanitizador según compilador
get_sanitizer_flags() {
    local compiler="$1"
    local sanitizer="$2"
    
    case "$compiler" in
        gcc|clang)
            case "$sanitizer" in
                asan)
                    echo "-fsanitize=address -fno-omit-frame-pointer -g"
                    ;;
                ubsan)
                    echo "-fsanitize=undefined -fno-omit-frame-pointer -g"
                    ;;
                tsan)
                    echo "-fsanitize=thread -fno-omit-frame-pointer -g"
                    ;;
                msan)
                    if [[ "$compiler" == "clang" ]]; then
                        echo "-fsanitize=memory -fno-omit-frame-pointer -g"
                    else
                        echo -e "${YELLOW}[WARN] MSan solo disponible en Clang${NC}" >&2
                        echo "-g"
                    fi
                    ;;
                all)
                    echo "-fsanitize=address,undefined -fno-omit-frame-pointer -g"
                    ;;
            esac
            ;;
        intel)
            case "$sanitizer" in
                asan)
                    echo "-fsanitize=address -fno-omit-frame-pointer -g"
                    ;;
                ubsan)
                    echo "-fsanitize=undefined -fno-omit-frame-pointer -g"
                    ;;
                *)
                    echo -e "${YELLOW}[WARN] Intel solo soporta asan y ubsan${NC}" >&2
                    echo "-g"
                    ;;
            esac
            ;;
        msvc)
            case "$sanitizer" in
                asan)
                    echo "/fsanitize=address /Zi /RTC1"
                    ;;
                *)
                    echo -e "${YELLOW}[WARN] MSVC solo soporta ASan${NC}" >&2
                    echo "/Zi /RTC1"
                    ;;
            esac
            ;;
    esac
}

# Obtener comando del compilador
get_compiler_cmd() {
    case "$1" in
        gcc)   echo "g++" ;;
        clang) echo "clang++" ;;
        intel) echo "icpx" ;;
        msvc)  echo "cl" ;;
    esac
}

# Flags base
get_base_flags() {
    local compiler="$1"
    case "$compiler" in
        gcc|clang|intel)
            echo "-std=c++20 -Wall -Wextra -I include -I include_new"
            ;;
        msvc)
            echo "/std:c++20 /W4 /EHsc /I include /I include_new"
            ;;
    esac
}

# Compilar
COMPILER_CMD=$(get_compiler_cmd "$COMPILER")
BASE_FLAGS=$(get_base_flags "$COMPILER")
SAN_FLAGS=$(get_sanitizer_flags "$COMPILER" "$SANITIZER")
OUTPUT_FILE="${BUILD_DIR}/${TYPE}_${FEATURE}_${TARGET}_${COMPILER}"

echo -e "${BLUE}============================================================${NC}"
echo -e "${BLUE}  COMPILACIÓN CON SANITIZADOR${NC}"
echo -e "${BLUE}============================================================${NC}"
echo -e "  Tipo:       ${GREEN}$TYPE${NC}"
echo -e "  Feature:    ${GREEN}$FEATURE${NC}"
echo -e "  Target:     ${GREEN}$TARGET${NC}"
echo -e "  Compilador: ${GREEN}$COMPILER${NC}"
echo -e "  Sanitizer:  ${GREEN}$SANITIZER${NC}"
echo -e "${BLUE}------------------------------------------------------------${NC}"

# Detectar si necesita threading
THREAD_FLAGS=""
if grep -qE '#include\s*<thread>|#include\s*<atomic>|thread_safety' "$SOURCE_FILE" 2>/dev/null; then
    if [[ "$COMPILER" != "msvc" ]]; then
        THREAD_FLAGS="-pthread"
        if [[ "$COMPILER" == "gcc" || "$COMPILER" == "clang" ]]; then
            THREAD_FLAGS="$THREAD_FLAGS -latomic"
        fi
    fi
fi

# Comando de compilación
if [[ "$COMPILER" == "msvc" ]]; then
    OUTPUT_FILE="${OUTPUT_FILE}.exe"
    CMD="$COMPILER_CMD $BASE_FLAGS $SAN_FLAGS $SOURCE_FILE /Fe:$OUTPUT_FILE"
else
    CMD="$COMPILER_CMD $BASE_FLAGS $SAN_FLAGS $THREAD_FLAGS $SOURCE_FILE -o $OUTPUT_FILE"
fi

echo -e "  Comando: ${YELLOW}$CMD${NC}"
echo -e "${BLUE}------------------------------------------------------------${NC}"

# Ejecutar compilación
if eval "$CMD"; then
    echo -e "${GREEN}[OK] Compilación exitosa: $OUTPUT_FILE${NC}"
    echo ""
    echo -e "${YELLOW}Para ejecutar con sanitizer:${NC}"
    echo -e "  $OUTPUT_FILE"
    echo ""
    echo -e "${YELLOW}Variables de entorno útiles:${NC}"
    case "$SANITIZER" in
        asan|all)
            echo "  ASAN_OPTIONS=detect_leaks=1:abort_on_error=1"
            ;;
        ubsan)
            echo "  UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1"
            ;;
        tsan)
            echo "  TSAN_OPTIONS=second_deadlock_stack=1"
            ;;
    esac
else
    echo -e "${RED}[FAIL] Error de compilación${NC}"
    exit 1
fi
