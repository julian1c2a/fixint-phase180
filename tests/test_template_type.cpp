// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// Test to see what template type T is being used
#include "int128_parameterized.hpp"
#include <iostream>
#include <type_traits>

using namespace nstd;

int main()
{
    // Verify that int128_ek_t can be constructed from int
    const int128_ek_t value{100};

    std::cout << "int128_ek_t{100} constructed successfully\n";
    std::cout << "  low()  = " << value.low() << "\n";
    std::cout << "  high() = " << value.high() << "\n";
    std::cout << "  is_zero() = " << value.is_zero() << "\n";

    // Verify basic type traits
    std::cout << "\nType traits:\n";
    std::cout
        << "  uint128_t is int128_param_t<unsigned_type, binnat>: "
        << std::is_same_v<uint128_t,
                          int128_param_t<signedness::unsigned_type, representation_form::binnat>> << "\n";
    std::cout << "  int128_tc_t is int128_param_t<signed_type, twos_complement>: "
              << std::is_same_v<int128_tc_t, int128_param_t<signedness::signed_type,
                                                            representation_form::twos_complement>> << "\n";

    std::cout << "\n[OK] Template type test passed\n";
    return 0;
}
