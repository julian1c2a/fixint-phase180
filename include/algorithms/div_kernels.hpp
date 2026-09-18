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
// @file       algorithms/div_kernels.hpp
// @brief      Los nucleos de division, sueltos y medibles
// @date       2026-09-17
// =============================================================================
//
// POR QUE ESTA CAPA EXISTE
// ------------------------
// Es la misma razon que `mul_kernels.hpp`, y la misma leccion: hasta el 17 sep
// 2026 Knuth D vivia DENTRO de `fixed_int_t::divmod`, unas 300 lineas con los
// casos especiales, las ramas `#if` por compilador y el bucle principal
// mezclados.
//
// Con el algoritmo dentro del metodo **no se pueden medir dos variantes
// entrelazadas**: habria que recompilar entre una y otra, y el protocolo de
// docs/PLAN_SESION_MEDICION.md exige rondas alternas en el mismo proceso. Sin
// eso no se puede concluir nada, como quedo demostrado con la multiplicacion.
//
// Aqui los nucleos son funciones libres sobre `std::array<uint64_t, N>`. No
// hace falta `friend`: `fixed_int_t` expone sus limbos.
//
// LA PERILLA: COMO SE ESTIMA q^
// -----------------------------
// El paso D3 de Knuth --estimar el digito del cociente y refinarlo-- es **el
// bucle interno de toda la division**, y es exactamente lo que sustituye
// Moller-Granlund con su division 3-por-2 y un inverso precalculado.
//
// Por eso la estimacion se pasa como PARAMETRO y no se esconde dentro. Es la
// leccion de `medio_reparto` en Karatsuba: una decision escondida dentro de un
// algoritmo no se puede medir, y lo que no se mide se decide por analogia. Con
// la perilla fuera, comparar las dos estimaciones es cambiar un tipo.
//
// Y sirvio: el 18 sep 2026, con las dos entrelazadas en un proceso, se vio que
// Moller-Granlund **gana hasta 3,33x con divisores cortos y PIERDE hasta un 47%
// cuando el divisor ocupa toda la anchura**, que es lo que dan dos operandos
// aleatorios. Sin la perilla ese segundo dato no se habria visto, y el primero
// habria bastado para integrarla. El resultado es `estimador_auto`.
// =============================================================================

#ifndef NSTD_ALGORITHMS_DIV_KERNELS_HPP
#define NSTD_ALGORITHMS_DIV_KERNELS_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#ifdef NSTD_DIV_COMPRUEBA_PRECONDICIONES
#include <cstdio>
#include <cstdlib>
#endif

#if __has_include("intrinsics/arithmetic_operations.hpp")
#include "intrinsics/arithmetic_operations.hpp"
#endif
#if __has_include("intrinsics/bit_operations.hpp")
#include "intrinsics/bit_operations.hpp"
#endif

namespace nstd
{
    namespace algorithms
    {
        /// @brief Primitivas de 64 bits que necesita la division.
        ///
        /// Estaban como miembros estaticos de `fixed_int_t` y solo las usaba
        /// `divmod`, asi que se mudan con ella. La forma en que estan escritas
        /// --intrinseco bajo `is_constant_evaluated`, portable siempre presente--
        /// **no se toca**: es lo que permite que `divmod`, `/` y `%` sean
        /// `constexpr` tambien en MSVC e ICX-Windows (T3.1, auditoria 23 ago 2026).
        namespace detail
        {
            /// @brief Producto completo `x*y`: devuelve los 64 bits bajos y deja
            ///        los altos en `hi`.
            [[nodiscard]] constexpr std::uint64_t mul_64x64(std::uint64_t x, std::uint64_t y,
                                                            std::uint64_t &hi) noexcept
            {
#if defined(__SIZEOF_INT128__)
                // Constexpr-friendly en GCC/Clang/ICX-Linux: sin rama.
                const unsigned __int128 p = static_cast<unsigned __int128>(x) * y;
                hi = static_cast<std::uint64_t>(p >> 64);
                return static_cast<std::uint64_t>(p);
#else
                if (!std::is_constant_evaluated())
                {
#if defined(_MSC_VER) && defined(_M_X64)
                    return _umul128(x, y, &hi);
#endif
                }
                // Portable: escuela 32x32 -> 64.
                const std::uint64_t xl = x & 0xFFFFFFFFU;
                const std::uint64_t xh = x >> 32;
                const std::uint64_t yl = y & 0xFFFFFFFFU;
                const std::uint64_t yh = y >> 32;

                const std::uint64_t p0 = xl * yl;
                const std::uint64_t p1 = xl * yh;
                const std::uint64_t p2 = xh * yl;
                const std::uint64_t p3 = xh * yh;

                const std::uint64_t mid = (p0 >> 32) + (p1 & 0xFFFFFFFFU) + (p2 & 0xFFFFFFFFU);
                hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
                return (p0 & 0xFFFFFFFFU) | (mid << 32);
#endif
            }

#ifdef NSTD_DIV_COMPRUEBA_PRECONDICIONES
            /// @brief Aborta diciendo QUE precondicion se ha roto.
            ///
            /// Solo existe con `NSTD_DIV_COMPRUEBA_PRECONDICIONES`, y el `#ifdef`
            /// tiene que envolver TAMBIEN esta definicion, no solo su uso: llama a
            /// `std::fprintf` y `std::abort`, cuyas cabeceras se incluyen unicamente
            /// con la macro encendida. Definirla siempre compilaba con clang --que
            /// recibe `<cstdio>` por via transitiva-- y fallaba con gcc en cuanto la
            /// cabecera se compila SOLA. Lo cazo `check_headers_selfcontained.py`,
            /// que es exactamente su oficio.
            ///
            /// No es `constexpr` A PROPOSITO: en evaluacion constante, llamarla
            /// convierte la violacion en un **error de compilacion** en vez de en
            /// un aborto en caliente.
            [[noreturn]] inline void precondicion_rota(const char *que) noexcept
            {
                std::fprintf(stderr, "nstd::algorithms: PRECONDICION ROTA: %s\n", que);
                std::abort();
            }
#endif

