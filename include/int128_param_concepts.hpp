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
// @file       int128_param_concepts.hpp
// @brief      C++20 concepts for parameterized 128-bit integer types
// @author     Julián Calderón Almendros
// @date       2026-02-04 (last edit)
// @version    1.0.0
// =============================================================================

#ifndef INT128_PARAM_CONCEPTS_HPP
#define INT128_PARAM_CONCEPTS_HPP

/**
 * @file int128_param_concepts.hpp
 * @brief C++20 concepts for int128_param_t<Sign, Form>
 *
 * This header defines C++20 concepts that work with the parameterized
 * template int128_param_t<signedness Sign, representation_form Form>,
 * including:
 * - Type detection concepts (is_int128, is_uint128, is_128bit_type)
 * - Conversion and compatibility concepts
 * - Bitwise and shift operation concepts
 * - Algorithm and container concepts
 * - Static conformance checks for STL compatibility
 */

#include "int128_parameterized.hpp"
#include "representation.hpp"
#include <concepts>
#include <type_traits>

namespace nstd
{

    // =============================================================================
    // HELPER TRAITS FOR TYPE DETECTION
    // =============================================================================

    namespace detail
    {

        /**
         * @brief Detects if a type is an instance of int128_param_t
         */
        template <typename T>
        struct is_int128_param_impl : std::false_type
        {
        };

        template <signedness Sign, representation_form Form>
        struct is_int128_param_impl<int128_param_t<Sign, Form>> : std::true_type
        {
        };

        /**
         * @brief Detects if a type is exactly uint128_t (unsigned binnat)
         */
        template <typename T>
        struct is_uint128_impl : std::bool_constant<
                                     std::is_same_v<std::remove_cv_t<T>, uint128_t>>
        {
        };

        /**
         * @brief Detects if a type is exactly int128_tc_t (signed TC)
         */
        template <typename T>
        struct is_int128_tc_impl : std::bool_constant<
                                       std::is_same_v<std::remove_cv_t<T>, int128_tc_t>>
        {
        };

        /**
         * @brief Detects if a type is exactly int128_ms_t (signed MS)
         */
        template <typename T>
        struct is_int128_ms_impl : std::bool_constant<
                                       std::is_same_v<std::remove_cv_t<T>, int128_ms_t>>
        {
        };

        /**
         * @brief Detects if a type is exactly int128_ek_t (signed EK)
         */
        template <typename T>
        struct is_int128_ek_impl : std::bool_constant<
                                       std::is_same_v<std::remove_cv_t<T>, int128_ek_t>>
        {
        };

        /**
         * @brief Detects if a type is any signed int128 variant (TC/MS/EK)
         */
        template <typename T>
        struct is_signed_int128_impl : std::bool_constant<
                                           is_int128_tc_impl<T>::value ||
                                           is_int128_ms_impl<T>::value ||
                                           is_int128_ek_impl<T>::value>
        {
        };

    } // namespace detail

    // =============================================================================
    // PUBLIC TYPE TRAITS
    // =============================================================================

    /**
     * @brief True if T is any instance of int128_param_t<Sign, Form>
     */
    template <typename T>
    inline constexpr bool is_128bit_type_v =
        detail::is_int128_param_impl<std::remove_cv_t<T>>::value;

    /**
     * @brief True if T is exactly uint128_t (unsigned binnat)
     */
    template <typename T>
    inline constexpr bool is_uint128_v = detail::is_uint128_impl<T>::value;

    /**
     * @brief True if T is exactly int128_tc_t (Two's Complement)
     */
    template <typename T>
    inline constexpr bool is_int128_tc_v = detail::is_int128_tc_impl<T>::value;

    /**
     * @brief True if T is exactly int128_ms_t (Magnitude-Sign)
     */
    template <typename T>
    inline constexpr bool is_int128_ms_v = detail::is_int128_ms_impl<T>::value;

    /**
     * @brief True if T is exactly int128_ek_t (Excess-K)
     */
    template <typename T>
    inline constexpr bool is_int128_ek_v = detail::is_int128_ek_impl<T>::value;

    /**
     * @brief True if T is any signed 128-bit type (TC/MS/EK)
     */
    template <typename T>
    inline constexpr bool is_signed_int128_v = detail::is_signed_int128_impl<T>::value;

