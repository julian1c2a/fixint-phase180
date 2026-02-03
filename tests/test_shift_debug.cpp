// =============================================================================
// Debug: Right Shift Test
// =============================================================================

#include "representation.hpp"
#include "int128_parameterized.hpp"
#include <iostream>

using namespace nstd;

int main()
{
    std::cout << "=== Right Shift Debug ===" << std::endl;

    // Test 1: Right shift zero
    std::cout << "\n--- Test 1: Right shift by 0 ---" << std::endl;
    int128_t x(42LL);
    std::cout << "x = " << x.to_string() << std::endl;
    std::cout << "x >> 0 = ";
    std::cout.flush();
    auto result = x >> 0;
    std::cout << result.to_string() << std::endl;
    std::cout << "Comparing x >> 0 == int128_t(42): ";
    std::cout.flush();
    bool equal = (result == int128_t(42LL));
    std::cout << (equal ? "true" : "false") << std::endl;

    // Test 2: Right shift by 1
    std::cout << "\n--- Test 2: Right shift by 1 ---" << std::endl;
    int128_t y(8LL);
    std::cout << "y = " << y.to_string() << std::endl;
    std::cout << "y >> 1 = ";
    std::cout.flush();
    auto result2 = y >> 1;
    std::cout << result2.to_string() << std::endl;
    std::cout << "Comparing y >> 1 == int128_t(4): ";
    std::cout.flush();
    bool equal2 = (result2 == int128_t(4LL));
    std::cout << (equal2 ? "true" : "false") << std::endl;

    std::cout << "\n=== Tests Complete ===" << std::endl;
    return 0;
}
