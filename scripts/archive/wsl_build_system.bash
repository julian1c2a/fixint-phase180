#!/bin/bash
# =============================================================================
# wsl_build_system.bash - Sistema de build para WSL Ubuntu
# =============================================================================
# Uso: bash scripts/wsl_build_system.bash <action> [args...]
#
# Este script replica el sistema de build de MSYS2 para WSL Ubuntu con:
#   - GCC 13, 14, 15
#   - Clang 18, 19, 20, 21
#   - Intel oneAPI icpx
#
# Acciones:
#   setup       - Instala compiladores y dependencias
#   detect      - Detecta compiladores disponibles
#   build       - Compila tests/benchs
#   check       - Ejecuta tests
#   run         - Ejecuta benchmarks
#   sanitize    - Compila con sanitizers
#   compare     - Benchmark comparativo
#   all         - Pipeline completo
#
# Ejemplos:
#   bash scripts/wsl_build_system.bash setup
#   bash scripts/wsl_build_system.bash build uint128 bits tests gcc-15 release
#   bash scripts/wsl_build_system.bash check uint128 bits all all
#   bash scripts/wsl_build_system.bash sanitize uint128 bits asan gcc-15
#   bash scripts/wsl_build_system.bash compare gcc-15 release-O3
# =============================================================================

set -e

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Directorio del proyecto
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Compiladores disponibles en Ubuntu
GCC_VERSIONS="13 14 15"
CLANG_VERSIONS="18 19 20 21"
INTEL_COMPILER="icpx"

# =============================================================================
# SETUP - Instalación de compiladores
# =============================================================================
cmd_setup() {
    echo -e "${BLUE}============================================================${NC}"
    echo -e "${BLUE}  SETUP - Instalación de Compiladores para Ubuntu${NC}"
    echo -e "${BLUE}============================================================${NC}"
    
    # Actualizar repositorios
    echo -e "${CYAN}[1/5] Actualizando repositorios...${NC}"
    sudo apt update
    
    # Instalar herramientas base
    echo -e "${CYAN}[2/5] Instalando herramientas base...${NC}"
    sudo apt install -y build-essential cmake ninja-build git curl wget
    
    # Instalar múltiples versiones de GCC
    echo -e "${CYAN}[3/5] Instalando GCC 13, 14, 15...${NC}"
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test 2>/dev/null || true
    sudo apt update
    
    for ver in $GCC_VERSIONS; do
        if ! command -v g++-$ver &>/dev/null; then
            echo -e "  Instalando g++-$ver..."
            sudo apt install -y g++-$ver gcc-$ver || echo -e "${YELLOW}  [WARN] g++-$ver no disponible${NC}"
        else
            echo -e "  ${GREEN}g++-$ver ya instalado${NC}"
        fi
    done
    
    # Instalar múltiples versiones de Clang
    echo -e "${CYAN}[4/5] Instalando Clang 18, 19, 20, 21...${NC}"
    
    # Agregar repositorio LLVM
    wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | sudo tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc >/dev/null 2>&1 || true
    
    # Detectar versión de Ubuntu
    UBUNTU_CODENAME=$(lsb_release -cs)
    echo "deb http://apt.llvm.org/$UBUNTU_CODENAME/ llvm-toolchain-$UBUNTU_CODENAME main" | sudo tee /etc/apt/sources.list.d/llvm.list >/dev/null 2>&1 || true
    
    for ver in $CLANG_VERSIONS; do
        echo "deb http://apt.llvm.org/$UBUNTU_CODENAME/ llvm-toolchain-$UBUNTU_CODENAME-$ver main" | sudo tee -a /etc/apt/sources.list.d/llvm.list >/dev/null 2>&1 || true
    done
    
    sudo apt update 2>/dev/null || true
    
    for ver in $CLANG_VERSIONS; do
        if ! command -v clang++-$ver &>/dev/null; then
            echo -e "  Instalando clang++-$ver..."
            sudo apt install -y clang-$ver || echo -e "${YELLOW}  [WARN] clang-$ver no disponible${NC}"
        else
            echo -e "  ${GREEN}clang++-$ver ya instalado${NC}"
        fi
    done
    
    # Intel oneAPI (instrucciones)
    echo -e "${CYAN}[5/5] Intel oneAPI...${NC}"
    if command -v icpx &>/dev/null; then
        echo -e "  ${GREEN}icpx ya instalado${NC}"
    else
        echo -e "  ${YELLOW}Intel oneAPI no instalado.${NC}"
        echo -e "  Instrucciones de instalación:"
        echo -e "    1. wget https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB"
        echo -e "    2. sudo apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB"
        echo -e "    3. echo 'deb https://apt.repos.intel.com/oneapi all main' | sudo tee /etc/apt/sources.list.d/oneAPI.list"
        echo -e "    4. sudo apt update && sudo apt install intel-oneapi-compiler-dpcpp-cpp"
        echo -e "    5. source /opt/intel/oneapi/setvars.sh"
    fi
    
    # Instalar herramientas de análisis
    echo -e "${CYAN}[+] Instalando herramientas de análisis...${NC}"
    sudo apt install -y cppcheck || true
    
    # Instalar Boost
    echo -e "${CYAN}[+] Instalando Boost...${NC}"
    sudo apt install -y libboost-all-dev || true
    
    echo ""
    echo -e "${GREEN}[OK] Setup completado${NC}"
    echo -e "Ejecuta 'bash scripts/wsl_build_system.bash detect' para ver compiladores disponibles"
}

