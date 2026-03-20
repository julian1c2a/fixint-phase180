#!/usr/bin/env bash
# Run extracted tests for CI/CD
# Usage: ./run_uint128_extracted_tests.bash <compiler> <build_type>
#   compiler: gcc, clang, msvc, intel
#   build_type: debug, release

# No set -e aquí, queremos continuar ejecutando todos los tests
set -o pipefail

COMPILER="${1:-gcc}"
BUILD_TYPE="${2:-release}"

echo "================================"
echo "Running Extracted Tests"
echo "Compiler: $COMPILER"
echo "Build Type: $BUILD_TYPE"
echo "================================"

# Detectar directorio raíz del proyecto
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Directorio de binarios
BUILD_DIR="build/build_tests/$COMPILER/$BUILD_TYPE"

if [ ! -d "$BUILD_DIR" ]; then
    echo "[FAIL] Build directory not found: $BUILD_DIR"
    echo "Run build_extracted_tests.bash first"
    exit 1
fi

# Encontrar todos los ejecutables
if [ "$COMPILER" = "msvc" ]; then
    TEST_BINARIES=("$BUILD_DIR"/*_extracted_tests.exe)
else
    # En Linux: sin extensión, en Windows/Cygwin: .exe
    if [ -f "$BUILD_DIR"/int128_algorithm_extracted_tests.exe ]; then
        TEST_BINARIES=("$BUILD_DIR"/*_extracted_tests.exe)
    else
        TEST_BINARIES=("$BUILD_DIR"/*_extracted_tests)
    fi
fi

if [ ${#TEST_BINARIES[@]} -eq 0 ]; then
    echo "⚠️  No test binaries found in $BUILD_DIR"
    exit 0
fi

echo "Found ${#TEST_BINARIES[@]} test binaries"
echo ""

# Ejecutar cada test
PASSED=0
FAILED=0

for test_bin in "${TEST_BINARIES[@]}"; do
    if [ ! -f "$test_bin" ]; then
        continue
    fi
    
    if [ ! -x "$test_bin" ] && [ "$COMPILER" != "msvc" ]; then
        chmod +x "$test_bin"
    fi
    
    test_name=$(basename "$test_bin")
    echo "Running: $test_name..."
    
    if "$test_bin" > "${test_bin}.output" 2>&1; then
        echo "  [OK] Passed"
        ((PASSED++))
    else
        EXIT_CODE=$?
        echo "  [FAIL] Failed (exit code: $EXIT_CODE)"
        echo "  Output:"
        cat "${test_bin}.output" | head -50
        ((FAILED++))
    fi
done

echo ""
echo "================================"
echo "Test Results"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"
echo "================================"

if [ $FAILED -gt 0 ]; then
    echo "[FAIL] Some tests failed"
    exit 1
fi

echo "[OK] All tests passed"
exit 0
