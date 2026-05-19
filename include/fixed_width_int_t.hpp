// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// fixed_width_int_t.hpp — Fixed-width integer templates (N x 64-bit limbs)
// Part of int128 Library - Phase 1.90
// License: BSL-1.0
// =============================================================================
//
// uint_fixed_t<N>: unsigned two's-complement integer over N uint64_t limbs.
// data[0] = lowest limb (LSB), data[N-1] = highest limb (MSB).
//
// N=1 → 64-bit   (alias uint64_fixed_t)
// N=2 → 128-bit  (alias uint128_fixed_t)
// N=4 → 256-bit  (alias uint256_fixed_t)
// N=8 → 512-bit  (alias uint512_fixed_t)
//
// Operations (all mod 2^(64N)):
//   Construction: from any integral T, from limb array, from bytes, from bitset
//   Assignment:   from any integral T
//   Arithmetic:   +, -, *, /, %, unary -, ++, --
//   Bitwise:      &, |, ^, ~, <<, >>
//   Comparison:   ==, !=, <, <=, >, >=
//   Conversion:   explicit operator bool/T/bytes/bitset; to_string/from_string
//   Utilities:    zero(), max(), one(), is_zero(), bit_width(), popcount()
//
// int_fixed_t<N>: signed two's-complement integer over N uint64_t limbs.
//   Construction: from any integral T, from uint_fixed_t<N>, from bytes, from bitset
//   Assignment:   from any integral T
//   Arithmetic:   +, -, *, /, %, unary -, ++, --  (mod 2^(64N))
//   Bitwise:      &, |, ^, ~, << (logical), >> (arithmetic)
//   Comparison:   ==, !=, <, <=, >, >= (signed)
//   Utilities:    zero(), one(), max_val(), min_val(), is_zero(), is_negative(), abs()
//   Conversion:   explicit operator bool/T/bytes/bitset; to_string/from_string

#ifndef FIXED_WIDTH_INT_T_HPP
#define FIXED_WIDTH_INT_T_HPP

#include <algorithm>
#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#if __has_include("intrinsics/arithmetic_operations.hpp")
#include "intrinsics/arithmetic_operations.hpp"
#endif

namespace nstd
{

    // =========================================================================
    // Parse error codes and result type
    // =========================================================================

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

    template <std::size_t N>
    class uint_fixed_t
    {
        static_assert(N >= 1, "uint_fixed_t requires at least 1 limb");

    public:
        // data[0] = least-significant limb, data[N-1] = most-significant limb
        std::array<std::uint64_t, N> data{};

        // =========================================================================
        // Construction
        // =========================================================================

        constexpr uint_fixed_t() noexcept = default;

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        explicit constexpr uint_fixed_t(T v) noexcept : data{}
        {
            data[0] = static_cast<std::uint64_t>(v);
            const std::uint64_t fill = (std::is_signed_v<T> && v < 0) ? ~std::uint64_t{0} : std::uint64_t{0};
            for (std::size_t i{1}; i < N; ++i)
                data[i] = fill;
        }

        // Construct from array of limbs (data[0]=LSB, data[N-1]=MSB)
        explicit constexpr uint_fixed_t(std::array<std::uint64_t, N> limbs) noexcept
            : data{limbs}
        {
        }

        explicit constexpr uint_fixed_t(const std::array<std::byte, N * 8> &bytes) noexcept : data{}
        {
            for (std::size_t i{0}; i < N; ++i)
                for (int j{0}; j < 8; ++j)
                    data[i] |= static_cast<std::uint64_t>(bytes[i * 8 + j]) << (j * 8);
        }

        explicit constexpr uint_fixed_t(const std::bitset<64 * N> &b) noexcept : data{}
        {
            for (std::size_t i{0}; i < N; ++i)
                for (int j{0}; j < 64; ++j)
                    if (b.test(i * 64 + j))
                        data[i] |= std::uint64_t{1} << j;
        }

        // =========================================================================
        // Named constructors
        // =========================================================================

