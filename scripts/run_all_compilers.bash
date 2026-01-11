#!/usr/bin/env bash
# Script maestro para ejecutar CI/CD localmente
# Ejecuta build y tests con todos los compiladores disponibles
# Usage: ./run_all_compilers.bash [--skip-msvc] [--only COMPILER]

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Opciones
SKIP_MSVC=false
ONLY_COMPILER=""

# Parsear argumentos
while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-msvc)
            SKIP_MSVC=true
            shift
            ;;
        --only)
            ONLY_COMPILER="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--skip-msvc] [--only COMPILER]"
            exit 1
            ;;
    esac
done

# Compiladores disponibles
if [ -n "$ONLY_COMPILER" ]; then
    COMPILERS=("$ONLY_COMPILER")
else
    COMPILERS=()
    
    # Detectar GCC
    if command -v g++ &> /dev/null; then
        COMPILERS+=("gcc")
    fi
    
    # Detectar Clang
    if command -v clang++ &> /dev/null; then
        COMPILERS+=("clang")
    fi
    
    # Detectar Intel
    if command -v icpx &> /dev/null; then
        COMPILERS+=("intel")
    fi
    
    # Detectar MSVC (solo en Windows/WSL con vcvarsall.bat configurado)
    if [ "$SKIP_MSVC" = false ] && command -v cl.exe &> /dev/null; then
        COMPILERS+=("msvc")
    fi
fi

BUILD_TYPES=("debug" "release")

# Verificar que hay al menos un compilador
if [ ${#COMPILERS[@]} -eq 0 ]; then
    echo -e "${RED}❌ No compilers detected${NC}"
    echo "Make sure g++, clang++, icpx, or cl.exe are in your PATH"
    exit 1
fi

echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║         CI/CD Local - int128 Library Test Suite           ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${YELLOW}Detected compilers:${NC} ${COMPILERS[*]}"
echo -e "${YELLOW}Build types:${NC} ${BUILD_TYPES[*]}"
echo ""

# Estadísticas globales
TOTAL_TESTS=0
TOTAL_PASSED=0
TOTAL_FAILED=0
TOTAL_BUILD_FAILED=0

declare -A RESULTS

# Función para ejecutar tests de un compilador
run_compiler_tests() {
    local compiler=$1
    local build_type=$2
    
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}Testing: ${YELLOW}$compiler${NC} - ${YELLOW}$build_type${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
    
    # Build
    echo -e "${YELLOW}▶ Building tests...${NC}"
    if ./scripts/build_extracted_tests.bash "$compiler" "$build_type"; then
        echo -e "${GREEN}✓ Build successful${NC}"
        
        # Run tests
        echo -e "${YELLOW}▶ Running tests...${NC}"
        if ./scripts/run_uint128_extracted_tests.bash "$compiler" "$build_type"; then
            echo -e "${GREEN}✓ All tests passed${NC}"
            RESULTS["${compiler}_${build_type}"]="PASS"
            ((TOTAL_PASSED++))
        else
            echo -e "${RED}✗ Some tests failed${NC}"
            RESULTS["${compiler}_${build_type}"]="FAIL"
            ((TOTAL_FAILED++))
        fi
    else
        echo -e "${RED}✗ Build failed${NC}"
        RESULTS["${compiler}_${build_type}"]="BUILD_FAIL"
        ((TOTAL_BUILD_FAILED++))
    fi
    
    ((TOTAL_TESTS++))
}

# Ejecutar todos los compiladores
START_TIME=$(date +%s)

for compiler in "${COMPILERS[@]}"; do
    for build_type in "${BUILD_TYPES[@]}"; do
        run_compiler_tests "$compiler" "$build_type"
    done
done

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))

# Resumen final
echo ""
echo -e "${BLUE}╔════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                      FINAL SUMMARY                         ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Tabla de resultados
echo -e "${YELLOW}Results by compiler:${NC}"
echo "┌────────────────────────┬─────────┬─────────┐"
echo "│ Compiler               │ Debug   │ Release │"
echo "├────────────────────────┼─────────┼─────────┤"

for compiler in "${COMPILERS[@]}"; do
    debug_result="${RESULTS[${compiler}_debug]}"
    release_result="${RESULTS[${compiler}_release]}"
    
    # Colorear resultados
    if [ "$debug_result" = "PASS" ]; then
        debug_display="${GREEN}✓ PASS${NC}"
    elif [ "$debug_result" = "FAIL" ]; then
        debug_display="${RED}✗ FAIL${NC}"
    else
        debug_display="${RED}✗ BUILD${NC}"
    fi
    
    if [ "$release_result" = "PASS" ]; then
        release_display="${GREEN}✓ PASS${NC}"
    elif [ "$release_result" = "FAIL" ]; then
        release_display="${RED}✗ FAIL${NC}"
    else
        release_display="${RED}✗ BUILD${NC}"
    fi
    
    printf "│ %-22s │ %-15b │ %-15b │\n" "$compiler" "$debug_display" "$release_display"
done

echo "└────────────────────────┴─────────┴─────────┘"
echo ""

# Estadísticas
echo -e "${YELLOW}Statistics:${NC}"
echo "  Total configurations: $TOTAL_TESTS"
echo -e "  ${GREEN}Passed: $TOTAL_PASSED${NC}"
echo -e "  ${RED}Failed: $TOTAL_FAILED${NC}"
echo -e "  ${RED}Build failed: $TOTAL_BUILD_FAILED${NC}"
echo "  Elapsed time: ${ELAPSED}s"
echo ""

# Exit code
if [ $TOTAL_FAILED -eq 0 ] && [ $TOTAL_BUILD_FAILED -eq 0 ]; then
    echo -e "${GREEN}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║  🎉 ALL TESTS PASSED! CI/CD LOCAL SUCCESSFUL 🎉           ║${NC}"
    echo -e "${GREEN}╚════════════════════════════════════════════════════════════╝${NC}"
    exit 0
else
    echo -e "${RED}╔════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║  ❌ SOME TESTS FAILED - CHECK LOGS ABOVE                   ║${NC}"
    echo -e "${RED}╚════════════════════════════════════════════════════════════╝${NC}"
    exit 1
fi
