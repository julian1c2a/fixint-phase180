// Debug test for EK addition
#include "int128_parameterized.hpp"
#include <iostream>
#include <iomanip>

using namespace nstd;

void print_ek(const char *label, const int128_ek_t &val)
{
    std::cout << label << ": high=0x" << std::hex << std::setw(16) << std::setfill('0')
              << val.high() << " low=0x" << val.low() << std::dec << "\n";
}

int main()
{
    constexpr std::uint64_t K_high = (1ULL << 62);
    constexpr std::uint64_t K_low = 0ULL;

    std::cout << "K (bias) = 0x" << std::hex << K_high << " " << K_low << std::dec << "\n\n";

    // Test 1: Simple addition 100 + 200 = 300
    int128_ek_t a{100};
    int128_ek_t b{200};

    std::cout << "=== Test 1: 100 + 200 ===\n";
    print_ek("a (100)", a);
    print_ek("b (200)", b);

    // Expected stored values:
    // a: real=100, stored=100+K
    // b: real=200, stored=200+K
    std::cout << "\nExpected a_stored: high=0x" << std::hex << K_high
              << " low=0x" << 100 << std::dec << "\n";
    std::cout << "Expected b_stored: high=0x" << std::hex << K_high
              << " low=0x" << 200 << std::dec << "\n";

    a += b;
    print_ek("a after +=", a);

    // Expected result: real=300, stored=300+K
    std::cout << "Expected result: high=0x" << std::hex << K_high
              << " low=0x" << 300 << std::dec << "\n";

    int128_ek_t expected{300};
    print_ek("expected (300)", expected);

    std::cout << "\nTest " << (a == expected ? "PASS" : "FAIL") << "\n\n";

    // Test 2: Check if constructor encodes with bias
    int128_ek_t zero{0};
    print_ek("zero constructor", zero);
    std::cout << "Expected zero: high=0x" << std::hex << K_high << " low=0x0\n";

    return 0;
}
