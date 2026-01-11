#!/usr/bin/env bash
# ==============================================================================
# run_demo.bash - Script para ejecutar demos compiladas
# ==============================================================================
# Ejecuta una demo previamente compilada, con soporte para argumentos
# 
# Uso:
#   run_demo.bash <category> <demo_name> [compiler] [mode] [args...]
#
# Argumentos:
#   category  : general | tutorials | examples | showcase | comparison | performance | integration
#   demo_name : nombre del archivo sin extensión .cpp
#   compiler  : gcc | clang | intel | msvc (default: gcc)
#   mode      : debug | release (default: release)
#   args...   : argumentos adicionales para la demo
#
# Ejemplos:
#   run_demo.bash tutorials 01_basic_operations
#   run_demo.bash examples uuid_generation gcc release 100
#   run_demo.bash showcase main clang debug
#   run_demo.bash performance division_algorithms intel release
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

if [[ $# -lt 2 ]]; then
    echo "Error: Se requieren al menos 2 argumentos"
    echo "Uso: $0 <category> <demo_name> [compiler] [mode] [args...]"
    echo ""
    echo "Categorías: general, tutorials, examples, showcase, comparison, performance, integration"
    echo ""
    echo "Ejemplos:"
    echo "  $0 tutorials 01_basic_operations"
    echo "  $0 examples uuid_generation gcc release 100"
    echo "  $0 showcase main"
    exit 1
fi

CATEGORY="$1"
DEMO_NAME="$2"
COMPILER="${3:-gcc}"
MODE="${4:-release}"

# Shift para obtener args adicionales
shift 4 2>/dev/null || shift $#
DEMO_ARGS=("$@")

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

BUILD_DIR="build/build_demos/${COMPILER}/${MODE}"

# Determinar ejecutable según compilador
if [[ "$COMPILER" == "msvc" || "$COMPILER" == "intel" ]]; then
    EXE_FILE="$BUILD_DIR/${DEMO_NAME}.exe"
else
    EXE_FILE="$BUILD_DIR/${DEMO_NAME}"
fi

# ========================= Color Output =========================

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo_success() { echo -e "${GREEN}[✓] $1${NC}"; }
echo_error() { echo -e "${RED}[✗] $1${NC}"; }
echo_info() { echo -e "${YELLOW}→ $1${NC}"; }
echo_header() { echo -e "${BLUE}═══ $1 ═══${NC}"; }
echo_output() { echo -e "${CYAN}$1${NC}"; }

# ========================= Check Executable =========================

if [[ ! -f "$EXE_FILE" ]]; then
    echo_error "Ejecutable no encontrado: $EXE_FILE"
    echo ""
    echo_info "¿Necesitas compilarlo primero?"
    echo "  bash scripts/build_demo.bash $CATEGORY $DEMO_NAME $COMPILER $MODE"
    exit 1
fi

# ========================= Execute Demo =========================

echo_header "Ejecutando Demo"
echo_info "Categoría: $CATEGORY"
echo_info "Demo: $DEMO_NAME"
echo_info "Compilador: $COMPILER [$MODE]"
if [[ ${#DEMO_ARGS[@]} -gt 0 ]]; then
    echo_info "Argumentos: ${DEMO_ARGS[*]}"
fi
echo ""
echo_output "────────────────────────────────────────────────────────────────"
echo ""

# Ejecutar con argumentos si se proporcionaron
if [[ ${#DEMO_ARGS[@]} -gt 0 ]]; then
    "$EXE_FILE" "${DEMO_ARGS[@]}"
    EXIT_CODE=$?
else
    "$EXE_FILE"
    EXIT_CODE=$?
fi

echo ""
echo_output "────────────────────────────────────────────────────────────────"

# ========================= Report Exit Status =========================

if [[ $EXIT_CODE -eq 0 ]]; then
    echo ""
    echo_success "Demo ejecutada exitosamente"
    exit 0
else
    echo ""
    echo_error "Demo terminó con código de error: $EXIT_CODE"
    exit $EXIT_CODE
fi
