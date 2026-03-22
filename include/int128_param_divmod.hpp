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
// @file       int128_param_divmod.hpp
// @brief      Consteval Granlund-Montgomery division/multiplication by constants
// @author     Julián Calderón Almendros
// @date       2026-03-22
// @version    1.0.0
// =============================================================================

#pragma once

#include <cstdint>
#include <array>
#include <utility>

namespace nstd
{
    namespace divmod_detail
    {

        using std::uint64_t;
        using std::uint8_t;

        // =====================================================================
        // Consteval-only 128-bit unsigned arithmetic (no dependency on int128_param_t)
        // =====================================================================

        struct ce_uint128
        {
            uint64_t hi{0};
            uint64_t lo{0};

            constexpr ce_uint128() noexcept = default;
            constexpr ce_uint128(uint64_t h, uint64_t l) noexcept : hi{h}, lo{l} {}
            constexpr explicit ce_uint128(uint64_t v) noexcept : hi{0}, lo{v} {}

            constexpr bool operator==(const ce_uint128 &o) const noexcept
            {
                return hi == o.hi && lo == o.lo;
            }
            constexpr bool operator!=(const ce_uint128 &o) const noexcept
            {
                return !(*this == o);
            }
            constexpr bool operator<(const ce_uint128 &o) const noexcept
            {
                return hi < o.hi || (hi == o.hi && lo < o.lo);
            }
            constexpr bool operator>=(const ce_uint128 &o) const noexcept
            {
                return !(*this < o);
            }
            constexpr bool operator>(const ce_uint128 &o) const noexcept
            {
                return o < *this;
            }
            constexpr bool operator<=(const ce_uint128 &o) const noexcept
            {
                return !(o < *this);
            }

            constexpr ce_uint128 operator+(const ce_uint128 &o) const noexcept
            {
                const uint64_t new_lo{lo + o.lo};
                const uint64_t carry{(new_lo < lo) ? 1ULL : 0ULL};
                return {hi + o.hi + carry, new_lo};
            }

            constexpr ce_uint128 operator-(const ce_uint128 &o) const noexcept
            {
                const uint64_t borrow{(lo < o.lo) ? 1ULL : 0ULL};
                return {hi - o.hi - borrow, lo - o.lo};
            }

            constexpr ce_uint128 operator>>(int s) const noexcept
            {
                if (s == 0)
                {
                    return *this;
                }
                if (s >= 128)
                {
                    return {0, 0};
                }
                if (s >= 64)
                {
                    return {0, hi >> (s - 64)};
                }
                return {hi >> s, (lo >> s) | (hi << (64 - s))};
            }

            constexpr ce_uint128 operator<<(int s) const noexcept
            {
                if (s == 0)
                {
                    return *this;
                }
                if (s >= 128)
                {
                    return {0, 0};
                }
                if (s >= 64)
                {
                    return {lo << (s - 64), 0};
                }
                return {(hi << s) | (lo >> (64 - s)), lo << s};
            }
        };

        // =====================================================================
        // Consteval 128-bit / 64-bit division
        // =====================================================================

        struct div_result_128_64
        {
            ce_uint128 quotient;
            uint64_t remainder;
        };

        /// @brief Divide 128-bit by 64-bit (bit-by-bit long division, constexpr)
        constexpr div_result_128_64 div_128_by_64(ce_uint128 n, uint64_t d) noexcept
        {
            // High part: q_hi = n.hi / d, remainder = n.hi % d
            const uint64_t q_hi{n.hi / d};
            uint64_t rem{n.hi % d};

            // Low part: divide [rem:n.lo] by d where rem < d
            // Bit-by-bit long division (64 iterations, fine for consteval)
            uint64_t q_lo{0};
            for (int i{63}; i >= 0; --i)
            {
                const uint64_t bit{(n.lo >> i) & 1ULL};
                const bool will_overflow{(rem >> 63) != 0};
                rem = (rem << 1) | bit;
                if (will_overflow || rem >= d)
                {
                    rem -= d;
                    q_lo |= (1ULL << i);
                }
            }
            return {{q_hi, q_lo}, rem};
        }

        // =====================================================================
        // GM Entry — Magic constants for a single divisor
        // =====================================================================

