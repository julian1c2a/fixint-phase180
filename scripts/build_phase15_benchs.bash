#!/usr/bin/env bash
# ==============================================================================
# build_phase15_benchs.bash - Build benchmarks con include_new/ (Fase 1.5)
# ==============================================================================
# Script temporal para compilar benchmarks usando el template unificado int128_base_t
# 
# Uso:
#   build_phase15_benchs.bash [type] [compiler] [mode]
#
# Argumentos:
#   type     : uint128 | int128 | both (default: both)
#   compiler : gcc | clang | intel | msvc (default: gcc)
#   mode     : debug | release (default: release)
#
# Ejemplos:
#   build_phase15_benchs.bash uint128 gcc release
#   build_phase15_benchs.bash int128 clang release
#   build_phase15_benchs.bash both gcc release
# ==============================================================================

set -euo pipefail

# ========================= Parse Arguments =========================

TYPE="${1:-both}"
COMPILER="${2:-gcc}"
MODE="${3:-release}"

# ========================= Validation =========================

if [[ "$TYPE" != "uint128" && "$TYPE" != "int128" && "$TYPE" != "both" ]]; then
    echo "Error: TYPE debe ser 'uint128', 'int128' o 'both'"
    exit 1
fi

VALID_COMPILERS=("gcc" "clang" "intel" "msvc")
if [[ ! " ${VALID_COMPILERS[*]} " =~ " ${COMPILER} " ]]; then
    echo "Error: COMPILER debe ser uno de: ${VALID_COMPILERS[*]}"
    exit 1
fi

VALID_MODES=("debug" "release")
if [[ ! " ${VALID_MODES[*]} " =~ " ${MODE} " ]]; then
    echo "Error: MODE debe ser uno de: ${VALID_MODES[*]}"
    exit 1
fi

# ========================= Configuration =========================

# ⚠️ CRÍTICO: Usar include_new/ en lugar de include/
INCLUDE_DIR="include_new"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_BASE="${PROJECT_ROOT}/build/phase15_benchs"

# ========================= Functions =========================

compile_bench() {
    local type="$1"
    local compiler="$2"
    local mode="$3"
    
    local bench_file="${PROJECT_ROOT}/benchs/${type}_t_extracted_benchs.cpp"
    local output_dir="${OUTPUT_BASE}/${compiler}/${mode}"
    local output_exe="${output_dir}/${type}_t_benchs_${compiler}"
    
    # Verificar que el archivo de benchmark existe
    if [[ ! -f "$bench_file" ]]; then
        echo "⚠️  Benchmark no encontrado: $bench_file"
        return 1
    fi
    
    mkdir -p "$output_dir"
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "🔧 Compilando: ${type} | ${compiler} | ${mode}"
    echo "📂 Headers: ${INCLUDE_DIR}/"
    echo "📝 Fuente: ${bench_file}"
    echo "🎯 Salida: ${output_exe}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    case "$compiler" in
        gcc)
            local flags="-std=c++20 -I${INCLUDE_DIR}"
            if [[ "$mode" == "debug" ]]; then
                flags="$flags -g -O0 -Wall -Wextra"
            else
                flags="$flags -O3 -DNDEBUG -march=native"
            fi
            
            /c/msys64/ucrt64/bin/g++ $flags "$bench_file" -o "${output_exe}.exe"
            ;;
            
        clang)
            local flags="-std=c++20 -I${INCLUDE_DIR}"
            if [[ "$mode" == "debug" ]]; then
                flags="$flags -g -O0 -Wall -Wextra"
            else
                flags="$flags -O3 -DNDEBUG -march=native"
            fi
            
            /c/msys64/clang64/bin/clang++ $flags "$bench_file" -o "${output_exe}.exe"
            ;;
            
        intel)
            echo "🔄 Configurando Intel oneAPI..."
            source "${PROJECT_ROOT}/scripts/setup_intel_combined.bash" x64
            
            local flags="-std=c++20 -I${INCLUDE_DIR}"
            if [[ "$mode" == "debug" ]]; then
                flags="$flags -g -O0"
            else
                flags="$flags -O3 -DNDEBUG -xHost"
            fi
            
            icpx $flags "$bench_file" -o "${output_exe}.exe"
            ;;
            
        msvc)
            echo "🔄 Configurando MSVC..."
            local vcvars="C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat"
            
            if [[ ! -f "$vcvars" ]]; then
                echo "❌ vcvarsall.bat no encontrado en: $vcvars"
                return 1
            fi
            
            local flags="/std:c++20 /I${INCLUDE_DIR}"
            if [[ "$mode" == "debug" ]]; then
                flags="$flags /Od /Zi /MDd"
            else
                flags="$flags /O2 /MD /DNDEBUG /arch:AVX2"
            fi
            
            cmd.exe /C "\"$vcvars\" x64 && cl $flags \"$bench_file\" /Fe:\"${output_exe}.exe\""
            ;;
            
        *)
            echo "❌ Compilador no soportado: $compiler"
            return 1
            ;;
    esac
    
    if [[ $? -eq 0 ]]; then
        echo "✅ Compilación exitosa: ${output_exe}.exe"
        return 0
    else
        echo "❌ Compilación falló"
        return 1
    fi
}

# ========================= Main Logic =========================

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║   FASE 1.5 - BUILD BENCHMARKS (Template Unificado)        ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Configuración:"
echo "  📦 Tipo: $TYPE"
echo "  🔧 Compilador: $COMPILER"
echo "  🎚️  Modo: $MODE (recomendado: release)"
echo "  📂 Headers: $INCLUDE_DIR/"
echo ""

# Determinar qué tipos compilar
TYPES_TO_BUILD=()
if [[ "$TYPE" == "both" ]]; then
    TYPES_TO_BUILD=("uint128" "int128")
else
    TYPES_TO_BUILD=("$TYPE")
fi

# Compilar
SUCCESS_COUNT=0
FAIL_COUNT=0

for type in "${TYPES_TO_BUILD[@]}"; do
    if compile_bench "$type" "$COMPILER" "$MODE"; then
        ((SUCCESS_COUNT++))
    else
        ((FAIL_COUNT++))
    fi
    echo ""
done

# Resumen final
echo "╔════════════════════════════════════════════════════════════╗"
echo "║   RESUMEN DE COMPILACIÓN                                   ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo "✅ Exitosas: $SUCCESS_COUNT"
echo "❌ Fallidas: $FAIL_COUNT"
echo ""

if [[ $FAIL_COUNT -gt 0 ]]; then
    exit 1
fi

exit 0
