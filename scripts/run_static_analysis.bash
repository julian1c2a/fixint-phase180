#!/bin/bash
# =============================================================================
# run_static_analysis.bash - Análisis estático del código
# =============================================================================
# Uso: bash scripts/run_static_analysis.bash [tool] [target]
#
# Herramientas disponibles:
#   cppcheck    - Análisis estático multiplataforma (recomendado)
#   clang-tidy  - Análisis basado en Clang
#   infer       - Facebook Infer (si está instalado)
#   all         - Ejecuta todas las herramientas disponibles
#
# Targets:
#   headers     - Analiza include/ e include_new/
#   tests       - Analiza tests/
#   benchs      - Analiza benchs/
#   demos       - Analiza demos/
#   all         - Analiza todo
#
# Ejemplos:
#   bash scripts/run_static_analysis.bash cppcheck headers
#   bash scripts/run_static_analysis.bash clang-tidy tests
#   bash scripts/run_static_analysis.bash all all
# =============================================================================

set -e

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Parámetros
TOOL="${1:-cppcheck}"
TARGET="${2:-headers}"

# Directorio del proyecto
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

# Crear directorio de reportes
REPORT_DIR="build/static_analysis"
mkdir -p "$REPORT_DIR"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Función para verificar herramienta
check_tool() {
    if command -v "$1" &> /dev/null; then
        return 0
    else
        return 1
    fi
}

# Obtener archivos según target
get_files() {
    local target="$1"
    case "$target" in
        headers)
            find include include_new -name "*.hpp" 2>/dev/null | head -100
            ;;
        tests)
            find tests -name "*.cpp" 2>/dev/null | head -50
            ;;
        benchs)
            find benchs -name "*.cpp" 2>/dev/null | head -50
            ;;
        demos)
            find demos -name "*.cpp" 2>/dev/null | head -50
            ;;
        all)
            find include include_new -name "*.hpp" 2>/dev/null | head -100
            find tests benchs demos -name "*.cpp" 2>/dev/null | head -100
            ;;
    esac
}

# ============================================================================
# CPPCHECK
# ============================================================================
run_cppcheck() {
    local target="$1"
    
    if ! check_tool "cppcheck"; then
        echo -e "${YELLOW}[WARN] cppcheck no instalado. Instalar con: pacman -S cppcheck${NC}"
        return 1
    fi
    
    echo -e "${CYAN}[CPPCHECK] Analizando $target...${NC}"
    
    local report_file="$REPORT_DIR/cppcheck_${target}_${TIMESTAMP}.txt"
    local xml_file="$REPORT_DIR/cppcheck_${target}_${TIMESTAMP}.xml"
    
    local dirs=""
    case "$target" in
        headers) dirs="include include_new" ;;
        tests)   dirs="tests" ;;
        benchs)  dirs="benchs" ;;
        demos)   dirs="demos" ;;
        all)     dirs="include include_new tests benchs demos" ;;
    esac
    
    # Ejecutar cppcheck
    cppcheck \
        --enable=all \
        --std=c++20 \
        --error-exitcode=0 \
        --suppress=missingIncludeSystem \
        --suppress=unusedFunction \
        --suppress=unmatchedSuppression \
        -I include \
        -I include_new \
        --output-file="$report_file" \
        --xml --xml-version=2 \
        $dirs 2> "$xml_file"
    
    # Contar errores
    local errors=$(grep -c "<error " "$xml_file" 2>/dev/null || echo "0")
    
    echo -e "${GREEN}[OK] Reporte: $report_file${NC}"
    echo -e "     Errores encontrados: $errors"
    
    # Mostrar resumen
    if [[ -s "$report_file" ]]; then
        echo -e "${YELLOW}Resumen de problemas:${NC}"
        head -20 "$report_file"
        if [[ $(wc -l < "$report_file") -gt 20 ]]; then
            echo -e "${YELLOW}... (ver archivo completo)${NC}"
        fi
    fi
    
    return 0
}

