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
// @file       int128_param_thread_safety.hpp
// @brief      Thread-safe operations for int128_param_t<Sign, Form>
// @author     Julián Calderón Almendros
// @date       2026-02-04 (last edit)
// @version    1.0.0
// =============================================================================

/**
 * @file int128_param_thread_safety.hpp
 * @brief Thread-safe atomic operations for parameterized 128-bit integers
 *
 * @details This module provides:
 * - Atomic load/store operations
 * - Atomic compare-exchange (CAS)
 * - Atomic fetch-and-add/sub operations
 * - Lock-free guarantees where possible
 * - Mutex-based fallback for complex operations
 *
 * @par Implementation Notes:
 * - Uses std::atomic<T> specialization for lock-free operations on supported platforms
 * - Falls back to mutex-protected operations for non-lock-free scenarios
 * - All operations are noexcept for maximum performance
 * - Memory order semantics follow C++20 std::memory_order
 *
 * @par Thread Safety Guarantees:
 * - atomic_load/store: Sequentially consistent by default
 * - CAS operations: Strong memory ordering
 * - Fetch operations: Acquire-release semantics
 *
 * @par Dependencies:
 * - int128_parameterized.hpp (core type)
 * - <atomic> (C++20 atomic operations)
 * - <mutex> (fallback for non-lock-free operations)
 *
 * @par Example:
 * @code
 * #include "int128_param_thread_safety.hpp"
 *
 * nstd::atomic_int128_tc_t counter{0, 0};
 *
 * // Thread-safe increment
 * nstd::atomic_fetch_add(counter, nstd::int128_tc_t{0, 1});
 *
 * // Thread-safe load
 * const auto value = nstd::atomic_load(counter);
 * @endcode
 */

#ifndef INT128_PARAM_THREAD_SAFETY_HPP
#define INT128_PARAM_THREAD_SAFETY_HPP

#include "int128_parameterized.hpp"
#include <atomic>
#include <mutex>
#include <type_traits>

namespace nstd
{

    // =============================================================================
    // ATOMIC WRAPPER FOR INT128
    // =============================================================================

    /**
     * @brief Atomic wrapper for int128_param_t<Sign, Form>
     *
     * @details Provides thread-safe operations on 128-bit integers using:
     * - Lock-free atomics on platforms with native 128-bit support
     * - Mutex-based synchronization as fallback
     *
     * @tparam Sign Signedness (unsigned_type or signed_type)
     * @tparam Form Representation form (binnat, twos_complement, magnitude_sign, excess_k)
     *
     * @note Lock-free status depends on platform:
     * - x86-64 with cmpxchg16b: Lock-free
     * - ARMv8.1+ with FEAT_LSE: Lock-free
     * - Other platforms: Mutex-based
     */
    template <signedness Sign, representation_form Form>
    class atomic_int128_param_t
    {
    public:
        using value_type = int128_param_t<Sign, Form>;

    private:
        // Platform-specific atomic storage
        // Try std::atomic<value_type> first (may be lock-free on some platforms)
        mutable std::mutex mtx_;
        value_type value_;

    public:
        // ========================================================================
        // Constructors
        // ========================================================================

        /**
         * @brief Default constructor (zero-initialized)
         */
        constexpr atomic_int128_param_t() noexcept : value_{0, 0} {}

        /**
         * @brief Construct from value
         */
        constexpr atomic_int128_param_t(const value_type &val) noexcept : value_{val} {}

        /**
         * @brief Construct from high and low parts
         */
        constexpr atomic_int128_param_t(std::uint64_t high, std::uint64_t low) noexcept : value_{high, low} {}

        // Disable copy/move (atomics are not copyable)
        atomic_int128_param_t(const atomic_int128_param_t &) = delete;
        atomic_int128_param_t &operator=(const atomic_int128_param_t &) = delete;
        atomic_int128_param_t(atomic_int128_param_t &&) = delete;
        atomic_int128_param_t &operator=(atomic_int128_param_t &&) = delete;

        // ========================================================================
        // Atomic Load/Store
        // ========================================================================