        struct gm_entry
        {
            uint64_t M_hi;       ///< Multiplier high limb
            uint64_t M_lo;       ///< Multiplier low limb
            uint8_t shift;       ///< Post-shift amount S
            bool needs_overflow; ///< true = add-shift correction, false = simple multiply-shift
        };

        // =====================================================================
        // compute_magic_128 — Granlund-Montgomery consteval
        //
        // Based on Hacker's Delight 2nd Ed, Section 10-9.
        // Adapted for 128-bit unsigned division by 64-bit constant.
        // =====================================================================

        constexpr gm_entry compute_magic_128(uint64_t d) noexcept
        {
            // W = 128
            const ce_uint128 UMAX{0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
            const ce_uint128 half{0x8000000000000000ULL, 0ULL}; // 2^127
            const ce_uint128 one{0ULL, 1ULL};
            const ce_uint128 zero{0ULL, 0ULL};

            // nc = (2^128-1) - ((2^128-1) mod d) - 1
            // nc is the largest value < 2^128 such that nc+1 is NOT a multiple of d
            const auto [ignore_q, umax_mod_d]{div_128_by_64(UMAX, d)};
            const ce_uint128 nc{UMAX - ce_uint128{umax_mod_d} - one};

            // q1 = floor(2^127 / nc), r1 = 2^127 mod nc
            // Since d < 2^64, nc >= 2^128 - 2^64 - 1 > 2^127.
            // Therefore q1 = 0 and r1 = 2^127.
            ce_uint128 q1{zero};
            ce_uint128 r1{half};

            // q2 = floor((2^127 - 1) / d), r2 = (2^127-1) mod d
            const ce_uint128 half_m1{half - one};
            const auto [q2_init, r2_init]{div_128_by_64(half_m1, d)};

            ce_uint128 q2{q2_init};
            uint64_t r2{r2_init};
            bool q2_overflowed{false};

            int p{127};

            for (;;)
            {
                ++p;

                // Update q1, r1: track 2^p / nc
                if (r1 >= nc - r1)
                {
                    q1 = q1 + q1 + one;
                    r1 = r1 + r1 - nc;
                }
                else
                {
                    q1 = q1 + q1;
                    r1 = r1 + r1;
                }

                // Update q2, r2: track (2^p - 1) / d
                // Conceptually: new = 2*old + 1; if new >= d then q2++ and new -= d
                if (r2 + 1 >= d - r2)
                {
                    // q2 = 2*q2 + 1 (may overflow 128 bits)
                    if (!q2_overflowed && q2 >= half)
                    {
                        q2_overflowed = true;
                    }
                    q2 = q2 + q2 + one;
                    r2 = r2 + r2 + 1 - d;
                }
                else
                {
                    // q2 = 2*q2
                    if (!q2_overflowed && q2 >= half)
                    {
                        q2_overflowed = true;
                    }
                    q2 = q2 + q2;
                    r2 = r2 + r2 + 1;
                }

                // Termination check
                const uint64_t delta{d - 1 - r2};
                const ce_uint128 delta128{0ULL, delta};

                // Break when q1 > delta, or (q1 == delta and r1 != 0)
                if (q1 > delta128 || (q1 == delta128 && r1 != zero))
                {
                    break;
                }

                // Safety bound: should never need more than W extra iterations
                if (p >= 256)
                {
                    break;
                }
            }

            // M = q2 + 1 (conceptually up to 129 bits if overflow)
            const ce_uint128 M{q2 + one};
            const uint8_t shift_val{static_cast<uint8_t>(p - 128)};

            return {M.hi, M.lo, shift_val, q2_overflowed};
        }

        // =====================================================================
        // Consteval helpers
        // =====================================================================

        /// @brief Count trailing zeros (for power-of-2 shift amounts)
        constexpr int ctz64(uint64_t v) noexcept
        {
            if (v == 0)
            {
                return 64;
            }
            int count{0};
            while ((v & 1ULL) == 0)
            {
                ++count;
                v >>= 1;
            }
            return count;
        }

        /// @brief Check if value is power of 2
        constexpr bool is_pow2(uint64_t v) noexcept
        {
            return v > 0 && (v & (v - 1)) == 0;
        }

        // =====================================================================
        // GM Lookup Table [0..1023]
        //
        // Indices 0, 1: unused (d=0 invalid, d=1 trivial)
        // Powers of 2: zeroed (handled by shift path)
        // All others: GM constants from compute_magic_128
        // =====================================================================

        inline constexpr uint64_t GM_LOOKUP_MAX{1023};

        inline constexpr std::array<gm_entry, GM_LOOKUP_MAX + 1> GM_TABLE =
            []() constexpr
        {
            std::array<gm_entry, GM_LOOKUP_MAX + 1> table{};
            for (uint64_t d{3}; d <= GM_LOOKUP_MAX; ++d)
            {
                if (is_pow2(d))
                {
                    continue; // Powers of 2 use shift path
                }
                table[d] = compute_magic_128(d);
            }
            return table;
        }();

        // =====================================================================
        // Constexpr-friendly 64x64 -> 128 multiplication (pure C++, no intrinsics)
        // =====================================================================

        struct u128_pair
        {
            uint64_t hi;
            uint64_t lo;
        };

        /// @brief Full 64x64 → 128-bit product using 32-bit half-products.
        /// Pure constexpr — no compiler intrinsics.
        static constexpr u128_pair mul64_full(uint64_t a, uint64_t b) noexcept
        {
            const uint64_t a_lo{a & 0xFFFFFFFFULL};
            const uint64_t a_hi{a >> 32};
            const uint64_t b_lo{b & 0xFFFFFFFFULL};
            const uint64_t b_hi{b >> 32};

            const uint64_t p00{a_lo * b_lo};
            const uint64_t p01{a_lo * b_hi};
            const uint64_t p10{a_hi * b_lo};
            const uint64_t p11{a_hi * b_hi};

            // Accumulate middle column (bits 32-95)
            const uint64_t mid{(p00 >> 32) + (p01 & 0xFFFFFFFFULL) + (p10 & 0xFFFFFFFFULL)};

            const uint64_t result_lo{(p00 & 0xFFFFFFFFULL) | ((mid & 0xFFFFFFFFULL) << 32)};
            const uint64_t result_hi{p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32)};

            return {result_hi, result_lo};
        }

