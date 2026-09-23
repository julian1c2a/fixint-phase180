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

/// @def NSTD_TOOM3_MIN
/// @brief Anchura desde la que el producto COMPLETO ENTRA en Toom-3.
///
/// Toom-3 hace cinco productos de M/3 donde Karatsuba hace tres de M/2:
/// exponente log5/log3 = 1,465 frente a log3/log2 = 1,585. Gana por arriba y
/// pierde por abajo.
///
/// **Medido el 17 sep 2026 con clang**, productos completos M x M -> 2M, cinco
/// variantes entrelazadas, minimo de 20 repeticiones, dos vueltas. Razones
/// Karatsuba/Toom-3 segun donde se corte la recursion:
///
///     M        Toom>=96   Toom>=256   Toom>=512   Toom>=1024
///     512      0,92 0,95  0,94 0,98   0,96 0,97   (no entra)
///     1024     1,05 1,00  1,02 1,02   1,00 0,98   0,98 1,00
///     2048     1,13 1,10  1,07 1,03   1,05 1,01   1,02 1,00
///     4096     1,12 1,13  1,17 1,20   1,04 1,07   1,08 1,06
///
/// De ahi salen los DOS umbrales, y por que son dos:
///
///   - En M=512 Toom-3 PIERDE con cualquier corte. Entrar por debajo de 1024
///     meteria una regresion del 5-8 % en las anchuras medias.
///   - Pero la ganancia viene de REPETIR el reparto: con la recursion cortada en
///     1024, M=4096 solo da 1,06-1,08x; bajandola a 96 sube a 1,12-1,13x.
///
/// Con un solo umbral no se pueden tener las dos cosas, porque el mismo M
/// aparece como producto de arriba y como subproducto de dentro. Por eso este
/// fija la ENTRADA y `NSTD_TOOM3_REC` fija la RECURSION.
///
/// @note La casilla M=512 con corte en 1024 sale a 0,99x-1,01x, o sea igual que
///       Karatsuba puro: es la comprobacion de que el banco mide lo que dice.
///
/// @note Esto es UNA maquina, como pasa con `NSTD_KARATSUBA_MIN`. GMP publica
///       para su `MUL_TOOM33_THRESHOLD` un rango de 38 a 122 limbos entre
///       modelos; el nuestro sale mucho mas alto porque nuestro Karatsuba y
///       nuestro Toom-3 no estan optimizados al mismo nivel que los suyos.
#ifndef NSTD_TOOM3_MIN
#define NSTD_TOOM3_MIN 1024
#endif

/// @def NSTD_TOOM3_REC
/// @brief Anchura hasta la que se sigue repartiendo en tres UNA VEZ DENTRO.
///
/// Es mas bajo que `NSTD_TOOM3_MIN` a proposito: entrar en Toom-3 con 512
/// limbos pierde, pero un subproducto de 512 limbos DENTRO de un producto de
/// 4096 sale mejor con Toom-3 que con Karatsuba. La explicacion mas probable es
/// la cache: a esa altura ya no queda nada util en L2, y los subproblemas de
/// Toom-3 son de M/3 mientras que los de Karatsuba son de M/2.
///
/// 96 y no 256: el 256 mide mejor en M=4096 concreto (1,17-1,20x frente a
/// 1,12-1,13x), pero el 96 gana en M=2048 (1,10-1,13x frente a 1,03-1,07x) y
/// gana de media y en el peor caso. Las dos opciones reproducen en las dos
/// vueltas, asi que la diferencia es real y no ruido: es que el optimo se mueve
/// con M y hay que elegir uno.
#ifndef NSTD_TOOM3_REC
#define NSTD_TOOM3_REC 96
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

            // =================================================================
            // Aritmetica de ANILLO, para Toom-3
            // =================================================================
            //
            // Toom-3 evalua en x = -1, asi que trabaja con valores NEGATIVOS
            // sobre arrays de limbos sin signo. En vez de arrastrar un bit de
            // signo por todo el algoritmo, se trabaja en complemento a dos
            // dentro de Z/2^(64W): sumar y restar son entonces las operaciones
            // de siempre, sin ramas.
            //
            // Lo que NO se salva solo es el producto de doble anchura; eso se
            // corrige en `toom3_full`, donde se explica.

            /// @brief `limbo -= v + p`, devolviendo el prestamo.
            constexpr unsigned char sub_limb_borrow(std::uint64_t &limbo, std::uint64_t v,
                                                    unsigned char p) noexcept
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                return intrinsics::subborrow_u64(p, limbo, v, &limbo);
#else
                const std::uint64_t viejo{limbo};
                limbo = viejo - v - p;
                return static_cast<unsigned char>((viejo < v || (p && viejo == v)) ? 1 : 0);
