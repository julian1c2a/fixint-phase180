# AI-GUIDE.md — Guía Técnica para IA y Desarrollador de Proyectos C/C++

> **Propósito:** Plantilla reutilizable para proyectos de bibliotecas C++ moderno (C++20+),
> con soporte multi-compilador, multi-plataforma, CI/CD integrado, y organización estricta de directorios.
>
> **Audiencia:** Agentes IA (Copilot, Gemini, Claude, etc.) y desarrolladores humanos.
>
> **Versión:** 1.1.0 — 17 marzo 2026

### Guías Relacionadas

| Documento | Ruta | Descripción |
|---|---|---|
| **ai-instructions.md** | [`AI_PROMPT/ai-instructions.md`](AI_PROMPT/ai-instructions.md) | Reglas críticas para agentes IA (rutas, workflow, std::byte, ASCII) |
| **Explicación del Proyecto** | [`AI_PROMPT/GENERAL_GUIDES/Explicación_del_Proyecto.md`](AI_PROMPT/GENERAL_GUIDES/Explicaci%C3%B3n_del_Proyecto.md) | 12 objetivos y 9 etapas del proyecto |
| **CI Workflow** | [`AI_PROMPT/workflow/ci.yml`](AI_PROMPT/workflow/ci.yml) | Build + test multi-compilador (push/PR) |
| **Benchmarks Workflow** | [`AI_PROMPT/workflow/benchmarks.yml`](AI_PROMPT/workflow/benchmarks.yml) | Suite completa de benchmarks (semanal) |
| **Release Workflow** | [`AI_PROMPT/workflow/release.yml`](AI_PROMPT/workflow/release.yml) | Empaquetado de release (tag v*.*.*) |

---

## Tabla de Contenidos

