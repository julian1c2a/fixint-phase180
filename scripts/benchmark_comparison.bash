#!/bin/bash
# =============================================================================
# benchmark_comparison.bash - Benchmarks comparativos con múltiples backends
# =============================================================================
# Uso: bash scripts/benchmark_comparison.bash [compiler] [mode] [iterations]
#
# Compara rendimiento de:
#   - nstd::uint128_t / nstd::int128_t (este proyecto)
#   - unsigned __int128 / __int128 (GCC/Clang builtin)
#   - boost::multiprecision::uint128_t (cpp_int backend)
#   - Tipos builtin (uint64_t, int64_t) como baseline
#
# Requisitos:
#   - Boost instalado: pacman -S mingw-w64-ucrt-x86_64-boost
#   - GCC o Clang (para __int128 builtin)
#
# Ejemplos:
#   bash scripts/benchmark_comparison.bash gcc release-O3
#   bash scripts/benchmark_comparison.bash clang release-Ofast 1000000
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
COMPILER="${1:-gcc}"
MODE="${2:-release-O3}"
ITERATIONS="${3:-100000}"

# Directorio del proyecto
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"

# Crear directorio de benchmarks
BUILD_DIR="build/benchmark_comparison/${COMPILER}/${MODE}"
mkdir -p "$BUILD_DIR"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_FILE="$BUILD_DIR/results_${TIMESTAMP}.txt"

# Flags según modo
get_mode_flags() {
    case "$MODE" in
        debug)         echo "-O0 -g" ;;
        release)       echo "-O2" ;;
        release-O1)    echo "-O1" ;;
        release-O2)    echo "-O2" ;;
        release-O3)    echo "-O3 -fexpensive-optimizations -funroll-loops -ftree-vectorize -march=native" ;;
        release-Ofast) echo "-Ofast -ffast-math -fexpensive-optimizations -march=native" ;;
        *)             echo "-O2" ;;
    esac
}

# Verificar Boost
check_boost() {
    local test_file=$(mktemp --suffix=.cpp)
    cat > "$test_file" << 'EOF'
#include <boost/multiprecision/cpp_int.hpp>
int main() {
    boost::multiprecision::uint128_t x = 42;
    return 0;
}
EOF
    
    if g++ -std=c++20 "$test_file" -o /dev/null 2>/dev/null; then
        rm -f "$test_file"
        return 0
    else
        rm -f "$test_file"
        return 1
    fi
}

# Crear archivo de benchmark
create_benchmark_source() {
    cat > "$BUILD_DIR/benchmark_comparison.cpp" << 'BENCHMARK_EOF'
// =============================================================================
// Benchmark Comparativo: nstd::uint128_t vs __int128 vs Boost.Multiprecision
// =============================================================================

#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdint>

// Este proyecto
#include "int128_base_tt.hpp"

// Boost (si está disponible)
#ifdef HAS_BOOST
#include <boost/multiprecision/cpp_int.hpp>
namespace bmp = boost::multiprecision;
#endif

// Builtin __int128 (GCC/Clang)
#ifdef __SIZEOF_INT128__
#define HAS_BUILTIN_INT128 1
#endif

using namespace nstd;

// Configuración
constexpr size_t ITERATIONS = BENCHMARK_ITERATIONS;
constexpr size_t WARMUP = 1000;

// Timer helper
class Timer {
    std::chrono::high_resolution_clock::time_point start_;
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed_ns() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::nano>(end - start_).count();
    }
    
    double elapsed_ms() const {
        return elapsed_ns() / 1'000'000.0;
    }
};

// Resultado de benchmark
struct BenchResult {
    std::string name;
    double time_ns;
    double ops_per_sec;
};

void print_header() {
    std::cout << "\n";
    std::cout << "+---------------------------+---------------+------------------+\n";
    std::cout << "| Tipo                      | Tiempo (ns)   | Ops/segundo      |\n";
    std::cout << "+---------------------------+---------------+------------------+\n";
}

