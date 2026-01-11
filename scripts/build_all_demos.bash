#!/usr/bin/env bash
# ==============================================================================
# build_all_demos.bash - Script para compilar múltiples demos
# ==============================================================================
# Compila todas las demos de una categoría o todas las categorías
# 
# Uso:
#   build_all_demos.bash <category> [compiler] [mode] [continue_on_error]
#
# Argumentos:
#   category          : general | tutorials | examples | showcase | comparison | 
#                       performance | integration | all
#   compiler          : gcc | clang | intel | msvc | all (default: gcc)
#   mode              : debug | release | all (default: release)
#   continue_on_error : yes | no (default: no) - Continuar si una demo falla
#
# Ejemplos:
#   build_all_demos.bash tutorials
#   build_all_demos.bash all gcc release
#   build_all_demos.bash examples all all yes
#   build_all_demos.bash showcase clang debug
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

if [[ $# -lt 1 ]]; then
    echo "Error: Se requiere al menos 1 argumento"
    echo "Uso: $0 <category> [compiler] [mode] [continue_on_error]"
    echo ""
    echo "Categorías: general, tutorials, examples, showcase, comparison, performance, integration, all"
    echo ""
    echo "Ejemplos:"
    echo "  $0 tutorials"
    echo "  $0 all gcc release"
    echo "  $0 examples all all yes"
    exit 1
fi

CATEGORY="$1"
COMPILER="${2:-gcc}"
MODE="${3:-release}"
CONTINUE_ON_ERROR="${4:-no}"

# ========================= Validation =========================

VALID_CATEGORIES=("general" "tutorials" "examples" "showcase" "comparison" "performance" "integration" "all")
if [[ ! " ${VALID_CATEGORIES[*]} " =~ " ${CATEGORY} " ]]; then
    echo "Error: CATEGORY debe ser uno de: ${VALID_CATEGORIES[*]}"
    exit 1
fi

VALID_COMPILERS=("gcc" "clang" "intel" "msvc" "all")
if [[ ! " ${VALID_COMPILERS[*]} " =~ " ${COMPILER} " ]]; then
    echo "Error: COMPILER debe ser uno de: ${VALID_COMPILERS[*]}"
    exit 1
fi

VALID_MODES=("debug" "release" "all")
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
MAGENTA='\033[0;35m'
NC='\033[0m'

echo_success() { echo -e "${GREEN}[✓] $1${NC}"; }
echo_error() { echo -e "${RED}[✗] $1${NC}"; }
echo_info() { echo -e "${YELLOW}→ $1${NC}"; }
echo_header() { echo -e "${BLUE}═══════════════════════════════════════════════════════${NC}"; }
echo_title() { echo -e "${MAGENTA}$1${NC}"; }

# ========================= Counters =========================

TOTAL_DEMOS=0
SUCCESS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

declare -a FAILED_DEMOS

# ========================= Determine Categories =========================

if [[ "$CATEGORY" == "all" ]]; then
    CATEGORIES=("general" "tutorials" "examples" "showcase" "comparison" "performance" "integration")
else
    CATEGORIES=("$CATEGORY")
fi

# ========================= Determine Compilers =========================

if [[ "$COMPILER" == "all" ]]; then
    COMPILERS=("gcc" "clang" "intel" "msvc")
else
    COMPILERS=("$COMPILER")
fi

# ========================= Determine Modes =========================

if [[ "$MODE" == "all" ]]; then
    MODES=("debug" "release")
else
    MODES=("$MODE")
fi

# ========================= Build Function =========================

build_demo() {
    local cat="$1"
    local demo="$2"
    local comp="$3"
    local mod="$4"
    
    local demo_name=$(basename "$demo" .cpp)
    
    echo_info "Compilando: $cat/$demo_name [$comp, $mod]"
    
    if bash scripts/build_demo.bash "$cat" "$demo_name" "$comp" "$mod" > /dev/null 2>&1; then
        echo_success "OK: $cat/$demo_name [$comp, $mod]"
        ((SUCCESS_COUNT++))
        return 0
    else
        echo_error "FAIL: $cat/$demo_name [$comp, $mod]"
        FAILED_DEMOS+=("$cat/$demo_name [$comp, $mod]")
        ((FAIL_COUNT++))
        
        if [[ "$CONTINUE_ON_ERROR" != "yes" ]]; then
            return 1
        fi
        return 0
    fi
}

# ========================= Main Build Loop =========================

echo_header
echo_title "  🔨 Compilando Demos - int128 Library"
echo_header
echo ""

for cat in "${CATEGORIES[@]}"; do
    DEMO_DIR="demos/$cat"
    
    if [[ ! -d "$DEMO_DIR" ]]; then
        echo_info "Categoría no existe: $cat (omitida)"
        continue
    fi
    
    # Obtener lista de demos en la categoría usando glob
    shopt -s nullglob
    DEMO_FILES=("$DEMO_DIR"/*.cpp)
    shopt -u nullglob
    
    if [[ ${#DEMO_FILES[@]} -eq 0 ]]; then
        echo_info "Sin demos en: $cat (omitida)"
        continue
    fi
    
    echo_title "📂 Categoría: $cat (${#DEMO_FILES[@]} demos)"
    echo ""
    
    for demo_file in "${DEMO_FILES[@]}"; do
        for comp in "${COMPILERS[@]}"; do
            for mod in "${MODES[@]}"; do
                ((TOTAL_DEMOS++))
                
                if ! build_demo "$cat" "$demo_file" "$comp" "$mod"; then
                    if [[ "$CONTINUE_ON_ERROR" != "yes" ]]; then
                        echo ""
                        echo_error "Compilación abortada por error"
                        exit 1
                    fi
                fi
            done
        done
    done
    
    echo ""
done

# ========================= Summary Report =========================

echo_header
echo_title "  📊 Resumen de Compilación"
echo_header
echo ""
echo "Total de compilaciones: $TOTAL_DEMOS"
echo_success "Exitosas: $SUCCESS_COUNT"
echo_error "Fallidas: $FAIL_COUNT"

if [[ ${#FAILED_DEMOS[@]} -gt 0 ]]; then
    echo ""
    echo_title "Demos fallidas:"
    for failed in "${FAILED_DEMOS[@]}"; do
        echo "  - $failed"
    done
fi

echo ""
echo_header

# ========================= Exit Status =========================

if [[ $FAIL_COUNT -gt 0 ]]; then
    exit 1
else
    echo_success "✨ Todas las demos compilaron exitosamente!"
    exit 0
fi