1. [Filosofía y Reglas Fundamentales](#1-filosofía-y-reglas-fundamentales)
2. [Estructura de Directorios](#2-estructura-de-directorios)
3. [Directorio `include/`](#3-directorio-include)
4. [Directorio `tests/`](#4-directorio-tests)
5. [Directorio `benchs/`](#5-directorio-benchs)
6. [Directorio `demos/`](#6-directorio-demos)
7. [Directorio `docs/`](#7-directorio-docs)
8. [Directorio `scripts/`](#8-directorio-scripts)
9. [Directorio `build/`](#9-directorio-build)
10. [Directorio `debugging/`](#10-directorio-debugging)
11. [Directorio `legacy-code/`](#11-directorio-legacy-code)
12. [Archivos del Directorio Raíz](#12-archivos-del-directorio-raíz)
13. [Sistema de Build — Jerarquía de 4 Capas](#13-sistema-de-build--jerarquía-de-4-capas)
14. [Compiladores, Versiones y Plataformas](#14-compiladores-versiones-y-plataformas)
15. [Sanitizers y Análisis Estático](#15-sanitizers-y-análisis-estático)
16. [CI/CD — GitHub Actions Workflows](#16-cicd--github-actions-workflows)
17. [Convenciones de Nombrado de Archivos](#17-convenciones-de-nombrado-de-archivos)
18. [Estándares de Codificación C++](#18-estándares-de-codificación-c)
19. [Flujo de Trabajo del Desarrollador](#19-flujo-de-trabajo-del-desarrollador)
20. [Reglas para Agentes IA](#20-reglas-para-agentes-ia)

---

## 1. Filosofía y Reglas Fundamentales

### Principio de Orden Estricto

**Ningún archivo debe existir fuera de su directorio asignado.**

| Tipo de archivo | Ubicación obligatoria | NUNCA en... |
|---|---|---|
| Headers (.hpp, .h) | `include/` | raíz, tests/, build/ |
| Tests (.cpp de test) | `tests/` | raíz, include/, demos/ |
| Benchmarks (.cpp de bench) | `benchs/` | raíz, tests/ |
| Demos (.cpp de demo) | `demos/` | raíz, tests/ |
| Documentación (.md para usuarios/mantenedor) | `docs/` | raíz (excepto README.md, CHANGELOG.md, LICENSE) |
| Scripts (Python, Bash, Bat) | `scripts/` | raíz (excepto make.py, Makefile, CMakeLists.txt) |
| Artefactos de compilación (.exe, .obj, .o, .pdb, .a) | `build/` | raíz, include/, tests/ |
| Código de depuración temporal | `debugging/` | raíz, tests/ |
| Código base de referencia (inmutable) | `legacy-code/` | cualquier otro sitio |

### Regla de la Raíz Limpia

En el directorio raíz **solo** se permiten:

- **Configuración de build:** `CMakeLists.txt`, `Makefile`, `make.py`, `conanfile.txt`, `Doxyfile`
- **Metadatos del proyecto:** `README.md`, `CHANGELOG.md`, `LICENSE`, `.gitignore`
- **Instrucciones IA:** `AI-GUIDE.md`, `.github/copilot-instructions.md`
- **Directorios:** los listados en la sección 2

**Prohibido en raíz:** `*.exe`, `*.obj`, `*.o`, `*.pdb`, `*.a`, `*.lib`, `*.so`, `*.dylib`,
archivos `.cpp` sueltos, archivos `.md` de sesión/estado temporal.

---

## 2. Estructura de Directorios

```
proyecto/
|
+-- include/              # Headers de la biblioteca (API pública)
|   +-- intrinsics/       # Operaciones de bajo nivel por compilador/plataforma
|   +-- detail/           # Headers internos (no API pública)
|
+-- tests/                # Tests unitarios (uno por módulo/feature)
|   +-- CMakeLists.txt    # Autodescubrimiento de test_*.cpp
|
+-- benchs/               # Benchmarks de rendimiento
|   +-- CMakeLists.txt    # Autodescubrimiento de benchmark_*.cpp / bench_*.cpp
|
+-- demos/                # Programas de ejemplo y tutoriales
|   +-- tutorials/        # Aprendizaje paso a paso
|   +-- examples/         # Casos de uso reales
|   +-- showcase/         # Características avanzadas
|   +-- comparison/       # vs otras bibliotecas
|   +-- performance/      # Demos de rendimiento
|   +-- integration/      # Integración con otras libs
|   +-- CMakeLists.txt
|
+-- docs/                 # Documentación del proyecto
|   +-- api/              # Referencia API (estilo cppreference)
|   +-- guides/           # Guías para desarrolladores
|   +-- ai-prompts/       # Instrucciones y contexto para agentes IA
|   +-- generated/        # Salida de Doxygen (en .gitignore)
|
+-- scripts/              # Scripts de build, CI, utilidades
|   +-- env_setup/        # Configuración de entornos de compilador
|   +-- wsl/              # Scripts específicos para WSL
|   +-- legacy/           # Scripts obsoletos (referencia)
|
+-- build/                # Artefactos de compilación (en .gitignore)
|   +-- build_tests/      # Ejecutables de tests
|   |   +-- gcc/
|   |   |   +-- debug/
|   |   |   +-- release/
|   |   +-- clang/
|   |   +-- msvc/
|   |   +-- intel/
|   +-- build_benchs/     # Ejecutables de benchmarks
|   |   +-- [compiler]/[mode]/
|   +-- build_demos/      # Ejecutables de demos
|   |   +-- [compiler]/[mode]/
|   +-- CMakeFiles/        # Cache de CMake
|   +-- build.ninja        # Archivo Ninja generado
|
+-- debugging/            # Área de trabajo temporal para depuración
|   +-- src/              # Código fuente temporal de debug
|   +-- build/            # Compilaciones temporales de debug
|
+-- legacy-code/          # Código base de referencia (INMUTABLE, no git-tracked)
|
+-- cmake/                # Módulos CMake del proyecto
|   +-- CompilerDetection.cmake
|   +-- SanitizerConfig.cmake
|   +-- StaticAnalysis.cmake
|   +-- TestConfig.cmake
|
+-- .github/              # GitHub Actions, Copilot instructions
|   +-- workflows/
|   |   +-- ci.yml
|   |   +-- benchmarks.yml
|   |   +-- release.yml
|   +-- copilot-instructions.md
|
+-- CMakeLists.txt        # Configuración raíz de CMake
+-- Makefile              # Interfaz Make (delega a scripts/)
+-- make.py               # Orquestador principal Python
+-- Doxyfile              # Configuración Doxygen
+-- conanfile.txt         # Dependencias Conan (opcional)
+-- README.md             # Presentación del proyecto
+-- CHANGELOG.md          # Registro de cambios
+-- LICENSE               # Licencia (BSL-1.0, MIT, etc.)
+-- AI-GUIDE.md           # Este archivo
+-- .gitignore
```

---

## 3. Directorio `include/`

Contiene **exclusivamente** headers (.hpp) de la biblioteca. Es lo que el usuario final instala.

### Reglas

- **Header-only:** si la biblioteca es header-only, todo el código compilable está aquí.
- **Subdirectorio `intrinsics/`:** operaciones que varían por compilador/plataforma
  (detección de compilador, intrínsecos de CPU, fallbacks portables).
- **Subdirectorio `detail/`:** implementaciones internas no destinadas al usuario final.
  Usar namespace `::detail` o `::internal`.
- **Sin archivos `.cpp`** en este directorio (excepto si hay partes compiladas con un `.cpp` en `src/`).
- **Sin archivos de test** — ni un solo `main()` debe existir aquí.
- **Include guards o `#pragma once`:** obligatorio en todo header.

### Estructura típica

```
include/
+-- mi_biblioteca.hpp                  # Header principal (incluye todo)
+-- mi_biblioteca_core.hpp             # Tipo central
+-- mi_biblioteca_algorithm.hpp        # Algoritmos
+-- mi_biblioteca_traits.hpp           # Type traits
+-- mi_biblioteca_format.hpp           # std::format support
+-- intrinsics/
|   +-- compiler_detection.hpp         # Macros de detección
|   +-- arithmetic_operations.hpp      # Multiplicación/división por hardware
|   +-- bit_operations.hpp             # CLZ, CTZ, popcount
+-- detail/
    +-- internal_helpers.hpp           # Funciones auxiliares internas
```

### Naming de headers

| Patrón | Propósito |
|---|---|
| `mi_lib.hpp` | Header principal (umbrella) |
| `mi_lib_[feature].hpp` | Módulo de feature |
| `mi_lib_traits.hpp` | Type traits y conceptos |
| `mi_lib_traits_specializations.hpp` | Especializaciones de std::numeric_limits, etc. |

---

## 4. Directorio `tests/`

Contiene **exclusivamente** archivos de test unitario.

### Reglas

- **Un archivo por módulo/feature:** `test_[feature].cpp`
- **Autodescubrimiento CMake:** el `CMakeLists.txt` interno usa `file(GLOB ...)` patrón `test_*.cpp`
- **Cada test es un ejecutable independiente** — no dependen entre sí.
- **Sin headers propios** — los tests incluyen directamente desde `include/`.
- **Convención de salida:**
  - `[OK]` para test pasado (ASCII, sin Unicode)
  - `[FAIL]` para test fallido
  - Código de salida 0 = éxito, != 0 = fallo

### Estructura

```
tests/
+-- CMakeLists.txt
+-- test_constructors.cpp
+-- test_arithmetic.cpp
+-- test_bitwise.cpp
+-- test_comparison.cpp
+-- test_string_io.cpp
+-- test_format.cpp
+-- test_traits.cpp
+-- test_thread_safety.cpp
```

### CMakeLists.txt de tests (plantilla)

```cmake
file(GLOB TEST_SOURCES "test_*.cpp")

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_features(${TEST_NAME} PRIVATE cxx_std_20)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
```

### Ejecución

```bash
# Via workflow (CORRECTO)
python make.py build [type] [feature] tests [compiler] [mode]
python make.py check [type] [feature] [compiler] [mode]

# Via CTest (desde build/)
ctest --output-on-failure
```

---

## 5. Directorio `benchs/`

Contiene **exclusivamente** benchmarks de rendimiento.

### Reglas

- **Patrón:** `benchmark_[feature].cpp` o `bench_[feature].cpp`
- **Autodescubrimiento CMake:** patrón `benchmark_*.cpp` y `bench_*.cpp`
- **Framework de medición:**
  - `std::chrono::high_resolution_clock` para timing
  - Fase de warmup obligatoria (estabilizar CPU/cache)
  - Mínimo 10,000 iteraciones para significancia estadística
  - Reportar: ns/operación, operaciones/segundo, speedup vs referencia
- **Sin dependencias de test:** los benchmarks son independientes de la suite de tests.
- **Comparar siempre contra:**
  - Tipos nativos (int32_t, int64_t, __int128)
  - Bibliotecas de referencia (Boost.Multiprecision, GMP, etc.)

### Estructura

```
benchs/
+-- CMakeLists.txt
+-- benchmark_arithmetic.cpp
+-- benchmark_division_algorithms.cpp
+-- benchmark_string_conversion.cpp
+-- benchmark_vs_boost.cpp
```

---

## 6. Directorio `demos/`

Programas de ejemplo organizados por categoría.

### Categorías

| Subdirectorio | Propósito | Audiencia |
|---|---|---|
| `tutorials/` | Aprendizaje paso a paso, numerado: `01_xxx`, `02_xxx` | Principiantes |
| `examples/` | Casos de uso reales (IPv6, criptografía, etc.) | Usuarios |
| `showcase/` | Características avanzadas de la lib | Usuarios avanzados |
| `comparison/` | Comparaciones vs otras bibliotecas | Evaluadores |
| `performance/` | Demostraciones de rendimiento | Performance-oriented |
| `integration/` | Integración con STL, Boost, etc. | Integradores |

### Reglas

- Cada demo es un `.cpp` autocontenido con `main()`.
- Todo demo debe compilar con todos los compiladores soportados.
- Los demos deben tener comentarios explicativos abundantes.

---

## 7. Directorio `docs/`

Toda la documentación que **no sea** README.md, CHANGELOG.md o LICENSE.

### Subdirectorios

| Subdirectorio | Contenido |
|---|---|
| `api/` | Referencia API estilo cppreference (archivos `API_[header].md`) |
| `guides/` | Guías de usuario, migración, buenas prácticas |
| `ai-prompts/` | Instrucciones contextuales para agentes IA, mensajes temporales |
| `generated/` | Salida de Doxygen (añadir a `.gitignore`) |

### Formato API (estilo cppreference)

````markdown
# API Reference — [Header Name]

## Synopsis
```cpp
namespace nstd {
    constexpr int popcount(uint128_t x) noexcept;
}
```

## Functions

### popcount
```cpp
constexpr int popcount(uint128_t x) noexcept;
```
**Brief:** Counts the number of 1-bits in x.
**Parameters:** `x` — Value to examine
**Return value:** Number of 1-bits
**Complexity:** O(1)

**Example:**
```cpp
const uint128_t val{0b11110000};
assert(nstd::popcount(val) == 4);
```
````

---

## 8. Directorio `scripts/`

Todos los scripts de automatización y utilidades.

### Subdirectorios

| Subdirectorio | Contenido |
|---|---|
| `env_setup/` | Scripts de configuración de entorno por compilador |
| `wsl/` | Scripts específicos para compilaciones en WSL |
| `legacy/` | Scripts obsoletos mantenidos como referencia |

### Scripts principales en `scripts/`

| Script | Lenguaje | Propósito |
|---|---|---|
| `build_generic.py` | Python | Compilación parametrizada (delega a CMake) |
| `check_generic.py` | Python | Ejecución de tests parametrizada |
| `run_generic.py` | Python | Ejecución de benchmarks |
| `build_generic.bash` | Bash | Alternativa Bash al build Python |
| `vcvars.py` | Python | Setup de entorno MSVC (variables temporales) |
| `detect_compilers.py` | Python | Autodescubrimiento de compiladores instalados |
| `multi_compiler_test.py` | Python | Validación cruzada con todos los compiladores |
| `cleanup_unicode.py` | Python | Limpieza de caracteres Unicode en fuentes |
| `run_static_analysis.bash` | Bash | Ejecución de cppcheck, clang-tidy, etc. |

### Regla fundamental

Los scripts Python en `scripts/` proporcionan **entornos de variables de entorno locales y temporales**
(vigentes solo durante la compilación) para cada compilador y plataforma. Todos los demás mecanismos
(Makefile, CMake) delegan a estos scripts.

---

## 9. Directorio `build/`

**Completamente generado. Debe estar en `.gitignore`. Nunca se versiona.**

### Estructura interna

```
build/
+-- build_tests/
|   +-- [compiler]/
|       +-- [mode]/
|           +-- test_constructors.exe
|           +-- test_arithmetic.exe
|           +-- ...
+-- build_benchs/
|   +-- [compiler]/
|       +-- [mode]/
|           +-- benchmark_arithmetic.exe
|           +-- ...
+-- build_demos/
|   +-- [compiler]/
|       +-- [mode]/
|           +-- 01_basic_operations.exe
|           +-- ...
+-- CMakeFiles/
+-- CMakeCache.txt
+-- build.ninja
```

### Convención de rutas de salida

```
build/build_[target]/[compiler]/[mode]/[nombre_ejecutable].[ext]
```

Donde:

- `[target]`: `tests`, `benchs`, `demos`
- `[compiler]`: `gcc`, `gcc-15`, `clang`, `clang-19`, `msvc`, `intel`
- `[mode]`: `debug`, `release`, `release-O1`, `release-O2`, `release-O3`, `release-Ofast`
- `[ext]`: `.exe` en Windows, sin extensión en Linux

### Reglas

- **Nunca versionar en git** — toda la carpeta `build/` está en `.gitignore`.
- **Nunca generar artefactos fuera de `build/`** — ni en raíz, ni en `tests/`, ni en `include/`.
- **Limpieza:** `python make.py clean` o `cmake --build build --target clean`.
- Todo directorio intermedio se crea automáticamente por el sistema de build.

---

## 10. Directorio `debugging/`

Área de trabajo **temporal** para depuración de problemas específicos.

### Reglas

- Contiene subdirectorios `src/` (código temporal) y `build/` (compilaciones temporales).
- **No se compila desde la raíz** — es un espacio aislado.
- El código aquí **no forma parte de la biblioteca**.
- Usar tag `[DEBUG]` al documentar sesiones de debugging.
- Limpiar después de resolver el problema. Puede estar vacío.

### Estructura

```
debugging/
+-- src/          # Código temporal para reproducir bugs
+-- build/        # Compilaciones de debug
```

### Cuándo usar

- Reproducir un bug del compilador (ej: bug de optimización GCC -O2).
- Aislar un problema de linkeo o de intrínsecos.
- Probar un nuevo compilador antes de integrarlo al workflow.

Toda compilación manual en `debugging/` debe documentarse:

```bash
# [DEBUG] - Bypassing workflow due to: GCC -O2 constexpr-if bug
# Issue: template instantiation fails at -O2 but works at -O1
g++ -std=c++20 -O2 -Iinclude debugging/src/repro_gcc_bug.cpp -o debugging/build/repro
```

---

## 11. Directorio `legacy-code/`

Código base de referencia desde el que partió el proyecto actual.

### Reglas Absolutas

- **INMUTABLE:** no se modifica, no se edita, nunca.
- **No seguible por git:** incluido en `.gitignore`. Se copia manualmente al clonar si se necesita.
- **Solo lectura:** es referencia para consultar la implementación original.
- **No se compila directamente:** no tiene targets en CMake ni en make.py.

### Propósito

Mantener una copia intacta de la versión anterior del proyecto (ej: fase 1.66 para un proyecto
en fase 1.75) para:

- Comparar decisiones de diseño.
- Consultar implementaciones originales de algoritmos.
- Verificar regresiones contra la versión base.

### Configuración en `.gitignore`

```gitignore
# Legacy code (read-only reference, not tracked)
legacy-code/
```

---

## 12. Archivos del Directorio Raíz

Solo los estrictamente necesarios:

| Archivo | Propósito | Obligatorio |
|---|---|---|
| `CMakeLists.txt` | Configuración raíz CMake | Si |
| `Makefile` | Interfaz Make (delega a scripts/) | Si |
| `make.py` | Orquestador Python principal | Si |
| `README.md` | Presentación del proyecto | Si |
| `CHANGELOG.md` | Registro de cambios (actualizar frecuentemente) | Si |
| `LICENSE` o `LICENSE.txt` | Licencia del proyecto | Si |
| `AI-GUIDE.md` | Este archivo | Si |
| `.gitignore` | Exclusiones de git | Si |
| `Doxyfile` | Configuración Doxygen | Opcional |
| `conanfile.txt` | Dependencias Conan | Opcional |
| `.clang-format` | Estilo de formateo | Recomendado |
| `.clang-tidy` | Reglas clang-tidy | Recomendado |

**Cualquier otro archivo `.md`, `.py`, `.bash`, `.bat`, `.cpp` debe ubicarse en su directorio correcto.**

---

## 13. Sistema de Build — Jerarquía de 4 Capas

```
+-----------------------------------------------+
| Capa 1: Python Scripts (make.py, scripts/*.py) |  <-- El usuario invoca aqui
+-----------------------------------------------+
| Capa 2: Makefile (orchestracion)               |  <-- Opcional, delega a Capa 1
+-----------------------------------------------+
| Capa 3: CMake/CTest (configuracion, deteccion) |  <-- Genera sistema de build
+-----------------------------------------------+
| Capa 4: Ninja (compilacion paralela)           |  <-- Ejecuta compilacion
+-----------------------------------------------+
```

### Capa 1 — Python (`make.py`, `scripts/*.py`)

**Responsabilidad:** interfaz para el usuario/IA. Selecciona compilador, modo, feature.
Configura variables de entorno temporales. Invoca CMake.

```bash
# Comandos principales
python make.py build [type] [feature] tests [compiler] [mode]
python make.py check [type] [feature] [compiler] [mode]
python make.py run [type] [feature] benchs [compiler] [mode]
python make.py test                   # Todos los tests
python make.py clean                  # Limpiar artefactos
python make.py demo [category] [name] # Compilar y ejecutar demo
python make.py init                   # Detectar compiladores
```

### Capa 2 — Makefile

**Responsabilidad:** atajo para usuarios de Make. Delega a `scripts/*.py`.

```bash
make build_tests FEATURE=algorithm COMPILER=gcc MODE=release
make check FEATURE=traits COMPILER=all MODE=all
make demo CATEGORY=tutorials DEMO=01_basic_operations
```

### Capa 3 — CMake/CTest

**Responsabilidad:** detección de compilador/plataforma, generación de build.ninja,
opciones de sanitizers/análisis, autodescubrimiento de targets.

Módulos en `cmake/`:

- `CompilerDetection.cmake` — detecta compilador, versión, plataforma
- `SanitizerConfig.cmake` — configura ASan, UBSan, TSan, MSan
- `StaticAnalysis.cmake` — configura cppcheck, clang-tidy, Infer, PVS-Studio
- `TestConfig.cmake` — integración con CTest

### Capa 4 — Ninja

**Responsabilidad:** compilación paralela. Se invoca automáticamente por CMake.

### Reglas NON-NEGOTIABLE

1. **NUNCA compilar manualmente** (`g++ ...`, `cl.exe ...`) excepto en `debugging/` con tag `[DEBUG]`.
2. **NUNCA saltar capas** (ej: invocar Ninja o CMake directamente sin pasar por Python).
3. **Documentar excepciones** si se debe violar una regla por depuración.

---

## 14. Compiladores, Versiones y Plataformas

### Matriz de Compiladores

| Compilador | Versiones | Plataforma | Entorno |
|---|---|---|---|
| **GCC** | 13, 14, 15 | Linux (WSL Ubuntu 25.04) | Nativo |
| **GCC** | 15.x | Windows | MSYS2 UCRT64 |
| **Clang** | 18, 19, 20, 21 | Linux (WSL Ubuntu 25.04) | Nativo |
| **Clang** | 19.x | Windows | MSYS2 Clang64 |
| **MSVC** | 2026 (v18) | Windows 11 | Visual Studio |
| **Intel ICX** | oneAPI latest | Windows 11 | Depende de MSVC |
| **Intel ICPX** | oneAPI latest | Linux (WSL) | Nativo |

### Rutas de Compiladores (Windows)

```
GCC:       /c/msys64/ucrt64/bin/g++
Clang:     /c/msys64/clang64/bin/clang++
MSVC:      C:\Program Files\Microsoft Visual Studio\18\Community\VC\...
Intel:     C:\Program Files (x86)\Intel\oneAPI\
vcvarsall: C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat
```

### Estándar

**C++20 obligatorio.** `CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED ON`.

### Niveles de Optimización para Release

| Modo | GCC/Clang | MSVC | Intel | Uso |
|---|---|---|---|---|
| `release-O1` | `-O1` | `/O1` | `-O1` | Optimización de tamaño |
| `release-O2` | `-O2` | `/O2` | `-O2` | Optimización estándar |
| `release-O3` | `-O3` | `/Ox` | `-O3` | Optimización agresiva |
| `release-Ofast` | `-Ofast` | `/Ox /fp:fast` | `-Ofast` | Velocidad máxima |

### Plataformas Futuras (roadmap)

- ARM64 (Linux, macOS)
- ARM32 (embedded)
- RISC-V 32/64

---

## 15. Sanitizers y Análisis Estático

### Sanitizers

| Sanitizer | GCC/Clang | MSVC | Intel | Flag |
|---|---|---|---|---|
| AddressSanitizer | Si | Si | Si | `-fsanitize=address` / `/fsanitize=address` |
| UndefinedBehaviorSan | Si | No | Si | `-fsanitize=undefined` |
| ThreadSanitizer | Si | No | No | `-fsanitize=thread` |
| MemorySanitizer | Clang only | No | No | `-fsanitize=memory` |

**Regla:** debug builds deben poder activar ASan+UBSan combinados:

```bash
-fsanitize=address,undefined -fno-omit-frame-pointer
```

### Análisis Estático

| Herramienta | Comando | Propósito |
|---|---|---|
| cppcheck | `cppcheck --enable=all --std=c++20 -I include/` | Análisis general |
| clang-tidy | `clang-tidy -p build/ src/*.cpp` | Modernización y bugs |
| Infer | `infer run -- g++ ...` | Análisis interprocedimental |
| PVS-Studio | `pvs-studio-analyzer analyze` | Análisis comercial (free OSS) |

### Cobertura

| Herramienta | Compilador | Uso |
|---|---|---|
| GCov | GCC | `--coverage` flag |
| LCOV | GCC | Genera HTML desde gcov |
| llvm-cov | Clang | `-fprofile-instr-generate -fcoverage-mapping` |

Configuración CMake:

```cmake
option(INT128_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(INT128_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(INT128_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(INT128_ENABLE_COVERAGE "Enable code coverage" OFF)
```

---

## 16. CI/CD — GitHub Actions Workflows

> Los archivos de workflow se encuentran en [`AI_PROMPT/workflow/`](AI_PROMPT/workflow/) y deben
> copiarse a `.github/workflows/` al activar CI/CD en el repositorio.

### Resumen de Workflows

| Workflow | Archivo | Trigger | Propósito |
|---|---|---|---|
| **CI** | [`ci.yml`](AI_PROMPT/workflow/ci.yml) | push/PR a main, develop, phase-* | Build + test multi-compilador |
| **Benchmarks** | [`benchmarks.yml`](AI_PROMPT/workflow/benchmarks.yml) | Semanal (dom 00:00 UTC) + manual | Suite completa de benchmarks |
| **Release** | [`release.yml`](AI_PROMPT/workflow/release.yml) | Tag `v*.*.*` + manual | Empaquetado y publicación de release |

### Workflow 1: CI (`ci.yml`)

**Trigger:** push/PR a ramas `main`, `develop`, `phase-*`; dispatch manual con selector de feature.

**Jobs:**

| Job | Runner | Matriz | Configs |
|---|---|---|---|
| `gcc-build` | ubuntu-24.04 | 3 versiones × 10 features × 2 modos | ~60 |
| `clang-build` | ubuntu-24.04 | 4 versiones × 10 features × 2 modos | ~80 |
| `msvc-build` | windows-latest | 1 versión × 2 modos | ~2 |
| `intel-build` | windows-latest (MSYS2) | Solo en PRs, release only | ~1 |
| `sanitizers` | ubuntu-24.04 | Clang-19, ASan + UBSan | features × 1 |
| `benchmarks` | ubuntu-24.04 | GCC-14, solo en PRs | ~1 |
| `code-quality` | ubuntu-24.04 | clang-format + clang-tidy | ~1 |
| `ci-dashboard` | ubuntu-24.04 | Resumen de todos los jobs | ~1 |

**Concurrencia:** cancela runs anteriores en el mismo branch.

**Artefactos:** ejecutables + resultados con retención 7 días.

### Workflow 2: Benchmarks (`benchmarks.yml`)

**Trigger:** schedule semanal (domingos 00:00 UTC) + dispatch manual con selector de compilador.

**Proceso:**
1. Setup MSYS2 UCRT64 con GCC, Clang, CMake, Ninja.
2. Ejecuta benchmarks para cada compilador de la matriz.
3. Genera reporte de comparación entre compiladores.
4. Si es PR: comenta los resultados directamente en el PR.

**Compiladores en matriz:** `gcc`, `clang`, `msvc`, `intel`.

### Workflow 3: Release (`release.yml`)

**Trigger:** push de tag `v*.*.*` + dispatch manual.

**Proceso:**
1. Setup MSYS2 UCRT64 + MSVC + Intel oneAPI.
2. Build con los 4 compiladores en modo release.
3. Empaqueta en ZIP: headers, tests, benchmarks, documentación.
4. Crea GitHub Release con changelog auto-generado.

**Artefactos del release:**
- `[lib]-[version]-headers.zip` — Solo headers (para header-only libs)
- `[lib]-[version]-[compiler]-binaries.zip` — Binarios por compilador
- `[lib]-[version]-docs.zip` — Documentación generada

### Principios CI/CD

- **Fail-fast: false** — no cancelar jobs al primer fallo (queremos ver todo el panorama).
- **Concurrencia:** cancelar runs anteriores si hay nuevo push al mismo branch.
- **Artefactos:** subir ejecutables y resultados de test con retención de 7 días.
- **El script CI llama a `python make.py`** — nunca compila directamente.
- **Sanitizers en CI:** ASan + UBSan con Clang-19 en cada PR.
- **Code Quality gate:** clang-format y clang-tidy deben pasar antes del merge.

### Plantilla de matriz CI mínima

```yaml
strategy:
  fail-fast: false
  matrix:
    compiler: [gcc-13, gcc-14, gcc-15, clang-18, clang-19, clang-20, msvc, intel]
    build_type: [debug, release]
    feature: [core, algorithm, traits, format, ...]
```

---

## 17. Convenciones de Nombrado de Archivos

### Headers

| Patrón | Ejemplo |
|---|---|
| `[lib]_[feature].hpp` | `int128_param_algorithm.hpp` |
| `[lib]_traits.hpp` | `int128_param_traits.hpp` |
| `[lib]_traits_specializations.hpp` | `int128_param_traits_specializations.hpp` |

### Tests

| Patrón | Ejemplo |
|---|---|
| `test_[feature].cpp` | `test_arithmetic.cpp` |
| `test_[type]_[feature].cpp` | `test_uint128_division.cpp` |

### Benchmarks

| Patrón | Ejemplo |
|---|---|
| `benchmark_[feature].cpp` | `benchmark_division_algorithms.cpp` |
| `bench_[type]_vs_[type].cpp` | `bench_uint128_vs_boost.cpp` |

### Ejecutables de salida

```
build/build_tests/[compiler]/[mode]/test_[feature].[ext]
build/build_benchs/[compiler]/[mode]/benchmark_[feature].[ext]
build/build_demos/[compiler]/[mode]/[demo_name].[ext]
```

---

## 18. Estándares de Codificación C++

### Resumen de Reglas

| Regla | Ejemplo correcto |
|---|---|
| **Const correctness** | `const auto result{compute()};` |
| **Brace initialization** | `const uint128_t x{42};` (nunca `= 42`) |
| **Constexpr/noexcept** | `constexpr uint64_t high() const noexcept { ... }` |
| **K&R braces** | `if (x) {` en la misma línea |
| **Siempre llaves** | `if (x) { return; }` (nunca sin llaves) |
| **Template en su línea** | `template <typename T>` separado de la firma |
| **Explicit constructors** | `explicit constexpr int128_t(int v) noexcept;` |
| **Explicit casts** | `static_cast<uint64_t>(v)` (nunca C-cast) |
| **std:: explícito** | `std::vector`, `std::string` (nunca `using namespace std`) |
| **snake_case** | tipos `_t`, templates `_tt`, enums `_ec_t` |
| **ASCII-only output** | `[OK]`, `[FAIL]` (nunca Unicode en consola) |
| **std::byte para buffers** | Nunca `uint8_t` para bytes crudos |
| **No exceptions en core** | `std::optional` o `std::pair<error, T>` |
| **Wrappers _or_throw()** | Solo en interfaz externa |

### Naming

| Elemento | Estilo | Ejemplo |
|---|---|---|
| Tipos/Clases | `snake_case_t` | `uint128_t`, `int128_base_t` |
| Templates tipos | `snake_case_tt` | `int128_base_tt` |
| Enum class | `snake_case_ec_t` | `parse_error_ec_t` |
| Funciones | `snake_case` | `to_string()`, `is_negative()` |
| Variables | `snake_case` | `const auto result{...}` |
| Constantes estáticas | `UPPER_SNAKE` | `BITS`, `BYTES`, `LIMBS` |
| Namespaces | `snake_case` | `nstd`, `intrinsics` |
| Archivos | `snake_case.hpp` | `int128_base_tt.hpp` |

### Documentación Doxygen

```cpp
/**
 * @brief One-line description
 * @param[in] param1 Description
 * @return Description
 * @throws None (noexcept)
 * @note Important notes
 *
 * @code
 * const uint128_t x{42};
 * const auto result = x.to_string();
 * @endcode
 */
```

### Licencia (obligatoria en headers y fuentes de biblioteca)

```cpp
// =============================================================================
// [Library Name] - Brief Description
// =============================================================================
//
// SPDX-License-Identifier: BSL-1.0
//
// Copyright (c) [years] [Author Name]
// Email: [email]
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or
//  https://www.boost.org/LICENSE_1_0.txt)
//
// =============================================================================
```

---

## 19. Flujo de Trabajo del Desarrollador

### Agregar una nueva feature

```
1. Crear header:       include/mi_lib_[feature].hpp
2. Crear test:         tests/test_[feature].cpp
3. Crear benchmark:    benchs/benchmark_[feature].cpp  (si aplica)
4. Compilar:           python make.py build [type] [feature] tests gcc release
5. Ejecutar tests:     python make.py check [type] [feature] gcc release
6. Validar multi-comp: python make.py check [type] [feature] all all
7. Actualizar docs:    docs/api/API_[FEATURE].md
8. Actualizar:         CHANGELOG.md
```

### Depurar un problema

```
1. Crear repro:        debugging/src/repro_[issue].cpp
2. Compilar manual:    # [DEBUG] tag obligatorio
3. Diagnosticar
4. Corregir en:        include/mi_lib_[feature].hpp
5. Compilar via workflow: python make.py build ...
6. Verificar fix:      python make.py check ...
7. Limpiar:            Borrar archivos de debugging/
```

### Ciclo CI/CD completo

> Ver [§16 CI/CD — GitHub Actions Workflows](#16-cicd--github-actions-workflows) para detalle
> de cada workflow y sus jobs.

```
1. Push a branch
2. GitHub Actions ejecuta ci.yml (~140 configs de compilador x feature x modo)
3. Sanitizers (ASan+UBSan) corren en paralelo con Clang-19
4. Code quality (clang-format, clang-tidy) ejecutado como gate
5. CI Dashboard resume resultados de todos los jobs
6. Todos los tests pasan -> merge permitido
7. Tag v*.*.* -> release.yml empaqueta headers + docs + binarios
8. Domingos: benchmarks.yml compara rendimiento entre compiladores
```

---

## 20. Reglas para Agentes IA

### Reglas Críticas

> **Referencia completa:** [`AI_PROMPT/ai-instructions.md`](AI_PROMPT/ai-instructions.md)
> contiene todas las reglas con ejemplos detallados.

1. **Nunca compilar manualmente.** Usar siempre `python make.py ...` o el workflow establecido.
2. **Nunca crear archivos en raíz** salvo los listados en la sección 12.
3. **Nunca crear ejecutables (.exe, .obj) fuera de `build/`.**
4. **Nunca modificar `legacy-code/`.** Es inmutable y de solo lectura.
5. **Reportar estado cada ~3 minutos** en sesiones activas de desarrollo.
6. **ASCII-only en output** de consola C++ — sin Unicode.
7. **Actualizar CHANGELOG.md** tras cada cambio significativo.

### Modo Interactivo

El usuario compila en su terminal (MSYS2, WSL, PowerShell) y reporta logs.
El agente IA:

- Propone cambios en código.
- Espera confirmación antes de cambios mayores.
- No ejecuta compilaciones largas autónomamente.

### Estructura de Respuesta del Agente

Al modificar código, el agente debe indicar:

1. **Archivo(s) modificado(s)** con ruta completa.
2. **Qué cambió** y por qué.
3. **Comando de compilación** para verificar.
4. **Test(s) afectado(s)** y cómo ejecutarlos.

### Diagnóstico

Antes de proponer una solución, el agente debe:

1. Leer el archivo fuente relevante.
2. Leer el test que falla.
3. Entender el error del compilador.
4. Proponer la corrección mínima necesaria (no sobre-ingeniería).

---

## Apéndice A — Plantilla `.gitignore` para este tipo de proyecto

```gitignore
# Build output
build/
build_*/
cmake-build-*/

# Legacy code (immutable, not tracked)
legacy-code/

# Debugging temporaries
debugging/build/

# CMake cache
CMakeFiles/
CMakeCache.txt
cmake_install.cmake
*.cmake
!cmake/*.cmake
build.ninja
rules.ninja

# Compiled objects and executables
*.exe
*.dll
*.so
*.dylib
*.o
*.obj
*.a
*.lib
*.pdb
*.gch

# IDE
.vscode/
.idea/
*.swp
*.swo
*~

# Python
__pycache__/
*.pyc
*.pyo

# Doxygen output
docs/generated/

# Test/coverage output
*.xml
test_results/
coverage/

# OS
.DS_Store
Thumbs.db

# Temporary
*.tmp
*.temp
```

---

## Apéndice B — Checklist para nuevo proyecto desde esta plantilla

- [ ] Crear estructura de directorios (sección 2)
- [ ] Copiar `CMakeLists.txt` raíz y adaptarlo
- [ ] Copiar `cmake/*.cmake` módulos
- [ ] Copiar `tests/CMakeLists.txt`, `benchs/CMakeLists.txt`, `demos/CMakeLists.txt`
- [ ] Configurar `make.py` con compiladores disponibles
- [ ] Configurar `.gitignore` (Apéndice A)
- [ ] Configurar `.github/workflows/ci.yml`
- [ ] Crear primer header en `include/`
- [ ] Crear primer test en `tests/`
- [ ] Ejecutar `python make.py init` para detectar compiladores
- [ ] Verificar: `python make.py build ... tests gcc debug`
- [ ] Verificar: `python make.py check ... gcc debug`
- [ ] Copiar código de referencia a `legacy-code/` si aplica
- [ ] Escribir `README.md`
- [ ] Escribir `CHANGELOG.md` con entrada inicial
- [ ] Adaptar este `AI-GUIDE.md` al proyecto concreto

---

## Apéndice C — Documentos Vinculados

Este archivo es el punto de entrada principal. Los siguientes documentos complementan
y amplían la información aquí contenida:

| Documento | Ruta | Relación con AI-GUIDE.md |
|---|---|---|
| **ai-instructions.md** | [`AI_PROMPT/ai-instructions.md`](AI_PROMPT/ai-instructions.md) | Reglas detalladas para agentes IA: rutas de compiladores, jerarquía de build, std::byte, byteswap, ASCII, licencias. Complementa §13, §18, §20. |
| **Explicación del Proyecto** | [`AI_PROMPT/GENERAL_GUIDES/Explicación_del_Proyecto.md`](AI_PROMPT/GENERAL_GUIDES/Explicaci%C3%B3n_del_Proyecto.md) | Visión general del proyecto: 12 objetivos, 9 etapas (TC, MS, EK, fixed-point, big integers, etc.). Complementa §1. |
| **CI Workflow** | [`AI_PROMPT/workflow/ci.yml`](AI_PROMPT/workflow/ci.yml) | Definición completa de CI multi-compilador (~400 líneas). Complementa §16. |
| **Benchmarks Workflow** | [`AI_PROMPT/workflow/benchmarks.yml`](AI_PROMPT/workflow/benchmarks.yml) | Suite semanal de benchmarks (~170 líneas). Complementa §16. |
| **Release Workflow** | [`AI_PROMPT/workflow/release.yml`](AI_PROMPT/workflow/release.yml) | Empaquetado automático de releases (~150 líneas). Complementa §16. |
| **CHANGELOG.md** | [`CHANGELOG.md`](CHANGELOG.md) | Registro de cambios del proyecto. |
| **README.md** | [`README.md`](README.md) | Presentación pública del proyecto. |

---

*Fin de AI-GUIDE.md*
