// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// fixed_width_int_t.hpp — Fixed-width integer templates (N x 64-bit limbs)
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// fixed_int_t<N, Sign, Form>: unified signed/unsigned fixed-width integer.
// data[0] = lowest limb (LSB), data[N-1] = highest limb (MSB).
//
// uint_fixed_t<N> = fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>
// int_fixed_t<N>  = fixed_int_t<N, signedness::signed_type,   representation_form::twos_complement>
//
// N=1 -> 64-bit, N=2 -> 128-bit, N=4 -> 256-bit, N=8 -> 512-bit
//
// Operations (all mod 2^(64N)):
//   Construction: from any integral T, from limb array, from bytes, from bitset
//   Assignment:   from any integral T
//   Arithmetic:   +, -, *, /, %, unary -, ++, --
//   Bitwise:      &, |, ^, ~, <<, >>
//   Comparison:   ==, !=, <, <=, >, >=
//   Conversion:   explicit operator bool/T/bytes/bitset; to_string/from_string
//   Utilities:    zero(), max(), min(), one(), is_zero(), bit_width(), popcount()

#ifndef FIXED_WIDTH_INT_T_HPP
#define FIXED_WIDTH_INT_T_HPP

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "representation.hpp"

#if __has_include("intrinsics/arithmetic_operations.hpp")
#include "intrinsics/arithmetic_operations.hpp"
#endif

#if __has_include("intrinsics/bit_operations.hpp")
#include "intrinsics/bit_operations.hpp"
#endif

namespace nstd
{

    // =========================================================================
    // Parse error codes and result type
    // =========================================================================

#ifndef NSTD_PARSE_COMMON_DEFINED
#define NSTD_PARSE_COMMON_DEFINED
    enum class parse_error : std::uint8_t
    {
        success = 0,
        null_pointer,
        empty_string,
        invalid_base,
        invalid_base_value,
        invalid_character,
        digit_out_of_range,
        no_digits,
        overflow,
        separator_at_boundaries,
        unknown_error
    };

    template <typename T>
    struct parse_result
    {
        parse_error error;
        T value;
        std::size_t error_index;

        constexpr bool success() const noexcept
        {
            return error == parse_error::success;
        }

        constexpr parse_result() noexcept
            : error(parse_error::success),
              value(T{}),
              error_index(std::string::npos)
        {
        }

        constexpr parse_result(parse_error err, T val, std::size_t idx) noexcept
            : error(err),
              value(val),
              error_index(idx)
        {
        }
    };
#endif // NSTD_PARSE_COMMON_DEFINED

    // Forward declaration so cross-type constructors can reference the alias
    template <std::size_t N,
              signedness Sign,
              representation_form Form>
    class fixed_int_t;

    // =============================================================================
    // Aliases (forward-declared so cross-type constructors compile)
    // =============================================================================

    template <std::size_t N>
    using uint_fixed_t = fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>;

    template <std::size_t N>
    using int_fixed_t = fixed_int_t<N, signedness::signed_type, representation_form::twos_complement>;

    // =============================================================================
    // fixed_int_t<N, Sign, Form> — unified signed/unsigned N-limb integer
    // =============================================================================

    template <std::size_t N,
              signedness Sign  = signedness::unsigned_type,
              representation_form Form = (Sign == signedness::unsigned_type
                                          ? representation_form::binnat
                                          : representation_form::twos_complement)>
    class fixed_int_t
    {
        static_assert(N >= 1, "fixed_int_t requires at least 1 limb");
        static_assert(
            (Sign == signedness::unsigned_type && Form == representation_form::binnat) ||
            (Sign == signedness::signed_type   && Form == representation_form::twos_complement),
            "Only binnat (unsigned) and twos_complement (signed) are currently implemented");

        static constexpr bool is_signed = (Sign == signedness::signed_type);

    public:
        static constexpr signedness sign{Sign};
        static constexpr representation_form form{Form};

        // data[0] = least-significant limb, data[N-1] = most-significant limb
        std::array<std::uint64_t, N> data{};

        // =========================================================================
        // Construction
        // =========================================================================

        constexpr fixed_int_t() noexcept = default;

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        explicit constexpr fixed_int_t(T v) noexcept : data{}
        {
            data[0] = static_cast<std::uint64_t>(v);
            const std::uint64_t fill = (std::is_signed_v<T> && v < 0) ? ~std::uint64_t{0} : std::uint64_t{0};
            for (std::size_t i{1}; i < N; ++i)
                data[i] = fill;
        }

        // Construct from array of limbs (data[0]=LSB, data[N-1]=MSB)
        explicit constexpr fixed_int_t(std::array<std::uint64_t, N> limbs) noexcept
            : data{limbs}
        {
        }

        explicit constexpr fixed_int_t(const std::array<std::byte, N * 8> &bytes) noexcept : data{}
        {
            for (std::size_t i{0}; i < N; ++i)
                for (int j{0}; j < 8; ++j)
                    data[i] |= static_cast<std::uint64_t>(bytes[i * 8 + j]) << (j * 8);
        }

        explicit constexpr fixed_int_t(const std::bitset<64 * N> &b) noexcept : data{}
        {
            for (std::size_t i{0}; i < N; ++i)
                for (int j{0}; j < 64; ++j)
                    if (b.test(i * 64 + j))
                        data[i] |= std::uint64_t{1} << j;
        }

#ifdef __SIZEOF_INT128__
        explicit constexpr fixed_int_t(unsigned __int128 v) noexcept : data{}
        {
            for (std::size_t i{0}; i < N && i < 2; ++i)
                data[i] = static_cast<std::uint64_t>(v >> (i * 64));
        }

        explicit constexpr fixed_int_t(__int128 v) noexcept : data{}
        {
            const unsigned __int128 uv = static_cast<unsigned __int128>(v);
            for (std::size_t i{0}; i < N && i < 2; ++i)
                data[i] = static_cast<std::uint64_t>(uv >> (i * 64));
            const std::uint64_t fill = (v < 0) ? ~std::uint64_t{0} : std::uint64_t{0};
            for (std::size_t i{2}; i < N; ++i)
                data[i] = fill;
        }
#endif

        // Cross-type constructor: from any other fixed_int_t<M, S2, F2>
        // Handles: same-sign different-N, different-sign, or any combination
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<(M != N || S2 != Sign || F2 != Form)>>
        explicit constexpr fixed_int_t(const fixed_int_t<M, S2, F2> &o) noexcept : data{}
        {
            constexpr std::size_t copy = M < N ? M : N;
            for (std::size_t i{0}; i < copy; ++i)
                data[i] = o.data[i];
            // sign-extend if source is signed and negative
            const bool src_neg = (S2 == signedness::signed_type) && ((o.data[M - 1] >> 63) != 0);
            const std::uint64_t fill = src_neg ? ~std::uint64_t{0} : std::uint64_t{0};
            for (std::size_t i{M}; i < N; ++i)
                data[i] = fill;
        }

        template <typename F, std::enable_if_t<std::is_floating_point_v<F>, int> = 0>
        explicit fixed_int_t(F v) noexcept : data{}
        {
            if constexpr (is_signed)
            {
                if (v >= F{0})
                    *this = fixed_int_t{uint_fixed_t<N>{v}};
                else
                    *this = fixed_int_t{-uint_fixed_t<N>{-v}};
            }
            else
            {
                if (!(v >= F{1}))
                    return;
                constexpr long double base = 18446744073709551616.0L; // 2^64
                long double tmp = std::trunc(static_cast<long double>(v));
                for (std::size_t i{0}; i < N && tmp >= 1.0L; ++i)
                {
                    const long double rem = std::fmod(tmp, base);
                    data[i] = static_cast<std::uint64_t>(rem);
                    tmp = std::trunc(tmp / base);
                }
            }
        }

        // =========================================================================
        // Named constructors
        // =========================================================================

        static constexpr fixed_int_t zero() noexcept
        {
            return fixed_int_t{};
        }

        static constexpr fixed_int_t one() noexcept
        {
            return fixed_int_t{std::uint64_t{1}};
        }

        static constexpr fixed_int_t max() noexcept
        {
            fixed_int_t r{};
            if constexpr (!is_signed)
            {
                for (auto &limb : r.data)
                    limb = ~std::uint64_t{0};
            }
            else
            {
                for (auto &limb : r.data)
                    limb = ~std::uint64_t{0};
                r.data[N - 1] >>= 1; // clear MSB
            }
            return r;
        }

        static constexpr fixed_int_t min() noexcept
        {
            if constexpr (!is_signed)
            {
                return fixed_int_t{};
            }
            else
            {
                fixed_int_t r{};
                r.data[N - 1] = std::uint64_t{1} << 63;
                return r;
            }
        }

        // Aliases for compatibility with old int_fixed_t
        template <bool S = is_signed, typename = std::enable_if_t<S>>
        static constexpr fixed_int_t max_val() noexcept { return max(); }

        template <bool S = is_signed, typename = std::enable_if_t<S>>
        static constexpr fixed_int_t min_val() noexcept { return min(); }

        // =========================================================================
        // Assignment
        // =========================================================================

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator=(T v) noexcept
        {
            return *this = fixed_int_t{v};
        }

#ifdef __SIZEOF_INT128__
        constexpr fixed_int_t &operator=(unsigned __int128 v) noexcept
        {
            return *this = fixed_int_t{v};
        }

        constexpr fixed_int_t &operator=(__int128 v) noexcept
        {
            return *this = fixed_int_t{v};
        }
#endif

        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<(M != N || S2 != Sign || F2 != Form)>>
        constexpr fixed_int_t &operator=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            return *this = fixed_int_t{o};
        }

        template <typename F, std::enable_if_t<std::is_floating_point_v<F>, int> = 0>
        fixed_int_t &operator=(F v) noexcept
        {
            return *this = fixed_int_t{v};
        }

        // =========================================================================
        // Predicates
        // =========================================================================

        constexpr bool is_zero() const noexcept
        {
            for (const auto &limb : data)
                if (limb != 0)
                    return false;
            return true;
        }

        constexpr bool is_negative() const noexcept
        {
            if constexpr (!is_signed)
                return false;
            else
                return (data[N - 1] >> 63) != 0;
        }

        // =========================================================================
        // Explicit conversions
        // =========================================================================

        [[nodiscard]] explicit constexpr operator bool() const noexcept
        {
            return !is_zero();
        }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        [[nodiscard]] explicit constexpr operator T() const noexcept
        {
            return static_cast<T>(data[0]);
        }

        [[nodiscard]] explicit constexpr operator std::array<std::byte, N * 8>() const noexcept
        {
            std::array<std::byte, N * 8> result{};
            for (std::size_t i{0}; i < N; ++i)
                for (int j{0}; j < 8; ++j)
                    result[i * 8 + j] = static_cast<std::byte>((data[i] >> (j * 8)) & 0xFF);
            return result;
        }

