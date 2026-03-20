// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int128_param_traits.hpp - Type traits
// Part of int128 Library
// License: BSL-1.0
// =============================================================================

#include "int128_param_traits.hpp"
#include <iostream>
#include <type_traits>

// Test counter
static int passed{0};
static int failed{0};

#define TEST(name, condition)                         \
    do                                                \
    {                                                 \
        if (condition)                                \
        {                                             \
            ++passed;                                 \
        }                                             \
        else                                          \
        {                                             \
            std::cout << "  [FAIL] " << name << "\n"; \
            ++failed;                                 \
        }                                             \
    } while (0)

int main()
{
    using nstd::int128_ek_t;
    using nstd::int128_ms_t;
    using nstd::int128_tc_t;
    using nstd::uint128_t;

    std::cout << "====================================================================\n";
    std::cout << "Type Traits Tests (common_type and helpers)\n";
    std::cout << "====================================================================\n\n";

    // ========================================================================
    // [Test 1] common_type - Same type
    // ========================================================================
    {
        std::cout << "[Test 1] common_type - Same type:\n";

        using CT1 = std::common_type_t<uint128_t, uint128_t>;
        using CT2 = std::common_type_t<int128_tc_t, int128_tc_t>;
        using CT3 = std::common_type_t<int128_ms_t, int128_ms_t>;

        TEST("same_type_uint128", (std::is_same_v<CT1, uint128_t>));
        TEST("same_type_int128_tc", (std::is_same_v<CT2, int128_tc_t>));
        TEST("same_type_int128_ms", (std::is_same_v<CT3, int128_ms_t>));

        std::cout << "  [OK] common_type same type (3/3)\n";
    }

    // ========================================================================
    // [Test 2] common_type - Different forms (promote to TC)
    // ========================================================================
    {
        std::cout << "\n[Test 2] common_type - Different forms:\n";

        // MS + TC → TC (standard form)
        using CT1 = std::common_type_t<int128_ms_t, int128_tc_t>;
        TEST("ms_tc_to_tc", (std::is_same_v<CT1, int128_tc_t>));

        // EK + TC → TC
        using CT2 = std::common_type_t<int128_ek_t, int128_tc_t>;
        TEST("ek_tc_to_tc", (std::is_same_v<CT2, int128_tc_t>));

        // MS + EK → TC
        using CT3 = std::common_type_t<int128_ms_t, int128_ek_t>;
        TEST("ms_ek_to_tc", (std::is_same_v<CT3, int128_tc_t>));

        std::cout << "  [OK] different forms promote to TC (3/3)\n";
    }

    // ========================================================================
    // [Test 3] common_type - Unsigned + Signed (promote to signed)
    // ========================================================================
    {
        std::cout << "\n[Test 3] common_type - Unsigned + Signed:\n";

        // uint128 + int128_tc → int128_tc (signed, TC form)
        using CT1 = std::common_type_t<uint128_t, int128_tc_t>;
        TEST("uint128_int128tc_to_signed", (std::is_same_v<CT1, int128_tc_t>));

        // Symmetric: int128_tc + uint128 → int128_tc
        using CT2 = std::common_type_t<int128_tc_t, uint128_t>;
        TEST("int128tc_uint128_to_signed", (std::is_same_v<CT2, int128_tc_t>));

        std::cout << "  [OK] unsigned + signed promotes to signed (2/2)\n";
    }

    // ========================================================================
    // [Test 4] common_type - int128 + builtin integral
    // ========================================================================
    {
        std::cout << "\n[Test 4] common_type - int128 + builtin:\n";

        // int128_tc + int64_t → int128_tc (preserve form)
        using CT1 = std::common_type_t<int128_tc_t, int64_t>;
        TEST("int128tc_int64_to_int128tc", (std::is_same_v<CT1, int128_tc_t>));

        // int128_ms + int32_t → int128_ms (preserve MS form)
        using CT2 = std::common_type_t<int128_ms_t, int32_t>;
        TEST("int128ms_int32_to_int128ms", (std::is_same_v<CT2, int128_ms_t>));

        // uint128 + uint64_t → uint128 (preserve unsigned)
        using CT3 = std::common_type_t<uint128_t, uint64_t>;
        TEST("uint128_uint64_to_uint128", (std::is_same_v<CT3, uint128_t>));

        // Symmetric: int64_t + int128_tc → int128_tc
        using CT4 = std::common_type_t<int64_t, int128_tc_t>;
        TEST("int64_int128tc_to_int128tc", (std::is_same_v<CT4, int128_tc_t>));

        std::cout << "  [OK] int128 + builtin preserves form (4/4)\n";
    }

    // ========================================================================
    // [Test 5] common_type - Unsigned builtin + Signed int128
    // ========================================================================
    {
        std::cout << "\n[Test 5] common_type - Unsigned builtin + Signed int128:\n";

        // uint64_t + int128_tc → int128_tc (promote to signed)
        using CT1 = std::common_type_t<uint64_t, int128_tc_t>;
        TEST("uint64_int128tc_to_signed", (std::is_same_v<CT1, int128_tc_t>));

        // int128_ms + uint32_t → int128_ms (preserve form, promote to signed)
        using CT2 = std::common_type_t<int128_ms_t, uint32_t>;
        TEST("int128ms_uint32_to_signed", (std::is_same_v<CT2, int128_ms_t>));

        std::cout << "  [OK] unsigned builtin + signed int128 (2/2)\n";
    }

    // ========================================================================
    // [Test 6] is_int128_param helper trait
    // ========================================================================
    {
        std::cout << "\n[Test 6] is_int128_param helper:\n";

        TEST("is_int128_param_uint128", std::is_int128_param_v<uint128_t>);
        TEST("is_int128_param_int128tc", std::is_int128_param_v<int128_tc_t>);
        TEST("is_int128_param_int128ms", std::is_int128_param_v<int128_ms_t>);
        TEST("is_int128_param_int128ek", std::is_int128_param_v<int128_ek_t>);

        TEST("is_int128_param_int64_false", !std::is_int128_param_v<int64_t>);
        TEST("is_int128_param_double_false", !std::is_int128_param_v<double>);

        std::cout << "  [OK] is_int128_param (6/6)\n";
    }

    // ========================================================================
    // [Test 7] nstd:: namespace mirrors
    // ========================================================================
    {
        std::cout << "\n[Test 7] nstd:: namespace mirrors:\n";

        // nstd::common_type_t should work
        using CT1 = nstd::common_type_t<uint128_t, int128_tc_t>;
        TEST("nstd_common_type_t", (std::is_same_v<CT1, int128_tc_t>));

        // nstd::is_int128_param_v should work
        TEST("nstd_is_int128_param_v", nstd::is_int128_param_v<uint128_t>);

        std::cout << "  [OK] nstd mirrors (2/2)\n";
    }

    // ========================================================================
    // [Test 8] Common type in generic code (real-world usage)
    // ========================================================================
    {
        std::cout << "\n[Test 8] Common type in generic code:\n";

        // Simulate generic function using common_type
        auto add_generic = [](auto a, auto b)
        {
            using CT = std::common_type_t<decltype(a), decltype(b)>;
            // Convert explicitly using high() and low() accessors
            CT a_converted{a.high(), a.low()};
            CT b_converted{b.high(), b.low()};
            return a_converted + b_converted;
        };

        const uint128_t a{0, 10};
        const int128_tc_t b{0, 20};
        const auto result{add_generic(a, b)};

        // Result should be int128_tc (signed, TC form)
        TEST("generic_add_type",
             (std::is_same_v<decltype(result), const int128_tc_t>));

        TEST("generic_add_value", result.low() == 30);

        std::cout << "  [OK] common type in generic code (2/2)\n";
    }

    // ========================================================================
    // [Test 9] Three-way common_type (TC, MS, EK)
    // ========================================================================
    {
        std::cout << "\n[Test 9] Three-way common_type:\n";

        // Common type of three different forms should be TC
        using CT1 = std::common_type_t<int128_tc_t, int128_ms_t>;
        using CT2 = std::common_type_t<CT1, int128_ek_t>;

        TEST("three_way_common_type", (std::is_same_v<CT2, int128_tc_t>));

        std::cout << "  [OK] three-way common type (1/1)\n";
    }

    // ========================================================================
    // [Test 10] Edge case - const/volatile qualifiers
    // ========================================================================
    {
        std::cout << "\n[Test 10] const/volatile qualifiers:\n";

        // common_type should strip cv-qualifiers
        using CT1 = std::common_type_t<const uint128_t, uint128_t>;
        TEST("const_stripped", (std::is_same_v<CT1, uint128_t>));

        using CT2 = std::common_type_t<volatile int128_tc_t, int128_tc_t>;
        TEST("volatile_stripped", (std::is_same_v<CT2, int128_tc_t>));

        std::cout << "  [OK] cv-qualifiers handled (2/2)\n";
    }

    // ========================================================================
    // Results
    // ========================================================================
    std::cout << "\n====================================================================\n";
    std::cout << "RESULTS:\n";
    std::cout << "  Passed: " << passed << "\n";
    std::cout << "  Failed: " << failed << "\n";
    std::cout << "  Total:  " << (passed + failed) << "\n";
    std::cout << "====================================================================\n";

    return (failed == 0) ? 0 : 1;
}
