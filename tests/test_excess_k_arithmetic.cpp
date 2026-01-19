// =============================================================================
// Test: Excess-K Arithmetic
// =============================================================================

#include "int128_param_iostreams.hpp" // Include for easy debugging via std::cout
#include <iostream>
#include <cassert>

using namespace nstd;

// A simple test function
void test_addition() {
    std::cout << "Test: EK Addition\n";

    // Expected values
    const int128_ek_t one = int128_ek_t::from_string("1");
    const int128_ek_t two = int128_ek_t::from_string("2");
    const int128_ek_t neg_one = int128_ek_t::from_string("-1");
    const int128_ek_t zero = int128_ek_t::from_string("0");

    // Test 1: 1 + 1 = 2
    int128_ek_t result1 = one;
    result1 += one;

    std::cout << "  1 + 1 = " << result1 << " (expected 2)\n";
    assert(result1 == two);
    std::cout << "  OK: 1 + 1 == 2\n";

    // Test 2: 1 + (-1) = 0
    int128_ek_t result2 = one;
    result2 += neg_one;
    
    std::cout << "  1 + (-1) = " << result2 << " (expected 0)\n";
    assert(result2 == zero);
    std::cout << "  OK: 1 + (-1) == 0\n";
}

int main() {
    try {
        test_addition();
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception." << std::endl;
        return 1;
    }

    std::cout << "\nOK: All Excess-K arithmetic tests passed!\n";
    return 0;
}
