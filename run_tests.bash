#!/bin/bash
# Simple multi-compiler test with file output

cd /c/msys64/ucrt64/home/julian/CppProjects/int128-phase175

mkdir -p build

{
    echo "===== MULTI-COMPILER TEST RESULTS ====="
    echo "Date: $(date)"
    echo ""
    
    # Test GCC -O0
    echo "[1/4] Testing GCC -O0..."
    if command -v g++ &> /dev/null; then
        g++ -std=c++20 -O0 -Iinclude tests/test_divmod_final.cpp -o build/test_gcc_o0.exe 2>&1 && {
            echo "GCC -O0 Compilation: SUCCESS"
            echo "Running GCC -O0 tests:"
            ./build/test_gcc_o0.exe 2>&1
        } || echo "GCC -O0 Compilation: FAILED"
    else
        echo "GCC not found"
    fi
    echo ""
    
    # Test Clang -O2
    echo "[2/4] Testing Clang -O2..."
    if command -v clang++ &> /dev/null; then
        clang++ -std=c++20 -O2 -Iinclude tests/test_divmod_final.cpp -o build/test_clang_o2.exe 2>&1 && {
            echo "Clang -O2 Compilation: SUCCESS"
            echo "Running Clang -O2 tests:"
            ./build/test_clang_o2.exe 2>&1
        } || echo "Clang -O2 Compilation: FAILED"
    else
        echo "Clang not found"
    fi
    echo ""
    
    # Test MSVC
    echo "[3/4] Testing MSVC 2026..."
    if command -v cl.exe &> /dev/null; then
        cl.exe /std:c++latest /O2 /Iinclude /Fe:build/test_msvc.exe tests/test_divmod_final.cpp 2>&1 | head -10 && {
            if [ -f build/test_msvc.exe ]; then
                echo "MSVC Compilation: SUCCESS"
                echo "Running MSVC tests:"
                ./build/test_msvc.exe 2>&1
            else
                echo "MSVC Compilation: FAILED (no .exe)"
            fi
        } || echo "MSVC Compilation: FAILED"
    else
        echo "MSVC cl.exe not found"
    fi
    echo ""
    
    # Test Intel ICX
    echo "[4/4] Testing Intel ICX..."
    if command -v icx &> /dev/null; then
        icx /std:c++20 /O2 /Iinclude /Fe:build/test_intel.exe tests/test_divmod_final.cpp 2>&1 | head -10 && {
            if [ -f build/test_intel.exe ]; then
                echo "Intel ICX Compilation: SUCCESS"
                echo "Running Intel ICX tests:"
                ./build/test_intel.exe 2>&1
            else
                echo "Intel ICX Compilation: FAILED (no .exe)"
            fi
        } || echo "Intel ICX Compilation: FAILED"
    else
        echo "Intel ICX (icx) not found"
    fi
    echo ""
    
    echo "===== END OF RESULTS ====="
} > /c/msys64/ucrt64/home/julian/CppProjects/int128-phase175/test_results.txt 2>&1

echo "Results saved to test_results.txt"
cat /c/msys64/ucrt64/home/julian/CppProjects/int128-phase175/test_results.txt
