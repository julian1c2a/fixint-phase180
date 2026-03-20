#!/bin/bash
set -e

# Ensure we are in the project root (assuming script is in scripts/)
cd "$(dirname "$0")/.."

echo "================================================================"
echo "🧪 VERIFYING REFACTORING (data[0] -> data[LSULL])"
echo "================================================================"

# 1. Phase 1.5 Unit Tests (General Functionality)
echo -e "\n[1/4] Compiling and running phase15_uint128_tests..."
g++ -std=c++20 -Iinclude_new tests/phase15_uint128_tests.cpp -o phase15_uint128_tests.exe
./phase15_uint128_tests.exe

echo -e "\n[2/4] Compiling and running phase15_int128_tests..."
g++ -std=c++20 -Iinclude_new tests/phase15_int128_tests.cpp -o phase15_int128_tests.exe
./phase15_int128_tests.exe

# 2. Division Optimization Tests (Heavily refactored area)
echo -e "\n[3/4] Compiling and running test_divrem_optimized_tt..."
g++ -std=c++20 -Iinclude_new test_divrem_optimized_tt.cpp -o test_divrem_optimized_tt.exe
./test_divrem_optimized_tt.exe

# 3. Euclidean Properties (Correctness check)
echo -e "\n[4/4] Compiling and running test_divrem_euclidean_properties..."
g++ -std=c++20 -Iinclude_new test_divrem_euclidean_properties.cpp -o test_divrem_euclidean_properties.exe
./test_divrem_euclidean_properties.exe

echo -e "\n✅ ALL VERIFICATION TESTS PASSED!"