        // =====================================================================
        // Constexpr mulhi_128 — upper 128 bits of 128x128 → 256 product.
        // Pure C++ (no intrinsics), suitable for constexpr contexts.
        // =====================================================================

        struct mulhi_result
        {
            uint64_t hi;
            uint64_t lo;
        };

        /// @brief Upper 128 bits of (n_hi:n_lo) * (m_hi:m_lo).
        /// Uses schoolbook 4-product decomposition with mul64_full.
        static constexpr mulhi_result ce_mulhi_128(
            uint64_t n_hi, uint64_t n_lo,
            uint64_t m_hi, uint64_t m_lo) noexcept
        {
            // n*m = n_hi*m_hi * 2^128 + (n_hi*m_lo + n_lo*m_hi) * 2^64 + n_lo*m_lo
            // We need bits [255:128] of the 256-bit product.

            const auto [p00_hi, p00_lo]{mul64_full(n_lo, m_lo)}; // bits [0..127]
            const auto [p01_hi, p01_lo]{mul64_full(n_lo, m_hi)}; // bits [64..191]
            const auto [p10_hi, p10_lo]{mul64_full(n_hi, m_lo)}; // bits [64..191]
            const auto [p11_hi, p11_lo]{mul64_full(n_hi, m_hi)}; // bits [128..255]

            // Column at position 64: p00_hi + p01_lo + p10_lo → carry feeds position 128
            uint64_t col1{p00_hi};
            uint64_t carry1{0};

            {
                const uint64_t tmp{col1 + p01_lo};
                carry1 += (tmp < col1) ? 1ULL : 0ULL;
                col1 = tmp;
            }
            {
                const uint64_t tmp{col1 + p10_lo};
                carry1 += (tmp < col1) ? 1ULL : 0ULL;
                col1 = tmp;
            }

            // Column at position 128 (h_lo): p01_hi + p10_hi + p11_lo + carry1
            uint64_t h_lo{p01_hi};
            uint64_t carry2{0};

            {
                const uint64_t tmp{h_lo + p10_hi};
                carry2 += (tmp < h_lo) ? 1ULL : 0ULL;
                h_lo = tmp;
            }
            {
                const uint64_t tmp{h_lo + p11_lo};
                carry2 += (tmp < h_lo) ? 1ULL : 0ULL;
                h_lo = tmp;
            }
            {
                const uint64_t tmp{h_lo + carry1};
                carry2 += (tmp < h_lo) ? 1ULL : 0ULL;
                h_lo = tmp;
            }

            // Column at position 192 (h_hi): p11_hi + carry2
            const uint64_t h_hi{p11_hi + carry2};

            return {h_hi, h_lo};
        }

