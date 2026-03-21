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
// @file       algorithms/karatsuba.hpp
// @brief      Karatsuba sub-quadratic multiplication (128x128->256)
// @author     Julián Calderón Almendros
// @date       2025-07-23
// @version    1.0.0
// =============================================================================

#pragma once

#include "int128_parameterized.hpp"

#include <array>
#include <cstdint>

namespace nstd {
namespace algorithms {

using std::uint64_t;

// =============================================================================
// uint256_result — 256-bit unsigned result from full 128x128 multiplication
// =============================================================================

/**
 * @brief 256-bit unsigned result stored as 4 little-endian 64-bit limbs.
 *
 * @details limbs[0] = least significant, limbs[3] = most significant.
 */
struct uint256_result {
    std::array<uint64_t, 4> limbs{};

    constexpr bool operator==(const uint256_result& other) const noexcept = default;
    constexpr bool operator!=(const uint256_result& other) const noexcept = default;

    /// Lower 128 bits as uint128_t
    inline uint128_t low128() const noexcept {
        return uint128_t{limbs[1], limbs[0]};
    }

    /// Upper 128 bits as uint128_t
    inline uint128_t high128() const noexcept {
        return uint128_t{limbs[3], limbs[2]};
    }

    /// Check if zero
    constexpr bool is_zero() const noexcept {
        return limbs[0] == 0 && limbs[1] == 0 && limbs[2] == 0 && limbs[3] == 0;
    }
};

// =============================================================================
// schoolbook_full_mul — Reference 128x128->256 (4 multiplications)
// =============================================================================

/**
 * @brief Schoolbook 128x128->256 full multiplication using 4 umul128 calls.
 *
 * @details Computes the full 256-bit product of two 128-bit unsigned integers.
 * Uses standard long multiplication with 64-bit limbs.
 * Serves as reference for verifying Karatsuba correctness.
 *
 * @param a First 128-bit operand
 * @param b Second 128-bit operand
 * @return Full 256-bit product
 */
inline uint256_result schoolbook_full_mul(const uint128_t& a, const uint128_t& b) noexcept {
    const uint64_t a_L{a.low()};
    const uint64_t a_H{a.high()};
    const uint64_t b_L{b.low()};
    const uint64_t b_H{b.high()};

    // 4 partial products: p_ij = a_i * b_j (each 64x64->128)
    uint64_t p00_H{0};
    const uint64_t p00_L{intrinsics::umul128(a_L, b_L, &p00_H)};

    uint64_t p01_H{0};
    const uint64_t p01_L{intrinsics::umul128(a_L, b_H, &p01_H)};

    uint64_t p10_H{0};
    const uint64_t p10_L{intrinsics::umul128(a_H, b_L, &p10_H)};

    uint64_t p11_H{0};
    const uint64_t p11_L{intrinsics::umul128(a_H, b_H, &p11_H)};

    // Combine: result = p11 * B^2 + (p01 + p10) * B + p00  (B = 2^64)
    uint256_result r{};
    r.limbs[0] = p00_L;

    // limbs[1] = p00_H + p01_L + p10_L
    uint64_t c1{0};
    uint64_t c2{0};
    intrinsics::addcarry_u64(0, p00_H, p01_L, &r.limbs[1]);
    c1 = (r.limbs[1] < p00_H) ? 1u : 0u;
    const uint64_t tmp1{r.limbs[1]};
    intrinsics::addcarry_u64(0, r.limbs[1], p10_L, &r.limbs[1]);
    c2 = (r.limbs[1] < tmp1) ? 1u : 0u;
    const uint64_t carry_to_2{c1 + c2};

    // limbs[2] = p01_H + p10_H + p11_L + carry
    uint64_t c3{0};
    uint64_t c4{0};
    uint64_t c5{0};
    intrinsics::addcarry_u64(0, p01_H, p10_H, &r.limbs[2]);
    c3 = (r.limbs[2] < p01_H) ? 1u : 0u;
    const uint64_t tmp2{r.limbs[2]};
    intrinsics::addcarry_u64(0, r.limbs[2], p11_L, &r.limbs[2]);
    c4 = (r.limbs[2] < tmp2) ? 1u : 0u;
    const uint64_t tmp3{r.limbs[2]};
    intrinsics::addcarry_u64(0, r.limbs[2], carry_to_2, &r.limbs[2]);
    c5 = (r.limbs[2] < tmp3) ? 1u : 0u;

    // limbs[3] = p11_H + carry
    r.limbs[3] = p11_H + c3 + c4 + c5;

    return r;
}

// =============================================================================
// karatsuba_full_mul — Sub-quadratic 128x128->256 (3 multiplications)
// =============================================================================

/**
 * @brief Karatsuba 128x128->256 full multiplication using 3 umul128 calls.
 *
 * @details Computes the full 256-bit product using the Karatsuba algorithm:
 *   z0 = a_L * b_L
 *   z2 = a_H * b_H
 *   z1 = (a_L + a_H)(b_L + b_H) - z0 - z2  (= a_L*b_H + a_H*b_L)
 *   result = z2 * B^2 + z1 * B + z0  (B = 2^64)
 *
 * The sums (a_L + a_H) and (b_L + b_H) can overflow to 65 bits,
 * which requires careful carry propagation.
 *
 * @param a First 128-bit operand
 * @param b Second 128-bit operand
 * @return Full 256-bit product
 */
inline uint256_result karatsuba_full_mul(const uint128_t& a, const uint128_t& b) noexcept {
    const uint64_t a_L{a.low()};
    const uint64_t a_H{a.high()};
    const uint64_t b_L{b.low()};
    const uint64_t b_H{b.high()};

    // Step 1: z0 = a_L * b_L (128-bit)
    uint64_t z0_H{0};
    const uint64_t z0_L{intrinsics::umul128(a_L, b_L, &z0_H)};

    // Step 2: z2 = a_H * b_H (128-bit)
    uint64_t z2_H{0};
    const uint64_t z2_L{intrinsics::umul128(a_H, b_H, &z2_H)};

    // Step 3: 65-bit sums with carry
    const uint64_t s_a{a_L + a_H};
    const unsigned char c_a{(s_a < a_L) ? static_cast<unsigned char>(1) : static_cast<unsigned char>(0)};

    const uint64_t s_b{b_L + b_H};
    const unsigned char c_b{(s_b < b_L) ? static_cast<unsigned char>(1) : static_cast<unsigned char>(0)};

    // Step 4: core product s_core = s_a * s_b (128-bit part)
    uint64_t sc_H{0};
    const uint64_t sc_L{intrinsics::umul128(s_a, s_b, &sc_H)};

    // Step 5: full 130-bit product p = (c_a:s_a) * (c_b:s_b)
    //   = s_core + c_a*s_b*B + c_b*s_a*B + c_a*c_b*B^2
    // Stored as 3 limbs: [p2 : p1 : p0]
    uint64_t p0{sc_L};
    uint64_t p1{sc_H};
    uint64_t p2{static_cast<uint64_t>(c_a & c_b)};

    if (c_a) {
        const uint64_t old{p1};
        p1 += s_b;
        if (p1 < old) { ++p2; }
    }
    if (c_b) {
        const uint64_t old{p1};
        p1 += s_a;
        if (p1 < old) { ++p2; }
    }

    // Step 6: z1 = p - z0 - z2 (3-limb subtraction)
    // First: t = p - z0
    uint64_t t0{0};
    uint64_t t1{0};
    unsigned char bw1{intrinsics::subborrow_u64(0, p0, z0_L, &t0)};
    unsigned char bw2{intrinsics::subborrow_u64(bw1, p1, z0_H, &t1)};
    const uint64_t t2{p2 - bw2};

    // Second: z1 = t - z2
    uint64_t z1_0{0};
    uint64_t z1_1{0};
    unsigned char bw3{intrinsics::subborrow_u64(0, t0, z2_L, &z1_0)};
    unsigned char bw4{intrinsics::subborrow_u64(bw3, t1, z2_H, &z1_1)};
    const uint64_t z1_2{t2 - bw4};

    // Step 7: Combine result = z2 * B^2 + z1 * B + z0
    // r[0] = z0_L
    // r[1] = z0_H + z1_0
    // r[2] = z2_L + z1_1 + carry
    // r[3] = z2_H + z1_2 + carry
    uint256_result r{};
    r.limbs[0] = z0_L;

    unsigned char ca1{intrinsics::addcarry_u64(0, z0_H, z1_0, &r.limbs[1])};
    unsigned char ca2{intrinsics::addcarry_u64(ca1, z2_L, z1_1, &r.limbs[2])};
    r.limbs[3] = z2_H + z1_2 + ca2;

    return r;
}

} // namespace algorithms
} // namespace nstd
