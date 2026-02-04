#include <iostream>
#include "../include/int128_parameterized.hpp"

using int128_tc_t = nstd::int128_param_t<nstd::signedness::signed_type, nstd::representation_form::twos_complement>;

int main()
{
    // Test: -20 / 4
    int128_tc_t a{static_cast<int64_t>(-20)};
    int128_tc_t b{0, 4};

    std::cout << "a before: is_negative=" << a.is_negative() << ", low=" << a.low() << ", high=" << a.high() << std::endl;
    std::cout << "b: is_negative=" << b.is_negative() << ", low=" << b.low() << ", high=" << b.high() << std::endl;

    a /= b;

    std::cout << "a after /= b: is_negative=" << a.is_negative() << ", low=" << a.low() << ", high=" << a.high() << std::endl;

    // Check the result
    if (a.is_negative() && a.low() == 5)
    {
        std::cout << "SUCCESS: Result is -5" << std::endl;
    }
    else
    {
        std::cout << "FAIL: Expected -5, got something else" << std::endl;
        std::cout << "actual a.is_negative()=" << a.is_negative() << std::endl;
        std::cout << "actual a.low()=" << a.low() << std::endl;
    }

    return 0;
}
