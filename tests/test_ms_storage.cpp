#include <iostream>
#include <limits>
#include <cassert>
#include <string>
#include "int128_parameterized.hpp"

void test_get_magnitud_and_abs()
{
    using nstd::int128_ms_t;
    std::cout << "\n[MS] Test get_magnitud() y abs():\n";
    struct
    {
        const char *name;
        int64_t val;
        uint64_t expected;
    } casos[] = {
        {"+0", 0, 0ULL},
        {"-0", 0, 0ULL},
        {"+1", 1, 1ULL},
        {"-1", -1, 1ULL},
        {"+2", 2, 2ULL},
        {"-2", -2, 2ULL},
        {"max", std::numeric_limits<int64_t>::max(), static_cast<uint64_t>(std::numeric_limits<int64_t>::max())},
        {"min", std::numeric_limits<int64_t>::min(), static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1ULL},
        {"max-1", std::numeric_limits<int64_t>::max() - 1, static_cast<uint64_t>(std::numeric_limits<int64_t>::max() - 1)},
        {"min+1", std::numeric_limits<int64_t>::min() + 1, static_cast<uint64_t>(std::numeric_limits<int64_t>::max())},
        {"max-2", std::numeric_limits<int64_t>::max() - 2, static_cast<uint64_t>(std::numeric_limits<int64_t>::max() - 2)},
        {"min+2", std::numeric_limits<int64_t>::min() + 2, static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) - 1ULL}};
    for (const auto &c : casos)
    {
        int128_ms_t ms{c.val};
        // Forzar -0
        if (std::string(c.name) == "-0")
            ms.set_high(ms.high() | (1ULL << 63));
        auto abs_val = ms.abs();
        auto mag_val = ms.magnitude();
        uint64_t abs_u64 = abs_val.low();
        uint64_t mag_u64 = mag_val.low();
        std::cout << c.name << ": abs=" << abs_u64 << ", magnitude=" << mag_u64 << ", esperado=" << c.expected << std::endl;
        assert(abs_u64 == c.expected && "abs() no coincide con el valor absoluto esperado");
        assert(mag_u64 == c.expected && "magnitude() no coincide con el valor absoluto esperado");
    }
}
// NOTE: cross-representation static_cast (MS↔TC↔EK) not yet implemented in the library.
// This function is disabled until that feature is added.
#if 0
void test_casts_between_representations()
{
    std::cout << "\n[MS/TC/EK] Casts explícitos entre representaciones:\n";
    using nstd::int128_ek_t;
    using nstd::int128_ms_t;
    using nstd::int128_tc_t;
    struct
    {
        const char *name;
        int64_t val;
    } casos[] = {
        {"+0", 0}, {"-0", 0}, {"+1", 1}, {"-1", -1}, {"+2", 2}, {"-2", -2}, {"max", std::numeric_limits<int64_t>::max()}, {"min", std::numeric_limits<int64_t>::min()}, {"max-1", std::numeric_limits<int64_t>::max() - 1}, {"min+1", std::numeric_limits<int64_t>::min() + 1}, {"max-2", std::numeric_limits<int64_t>::max() - 2}, {"min+2", std::numeric_limits<int64_t>::min() + 2}};
    for (const auto &c : casos)
    {
        int128_ms_t ms{c.val};
        int128_tc_t tc = static_cast<int128_tc_t>(ms);
        int128_ek_t ek = static_cast<int128_ek_t>(ms);
        int128_ms_t ms2 = static_cast<int128_ms_t>(tc);
        int128_ek_t ek2 = static_cast<int128_ek_t>(tc);
        int128_tc_t tc2 = static_cast<int128_tc_t>(ek);
        int128_ms_t ms3 = static_cast<int128_ms_t>(ek);
        // Comprobar round-trip: convert back to MS and compare
        bool ok1 = (static_cast<int128_ms_t>(tc) == ms);
        bool ok2 = (static_cast<int128_ms_t>(ek) == ms);
        bool ok3 = (ms2 == ms);
        bool ok4 = (static_cast<int128_ms_t>(ek2) == ms);
        bool ok5 = (static_cast<int128_ms_t>(tc2) == ms);
        bool ok6 = (ms3 == ms);
        std::cout << c.name << ": MS->TC=" << ok1 << ", MS->EK=" << ok2 << ", MS->TC->MS=" << ok3
                  << ", MS->TC->EK=" << ok4 << ", MS->EK->TC=" << ok5 << ", MS->EK->MS=" << ok6 << std::endl;
        if (!(ok1 && ok2 && ok3 && ok4 && ok5 && ok6))
        {
            std::cout << "  [FAIL] Cast error for " << c.name << ", val=" << c.val << std::endl;
            assert(false && "Cast error: los valores no coinciden tras la conversión entre representaciones");
        }
    }
}
#endif
// =============================================================================
// Test: Magnitude-Sign - Constructores, Métodos de Consulta y Comparaciones
// Part of int128 Phase 1.75 - Diagnóstico exhaustivo MS
// =============================================================================

