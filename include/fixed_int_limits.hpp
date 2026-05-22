// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// fixed_int_limits.hpp — std::numeric_limits for fixed_int_t<N, Sign, Form>
// Part of int128 Library - Phase 1.81 (Fase MS-INTEROP, T5)
// License: BSL-1.0
// =============================================================================
//
// std::numeric_limits is specializable for user-defined types per the C++
// standard (§17.6.4.2.1). We provide a partial specialization covering ALL
// (N, Sign, Form) combinations of fixed_int_t. It compiles for any valid
// instantiation — currently:
//   - <N, unsigned_type, binnat>          → uint_fixed_t<N>
//   - <N, signed_type,   twos_complement> → int_fixed_t<N>
// Future Forms (MS, EK) gained for fixed_int_t will be picked up automatically
// because we dispatch on Sign / Form via if constexpr — no extra specialization
// is required unless the bit layout of min/max differs (then add an additional
// partial specialization).
//
// Values (mirrors std::numeric_limits for fundamental integers):
//   digits         = 64*N        (unsigned)         | 64*N - 1   (signed)
//   digits10       = (digits * 30103) / 100000      (floor(digits * log10(2)))
//   min() / max()  delegate to fixed_int_t<...>::min() / ::max()
//   lowest()       = min()
//   is_signed      true iff Sign == signed_type
//   is_modulo      true iff Sign == unsigned_type   (signed overflow is UB)
//   is_integer     true; is_exact true; is_bounded true; radix = 2

#ifndef FIXED_INT_LIMITS_HPP
#define FIXED_INT_LIMITS_HPP

#include "fixed_width_int_t.hpp"

#include <limits>

namespace std
{

    template <size_t N, ::nstd::signedness Sign, ::nstd::representation_form Form>
    class numeric_limits<::nstd::fixed_int_t<N, Sign, Form>>
    {
    public:
        using value_type = ::nstd::fixed_int_t<N, Sign, Form>;

        static constexpr bool is_specialized = true;
        static constexpr bool is_signed      = (Sign == ::nstd::signedness::signed_type);
        static constexpr bool is_integer     = true;
        static constexpr bool is_exact       = true;
        static constexpr bool has_infinity        = false;
        static constexpr bool has_quiet_NaN       = false;
        static constexpr bool has_signaling_NaN   = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss     = false;
        static constexpr float_round_style round_style = round_toward_zero;
        static constexpr bool is_iec559           = false;
        static constexpr bool is_bounded          = true;
        static constexpr bool is_modulo           = !is_signed;  // unsigned wraps; signed overflow is UB
        static constexpr int  digits              = static_cast<int>(64 * N) - (is_signed ? 1 : 0);
        static constexpr int  digits10            = (digits * 30103) / 100000; // floor(digits * log10(2))
        static constexpr int  max_digits10        = 0;
        static constexpr int  radix               = 2;
        static constexpr int  min_exponent        = 0;
        static constexpr int  min_exponent10      = 0;
        static constexpr int  max_exponent        = 0;
        static constexpr int  max_exponent10      = 0;
        static constexpr bool traps               = false;
        static constexpr bool tinyness_before     = false;

        static constexpr value_type min() noexcept   { return value_type::min(); }
        static constexpr value_type max() noexcept   { return value_type::max(); }
        static constexpr value_type lowest() noexcept { return value_type::min(); }
        static constexpr value_type epsilon() noexcept       { return value_type::zero(); }
        static constexpr value_type round_error() noexcept   { return value_type::zero(); }
        static constexpr value_type infinity() noexcept      { return value_type::zero(); }
        static constexpr value_type quiet_NaN() noexcept     { return value_type::zero(); }
        static constexpr value_type signaling_NaN() noexcept { return value_type::zero(); }
        static constexpr value_type denorm_min() noexcept    { return value_type::zero(); }
    };

} // namespace std

#endif // FIXED_INT_LIMITS_HPP
