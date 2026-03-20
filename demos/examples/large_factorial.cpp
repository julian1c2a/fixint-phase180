// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Example: Large Factorial with uint128_t
// Part of int128 Library - https://github.com/julian1c2a/int128-phase175
// License: BSL-1.0
// =============================================================================
//
// Demonstrates computing factorials that overflow 64-bit integers.
// uint64_t overflows at 21!, but uint128_t can handle up to 34!
//

#include "int128_parameterized.hpp"
#include <iostream>
#include <cstdint>

using nstd::uint128_t;

// Compute n! using uint128_t
uint128_t factorial_128(int n)
{
    uint128_t result(1ULL);
    for (int i = 2; i <= n; ++i)
    {
        result = result * uint128_t(static_cast<uint64_t>(i));
    }
    return result;
}

// Compute n! using uint64_t (will overflow for n > 20)
uint64_t factorial_64(int n)
{
    uint64_t result = 1;
    for (int i = 2; i <= n; ++i)
    {
        result *= static_cast<uint64_t>(i);
    }
    return result;
}

int main()
{
    std::cout << "=== Example: Large Factorials ===" << std::endl;
    std::cout << std::endl;

    std::cout << "  n   uint64_t            uint128_t" << std::endl;
    std::cout << "----  -------------------  ----------------------------------------" << std::endl;

    for (int n = 1; n <= 34; ++n)
    {
        const uint128_t f128 = factorial_128(n);
        const uint64_t f64 = factorial_64(n);

        const bool overflow_64 = (n > 20);

        std::cout << " ";
        if (n < 10)
        {
            std::cout << " ";
        }
        std::cout << n << "   ";

        if (overflow_64)
        {
            std::cout << "(overflow)          ";
        }
        else
        {
            // Print uint64 right-aligned in 20 chars
            const std::string s64 = std::to_string(f64);
            for (size_t i = s64.size(); i < 20; ++i)
            {
                std::cout << ' ';
            }
            std::cout << s64;
        }

        std::cout << "  " << f128.to_string() << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Note: uint64_t overflows after 20!" << std::endl;
    std::cout << "      uint128_t max = 2^128 - 1 = " << std::endl;
    std::cout << "      340282366920938463463374607431768211455" << std::endl;
    std::cout << "      34! = " << factorial_128(34).to_string() << std::endl;
    std::cout << std::endl;

    std::cout << "[OK] Factorial example complete." << std::endl;
    return 0;
}
