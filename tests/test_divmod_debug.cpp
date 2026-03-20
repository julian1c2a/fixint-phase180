// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Debug Test: 128/128 Division Analysis
// =============================================================================

#include "int128_parameterized.hpp"
#include "representation.hpp"
#include <iostream>
#include <iomanip>

using namespace nstd;

int main()
{
    std::cout << "\n====================================================================\n";
    std::cout << "DEBUG: 2^127 / 2 Division\n";
    std::cout << "====================================================================\n";

    // Create 2^127: constructor takes (high, low), stores as data{low, high}
    // To get 2^127, we need data[1] = 0x8000000000000000, data[0] = 0x0
    // Constructor (high, low) stores as data{low, high}, so:
    // Constructor(0x8000000000000000, 0x0) → data{0x0, 0x8000000000000000}

    const uint128_t dividend{0x8000000000000000ULL, 0x0};
    const uint128_t divisor{0x0, 0x2}; // Constructor: (high, low) → data{low, high} = {2, 0}

    std::cout << "\nInputs:\n";
    std::cout << std::hex << std::setfill('0');
    std::cout << "  Dividend: data[0]=0x" << std::setw(16) << dividend.low()
              << " data[1]=0x" << std::setw(16) << dividend.high() << "\n";
    std::cout << "  Divisor:  data[0]=0x" << std::setw(16) << divisor.low()
              << " data[1]=0x" << std::setw(16) << divisor.high() << "\n";

    auto [quotient, remainder] = dividend.divmod(divisor);

    std::cout << "\nActual Result:\n";
    std::cout << "  Quotient: data[0]=0x" << std::setw(16) << quotient.low()
              << " data[1]=0x" << std::setw(16) << quotient.high() << "\n";
    std::cout << "  Remainder: data[0]=0x" << std::setw(16) << remainder.low()
              << " data[1]=0x" << std::setw(16) << remainder.high() << "\n";

    const uint128_t expected_quotient{0x4000000000000000ULL, 0x0};
    const uint128_t expected_remainder{0x0, 0x0};

    std::cout << "\nExpected Result:\n";
    std::cout << "  Quotient: data[0]=0x" << std::setw(16) << expected_quotient.low()
              << " data[1]=0x" << std::setw(16) << expected_quotient.high() << "\n";
    std::cout << "  Remainder: data[0]=0x" << std::setw(16) << expected_remainder.low()
              << " data[1]=0x" << std::setw(16) << expected_remainder.high() << "\n";

    std::cout << std::dec << "\nAnalysis:\n";
    if (quotient == expected_quotient && remainder == expected_remainder)
    {
        std::cout << "  ✓ Test PASSED\n";
        return 0;
    }
    else
    {
        std::cout << "  ✗ Test FAILED\n";
        if (quotient != expected_quotient)
        {
            std::cout << "    - Quotient mismatch\n";
            std::cout << "    - Expected 0x4000000000000000 in one word, got split across both\n";
            std::cout << "    - Possible cause: bit indexing in binary long division\n";
        }
        if (remainder != expected_remainder)
        {
            std::cout << "    - Remainder mismatch\n";
        }
        return 1;
    }
}
