// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// int128 Library - Parameterized Integer Template (Phase 1.75)
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
// @file       int128_parameterized.hpp
// @brief      128-bit integer template with parameterized representation
// @author     Julián Calderón Almendros
// @date       2026-01-11
// @version    1.0.0
// =============================================================================

#ifndef INT128_PARAMETERIZED_HPP
#define INT128_PARAMETERIZED_HPP

// Include standard library headers first
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <array>
#include <bitset>
#include <stdexcept>

// Then include project headers
#include "representation.hpp"

// Include intrinsics for optimized operations (optional, fallback available)
#if __has_include("intrinsics/arithmetic_operations.hpp")
#include "intrinsics/arithmetic_operations.hpp"
#endif

#if __has_include("intrinsics/bit_operations.hpp")
#include "intrinsics/bit_operations.hpp"
#endif

#if __has_include("intrinsics/byte_operations.hpp")
#include "intrinsics/byte_operations.hpp"
#endif

// Consteval/constexpr Granlund-Montgomery division by compile-time constants
#if __has_include("int128_param_divmod.hpp")
#include "int128_param_divmod.hpp"
#endif

namespace nstd
{
    // =============================================================================
    // Parse Error Enums and Result Structures
    // =============================================================================

#ifndef NSTD_PARSE_COMMON_DEFINED
#define NSTD_PARSE_COMMON_DEFINED
    /**
     * @brief Enum for parsing error codes
     */
    enum class parse_error : std::uint8_t
    {
        success = 0,             ///< Parsing successful
        null_pointer,            ///< Null pointer provided
        empty_string,            ///< Empty string
        invalid_base,            ///< Base out of range [2, 36]
        invalid_base_value,      ///< Invalid base (alias of invalid_base)
        invalid_character,       ///< Invalid character for specified base
        digit_out_of_range,      ///< Digit out of range for base
        no_digits,               ///< No valid digits found
        overflow,                ///< Result exceeds type range
        separator_at_boundaries, ///< Separator at start or end
        unknown_error            ///< Unknown error
    };

    /**
     * @brief Structure encapsulating parse_ct result with error detection
     *
     * @tparam T Data type to parse (int128_param_t, etc.)
     *
     * @details Allows parse_ct_safe to return error with exact error location (error_index)
     * without throwing exceptions. Essential for constexpr contexts where exceptions not allowed.
     */
    template <typename T>
    struct parse_result
    {
        parse_error error;       ///< Error code
        T value;                 ///< Parsed value
        std::size_t error_index; ///< Index of error in string (npos if success)

        /**
         * @brief Checks if parse was successful
         * @return true if error == parse_error::success, false otherwise
         */
        constexpr bool success() const noexcept
        {
            return error == parse_error::success;
        }

        /**
         * @brief Default constructor
         * Initializes with error=success, value=0, error_index=npos
         */
        constexpr parse_result() noexcept
            : error(parse_error::success),
              value(T{}),
              error_index(std::string::npos)
        {
        }

        /**
         * @brief Custom constructor
         */
        constexpr parse_result(parse_error err, T val, std::size_t idx) noexcept
            : error(err),
              value(val),
              error_index(idx)
        {
        }
    };
#endif // NSTD_PARSE_COMMON_DEFINED

    // =============================================================================
    // Main Parameterized Integer Template
    // =============================================================================

    /**
     * @class int128_param_t
     * @brief 128-bit integer with parameterized representation and signedness
     *
     * @tparam Sign Signedness (signed_type or unsigned_type)
     * @tparam Form Representation form (twos_complement, magnitude_sign, excess_k)
     *
     * @details This template extends the unified template from Phase 1.66 by
     * adding a representation_form parameter, allowing investigation into different
     * encodings suitable for floating-point research.
     *
     * **Memory Layout:**
     * - data[0]: Low 64 bits (LSB)
     * - data[1]: High 64 bits (MSB, contains sign or bias information)
     * - Always 16 bytes, little-endian indexing
     *
     * **Type Aliases Generated:**
     * - uint128_tc_t:     unsigned, two's complement (default)
     * - int128_tc_t:      signed, two's complement (Phase 1.66 equivalent)
     * - uint128_ms_t:     unsigned, magnitude-sign
     * - int128_ms_t:      signed, magnitude-sign (Phase 1.75 primary)
     * - uint128_ek_t:     unsigned, excess-k (future)
     * - int128_ek_t:      signed, excess-k (future)
     *
     * @invariant Storage is always 128 bits (16 bytes)
     * @invariant Representation is immutable after construction
     * @invariant All operations are noexcept unless otherwise noted
     */
    template <signedness Sign = signedness::unsigned_type,
              representation_form Form = (Sign == signedness::unsigned_type ? representation_form::binnat : representation_form::twos_complement)>
    class int128_param_t
    {
        // ===================== LIMB/Utility Constants =====================
    public:
        static constexpr signedness sign{Sign};
        static constexpr representation_form form{Form};
        static constexpr bool is_signed{Sign == signedness::signed_type};
        static constexpr bool is_binnat{Form == representation_form::binnat};
        static constexpr bool is_twos_complement{Form == representation_form::twos_complement};
        static constexpr bool is_magnitude_sign{Form == representation_form::magnitude_sign};
        static constexpr bool is_excess_k{Form == representation_form::excess_k};
        static constexpr int BITS{128};
        static constexpr int BYTES{16};
        // Enforce valid combinations: unsigned solo binnat, signed solo TC/MS/EK
        static_assert(
            (Sign == signedness::unsigned_type) == (Form == representation_form::binnat),
            "Combinación inválida: unsigned solo permite binnat; signed solo permite TC, MS o EK");

