// =============================================================================
// Benchmark Infrastructure: RDTSC Cycle Measurement & Anti-Optimization
// Part of int128 Library - https://github.com/julian1c2a/int128-phase175
// License: BSL-1.0
// =============================================================================
//
// Shared infrastructure for all benchmarks in the int128 library.
// Provides: rdtsc(), CycleTimer, doNotOptimize<T>(), BenchResult, print helpers.
//
// Usage:
//   #include "bench_common.hpp"
//
// All benchmarks MUST use this header instead of defining their own
// timing infrastructure. See docs/PLAN_BENCHMARK_AND_TESTING_METHODOLOGY.md.
//
// =============================================================================

#ifndef BENCH_COMMON_HPP
#define BENCH_COMMON_HPP

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>

// ============================================================================
// Contador de tiempo, no de ciclos  -- OJO CON EL NOMBRE
//
// Esto se llamaba "cycle-accurate, clock-frequency independent", y ESO ES FALSO
// en los procesadores modernos. El TSC de hoy es *invariante*: avanza a una
// frecuencia de referencia FIJA, no a la del nucleo. O sea que RDTSC es un
// cronometro de alta resolucion, no un contador de ciclos.
//
// La consecuencia importa: si el turbo sube o baja --por carga, por temperatura,
// por otro proceso--, el trabajo hecho por tic cambia, y la cifra de "ciclos por
// operacion" se mueve AUNQUE EL CODIGO SEA IDENTICO. Elegir RDTSC no inmuniza
// contra el ruido del sistema, que es lo que se creia.
//
// Medido el 5 sep 2026: el mismo binario, en la misma maquina, da una mediana
// del 5,1 % de diferencia entre ejecuciones, y hasta un 52 % si la maquina esta
// haciendo otra cosa. Ver docs/PERFORMANCE.md.
//
// Se conserva RDTSC porque es lo mejor que hay de forma portable --contar ciclos
// de verdad pide contadores de rendimiento del nucleo, con privilegios y sin
// portabilidad--, pero se deja de llamarlo lo que no es.
//
// Contador por arquitectura:
//   x86_64:  RDTSC (invariante: tiempo, no ciclos)
//   ARM64:   CNTVCT_EL0 virtual counter (sub-nanosecond, ~1-50 MHz freq)
//   RISC-V:  rdtime CSR (platform timer, comparable to ARM64 virtual counter)
//   Fallback: std::chrono nanoseconds cast to uint64_t
// ============================================================================

#if defined(_MSC_VER) || (defined(__INTEL_LLVM_COMPILER) && defined(_WIN32))
#include <intrin.h>
static inline std::uint64_t rdtsc() { return __rdtsc(); }
#elif defined(__INTEL_LLVM_COMPILER)
#include <x86intrin.h>
static inline std::uint64_t rdtsc() { return __rdtsc(); }
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
static inline std::uint64_t rdtsc() { return __builtin_ia32_rdtsc(); }
#elif defined(__aarch64__) || defined(_M_ARM64)
static inline std::uint64_t rdtsc()
{
    std::uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}
#elif defined(__riscv)
static inline std::uint64_t rdtsc()
{
    std::uint64_t val;
    __asm__ volatile("rdtime %0" : "=r"(val));
    return val;
}
#else
#include <chrono>
static inline std::uint64_t rdtsc()
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          std::chrono::steady_clock::now().time_since_epoch())
                                          .count());
}
#endif

// ============================================================================
// Configuration defaults (overridable via -D at compile time)
// ============================================================================

#ifndef BENCH_ITERATIONS
#define BENCH_ITERATIONS 5000000
#endif

#ifndef BENCH_WARMUP
#define BENCH_WARMUP 10000
#endif

static constexpr std::size_t ITERATIONS{BENCH_ITERATIONS};
static constexpr std::size_t WARMUP{BENCH_WARMUP};

// ============================================================================
// Cycle counter (RDTSC-based, clock-frequency independent)
// ============================================================================

class CycleTimer
{
    std::uint64_t start_;

public:
    CycleTimer() : start_{rdtsc()} {}

    void reset() { start_ = rdtsc(); }

    std::uint64_t elapsed_cycles() const { return rdtsc() - start_; }
};

// ============================================================================
// Prevent optimization (volatile sink)
// ============================================================================
//
// Compiler-specific implementations to prevent dead code elimination
// without introducing unnecessary memory traffic or register shuffling.