        [[nodiscard]] explicit constexpr operator std::bitset<64 * N>() const noexcept
        {
            std::bitset<64 * N> result{};
            for (std::size_t i{0}; i < N; ++i)
                for (int j{0}; j < 64; ++j)
                    if ((data[i] & (std::uint64_t{1} << j)) != 0)
                        result.set(i * 64 + j);
            return result;
        }

#ifdef __SIZEOF_INT128__
        [[nodiscard]] explicit constexpr operator unsigned __int128() const noexcept
        {
            unsigned __int128 r{0};
            for (std::size_t i{0}; i < N && i < 2; ++i)
                r |= static_cast<unsigned __int128>(data[i]) << (i * 64);
            return r;
        }

        [[nodiscard]] explicit constexpr operator __int128() const noexcept
        {
            return static_cast<__int128>(static_cast<unsigned __int128>(*this));
        }
#endif

        template <typename F, std::enable_if_t<std::is_floating_point_v<F>, int> = 0>
        [[nodiscard]] explicit operator F() const noexcept
        {
            if constexpr (is_signed)
            {
                if (is_negative())
                    return -static_cast<F>(uint_fixed_t<N>{-(*this)});
                return static_cast<F>(uint_fixed_t<N>{*this});
            }
            else
            {
                constexpr long double base = 18446744073709551616.0L; // 2^64
                long double result{0};
                for (std::size_t i{N}; i-- > 0;)
                    result = result * base + static_cast<long double>(data[i]);
                return static_cast<F>(result);
            }
        }

        // =========================================================================
        // Comparison
        // =========================================================================

        constexpr bool operator==(const fixed_int_t &o) const noexcept
        {
            return data == o.data;
        }

        constexpr bool operator!=(const fixed_int_t &o) const noexcept
        {
            return !(*this == o);
        }

        constexpr bool operator<(const fixed_int_t &o) const noexcept
        {
            if constexpr (is_signed)
            {
                const bool a_neg = is_negative();
                const bool b_neg = o.is_negative();
                if (a_neg != b_neg)
                    return a_neg;
                // same sign: unsigned limb comparison
            }
            for (std::size_t i{N}; i-- > 0;)
            {
                if (data[i] != o.data[i])
                    return data[i] < o.data[i];
            }
            return false;
        }

        constexpr bool operator<=(const fixed_int_t &o) const noexcept
        {
            return !(o < *this);
        }

        constexpr bool operator>(const fixed_int_t &o) const noexcept
        {
            return o < *this;
        }

        constexpr bool operator>=(const fixed_int_t &o) const noexcept
        {
            return !(*this < o);
        }

        // =========================================================================
        // Bitwise
        // =========================================================================

        constexpr fixed_int_t operator~() const noexcept
        {
            fixed_int_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = ~data[i];
            return r;
        }

        constexpr fixed_int_t operator&(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = data[i] & o.data[i];
            return r;
        }

        constexpr fixed_int_t operator|(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = data[i] | o.data[i];
            return r;
        }

        constexpr fixed_int_t operator^(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = data[i] ^ o.data[i];
            return r;
        }

        constexpr fixed_int_t &operator&=(const fixed_int_t &o) noexcept
        {
            *this = *this & o;
            return *this;
        }

        constexpr fixed_int_t &operator|=(const fixed_int_t &o) noexcept
        {
            *this = *this | o;
            return *this;
        }

        constexpr fixed_int_t &operator^=(const fixed_int_t &o) noexcept
        {
            *this = *this ^ o;
            return *this;
        }

        // Left shift (logical for both signed and unsigned)
        constexpr fixed_int_t operator<<(unsigned shift) const noexcept
        {
            fixed_int_t r{};
            if (shift >= 64U * N)
                return r;
            const std::size_t ls = shift / 64U;
            const unsigned bs = shift % 64U;
            if (bs == 0)
            {
                for (std::size_t i{ls}; i < N; ++i)
                    r.data[i] = data[i - ls];
            }
            else
            {
                for (std::size_t i{ls}; i < N; ++i)
                {
                    r.data[i] = data[i - ls] << bs;
                    if (i > ls)
                        r.data[i] |= data[i - ls - 1] >> (64U - bs);
                }
            }
            return r;
        }

        // Right shift: logical for unsigned, arithmetic for signed
        constexpr fixed_int_t operator>>(unsigned shift) const noexcept
        {
            if constexpr (!is_signed)
            {
                fixed_int_t r{};
                if (shift >= 64U * N)
                    return r;
                const std::size_t ls = shift / 64U;
                const unsigned bs = shift % 64U;
                if (bs == 0)
                {
                    for (std::size_t i{0}; i + ls < N; ++i)
                        r.data[i] = data[i + ls];
                }
                else
                {
                    for (std::size_t i{0}; i + ls < N; ++i)
                    {
                        r.data[i] = data[i + ls] >> bs;
                        if (i + ls + 1 < N)
                            r.data[i] |= data[i + ls + 1] << (64U - bs);
                    }
                }
                return r;
            }
            else
            {
                // arithmetic right shift
                if (shift == 0)
                    return *this;
                if (shift >= 64U * N)
                    return is_negative() ? fixed_int_t{std::int64_t{-1}} : zero();
                if (!is_negative())
                    return fixed_int_t{uint_fixed_t<N>{*this} >> shift};
                const uint_fixed_t<N> fill = uint_fixed_t<N>::max() << (64U * N - shift);
                return fixed_int_t{(uint_fixed_t<N>{*this} >> shift) | fill};
            }
        }

        constexpr fixed_int_t &operator<<=(unsigned shift) noexcept
        {
            *this = *this << shift;
            return *this;
        }

        constexpr fixed_int_t &operator>>=(unsigned shift) noexcept
        {
            *this = *this >> shift;
            return *this;
        }

        // =========================================================================
        // Arithmetic — addition/subtraction (ripple-carry via intrinsics or portable)
        // =========================================================================

        constexpr fixed_int_t operator+(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};
            unsigned char carry{0};
            for (std::size_t i{0}; i < N; ++i)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                carry = intrinsics::addcarry_u64(carry, data[i], o.data[i], &r.data[i]);
#else
                const std::uint64_t s = data[i] + o.data[i] + carry;
                carry = static_cast<unsigned char>(
                    (s < data[i]) || (carry != 0 && s == data[i]) ? 1 : 0);
                r.data[i] = s;
#endif
            }
            return r;
        }

        constexpr fixed_int_t operator-(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};
            unsigned char borrow{0};
            for (std::size_t i{0}; i < N; ++i)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                borrow = intrinsics::subborrow_u64(borrow, data[i], o.data[i], &r.data[i]);
#else
                const std::uint64_t d = data[i] - o.data[i] - borrow;
                borrow = static_cast<unsigned char>(
                    (data[i] < o.data[i]) || (borrow != 0 && data[i] == o.data[i]) ? 1 : 0);
                r.data[i] = d;
