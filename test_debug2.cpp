#include "include/int128_parameterized.hpp"
#include <iostream>

int main()
{
    std::cout << "Test 1: Constructor(uint64_t)" << std::endl;
    nstd::uint128_t x1(0xDEADBEEFCAFEBABEULL);
    std::cout << "high: " << std::hex << x1.high() << ", low: " << std::hex << x1.low() << std::endl;

    std::cout << "\nTest 2: Constructor(uint64_t, uint64_t)" << std::endl;
    nstd::uint128_t x2(0xDEADBEEFCAFEBABEULL, 0x0102030405060708ULL);
    std::cout << "high: " << std::hex << x2.high() << ", low: " << std::hex << x2.low() << std::endl;

    std::cout << "\nDirect construction:" << std::endl;
    nstd::uint128_t x3;
    x3.set_high(0x0102030405060708ULL);
    x3.set_low(0xDEADBEEFCAFEBABEULL);
    std::cout << "high: " << std::hex << x3.high() << ", low: " << std::hex << x3.low() << std::endl;

    return 0;
}