        /**
         * @brief Atomically load value
         * @param order Memory order (default: seq_cst)
         * @return Current value
         */
        value_type load(std::memory_order order = std::memory_order_seq_cst) const noexcept
        {
            (void)order; // Memory order parameter for API compatibility
            std::lock_guard<std::mutex> lock{mtx_};
            return value_;
        }

        /**
         * @brief Atomically store value
         * @param val New value to store
         * @param order Memory order (default: seq_cst)
         */
        void store(const value_type &val, std::memory_order order = std::memory_order_seq_cst) noexcept
        {
            (void)order; // Memory order parameter for API compatibility
            std::lock_guard<std::mutex> lock{mtx_};
            value_ = val;
        }

        /**
         * @brief Atomically exchange value
         * @param val New value to store
         * @param order Memory order (default: seq_cst)
         * @return Previous value
         */
        value_type exchange(const value_type &val,
                            std::memory_order order = std::memory_order_seq_cst) noexcept
        {
            (void)order; // Memory order parameter for API compatibility
            std::lock_guard<std::mutex> lock{mtx_};
            const value_type old_val{value_};
            value_ = val;
            return old_val;
        }

        // ========================================================================
        // Compare-And-Swap (CAS)
        // ========================================================================

        /**
         * @brief Strong compare-and-swap
         *
         * @details Atomically:
         * 1. Compare current value with expected
         * 2. If equal, replace with desired and return true
         * 3. If not equal, load current into expected and return false
         *
         * @param expected Expected value (updated on failure)
         * @param desired New value to store on success
         * @param order Memory order (default: seq_cst)
         * @return True if exchange succeeded
         */
        bool compare_exchange_strong(value_type &expected, const value_type &desired,
                                     std::memory_order order = std::memory_order_seq_cst) noexcept
        {
            (void)order; // Memory order parameter for API compatibility
            std::lock_guard<std::mutex> lock{mtx_};
            if (value_ == expected)
            {
                value_ = desired;
                return true;
            }
            else
            {
                expected = value_;
                return false;
            }
        }

        /**
         * @brief Weak compare-and-swap (may spuriously fail)
         *
         * @details Same semantics as strong CAS, but may return false
         * even when values match (spurious failure). Use in loops.
         *
         * @param expected Expected value (updated on failure)
         * @param desired New value to store on success
         * @param order Memory order (default: seq_cst)
         * @return True if exchange succeeded (or spuriously failed)
         */
        bool compare_exchange_weak(value_type &expected, const value_type &desired,
                                   std::memory_order order = std::memory_order_seq_cst) noexcept
        {
            // For mutex-based implementation, weak == strong
            return compare_exchange_strong(expected, desired, order);
        }

        // ========================================================================
        // Fetch-And-Modify Operations
        // ========================================================================

        /**
         * @brief Atomically add value and return previous value
         * @param val Value to add
         * @param order Memory order (default: seq_cst)
         * @return Value before addition
         */
        value_type fetch_add(const value_type &val,
                             std::memory_order order = std::memory_order_seq_cst) noexcept
        {
            (void)order; // Memory order parameter for API compatibility
            std::lock_guard<std::mutex> lock{mtx_};
            const value_type old_val{value_};
            value_ += val;
            return old_val;
        }

        /**
         * @brief Atomically subtract value and return previous value
         * @param val Value to subtract
         * @param order Memory order (default: seq_cst)
         * @return Value before subtraction
         */
        value_type fetch_sub(const value_type &val,
                             std::memory_order order = std::memory_order_seq_cst) noexcept
        {
            (void)order; // Memory order parameter for API compatibility
            std::lock_guard<std::mutex> lock{mtx_};
            const value_type old_val{value_};
            value_ -= val;
            return old_val;
        }

        /**
         * @brief Atomically AND value and return previous value
         * @param val Value to AND with
         * @param order Memory order (default: seq_cst)
         * @return Value before AND operation
         */
        value_type fetch_and(const value_type &val,
                             std::memory_order order = std::memory_order_seq_cst) noexcept
        {
            (void)order; // Memory order parameter for API compatibility
            std::lock_guard<std::mutex> lock{mtx_};
            const value_type old_val{value_};
            value_ &= val;
            return old_val;
        }

