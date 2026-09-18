// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// int128 Library - Representation Forms (Phase 1.75)
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0
//
// Copyright (c) 2024-2026 Julián Calderón Almendros
// Email: julian.calderon.almendros@gmail.com
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)
//
// =============================================================================
// @file       representation.hpp
// @brief      Representation forms for 128-bit integers (Phase 1.75)
// @author     Julián Calderón Almendros
// @date       2026-01-11
// @version    1.0.0
// =============================================================================

#ifndef INT128_REPRESENTATION_HPP
#define INT128_REPRESENTATION_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace nstd
{

    // =============================================================================
    // Representation Forms Enumeration
    // =============================================================================

    /**
     * @enum representation_form
     * @brief Enumeration for different binary representations of signed integers
     *
     * @details Specifies how the sign and magnitude are encoded in the binary
     * representation of the integer. This allows the same underlying storage to
     * be interpreted in different ways (two's complement, magnitude-sign, etc.)
     */
    /**
     * @brief Formas de representación para int128_param_t
     * - binnat: binario natural (solo unsigned, sin signo, sin codificación especial)
     * - twos_complement: complemento a dos (solo signed)
     * - magnitude_sign: magnitud y signo (solo signed)
     * - excess_k: exceso-K (solo signed)
     */
    enum class representation_form : std::uint8_t
    {
        /**
         * @brief Binario natural (solo unsigned)
         *
         * No hay codificación de signo ni bias. Simplemente almacena el valor binario puro.
         * Range: [0, 2^128-1]
         * Ejemplo: 42 = 0x2A
         */
        binnat = 0,

        /**
         * @brief Two's Complement (Standard, Phase 1.66)
         *
         * Sign bit at MSB with inversion of remaining bits.
         * Range: [-2^127, 2^127-1]
         * Operations: Optimized via hardware intrinsics
         *
         * Encoding:
         * - Positive: MSB=0, rest=magnitude
         * - Negative: MSB=1, rest=inverted(|magnitude|)+1
         *
         * Example: -1 = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
         */
        twos_complement = 1,

        /**
         * @brief Magnitude-Sign (Phase 1.75 Investigation)
         *
         * Separate sign bit and magnitude representation.
         * Range: [-2^127+1, 2^127-1] (note: -0 differs from +0)
         * Operations: Requires explicit sign handling
         *
         * Encoding:
         * - Sign bit (MSB): 0=positive, 1=negative
         * - Remaining 127 bits: Magnitude (unsigned)
         *
         * Example: -42 = [1|0000...0101010] (sign=1, magnitude=42)
         */
        magnitude_sign = 2,

        /**
         * @brief Excess-k (Bias Notation, Future - Phase 1.75)
         *
         * Used primarily for exponents in IEEE 754 floating point.
         * Range: [-k, 2^127-k]
         * Operations: Add/subtract constant k
         *
         * Encoding:
         * - Value = stored_value - bias_k
         * - Typically used with k=2^(n-1)
         *
         * Example (k=64): stored=100 represents value=36
         */
        excess_k = 3
    };

    // =============================================================================
    // Signedness Enumeration (from Phase 1.66)
    // =============================================================================

    /**
     * @enum signedness
     * @brief Enumeration for signed vs unsigned interpretation
     *
     * @details This is orthogonal to representation_form.
     * For example, both can be combined:
     * - int128_t: signedness::signed_type + representation_form::twos_complement
     * - uint128_t: signedness::unsigned_type + any representation_form
     */
    enum class signedness : bool
    {
        unsigned_type = false, ///< Unsigned: [0, 2^128-1]
        signed_type = true     ///< Signed: dependent on representation_form
    };

    // =============================================================================
    // Representation Form Traits (Metaprogramming)
    // =============================================================================

    /**
     * @struct representation_traits
     * @brief Compile-time traits for representation forms
     *
     * @details Provides static information about each representation form
     * for metaprogramming and optimization decisions.
     *
     * @tparam Form The representation form to query
     */
    template <representation_form Form>
    struct representation_traits;

    /**
     * @brief Specialization for Binario Natural (binnat)
     *
     * @details Era la unica de las cuatro formas SIN especializacion, de modo
     *          que cualquier codigo generico que consultara
     *          `representation_traits<binnat>` no compilaba. Se detecto el
     *          25 ago 2026 al escribir ADR-005 y quedo anotado como cabo suelto.
     *
     *          Su contenido no es una eleccion: lo determina
     *          [ADR-011](../docs/decisions/ADR-011-sin-signo-equivale-a-binnat.md),
     *          que fija que **sin signo y `binnat` son la misma cosa**. Un tipo
     *          sin signo no tiene signo que codificar, asi que no hay bit de
     *          signo, no hay inversion, no hay dos ceros y no hay sesgo; y su
     *          aritmetica es la misma aritmetica modular que la del complemento
     *          a dos, de modo que aprovecha el hardware igual de bien.
     *
     * See ADR-011.
     */
    template <>
    struct representation_traits<representation_form::binnat>
    {
        /// @brief Human-readable name
        static constexpr const char *name = "Binario Natural";

        /// @brief No hay bit de signo: no hay signo que codificar.
        static constexpr bool has_implicit_sign_bit = false;

        /// @brief No hay negativos, luego no hay nada que invertir.
        static constexpr bool uses_inversion = false;

        /// @brief Misma aritmetica modular que el complemento a dos: mismos
        ///        intrinsecos, mismo rendimiento.
        static constexpr bool hardware_optimized = true;

        /// @brief Un solo cero.
        static constexpr bool has_two_zeros = false;

        /// @brief Sin sesgo, al contrario que Exceso-K.
        static constexpr bool uses_bias = false;

        /// @brief Minimo: cero. El rango es [0, 2^n - 1].
        static constexpr std::uint64_t min_u64 = 0ULL;

        /// @brief Maximo de un limbo.
        static constexpr std::uint64_t max_u64 = std::numeric_limits<std::uint64_t>::max();
    };

    /**
     * @brief Specialization for Two's Complement
     */
    template <>
    struct representation_traits<representation_form::twos_complement>
    {
        /// @brief Human-readable name
        static constexpr const char *name = "Two's Complement";

        /// @brief Whether sign bit is implicit (MSB)
        static constexpr bool has_implicit_sign_bit = true;

        /// @brief Whether magnitude needs inversion for negatives
        static constexpr bool uses_inversion = true;

        /// @brief Hardware support (intrinsics available)
        static constexpr bool hardware_optimized = true;

        /// @brief Minimum value for signed interpretation
        static constexpr std::int64_t min_i64 = std::numeric_limits<std::int64_t>::min();

        /// @brief Maximum value for signed interpretation
        static constexpr std::int64_t max_i64 = std::numeric_limits<std::int64_t>::max();
    };

    /**
     * @brief Specialization for Magnitude-Sign
     */
    template <>
    struct representation_traits<representation_form::magnitude_sign>
    {
        /// @brief Human-readable name
        static constexpr const char *name = "Magnitude-Sign";

        /// @brief Whether sign bit is implicit (MSB)
        static constexpr bool has_implicit_sign_bit = true;

        /// @brief Whether magnitude needs inversion for negatives
        static constexpr bool uses_inversion = false; // Direct magnitude

        /// @brief Hardware support
        static constexpr bool hardware_optimized = false; // Requires software

        /// @brief Note: Has both +0 and -0
        static constexpr bool has_two_zeros = true;

        /// @brief Minimum value (excluding -0 duplicate)
        static constexpr std::int64_t min_i64 = -(std::numeric_limits<std::int64_t>::max());

        /// @brief Maximum value
        static constexpr std::int64_t max_i64 = std::numeric_limits<std::int64_t>::max();
    };

    /**
     * @brief Specialization for Excess-k (Bias)
     */
    template <>
    struct representation_traits<representation_form::excess_k>
    {
        /// @brief Human-readable name
        static constexpr const char *name = "Excess-k (Bias)";

        /// @brief Sign bit representation
        static constexpr bool has_implicit_sign_bit = false; // Uses offset

        /// @brief Uses constant offset (bias)
        static constexpr bool uses_bias = true;

        /// @brief Bias high word for 128-bit (K = 2^126, stored as bias_high = 2^62)
        static constexpr std::uint64_t default_bias_high = (1ULL << 62);
        /// @brief Bias low word (always 0)
        static constexpr std::uint64_t default_bias_low = 0ULL;

        /// @brief Hardware support
        static constexpr bool hardware_optimized = false; // Custom bias handling
    };

    // =============================================================================
    // Conversion Functions between Representations
    // =============================================================================

    /**
     * @brief Convert magnitude-sign to two's complement
     *
     * @param magnitude_sign_value Value in magnitude-sign representation
     * @return Equivalent value in two's complement
     */
    inline constexpr std::uint64_t ms_to_twos_complement(std::uint64_t magnitude_sign_value) noexcept
    {
        const bool is_negative = (magnitude_sign_value & (1ULL << 63)) != 0;
        const std::uint64_t magnitude = magnitude_sign_value & ~(1ULL << 63);

        if (!is_negative)
        {
            return magnitude;
        }

        // Two's complement: invert and add 1
        return (~magnitude) + 1;
    }

    /**
     * @brief Convert two's complement to magnitude-sign
     *
     * @param twos_complement_value Value in two's complement
     * @return Equivalent value in magnitude-sign
     */
    inline constexpr std::uint64_t twos_complement_to_ms(std::uint64_t twos_complement_value) noexcept
    {
        const bool is_negative = (twos_complement_value & (1ULL << 63)) != 0;

        if (!is_negative)
        {
            return twos_complement_value;
        }

        // Extract magnitude: negate the two's complement value
        std::uint64_t magnitude = (~twos_complement_value) + 1;

        // Set sign bit
        return magnitude | (1ULL << 63);
    }

    // =============================================================================
    // 128-bit Conversion Functions Between Representations
    // =============================================================================

    // TC <-> MS (128 bits)
    inline constexpr void ms128_to_twos_complement(uint64_t ms_high, uint64_t ms_low, uint64_t &tc_high,
                                                   uint64_t &tc_low) noexcept
    {
        // MS: sign bit is MSB of high
        const bool is_negative = (ms_high & (1ULL << 63)) != 0;
        const uint64_t mag_high = ms_high & ~(1ULL << 63);
        if (!is_negative)
        {
            tc_high = mag_high;
            tc_low = ms_low;
        }
        else
        {
            // Two's complement: invert and add 1 (128 bits)
            tc_high = ~mag_high;
            tc_low = ~ms_low;
            // Add 1 (handle carry)
            if (++tc_low == 0)
                ++tc_high;
        }
    }

    inline constexpr void twos_complement128_to_ms(uint64_t tc_high, uint64_t tc_low, uint64_t &ms_high,
                                                   uint64_t &ms_low) noexcept
    {
        const bool is_negative = (tc_high & (1ULL << 63)) != 0;
        if (!is_negative)
        {
            ms_high = tc_high;
            ms_low = tc_low;
        }
        else
        {
            // Negate (128 bits)
            uint64_t mag_low = ~tc_low + 1;
            uint64_t mag_high = ~tc_high + (mag_low == 0 ? 1 : 0);
            // Set sign bit
            ms_high = (mag_high & ~(1ULL << 63)) | (1ULL << 63);
            ms_low = mag_low;
        }
    }

    // TC <-> EK (128 bits)
    inline constexpr void twos_complement128_to_excess_k(uint64_t tc_high, uint64_t tc_low, uint64_t &ek_high,
                                                         uint64_t &ek_low) noexcept
    {
        // EK: stored = value + bias
        constexpr uint64_t bias_high = (1ULL << 62);
        constexpr uint64_t bias_low = 0;
        // Add bias (128 bits)
        ek_low = tc_low + bias_low;
        ek_high = tc_high + bias_high + (ek_low < tc_low ? 1 : 0);
    }

    inline constexpr void excess_k128_to_twos_complement(uint64_t ek_high, uint64_t ek_low, uint64_t &tc_high,
                                                         uint64_t &tc_low) noexcept
    {
        constexpr uint64_t bias_high = (1ULL << 62);
        constexpr uint64_t bias_low = 0;
        // Subtract bias (128 bits)
        tc_low = ek_low - bias_low;
        tc_high = ek_high - bias_high - (ek_low < bias_low ? 1 : 0);
    }

    // MS <-> EK (128 bits)
    inline constexpr void ms128_to_excess_k(uint64_t ms_high, uint64_t ms_low, uint64_t &ek_high,
                                            uint64_t &ek_low) noexcept
    {
        uint64_t tc_high, tc_low;
        ms128_to_twos_complement(ms_high, ms_low, tc_high, tc_low);
        twos_complement128_to_excess_k(tc_high, tc_low, ek_high, ek_low);
    }

    inline constexpr void excess_k128_to_ms(uint64_t ek_high, uint64_t ek_low, uint64_t &ms_high,
                                            uint64_t &ms_low) noexcept
    {
        uint64_t tc_high, tc_low;
        excess_k128_to_twos_complement(ek_high, ek_low, tc_high, tc_low);
        twos_complement128_to_ms(tc_high, tc_low, ms_high, ms_low);
    }

    // =========================================================================
    // Las mismas conversiones, pero para N limbos (P1.5 tramo 3)
    // =========================================================================
    //
    // Las de arriba estan fijadas a 128 bits porque nacieron para
    // `int128_param_t`. `fixed_int_t<N, ...>` necesita las mismas para cualquier
    // N, y son la pieza sobre la que se apoya el porte de Magnitud-Signo y
    // Exceso-K (ADR-006).
    //
    // POR QUE ESTO BASTA, Y NO HACE FALTA ARITMETICA NUEVA
    // ----------------------------------------------------
    // Magnitud-Signo y Exceso-K son **codificaciones**, no aritmeticas. Sumar dos
    // numeros en MS no se hace «sumando en MS»: se decodifica a complemento a
    // dos, se suma con el codigo que ya existe, y se recodifica. Es exactamente
    // lo que hace `int128_param_t`, y es lo que evita duplicar los 41 puntos del
    // tipo que hoy deciden por el signo.
    //
    // EL SESGO DE EXCESO-K: 2^(64N-1), Y NO EL DE `int128_param_t`
    // ------------------------------------------------------------
    // `representation_traits<excess_k>::default_bias_high` vale `1ULL << 62`, o
    // sea un sesgo de **2^126** para 128 bits. Comprobado el 18 sep 2026, ese
    // valor da un rango asimetrico que **ni siquiera llega a -2^127**:
    //
    //     sesgo 2^126 -> [-2^126, 3*2^126 - 1]   asimetrico, no cubre int128
    //     sesgo 2^127 -> [-2^127,   2^127 - 1]   exactamente el rango de int128
    //
    // Aqui se usa **2^(64N-1)**, que es el canonico y el unico que hace de
    // Exceso-K una biyeccion con el rango con signo de la misma anchura. El 2^126
    // de `int128_param_t` se queda como esta: ese tipo se retira por ADR-006 y
    // cambiarselo ahora romperia su propia paridad sin ganar nada.
    //
    // Con ese sesgo, Exceso-K es **el complemento a dos con el bit alto
    // invertido**, que es la identidad conocida entre complemento a dos y
    // «offset binary». De ahi que las dos conversiones sean la misma funcion.

    namespace repr
    {
        /// @brief `true` si el limbo alto tiene su bit mas significativo a uno.
        template <std::size_t N>
        [[nodiscard]] constexpr bool bit_de_signo(const std::array<std::uint64_t, N> &x) noexcept
        {
            return (x[N - 1] >> 63) != 0;
        }

        /// @brief Niega en complemento a dos, en su sitio: invertir y sumar uno.
        template <std::size_t N>
        constexpr void niega_c2(std::array<std::uint64_t, N> &x) noexcept
        {
            std::uint64_t acarreo = 1;
            for (std::size_t i = 0; i < N; ++i)
            {
                const std::uint64_t inv = ~x[i];
                x[i] = inv + acarreo;
                acarreo = (x[i] < acarreo) ? 1U : 0U;
            }
        }

        /// @brief Magnitud-Signo -> complemento a dos.
        ///
        /// En MS el bit alto es el signo y el resto es la **magnitud**, que es un
        /// natural. Si el signo esta puesto, se quita y se niega.
        ///
        /// @note `-0` y `+0` son valores distintos en MS y los dos van a cero en
        ///       complemento a dos. **La conversion no es inyectiva**, y por eso
        ///       la vuelta no devuelve siempre el mismo patron de bits: ver
        ///       `c2_a_ms`.
        template <std::size_t N>
        constexpr void ms_a_c2(std::array<std::uint64_t, N> &x) noexcept
        {
            const bool negativo = bit_de_signo<N>(x);
            x[N - 1] &= ~(std::uint64_t{1} << 63); // quitar el bit de signo
            if (negativo)
                niega_c2<N>(x);
        }

        /// @brief Complemento a dos -> Magnitud-Signo.
        ///
        /// @warning El minimo de complemento a dos --`-2^(64N-1)`-- **no tiene
        ///          representacion en Magnitud-Signo**: su magnitud es `2^(64N-1)`
        ///          y no cabe junto al bit de signo. Aqui se satura al mas
        ///          negativo representable, que es `-(2^(64N-1) - 1)`. Es la
        ///          asimetria clasica, y es informacion que se pierde: quien
        ///          convierta de ida y vuelta por complemento a dos tiene que
        ///          saberlo.
        template <std::size_t N>
        constexpr void c2_a_ms(std::array<std::uint64_t, N> &x) noexcept
        {
            if (!bit_de_signo<N>(x))
                return; // positivo: los dos lo escriben igual

            // ¿Es el minimo? En complemento a dos es el unico negativo cuyo
            // negado es el mismo.
            bool es_el_minimo = (x[N - 1] == (std::uint64_t{1} << 63));
            for (std::size_t i = 0; i + 1 < N && es_el_minimo; ++i)
                es_el_minimo = (x[i] == 0);

            niega_c2<N>(x); // ahora x es la magnitud
            if (es_el_minimo)
            {
                // Saturar: la magnitud seria 2^(64N-1), que pisa el bit de signo.
                for (std::size_t i = 0; i + 1 < N; ++i)
                    x[i] = ~std::uint64_t{0};
                x[N - 1] = (std::uint64_t{1} << 63) - 1;
            }
            x[N - 1] |= (std::uint64_t{1} << 63); // poner el signo
        }

        /// @brief Complemento a dos <-> Exceso-K, con sesgo `2^(64N-1)`.
        ///
        /// Con ese sesgo la operacion es **invertir el bit alto**, en los dos
        /// sentidos: es la identidad entre complemento a dos y «offset binary».
        /// Por eso hay una sola funcion y no dos.
        template <std::size_t N>
        constexpr void c2_ek_ida_y_vuelta(std::array<std::uint64_t, N> &x) noexcept
        {
            x[N - 1] ^= (std::uint64_t{1} << 63);
        }

        /// @brief Lleva `x` de la representacion `Desde` a complemento a dos.
        template <representation_form Desde, std::size_t N>
        constexpr void a_c2(std::array<std::uint64_t, N> &x) noexcept
        {
            if constexpr (Desde == representation_form::magnitude_sign)
                ms_a_c2<N>(x);
            else if constexpr (Desde == representation_form::excess_k)
                c2_ek_ida_y_vuelta<N>(x);
            // binnat y twos_complement ya estan en el formato que usa la
            // aritmetica: no se toca nada.
        }

        /// @brief Lleva `x` de complemento a dos a la representacion `Hacia`.
        template <representation_form Hacia, std::size_t N>
        constexpr void desde_c2(std::array<std::uint64_t, N> &x) noexcept
        {
            if constexpr (Hacia == representation_form::magnitude_sign)
                c2_a_ms<N>(x);
            else if constexpr (Hacia == representation_form::excess_k)
                c2_ek_ida_y_vuelta<N>(x);
        }

    } // namespace repr

} // namespace nstd

#endif // INT128_REPRESENTATION_HPP