            /// @brief Division `(hi:lo) / d`.
            ///
            /// @pre **`hi < d`.** No es documentacion decorativa: es lo que
            ///      garantiza que el cociente cabe en 64 bits, y desde el 17 sep
            ///      2026 tambien lo que impide que `divq` lance `#DE`. Ver abajo.
            ///
            /// @note **La precondicion esta en el nombre a proposito.** Estaba solo
            ///       en este `@pre`, y un llamante futuro puede no leer el Doxygen;
            ///       lo que no puede es no leer lo que teclea. El nombre largo es
            ///       el precio de que sea imposible llamarla sin enterarse.
            ///
            /// @note Y se puede COMPROBAR, no solo razonar: compilando con
            ///       `NSTD_DIV_COMPRUEBA_PRECONDICIONES` la funcion verifica
            ///       `hi < d` en cada llamada y aborta diciendo cual se rompio. La
            ///       suite entera se corre asi de vez en cuando; ver
            ///       `scripts/check_precondiciones_div.py`.
            ///
            /// @param hi Mitad alta del dividendo. @param lo Mitad baja.
            /// @param d Divisor, distinto de cero. @param rem Destino del resto.
            /// @return El cociente de 64 bits.
            [[nodiscard]] constexpr std::uint64_t div_128_64_hi_menor_que_d(std::uint64_t hi,
                                                                            std::uint64_t lo, std::uint64_t d,
                                                                            std::uint64_t &rem) noexcept
            {
#ifdef NSTD_DIV_COMPRUEBA_PRECONDICIONES
                // La precondicion, COMPROBADA. Apagada por defecto porque en el
                // camino caliente cuesta una comparacion por limbo; encendida, la
                // suite entera la ejercita. Ver la nota de arriba.
                if (!(hi < d))
                    precondicion_rota("div_128_64_hi_menor_que_d: hi >= d, el cociente no cabe en "
                                      "64 bits");
#endif
#if defined(__SIZEOF_INT128__) && !(defined(__INTEL_LLVM_COMPILER) && (defined(_WIN32) || defined(_WIN64)))
                if (!std::is_constant_evaluated())
                {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)
                    // UNA INSTRUCCION, NO UNA LLAMADA.
                    //
                    // Aqui ponia que «con hi < d, __udivti3 emite un solo divq».
                    // **Es falso**, y se comprobo mirando el codigo emitido el 17
                    // sep 2026: en la unidad de prueba habia 16 llamadas a
                    // `__udivti3` y solo 4 `divq`. Clang no puede saber que
                    // `hi < d` --es una precondicion escrita en la documentacion,
                    // no en el tipo-- asi que llama a la rutina general de 128/128.
                    //
                    // Medido con el bucle de `div_un_limbo`, las dos variantes
                    // entrelazadas y 20 repeticiones: **1,26x-1,44x, mediana
                    // 1,35x**, reproducido en N = 4, 8, 16, 32, 64, 128 y 256. El
                    // coste por limbo baja de ~115 a ~85 ciclos.
                    //
                    // No es 3x, y la razon esta en los numeros: de esos ~115
                    // ciclos, unos 30 son la llamada y los ~85 restantes son la
                    // LATENCIA del propio `divq`. La cadena es serial --cada
                    // division espera al resto de la anterior-- asi que no hay
                    // nada que solapar.
                    //
                    // @warning ESTO CAMBIA EL MODO DE FALLO. Si un llamante
                    //          incumpliera `hi < d`, `__udivti3` devolvia un valor
                    //          equivocado en silencio y `divq` lanza `#DE`, que
                    //          mata el proceso. Es mejor fallo --ruidoso en vez de
                    //          callado-- pero es otro, y la precondicion pasa a ser
                    //          critica y no meramente documental.
                    //
                    //          Los tres llamantes la cumplen, y por construccion:
                    //          `div_un_limbo` la tiene como INVARIANTE del bucle
                    //          --cada resto es menor que el divisor--, el estimador
                    //          de Knuth comprueba `u0 < v1` antes de llamar, y el
                    //          camino de N=2 obtiene `r_hi = a[1] % d`.
                    //
                    // Fuera de x86-64 no hay instruccion equivalente y se cae al
                    // `__int128` de abajo, que es lo que usan ARM64, ARM32 y
                    // RISC-V. En evaluacion constante, tambien.
                    std::uint64_t q, r;
                    __asm__("divq %4" : "=a"(q), "=d"(r) : "0"(lo), "1"(hi), "rm"(d));
                    rem = r;
                    return q;
#endif
                }
                // GCC/Clang/ICX-Linux fuera de x86-64, y el camino constexpr.
                const unsigned __int128 u = (static_cast<unsigned __int128>(hi) << 64) | lo;
                rem = static_cast<std::uint64_t>(u % d);
                return static_cast<std::uint64_t>(u / d);
#else
                if (!std::is_constant_evaluated())
                {
#if defined(__INTEL_LLVM_COMPILER) && (defined(_WIN32) || defined(_WIN64)) && defined(_M_X64)
                    // ICX en Windows define __SIZEOF_INT128__ pero su runtime no
                    // trae __udivti3/__umodti3: enlazaria mal. Usa asm estilo GCC
                    // (frontend Clang/LLVM).
                    std::uint64_t q, r;
                    __asm__("divq %4" : "=a"(q), "=d"(r) : "0"(lo), "1"(hi), "rm"(d));
                    rem = r;
                    return q;
#elif defined(_MSC_VER) && defined(_M_X64)
                    return _udiv128(hi, lo, d, &rem);
#endif
                }

