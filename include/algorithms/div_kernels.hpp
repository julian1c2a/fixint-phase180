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
        template <std::size_t N>
        [[nodiscard]] constexpr std::uint64_t div_un_limbo(const std::array<std::uint64_t, N> &a,
                                                           std::uint64_t d,
                                                           std::array<std::uint64_t, N> &q) noexcept
        {
            std::uint64_t resto = 0;
            for (std::size_t i = N; i-- > 0;)
                q[i] = detail::div_128_64_hi_menor_que_d(resto, a[i], d, resto);
            return resto;
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
        /// @tparam Estimador Como se calcula el digito del cociente. Ver
        ///         `estimador_knuth` y la nota de la cabecera del fichero.
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
        template <std::size_t N, bool Limpiar = true, typename Estimador = estimador_knuth>
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
