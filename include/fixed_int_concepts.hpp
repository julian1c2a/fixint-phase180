// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// fixed_int_concepts.hpp — C++20 concepts for fixed_int_t<N, Sign, Form>
// Part of int128 Library - Phase 1.81 (Fase MS-INTEROP, T4)
// License: BSL-1.0
// =============================================================================
//
// Provides:
//   - Detection concepts: fixed_int_type, signed_fixed_int_type, unsigned_fixed_int_type
//   - Aglutinating concepts (built-in ∪ fixed_int_t):
//       nstd::integral<T>          ≡ std::integral<T>          ∨ fixed_int_type<T>
//       nstd::signed_integral<T>   ≡ std::signed_integral<T>   ∨ signed_fixed_int_type<T>
//       nstd::unsigned_integral<T> ≡ std::unsigned_integral<T> ∨ unsigned_fixed_int_type<T>
//
// WHY OWN CONCEPTS?
//   std::is_integral / std::is_arithmetic are non-specializable per the C++
//   standard. Consequently, the std:: concepts (std::integral, std::signed_integral,
//   std::unsigned_integral) reject our fixed_int_t. We provide nstd:: equivalents
//   so generic code that wants to accept built-ins AND fixed_int_t can use a
//   single concept.
//
// CONFLICT NOTE:
//   int128_param_concepts.hpp defines nstd::integral / nstd::signed_integral /
//   nstd::unsigned_integral with a DIFFERENT body (built-in ∪ int128_param_t).
//   Including both headers in the same TU would be an ODR violation. We guard
//   against this with the header-guard macro of int128_param_concepts.hpp; if
//   that file was included first, we skip our aglutinating concepts and the
//   user gets the int128_param_t-flavored ones (which do NOT cover fixed_int_t).
//   In that mixed-usage scenario, use the detection concepts below directly.

#ifndef FIXED_INT_CONCEPTS_HPP
#define FIXED_INT_CONCEPTS_HPP

#include "fixed_width_int_t.hpp"

#include <concepts>
#include <type_traits>

namespace nstd
{

    // =========================================================================
    // Detection concepts — always available, collision-free
    // =========================================================================

    /// @brief Matches any instance of fixed_int_t<N, Sign, Form>.
    template <typename T>
    concept fixed_int_type = is_fixed_int_v<T>;

    /// @brief Matches a signed fixed_int_t (Sign == signed_type).
    template <typename T>
    concept signed_fixed_int_type = is_signed_fixed_int_v<T>;

    /// @brief Matches an unsigned fixed_int_t (Sign == unsigned_type).
    template <typename T>
    concept unsigned_fixed_int_type = is_unsigned_fixed_int_v<T>;

    // =========================================================================
    // Aglutinating concepts — guarded against conflict with int128_param_concepts.hpp
    // =========================================================================

#ifndef INT128_PARAM_CONCEPTS_HPP

    /// @brief Built-in integral OR fixed_int_t. Excludes `bool` to mirror typical
    ///        arithmetic use; users who want bool must add it explicitly.
    template <typename T>
    concept integral = (std::integral<T> && !std::is_same_v<std::remove_cv_t<T>, bool>) || fixed_int_type<T>;

    /// @brief Signed built-in integral OR signed fixed_int_t.
    template <typename T>
    concept signed_integral = std::signed_integral<T> || signed_fixed_int_type<T>;

    /// @brief Unsigned built-in integral OR unsigned fixed_int_t. Excludes `bool`.
    template <typename T>
    concept unsigned_integral = (std::unsigned_integral<T> && !std::is_same_v<std::remove_cv_t<T>, bool>) ||
                                unsigned_fixed_int_type<T>;

#endif // !INT128_PARAM_CONCEPTS_HPP

} // namespace nstd

#endif // FIXED_INT_CONCEPTS_HPP
