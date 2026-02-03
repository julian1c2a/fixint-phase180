#!/bin/bash
# =============================================================================
# Compile and run all tests for Phase 1.75
# =============================================================================

INCLUDE_DIR="include"
TEST_DIR="tests"
BUILD_DIR="build"
CXX="g++"
CXXFLAGS="-std=c++20 -O2 -I$INCLUDE_DIR"

# Create build directory
mkdir -p "$BUILD_DIR"

# List of all test files
TEST_FILES=(
    "test_priority1_constructors.cpp"
    "test_priority2_magnitude_sign.cpp"
    "test_priority3_representations_ms_ek.cpp"
    "test_priority4_arithmetic.cpp"
    "test_priority5_string_io.cpp"
    "test_priority6_bitwise.cpp"
    "test_priority7_shift.cpp"
    "test_priority8_bitops.cpp"
    "test_priority9_friends.cpp"
    "test_priority10_float.cpp"
    "test_priority11_array.cpp"
    "test_param_bits.cpp"
    "test_param_cmath.cpp"
    "test_param_limits.cpp"
    "test_param_numeric.cpp"
    "test_param_iostreams.cpp"
    "test_native_arithmetic.cpp"
    "test_excess_k_basic.cpp"
    "test_excess_k_comparison.cpp"
    "test_ms_storage.cpp"
    "test_representation_conversions.cpp"
)

# Counters
TOTAL=0
PASSED=0
FAILED=0

echo "======================================================================="
echo "Phase 1.75 - Full Test Suite"
echo "======================================================================="
echo ""

for test_file in "${TEST_FILES[@]}"; do
    test_name="${test_file%.cpp}"
    exe_file="$BUILD_DIR/${test_name}.exe"
    
    echo "Compiling $test_file..."
    if $CXX $CXXFLAGS "$TEST_DIR/$test_file" -o "$exe_file" 2>&1 | grep -i "error"; then
        echo "  [COMPILE ERROR] $test_name"
        ((FAILED++))
        ((TOTAL++))
        echo ""
        continue
    fi
    
    echo "Running $test_name..."
    if "./$exe_file" > /dev/null 2>&1; then
        echo "  [PASS] $test_name"
        ((PASSED++))
    else
        echo "  [FAIL] $test_name"
        "./$exe_file"
        ((FAILED++))
    fi
    ((TOTAL++))
    echo ""
done

echo "======================================================================="
echo "RESULTS:"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"
echo "  Total:  $TOTAL"
echo "======================================================================="

if [ $FAILED -eq 0 ]; then
    exit 0
else
    exit 1
fi