                // Portable. Caso rapido hi == 0 y, si no, division larga bit a bit.
                if (hi == 0)
                {
                    rem = lo % d;
                    return lo / d;
                }
                std::uint64_t r = hi;
                std::uint64_t q = 0;
                for (int bit = 63; bit >= 0; --bit)
                {
                    const bool ovf = (r >> 63) != 0;
                    r = (r << 1) | ((lo >> bit) & 1U);
                    if (ovf || r >= d)
                    {
                        r -= d;
                        q |= std::uint64_t{1} << bit;
                    }
                }
                rem = r;
                return q;
#endif
            }

            /// @brief Cuenta los limbos significativos, o sea sin los ceros de
            ///        arriba. Devuelve 0 si el valor es cero.
            template <std::size_t N>
            [[nodiscard]] constexpr std::size_t
            limbos_significativos(const std::array<std::uint64_t, N> &x) noexcept
            {
                std::size_t n = N;
                while (n > 0 && x[n - 1] == 0)
                    --n;
                return n;
            }

            /// @brief El desplazamiento que deja el limbo alto con su bit mas
            ///        significativo a uno. Es el paso D1 de Knuth.
            [[nodiscard]] constexpr int normalizacion(std::uint64_t alto) noexcept
            {
#if __has_include("intrinsics/bit_operations.hpp")
                return intrinsics::clz64(alto);
#else
                int s = 0;
                std::uint64_t tmp = alto;
                while ((tmp & (std::uint64_t{1} << 63)) == 0)
                {
                    ++s;
                    tmp <<= 1;
                }
                return s;
#endif
            }

            /// @brief El inverso de un divisor NORMALIZADO, para la division 2/1.
            ///
            /// `v = floor((2^128 - 1) / d) - 2^64`, que es lo que pide el algoritmo 1
            /// de Moller y Granlund (*Improved division by invariant integers*, 2011).
            ///
            /// @pre `d` **normalizado**: su bit mas alto a uno.
            ///
            /// @note Se calcula con la propia `div_128_64_hi_menor_que_d`, y su
            ///       precondicion se cumple por construccion: con `d` normalizado,
            ///       `~d < d`.
            ///
            /// @warning **Cuesta una division de hardware.** Por eso hay umbral: ver
            ///          `div_un_limbo`.
            [[nodiscard]] constexpr std::uint64_t inverso_2por1(std::uint64_t d) noexcept
            {
                std::uint64_t r = 0;
                return div_128_64_hi_menor_que_d(~d, ~std::uint64_t{0}, d, r);
            }

            /// @brief Division de dos limbos por uno, **sin dividir**.
            ///
            /// Es el algoritmo 4 de Moller-Granlund: sustituye la division por dos
            /// multiplicaciones y unas sumas, usando el inverso precalculado. Las dos
            /// correcciones del final se toman con probabilidad muy baja.
            ///
            /// @pre `d` **normalizado** y `u1 < d`.
            /// @param u1 Limbo alto del dividendo. @param u0 Limbo bajo.
            /// @param d Divisor normalizado. @param v Su inverso.
            /// @param rem Destino del resto.
            /// @return El cociente de 64 bits.
            ///
            /// @note Lo que gana no es «menos operaciones», es **menos latencia**. La
            ///       cadena de `div_un_limbo` es serial --cada division espera al
            ///       resto de la anterior-- y `divq` tiene ~85 ciclos de latencia
            ///       frente a los ~3-5 de una multiplicacion.
            [[nodiscard]] constexpr std::uint64_t div_2por1_preinv(std::uint64_t u1, std::uint64_t u0,
                                                                   std::uint64_t d, std::uint64_t v,
                                                                   std::uint64_t &rem) noexcept
            {
                std::uint64_t q1 = 0;
                const std::uint64_t q0 = mul_64x64(v, u1, q1);

                // (q1,q0) += (u1,u0), en 128 bits
                const std::uint64_t s0 = q0 + u0;
                const std::uint64_t acarreo = (s0 < q0) ? 1u : 0u;
                q1 = q1 + u1 + acarreo + 1;

                std::uint64_t alto = 0;
                const std::uint64_t bajo = mul_64x64(q1, d, alto);
                std::uint64_t r = u0 - bajo;

                if (r > s0)
                {
                    --q1;
                    r += d;
                }
                if (r >= d)
                {
                    ++q1;
                    r -= d;
                }
                rem = r;
                return q1;
            }

            /// @brief El inverso de un divisor NORMALIZADO de **dos** limbos, para
            ///        la division 3/2. Algoritmo 6 de Moller-Granlund.
            ///
            /// @pre `d1` **normalizado**: su bit mas alto a uno.
            /// @param d1 Limbo alto del divisor. @param d0 El bajo.
            ///
            /// @warning **Cuesta una division de hardware**, la de `inverso_2por1`,
            ///          mas cuatro multiplicaciones. Se paga una vez por llamada a
            ///          `div_knuth_d`, no una vez por digito: por eso el umbral se
            ///          mide en **digitos de cociente**. Ver `NSTD_MG_3POR2_MIN`.
            [[nodiscard]] constexpr std::uint64_t inverso_3por2(std::uint64_t d1, std::uint64_t d0) noexcept
            {
                std::uint64_t v = inverso_2por1(d1);

                std::uint64_t p = d1 * v; // solo la parte baja, a proposito
                p += d0;
                if (p < d0)
                {
                    --v;
                    if (p >= d1)
                    {
                        --v;
                        p -= d1;
                    }
                    p -= d1;
                }

                std::uint64_t t1 = 0;
                const std::uint64_t t0 = mul_64x64(v, d0, t1);
                p += t1;
                if (p < t1)
                {
                    --v;
                    if (p > d1 || (p == d1 && t0 >= d0))
                        --v;
                }
                return v;
            }