        // =====================================================================
        // Runtime/constexpr division using GM entry (operates on raw limbs)
        // =====================================================================

        struct limb_pair
        {
            uint64_t hi;
            uint64_t lo;
        };

        /// @brief Apply right-shift to a 128-bit {hi, lo} pair.
        static constexpr limb_pair rshift_128(uint64_t hi, uint64_t lo, int s) noexcept
        {
            if (s == 0)
            {
                return {hi, lo};
            }
            if (s >= 64)
            {
                return {0ULL, hi >> (s - 64)};
            }
            return {hi >> s, (lo >> s) | (hi << (64 - s))};
        }

        /// @brief GM division on raw 128-bit limbs using precomputed entry.
        /// Returns quotient as {hi, lo} pair.
        static constexpr limb_pair gm_div_limbs(
            uint64_t n_hi, uint64_t n_lo,
            const gm_entry &entry) noexcept
        {
            // t = mulhi(n, M)
            const auto [t_hi, t_lo]{ce_mulhi_128(n_hi, n_lo, entry.M_hi, entry.M_lo)};

            if (entry.needs_overflow)
            {
                // Overflow correction: q = (t + ((n - t) >> 1)) >> (shift - 1)
                // diff = n - t
                const uint64_t borrow{(n_lo < t_lo) ? 1ULL : 0ULL};
                const uint64_t diff_lo{n_lo - t_lo};
                const uint64_t diff_hi{n_hi - t_hi - borrow};

                // half_diff = diff >> 1
                const uint64_t hd_lo{(diff_lo >> 1) | (diff_hi << 63)};
                const uint64_t hd_hi{diff_hi >> 1};

                // corrected = t + half_diff
                const uint64_t c_lo{t_lo + hd_lo};
                const uint64_t carry{(c_lo < t_lo) ? 1ULL : 0ULL};
                const uint64_t c_hi{t_hi + hd_hi + carry};

                // q = corrected >> (shift - 1)
                return rshift_128(c_hi, c_lo, entry.shift - 1);
            }
            else
            {
                // Simple: q = t >> shift
                return rshift_128(t_hi, t_lo, entry.shift);
            }
        }

        // =====================================================================
        // 128-bit subtraction on raw limbs (for remainder = n - q*d)
        // =====================================================================

        /// @brief Multiply 128-bit {hi,lo} by 64-bit scalar d. Returns low 128 bits.
        static constexpr limb_pair mul_128_by_64(
            uint64_t hi, uint64_t lo, uint64_t d) noexcept
        {
            const auto [prod_lo_hi, prod_lo_lo]{mul64_full(lo, d)};
            const auto [prod_hi_hi, prod_hi_lo]{mul64_full(hi, d)};

            // result_lo = prod_lo_lo
            // result_hi = prod_lo_hi + prod_hi_lo (ignore overflow, only need low 128)
            return {prod_lo_hi + prod_hi_lo, prod_lo_lo};
        }

        /// @brief Subtract two 128-bit limb pairs: a - b.
        static constexpr limb_pair sub_128(
            uint64_t a_hi, uint64_t a_lo,
            uint64_t b_hi, uint64_t b_lo) noexcept
        {
            const uint64_t borrow{(a_lo < b_lo) ? 1ULL : 0ULL};
            return {a_hi - b_hi - borrow, a_lo - b_lo};
        }

    } // namespace divmod_detail
} // namespace nstd