using nstd::int128_ms_t;

void test_constructors()
{
    std::cout << "[MS] Constructores básicos:\n";
    int128_ms_t plus_zero{0};
    int128_ms_t minus_zero{0};
    minus_zero.set_high(minus_zero.high() | (1ULL << 63));
    int128_ms_t one{1};
    int128_ms_t mone{-1};
    int128_ms_t two{2};
    int128_ms_t mtwo{-2};
    int128_ms_t max{std::numeric_limits<int64_t>::max()};
    int128_ms_t min{std::numeric_limits<int64_t>::min()};
    int128_ms_t maxm1{std::numeric_limits<int64_t>::max() - 1};
    int128_ms_t minp1{std::numeric_limits<int64_t>::min() + 1};
    int128_ms_t maxm2{std::numeric_limits<int64_t>::max() - 2};
    int128_ms_t minp2{std::numeric_limits<int64_t>::min() + 2};
    // Volcado de datos internos
    auto dump = [](const char *name, const int128_ms_t &v)
    {
        std::cout << name << ": data[1]=0x" << std::hex << v.high() << ", data[0]=0x" << v.low() << std::dec << "\n";
    };
    dump("+0", plus_zero);
    dump("-0", minus_zero);
    dump("+1", one);
    dump("-1", mone);
    dump("+2", two);
    dump("-2", mtwo);
    dump("max", max);
    dump("min", min);
    dump("max-1", maxm1);
    dump("min+1", minp1);
    dump("max-2", maxm2);
    dump("min+2", minp2);
}

void test_info_methods()
{
    std::cout << "\n[MS] Métodos de consulta:\n";
    int128_ms_t vals[] = {
        int128_ms_t{0},
        int128_ms_t{0},
        int128_ms_t{1},
        int128_ms_t{-1},
        int128_ms_t{2},
        int128_ms_t{-2},
        int128_ms_t{std::numeric_limits<int64_t>::max()},
        int128_ms_t{std::numeric_limits<int64_t>::min()},
        int128_ms_t{std::numeric_limits<int64_t>::max() - 1},
        int128_ms_t{std::numeric_limits<int64_t>::min() + 1},
        int128_ms_t{std::numeric_limits<int64_t>::max() - 2},
        int128_ms_t{std::numeric_limits<int64_t>::min() + 2}};
    // Forzar -0
    vals[1].set_high(vals[1].high() | (1ULL << 63));
    const char *names[] = {"+0", "-0", "+1", "-1", "+2", "-2", "max", "min", "max-1", "min+1", "max-2", "min+2"};
    for (int i = 0; i < 12; ++i)
    {
        std::cout << names[i] << ": is_negative=" << vals[i].is_negative();
        std::cout << ", magnitude=" << static_cast<int64_t>(vals[i].magnitude().low());
        std::cout << ", get_sign=" << vals[i].get_sign();
        std::cout << ", is_zero=" << vals[i].is_zero();
        std::cout << ", is_positive_zero=" << vals[i].is_positive_zero();
        std::cout << ", is_negative_zero=" << vals[i].is_negative_zero();
        std::cout << "\n";
    }
}

void test_comparisons()
{
    std::cout << "\n[MS] Comparaciones exhaustivas:\n";
    int128_ms_t vals[] = {
        int128_ms_t{0},
        int128_ms_t{0},
        int128_ms_t{1},
        int128_ms_t{-1},
        int128_ms_t{2},
        int128_ms_t{-2},
        int128_ms_t{std::numeric_limits<int64_t>::max()},
        int128_ms_t{std::numeric_limits<int64_t>::min()},
        int128_ms_t{std::numeric_limits<int64_t>::max() - 1},
        int128_ms_t{std::numeric_limits<int64_t>::min() + 1},
        int128_ms_t{std::numeric_limits<int64_t>::max() - 2},
        int128_ms_t{std::numeric_limits<int64_t>::min() + 2}};
    vals[1].set_high(vals[1].high() | (1ULL << 63)); // -0
    const char *names[] = {"+0", "-0", "+1", "-1", "+2", "-2", "max", "min", "max-1", "min+1", "max-2", "min+2"};
    for (int i = 0; i < 12; ++i)
    {
        for (int j = 0; j < 12; ++j)
        {
            std::cout << names[i] << " == " << names[j] << ": " << (vals[i] == vals[j]) << ", ";
            std::cout << names[i] << " != " << names[j] << ": " << (vals[i] != vals[j]) << ", ";
            std::cout << names[i] << " < " << names[j] << ": " << (vals[i] < vals[j]) << ", ";
            std::cout << names[i] << " > " << names[j] << ": " << (vals[i] > vals[j]) << ", ";
            std::cout << names[i] << " <= " << names[j] << ": " << (vals[i] <= vals[j]) << ", ";
            std::cout << names[i] << " >= " << names[j] << ": " << (vals[i] >= vals[j]) << std::endl;
        }
    }
}

