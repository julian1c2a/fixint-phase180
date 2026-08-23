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
// @file       int128_param_traits.hpp
// @brief      Additional type traits for int128_param_t<Sign, Form>
// @author     Julián Calderón Almendros
// @date       2026-02-04 (last edit)
// @version    1.0.0
// =============================================================================

/**
 * @file int128_param_traits.hpp
 * @brief Additional STL-style traits for parameterized 128-bit integers
 *
 * @details This module provides:
 * - std::common_type specializations for mixed-type operations
 * - std::hash specializations for unordered containers (see traits_specializations.hpp)
 * - Type promotion rules for arithmetic operations
 *
 * @par Common Type Rules:
 * - Same type → Same type
 * - int128 + int128 (different forms) → Two's Complement (standard form)
 * - int128 + builtin integral → int128 (wider type)
 * - unsigned + signed → Larger signed type (standard C++ promotion)
 *
 * @par Dependencies:
 * - int128_parameterized.hpp (core type)
 * - `<type_traits>` (STL traits)
 * - `<utility>` (std::declval)
 *
 * @par Example:
 * @code
 * #include "int128_param_traits.hpp"
 *
 * using CT1 = std::common_type_t<uint128_t, int128_tc_t>;  // int128_tc_t
 * using CT2 = std::common_type_t<int128_ms_t, int64_t>;    // int128_ms_t
 * @endcode
 */

#ifndef INT128_PARAM_TRAITS_HPP
#define INT128_PARAM_TRAITS_HPP

#include "int128_parameterized.hpp"
#include <type_traits>
#include <utility>

namespace std
{

    // =============================================================================
    // COMMON_TYPE SPECIALIZATIONS
    // =============================================================================

    /**
     * @brief Common type for two int128 types (same signedness and form)
     *
     * @details When both types are identical, common_type is the same type.
     * This specialization handles the trivial case.
     */
    template <nstd::signedness Sign, nstd::representation_form Form>
    struct common_type<nstd::int128_param_t<Sign, Form>, nstd::int128_param_t<Sign, Form>>
    {
        using type = nstd::int128_param_t<Sign, Form>;
    };

    /**
     * @brief Common type for int128 (any form) + int128 (any form)
     *
     * @details When combining different representation forms:
     * - Result form: Two's Complement (standard form, most compatible)
     * - Result signedness: Determined by standard promotion rules
     *   - unsigned + unsigned → unsigned
     *   - signed + signed → signed
     *   - unsigned + signed → signed (standard C++ rule)
     *
     * @note Two's Complement is chosen because:
     * - Hardware-optimized on all modern platforms
     * - Standard signed representation in C/C++
     * - Direct hardware instruction mapping
     */
    template <nstd::signedness Sign1, nstd::representation_form Form1, nstd::signedness Sign2,
              nstd::representation_form Form2>
    struct common_type<nstd::int128_param_t<Sign1, Form1>, nstd::int128_param_t<Sign2, Form2>>
    {
        // Determine signedness using standard C++ promotion rules
        static constexpr nstd::signedness result_sign =
            (Sign1 == nstd::signedness::signed_type || Sign2 == nstd::signedness::signed_type)
                ? nstd::signedness::signed_type
                : nstd::signedness::unsigned_type;

        // Always use Two's Complement for mixed-form operations
        using type = nstd::int128_param_t<result_sign, nstd::representation_form::twos_complement>;
    };

    /**
     * @brief Common type for int128 with cv-qualifiers (strips cv)
     *
     * @details Handles const/volatile int128 by stripping cv-qualifiers
     * and forwarding to the base int128+int128 specialization.
     */
    template <nstd::signedness Sign1, nstd::representation_form Form1, nstd::signedness Sign2,
              nstd::representation_form Form2>
    struct common_type<const nstd::int128_param_t<Sign1, Form1>, nstd::int128_param_t<Sign2, Form2>>
        : common_type<nstd::int128_param_t<Sign1, Form1>, nstd::int128_param_t<Sign2, Form2>>
    {
    };

