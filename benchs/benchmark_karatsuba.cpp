// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// int128 Library - 128-bit Integer Types for C++20
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0
//
// Copyright (c) 2024-2026 Julian Calderon Almendros
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//
// =============================================================================
// @file       benchmark_karatsuba.cpp
// @brief      Karatsuba frente a la multiplicacion escolar, para N=2,3,4,8,16
// @date       2026-08-25
// =============================================================================
//
// `fixed_int_t::operator*` toma el camino de Karatsuba para N=4 y N=8, y el
// escolar O(N^2) para el resto. Karatsuba era una de las optimizaciones de
// cabecera de v1.90 y nunca se habia medido: este benchmark existe para eso.
//
// Metodo: las dos variantes se miden INTERCALADAS dentro de cada ronda, y se
// toma el MINIMO de las rondas para cada caso. Intercalar reparte por igual la
// deriva termica y el ruido del planificador; el minimo se queda con la ronda
// menos contaminada, que es la que mas se parece al coste real. Una media
// mediria sobre todo el ruido del sistema.
//
// EL CONTROL ES N=16. Ahi la biblioteca usa el mismo bucle escolar que la
// implementacion de referencia de abajo, asi que la razon TIENE que salir
// ~1.00x. Si no sale, la referencia no es fiel y ninguna otra cifra vale.
//
// (La primera version de este benchmark uso una propagacion de acarreo
// portable, con un `while` y un salto dependiente de los datos, en vez de los
// intrinsecos de la biblioteca. El control salio 2.00x en N=16 y 6.23x en N=2:
// no se estaba midiendo Karatsuba contra el metodo escolar, sino la biblioteca
// contra un espantapajaros. De ahi que el control este aqui.)
// =============================================================================

#include "../include/fixed_width_int_t.hpp"
#include "../include/intrinsics/arithmetic_operations.hpp"
#include "bench_common.hpp"

#include <vector>

using namespace nstd;

// ============================================================================
// Multiplicacion escolar O(N^2), el camino que Karatsuba pretende batir
// ============================================================================
//
// COPIA FIEL del bucle general de `fixed_int_t::operator*`, con sus mismas
// primitivas: `intrinsics::umul128` para el producto 64x64->128 y
// `intrinsics::addcarry_u64` para la cadena de acarreo (que compila a ADC).
// Lo unico que cambia entre las dos ramas medidas es el algoritmo.

namespace ref
{
    inline unsigned char add_limb(std::uint64_t &limb, std::uint64_t v) noexcept
    {
        return intrinsics::addcarry_u64(0, limb, v, &limb);
    }

    inline unsigned char add_limb_carry(std::uint64_t &limb, std::uint64_t v, unsigned char c) noexcept
    {
        return intrinsics::addcarry_u64(c, limb, v, &limb);
    }
} // namespace ref

template <std::size_t N>
[[nodiscard]] uint_fixed_t<N> schoolbook_mul(const uint_fixed_t<N> &a, const uint_fixed_t<N> &b) noexcept
{
    uint_fixed_t<N> out{};
    auto &r = out.limbs_ref();
    const auto &x = a.limbs();
    const auto &y = b.limbs();

    for (std::size_t i{0}; i < N; ++i)
    {
        for (std::size_t j{0}; i + j < N; ++j)
        {
            std::uint64_t hi{0};
            const std::uint64_t lo = intrinsics::umul128(x[i], y[j], &hi);
            unsigned char c = ref::add_limb(r[i + j], lo);
            const std::size_t next = i + j + 1;
            if (next < N)
            {
                c = ref::add_limb_carry(r[next], hi, c);
                for (std::size_t k{next + 1}; k < N && c; ++k)
                    c = ref::add_limb(r[k], std::uint64_t{c});
            }
        }
    }
    return out;
}

// ============================================================================
// Operandos
// ============================================================================
//
// xorshift64* con semilla fija: reproducible entre ejecuciones y entre
// compiladores, y sin depender de <random>, cuya distribucion no esta
// especificada de forma portable.

static std::uint64_t seed_state{0x9E3779B97F4A7C15ull};

static std::uint64_t next_u64() noexcept
{
    seed_state ^= seed_state >> 12;
    seed_state ^= seed_state << 25;
    seed_state ^= seed_state >> 27;
    return seed_state * 0x2545F4914F6CDD1Dull;
}

template <std::size_t N>
static std::vector<uint_fixed_t<N>> make_operands(std::size_t count)
{
    std::vector<uint_fixed_t<N>> v;
    v.reserve(count);
    for (std::size_t k{0}; k < count; ++k)
    {
        uint_fixed_t<N> x{};
        for (std::size_t i{0}; i < N; ++i)
            x.set_limb(i, next_u64());
        v.push_back(x);
    }
    return v;
}

