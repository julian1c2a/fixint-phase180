// Test to see what template type T is being used
#include "int128_parameterized.hpp"
#include <iostream>
#include <type_traits>

using namespace nstd;

// Helper to print type name
template <typename T>
const char *type_name()
{
    if constexpr (std::is_same_v<T, int>)
        return "int";
    if constexpr (std::is_same_v<T, long>)
        return "long";
    if constexpr (std::is_same_v<T, long long>)
        return "long long";
    if constexpr (std::is_same_v<T, unsigned int>)
        return "unsigned int";
    if constexpr (std::is_same_v<T, unsigned long>)
        return "unsigned long";
    if constexpr (std::is_same_v<T, unsigned long long>)
        return "unsigned long long";
    return "unknown";
}

// Modified constructor for testing
template <signedness Sign, representation_form Form>
template <typename T, typename>
constexpr int128_param_t<Sign, Form>::int128_param_t(T value) noexcept : data{0, 0}
{
    // This won't compile, but shows concept - we need to add diagnostics to actual constructor
    std::cout << "Constructor called with T = " << type_name<T>() << std::endl;
    std::cout << "is_signed_v<T> = " << std::is_signed_v<T> << std::endl;
    std::cout << "is_excess_k = " << int128_param_t<Sign, Form>::is_excess_k << std::endl;
}

int main()
{
    // This should instantiate int128_param_t(int)
    int128_ek_t value{100};
    return 0;
}