#endif
            }
            return r;
        }

        constexpr fixed_int_t operator-() const noexcept
        {
            return ~(*this) + one();
        }

        constexpr fixed_int_t &operator+=(const fixed_int_t &o) noexcept
        {
            unsigned char carry{0};
            for (std::size_t i{0}; i < N; ++i)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                carry = intrinsics::addcarry_u64(carry, data[i], o.data[i], &data[i]);
#else
                const std::uint64_t a = data[i];
                const std::uint64_t s = a + o.data[i] + carry;
                carry = static_cast<unsigned char>((s < a) || (carry != 0 && s == a) ? 1 : 0);
                data[i] = s;
#endif
            }
            return *this;
        }

        constexpr fixed_int_t &operator-=(const fixed_int_t &o) noexcept
        {
            unsigned char borrow{0};
            for (std::size_t i{0}; i < N; ++i)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                borrow = intrinsics::subborrow_u64(borrow, data[i], o.data[i], &data[i]);
#else
                const std::uint64_t a = data[i];
                const std::uint64_t d = a - o.data[i] - borrow;
                borrow = static_cast<unsigned char>((a < o.data[i]) || (borrow != 0 && a == o.data[i]) ? 1 : 0);
                data[i] = d;
#endif
            }
            return *this;
        }

        constexpr fixed_int_t &operator++() noexcept
        {
            *this += one();
            return *this;
        }

        constexpr fixed_int_t operator++(int) noexcept
        {
            fixed_int_t tmp{*this};
            ++(*this);
            return tmp;
        }

        constexpr fixed_int_t &operator--() noexcept
        {
            *this -= one();
            return *this;
        }

        constexpr fixed_int_t operator--(int) noexcept
        {
            fixed_int_t tmp{*this};
            --(*this);
            return tmp;
        }

        // =========================================================================
        // Arithmetic — multiplication (mod 2^(64N))
        //
        // N=2 fast path:
        //   GCC/Clang/ICX (has __uint128_t): single __uint128_t multiply (constexpr-safe)
        //   MSVC x64 (no __uint128_t):       _umul128 + two 64-bit muls (runtime only)
        // N=4/8 Karatsuba (runtime only): T(N)=3·T(N/2)+O(N), T(2)=3 umul128
        //   kmul_full<N/2> for the full lower product; half-width operator* for middle terms
        //   N=4: 9 umul128+0 (vs 10 schoolbook); N=8: 19 umul128+8 muls (vs 36)
        // Fallback: schoolbook O(N^2) for N∉{2,4,8} or constexpr on MSVC
        // =========================================================================

        constexpr fixed_int_t operator*(const fixed_int_t &o) const noexcept
        {
            fixed_int_t r{};

#ifdef __SIZEOF_INT128__
            if constexpr (N == 2)
            {
                const std::uint64_t a0 = data[0], a1 = data[1];
                const std::uint64_t b0 = o.data[0], b1 = o.data[1];
                const unsigned __int128 p = static_cast<unsigned __int128>(a0) * b0;
                r.data[0] = static_cast<std::uint64_t>(p);
                r.data[1] = static_cast<std::uint64_t>(p >> 64) + a0 * b1 + a1 * b0;
                return r;
            }
#elif defined(_MSC_VER) && defined(_M_X64)
            if constexpr (N == 2)
            {
                if (!std::is_constant_evaluated())
                {
                    // 128x128 -> 128 (low): need only 3 of the 4 partial products
                    // a0*b0: full 128-bit product (both halves used)
                    // a0*b1, a1*b0: only low 64 bits (upper half -> result[2], discarded)
                    // a1*b1: entirely discarded (-> result[2+], mod 2^128)
                    std::uint64_t hi00;
                    const std::uint64_t lo00 = _umul128(data[0], o.data[0], &hi00);
                    r.data[0] = lo00;
                    r.data[1] = hi00 + data[0] * o.data[1] + data[1] * o.data[0];
                    return r;
                }
            }
#endif

            // N=4/8 Karatsuba: full lower product via kmul_full<N/2>,
            // middle terms via half-width operator* (recurses automatically for N=8).
            if constexpr (N == 4 || N == 8)
            {
                if (!std::is_constant_evaluated())
                {
                    constexpr std::size_t HH = N / 2;
                    using half_t = uint_fixed_t<HH>;

                    half_t a_lo{}, a_hi{}, b_lo{}, b_hi{};
                    for (std::size_t i = 0; i < HH; ++i)
                    {
                        a_lo.data[i] = data[i];       a_hi.data[i] = data[HH + i];
                        b_lo.data[i] = o.data[i];     b_hi.data[i] = o.data[HH + i];
                    }

                    const auto z0    = kmul_full<HH>(a_lo.data, b_lo.data);
                    const half_t mid = a_lo * b_hi + a_hi * b_lo;

                    for (std::size_t i = 0; i < N; ++i) r.data[i] = z0[i];
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < HH; ++i)
                        c = add_limb_carry(r.data[HH + i], mid.data[i], c);
                    return r;
                }
            }

            // General schoolbook O(N^2) — used for N∉{2,4,8} or constexpr on MSVC
            for (std::size_t i{0}; i < N; ++i)
            {
                for (std::size_t j{0}; i + j < N; ++j)
                {
                    std::uint64_t hi{0};
#if __has_include("intrinsics/arithmetic_operations.hpp")
                    const std::uint64_t lo = intrinsics::umul128(data[i], o.data[j], &hi);
#else
                    const std::uint64_t a_lo = data[i] & 0xFFFFFFFFULL;
                    const std::uint64_t a_hi = data[i] >> 32;
                    const std::uint64_t b_lo = o.data[j] & 0xFFFFFFFFULL;
                    const std::uint64_t b_hi = o.data[j] >> 32;
                    const std::uint64_t p0 = a_lo * b_lo;
                    const std::uint64_t p1 = a_lo * b_hi;
                    const std::uint64_t p2 = a_hi * b_lo;
                    const std::uint64_t p3 = a_hi * b_hi;
                    const std::uint64_t mid =
                        (p0 >> 32) + (p1 & 0xFFFFFFFFULL) + (p2 & 0xFFFFFFFFULL);
                    const std::uint64_t lo = (p0 & 0xFFFFFFFFULL) | (mid << 32);
                    hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
#endif
                    unsigned char c = add_limb(r.data[i + j], lo);
                    const std::size_t next = i + j + 1;
                    if (next < N)
                    {
                        c = add_limb_carry(r.data[next], hi, c);
                        for (std::size_t k{next + 1}; k < N && c; ++k)
                            c = add_limb(r.data[k], std::uint64_t{c});
                    }
                }
            }
            return r;
        }

        constexpr fixed_int_t &operator*=(const fixed_int_t &o) noexcept
        {
#ifdef __SIZEOF_INT128__
            if constexpr (N == 2)
            {
                // Schoolbook 128x128->128 with explicit 64-bit variables.
                // GCC/Clang generate better register allocation than a128*b128.
                const std::uint64_t a0 = data[0], a1 = data[1];
                const std::uint64_t b0 = o.data[0], b1 = o.data[1];
                const unsigned __int128 p = static_cast<unsigned __int128>(a0) * b0;
                data[0] = static_cast<std::uint64_t>(p);
                data[1] = static_cast<std::uint64_t>(p >> 64) + a0 * b1 + a1 * b0;
                return *this;
            }
#elif defined(_MSC_VER) && defined(_M_X64)
            if constexpr (N == 2)
            {
                if (!std::is_constant_evaluated())
                {
                    const std::uint64_t a0 = data[0], a1 = data[1];
                    std::uint64_t hi00;
                    const std::uint64_t lo00 = _umul128(a0, o.data[0], &hi00);
                    data[0] = lo00;
                    data[1] = hi00 + a0 * o.data[1] + a1 * o.data[0];
                    return *this;
                }
            }
#endif
            *this = *this * o;
            return *this;
        }

        // =========================================================================
        // Arithmetic — division and modulo
        //
        // Unsigned N=2 fast paths (in order):
        //   [0]  a < b                  → {0, a}  (early-out, all N)
        //   [1]  __uint128_t available  → 128/64 or 128/128 via hardware  (GCC/Clang/ICX)
        //   [2]  MSVC x64               → 128/64 via _udiv128; 128/128 → binary long div
        //   [3]  fallback               → binary long division O(64N^2)
        //
        // Signed: reduce to unsigned, apply sign rules.
        // Throws std::domain_error on division by zero.
        // =========================================================================

        static std::pair<fixed_int_t, fixed_int_t>
        divmod(const fixed_int_t &a, const fixed_int_t &b)
        {
            if (b.is_zero())
                throw std::domain_error("fixed_int_t::divmod: division by zero");

            if constexpr (!is_signed)
            {
                if (a < b)
                    return {fixed_int_t{}, a};

// Intel ICX on Windows (MSVC ABI) defines __SIZEOF_INT128__ but ships without
// __udivti3/__umodti3 in its runtime — those calls would fail to link.
// We fall through to the _udiv128 path (same as pure MSVC) instead.
#if defined(__SIZEOF_INT128__) && \
    !(defined(__INTEL_LLVM_COMPILER) && (defined(_WIN32) || defined(_WIN64)))
                if constexpr (N == 2)
                {
                    const unsigned __int128 u =
                        (static_cast<unsigned __int128>(a.data[1]) << 64) | a.data[0];

                    if (b.data[1] == 0)
                    {
                        // Fast path: 64-bit divisor — single 128/64 hardware division
                        const std::uint64_t d = b.data[0];
                        const unsigned __int128 q128 = u / d;
                        const std::uint64_t rem = static_cast<std::uint64_t>(u % d);
                        fixed_int_t q{};
                        q.data[0] = static_cast<std::uint64_t>(q128);
                        q.data[1] = static_cast<std::uint64_t>(q128 >> 64);
                        return {q, fixed_int_t{rem}};
                    }

                    // General 128/128: compiler emits __udivti3 (Knuth D internally)
                    const unsigned __int128 v =
                        (static_cast<unsigned __int128>(b.data[1]) << 64) | b.data[0];
                    const unsigned __int128 q128 = u / v;
                    const unsigned __int128 r128 = u % v;
                    fixed_int_t q{}, r{};
                    q.data[0] = static_cast<std::uint64_t>(q128);
                    q.data[1] = static_cast<std::uint64_t>(q128 >> 64);
                    r.data[0] = static_cast<std::uint64_t>(r128);
                    r.data[1] = static_cast<std::uint64_t>(r128 >> 64);
                    return {q, r};
                }
#elif defined(__INTEL_LLVM_COMPILER) && (defined(_WIN32) || defined(_WIN64)) && defined(_M_X64)
                // ICX Windows: no __udivti3 runtime, no _udiv128 intrinsic.
                // ICX uses the Clang/LLVM frontend which supports GCC-style inline asm.
                if constexpr (N == 2)
                {
                    if (b.data[1] == 0)
                    {
                        const std::uint64_t d = b.data[0];
                        const std::uint64_t q_hi = a.data[1] / d;
                        const std::uint64_t r_hi = a.data[1] % d;
                        std::uint64_t q_lo, rem;
                        __asm__("divq %4"
                                : "=a"(q_lo), "=d"(rem)
                                : "0"(a.data[0]), "1"(r_hi), "rm"(d));
                        fixed_int_t q{};
                        q.data[0] = q_lo;
                        q.data[1] = q_hi;
                        return {q, fixed_int_t{rem}};
                    }
                    // 128/128: fall through to binary long division
                }
#elif defined(_MSC_VER) && defined(_M_X64)
                if constexpr (N == 2)
                {
                    if (b.data[1] == 0)
                    {
                        // MSVC x64: two-step 128/64 via _udiv128
                        const std::uint64_t d = b.data[0];
                        const std::uint64_t q_hi = a.data[1] / d;
                        const std::uint64_t r_hi = a.data[1] % d;
                        std::uint64_t rem;
                        const std::uint64_t q_lo = _udiv128(r_hi, a.data[0], d, &rem);
                        fixed_int_t q{};
                        q.data[0] = q_lo;
                        q.data[1] = q_hi;
                        return {q, fixed_int_t{rem}};
                    }
                    // MSVC 128/128: fall through to binary long division below
                }
#endif

                // ─────────────────────────────────────────────────────────────────
                // Single-limb divisor fast path — b fits entirely in data[0].
                //
                // Replaces O(64N²) binary long div with O(N) hardware DIV instructions:
                // iterate from MSL to LSL, each step divides (rem:a[i]) by d where
                // rem < d is a loop invariant (guaranteed: each remainder < divisor).
                //
                // With rem < d, libgcc's __udivti3 / _udiv128 emit a SINGLE divq.
                // Total cost: N divq instructions instead of 64N² bit-loop iterations.
                //
                // (For N=2 on GCC/Clang, the __uint128_t block above already handled
                //  the b.data[1]==0 case and returned; this is the critical path for N≥3.
                //  For N=2 on MSVC/ICX-Win, the check below finds single_limb=false
                //  since the 128/128 fallback only reaches here when b.data[1]!=0.)
                // ─────────────────────────────────────────────────────────────────
                {
                    bool single_limb_b = true;
                    for (std::size_t k = 1; k < N; ++k)
                        if (b.data[k] != 0) { single_limb_b = false; break; }

                    if (single_limb_b)
                    {
                        const std::uint64_t d = b.data[0];
                        fixed_int_t q{};
                        std::uint64_t rem = 0;
                        for (std::size_t i = N; i-- > 0;)
                        {
#if defined(__SIZEOF_INT128__) && \
    !(defined(__INTEL_LLVM_COMPILER) && (defined(_WIN32) || defined(_WIN64)))
                            // __udivti3 detects rem < d and emits a single divq
                            const unsigned __int128 cur =
                                (static_cast<unsigned __int128>(rem) << 64) | a.data[i];
                            q.data[i] = static_cast<std::uint64_t>(cur / d);
                            rem       = static_cast<std::uint64_t>(cur % d);
#elif defined(__INTEL_LLVM_COMPILER) && \
      (defined(_WIN32) || defined(_WIN64)) && defined(_M_X64)
                            // ICX Windows: GCC-style inline asm (Clang/LLVM frontend)
                            __asm__("divq %4"
                                    : "=a"(q.data[i]), "=d"(rem)
                                    : "0"(a.data[i]), "1"(rem), "rm"(d));
#elif defined(_MSC_VER) && defined(_M_X64)
                            // MSVC x64: rem < d invariant → single DIV instruction
                            q.data[i] = _udiv128(rem, a.data[i], d, &rem);
#else
                            // Portable fallback: rem==0 fast case + bit-by-bit otherwise
                            if (rem == 0)
                            {
                                q.data[i] = a.data[i] / d;
                                rem       = a.data[i] % d;
                            }
                            else
                            {
                                std::uint64_t qi = 0;
                                std::uint64_t n  = a.data[i];
                                for (int bit = 63; bit >= 0; --bit)
                                {
                                    rem = (rem << 1) | (n >> 63);
                                    n <<= 1;
                                    if (rem >= d) { rem -= d; qi |= std::uint64_t{1} << bit; }
                                }
                                q.data[i] = qi;
                            }
#endif
                        }
                        return {q, fixed_int_t{rem}};
                    }
                }

                // ─────────────────────────────────────────────────────────────────
                // Knuth Algorithm D — general N-limb ÷ M-limb (M ≥ 2)
                // TAOCP Vol. 2 §4.3.1. Base B = 2^64.
                // Reached only when b has ≥ 2 significant limbs.
                // ─────────────────────────────────────────────────────────────────
                {
                    // Count significant limbs of b (guaranteed ≥ 2 here)
                    std::size_t n = N;
                    while (b.data[n - 1] == 0) --n;

                    const std::size_t m_quot = N - n; // quotient digits: q.data[0..m_quot]

                    // D1. Normalize: find shift s so that v[n-1] has its MSB set.
#if __has_include("intrinsics/bit_operations.hpp")
                    const int s = intrinsics::clz64(b.data[n - 1]);
#else
                    int s = 0;
                    {
                        std::uint64_t tmp = b.data[n - 1];
                        while ((tmp & (std::uint64_t{1} << 63)) == 0) { ++s; tmp <<= 1; }
                    }
#endif

                    std::array<std::uint64_t, N>     v{}; // normalized divisor  [0..n-1]
                    std::array<std::uint64_t, N + 1> u{}; // normalized dividend [0..N]

                    if (s == 0)
                    {
                        for (std::size_t i = 0; i < n; ++i) v[i] = b.data[i];
                        for (std::size_t i = 0; i < N; ++i) u[i] = a.data[i];
                        // u[N] stays 0
                    }
                    else
                    {
                        for (std::size_t i = n - 1; i > 0; --i)
                            v[i] = (b.data[i] << s) | (b.data[i - 1] >> (64 - s));
                        v[0] = b.data[0] << s;
                        u[N] = a.data[N - 1] >> (64 - s);
                        for (std::size_t i = N - 1; i > 0; --i)
                            u[i] = (a.data[i] << s) | (a.data[i - 1] >> (64 - s));
                        u[0] = a.data[0] << s;
                    }

                    const std::uint64_t v1 = v[n - 1];
                    const std::uint64_t v2 = v[n - 2]; // safe: n ≥ 2

                    fixed_int_t q{};

                    // D2–D7. Main loop: j = m_quot down to 0
                    for (std::size_t j = m_quot + 1; j-- > 0;)
                    {
                        const std::uint64_t u0 = u[j + n];       // top window limb
                        const std::uint64_t u1 = u[j + n - 1];   // next limb
                        const std::uint64_t u2 = u[j + n - 2];   // limb below (n≥2,j≥0)

                        // D3. Estimate trial quotient q̂
                        std::uint64_t q_hat, r_hat = 0;
                        bool skip_refine = false;

                        if (u0 > v1)
                        {
                            // r̂ ≥ B for sure — skip refinement entirely
                            q_hat = ~std::uint64_t{0};
                            skip_refine = true;
                        }
                        else if (u0 == v1)
                        {
                            q_hat = ~std::uint64_t{0};
                            r_hat = u1 + v1;
                            skip_refine = (r_hat < u1); // overflow ⟹ r̂ ≥ B
                        }
                        else
                        {
                            // 0 ≤ u0 < v1: exact 128/64 hardware division
#if defined(__SIZEOF_INT128__) && \
    !(defined(__INTEL_LLVM_COMPILER) && (defined(_WIN32) || defined(_WIN64)))
                            {
                                const unsigned __int128 ud =
                                    (static_cast<unsigned __int128>(u0) << 64) | u1;
                                q_hat = static_cast<std::uint64_t>(ud / v1);
                                r_hat = static_cast<std::uint64_t>(ud % v1);
                            }
#elif defined(__INTEL_LLVM_COMPILER) && \
      (defined(_WIN32) || defined(_WIN64)) && defined(_M_X64)
                            __asm__("divq %4"
                                    : "=a"(q_hat), "=d"(r_hat)
                                    : "0"(u1), "1"(u0), "rm"(v1));
#elif defined(_MSC_VER) && defined(_M_X64)
                            q_hat = _udiv128(u0, u1, v1, &r_hat);
#else
                            // Portable bit-by-bit 128/64 (v1 ≥ 2^63 after normalization)
                            {
                                std::uint64_t rem = u0;
                                q_hat = 0;
                                for (int bit = 63; bit >= 0; --bit)
                                {
                                    const bool ovf = (rem >> 63) != 0;
                                    rem = (rem << 1) | ((u1 >> bit) & 1);
                                    if (ovf || rem >= v1)
                                    {
                                        rem -= v1;
                                        q_hat |= std::uint64_t{1} << bit;
                                    }
                                }
                                r_hat = rem;
                            }
#endif
                        }

                        // D3. Refinement: while q̂·v2 > r̂·B + u2, do q̂--, r̂ += v1
                        if (!skip_refine)
                        {
                            while (true)
                            {
#if defined(__SIZEOF_INT128__)
                                const unsigned __int128 lhs =
                                    static_cast<unsigned __int128>(q_hat) * v2;
                                const unsigned __int128 rhs =
                                    (static_cast<unsigned __int128>(r_hat) << 64) | u2;
                                if (lhs <= rhs) break;
#elif defined(_MSC_VER) && defined(_M_X64)
                                std::uint64_t lhs_hi;
                                const std::uint64_t lhs_lo = _umul128(q_hat, v2, &lhs_hi);
                                if (lhs_hi < r_hat ||
                                    (lhs_hi == r_hat && lhs_lo <= u2)) break;
#else
                                break; // conservative: D5 add-back corrects any error
#endif
                                --q_hat;
                                const std::uint64_t r_new = r_hat + v1;
                                if (r_new < r_hat) break; // r̂ overflowed B
                                r_hat = r_new;
                            }
                        }

                        // D4. Multiply-subtract: u[j..j+n] -= q̂ × v[0..n-1]
                        std::uint64_t borrow = 0;
                        for (std::size_t i = 0; i < n; ++i)
                        {
#if defined(__SIZEOF_INT128__)
                            const unsigned __int128 prod =
                                static_cast<unsigned __int128>(q_hat) * v[i] + borrow;
                            const std::uint64_t sub = static_cast<std::uint64_t>(prod);
                            borrow = static_cast<std::uint64_t>(prod >> 64);
                            if (u[j + i] < sub) ++borrow;
                            u[j + i] -= sub;
#elif defined(_MSC_VER) && defined(_M_X64)
                            {
                                std::uint64_t prod_hi;
                                const std::uint64_t prod_lo = _umul128(q_hat, v[i], &prod_hi);
                                const std::uint64_t sub     = prod_lo + borrow;
                                borrow = prod_hi + (sub < prod_lo ? 1U : 0U);
                                if (u[j + i] < sub) ++borrow;
                                u[j + i] -= sub;
                            }
#else
                            // Portable 32×32→64 school multiply
                            {
                                const std::uint64_t ql = q_hat & 0xFFFFFFFFU;
                                const std::uint64_t qh = q_hat >> 32;
                                const std::uint64_t vl = v[i]   & 0xFFFFFFFFU;
                                const std::uint64_t vh = v[i]   >> 32;
                                const std::uint64_t p0 = ql * vl;
                                const std::uint64_t p1 = ql * vh + qh * vl;
                                const std::uint64_t p2 = qh * vh;
                                const std::uint64_t mid_lo  = p1 << 32;
                                const std::uint64_t mid_ov  = (p1 < ql * vh) ? std::uint64_t{1} << 32 : 0;
                                const std::uint64_t prod_lo = p0 + mid_lo;
                                const std::uint64_t lo_ov   = prod_lo < p0 ? 1U : 0U;
                                const std::uint64_t prod_hi = p2 + (p1 >> 32) + mid_ov + lo_ov;
                                const std::uint64_t sub     = prod_lo + borrow;
                                borrow = prod_hi + (sub < prod_lo ? 1U : 0U);
                                if (u[j + i] < sub) ++borrow;
                                u[j + i] -= sub;
                            }
#endif
                        }

                        // D5. Add-back if underflow (happens with prob ~2/B per step)
                        if (u[j + n] < borrow)
                        {
                            u[j + n] -= borrow; // intentional wrap
                            std::uint64_t carry = 0;
                            for (std::size_t i = 0; i < n; ++i)
                            {
                                const std::uint64_t s1 = u[j + i] + v[i];
                                const std::uint64_t s2 = s1 + carry;
                                carry = (s1 < u[j + i]) + (s2 < s1);
                                u[j + i] = s2;
                            }
                            u[j + n] += carry;
                            --q_hat;
                        }
                        else
                        {
                            u[j + n] -= borrow;
                        }

                        // D6. Store quotient digit
                        q.data[j] = q_hat;
                    }

                    // D8. Unnormalize: remainder is u[0..n-1] right-shifted by s
                    fixed_int_t r{};
                    if (s == 0)
                    {
                        for (std::size_t i = 0; i < n; ++i) r.data[i] = u[i];
                    }
                    else
                    {
                        for (std::size_t i = 0; i < n - 1; ++i)
                            r.data[i] = (u[i] >> s) | (u[i + 1] << (64 - s));
                        r.data[n - 1] = u[n - 1] >> s;
                    }

                    return {q, r};
                }

                // Fallback: binary long division O(64N^2)
                // (dead code when Knuth D is compiled — kept as safety net)
                fixed_int_t q{};
                fixed_int_t r{};

                for (std::size_t i{64U * N}; i-- > 0;)
                {
                    r <<= 1;
                    r.data[0] |= (a.data[i / 64U] >> (i % 64U)) & std::uint64_t{1};
                    if (!(r < b))
                    {
                        r -= b;
                        q.data[i / 64U] |= std::uint64_t{1} << (i % 64U);
                    }
                }
                return {q, r};
            }
            else
            {
                const bool a_neg = a.is_negative();
                const bool b_neg = b.is_negative();
                using U = uint_fixed_t<N>;
                const U ua = a_neg ? U{-a} : U{a};
                const U ub = b_neg ? U{-b} : U{b};
                const auto [uq, ur] = U::divmod(ua, ub);
                const bool q_neg = (a_neg != b_neg);
                fixed_int_t q{q_neg ? -fixed_int_t{uq} : fixed_int_t{uq}};
                fixed_int_t r{a_neg ? -fixed_int_t{ur} : fixed_int_t{ur}};
                return {q, r};
            }
        }

        fixed_int_t operator/(const fixed_int_t &o) const
        {
            return divmod(*this, o).first;
        }

        fixed_int_t operator%(const fixed_int_t &o) const
        {
            return divmod(*this, o).second;
        }

        fixed_int_t &operator/=(const fixed_int_t &o)
        {
            *this = *this / o;
            return *this;
        }

        fixed_int_t &operator%=(const fixed_int_t &o)
        {
            *this = *this % o;
            return *this;
        }

        // =========================================================================
        // Compound assignments — mixed integral types
        // =========================================================================

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator+=(T v) noexcept { *this += fixed_int_t{v}; return *this; }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator-=(T v) noexcept { *this -= fixed_int_t{v}; return *this; }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator*=(T v) noexcept { *this *= fixed_int_t{v}; return *this; }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        fixed_int_t &operator/=(T v) { *this /= fixed_int_t{v}; return *this; }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        fixed_int_t &operator%=(T v) { *this %= fixed_int_t{v}; return *this; }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator&=(T v) noexcept { *this &= fixed_int_t{v}; return *this; }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator|=(T v) noexcept { *this |= fixed_int_t{v}; return *this; }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr fixed_int_t &operator^=(T v) noexcept { *this ^= fixed_int_t{v}; return *this; }