        /**
         * @brief Return maximum representable value
         *
         * @details For unsigned: all bits set (2^128 - 1)
         *          For TC signed: 0x7FFF... (2^127 - 1)
         *          For MS signed: magnitude 0x7FFF..., sign=0 (2^127 - 1)
         *          For EK signed: stored value = 2^127 - 1 + K (real max = 2^127 - 1)
         */
        static constexpr int128_param_t max() noexcept
        {
            if constexpr (Sign == signedness::unsigned_type)
            {
                // Unsigned: all bits set
                return int128_param_t{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
            }
            else if constexpr (Form == representation_form::twos_complement)
            {
                // TC signed: MSB=0, rest=1 → 0x7FFF...
                return int128_param_t{0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
            }
            else if constexpr (Form == representation_form::magnitude_sign)
            {
                // MS signed: sign bit=0, magnitude=max → 0x7FFF...
                return int128_param_t{0x7FFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
            }
            else // excess_k
            {
                // EK: stored = real_max + K = (2^127 - 1) + 2^126 = 3·2^126 - 1
                // K = 0x4000000000000000 0x0000000000000000
                // max_real = 0x7FFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF
                // stored = 0xBFFFFFFFFFFFFFFF FFFFFFFFFFFFFFFF
                return int128_param_t{0xBFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
            }
        }

        /**
         * @brief Return minimum representable value
         *
         * @details For unsigned: 0
         *          For TC signed: 0x8000... (-2^127)
         *          For MS signed: magnitude 0x7FFF..., sign=1 (-2^127 + 1 because no -0)
         *          For EK signed: stored value = 0 (real min = -K = -2^126)
         */
        static constexpr int128_param_t min() noexcept
        {
            if constexpr (Sign == signedness::unsigned_type)
            {
                // Unsigned: zero
                return int128_param_t{0, 0};
            }
            else if constexpr (Form == representation_form::twos_complement)
            {
                // TC signed: MSB=1, rest=0 → 0x8000... = -2^127
                return int128_param_t{0x8000000000000000ULL, 0};
            }
            else if constexpr (Form == representation_form::magnitude_sign)
            {
                // MS signed: sign bit=1, magnitude=max → -2^127 + 1 (no tiene -2^127)
                // (Because MS can't represent -2^127, only -(2^127 - 1))
                return int128_param_t{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
            }
            else // excess_k
            {
                // EK: stored = real_min + K = -2^127 + 2^126 = -2^126
                // Stored value = 0x0000000000000000 0x0000000000000000
                return int128_param_t{0, 0};
            }
        }

    private:
        std::uint64_t data[2]{0, 0};

        /// @internal
        /// @brief Two-digit lookup table: "00", "01", ..., "99" concatenated.
        /// Used by write_u64_digits and write_19_padded_digits for 2x fewer divisions.
        static constexpr char DIGIT_PAIRS_[201] =
            "00010203040506070809"
            "10111213141516171819"
            "20212223242526272829"
            "30313233343536373839"
            "40414243444546474849"
            "50515253545556575859"
            "60616263646566676869"
            "70717273747576777879"
            "80818283848586878889"
            "90919293949596979899";

        /// @internal
        /// @brief Write a uint64_t as decimal digits (variable length, no zero-padding).
        /// Uses 2-digit pairs via divmod_const<100> to halve the number of divisions.
        static inline void write_u64_digits(char *buf, int &pos, std::uint64_t val) noexcept
        {
            // Use GM divmod_const<100> for 2-digit extraction
            int128_param_t<signedness::unsigned_type, representation_form::binnat> temp{0, val};

            while (temp.low() >= 100)
            {
                auto [q, r] = temp.template divmod_const<100>();
                const std::uint64_t pair = r.low();
                buf[--pos] = DIGIT_PAIRS_[pair * 2 + 1];
                buf[--pos] = DIGIT_PAIRS_[pair * 2];
                temp = q;
            }

            const std::uint64_t final_val = temp.low();
            if (final_val >= 10)
            {
                buf[--pos] = DIGIT_PAIRS_[final_val * 2 + 1];
                buf[--pos] = DIGIT_PAIRS_[final_val * 2];
            }
            else
            {
                buf[--pos] = static_cast<char>('0' + final_val);
            }
        }

        /// @internal
        /// @brief Write exactly 19 decimal digits from a uint64_t (zero-padded).
        /// 19 is odd: 1 digit + 9 pairs of 2 digits.
        static inline void write_19_padded_digits(char *buf, int &pos, std::uint64_t val) noexcept
        {
            // Use GM divmod_const for digit extraction
            int128_param_t<signedness::unsigned_type, representation_form::binnat> temp{0, val};

            // 19 is odd: extract 1 digit first, then 9 pairs
            auto [q1, r1] = temp.template divmod_const<10>();
            buf[--pos] = static_cast<char>('0' + r1.low());

            for (int i{0}; i < 9; ++i)
            {
                auto [q, r] = q1.template divmod_const<100>();
                const std::uint64_t pair = r.low();
                buf[--pos] = DIGIT_PAIRS_[pair * 2 + 1];
                buf[--pos] = DIGIT_PAIRS_[pair * 2];
                q1 = q;
            }
        }

        /// @internal
        /// @brief Convert from source representation (src_high, src_low) to this representation
        ///
        /// @tparam S2 Source signedness
        /// @tparam F2 Source representation form
        ///
        /// @details Conversion strategy matrix:
        ///   Same form:     direct bit copy (binnat↔TC per C++20 modular semantics)
        ///   src=TC:        use tc→target conversion directly
        ///   target=TC:     use src→tc conversion directly
        ///   otherwise:     src→TC→target (TC as pivot)
        template <signedness S2, representation_form F2>
        constexpr void convert_from(std::uint64_t src_high, std::uint64_t src_low) noexcept
        {
            constexpr representation_form src_form{F2};
            constexpr representation_form dst_form{Form};

            // =================================================================
            // Case 1: binnat ↔ TC — C++20 modular bit reinterpretation
            // unsigned→signed: value is unchanged if it fits, otherwise
            //   implementation-defined (C++20: modular, same bit pattern)
            // signed→unsigned: well-defined modular reduction (same bits)
            // =================================================================
            if constexpr ((src_form == representation_form::binnat && dst_form == representation_form::twos_complement) ||
                          (src_form == representation_form::twos_complement && dst_form == representation_form::binnat))
            {
                data[0] = src_low;
                data[1] = src_high;
            }
            // =================================================================
            // Case 2: TC → MS
            // =================================================================
            else if constexpr (src_form == representation_form::twos_complement && dst_form == representation_form::magnitude_sign)
            {
                twos_complement128_to_ms(src_high, src_low, data[1], data[0]);
            }
            // =================================================================
            // Case 3: MS → TC
            // =================================================================
            else if constexpr (src_form == representation_form::magnitude_sign && dst_form == representation_form::twos_complement)
            {
                ms128_to_twos_complement(src_high, src_low, data[1], data[0]);
            }
            // =================================================================
            // Case 4: TC → EK
            // =================================================================
            else if constexpr (src_form == representation_form::twos_complement && dst_form == representation_form::excess_k)
            {
                twos_complement128_to_excess_k(src_high, src_low, data[1], data[0]);
            }
            // =================================================================
            // Case 5: EK → TC
            // =================================================================
            else if constexpr (src_form == representation_form::excess_k && dst_form == representation_form::twos_complement)
            {
                excess_k128_to_twos_complement(src_high, src_low, data[1], data[0]);
            }
            // =================================================================
            // Case 6: MS → EK (via TC pivot)
            // =================================================================
            else if constexpr (src_form == representation_form::magnitude_sign && dst_form == representation_form::excess_k)
            {
                ms128_to_excess_k(src_high, src_low, data[1], data[0]);
            }
            // =================================================================
            // Case 7: EK → MS (via TC pivot)
            // =================================================================
            else if constexpr (src_form == representation_form::excess_k && dst_form == representation_form::magnitude_sign)
            {
                excess_k128_to_ms(src_high, src_low, data[1], data[0]);
            }
            // =================================================================
            // Case 8: binnat ↔ MS or binnat ↔ EK (via TC pivot)
            // =================================================================
            else if constexpr (src_form == representation_form::binnat)
            {
                // binnat → TC (copy bits) → target
                convert_from<signedness::signed_type, representation_form::twos_complement>(src_high, src_low);
            }
            else if constexpr (dst_form == representation_form::binnat)
            {
                // source → TC first
                std::uint64_t tc_high{0};
                std::uint64_t tc_low{0};
                if constexpr (src_form == representation_form::magnitude_sign)
                {
                    ms128_to_twos_complement(src_high, src_low, tc_high, tc_low);
                }
                else if constexpr (src_form == representation_form::excess_k)
                {
                    excess_k128_to_twos_complement(src_high, src_low, tc_high, tc_low);
                }
                // TC → binnat (copy bits)
                data[0] = tc_low;
                data[1] = tc_high;
            }
            // =================================================================
            // Case 9: Same form, different signedness only
            // (shouldn't happen given valid constraints, but safe fallback)
            // =================================================================
            else
            {
                data[0] = src_low;
                data[1] = src_high;
            }
        }

        // Targeted GCC -O2 workaround: only applied to EK construction.
        // GCC 15.2.0 incorrectly eliminates the bias addition in EK construction at -O2.
        // [[gnu::optimize("O0"), gnu::noinline]] suppresses it only for this helper,
        // leaving TC, MS, and binnat constructors fully optimizable.
#if defined(__GNUC__) && !defined(__clang__)
        [[gnu::optimize("O0"), gnu::noinline]]
#endif
        void ek_store_bias(std::uint64_t low, std::uint64_t high) noexcept
        {
            constexpr std::uint64_t bias_high{1ULL << 62};
            data[0] = low;
            data[1] = high + bias_high;
        }

    public:
        // ===================== Constructors =====================

        /// @brief Default constructor (zero)
        constexpr int128_param_t() noexcept : data{0, 0} {}

        /// @brief Copy constructor
        constexpr int128_param_t(const int128_param_t &other) noexcept = default;

        /// @brief Move constructor
        constexpr int128_param_t(int128_param_t &&other) noexcept = default;

        // ===================== Cross-Representation Constructors =====================

        /// @brief Cross-representation copy constructor
        ///
        /// Converts between any two valid int128_param_t instantiations:
        ///   binnat ↔ TC: C++20 standard modular bit reinterpretation
        ///   TC ↔ MS, TC ↔ EK, MS ↔ EK: via representation.hpp conversion functions
        ///   binnat ↔ MS, binnat ↔ EK: via TC as pivot
        ///
        /// @tparam S2 Source signedness
        /// @tparam F2 Source representation form
        /// @note explicit — use static_cast<target_t>(source) to invoke
        template <signedness S2, representation_form F2,
                  typename = std::enable_if_t<(S2 != Sign) || (F2 != Form)>>
        explicit constexpr int128_param_t(const int128_param_t<S2, F2> &other) noexcept
            : data{0, 0}
        {
            convert_from<S2, F2>(other.high(), other.low());
        }

        /// @brief Cross-representation move constructor
        ///
        /// Same semantics as copy — for trivial types move == copy,
        /// but provided for completeness and API symmetry.
        ///
        /// @tparam S2 Source signedness
        /// @tparam F2 Source representation form
        template <signedness S2, representation_form F2,
                  typename = std::enable_if_t<(S2 != Sign) || (F2 != Form)>>
        explicit constexpr int128_param_t(int128_param_t<S2, F2> &&other) noexcept
            : data{0, 0}
        {
            convert_from<S2, F2>(other.high(), other.low());
        }

        /// @brief Constructor from (high, low) pair
        ///
        /// ⚠️ CRITICAL: Parameter order is NOT intuitive!
        /// Constructor parameters: (high_value, low_value)
        /// Storage: data{low, high}  ← Stored in REVERSE order!
        ///
        /// @example
        /// @code
        ///   // To represent value 2:
        ///   const auto x = int128_param_t{0x0, 0x2};  // ✓ Correct (high=0, low=2)
        ///   // NOT: int128_param_t{0x2, 0x0}  ❌ Wrong (creates 2^65 instead of 2)
        ///
        ///   // To represent 2^64:
        ///   const auto y = int128_param_t{0x0, 0x1};  // ✓ Correct (high=0, low=2^64)
        ///
        ///   // To represent 2^127:
        ///   const auto z = int128_param_t{0x8000000000000000ULL, 0x0};  // ✓ Correct
        /// @endcode
        ///
        /// @see data for storage layout: data[0] = low, data[1] = high
        template <typename T1, typename T2>
        explicit constexpr int128_param_t(T1 high, T2 low) noexcept
            : data{static_cast<std::uint64_t>(low), static_cast<std::uint64_t>(high)} {}

        /// @brief Constructor from single integral value (zero-extends or sign-extends)
        ///
        /// EK construction dispatches to ek_store_bias() at runtime to work around
        /// a GCC 15.2.0 -O2 bug that incorrectly eliminated the bias addition.
        /// TC, MS, and binnat paths compile fully optimized.
        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        explicit constexpr int128_param_t(T value) noexcept : data{0, 0}
        {
            // ========================================================================
            // EXCESS-K Representation (Bias Notation)
            // ========================================================================
            if constexpr (is_excess_k && is_signed)
            {
                constexpr std::uint64_t bias_high{1ULL << 62};

                if constexpr (std::is_signed_v<T>)
                {
                    const std::uint64_t value_low{static_cast<std::uint64_t>(value)};
                    const std::uint64_t value_high{(value < 0) ? std::numeric_limits<std::uint64_t>::max() : std::uint64_t{0}};

                    if (std::is_constant_evaluated())
                    {
                        data[0] = value_low;
                        data[1] = value_high + bias_high;
                    }
                    else
                    {
                        ek_store_bias(value_low, value_high);
                    }
                }
                else
                {
                    const std::uint64_t value_low{static_cast<std::uint64_t>(value)};
                    const std::uint64_t value_high{[&]() constexpr noexcept -> std::uint64_t
                                                   {
                                                       if constexpr (sizeof(T) > sizeof(std::uint64_t))
                                                       {
                                                           return static_cast<std::uint64_t>(value >> 64);
                                                       }
                                                       else
                                                       {
                                                           return std::uint64_t{0};
                                                       }
                                                   }()};

                    if (std::is_constant_evaluated())
                    {
                        data[0] = value_low;
                        data[1] = value_high + bias_high;
                    }
                    else
                    {
                        ek_store_bias(value_low, value_high);
                    }
                }
                return;
            }

            // ========================================================================
            // MAGNITUDE-SIGN Representation
            // ========================================================================
            if constexpr (is_magnitude_sign && is_signed)
            {
                if constexpr (std::is_signed_v<T>)
                {
                    if (value < 0)
                    {
                        using UnsignedT = std::make_unsigned_t<T>;
                        data[0] = static_cast<std::uint64_t>(-static_cast<UnsignedT>(value));
                        data[1] = std::uint64_t{1ULL << 63};
                    }
                    else
                    {
                        data[0] = static_cast<std::uint64_t>(value);
                        data[1] = std::uint64_t{0};
                    }
                }
                else
                {
                    data[0] = static_cast<std::uint64_t>(value);
                    if constexpr (sizeof(T) > sizeof(std::uint64_t))
                    {
                        data[1] = static_cast<std::uint64_t>(value >> 64);
                    }
                    else
                    {
                        data[1] = std::uint64_t{0};
                    }
                }
                return;
            }

            // ========================================================================
            // TWO'S COMPLEMENT (TC) and BINNAT (unsigned binary natural)
            // ========================================================================
            if constexpr (std::is_signed_v<T>)
            {
                const bool negative{value < 0};
                data[0] = static_cast<std::uint64_t>(value);
                data[1] = negative ? std::numeric_limits<std::uint64_t>::max() : std::uint64_t{0};
            }
            else
            {
                data[0] = static_cast<std::uint64_t>(value);
                if constexpr (sizeof(T) > sizeof(std::uint64_t))
                {
                    data[1] = static_cast<std::uint64_t>(value >> 64);
                }
                else
                {
                    data[1] = std::uint64_t{0};
                }
            }
        }

        // ===================== Assignment Operators =====================

        constexpr int128_param_t &operator=(const int128_param_t &) noexcept = default;
        constexpr int128_param_t &operator=(int128_param_t &&) noexcept = default;

        // ===================== Cross-Representation Assignment Operators =====================

        /// @brief Cross-representation copy assignment operator
        ///
        /// Converts and assigns from any other valid int128_param_t instantiation.
        /// Uses the same conversion strategy as the cross-representation constructors:
        ///   binnat ↔ TC: C++20 modular bit reinterpretation
        ///   TC ↔ MS, TC ↔ EK, MS ↔ EK: via representation.hpp conversion functions
        ///   binnat ↔ MS, binnat ↔ EK: via TC as pivot
        ///
        /// @tparam S2 Source signedness
        /// @tparam F2 Source representation form
        /// @note Use: target = static_cast<target_t>(source); or target = target_t{source};
        template <signedness S2, representation_form F2,
                  typename = std::enable_if_t<(S2 != Sign) || (F2 != Form)>>
        constexpr int128_param_t &operator=(const int128_param_t<S2, F2> &other) noexcept
        {
            convert_from<S2, F2>(other.high(), other.low());
            return *this;
        }

        /// @brief Cross-representation move assignment operator
        ///
        /// Same semantics as copy — for trivial types move == copy,
        /// but provided for completeness and API symmetry.
        ///
        /// @tparam S2 Source signedness
        /// @tparam F2 Source representation form
        template <signedness S2, representation_form F2,
                  typename = std::enable_if_t<(S2 != Sign) || (F2 != Form)>>
        constexpr int128_param_t &operator=(int128_param_t<S2, F2> &&other) noexcept
        {
            convert_from<S2, F2>(other.high(), other.low());
            return *this;
        }

        // ===================== Public API =====================

        /// @brief Assignment from integral type
        template <typename T>
        constexpr int128_param_t &operator=(T value) noexcept
        {
            return *this = int128_param_t{value};
        }

        /**
         * @brief Assignment from float (delegates to constructor)
         * @param value Float value to assign
         * @return Reference to this object
         * @note Truncates fractional part, handles NaN/overflow
         */
        int128_param_t &operator=(float value) noexcept
        {
            return *this = int128_param_t{value};
        }

        /**
         * @brief Assignment from double (delegates to constructor)
         * @param value Double value to assign
         * @return Reference to this object
         * @note Truncates fractional part, handles NaN/overflow
         */
        int128_param_t &operator=(double value) noexcept
        {
            return *this = int128_param_t{value};
        }

        /**
         * @brief Assignment from long double (delegates to constructor)
         * @param value Long double value to assign
         * @return Reference to this object
         * @note Truncates fractional part, handles NaN/overflow
         */
        int128_param_t &operator=(long double value) noexcept
        {
            return *this = int128_param_t{value};
        }

        // ========================================================================
        // Accessors
        // ========================================================================

        /// @brief Get high 64 bits (MSB)
        constexpr std::uint64_t high() const noexcept { return data[1]; }

        /// @brief Get low 64 bits (LSB)
        constexpr std::uint64_t low() const noexcept { return data[0]; }

        /// @brief Set high 64 bits
        template <typename T>
        constexpr void set_high(T value) noexcept { data[1] = static_cast<std::uint64_t>(value); }

        /// @brief Set low 64 bits
        template <typename T>
        constexpr void set_low(T value) noexcept { data[0] = static_cast<std::uint64_t>(value); }

        // ========================================================================
        // Representation-Specific Methods
        // ========================================================================

        /**
         * @brief Check if value is negative (representation-aware)
         *
         * **Two's Complement:** MSB is sign bit
         * **Magnitude-Sign:** Explicit sign bit at data[1] MSB
         * **Excess-k:** Compare against bias
         *
         * @return true if negative, false if positive (or zero)
         */
        [[nodiscard]] constexpr bool is_negative() const noexcept
        {
            if constexpr (!is_signed)
            {
                return false;
            }
            else if constexpr (is_twos_complement)
            {
                return (data[1] & (std::uint64_t{1} << 63)) != std::uint64_t{0};
            }
            else if constexpr (is_magnitude_sign)
            {
                return (data[1] & (std::uint64_t{1} << 63)) != std::uint64_t{0};
            }
            else /* excess_k */
            {
                constexpr std::uint64_t bias_high{1ull << 62};
                constexpr std::uint64_t bias_low{0ull};
                if (data[1] < bias_high)
                {
                    return true;
                }
                else if (data[1] > bias_high)
                {
                    return false;
                }
                else
                {
                    return data[0] < bias_low;
                }
            }
        }

        /**
         * @brief Get magnitude (sign-independent absolute value)
                return (data[1] & (std::uint64_t{1} << 63)) != std::uint64_t{0};
         * **Two's Complement:** Negation for negatives
                return (data[1] & (std::uint64_t{1} << 63)) != std::uint64_t{0};
         *
         * @return Magnitude as uint128_param_t
         */
        [[nodiscard]] constexpr int128_param_t magnitude() const noexcept
        {
            if constexpr (!is_signed)
            {
                return *this;
            }
            else if constexpr (is_twos_complement)
            {
                return is_negative() ? (-(*this)) : (*this);
            }
            else if constexpr (is_magnitude_sign)
            {
                int128_param_t result{*this};
                result.data[1] &= ~(std::uint64_t{1} << 63); // Clear sign bit
                return result;
            }
            else /* excess_k */
            {
                constexpr std::uint64_t bias_high{1ull << 62};
                constexpr std::uint64_t bias_low{0ull};
                int128_param_t result{*this};
                bool borrow{false};
                if (result.data[0] < bias_low)
                {
                    borrow = true;
                }
                result.data[0] = result.data[0] - bias_low;
                if (borrow)
                {
                    if (result.data[1] == std::uint64_t{0})
                    {
                        result.data[1] = ~std::uint64_t{0} - bias_high + std::uint64_t{1};
                    }
                    else
                    {
                        result.data[1] = result.data[1] - bias_high - std::uint64_t{1};
                    }
                }
                else
                {
                    result.data[1] = result.data[1] - bias_high;
                }
                if ((result.data[1] & (std::uint64_t{1} << 63)) != std::uint64_t{0})
                {
                    result.data[0] = ~result.data[0];
                    result.data[1] = ~result.data[1];
                    result.data[0] = result.data[0] + std::uint64_t{1};
                    if (result.data[0] == std::uint64_t{0})
                    {
                        result.data[1] = result.data[1] + std::uint64_t{1};
                    }
                }
                return result;
            }
        }

        /**
         * @brief Get sign as separate value (+1, 0, -1)
         *
         * **Two's Complement:** Extract from MSB
         * **Magnitude-Sign:** Explicit sign bit
         *
         * @return -1 for negative, 0 for zero, +1 for positive
         */
        [[nodiscard]] constexpr int get_sign() const noexcept
        {
            if constexpr (!is_signed)
            {
                return is_zero() ? 0 : 1;
            }
            else
            {
                return is_zero() ? 0 : (is_negative() ? -1 : 1);
            }
        }

        /// @brief Check if value is zero
        [[nodiscard]] constexpr bool is_zero() const noexcept
        {
            if constexpr (is_magnitude_sign)
            {
                constexpr std::uint64_t magnitude_mask{(std::uint64_t{1} << 63) - std::uint64_t{1}};
                return (data[0] == std::uint64_t{0}) && ((data[1] & magnitude_mask) == std::uint64_t{0});
            }
            else if constexpr (is_excess_k)
            {
                constexpr std::uint64_t bias_high{1ull << 62};
                constexpr std::uint64_t bias_low{0ull};
                return (data[0] == bias_low) && (data[1] == bias_high);
            }
            else // twos_complement
            {
                return (data[0] == std::uint64_t{0}) && (data[1] == std::uint64_t{0});
            }
        }

        /**
         * @brief Check if value is positive zero (+0)
         *
         * Only meaningful for magnitude-sign representation.
         * In two's complement, there is only one zero.
         *
         * @return true if zero AND sign bit is 0 (positive zero)
         */
        [[nodiscard]] constexpr bool is_positive_zero() const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                return is_zero() && !is_negative();
            }
            else
            {
                return is_zero();
            }
        }

        /**
         * @brief Check if value is negative zero (-0)
         *
         * Only meaningful for magnitude-sign representation.
         * In two's complement, there is only one zero.
         *
         * @return true if zero AND sign bit is 1 (negative zero)
         */
        [[nodiscard]] constexpr bool is_negative_zero() const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                return is_zero() && is_negative();
            }
            else
            {
                return false;
            }
        }

        // ========================================================================
        // Conversions to String
        // ========================================================================

        /**
         * @brief Convert to string in specified base (2-36)
         *
         * Converts the 128-bit value to string representation in the given base.
         * - Base 2: Binary (digits 0-1)
         * - Base 8: Octal (digits 0-7)
         * - Base 10: Decimal (digits 0-9)
         * - Base 16: Hexadecimal (digits 0-9, A-F)
         * - Base 2-36: General (digits 0-9, A-Z)
         *
         * For signed types, includes the minus sign if negative.
         *
         * @param base Base for conversion (2-36). Default is 10 (decimal).
         * @return String representation in the specified base
         */
        std::string to_string(int base = 10) const noexcept
        {
            // Validate base
            if (base < 2 || base > 36)
                base = 10;

            // Handle zero - works for all representations
            if (is_zero())
                return "0";

            // Non-decimal bases use a separate noinline method so the compiler
            // does not inflate the decimal path's stack frame with buf[130].
            if (base != 10) [[unlikely]]
            {
                return to_string_nondecimal_(base);
            }

            // ================================================================
            // STEP 1: Determine if negative and extract magnitude limbs
            // ================================================================
            const bool is_negative_value{is_signed ? is_negative() : false};

            // ================================================================
            // STEP 2: Convert absolute value to string
            // ================================================================

            // ================================================================
            // Decimal path: extract magnitude limbs directly on the stack.
            // For TC signed negatives, negate via two's complement on limbs.
            // Avoids constructing a temporary int128_param_t + operator-().
            // ================================================================
            std::uint64_t n_hi{data[1]};
            std::uint64_t n_lo{data[0]};

            if constexpr (is_signed)
            {
                if (is_negative_value)
                {
                    if constexpr (is_twos_complement)
                    {
                        // Two's complement negate: ~x + 1
                        n_lo = ~n_lo + 1;
                        n_hi = ~n_hi + (n_lo == 0 ? 1 : 0);
                    }
                    else if constexpr (is_magnitude_sign)
                    {
                        // Magnitude-sign: just clear the sign bit
                        n_hi &= ~(std::uint64_t{1} << 63);
                    }
                    else /* excess_k */
                    {
                        // Excess-K: subtract bias (K = 2^126, K_low = 0)
                        // Since K_low = 0, no borrow from the low-word subtraction.
                        constexpr std::uint64_t bias_high{std::uint64_t{1} << 62};
                        n_hi = n_hi - bias_high; // uint64 wrap is correct for 128-bit
                        // {n_hi, n_lo} is now raw - K (negative, since raw < K)
                        // Negate to get absolute value.
                        n_lo = ~n_lo + 1;
                        n_hi = ~n_hi + (n_lo == 0 ? 1 : 0);
                    }
                }
                else if constexpr (is_excess_k)
                {
                    // Positive EK: subtract bias to get magnitude
                    constexpr std::uint64_t bias_high{std::uint64_t{1} << 62};
                    n_hi = n_hi - bias_high;
                }
            }

            // 10^19-chunking + native 64-bit division.
            // Divide by 10^19 via Granlund-Montgomery overflow method to extract
            // chunks of up to 19 digits that fit in uint64_t, then convert each
            // chunk with native 64-bit modulo (~1 cyc/digit vs ~30 cyc mulhi_128).
            // MAX128 ~ 3.4e38, so at most 3 chunks (2 full + 1 partial top).

            char buf[40]; // max 39 digits for uint128_t
            int pos{39};

            if (n_hi == 0)
            {
                // Fits in uint64_t -- pure native 64-bit division
                write_u64_digits(buf, pos, n_lo);
            }
            else
            {
                // 128-bit value: use GM divmod_const for chunking
                // Create temporary int128_param_t for divmod_const
                int128_param_t<signedness::unsigned_type, representation_form::binnat> temp{n_hi, n_lo};

                // Extract lowest 19-digit chunk using divmod_const<10^19>
                constexpr std::uint64_t pow19 = 10000000000000000000ull;
                auto [q1, r0_obj] = temp.template divmod_const<pow19>();
                const std::uint64_t r0 = r0_obj.low();

                if (q1.high() != 0 || q1.low() >= pow19)
                {
                    // 3 chunks: divide quotient by 10^19 again
                    auto [q2, r1_obj] = q1.template divmod_const<pow19>();
                    const std::uint64_t r1 = r1_obj.low();

                    // Chunk 0 (bottom 19 digits, zero-padded)
                    write_19_padded_digits(buf, pos, r0);
                    // Chunk 1 (middle 19 digits, zero-padded)
                    write_19_padded_digits(buf, pos, r1);
                    // Chunk 2 (top, variable length, no padding)
                    write_u64_digits(buf, pos, q2.low());
                }
                else
                {
                    // 2 chunks: quotient fits in uint64_t
                    // Chunk 0 (bottom 19 digits, zero-padded)
                    write_19_padded_digits(buf, pos, r0);
                    // Chunk 1 (top, variable length, no padding)
                    write_u64_digits(buf, pos, q1.low());
                }
            }

            const std::string result{&buf[pos], static_cast<std::size_t>(39 - pos)};

            if (is_negative_value)
            {
                return "-" + result;
            }

            return result;
        }

        /// @internal
        /// @brief Non-decimal to_string path (bases 2-9, 11-36).
        /// Kept noinline so the decimal path gets a lighter stack frame.
#if defined(_MSC_VER)
        __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
        __attribute__((noinline))
#endif
        std::string to_string_nondecimal_(int base) const noexcept
        {
            const bool is_negative_value{is_signed ? is_negative() : false};

            // Extract magnitude limbs
            std::uint64_t n_hi{data[1]};
            std::uint64_t n_lo{data[0]};

            if constexpr (is_signed)
            {
                if (is_negative_value)
                {
                    if constexpr (is_twos_complement)
                    {
                        n_lo = ~n_lo + 1;
                        n_hi = ~n_hi + (n_lo == 0 ? 1 : 0);
                    }
                    else if constexpr (is_magnitude_sign)
                    {
                        n_hi &= ~(std::uint64_t{1} << 63);
                    }
                    else /* excess_k */
                    {
                        constexpr std::uint64_t bias_high{std::uint64_t{1} << 62};
                        const bool borrow{n_lo == 0 && bias_high > 0};
                        n_lo = n_lo - 0;
                        n_hi = n_hi - bias_high - (borrow ? 1 : 0);
                        n_lo = ~n_lo + 1;
                        n_hi = ~n_hi + (n_lo == 0 ? 1 : 0);
                    }
                }
                else if constexpr (is_excess_k)
                {
                    constexpr std::uint64_t bias_high{std::uint64_t{1} << 62};
                    n_hi = n_hi - bias_high;
                }
            }

            // Check if base is a power of 2 (2, 4, 8, 16, 32)
            const bool is_pow2_base{(base & (base - 1)) == 0};

            constexpr const char *digits{"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
            // Max 129 chars: 128 binary digits + sign
            char buf[130];
            int pos{129};

            if (is_pow2_base)
            {
                // Power-of-2 base: extract groups of bits via shift+mask
                // bits_per_digit: base=2->1, 4->2, 8->3, 16->4, 32->5
                const int bits_per_digit{(base == 2) ? 1 : (base == 4) ? 2
                                                       : (base == 8)   ? 3
                                                       : (base == 16)  ? 4
                                                                       : 5};
                const std::uint64_t mask{static_cast<std::uint64_t>(base - 1)};

                // Extract digits via continuous 128-bit right shift
                // Avoids boundary artifacts for bases where 64 % bits_per_digit != 0
                while (n_hi > 0 || n_lo > 0)
                {
                    buf[--pos] = digits[n_lo & mask];
                    // 128-bit right shift by bits_per_digit
                    n_lo = (n_lo >> bits_per_digit) | (n_hi << (64 - bits_per_digit));
                    n_hi >>= bits_per_digit;
                }
            }
            else
            {
                // Generic path for non-power-of-2 bases (3, 5, 6, 7, 9, 11-15, 17-31, 33-36)
                // Multi-word division loop on raw limbs
                while (n_hi != 0 || n_lo != 0)
                {
                    std::uint64_t q_hi{0};
                    std::uint64_t remainder{0};

                    const std::uint64_t high_dividend{n_hi};
                    q_hi = high_dividend / static_cast<std::uint64_t>(base);
                    remainder = high_dividend % static_cast<std::uint64_t>(base);

                    const std::uint64_t mid_dividend{(remainder << 32) | ((n_lo >> 32) & 0xFFFFFFFFULL)};
                    const std::uint64_t mid_quotient{mid_dividend / static_cast<std::uint64_t>(base)};
                    remainder = mid_dividend % static_cast<std::uint64_t>(base);

                    const std::uint64_t low_dividend{(remainder << 32) | (n_lo & 0xFFFFFFFFULL)};
                    const std::uint64_t low_quotient{low_dividend / static_cast<std::uint64_t>(base)};
                    remainder = low_dividend % static_cast<std::uint64_t>(base);

                    n_hi = q_hi;
                    n_lo = (mid_quotient << 32) | (low_quotient & 0xFFFFFFFFULL);
                    buf[--pos] = digits[remainder];
                }
            }

            if (is_negative_value)
            {
                buf[--pos] = '-';
            }

            return std::string(&buf[pos], static_cast<std::size_t>(129 - pos));
        }

        /**
         * @brief Convert to C-string using a rotating static buffer.
         *
         * @details Returns a pointer to an internal static buffer. Uses 4
         * rotating slots so up to 4 concurrent calls can be used without
         * stomping (e.g., printf("%s %s %s %s", a.to_cstr(), b.to_cstr(), ...)).
         * NOT thread-safe. For thread-safe usage, prefer to_string().
         *
         * @param base Numeric base (2-36, default 10)
         * @return Pointer to null-terminated static buffer (valid until 4th subsequent call)
         */
        const char *to_cstr(int base = 10) const noexcept
        {
            // 4 rotating buffers: 130 chars max (128 binary digits + sign + nul)
            static thread_local char buffers[4][131];
            static thread_local int slot{0};

            char *buf{buffers[slot & 3]};
            slot = (slot + 1) & 3;

            const std::string s{to_string(base)};
            const std::size_t len{s.size() < 130 ? s.size() : 130};
            for (std::size_t i{0}; i < len; ++i)
            {
                buf[i] = s[i];
            }
            buf[len] = '\0';
            return buf;
        }

        // ========================================================================
        // Conversions from String
        // ========================================================================

        /**
         * @brief Safe constexpr parser for string input with error reporting
         *
         * @details Parses string at compile-time (constexpr). Supports:
         * - Decimal (default): "12345"
         * - Hexadecimal: "0xDEADBEEF", "0XDEADBEEF"
         * - Binary: "0b11110000", "0B11110000"
         * - Octal: "0777"
         * - Separators: ignored (underscores, single quotes)
         *
         * @param str Null-terminated string to parse
         * @return parse_result<> containing error code, value, and error index
         *
         * @code
         * constexpr auto result = int128_tc_t::parse_ct_safe("0xDEADBEEF");
         * if (result.success()) {
         *     // Use result.value
         * } else {
         *     // Check result.error and result.error_index
         * }
         * @endcode
         */
        static constexpr parse_result<int128_param_t> parse_ct_safe(const char *str) noexcept
        {
            parse_result<int128_param_t> result{};

            if (!str)
            {
                result.error = parse_error::null_pointer;
                result.error_index = 0;
                return result;
            }

            if (*str == '\0')
            {
                result.error = parse_error::empty_string;
                result.error_index = 0;
                return result;
            }

            int base{10};
            const char *ptr{str};
            std::size_t index{0};
            bool is_negative{false};

            if constexpr (is_signed)
            {
                if (*ptr == '-')
                {
                    is_negative = true;
                    ++ptr;
                    ++index;
                }
                else if (*ptr == '+')
                {
                    ++ptr;
                    ++index;
                }
            }

            if (*ptr == '0' && *(ptr + 1) != '\0')
            {
                const char next{*(ptr + 1)};
                if (next == 'x' || next == 'X')
                {
                    base = 16;
                    ptr += 2;
                    index += 2;
                }
                else if (next == 'b' || next == 'B')
                {
                    base = 2;
                    ptr += 2;
                    index += 2;
                }
                else if (next >= '0' && next <= '7')
                {
                    base = 8;
                }
            }

            // Always use unsigned accumulation to avoid false overflow at 2^127
            int128_param_t<signedness::unsigned_type, representation_form::binnat> temp_val{};
            bool found_digit{false};
            std::size_t digit_start_index{index};

            if (base == 10)
            {
                // Fast path: accumulate up to 19 digits in uint64_t per chunk,
                // then do one 128-bit multiply+add per chunk flush.
                // This reduces 128-bit multiplications from N (digits) to at most 3.
                constexpr std::uint64_t pow10[20] = {
                    1ull, 10ull, 100ull, 1000ull, 10000ull, 100000ull,
                    1000000ull, 10000000ull, 100000000ull, 1000000000ull,
                    10000000000ull, 100000000000ull, 1000000000000ull,
                    10000000000000ull, 100000000000000ull, 1000000000000000ull,
                    10000000000000000ull, 100000000000000000ull,
                    1000000000000000000ull, 10000000000000000000ull};

                std::uint64_t chunk{0};
                int chunk_digits{0};

                while (*ptr != '\0')
                {
                    const char c{*ptr};

                    if (c == '_' || c == '\'')
                    {
                        if (!found_digit && *(ptr + 1) != '\0')
                        {
                            result.error = parse_error::separator_at_boundaries;
                            result.error_index = index;
                            return result;
                        }
                        ++ptr;
                        ++index;
                        continue;
                    }

                    if (c < '0' || c > '9')
                    {
                        result.error = parse_error::invalid_character;
                        result.error_index = index;
                        return result;
                    }

                    found_digit = true;
                    chunk = chunk * 10 + static_cast<std::uint64_t>(c - '0');
                    ++chunk_digits;

                    if (chunk_digits == 19)
                    {
                        const auto old_value{temp_val};
                        temp_val = temp_val * pow10[19] + chunk;
                        if (temp_val < old_value)
                        {
                            result.error = parse_error::overflow;
                            result.error_index = index;
                            return result;
                        }
                        chunk = 0;
                        chunk_digits = 0;
                    }

                    ++ptr;
                    ++index;
                }

                // Flush remaining digits
                if (chunk_digits > 0)
                {
                    const auto old_value{temp_val};
                    temp_val = temp_val * pow10[chunk_digits] + chunk;
                    if (temp_val < old_value)
                    {
                        result.error = parse_error::overflow;
                        result.error_index = index;
                        return result;
                    }
                }
            }
            else
            {
                // Non-decimal path for bases 2-36 (except 10)
                // Detect power-of-2 bases for shift optimization
                const bool is_pow2_base{(base & (base - 1)) == 0 && base >= 2};
                const int bits_per_digit{is_pow2_base ? ((base == 2) ? 1 : (base == 4) ? 2
                                                                       : (base == 8)   ? 3
                                                                       : (base == 16)  ? 4
                                                                                       : 5)
                                                      : 0};

                while (*ptr != '\0')
                {
                    unsigned digit{0};
                    const char c{*ptr};

                    if (c == '_' || c == '\'')
                    {
                        if (!found_digit && *(ptr + 1) != '\0')
                        {
                            result.error = parse_error::separator_at_boundaries;
                            result.error_index = index;
                            return result;
                        }
                        ++ptr;
                        ++index;
                        continue;
                    }

                    if (c >= '0' && c <= '9')
                    {
                        digit = static_cast<unsigned>(c - '0');
                    }
                    else if (c >= 'a' && c <= 'z')
                    {
                        digit = static_cast<unsigned>(c - 'a' + 10);
                    }
                    else if (c >= 'A' && c <= 'Z')
                    {
                        digit = static_cast<unsigned>(c - 'A' + 10);
                    }
                    else
                    {
                        result.error = parse_error::invalid_character;
                        result.error_index = index;
                        return result;
                    }

                    if (digit >= static_cast<unsigned>(base))
                    {
                        result.error = parse_error::digit_out_of_range;
                        result.error_index = index;
                        return result;
                    }

                    found_digit = true;

                    if (is_pow2_base)
                    {
                        // Overflow: if top bits_per_digit bits of high limb are set,
                        // shifting left would lose bits (overflow 128 bits)
                        if (temp_val.high() >> (64 - bits_per_digit))
                        {
                            result.error = parse_error::overflow;
                            result.error_index = index;
                            return result;
                        }
                        temp_val = (temp_val << bits_per_digit) | static_cast<int>(digit);
                    }
                    else
                    {
                        const auto old_value{temp_val};
                        temp_val = temp_val * base + digit;

                        if (temp_val < old_value && digit != 0)
                        {
                            result.error = parse_error::overflow;
                            result.error_index = index;
                            return result;
                        }
                    }

                    ++ptr;
                    ++index;
                }
            }

            if (!found_digit)
            {
                result.error = parse_error::no_digits;
                result.error_index = digit_start_index;
                return result;
            }

            // Signed overflow check: magnitude must fit in signed range
            if constexpr (is_signed)
            {
                // Max positive magnitude: 2^127 - 1 (INT128_MAX)
                // Max negative magnitude: 2^127     (INT128_MIN = -2^127)
                constexpr auto max_positive_mag =
                    int128_param_t<signedness::unsigned_type, representation_form::binnat>{
                        0x7FFFFFFFFFFFFFFFull, 0xFFFFFFFFFFFFFFFFull};
                constexpr auto max_negative_mag =
                    int128_param_t<signedness::unsigned_type, representation_form::binnat>{
                        0x8000000000000000ull, 0x0000000000000000ull};

                if (is_negative)
                {
                    if (temp_val > max_negative_mag)
                    {
                        result.error = parse_error::overflow;
                        result.error_index = index;
                        return result;
                    }
                }
                else
                {
                    if (temp_val > max_positive_mag)
                    {
                        result.error = parse_error::overflow;
                        result.error_index = index;
                        return result;
                    }
                }
            }

            if (is_negative)
            {
                temp_val = -temp_val;
            }

            // Transfer bits from unsigned accumulator to target type
            // All paths use high()/low() which are plain uint64_t
            if constexpr (is_excess_k)
            {
                constexpr std::uint64_t bias_high{1ull << 62};
                const std::uint64_t fh{temp_val.high() + bias_high};
                result.value.set_high(fh);
                result.value.set_low(temp_val.low());
            }
            else if constexpr (is_magnitude_sign)
            {
                // After negation, check if high bit is set (was negative)
                if (is_negative)
                {
                    // Store positive magnitude with sign bit
                    const auto mag{-temp_val};
                    result.value.set_high(mag.high() | (std::uint64_t{1} << 63));
                    result.value.set_low(mag.low());
                }
                else
                {
                    result.value.set_high(temp_val.high());
                    result.value.set_low(temp_val.low());
                }
            }
            else
            {
                result.value.set_high(temp_val.high());
                result.value.set_low(temp_val.low());
            }

            result.error = parse_error::success;
            result.error_index = static_cast<std::size_t>(-1);
            return result;
        }

        /**
         * @brief Parse string with automatic base detection (constexpr)
         *
         * @details Can be evaluated at compile-time or runtime. Throws on error.
         * For safe version with error reporting, use parse_ct_safe() instead.
         *
         * @param str Null-terminated string to parse
         * @return Parsed value
         * @throw std::invalid_argument if string is invalid
         * @throw std::out_of_range if value overflows
         *
         * @code
         * // Compile-time usage:
         * constexpr auto val = int128_tc_t::from_string("0xDEADBEEF");
         *
         * // Runtime usage:
         * auto val = int128_tc_t::from_string(user_input);  // Can fail at runtime
         * @endcode
         */
        static constexpr int128_param_t from_string(const char *str)
        {
            auto safe_result = parse_ct_safe(str);
            if (!safe_result.success())
            {
                // Provide informative error message
                switch (safe_result.error)
                {
                case parse_error::null_pointer:
                    throw std::invalid_argument("Null pointer");
                case parse_error::empty_string:
                    throw std::invalid_argument("Empty string");
                case parse_error::invalid_character:
                    throw std::invalid_argument("Invalid character");
                case parse_error::digit_out_of_range:
                    throw std::invalid_argument("Digit out of range");
                case parse_error::no_digits:
                    throw std::invalid_argument("No digits found");
                case parse_error::overflow:
                    throw std::out_of_range("Number too large");
                case parse_error::separator_at_boundaries:
                    throw std::invalid_argument("Separator at invalid position");
                default:
                    throw std::invalid_argument("Parse error");
                }
            }
            return safe_result.value;
        }

        // ========================================================================
        // Byte Operations
        // ========================================================================

        /// @brief Get byte at index (0=LSB, 15=MSB, little-endian)
        constexpr std::byte get_byte(size_t index) const
        {
            if (index >= 16)
                throw std::out_of_range("byte index out of range");
            return std::byte((index < 8) ? (data[0] >> (index * 8)) : (data[1] >> ((index - 8) * 8)));
        }

        /// @brief Set byte at index
        constexpr void set_byte(size_t index, std::byte value)
        {
            if (index >= 16)
                throw std::out_of_range("byte index out of range");
            std::uint64_t &target = (index < 8) ? data[0] : data[1];
            int shift = (index < 8) ? (index * 8) : ((index - 8) * 8);
            target = (target & ~(0xFFULL << shift)) | (std::to_integer<std::uint64_t>(value) << shift);
        }

        /// @brief Reverse the byte order of the 128-bit value
        /// @return New value with bytes in reversed order
        constexpr int128_param_t byteswap() const noexcept
        {
            int128_param_t result{};
#if __has_include("intrinsics/byte_operations.hpp")
            result.data[0] = intrinsics::bswap64(data[1]);
            result.data[1] = intrinsics::bswap64(data[0]);
#else
            // Portable fallback: reverse 16 bytes manually
            for (std::size_t i{0}; i < 16; ++i)
            {
                const std::byte b{get_byte(15 - i)};
                std::uint64_t &target = (i < 8) ? result.data[0] : result.data[1];
                const int shift = (i < 8) ? static_cast<int>(i * 8) : static_cast<int>((i - 8) * 8);
                target |= (std::to_integer<std::uint64_t>(b) << shift);
            }
#endif
            return result;
        }

        /// @brief Convert to big-endian byte order (network byte order)
        /// @return Byte array in big-endian order (MSB first)
        constexpr std::array<std::byte, 16> to_big_endian() const noexcept
        {
            const int128_param_t swapped{byteswap()};
            std::array<std::byte, 16> result{};
            for (std::size_t i{0}; i < 8; ++i)
            {
                result[i] = static_cast<std::byte>((swapped.data[0] >> (i * 8)) & 0xFFu);
            }
            for (std::size_t i{0}; i < 8; ++i)
            {
                result[i + 8] = static_cast<std::byte>((swapped.data[1] >> (i * 8)) & 0xFFu);
            }
            return result;
        }

        /// @brief Construct from big-endian byte array (network byte order)
        /// @param bytes 16-byte array in big-endian order (MSB first)
        /// @return Value reconstructed from big-endian bytes
        static constexpr int128_param_t from_big_endian(const std::array<std::byte, 16> &bytes) noexcept
        {
            int128_param_t tmp{};
            for (std::size_t i{0}; i < 8; ++i)
            {
                tmp.data[0] |= (std::to_integer<std::uint64_t>(bytes[i]) << (i * 8));
            }
            for (std::size_t i{0}; i < 8; ++i)
            {
                tmp.data[1] |= (std::to_integer<std::uint64_t>(bytes[i + 8]) << (i * 8));
            }
            return tmp.byteswap();
        }

        /// @brief Convert to little-endian byte array (native x86 order)
        /// @return Byte array in little-endian order (LSB first)
        constexpr std::array<std::byte, 16> to_little_endian() const noexcept
        {
            std::array<std::byte, 16> result{};
            for (std::size_t i{0}; i < 8; ++i)
            {
                result[i] = static_cast<std::byte>((data[0] >> (i * 8)) & 0xFFu);
            }
            for (std::size_t i{0}; i < 8; ++i)
            {
                result[i + 8] = static_cast<std::byte>((data[1] >> (i * 8)) & 0xFFu);
            }
            return result;
        }

        /// @brief Construct from little-endian byte array (native x86 order)
        /// @param bytes 16-byte array in little-endian order (LSB first)
        /// @return Value reconstructed from little-endian bytes
        static constexpr int128_param_t from_little_endian(const std::array<std::byte, 16> &bytes) noexcept
        {
            int128_param_t result{};
            for (std::size_t i{0}; i < 8; ++i)
            {
                result.data[0] |= (std::to_integer<std::uint64_t>(bytes[i]) << (i * 8));
            }
            for (std::size_t i{0}; i < 8; ++i)
            {
                result.data[1] |= (std::to_integer<std::uint64_t>(bytes[i + 8]) << (i * 8));
            }
            return result;
        }

        // ========================================================================
        // Comparison Operators
        // ========================================================================

        /// @brief Equality operator (representation-aware)
        constexpr bool operator==(const int128_param_t &other) const noexcept
        {
            // Special handling for MS: +0 and -0 are mathematically equal
            if constexpr (is_magnitude_sign)
            {
                // Both zero? (magnitude bits = 0)
                if (is_zero() && other.is_zero())
                    return true; // +0 == -0 in MS
            }

            // Otherwise compare bit patterns
            return data[0] == other.data[0] && data[1] == other.data[1];
        }

        /// @brief Inequality operator (representation-agnostic)
        constexpr bool operator!=(const int128_param_t &other) const noexcept
        {
            return !(*this == other);
        }

        /**
         * @brief Less-than operator (representation-aware)
         *
         * **Two's Complement:** Standard signed/unsigned comparison
         * **Magnitude-Sign Signed:** Compare signs first, then magnitudes
         *                            For negatives: INVERTED comparison (-2 < -1 means |2| > |1|)
         */
        constexpr bool operator<(const int128_param_t &other) const noexcept
        {
            if constexpr (!is_signed)
            {
                // Unsigned: comparación directa
                if (data[1] != other.data[1])
                    return data[1] < other.data[1];
                return data[0] < other.data[0];
            }
            else if constexpr (is_magnitude_sign)
            {
                // Magnitude-Sign signed comparison
                bool this_negative = is_negative();
                bool other_negative = other.is_negative();
                if (this_negative != other_negative)
                    return this_negative;
                std::uint64_t this_mag_low = data[0];
                std::uint64_t this_mag_high = data[1] & ~(1ULL << 63);
                std::uint64_t other_mag_low = other.data[0];
                std::uint64_t other_mag_high = other.data[1] & ~(1ULL << 63);
                if (this_negative)
                {
                    if (this_mag_high != other_mag_high)
                        return this_mag_high > other_mag_high;
                    return this_mag_low > other_mag_low;
                }
                else
                {
                    if (this_mag_high != other_mag_high)
                        return this_mag_high < other_mag_high;
                    return this_mag_low < other_mag_low;
                }
            }
            else if constexpr (is_excess_k)
            {
                // Excess-K: raw stored = value + K; larger raw ↔ larger value
                // Compare raw values as unsigned 128-bit (monotone mapping)
                if (data[1] != other.data[1])
                    return data[1] < other.data[1];
                return data[0] < other.data[0];
            }
            else
            {
                // Two's Complement signed comparison
                std::int64_t this_high = static_cast<std::int64_t>(data[1]);
                std::int64_t other_high = static_cast<std::int64_t>(other.data[1]);
                if (this_high != other_high)
                    return this_high < other_high;
                return data[0] < other.data[0];
            }
        }

        /**
         * @brief Greater-than operator (representation-aware)
         */
        constexpr bool operator>(const int128_param_t &other) const noexcept
        {
            return other < *this;
        }

        /**
         * @brief Less-than-or-equal operator (representation-aware)
         */
        constexpr bool operator<=(const int128_param_t &other) const noexcept
        {
            return (*this < other) || (*this == other);
        }

        /**
         * @brief Greater-than-or-equal operator (representation-aware)
         */
        constexpr bool operator>=(const int128_param_t &other) const noexcept
        {
            return (*this > other) || (*this == other);
        }

        // ========================================================================
        // Arithmetic Operators
        // ========================================================================

        /**
         * @brief Unary plus operator
         * @return Copy of this value
         */
        constexpr int128_param_t operator+() const noexcept
        {
            return *this;
        }

        /**
         * @brief Unary negation operator (representation-aware)
         *
         * **Unsigned (binnat):** Two's complement negation (like builtin unsigned)
         * **Two's Complement:** Invert all bits and add 1
         * **Magnitude-Sign:** Flip sign bit (single bit operation)
         * **Excess-K:** Negate via: -x = bias - (x - bias) = 2·bias - x
         *
         * @return Negated value
         *
         * @note Unsigned negation follows C++ builtin behavior (wraps around)
         */
        constexpr int128_param_t operator-() const noexcept
        {
            if constexpr (!is_signed)
            {
                // Unsigned: Two's complement negation (like builtin unsigned)
                int128_param_t result;
                result.data[0] = ~data[0];
                result.data[1] = ~data[1];
                // Add 1 to low word
                ++result.data[0];
                // Propagate carry to high word
                if (result.data[0] == 0)
                    ++result.data[1];
                return result;
            }
            else if constexpr (is_magnitude_sign)
            {
                // Magnitude-Sign: Just flip the sign bit (MSB of data[1])
                int128_param_t result = *this;
                result.data[1] ^= (1ULL << 63);
                return result;
            }
            else if constexpr (is_excess_k)
            {
                // Excess-K: -x = 2·bias - x
                // bias = 2^126, so 2·bias = 2^127
                constexpr uint64_t two_bias_high = (1ULL << 63);
                constexpr uint64_t two_bias_low = 0;

                int128_param_t result;

                // Subtract this value from 2·bias
                bool borrow = false;
                if (two_bias_low < data[0])
                {
                    borrow = true;
                }
                result.data[0] = two_bias_low - data[0];

                if (borrow)
                {
                    result.data[1] = two_bias_high - data[1] - 1;
                }
                else
                {
                    result.data[1] = two_bias_high - data[1];
                }

                return result;
            }
            else // twos_complement
            {
                // Two's Complement: Invert bits and add 1
                int128_param_t result;
                result.data[0] = ~data[0];
                result.data[1] = ~data[1];
                // Add 1 to low word
                ++result.data[0];
                // Propagate carry to high word
                if (result.data[0] == 0)
                    ++result.data[1];
                return result;
            }
        }

        // ========================================================================
        // Integral Conversions
        // ========================================================================

        /// @brief Explicit conversion to bool
        ///
        /// @return true if value is non-zero, false if zero
        /// @note Representation-aware: uses is_zero() which handles EK/MS/TC/binnat
        [[nodiscard]] explicit constexpr operator bool() const noexcept
        {
            return !is_zero();
        }

        /// @brief Explicit conversion to built-in integral type
        ///
        /// @tparam T Target integral type (uint64_t, int64_t, uint32_t, int, etc.)
        /// @return Truncated value in target type
        ///
        /// @details Semantics mirror standard C++ narrowing conversions:
        ///   - For unsigned targets: modular truncation of the magnitude
        ///   - For signed targets: implementation-defined (C++20: modular)
        ///   - For MS/EK: converts to real value first, then truncates
        ///   - Values exceeding target range are truncated (no saturation)
        ///
        /// @example
        /// @code
        ///   const uint128_t big{0x1, 0x00000000DEADBEEF};
        ///   const auto lo = static_cast<uint64_t>(big);  // 0x00000000DEADBEEF
        ///   const auto lo32 = static_cast<uint32_t>(big); // 0xDEADBEEF
        /// @endcode
        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        [[nodiscard]] explicit constexpr operator T() const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: extract magnitude, apply sign
                const std::uint64_t mag_low{data[0]};
                const bool negative{(data[1] & (1ULL << 63)) != 0};
                if constexpr (std::is_signed_v<T>)
                {
                    const auto unsigned_val{static_cast<std::make_unsigned_t<T>>(mag_low)};
                    return negative
                               ? static_cast<T>(-static_cast<std::make_unsigned_t<T>>(unsigned_val))
                               : static_cast<T>(unsigned_val);
                }
                else
                {
                    // unsigned target from signed source: modular
                    if (negative)
                    {
                        return static_cast<T>(-static_cast<std::uint64_t>(mag_low));
                    }
                    return static_cast<T>(mag_low);
                }
            }
            else if constexpr (is_excess_k && is_signed)
            {
                // EK: stored - bias = real value
                constexpr std::uint64_t bias_high{1ULL << 62};
                // Subtract bias to get TC-like value
                const std::uint64_t borrow{(data[0] < 0ULL) ? 1ULL : 0ULL};
                const std::uint64_t real_low{data[0] - 0ULL}; // bias_low == 0
                const std::uint64_t real_high{data[1] - bias_high - borrow};
                // Now real_high:real_low is in TC interpretation
                return static_cast<T>(real_low);
            }
            else
            {
                // TC and binnat: direct truncation (C++20 modular semantics)
                return static_cast<T>(data[0]);
            }
        }