# ============================================================================
# CLANG-TIDY
# ============================================================================
run_clang_tidy() {
    local target="$1"
    
    if ! check_tool "clang-tidy"; then
        echo -e "${YELLOW}[WARN] clang-tidy no instalado. Viene con Clang.${NC}"
        return 1
    fi
    
    echo -e "${CYAN}[CLANG-TIDY] Analizando $target...${NC}"
    
    local report_file="$REPORT_DIR/clang_tidy_${target}_${TIMESTAMP}.txt"
    
    # Configuración de checks
    local checks="clang-analyzer-*,bugprone-*,performance-*,modernize-*,readability-*"
    checks="$checks,-modernize-use-trailing-return-type"
    checks="$checks,-readability-magic-numbers"
    checks="$checks,-bugprone-easily-swappable-parameters"
    
    local files=$(get_files "$target")
    
    if [[ -z "$files" ]]; then
        echo -e "${YELLOW}[WARN] No se encontraron archivos para $target${NC}"
        return 1
    fi
    
    # Ejecutar clang-tidy en cada archivo
    echo "" > "$report_file"
    local count=0
    local errors=0
    
    for file in $files; do
        if [[ -f "$file" ]]; then
            echo -e "  Analizando: $file"
            clang-tidy "$file" \
                -checks="$checks" \
                -- -std=c++20 -I include -I include_new 2>&1 >> "$report_file" || ((errors++))
            ((count++))
        fi
    done
    
    echo -e "${GREEN}[OK] Archivos analizados: $count${NC}"
    echo -e "     Reporte: $report_file"
    
    # Mostrar resumen
    local warnings=$(grep -c "warning:" "$report_file" 2>/dev/null || echo "0")
    echo -e "     Warnings encontrados: $warnings"
    
    return 0
}

# ============================================================================
# INFER (Facebook)
# ============================================================================
run_infer() {
    local target="$1"
    
    if ! check_tool "infer"; then
        echo -e "${YELLOW}[WARN] Infer no instalado.${NC}"
        echo -e "       Instalación: https://fbinfer.com/docs/getting-started${NC}"
        return 1
    fi
    
    echo -e "${CYAN}[INFER] Analizando $target...${NC}"
    
    local report_dir="$REPORT_DIR/infer_${target}_${TIMESTAMP}"
    mkdir -p "$report_dir"
    
    local files=$(get_files "$target" | grep "\.cpp$" | head -10)
    
    if [[ -z "$files" ]]; then
        echo -e "${YELLOW}[WARN] No se encontraron archivos .cpp para $target${NC}"
        return 1
    fi
    
    # Infer necesita compilar, usamos un archivo de ejemplo
    for file in $files; do
        if [[ -f "$file" ]]; then
            echo -e "  Analizando: $file"
            infer run --results-dir "$report_dir" -- \
                g++ -std=c++20 -I include -I include_new -c "$file" -o /dev/null 2>/dev/null || true
        fi
    done
    
    # Mostrar reporte
    if [[ -f "$report_dir/report.txt" ]]; then
        echo -e "${GREEN}[OK] Reporte: $report_dir/report.txt${NC}"
        infer report --results-dir "$report_dir"
    else
        echo -e "${YELLOW}[INFO] Sin problemas detectados o análisis incompleto${NC}"
    fi
    
    return 0
}

# ============================================================================
# MAIN
# ============================================================================
echo -e "${BLUE}============================================================${NC}"
echo -e "${BLUE}  ANÁLISIS ESTÁTICO DE CÓDIGO${NC}"
echo -e "${BLUE}============================================================${NC}"
echo -e "  Herramienta: ${GREEN}$TOOL${NC}"
echo -e "  Target:      ${GREEN}$TARGET${NC}"
echo -e "  Reportes:    ${GREEN}$REPORT_DIR/${NC}"
echo -e "${BLUE}------------------------------------------------------------${NC}"

case "$TOOL" in
    cppcheck)
        run_cppcheck "$TARGET"
        ;;
    clang-tidy)
        run_clang_tidy "$TARGET"
        ;;
    infer)
        run_infer "$TARGET"
        ;;
    all)
        echo -e "${CYAN}Ejecutando todas las herramientas disponibles...${NC}"
        echo ""
        run_cppcheck "$TARGET" || true
        echo ""
        run_clang_tidy "$TARGET" || true
        echo ""
        run_infer "$TARGET" || true
        ;;
    *)
        echo -e "${RED}[ERROR] Herramienta no reconocida: $TOOL${NC}"
        echo -e "Opciones: cppcheck, clang-tidy, infer, all"
        exit 1
        ;;
esac

echo ""
echo -e "${BLUE}============================================================${NC}"
echo -e "${GREEN}[DONE] Análisis completado${NC}"
echo -e "${BLUE}============================================================${NC}"