#endif
            }

            /// @brief `dst += src` en el anillo de W limbos. Lo que se sale, se tira.
            template <std::size_t W>
            constexpr void suma_anillo(std::array<std::uint64_t, W> &dst,
                                       const std::array<std::uint64_t, W> &src) noexcept
            {
                unsigned char c = 0;
                for (std::size_t i = 0; i < W; ++i)
                    c = add_limb_carry(dst[i], src[i], c);
            }

            /// @brief `dst -= src` en el anillo de W limbos.
            template <std::size_t W>
            constexpr void resta_anillo(std::array<std::uint64_t, W> &dst,
                                        const std::array<std::uint64_t, W> &src) noexcept
            {
                unsigned char p = 0;
                for (std::size_t i = 0; i < W; ++i)
                    p = sub_limb_borrow(dst[i], src[i], p);
            }

            /// @brief `x <<= s` con s < 64, en el anillo de W limbos.
            template <std::size_t W>
            constexpr void desplaza_izq(std::array<std::uint64_t, W> &x, unsigned s) noexcept
            {
                if (s == 0)
                    return;
                for (std::size_t i = W; i-- > 1;)
                    x[i] = (x[i] << s) | (x[i - 1] >> (64 - s));
                x[0] <<= s;
            }

            /// @brief `x /= 2`, exacta y CON SIGNO: desplazamiento aritmetico.
            ///
            /// Tiene que ser aritmetico. Con desplazamiento logico un valor
            /// negativo par da un positivo enorme, y el error no aparece hasta
            /// que el punto x = -1 sale negativo de verdad -- que con operandos
            /// al azar uniformes pasa pocas veces.
            template <std::size_t W>
            constexpr void mitad_exacta(std::array<std::uint64_t, W> &x) noexcept
            {
                const std::uint64_t signo = x[W - 1] >> 63;
                for (std::size_t i = 0; i + 1 < W; ++i)
                    x[i] = (x[i] >> 1) | (x[i + 1] << 63);
                x[W - 1] = (x[W - 1] >> 1) | (signo << 63);
            }

            /// El inverso de 3 modulo **2^64**: 3 * 0xAAAAAAAAAAAAAAAB = 2^65 + 1.
            inline constexpr std::uint64_t INV3 = 0xAAAA'AAAA'AAAA'AAABull;

            /// @brief `x /= 3`, exacta, en el anillo de W limbos.
            ///
            /// NO VALE MULTIPLICAR POR `INV3` LIMBO A LIMBO, y es un error que
            /// parece razonable: `INV3` invierte a 3 modulo 2^64, no modulo
            /// 2^(64W). El producto `3 * INV3` vale 2^65 + 1, que truncado a un
            /// limbo es 1 pero sobre el anillo entero deja 2^65 de residuo, y
            /// ese residuo contamina los limbos altos. Comprobado el 16 sep 2026
            /// con M=3 y todo unos: 6q^2 daba [2, 0, ...fa, 3] en vez de
            /// [2, ...fc, 1, 0].
            ///
            /// Lo correcto es la cadena de division exacta de Jebelean, que
            /// cuesta lo mismo --una multiplicacion por limbo-- y si calcula
            /// `x * 3^-1` en todo el anillo: en cada paso elige el limbo del
            /// cociente que anula el limbo actual y pasa al siguiente lo que se
            /// lleva.
            ///
            /// Vale tambien para valores negativos en complemento a dos. No
            /// porque su representacion sea divisible por 3 --no lo es, ya que
            /// 2^(64W) = 1 mod 3-- sino porque lo que calcula la cadena es el
            /// producto por el inverso en el anillo, y ahi 3 es una unidad.
            template <std::size_t W>
            constexpr void entre_tres(std::array<std::uint64_t, W> &x) noexcept
            {
                std::uint64_t acarreo = 0;
                for (std::size_t i = 0; i < W; ++i)
                {
                    const std::uint64_t l = x[i];
                    const std::uint64_t d = l - acarreo;
                    const std::uint64_t prestamo = (d > l) ? 1ull : 0ull;
                    const std::uint64_t q = d * INV3;
                    x[i] = q;
                    std::uint64_t alto = 0;
                    (void)mul64(q, 3ull, alto); // lo que q*3 se lleva al siguiente
                    acarreo = prestamo + alto;  // <= 3: no desborda
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
        // Declaracion adelantada: por encima del umbral el producto completo se
        // va a Toom-3, y Toom-3 vuelve a llamar aqui para sus cinco
        // subproductos. La recursion es mutua.
        template <std::size_t M, std::size_t Base, std::size_t ToomMin = NSTD_TOOM3_REC>
        [[nodiscard]] inline std::array<std::uint64_t, 2 * M>
        toom3_full(const std::array<std::uint64_t, M> &a, const std::array<std::uint64_t, M> &b) noexcept;

        /// @tparam M Numero de limbos de cada factor. Cualquiera >= 1.
        /// @tparam Base Anchura a partir de la cual se corta la recursion y se
        ///         usa el escolar completo. Es un umbral MEDIBLE, no una
        ///         constante magica.
        /// @tparam ToomMin Anchura a partir de la cual se usa Toom-3 en vez de
        ///         Karatsuba. Por defecto `NSTD_TOOM3_MIN`, que es el umbral de
        ///         ENTRADA; cuando la llamada viene de dentro de `toom3_full`
        ///         vale `NSTD_TOOM3_REC`, que es mas bajo. Las dos macros
        ///         estan documentadas mas arriba, cada una con su medida.
        /// @param a Primer factor. @param b Segundo factor.
        /// @return El producto exacto, 2M limbos.
        template <std::size_t M, std::size_t Base, std::size_t ToomMin = NSTD_TOOM3_MIN>
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
            else if constexpr (M >= ToomMin && M >= 3)
            {
                // TOOM-3. Cinco productos de M/3 donde Karatsuba hace tres de
                // M/2. Va ANTES de la rama de impares a proposito: Toom-3 parte
                // en tercios y no necesita que M sea par, asi que rellenar un
                // limbo antes de llegar aqui seria trabajo tirado.
                //
                // A partir de aqui el umbral pasa a ser `NSTD_TOOM3_REC`: una
                // vez dentro se sigue repartiendo mas abajo de lo que se habria
                // entrado desde fuera. Ver los @def de las dos macros.
                return toom3_full<M, Base, NSTD_TOOM3_REC>(a, b);
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
                const auto rp = kmul_full_gen<M + 1, Base, ToomMin>(ap, bp);
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
                const auto z0 = kmul_full_gen<HH, Base, ToomMin>(a_lo, b_lo);
                const auto z2 = kmul_full_gen<HH, Base, ToomMin>(a_hi, b_hi);

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
                    const auto pp = kmul_full_gen<HH, Base, ToomMin>(sum_a, sum_b);
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

        // =====================================================================
        // 3 bis. Toom-3
        // =====================================================================

        /// @brief Producto COMPLETO MxM -> 2M por Toom-3, para cualquier M >= 3.
        ///
        /// Parte cada factor en TRES trozos de k = techo(M/3) limbos, evalua el
        /// polinomio en cinco puntos, multiplica, e interpola. Cinco productos
        /// de M/3 donde Karatsuba hace tres de M/2.
        ///
        /// LO QUE HACE DIFICIL A TOOM-3, Y QUE NO APARECE EN KARATSUBA
        /// ----------------------------------------------------------
        ///
        /// 1. HAY VALORES NEGATIVOS. El punto x = -1 obliga a evaluar
        ///    `a0 - a1 + a2`, que puede ser negativo, sobre arrays de limbos sin
        ///    signo. Se trabaja en complemento a dos dentro de Z/2^(64W), con
        ///    los ayudantes de `detail`.
        ///
        /// 2. PERO EL PRODUCTO DE DOBLE ANCHURA NO SE SALVA SOLO. Multiplicar
        ///    sin signo dos representaciones en complemento a dos acierta los
        ///    limbos BAJOS y nada mas: si X = x + 2^(64E) porque x < 0, entonces
        ///
        ///        X*Y = x*y + y*2^(64E)    (mod 2^(128E))
        ///
        ///    y la mitad alta sale mal. La correccion es restar Y de la mitad
        ///    alta cuando X es negativo, y X cuando lo es Y: dos restas de E
        ///    limbos. Olvidarla da un resultado que parece correcto en las
        ///    anchuras pequenas y falla en cuanto el punto -1 se vuelve negativo
        ///    de verdad. Solo `v(-1)` la necesita: `a0+a1+a2` y `a0+2a1+4a2` son
        ///    sumas de no negativos y ademas caben con el bit alto a cero, ya
        ///    que 7*2^(64k) < 2^(64k+3).
        ///
        /// 3. LA INTERPOLACION DIVIDE. Por 2 --desplazamiento ARITMETICO-- y por
        ///    3 --cadena de Jebelean--. Ver `detail::entre_tres`.
        ///
        /// LA INTERPOLACION, ESCRITA
        /// -------------------------
        ///   C(x) = c0 + c1 x + c2 x^2 + c3 x^3 + c4 x^4, en 0, 1, -1, 2, inf
        ///
        ///   c0 = v0                        c4 = vinf
        ///   A  = (v1 + vm1)/2 = c0+c2+c4   ->  c2 = A - c0 - c4
        ///   S  = (v1 - vm1)/2 = c1 + c3
        ///   D  = (v2 - c0 - 4c2 - 16c4)/2 = c1 + 4c3
        ///   c3 = (D - S)/3                 c1 = S - c3
        ///
        /// Las dos divisiones son exactas por construccion: `v2 - c0 - 4c2 -
        /// 16c4` vale `2c1 + 8c3`, y `D - S` vale `3c3`.
        ///
        /// @tparam M Numero de limbos de cada factor. >= 3.
        /// @tparam Base Corte de la recursion de Karatsuba; ver `kmul_full_gen`.
        /// @tparam ToomMin Hasta donde se sigue repartiendo en tres. Los cinco
        ///         subproductos vuelven a entrar por `kmul_full_gen` con este
        ///         mismo umbral, asi que la recursion se reparte sola.
        /// @param a Primer factor. @param b Segundo factor.
        /// @return El producto exacto, 2M limbos.
        template <std::size_t M, std::size_t Base, std::size_t ToomMin>
        [[nodiscard]] inline std::array<std::uint64_t, 2 * M>
        toom3_full(const std::array<std::uint64_t, M> &a, const std::array<std::uint64_t, M> &b) noexcept
        {
            static_assert(M >= 3, "toom3_full: hacen falta al menos tres limbos");

            constexpr std::size_t k = (M + 2) / 3; // techo de M/3
            constexpr std::size_t E = k + 1;       // anchura de los cinco valores
            constexpr std::size_t P = 2 * E;       // anchura de los cinco productos

            using vec_e = std::array<std::uint64_t, E>;
            using vec_p = std::array<std::uint64_t, P>;

            // ── el reparto en tres, rellenando con ceros hasta 3k ────────────
            // Se rellena en vez de arrastrar una tercera anchura: el trozo alto
            // tiene M-2k limbos, que puede ser menor que k, y llevar esa anchura
            // por todo el algoritmo no compra nada.
            vec_e A0{}, A1{}, A2{}, B0{}, B1{}, B2{};
            for (std::size_t i = 0; i < k; ++i)
            {
                A0[i] = a[i];
                B0[i] = b[i];
                A1[i] = (k + i < M) ? a[k + i] : 0;
                B1[i] = (k + i < M) ? b[k + i] : 0;
                A2[i] = (2 * k + i < M) ? a[2 * k + i] : 0;
                B2[i] = (2 * k + i < M) ? b[2 * k + i] : 0;
            }

            // ── los cinco puntos: 0, 1, -1, 2, infinito ──────────────────────
            vec_e pa1 = A0, pb1 = B0;
            detail::suma_anillo<E>(pa1, A1);
            detail::suma_anillo<E>(pa1, A2); // a0 + a1 + a2
            detail::suma_anillo<E>(pb1, B1);
            detail::suma_anillo<E>(pb1, B2);

            vec_e pam1 = A0, pbm1 = B0;
            detail::resta_anillo<E>(pam1, A1);
            detail::suma_anillo<E>(pam1, A2); // a0 - a1 + a2  <-- PUEDE SER NEGATIVO
            detail::resta_anillo<E>(pbm1, B1);
            detail::suma_anillo<E>(pbm1, B2);

            // a0 + 2a1 + 4a2, por Horner: ((2*a2) + a1)*2 + a0
            vec_e pa2 = A2, pb2 = B2;
            detail::desplaza_izq<E>(pa2, 1);
            detail::suma_anillo<E>(pa2, A1);
            detail::desplaza_izq<E>(pa2, 1);
            detail::suma_anillo<E>(pa2, A0);
            detail::desplaza_izq<E>(pb2, 1);
            detail::suma_anillo<E>(pb2, B1);
            detail::desplaza_izq<E>(pb2, 1);
            detail::suma_anillo<E>(pb2, B0);

            // ── los cinco productos ──────────────────────────────────────────
            const vec_p v0 = kmul_full_gen<E, Base, ToomMin>(A0, B0);
            const vec_p v1 = kmul_full_gen<E, Base, ToomMin>(pa1, pb1);
            const vec_p v2 = kmul_full_gen<E, Base, ToomMin>(pa2, pb2);
            const vec_p vinf = kmul_full_gen<E, Base, ToomMin>(A2, B2);

            vec_p vm1 = kmul_full_gen<E, Base, ToomMin>(pam1, pbm1);
            // EL ARREGLO DEL SIGNO. Ver el punto 2 de la cabecera: el producto
            // sin signo solo acierta la mitad baja cuando un factor es negativo.
            if (pam1[E - 1] >> 63)
            {
                unsigned char p = 0;
                for (std::size_t i = 0; i < E; ++i)
                    p = detail::sub_limb_borrow(vm1[E + i], pbm1[i], p);
            }
            if (pbm1[E - 1] >> 63)
            {
                unsigned char p = 0;
                for (std::size_t i = 0; i < E; ++i)
                    p = detail::sub_limb_borrow(vm1[E + i], pam1[i], p);
            }

            // ── la interpolacion ─────────────────────────────────────────────
            const vec_p &c0 = v0;
            const vec_p &c4 = vinf;

            vec_p c2 = v1; // A = (v1 + vm1)/2 = c0 + c2 + c4
            detail::suma_anillo<P>(c2, vm1);
            detail::mitad_exacta<P>(c2);
            detail::resta_anillo<P>(c2, c0);
            detail::resta_anillo<P>(c2, c4);

            vec_p sum = v1; // S = (v1 - vm1)/2 = c1 + c3
            detail::resta_anillo<P>(sum, vm1);
            detail::mitad_exacta<P>(sum);

            vec_p d = v2; // D = (v2 - c0 - 4c2 - 16c4)/2 = c1 + 4c3
            detail::resta_anillo<P>(d, c0);
            {
                vec_p t = c2;
                detail::desplaza_izq<P>(t, 2);
                detail::resta_anillo<P>(d, t);
            }
            {
                vec_p t = c4;
                detail::desplaza_izq<P>(t, 4);
                detail::resta_anillo<P>(d, t);
            }
            detail::mitad_exacta<P>(d);

            vec_p c3 = d; // c3 = (D - S)/3
            detail::resta_anillo<P>(c3, sum);
            detail::entre_tres<P>(c3);

            vec_p c1 = sum; // c1 = S - c3
            detail::resta_anillo<P>(c1, c3);

            // ── el montaje: r = c0 + c1 B + c2 B^2 + c3 B^3 + c4 B^4 ─────────
            // Los cinco coeficientes son NO NEGATIVOS y caben, asi que los
            // limbos que se salen de 2M son cero y se pueden ignorar.
            std::array<std::uint64_t, 2 * M> r{};
            const vec_p *cs[5] = {&c0, &c1, &c2, &c3, &c4};
            for (std::size_t t = 0; t < 5; ++t)
            {
                const std::size_t base = t * k;
                if (base >= 2 * M)
                    break;
                unsigned char c = 0;
                std::size_t j = 0;
                for (; j < P && base + j < 2 * M; ++j)
                    c = detail::add_limb_carry(r[base + j], (*cs[t])[j], c);
                for (std::size_t q = base + j; c && q < 2 * M; ++q)
                    c = detail::add_limb(r[q], std::uint64_t{c});
            }
            return r;
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
            /// @brief `z = x * y` en media anchura: desenrollado hasta
            ///        `TopeDesenrollado`, en bucle por encima. La decision es
            ///        `if constexpr`, asi que no queda rama en el binario.
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
            /// @brief `z = x * y` en media anchura, volviendo a Karatsuba.
            ///        Definido fuera de la clase porque necesita ver el
            ///        equilibrado, que se declara mas abajo.
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
        /// @tparam Base Corte de la recursion, que se ARRASTRA hacia abajo.
        ///
        /// @note `Base` esta aqui porque antes no estaba: el medio llamaba a
        ///       `mul_karatsuba_equilibrado<H, 8, ...>` con el 8 escrito a mano,
        ///       asi que cambiar `Base` en la llamada de arriba no cambiaba nada
        ///       por debajo del primer nivel. Cualquier barrido de `Base` hecho
        ///       sin esto mide una mezcla y no un corte.
        template <std::size_t TopeDesenrollado, std::size_t MinKaratsuba, std::size_t Base = 8>
        struct medio_reparto
        {
            /// @brief `z = x * y` en media anchura repartiendo otra vez, con
            ///        `Base` arrastrada hacia abajo para que un barrido de
            ///        umbrales mida un corte y no una mezcla.
            template <std::size_t H>
            void operator()(const std::array<std::uint64_t, H> &x, const std::array<std::uint64_t, H> &y,
                            std::array<std::uint64_t, H> &z) const noexcept
            {
                if constexpr (H >= MinKaratsuba && H >= 2)
                    mul_karatsuba_equilibrado<H, Base, medio_reparto<TopeDesenrollado, MinKaratsuba, Base>>(
                        x, y, z, medio_reparto<TopeDesenrollado, MinKaratsuba, Base>{});
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

        // =====================================================================
        // 2 bis. El CUADRADO, que no es un producto cualquiera
        // =====================================================================
        //
        // `a * a` tiene la mitad de trabajo que `a * b`, y la biblioteca no lo
        // aprovechaba: pasaba por `operator*` como si los dos factores fueran
        // distintos.
        //
        // EN EL ESCOLAR: los productos cruzados `a[i]*a[j]` con i != j aparecen
        // DOS VECES --una como (i,j) y otra como (j,i)-- y son iguales. Se
        // calcula la mitad de arriba de la tabla, se dobla, y se suman aparte
        // los N terminos de la diagonal `a[i]*a[i]`. De N^2 productos se pasa a
        // N(N+1)/2, que tiende a la mitad.
        //
        // EN KARATSUBA: los dos terminos del medio son `a_lo*a_hi` y
        // `a_hi*a_lo`, que son EL MISMO. Se calcula uno y se suma dos veces. De
        // tres productos de media anchura se pasa a dos.
        //
        // GMP mantiene umbrales separados para el cuadrado por esto mismo; su
        // `SQR_TOOM2_THRESHOLD` suele ser cerca del doble del de multiplicar.

        /// @brief Cuadrado COMPLETO MxM -> 2M, escolar, aprovechando la simetria.
        ///
        /// @param a Valor a elevar al cuadrado.
        /// @param r Destino de 2M limbos. **Se pone a cero al entrar.**
        template <std::size_t M>
        constexpr void sqr_full_escolar(const std::array<std::uint64_t, M> &a,
                                        std::array<std::uint64_t, 2 * M> &r) noexcept
        {
            r.fill(0);

            // Mitad estricta de arriba: los cruzados, una sola vez.
            for (std::size_t i = 0; i < M; ++i)
            {
                for (std::size_t j = i + 1; j < M; ++j)
                {
                    std::uint64_t alto = 0;
                    const std::uint64_t bajo = detail::mul64(a[i], a[j], alto);
                    unsigned char c = detail::add_limb(r[i + j], bajo);
                    std::size_t k = i + j + 1;
                    c = detail::add_limb_carry(r[k], alto, c);
                    while (c && ++k < 2 * M)
                        c = detail::add_limb(r[k], std::uint64_t{c});
                }
            }

            // Doblar: todo lo acumulado son cruzados, y cada uno cuenta dos
            // veces. Un desplazamiento de un bit sobre los 2M limbos.
            unsigned char acarreo = 0;
            for (std::size_t i = 0; i < 2 * M; ++i)
            {
                const unsigned char siguiente = static_cast<unsigned char>(r[i] >> 63);
                r[i] = (r[i] << 1) | acarreo;
                acarreo = siguiente;
            }

            // Y ahora la diagonal, que NO se dobla.
            for (std::size_t i = 0; i < M; ++i)
            {
                std::uint64_t alto = 0;
                const std::uint64_t bajo = detail::mul64(a[i], a[i], alto);
                unsigned char c = detail::add_limb(r[2 * i], bajo);
                std::size_t k = 2 * i + 1;
                if (k < 2 * M)
                {
                    c = detail::add_limb_carry(r[k], alto, c);
                    while (c && ++k < 2 * M)
                        c = detail::add_limb(r[k], std::uint64_t{c});
                }
            }
        }

        // NO HAY `sqr_escolar_bucle`, Y ES A PROPOSITO.
        //
        // Se escribio el 16 sep 2026 y se retiro el mismo dia, medido: perdia
        // contra el producto normal en las dieciseis anchuras del barrido, de
        // 0,35x a 0,80x. La razon es de diseno, no de aritmetica: se implemento
        // calculando el cuadrado COMPLETO de 2N limbos y truncando a N, porque
        // doblar los cruzados sin perder acarreos es mas comodo asi.
        //
        // Pero eso cuesta exactamente lo que la simetria ahorra --el doble de
        // trabajo para ahorrar la mitad-- y encima deja la sobrecarga del array
        // intermedio. Un cuadrado escolar modular que valiera la pena tendria
        // que doblar sobre N limbos, arrastrando el acarreo que se sale.
        //
        // No se ha escrito porque no hace falta: `sqr_karatsuba_equilibrado`
        // gana desde N=4 (2,47x medido), asi que no queda banda para el.

        /// @brief Cuadrado COMPLETO MxM -> 2M por Karatsuba, cualquier M.
        ///
        /// Tres cuadrados de media anchura en vez de tres productos: `z0 =
        /// a_lo^2`, `z2 = a_hi^2` y `z1 = (a_lo+a_hi)^2 - z0 - z2`.
        ///
        /// @tparam Base Corte de la recursion; ver `kmul_full_gen`.
        template <std::size_t M, std::size_t Base>
        [[nodiscard]] inline std::array<std::uint64_t, 2 * M>
        sqr_full_gen(const std::array<std::uint64_t, M> &a) noexcept
        {
            std::array<std::uint64_t, 2 * M> r{};

            if constexpr (M <= Base)
            {
                sqr_full_escolar<M>(a, r);
                return r;
            }
            else if constexpr (M % 2 == 1)
            {
                std::array<std::uint64_t, M + 1> ap{};
                for (std::size_t i = 0; i < M; ++i)
                    ap[i] = a[i];
                const auto rp = sqr_full_gen<M + 1, Base>(ap);
                for (std::size_t i = 0; i < 2 * M; ++i)
                    r[i] = rp[i];
                return r;
            }
            else
            {
                // El cuadrado completo por Karatsuba tiene el mismo esqueleto
                // que el producto, pero con `a` en los dos sitios. Se apoya en
                // `kmul_full_gen` para el termino cruzado, que es el unico que
                // no es un cuadrado.
                constexpr std::size_t HH = M / 2;
                std::array<std::uint64_t, HH> a_lo{}, a_hi{};
                for (std::size_t i = 0; i < HH; ++i)
                {
                    a_lo[i] = a[i];
                    a_hi[i] = a[HH + i];
                }

                const auto z0 = sqr_full_gen<HH, Base>(a_lo);        // a_lo^2
                const auto z2 = sqr_full_gen<HH, Base>(a_hi);        // a_hi^2
                const auto z1 = kmul_full_gen<HH, Base>(a_lo, a_hi); // a_lo*a_hi

                // r = z0 + 2*z1*B^HH + z2*B^(2HH)
                for (std::size_t i = 0; i < 2 * HH; ++i)
                    r[i] = z0[i];
                for (std::size_t i = 0; i < 2 * HH; ++i)
                    r[2 * HH + i] = z2[i];

                // El cruzado se suma DOS veces, desplazado HH limbos.
                for (int vez = 0; vez < 2; ++vez)
                {
                    unsigned char c = 0;
                    std::size_t i = 0;
                    for (; i < 2 * HH; ++i)
                        c = detail::add_limb_carry(r[HH + i], z1[i], c);
                    for (std::size_t k = HH + i; c && k < 2 * M; ++k)
                        c = detail::add_limb(r[k], std::uint64_t{c});
                }
                return r;
            }
        }

        /// @brief Cuadrado modular NxN -> N por Karatsuba equilibrado.
        ///
        /// Frente a `mul_karatsuba_equilibrado(a, a, r)`, se ahorra **uno de los
        /// dos productos del medio**: `a_lo*a_hi` y `a_hi*a_lo` son el mismo.
        ///
        /// @param a Valor. @param r Destino.
        /// @param medio Como calcular el unico termino del medio.
        template <std::size_t N, std::size_t Base = 8, typename Medio = medio_escolar<21>>
        void sqr_karatsuba_equilibrado(const std::array<std::uint64_t, N> &a, std::array<std::uint64_t, N> &r,
                                       Medio medio = Medio{}) noexcept
        {
            static_assert(N >= 2, "sqr_karatsuba_equilibrado: hacen falta al menos dos limbos");

            if constexpr (N % 2 == 1)
            {
                std::array<std::uint64_t, N + 1> ap{}, rp{};
                for (std::size_t i = 0; i < N; ++i)
                    ap[i] = a[i];
                sqr_karatsuba_equilibrado<N + 1, Base>(ap, rp, medio);
                for (std::size_t i = 0; i < N; ++i)
                    r[i] = rp[i];
            }
            else
            {
                constexpr std::size_t HH = N / 2;
                std::array<std::uint64_t, HH> a_lo{}, a_hi{};
                for (std::size_t i = 0; i < HH; ++i)
                {
                    a_lo[i] = a[i];
                    a_hi[i] = a[HH + i];
                }

                const auto z0 = sqr_full_gen<HH, Base>(a_lo);

                // UN SOLO producto del medio, no dos.
                std::array<std::uint64_t, HH> m{};
                medio(a_lo, a_hi, m);

                for (std::size_t i = 0; i < N; ++i)
                    r[i] = z0[i];

                // Se suma dos veces, que es lo mismo que doblarlo.
                for (int vez = 0; vez < 2; ++vez)
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < HH; ++i)
                        c = detail::add_limb_carry(r[HH + i], m[i], c);
                }
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