            /// @brief Divide tres limbos entre dos, **sin dividir**. Algoritmo 5 de
            ///        Moller-Granlund.
            ///
            /// @pre `d1` **normalizado** y `(u2,u1) < (d1,d0)`. La segunda no la
            ///      garantiza Knuth D: ver `estimador_moller_granlund`, que la
            ///      comprueba antes de llamar aqui.
            /// @param u2 Limbo alto del dividendo. @param u1 El medio. @param u0 El
            ///        bajo. @param d1 Limbo alto del divisor. @param d0 El bajo.
            /// @param v El inverso de `(d1,d0)`, de `inverso_3por2`.
            /// @return El cociente **exacto** del subproblema, que cabe en un limbo.
            [[nodiscard]] constexpr std::uint64_t div_3por2_preinv(std::uint64_t u2, std::uint64_t u1,
                                                                   std::uint64_t u0, std::uint64_t d1,
                                                                   std::uint64_t d0, std::uint64_t v) noexcept
            {
                std::uint64_t q1 = 0;
                std::uint64_t q0 = mul_64x64(v, u2, q1);
                q0 += u1;
                if (q0 < u1)
                    ++q1;
                q1 += u2;

                std::uint64_t r1 = u1 - q1 * d1;

                std::uint64_t t1 = 0;
                const std::uint64_t t0 = mul_64x64(d0, q1, t1);

                // (r1:u0) - (t1:t0) - (d1:d0), en 128 bits
                std::uint64_t r0 = u0 - t0;
                std::uint64_t prestamo = (u0 < t0) ? 1u : 0u;
                r1 = r1 - t1 - prestamo;

                const std::uint64_t r0_menos_d0 = r0 - d0;
                prestamo = (r0 < d0) ? 1u : 0u;
                r1 = r1 - d1 - prestamo;
                r0 = r0_menos_d0;

                ++q1;

                // Las dos correcciones del final, de probabilidad muy baja.
                if (r1 >= q0)
                {
                    --q1;
                    const std::uint64_t suma = r0 + d0;
                    r1 = r1 + d1 + ((suma < r0) ? 1u : 0u);
                    r0 = suma;
                }
                if (r1 > d1 || (r1 == d1 && r0 >= d0))
                    ++q1;

                return q1;
            }

        } // namespace detail

        // =====================================================================
        // 1. Divisor de UN limbo
        // =====================================================================

        /// @brief `a / d` y `a % d` con `d` de un solo limbo.
        ///
        /// Sustituye la division binaria de O(64N^2) por **N instrucciones DIV**:
        /// se recorre de arriba abajo dividiendo `(resto:a[i])` entre `d`, y
        /// `resto < d` es invariante del bucle. Con esa invariante `__udivti3` y
        /// `_udiv128` emiten **un solo `divq`** por limbo.
        ///
        /// @tparam N Numero de limbos.
        /// @param a Dividendo. @param d Divisor, distinto de cero.
        /// @param q Destino del cociente.
        /// @return El resto, que cabe en un limbo.
        /// @def NSTD_MG_2POR1_MIN
        /// @brief Anchura desde la que `div_un_limbo` usa Moller-Granlund.
        ///
        /// MG cambia cada division por dos multiplicaciones con un inverso
        /// precalculado, **pero calcular el inverso cuesta una division**. Con uno o
        /// dos limbos esa division extra no se amortiza.
        ///
        /// **Medido el 18 sep 2026 con clang**, el bucle entero --normalizacion e
        /// inverso incluidos--, las dos variantes entrelazadas, 20 repeticiones:
        ///
        ///     N      divq       MG     razon        N      divq       MG     razon
        ///     1        24      103     0,23x        16    1 258      353     3,57x
        ///     2       118      130     0,91x        32    2 695      604     4,46x
        ///     3       189      132     1,43x        64    5 296    1 126     4,71x
        ///     4       284      137     2,07x       128   11 034    2 222     4,97x
        ///     8       626      218     2,88x       256   20 860    4 198     4,97x
        ///
        /// El coste por limbo cae de ~84 a ~17 ciclos, y la razon crece con N porque
        /// el coste fijo se amortiza.
        ///
        /// @warning **El umbral no es cosmetico.** En N=1 MG es 4x MAS LENTO: se
        ///          pagan dos divisiones donde bastaba una. Un barrido que empezara
        ///          en N=4 --como el primero que se hizo-- lo habria tapado por
        ///          completo, y `uint64_fixed_t` y `uint128_fixed_t`, que son los
        ///          tipos mas usados, habrian salido perdiendo.
#ifndef NSTD_MG_2POR1_MIN
#define NSTD_MG_2POR1_MIN 3
#endif

