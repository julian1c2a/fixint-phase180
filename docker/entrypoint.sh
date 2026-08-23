#!/bin/bash
# =============================================================================
# entrypoint.sh — Punto de entrada para contenedor int128
# =============================================================================

set -e

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

log_info() { echo -e "${CYAN}[INFO]${NC} $@"; }
log_success() { echo -e "${GREEN}[OK]${NC} $@"; }
log_error() { echo -e "${RED}[ERROR]${NC} $@"; }
log_header() { echo -e "${BLUE}$@${NC}"; }

cd /project

log_header "================================================================="
log_header "  int128/uint128 - Automated Build & Test System (Docker)"
log_header "================================================================="
echo ""

log_info "Detectando compiladores disponibles..."
gcc_version=$(gcc --version | head -1)
clang_version=$(clang --version | head -1)
python_version=$(python3 --version)
cmake_version=$(cmake --version | head -1)

echo "  $gcc_version"
echo "  $clang_version"
echo "  $python_version"
echo "  $cmake_version"
echo ""

COMMAND=${1:-test}

case "$COMMAND" in
    
    test)
        log_header "Pipeline de TEST COMPLETO"
        python3 make.py test
        ;;
    
    bench)
        log_header "Pipeline de BENCHMARK COMPLETO"
        python3 make.py bench
        ;;
    
    all)
        log_header "Pipeline COMPLETO"
        python3 make.py all
        python3 make.py test
        python3 make.py bench
        ;;
    
    quick)
        log_header "Pipeline RÁPIDO (GCC release)"
        python3 make.py build uint128 bits tests gcc release
        python3 make.py check uint128 bits gcc release
        ;;
    
    build|check|run|init|list|clean|analyze|demo|sanitize|compare|docker|wsl)
        python3 make.py "$@"
        ;;
    
    help|--help|-h)
        cat << 'EOF'

INT128/UINT128 - Docker Entrypoint
===================================

COMANDOS:
  test        Tests completos (default)
  bench       Benchmarks completos
  all         Pipeline completo
  quick       Rápido (GCC release)
  
  build       Compilar (delega a make.py)
  check       Verificar tests
  run         Ejecutar benchmarks
  
  init        Inicializar
  list        Listar combinaciones
  clean       Limpiar
  analyze     Análisis estático
  
EJEMPLOS:
  docker run --rm fixint-build:latest
  docker run --rm fixint-build:latest test
  docker run --rm fixint-build:latest bench
  docker run --rm fixint-build:latest build uint128 bits tests gcc release
  docker run --rm fixint-build:latest list
  docker run --rm fixint-build:latest bash    (shell interactivo)

DOCUMENTACIÓN:
  Ver DOCKER_BUILD_SYSTEM.md para guía completa

EOF
        ;;
    
    bash|sh|/bin/bash|/bin/sh|python3|python|gcc|g++|clang|clang++|cmake|make)
        exec "$@"
        ;;
    
    *)
        # Si no está en los comandos conocidos, intentar ejecutar como comando del shell
        exec "$@"
        ;;
esac

exit $?