        // ========================================================================
        // Float/Double Conversions (Priority 10)
        // ========================================================================

        /**
         * @brief Explicit conversion to double
         *
         * @return Double representation of this value
         *
         * @details
         * - Precision loss may occur (double has 52-bit mantissa)
         * - For MS signed: converts magnitude, applies sign
         * - For TC signed: handles negative values via two's complement
         * - Large values may lose precision or become infinity
         *
         * @example
         * uint128_tc_t x{0, 100};
         * double d = static_cast<double>(x);  // d = 100.0
         */
        [[nodiscard]] explicit constexpr operator double() const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                const bool negative{(data[1] & (1ULL << 63)) != 0};
                const double result{static_cast<double>(mag_high) * 18446744073709551616.0 +
                                    static_cast<double>(data[0])};
                return negative ? -result : result;
            }
            else if constexpr (is_signed)
            {
                if (is_negative())
                {
                    const int128_param_t abs_val{-(*this)};
                    const int128_param_t<signedness::unsigned_type, representation_form::binnat> unsigned_val{abs_val.high(), abs_val.low()};
                    return -static_cast<double>(unsigned_val);
                }
            }
            return static_cast<double>(data[1]) * 18446744073709551616.0 +
                   static_cast<double>(data[0]);
        }

        /**
         * @brief Explicit conversion to long double
         *
         * @return Long double representation of this value
         *
         * @details
         * - Better precision than double (typically 64-bit mantissa on x86)
         * - Same overflow considerations as double
         * - Representation-aware for MS signed values
         */
        [[nodiscard]] explicit constexpr operator long double() const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                const bool negative{(data[1] & (1ULL << 63)) != 0};
                const long double result{static_cast<long double>(mag_high) * 18446744073709551616.0L +
                                         static_cast<long double>(data[0])};
                return negative ? -result : result;
            }
            else if constexpr (is_signed)
            {
                if (is_negative())
                {
                    const int128_param_t abs_val{-(*this)};
                    const int128_param_t<signedness::unsigned_type, representation_form::binnat> unsigned_val{abs_val.high(), abs_val.low()};
                    return -static_cast<long double>(unsigned_val);
                }
            }
            return static_cast<long double>(data[1]) * 18446744073709551616.0L +
                   static_cast<long double>(data[0]);
        }

        // ========================================================================
        // Cross-Representation Explicit Cast Operators
        // ========================================================================

        /// @brief Explicit conversion to another int128_param_t representation
        ///
        /// Enables: static_cast<int128_tc_t>(ms_value)
        ///          static_cast<uint128_t>(tc_value)
        ///          etc.
        ///
        /// Uses the same conversion strategy as cross-representation constructors:
        ///   binnat ↔ TC: C++20 modular bit reinterpretation
        ///   TC ↔ MS, TC ↔ EK, MS ↔ EK: via representation.hpp conversion functions
        ///   binnat ↔ MS, binnat ↔ EK: via TC as pivot
        ///
        /// @tparam S2 Target signedness
        /// @tparam F2 Target representation form
        /// @return New int128_param_t<S2, F2> with converted value
        template <signedness S2, representation_form F2,
                  typename = std::enable_if_t<(S2 != Sign) || (F2 != Form)>>
        [[nodiscard]] explicit constexpr operator int128_param_t<S2, F2>() const noexcept
        {
            return int128_param_t<S2, F2>{*this};
        }

        /**
         * @brief Constructor from float (explicit)
         *
         * @param value Float value to convert (delegates to double constructor)
         *
         * @details Converts to double and uses double constructor.
         * Supports TC, MS, and EK representations.
         */
        explicit constexpr int128_param_t(float value) noexcept
            : int128_param_t(static_cast<double>(value)) {}

        /**
         * @brief Constructor from double (explicit)
         *
         * @param value Double value to convert
         *
         * @details
         * - Truncates fractional part
         * - Handles negative values appropriately for TC/MS
         * - Overflow/underflow behavior: saturates to max/min value
         *
         * @note Requires std::isfinite(), std::isnan() checks at runtime
         */
        explicit constexpr int128_param_t(double value) noexcept
            : data{0, 0}
        {
            // Handle special values
            if (value != value) // NaN check
            {
                // For EK, zero is represented as bias K
                if constexpr (is_excess_k && is_signed)
                {
                    data[1] = 1ULL << 62; // Set bias K
                }
                return;
            }

            const bool negative{value < 0.0};
            double abs_val{negative ? -value : value};

            // Handle overflow (too large for 128-bit)
            if (abs_val >= 340282366920938463463374607431768211456.0) // 2^128
            {
                // Saturate to max
                if constexpr (is_signed)
                {
                    if (negative)
                    {
                        // Set to minimum value
                        data[0] = 0;
                        data[1] = 0x8000000000000000ULL;
                        return;
                    }
                }
                // Max value
                data[0] = ~0ULL;
                data[1] = ~0ULL;
                return;
            }

            // Extract high and low parts
            if (abs_val >= 18446744073709551616.0) // 2^64
            {
                const double high_part{abs_val / 18446744073709551616.0};
                data[1] = static_cast<uint64_t>(high_part);
                abs_val -= static_cast<double>(data[1]) * 18446744073709551616.0;
            }

            data[0] = static_cast<uint64_t>(abs_val);

            // Apply sign for signed types
            if constexpr (is_signed)
            {
                if (negative)
                {
                    if constexpr (is_magnitude_sign)
                    {
                        // MS: set sign bit
                        data[1] |= (1ULL << 63);
                    }
                    else if constexpr (is_excess_k)
                    {
                        // EK: for negative, compute K - magnitude
                        // Instead of storing magnitude, we need to store (bias - magnitude)
                        // This is done by negating first, then adding bias below
                        std::uint64_t new_low{~data[0] + 1ULL};
                        std::uint64_t carry{(new_low < 1ULL) ? 1ULL : 0ULL};
                        data[0] = new_low;
                        data[1] = ~data[1] + carry;
                    }
                    else
                    {
                        // TC: negate
                        *this = -*this;
                    }
                }
            }

            // Add bias for Excess-K (ALWAYS for EK, whether positive or negative)
            if constexpr (is_excess_k && is_signed)
            {
                constexpr std::uint64_t bias_high{1ULL << 62};
                constexpr std::uint64_t bias_low{0ULL};
                std::uint64_t sum_low{data[0] + bias_low};
                std::uint64_t carry{(sum_low < data[0]) ? 1ULL : 0ULL};
                data[0] = sum_low;
                data[1] = data[1] + bias_high + carry;
            }
        }

        /**
         * @brief Constructor from long double (explicit)
         *
         * @param value Long double value to convert
         *
         * @details Same behavior as double constructor but with better precision
         */
        explicit constexpr int128_param_t(long double value) noexcept
            : data{0, 0}
        {
            // Handle special values
            if (value != value) // NaN
            {
                // For EK, zero is represented as bias K
                if constexpr (is_excess_k && is_signed)
                {
                    data[1] = 1ULL << 62; // Set bias K
                }
                return;
            }

            const bool negative{value < 0.0L};
            long double abs_val{negative ? -value : value};

            // Handle overflow
            if (abs_val >= 340282366920938463463374607431768211456.0L) // 2^128
            {
                if constexpr (is_signed)
                {
                    if (negative)
                    {
                        data[0] = 0;
                        data[1] = 0x8000000000000000ULL;
                        return;
                    }
                }
                data[0] = ~0ULL;
                data[1] = ~0ULL;
                return;
            }

            // Extract parts
            if (abs_val >= 18446744073709551616.0L) // 2^64
            {
                const long double high_part{abs_val / 18446744073709551616.0L};
                data[1] = static_cast<uint64_t>(high_part);
                abs_val -= static_cast<long double>(data[1]) * 18446744073709551616.0L;
            }

            data[0] = static_cast<uint64_t>(abs_val);

            // Apply sign
            if constexpr (is_signed)
            {
                if (negative)
                {
                    if constexpr (is_magnitude_sign)
                    {
                        data[1] |= (1ULL << 63);
                    }
                    else if constexpr (is_excess_k)
                    {
                        // EK: for negative, compute K - magnitude
                        // Negate first, then add bias below gives us K + (-magnitude)
                        std::uint64_t new_low{~data[0] + 1ULL};
                        std::uint64_t carry{(new_low < 1ULL) ? 1ULL : 0ULL};
                        data[0] = new_low;
                        data[1] = ~data[1] + carry;
                    }
                    else
                    {
                        *this = -*this;
                    }
                }
            }

            // Add bias for Excess-K (ALWAYS for EK, whether positive or negative)
            if constexpr (is_excess_k && is_signed)
            {
                constexpr std::uint64_t bias_high{1ULL << 62};
                constexpr std::uint64_t bias_low{0ULL};
                std::uint64_t sum_low{data[0] + bias_low};
                std::uint64_t carry{(sum_low < data[0]) ? 1ULL : 0ULL};
                data[0] = sum_low;
                data[1] = data[1] + bias_high + carry;
            }
        }

        // ========================================================================
        // Array & Bitset Conversions (Priority 11)
        // ========================================================================

        /**
         * @brief Convert to std::array of bytes (little-endian)
         *
         * Serializes the 128-bit value to a 16-byte array in little-endian order.
         * - Bytes [0..7] = low 64 bits (data[0])
         * - Bytes [8..15] = high 64 bits (data[1])
         *
         * For MS representation, the sign bit is included in the serialization.
         *
         * @return std::array<std::byte, 16> containing the byte representation
         *
         * @note Little-endian byte order matches internal storage (data[0]=low)
         *
         * @code
         * uint128_tc_t x{0x1234567890ABCDEFULL, 0xFEDCBA0987654321ULL};
         * auto bytes = static_cast<std::array<std::byte, 16>>(x);
         * // bytes[0] = 0xEF, bytes[1] = 0xCD, ...
         * // bytes[15] = 0xFE
         * @endcode
         */
        explicit constexpr operator std::array<std::byte, 16>() const noexcept
        {
            std::array<std::byte, 16> result{};

            // Low 64 bits (data[0]) → bytes [0..7]
            for (int i{0}; i < 8; ++i)
            {
                result[i] = static_cast<std::byte>((data[0] >> (i * 8)) & 0xFF);
            }

            // High 64 bits (data[1]) → bytes [8..15]
            for (int i{0}; i < 8; ++i)
            {
                result[i + 8] = static_cast<std::byte>((data[1] >> (i * 8)) & 0xFF);
            }

            return result;
        }

        /**
         * @brief Convert to std::bitset<128>
         *
         * Creates a bitset representation where:
         * - bit 0 = LSB (least significant bit of data[0])
         * - bit 63 = MSB of data[0]
         * - bit 64 = LSB of data[1]
         * - bit 127 = MSB (for MS signed: this is the sign bit)
         *
         * @return std::bitset<128> with bits set according to internal storage
         *
         * @code
         * uint128_tc_t x{0xFF};
         * auto bits = static_cast<std::bitset<128>>(x);
         * // bits[0..7] = 1, bits[8..127] = 0
         * @endcode
         */
        explicit constexpr operator std::bitset<128>() const noexcept
        {
            std::bitset<128> result{};

            // Set bits from low limb (data[0])
            for (int i{0}; i < 64; ++i)
            {
                if ((data[0] & (1ULL << i)) != 0)
                {
                    result.set(i);
                }
            }

            // Set bits from high limb (data[1])
            for (int i{0}; i < 64; ++i)
            {
                if ((data[1] & (1ULL << i)) != 0)
                {
                    result.set(i + 64);
                }
            }

            return result;
        }

        /**
         * @brief Construct from std::array of bytes (little-endian)
         *
         * Deserializes a 16-byte array in little-endian order to a 128-bit value.
         * - Bytes [0..7] → low 64 bits (data[0])
         * - Bytes [8..15] → high 64 bits (data[1])
         *
         * For MS signed types, the sign bit (MSB of byte[15]) is preserved.
         *
         * @param bytes Byte array in little-endian order
         *
         * @note This is the inverse operation of operator std::array<std::byte, 16>()
         *
         * @code
         * std::array<std::byte, 16> bytes{};
         * bytes[0] = std::byte{0xFF};
         * bytes[1] = std::byte{0x00};
         * // ... (remaining bytes)
         * uint128_tc_t x{bytes};  // Deserializes from byte array
         * @endcode
         */
        explicit constexpr int128_param_t(const std::array<std::byte, 16> &bytes) noexcept
            : data{0, 0}
        {
            for (int i{0}; i < 8; ++i)
            {
                data[0] |= (static_cast<std::uint64_t>(bytes[i]) << (i * 8));
            }
            for (int i{0}; i < 8; ++i)
            {
                data[1] |= (static_cast<std::uint64_t>(bytes[i + 8]) << (i * 8));
            }
        }

        /**
         * @brief Construct from std::bitset<128>
         *
         * Creates an int128 value from a bitset where:
         * - bit 0 = LSB (least significant bit of data[0])
         * - bit 63 = MSB of data[0]
         * - bit 64 = LSB of data[1]
         * - bit 127 = MSB (for MS signed: this becomes the sign bit)
         *
         * @param bits Bitset with 128 bits
         *
         * @note This is the inverse operation of operator std::bitset<128>()
         *
         * @code
         * std::bitset<128> bits{};
         * bits.set(0);  // Set LSB
         * bits.set(127);  // Set MSB
         * uint128_tc_t x{bits};
         * @endcode
         */
        explicit constexpr int128_param_t(const std::bitset<128> &bits) noexcept
            : data{0, 0}
        {
            for (int i{0}; i < 64; ++i)
            {
                if (bits.test(i))
                {
                    data[0] |= (std::uint64_t{1} << i);
                }
            }
            for (int i{0}; i < 64; ++i)
            {
                if (bits.test(i + 64))
                {
                    data[1] |= (std::uint64_t{1} << i);
                }
            }
        }

        // ========================================================================
        // Arithmetic Operations
        // ========================================================================

        /**
         * @brief Pre-increment operator (SEMANTIC for all representations)
         *
         * **Two's Complement (TC):** SEMANTIC - Standard binary +1 matches real value increment
         * **Magnitude-Sign (MS):** SEMANTIC - Increments magnitude with sign handling
         * **Excess-K (EK):** SEMANTIC - Binary +1 on stored value
         *   - Real value x → x+1: stored (x+K) + 1 = (x+1)+K ✓
         *   - No bias compensation needed (adding constant to real value)
         *
         * @return Reference to incremented value
         */
        constexpr int128_param_t &operator++() noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS signed: check if negative
                if (is_negative())
                {
                    // Special case: -0 → +1 (symmetric to -- handling +0 → -1)
                    if (data[0] == 0 && (data[1] & ~(1ULL << 63)) == 0)
                    {
                        data[0] = 1;
                        data[1] = 0; // clear sign bit, magnitude = 1 → value = +1
                    }
                    else
                    {
                        // Negative: decrement magnitude (moves toward zero)
                        if (data[0] == 0)
                        {
                            --data[1];
                        }
                        --data[0];
                        // Clear sign bit if magnitude becomes zero
                        if (data[0] == 0 && (data[1] & ~(1ULL << 63)) == 0)
                        {
                            data[1] &= ~(1ULL << 63);
                        }
                    }
                }
                else
                {
                    // Positive: increment magnitude
                    ++data[0];
                    if (data[0] == 0)
                    {
                        ++data[1];
                    }
                }
            }
            else
            {
                // TC, EK, and unsigned: standard binary increment
                // For EK: (x+K) + 1 = (x+1) + K (correct representation)
                ++data[0];
                if (data[0] == 0)
                {
                    ++data[1];
                }
            }
            return *this;
        }

        /**
         * @brief Post-increment operator (native for all representations)
         * @param Dummy parameter (unused)
         * @return Copy of original value before increment
         */
        constexpr int128_param_t operator++(int) noexcept
        {
            int128_param_t temp = *this;
            ++(*this);
            return temp;
        }

        /**
         * @brief Pre-decrement operator (SEMANTIC for all representations)
         *
         * **Two's Complement (TC):** SEMANTIC - Standard binary -1 matches real value decrement
         * **Magnitude-Sign (MS):** SEMANTIC - Decrements magnitude with sign handling
         * **Excess-K (EK):** SEMANTIC - Binary -1 on stored value
         *   - Real value x → x-1: stored (x+K) - 1 = (x-1)+K ✓
         *   - No bias compensation needed (subtracting constant from real value)
         *
         * @return Reference to decremented value
         */
        constexpr int128_param_t &operator--() noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS signed: check if negative or positive zero
                if (is_negative())
                {
                    // Negative: increment magnitude (moves away from zero)
                    ++data[0];
                    if (data[0] == 0)
                    {
                        ++data[1];
                    }
                }
                else if (is_zero())
                {
                    // +0 → -1: set magnitude to 1 and sign bit
                    data[0] = 1;
                    data[1] = (1ULL << 63);
                }
                else
                {
                    // Positive: decrement magnitude
                    if (data[0] == 0)
                    {
                        --data[1];
                    }
                    --data[0];
                }
            }
            else
            {
                // TC, EK, and unsigned: standard binary decrement
                // For EK: (x+K) - 1 = (x-1) + K (correct representation)
                if (data[0] == 0)
                {
                    --data[1];
                }
                --data[0];
            }
            return *this;
        }

        /**
         * @brief Post-decrement operator (native for all representations)
         * @param Dummy parameter (unused)
         * @return Copy of original value before decrement
         */
        constexpr int128_param_t operator--(int) noexcept
        {
            int128_param_t temp = *this;
            --(*this);
            return temp;
        }

        /**
         * @brief Pure increment — returns value + 1 without modifying the original
         *
         * Equivalent to `x + 1` but implemented via ++copy for correctness
         * across all representations (TC, MS, EK, unsigned).
         *
         * @return New value equal to this + 1
         */
        [[nodiscard]] constexpr int128_param_t incr() const noexcept
        {
            int128_param_t result{*this};
            ++result;
            return result;
        }

        /**
         * @brief Pure decrement — returns value - 1 without modifying the original
         *
         * Equivalent to `x - 1` but implemented via --copy for correctness
         * across all representations (TC, MS, EK, unsigned).
         *
         * @return New value equal to this - 1
         */
        [[nodiscard]] constexpr int128_param_t decr() const noexcept
        {
            int128_param_t result{*this};
            --result;
            return result;
        }

        /**
         * @brief Addition assignment operator (SEMANTIC for all representations)
         *
         * **Two's Complement (TC):** SEMANTIC - Binary addition matches real value addition
         * **Magnitude-Sign (MS):** SEMANTIC - Binary addition on magnitude (unsigned behavior)
         * **Excess-K (EK):** SEMANTIC - Native implementation with bias compensation
         *   - Real: (x) + (y) = (x+y)
         *   - Stored: (x+K) + (y+K) = (x+y) + 2K
         *   - Compensation: Subtract K to get (x+y) + K (correct stored result)
         *   - Avoids expensive conversion to TC
         *
         * @param other Value to add
         * @return Reference to this (modified)
         */
        constexpr int128_param_t &operator+=(const int128_param_t &other) noexcept
        {
            if constexpr (is_excess_k)
            {
                // Suma en Excess-K: (x - K) + (y - K) = (x + y) - K
                // K = 2^126 = 0x4000000000000000 (high), 0x0 (low)
                constexpr std::uint64_t bias_high = (1ULL << 62);
                constexpr std::uint64_t bias_low = 0ULL;

                // Suma x + y usando intrinsics
#if __has_include("intrinsics/arithmetic_operations.hpp")
                if (!std::is_constant_evaluated())
                {
                    unsigned char carry = intrinsics::addcarry_u64(0, data[0], other.data[0], &data[0]);
                    std::uint64_t sum_high;
                    intrinsics::addcarry_u64(carry, data[1], other.data[1], &sum_high);

                    // Restar bias (K) - también con intrinsics
                    unsigned char borrow = intrinsics::subborrow_u64(0, data[0], bias_low, &data[0]);
                    intrinsics::subborrow_u64(borrow, sum_high, bias_high, &data[1]);
                    return *this;
                }
#endif
                // Fallback constexpr portable
                std::uint64_t sum_low = data[0] + other.data[0];
                std::uint64_t carry = (sum_low < data[0]) ? 1 : 0;
                std::uint64_t sum_high = data[1] + other.data[1] + carry;

                // Restar bias (K)
                std::uint64_t new_low = sum_low - bias_low;
                std::uint64_t borrow = (sum_low < bias_low) ? 1 : 0;
                std::uint64_t new_high = sum_high - bias_high - borrow;

                data[0] = new_low;
                data[1] = new_high;
                return *this;
            }
            else if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: Reglas de suma con signo y magnitud
                const bool lhs_neg = is_negative();
                const bool rhs_neg = other.is_negative();

                // Extraer magnitudes (sin bit de signo)
                std::uint64_t lhs_mag_low = data[0];
                std::uint64_t lhs_mag_high = data[1] & 0x7FFFFFFFFFFFFFFFULL;
                std::uint64_t rhs_mag_low = other.data[0];
                std::uint64_t rhs_mag_high = other.data[1] & 0x7FFFFFFFFFFFFFFFULL;

                if (lhs_neg == rhs_neg)
                {
                    // Mismo signo: sumar magnitudes, preservar signo
#if __has_include("intrinsics/arithmetic_operations.hpp")
                    if (!std::is_constant_evaluated())
                    {
                        std::uint64_t result_low, result_high;
                        unsigned char carry = intrinsics::addcarry_u64(0, lhs_mag_low, rhs_mag_low, &result_low);
                        intrinsics::addcarry_u64(carry, lhs_mag_high, rhs_mag_high, &result_high);

                        data[0] = result_low;
                        data[1] = result_high & 0x7FFFFFFFFFFFFFFFULL;

                        if (lhs_neg)
                        {
                            data[1] |= 0x8000000000000000ULL;
                        }
                        return *this;
                    }
#endif
                    // Fallback constexpr portable
                    std::uint64_t new_low = lhs_mag_low + rhs_mag_low;
                    std::uint64_t carry = (new_low < lhs_mag_low) ? 1 : 0;
                    std::uint64_t new_high = lhs_mag_high + rhs_mag_high + carry;

                    data[0] = new_low;
                    data[1] = new_high & 0x7FFFFFFFFFFFFFFFULL; // Limpiar bit de signo

                    // Aplicar signo
                    if (lhs_neg)
                    {
                        data[1] |= 0x8000000000000000ULL;
                    }
                }
                else
                {
                    // Distinto signo: restar magnitudes, signo del mayor
                    // Comparar magnitudes
                    bool lhs_greater = (lhs_mag_high > rhs_mag_high) ||
                                       (lhs_mag_high == rhs_mag_high && lhs_mag_low >= rhs_mag_low);

                    std::uint64_t new_low, new_high;
                    bool result_neg;

#if __has_include("intrinsics/arithmetic_operations.hpp")
                    if (!std::is_constant_evaluated())
                    {
                        if (lhs_greater)
                        {
                            unsigned char borrow = intrinsics::subborrow_u64(0, lhs_mag_low, rhs_mag_low, &new_low);
                            intrinsics::subborrow_u64(borrow, lhs_mag_high, rhs_mag_high, &new_high);
                            result_neg = lhs_neg;
                        }
                        else
                        {
                            unsigned char borrow = intrinsics::subborrow_u64(0, rhs_mag_low, lhs_mag_low, &new_low);
                            intrinsics::subborrow_u64(borrow, rhs_mag_high, lhs_mag_high, &new_high);
                            result_neg = rhs_neg;
                        }
                    }
                    else
#endif
                    {
                        // Fallback constexpr portable
                        if (lhs_greater)
                        {
                            // |lhs| >= |rhs|: resultado = |lhs| - |rhs|, signo de lhs
                            new_low = lhs_mag_low - rhs_mag_low;
                            std::uint64_t borrow = (new_low > lhs_mag_low) ? 1 : 0;
                            new_high = lhs_mag_high - rhs_mag_high - borrow;
                            result_neg = lhs_neg;
                        }
                        else
                        {
                            // |rhs| > |lhs|: resultado = |rhs| - |lhs|, signo de rhs
                            new_low = rhs_mag_low - lhs_mag_low;
                            std::uint64_t borrow = (new_low > rhs_mag_low) ? 1 : 0;
                            new_high = rhs_mag_high - lhs_mag_high - borrow;
                            result_neg = rhs_neg;
                        }
                    }

                    data[0] = new_low;
                    data[1] = new_high & 0x7FFFFFFFFFFFFFFFULL;

                    // Aplicar signo (solo si no es cero)
                    if (result_neg && !(new_low == 0 && new_high == 0))
                    {
                        data[1] |= 0x8000000000000000ULL;
                    }
                }
                return *this;
            }
            else
            {
                // TC y unsigned: suma binaria estándar con intrinsics optimizados
#if __has_include("intrinsics/arithmetic_operations.hpp")
                if (!std::is_constant_evaluated())
                {
                    // Optimized path: Full 128-bit ADD+ADC via __uint128_t on GCC/Clang,
                    // or _addcarry_u64 chain on MSVC
                    intrinsics::add128(data[0], data[1], other.data[0], other.data[1], &data[0], &data[1]);
                    return *this;
                }
#endif
                // Fallback constexpr portable
                std::uint64_t new_low = data[0] + other.data[0];
                std::uint64_t carry = (new_low < data[0]) ? 1 : 0;
                data[0] = new_low;
                data[1] = data[1] + other.data[1] + carry;
                return *this;
            }
        }

        /**
         * @brief Addition operator
         * @param other Value to add
         * @return Sum of this and other
         */
        constexpr int128_param_t operator+(const int128_param_t &other) const noexcept
        {
            int128_param_t result = *this;
            result += other;
            return result;
        }

        /**
         * @brief Subtraction assignment operator (SEMANTIC for all representations)
         *
         * **Two's Complement (TC):** SEMANTIC - Binary subtraction matches real value subtraction
         * **Magnitude-Sign (MS):** SEMANTIC - Binary subtraction on magnitude (unsigned behavior)
         * **Excess-K (EK):** SEMANTIC - Native implementation with bias compensation
         *   - Real: (x) - (y) = (x-y)
         *   - Stored: (x+K) - (y+K) = (x-y) (bias cancels!)
         *   - Compensation: Add K to get (x-y) + K (correct stored result)
         *   - Avoids expensive conversion to TC
         *
         * @param other Value to subtract
         * @return Reference to this (modified)
         */
        constexpr int128_param_t &operator-=(const int128_param_t &other) noexcept
        {
            if constexpr (is_excess_k)
            {
                // Resta en Excess-K: (x - K) - (y - K) = (x - y) + K
                // K = 2^126 = 0x4000000000000000 (high), 0x0 (low)
                constexpr std::uint64_t bias_high = (1ULL << 62);
                constexpr std::uint64_t bias_low = 0ULL;

#if __has_include("intrinsics/arithmetic_operations.hpp")
                if (!std::is_constant_evaluated())
                {
                    // Optimized path: Use hardware intrinsics (SBB and ADC instructions)
                    std::uint64_t diff_low{0};
                    std::uint64_t diff_high{0};

                    // Step 1: Subtract x - y using SBB (subtract with borrow)
                    unsigned char borrow = intrinsics::subborrow_u64(0, data[0], other.data[0], &diff_low);
                    intrinsics::subborrow_u64(borrow, data[1], other.data[1], &diff_high);

                    // Step 2: Add bias (K) using ADC (add with carry)
                    unsigned char carry = intrinsics::addcarry_u64(0, diff_low, bias_low, &data[0]);
                    intrinsics::addcarry_u64(carry, diff_high, bias_high, &data[1]);

                    return *this;
                }
#endif
                // Portable fallback for constexpr contexts
                // Restar x - y
                std::uint64_t diff_low = data[0] - other.data[0];
                std::uint64_t borrow = (diff_low > data[0]) ? 1 : 0;
                std::uint64_t diff_high = data[1] - other.data[1] - borrow;

                // Sumar bias (K)
                std::uint64_t new_low = diff_low + bias_low;
                std::uint64_t carry = (new_low < diff_low) ? 1 : 0;
                std::uint64_t new_high = diff_high + bias_high + carry;

                data[0] = new_low;
                data[1] = new_high;
                return *this;
            }
            else if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: a - b = a + (-b), negar el signo de other y sumar
                int128_param_t negated_other = other;

                // Negar signo de other (si no es cero)
                if (!(other.data[0] == 0 && (other.data[1] & 0x7FFFFFFFFFFFFFFFULL) == 0))
                {
                    negated_other.data[1] ^= 0x8000000000000000ULL; // Toggle sign bit
                }

                // Delegar a operator+= (que ya está optimizado con intrinsics)
                return operator+=(negated_other);
            }
            else
            {
                // TC y unsigned: resta binaria estándar
#if __has_include("intrinsics/arithmetic_operations.hpp")
                if (!std::is_constant_evaluated())
                {
                    // Optimized path: Full 128-bit SUB+SBB via __uint128_t on GCC/Clang,
                    // or _subborrow_u64 chain on MSVC
                    intrinsics::sub128(data[0], data[1], other.data[0], other.data[1], &data[0], &data[1]);
                    return *this;
                }
#endif
                // Portable fallback for constexpr contexts
                std::uint64_t new_low = data[0] - other.data[0];
                std::uint64_t borrow = (new_low > data[0]) ? 1 : 0;
                data[0] = new_low;
                data[1] = data[1] - other.data[1] - borrow;
                return *this;
            }
        }

        /**
         * @brief Subtraction operator
         * @param other Value to subtract
         * @return Difference of this and other
         */
        constexpr int128_param_t operator-(const int128_param_t &other) const noexcept
        {
            int128_param_t result = *this;
            result -= other;
            return result;
        }

        /**
         * @brief Multiplication assignment operator
         *
         * **Two's Complement (TC):** SEMANTIC - Binary multiplication matches real value multiplication
         * **Magnitude-Sign (MS):** SEMANTIC - Extracts magnitudes, multiplies, applies sign rule
         * **Excess-K (EK):** DELETED - Multiplication has no meaningful semantics for biased exponents.
         *   Use operator+ / operator- for IEEE 754 exponent arithmetic.
         *
         * Note: Only handles 64-bit × 64-bit → 128-bit products efficiently.
         * Full 128-bit × 128-bit requires more complex logic.
         *
         * @param other Value to multiply
         * @return Reference to this (modified)
         */
        constexpr int128_param_t &operator*=(const int128_param_t &) noexcept
            requires(is_excess_k)
        = delete;

        constexpr int128_param_t &operator*=(const int128_param_t &other) noexcept
            requires(!is_excess_k)
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: Extract magnitudes, multiply, apply sign rule
                const bool lhs_neg{is_negative()};
                const bool rhs_neg{other.is_negative()};
                const bool result_neg{lhs_neg != rhs_neg}; // XOR for sign

                // Extract magnitudes (clear sign bit)
                std::uint64_t a_low = data[0];
                std::uint64_t a_high = data[1] & ~(1ULL << 63); // Clear sign bit
                std::uint64_t b_low = other.data[0];
                std::uint64_t b_high = other.data[1] & ~(1ULL << 63); // Clear sign bit