#ifdef __SIZEOF_INT128__
        constexpr fixed_int_t &operator+=(unsigned __int128 v) noexcept { *this += fixed_int_t{v}; return *this; }
        constexpr fixed_int_t &operator-=(unsigned __int128 v) noexcept { *this -= fixed_int_t{v}; return *this; }
        constexpr fixed_int_t &operator*=(unsigned __int128 v) noexcept { *this *= fixed_int_t{v}; return *this; }
        fixed_int_t &operator/=(unsigned __int128 v) { *this /= fixed_int_t{v}; return *this; }
        fixed_int_t &operator%=(unsigned __int128 v) { *this %= fixed_int_t{v}; return *this; }
        constexpr fixed_int_t &operator&=(unsigned __int128 v) noexcept { *this &= fixed_int_t{v}; return *this; }
        constexpr fixed_int_t &operator|=(unsigned __int128 v) noexcept { *this |= fixed_int_t{v}; return *this; }
        constexpr fixed_int_t &operator^=(unsigned __int128 v) noexcept { *this ^= fixed_int_t{v}; return *this; }

        constexpr fixed_int_t &operator+=(__int128 v) noexcept { *this += fixed_int_t{v}; return *this; }
        constexpr fixed_int_t &operator-=(__int128 v) noexcept { *this -= fixed_int_t{v}; return *this; }
        constexpr fixed_int_t &operator*=(__int128 v) noexcept { *this *= fixed_int_t{v}; return *this; }
        fixed_int_t &operator/=(__int128 v) { *this /= fixed_int_t{v}; return *this; }
        fixed_int_t &operator%=(__int128 v) { *this %= fixed_int_t{v}; return *this; }
        constexpr fixed_int_t &operator&=(__int128 v) noexcept { *this &= fixed_int_t{v}; return *this; }
        constexpr fixed_int_t &operator|=(__int128 v) noexcept { *this |= fixed_int_t{v}; return *this; }
        constexpr fixed_int_t &operator^=(__int128 v) noexcept { *this ^= fixed_int_t{v}; return *this; }