void print_result(const BenchResult& r) {
    std::cout << "| " << std::left << std::setw(25) << r.name 
              << " | " << std::right << std::setw(13) << std::fixed << std::setprecision(2) << r.time_ns
              << " | " << std::setw(16) << std::scientific << std::setprecision(2) << r.ops_per_sec
              << " |\n";
}

void print_footer() {
    std::cout << "+---------------------------+---------------+------------------+\n";
}

// =============================================================================
// BENCHMARKS
// =============================================================================

// --- Addition ---
template<typename T>
BenchResult bench_addition(const std::string& name) {
    T a{12345678901234567890ULL};
    T b{98765432109876543210ULL};
    T result{0};
    
    // Warmup
    for (size_t i = 0; i < WARMUP; ++i) {
        result = a + b;
        a = result;
    }
    
    Timer timer;
    for (size_t i = 0; i < ITERATIONS; ++i) {
        result = a + b;
        a = result ^ b;  // Prevent optimization
    }
    double elapsed = timer.elapsed_ns();
    
    return {name, elapsed / ITERATIONS, ITERATIONS / (elapsed / 1e9)};
}

// --- Multiplication ---
template<typename T>
BenchResult bench_multiplication(const std::string& name) {
    T a{123456789ULL};
    T b{987654321ULL};
    T result{0};
    
    for (size_t i = 0; i < WARMUP; ++i) {
        result = a * b;
    }
    
    Timer timer;
    for (size_t i = 0; i < ITERATIONS; ++i) {
        result = a * b;
        a = (result & T{0xFFFFFFFF}) + T{1};  // Prevent optimization
    }
    double elapsed = timer.elapsed_ns();
    
    return {name, elapsed / ITERATIONS, ITERATIONS / (elapsed / 1e9)};
}

// --- Division ---
template<typename T>
BenchResult bench_division(const std::string& name) {
    T a{12345678901234567890ULL};
    T b{12345ULL};
    T result{0};
    
    for (size_t i = 0; i < WARMUP; ++i) {
        result = a / b;
    }
    
    Timer timer;
    for (size_t i = 0; i < ITERATIONS; ++i) {
        result = a / b;
        a = result + T{1};  // Prevent optimization
    }
    double elapsed = timer.elapsed_ns();
    
    return {name, elapsed / ITERATIONS, ITERATIONS / (elapsed / 1e9)};
}

// --- Bitwise XOR ---
template<typename T>
BenchResult bench_xor(const std::string& name) {
    T a{0xDEADBEEFDEADBEEFULL};
    T b{0xCAFEBABECAFEBABEULL};
    T result{0};
    
    for (size_t i = 0; i < WARMUP; ++i) {
        result = a ^ b;
    }
    
    Timer timer;
    for (size_t i = 0; i < ITERATIONS; ++i) {
        result = a ^ b;
        a = result;
    }
    double elapsed = timer.elapsed_ns();
    
    return {name, elapsed / ITERATIONS, ITERATIONS / (elapsed / 1e9)};
}

// --- Comparison ---
template<typename T>
BenchResult bench_comparison(const std::string& name) {
    T a{12345678901234567890ULL};
    T b{12345678901234567891ULL};
    volatile bool result = false;
    
    for (size_t i = 0; i < WARMUP; ++i) {
        result = (a < b);
    }
    
    Timer timer;
    for (size_t i = 0; i < ITERATIONS; ++i) {
        result = (a < b);
        if (result) a = a + T{1};
    }
    double elapsed = timer.elapsed_ns();
    
    return {name, elapsed / ITERATIONS, ITERATIONS / (elapsed / 1e9)};
}

// =============================================================================
// MAIN
// =============================================================================

