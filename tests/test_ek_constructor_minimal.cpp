// Minimal test to debug EK constructor with optimization
#include "int128_parameterized.hpp"
#include <iostream>
#include <iomanip>

using namespace nstd;

int main()
{
    // Test with optimization -O2
    constexpr std::uint64_t K_high = (1ULL << 62);

    std::cout << "=== Excess-K Constructor Test ===" << std::endl;
    std::cout << "Expected bias K = 0x" << std::hex << K_high << "00000000000000000" << std::dec << "\n\n";

    // Direct constructor call
    int128_ek_t value{100};

    std::cout << "int128_ek_t{100}:" << std::endl;
    std::cout << "  high = 0x" << std::hex << std::setw(16) << std::setfill('0') << value.high() << std::endl;
    std::cout << "  low  = 0x" << std::setw(16) << std::setfill('0') << value.low() << std::endl;
    std::cout << "  Expected: high = 0x" << std::setw(16) << K_high << ", low = 0x" << std::setw(16) << 100 << std::dec << std::endl;

    // Check constexpr flags
    std::cout << "\nType traits:" << std::endl;
    std::cout << "  is_signed = " << int128_ek_t::is_signed << std::endl;
    std::cout << "  is_excess_k = " << int128_ek_t::is_excess_k << std::endl;
    std::cout << "  Form value = " << static_cast<int>(int128_ek_t::form) << std::endl;
    std::cout << "  Sign value = " << static_cast<int>(int128_ek_t::sign) << std::endl;

    return 0;
}