        template <std::size_t N>
        [[nodiscard]] constexpr std::uint64_t div_un_limbo(const std::array<std::uint64_t, N> &a,
                                                           std::uint64_t d,
                                                           std::array<std::uint64_t, N> &q) noexcept
        {
            if constexpr (N >= NSTD_MG_2POR1_MIN)
            {
                const int s = detail::normalizacion(d);
                const std::uint64_t dn = d << s;
                const std::uint64_t v = detail::inverso_2por1(dn);
                std::uint64_t resto = 0;

                if (s == 0)
                {
                    for (std::size_t i = N; i-- > 0;)
                        q[i] = detail::div_2por1_preinv(resto, a[i], dn, v, resto);
                    return resto;
                }

                // Con desplazamiento, el DIVIDENDO tambien hay que recorrerlo
                // desplazado `s` bits a la izquierda, arrastrando lo que cruza de un
                // limbo al siguiente; y el resto se desplaza de vuelta al final.
                // Ese trasiego es parte del coste, y esta contado en las cifras.
                resto = a[N - 1] >> (64 - s);
                std::uint64_t arrastre = a[N - 1] << s;
                for (std::size_t i = N - 1; i-- > 0;)
                {
                    const std::uint64_t trozo = arrastre | (a[i] >> (64 - s));
                    q[i + 1] = detail::div_2por1_preinv(resto, trozo, dn, v, resto);
                    arrastre = a[i] << s;
                }
                q[0] = detail::div_2por1_preinv(resto, arrastre, dn, v, resto);
                return resto >> s;
            }
            else
            {
                // N = 1 o 2: el inverso costaria mas que las divisiones que ahorra.
                std::uint64_t resto = 0;
                for (std::size_t i = N; i-- > 0;)
                    q[i] = detail::div_128_64_hi_menor_que_d(resto, a[i], d, resto);
                return resto;
            }
        }

        // =====================================================================
        // 2. La perilla: como se estima el digito del cociente (paso D3)
        // =====================================================================

        /// @brief La estimacion clasica de Knuth: dividir por el limbo alto y
        ///        refinar mirando el siguiente.
        ///
        /// Es el bucle interno de toda la division, y por eso es la perilla. El
        /// refinamiento compara `q^ * v2` contra `r^ * B + u2` de forma EXACTA;
        /// antes, en plataformas sin `__int128` ni `_umul128`, se salia sin
        /// comparar y se dejaba que el add-back del paso D5 corrigiera.
        ///
        /// @note Cuando llegue Moller-Granlund (P2.10) sera otro tipo con este
        ///       mismo `operator()`, y compararlos sera cambiar un parametro de
        ///       plantilla. Ese es justo el punto de tenerlo fuera.
        struct estimador_knuth
        {
            /// @brief No necesita preparacion. Existe porque el nucleo se la pide a
            ///        todos: es donde `estimador_moller_granlund` calcula su inverso.
            constexpr void prepara(std::uint64_t /*v1*/, std::uint64_t /*v2*/,
                                   std::size_t /*digitos*/) noexcept
            {
            }

            /// @param u0 Limbo alto de la ventana. @param u1 El siguiente.
            /// @param u2 El de debajo. @param v1 Limbo alto del divisor.
            /// @param v2 El siguiente del divisor.
            [[nodiscard]] constexpr std::uint64_t operator()(std::uint64_t u0, std::uint64_t u1,
                                                             std::uint64_t u2, std::uint64_t v1,
                                                             std::uint64_t v2) const noexcept
            {
                std::uint64_t q_hat = 0, r_hat = 0;
                bool sin_refinar = false;

                if (u0 > v1)
                {
                    // r^ >= B seguro: no hay nada que refinar.
                    q_hat = ~std::uint64_t{0};
                    sin_refinar = true;
                }
                else if (u0 == v1)
                {
                    q_hat = ~std::uint64_t{0};
                    r_hat = u1 + v1;
                    sin_refinar = (r_hat < u1); // desbordo ==> r^ >= B
                }
                else
                {
                    // 0 <= u0 < v1: division 128/64 exacta.
                    q_hat = detail::div_128_64_hi_menor_que_d(u0, u1, v1, r_hat);
                }

                if (!sin_refinar)
                {
                    while (true)
                    {
                        std::uint64_t alto = 0;
                        const std::uint64_t bajo = detail::mul_64x64(q_hat, v2, alto);
                        if (alto < r_hat || (alto == r_hat && bajo <= u2))
                            break;
                        --q_hat;
                        const std::uint64_t r_nuevo = r_hat + v1;
                        if (r_nuevo < r_hat)
                            break; // r^ desbordo B
                        r_hat = r_nuevo;
                    }
                }

                return q_hat;
            }
        };

        /// @def NSTD_MG_3POR2_MIN
        /// @brief Digitos de cociente desde los que compensa la estimacion 3/2.
        ///
        /// **No es una anchura, son digitos de cociente**, que son `N - n + 1` con
        /// `n` los limbos significativos del divisor. La 3/2 ahorra por digito, pero
        /// su inverso se paga **una vez por llamada**: con un solo digito no hay
        /// sobre que amortizarlo.
        ///
        /// Y el caso de un digito no es raro: es `n == N`, justo lo que dan dos
        /// operandos aleatorios de la misma anchura.
        ///
        /// **Medido el 18 sep 2026 con clang**, las variantes entrelazadas en un
        /// mismo proceso, 20 repeticiones, Knuth D entero:
        ///
        ///     digitos    N=16            N=64
        ///        1       0,78x           0,91x     <-- pierde
        ///        2       1,03x           0,98x     <-- empata: no compensa
        ///        3       1,28x           1,11x     <-- desde aqui gana
        ///        4       1,39x           1,13x
        ///        5       1,48x           1,15x
        ///        7-9     1,61x           1,28x
        ///
        /// Con `n = 2` --el divisor corto, donde el coste por digito manda-- la
        /// ganancia llega a **3,33x** en N=128.
        ///
        /// El umbral esta en 3 y no en 2 porque la casilla de dos digitos **empata
        /// dentro del ruido**: 0,96x, 0,98x, 1,02x y 1,03x en cuatro tandas. Un
        /// cambio que no se distingue del ruido no se integra.
        ///
        /// @warning Bajarla a 1 hace **perder hasta un 45%** en el caso mas comun,
        ///          que es el divisor de la anchura entera.
#ifndef NSTD_MG_3POR2_MIN
#define NSTD_MG_3POR2_MIN 3
#endif

