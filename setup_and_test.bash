#!/bin/bash
# Setup script para MSVC e Intel, luego ejecutar tests

export MSVC_VER="14.50.35717"
export KIT_VER="10.0.26100.0"

# MSVC paths
export VS_ROOT="/c/Program Files/Microsoft Visual Studio/18/Community"
export KIT_ROOT="/c/Program Files (x86)/Windows Kits/10"

export MSVC_BIN="$VS_ROOT/VC/Tools/MSVC/$MSVC_VER/bin/Hostx64/x64"
export KIT_BIN="$KIT_ROOT/bin/$KIT_VER/x64"

# Add to PATH
export PATH="$MSVC_BIN:$KIT_BIN:$PATH"

# Intel oneAPI setup
INTEL_ROOT="/c/Program Files (x86)/Intel/oneAPI"
if [ -f "$INTEL_ROOT/setvars.sh" ]; then
    source "$INTEL_ROOT/setvars.sh" &> /dev/null || true
fi

# Verify MSVC
if command -v cl.exe &> /dev/null; then
    echo "[OK] MSVC cl.exe found in PATH"
else
    echo "[WARN] MSVC cl.exe not found - trying to add explicitly"
    export PATH="$MSVC_BIN:$PATH"
fi

# Verify Intel
if command -v icx &> /dev/null; then
    echo "[OK] Intel ICX found in PATH"
else
    echo "[WARN] Intel ICX not found in PATH"
fi

# Run multi-compiler tests
echo ""
echo "Running multi-compiler test suite..."
echo ""
cd /c/msys64/ucrt64/home/julian/CppProjects/int128-phase175
python3 multi_compiler_test.py

# Read and display results
if [ -f compiler_results.txt ]; then
    echo ""
    echo "=== RESULTS FILE CONTENTS ==="
    cat compiler_results.txt
fi