#endif

        // =========================================================================
        // Cross-N same-sign compound assignments — fixed_int_t<M, Sign, Form> (M != N)
        // Both operands promoted to the wider type; result truncated to N.
        // =========================================================================

        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator+=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} + fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator-=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} - fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator*=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} * fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        fixed_int_t &operator/=(const fixed_int_t<M, Sign, Form> &o)
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} / fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        fixed_int_t &operator%=(const fixed_int_t<M, Sign, Form> &o)
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} % fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator&=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} & fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator|=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} | fixed_int_t<R, Sign, Form>{o}};
        }
        template <std::size_t M, typename = std::enable_if_t<M != N>>
        constexpr fixed_int_t &operator^=(const fixed_int_t<M, Sign, Form> &o) noexcept
        {
            constexpr std::size_t R = N > M ? N : M;
            return *this = fixed_int_t{fixed_int_t<R, Sign, Form>{*this} ^ fixed_int_t<R, Sign, Form>{o}};
        }

        // =========================================================================
        // Mixed-sign compound assignments
        // C++ usual arithmetic conversions:
        //   uint op= int: N >= M -> uint wins; N < M -> int wins
        //   int  op= uint: N > M -> int wins; N <= M -> uint wins
        // =========================================================================

        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator+=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<(N >= M),
                    fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type,   representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} + R{o}};
            }
            else
            {
                using R = std::conditional_t<(N > M),
                    fixed_int_t<N, signedness::signed_type,   representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} + R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator-=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<(N >= M),
                    fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type,   representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} - R{o}};
            }
            else
            {
                using R = std::conditional_t<(N > M),
                    fixed_int_t<N, signedness::signed_type,   representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} - R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator*=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<(N >= M),
                    fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type,   representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} * R{o}};
            }
            else
            {
                using R = std::conditional_t<(N > M),
                    fixed_int_t<N, signedness::signed_type,   representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} * R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        fixed_int_t &operator/=(const fixed_int_t<M, S2, F2> &o)
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<(N >= M),
                    fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type,   representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} / R{o}};
            }
            else
            {
                using R = std::conditional_t<(N > M),
                    fixed_int_t<N, signedness::signed_type,   representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} / R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        fixed_int_t &operator%=(const fixed_int_t<M, S2, F2> &o)
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<(N >= M),
                    fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type,   representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} % R{o}};
            }
            else
            {
                using R = std::conditional_t<(N > M),
                    fixed_int_t<N, signedness::signed_type,   representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} % R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator&=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<(N >= M),
                    fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type,   representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} & R{o}};
            }
            else
            {
                using R = std::conditional_t<(N > M),
                    fixed_int_t<N, signedness::signed_type,   representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} & R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator|=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<(N >= M),
                    fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type,   representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} | R{o}};
            }
            else
            {
                using R = std::conditional_t<(N > M),
                    fixed_int_t<N, signedness::signed_type,   representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} | R{o}};
            }
        }
        template <std::size_t M, signedness S2, representation_form F2,
                  typename = std::enable_if_t<S2 != Sign>>
        constexpr fixed_int_t &operator^=(const fixed_int_t<M, S2, F2> &o) noexcept
        {
            if constexpr (!is_signed)
            {
                using R = std::conditional_t<(N >= M),
                    fixed_int_t<N, signedness::unsigned_type, representation_form::binnat>,
                    fixed_int_t<M, signedness::signed_type,   representation_form::twos_complement>>;
                return *this = fixed_int_t{R{*this} ^ R{o}};
            }
            else
            {
                using R = std::conditional_t<(N > M),
                    fixed_int_t<N, signedness::signed_type,   representation_form::twos_complement>,
                    fixed_int_t<M, signedness::unsigned_type, representation_form::binnat>>;
                return *this = fixed_int_t{R{*this} ^ R{o}};
            }
        }

        // =========================================================================
        // Utility
        // =========================================================================

        // Number of significant bits (floor(log2(x))+1), returns 0 for zero
        constexpr unsigned bit_width() const noexcept
        {
            for (std::size_t i{N}; i-- > 0;)
            {
                if (data[i] != 0)
                {
                    unsigned w{0};
                    std::uint64_t v = data[i];
                    while (v != 0)
                    {
                        v >>= 1;
                        ++w;
                    }
                    return static_cast<unsigned>(i * 64U) + w;
                }
            }
            return 0;
        }

        // Population count (number of set bits)
        constexpr unsigned popcount() const noexcept
        {
            unsigned total{0};
            for (const auto &limb : data)
            {
                std::uint64_t v = limb;
                while (v != 0)
                {
                    total += static_cast<unsigned>(v & 1U);
                    v >>= 1;
                }
            }
            return total;
        }

        // Count leading zeros (MSB end); returns 64*N for zero
        constexpr unsigned count_leading_zeros() const noexcept
        {
            return 64U * static_cast<unsigned>(N) - bit_width();
        }

        // Count trailing zeros (LSB end); returns 64*N for zero
        constexpr unsigned count_trailing_zeros() const noexcept
        {
            for (std::size_t i{0}; i < N; ++i)
            {
                if (data[i] != 0)
                {
#if __has_include("intrinsics/bit_operations.hpp")
                    return static_cast<unsigned>(i * 64U) + intrinsics::ctz64(data[i]);
#else
                    std::uint64_t v = data[i];
                    unsigned n{0};
                    while ((v & 1U) == 0) { v >>= 1; ++n; }
                    return static_cast<unsigned>(i * 64U) + n;
#endif
                }
            }
            return 64U * static_cast<unsigned>(N);
        }

        [[nodiscard]] constexpr bool is_power_of_two() const noexcept
        {
            return !is_zero() && (*this & (*this - one())).is_zero();
        }

        // Signed-only utilities (guarded by enable_if)
        template <bool S = is_signed, typename = std::enable_if_t<S>>
        constexpr fixed_int_t abs() const noexcept
        {
            return is_negative() ? -(*this) : *this;
        }

        template <bool S = is_signed, typename = std::enable_if_t<S>>
        [[nodiscard]] constexpr bool is_positive() const noexcept
        {
            return !is_zero() && !is_negative();
        }

        template <bool S = is_signed, typename = std::enable_if_t<S>>
        [[nodiscard]] constexpr int signum() const noexcept
        {
            if (is_zero()) return 0;
            return is_negative() ? -1 : 1;
        }

        // =========================================================================
        // String conversion — base 10
        // =========================================================================

        std::string to_string() const
        {
            if (is_zero())
                return "0";
            if constexpr (is_signed)
            {
                if (is_negative())
                    return "-" + uint_fixed_t<N>{-(*this)}.to_string();
            }
            // unsigned path: chunk-based base-10 (divides by 10^19 per iteration)
            constexpr std::size_t max_digits = N * 20 + 1;
            char buf[max_digits];
            int pos = static_cast<int>(max_digits);

            const uint_fixed_t<N> chunk_base{std::uint64_t{10000000000000000000ULL}};
            uint_fixed_t<N> tmp{*this};

            while (!tmp.is_zero())
            {
                const auto [q, r] = uint_fixed_t<N>::divmod(tmp, chunk_base);
                const std::uint64_t chunk = r.data[0];
                if (q.is_zero())
                    write_u64_digits(buf, pos, chunk);
                else
                    write_19_padded_digits(buf, pos, chunk);
                tmp = q;
            }

            return std::string(buf + pos, buf + max_digits);
        }

        static fixed_int_t from_string(const char *s)
        {
            if constexpr (is_signed)
            {
                if (!s || *s == '\0')
                    throw std::invalid_argument("fixed_int_t::from_string: empty string");
                if (*s == '-')
                {
                    if (*(s + 1) == '\0')
                        throw std::invalid_argument("fixed_int_t::from_string: only minus sign");
                    return -fixed_int_t{uint_fixed_t<N>::from_string(s + 1)};
                }
                return fixed_int_t{uint_fixed_t<N>::from_string(s)};
            }
            else
            {
                if (!s || *s == '\0')
                    throw std::invalid_argument("fixed_int_t::from_string: empty string");
                const fixed_int_t ten{std::uint64_t{10}};
                fixed_int_t result{};
                bool any{false};
                for (; *s != '\0'; ++s)
                {
                    const char c{*s};
                    if (c < '0' || c > '9')
                        throw std::invalid_argument("fixed_int_t::from_string: invalid character");
                    result = result * ten + fixed_int_t{static_cast<std::uint64_t>(c - '0')};
                    any = true;
                }
                if (!any)
                    throw std::invalid_argument("fixed_int_t::from_string: no digits");
                return result;
            }
        }

    private:
        // Add v to limb, return carry (0 or 1)
        static constexpr unsigned char add_limb(std::uint64_t &limb, std::uint64_t v) noexcept
        {
#if __has_include("intrinsics/arithmetic_operations.hpp")
            return intrinsics::addcarry_u64(0, limb, v, &limb);
#else
            const std::uint64_t old{limb};
            limb += v;
            return static_cast<unsigned char>(limb < old ? 1 : 0);
#endif
        }

        // Add v + carry_in to limb, return carry_out (0 or 1)
        static constexpr unsigned char add_limb_carry(std::uint64_t &limb, std::uint64_t v,
                                                      unsigned char c) noexcept
        {
#if __has_include("intrinsics/arithmetic_operations.hpp")
            return intrinsics::addcarry_u64(c, limb, v, &limb);
#else
            const std::uint64_t old{limb};
            limb += v + c;
            return static_cast<unsigned char>((limb < old || (c && limb == old)) ? 1 : 0);
#endif
        }

        // =========================================================================
        // Karatsuba full multiply: M×M → 2M limb product (unsigned, limb arrays).
        // Recurrence T(1)=1, T(M)=3·T(M/2)+O(M):  T(2)=3, T(4)=9 umul128 calls.
        // =========================================================================

        template <std::size_t M>
        [[nodiscard]] static std::array<std::uint64_t, 2 * M>
        kmul_full(const std::array<std::uint64_t, M> &a,
                  const std::array<std::uint64_t, M> &b) noexcept
        {
            std::array<std::uint64_t, 2 * M> r{};

            if constexpr (M == 1)
            {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                r[0] = intrinsics::umul128(a[0], b[0], &r[1]);
#else
                const std::uint64_t al = a[0] & 0xFFFF'FFFFull;
                const std::uint64_t ah = a[0] >> 32;
                const std::uint64_t bl = b[0] & 0xFFFF'FFFFull;
                const std::uint64_t bh = b[0] >> 32;
                const std::uint64_t p0 = al * bl, p1 = al * bh;
                const std::uint64_t p2 = ah * bl, p3 = ah * bh;
                const std::uint64_t mid =
                    (p0 >> 32) + (p1 & 0xFFFF'FFFFull) + (p2 & 0xFFFF'FFFFull);
                r[0] = (p0 & 0xFFFF'FFFFull) | (mid << 32);
                r[1] = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
#endif
                return r;
            }
            else
            {
                static_assert(M % 2 == 0, "kmul_full: M must be even");
                constexpr std::size_t HH = M / 2;

                // ── Split into low/high halves ────────────────────────────────────
                std::array<std::uint64_t, HH> a_lo{}, a_hi{}, b_lo{}, b_hi{};
                for (std::size_t i = 0; i < HH; ++i)
                {
                    a_lo[i] = a[i];    a_hi[i] = a[HH + i];
                    b_lo[i] = b[i];    b_hi[i] = b[HH + i];
                }

                // ── z0 = a_lo·b_lo,  z2 = a_hi·b_hi  (each 2HH limbs) ───────────
                const auto z0 = kmul_full<HH>(a_lo, b_lo);
                const auto z2 = kmul_full<HH>(a_hi, b_hi);

                // ── sum_a = a_lo + a_hi,  sum_b = b_lo + b_hi  (+carry ca, cb) ───
                std::array<std::uint64_t, HH> sum_a{}, sum_b{};
                unsigned char ca = 0, cb = 0;
                for (std::size_t i = 0; i < HH; ++i)
                {
                    sum_a[i] = a_hi[i];
                    ca = add_limb_carry(sum_a[i], a_lo[i], ca);
                    sum_b[i] = b_hi[i];
                    cb = add_limb_carry(sum_b[i], b_lo[i], cb);
                }

                // ── p = (sum_a + ca·B^HH) · (sum_b + cb·B^HH)  (2HH+1 limbs) ───
                // = sum_a·sum_b + ca·sum_b·B^HH + cb·sum_a·B^HH + ca·cb·B^(2HH)
                std::array<std::uint64_t, 2 * HH + 1> p{};
                {
                    const auto pp = kmul_full<HH>(sum_a, sum_b);
                    for (std::size_t i = 0; i < 2 * HH; ++i) p[i] = pp[i];
                }
                if (ca)
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < HH; ++i)
                        c = add_limb_carry(p[HH + i], sum_b[i], c);
                    p[2 * HH] += c;
                }
                if (cb)
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < HH; ++i)
                        c = add_limb_carry(p[HH + i], sum_a[i], c);
                    p[2 * HH] += c;
                }
                if (ca & cb) ++p[2 * HH];

                // ── z1 = p − z0 − z2  (guaranteed ≥ 0, fits in 2HH+1 limbs) ─────
                std::array<std::uint64_t, 2 * HH + 1> z1 = p;
                {
                    unsigned char borrow = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                    {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                        borrow = intrinsics::subborrow_u64(borrow, z1[i], z0[i], &z1[i]);
#else
                        const std::uint64_t av = z1[i];
                        z1[i] = av - z0[i] - borrow;
                        borrow = static_cast<unsigned char>(
                            (av < z0[i]) || (borrow && av == z0[i]) ? 1 : 0);
#endif
                    }
                    z1[2 * HH] -= borrow;
                }
                {
                    unsigned char borrow = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                    {
#if __has_include("intrinsics/arithmetic_operations.hpp")
                        borrow = intrinsics::subborrow_u64(borrow, z1[i], z2[i], &z1[i]);
#else
                        const std::uint64_t av = z1[i];
                        z1[i] = av - z2[i] - borrow;
                        borrow = static_cast<unsigned char>(
                            (av < z2[i]) || (borrow && av == z2[i]) ? 1 : 0);
#endif
                    }
                    z1[2 * HH] -= borrow;
                }

                // ── Combine: r = z0 + z1·B^HH + z2·B^(2HH) ─────────────────────
                for (std::size_t i = 0; i < 2 * HH; ++i) r[i] = z0[i];
                {
                    unsigned char c = 0;
                    std::size_t i = 0;
                    for (; i <= 2 * HH; ++i)
                        c = add_limb_carry(r[HH + i], z1[i], c);
                    for (; c && HH + i < 2 * M; ++i)
                        c = add_limb(r[HH + i], std::uint64_t{1});
                }
                {
                    unsigned char c = 0;
                    for (std::size_t i = 0; i < 2 * HH; ++i)
                        c = add_limb_carry(r[2 * HH + i], z2[i], c);
                }

                return r;
            }
        }

        // Two-digit lookup: "00", "01", ..., "99"
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

        // Write val as decimal digits into buf[..pos-1] (no zero-padding)
        static inline void write_u64_digits(char *buf, int &pos, std::uint64_t val) noexcept
        {
            while (val >= 100)
            {
                const std::uint64_t q = val / 100;
                const std::uint64_t r = val % 100;
                buf[--pos] = DIGIT_PAIRS_[r * 2 + 1];
                buf[--pos] = DIGIT_PAIRS_[r * 2];
                val = q;
            }
            if (val >= 10)
            {
                buf[--pos] = DIGIT_PAIRS_[val * 2 + 1];
                buf[--pos] = DIGIT_PAIRS_[val * 2];
            }
            else
            {
                buf[--pos] = static_cast<char>('0' + val);
            }
        }

        // Write exactly 19 decimal digits from val into buf[..pos-1] (zero-padded)
        // Precondition: val < 10^19
        static inline void write_19_padded_digits(char *buf, int &pos, std::uint64_t val) noexcept
        {
            for (int i{0}; i < 9; ++i)
            {
                const std::uint64_t q = val / 100;
                const std::uint64_t r = val % 100;
                buf[--pos] = DIGIT_PAIRS_[r * 2 + 1];
                buf[--pos] = DIGIT_PAIRS_[r * 2];
                val = q;
            }
            buf[--pos] = static_cast<char>('0' + val);
        }
    };

    // =============================================================================
    // Type aliases — unsigned
    // =============================================================================

    using uint64_fixed_t   = uint_fixed_t<1>;
    using uint128_fixed_t  = uint_fixed_t<2>;
    using uint256_fixed_t  = uint_fixed_t<4>;
    using uint512_fixed_t  = uint_fixed_t<8>;
    using uint1024_fixed_t = uint_fixed_t<16>;

    // =============================================================================
    // Type aliases — signed
    // =============================================================================

    using int64_fixed_t   = int_fixed_t<1>;
    using int128_fixed_t  = int_fixed_t<2>;
    using int256_fixed_t  = int_fixed_t<4>;
    using int512_fixed_t  = int_fixed_t<8>;
    using int1024_fixed_t = int_fixed_t<16>;

    template <std::size_t N>
    [[nodiscard]] constexpr uint_fixed_t<N> succ(const uint_fixed_t<N> &x) noexcept
    {
        return x + uint_fixed_t<N>::one();
    }

    template <std::size_t N>
    [[nodiscard]] constexpr uint_fixed_t<N> pred(const uint_fixed_t<N> &x) noexcept
    {
        return x - uint_fixed_t<N>::one();
    }

    template <std::size_t N>
    [[nodiscard]] constexpr int_fixed_t<N> succ(const int_fixed_t<N> &x) noexcept
    {
        return x + int_fixed_t<N>::one();
    }

    template <std::size_t N>
    [[nodiscard]] constexpr int_fixed_t<N> pred(const int_fixed_t<N> &x) noexcept
    {
        return x - int_fixed_t<N>::one();
    }

    // =========================================================================
    // detail — SFINAE helpers
    // =========================================================================

    namespace detail
    {
        template <typename T>
        using if_integral = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>;

        // C++ usual arithmetic conversions for int_fixed_t<N> op uint_fixed_t<M>:
        // N > M  -> int_fixed_t<N>  (signed has higher rank, unsigned zero-extends)
        // N <= M -> uint_fixed_t<M> (unsigned has >= rank, signed converts to unsigned)
        template <std::size_t N, std::size_t M>
        using mixed_iu_t = std::conditional_t<(N > M), int_fixed_t<N>, uint_fixed_t<M>>;
    }

    // =========================================================================
    // Free-function binary operators — uint_fixed_t<N> mixed with integral T
    // =========================================================================

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator+(const uint_fixed_t<N> &a, T b) noexcept { return a + uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator+(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} + b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator-(const uint_fixed_t<N> &a, T b) noexcept { return a - uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator-(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} - b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator*(const uint_fixed_t<N> &a, T b) noexcept { return a * uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator*(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} * b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    uint_fixed_t<N> operator/(const uint_fixed_t<N> &a, T b) { return a / uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    uint_fixed_t<N> operator/(T a, const uint_fixed_t<N> &b) { return uint_fixed_t<N>{a} / b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    uint_fixed_t<N> operator%(const uint_fixed_t<N> &a, T b) { return a % uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    uint_fixed_t<N> operator%(T a, const uint_fixed_t<N> &b) { return uint_fixed_t<N>{a} % b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator&(const uint_fixed_t<N> &a, T b) noexcept { return a & uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator&(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} & b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator|(const uint_fixed_t<N> &a, T b) noexcept { return a | uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator|(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} | b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator^(const uint_fixed_t<N> &a, T b) noexcept { return a ^ uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr uint_fixed_t<N> operator^(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} ^ b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator==(const uint_fixed_t<N> &a, T b) noexcept { return a == uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator==(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} == b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator!=(const uint_fixed_t<N> &a, T b) noexcept { return a != uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator!=(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} != b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<(const uint_fixed_t<N> &a, T b) noexcept { return a < uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} < b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<=(const uint_fixed_t<N> &a, T b) noexcept { return a <= uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<=(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} <= b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>(const uint_fixed_t<N> &a, T b) noexcept { return a > uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} > b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>=(const uint_fixed_t<N> &a, T b) noexcept { return a >= uint_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>=(T a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} >= b; }

#ifdef __SIZEOF_INT128__
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator+(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a + uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator+(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} + b; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator+(const uint_fixed_t<N> &a, __int128 b) noexcept { return a + uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator+(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} + b; }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator-(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a - uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator-(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} - b; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator-(const uint_fixed_t<N> &a, __int128 b) noexcept { return a - uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator-(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} - b; }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator*(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a * uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator*(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} * b; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator*(const uint_fixed_t<N> &a, __int128 b) noexcept { return a * uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator*(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} * b; }

    template <std::size_t N>
    uint_fixed_t<N> operator/(const uint_fixed_t<N> &a, unsigned __int128 b) { return a / uint_fixed_t<N>{b}; }
    template <std::size_t N>
    uint_fixed_t<N> operator/(unsigned __int128 a, const uint_fixed_t<N> &b) { return uint_fixed_t<N>{a} / b; }
    template <std::size_t N>
    uint_fixed_t<N> operator/(const uint_fixed_t<N> &a, __int128 b) { return a / uint_fixed_t<N>{b}; }
    template <std::size_t N>
    uint_fixed_t<N> operator/(__int128 a, const uint_fixed_t<N> &b) { return uint_fixed_t<N>{a} / b; }

    template <std::size_t N>
    uint_fixed_t<N> operator%(const uint_fixed_t<N> &a, unsigned __int128 b) { return a % uint_fixed_t<N>{b}; }
    template <std::size_t N>
    uint_fixed_t<N> operator%(unsigned __int128 a, const uint_fixed_t<N> &b) { return uint_fixed_t<N>{a} % b; }
    template <std::size_t N>
    uint_fixed_t<N> operator%(const uint_fixed_t<N> &a, __int128 b) { return a % uint_fixed_t<N>{b}; }
    template <std::size_t N>
    uint_fixed_t<N> operator%(__int128 a, const uint_fixed_t<N> &b) { return uint_fixed_t<N>{a} % b; }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator&(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a & uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator&(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} & b; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator&(const uint_fixed_t<N> &a, __int128 b) noexcept { return a & uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator&(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} & b; }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator|(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a | uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator|(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} | b; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator|(const uint_fixed_t<N> &a, __int128 b) noexcept { return a | uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator|(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} | b; }

    template <std::size_t N>
    constexpr uint_fixed_t<N> operator^(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a ^ uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator^(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} ^ b; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator^(const uint_fixed_t<N> &a, __int128 b) noexcept { return a ^ uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr uint_fixed_t<N> operator^(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} ^ b; }

    template <std::size_t N>
    constexpr bool operator==(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a == uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator==(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} == b; }
    template <std::size_t N>
    constexpr bool operator==(const uint_fixed_t<N> &a, __int128 b) noexcept { return a == uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator==(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} == b; }

    template <std::size_t N>
    constexpr bool operator!=(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a != uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator!=(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} != b; }
    template <std::size_t N>
    constexpr bool operator!=(const uint_fixed_t<N> &a, __int128 b) noexcept { return a != uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator!=(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} != b; }

    template <std::size_t N>
    constexpr bool operator<(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a < uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator<(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} < b; }
    template <std::size_t N>
    constexpr bool operator<(const uint_fixed_t<N> &a, __int128 b) noexcept { return a < uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator<(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} < b; }

    template <std::size_t N>
    constexpr bool operator<=(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a <= uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator<=(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} <= b; }
    template <std::size_t N>
    constexpr bool operator<=(const uint_fixed_t<N> &a, __int128 b) noexcept { return a <= uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator<=(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} <= b; }

    template <std::size_t N>
    constexpr bool operator>(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a > uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator>(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} > b; }
    template <std::size_t N>
    constexpr bool operator>(const uint_fixed_t<N> &a, __int128 b) noexcept { return a > uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator>(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} > b; }

    template <std::size_t N>
    constexpr bool operator>=(const uint_fixed_t<N> &a, unsigned __int128 b) noexcept { return a >= uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator>=(unsigned __int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} >= b; }
    template <std::size_t N>
    constexpr bool operator>=(const uint_fixed_t<N> &a, __int128 b) noexcept { return a >= uint_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator>=(__int128 a, const uint_fixed_t<N> &b) noexcept { return uint_fixed_t<N>{a} >= b; }
#endif

    // =========================================================================
    // Free-function binary operators — int_fixed_t<N> mixed with integral T
    // =========================================================================

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator+(const int_fixed_t<N> &a, T b) noexcept { return a + int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator+(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} + b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator-(const int_fixed_t<N> &a, T b) noexcept { return a - int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator-(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} - b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator*(const int_fixed_t<N> &a, T b) noexcept { return a * int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator*(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} * b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    int_fixed_t<N> operator/(const int_fixed_t<N> &a, T b) { return a / int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    int_fixed_t<N> operator/(T a, const int_fixed_t<N> &b) { return int_fixed_t<N>{a} / b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    int_fixed_t<N> operator%(const int_fixed_t<N> &a, T b) { return a % int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    int_fixed_t<N> operator%(T a, const int_fixed_t<N> &b) { return int_fixed_t<N>{a} % b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator&(const int_fixed_t<N> &a, T b) noexcept { return a & int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator&(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} & b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator|(const int_fixed_t<N> &a, T b) noexcept { return a | int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator|(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} | b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator^(const int_fixed_t<N> &a, T b) noexcept { return a ^ int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr int_fixed_t<N> operator^(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} ^ b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator==(const int_fixed_t<N> &a, T b) noexcept { return a == int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator==(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} == b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator!=(const int_fixed_t<N> &a, T b) noexcept { return a != int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator!=(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} != b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<(const int_fixed_t<N> &a, T b) noexcept { return a < int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} < b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<=(const int_fixed_t<N> &a, T b) noexcept { return a <= int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator<=(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} <= b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>(const int_fixed_t<N> &a, T b) noexcept { return a > int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} > b; }

    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>=(const int_fixed_t<N> &a, T b) noexcept { return a >= int_fixed_t<N>{b}; }
    template <std::size_t N, typename T, typename = detail::if_integral<T>>
    constexpr bool operator>=(T a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} >= b; }

#ifdef __SIZEOF_INT128__
    template <std::size_t N>
    constexpr int_fixed_t<N> operator+(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a + int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator+(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} + b; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator+(const int_fixed_t<N> &a, __int128 b) noexcept { return a + int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator+(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} + b; }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator-(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a - int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator-(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} - b; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator-(const int_fixed_t<N> &a, __int128 b) noexcept { return a - int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator-(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} - b; }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator*(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a * int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator*(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} * b; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator*(const int_fixed_t<N> &a, __int128 b) noexcept { return a * int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator*(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} * b; }

    template <std::size_t N>
    int_fixed_t<N> operator/(const int_fixed_t<N> &a, unsigned __int128 b) { return a / int_fixed_t<N>{b}; }
    template <std::size_t N>
    int_fixed_t<N> operator/(unsigned __int128 a, const int_fixed_t<N> &b) { return int_fixed_t<N>{a} / b; }
    template <std::size_t N>
    int_fixed_t<N> operator/(const int_fixed_t<N> &a, __int128 b) { return a / int_fixed_t<N>{b}; }
    template <std::size_t N>
    int_fixed_t<N> operator/(__int128 a, const int_fixed_t<N> &b) { return int_fixed_t<N>{a} / b; }

    template <std::size_t N>
    int_fixed_t<N> operator%(const int_fixed_t<N> &a, unsigned __int128 b) { return a % int_fixed_t<N>{b}; }
    template <std::size_t N>
    int_fixed_t<N> operator%(unsigned __int128 a, const int_fixed_t<N> &b) { return int_fixed_t<N>{a} % b; }
    template <std::size_t N>
    int_fixed_t<N> operator%(const int_fixed_t<N> &a, __int128 b) { return a % int_fixed_t<N>{b}; }
    template <std::size_t N>
    int_fixed_t<N> operator%(__int128 a, const int_fixed_t<N> &b) { return int_fixed_t<N>{a} % b; }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator&(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a & int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator&(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} & b; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator&(const int_fixed_t<N> &a, __int128 b) noexcept { return a & int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator&(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} & b; }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator|(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a | int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator|(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} | b; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator|(const int_fixed_t<N> &a, __int128 b) noexcept { return a | int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator|(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} | b; }

    template <std::size_t N>
    constexpr int_fixed_t<N> operator^(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a ^ int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator^(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} ^ b; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator^(const int_fixed_t<N> &a, __int128 b) noexcept { return a ^ int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr int_fixed_t<N> operator^(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} ^ b; }

    template <std::size_t N>
    constexpr bool operator==(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a == int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator==(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} == b; }
    template <std::size_t N>
    constexpr bool operator==(const int_fixed_t<N> &a, __int128 b) noexcept { return a == int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator==(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} == b; }

    template <std::size_t N>
    constexpr bool operator!=(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a != int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator!=(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} != b; }
    template <std::size_t N>
    constexpr bool operator!=(const int_fixed_t<N> &a, __int128 b) noexcept { return a != int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator!=(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} != b; }

    template <std::size_t N>
    constexpr bool operator<(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a < int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator<(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} < b; }
    template <std::size_t N>
    constexpr bool operator<(const int_fixed_t<N> &a, __int128 b) noexcept { return a < int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator<(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} < b; }

    template <std::size_t N>
    constexpr bool operator<=(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a <= int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator<=(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} <= b; }
    template <std::size_t N>
    constexpr bool operator<=(const int_fixed_t<N> &a, __int128 b) noexcept { return a <= int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator<=(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} <= b; }

    template <std::size_t N>
    constexpr bool operator>(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a > int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator>(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} > b; }
    template <std::size_t N>
    constexpr bool operator>(const int_fixed_t<N> &a, __int128 b) noexcept { return a > int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator>(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} > b; }

    template <std::size_t N>
    constexpr bool operator>=(const int_fixed_t<N> &a, unsigned __int128 b) noexcept { return a >= int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator>=(unsigned __int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} >= b; }
    template <std::size_t N>
    constexpr bool operator>=(const int_fixed_t<N> &a, __int128 b) noexcept { return a >= int_fixed_t<N>{b}; }
    template <std::size_t N>
    constexpr bool operator>=(__int128 a, const int_fixed_t<N> &b) noexcept { return int_fixed_t<N>{a} >= b; }
#endif

    // =========================================================================
    // Cross-N binary operators — uint_fixed_t<N> op uint_fixed_t<M> (N != M)
    // =========================================================================

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator+(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} + uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator-(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} - uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator*(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} * uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    uint_fixed_t<(N > M ? N : M)> operator/(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b)
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} / uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    uint_fixed_t<(N > M ? N : M)> operator%(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b)
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} % uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator&(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} & uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator|(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} | uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr uint_fixed_t<(N > M ? N : M)> operator^(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} ^ uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator==(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} == uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator!=(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} != uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator<(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} < uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator<=(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} <= uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator>(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} > uint_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator>=(const uint_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return uint_fixed_t<R>{a} >= uint_fixed_t<R>{b};
    }

    // =========================================================================
    // Cross-N binary operators — int_fixed_t<N> op int_fixed_t<M> (N != M)
    // =========================================================================

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator+(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} + int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator-(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} - int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator*(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} * int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    int_fixed_t<(N > M ? N : M)> operator/(const int_fixed_t<N> &a, const int_fixed_t<M> &b)
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} / int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    int_fixed_t<(N > M ? N : M)> operator%(const int_fixed_t<N> &a, const int_fixed_t<M> &b)
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} % int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator&(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} & int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator|(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} | int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr int_fixed_t<(N > M ? N : M)> operator^(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} ^ int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator==(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} == int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator!=(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} != int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator<(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} < int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator<=(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} <= int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator>(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} > int_fixed_t<R>{b};
    }

    template <std::size_t N, std::size_t M, typename = std::enable_if_t<N != M>>
    constexpr bool operator>=(const int_fixed_t<N> &a, const int_fixed_t<M> &b) noexcept
    {
        constexpr std::size_t R = N > M ? N : M;
        return int_fixed_t<R>{a} >= int_fixed_t<R>{b};
    }

    // =========================================================================
    // Mixed-sign free operators — int_fixed_t<N> op uint_fixed_t<M>
    // C++ usual arithmetic conversions: N > M -> int_fixed_t<N>; N <= M -> uint_fixed_t<M>.
    // Both orientations (int op uint, uint op int) produce the same result type.
    // =========================================================================

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator+(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} + R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator+(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} + R{b}; }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator-(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} - R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator-(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} - R{b}; }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator*(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} * R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator*(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} * R{b}; }

    template <std::size_t N, std::size_t M>
    detail::mixed_iu_t<N, M> operator/(const int_fixed_t<N> &a, const uint_fixed_t<M> &b)
    { using R = detail::mixed_iu_t<N, M>; return R{a} / R{b}; }
    template <std::size_t N, std::size_t M>
    detail::mixed_iu_t<N, M> operator/(const uint_fixed_t<M> &a, const int_fixed_t<N> &b)
    { using R = detail::mixed_iu_t<N, M>; return R{a} / R{b}; }

    template <std::size_t N, std::size_t M>
    detail::mixed_iu_t<N, M> operator%(const int_fixed_t<N> &a, const uint_fixed_t<M> &b)
    { using R = detail::mixed_iu_t<N, M>; return R{a} % R{b}; }
    template <std::size_t N, std::size_t M>
    detail::mixed_iu_t<N, M> operator%(const uint_fixed_t<M> &a, const int_fixed_t<N> &b)
    { using R = detail::mixed_iu_t<N, M>; return R{a} % R{b}; }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator&(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} & R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator&(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} & R{b}; }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator|(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} | R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator|(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} | R{b}; }

    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator^(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} ^ R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr detail::mixed_iu_t<N, M> operator^(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} ^ R{b}; }

    template <std::size_t N, std::size_t M>
    constexpr bool operator==(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} == R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr bool operator==(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} == R{b}; }

    template <std::size_t N, std::size_t M>
    constexpr bool operator!=(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} != R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr bool operator!=(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} != R{b}; }

    template <std::size_t N, std::size_t M>
    constexpr bool operator<(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} < R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr bool operator<(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} < R{b}; }

    template <std::size_t N, std::size_t M>
    constexpr bool operator<=(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} <= R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr bool operator<=(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} <= R{b}; }

    template <std::size_t N, std::size_t M>
    constexpr bool operator>(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} > R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr bool operator>(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} > R{b}; }

    template <std::size_t N, std::size_t M>
    constexpr bool operator>=(const int_fixed_t<N> &a, const uint_fixed_t<M> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} >= R{b}; }
    template <std::size_t N, std::size_t M>
    constexpr bool operator>=(const uint_fixed_t<M> &a, const int_fixed_t<N> &b) noexcept
    { using R = detail::mixed_iu_t<N, M>; return R{a} >= R{b}; }

    // =========================================================================
    // Higher arithmetic — mul_wide, pow, sqrt, gcd, lcm, checked_*
    // =========================================================================

    template <std::size_t N>
    [[nodiscard]] constexpr uint_fixed_t<2 * N>
    mul_wide(const uint_fixed_t<N> &a, const uint_fixed_t<N> &b) noexcept
    {
        return uint_fixed_t<2 * N>{a} * uint_fixed_t<2 * N>{b};
    }

    template <std::size_t N>
    [[nodiscard]] constexpr int_fixed_t<2 * N>
    mul_wide(const int_fixed_t<N> &a, const int_fixed_t<N> &b) noexcept
    {
        return int_fixed_t<2 * N>{a} * int_fixed_t<2 * N>{b};
    }

    template <std::size_t N>
    [[nodiscard]] constexpr uint_fixed_t<N>
    pow(uint_fixed_t<N> base, uint_fixed_t<N> exp) noexcept
    {
        uint_fixed_t<N> result = uint_fixed_t<N>::one();
        while (!exp.is_zero())
        {
            if (exp.data[0] & std::uint64_t{1})
                result *= base;
            base *= base;
            exp >>= 1;
        }
        return result;
    }

    template <std::size_t N>
    [[nodiscard]] constexpr int_fixed_t<N>
    pow(int_fixed_t<N> base, uint_fixed_t<N> exp) noexcept
    {
        int_fixed_t<N> result = int_fixed_t<N>::one();
        while (!exp.is_zero())
        {
            if (exp.data[0] & std::uint64_t{1})
                result *= base;
            base *= base;
            exp >>= 1;
        }
        return result;
    }

    template <std::size_t N>
    [[nodiscard]] uint_fixed_t<N> sqrt(const uint_fixed_t<N> &x)
    {
        if (x.is_zero())
            return uint_fixed_t<N>{};
        const unsigned bw = x.bit_width();
        uint_fixed_t<N> r = uint_fixed_t<N>::one() << ((bw + 1) / 2);
        for (;;)
        {
            const uint_fixed_t<N> nr = (r + x / r) >> 1;
            if (nr >= r)
                break;
            r = nr;
        }
        return r;
    }

    template <std::size_t N>
    [[nodiscard]] constexpr uint_fixed_t<N>
    gcd(uint_fixed_t<N> a, uint_fixed_t<N> b) noexcept
    {
        if (a.is_zero()) return b;
        if (b.is_zero()) return a;
        const unsigned ka = a.count_trailing_zeros();
        const unsigned kb = b.count_trailing_zeros();
        const unsigned k  = ka < kb ? ka : kb;
        a >>= ka;
        b >>= kb;
        while (!b.is_zero())
        {
            if (a < b)
            {
                uint_fixed_t<N> t = a;
                a = b;
                b = t;
            }
            a -= b;
            if (!a.is_zero())
                a >>= a.count_trailing_zeros();
        }
        return a << k;
    }

    template <std::size_t N>
    [[nodiscard]] constexpr uint_fixed_t<N>
    gcd(const int_fixed_t<N> &a, const int_fixed_t<N> &b) noexcept
    {
        return gcd(a.is_negative() ? uint_fixed_t<N>{-a} : uint_fixed_t<N>{a},
                   b.is_negative() ? uint_fixed_t<N>{-b} : uint_fixed_t<N>{b});
    }

    template <std::size_t N>
    [[nodiscard]] uint_fixed_t<N>
    lcm(const uint_fixed_t<N> &a, const uint_fixed_t<N> &b)
    {
        if (a.is_zero() || b.is_zero())
            return uint_fixed_t<N>{};
        return a / gcd(a, b) * b;
    }

    template <std::size_t N>
    [[nodiscard]] uint_fixed_t<N>
    lcm(const int_fixed_t<N> &a, const int_fixed_t<N> &b)
    {
        const uint_fixed_t<N> ua = a.is_negative() ? uint_fixed_t<N>{-a} : uint_fixed_t<N>{a};
        const uint_fixed_t<N> ub = b.is_negative() ? uint_fixed_t<N>{-b} : uint_fixed_t<N>{b};
        return lcm(ua, ub);
    }

    template <std::size_t N>
    [[nodiscard]] constexpr std::optional<uint_fixed_t<N>>
    checked_add(const uint_fixed_t<N> &a, const uint_fixed_t<N> &b) noexcept
    {
        const uint_fixed_t<N> r = a + b;
        if (r < a) return std::nullopt;
        return r;
    }

    template <std::size_t N>
    [[nodiscard]] constexpr std::optional<uint_fixed_t<N>>
    checked_sub(const uint_fixed_t<N> &a, const uint_fixed_t<N> &b) noexcept
    {
        if (b > a) return std::nullopt;
        return a - b;
    }

    template <std::size_t N>
    [[nodiscard]] constexpr std::optional<uint_fixed_t<N>>
    checked_mul(const uint_fixed_t<N> &a, const uint_fixed_t<N> &b) noexcept
    {
        const uint_fixed_t<2 * N> wide = mul_wide(a, b);
        for (std::size_t i = N; i < 2 * N; ++i)
            if (wide.data[i] != 0)
                return std::nullopt;
        return uint_fixed_t<N>{wide};
    }

    template <std::size_t N>
    [[nodiscard]] constexpr std::optional<int_fixed_t<N>>
    checked_add(const int_fixed_t<N> &a, const int_fixed_t<N> &b) noexcept
    {
        const int_fixed_t<N> r = a + b;
        const bool a_neg = a.is_negative();
        if (a_neg == b.is_negative() && r.is_negative() != a_neg)
            return std::nullopt;
        return r;
    }

    template <std::size_t N>
    [[nodiscard]] constexpr std::optional<int_fixed_t<N>>
    checked_sub(const int_fixed_t<N> &a, const int_fixed_t<N> &b) noexcept
    {
        const int_fixed_t<N> r = a - b;
        const bool a_neg = a.is_negative();
        if (a_neg != b.is_negative() && r.is_negative() != a_neg)
            return std::nullopt;
        return r;
    }

    template <std::size_t N>
    [[nodiscard]] constexpr std::optional<int_fixed_t<N>>
    checked_mul(const int_fixed_t<N> &a, const int_fixed_t<N> &b) noexcept
    {
        const int_fixed_t<2 * N> wide = mul_wide(a, b);
        const std::uint64_t fill = wide.data[N - 1] >> 63 ? ~std::uint64_t{0} : std::uint64_t{0};
        for (std::size_t i = N; i < 2 * N; ++i)
            if (wide.data[i] != fill)
                return std::nullopt;
        return int_fixed_t<N>{wide};
    }

} // namespace nstd

#endif // FIXED_WIDTH_INT_T_HPP