        /// @brief La estimacion de q̂ por Moller-Granlund 3/2, **pura**: sin umbral.
        ///
        /// Esta sirve para medir contra `estimador_knuth`. La que usa el nucleo por
        /// omision es `estimador_auto`, que elige entre las dos.
        ///
        /// @note Devuelve el cociente **exacto** del subproblema de tres limbos entre
        ///       dos, que como estimacion es a lo sumo uno mayor que el digito
        ///       verdadero: la misma garantia que da la estimacion refinada de Knuth,
        ///       asi que los pasos D4 y D5 valen sin tocarlos.
        struct estimador_moller_granlund
        {
            std::uint64_t inverso{0};

            constexpr void prepara(std::uint64_t v1, std::uint64_t v2, std::size_t /*digitos*/) noexcept
            {
                inverso = detail::inverso_3por2(v1, v2);
            }

            [[nodiscard]] constexpr std::uint64_t operator()(std::uint64_t u0, std::uint64_t u1,
                                                             std::uint64_t u2, std::uint64_t v1,
                                                             std::uint64_t v2) const noexcept
            {
                // La 3/2 exige (u0,u1) < (v1,v2), y **Knuth D no lo garantiza**: su
                // invariante es que la ventana entera de n+1 limbos vale menos que el
                // divisor por B, y los dos limbos altos pueden empatar si los de
                // abajo compensan.
                //
                // Cuando eso pasa el digito es B-1 **exactamente**, no estimado: con
                // v1 normalizado sale U/V > B - 2/B, y el invariante da q <= B-1.
                //
                // Esta rama no la encontro ningun operando aleatorio: 9300 casos
                // pasaron limpios. La encontro el primer barrido de esquinas.
                if (u0 > v1 || (u0 == v1 && u1 >= v2))
                    return ~std::uint64_t{0};

                return detail::div_3por2_preinv(u0, u1, u2, v1, v2, inverso);
            }
        };

        /// @brief El estimador por omision: Knuth cuando hay pocos digitos de
        ///        cociente, Moller-Granlund cuando hay con que amortizar el inverso.
        ///
        /// La decision **no puede ser un parametro de plantilla**: depende de `n`,
        /// que no se conoce hasta ejecucion. Asi que vive aqui dentro, y la rama por
        /// digito la predice el procesador siempre bien, porque no cambia dentro de
        /// una llamada.
        ///
        /// @note Se midio tambien la alternativa sin rama --sacar la decision fuera y
        ///       tener dos bucles-- y **empata en velocidad dentro del ruido**,
        ///       pero cuesta **1,69x de codigo objeto** (339 KB frente a 201 KB,
        ///       instanciando N = 2..64). Por eso el nucleo se instancia una sola vez.
        ///
        /// @tparam N La anchura, que hace falta para una razon: con `n >= 2` siempre,
        ///         los digitos de cociente nunca pasan de `N - 1`. Si eso ya es menor
        ///         que el umbral, **Moller-Granlund no puede usarse nunca en esta
        ///         anchura**, y entonces no se compila: el `if constexpr` deja el
        ///         tipo vacio y la division corta se queda exactamente como estaba,
        ///         sin rama ni estado que arrastrar.
        template <std::size_t N>
        struct estimador_auto
        {
            /// Si en esta anchura MG no cabe, ni se instancia.
            static constexpr bool alcanzable = (N >= NSTD_MG_3POR2_MIN + 1);

            struct vacio
            {
            };
            struct con_inverso
            {
                std::uint64_t inverso{0};
                bool usa_mg{false};
            };

            [[no_unique_address]] std::conditional_t<alcanzable, con_inverso, vacio> est{};

            constexpr void prepara(std::uint64_t v1, std::uint64_t v2, std::size_t digitos) noexcept
            {
                if constexpr (alcanzable)
                {
                    est.usa_mg = (digitos >= NSTD_MG_3POR2_MIN);
                    // Si no se va a usar, **ni siquiera se calcula**: es una division.
                    est.inverso = est.usa_mg ? detail::inverso_3por2(v1, v2) : 0;
                }
                else
                {
                    (void)v1;
                    (void)v2;
                    (void)digitos;
                }
            }

            [[nodiscard]] constexpr std::uint64_t operator()(std::uint64_t u0, std::uint64_t u1,
                                                             std::uint64_t u2, std::uint64_t v1,
                                                             std::uint64_t v2) const noexcept
            {
                if constexpr (alcanzable)
                {
                    if (est.usa_mg)
                    {
                        if (u0 > v1 || (u0 == v1 && u1 >= v2))
                            return ~std::uint64_t{0};
                        return detail::div_3por2_preinv(u0, u1, u2, v1, v2, est.inverso);
                    }
                }
                return estimador_knuth{}(u0, u1, u2, v1, v2);
            }
        };

        // =====================================================================
        // 2b. Divisor de un limbo CONOCIDO EN COMPILACION
        // =====================================================================