    template <nstd::signedness Sign1, nstd::representation_form Form1, nstd::signedness Sign2,
              nstd::representation_form Form2>
    struct common_type<volatile nstd::int128_param_t<Sign1, Form1>, nstd::int128_param_t<Sign2, Form2>>
        : common_type<nstd::int128_param_t<Sign1, Form1>, nstd::int128_param_t<Sign2, Form2>>
    {
    };

    template <nstd::signedness Sign1, nstd::representation_form Form1, nstd::signedness Sign2,
              nstd::representation_form Form2>
    struct common_type<nstd::int128_param_t<Sign1, Form1>, const nstd::int128_param_t<Sign2, Form2>>
        : common_type<nstd::int128_param_t<Sign1, Form1>, nstd::int128_param_t<Sign2, Form2>>
    {
    };

    template <nstd::signedness Sign1, nstd::representation_form Form1, nstd::signedness Sign2,
              nstd::representation_form Form2>
    struct common_type<nstd::int128_param_t<Sign1, Form1>, volatile nstd::int128_param_t<Sign2, Form2>>
        : common_type<nstd::int128_param_t<Sign1, Form1>, nstd::int128_param_t<Sign2, Form2>>
    {
    };

    /**
     * @brief Common type for int128 + builtin integral
     *
     * @details When combining int128 with builtin integral types:
     * - Result type: int128 (wider type)
     * - Result signedness: Determined by standard promotion rules
     * - Result form: Preserve int128's representation form
     *
     * @note Builtin integral promotes to int128, not the other way around.
     * @note cv-qualifiers are stripped from T before type checking.
     */
    template <nstd::signedness Sign, nstd::representation_form Form, typename T>
    struct common_type<nstd::int128_param_t<Sign, Form>, T>
    {
        using T_bare = std::remove_cv_t<T>;
        static_assert(std::is_integral_v<T_bare>, "common_type with non-integral type not supported");

        // Determine signedness using standard C++ promotion rules
        static constexpr nstd::signedness result_sign =
            (Sign == nstd::signedness::signed_type || std::is_signed_v<T_bare>)
                ? nstd::signedness::signed_type
                : nstd::signedness::unsigned_type;

        // Preserve int128's representation form
        using type = nstd::int128_param_t<result_sign, Form>;
    };

    /**
     * @brief Common type for builtin integral + int128 (symmetric)
     *
     * @details This is the symmetric version of the above specialization.
     * The result is the same regardless of argument order.
     * cv-qualifiers are automatically handled by forwarding.
     */
    template <typename T, nstd::signedness Sign, nstd::representation_form Form>
    struct common_type<T, nstd::int128_param_t<Sign, Form>> : common_type<nstd::int128_param_t<Sign, Form>, T>
    {
    };

    // =============================================================================
    // ADDITIONAL TRAIT HELPERS (optional, for completeness)
    // =============================================================================

    /**
     * @brief Helper to check if a type is any int128_param_t instantiation
     */
    template <typename T>
    struct is_int128_param : std::false_type
    {
    };

    template <nstd::signedness Sign, nstd::representation_form Form>
    struct is_int128_param<nstd::int128_param_t<Sign, Form>> : std::true_type
    {
    };

    /**
     * @brief Helper variable template for is_int128_param
     */
    template <typename T>
    inline constexpr bool is_int128_param_v = is_int128_param<T>::value;

} // namespace std

namespace nstd
{

    // =============================================================================
    // NSTD NAMESPACE MIRRORS (for consistency with traits_specializations.hpp)
    // =============================================================================

    /**
     * @brief Mirror std::common_type in nstd namespace for consistency
     *
     * @details This allows users to use nstd::common_type instead of std::common_type
     * when working with int128 types, maintaining consistency with the library's
     * trait specializations pattern.
     */
    using std::common_type;

    /**
     * @brief Mirror std::common_type_t helper in nstd namespace
     */
    template <typename... Ts>
    using common_type_t = typename std::common_type<Ts...>::type;

    /**
     * @brief Mirror is_int128_param in nstd namespace
     */
    using std::is_int128_param;
    using std::is_int128_param_v;

} // namespace nstd

#endif // INT128_PARAM_TRAITS_HPP
