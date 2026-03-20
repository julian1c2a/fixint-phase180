// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
// =============================================================================
// Test: int128_param_thread_safety.hpp - Thread-safe operations
// Part of int128 Library
// License: BSL-1.0
// =============================================================================

#include "int128_param_thread_safety.hpp"
#include <iostream>
#include <thread>
#include <vector>

// Test counter
static int passed{0};
static int failed{0};

#define TEST(name, condition)                         \
    do                                                \
    {                                                 \
        if (condition)                                \
        {                                             \
            ++passed;                                 \
        }                                             \
        else                                          \
        {                                             \
            std::cout << "  [FAIL] " << name << "\n"; \
            ++failed;                                 \
        }                                             \
    } while (0)

int main()
{
    using nstd::atomic_int128_ms_t;
    using nstd::atomic_int128_tc_t;
    using nstd::atomic_uint128_t;
    using nstd::int128_ms_t;
    using nstd::int128_tc_t;
    using nstd::uint128_t;

    std::cout << "====================================================================\n";
    std::cout << "Thread Safety Tests (atomic operations)\n";
    std::cout << "====================================================================\n\n";

    // ========================================================================
    // [Test 1] Atomic load/store
    // ========================================================================
    {
        std::cout << "[Test 1] Atomic load/store:\n";

        atomic_int128_tc_t atomic_val{0, 42};

        // Store new value
        const int128_tc_t new_val{0, 100};
        atomic_val.store(new_val);

        // Load and verify
        const auto loaded{atomic_val.load()};

        TEST("atomic_load_store", loaded.low() == 100 && loaded.high() == 0);
        std::cout << "  [OK] atomic_load_store\n";
    }

    // ========================================================================
    // [Test 2] Atomic exchange
    // ========================================================================
    {
        std::cout << "\n[Test 2] Atomic exchange:\n";

        atomic_int128_tc_t atomic_val{0, 42};

        const int128_tc_t new_val{0, 100};
        const auto old_val{atomic_val.exchange(new_val)};

        const auto current{atomic_val.load()};

        TEST("atomic_exchange_old", old_val.low() == 42);
        TEST("atomic_exchange_new", current.low() == 100);
        std::cout << "  [OK] atomic_exchange (2/2)\n";
    }

    // ========================================================================
    // [Test 3] Compare-and-swap (CAS) success
    // ========================================================================
    {
        std::cout << "\n[Test 3] CAS success:\n";

        atomic_int128_tc_t atomic_val{0, 42};

        int128_tc_t expected{0, 42};
        const int128_tc_t desired{0, 100};

        const bool success{atomic_val.compare_exchange_strong(expected, desired)};
        const auto current{atomic_val.load()};

        TEST("cas_success_return", success);
        TEST("cas_success_value", current.low() == 100);
        std::cout << "  [OK] CAS success (2/2)\n";
    }

    // ========================================================================
    // [Test 4] Compare-and-swap (CAS) failure
    // ========================================================================
    {
        std::cout << "\n[Test 4] CAS failure:\n";

        atomic_int128_tc_t atomic_val{0, 42};

        int128_tc_t expected{0, 99}; // Wrong expected value
        const int128_tc_t desired{0, 100};

        const bool success{atomic_val.compare_exchange_strong(expected, desired)};
        const auto current{atomic_val.load()};

        TEST("cas_failure_return", !success);
        TEST("cas_failure_unchanged", current.low() == 42);
        TEST("cas_failure_expected_updated", expected.low() == 42);
        std::cout << "  [OK] CAS failure (3/3)\n";
    }

    // ========================================================================
    // [Test 5] Fetch-and-add
    // ========================================================================
    {
        std::cout << "\n[Test 5] Fetch-and-add:\n";

        atomic_int128_tc_t atomic_val{0, 10};

        const int128_tc_t delta{0, 5};
        const auto old_val{atomic_val.fetch_add(delta)};
        const auto new_val{atomic_val.load()};

        TEST("fetch_add_old", old_val.low() == 10);
        TEST("fetch_add_new", new_val.low() == 15);
        std::cout << "  [OK] fetch_add (2/2)\n";
    }

    // ========================================================================
    // [Test 6] Fetch-and-sub
    // ========================================================================
    {
        std::cout << "\n[Test 6] Fetch-and-sub:\n";

        atomic_int128_tc_t atomic_val{0, 10};

        const int128_tc_t delta{0, 3};
        const auto old_val{atomic_val.fetch_sub(delta)};
        const auto new_val{atomic_val.load()};

        TEST("fetch_sub_old", old_val.low() == 10);
        TEST("fetch_sub_new", new_val.low() == 7);
        std::cout << "  [OK] fetch_sub (2/2)\n";
    }

    // ========================================================================
    // [Test 7] Fetch-and-bitwise (AND, OR, XOR)
    // ========================================================================
    {
        std::cout << "\n[Test 7] Fetch-and-bitwise:\n";

        // Fetch-and-AND
        atomic_uint128_t atomic_and{0, 0xFF};
        const auto old_and{atomic_and.fetch_and(uint128_t{0, 0x0F})};
        const auto new_and{atomic_and.load()};

        TEST("fetch_and_old", old_and.low() == 0xFF);
        TEST("fetch_and_new", new_and.low() == 0x0F);

        // Fetch-and-OR
        atomic_uint128_t atomic_or{0, 0x0F};
        const auto old_or{atomic_or.fetch_or(uint128_t{0, 0xF0})};
        const auto new_or{atomic_or.load()};

        TEST("fetch_or_old", old_or.low() == 0x0F);
        TEST("fetch_or_new", new_or.low() == 0xFF);

        // Fetch-and-XOR
        atomic_uint128_t atomic_xor{0, 0xFF};
        const auto old_xor{atomic_xor.fetch_xor(uint128_t{0, 0x0F})};
        const auto new_xor{atomic_xor.load()};

        TEST("fetch_xor_old", old_xor.low() == 0xFF);
        TEST("fetch_xor_new", new_xor.low() == 0xF0);

        std::cout << "  [OK] fetch bitwise (6/6)\n";
    }

    // ========================================================================
    // [Test 8] Increment/decrement operators
    // ========================================================================
    {
        std::cout << "\n[Test 8] Increment/decrement:\n";

        atomic_int128_tc_t atomic_val{0, 10};

        // Prefix increment
        const auto pre_inc{++atomic_val};
        TEST("pre_increment", pre_inc.low() == 11);

        // Postfix increment
        const auto post_inc{atomic_val++};
        const auto after_post_inc{atomic_val.load()};
        TEST("post_increment_old", post_inc.low() == 11);
        TEST("post_increment_new", after_post_inc.low() == 12);

        // Prefix decrement
        const auto pre_dec{--atomic_val};
        TEST("pre_decrement", pre_dec.low() == 11);

        // Postfix decrement
        const auto post_dec{atomic_val--};
        const auto after_post_dec{atomic_val.load()};
        TEST("post_decrement_old", post_dec.low() == 11);
        TEST("post_decrement_new", after_post_dec.low() == 10);

        std::cout << "  [OK] increment/decrement (6/6)\n";
    }

    // ========================================================================
    // [Test 9] Compound assignment operators
    // ========================================================================
    {
        std::cout << "\n[Test 9] Compound assignment:\n";

        atomic_uint128_t atomic_val{0, 100};

        // Add-assign
        atomic_val += uint128_t{0, 10};
        TEST("add_assign", atomic_val.load().low() == 110);

        // Subtract-assign
        atomic_val -= uint128_t{0, 5};
        TEST("sub_assign", atomic_val.load().low() == 105);

        // AND-assign
        atomic_val &= uint128_t{0, 0xFF};
        TEST("and_assign", atomic_val.load().low() == 105);

        // OR-assign
        atomic_val |= uint128_t{0, 0xF0};
        TEST("or_assign", (atomic_val.load().low() & 0xF0) == 0xF0);

        // XOR-assign
        atomic_val ^= uint128_t{0, 0x0F};
        const auto xor_result{atomic_val.load().low()};
        TEST("xor_assign", xor_result != 105); // Value changed

        std::cout << "  [OK] compound assignment (5/5)\n";
    }

    // ========================================================================
    // [Test 10] Free function API
    // ========================================================================
    {
        std::cout << "\n[Test 10] Free function API:\n";

        atomic_int128_tc_t atomic_val{0, 42};

        // atomic_load
        const auto loaded{nstd::atomic_load(&atomic_val)};
        TEST("free_atomic_load", loaded.low() == 42);

        // atomic_store
        nstd::atomic_store(&atomic_val, int128_tc_t{0, 100});
        TEST("free_atomic_store", atomic_val.load().low() == 100);

        // atomic_exchange
        const auto exchanged{nstd::atomic_exchange(&atomic_val, int128_tc_t{0, 200})};
        TEST("free_atomic_exchange_old", exchanged.low() == 100);
        TEST("free_atomic_exchange_new", atomic_val.load().low() == 200);

        // atomic_compare_exchange_strong
        int128_tc_t expected{0, 200};
        const bool cas_success{nstd::atomic_compare_exchange_strong(
            &atomic_val, &expected, int128_tc_t{0, 300})};
        TEST("free_cas", cas_success && atomic_val.load().low() == 300);

        // atomic_fetch_add
        const auto fetch_add_old{nstd::atomic_fetch_add(&atomic_val, int128_tc_t{0, 50})};
        TEST("free_fetch_add_old", fetch_add_old.low() == 300);
        TEST("free_fetch_add_new", atomic_val.load().low() == 350);

        std::cout << "  [OK] free function API (7/7)\n";
    }

    // ========================================================================
    // [Test 11] Multi-threaded increment (race condition test)
    // ========================================================================
    {
        std::cout << "\n[Test 11] Multi-threaded increment:\n";

        atomic_int128_tc_t counter{0, 0};
        constexpr int num_threads{4};
        constexpr int increments_per_thread{1000};

        std::vector<std::thread> threads{};
        threads.reserve(num_threads);

        for (int i = 0; i < num_threads; ++i)
        {
            threads.emplace_back([&counter]()
                                 {
                for (int j = 0; j < increments_per_thread; ++j) {
                    ++counter;
                } });
        }

        for (auto &t : threads)
        {
            t.join();
        }

        const auto final_value{counter.load()};
        const uint64_t expected{num_threads * increments_per_thread};

        TEST("multithreaded_increment", final_value.low() == expected);
        std::cout << "  [OK] multithreaded increment (counter = " << final_value.low() << ")\n";
    }

    // ========================================================================
    // [Test 12] Multi-threaded CAS loop (lock-free counter)
    // ========================================================================
    {
        std::cout << "\n[Test 12] Multi-threaded CAS loop:\n";

        atomic_int128_tc_t counter{0, 0};
        constexpr int num_threads{4};
        constexpr int increments_per_thread{500};

        std::vector<std::thread> threads{};
        threads.reserve(num_threads);

        for (int i = 0; i < num_threads; ++i)
        {
            threads.emplace_back([&counter]()
                                 {
                for (int j = 0; j < increments_per_thread; ++j) {
                    int128_tc_t expected = counter.load();
                    int128_tc_t desired;
                    do {
                        desired = expected + int128_tc_t{0, 1};
                    } while (!counter.compare_exchange_weak(expected, desired));
                } });
        }

        for (auto &t : threads)
        {
            t.join();
        }

        const auto final_value{counter.load()};
        const uint64_t expected{num_threads * increments_per_thread};

        TEST("multithreaded_cas_loop", final_value.low() == expected);
        std::cout << "  [OK] multithreaded CAS loop (counter = " << final_value.low() << ")\n";
    }

    // ========================================================================
    // [Test 13] MS representation atomic operations
    // ========================================================================
    {
        std::cout << "\n[Test 13] MS representation atomics:\n";

        atomic_int128_ms_t atomic_ms{0, 42};

        // Load/store
        atomic_ms.store(int128_ms_t{0, 100});
        const auto loaded{atomic_ms.load()};
        TEST("ms_load_store", loaded.low() == 100);

        // Fetch-add
        atomic_ms.fetch_add(int128_ms_t{0, 50});
        const auto after_add{atomic_ms.load()};
        TEST("ms_fetch_add", after_add.low() == 150);

        // Increment
        ++atomic_ms;
        const auto after_inc{atomic_ms.load()};
        TEST("ms_increment", after_inc.low() == 151);

        std::cout << "  [OK] MS representation atomics (3/3)\n";
    }

    // ========================================================================
    // [Test 14] Lock-free query
    // ========================================================================
    {
        std::cout << "\n[Test 14] Lock-free query:\n";

        const bool lock_free{atomic_int128_tc_t::is_lock_free()};
        const bool always_lock_free{atomic_int128_tc_t::is_always_lock_free()};

        // This implementation is mutex-based, so should return false
        TEST("is_lock_free", !lock_free);
        TEST("is_always_lock_free", !always_lock_free);

        std::cout << "  [OK] lock-free query (mutex-based, returns false)\n";
    }

    // ========================================================================
    // Results
    // ========================================================================
    std::cout << "\n====================================================================\n";
    std::cout << "RESULTS:\n";
    std::cout << "  Passed: " << passed << "\n";
    std::cout << "  Failed: " << failed << "\n";
    std::cout << "  Total:  " << (passed + failed) << "\n";
    std::cout << "====================================================================\n";

    return (failed == 0) ? 0 : 1;
}