        /// @brief `a / D` y `a % D` con `D` constante de plantilla, de un limbo.
        ///
        /// Es el tramo 2e de P1.5, y **no es Granlund-Montgomery portado**: es
        /// `div_un_limbo` con el preambulo evaluado en compilacion.
        ///
        /// POR QUE ASI, Y NO PORTANDO EL ALGORITMO VIEJO
        /// ---------------------------------------------
        /// `div_un_limbo` hace tres cosas antes del bucle --normalizar, desplazar
        /// el divisor y calcular su inverso, que cuesta un `divq`-- y luego una
        /// pasada de `div_2por1_preinv` por limbo. Con `D` constante, las tres
        /// primeras **son constantes**, y `normalizacion` e `inverso_2por1` ya
        /// eran `constexpr`, asi que el compilador las resuelve sin que haya que
        /// escribir un algoritmo nuevo.
        ///
        /// CUANTO GANA, MEDIDO
        /// -------------------
        /// **El 18 sep 2026 con clang**, `div<D>()` contra `operator/` con el
        /// mismo divisor, entrelazadas, 20 repeticiones:
        ///
        ///     N        1     2     3     4     8    16    32    64   128
        ///     razon  3,03  8,13  5,73  3,07  1,92  1,39  1,10  1,06  1,03
        ///
        /// Y con N=4, moviendo el divisor: 3,07x (D=7), 3,18x (D=1e9+7), 5,29x
        /// (D=1e19), 6,98x (D=2^63) y 3,74x (D=2^64-1). **Nunca pierde.**
        ///
        /// **La ganancia esta en N pequena y se agota hacia N=32**, porque el
        /// preambulo es un coste fijo y el bucle crece con N. Es el mismo patron
        /// que en `NSTD_MG_3POR2_MIN`: coste fijo contra ahorro por iteracion.
        ///
        /// @note Una primera medida, pasando `s`, `dn` y `v` como parametros de
        ///       EJECUCION, daba 2,12x-3,63x y se llamo «el techo». Era al reves:
        ///       resulto ser el suelo. Con `D` constante de plantilla las tres
        ///       son constantes de compilacion, y el compilador especializa el
        ///       desplazamiento y la multiplicacion por el reciproco. De ahi que
        ///       el 3,63x de N=2 acabara siendo 8,13x.
        ///
        /// @warning `PERFORMANCE.md` decia que este truco «solo paga con divisores
        ///          grandes» y que con los pequenos es mas lento. Eso se midio el
        ///          9 sep 2026 sobre la implementacion vieja de `int128_param_t`
        ///          y **con el eje equivocado**: lo que manda no es el tamano del
        ///          divisor sino `N`. Ademas su rival ha cambiado dos veces desde
        ///          entonces (Moller-Granlund 2/1 y el `divq` en linea).
        ///
        /// @tparam D El divisor. **No puede ser cero**, y se comprueba en
        ///         compilacion, que es la ventaja de tenerlo como parametro.
        /// @tparam N Numero de limbos.
        /// @param a Dividendo. @param q Destino del cociente.
        /// @return El resto, que cabe en un limbo.
        template <std::uint64_t D, std::size_t N>
        [[nodiscard]] constexpr std::uint64_t div_por_constante(const std::array<std::uint64_t, N> &a,
                                                                std::array<std::uint64_t, N> &q) noexcept
        {
            static_assert(D != 0, "division por cero, y aqui se ve en compilacion");

            // Las tres lineas del preambulo, ahora constantes.
            constexpr int s = detail::normalizacion(D);
            constexpr std::uint64_t dn = D << s;
            constexpr std::uint64_t v = detail::inverso_2por1(dn);

            std::uint64_t resto = 0;

            if constexpr (s == 0)
            {
                for (std::size_t i = N; i-- > 0;)
                    q[i] = detail::div_2por1_preinv(resto, a[i], dn, v, resto);
                return resto;
            }
            else if constexpr (N == 1)
            {
                // Con un solo limbo no hay arrastre que encadenar.
                q[0] = detail::div_2por1_preinv(a[0] >> (64 - s), a[0] << s, dn, v, resto);
                return resto >> s;
            }
            else
            {
                resto = a[N - 1] >> (64 - s);
                std::uint64_t arrastre = a[N - 1] << s;
                for (std::size_t i = N - 1; i-- > 0;)
                {
                    const std::uint64_t trozo = arrastre | (a[i] >> (64 - s));
                    q[i + 1] = detail::div_2por1_preinv(resto, trozo, dn, v, resto);
                    arrastre = a[i] << s;
                }
                q[0] = detail::div_2por1_preinv(resto, arrastre, dn, v, resto);
                return resto >> s;
            }
        }

        /// @brief Solo el resto de `a % D`, sin escribir el cociente.
        ///
        /// @note No ahorra el bucle --el resto se obtiene arrastrandolo limbo a
        ///       limbo, asi que hay que recorrerlos todos igual--, pero si ahorra
        ///       las N escrituras del cociente y deja al compilador tirar los
        ///       calculos que solo servian para el.
        template <std::uint64_t D, std::size_t N>
        [[nodiscard]] constexpr std::uint64_t
        mod_por_constante(const std::array<std::uint64_t, N> &a) noexcept
        {
            std::array<std::uint64_t, N> tirar{};
            return div_por_constante<D, N>(a, tirar);
        }

        // =====================================================================
        // 3. Knuth D, el caso general
        // =====================================================================