# =============================================================================
# DETECT - Detectar compiladores disponibles
# =============================================================================
cmd_detect() {
    echo -e "${BLUE}============================================================${NC}"
    echo -e "${BLUE}  COMPILADORES DETECTADOS${NC}"
    echo -e "${BLUE}============================================================${NC}"
    
    echo -e "\n${CYAN}GCC:${NC}"
    for ver in $GCC_VERSIONS; do
        if command -v g++-$ver &>/dev/null; then
            version=$(g++-$ver --version | head -1)
            echo -e "  ${GREEN}✓${NC} g++-$ver: $version"
        else
            echo -e "  ${RED}✗${NC} g++-$ver: No instalado"
        fi
    done
    
    # También verificar g++ sin versión
    if command -v g++ &>/dev/null; then
        version=$(g++ --version | head -1)
        echo -e "  ${GREEN}✓${NC} g++ (default): $version"
    fi
    
    echo -e "\n${CYAN}Clang:${NC}"
    for ver in $CLANG_VERSIONS; do
        if command -v clang++-$ver &>/dev/null; then
            version=$(clang++-$ver --version | head -1)
            echo -e "  ${GREEN}✓${NC} clang++-$ver: $version"
        else
            echo -e "  ${RED}✗${NC} clang++-$ver: No instalado"
        fi
    done
    
    # También verificar clang++ sin versión
    if command -v clang++ &>/dev/null; then
        version=$(clang++ --version | head -1)
        echo -e "  ${GREEN}✓${NC} clang++ (default): $version"
    fi
    
    echo -e "\n${CYAN}Intel:${NC}"
    if command -v icpx &>/dev/null; then
        version=$(icpx --version 2>&1 | head -1)
        echo -e "  ${GREEN}✓${NC} icpx: $version"
    else
        echo -e "  ${RED}✗${NC} icpx: No instalado"
        echo -e "      Para instalar: source /opt/intel/oneapi/setvars.sh"
    fi
    
    echo -e "\n${CYAN}Herramientas:${NC}"
    for tool in cppcheck clang-tidy infer; do
        if command -v $tool &>/dev/null; then
            echo -e "  ${GREEN}✓${NC} $tool"
        else
            echo -e "  ${RED}✗${NC} $tool"
        fi
    done
    
    echo -e "\n${CYAN}Boost:${NC}"
    if [ -f /usr/include/boost/version.hpp ]; then
        boost_ver=$(grep "define BOOST_LIB_VERSION" /usr/include/boost/version.hpp | cut -d'"' -f2)
        echo -e "  ${GREEN}✓${NC} Boost $boost_ver"
    else
        echo -e "  ${RED}✗${NC} Boost no instalado"
    fi
}

