#!/bin/bash
set -e

# Ir al directorio raíz del proyecto
cd "$(dirname "$0")/.."

echo "🔧 Corrigiendo estructura del proyecto..."

# 1. Corregir nombre del archivo base si es incorrecto (int64 -> int128)
if [ -f "include_new/int64_base_tt.hpp" ] && [ ! -f "include_new/int128_base_tt.hpp" ]; then
    mv "include_new/int64_base_tt.hpp" "include_new/int128_base_tt.hpp"
    echo "✅ Renombrado: include_new/int64_base_tt.hpp -> include_new/int128_base_tt.hpp"
fi

# 2. Crear el archivo puente int128_base.hpp si no existe
if [ ! -f "include_new/int128_base.hpp" ]; then
    echo "#ifndef INT128_BASE_HPP" > "include_new/int128_base.hpp"
    echo "#define INT128_BASE_HPP" >> "include_new/int128_base.hpp"
    echo "" >> "include_new/int128_base.hpp"
    echo '#include "int128_base_tt.hpp"' >> "include_new/int128_base.hpp"
    echo "" >> "include_new/int128_base.hpp"
    echo "#endif // INT128_BASE_HPP" >> "include_new/int128_base.hpp"
    echo "✅ Creado: include_new/int128_base.hpp"
else
    echo "ℹ️  include_new/int128_base.hpp ya existe"
fi

echo "🏁 Estructura corregida."