    // =============================================================================
    // BASIC CONCEPTS
    // =============================================================================

    /**
     * @brief Concept for types that are instances of int128_param_t
     */
    template <typename T>
    concept int128_type = is_128bit_type_v<T>;

    /**
     * @brief Concept for types that are uint128_t specifically
     */
    template <typename T>
    concept uint128_type = is_uint128_v<T>;

    /**
     * @brief Concept for types that are any signed int128 variant (TC/MS/EK)
     */
    template <typename T>
    concept signed_int128_type = is_signed_int128_v<T>;

    /**
     * @brief Concept for types that are int128_tc_t specifically
     */
    template <typename T>
    concept int128_tc_type = is_int128_tc_v<T>;

    /**
     * @brief Concept for types that are int128_ms_t specifically
     */
    template <typename T>
    concept int128_ms_type = is_int128_ms_v<T>;

    /**
     * @brief Concept for types that are int128_ek_t specifically
     */
    template <typename T>
    concept int128_ek_type = is_int128_ek_v<T>;

    /**
     * @brief Concept for types convertible to int128_param_t<Sign, Form>
     *
     * @tparam T Type to check
     *
     * A type satisfies this concept if it's a standard integral type
     * or an instance of int128_param_t<Sign, Form>.
     */
    template <typename T>
    concept int128_convertible = std::integral<T> || int128_type<T>;

    /**
     * @brief Concept for integral types including int128_param_t
     *
     * @tparam T Type to check
     *
     * Extends std::integral to include uint128_t and all signed variants.
     * Use instead of std::integral when 128-bit support is needed.
     */
    template <typename T>
    concept integral = std::integral<T> || int128_type<T>;

    /**
     * @brief Concept for types compatible with int128_param_t operations
     *
     * @tparam T Type to check
     *
     * Includes all convertible types and also floating-point types
     * for mixed operations.
     */
    template <typename T>
    concept int128_compatible = int128_convertible<T> || std::floating_point<T>;

    /**
     * @brief Concept for types usable with bitwise operations
     *
     * @tparam T Type to check
     *
     * Only integral types can perform safe bitwise operations.
     */
    template <typename T>
    concept int128_bitwise_compatible = std::integral<T> || int128_type<T>;

    /**
     * @brief Concept for signed integral types compatible with int128
     *
     * @tparam T Type to check
     *
     * Verifies that the type is a standard signed integer or any signed int128.
     */
    template <typename T>
    concept int128_signed_compatible = std::signed_integral<T> || is_signed_int128_v<T>;

    /**
     * @brief Concept for unsigned integral types compatible with int128
     *
     * @tparam T Type to check
     *
     * Verifies that the type is a standard unsigned integer or uint128_t.
     */
    template <typename T>
    concept int128_unsigned_compatible = std::unsigned_integral<T> || is_uint128_v<T>;

    // =============================================================================
    // OPERATION-SPECIFIC CONCEPTS
    // =============================================================================

    /**
     * @brief Concept for valid shift operation operands
     *
     * @tparam T Type of the shift operand
     *
     * Shift operands must be small integral types (int, unsigned int, etc.)
     */
    template <typename T>
    concept shift_operand = std::integral<T> && (sizeof(T) <= sizeof(int));

    /**
     * @brief Concept for types valid in arithmetic operations with int128
     *
     * @tparam T Type to check
     *
     * Arithmetic operations (+, -, *, /, %) support mixing integral and
     * floating-point types with int128.
     */
    template <typename T>
    concept arithmetic_operand = int128_convertible<T> || std::floating_point<T>;

    /**
     * @brief Concept for types valid in comparison operations
     *
     * @tparam T Type to check
     *
     * Comparison operators support all int128_compatible types.
     */
    template <typename T>
    concept comparable_with_int128 = int128_compatible<T>;

    /**
     * @brief Concept for types that can be accumulated
     *
     * @tparam T Type to check
     *
     * Accumulation requires arithmetic operations and default construction.
     */
    template <typename T>
    concept accumulative_type =
        std::default_initializable<T> &&
        requires(T a, T b) {
            { a + b } -> std::convertible_to<T>;
        };

