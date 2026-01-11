#include "include/int128_parameterized.hpp"
#include <iostream>
#include <iomanip>

int main()
{
    nstd::uint128_t x(0xDEADBEEFCAFEBABEULL, 0x0102030405060708ULL);
    std::cout << std::hex << std::setfill('0') << std::setw(16);
    std::cout << "high: " << x.high() << std::endl;
    std::cout << "low: " << x.low() << std::endl;
    std::cout << "Expected high: 0102030405060708" << std::endl;
    std::cout << "Expected low: DEADBEEFCAFEBABE" << std::endl;
    return 0;
}
