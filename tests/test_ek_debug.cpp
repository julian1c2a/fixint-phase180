// =============================================================================
// Test: Debug EK Arithmetic
// Part of int128 Library - Phase 1.75
// License: BSL-1.0
// =============================================================================

#include "int128_parameterized.hpp"
#include <iostream>
#include <iomanip>

using namespace nstd;

// Helper para imprimir en hex los valores internos
void print_value(const char *label, const int128_ek_t &val)
{
    std::cout << label << ": "
              << "to_string()=" << val.to_string()
              << ", high=0x" << std::hex << std::setw(16) << std::setfill('0') << val.high()
              << ", low=0x" << std::hex << std::setw(16) << std::setfill('0') << val.low()
              << std::dec << std::endl;
}

int main()
{
    std::cout << "=== EK Debug Tests ===" << std::endl;
    std::cout << "Bias K = 2^126 = 0x4000000000000000 (high) 0x0000000000000000 (low)" << std::endl;
    std::cout << std::endl;

    // Test 1: Zero
    std::cout << "--- Test 1: Zero ---" << std::endl;
    int128_ek_t zero{0};
    print_value("zero{0}", zero);
    std::cout << std::endl;

    // Test 2: +1
    std::cout << "--- Test 2: +1 ---" << std::endl;
    int128_ek_t one{1};
    print_value("one{1}", one);
    std::cout << std::endl;

    // Test 3: Pre-increment from zero
    std::cout << "--- Test 3: ++zero ---" << std::endl;
    int128_ek_t a{0};
    print_value("Before ++a", a);
    ++a;
    print_value("After  ++a", a);
    std::cout << "Expected: to_string()=1" << std::endl;
    std::cout << "Actual:   to_string()=" << a.to_string() << std::endl;
    std::cout << std::endl;

    // Test 4: Addition 10 + 20
    std::cout << "--- Test 4: 10 + 20 ---" << std::endl;
    int128_ek_t ten{10};
    int128_ek_t twenty{20};
    print_value("ten{10}", ten);
    print_value("twenty{20}", twenty);
    auto sum = ten + twenty;
    print_value("sum = ten + twenty", sum);
    std::cout << "Expected: to_string()=30" << std::endl;
    std::cout << "Actual:   to_string()=" << sum.to_string() << std::endl;
    std::cout << std::endl;

    // Test 5: Manual verification of bias arithmetic
    std::cout << "--- Test 5: Manual bias calculation ---" << std::endl;
    std::cout << "For EK addition: (x+K) + (y+K) = (x+y) + 2K" << std::endl;
    std::cout << "To get (x+y) + K, we need to subtract K" << std::endl;
    std::cout << std::endl;

    // ten = 10 + K
    // twenty = 20 + K
    // ten + twenty = (10+K) + (20+K) = 30 + 2K
    // We need to subtract K to get 30 + K

    std::cout << "ten.high() = 0x" << std::hex << ten.high() << std::dec << std::endl;
    std::cout << "ten.low() = 0x" << std::hex << ten.low() << std::dec << std::endl;
    std::cout << "Expected for +10: high=0x4000000000000000, low=0x000000000000000A" << std::endl;
    std::cout << std::endl;

    return 0;
}