// ============================================================================
// Medida
// ============================================================================

static constexpr std::size_t OPERANDS{256};
static constexpr std::size_t ROUNDS{7};
static constexpr std::size_t ITERS{400000};

template <std::size_t N, typename F>
static double measure(const std::vector<uint_fixed_t<N>> &xs, F op)
{
    uint_fixed_t<N> sink{};

    // Calentamiento: se descarta.
    for (std::size_t k{0}; k < WARMUP; ++k)
    {
        sink = op(xs[k % OPERANDS], xs[(k + 1) % OPERANDS]);
        doNotOptimize(sink);
    }

    CycleTimer t;
    for (std::size_t k{0}; k < ITERS; ++k)
    {
        sink = op(xs[k % OPERANDS], xs[(k + 1) % OPERANDS]);
        doNotOptimize(sink);
    }
    return static_cast<double>(t.elapsed_cycles()) / static_cast<double>(ITERS);
}

// `nota`: "" para los casos medidos, un texto para los que son control o
// camino especializado.
template <std::size_t N>
static void bench_one(const char *etiqueta, const char *nota)
{
    const auto xs = make_operands<N>(OPERANDS);

    double mejor_k{1e300};
    double mejor_e{1e300};

    for (std::size_t r{0}; r < ROUNDS; ++r)
    {
        // Intercaladas dentro de la ronda: el ruido cae por igual en las dos.
        const double ck =
            measure<N>(xs, [](const uint_fixed_t<N> &a, const uint_fixed_t<N> &b) { return a * b; });
        const double ce = measure<N>(xs, [](const uint_fixed_t<N> &a, const uint_fixed_t<N> &b)
                                     { return schoolbook_mul<N>(a, b); });
        if (ck < mejor_k)
            mejor_k = ck;
        if (ce < mejor_e)
            mejor_e = ce;
    }

    std::cout << "| " << std::left << std::setw(29) << etiqueta << " | " << std::right << std::fixed
              << std::setprecision(2) << std::setw(12) << mejor_k << " | " << std::setw(6)
              << (mejor_e / mejor_k) << "x   |";
    if (nota[0] != '\0')
        std::cout << "   <- " << nota;
    std::cout << "\n";

    std::cout << "| " << std::left << std::setw(29) << "   escolar O(N^2)" << " | " << std::right
              << std::fixed << std::setprecision(2) << std::setw(12) << mejor_e << " |        "
              << " |\n";
}

// ============================================================================
// Correccion antes que velocidad
// ============================================================================
//
// Un benchmark de dos implementaciones que no calculan lo mismo no mide nada.

template <std::size_t N>
static bool check_equal()
{
    const auto xs = make_operands<N>(64);
    for (std::size_t i{0}; i < xs.size(); ++i)
        for (std::size_t j{0}; j < xs.size(); ++j)
            if (xs[i] * xs[j] != schoolbook_mul<N>(xs[i], xs[j]))
                return false;
    return true;
}

int main()
{
    std::cout << "\n=== Karatsuba frente a multiplicacion escolar ===\n";
    std::cout << "operandos: " << OPERANDS << " pseudoaleatorios, " << ITERS << " iteraciones x " << ROUNDS
              << " rondas intercaladas, minimo por caso\n";

    std::cout << "\n[correccion]\n";
    const bool ok =
        check_equal<2>() && check_equal<3>() && check_equal<4>() && check_equal<8>() && check_equal<16>();
    std::cout << "  las dos implementaciones coinciden en N=2,3,4,8,16: " << (ok ? "SI" : "NO") << "\n";
    if (!ok)
    {
        std::cout << "  ABORTADO: no tiene sentido medir dos cosas que no calculan lo mismo.\n";
        return 1;
    }

    print_header("multiplicacion, ciclos por operacion");
    std::cout << "|   razon = escolar / camino de la biblioteca;  >1.00x = la biblioteca gana\n";
    print_separator();
    bench_one<4>("N=4  (256 bits)", "");
    bench_one<8>("N=8  (512 bits)", "");
    bench_one<2>("N=2  (128 bits)", "camino especializado de 128 bits");
    bench_one<16>("N=16 (1024 bits)", "CONTROL: debe salir ~1.00x");
    bench_one<3>("N=3  (192 bits)", "CONTROL: debe salir ~1.00x");
    print_footer();

    std::cout << "\nSi alguno de los dos CONTROL no sale entre 0.95x y 1.05x, la\n"
              << "implementacion de referencia no es fiel y ninguna cifra de arriba vale.\n";

    return 0;
}
