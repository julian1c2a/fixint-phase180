#include "include/int128_parameterized.hpp"
#include <iostream>

using namespace nstd;

int main()
{
    std::cout << "=== MS Comparison Test ===" << std::endl;

    // Positive comparison (normal)
    int128_ms_t pos1(1LL);
    int128_ms_t pos2(2LL);
    std::cout << "Positive: 1 < 2 = " << (pos1 < pos2) << " (expected: true)" << std::endl;
    std::cout << "Positive: 2 > 1 = " << (pos2 > pos1) << " (expected: true)" << std::endl;

    // Negative comparison (INVERTED)
    // -1 has magnitude 1, -2 has magnitude 2
    // So -1 > -2 (because |-1| < |-2|)
    int128_ms_t neg1(-1LL);
    int128_ms_t neg2(-2LL);
    std::cout << "\nNegative (inverted): -1 < -2 = " << (neg1 < neg2) << " (expected: false)" << std::endl;
    std::cout << "Negative (inverted): -1 > -2 = " << (neg1 > neg2) << " (expected: true)" << std::endl;
    std::cout << "Negative (inverted): -2 < -1 = " << (neg2 < neg1) << " (expected: true)" << std::endl;

    // Cross-sign
    std::cout << "\nCross-sign: -2 < 1 = " << (neg2 < pos1) << " (expected: true)" << std::endl;
    std::cout << "Cross-sign: 1 > -2 = " << (pos1 > neg2) << " (expected: true)" << std::endl;

    return 0;
}