template <typename T>
static void doNotOptimize(T &val)
{
#if defined(_MSC_VER) || defined(__INTEL_LLVM_COMPILER)
    // MSVC/Intel-Windows: read+write one byte through volatile pointer.
    // Forces the compiler to actually compute val (can't be eliminated).
    *reinterpret_cast<char volatile *>(&val) = *reinterpret_cast<char volatile *>(&val);
#elif defined(__clang__)
    // Clang: "+r,m" works well -- Clang chooses memory operands for structs,
    // which avoids register shuffling.
    asm volatile("" : "+r,m"(val) : : "memory");
#else
    // GCC: For 128-bit structs, "+r,m" causes 4 unnecessary register
    // shuffles per iteration (GCC can't map a struct to a register pair).
    // Fix: split into two separate "+r" constraints on each uint64_t half.
    // This generates the same optimal addq/adcq or subq/sbbq as __int128.
    // NOTE: No "memory" clobber for 16-byte types — the "+r" constraints
    // already prevent dead-code elimination. The "memory" clobber causes
    // GCC to spill structs to stack (but not __int128 register pairs),
    // creating a measurement artifact.
    if constexpr (sizeof(T) == 16 && alignof(T) >= alignof(std::uint64_t))
    {
        auto *p = reinterpret_cast<std::uint64_t *>(&val);
        asm volatile("" : "+r"(p[0]), "+r"(p[1]));
    }
    else
    {
        asm volatile("" : "+r,m"(val) : : "memory");
    }
#endif
}

// ============================================================================
// Registro legible por maquina
// ============================================================================
//
// Ademas de la tabla para leer, cada benchmark puede dejar sus cifras en un
// fichero para que se guarden en el historico. Se activa poniendo la variable
// de entorno BENCH_OUT; si no esta, no pasa nada y la salida de siempre no
// cambia. Formato: una linea por medida, separada por tabuladores.
//
//     <caso>\t<valor>\t<unidad>
//
// Los metadatos que exige la regla de docs/PERFORMANCE.md --fecha, compilador,
// maquina, modo, commit-- NO se ponen aqui: los anade scripts/bench_history.py,
// que es quien los sabe. Un benchmark no tiene por que saber en que commit esta.

static void bench_record(const char *caso, double valor, const char *unidad = "cyc/op")
{
    const char *destino = std::getenv("BENCH_OUT");
    if (destino == nullptr || *destino == '\0')
        return;

    std::ofstream f(destino, std::ios::app);
    if (!f)
        return;
    f << caso << '\t' << std::fixed << std::setprecision(4) << valor << '\t' << unidad << '\n';
}

// ============================================================================
// Result formatting
// ============================================================================

struct BenchResult
{
    std::string name;
    double cycles_per_op;
};

static void print_separator()
{
    std::cout << "+-------------------------------+--------------+-----------+\n";
}

// La seccion en curso, para que las medidas del historico sepan de QUE operacion
// son.
//
// POR QUE. `benchmark_vs_builtin` mide siete operaciones --suma, resta,
// producto, division, desplazamiento, xor y comparacion-- sobre los mismos
// dieciseis tipos, y registraba cada medida con el nombre del TIPO a secas. En
// el TSV salian siete filas `uint64_t` indistinguibles, y una medida que no dice
// que operacion es no se puede comparar con la de manana. Visto el 9 sep 2026 al
// ir a re-medir las tablas heredadas (P2.3).
static std::string g_seccion_actual;

static void print_header(const char *operation)
{
    g_seccion_actual = operation ? operation : "";
    std::cout << "\n[" << operation << "]\n";
    print_separator();
    std::cout << "| Type                          |  cyc/op      | vs u64    |\n";
    print_separator();
}

static void print_result(const BenchResult &r, double baseline_cyc)
{
    const std::string etiqueta = g_seccion_actual.empty() ? r.name : (g_seccion_actual + " / " + r.name);
    bench_record(etiqueta.c_str(), r.cycles_per_op);
    const double ratio{(baseline_cyc > 0.0) ? r.cycles_per_op / baseline_cyc : 0.0};
    std::cout << "| " << std::left << std::setw(29) << r.name << " | " << std::right << std::fixed
              << std::setprecision(2) << std::setw(12) << r.cycles_per_op << " | " << std::fixed
              << std::setprecision(2) << std::setw(6) << ratio << "x   |\n";
}

static void print_footer() { print_separator(); }

#endif // BENCH_COMMON_HPP
