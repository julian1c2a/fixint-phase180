#!/usr/bin/env bash
# ==============================================================================
# catalog_demos.bash - Generador automático de catálogo de demos
# ==============================================================================
# Genera DEMOS_CATALOG.md con inventario completo de todas las demos disponibles.
# Analiza archivos .cpp y extrae comentarios de descripción.
# 
# Uso:
#   catalog_demos.bash [output_file]
#
# Argumentos:
#   output_file : Archivo de salida (default: DEMOS_CATALOG.md)
#
# Ejemplos:
#   catalog_demos.bash
#   catalog_demos.bash docs/catalog.md
# ==============================================================================

set -euo pipefail

# ========================= Configuration =========================

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

OUTPUT_FILE="${1:-DEMOS_CATALOG.md}"

CATEGORIES=("general" "tutorials" "examples" "showcase" "comparison" "performance" "integration")

# ========================= Color Output =========================

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo_success() { echo -e "${GREEN}[✓] $1${NC}"; }
echo_info() { echo -e "${YELLOW}→ $1${NC}"; }
echo_header() { echo -e "${BLUE}═══ $1 ═══${NC}"; }

# ========================= Helper Functions =========================

# Extrae la primera línea de comentario de un archivo .cpp
extract_description() {
    local file="$1"
    
    # Buscar primera línea con comentario // después de includes
    local desc=$(grep -m 1 -E '^\s*//' "$file" 2>/dev/null | sed 's|^\s*//\s*||' || echo "")
    
    # Si no hay comentario //, buscar comentario /* */
    if [[ -z "$desc" ]]; then
        desc=$(grep -m 1 -E '^\s*/\*' "$file" 2>/dev/null | sed 's|^\s*/\*\s*||' | sed 's|\s*\*/||' || echo "Sin descripción")
    fi
    
    # Si todavía no hay descripción, buscar en las primeras 10 líneas
    if [[ -z "$desc" || "$desc" == "Sin descripción" ]]; then
        desc=$(head -10 "$file" | grep -E '^\s*//' | head -1 | sed 's|^\s*//\s*||' || echo "Sin descripción")
    fi
    
    # Si aún no hay nada, valor por defecto
    if [[ -z "$desc" ]]; then
        desc="Demo de la biblioteca int128"
    fi
    
    echo "$desc"
}

# Cuenta líneas de código (sin comentarios ni líneas vacías)
count_loc() {
    local file="$1"
    local loc=$(grep -cve '^\s*$' -e '^\s*//' -e '^\s*/\*' "$file" 2>/dev/null || echo "0")
    echo "$loc"
}

# Detecta características usadas en el archivo
detect_features() {
    local file="$1"
    local features=""
    
    grep -q "std::atomic" "$file" && features+="thread_safety, "
    grep -q "#include.*algorithm" "$file" && features+="algorithm, "
    grep -q "#include.*numeric" "$file" && features+="numeric, "
    grep -q "#include.*iostream" "$file" && features+="iostreams, "
    grep -q "#include.*format\|std::format" "$file" && features+="format, "
    grep -q "operator<<\|operator>>" "$file" && features+="iostreams, "
    grep -q "std::ranges" "$file" && features+="ranges, "
    grep -q "constexpr" "$file" && features+="constexpr, "
    grep -q "concept\|requires" "$file" && features+="concepts, "
    
    # Eliminar última coma y espacios
    features=$(echo "$features" | sed 's|, $||')
    
    if [[ -z "$features" ]]; then
        features="básico"
    fi
    
    echo "$features"
}

# ========================= Generate Catalog =========================

echo_header "Generando Catálogo de Demos"
echo_info "Archivo: $OUTPUT_FILE"
echo ""

# Crear archivo
cat > "$OUTPUT_FILE" << 'EOF'
# 📚 Catálogo de Demos - int128 Library

> **Generado automáticamente** - No editar manualmente  
> Ejecuta `bash scripts/catalog_demos.bash` para actualizar

Este documento lista todas las demos disponibles organizadas por categoría.

## 📊 Estadísticas Generales

EOF

# ========================= Count Totals =========================

TOTAL_DEMOS=0
TOTAL_LOC=0

