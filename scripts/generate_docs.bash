#!/usr/bin/env bash
# Script para generar documentación con Doxygen

set -e  # Salir si hay errores

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "================================================================================"
echo "📚 Generando Documentación con Doxygen"
echo "================================================================================"
echo ""
echo "Directorio del proyecto: $PROJECT_ROOT"
echo ""

# Verificar que Doxygen está instalado
if ! command -v doxygen &> /dev/null; then
    echo "❌ Error: Doxygen no está instalado"
    echo ""
    echo "Instalación:"
    echo "  MSYS2:   pacman -S mingw-w64-ucrt-x86_64-doxygen"
    echo "  Linux:   sudo apt install doxygen graphviz"
    echo "  macOS:   brew install doxygen graphviz"
    echo "  Windows: choco install doxygen.install graphviz"
    exit 1
fi

# Verificar Graphviz (opcional pero recomendado)
if ! command -v dot &> /dev/null; then
    echo "⚠️  Advertencia: Graphviz (dot) no encontrado"
    echo "   Los diagramas de clases no se generarán"
    echo "   Instala: pacman -S mingw-w64-ucrt-x86_64-graphviz"
    echo ""
fi

# Cambiar al directorio raíz del proyecto
cd "$PROJECT_ROOT"

# Crear directorio de salida si no existe
mkdir -p documentation/generated

echo "🔍 Verificando archivos de entrada..."
if [ ! -f "Doxyfile" ]; then
    echo "❌ Error: No se encontró Doxyfile"
    exit 1
fi

if [ ! -d "include" ]; then
    echo "❌ Error: No se encontró el directorio include/"
    exit 1
fi

echo "✅ Archivos de entrada verificados"
echo ""

echo "⚙️  Ejecutando Doxygen..."
echo "--------------------------------------------------------------------------------"

# Ejecutar Doxygen
if doxygen Doxyfile; then
    echo ""
    echo "================================================================================"
    echo "✅ Documentación generada exitosamente"
    echo "================================================================================"
    echo ""
    echo "📂 Ubicación: $PROJECT_ROOT/documentation/generated/html"
    echo ""
    echo "🌐 Para ver la documentación:"
    echo "   1. Abrir en navegador:"
    echo "      file://$(cygpath -w "$PROJECT_ROOT/documentation/generated/html/index.html" 2>/dev/null || echo "$PROJECT_ROOT/documentation/generated/html/index.html")"
    echo ""
    echo "   2. O desde la línea de comandos:"
    echo "      start documentation/generated/html/index.html     (Windows)"
    echo "      open documentation/generated/html/index.html      (macOS)"
    echo "      xdg-open documentation/generated/html/index.html  (Linux)"
    echo ""
    echo "================================================================================"
    
    # Ofrecer abrir automáticamente
    echo ""
    read -p "¿Abrir documentación en el navegador? (s/n): " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[SsYy]$ ]]; then
        if command -v start &> /dev/null; then
            start documentation/generated/html/index.html
        elif command -v open &> /dev/null; then
            open documentation/generated/html/index.html
        elif command -v xdg-open &> /dev/null; then
            xdg-open documentation/generated/html/index.html
        else
            echo "No se pudo abrir automáticamente. Abre manualmente:"
            echo "  $PROJECT_ROOT/documentation/generated/html/index.html"
        fi
    fi
else
    echo ""
    echo "================================================================================"
    echo "❌ Error al generar documentación"
    echo "================================================================================"
    echo ""
    echo "Verifica los errores anteriores y vuelve a intentar."
    echo ""
    exit 1
fi