    /**
     * @brief Concept for types that support bitwise rotation
     *
     * @tparam T Type to check
     *
     * Rotation requires integral types with bitwise operations.
     */
    template <typename T>
    concept rotatable = int128_bitwise_compatible<T>;

    // =============================================================================
    // ALGORITHM CONCEPTS
    // =============================================================================

    /**
     * @brief Concept for predicates usable with int128 algorithms
     *
     * @tparam Pred Predicate type
     * @tparam T Value type (int128_param_t instance)
     *
     * Predicate must be callable with T and return bool.
     */
    template <typename Pred, typename T>
    concept int128_predicate = int128_type<T> &&
                               requires(Pred pred, T value) {
                                   { pred(value) } -> std::convertible_to<bool>;
                               };

    /**
     * @brief Concept for iterators pointing to int128 values
     *
     * @tparam Iter Iterator type
     *
     * Iterator must dereference to an int128_type.
     */
    template <typename Iter>
    concept int128_iterator = std::input_or_output_iterator<Iter> &&
                              int128_type<typename std::iterator_traits<Iter>::value_type>;

    /**
     * @brief Concept for ranges containing int128 values
     *
     * @tparam Range Range type
     *
     * Range must have begin()/end() returning int128_iterators.
     */
    template <typename Range>
    concept int128_range = std::ranges::range<Range> &&
                           int128_type<std::ranges::range_value_t<Range>>;

    // =============================================================================
    // CONTAINER CONCEPTS
    // =============================================================================

    /**
     * @brief Concept for containers holding int128 types
     *
     * @tparam Container Container type
     *
     * Container must satisfy standard container requirements with int128 values.
     */
    template <typename Container>
    concept int128_container =
        requires(Container c) {
            typename Container::value_type;
            { c.begin() } -> std::input_or_output_iterator;
            { c.end() } -> std::input_or_output_iterator;
            { c.size() } -> std::convertible_to<std::size_t>;
        } && int128_type<typename Container::value_type>;

    // =============================================================================
    // STL CONFORMANCE CHECKS
    // =============================================================================

    /**
     * @namespace int128_concept_checks
     * @brief Static assertions verifying STL concept conformance
     *
     * These compile-time checks ensure that int128_param_t types satisfy
     * all required STL concepts for use with standard algorithms and containers.
     */
    namespace int128_concept_checks
    {

        // --- Checks for uint128_t ---
        static_assert(std::regular<uint128_t>,
                      "uint128_t must satisfy std::regular");
        static_assert(std::totally_ordered<uint128_t>,
                      "uint128_t must satisfy std::totally_ordered");
        static_assert(std::equality_comparable<uint128_t>,
                      "uint128_t must be equality comparable");
        static_assert(std::default_initializable<uint128_t>,
                      "uint128_t must be default-constructible");
        static_assert(std::copy_constructible<uint128_t>,
                      "uint128_t must be copy-constructible");
        static_assert(std::move_constructible<uint128_t>,
                      "uint128_t must be move-constructible");
        static_assert(std::assignable_from<uint128_t &, const uint128_t &>,
                      "uint128_t must support assignment");

        // --- Checks for int128_tc_t (Two's Complement) ---
        static_assert(std::regular<int128_tc_t>,
                      "int128_tc_t must satisfy std::regular");
        static_assert(std::totally_ordered<int128_tc_t>,
                      "int128_tc_t must satisfy std::totally_ordered");
        static_assert(std::equality_comparable<int128_tc_t>,
                      "int128_tc_t must be equality comparable");
        static_assert(std::default_initializable<int128_tc_t>,
                      "int128_tc_t must be default-constructible");
        static_assert(std::copy_constructible<int128_tc_t>,
                      "int128_tc_t must be copy-constructible");
        static_assert(std::move_constructible<int128_tc_t>,
                      "int128_tc_t must be move-constructible");
        static_assert(std::assignable_from<int128_tc_t &, const int128_tc_t &>,
                      "int128_tc_t must support assignment");

        // --- Checks for int128_ms_t (Magnitude-Sign) ---
        static_assert(std::regular<int128_ms_t>,
                      "int128_ms_t must satisfy std::regular");
        static_assert(std::totally_ordered<int128_ms_t>,
                      "int128_ms_t must satisfy std::totally_ordered");
        static_assert(std::equality_comparable<int128_ms_t>,
                      "int128_ms_t must be equality comparable");