int main() {
    std::cout << "============================================================\n";
    std::cout << "  BENCHMARK COMPARATIVO - int128 Performance\n";
    std::cout << "============================================================\n";
    std::cout << "  Iteraciones: " << ITERATIONS << "\n";
    std::cout << "  Warmup:      " << WARMUP << "\n";
    std::cout << "============================================================\n";
    
    std::vector<BenchResult> results;
    
    // =========================================================================
    // ADDITION
    // =========================================================================
    std::cout << "\n[1/5] Benchmark: Addition (+)\n";
    print_header();
    
    // Builtin integers (baseline)
    auto r = bench_addition<uint8_t>("uint8_t");
    print_result(r);
    r = bench_addition<uint16_t>("uint16_t");
    print_result(r);
    r = bench_addition<uint32_t>("uint32_t");
    print_result(r);
    r = bench_addition<uint64_t>("uint64_t");
    print_result(r);
    r = bench_addition<int8_t>("int8_t");
    print_result(r);
    r = bench_addition<int16_t>("int16_t");
    print_result(r);
    r = bench_addition<int32_t>("int32_t");
    print_result(r);
    r = bench_addition<int64_t>("int64_t");
    print_result(r);
    
    // Este proyecto
    r = bench_addition<uint128_t>("nstd::uint128_t");
    print_result(r);
    r = bench_addition<int128_t>("nstd::int128_t");
    print_result(r);
    
#ifdef HAS_BUILTIN_INT128
    r = bench_addition<unsigned __int128>("unsigned __int128");
    print_result(r);
    r = bench_addition<__int128>("__int128");
    print_result(r);
#endif

#ifdef HAS_BOOST
    r = bench_addition<bmp::uint128_t>("boost::uint128_t");
    print_result(r);
#endif
    
    print_footer();
    
    // ==========================8_t>("uint8_t");
    print_result(r);
    r = bench_multiplication<uint16_t>("uint16_t");
    print_result(r);
    r = bench_multiplication<uint32_t>("uint32_t");
    print_result(r);
    r = bench_multiplication<uint64_t>("uint64_t");
    print_result(r);
    r = bench_multiplication<int64_t>("int64_t");
    print_result(r);
    
    r = bench_multiplication<uint128_t>("nstd::uint128_t");
    print_result(r);
    r = bench_multiplication<int128_t>("nstd::int128_t");
    print_result(r);
    
#ifdef HAS_BUILTIN_INT128
    r = bench_multiplication<unsigned __int128>("unsigned __int128");
    print_result(r);
    r = bench_multiplication<__int128>("
    
    r = bench_multiplication<uint128_t>("nstd::uint128_t");
    print_result(r);
    
#ifdef HAS_BUILTIN_INT128
    r = bench_multiplication<unsigned __int128>("unsigned __int128");
    print_result(r);
#endif

#ifdef HAS_BOOST
    r = bench_multiplicatio32_t>("uint32_t");
    print_result(r);
    r = bench_division<uint64_t>("uint64_t");
    print_result(r);
    r = bench_division<int64_t>("int64_t");
    print_result(r);
    
    r = bench_division<uint128_t>("nstd::uint128_t");
    print_result(r);
    r = bench_division<int128_t>("nstd::int128_t");
    print_result(r);
    
#ifdef HAS_BUILTIN_INT128
    r = bench_division<unsigned __int128>("unsigned __int128");
    print_result(r);
    r = bench_division<__int128>("
    // =========================================================================
    std::cout << "\n[3/5] Benchmark: Division (/)\n";
    print_header();
    
    r = bench_division<uint64_t>("uint64_t (baseline)");
    print_result(r);
    
    r = bench_division<uint128_t>("nstd::uint128_t");
    print_result(r);
    
#ifdef HAS_BUILTIN_INT128
    r = bench_division<unsigned __int128>("unsigned __int128");
    print_result(r);
#endif

#ifdef HAS_BOOST
    r = bench_division<bmp::uint128_t>("boost::uint128_t");
    print_result(r);
#endif
    
    print_footer();32_t>("uint32_t");
    print_result(r);
    r = bench_xor<uint64_t>("uint64_t");
    print_result(r);
    
    r = bench_xor<uint128_t>("nstd::uint128_t");
    print_result(r);
    r = bench_xor<int128_t>("nstd::int128_t");
    print_result(r);
    
#ifdef HAS_BUILTIN_INT128
    r = bench_xor<unsigned __int128>("unsigned __int128");
    print_result(r);
    r = bench_xor<__int128>("
    r = bench_xor<uint64_t>("uint64_t (baseline)");
    print_result(r);
    
    r = bench_xor<uint128_t>("nstd::uint128_t");
    print_result(r);
    
#ifdef HAS_BUILTIN_INT128
    r = bench_xor<unsigned __int128>("unsigned __int128");
    print_result(r);
#endif
32_t>("uint32_t");
    print_result(r);
    r = bench_comparison<uint64_t>("uint64_t");
    print_result(r);
    r = bench_comparison<int64_t>("int64_t");
    print_result(r);
    
    r = bench_comparison<uint128_t>("nstd::uint128_t");
    print_result(r);
    r = bench_comparison<int128_t>("nstd::int128_t");
    print_result(r);
    
#ifdef HAS_BUILTIN_INT128
    r = bench_comparison<unsigned __int128>("unsigned __int128");
    print_result(r);
    r = bench_comparison<__int128>("
    // =========================================================================
    // COMPARISON
    // =========================================================================
    std::cout << "\n[5/5] Benchmark: Comparison (<)\n";
    print_header();
    
    r = bench_comparison<uint64_t>("uint64_t (baseline)");
    print_result(r);
    
    r = bench_comparison<uint128_t>("nstd::uint128_t");
    print_result(r);
    
#ifdef HAS_BUILTIN_INT128
    r = bench_comparison<unsigned __int128>("unsigned __int128");
    print_result(r);
#endif

#ifdef HAS_BOOST
    r = bench_comparison<bmp::uint128_t>("boost::uint128_t");
    print_result(r);
#endif
    
    print_footer();
    
    std::cout << "\n============================================================\n";
    std::cout << "  BENCHMARK COMPLETADO\n";
    std::cout << "============================================================\n";
    
    return 0;
}
BENCHMARK_EOF

    # Reemplazar BENCHMARK_ITERATIONS
    sed -i "s/BENCHMARK_ITERATIONS/${ITERATIONS}/" "$BUILD_DIR/benchmark_comparison.cpp"
}

# =============================================================================
# MAIN
# =============================================================================
echo -e "${BLUE}============================================================${NC}"
echo -e "${BLUE}  BENCHMARK COMPARATIVO${NC}"
echo -e "${BLUE}============================================================${NC}"
echo -e "  Compilador:   ${GREEN}$COMPILER${NC}"
echo -e "  Modo:         ${GREEN}$MODE${NC}"
echo -e "  Iteraciones:  ${GREEN}$ITERATIONS${NC}"
echo -e "${BLUE}------------------------------------------------------------${NC}"

# Verificar Boost
HAS_BOOST=0
if check_boost; then
    echo -e "  Boost:        ${GREEN}Detectado${NC}"
    HAS_BOOST=1
else
    echo -e "  Boost:        ${YELLOW}No encontrado (instalar: pacman -S mingw-w64-ucrt-x86_64-boost)${NC}"
fi

# Crear fuente
create_benchmark_source

# Compilar
MODE_FLAGS=$(get_mode_flags "$MODE")
COMPILER_CMD="g++"
[[ "$COMPILER" == "clang" ]] && COMPILER_CMD="clang++"

DEFINES=""
[[ $HAS_BOOST -eq 1 ]] && DEFINES="-DHAS_BOOST"

OUTPUT="$BUILD_DIR/benchmark_comparison"

echo -e "${BLUE}------------------------------------------------------------${NC}"
echo -e "  Compilando..."

CMD="$COMPILER_CMD -std=c++20 $MODE_FLAGS -I include -I include_new $DEFINES $BUILD_DIR/benchmark_comparison.cpp -o $OUTPUT"
echo -e "  ${YELLOW}$CMD${NC}"

if eval "$CMD"; then
    echo -e "${GREEN}[OK] Compilación exitosa${NC}"
else
    echo -e "${RED}[FAIL] Error de compilación${NC}"
    exit 1
fi

# Ejecutar
echo -e "${BLUE}------------------------------------------------------------${NC}"
echo -e "  Ejecutando benchmark..."
echo ""

"$OUTPUT" | tee "$RESULT_FILE"

echo ""
echo -e "${GREEN}[OK] Resultados guardados en: $RESULT_FILE${NC}"
