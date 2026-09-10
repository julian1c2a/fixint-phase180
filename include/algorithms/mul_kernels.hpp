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
// @file       mul_kernels.hpp
// @brief      Los nucleos de multiplicacion, sueltos y comparables entre si
// @date       2026-09-10
// =============================================================================
//
// POR QUE EXISTE ESTE FICHERO
// ---------------------------
// Hasta ahora los tres caminos de `operator*` --escolar en bucle, escolar
// desenrollado y Karatsuba-- vivian dentro de `mul_sin_marca`, en una cadena de
// `if constexpr` que elige UNO en tiempo de compilacion segun N y las macros.
//
// Eso hacia IMPOSIBLE medirlos bien. Para comparar dos caminos en la misma N
// habia que recompilar entero, y entonces no se puede cumplir la regla de
// RONDAS ENTRELAZADAS del protocolo de medicion --alternar entre las variantes
// que se comparan-- porque solo hay una variante en el binario. Y esa es justo
// la regla que protege de la deriva termica y de la posicion dentro de la
// ejecucion, que en este proyecto ya se ha comprobado que mueven la medida.
//
// Aqui cada nucleo es una funcion libre con nombre propio, y se pueden llamar
// los tres seguidos sobre la misma N en el mismo proceso.
//
// POR QUE SOBRE `std::array` Y NO SOBRE `fixed_int_t`
// ---------------------------------------------------
// Porque no hace falta el tipo para multiplicar limbos, y porque asi los
// nucleos se prueban solos, sin pasar por la clase. Es la misma separacion que
// hace GMP con su capa `mpn`: funciones sobre vectores de limbos, y los tipos
// por encima.
//
// **No hace falta declararlos `friend`.** `fixed_int_t::limbs()` y
// `limbs_ref()` ya son publicas y devuelven el array por referencia, que es el
// mismo acceso que daria la amistad. Declarar amigos obligaria a editar la
// clase por cada experimento y a quitarlos despues.
//
// QUE NO ES ESTE FICHERO
// ----------------------
// No es (todavia) la implementacion que usa `fixed_int_t`. De momento convive
// con la de la clase, y hay un test diferencial que exige que den EXACTAMENTE
// lo mismo. Unificar las dos es el paso siguiente, y va aparte para que si algo
// se rompe se sepa cual de las dos cosas fue.
//
// EL SOBRE DE INSTANCIACION
// -------------------------
// No todos los nucleos caben en toda N. Medido con clang -O2 el 10 sep 2026:
//
//   escolar en bucle        toda N; el codigo no crece con N
//   escolar desenrollado    O(N^2) EN TAMANO DE CODIGO:
//                             N= 32 ->  3,0 s y  134 KB
//                             N= 64 -> 10,4 s y  541 KB
//                             N=128 -> 37,0 s y 2250 KB
//                           Por encima de N~128 deja de ser instanciable en la
//                           practica (N=512 serian ~10 min y ~36 MB).
//   Karatsuba               solo potencias de dos, por como esta escrito hoy.
//
// Asi que la comparacion a tres bandas solo existe en N = 32, 64 y 128.
// =============================================================================

#ifndef NSTD_ALGORITHMS_MUL_KERNELS_HPP
#define NSTD_ALGORITHMS_MUL_KERNELS_HPP

#include <array>
#include <cstddef>
#include <cstdint>

#if __has_include("intrinsics/arithmetic_operations.hpp")
#include "intrinsics/arithmetic_operations.hpp"
#endif

namespace nstd
{
    namespace algorithms
    {
        /// @brief Piezas de un solo limbo. Son las mismas que usa `fixed_int_t`;
        ///        estan repetidas aqui para que la capa se sostenga sola.
        namespace detail
        {
            /// @brief Producto 64x64 -> 128. La parte baja se devuelve, la alta
            ///        va a `hi`.
            [[nodiscard]] constexpr std::uint64_t mul64(std::uint64_t a, std::uint64_t b,
                                                        std::uint64_t &hi) noexcept
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                return intrinsics::umul128(a, b, &hi);
#else
                const std::uint64_t a_lo = a & 0xFFFFFFFFULL;
                const std::uint64_t a_hi = a >> 32;
                const std::uint64_t b_lo = b & 0xFFFFFFFFULL;
                const std::uint64_t b_hi = b >> 32;
                const std::uint64_t p0 = a_lo * b_lo;
                const std::uint64_t p1 = a_lo * b_hi;
                const std::uint64_t p2 = a_hi * b_lo;
                const std::uint64_t p3 = a_hi * b_hi;
                const std::uint64_t mid = (p0 >> 32) + (p1 & 0xFFFFFFFFULL) + (p2 & 0xFFFFFFFFULL);
                hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
                return (p0 & 0xFFFFFFFFULL) | (mid << 32);
#endif
            }

