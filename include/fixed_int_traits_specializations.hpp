// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// int128 Library - 128-bit Integer Types for C++20
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
// @file       fixed_int_traits_specializations.hpp
// @brief      Traits nstd::is_* y std::common_type para fixed_int_t
// @author     Julián Calderón Almendros
// @date       2026-08-23 (last edit)
// @version    1.90.0
// =============================================================================

// =============================================================================
// fixed_int_traits_specializations.hpp — Type traits & common_type for fixed_int_t
// Part of int128 Library - Phase 1.81 (Fase MS-INTEROP, T3+T4)
// License: BSL-1.0
// =============================================================================
//
// Provides:
//   - nstd::is_integral / is_arithmetic / is_signed / is_unsigned   (specialized for fixed_int_t)
//   - nstd::make_signed / make_unsigned                             (specialized for fixed_int_t)
//   - std::common_type<fixed_int_t..., fixed_int_t...>              (allowed by std)
//   - std::common_type<fixed_int_t..., T>                           (with built-in integral T)
//
// WHY nstd:: instead of std::?
//   The C++ standard prohibits specializing std::is_integral, std::is_arithmetic,
//   std::is_signed, std::is_unsigned, std::make_signed, std::make_unsigned for
//   user-defined types. We therefore provide nstd:: equivalents that delegate to
//   std:: for built-in types and add specializations for fixed_int_t.
//
//   std::common_type IS specializable for user types (§22.10.7.6) — we use std::
//   directly for it.
//
// CONFLICT WITH int128_param_traits_specializations.hpp:
//   That header defines the same primary nstd:: templates. To avoid ODR
//   conflicts when both headers are included in the same TU, we guard the
//   primary template definitions with a shared macro NSTD_TRAITS_PRIMARY_DEFINED.

#ifndef FIXED_INT_TRAITS_SPECIALIZATIONS_HPP
#define FIXED_INT_TRAITS_SPECIALIZATIONS_HPP

#include "fixed_width_int_t.hpp"

#include <type_traits>

// libc++ defines is_integral etc. for __int128 internally; our nstd:: primaries
// must still work but the _v helpers may need different handling.
#if defined(_LIBCPP_VERSION)
#define FIXED_INT_USING_LIBCPP 1
#else
#define FIXED_INT_USING_LIBCPP 0
#endif

namespace nstd
{

    // =============================================================================
    // Primary nstd:: trait templates — define ONCE per TU (shared with int128_param)
    // =============================================================================
    //
    // If int128_param_traits_specializations.hpp was included first (header guard
    // INT128_PARAM_TRAITS_SPECIALIZATIONS_HPP set), its primaries are already in
    // scope and we skip ours. Otherwise we define them here. Either way the
    // fixed_int_t specializations below see a valid primary template.

#if !FIXED_INT_USING_LIBCPP && !defined(INT128_PARAM_TRAITS_SPECIALIZATIONS_HPP) && \
    !defined(NSTD_TRAITS_PRIMARY_DEFINED)
#define NSTD_TRAITS_PRIMARY_DEFINED 1

    template <typename T>
    struct is_integral : std::is_integral<T>
    {
    };
    template <typename T>
    struct is_arithmetic : std::is_arithmetic<T>
    {
    };
    template <typename T>
    struct is_signed : std::is_signed<T>
    {
    };
    template <typename T>
    struct is_unsigned : std::is_unsigned<T>
    {
    };

    template <typename T>
    inline constexpr bool is_integral_v = is_integral<T>::value;
    template <typename T>
    inline constexpr bool is_arithmetic_v = is_arithmetic<T>::value;
    template <typename T>
    inline constexpr bool is_signed_v = is_signed<T>::value;
    template <typename T>
    inline constexpr bool is_unsigned_v = is_unsigned<T>::value;

#endif // !FIXED_INT_USING_LIBCPP && !INT128_PARAM_TRAITS_SPECIALIZATIONS_HPP && !NSTD_TRAITS_PRIMARY_DEFINED

    // =============================================================================
    // Specializations for fixed_int_t<N, Sign, Form>
    // =============================================================================

#if !FIXED_INT_USING_LIBCPP