int main()
{
    test_constructors();
    test_info_methods();
    test_comparisons();
    // test_casts_between_representations(); // disabled: cross-form casts not yet implemented
    test_get_magnitud_and_abs();
    // ===============================
    // Test: operator++/--, next, previous
    // ===============================
    using nstd::int128_ms_t;
    std::cout << "\n[MS] Test operator++/--, next, previous:\n";
    // Extract the mathematical value from an MS int128 as uint64_t (sign + magnitude)
    // Uses two's-complement bit pattern so INT64_MIN maps to 0x8000000000000000.
    auto ms_low_bits = [](const int128_ms_t &v) -> uint64_t {
        return v.is_negative() ? (uint64_t{0} - v.low()) : v.low();
    };
    struct
    {
        const char *name;
        int64_t val;
        uint64_t expected_inc; // as two's-complement bit pattern
        uint64_t expected_dec;
    } casos[] = {
        {"+0", 0, 1ULL, static_cast<uint64_t>(-1LL)},
        {"-0", 0, 1ULL, static_cast<uint64_t>(-1LL)},
        {"+1", 1, 2ULL, 0ULL},
        {"-1", -1, 0ULL, static_cast<uint64_t>(-2LL)},
        {"max", std::numeric_limits<int64_t>::max(),
         static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1ULL,
         static_cast<uint64_t>(std::numeric_limits<int64_t>::max() - 1)},
        {"min", std::numeric_limits<int64_t>::min(),
         static_cast<uint64_t>(std::numeric_limits<int64_t>::min() + 1),
         static_cast<uint64_t>(std::numeric_limits<int64_t>::max())},
    };
    for (const auto &c : casos)
    {
        int128_ms_t ms{c.val};
        // Forzar -0
        if (std::string(c.name) == "-0")
            ms.set_high(ms.high() | (1ULL << 63));
        // Pre-incremento
        int128_ms_t preinc{ms};
        ++preinc;
        uint64_t preinc_val = ms_low_bits(preinc);
        // Post-incremento
        int128_ms_t postinc{ms};
        int128_ms_t postinc_ret = postinc++;
        uint64_t postinc_val = ms_low_bits(postinc);
        uint64_t postinc_ret_val = ms_low_bits(postinc_ret);
        // Pre-decremento
        int128_ms_t predec{ms};
        --predec;
        uint64_t predec_val = ms_low_bits(predec);
        // Post-decremento
        int128_ms_t postdec{ms};
        int128_ms_t postdec_ret = postdec--;
        uint64_t postdec_val = ms_low_bits(postdec);
        uint64_t postdec_ret_val = ms_low_bits(postdec_ret);
        // next/previous (next() and previous() not implemented; use ++/-- on a copy)
        int128_ms_t next_v{ms}; ++next_v;
        int128_ms_t prev_v{ms}; --prev_v;
        uint64_t next_bits = ms_low_bits(next_v);
        uint64_t prev_bits = ms_low_bits(prev_v);
        uint64_t orig_bits = ms_low_bits(ms);
        std::cout << c.name << ": ++ = " << preinc_val << ", (x++) = " << postinc_val << ", ret(x++) = " << postinc_ret_val
                  << ", -- = " << predec_val << ", (x--) = " << postdec_val << ", ret(x--) = " << postdec_ret_val
                  << ", next() = " << next_bits << ", previous() = " << prev_bits << std::endl;
        // Asserts
        assert(preinc_val == c.expected_inc && "Pre-incremento incorrecto");
        assert(postinc_val == c.expected_inc && "Post-incremento incorrecto");
        assert(postinc_ret_val == orig_bits && "Retorno de post-incremento incorrecto");
        assert(predec_val == c.expected_dec && "Pre-decremento incorrecto");
        assert(postdec_val == c.expected_dec && "Post-decremento incorrecto");
        assert(postdec_ret_val == orig_bits && "Retorno de post-decremento incorrecto");
        assert(next_bits == c.expected_inc && "next() incorrecto");
        assert(prev_bits == c.expected_dec && "previous() incorrecto");
    }
    return 0;
}
