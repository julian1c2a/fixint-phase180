// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Example: IPv6 Address handling with uint128_t
// Part of int128 Library - https://github.com/julian1c2a/int128-phase175
// License: BSL-1.0
// =============================================================================
//
// Demonstrates using uint128_t to represent and manipulate IPv6 addresses,
// which are naturally 128-bit values.
//

#include "int128_parameterized.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <array>

using nstd::uint128_t;

// Format a uint128_t as an IPv6 address string (e.g., "2001:0db8:85a3::8a2e:0370:7334")
std::string to_ipv6_string(const uint128_t &addr)
{
    const uint64_t hi = addr.high();
    const uint64_t lo = addr.low();

    // Extract 8 groups of 16 bits each
    std::array<uint16_t, 8> groups{};
    groups[0] = static_cast<uint16_t>((hi >> 48) & 0xFFFF);
    groups[1] = static_cast<uint16_t>((hi >> 32) & 0xFFFF);
    groups[2] = static_cast<uint16_t>((hi >> 16) & 0xFFFF);
    groups[3] = static_cast<uint16_t>(hi & 0xFFFF);
    groups[4] = static_cast<uint16_t>((lo >> 48) & 0xFFFF);
    groups[5] = static_cast<uint16_t>((lo >> 32) & 0xFFFF);
    groups[6] = static_cast<uint16_t>((lo >> 16) & 0xFFFF);
    groups[7] = static_cast<uint16_t>(lo & 0xFFFF);

    std::ostringstream oss;
    for (int i = 0; i < 8; ++i)
    {
        if (i > 0)
        {
            oss << ':';
        }
        oss << std::hex << std::setfill('0') << std::setw(4) << groups[i];
    }
    return oss.str();
}

// Apply a subnet mask (prefix length) to an IPv6 address
uint128_t apply_prefix(const uint128_t &addr, int prefix_len)
{
    if (prefix_len >= 128)
    {
        return addr;
    }
    if (prefix_len <= 0)
    {
        return uint128_t(0ULL);
    }

    // Build mask: top prefix_len bits set to 1
    uint128_t mask(0ULL);
    if (prefix_len >= 64)
    {
        mask = uint128_t(0xFFFFFFFFFFFFFFFFULL, 0ULL);
        const int remaining = prefix_len - 64;
        if (remaining > 0)
        {
            const uint64_t low_mask = ~((1ULL << (64 - remaining)) - 1ULL);
            mask = uint128_t(0xFFFFFFFFFFFFFFFFULL, low_mask);
        }
    }
    else
    {
        const uint64_t high_mask = ~((1ULL << (64 - prefix_len)) - 1ULL);
        mask = uint128_t(high_mask, 0ULL);
    }

    return addr & mask;
}

int main()
{
    std::cout << "=== Example: IPv6 Address Handling ===" << std::endl;
    std::cout << std::endl;

    // Construct an IPv6 address: 2001:0db8:85a3:0000:0000:8a2e:0370:7334
    const uint64_t hi = (0x2001ULL << 48) | (0x0db8ULL << 32) | (0x85a3ULL << 16) | 0x0000ULL;
    const uint64_t lo = (0x0000ULL << 48) | (0x8a2eULL << 32) | (0x0370ULL << 16) | 0x7334ULL;
    const uint128_t ipv6_addr(hi, lo);

    std::cout << "IPv6 address: " << to_ipv6_string(ipv6_addr) << std::endl;
    std::cout << "As integer:   " << ipv6_addr.to_string() << std::endl;
    std::cout << "As hex:       0x" << ipv6_addr.to_string(16) << std::endl;
    std::cout << std::endl;

    // Network prefix /48
    const uint128_t network = apply_prefix(ipv6_addr, 48);
    std::cout << "Network /48:  " << to_ipv6_string(network) << std::endl;

    // Network prefix /64
    const uint128_t subnet = apply_prefix(ipv6_addr, 64);
    std::cout << "Subnet  /64:  " << to_ipv6_string(subnet) << std::endl;
    std::cout << std::endl;

    // Loopback address ::1
    const uint128_t loopback(0ULL, 1ULL);
    std::cout << "Loopback:     " << to_ipv6_string(loopback) << std::endl;

    // All-zeros
    const uint128_t unspecified(0ULL);
    std::cout << "Unspecified:  " << to_ipv6_string(unspecified) << std::endl;
    std::cout << std::endl;

    // Address comparison
    std::cout << "loopback > unspecified: "
              << (loopback > unspecified ? "true" : "false") << std::endl;
    std::cout << "loopback == loopback:   "
              << (loopback == loopback ? "true" : "false") << std::endl;
    std::cout << std::endl;

    std::cout << "[OK] IPv6 example complete." << std::endl;
    return 0;
}