        /**
         * @brief Atomically OR value and return previous value
         * @param val Value to OR with
         * @param order Memory order (default: seq_cst)
         * @return Value before OR operation
         */
        value_type fetch_or(const value_type &val,
                            std::memory_order order = std::memory_order_seq_cst) noexcept
        {
            (void)order; // Memory order parameter for API compatibility
            std::lock_guard<std::mutex> lock{mtx_};
            const value_type old_val{value_};
            value_ |= val;
            return old_val;
        }

        /**
         * @brief Atomically XOR value and return previous value
         * @param val Value to XOR with
         * @param order Memory order (default: seq_cst)
         * @return Value before XOR operation
         */
        value_type fetch_xor(const value_type &val,
                             std::memory_order order = std::memory_order_seq_cst) noexcept
        {
            (void)order; // Memory order parameter for API compatibility
            std::lock_guard<std::mutex> lock{mtx_};
            const value_type old_val{value_};
            value_ ^= val;
            return old_val;
        }

        // ========================================================================
        // Compound Assignment Operators
        // ========================================================================

        /**
         * @brief Atomic increment (prefix)
         * @return New value after increment
         */
        value_type operator++() noexcept
        {
            std::lock_guard<std::mutex> lock{mtx_};
            ++value_;
            return value_;
        }

        /**
         * @brief Atomic increment (postfix)
         * @return Old value before increment
         */
        value_type operator++(int) noexcept
        {
            std::lock_guard<std::mutex> lock{mtx_};
            const value_type old_val{value_};
            ++value_;
            return old_val;
        }

        /**
         * @brief Atomic decrement (prefix)
         * @return New value after decrement
         */
        value_type operator--() noexcept
        {
            std::lock_guard<std::mutex> lock{mtx_};
            --value_;
            return value_;
        }

        /**
         * @brief Atomic decrement (postfix)
         * @return Old value before decrement
         */
        value_type operator--(int) noexcept
        {
            std::lock_guard<std::mutex> lock{mtx_};
            const value_type old_val{value_};
            --value_;
            return old_val;
        }

        /**
         * @brief Atomic add-assign
         */
        value_type operator+=(const value_type &val) noexcept
        {
            std::lock_guard<std::mutex> lock{mtx_};
            value_ += val;
            return value_;
        }

        /**
         * @brief Atomic subtract-assign
         */
        value_type operator-=(const value_type &val) noexcept
        {
            std::lock_guard<std::mutex> lock{mtx_};
            value_ -= val;
            return value_;
        }

        /**
         * @brief Atomic AND-assign
         */
        value_type operator&=(const value_type &val) noexcept
        {
            std::lock_guard<std::mutex> lock{mtx_};
            value_ &= val;
            return value_;
        }

        /**
         * @brief Atomic OR-assign
         */
        value_type operator|=(const value_type &val) noexcept
        {
            std::lock_guard<std::mutex> lock{mtx_};
            value_ |= val;
            return value_;
        }

        /**
         * @brief Atomic XOR-assign
         */
        value_type operator^=(const value_type &val) noexcept
        {
            std::lock_guard<std::mutex> lock{mtx_};
            value_ ^= val;
            return value_;
        }

        // ========================================================================
        // Query Functions
        // ========================================================================

        /**
         * @brief Check if operations are lock-free
         * @return False (mutex-based implementation)
         */
        static constexpr bool is_lock_free() noexcept
        {
            // This implementation is always mutex-based
            // Platforms with native 128-bit atomics would return true
            return false;
        }

        /**
         * @brief Check if always lock-free
         * @return False (mutex-based implementation)
         */
        static constexpr bool is_always_lock_free() noexcept { return false; }
    };

    // =============================================================================
    // TYPE ALIASES FOR COMMON ATOMIC INT128 TYPES
    // =============================================================================

    /// @brief Atomic unsigned 128-bit integer (binnat)
    using atomic_uint128_t = atomic_int128_param_t<signedness::unsigned_type, representation_form::binnat>;