# =============================================================================
# GET_COMPILER_CMD - Obtener comando del compilador
# =============================================================================
get_compiler_cmd() {
    local compiler="$1"
    
    case "$compiler" in
        gcc|gcc-default)
            echo "g++"
            ;;
        gcc-13|gcc-14|gcc-15)
            local ver="${compiler#gcc-}"
            if command -v g++-$ver &>/dev/null; then
                echo "g++-$ver"
            else
                echo "g++"
            fi
            ;;
        clang|clang-default)
            echo "clang++"
            ;;
        clang-18|clang-19|clang-20|clang-21)
            local ver="${compiler#clang-}"
            if command -v clang++-$ver &>/dev/null; then
                echo "clang++-$ver"
            else
                echo "clang++"
            fi
            ;;
        intel|icpx)
            echo "icpx"
            ;;
        *)
            echo "g++"
            ;;
    esac
}

# =============================================================================
# GET_MODE_FLAGS - Flags según modo
# =============================================================================
get_mode_flags() {
    local mode="$1"
    
    case "$mode" in
        debug)
            echo "-O0 -g -DDEBUG"
            ;;
        release|release-O2)
            echo "-O2 -DNDEBUG"
            ;;
        release-O1)
            echo "-O1 -DNDEBUG"
            ;;
        release-O3)
            echo "-O3 -fexpensive-optimizations -funroll-loops -ftree-vectorize -march=native -DNDEBUG"
            ;;
        release-Ofast)
            echo "-Ofast -ffast-math -march=native -DNDEBUG"
            ;;
        *)
            echo "-O2 -DNDEBUG"
            ;;
    esac
}