#if __has_include("intrinsics/arithmetic_operations.hpp")
                if (!std::is_constant_evaluated())
                {
                    // Optimized path: Use umul128 intrinsics for 128-bit multiplication
                    // 128×128 → 256-bit multiplication, keep low 128 bits
                    // Formula: (a_high*2^64 + a_low) × (b_high*2^64 + b_low)
                    //        = a_low×b_low + 2^64(a_high×b_low + a_low×b_high) + 2^128(a_high×b_high)
                    //        We only need low 128 bits, so ignore terms ≥2^128

                    std::uint64_t high_part{0};
                    data[0] = intrinsics::umul128(a_low, b_low, &high_part);   // a_low × b_low → (low, high)
                    data[1] = high_part + (a_high * b_low) + (a_low * b_high); // Add cross terms
                }
                else
#endif
                {
                    // Portable fallback for constexpr contexts
                    // Compute high 64 bits of a_low × b_low via schoolbook on 32-bit halves
                    const std::uint64_t a_lo32 = a_low & 0xFFFFFFFFULL;
                    const std::uint64_t a_hi32 = a_low >> 32;
                    const std::uint64_t b_lo32 = b_low & 0xFFFFFFFFULL;
                    const std::uint64_t b_hi32 = b_low >> 32;
                    const std::uint64_t t0 = a_lo32 * b_lo32;
                    const std::uint64_t t1 = a_lo32 * b_hi32 + (t0 >> 32);
                    const std::uint64_t t2 = a_hi32 * b_lo32 + (t1 & 0xFFFFFFFFULL);
                    const std::uint64_t p00_hi = a_hi32 * b_hi32 + (t1 >> 32) + (t2 >> 32);
                    data[0] = (t2 << 32) | (t0 & 0xFFFFFFFFULL);
                    data[1] = p00_hi + (a_high * b_low) + (a_low * b_high);
                }

                // Clear sign bit (magnitude multiplication may overflow into it)
                data[1] &= ~(std::uint64_t{1} << 63);

                // Apply sign only if result is non-zero and sign rule says negative
                if (result_neg && (data[0] != 0 || data[1] != 0))
                {
                    data[1] |= (std::uint64_t{1} << 63);
                }
            }
            else
            {
                // TC and unsigned: binary multiplication
                std::uint64_t a_low = data[0];
                std::uint64_t a_high = data[1];
                std::uint64_t b_low = other.data[0];
                std::uint64_t b_high = other.data[1];

#if __has_include("intrinsics/arithmetic_operations.hpp")
                if (!std::is_constant_evaluated())
                {
                    // Optimized path: Use umul128 intrinsics for 128-bit multiplication
                    std::uint64_t high_part{0};
                    data[0] = intrinsics::umul128(a_low, b_low, &high_part);   // a_low × b_low → (low, high)
                    data[1] = high_part + (a_high * b_low) + (a_low * b_high); // Add cross terms
                    return *this;
                }
#endif
                // Portable fallback for constexpr contexts
#if defined(__SIZEOF_INT128__)
                const __uint128_t p00 = static_cast<__uint128_t>(a_low) * b_low;
                data[0] = static_cast<std::uint64_t>(p00);
                data[1] = static_cast<std::uint64_t>(p00 >> 64) + (a_high * b_low) + (a_low * b_high);
#else
                const std::uint64_t a_lo32 = a_low & 0xFFFFFFFFULL;
                const std::uint64_t a_hi32 = a_low >> 32;
                const std::uint64_t b_lo32 = b_low & 0xFFFFFFFFULL;
                const std::uint64_t b_hi32 = b_low >> 32;
                const std::uint64_t t0 = a_lo32 * b_lo32;
                const std::uint64_t t1 = a_lo32 * b_hi32 + (t0 >> 32);
                const std::uint64_t t2 = a_hi32 * b_lo32 + (t1 & 0xFFFFFFFFULL);
                data[0] = (t2 << 32) | (t0 & 0xFFFFFFFFULL);
                data[1] = a_hi32 * b_hi32 + (t1 >> 32) + (t2 >> 32) + (a_high * b_low) + (a_low * b_high);
#endif
            }

            return *this;
        }

        /**
         * @brief Multiplication operator
         *
         * **Excess-K (EK):** DELETED — see operator*= for rationale.
         */
        constexpr int128_param_t operator*(const int128_param_t &) const noexcept
            requires(is_excess_k)
        = delete;

        constexpr int128_param_t operator*(const int128_param_t &other) const noexcept
            requires(!is_excess_k)
        {
            int128_param_t result = *this;
            result *= other;
            return result;
        }

        /**
         * @brief Division assignment operator
         *
         * **Excess-K (EK):** DELETED — division has no meaningful semantics for biased exponents.
         *
         * @param other Divisor (must be non-zero)
         * @return Reference to this (modified)
         */
        constexpr int128_param_t &operator/=(const int128_param_t &) noexcept
            requires(is_excess_k)
        = delete;

        constexpr int128_param_t &operator/=(const int128_param_t &other) noexcept
            requires(!is_excess_k)
        {
            auto qr = this->divmod(other);
            *this = qr.first;
            return *this;
        }

        /**
         * @brief Division operator
         *
         * **Excess-K (EK):** DELETED — see operator/= for rationale.
         */
        constexpr int128_param_t operator/(const int128_param_t &) const noexcept
            requires(is_excess_k)
        = delete;

        constexpr int128_param_t operator/(const int128_param_t &other) const noexcept
            requires(!is_excess_k)
        {
            int128_param_t result = *this;
            result /= other;
            return result;
        }

        /**
         * @brief Modulo assignment operator
         *
         * **Excess-K (EK):** DELETED — modulo has no meaningful semantics for biased exponents.
         */
        constexpr int128_param_t &operator%=(const int128_param_t &) noexcept
            requires(is_excess_k)
        = delete;

        constexpr int128_param_t &operator%=(const int128_param_t &other) noexcept
            requires(!is_excess_k)
        {
            auto qr = this->divmod(other);
            *this = qr.second;
            return *this;
        }

        /**
         * @brief Modulo operator
         *
         * **Excess-K (EK):** DELETED — see operator%= for rationale.
         */
        constexpr int128_param_t operator%(const int128_param_t &) const noexcept
            requires(is_excess_k)
        = delete;

        constexpr int128_param_t operator%(const int128_param_t &other) const noexcept
            requires(!is_excess_k)
        {
            int128_param_t result = *this;
            result %= other;
            return result;
        }

        // ========================================================================
        // Compound Assignment Operators for Built-in Integral Types
        // ========================================================================

        /// @brief Addition assignment from built-in integral type
        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr int128_param_t &operator+=(T rhs) noexcept
        {
            return *this += int128_param_t{rhs};
        }

        /// @brief Subtraction assignment from built-in integral type
        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr int128_param_t &operator-=(T rhs) noexcept
        {
            return *this -= int128_param_t{rhs};
        }

        /// @brief Multiplication assignment from built-in integral type
        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr int128_param_t &operator*=(T rhs) noexcept
            requires(!is_excess_k)
        {
            return *this *= int128_param_t{rhs};
        }

        /// @brief Division assignment from built-in integral type
        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr int128_param_t &operator/=(T rhs) noexcept
            requires(!is_excess_k)
        {
            return *this /= int128_param_t{rhs};
        }

        /// @brief Modulo assignment from built-in integral type
        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr int128_param_t &operator%=(T rhs) noexcept
            requires(!is_excess_k)
        {
            return *this %= int128_param_t{rhs};
        }

        // ========================================================================
        // Bitwise Operators (AND, OR, XOR, NOT)
        // ========================================================================

        /**
         * @brief Bitwise AND operator (representation-aware)
         *
         * **Two's Complement:** Standard bitwise AND
         * **Magnitude-Sign:** Apply AND to magnitude bits only, preserve signs
         */
        constexpr int128_param_t operator&(const int128_param_t &other) const noexcept
        {
            int128_param_t result;

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: AND magnitudes, preserve signs separately
                std::uint64_t this_mag_low = data[0];
                std::uint64_t this_mag_high = data[1] & ~(1ULL << 63);
                std::uint64_t other_mag_low = other.data[0];
                std::uint64_t other_mag_high = other.data[1] & ~(1ULL << 63);

                result.data[0] = this_mag_low & other_mag_low;
                result.data[1] = (this_mag_high & other_mag_high) | (data[1] & (1ULL << 63));
            }
            else
            {
                // TC and unsigned: standard bitwise AND
                result.data[0] = data[0] & other.data[0];
                result.data[1] = data[1] & other.data[1];
            }

            return result;
        }

        /**
         * @brief Bitwise AND assignment operator
         */
        constexpr int128_param_t &operator&=(const int128_param_t &other) noexcept
        {
            *this = *this & other;
            return *this;
        }

        /**
         * @brief Bitwise OR operator (representation-aware)
         *
         * **Two's Complement:** Standard bitwise OR
         * **Magnitude-Sign:** Apply OR to magnitude bits only, preserve signs
         */
        constexpr int128_param_t operator|(const int128_param_t &other) const noexcept
        {
            int128_param_t result{};

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: OR magnitudes, preserve signs separately
                const std::uint64_t this_mag_low{data[0]};
                const std::uint64_t this_mag_high{data[1] & ~(1ULL << 63)};
                const std::uint64_t other_mag_low{other.data[0]};
                const std::uint64_t other_mag_high{other.data[1] & ~(1ULL << 63)};

                result.data[0] = this_mag_low | other_mag_low;
                result.data[1] = (this_mag_high | other_mag_high) | (data[1] & (1ULL << 63));
            }
            else
            {
                // TC and unsigned: standard bitwise OR
                result.data[0] = data[0] | other.data[0];
                result.data[1] = data[1] | other.data[1];
            }

            return result;
        }

        /**
         * @brief Bitwise OR assignment operator
         */
        constexpr int128_param_t &operator|=(const int128_param_t &other) noexcept
        {
            *this = *this | other;
            return *this;
        }

        /**
         * @brief Bitwise XOR operator (representation-aware)
         *
         * **Two's Complement:** Standard bitwise XOR
         * **Magnitude-Sign:** Apply XOR to magnitude bits only, preserve signs
         */
        constexpr int128_param_t operator^(const int128_param_t &other) const noexcept
        {
            int128_param_t result{};

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: XOR magnitudes, preserve signs separately
                const std::uint64_t this_mag_low{data[0]};
                const std::uint64_t this_mag_high{data[1] & ~(1ULL << 63)};
                const std::uint64_t other_mag_low{other.data[0]};
                const std::uint64_t other_mag_high{other.data[1] & ~(1ULL << 63)};

                result.data[0] = this_mag_low ^ other_mag_low;
                result.data[1] = (this_mag_high ^ other_mag_high) | (data[1] & (1ULL << 63));
            }
            else
            {
                // TC and unsigned: standard bitwise XOR
                result.data[0] = data[0] ^ other.data[0];
                result.data[1] = data[1] ^ other.data[1];
            }

            return result;
        }

        /**
         * @brief Bitwise XOR assignment operator
         */
        constexpr int128_param_t &operator^=(const int128_param_t &other) noexcept
        {
            *this = *this ^ other;
            return *this;
        }

        /**
         * @brief Bitwise NOT operator (representation-aware)
         *
         * **Two's Complement:** Standard bitwise complement (flip all bits)
         * **Magnitude-Sign:** Invert magnitude bits only, preserve sign bit
         */
        constexpr int128_param_t operator~() const noexcept
        {
            int128_param_t result{};

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: Invert magnitude bits, preserve sign bit
                const std::uint64_t mag_low{data[0]};
                const std::uint64_t mag_high{data[1] & ~(1ULL << 63)};

                result.data[0] = ~mag_low;
                result.data[1] = (~mag_high) | (data[1] & (1ULL << 63));
            }
            else
            {
                // TC and unsigned: standard bitwise NOT
                result.data[0] = ~data[0];
                result.data[1] = ~data[1];
            }

            return result;
        }

        // ========================================================================
        // Shift Operators
        // ========================================================================

        /// @brief Left shift assignment operator
        constexpr int128_param_t &operator<<=(int shift) noexcept
        {
            if (shift <= 0)
            {
                return *this;
            }
            if (shift >= 128)
            {
                data[0] = 0;
                data[1] = 0;
                return *this;
            }

            if constexpr (is_magnitude_sign && is_signed)
            {
                // MS: Extract sign, shift magnitude como unsigned, restaurar signo
                const std::uint64_t sign_bit{data[1] & (1ULL << 63)};
                const std::uint64_t mag_high{data[1] & ~(1ULL << 63)};

                if (shift >= 64)
                {
                    // Shift magnitude (as if unsigned)
                    const std::uint64_t new_high{data[0] << (shift - 64)};
                    data[0] = 0;
                    // Restore sign
                    data[1] = new_high | sign_bit;
                }
                else
                {
                    // Shift magnitude (as if unsigned)
                    const std::uint64_t new_high{(mag_high << shift) | (data[0] >> (64 - shift))};
                    const std::uint64_t new_low{data[0] << shift};
                    data[0] = new_low;
                    // Restore sign
                    data[1] = new_high | sign_bit;
                }
            }
            else
            {
                // TC y unsigned: desplazamiento estándar
                if (shift >= 64)
                {
                    const std::uint64_t new_high{data[0] << (shift - 64)};
                    data[0] = 0;
                    data[1] = new_high;
                }
                else
                {
                    const std::uint64_t new_high{(data[1] << shift) | (data[0] >> (64 - shift))};
                    const std::uint64_t new_low{data[0] << shift};
                    data[0] = new_low;
                    data[1] = new_high;
                }
            }

            return *this;
        }

        /// @brief Left shift operator
        constexpr int128_param_t operator<<(int shift) const noexcept
        {
            int128_param_t result(*this);
            result <<= shift;
            return result;
        }

        /// @brief Left shift assignment with integral type
        template <typename T>
        constexpr int128_param_t &operator<<=(T shift) noexcept
        {
            return *this <<= static_cast<int>(shift);
        }

        /// @brief Left shift with integral type
        template <typename T>
        constexpr int128_param_t operator<<(T shift) const noexcept
        {
            return *this << static_cast<int>(shift);
        }

        /// @brief Right shift assignment operator (arithmetic for signed TC, logical for unsigned and MS)
        constexpr int128_param_t &operator>>=(int shift) noexcept
        {
            if (shift <= 0)
            {
                return *this;
            }
            if (shift >= 128)
            {
                if constexpr (is_magnitude_sign && is_signed)
                {
                    // MS: Shift magnitude logically, preserve sign
                    const std::uint64_t sign_bit{data[1] & (1ULL << 63)};
                    data[0] = 0;
                    data[1] = sign_bit;
                }
                else if constexpr (is_signed)
                {
                    // TC signed: desplazamiento aritmético - propaga signo
                    const bool is_negative{static_cast<std::int64_t>(data[1]) < 0};
                    data[0] = is_negative ? ~0ull : 0ull;
                    data[1] = is_negative ? ~0ull : 0ull;
                }
                else
                {
                    // Unsigned: desplazamiento lógico - rellena con 0s
                    data[0] = 0;
                    data[1] = 0;
                }
                return *this;
            }

            if (shift >= 64)
            {
                if constexpr (is_magnitude_sign && is_signed)
                {
                    // MS: Extrae signo, desplaza magnitud como unsigned, restaura signo
                    const std::uint64_t sign_bit{data[1] & (1ULL << 63)};
                    const std::uint64_t mag_high{data[1] & ~(1ULL << 63)};
                    // Shift magnitude (as if unsigned)
                    const std::uint64_t new_low{mag_high >> (shift - 64)};
                    data[0] = new_low;
                    // Restore sign
                    data[1] = sign_bit;
                }
                else if constexpr (is_signed)
                {
                    // TC signed: desplazamiento aritmético
                    const std::int64_t sign_extended{static_cast<std::int64_t>(data[1]) >> (shift - 64)};
                    const std::int64_t all_sign{static_cast<std::int64_t>(data[1]) >> 63};
                    data[0] = static_cast<std::uint64_t>(sign_extended);
                    data[1] = static_cast<std::uint64_t>(all_sign);
                }
                else
                {
                    // Unsigned: desplazamiento lógico
                    const std::uint64_t new_low{data[1] >> (shift - 64)};
                    data[0] = new_low;
                    data[1] = 0;
                }
            }
            else
            {
                const std::uint64_t new_low{(data[0] >> shift) | (data[1] << (64 - shift))};
                if constexpr (is_magnitude_sign && is_signed)
                {
                    // MS: Extrae signo, desplaza magnitud como unsigned, restaura signo
                    const std::uint64_t sign_bit{data[1] & (1ULL << 63)};
                    const std::uint64_t mag_high{data[1] & ~(1ULL << 63)};
                    // Shift magnitude (as if unsigned)
                    const std::uint64_t new_high{mag_high >> shift};
                    data[0] = new_low;
                    // Restore sign
                    data[1] = new_high | sign_bit;
                }
                else if constexpr (is_signed)
                {
                    // TC signed: desplazamiento aritmético
                    const std::int64_t sign_extended{static_cast<std::int64_t>(data[1]) >> shift};
                    data[0] = new_low;
                    data[1] = static_cast<std::uint64_t>(sign_extended);
                }
                else
                {
                    // Unsigned: desplazamiento lógico
                    const std::uint64_t new_high{data[1] >> shift};
                    data[0] = new_low;
                    data[1] = new_high;
                }
            }

            return *this;
        }

        /// @brief Right shift operator (non-modifying)
        constexpr int128_param_t operator>>(int shift) const noexcept
        {
            int128_param_t result(*this);
            result >>= shift;
            return result;
        }

        /// @brief Right shift with integral type
        template <typename T>
        constexpr int128_param_t operator>>(T shift) const noexcept
        {
            return *this >> static_cast<int>(shift);
        }

        // =========================================================================
        // Bit Manipulation Functions (Priority 8)
        // =========================================================================

        /**
         * @brief Count trailing zero bits (from right/LSB)
         *
         * @return Number of consecutive zero bits starting from LSB
         *
         * @details
         * - Returns 128 (or 127 for MS magnitude) if all bits are zero
         * - For MS signed: operates on magnitude only (ignores sign bit)
         * - Uses __builtin_ctzll for hardware optimization
         */
        constexpr int trailing_zeros() const noexcept
        {
#if __has_include("intrinsics/bit_operations.hpp")
            if (!std::is_constant_evaluated())
            {
                // Optimized path: Use TZCNT hardware instruction
                if constexpr (is_magnitude_sign && is_signed)
                {
                    const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                    if (data[0] != 0)
                    {
                        return intrinsics::ctz64(data[0]);
                    }
                    if (mag_high != 0)
                    {
                        return 64 + intrinsics::ctz64(mag_high);
                    }
                    return 127; // MS magnitude is 127 bits
                }
                else
                {
                    if (data[0] != 0)
                    {
                        return intrinsics::ctz64(data[0]);
                    }
                    if (data[1] != 0)
                    {
                        return 64 + intrinsics::ctz64(data[1]);
                    }
                    return 128;
                }
            }
#endif
            // Portable fallback for constexpr contexts
            if constexpr (is_magnitude_sign && is_signed)
            {
                const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                if (data[0] != 0)
                {
#if defined(__has_include) && __has_include("intrinsics/bit_operations.hpp")
                    return intrinsics::ctz64(data[0]);
#else
                    int count = 0;
                    uint64_t val = data[0];
                    while ((val & 1) == 0)
                    {
                        val >>= 1;
                        count++;
                    }
                    return count;
#endif
                }
                if (mag_high != 0)
                {
#if defined(__has_include) && __has_include("intrinsics/bit_operations.hpp")
                    return 64 + intrinsics::ctz64(mag_high);
#else
                    int count = 0;
                    uint64_t val = mag_high;
                    while ((val & 1) == 0)
                    {
                        val >>= 1;
                        count++;
                    }
                    return 64 + count;
#endif
                }
                return 127; // MS magnitude is 127 bits
            }
            else
            {
                if (data[0] != 0)
                {
#if defined(__has_include) && __has_include("intrinsics/bit_operations.hpp")
                    return intrinsics::ctz64(data[0]);
#else
                    int count = 0;
                    uint64_t val = data[0];
                    while ((val & 1) == 0)
                    {
                        val >>= 1;
                        count++;
                    }
                    return count;
#endif
                }
                if (data[1] != 0)
                {
#if defined(__has_include) && __has_include("intrinsics/bit_operations.hpp")
                    return 64 + intrinsics::ctz64(data[1]);
#else
                    int count = 0;
                    uint64_t val = data[1];
                    while ((val & 1) == 0)
                    {
                        val >>= 1;
                        count++;
                    }
                    return 64 + count;
#endif
                }
                return 128;
            }
        }

        /**
         * @brief Count leading zero bits (from left/MSB)
         *
         * @return Number of consecutive zero bits starting from MSB
         *
         * @details
         * - For MS signed: operates on 127-bit magnitude
         * - Uses __builtin_clzll for hardware optimization
         */
        constexpr int leading_zeros() const noexcept
        {
#if __has_include("intrinsics/bit_operations.hpp")
            if (!std::is_constant_evaluated())
            {
                // Optimized path: Use LZCNT hardware instruction
                if constexpr (is_magnitude_sign && is_signed)
                {
                    const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                    if (mag_high != 0)
                    {
                        return intrinsics::clz64(mag_high) - 1;
                    }
                    if (data[0] != 0)
                    {
                        return 63 + intrinsics::clz64(data[0]);
                    }
                    return 127; // MS magnitude is 127 bits
                }
                else
                {
                    if (data[1] != 0)
                    {
                        return intrinsics::clz64(data[1]);
                    }
                    if (data[0] != 0)
                    {
                        return 64 + intrinsics::clz64(data[0]);
                    }
                    return 128;
                }
            }
#endif
            // Portable fallback for constexpr contexts
            if constexpr (is_magnitude_sign && is_signed)
            {
                const uint64_t mag_high{data[1] & ~(1ULL << 63)};
                if (mag_high != 0)
                {
#if defined(__has_include) && __has_include("intrinsics/bit_operations.hpp")
                    return intrinsics::clz64(mag_high);
#else
                    uint64_t val = mag_high;
                    int count = 0;
                    if ((val & 0xFFFFFFFF00000000ULL) == 0)
                    {
                        count += 32;
                        val <<= 32;
                    }
                    if ((val & 0xFFFF000000000000ULL) == 0)
                    {
                        count += 16;
                        val <<= 16;
                    }
                    if ((val & 0xFF00000000000000ULL) == 0)
                    {
                        count += 8;
                        val <<= 8;
                    }
                    if ((val & 0xF000000000000000ULL) == 0)
                    {
                        count += 4;
                        val <<= 4;
                    }
                    if ((val & 0xC000000000000000ULL) == 0)
                    {
                        count += 2;
                        val <<= 2;
                    }
                    if ((val & 0x8000000000000000ULL) == 0)
                    {
                        count += 1;
                    }
                    return count;
#endif
                }
                if (data[0] != 0)
                {
#if defined(__has_include) && __has_include("intrinsics/bit_operations.hpp")
                    return 64 + intrinsics::clz64(data[0]);
#else
                    uint64_t val = data[0];
                    int count = 0;
                    if ((val & 0xFFFFFFFF00000000ULL) == 0)
                    {
                        count += 32;
                        val <<= 32;
                    }
                    if ((val & 0xFFFF000000000000ULL) == 0)
                    {
                        count += 16;
                        val <<= 16;
                    }
                    if ((val & 0xFF00000000000000ULL) == 0)
                    {
                        count += 8;
                        val <<= 8;
                    }
                    if ((val & 0xF000000000000000ULL) == 0)
                    {
                        count += 4;
                        val <<= 4;
                    }
                    if ((val & 0xC000000000000000ULL) == 0)
                    {
                        count += 2;
                        val <<= 2;
                    }
                    if ((val & 0x8000000000000000ULL) == 0)
                    {
                        count += 1;
                    }
                    return 64 + count;
#endif
                }
                return 127; // MS magnitude is 127 bits
            }
            else
            {
                if (data[1] != 0)
                {
#if defined(__has_include) && __has_include("intrinsics/bit_operations.hpp")
                    return intrinsics::clz64(data[1]);
#else
                    uint64_t val = data[1];
                    int count = 0;
                    if ((val & 0xFFFFFFFF00000000ULL) == 0)
                    {
                        count += 32;
                        val <<= 32;
                    }
                    if ((val & 0xFFFF000000000000ULL) == 0)
                    {
                        count += 16;
                        val <<= 16;
                    }
                    if ((val & 0xFF00000000000000ULL) == 0)
                    {
                        count += 8;
                        val <<= 8;
                    }
                    if ((val & 0xF000000000000000ULL) == 0)
                    {
                        count += 4;
                        val <<= 4;
                    }
                    if ((val & 0xC000000000000000ULL) == 0)
                    {
                        count += 2;
                        val <<= 2;
                    }
                    if ((val & 0x8000000000000000ULL) == 0)
                    {
                        count += 1;
                    }
                    return count;
#endif
                }
                if (data[0] != 0)
                {
#if defined(__has_include) && __has_include("intrinsics/bit_operations.hpp")
                    return 64 + intrinsics::clz64(data[0]);
#else
                    uint64_t val = data[0];
                    int count = 0;
                    if ((val & 0xFFFFFFFF00000000ULL) == 0)
                    {
                        count += 32;
                        val <<= 32;
                    }
                    if ((val & 0xFFFF000000000000ULL) == 0)
                    {
                        count += 16;
                        val <<= 16;
                    }
                    if ((val & 0xFF00000000000000ULL) == 0)
                    {
                        count += 8;
                        val <<= 8;
                    }
                    if ((val & 0xF000000000000000ULL) == 0)
                    {
                        count += 4;
                        val <<= 4;
                    }
                    if ((val & 0xC000000000000000ULL) == 0)
                    {
                        count += 2;
                        val <<= 2;
                    }
                    if ((val & 0x8000000000000000ULL) == 0)
                    {
                        count += 1;
                    }
                    return 64 + count;
#endif
                }
                return 128;
            }
        }

        /**
         * @brief Count set bits (population count)
         *
         * @return The number of bits set to 1
         *
         * @details
         * - For MS signed: operates on magnitude only
         */
        constexpr int count_ones() const noexcept
        {
#if __has_include("intrinsics/bit_operations.hpp")
            if (!std::is_constant_evaluated())
            {
                // Optimized path: Use POPCNT hardware instruction
                if constexpr (is_magnitude_sign && is_signed)
                {
                    const uint64_t mag_high = data[1] & ~(1ULL << 63);
                    return intrinsics::popcount64(mag_high) + intrinsics::popcount64(data[0]);
                }
                else
                {
                    return intrinsics::popcount64(data[1]) + intrinsics::popcount64(data[0]);
                }
            }
#endif
            // Portable fallback for constexpr contexts
            if constexpr (is_magnitude_sign && is_signed)
            {
                const uint64_t mag_high = data[1] & ~(1ULL << 63);
#if defined(__has_include) && __has_include("intrinsics/bit_operations.hpp")
                return intrinsics::popcount64(mag_high) + intrinsics::popcount64(data[0]);
#else
                // Brian Kernighan's algorithm fallback
                auto count_bits = [](uint64_t v) -> int
                {
                    int count = 0;
                    while (v)
                    {
                        v &= v - 1;
                        count++;
                    }
                    return count;
                };
                return count_bits(mag_high) + count_bits(data[0]);
#endif
            }
            else
            {
#if defined(__has_include) && __has_include("intrinsics/bit_operations.hpp")
                return intrinsics::popcount64(data[1]) + intrinsics::popcount64(data[0]);
#else
                auto count_bits = [](uint64_t v) -> int
                {
                    int count = 0;
                    while (v)
                    {
                        v &= v - 1;
                        count++;
                    }
                    return count;
                };
                return count_bits(data[1]) + count_bits(data[0]);
#endif
            }
        }

        /// @brief Alias for count_ones()
        constexpr int popcount() const noexcept { return count_ones(); }

        /**
         * @brief Check if the number is a power of 2
         *
         * @return true if the number is a power of 2, false otherwise
         *
         * @details For MS signed, operates on magnitude. Negative numbers are not powers of 2.
         */
        constexpr bool is_power_of_2() const noexcept
        {
            if constexpr (is_signed)
            {
                if (is_negative())
                    return false;
            }
            if (is_zero())
                return false;
            return count_ones() == 1;
        }

        /**
         * @brief Returns the minimum number of bits required to represent the value.
         *
         * @return The bit width of the value.
         */
        constexpr int bit_width() const noexcept
        {
            if (is_zero())
                return 0;

            if constexpr (is_magnitude_sign && is_signed)
            {
                return 127 - leading_zeros();
            }
            else
            {
                return 128 - leading_zeros();
            }
        }

        /**
         * @brief Performs a circular left shift (rotate)
         *
         * @param shift The number of bits to rotate left
         * @return The rotated value
         */
        constexpr int128_param_t rotate_left(int shift) const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                const int s = shift & 127;
                if (s == 0)
                    return *this;

                uint64_t sign_bit = data[1] & (1ULL << 63);
                int128_param_t temp(data[1] & ~(1ULL << 63), data[0]);

                int128_param_t shifted = temp << s;
                int128_param_t rotated = temp >> (127 - s);

                int128_param_t result = shifted | rotated;
                result.data[1] &= ~(1ULL << 63);
                result.data[1] |= sign_bit;
                return result;
            }
            else
            {
                const int s = shift & 127;
                if (s == 0)
                    return *this;
                return (*this << s) | (*this >> (128 - s));
            }
        }

        /**
         * @brief Performs a circular right shift (rotate)
         *
         * @param shift The number of bits to rotate right
         * @return The rotated value
         */
        constexpr int128_param_t rotate_right(int shift) const noexcept
        {
            if constexpr (is_magnitude_sign && is_signed)
            {
                const int s = shift & 127;
                if (s == 0)
                    return *this;
                return rotate_left(127 - s);
            }
            else
            {
                const int s = shift & 127;
                if (s == 0)
                    return *this;
                return (*this >> s) | (*this << (128 - s));
            }
        }

        /**
         * @brief Reverses the order of bits.
         *
         * @return Value with all bits reversed.
         *
         * @details For MS signed, reverses 127 magnitude bits, preserves sign.
         */
        constexpr int128_param_t reverse_bits() const noexcept
        {
            auto reverse64 = [](uint64_t n)
            {
                n = (n >> 32) | (n << 32);
                n = ((n & 0xFFFF0000FFFF0000ULL) >> 16) | ((n & 0x0000FFFF0000FFFFULL) << 16);
                n = ((n & 0xFF00FF00FF00FF00ULL) >> 8) | ((n & 0x00FF00FF00FF00FFULL) << 8);
                n = ((n & 0xF0F0F0F0F0F0F0F0ULL) >> 4) | ((n & 0x0F0F0F0F0F0F0F0FULL) << 4);
                n = ((n & 0xCCCCCCCCCCCCCCCCULL) >> 2) | ((n & 0x3333333333333333ULL) << 2);
                n = ((n & 0xAAAAAAAAAAAAAAAAULL) >> 1) | ((n & 0x5555555555555555ULL) << 1);
                return n;
            };

            if constexpr (is_magnitude_sign && is_signed)
            {
                uint64_t sign_bit = data[1] & (1ULL << 63);
                uint64_t mag_high = data[1] & ~(1ULL << 63);

                uint64_t reversed_low_part = reverse64(mag_high);
                uint64_t reversed_high_part = reverse64(data[0]);

                int128_param_t temp(reversed_high_part, reversed_low_part);
                temp >>= 1;

                temp.data[1] |= sign_bit;
                return temp;
            }
            else
            {
                return int128_param_t(reverse64(data[0]), reverse64(data[1]));
            }
        }

        /**
         * @brief Division with remainder (divmod operation)
         *
         * @param divisor The divisor
         * @return Pair of (quotient, remainder)
         *
         * @details
         * Efficient combined division and modulo operation.
         * Representation-aware for MS (operates on magnitude).
         *
         * @example
         * auto [quot, rem] = uint128_tc_t{100}.divmod(uint128_tc_t{7});
         * // quot = 14, rem = 2
         */
        [[nodiscard]] constexpr std::pair<int128_param_t, int128_param_t>
        divmod(const int128_param_t &) const noexcept
            requires(is_excess_k)
        = delete;

        [[nodiscard]] constexpr std::pair<int128_param_t, int128_param_t>
        divmod(const int128_param_t &other) const noexcept
            requires(!is_excess_k)
        {
            if constexpr (!is_signed)
            {
                // Unsigned: use Knuth D (optimized) when available
                return D_knuth_divrem(other);
            }
            else if constexpr (is_magnitude_sign)
            {
                // MS: Work with magnitudes, apply sign rules
                const bool neg_dividend{is_negative()};
                const bool neg_divisor{other.is_negative()};

                // Extract magnitudes
                const auto dividend_mag{magnitude()};
                const auto divisor_mag{other.magnitude()};

                if (divisor_mag.is_zero())
                {
                    return {int128_param_t{0}, *this};
                }

                // Divide magnitudes (unsigned division) - use Knuth D
                auto [quotient, remainder] = dividend_mag.D_knuth_divrem(divisor_mag);

                // Apply sign rules
                if (neg_dividend != neg_divisor && !quotient.is_zero())
                {
                    quotient.data[1] |= (1ULL << 63); // Set sign bit
                }
                if (neg_dividend && !remainder.is_zero())
                {
                    remainder.data[1] |= (1ULL << 63); // Set sign bit
                }

                return {quotient, remainder};
            }
            else
            {
                // TC and EK: handle negatives with two's complement
                const bool neg_dividend{is_negative()};
                const bool neg_divisor{other.is_negative()};

                // Compute absolute values (requires signed types)
                int128_param_t dividend_abs{*this};
                int128_param_t divisor_abs{other};

                if (neg_dividend)
                {
                    dividend_abs = -dividend_abs;
                }
                if (neg_divisor)
                {
                    divisor_abs = -divisor_abs;
                }

                if (divisor_abs.is_zero())
                {
                    return {int128_param_t{0}, *this};
                }

                // Use Knuth D on absolute values
                auto [quotient, remainder] = dividend_abs.D_knuth_divrem(divisor_abs);

                // Apply signs to results
                if (neg_dividend != neg_divisor && !quotient.is_zero())
                {
                    quotient = -quotient;
                }
                if (neg_dividend && !remainder.is_zero())
                {
                    remainder = -remainder;
                }

                return {quotient, remainder};
            }
        }

        // ========================================================================
        // Division/Modulo by Compile-Time Constant (Granlund-Montgomery)
        //
        // Uses constexpr magic-number computation from Hacker's Delight 10-9.
        // For D <= 1023, constants come from the precomputed GM_TABLE.
        // For D > 1023, constants are computed on-the-fly at compile time.
        // ========================================================================

        /**
         * @brief Divide by compile-time constant using Granlund-Montgomery multiply-shift.
         *
         * @tparam D Divisor (must be > 0)
         * @return Quotient floor(|this| / D), with sign applied for signed types
         *
         * @note For unsigned types: pure multiply-shift, no branching.
         * @note For signed types: abs → unsigned div → re-sign (truncation toward zero).
         * @note Excess-K representation: DELETED (no meaningful division semantics).
         */
        template <std::uint64_t D>
            requires(D > 0)
        [[nodiscard]] constexpr int128_param_t div() const noexcept
            requires(is_excess_k)
        = delete;

        template <std::uint64_t D>
            requires(D > 0)
        [[nodiscard]] constexpr int128_param_t div() const noexcept
            requires(!is_excess_k)
        {
            if constexpr (!is_signed)
            {
                // Unsigned: direct GM division
                if constexpr (divmod_detail::is_pow2(D))
                {
                    return *this >> divmod_detail::ctz64(D);
                }
                else
                {
                    constexpr auto entry{divmod_detail::compute_magic_128(D)};
                    const auto [q_hi, q_lo]{divmod_detail::gm_div_limbs(
                        data[1], data[0], entry)};
                    return int128_param_t{q_hi, q_lo};
                }
            }
            else
            {
                // Signed: abs → unsigned div → re-sign
                const bool neg{is_negative()};
                const int128_param_t abs_val{neg ? -(*this) : *this};

                if constexpr (divmod_detail::is_pow2(D))
                {
                    const int128_param_t q{abs_val >> divmod_detail::ctz64(D)};
                    return neg ? -q : q;
                }
                else
                {
                    constexpr auto entry{divmod_detail::compute_magic_128(D)};
                    const auto [q_hi, q_lo]{divmod_detail::gm_div_limbs(
                        abs_val.data[1], abs_val.data[0], entry)};
                    const int128_param_t q{q_hi, q_lo};
                    return neg ? -q : q;
                }
            }
        }

        /**
         * @brief Modulo by compile-time constant using Granlund-Montgomery.
         *
         * @tparam D Divisor (must be > 0)
         * @return Remainder this - floor(this/D)*D, with sign matching dividend.
         */
        template <std::uint64_t D>
            requires(D > 0)
        [[nodiscard]] constexpr int128_param_t mod() const noexcept
            requires(is_excess_k)
        = delete;

        template <std::uint64_t D>
            requires(D > 0)
        [[nodiscard]] constexpr int128_param_t mod() const noexcept
            requires(!is_excess_k)
        {
            const int128_param_t q{this->template div<D>()};
            return *this - q * int128_param_t{D};
        }

        /**
         * @brief Combined division and modulo by compile-time constant.
         *
         * @tparam D Divisor (must be > 0)
         * @return Pair {quotient, remainder}
         */
        template <std::uint64_t D>
            requires(D > 0)
        [[nodiscard]] constexpr std::pair<int128_param_t, int128_param_t>
        divmod_const() const noexcept
            requires(is_excess_k)
        = delete;

        template <std::uint64_t D>
            requires(D > 0)
        [[nodiscard]] constexpr std::pair<int128_param_t, int128_param_t>
        divmod_const() const noexcept
            requires(!is_excess_k)
        {
            const int128_param_t q{this->template div<D>()};
            const int128_param_t r{*this - q * int128_param_t{D}};
            return {q, r};
        }

        // ========================================================================
        // Multiplication by Compile-Time Constant (shift-add chain)
        // ========================================================================

        /**
         * @brief Multiply by compile-time constant using binary shift-add decomposition.
         *
         * @tparam K Factor (uint64_t)
         * @return this * K
         *
         * @details Generates O(log K) shift+add operations. The compiler fully
         * evaluates the recursion at compile time, emitting only the minimal
         * sequence of shifts and adds for the specific K value.
         *
         * @example
         * const uint128_t n{42};
         * const uint128_t r = n.mul<10>();  // 420
         */
        template <std::uint64_t K>
        [[nodiscard]] constexpr int128_param_t mul() const noexcept
        {
            if constexpr (K == 0)
            {
                return int128_param_t{0};
            }
            else if constexpr (K == 1)
            {
                return *this;
            }
            else if constexpr (divmod_detail::is_pow2(K))
            {
                return *this << divmod_detail::ctz64(K);
            }
            else if constexpr (K % 2 == 0)
            {
                // Even K: mul<K/2>() << 1
                return this->template mul<K / 2>() << 1;
            }
            else
            {
                // Odd K: mul<(K-1)/2>() << 1 + *this
                return (this->template mul<(K - 1) / 2>() << 1) + *this;
            }
        }

        /**
         * @brief Optimized binary division for unsigned 128-bit integers
         *
         * @details Implements multiple optimization levels:
         * - [Fast paths] Zero divisor, zero dividend, divisor > dividend, divisor == dividend
         * - [Level 1] Power-of-2 divisors: shift right optimization O(1)
         * - [Level 2] Small specific divisors (3-15): native 64-bit division when possible
         * - [Level 3] 64-bit values: native CPU division
         * - [Level 4] 64-bit divisor / 128-bit dividend: hybrid algorithm
         * - [Level 5] Common trailing zeros: reduce both operands
         * - [Level 6] General case: long division bit-by-bit O(128)
         *
         * @param divisor The divisor (treated as unsigned)
         * @return Pair {quotient, remainder}
         *
         * @note Treats all values as UNSIGNED (no sign checks)
         * @warning If divisor == 0, returns {0, 0}
         */
        constexpr std::pair<int128_param_t, int128_param_t>
        big_bin_divrem(const int128_param_t &divisor) const noexcept
        {
            // [0.a] Fast path: divisor is 0 (undefined behavior, return 0)
            if (divisor.data[0] == 0 && divisor.data[1] == 0)
            {
                return {int128_param_t{0}, int128_param_t{0}};
            }

            // [0.b] Fast path: dividend is 0
            if (data[0] == 0 && data[1] == 0)
            {
                return {int128_param_t{0}, int128_param_t{0}};
            }

            // [0.c] Fast path: divisor > dividend (unsigned comparison)
            const bool divisor_greater =
                (divisor.data[1] > data[1]) ||
                (divisor.data[1] == data[1] && divisor.data[0] > data[0]);
            if (divisor_greater)
            {
                return {int128_param_t{0}, *this};
            }

            // [0.d] Fast path: divisor == dividend
            if (data[0] == divisor.data[0] && data[1] == divisor.data[1])
            {
                return {int128_param_t{1}, int128_param_t{0}};
            }

            // [0.e] Fast path: divisor == 1
            if (divisor.data[0] == 1ull && divisor.data[1] == 0ull)
            {
                return {*this, int128_param_t{0}};
            }

            // ========================================================================
            // [1] OPTIMIZATIONS FOR SMALL SPECIFIC DIVISORS (up to 15)
            // ========================================================================

            // Only apply if divisor fits in 64 bits
            if (divisor.data[1] == 0)
            {
                const uint64_t d = divisor.data[0];

                // [1.1] Powers of 2: optimized right shift
                if (d != 0 && (d & (d - 1)) == 0)
                {
                    // Count trailing zeros to get the exponent
                    int shift = 0;
                    uint64_t temp = d;
                    while ((temp & 1) == 0)
                    {
                        temp >>= 1;
                        ++shift;
                    }

                    // Quotient = *this >> shift
                    // Remainder = *this & (d - 1)
                    const uint64_t mask = d - 1;
                    const uint64_t remainder = data[0] & mask;

                    int128_param_t quotient;
                    if (shift >= 64)
                    {
                        // Shift >= 64: move high to low
                        quotient.data[0] = data[1] >> (shift - 64);
                        quotient.data[1] = 0;
                    }
                    else if (shift > 0)
                    {
                        // Normal shift
                        quotient.data[0] = (data[0] >> shift) | (data[1] << (64 - shift));
                        quotient.data[1] = data[1] >> shift;
                    }
                    else
                    {
                        quotient = *this;
                    }

                    int128_param_t rem_obj{0};
                    rem_obj.data[0] = remainder;
                    return {quotient, rem_obj};
                }

                // [1.2-1.12] Common specific divisors (not powers of 2)
                switch (d)
                {
                case 3:
                case 5:
                case 6:
                case 7:
                case 9:
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                case 15:
                    // For these cases, if dividend fits in 64 bits, use native division
                    if (data[1] == 0)
                    {
                        const uint64_t q = data[0] / d;
                        const uint64_t r = data[0] % d;
                        return {int128_param_t{0, q}, int128_param_t{0, r}};
                    }
                    break;

                default:
                    break;
                }
            }

            // ========================================================================
            // [2] BOTH VALUES FIT IN 64 BITS
            // ========================================================================

            if (data[1] == 0 && divisor.data[1] == 0)
            {
                const uint64_t q = data[0] / divisor.data[0];
                const uint64_t r = data[0] % divisor.data[0];
                return {int128_param_t{0, q}, int128_param_t{0, r}};
            }

            // ========================================================================
            // [3] OPTIMIZATION FOR 64-BIT DIVISOR (128-bit dividend)
            // ========================================================================

            // Safe only when divisor_64 <= 2^63: remainder << 1 stays within uint64_t.
            // Divisors > 2^63 (e.g. 10^19) fall through to the general 128/128 path below.
            if (divisor.data[1] == 0 && divisor.data[0] <= (uint64_t{1} << 63))
            {
                const uint64_t divisor_64 = divisor.data[0];

                // 128-bit dividend / 64-bit divisor
                // Hybrid algorithm: divide high first, then process low bit-by-bit
                uint64_t quotient_low = 0;
                uint64_t quotient_high = data[1] / divisor_64;
                uint64_t remainder = data[1] % divisor_64;

                // Divide low part bit-by-bit (process from MSB)
                for (int i = 63; i >= 0; --i)
                {
                    remainder = (remainder << 1) | ((data[0] >> i) & 1);
                    if (remainder >= divisor_64)
                    {
                        remainder -= divisor_64;
                        quotient_low |= (1ULL << i);
                    }
                }

                return {int128_param_t{quotient_high, quotient_low},
                        int128_param_t{0, remainder}};
            }

            // ========================================================================
            // [4] OPTIMIZATION: FACTORIZATION OF COMMON POWERS OF 2
            // ========================================================================
            // If n = n' * 2^k and m = m' * 2^h (where n', m' are odd or have fewer 2s)
            // Then: n / m = n' / m'  (quotient doesn't change if we divide both by 2^s)
            //       n % m = (n' % m') * 2^s  where s = min(k, h)
            //
            // Benefit: Reduces effective number of bits in division
            // Especially useful when both numbers have many trailing zeros

            const int tz_n = trailing_zeros();
            const int tz_m = divisor.trailing_zeros();

            // Only optimize if both have trailing zeros and at least one has many
            // Threshold: at least 4 bits of common trailing zeros
            const int common_tz = (tz_n < tz_m) ? tz_n : tz_m;

            if (common_tz >= 4)
            {
                // Divide both by 2^common_tz
                int128_param_t n_reduced = *this >> common_tz;
                int128_param_t m_reduced = divisor >> common_tz;

                // Do division with reduced values (recursion)
                auto [q_reduced, r_reduced] = n_reduced.big_bin_divrem(m_reduced);

                // Quotient is the same, remainder is multiplied by 2^common_tz
                return {q_reduced, r_reduced << common_tz};
            }

            // ========================================================================
            // [5] GENERAL CASE: LONG BINARY DIVISION (128 bits / 128 bits)
            // ========================================================================

            int128_param_t quotient{0};
            int128_param_t remainder{0};

            // Long binary division (school algorithm)
            // Process bits from MSB to LSB
            for (int i = 127; i >= 0; --i)
            {
                remainder <<= 1;
                const int word = i / 64;
                const int bit = i % 64;
                if ((data[word] & (1ULL << bit)) != 0)
                {
                    remainder.data[0] |= 1;
                }

                // If remainder >= divisor, subtract and add 1 to quotient
                if (remainder >= divisor)
                {
                    remainder -= divisor;
                    const int q_word = i / 64;
                    const int q_bit = i % 64;
                    quotient.data[q_word] |= (1ULL << q_bit);
                }
            }

            return {quotient, remainder};
        }

        /**
         * @brief Knuth's Algorithm D for division (128-bit / 128-bit)
         *
         * @details
         * True implementation of Knuth's Algorithm D (TAOCP Vol. 2, Section 4.3.1).
         *
         * Algorithm steps:
         * - D1: Normalize (shift divisor left so MSB = 1, shift dividend same amount)
         * - D2: Initialize quotient length (q_hi for upper 64 bits of quotient)
         * - D3-D7: Loop twice (for high and low 64 bits of quotient):
         *   - D3: Estimate quotient digit q_hat using 128/64 division
         *   - D4-D6: Multiply q_hat * divisor, subtract from dividend portion, refine
         *   - D7: Place quotient digit
         * - D8-D9: Denormalize remainder (shift right by normalization shift)
         *
         * @param divisor The divisor
         * @return Pair of (quotient, remainder)
         *
         * @note GCC/Clang with __uint128_t: Full optimization enabled
         *       MSVC/Intel without __uint128_t: Falls back to big_bin_divrem()
         */
        [[nodiscard]] constexpr std::pair<int128_param_t, int128_param_t>
        D_knuth_divrem(const int128_param_t &divisor) const noexcept
        {
            // ================================================================
            // Knuth Algorithm D - Optimized 128-bit unsigned division
            // Fast paths [0-2] are pure C++ and available on all compilers.
            // Path [3] uses _udiv128 on MSVC, __uint128_t on GCC/Clang.
            // 128/128 general case uses __uint128_t (GCC/Clang only).
            // ================================================================

            // [0.a] Fast path: divisor is 0
            if (divisor.data[0] == 0 && divisor.data[1] == 0)
            {
                return {int128_param_t{0}, int128_param_t{0}};
            }

            // [0.b] Fast path: dividend is 0
            if (data[0] == 0 && data[1] == 0)
            {
                return {int128_param_t{0}, int128_param_t{0}};
            }

            // [0.c] Fast path: divisor > dividend (unsigned comparison)
            const bool divisor_greater =
                (divisor.data[1] > data[1]) ||
                (divisor.data[1] == data[1] && divisor.data[0] > data[0]);
            if (divisor_greater)
            {
                return {int128_param_t{0}, *this};
            }

            // [0.d] Fast path: divisor == dividend
            if (data[0] == divisor.data[0] && data[1] == divisor.data[1])
            {
                return {int128_param_t{1}, int128_param_t{0}};
            }

            // [0.e] Fast path: divisor == 1
            if (divisor.data[0] == 1ull && divisor.data[1] == 0ull)
            {
                return {*this, int128_param_t{0}};
            }

            // ================================================================
            // 64-bit divisor optimizations (all compilers)
            // ================================================================
            if (divisor.data[1] == 0)
            {
                const uint64_t d = divisor.data[0];

                // [1] Power-of-2: shift right O(1) — portable via intrinsics::ctz64
                if ((d & (d - 1)) == 0)
                {
                    const int shift = intrinsics::ctz64(d);
                    const uint64_t mask = d - 1;
                    const uint64_t rem = data[0] & mask;

                    int128_param_t quotient{0};
                    if (shift >= 64)
                    {
                        quotient.data[0] = data[1] >> (shift - 64);
                        quotient.data[1] = 0;
                    }
                    else if (shift > 0)
                    {
                        quotient.data[0] = (data[0] >> shift) | (data[1] << (64 - shift));
                        quotient.data[1] = data[1] >> shift;
                    }
                    else
                    {
                        quotient = *this;
                    }

                    int128_param_t rem_obj{0};
                    rem_obj.data[0] = rem;
                    return {quotient, rem_obj};
                }

                // [2] Both fit in 64 bits: native CPU division — portable
                if (data[1] == 0)
                {
                    const uint64_t q = data[0] / d;
                    const uint64_t r = data[0] % d;
                    return {int128_param_t{0ull, q}, int128_param_t{0ull, r}};
                }

                // [3] 128-bit dividend / 64-bit divisor: two native divisions
                //     GCC/Clang: via __uint128_t; MSVC: via _udiv128 (runtime only)
#if defined(INTRINSICS_COMPILER_MSVC) && INTRINSICS_COMPILER_MSVC
                if (!INTRINSICS_IS_CONSTANT_EVALUATED())
                {
                    const uint64_t q_hi = data[1] / d;
                    const uint64_t r_hi = data[1] % d;
                    uint64_t r_final;
                    const uint64_t q_lo = intrinsics::div128_64_composed(r_hi, data[0], d, &r_final);
                    return {int128_param_t{q_hi, q_lo}, int128_param_t{0ull, r_final}};
                }
                // MSVC constexpr: fall through to big_bin_divrem below
#elif INTRINSICS_HAS_INT128
                {
                    const uint64_t q_hi = data[1] / d;
                    const uint64_t r_hi = data[1] % d;
                    uint64_t r_final;
                    const uint64_t q_lo = intrinsics::div128_64_composed(r_hi, data[0], d, &r_final);
                    return {int128_param_t{q_hi, q_lo}, int128_param_t{0ull, r_final}};
                }
                // No __uint128_t (e.g. arm32): fall through to big_bin_divrem below
#endif
            }

#if INTRINSICS_HAS_INT128
            // ================================================================
            // 128/128 general case: Knuth Algorithm D via __uint128_t
            // GCC/Clang/ICX implement __udivti3 using Knuth D internally
            // ================================================================
            {
                const __uint128_t u = (static_cast<__uint128_t>(data[1]) << 64) | data[0];
                const __uint128_t v = (static_cast<__uint128_t>(divisor.data[1]) << 64) | divisor.data[0];
                const __uint128_t q = u / v;
                const __uint128_t r = u - q * v;

                int128_param_t quotient{0};
                quotient.data[0] = static_cast<uint64_t>(q);
                quotient.data[1] = static_cast<uint64_t>(q >> 64);

                int128_param_t remainder{0};
                remainder.data[0] = static_cast<uint64_t>(r);
                remainder.data[1] = static_cast<uint64_t>(r >> 64);

                return {quotient, remainder};
            }
#endif
            // Fallback: MSVC (128/128 case or constexpr) → binary long division
            return big_bin_divrem(divisor);
        }

    public:
        /**
         * @brief Swap contents with another int128_param_t
         *
         * @param other The other value to swap with
         */
        constexpr void swap(int128_param_t &other) noexcept
        {
            const std::uint64_t temp_low{data[0]};
            const std::uint64_t temp_high{data[1]};
            data[0] = other.data[0];
            data[1] = other.data[1];
            other.data[0] = temp_low;
            other.data[1] = temp_high;
        }

        /**
         * @brief Get absolute value (magnitude)
         *
         * @return Absolute value of this number
         *
         * @details
         * - For unsigned types: returns self
         * - For TC signed: negates if negative
         * - For MS signed: returns magnitude directly
         */
        constexpr int128_param_t abs() const noexcept
        {
            if constexpr (!is_signed)
            {
                return *this;
            }
            else if constexpr (is_magnitude_sign)
            {
                int128_param_t result{*this};
                result.data[1] &= ~(std::uint64_t{1} << 63);
                return result;
            }
            else
            {
                return is_negative() ? -(*this) : (*this);
            }
        }

        /**
         * @brief Friend arithmetic operators for mixed-type operations with built-in integrals
         *
         * Enables symmetric expressions: int128_value + 42 AND 42 + int128_value
         * Constrained to std::is_integral_v<T> to avoid ambiguity with other types.
         * For EK: *, /, % are deleted (no meaningful semantics).
         */
        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        friend constexpr int128_param_t operator+(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs + int128_param_t{rhs};
        }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        friend constexpr int128_param_t operator+(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} + rhs;
        }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        friend constexpr int128_param_t operator-(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs - int128_param_t{rhs};
        }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        friend constexpr int128_param_t operator-(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} - rhs;
        }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        friend constexpr int128_param_t operator*(const int128_param_t &lhs, T rhs) noexcept
            requires(!is_excess_k)
        {
            return lhs * int128_param_t{rhs};
        }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        friend constexpr int128_param_t operator*(T lhs, const int128_param_t &rhs) noexcept
            requires(!is_excess_k)
        {
            return int128_param_t{lhs} * rhs;
        }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        friend constexpr int128_param_t operator/(const int128_param_t &lhs, T rhs) noexcept
            requires(!is_excess_k)
        {
            return lhs / int128_param_t{rhs};
        }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        friend constexpr int128_param_t operator/(T lhs, const int128_param_t &rhs) noexcept
            requires(!is_excess_k)
        {
            return int128_param_t{lhs} / rhs;
        }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        friend constexpr int128_param_t operator%(const int128_param_t &lhs, T rhs) noexcept
            requires(!is_excess_k)
        {
            return lhs % int128_param_t{rhs};
        }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        friend constexpr int128_param_t operator%(T lhs, const int128_param_t &rhs) noexcept
            requires(!is_excess_k)
        {
            return int128_param_t{lhs} % rhs;
        }

        /**
         * @brief Friend comparison operators for mixed-type operations
         */
        template <typename T>
        friend constexpr bool operator==(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs == int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator==(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} == rhs;
        }

        template <typename T>
        friend constexpr bool operator!=(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs != int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator!=(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} != rhs;
        }

        template <typename T>
        friend constexpr bool operator<(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs < int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator<(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} < rhs;
        }

        template <typename T>
        friend constexpr bool operator<=(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs <= int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator<=(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} <= rhs;
        }

        template <typename T>
        friend constexpr bool operator>(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs > int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator>(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} > rhs;
        }

        template <typename T>
        friend constexpr bool operator>=(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs >= int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr bool operator>=(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} >= rhs;
        }

        /**
         * @brief Friend bitwise AND for mixed-type operations
         */
        template <typename T>
        friend constexpr int128_param_t operator&(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs & int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr int128_param_t operator&(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} & rhs;
        }

        /**
         * @brief Friend bitwise OR for mixed-type operations
         */
        template <typename T>
        friend constexpr int128_param_t operator|(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs | int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr int128_param_t operator|(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} | rhs;
        }

        /**
         * @brief Friend bitwise XOR for mixed-type operations
         */
        template <typename T>
        friend constexpr int128_param_t operator^(const int128_param_t &lhs, T rhs) noexcept
        {
            return lhs ^ int128_param_t{rhs};
        }

        template <typename T>
        friend constexpr int128_param_t operator^(T lhs, const int128_param_t &rhs) noexcept
        {
            return int128_param_t{lhs} ^ rhs;
        }

        /**
         * @brief Friend swap function (ADL-findable)
         */
        friend constexpr void swap(int128_param_t &a, int128_param_t &b) noexcept
        {
            a.swap(b);
        }
    };

    // =============================================================================
    // Type Aliases
    // =============================================================================

    // Unsigned (binnat only)
    using uint128_t = int128_param_t<signedness::unsigned_type, representation_form::binnat>;

    // Signed (TC, MS, EK)
    using int128_t = int128_param_t<signedness::signed_type, representation_form::twos_complement>;
    using int128_tc_t = int128_param_t<signedness::signed_type, representation_form::twos_complement>;
    using int128_ms_t = int128_param_t<signedness::signed_type, representation_form::magnitude_sign>;
    using int128_ek_t = int128_param_t<signedness::signed_type, representation_form::excess_k>;

    // Legacy aliases (backward compatible)
    using uint128_bn_t = uint128_t; // Binario Natural = default unsigned

} // namespace nstd

#endif // INT128_PARAMETERIZED_HPP