            /// @brief `limb += v`, devolviendo el acarreo.
            constexpr unsigned char add_limb(std::uint64_t &limb, std::uint64_t v) noexcept
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                return intrinsics::addcarry_u64(0, limb, v, &limb);
#else
                const std::uint64_t old{limb};
                limb += v;
                return static_cast<unsigned char>(limb < old ? 1 : 0);
#endif
            }

            /// @brief `limb += v + c`, devolviendo el acarreo.
            constexpr unsigned char add_limb_carry(std::uint64_t &limb, std::uint64_t v,
                                                   unsigned char c) noexcept
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                return intrinsics::addcarry_u64(c, limb, v, &limb);
#else
                const std::uint64_t old{limb};
                limb += v + c;
                return static_cast<unsigned char>((limb < old || (c && limb == old)) ? 1 : 0);
#endif
            }
            /// @brief Producto COMPLETO MxM -> 2M, escolar. Sin truncar.
            ///
            /// Es el caso base de `kmul_full_gen`, y se distingue de
            /// `mul_escolar_bucle` en que aquel es MODULAR --tira lo que se sale de
            /// N limbos-- y este conserva los 2M.
            ///
            /// @param a Primer factor. @param b Segundo factor.
            /// @param r Destino de 2M limbos. **Se pone a cero al entrar.**
            template <std::size_t M>
            constexpr void mul_full_escolar(const std::array<std::uint64_t, M> &a,
                                            const std::array<std::uint64_t, M> &b,
                                            std::array<std::uint64_t, 2 * M> &r) noexcept
            {
                r.fill(0);
                for (std::size_t i = 0; i < M; ++i)
                {
                    std::uint64_t acarreo = 0;
                    for (std::size_t j = 0; j < M; ++j)
                    {
                        std::uint64_t alto = 0;
                        const std::uint64_t bajo = mul64(a[i], b[j], alto);
                        unsigned char c = add_limb(r[i + j], bajo);
                        alto += c; // no puede desbordar: alto <= 2^64-2
                        c = add_limb(r[i + j], acarreo);
                        alto += c;
                        acarreo = alto;
                    }
                    // El acarreo final entra en el limbo i+M, y puede propagarse.
                    std::size_t k = i + M;
                    unsigned char c = add_limb(r[k], acarreo);
                    while (c && ++k < 2 * M)
                        c = add_limb(r[k], std::uint64_t{c});
                }
            }

        } // namespace detail

        // =====================================================================
        // 1. Escolar en bucle
        // =====================================================================

        /// @brief Producto modular NxN -> N, escolar, con bucles.
        ///
        /// El unico que vale para cualquier N: su tamano de codigo no crece.
        ///
        /// @param a Primer factor, en limbos little-endian.
        /// @param b Segundo factor.
        /// @param r Destino. **Se pone a cero al entrar.**
        /// @tparam Limpiar Si el nucleo debe poner `r` a cero al entrar. **Ponerlo
        ///         a `false` cuando el destino YA es cero**, que es lo que pasa
        ///         cuando lo llama `fixed_int_t::mul_sin_marca`: alli `r{}` ya
        ///         value-inicializa, y volver a limpiarlo costaba entre un 8 y
        ///         un 23 % en las anchuras pequenas -- medido en el antes/despues
        ///         de conectar `operator*`.
        template <std::size_t N, bool Limpiar = true>
        constexpr void mul_escolar_bucle(const std::array<std::uint64_t, N> &a,
                                         const std::array<std::uint64_t, N> &b,
                                         std::array<std::uint64_t, N> &r) noexcept
        {
            if constexpr (Limpiar)
                r.fill(0);
            for (std::size_t i{0}; i < N; ++i)
            {
                for (std::size_t j{0}; i + j < N; ++j)
                {
                    std::uint64_t hi{0};
                    const std::uint64_t lo = detail::mul64(a[i], b[j], hi);
                    unsigned char c = detail::add_limb(r[i + j], lo);
                    const std::size_t next = i + j + 1;
                    if (next < N)
                    {
                        c = detail::add_limb_carry(r[next], hi, c);
                        for (std::size_t k{next + 1}; k < N && c; ++k)
                            c = detail::add_limb(r[k], std::uint64_t{c});
                    }
                }
            }
        }

        // =====================================================================
        // 2. Escolar desenrollado por recursion de plantilla
        // =====================================================================

        namespace detail
        {
            /// @brief Propaga un acarreo desde el limbo K hacia arriba.
            template <std::size_t K, std::size_t N>
            constexpr void propaga_desde(std::array<std::uint64_t, N> &r, unsigned char c) noexcept
            {
                if constexpr (K < N)
                {
                    if (c)
                        propaga_desde<K + 1, N>(r, add_limb(r[K], std::uint64_t{c}));
                }
            }

            /// @brief Una fila del escolar: los productos x[I]*y[J] con J creciente.
            template <std::size_t I, std::size_t J, std::size_t N>
            constexpr void fila_desenrollada(std::array<std::uint64_t, N> &r,
                                             const std::array<std::uint64_t, N> &x,
                                             const std::array<std::uint64_t, N> &y) noexcept
            {
                if constexpr (I + J < N)
                {
                    std::uint64_t hi{0};
                    const std::uint64_t lo = mul64(x[I], y[J], hi);
                    unsigned char c = add_limb(r[I + J], lo);
                    if constexpr (I + J + 1 < N)
                    {
                        c = add_limb_carry(r[I + J + 1], hi, c);
                        propaga_desde<I + J + 2, N>(r, c);
                    }
                    fila_desenrollada<I, J + 1, N>(r, x, y);
                }
            }

            /// @brief Todas las filas.
            template <std::size_t I, std::size_t N>
            constexpr void filas_desenrolladas(std::array<std::uint64_t, N> &r,
                                               const std::array<std::uint64_t, N> &x,
                                               const std::array<std::uint64_t, N> &y) noexcept
            {
                if constexpr (I < N)
                {
                    fila_desenrollada<I, 0, N>(r, x, y);
                    filas_desenrolladas<I + 1, N>(r, x, y);
                }
            }
        } // namespace detail

        /// @brief Producto modular NxN -> N, escolar, DESENROLLADO por
        ///        recursion de plantilla.
        ///
        /// @warning Su tamano de codigo es **O(N^2)**. Ver el sobre de
        ///          instanciacion en la cabecera del fichero: por encima de
        ///          N~128 deja de ser practico.
        ///
        /// @param a Primer factor. @param b Segundo factor.
        /// @param r Destino. **Se pone a cero al entrar.**
        /// @tparam Limpiar Si el nucleo debe poner `r` a cero al entrar. **Ponerlo
        ///         a `false` cuando el destino YA es cero**, que es lo que pasa
        ///         cuando lo llama `fixed_int_t::mul_sin_marca`: alli `r{}` ya
        ///         value-inicializa, y volver a limpiarlo costaba entre un 8 y
        ///         un 23 % en las anchuras pequenas -- medido en el antes/despues
        ///         de conectar `operator*`.
        template <std::size_t N, bool Limpiar = true>
        constexpr void mul_escolar_desenrollado(const std::array<std::uint64_t, N> &a,
                                                const std::array<std::uint64_t, N> &b,
                                                std::array<std::uint64_t, N> &r) noexcept
        {
            if constexpr (Limpiar)
                r.fill(0);
            detail::filas_desenrolladas<0, N>(r, a, b);
        }

        // =====================================================================
        // 3. Karatsuba
        // =====================================================================

        template <std::size_t M>
        [[nodiscard]] inline std::array<std::uint64_t, 2 * M>
        kmul_full(const std::array<std::uint64_t, M> &a, const std::array<std::uint64_t, M> &b) noexcept
        {
            std::array<std::uint64_t, 2 * M> r{};

            if constexpr (M == 1)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                r[0] = intrinsics::umul128(a[0], b[0], &r[1]);
#else
                const std::uint64_t al = a[0] & 0xFFFF'FFFFull;
                const std::uint64_t ah = a[0] >> 32;
                const std::uint64_t bl = b[0] & 0xFFFF'FFFFull;
                const std::uint64_t bh = b[0] >> 32;
                const std::uint64_t p0 = al * bl, p1 = al * bh;
                const std::uint64_t p2 = ah * bl, p3 = ah * bh;
                const std::uint64_t mid = (p0 >> 32) + (p1 & 0xFFFF'FFFFull) + (p2 & 0xFFFF'FFFFull);
                r[0] = (p0 & 0xFFFF'FFFFull) | (mid << 32);
                r[1] = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
#endif
                return r;
            }
            else
            {
                static_assert(M % 2 == 0, "kmul_full: M must be even");
                constexpr std::size_t HH = M / 2;

                // ── Split into low/high halves ────────────────────────────────────
                std::array<std::uint64_t, HH> a_lo{}, a_hi{}, b_lo{}, b_hi{};
                for (std::size_t i = 0; i < HH; ++i)
                {
                    a_lo[i] = a[i];
                    a_hi[i] = a[HH + i];
                    b_lo[i] = b[i];
                    b_hi[i] = b[HH + i];
                }

                // ── z0 = a_lo·b_lo,  z2 = a_hi·b_hi  (each 2HH limbs) ───────────
                const auto z0 = kmul_full<HH>(a_lo, b_lo);
                const auto z2 = kmul_full<HH>(a_hi, b_hi);

                // ── sum_a = a_lo + a_hi,  sum_b = b_lo + b_hi  (+carry ca, cb) ───
                std::array<std::uint64_t, HH> sum_a{}, sum_b{};
                unsigned char ca = 0, cb = 0;
                for (std::size_t i = 0; i < HH; ++i)
                {
                    sum_a[i] = a_hi[i];
                    ca = detail::add_limb_carry(sum_a[i], a_lo[i], ca);
                    sum_b[i] = b_hi[i];
                    cb = detail::add_limb_carry(sum_b[i], b_lo[i], cb);
                }

                // ── p = (sum_a + ca·B^HH) · (sum_b + cb·B^HH)  (2HH+1 limbs) ───
                // = sum_a·sum_b + ca·sum_b·B^HH + cb·sum_a·B^HH + ca·cb·B^(2HH)
                std::array<std::uint64_t, 2 * HH + 1> p{};
                {
                    const auto pp = kmul_full<HH>(sum_a, sum_b);
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                        p[i] = pp[i];
                }
                if (ca)
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < HH; ++i)
                        c = detail::add_limb_carry(p[HH + i], sum_b[i], c);
                    p[2 * HH] += c;
                }
                if (cb)
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < HH; ++i)
                        c = detail::add_limb_carry(p[HH + i], sum_a[i], c);
                    p[2 * HH] += c;
                }
                if (ca & cb)
                    ++p[2 * HH];

                // ── z1 = p − z0 − z2  (guaranteed ≥ 0, fits in 2HH+1 limbs) ─────
                std::array<std::uint64_t, 2 * HH + 1> z1 = p;
                {
                    unsigned char borrow = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                    {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                        borrow = intrinsics::subborrow_u64(borrow, z1[i], z0[i], &z1[i]);
#else
                        const std::uint64_t av = z1[i];
                        z1[i] = av - z0[i] - borrow;
                        borrow = static_cast<unsigned char>((av < z0[i]) || (borrow && av == z0[i]) ? 1 : 0);
#endif
                    }
                    z1[2 * HH] -= borrow;
                }
                {
                    unsigned char borrow = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                    {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                        borrow = intrinsics::subborrow_u64(borrow, z1[i], z2[i], &z1[i]);
#else
                        const std::uint64_t av = z1[i];
                        z1[i] = av - z2[i] - borrow;
                        borrow = static_cast<unsigned char>((av < z2[i]) || (borrow && av == z2[i]) ? 1 : 0);
#endif
                    }
                    z1[2 * HH] -= borrow;
                }

                // ── Combine: r = z0 + z1·B^HH + z2·B^(2HH) ─────────────────────
                for (std::size_t i = 0; i < 2 * HH; ++i)
                    r[i] = z0[i];
                {
                    unsigned char c = 0;
                    std::size_t i = 0;
                    for (; i <= 2 * HH; ++i)
                        c = detail::add_limb_carry(r[HH + i], z1[i], c);
                    for (; c && HH + i < 2 * M; ++i)
                        c = detail::add_limb(r[HH + i], std::uint64_t{1});
                }
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                        c = detail::add_limb_carry(r[2 * HH + i], z2[i], c);
                }

                return r;
            }
        }
        /// @brief Producto COMPLETO MxM -> 2M por Karatsuba, para **cualquier M**.
        ///
        /// Es el nucleo del reparto equilibrado. Frente a `kmul_full`, que exige
        /// potencia de dos, este parte en dos mitades iguales cuando M es par y
        /// **rellena con un solo limbo** cuando es impar.
        ///
        /// @note El relleno es de UN limbo, no hasta la potencia de dos
        ///       siguiente. Rellenar hasta la potencia de dos se midio y sale
        ///       peor: para N=48 daria el coste de N=64, que es mayor que el del
        ///       escolar. Un limbo cuesta un factor `(1 + 1,585/M)`, que se
        ///       desvanece al crecer M.
        ///
        /// @tparam M Numero de limbos de cada factor. Cualquiera >= 1.
        /// @tparam Base Anchura a partir de la cual se corta la recursion y se
        ///         usa el escolar completo. Es un umbral MEDIBLE, no una
        ///         constante magica.
        /// @param a Primer factor. @param b Segundo factor.
        /// @return El producto exacto, 2M limbos.
        template <std::size_t M, std::size_t Base>
        [[nodiscard]] inline std::array<std::uint64_t, 2 * M>
        kmul_full_gen(const std::array<std::uint64_t, M> &a, const std::array<std::uint64_t, M> &b) noexcept
        {
            std::array<std::uint64_t, 2 * M> r{};

            if constexpr (M <= Base)
            {
                // Caso base: escolar completo. El de `kmul_full` era M==1; aqui
                // se corta antes porque por debajo de una decena de limbos la
                // recursion cuesta mas de lo que ahorra -- medido.
                detail::mul_full_escolar<M>(a, b, r);
                return r;
            }
            else if constexpr (M % 2 == 1)
            {
                // ANCHURA IMPAR. Karatsuba parte por la mitad, y una mitad de un
                // impar no existe. Se rellena con UN limbo de ceros --no se
                // redondea a la potencia de dos siguiente, que era el error del
                // reparto descartado-- y se recorta al salir. El coste extra es
                // el de un limbo, no el de duplicar la anchura.
                std::array<std::uint64_t, M + 1> ap{}, bp{};
                for (std::size_t i = 0; i < M; ++i)
                {
                    ap[i] = a[i];
                    bp[i] = b[i];
                }
                const auto rp = kmul_full_gen<M + 1, Base>(ap, bp);
                for (std::size_t i = 0; i < 2 * M; ++i)
                    r[i] = rp[i];
                return r;
            }
            else if constexpr (false)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                r[0] = intrinsics::umul128(a[0], b[0], &r[1]);
#else
                const std::uint64_t al = a[0] & 0xFFFF'FFFFull;
                const std::uint64_t ah = a[0] >> 32;
                const std::uint64_t bl = b[0] & 0xFFFF'FFFFull;
                const std::uint64_t bh = b[0] >> 32;
                const std::uint64_t p0 = al * bl, p1 = al * bh;
                const std::uint64_t p2 = ah * bl, p3 = ah * bh;
                const std::uint64_t mid = (p0 >> 32) + (p1 & 0xFFFF'FFFFull) + (p2 & 0xFFFF'FFFFull);
                r[0] = (p0 & 0xFFFF'FFFFull) | (mid << 32);
                r[1] = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
#endif
                return r;
            }
            else
            {
                constexpr std::size_t HH = M / 2;

                // ── Split into low/high halves ────────────────────────────────────
                std::array<std::uint64_t, HH> a_lo{}, a_hi{}, b_lo{}, b_hi{};
                for (std::size_t i = 0; i < HH; ++i)
                {
                    a_lo[i] = a[i];
                    a_hi[i] = a[HH + i];
                    b_lo[i] = b[i];
                    b_hi[i] = b[HH + i];
                }

                // ── z0 = a_lo·b_lo,  z2 = a_hi·b_hi  (each 2HH limbs) ───────────
                const auto z0 = kmul_full_gen<HH, Base>(a_lo, b_lo);
                const auto z2 = kmul_full_gen<HH, Base>(a_hi, b_hi);

                // ── sum_a = a_lo + a_hi,  sum_b = b_lo + b_hi  (+carry ca, cb) ───
                std::array<std::uint64_t, HH> sum_a{}, sum_b{};
                unsigned char ca = 0, cb = 0;
                for (std::size_t i = 0; i < HH; ++i)
                {
                    sum_a[i] = a_hi[i];
                    ca = detail::add_limb_carry(sum_a[i], a_lo[i], ca);
                    sum_b[i] = b_hi[i];
                    cb = detail::add_limb_carry(sum_b[i], b_lo[i], cb);
                }

                // ── p = (sum_a + ca·B^HH) · (sum_b + cb·B^HH)  (2HH+1 limbs) ───
                // = sum_a·sum_b + ca·sum_b·B^HH + cb·sum_a·B^HH + ca·cb·B^(2HH)
                std::array<std::uint64_t, 2 * HH + 1> p{};
                {
                    const auto pp = kmul_full_gen<HH, Base>(sum_a, sum_b);
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                        p[i] = pp[i];
                }
                if (ca)
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < HH; ++i)
                        c = detail::add_limb_carry(p[HH + i], sum_b[i], c);
                    p[2 * HH] += c;
                }
                if (cb)
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < HH; ++i)
                        c = detail::add_limb_carry(p[HH + i], sum_a[i], c);
                    p[2 * HH] += c;
                }
                if (ca & cb)
                    ++p[2 * HH];

                // ── z1 = p − z0 − z2  (guaranteed ≥ 0, fits in 2HH+1 limbs) ─────
                std::array<std::uint64_t, 2 * HH + 1> z1 = p;
                {
                    unsigned char borrow = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                    {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                        borrow = intrinsics::subborrow_u64(borrow, z1[i], z0[i], &z1[i]);
#else
                        const std::uint64_t av = z1[i];
                        z1[i] = av - z0[i] - borrow;
                        borrow = static_cast<unsigned char>((av < z0[i]) || (borrow && av == z0[i]) ? 1 : 0);
#endif
                    }
                    z1[2 * HH] -= borrow;
                }
                {
                    unsigned char borrow = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                    {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                        borrow = intrinsics::subborrow_u64(borrow, z1[i], z2[i], &z1[i]);
#else
                        const std::uint64_t av = z1[i];
                        z1[i] = av - z2[i] - borrow;
                        borrow = static_cast<unsigned char>((av < z2[i]) || (borrow && av == z2[i]) ? 1 : 0);
#endif
                    }
                    z1[2 * HH] -= borrow;
                }

                // ── Combine: r = z0 + z1·B^HH + z2·B^(2HH) ─────────────────────
                for (std::size_t i = 0; i < 2 * HH; ++i)
                    r[i] = z0[i];
                {
                    unsigned char c = 0;
                    std::size_t i = 0;
                    for (; i <= 2 * HH; ++i)
                        c = detail::add_limb_carry(r[HH + i], z1[i], c);
                    for (; c && HH + i < 2 * M; ++i)
                        c = detail::add_limb(r[HH + i], std::uint64_t{1});
                }
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                        c = detail::add_limb_carry(r[2 * HH + i], z2[i], c);
                }

                return r;
            }
        }

        /// @brief Producto modular NxN -> N por Karatsuba.
        ///
        /// @pre **N tiene que ser potencia de dos y par.** Es la limitacion del
        ///      reparto de hoy, y es justo lo que el reparto equilibrado de
        ///      [PLAN_MULTIPLICACION] viene a quitar.
        ///
        /// La mitad baja sale de `kmul_full<N/2>`, que da el producto completo
        /// de 2*(N/2) = N limbos; los terminos del medio se calculan aparte y se
        /// suman desplazados N/2 limbos. Lo que se sale por arriba se descarta,
        /// que es lo que significa "modular".
        ///
        /// @param a Primer factor. @param b Segundo factor.
        /// @param r Destino. **Se pone a cero al entrar.**
        /// @tparam Limpiar Si el nucleo debe poner `r` a cero al entrar. **Ponerlo
        ///         a `false` cuando el destino YA es cero**, que es lo que pasa
        ///         cuando lo llama `fixed_int_t::mul_sin_marca`: alli `r{}` ya
        ///         value-inicializa, y volver a limpiarlo costaba entre un 8 y
        ///         un 23 % en las anchuras pequenas -- medido en el antes/despues
        ///         de conectar `operator*`.
        /// @param medio Como calcular los dos productos del medio, de N/2
        ///        limbos cada uno. Se pasa como parametro a proposito: es
        ///        **otra decision de reparto**, y dejarla dentro escondia que
        ///        Karatsuba a N=32 usa hoy el desenrollado a N=16 por debajo.
        template <std::size_t N, typename Medio>
        void mul_karatsuba_pot2(const std::array<std::uint64_t, N> &a, const std::array<std::uint64_t, N> &b,
                                std::array<std::uint64_t, N> &r, Medio medio) noexcept
        {
            static_assert(N >= 2 && (N & (N - 1)) == 0,
                          "mul_karatsuba_pot2: N tiene que ser potencia de dos. Es la "
                          "limitacion del reparto de hoy, no de Karatsuba.");
            constexpr std::size_t HH = N / 2;

            std::array<std::uint64_t, HH> a_lo{}, a_hi{}, b_lo{}, b_hi{};
            for (std::size_t i = 0; i < HH; ++i)
            {
                a_lo[i] = a[i];
                a_hi[i] = a[HH + i];
                b_lo[i] = b[i];
                b_hi[i] = b[HH + i];
            }

            const auto z0 = kmul_full<HH>(a_lo, b_lo);

            // mid = a_lo*b_hi + a_hi*b_lo, en HH limbos y modular: lo que se
            // desborda de HH limbos acabaria por encima de N y se tira igual.
            std::array<std::uint64_t, HH> m1{}, m2{};
            medio(a_lo, b_hi, m1);
            medio(a_hi, b_lo, m2);
            unsigned char cm = 0;
            for (std::size_t i = 0; i < HH; ++i)
                cm = detail::add_limb_carry(m1[i], m2[i], cm);

            for (std::size_t i = 0; i < N; ++i)
                r[i] = z0[i];
            unsigned char c = 0;
            for (std::size_t i = 0; i < HH; ++i)
                c = detail::add_limb_carry(r[HH + i], m1[i], c);
        }

        /// @brief Escolar para los terminos del medio, desenrollado hasta
        ///        `TopeDesenrollado` y en bucle por encima.
        ///
        /// @tparam TopeDesenrollado Hasta donde se desenrolla. **Es un parametro
        ///         de plantilla y no la macro `NSTD_DESENROLLA_MAX` a
        ///         proposito**: esta capa no debe depender de una macro que
        ///         define otro fichero, y ademas el barrido de umbrales
        ///         necesita cambiarlo sin recompilar el mundo.
        template <std::size_t TopeDesenrollado>
        struct medio_escolar
        {
            template <std::size_t H>
            void operator()(const std::array<std::uint64_t, H> &x, const std::array<std::uint64_t, H> &y,
                            std::array<std::uint64_t, H> &z) const noexcept
            {
                if constexpr (H <= TopeDesenrollado)
                    mul_escolar_desenrollado<H>(x, y, z);
                else
                    mul_escolar_bucle<H>(x, y, z);
            }
        };

        /// @brief Karatsuba por debajo, para el termino del medio. Solo vale si
        ///        la mitad sigue siendo potencia de dos, que lo es por
        ///        construccion.
        template <std::size_t TopeDesenrollado, std::size_t MinKaratsuba>
        struct medio_karatsuba
        {
            template <std::size_t H>
            void operator()(const std::array<std::uint64_t, H> &x, const std::array<std::uint64_t, H> &y,
                            std::array<std::uint64_t, H> &z) const noexcept;
        };

        // Declaracion adelantada: `medio_reparto` necesita al equilibrado, y el
        // equilibrado necesita un `medio`.
        template <std::size_t N, std::size_t Base = 8, typename Medio = medio_escolar<26>>
        void mul_karatsuba_equilibrado(const std::array<std::uint64_t, N> &a,
                                       const std::array<std::uint64_t, N> &b, std::array<std::uint64_t, N> &r,
                                       Medio medio = Medio{}) noexcept;

        /// @brief El termino del medio vuelve al REPARTO COMPLETO, no al escolar.
        ///
        /// Es la estrategia correcta y la que usaba el Karatsuba viejo sin
        /// decirlo: sus terminos del medio salian del `operator*` de media
        /// anchura, que vuelve a repartir. Con `medio_escolar` a secas, un
        /// producto del medio de 32 limbos cae al bucle aunque Karatsuba le gane
        /// 2,4x -- medido: hacerlo asi dejaba N=64 un 11 % PEOR que antes.
        ///
        /// @tparam MinKaratsuba Desde donde el medio vuelve a usar Karatsuba.
        /// @tparam TopeDesenrollado Hasta donde desenrolla el escolar.
        template <std::size_t TopeDesenrollado, std::size_t MinKaratsuba>
        struct medio_reparto
        {
            template <std::size_t H>
            void operator()(const std::array<std::uint64_t, H> &x, const std::array<std::uint64_t, H> &y,
                            std::array<std::uint64_t, H> &z) const noexcept
            {
                if constexpr (H >= MinKaratsuba && H >= 2)
                    mul_karatsuba_equilibrado<H, 8, medio_reparto<TopeDesenrollado, MinKaratsuba>>(
                        x, y, z, medio_reparto<TopeDesenrollado, MinKaratsuba>{});
                else if constexpr (H <= TopeDesenrollado)
                    mul_escolar_desenrollado<H>(x, y, z);
                else
                    mul_escolar_bucle<H>(x, y, z);
            }
        };

        /// @brief Lo que hace `fixed_int_t` hoy con los terminos del medio: el
        ///        `operator*` de media anchura, que con los valores por defecto
        ///        de las macros acaba en el escolar desenrollado hasta 20.
        ///
        /// @note Se le da nombre porque **es una decision de reparto mas**, y
        ///       tenerla escondida dentro de Karatsuba ocultaba que a N=32 el
        ///       camino real es «kmul_full<16> mas desenrollado a 16».
        using medio_como_hoy = medio_escolar<20>;

        /// @brief Producto modular NxN -> N por Karatsuba, **para cualquier N**.
        ///
        /// Es lo que el reparto de hoy no puede hacer: `mul_karatsuba_pot2`
        /// exige que N sea potencia de dos, y por eso toda anchura mayor que el
        /// tope de desenrollado que no lo sea cae al bucle escolar y cuesta 3-4x
        /// por limbo. Ver el acantilado en docs/PERFORMANCE.md.
        ///
        /// La mitad baja sale del producto COMPLETO de N/2 limbos, que da
        /// exactamente N; los dos terminos del medio son productos modulares de
        /// N/2 y se suman desplazados N/2 limbos. Lo que se sale por arriba se
        /// descarta.
        ///
        /// Con N impar se rellena con **un** limbo y se recorta: el limbo alto de
        /// los dos factores es cero, asi que los N limbos bajos del producto en
        /// N+1 son los N limbos bajos del producto verdadero.
        ///
        /// @tparam N Numero de limbos. Cualquiera >= 2.
        /// @tparam Base Corte de la recursion; ver `kmul_full_gen`.
        /// @param a Primer factor. @param b Segundo factor.
        /// @param r Destino. **Se pone a cero al entrar.**
        /// @tparam Limpiar Si el nucleo debe poner `r` a cero al entrar. **Ponerlo
        ///         a `false` cuando el destino YA es cero**, que es lo que pasa
        ///         cuando lo llama `fixed_int_t::mul_sin_marca`: alli `r{}` ya
        ///         value-inicializa, y volver a limpiarlo costaba entre un 8 y
        ///         un 23 % en las anchuras pequenas -- medido en el antes/despues
        ///         de conectar `operator*`.
        /// @param medio Como calcular los dos productos del medio. Es otra
        ///        decision de reparto, y se pasa a proposito en vez de
        ///        esconderla dentro.
        template <std::size_t N, std::size_t Base, typename Medio>
        void mul_karatsuba_equilibrado(const std::array<std::uint64_t, N> &a,
                                       const std::array<std::uint64_t, N> &b, std::array<std::uint64_t, N> &r,
                                       Medio medio) noexcept
        {
            static_assert(N >= 2, "mul_karatsuba_equilibrado: hacen falta al menos dos limbos");

            if constexpr (N % 2 == 1)
            {
                std::array<std::uint64_t, N + 1> ap{}, bp{}, rp{};
                for (std::size_t i = 0; i < N; ++i)
                {
                    ap[i] = a[i];
                    bp[i] = b[i];
                }
                mul_karatsuba_equilibrado<N + 1, Base>(ap, bp, rp, medio);
                for (std::size_t i = 0; i < N; ++i)
                    r[i] = rp[i];
            }
            else
            {
                constexpr std::size_t HH = N / 2;

                std::array<std::uint64_t, HH> a_lo{}, a_hi{}, b_lo{}, b_hi{};
                for (std::size_t i = 0; i < HH; ++i)
                {
                    a_lo[i] = a[i];
                    a_hi[i] = a[HH + i];
                    b_lo[i] = b[i];
                    b_hi[i] = b[HH + i];
                }

                const auto z0 = kmul_full_gen<HH, Base>(a_lo, b_lo);

                std::array<std::uint64_t, HH> m1{}, m2{};
                medio(a_lo, b_hi, m1);
                medio(a_hi, b_lo, m2);
                unsigned char cm = 0;
                for (std::size_t i = 0; i < HH; ++i)
                    cm = detail::add_limb_carry(m1[i], m2[i], cm);

                for (std::size_t i = 0; i < N; ++i)
                    r[i] = z0[i];
                unsigned char c = 0;
                for (std::size_t i = 0; i < HH; ++i)
                    c = detail::add_limb_carry(r[HH + i], m1[i], c);
            }
        }

        template <std::size_t TopeDesenrollado, std::size_t MinKaratsuba>
        template <std::size_t H>
        void medio_karatsuba<TopeDesenrollado, MinKaratsuba>::operator()(
            const std::array<std::uint64_t, H> &x, const std::array<std::uint64_t, H> &y,
            std::array<std::uint64_t, H> &z) const noexcept
        {
            if constexpr (H >= MinKaratsuba && H >= 2 && (H & (H - 1)) == 0)
                mul_karatsuba_pot2<H>(x, y, z, medio_karatsuba<TopeDesenrollado, MinKaratsuba>{});
            else if constexpr (H <= TopeDesenrollado)
                mul_escolar_desenrollado<H>(x, y, z);
            else
                mul_escolar_bucle<H>(x, y, z);
        }

    } // namespace algorithms
} // namespace nstd

#endif // NSTD_ALGORITHMS_MUL_KERNELS_HPP