        // --- Size checks ---
        static_assert(sizeof(uint128_t) == 16,
                      "uint128_t must be 128 bits");
        static_assert(sizeof(int128_tc_t) == 16,
                      "int128_tc_t must be 128 bits");
        static_assert(sizeof(int128_ms_t) == 16,
                      "int128_ms_t must be 128 bits");
        static_assert(sizeof(int128_ek_t) == 16,
                      "int128_ek_t must be 128 bits");

        // --- Signedness checks ---
        static_assert(int128_tc_t{-1} < int128_tc_t{0},
                      "int128_tc_t must handle negative values");
        static_assert(int128_tc_t{1} > int128_tc_t{0},
                      "int128_tc_t must handle positive values");
        static_assert(int128_ms_t{-1} < int128_ms_t{0},
                      "int128_ms_t must handle negative values");

        // --- Type trait checks ---
        static_assert(is_128bit_type_v<uint128_t>,
                      "uint128_t must be detected as 128-bit type");
        static_assert(is_128bit_type_v<int128_tc_t>,
                      "int128_tc_t must be detected as 128-bit type");
        static_assert(is_128bit_type_v<int128_ms_t>,
                      "int128_ms_t must be detected as 128-bit type");
        static_assert(is_128bit_type_v<int128_ek_t>,
                      "int128_ek_t must be detected as 128-bit type");

        static_assert(is_uint128_v<uint128_t>,
                      "uint128_t must be detected as uint128");
        static_assert(!is_uint128_v<int128_tc_t>,
                      "int128_tc_t must not be detected as uint128");

        static_assert(is_signed_int128_v<int128_tc_t>,
                      "int128_tc_t must be detected as signed int128");
        static_assert(is_signed_int128_v<int128_ms_t>,
                      "int128_ms_t must be detected as signed int128");
        static_assert(is_signed_int128_v<int128_ek_t>,
                      "int128_ek_t must be detected as signed int128");
        static_assert(!is_signed_int128_v<uint128_t>,
                      "uint128_t must not be detected as signed int128");

        // --- Concept checks ---
        static_assert(int128_type<uint128_t>,
                      "uint128_t must satisfy int128_type");
        static_assert(int128_type<int128_tc_t>,
                      "int128_tc_t must satisfy int128_type");
        static_assert(int128_type<int128_ms_t>,
                      "int128_ms_t must satisfy int128_type");
        static_assert(int128_type<int128_ek_t>,
                      "int128_ek_t must satisfy int128_type");

        static_assert(uint128_type<uint128_t>,
                      "uint128_t must satisfy uint128_type");
        static_assert(!uint128_type<int128_tc_t>,
                      "int128_tc_t must not satisfy uint128_type");

        static_assert(signed_int128_type<int128_tc_t>,
                      "int128_tc_t must satisfy signed_int128_type");
        static_assert(signed_int128_type<int128_ms_t>,
                      "int128_ms_t must satisfy signed_int128_type");
        static_assert(signed_int128_type<int128_ek_t>,
                      "int128_ek_t must satisfy signed_int128_type");
        static_assert(!signed_int128_type<uint128_t>,
                      "uint128_t must not satisfy signed_int128_type");

        static_assert(int128_tc_type<int128_tc_t>,
                      "int128_tc_t must satisfy int128_tc_type");
        static_assert(!int128_tc_type<int128_ms_t>,
                      "int128_ms_t must not satisfy int128_tc_type");

        static_assert(int128_ms_type<int128_ms_t>,
                      "int128_ms_t must satisfy int128_ms_type");
        static_assert(!int128_ms_type<int128_tc_t>,
                      "int128_tc_t must not satisfy int128_ms_type");

        static_assert(int128_ek_type<int128_ek_t>,
                      "int128_ek_t must satisfy int128_ek_type");
        static_assert(!int128_ek_type<int128_tc_t>,
                      "int128_tc_t must not satisfy int128_ek_type");

    } // namespace int128_concept_checks

} // namespace nstd

#endif // INT128_PARAM_CONCEPTS_HPP
