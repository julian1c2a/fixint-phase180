#include "include/int128_parameterized.hpp"
#include <iostream>
#include <iomanip>

using namespace nstd;

int main()
{
    int128_ms_t neg1(-1LL);
    int128_ms_t neg2(-2LL);

    std::cout << std::hex << std::setfill('0') << std::setw(16);
    std::cout << "neg1(-1LL):" << std::endl;
    std::cout << "  high: " << neg1.high() << std::endl;
    std::cout << "  low: " << neg1.low() << std::endl;
    std::cout << "  is_negative: " << (int)neg1.is_negative() << std::endl;

    std::cout << "\nneg2(-2LL):" << std::endl;
    std::cout << "  high: " << neg2.high() << std::endl;
    std::cout << "  low: " << neg2.low() << std::endl;
    std::cout << "  is_negative: " << (int)neg2.is_negative() << std::endl;

    return 0;
}
