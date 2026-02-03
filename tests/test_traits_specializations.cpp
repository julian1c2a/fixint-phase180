// =============================================================================
// Test: Type Traits Specializations (nstd::)
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

// First include the main header (defines type aliases)
#include "representation.hpp"
#include "int128_parameterized.hpp"
// Then include traits (uses the type aliases)
#include "int128_param_traits_specializations.hpp"
#include <iostream>
#include <unordered_map>

using namespace nstd;

int test_count = 0, pass_count = 0;

#define TEST(name, condition)                              \
    do                                                     \
    {                                                      \
        test_count++;                                      \
        if (condition)                                     \
        {                                                  \
            pass_count++;                                  \
            std::cout << "  [OK] " << name << std::endl;   \
        }                                                  \
        else                                               \
        {                                                  \
            std::cout << "  [FAIL] " << name << std::endl; \
        }                                                  \
    } while (false)

int main()
{
    std::cout << "====================================================================" << std::endl;
    std::cout << "Type Traits Specializations Tests (4 valid types)" << std::endl;
    std::cout << "====================================================================" << std::endl;

    // Type aliases for the 4 valid types
    using binnat_t = int128_param_t<signedness::unsigned_type, representation_form::binnat>;
    using int128_tc_t = int128_param_t<signedness::signed_type, representation_form::twos_complement>;
    using int128_ms_t = int128_param_t<signedness::signed_type, representation_form::magnitude_sign>;
    using int128_ek_t = int128_param_t<signedness::signed_type, representation_form::excess_k>;

    // ========================================================================
    // Group 1: is_integral (4 types)
    // ========================================================================
    std::cout << "\n[Group 1] is_integral:" << std::endl;

    TEST("binnat_is_integral", nstd::is_integral_v<binnat_t>);
    TEST("int128_tc_t_is_integral", nstd::is_integral_v<int128_tc_t>);
    TEST("int128_ms_t_is_integral", nstd::is_integral_v<int128_ms_t>);
    TEST("int128_ek_t_is_integral", nstd::is_integral_v<int128_ek_t>);

    // ========================================================================
    // Group 2: is_signed / is_unsigned
    // ========================================================================
    std::cout << "\n[Group 2] is_signed / is_unsigned:" << std::endl;

    TEST("binnat_is_unsigned", nstd::is_unsigned_v<binnat_t>);
    TEST("int128_tc_t_is_signed", nstd::is_signed_v<int128_tc_t>);
    TEST("int128_ms_t_is_signed", nstd::is_signed_v<int128_ms_t>);
    TEST("int128_ek_t_is_signed", nstd::is_signed_v<int128_ek_t>);

    // ========================================================================
    // Group 3: is_arithmetic
    // ========================================================================
    std::cout << "\n[Group 3] is_arithmetic:" << std::endl;

    TEST("binnat_is_arithmetic", nstd::is_arithmetic_v<binnat_t>);
    TEST("int128_tc_t_is_arithmetic", nstd::is_arithmetic_v<int128_tc_t>);
    TEST("int128_ms_t_is_arithmetic", nstd::is_arithmetic_v<int128_ms_t>);
    TEST("int128_ek_t_is_arithmetic", nstd::is_arithmetic_v<int128_ek_t>);

    // ========================================================================
    // Group 4: Trivial properties
    // ========================================================================
    std::cout << "\n[Group 4] Trivial properties:" << std::endl;

    TEST("binnat_trivially_copyable", nstd::is_trivially_copyable_v<binnat_t>);
    TEST("int128_tc_t_trivially_copyable", nstd::is_trivially_copyable_v<int128_tc_t>);
    TEST("int128_ms_t_standard_layout", nstd::is_standard_layout_v<int128_ms_t>);
    TEST("int128_ek_t_trivially_destructible", nstd::is_trivially_destructible_v<int128_ek_t>);

    // ========================================================================
    // Group 5: make_signed / make_unsigned
    // ========================================================================
    std::cout << "\n[Group 5] make_signed / make_unsigned:" << std::endl;

    TEST("make_signed_binnat", (std::is_same_v<nstd::make_signed_t<binnat_t>, int128_tc_t>));
    TEST("make_unsigned_int128_tc", (std::is_same_v<nstd::make_unsigned_t<int128_tc_t>, binnat_t>));
    TEST("make_unsigned_int128_ms", (std::is_same_v<nstd::make_unsigned_t<int128_ms_t>, binnat_t>));
    TEST("make_unsigned_int128_ek", (std::is_same_v<nstd::make_unsigned_t<int128_ek_t>, binnat_t>));

    // ========================================================================
    // Group 6: Hash
    // ========================================================================
    std::cout << "\n[Group 6] Hash:" << std::endl;

    binnat_t val_bn{100};
    int128_tc_t val_tc{-50};
    int128_ms_t val_ms{42};
    int128_ek_t val_ek{200};

    nstd::hash<binnat_t> hasher_bn;
    nstd::hash<int128_tc_t> hasher_tc;
    nstd::hash<int128_ms_t> hasher_ms;
    nstd::hash<int128_ek_t> hasher_ek;

    size_t h_bn = hasher_bn(val_bn);
    size_t h_tc = hasher_tc(val_tc);
    size_t h_ms = hasher_ms(val_ms);
    size_t h_ek = hasher_ek(val_ek);

    TEST("hash_binnat_compiles", h_bn != 0 || true);
    TEST("hash_tc_compiles", h_tc != 0 || true);
    TEST("hash_ms_compiles", h_ms != 0 || true);
    TEST("hash_ek_compiles", h_ek != 0 || true);

    // Test with std::unordered_map
    std::unordered_map<binnat_t, int, nstd::hash<binnat_t>> map;
    map[binnat_t{42}] = 100;
    TEST("hash_in_unordered_map", map[binnat_t{42}] == 100);

    // ========================================================================
    // Group 7: Backward compatibility (uint128_t/int128_t aliases)
    // ========================================================================
    std::cout << "\n[Group 7] Backward compatibility:" << std::endl;

    TEST("uint128_t_is_integral", nstd::is_integral_v<uint128_t>);
    TEST("int128_t_is_integral", nstd::is_integral_v<int128_t>);
    TEST("uint128_t_is_unsigned", nstd::is_unsigned_v<uint128_t>);
    TEST("int128_t_is_signed", nstd::is_signed_v<int128_t>);
    TEST("make_signed_uint128_t", (std::is_same_v<nstd::make_signed_t<uint128_t>, int128_t>));
    TEST("make_unsigned_int128_t", (std::is_same_v<nstd::make_unsigned_t<int128_t>, uint128_t>));

    // ========================================================================
    // Group 8: Builtin types (verify nstd:: delegates to std::)
    // ========================================================================
    std::cout << "\n[Group 8] Builtin types (nstd:: delegates to std::):" << std::endl;

    TEST("int_is_integral", nstd::is_integral_v<int>);
    TEST("uint64_t_is_unsigned", nstd::is_unsigned_v<uint64_t>);
    TEST("double_is_arithmetic", nstd::is_arithmetic_v<double>);
    TEST("float_not_integral", !nstd::is_integral_v<float>);

    // ========================================================================
    // Results
    // ========================================================================
    std::cout << "\n====================================================================" << std::endl;
    std::cout << "RESULTS:" << std::endl;
    std::cout << "  Passed: " << pass_count << std::endl;
    std::cout << "  Failed: " << (test_count - pass_count) << std::endl;
    std::cout << "  Total:  " << test_count << std::endl;
    std::cout << "====================================================================" << std::endl;

    return (pass_count == test_count) ? 0 : 1;
}