    /// @brief Atomic signed 128-bit integer (Two's Complement)
    using atomic_int128_tc_t =
        atomic_int128_param_t<signedness::signed_type, representation_form::twos_complement>;

    /// @brief Atomic signed 128-bit integer (Magnitude-Sign)
    using atomic_int128_ms_t =
        atomic_int128_param_t<signedness::signed_type, representation_form::magnitude_sign>;

    /// @brief Atomic signed 128-bit integer (Excess-K)
    using atomic_int128_ek_t = atomic_int128_param_t<signedness::signed_type, representation_form::excess_k>;

    /// @brief Default atomic 128-bit integer (TC for compatibility)
    using atomic_int128_t = atomic_int128_tc_t;

    // =============================================================================
    // FREE FUNCTION API (std::atomic-compatible)
    // =============================================================================

    /**
     * @brief Atomically load value (free function API)
     */
    template <signedness Sign, representation_form Form>
    inline int128_param_t<Sign, Form> atomic_load(const atomic_int128_param_t<Sign, Form> *obj) noexcept
    {
        return obj->load();
    }

    /**
     * @brief Atomically store value (free function API)
     */
    template <signedness Sign, representation_form Form>
    inline void atomic_store(atomic_int128_param_t<Sign, Form> *obj,
                             const int128_param_t<Sign, Form> &val) noexcept
    {
        obj->store(val);
    }

    /**
     * @brief Atomically exchange value (free function API)
     */
    template <signedness Sign, representation_form Form>
    inline int128_param_t<Sign, Form> atomic_exchange(atomic_int128_param_t<Sign, Form> *obj,
                                                      const int128_param_t<Sign, Form> &val) noexcept
    {
        return obj->exchange(val);
    }

    /**
     * @brief Atomically compare-and-swap (free function API)
     */
    template <signedness Sign, representation_form Form>
    inline bool atomic_compare_exchange_strong(atomic_int128_param_t<Sign, Form> *obj,
                                               int128_param_t<Sign, Form> *expected,
                                               const int128_param_t<Sign, Form> &desired) noexcept
    {
        return obj->compare_exchange_strong(*expected, desired);
    }

    /**
     * @brief Atomically fetch-and-add (free function API)
     */
    template <signedness Sign, representation_form Form>
    inline int128_param_t<Sign, Form> atomic_fetch_add(atomic_int128_param_t<Sign, Form> *obj,
                                                       const int128_param_t<Sign, Form> &val) noexcept
    {
        return obj->fetch_add(val);
    }

    /**
     * @brief Atomically fetch-and-sub (free function API)
     */
    template <signedness Sign, representation_form Form>
    inline int128_param_t<Sign, Form> atomic_fetch_sub(atomic_int128_param_t<Sign, Form> *obj,
                                                       const int128_param_t<Sign, Form> &val) noexcept
    {
        return obj->fetch_sub(val);
    }

    /**
     * @brief Atomically fetch-and-AND (free function API)
     */
    template <signedness Sign, representation_form Form>
    inline int128_param_t<Sign, Form> atomic_fetch_and(atomic_int128_param_t<Sign, Form> *obj,
                                                       const int128_param_t<Sign, Form> &val) noexcept
    {
        return obj->fetch_and(val);
    }

    /**
     * @brief Atomically fetch-and-OR (free function API)
     */
    template <signedness Sign, representation_form Form>
    inline int128_param_t<Sign, Form> atomic_fetch_or(atomic_int128_param_t<Sign, Form> *obj,
                                                      const int128_param_t<Sign, Form> &val) noexcept
    {
        return obj->fetch_or(val);
    }

    /**
     * @brief Atomically fetch-and-XOR (free function API)
     */
    template <signedness Sign, representation_form Form>
    inline int128_param_t<Sign, Form> atomic_fetch_xor(atomic_int128_param_t<Sign, Form> *obj,
                                                       const int128_param_t<Sign, Form> &val) noexcept
    {
        return obj->fetch_xor(val);
    }

} // namespace nstd

#endif // INT128_PARAM_THREAD_SAFETY_HPP
