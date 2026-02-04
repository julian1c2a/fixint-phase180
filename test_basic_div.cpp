#include "include/int128_parameterized.hpp"
#include <iostream>
#include <iomanip>

using namespace nstd;

int main()
{
    // Test: 2^127 / 2 = 2^126
    const uint128_t dividend{0x8000000000000000ULL, 0x0};
    const uint128_t divisor{0x2, 0x0};

    std::cout << "Dividend = 2^127\n";
    std::cout << "  high() = " << std::hex << dividend.high() << "\n";
    std::cout << "  low()  = " << std::hex << dividend.low() << "\n";

    std::cout << "\nDivisor = 2\n";
    std::cout << "  high() = " << std::hex << divisor.high() << "\n";
    std::cout << "  low()  = " << std::hex << divisor.low() << "\n";

    auto [q, r] = dividend.D_knuth_divrem(divisor);

    std::cout << "\nResult from D_knuth_divrem:\n";
    std::cout << "  Quotient high() = " << std::hex << q.high() << "\n";
    std::cout << "  Quotient low()  = " << std::hex << q.low() << "\n";
    std::cout << "  Expected: 2^126 = 0x4000000000000000 in low\n";

    // 2^126 = 0x4000000000000000, should be in data[0] (low)
    std::cout << "  Correct? " << (q.low() == 0x4000000000000000ULL && q.high() == 0 ? "YES" : "NO") << "\n";

    return 0;
}
