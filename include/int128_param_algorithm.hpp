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
// @file       int128_param_algorithm.hpp
// @brief      Algorithm functions for int128_param_t<S, F>
// @author     Julián Calderón Almendros
// @date       2026-02-04 (last edit)
// @version    1.75.0
// =============================================================================

#ifndef INT128_PARAM_ALGORITHM_HPP
#define INT128_PARAM_ALGORITHM_HPP

#include "int128_parameterized.hpp"
#include <algorithm>
#include <iterator>
#include <type_traits>

namespace nstd
{
    // ========================================================================
    // RANGE ALGORITHMS (STL-compatible)
    // ========================================================================

    /**
     * @brief Fill range with value
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @tparam ForwardIt Forward iterator type
     * @param first Beginning of range
     * @param last End of range
     * @param value Value to fill
     *
     * @details Assigns value to all elements in [first, last)
     * @complexity O(n) where n = std::distance(first, last)
     */
    template <signedness S, representation_form F, typename ForwardIt>
    constexpr void fill(ForwardIt first, ForwardIt last, const int128_param_t<S, F> &value) noexcept
    {
        while (first != last)
        {
            *first++ = value;
        }
    }

    /**
     * @brief Fill n elements with value
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @tparam OutputIt Output iterator type
     * @tparam Size Size type
     * @param first Beginning of range
     * @param n Number of elements to fill
     * @param value Value to fill
     * @return Iterator to element past the last written
     *
     * @details Assigns value to first n elements starting at first
     * @complexity O(n)
     */
    template <signedness S, representation_form F, typename OutputIt, typename Size>
    constexpr OutputIt fill_n(OutputIt first, Size n, const int128_param_t<S, F> &value) noexcept
    {
        for (Size i = 0; i < n; ++i)
        {
            *first++ = value;
        }
        return first;
    }

    /**
     * @brief Reverse range in-place
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @tparam BidirIt Bidirectional iterator type
     * @param first Beginning of range
     * @param last End of range
     *
     * @details Reverses order of elements in [first, last)
     * @complexity O(n/2) where n = std::distance(first, last)
     */
    template <signedness S, representation_form F, typename BidirIt>
    constexpr void reverse(BidirIt first, BidirIt last) noexcept
    {
        while ((first != last) && (first != --last))
        {
            std::iter_swap(first++, last);
        }
    }

    /**
     * @brief Find value in range
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @tparam InputIt Input iterator type
     * @param first Beginning of range
     * @param last End of range
     * @param value Value to search for
     * @return Iterator to first occurrence or last if not found
     *
     * @details Linear search for value using operator==
     * @complexity O(n) where n = std::distance(first, last)
     */
    template <signedness S, representation_form F, typename InputIt>
    constexpr InputIt find(InputIt first, InputIt last, const int128_param_t<S, F> &value) noexcept
    {
        while (first != last)
        {
            if (*first == value)
            {
                return first;
            }
            ++first;
        }
        return last;
    }

    /**
     * @brief Count occurrences of value in range
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @tparam InputIt Input iterator type
     * @param first Beginning of range
     * @param last End of range
     * @param value Value to count
     * @return Number of occurrences
     *
     * @details Counts elements equal to value using operator==
     * @complexity O(n) where n = std::distance(first, last)
     */
    template <signedness S, representation_form F, typename InputIt>
    constexpr typename std::iterator_traits<InputIt>::difference_type
    count(InputIt first, InputIt last, const int128_param_t<S, F> &value) noexcept
    {
        typename std::iterator_traits<InputIt>::difference_type result{0};
        while (first != last)
        {
            if (*first == value)
            {
                ++result;
            }
            ++first;
        }
        return result;
    }