    template <std::size_t N, signedness S, representation_form F>
    struct is_integral<fixed_int_t<N, S, F>> : std::true_type
    {
    };

    template <std::size_t N, signedness S, representation_form F>
    struct is_arithmetic<fixed_int_t<N, S, F>> : std::true_type
    {
    };

    template <std::size_t N, representation_form F>
    struct is_signed<fixed_int_t<N, signedness::signed_type, F>> : std::true_type
    {
    };

    template <std::size_t N, representation_form F>
    struct is_signed<fixed_int_t<N, signedness::unsigned_type, F>> : std::false_type
    {
    };

    template <std::size_t N, representation_form F>
    struct is_unsigned<fixed_int_t<N, signedness::unsigned_type, F>> : std::true_type
    {
    };

    template <std::size_t N, representation_form F>
    struct is_unsigned<fixed_int_t<N, signedness::signed_type, F>> : std::false_type
    {
    };

#endif // !FIXED_INT_USING_LIBCPP

    // =============================================================================
    // make_signed / make_unsigned — always-available specializations
    // =============================================================================
    //
    // If int128_param_traits_specializations.hpp already defined the primary
    // nstd::make_signed / make_unsigned templates, we just add our specializations.
    // Otherwise we define the primaries here too.

#if !defined(INT128_PARAM_TRAITS_SPECIALIZATIONS_HPP) && !defined(NSTD_MAKE_SIGNED_PRIMARY_DEFINED)
#define NSTD_MAKE_SIGNED_PRIMARY_DEFINED 1

    /// @brief Version de `std::make_signed` que **tambien acepta**
    ///        `fixed_int_t`.
    ///
    /// El estandar prohibe especializar `std::make_signed` para tipos que no son
    /// enteros del lenguaje, asi que la biblioteca ofrece la suya en `nstd`. Para
    /// los tipos del lenguaje delega en la del estandar.
    ///
    /// @tparam T Entero del lenguaje o `fixed_int_t`.
    template <typename T>
    struct make_signed
    {
        using type = std::make_signed_t<T>; ///< El tipo resultante.
    };

    /// @brief Version de `std::make_unsigned` que tambien acepta `fixed_int_t`.
    /// @tparam T Entero del lenguaje o `fixed_int_t`.
    template <typename T>
    struct make_unsigned
    {
        using type = std::make_unsigned_t<T>; ///< El tipo resultante.
    };

    /// @brief Atajo de `make_signed<T>::type`.
    template <typename T>
    using make_signed_t = typename make_signed<T>::type;

    /// @brief Atajo de `make_unsigned<T>::type`.
    template <typename T>
    using make_unsigned_t = typename make_unsigned<T>::type;

#endif

    // fixed_int_t specializations (default Form for the produced type follows the
    // canonical alias: binnat for unsigned, twos_complement for signed).
    /// @brief `make_signed` de un `fixed_int_t` sin signo: da `int_fixed_t<N>`,
    ///        con la misma anchura y la representacion canonica con signo.
    template <std::size_t N, representation_form F>
    struct make_signed<fixed_int_t<N, signedness::unsigned_type, F>>
    {
        using type = int_fixed_t<N>; ///< El tipo resultante.
    };

    /// @brief `make_signed` de un tipo que ya tiene signo: identidad.
    template <std::size_t N, representation_form F>
    struct make_signed<fixed_int_t<N, signedness::signed_type, F>>
    {
        using type = fixed_int_t<N, signedness::signed_type, F>; ///< El propio tipo.
    };

    /// @brief `make_unsigned` de un `fixed_int_t` con signo: da
    ///        `uint_fixed_t<N>`, misma anchura, representacion `binnat`.
    template <std::size_t N, representation_form F>
    struct make_unsigned<fixed_int_t<N, signedness::signed_type, F>>
    {
        using type = uint_fixed_t<N>; ///< El tipo resultante.
    };