# =============================================================================
# BUILD - Compilar tests/benchs
# =============================================================================
cmd_build() {
    local type="$1"
    local feature="$2"
    local target="$3"
    local compiler="$4"
    local mode="$5"
    
    if [[ -z "$type" || -z "$feature" || -z "$target" ]]; then
        echo -e "${RED}Uso: bash scripts/wsl_build_system.bash build <type> <feature> <target> [compiler] [mode]${NC}"
        echo -e "  type:     uint128 | int128"
        echo -e "  feature:  t | bits | numeric | algorithm | etc."
        echo -e "  target:   tests | benchs"
        echo -e "  compiler: gcc-13 | gcc-14 | gcc-15 | clang-18..21 | icpx | all (default: gcc-15)"
        echo -e "  mode:     debug | release | release-O3 | all (default: release)"
        return 1
    fi
    
    compiler="${compiler:-gcc-15}"
    mode="${mode:-release}"
    
    cd "$PROJECT_ROOT"
    
    # Determinar archivo fuente
    local source_file
    if [[ "$target" == "tests" ]]; then
        source_file="tests/${type}_${feature}_extracted_tests.cpp"
    else
        source_file="benchs/${type}_${feature}_extracted_benchs.cpp"
    fi
    
    if [[ ! -f "$source_file" ]]; then
        echo -e "${RED}[ERROR] No existe: $source_file${NC}"
        return 1
    fi
    
    # Expandir "all"
    local compilers=("$compiler")
    local modes=("$mode")
    
    if [[ "$compiler" == "all" ]]; then
        compilers=()
        for ver in $GCC_VERSIONS; do
            command -v g++-$ver &>/dev/null && compilers+=("gcc-$ver")
        done
        for ver in $CLANG_VERSIONS; do
            command -v clang++-$ver &>/dev/null && compilers+=("clang-$ver")
        done
        command -v icpx &>/dev/null && compilers+=("icpx")
    fi
    
    if [[ "$mode" == "all" ]]; then
        modes=("debug" "release" "release-O3")
    fi
    
    echo -e "${BLUE}============================================================${NC}"
    echo -e "${BLUE}  BUILD - WSL Ubuntu${NC}"
    echo -e "${BLUE}============================================================${NC}"
    echo -e "  Source: $source_file"
    echo -e "  Compilers: ${compilers[*]}"
    echo -e "  Modes: ${modes[*]}"
    echo -e "${BLUE}------------------------------------------------------------${NC}"
    
    local success=0
    local failed=0
    
    for comp in "${compilers[@]}"; do
        for m in "${modes[@]}"; do
            local compiler_cmd=$(get_compiler_cmd "$comp")
            local mode_flags=$(get_mode_flags "$m")
            
            local build_dir="build/wsl_build_${target}/${comp}/${m}"
            mkdir -p "$build_dir"
            
            local output="${build_dir}/${type}_${feature}_${target}_${comp}"
            
            # Detectar threading
            local thread_flags=""
            if grep -qE '#include\s*<thread>|#include\s*<atomic>|thread_safety' "$source_file" 2>/dev/null; then
                thread_flags="-pthread -latomic"
            fi
            
            local cmd="$compiler_cmd -std=c++20 -Wall -Wextra -I include -I include_new $mode_flags $thread_flags $source_file -o $output"
            
            echo -ne "  [$comp] [$m] Compilando... "
            
            if eval "$cmd" 2>/dev/null; then
                echo -e "${GREEN}[OK]${NC}"
                ((success++))
            else
                echo -e "${RED}[FAIL]${NC}"
                ((failed++))
            fi
        done
    done
    
    echo -e "${BLUE}------------------------------------------------------------${NC}"
    echo -e "  Exitosos: ${GREEN}$success${NC}, Fallidos: ${RED}$failed${NC}"
}

