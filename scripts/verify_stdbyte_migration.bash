#!/bin/bash
# Compilación directa del demo después de migración std::byte

cd /c/msys64/ucrt64/home/julian/CppProjects/int128-phase166 || exit 1

echo "====================================================================="
echo "   VERIFICACIÓN: Compilación después de migración a std::byte"
echo "====================================================================="
echo ""

# Crear directorio de salida
mkdir -p build/build_demos/gcc/release

# Compilar con g++
echo "[*] Compilando demo_bytes_bitset.cpp con g++ (release)..."
/c/msys64/ucrt64/bin/g++ -std=c++20 -O2 -Wall -Wextra -Iinclude \
    demos/general/demo_bytes_bitset.cpp \
    -o build/build_demos/gcc/release/demo_bytes_bitset

if [ $? -eq 0 ]; then
    echo "[OK] Compilación exitosa"
    echo ""
    echo "Ejecutando demo..."
    echo "====================================================================="
    ./build/build_demos/gcc/release/demo_bytes_bitset
    exit_code=$?
    echo "====================================================================="
    if [ $exit_code -eq 0 ]; then
        echo "[OK] Demo ejecutado correctamente"
    else
        echo "[FAIL] Demo terminó con exit code $exit_code"
    fi
else
    echo "[FAIL] Error de compilación"
    exit 1
fi