    /// @brief `make_unsigned` de un tipo que ya es sin signo: identidad.
    template <std::size_t N, representation_form F>
    struct make_unsigned<fixed_int_t<N, signedness::unsigned_type, F>>
    {
        using type = fixed_int_t<N, signedness::unsigned_type, F>; ///< El propio tipo.
    };

} // namespace nstd

// =============================================================================
// std::common_type specializations — allowed by §22.10.7.6 for user types
// =============================================================================
//
// Rules (mirror C++ usual arithmetic conversions):
//   common_type<int_fixed_t<N>,  int_fixed_t<M>>  = int_fixed_t<max(N,M)>
//   common_type<uint_fixed_t<N>, uint_fixed_t<M>> = uint_fixed_t<max(N,M)>
//   common_type<int_fixed_t<N>,  uint_fixed_t<M>> = nstd::mixed_iu_t<N,M>
//   common_type<uint_fixed_t<N>, int_fixed_t<M>>  = nstd::mixed_iu_t<M,N>
//   common_type<fixed_int_t<N,Sign,Form>, T>     = fixed_int_t<N,Sign,Form>  (T integral built-in)
//
// We specialize on the canonical alias pairs (binnat/twos_complement). Other
// Form combinations fall through to the primary template (undefined) for now;
// MS/EK extensions go here when those Forms gain support for fixed_int_t.

namespace std
{
    /// @brief `std::common_type` de dos `int_fixed_t`: gana la anchura mayor.
    template <std::size_t N, std::size_t M>
    struct common_type<::nstd::int_fixed_t<N>, ::nstd::int_fixed_t<M>>
    {
        using type = ::nstd::int_fixed_t<(N > M ? N : M)>; ///< El mas ancho de los dos.
    };

    /// @brief `std::common_type` de dos `uint_fixed_t`: gana la anchura mayor.
    template <std::size_t N, std::size_t M>
    struct common_type<::nstd::uint_fixed_t<N>, ::nstd::uint_fixed_t<M>>
    {
        using type = ::nstd::uint_fixed_t<(N > M ? N : M)>; ///< El mas ancho de los dos.
    };

    /// @brief `std::common_type` de con signo y sin signo, por las conversiones
    ///        aritmeticas usuales de C++: si el sin signo es igual o mas ancho,
    ///        **gana el sin signo**. Ver `nstd::mixed_iu_t`.
    template <std::size_t N, std::size_t M>
    struct common_type<::nstd::int_fixed_t<N>, ::nstd::uint_fixed_t<M>>
    {
        using type = ::nstd::mixed_iu_t<N, M>; ///< El tipo resultante.
    };

    /// @brief La orientacion contraria de la anterior, que da el mismo tipo.
    template <std::size_t N, std::size_t M>
    struct common_type<::nstd::uint_fixed_t<N>, ::nstd::int_fixed_t<M>>
    {
        using type = ::nstd::mixed_iu_t<M, N>; ///< El tipo resultante.
    };

    /// @brief `std::common_type` de un `fixed_int_t` con un entero del lenguaje:
    ///        gana el `fixed_int_t`, que siempre es igual o mas ancho.
    /// @tparam T Entero del lenguaje distinto de `bool`; si no, falla el
    ///         `static_assert` con un mensaje que lo dice.
    template <std::size_t N, ::nstd::signedness S, ::nstd::representation_form F, typename T>
    struct common_type<::nstd::fixed_int_t<N, S, F>, T>
    {
        static_assert(std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>,
                      "common_type<fixed_int_t, T>: T must be a built-in integral (not bool)");
        using type = ::nstd::fixed_int_t<N, S, F>; ///< El lado `fixed_int_t`.
    };

    /// @brief La orientacion contraria de la anterior, que da el mismo tipo.
    template <typename T, std::size_t N, ::nstd::signedness S, ::nstd::representation_form F>
    struct common_type<T, ::nstd::fixed_int_t<N, S, F>>
    {
        static_assert(std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>,
                      "common_type<T, fixed_int_t>: T must be a built-in integral (not bool)");
        using type = ::nstd::fixed_int_t<N, S, F>; ///< El lado `fixed_int_t`.
    };

} // namespace std

#endif // FIXED_INT_TRAITS_SPECIALIZATIONS_HPP