# =============================================================================
# CHECK - Ejecutar tests
# =============================================================================
cmd_check() {
    local type="$1"
    local feature="$2"
    local compiler="${3:-all}"
    local mode="${4:-all}"
    
    if [[ -z "$type" || -z "$feature" ]]; then
        echo -e "${RED}Uso: bash scripts/wsl_build_system.bash check <type> <feature> [compiler] [mode]${NC}"
        return 1
    fi
    
    cd "$PROJECT_ROOT"
    
    echo -e "${BLUE}============================================================${NC}"
    echo -e "${BLUE}  CHECK - Ejecutar Tests WSL${NC}"
    echo -e "${BLUE}============================================================${NC}"
    
    # Buscar ejecutables
    local executables=()
    
    if [[ "$compiler" == "all" && "$mode" == "all" ]]; then
        executables=($(find build/wsl_build_tests -name "${type}_${feature}_tests_*" -type f -executable 2>/dev/null))
    elif [[ "$compiler" == "all" ]]; then
        executables=($(find build/wsl_build_tests/*/$mode -name "${type}_${feature}_tests_*" -type f -executable 2>/dev/null))
    elif [[ "$mode" == "all" ]]; then
        executables=($(find build/wsl_build_tests/$compiler -name "${type}_${feature}_tests_*" -type f -executable 2>/dev/null))
    else
        executables=($(find build/wsl_build_tests/$compiler/$mode -name "${type}_${feature}_tests_*" -type f -executable 2>/dev/null))
    fi
    
    if [[ ${#executables[@]} -eq 0 ]]; then
        echo -e "${YELLOW}No se encontraron ejecutables. Compilando primero...${NC}"
        cmd_build "$type" "$feature" tests "$compiler" "$mode"
        executables=($(find build/wsl_build_tests -name "${type}_${feature}_tests_*" -type f -executable 2>/dev/null))
    fi
    
    local pass=0
    local fail=0
    
    for exe in "${executables[@]}"; do
        local name=$(basename "$exe")
        echo -ne "  $name: "
        
        if timeout 60 "$exe" >/dev/null 2>&1; then
            echo -e "${GREEN}PASS${NC}"
            ((pass++))
        else
            echo -e "${RED}FAIL${NC}"
            ((fail++))
        fi
    done
    
    echo -e "${BLUE}------------------------------------------------------------${NC}"
    echo -e "  PASS: ${GREEN}$pass${NC}, FAIL: ${RED}$fail${NC}"
    
    [[ $fail -eq 0 ]] && return 0 || return 1
}

# =============================================================================
# SANITIZE - Compilar con sanitizers
# =============================================================================
cmd_sanitize() {
    local type="$1"
    local feature="$2"
    local sanitizer="${3:-asan}"
    local compiler="${4:-gcc-15}"
    local target="${5:-tests}"
    
    if [[ -z "$type" || -z "$feature" ]]; then
        echo -e "${RED}Uso: bash scripts/wsl_build_system.bash sanitize <type> <feature> [sanitizer] [compiler] [target]${NC}"
        echo -e "  sanitizer: asan | ubsan | tsan | msan | all"
        return 1
    fi
    
    cd "$PROJECT_ROOT"
    
    local source_file
    if [[ "$target" == "tests" ]]; then
        source_file="tests/${type}_${feature}_extracted_tests.cpp"
    else
        source_file="benchs/${type}_${feature}_extracted_benchs.cpp"
    fi
    
    if [[ ! -f "$source_file" ]]; then
        echo -e "${RED}[ERROR] No existe: $source_file${NC}"
        return 1
    fi
    
    local compiler_cmd=$(get_compiler_cmd "$compiler")
    
    # Flags de sanitizer
    local san_flags
    case "$sanitizer" in
        asan)   san_flags="-fsanitize=address -fno-omit-frame-pointer -g" ;;
        ubsan)  san_flags="-fsanitize=undefined -fno-omit-frame-pointer -g" ;;
        tsan)   san_flags="-fsanitize=thread -fno-omit-frame-pointer -g" ;;
        msan)   san_flags="-fsanitize=memory -fno-omit-frame-pointer -g" ;;
        all)    san_flags="-fsanitize=address,undefined -fno-omit-frame-pointer -g" ;;
    esac
    
    local build_dir="build/wsl_sanitized/${compiler}/${sanitizer}"
    mkdir -p "$build_dir"
    
    local output="${build_dir}/${type}_${feature}_${target}"
    
    echo -e "${BLUE}============================================================${NC}"
    echo -e "${BLUE}  SANITIZE - WSL Ubuntu${NC}"
    echo -e "${BLUE}============================================================${NC}"
    echo -e "  Sanitizer: ${GREEN}$sanitizer${NC}"
    echo -e "  Compiler:  ${GREEN}$compiler_cmd${NC}"
    echo -e "${BLUE}------------------------------------------------------------${NC}"
    
    local thread_flags=""
    if grep -qE '#include\s*<thread>|#include\s*<atomic>|thread_safety' "$source_file" 2>/dev/null; then
        thread_flags="-pthread"
    fi
    
    local cmd="$compiler_cmd -std=c++20 -Wall -Wextra -I include -I include_new $san_flags $thread_flags $source_file -o $output"
    
    echo -e "  Comando: $cmd"
    echo ""
    
    if eval "$cmd"; then
        echo -e "${GREEN}[OK] Compilación exitosa: $output${NC}"
        echo ""
        echo -e "${YELLOW}Ejecutando con sanitizer...${NC}"
        echo ""
        "$output"
    else
        echo -e "${RED}[FAIL] Error de compilación${NC}"
        return 1
    fi
}

# =============================================================================
# COMPARE - Benchmark comparativo
# =============================================================================
cmd_compare() {
    local compiler="${1:-gcc-15}"
    local mode="${2:-release-O3}"
    local iterations="${3:-100000}"
    
    cd "$PROJECT_ROOT"
    
    # Usar el script existente adaptado
    if [[ -f "scripts/benchmark_comparison.bash" ]]; then
        # Adaptar el compilador para Linux
        local linux_compiler
        case "$compiler" in
            gcc-*) linux_compiler="${compiler#gcc-}"; linux_compiler="g++-$linux_compiler" ;;
            clang-*) linux_compiler="${compiler#clang-}"; linux_compiler="clang++-$linux_compiler" ;;
            *) linux_compiler="g++" ;;
        esac
        
        echo -e "${BLUE}============================================================${NC}"
        echo -e "${BLUE}  BENCHMARK COMPARATIVO - WSL${NC}"
        echo -e "${BLUE}============================================================${NC}"
        echo -e "  Compiler: $linux_compiler"
        echo -e "  Mode: $mode"
        echo -e "  Iterations: $iterations"
        
        # El script benchmark_comparison.bash debería funcionar en Linux
        # pero necesitamos ajustar el compilador
        export WSL_COMPILER="$linux_compiler"
        bash scripts/benchmark_comparison.bash gcc "$mode" "$iterations"
    else
        echo -e "${RED}Script benchmark_comparison.bash no encontrado${NC}"
        return 1
    fi
}

# =============================================================================
# HELP
# =============================================================================
cmd_help() {
    echo -e "${BLUE}============================================================${NC}"
    echo -e "${BLUE}  WSL BUILD SYSTEM - int128 Library${NC}"
    echo -e "${BLUE}============================================================${NC}"
    echo ""
    echo -e "${CYAN}ACCIONES:${NC}"
    echo "  setup      - Instalar compiladores y dependencias"
    echo "  detect     - Detectar compiladores disponibles"
    echo "  build      - Compilar tests/benchs"
    echo "  check      - Ejecutar tests"
    echo "  run        - Ejecutar benchmarks"
    echo "  sanitize   - Compilar con sanitizers"
    echo "  compare    - Benchmark comparativo"
    echo "  help       - Esta ayuda"
    echo ""
    echo -e "${CYAN}COMPILADORES DISPONIBLES:${NC}"
    echo "  gcc-13, gcc-14, gcc-15"
    echo "  clang-18, clang-19, clang-20, clang-21"
    echo "  icpx (Intel)"
    echo "  all (todos los disponibles)"
    echo ""
    echo -e "${CYAN}MODOS:${NC}"
    echo "  debug, release, release-O1, release-O2, release-O3, release-Ofast, all"
    echo ""
    echo -e "${CYAN}SANITIZERS:${NC}"
    echo "  asan, ubsan, tsan, msan, all"
    echo ""
    echo -e "${CYAN}EJEMPLOS:${NC}"
    echo "  bash scripts/wsl_build_system.bash setup"
    echo "  bash scripts/wsl_build_system.bash detect"
    echo "  bash scripts/wsl_build_system.bash build uint128 bits tests gcc-15 release"
    echo "  bash scripts/wsl_build_system.bash check uint128 bits all all"
    echo "  bash scripts/wsl_build_system.bash sanitize uint128 bits asan gcc-15"
    echo "  bash scripts/wsl_build_system.bash compare gcc-15 release-O3 100000"
}

# =============================================================================
# MAIN
# =============================================================================
ACTION="${1:-help}"
shift || true

case "$ACTION" in
    setup)    cmd_setup "$@" ;;
    detect)   cmd_detect "$@" ;;
    build)    cmd_build "$@" ;;
    check)    cmd_check "$@" ;;
    run)      cmd_build "$@"; cmd_check "$@" ;;  # run = build + execute benchs
    sanitize) cmd_sanitize "$@" ;;
    compare)  cmd_compare "$@" ;;
    help|--help|-h) cmd_help ;;
    *)
        echo -e "${RED}Acción desconocida: $ACTION${NC}"
        cmd_help
        exit 1
        ;;
esac
