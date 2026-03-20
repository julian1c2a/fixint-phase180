#!/bin/bash
# ==============================================================================
# compile_wsl.bash - Compilación cruzada en WSL (Ubuntu 25.04)
# ==============================================================================
# 
# Compila los tests del proyecto con múltiples compiladores en WSL:
#   - g++-13, g++-14, g++-15
#   - clang++-18, clang++-19, clang++-20, clang++-21
#   - icpx (Intel oneAPI)
#
# USO:
#   ./scripts/compile_wsl.bash [archivo.cpp] [modo]
#   
#   modo: debug | release (default: release)
#
# EJEMPLOS:
#   ./scripts/compile_wsl.bash test_divrem_euclidean_properties.cpp
#   ./scripts/compile_wsl.bash test_divrem_euclidean_properties.cpp debug
#
# REQUISITOS (en Ubuntu 25.04):
#   sudo apt install g++-13 g++-14 g++-15 clang-18 clang-19 clang-20 clang-21
#   # Intel oneAPI: https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html
# ==============================================================================

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Argumentos
SOURCE_FILE="${1:-test_divrem_euclidean_properties.cpp}"
MODE="${2:-release}"

# Flags según modo
if [ "$MODE" == "debug" ]; then
    OPT_FLAGS="-O0 -g -DDEBUG"
else
    OPT_FLAGS="-O2 -DNDEBUG"
fi

# Flags comunes
COMMON_FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -I."

# Directorio de salida
OUTPUT_DIR="build/wsl/${MODE}"
mkdir -p "$OUTPUT_DIR"

# Lista de compiladores a probar
declare -a GCC_VERSIONS=("g++-13" "g++-14" "g++-15")
declare -a CLANG_VERSIONS=("clang++-18" "clang++-19" "clang++-20" "clang++-21")
declare -a INTEL_VERSIONS=("icpx")

# Nombre base del archivo (sin extensión)
BASENAME=$(basename "$SOURCE_FILE" .cpp)

echo -e "${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║   COMPILACIÓN WSL - ${SOURCE_FILE}${NC}"
echo -e "${CYAN}║   Modo: ${MODE}${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Contadores
PASSED=0
FAILED=0
SKIPPED=0

# Función para compilar con un compilador
compile_with() {
    local COMPILER="$1"
    local OUTPUT="${OUTPUT_DIR}/${BASENAME}_${COMPILER/+/pp}"
    
    # Verificar si el compilador existe
    if ! command -v "$COMPILER" &> /dev/null; then
        echo -e "${YELLOW}[SKIP]${NC} $COMPILER no encontrado"
        ((SKIPPED++))
        return
    fi
    
    # Compilar
    echo -n "Compilando con $COMPILER... "
    
    if $COMPILER $COMMON_FLAGS $OPT_FLAGS "$SOURCE_FILE" -o "$OUTPUT" 2>/tmp/compile_error_$$.txt; then
        echo -e "${GREEN}[OK]${NC}"
        ((PASSED++))
        
        # Ejecutar el test si compiló correctamente
        echo -n "  Ejecutando... "
        if "$OUTPUT" > /tmp/run_output_$$.txt 2>&1; then
            # Extraer resumen
            SUMMARY=$(grep -E "Tests pasados|RESUMEN" /tmp/run_output_$$.txt | tail -1)
            echo -e "${GREEN}[OK]${NC} $SUMMARY"
        else
            echo -e "${RED}[FAIL]${NC} (exit code: $?)"
            cat /tmp/run_output_$$.txt | tail -5
        fi
    else
        echo -e "${RED}[FAIL]${NC}"
        cat /tmp/compile_error_$$.txt | head -10
        ((FAILED++))
    fi
    
    rm -f /tmp/compile_error_$$.txt /tmp/run_output_$$.txt
}

# Compilar con GCC
echo -e "\n${CYAN}=== GCC ===${NC}"
for compiler in "${GCC_VERSIONS[@]}"; do
    compile_with "$compiler"
done

# Compilar con Clang
echo -e "\n${CYAN}=== Clang ===${NC}"
for compiler in "${CLANG_VERSIONS[@]}"; do
    compile_with "$compiler"
done

# Compilar con Intel
echo -e "\n${CYAN}=== Intel ===${NC}"
# Intentar cargar entorno Intel si existe
if [ -f "/opt/intel/oneapi/setvars.sh" ]; then
    source /opt/intel/oneapi/setvars.sh > /dev/null 2>&1
fi
for compiler in "${INTEL_VERSIONS[@]}"; do
    compile_with "$compiler"
done

# Resumen final
echo -e "\n${CYAN}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║                        RESUMEN                             ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════════════════════╝${NC}"
echo -e "${GREEN}Compilaciones exitosas: $PASSED${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "${RED}Compilaciones fallidas: $FAILED${NC}"
fi
if [ $SKIPPED -gt 0 ]; then
    echo -e "${YELLOW}Compiladores no disponibles: $SKIPPED${NC}"
fi

# Exit code
if [ $FAILED -gt 0 ]; then
    exit 1
fi
exit 0