        static constexpr uint_fixed_t zero() noexcept 
        { 
            return uint_fixed_t{}; 
        }

        static constexpr uint_fixed_t one() noexcept
        {
            return uint_fixed_t{std::uint64_t{1}};
        }

        static constexpr uint_fixed_t max() noexcept
        {
            uint_fixed_t r{};
            for (auto &limb : r.data)
                limb = ~std::uint64_t{0};
            return r;
        }

        static constexpr uint_fixed_t min() noexcept { return zero(); }

        // =========================================================================
        // Assignment
        // =========================================================================

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr uint_fixed_t &operator=(T v) noexcept
        {
            return *this = uint_fixed_t{v};
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

        // =========================================================================
        // Explicit conversions
        // =========================================================================

        [[nodiscard]] explicit constexpr operator bool() const noexcept { return !is_zero(); }

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

        // =========================================================================
        // Comparison
        // =========================================================================

        constexpr bool operator==(const uint_fixed_t &o) const noexcept
        {
            return data == o.data;
        }

        constexpr bool operator!=(const uint_fixed_t &o) const noexcept
        {
            return !(*this == o);
        }

        constexpr bool operator<(const uint_fixed_t &o) const noexcept
        {
            for (std::size_t i{N}; i-- > 0;)
            {
                if (data[i] != o.data[i])
                    return data[i] < o.data[i];
            }
            return false;
        }

        constexpr bool operator<=(const uint_fixed_t &o) const noexcept
        {
            return !(o < *this);
        }

        constexpr bool operator>(const uint_fixed_t &o) const noexcept
        {
            return o < *this;
        }

        constexpr bool operator>=(const uint_fixed_t &o) const noexcept
        {
            return !(*this < o);
        }

        // =========================================================================
        // Bitwise
        // =========================================================================

        constexpr uint_fixed_t operator~() const noexcept
        {
            uint_fixed_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = ~data[i];
            return r;
        }

        constexpr uint_fixed_t operator&(const uint_fixed_t &o) const noexcept
        {
            uint_fixed_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = data[i] & o.data[i];
            return r;
        }

        constexpr uint_fixed_t operator|(const uint_fixed_t &o) const noexcept
        {
            uint_fixed_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = data[i] | o.data[i];
            return r;
        }

        constexpr uint_fixed_t operator^(const uint_fixed_t &o) const noexcept
        {
            uint_fixed_t r{};
            for (std::size_t i{0}; i < N; ++i)
                r.data[i] = data[i] ^ o.data[i];
            return r;
        }

        constexpr uint_fixed_t &operator&=(const uint_fixed_t &o) noexcept
        {
            *this = *this & o;
            return *this;
        }

        constexpr uint_fixed_t &operator|=(const uint_fixed_t &o) noexcept
        {
            *this = *this | o;
            return *this;
        }

        constexpr uint_fixed_t &operator^=(const uint_fixed_t &o) noexcept
        {
            *this = *this ^ o;
            return *this;
        }

        // Left shift (logical)
        constexpr uint_fixed_t operator<<(unsigned shift) const noexcept
        {
            uint_fixed_t r{};
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

        // Right shift (logical / unsigned)
        constexpr uint_fixed_t operator>>(unsigned shift) const noexcept
        {
            uint_fixed_t r{};
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

        constexpr uint_fixed_t &operator<<=(unsigned shift) noexcept
        {
            *this = *this << shift;
            return *this;
        }

        constexpr uint_fixed_t &operator>>=(unsigned shift) noexcept
        {
            *this = *this >> shift;
            return *this;
        }

        // =========================================================================
        // Arithmetic — addition/subtraction (ripple-carry via intrinsics or portable)
        // =========================================================================

        constexpr uint_fixed_t operator+(const uint_fixed_t &o) const noexcept
        {
            uint_fixed_t r{};
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

        constexpr uint_fixed_t operator-(const uint_fixed_t &o) const noexcept
        {
            uint_fixed_t r{};
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

        constexpr uint_fixed_t operator-() const noexcept
        {
            return ~(*this) + one();
        }

        constexpr uint_fixed_t &operator+=(const uint_fixed_t &o) noexcept
        {
            *this = *this + o;
            return *this;
        }

        constexpr uint_fixed_t &operator-=(const uint_fixed_t &o) noexcept
        {
            *this = *this - o;
            return *this;
        }

        constexpr uint_fixed_t &operator++() noexcept
        {
            *this += one();
            return *this;
        }

        constexpr uint_fixed_t operator++(int) noexcept
        {
            uint_fixed_t tmp{*this};
            ++(*this);
            return tmp;
        }

        constexpr uint_fixed_t &operator--() noexcept
        {
            *this -= one();
            return *this;
        }

        constexpr uint_fixed_t operator--(int) noexcept
        {
            uint_fixed_t tmp{*this};
            --(*this);
            return tmp;
        }

        // =========================================================================
        // Arithmetic — multiplication (schoolbook O(N²), mod 2^(64N))
        //
        // For each partial product a[i]*b[j] = (hi:lo) at position i+j:
        //   1. Add lo to r[i+j] via addcarry → carry c
        //   2. Add hi+c to r[i+j+1] via addcarry → carry c
        //   3. Propagate c through r[i+j+2..N-1]
        //
        // This keeps each addcarry operand in [0, 2^64-1], avoiding overflow in carry.
        // =========================================================================

        constexpr uint_fixed_t operator*(const uint_fixed_t &o) const noexcept
        {
            uint_fixed_t r{};
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
                    // Add lo to r[i+j]
                    unsigned char c = add_limb(r.data[i + j], lo);
                    // Add hi (with carry c) to r[i+j+1], then propagate
                    const std::size_t next = i + j + 1;
                    if (next < N)
                    {
                        c = add_limb_carry(r.data[next], hi, c);
                        // Ripple remaining carry
                        for (std::size_t k{next + 1}; k < N && c; ++k)
                            c = add_limb(r.data[k], std::uint64_t{c});
                    }
                }
            }
            return r;
        }

        constexpr uint_fixed_t &operator*=(const uint_fixed_t &o) noexcept
        {
            *this = *this * o;
            return *this;
        }

        // =========================================================================
        // Arithmetic — division and modulo (binary long division, O(64N²))
        //
        // Binary long division: process dividend MSB-to-LSB, build remainder r
        // one bit at a time. When r >= divisor, subtract and set quotient bit.
        //
        // Theorem: a == (a/b)*b + (a%b)  with  0 <= a%b < b
        // Throws std::domain_error on division by zero.
        // =========================================================================

        static std::pair<uint_fixed_t, uint_fixed_t>
        divmod(const uint_fixed_t &a, const uint_fixed_t &b)
        {
            if (b.is_zero())
                throw std::domain_error("uint_fixed_t::divmod: division by zero");
            if (a < b)
                return {uint_fixed_t{}, a};

            uint_fixed_t q{};
            uint_fixed_t r{};

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

        uint_fixed_t operator/(const uint_fixed_t &o) const
        {
            return divmod(*this, o).first;
        }

        uint_fixed_t operator%(const uint_fixed_t &o) const
        {
            return divmod(*this, o).second;
        }

        uint_fixed_t &operator/=(const uint_fixed_t &o)
        {
            *this = *this / o;
            return *this;
        }

        uint_fixed_t &operator%=(const uint_fixed_t &o)
        {
            *this = *this % o;
            return *this;
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

        // =========================================================================
        // String conversion — base 10
        // =========================================================================

        // Convert to decimal string (chunk-based: divides by 10^19 per iteration)
        std::string to_string() const
        {
            if (is_zero())
                return "0";

            constexpr std::size_t max_digits = N * 20 + 1;
            char buf[max_digits];
            int pos = static_cast<int>(max_digits);

            const uint_fixed_t chunk_base{std::uint64_t{10000000000000000000ULL}};
            uint_fixed_t tmp{*this};

            while (!tmp.is_zero())
            {
                const auto [q, r] = divmod(tmp, chunk_base);
                const std::uint64_t chunk = r.data[0];
                if (q.is_zero())
                    write_u64_digits(buf, pos, chunk);
                else
                    write_19_padded_digits(buf, pos, chunk);
                tmp = q;
            }

            return std::string(buf + pos, buf + max_digits);
        }

        // Parse from decimal string (throws std::invalid_argument on bad input)
        static uint_fixed_t from_string(const char *s)
        {
            if (!s || *s == '\0')
                throw std::invalid_argument("uint_fixed_t::from_string: empty string");
            const uint_fixed_t ten{std::uint64_t{10}};
            uint_fixed_t result{};
            bool any{false};
            for (; *s != '\0'; ++s)
            {
                const char c{*s};
                if (c < '0' || c > '9')
                    throw std::invalid_argument("uint_fixed_t::from_string: invalid character");
                result = result * ten + uint_fixed_t{static_cast<std::uint64_t>(c - '0')};
                any = true;
            }
            if (!any)
                throw std::invalid_argument("uint_fixed_t::from_string: no digits");
            return result;
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

    using uint128_fixed_t = uint_fixed_t<2>;
    using uint256_fixed_t = uint_fixed_t<4>;
    using uint512_fixed_t = uint_fixed_t<8>;
    using uint1024_fixed_t = uint_fixed_t<16>;

    // =============================================================================
    // int_fixed_t<N> — signed two's-complement, N × 64-bit limbs
    // =============================================================================

    template <std::size_t N>
    class int_fixed_t
    {
        static_assert(N >= 1, "int_fixed_t requires at least 1 limb");

    public:
        uint_fixed_t<N> bits{};

        // =========================================================================
        // Construction
        // =========================================================================

        constexpr int_fixed_t() noexcept = default;

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        explicit constexpr int_fixed_t(T v) noexcept : bits{}
        {
            bits.data[0] = static_cast<std::uint64_t>(v);
            const std::uint64_t fill = (std::is_signed_v<T> && v < 0) ? ~std::uint64_t{0} : std::uint64_t{0};
            for (std::size_t i{1}; i < N; ++i)
                bits.data[i] = fill;
        }

        explicit constexpr int_fixed_t(const uint_fixed_t<N> &u) noexcept : bits{u} {}

        explicit constexpr int_fixed_t(const std::array<std::byte, N * 8> &bytes) noexcept
            : bits{bytes}
        {
        }

        explicit constexpr int_fixed_t(const std::bitset<64 * N> &b) noexcept
            : bits{b}
        {
        }

        // =========================================================================
        // Named constructors
        // =========================================================================

        static constexpr int_fixed_t zero() noexcept { return int_fixed_t{}; }
        static constexpr int_fixed_t one() noexcept { return int_fixed_t{std::int64_t{1}}; }

        // max = 0x7FFF…FFFF (MSB=0, rest=1)
        static constexpr int_fixed_t max_val() noexcept
        {
            int_fixed_t r{};
            r.bits = uint_fixed_t<N>::max();
            r.bits.data[N - 1] >>= 1;
            return r;
        }

        // min = 0x8000…0000 (MSB=1, rest=0)
        static constexpr int_fixed_t min_val() noexcept
        {
            int_fixed_t r{};
            r.bits.data[N - 1] = std::uint64_t{1} << 63;
            return r;
        }

        static constexpr int_fixed_t min() noexcept { return min_val(); }
        static constexpr int_fixed_t max() noexcept { return max_val(); }

        // =========================================================================
        // Assignment
        // =========================================================================

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        constexpr int_fixed_t &operator=(T v) noexcept
        {
            return *this = int_fixed_t{v};
        }

        // =========================================================================
        // Predicates
        // =========================================================================

        constexpr bool is_zero() const noexcept { return bits.is_zero(); }
        constexpr bool is_negative() const noexcept { return (bits.data[N - 1] >> 63) != 0; }

        // =========================================================================
        // Explicit conversions
        // =========================================================================

        [[nodiscard]] explicit constexpr operator bool() const noexcept { return !is_zero(); }

        template <typename T,
                  typename = std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<std::remove_cv_t<T>, bool>>>
        [[nodiscard]] explicit constexpr operator T() const noexcept
        {
            return static_cast<T>(bits.data[0]);
        }

        [[nodiscard]] explicit constexpr operator std::array<std::byte, N * 8>() const noexcept
        {
            return static_cast<std::array<std::byte, N * 8>>(bits);
        }

        [[nodiscard]] explicit constexpr operator std::bitset<64 * N>() const noexcept
        {
            return static_cast<std::bitset<64 * N>>(bits);
        }

        // =========================================================================
        // Comparison (signed)
        // =========================================================================

        constexpr bool operator==(const int_fixed_t &o) const noexcept { return bits == o.bits; }
        constexpr bool operator!=(const int_fixed_t &o) const noexcept { return bits != o.bits; }

        constexpr bool operator<(const int_fixed_t &o) const noexcept
        {
            const bool a_neg = is_negative();
            const bool b_neg = o.is_negative();
            if (a_neg != b_neg)
                return a_neg;
            return bits < o.bits;
        }

        constexpr bool operator<=(const int_fixed_t &o) const noexcept { return !(o < *this); }
        constexpr bool operator>(const int_fixed_t &o) const noexcept { return o < *this; }
        constexpr bool operator>=(const int_fixed_t &o) const noexcept { return !(*this < o); }

        // =========================================================================
        // Bitwise
        // =========================================================================

        constexpr int_fixed_t operator~() const noexcept { return int_fixed_t{~bits}; }

        constexpr int_fixed_t operator&(const int_fixed_t &o) const noexcept
        {
            return int_fixed_t{bits & o.bits};
        }
        constexpr int_fixed_t operator|(const int_fixed_t &o) const noexcept
        {
            return int_fixed_t{bits | o.bits};
        }
        constexpr int_fixed_t operator^(const int_fixed_t &o) const noexcept
        {
            return int_fixed_t{bits ^ o.bits};
        }

        constexpr int_fixed_t &operator&=(const int_fixed_t &o) noexcept
        {
            bits &= o.bits;
            return *this;
        }
        constexpr int_fixed_t &operator|=(const int_fixed_t &o) noexcept
        {
            bits |= o.bits;
            return *this;
        }
        constexpr int_fixed_t &operator^=(const int_fixed_t &o) noexcept
        {
            bits ^= o.bits;
            return *this;
        }

        // Left shift (logical)
        constexpr int_fixed_t operator<<(unsigned shift) const noexcept
        {
            return int_fixed_t{bits << shift};
        }

        // Right shift (arithmetic — fill with sign bit)
        constexpr int_fixed_t operator>>(unsigned shift) const noexcept
        {
            if (shift == 0)
                return *this;
            if (shift >= 64U * N)
                return is_negative() ? int_fixed_t{std::int64_t{-1}} : zero();
            if (!is_negative())
                return int_fixed_t{bits >> shift};
            const uint_fixed_t<N> fill = uint_fixed_t<N>::max() << (64U * N - shift);
            return int_fixed_t{(bits >> shift) | fill};
        }

        constexpr int_fixed_t &operator<<=(unsigned shift) noexcept
        {
            *this = *this << shift;
            return *this;
        }
        constexpr int_fixed_t &operator>>=(unsigned shift) noexcept
        {
            *this = *this >> shift;
            return *this;
        }

        // =========================================================================
        // Arithmetic — add, subtract, negate, inc/dec, multiply (mod 2^(64N))
        // =========================================================================

        constexpr int_fixed_t operator+(const int_fixed_t &o) const noexcept
        {
            return int_fixed_t{bits + o.bits};
        }
        constexpr int_fixed_t operator-(const int_fixed_t &o) const noexcept
        {
            return int_fixed_t{bits - o.bits};
        }
        constexpr int_fixed_t operator-() const noexcept { return int_fixed_t{-bits}; }

        constexpr int_fixed_t &operator+=(const int_fixed_t &o) noexcept
        {
            bits += o.bits;
            return *this;
        }
        constexpr int_fixed_t &operator-=(const int_fixed_t &o) noexcept
        {
            bits -= o.bits;
            return *this;
        }

        constexpr int_fixed_t &operator++() noexcept
        {
            ++bits;
            return *this;
        }
        constexpr int_fixed_t operator++(int) noexcept
        {
            int_fixed_t t{*this};
            ++(*this);
            return t;
        }
        constexpr int_fixed_t &operator--() noexcept
        {
            --bits;
            return *this;
        }
        constexpr int_fixed_t operator--(int) noexcept
        {
            int_fixed_t t{*this};
            --(*this);
            return t;
        }

        constexpr int_fixed_t operator*(const int_fixed_t &o) const noexcept
        {
            return int_fixed_t{bits * o.bits};
        }
        constexpr int_fixed_t &operator*=(const int_fixed_t &o) noexcept
        {
            bits *= o.bits;
            return *this;
        }

        // =========================================================================
        // Arithmetic — division and modulo (truncation-toward-zero, C++ semantics)
        //
        // Quotient sign: negative iff operands differ in sign.
        // Remainder sign: same as dividend.
        // Invariant: a == (a/b)*b + (a%b)
        // Throws std::domain_error on division by zero.
        // =========================================================================

        static std::pair<int_fixed_t, int_fixed_t>
        divmod(const int_fixed_t &a, const int_fixed_t &b)
        {
            if (b.is_zero())
                throw std::domain_error("int_fixed_t::divmod: division by zero");

            const bool a_neg = a.is_negative();
            const bool b_neg = b.is_negative();
            const uint_fixed_t<N> ua = a_neg ? (-a).bits : a.bits;
            const uint_fixed_t<N> ub = b_neg ? (-b).bits : b.bits;
            const auto [uq, ur] = uint_fixed_t<N>::divmod(ua, ub);

            const bool q_neg = (a_neg != b_neg);
            int_fixed_t q{q_neg ? -int_fixed_t{uq} : int_fixed_t{uq}};
            int_fixed_t r{a_neg ? -int_fixed_t{ur} : int_fixed_t{ur}};
            return {q, r};
        }

        int_fixed_t operator/(const int_fixed_t &o) const { return divmod(*this, o).first; }
        int_fixed_t operator%(const int_fixed_t &o) const { return divmod(*this, o).second; }
        int_fixed_t &operator/=(const int_fixed_t &o)
        {
            *this = *this / o;
            return *this;
        }
        int_fixed_t &operator%=(const int_fixed_t &o)
        {
            *this = *this % o;
            return *this;
        }

        // =========================================================================
        // Utility
        // =========================================================================

        // Absolute value (undefined for min_val())
        constexpr int_fixed_t abs() const noexcept
        {
            return is_negative() ? -(*this) : *this;
        }

        // =========================================================================
        // String conversion — signed decimal
        // =========================================================================

        std::string to_string() const
        {
            if (is_zero())
                return "0";
            if (is_negative())
                return "-" + (-(*this)).bits.to_string();
            return bits.to_string();
        }

        static int_fixed_t from_string(const char *s)
        {
            if (!s || *s == '\0')
                throw std::invalid_argument("int_fixed_t::from_string: empty string");
            if (*s == '-')
            {
                if (*(s + 1) == '\0')
                    throw std::invalid_argument("int_fixed_t::from_string: only minus sign");
                return -int_fixed_t{uint_fixed_t<N>::from_string(s + 1)};
            }
            return int_fixed_t{uint_fixed_t<N>::from_string(s)};
        }
    };

    // =============================================================================
    // Type aliases — signed
    // =============================================================================

    using int128_fixed_t = int_fixed_t<2>;
    using int256_fixed_t = int_fixed_t<4>;
    using int512_fixed_t = int_fixed_t<8>;
    using int1024_fixed_t = int_fixed_t<16>;

} // namespace nstd

#endif // FIXED_WIDTH_INT_T_HPP