        /// @brief `a / b` y `a % b` por el algoritmo D de Knuth (TAOCP vol. 2
        ///        §4.3.1), base B = 2^64.
        ///
        /// @pre `b` tiene **al menos dos limbos significativos**. El caso de un
        ///      limbo lo resuelve `div_un_limbo`, que es mucho mas barato, y el
        ///      de cero lo rechaza el llamante.
        ///
        /// @tparam N Numero de limbos de los operandos.
        /// @tparam Estimador Como se calcula el digito del cociente. Por omision
        ///         `estimador_auto`, que elige en ejecucion; las dos puras son
        ///         `estimador_knuth` y `estimador_moller_granlund`, y estan ahi para
        ///         poder medirlas. Ver la nota de la cabecera del fichero.
        ///         El nucleo le pide `prepara(v1, v2, digitos)` **una vez** antes del
        ///         bucle, y luego `operator()` por cada digito.
        /// @param a Dividendo. @param b Divisor.
        /// @param q Destino del cociente. **Se pone a cero al entrar.**
        /// @param r Destino del resto. **Se pone a cero al entrar.**
        /// @tparam Limpiar Si el nucleo debe poner `q` y `r` a cero al entrar.
        ///         **Ponerlo a `false` cuando los destinos YA son cero**, que es
        ///         lo que pasa cuando llama `fixed_int_t::divmod`: alli
        ///         `fixed_int_t q{}, r{}` ya value-inicializa. Volver a limpiarlos
        ///         costaba 345 lineas de ensamblador de mas -- medido comparando
        ///         el codigo emitido contra el arbol de HEAD.
        /// @param est La estimacion a usar.
        template <std::size_t N, bool Limpiar = true, typename Estimador = estimador_auto<N>>
        constexpr void div_knuth_d(const std::array<std::uint64_t, N> &a,
                                   const std::array<std::uint64_t, N> &b, std::array<std::uint64_t, N> &q,
                                   std::array<std::uint64_t, N> &r, Estimador est = Estimador{}) noexcept
        {
            static_assert(N >= 2, "div_knuth_d: hacen falta al menos dos limbos");

            if constexpr (Limpiar)
            {
                q.fill(0);
                r.fill(0);
            }

            const std::size_t n = detail::limbos_significativos<N>(b);
            const std::size_t m_quot = N - n; // digitos del cociente: q[0..m_quot]

            // D1. Normalizar: desplazar para que v[n-1] tenga su bit alto a uno.
            const int s = detail::normalizacion(b[n - 1]);

            std::array<std::uint64_t, N> v{};     // divisor normalizado  [0..n-1]
            std::array<std::uint64_t, N + 1> u{}; // dividendo normalizado [0..N]

            if (s == 0)
            {
                for (std::size_t i = 0; i < n; ++i)
                    v[i] = b[i];
                for (std::size_t i = 0; i < N; ++i)
                    u[i] = a[i];
                // u[N] se queda en cero
            }
            else
            {
                for (std::size_t i = n - 1; i > 0; --i)
                    v[i] = (b[i] << s) | (b[i - 1] >> (64 - s));
                v[0] = b[0] << s;
                u[N] = a[N - 1] >> (64 - s);
                for (std::size_t i = N - 1; i > 0; --i)
                    u[i] = (a[i] << s) | (a[i - 1] >> (64 - s));
                u[0] = a[0] << s;
            }

            const std::uint64_t v1 = v[n - 1];
            const std::uint64_t v2 = v[n - 2]; // seguro: n >= 2

            // Lo que el estimador necesite calcular UNA vez por llamada. Se le dan
            // los digitos de cociente porque es sobre ellos sobre lo que se amortiza
            // ese calculo: ver `NSTD_MG_3POR2_MIN`.
            est.prepara(v1, v2, m_quot + 1);

            // D2-D7. El bucle principal, de j = m_quot hacia abajo.
            for (std::size_t j = m_quot + 1; j-- > 0;)
            {
                const std::uint64_t u0 = u[j + n];     // limbo alto de la ventana
                const std::uint64_t u1 = u[j + n - 1]; // el siguiente
                const std::uint64_t u2 = u[j + n - 2]; // el de debajo (n>=2, j>=0)

                // D3. Estimar el digito. AQUI ESTA LA PERILLA.
                std::uint64_t q_hat = est(u0, u1, u2, v1, v2);

                // D4. Multiplicar y restar: u[j..j+n] -= q^ * v[0..n-1]
                std::uint64_t prestamo = 0;
                for (std::size_t i = 0; i < n; ++i)
                {
                    std::uint64_t prod_alto = 0;
                    const std::uint64_t prod_bajo = detail::mul_64x64(q_hat, v[i], prod_alto);
                    const std::uint64_t resta = prod_bajo + prestamo;
                    prestamo = prod_alto + (resta < prod_bajo ? 1U : 0U);
                    if (u[j + i] < resta)
                        ++prestamo;
                    u[j + i] -= resta;
                }

                // D5. Devolver lo restado de mas, si hubo. Pasa con probabilidad
                // ~2/B por paso, o sea casi nunca, pero cuando pasa hay que
                // corregir o el resultado es basura.
                if (u[j + n] < prestamo)
                {
                    u[j + n] -= prestamo; // el envoltorio es intencionado
                    std::uint64_t acarreo = 0;
                    for (std::size_t i = 0; i < n; ++i)
                    {
                        const std::uint64_t s1 = u[j + i] + v[i];
                        const std::uint64_t s2 = s1 + acarreo;
                        acarreo = (s1 < u[j + i]) + (s2 < s1);
                        u[j + i] = s2;
                    }
                    u[j + n] += acarreo;
                    --q_hat;
                }
                else
                {
                    u[j + n] -= prestamo;
                }

                // D6. Guardar el digito del cociente.
                q[j] = q_hat;
            }

            // D8. Desnormalizar: el resto es u[0..n-1] desplazado a la derecha.
            if (s == 0)
            {
                for (std::size_t i = 0; i < n; ++i)
                    r[i] = u[i];
            }
            else
            {
                for (std::size_t i = 0; i < n - 1; ++i)
                    r[i] = (u[i] >> s) | (u[i + 1] << (64 - s));
                r[n - 1] = u[n - 1] >> s;
            }
        }

    } // namespace algorithms
} // namespace nstd

#endif // NSTD_ALGORITHMS_DIV_KERNELS_HPP
