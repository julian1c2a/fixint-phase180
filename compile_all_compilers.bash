#!/bin/bash
# =============================================================================
# Multi-Compiler Test Suite: GCC, Clang, MSVC, Intel
# =============================================================================
# Propósito: Compilar y testear division algorithm con todos los compiladores
# Uso: bash compile_all_compilers.bash
# =============================================================================

set -e

PROJECT_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
TEST_FILE="tests/test_divmod_final.cpp"
BUILD_DIR="build"
INCLUDE_DIR="include"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

cd "$PROJECT_ROOT"

echo -e "${BLUE}========================================================================${NC}"
echo -e "${BLUE}  Multi-Compiler Division Test Suite${NC}"
echo -e "${BLUE}========================================================================${NC}"
echo ""

# Ensure build directory exists
mkdir -p "$BUILD_DIR"

# =============================================================================
# GCC -O0 (Baseline)
# =============================================================================
echo -e "${YELLOW}[1/4] GCC -O0 (Baseline - No Optimization)${NC}"
if command -v g++ &> /dev/null; then
    g++ -std=c++20 -O0 -I"$INCLUDE_DIR" "$TEST_FILE" -o "$BUILD_DIR/test_gcc_o0.exe"
    echo -e "${GREEN}[OK]${NC} Compilation successful"
    echo "[RUNNING]"
    "$BUILD_DIR/test_gcc_o0.exe"
    echo ""
else
    echo -e "${RED}[SKIP]${NC} g++ not found in PATH"
    echo ""
fi

# =============================================================================
# Clang -O2 (Optimized)
# =============================================================================
echo -e "${YELLOW}[2/4] Clang -O2 (Optimized)${NC}"
if command -v clang++ &> /dev/null; then
    clang++ -std=c++20 -O2 -I"$INCLUDE_DIR" "$TEST_FILE" -o "$BUILD_DIR/test_clang_o2.exe"
    echo -e "${GREEN}[OK]${NC} Compilation successful"
    echo "[RUNNING]"
    "$BUILD_DIR/test_clang_o2.exe"
    echo ""
else
    echo -e "${RED}[SKIP]${NC} clang++ not found in PATH"
    echo ""
fi

# =============================================================================
# MSVC 2026
# =============================================================================
echo -e "${YELLOW}[3/4] MSVC 2026 (Windows Native Compiler)${NC}"
if command -v cl.exe &> /dev/null; then
    # MSVC expects Windows paths with backslashes
    TEST_FILE_WIN=$(echo "$TEST_FILE" | sed 's/\//\\/g')
    BUILD_EXE_WIN=$(echo "$BUILD_DIR/test_msvc.exe" | sed 's/\//\\/g')
    
    cl.exe /std:c++latest /O2 /I"$INCLUDE_DIR" /Fe:"$BUILD_EXE_WIN" "$TEST_FILE_WIN" 2>&1 | head -20
    
    if [ -f "$BUILD_DIR/test_msvc.exe" ]; then
        echo -e "${GREEN}[OK]${NC} Compilation successful"
        echo "[RUNNING]"
        "$BUILD_DIR/test_msvc.exe"
        echo ""
    else
        echo -e "${RED}[FAIL]${NC} Compilation failed"
        echo ""
    fi
else
    echo -e "${RED}[SKIP]${NC} cl.exe (MSVC) not found in PATH"
    echo "         Try: source legacy-code/int128-phase166/scripts/env_setup/setup_msvc_env.bash"
    echo ""
fi

# =============================================================================
# Intel ICX (Intel oneAPI Compiler)
# =============================================================================
echo -e "${YELLOW}[4/4] Intel ICX (Intel oneAPI Compiler)${NC}"
if command -v icx &> /dev/null; then
    TEST_FILE_WIN=$(echo "$TEST_FILE" | sed 's/\//\\/g')
    BUILD_EXE_WIN=$(echo "$BUILD_DIR/test_intel.exe" | sed 's/\//\\/g')
    
    icx /std:c++20 /O2 /I"$INCLUDE_DIR" /Fe:"$BUILD_EXE_WIN" "$TEST_FILE_WIN" 2>&1 | head -20
    
    if [ -f "$BUILD_DIR/test_intel.exe" ]; then
        echo -e "${GREEN}[OK]${NC} Compilation successful"
        echo "[RUNNING]"
        "$BUILD_DIR/test_intel.exe"
        echo ""
    else
        echo -e "${RED}[FAIL]${NC} Compilation failed"
        echo ""
    fi
else
    echo -e "${RED}[SKIP]${NC} icx (Intel oneAPI) not found in PATH"
    echo "         Try: source legacy-code/int128-phase166/scripts/env_setup/setup_intel_env.bash"
    echo ""
fi

# =============================================================================
# Summary
# =============================================================================
echo -e "${BLUE}========================================================================${NC}"
echo -e "${BLUE}  Test Summary${NC}"
echo -e "${BLUE}========================================================================${NC}"
echo ""
echo "Test file: $TEST_FILE"
echo "Build directory: $BUILD_DIR"
echo "Include directory: $INCLUDE_DIR"
echo ""
echo "Generated executables:"
ls -lh "$BUILD_DIR"/test_*.exe 2>/dev/null || echo "  (No executables compiled)"
echo ""
echo -e "${GREEN}✓ All available compilers tested${NC}"
echo ""