    /**
     * @brief Check if all elements satisfy predicate
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @tparam InputIt Input iterator type
     * @tparam UnaryPredicate Predicate type
     * @param first Beginning of range
     * @param last End of range
     * @param pred Unary predicate
     * @return true if pred(*it) is true for all elements
     *
     * @details Returns true if range is empty
     * @complexity O(n) where n = std::distance(first, last)
     */
    template <signedness S, representation_form F, typename InputIt, typename UnaryPredicate>
    constexpr bool all_of(InputIt first, InputIt last, UnaryPredicate pred) noexcept
    {
        while (first != last)
        {
            if (!pred(*first))
            {
                return false;
            }
            ++first;
        }
        return true;
    }

    /**
     * @brief Check if any element satisfies predicate
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @tparam InputIt Input iterator type
     * @tparam UnaryPredicate Predicate type
     * @param first Beginning of range
     * @param last End of range
     * @param pred Unary predicate
     * @return true if pred(*it) is true for at least one element
     *
     * @details Returns false if range is empty
     * @complexity O(n) where n = std::distance(first, last)
     */
    template <signedness S, representation_form F, typename InputIt, typename UnaryPredicate>
    constexpr bool any_of(InputIt first, InputIt last, UnaryPredicate pred) noexcept
    {
        while (first != last)
        {
            if (pred(*first))
            {
                return true;
            }
            ++first;
        }
        return false;
    }

    /**
     * @brief Check if no element satisfies predicate
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @tparam InputIt Input iterator type
     * @tparam UnaryPredicate Predicate type
     * @param first Beginning of range
     * @param last End of range
     * @param pred Unary predicate
     * @return true if pred(*it) is false for all elements
     *
     * @details Returns true if range is empty
     * @complexity O(n) where n = std::distance(first, last)
     */
    template <signedness S, representation_form F, typename InputIt, typename UnaryPredicate>
    constexpr bool none_of(InputIt first, InputIt last, UnaryPredicate pred) noexcept
    {
        while (first != last)
        {
            if (pred(*first))
            {
                return false;
            }
            ++first;
        }
        return true;
    }

    /**
     * @brief Find minimum element in range
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @tparam ForwardIt Forward iterator type
     * @param first Beginning of range
     * @param last End of range
     * @return Iterator to minimum element or last if range is empty
     *
     * @details Uses operator< for comparison
     * @complexity O(n) where n = std::distance(first, last)
     */
    template <signedness S, representation_form F, typename ForwardIt>
    constexpr ForwardIt min_element(ForwardIt first, ForwardIt last) noexcept
    {
        if (first == last)
        {
            return last;
        }

        ForwardIt smallest{first};
        ++first;

        while (first != last)
        {
            if (*first < *smallest)
            {
                smallest = first;
            }
            ++first;
        }
        return smallest;
    }

    /**
     * @brief Find maximum element in range
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @tparam ForwardIt Forward iterator type
     * @param first Beginning of range
     * @param last End of range
     * @return Iterator to maximum element or last if range is empty
     *
     * @details Uses operator< for comparison
     * @complexity O(n) where n = std::distance(first, last)
     */
    template <signedness S, representation_form F, typename ForwardIt>
    constexpr ForwardIt max_element(ForwardIt first, ForwardIt last) noexcept
    {
        if (first == last)
        {
            return last;
        }

        ForwardIt largest{first};
        ++first;

        while (first != last)
        {
            if (*largest < *first)
            {
                largest = first;
            }
            ++first;
        }
        return largest;
    }

    /**
     * @brief Accumulate (sum) elements in range
     *
     * @tparam S Signedness of type
     * @tparam F Representation form
     * @tparam InputIt Input iterator type
     * @param first Beginning of range
     * @param last End of range
     * @param init Initial value
     * @return Sum of init and all elements in range
     *
     * @details Uses operator+= for accumulation
     * @complexity O(n) where n = std::distance(first, last)
     * @note No overflow checking (wraps around)
     */
    template <signedness S, representation_form F, typename InputIt>
    constexpr int128_param_t<S, F> accumulate(InputIt first, InputIt last, int128_param_t<S, F> init) noexcept
    {
        while (first != last)
        {
            init += *first;
            ++first;
        }
        return init;
    }

} // namespace nstd

#endif // INT128_PARAM_ALGORITHM_HPP
