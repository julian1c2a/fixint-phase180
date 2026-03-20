#!/usr/bin/env bash
# Build extracted tests for CI/CD
# Usage: ./build_extracted_tests.bash <compiler> <build_type>
#   compiler: gcc, clang, msvc, intel
#   build_type: debug, release

# No set -e aquí, queremos continuar compilando todo
set -o pipefail

COMPILER="${1:-gcc}"
BUILD_TYPE="${2:-release}"

echo "================================"
echo "Building Extracted Tests"
echo "Compiler: $COMPILER"
echo "Build Type: $BUILD_TYPE"
echo "================================"

# Detectar directorio raíz del proyecto
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

# Crear directorio de build
BUILD_DIR="build/build_tests/$COMPILER/$BUILD_TYPE"
mkdir -p "$BUILD_DIR"

# Flags de compilación según tipo
if [ "$BUILD_TYPE" = "debug" ]; then
    OPT_FLAGS="-O0 -g"
else
    OPT_FLAGS="-O3 -DNDEBUG"
fi

# Configurar compilador
case "$COMPILER" in
    gcc)
        CXX="${CXX:-g++}"
        EXTRA_FLAGS="-Wall -Wextra -std=c++20"
        ;;
    clang)
        CXX="${CXX:-clang++}"
        EXTRA_FLAGS="-Wall -Wextra -std=c++20"
        ;;
    msvc)
        CXX="cl"
        # Git Bash convierte /flag a rutas, usar // para escapar
        OPT_FLAGS="//Od //Zi"
        [ "$BUILD_TYPE" = "release" ] && OPT_FLAGS="//O2 //DNDEBUG"
        EXTRA_FLAGS="//std:c++20 //EHsc //W4"
        ;;
    intel)
        CXX="${CXX:-icpx}"
        EXTRA_FLAGS="-Wall -Wextra -std=c++20"
        ;;
    *)
        echo "Unknown compiler: $COMPILER"
        exit 1
        ;;
esac

echo "Using compiler: $CXX"
echo "Optimization flags: $OPT_FLAGS"
echo ""

# Encontrar todos los tests *_extracted_tests.cpp
TEST_FILES=(tests/*_extracted_tests.cpp)

if [ ${#TEST_FILES[@]} -eq 0 ]; then
    echo "⚠️  No test files found matching *_extracted_tests.cpp"
    exit 0
fi

echo "Found ${#TEST_FILES[@]} test files"
echo ""

# Compilar cada test
COMPILED=0
FAILED=0

for test_file in "${TEST_FILES[@]}"; do
    if [ ! -f "$test_file" ]; then
        continue
    fi
    
    test_name=$(basename "$test_file" .cpp)
    output="$BUILD_DIR/${test_name}"
    
    # Flags adicionales según el tipo de test
    extra_libs=""
    if [[ "$test_name" == *"thread_safety"* ]]; then
        extra_libs="-latomic -pthread"
    fi
    
    echo "Compiling: $test_name..."
    
    if [ "$COMPILER" = "msvc" ]; then
        # MSVC: usar archivo de respuesta para evitar conversión de paths por Git Bash
        abs_output=$(cd "$(dirname "${output}")" && pwd -W 2>/dev/null || pwd)
        output_name=$(basename "${output}")
        abs_test=$(cd "$(dirname "$test_file")" && pwd -W 2>/dev/null || pwd)
        test_name_file=$(basename "$test_file")
        abs_include=$(cd include && pwd -W 2>/dev/null || pwd)
        
        # Crear archivo de respuesta temporal
        rsp_file="${output}.rsp"
        if [ "$BUILD_TYPE" = "release" ]; then
            echo "/std:c++20 /EHsc /W4 /O2 /DNDEBUG /I\"$abs_include\" \"$abs_test\\$test_name_file\" /Fe:\"$abs_output\\${output_name}.exe\"" > "$rsp_file"
        else
            echo "/std:c++20 /EHsc /W4 /Od /Zi /I\"$abs_include\" \"$abs_test\\$test_name_file\" /Fe:\"$abs_output\\${output_name}.exe\"" > "$rsp_file"
        fi
        
        # Ejecutar cl.exe con archivo de respuesta
        if cl.exe "@$rsp_file" > "${output}.log" 2>&1; then
            echo "  [OK] Success"
            ((COMPILED++))
        else
            echo "  [FAIL] Failed"
            echo "  === Error log (first 30 lines) ==="
            head -30 "${output}.log"
            echo "  ==================================="
            ((FAILED++))
        fi
    else
        # GCC/Clang/Intel
        if $CXX $EXTRA_FLAGS $OPT_FLAGS -I"include" "$test_file" -o "$output" $extra_libs > "${output}.log" 2>&1; then
            echo "  [OK] Success"
            ((COMPILED++))
        else
            echo "  [FAIL] Failed"
            echo "  === Error log (first 30 lines) ==="
            head -30 "${output}.log"
            echo "  ==================================="
            ((FAILED++))
        fi
    fi
done

echo ""
echo "================================"
echo "Build Summary"
echo "  Compiled: $COMPILED"
echo "  Failed: $FAILED"
echo "================================"

if [ $FAILED -gt 0 ]; then
    echo "[FAIL] Some tests failed to compile"
    exit 1
fi

echo "[OK] All tests compiled successfully"
exit 0
