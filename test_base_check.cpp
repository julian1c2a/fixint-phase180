#include "int128_parameterized.hpp"
#include <iostream>
using namespace nstd;
int main() {
    uint128_tc_t val{0, 255};
    std::cout << "Dec: " << val.to_string(10) << "\n";
    std::cout << "Hex: " << val.to_string(16) << "\n";
    std::cout << "Oct: " << val.to_string(8) << "\n";
    return 0;
}