for cat in "${CATEGORIES[@]}"; do
    if [[ -d "demos/$cat" ]]; then
        for file in demos/$cat/*.cpp; do
            [[ -f "$file" ]] || continue
            TOTAL_DEMOS=$((TOTAL_DEMOS + 1))
            LOC=$(count_loc "$file")
            TOTAL_LOC=$((TOTAL_LOC + LOC))
        done
    fi
done

cat >> "$OUTPUT_FILE" << EOF
- **Total de demos**: $TOTAL_DEMOS
- **Total de líneas de código**: ~$TOTAL_LOC LOC
- **Categorías**: ${#CATEGORIES[@]}
- **Última actualización**: $(date '+%Y-%m-%d %H:%M:%S')

---

EOF

# ========================= Generate Categories =========================

for cat in "${CATEGORIES[@]}"; do
    DEMO_DIR="demos/$cat"
    
    if [[ ! -d "$DEMO_DIR" ]]; then
        echo_info "Categoría no existe: $cat (omitida)"
        continue
    fi
    
    # Usar glob de bash para obtener lista de demos
    shopt -s nullglob
    DEMO_FILES=($DEMO_DIR/*.cpp)
    shopt -u nullglob
    
    if [[ ${#DEMO_FILES[@]} -eq 0 ]]; then
        echo_info "Sin demos en: $cat (omitida)"
        continue
    fi
    
    echo_info "Procesando: $cat (${#DEMO_FILES[@]} demos)"
    
    # Título de categoría
    case "$cat" in
        general)
            ICON="🔧"
            TITLE="General - Demos Ad-hoc"
            DESC="Demostraciones experimentales y características específicas"
            ;;
        tutorials)
            ICON="🎓"
            TITLE="Tutorials - Paso a Paso"
            DESC="13 tutoriales progresivos desde básico hasta avanzado"
            ;;
        examples)
            ICON="💼"
            TITLE="Examples - Casos de Uso Reales"
            DESC="Aplicaciones prácticas del mundo real"
            ;;
        showcase)
            ICON="🌟"
            TITLE="Showcase - Demos Impresionantes"
            DESC="Presentaciones visuales e interactivas"
            ;;
        comparison)
            ICON="⚖️"
            TITLE="Comparison - Otras Librerías"
            DESC="Comparaciones con Boost, GMP, tipos nativos"
            ;;
        performance)
            ICON="🔬"
            TITLE="Performance - Análisis Profundo"
            DESC="Deep-dive en rendimiento de operaciones"
            ;;
        integration)
            ICON="🔌"
            TITLE="Integration - Ecosistema Externo"
            DESC="Integración con JSON, bases de datos, networking"
            ;;
    esac
    
    cat >> "$OUTPUT_FILE" << EOF
## $ICON $TITLE

**Descripción**: $DESC  
**Ubicación**: [\`demos/$cat/\`](demos/$cat/)  
**Total de demos**: ${#DEMO_FILES[@]}

EOF
    
    # Tabla de demos
    cat >> "$OUTPUT_FILE" << 'EOF'
| Demo | Descripción | LOC | Características |
|------|-------------|-----|-----------------|
EOF
    
    for demo_file in "${DEMO_FILES[@]}"; do
        DEMO_NAME=$(basename "$demo_file" .cpp)
        DESC=$(extract_description "$demo_file")
        LOC=$(count_loc "$demo_file")
        FEATURES=$(detect_features "$demo_file")
        
        # Limpiar descripción para markdown
        DESC=$(echo "$DESC" | sed 's/|/\\|/g')
        
        echo "| [\`$DEMO_NAME.cpp\`](demos/$cat/$DEMO_NAME.cpp) | $DESC | $LOC | $FEATURES |" >> "$OUTPUT_FILE"
    done
    
    echo "" >> "$OUTPUT_FILE"
    
    # Instrucciones rápidas
    cat >> "$OUTPUT_FILE" << EOF
### 🚀 Quick Start - $cat

\`\`\`bash
# Compilar una demo específica
bash scripts/build_demo.bash $cat <demo_name> gcc release

# Compilar todas las demos de esta categoría
bash scripts/build_all_demos.bash $cat gcc release

# Ejecutar una demo
bash scripts/run_demo.bash $cat <demo_name>
\`\`\`

---

EOF
done

# ========================= Footer =========================

cat >> "$OUTPUT_FILE" << 'EOF'
## 🛠️ Comandos Útiles

### Compilación

```bash
# Una demo específica
bash scripts/build_demo.bash <category> <demo_name> [compiler] [mode]

# Todas las demos de una categoría
bash scripts/build_all_demos.bash <category> [compiler] [mode]

# Todas las demos de todas las categorías
bash scripts/build_all_demos.bash all gcc release
```

### Ejecución

```bash
# Ejecutar una demo
bash scripts/run_demo.bash <category> <demo_name> [compiler] [mode] [args...]

# Ejemplo con argumentos
bash scripts/run_demo.bash examples uuid_generation gcc release 100
```

### Testing

```bash
# Verificar que todas compilan
bash scripts/test_demos.bash

# Verificar solo una categoría
bash scripts/test_demos.bash tutorials

# Verificar con compilador específico
bash scripts/test_demos.bash all clang release
```

### Regenerar Catálogo

```bash
# Actualizar este archivo
bash scripts/catalog_demos.bash
```

---

## 📖 Documentación Relacionada

- **README Principal**: [README.md](README.md)
- **Guía de Demos**: [demos/README.md](demos/README.md)
- **Guía de Compilación**: [PROMPT.md](PROMPT.md)
- **Estado del Proyecto**: [PROYECTO_COMPLETO.md](PROYECTO_COMPLETO.md)

---

## 🎯 Recomendaciones

### 🆕 Si eres nuevo:
1. Empieza con [tutorials/01_basic_operations.cpp](demos/tutorials/01_basic_operations.cpp)
2. Sigue los tutoriales en orden secuencial (01 → 13)
3. Experimenta con [examples/](demos/examples/) simples

### 🔧 Si buscas ejemplos prácticos:
1. Explora [examples/](demos/examples/) para casos reales
2. Revisa [showcase/](demos/showcase/) para ver posibilidades
3. Consulta [comparison/](demos/comparison/) para decidir entre librerías

### 🚀 Si necesitas optimización:
1. Analiza [performance/](demos/performance/) para entender benchmarks
2. Estudia algoritmos de división y multiplicación
3. Revisa efectos de caché y branch prediction

### 🔌 Si integras con otros sistemas:
1. Consulta [integration/](demos/integration/) para tu caso de uso
2. Verifica ejemplos de serialización (JSON, Protobuf)
3. Revisa integración con bases de datos

---

**📅 Última actualización**: Generado automáticamente el $(date '+%Y-%m-%d %H:%M:%S')

EOF

# ========================= Success =========================

echo ""
echo_success "Catálogo generado: $OUTPUT_FILE"
echo_info "Total de demos catalogadas: $TOTAL_DEMOS"
echo_info "Total de LOC: ~$TOTAL_LOC"
echo ""
echo_info "Para ver el catálogo:"
echo "  cat $OUTPUT_FILE"
echo "  o abre el archivo en tu editor"
