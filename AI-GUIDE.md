# AI-GUIDE.md — Guía Técnica para IA y Desarrollador de Proyectos C/C++

> **Propósito:** Plantilla reutilizable para proyectos de bibliotecas C++ moderno (C++20+),
> con soporte multi-compilador, multi-plataforma, CI/CD integrado, y organización estricta de directorios.
>
> **Audiencia:** Agentes IA (Copilot, Gemini, Claude, etc.) y desarrolladores humanos.
>
> **Versión:** 2.0.2 — 22 marzo 2026

### Guías Relacionadas

| Documento | Ruta | Descripción |
|---|---|---|
| **ai-instructions.md** | [`STYLE_CONVENTIONS.md`](STYLE_CONVENTIONS.md) y [`NAMING_CONVENTIONS.md`](NAMING_CONVENTIONS.md) | Convenciones de código y de nombres (antes en `AI_PROMPT/ai-instructions.md`, hoy archivado) |
| **Explicación del Proyecto** | [`AI_PROMPT/GENERAL_GUIDES/Explicación_del_Proyecto.md`](AI_PROMPT/GENERAL_GUIDES/Explicaci%C3%B3n_del_Proyecto.md) | 12 objetivos y 9 etapas del proyecto |
| **CI Workflow** | [`AI_PROMPT/workflow/ci.yml`](AI_PROMPT/workflow/ci.yml) | Build + test multi-compilador (push/PR) |
| **Benchmarks Workflow** | [`AI_PROMPT/workflow/benchmarks.yml`](AI_PROMPT/workflow/benchmarks.yml) | Suite completa de benchmarks (semanal) |
| **Release Workflow** | [`AI_PROMPT/workflow/release.yml`](AI_PROMPT/workflow/release.yml) | Empaquetado de release (tag v*.*.*) |
| **ROADMAP.md** | [`ROADMAP.md`](ROADMAP.md) | Las 9 etapas del proyecto y su estado; decisiones abiertas y deuda anotada |
| **docs/archive/SESSION_STATE.md** | [`docs/archive/SESSION_STATE.md`](docs/archive/SESSION_STATE.md) | Contexto volátil: estado actual para onboarding rápido |
| **CONTRIBUTING.md** | [`CONTRIBUTING.md`](CONTRIBUTING.md) | Guía de contribución: entorno, los 4 verificadores, estilo, commits |
| **SECURITY.md** | [`SECURITY.md`](SECURITY.md) | Política de seguridad; alcance y el aviso de que NO hay tiempo constante |

---

> **Guía única del proyecto.** Desde el 25 ago 2026 esta es la única guía de
> desarrollo. `AI_PROMPT/ai-instructions.md` está archivado y
> `.github/copilot-instructions.md` es un puntero: eran, respectivamente, otra
> guía y una copia de ella.
>
> Dos temas tienen documento propio por ser los que más se consultan:
>
> - [`NAMING_CONVENTIONS.md`](NAMING_CONVENTIONS.md) — cómo se nombran las cosas
> - [`STYLE_CONVENTIONS.md`](STYLE_CONVENTIONS.md) — cómo se escribe el código
>
> Y para quien viene de fuera, [`CONTRIBUTING.md`](CONTRIBUTING.md) tiene lo
> necesario para compilar, probar y enviar un cambio.

## Tabla de Contenidos

**Parte I — Estructura del Proyecto**

1. [Filosofía y Reglas Fundamentales](#1-filosofía-y-reglas-fundamentales)
2. [Estructura de Directorios](#2-estructura-de-directorios)
3. [Directorio `include/`](#3-directorio-include)
4. [Directorio `tests/`](#4-directorio-tests)
5. [Directorio `benchs/`](#5-directorio-benchs)
6. [Directorio `demos/`](#6-directorio-demos)
7. [Directorio `docs/` — Estrategia de Documentación](#7-directorio-docs--estrategia-de-documentación)
8. [Directorio `scripts/`](#8-directorio-scripts)
9. [Directorio `build/`](#9-directorio-build)
10. [Directorio `debugging/`](#10-directorio-debugging)
11. [Directorio `legacy-code/`](#11-directorio-legacy-code)
12. [Archivos del Directorio Raíz](#12-archivos-del-directorio-raíz)

**Parte II — Build, Compiladores y CI/CD**

1. [Sistema de Build — Jerarquía de 4 Capas](#13-sistema-de-build--jerarquía-de-4-capas)
2. [Compiladores, Versiones y Plataformas](#14-compiladores-versiones-y-plataformas)
3. [Sanitizers y Análisis Estático](#15-sanitizers-y-análisis-estático)
4. [CI/CD — GitHub Actions Workflows](#16-cicd--github-actions-workflows)
5. [Gestión de Dependencias](#17-gestión-de-dependencias)

**Parte III — Estándares y Convenciones**

1. [Convenciones de Nombrado de Archivos](#18-convenciones-de-nombrado-de-archivos)
2. [Estándares de Codificación C++](#19-estándares-de-codificación-c)
3. [CHANGELOG y Versionado Semántico](#20-changelog-y-versionado-semántico)
4. [Conventional Commits](#21-conventional-commits)
5. [Estrategia de Branching](#22-estrategia-de-branching)
6. [Política de Estabilidad de API](#23-política-de-estabilidad-de-api)

**Parte IV — Gestión del Proyecto**

1. [SESSION_STATE — Contexto Persistente](#24-session_state--contexto-persistente)
2. [ROADMAP — Línea de Tiempo del Proyecto](#25-roadmap--línea-de-tiempo-del-proyecto)
3. [Architecture Decision Records (ADRs)](#26-architecture-decision-records-adrs)
4. [Known Issues por Plataforma](#27-known-issues-por-plataforma)
5. [Métricas de Calidad Mínimas](#28-métricas-de-calidad-mínimas)

**Parte V — Flujo de Trabajo y Reglas**

1. [Flujo de Trabajo del Desarrollador](#29-flujo-de-trabajo-del-desarrollador)
2. [Reglas para Agentes IA](#30-reglas-para-agentes-ia)
3. [Comandos Interactivos para la IA](#31-comandos-interactivos-para-la-ia)

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

## 7. Directorio `docs/` — Estrategia de Documentación

Toda la documentación que **no sea** README.md, CHANGELOG.md o LICENSE.

La documentación se organiza en **tres niveles** con audiencias distintas:

### Nivel 1 — Documentación Interna (en el código)

**Audiencia:** Mantenedores y contribuidores que leen el código fuente.

| Elemento | Ubicación | Formato | Obligatoriedad |
|---|---|---|---|
| Comentarios Doxygen de API pública | Headers en `include/` | `/** @brief ... */` | Obligatorio en toda función/clase pública |
| Comentarios `@internal` | Headers en `include/` | `/** @internal ... */` | Obligatorio en funciones no-triviales de `detail/` |
| Comentarios de implementación | Dentro de funciones | `// Explicación de por qué` | Solo cuando la lógica no es evidente |
| `@file` header | Inicio de cada `.hpp` | Doxygen file-level | Obligatorio |
| License header | Inicio de cada `.hpp`/`.cpp` | SPDX + copyright | Obligatorio en lib, simplificado en tests |

**Reglas de documentación interna:**

- **Documentar el "por qué", no el "qué".** El código dice qué hace; el comentario dice por qué.
- **Toda función pública** debe tener: `@brief`, `@param`, `@return`, `@throws`/`noexcept`, `@code` ejemplo.
- **Toda función interna no-trivial** debe tener: `@internal`, `@brief`, `@par Implementation Notes`.
- **No documentar lo obvio:** `/// Returns the value` sobre `getValue()` es ruido.
- **Mantener sincronizado:** documentación desactualizada es peor que ninguna documentación.

```cpp
// CORRECTO — documenta decisión no obvia
// Usamos Karatsuba para multiplicación porque threshold > 64 bits
// reduce operaciones de O(n^2) a O(n^1.585)
const auto product{karatsuba_multiply(a, b)};

// INCORRECTO — documenta lo obvio
// Multiplica a por b
const auto product{a * b};
```

### Nivel 2 — Documentación para Usuarios de la Biblioteca

**Audiencia:** Desarrolladores que usan la biblioteca en sus proyectos.

| Documento | Ubicación | Propósito |
|---|---|---|
| `README.md` | Raíz | Introducción, instalación rápida, ejemplo mínimo |
| `docs/api/API_[header].md` | `docs/api/` | Referencia API estilo cppreference |
| `docs/guides/getting_started.md` | `docs/guides/` | Tutorial de inicio paso a paso |
| `docs/guides/migration_guide.md` | `docs/guides/` | Migración desde versión anterior u otra lib |
| `docs/guides/best_practices.md` | `docs/guides/` | Patrones recomendados, anti-patrones |
| `docs/guides/faq.md` | `docs/guides/` | Preguntas frecuentes |
| `demos/tutorials/` | `demos/` | Código de ejemplo ejecutable |
| `docs/generated/` | `docs/generated/` | Doxygen HTML (auto-generado, en `.gitignore`) |

### Nivel 3 — Documentación para Desarrolladores/Mantenedores

**Audiencia:** Contribuidores al propio proyecto, agentes IA.

| Documento | Ubicación | Propósito |
|---|---|---|
| `AI-GUIDE.md` | Raíz | Este archivo — guía maestra del proyecto |
| `CONTRIBUTING.md` | Raíz | Cómo contribuir, code review, estilo |
| `SECURITY.md` | Raíz | Política de seguridad |
| `CHANGELOG.md` | Raíz | Historial de cambios con SemVer |
| `ROADMAP.md` | Raíz | Línea de tiempo del proyecto |
| `docs/archive/SESSION_STATE.md` | Raíz | Estado actual para onboarding rápido |
| `docs/decisions/ADR-NNN-*.md` | `docs/decisions/` | Architecture Decision Records |
| `docs/guides/internals.md` | `docs/guides/` | Arquitectura interna, diseño de algoritmos |
| `docs/guides/adding_features.md` | `docs/guides/` | Cómo añadir un nuevo módulo/feature |
| `docs/guides/compiler_notes.md` | `docs/guides/` | Notas específicas por compilador/plataforma |
| `AI_PROMPT/ai-instructions.md` | `AI_PROMPT/` | Reglas específicas para agentes IA |

### Subdirectorios de `docs/`

```
docs/
+-- api/                  # Referencia API (estilo cppreference)
|   +-- API_CORE.md
|   +-- API_ALGORITHM.md
|   +-- API_FORMAT.md
|   +-- ...
+-- guides/               # Guías narrativas
|   +-- getting_started.md
|   +-- best_practices.md
|   +-- migration_guide.md
|   +-- internals.md
|   +-- adding_features.md
|   +-- compiler_notes.md
|   +-- faq.md
+-- decisions/            # Architecture Decision Records
|   +-- README.md         # indice y estado de cada ADR
|   +-- TEMPLATE.md
|   +-- ADR-001-constructores-y-conversiones-explicitos.md
|   +-- ADR-002-almacenamiento-little-endian-de-limbos.md
|   +-- ...
+-- generated/            # Salida de Doxygen (en .gitignore)
```

### Configuración Doxygen

El `Doxyfile` en raíz debe configurar:

```
INPUT                  = include/
RECURSIVE              = YES
EXTRACT_ALL            = NO
EXTRACT_PRIVATE        = NO
GENERATE_HTML          = YES
HTML_OUTPUT            = docs/generated
USE_MDFILE_AS_MAINPAGE = README.md
WARN_IF_UNDOCUMENTED   = YES
```

**Regla:** `WARN_IF_UNDOCUMENTED = YES` asegura que Doxygen avise de funciones públicas
sin documentar. El CI puede fallar si hay warnings de Doxygen.

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
**Since:** v1.0.0
**Stability:** Stable

**Example:**
```cpp
const uint128_t val{0b11110000};
assert(nstd::popcount(val) == 4);
```
````

### Reglas de Sincronización de Documentación

| Evento | Documentación a actualizar |
|---|---|
| Nueva función pública | Doxygen en header + `docs/api/API_[FEATURE].md` |
| Cambio de firma/comportamiento | Doxygen + API doc + `CHANGELOG.md` |
| Nueva feature/módulo | Todo lo anterior + `docs/guides/getting_started.md` si aplica |
| Decisión de diseño importante | `docs/decisions/ADR-NNN-*.md` |
| Fix de bug | `CHANGELOG.md` + comentario en código si es sutil |
| Deprecación | `[[deprecated]]` en código + API doc + `CHANGELOG.md` + migration guide |

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

### Archivos de configuración de build

| Archivo | Propósito | Obligatorio |
|---|---|---|
| `CMakeLists.txt` | Configuración raíz CMake | Sí |
| `Makefile` | Interfaz Make (delega a scripts/) | Sí |
| `make.py` | Orquestador Python principal | Sí |
| `Doxyfile` | Configuración Doxygen | Opcional |
| `conanfile.txt` | Dependencias Conan | Opcional |
| `.clang-format` | Estilo de formateo | Recomendado |
| `.clang-tidy` | Reglas clang-tidy | Recomendado |
| `.gitignore` | Exclusiones de git | Sí |

### Documentación del proyecto

| Archivo | Propósito | Obligatorio | Ver sección |
|---|---|---|---|
| `README.md` | Presentación del proyecto | Sí | — |
| `CHANGELOG.md` | Registro de cambios con SemVer | Sí | [§20](#20-changelog-y-versionado-semántico) |
| `LICENSE` o `LICENSE.txt` | Licencia del proyecto | Sí | — |
| `AI-GUIDE.md` | Este archivo — guía maestra | Sí | — |
| `ROADMAP.md` | Línea de tiempo del proyecto | Sí | [§25](#25-roadmap--línea-de-tiempo-del-proyecto) |
| `docs/archive/SESSION_STATE.md` | Contexto volátil del estado actual | Sí | [§24](#24-session_state--contexto-persistente) |
| `CONTRIBUTING.md` | Guía de contribución externa | Sí | [Apéndice D](#apéndice-d--plantilla-contributingmd) |
| `SECURITY.md` | Política de seguridad y vulnerabilidades | Recomendado | [Apéndice E](#apéndice-e--plantilla-securitymd) |

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

## 17. Gestión de Dependencias

### Filosofía

Para bibliotecas header-only, las dependencias en runtime deben ser **cero**.
Las dependencias se clasifican en:

| Categoría | Ejemplos | Gestionada con | Obligatoria |
|---|---|---|---|
| **Runtime (biblioteca)** | Ninguna (header-only) | — | 0 dependencias |
| **Build** | CMake, Ninja, Python | Sistema / MSYS2 | Sí |
| **Test** | Framework propio (sin deps) | Ninguna | Solo para tests |
| **Benchmark** | Boost.Multiprecision, GMP | Conan / sistema | Solo para benchmarks |
| **Docs** | Doxygen, Graphviz | Sistema | Solo para generación |
| **Análisis** | cppcheck, clang-tidy, Infer | Sistema | Solo para CI/quality |

### Conan (dependencias de benchmark/test)

```ini
# conanfile.txt
[requires]
boost/1.85.0
gmp/6.3.0

[generators]
CMakeDeps
CMakeToolchain

[options]
boost/*:header_only=True
```

### CMake FetchContent (alternativa sin Conan)

```cmake
include(FetchContent)
FetchContent_Declare(
  boost_multiprecision
  GIT_REPOSITORY https://github.com/boostorg/multiprecision.git
  GIT_TAG boost-1.85.0
)
FetchContent_MakeAvailable(boost_multiprecision)
```

### Reglas

- **La biblioteca en sí NUNCA depende de Boost, GMP, ni ninguna otra lib externa.**
- Las dependencias externas solo se usan en `benchs/` y opcionalmente en `demos/comparison/`.
- Toda dependencia debe estar documentada en `conanfile.txt` o en `CMakeLists.txt`.
- El CI debe poder ejecutar tests sin dependencias externas.

---

## 18. Convenciones de Nombrado de Archivos

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

## 19. Estándares de Codificación C++

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

## 20. CHANGELOG y Versionado Semantico

### Formato del CHANGELOG

Seguir estrictamente [Keep a Changelog](https://keepachangelog.com/) v1.1.0:

```markdown
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- New feature X for module Y

### Changed
- Modified behavior of Z

## [1.2.0] - 2026-03-15

### Added
- New `popcount()` function for uint128_t

### Fixed
- Division by zero edge case in signed division
```

### Categorias permitidas

| Categoria | Cuando usar |
|---|---|
| `Added` | Nuevas funcionalidades |
| `Changed` | Cambios en funcionalidad existente |
| `Deprecated` | Funcionalidades que se eliminaran en el futuro |
| `Removed` | Funcionalidades eliminadas |
| `Fixed` | Correcciones de bugs |
| `Security` | Correcciones de vulnerabilidades |

### Versionado Semantico (SemVer 2.0)

```
MAJOR.MINOR.PATCH

MAJOR: cambio incompatible de API (breaking change)
MINOR: nueva funcionalidad compatible hacia atras
PATCH: correccion de bug compatible hacia atras
```

| Cambio | Ejemplo | Version bump |
|---|---|---|
| Eliminar funcion publica | Quitar `to_cstr()` | MAJOR |
| Cambiar firma de funcion | `to_string()` ahora retorna `std::string_view` | MAJOR |
| Nueva funcion | Agregar `bit_ceil()` | MINOR |
| Nuevo modulo/header | `mi_lib_ranges.hpp` | MINOR |
| Fix de bug sin cambio de API | Corregir division signed | PATCH |
| Mejora de rendimiento | Optimizar multiplicacion | PATCH |

### Reglas

- **`[Unreleased]`** siempre existe al inicio — acumula cambios pre-release.
- **Fecha en formato ISO 8601:** `YYYY-MM-DD`.
- **Una entrada por cambio** — no agrupar multiples cambios en una linea.
- **Actualizar en cada commit significativo** — no al final de la sesion.
- **Pre-release:** usar sufijos como `1.0.0-alpha`, `1.0.0-beta.1`, `1.0.0-rc.1`.

---

## 21. Conventional Commits

### Formato

```
<type>(<scope>): <description>

[optional body]

[optional footer(s)]
```

### Tipos permitidos

| Tipo | Proposito | Ejemplo |
|---|---|---|
| `feat` | Nueva funcionalidad | `feat(algorithm): add constexpr sort for uint128_t` |
| `fix` | Correccion de bug | `fix(division): correct signed division for INT_MIN` |
| `perf` | Mejora de rendimiento | `perf(multiply): optimize karatsuba threshold` |
| `test` | Agregar/modificar tests | `test(format): add edge cases for hex formatting` |
| `docs` | Documentacion | `docs(api): update API_CORE.md with new functions` |
| `chore` | Mantenimiento | `chore(ci): update GCC matrix to v15` |
| `refactor` | Refactorizacion sin cambio funcional | `refactor(intrinsics): extract common pattern` |
| `style` | Formato/estilo (no cambia logica) | `style: apply clang-format to all headers` |
| `build` | Sistema de build | `build(cmake): add sanitizer detection module` |
| `ci` | CI/CD | `ci: add Clang-21 to test matrix` |

### Scopes validos

Nombres de features (`algorithm`, `bits`, `format`, `traits`, `division`, etc.)
mas scopes generales: `ci`, `build`, `docs`, `deps`, `core`.

### Breaking changes

```
feat(core)!: change storage layout to big-endian

BREAKING CHANGE: data[0] is now the most significant limb.
All serialization code must be updated.
```

### Reglas

- **Primera linea:** maximo 72 caracteres.
- **Body:** separado por linea en blanco, explica el "por que".
- **Footer:** `BREAKING CHANGE:`, `Fixes #123`, `Refs #456`.
- **Un commit = un cambio logico** — no mezclar feat + fix en un commit.

---

## 22. Estrategia de Branching

### Modelo de Ramas

| Branch | Proposito | Protegido | Merge a |
|---|---|---|---|
| `main` | Release estable | Si (requiere PR + CI green) | — |
| `develop` | Integracion de features | Si (requiere CI green) | `main` |
| `phase-X.YZ` | Fase de desarrollo activa | No | `develop` |
| `feature/nombre` | Feature individual | No | `develop` o `phase-*` |
| `fix/nombre` | Correccion de bug | No | `develop` o `main` |
| `experiment/nombre` | Pruebas exploratorias | No | Se descarta o merge |
| `release/vX.Y.Z` | Preparacion de release | No | `main` + tag |

### Flujo tipico

```
feature/new-algorithm --> develop --> release/v1.2.0 --> main (tag v1.2.0)
                               \                          /
                                +--- fix/hotfix-xxx ------+
```

### Reglas

- **`main` siempre compila** con todos los compiladores.
- **`develop`** puede tener features incompletos pero debe compilar.
- **Branches de feature** se eliminan tras merge.
- **Tags** solo en `main`: `v1.0.0`, `v1.1.0-beta.1`.
- **Nunca force-push** a `main` o `develop`.

---

## 23. Politica de Estabilidad de API

### Niveles de estabilidad

| Nivel | Significado | Marcado en codigo | Garantia |
|---|---|---|---|
| **Stable** | No cambiara sin MAJOR bump | Sin marca (default) | Compatible dentro de MAJOR |
| **Provisional** | Puede cambiar en MINOR | `// @stability provisional` | Sin garantia entre MINOR |
| **Experimental** | Puede desaparecer | `// @stability experimental` | Sin garantia |
| **Deprecated** | Se eliminara en proxima MAJOR | `[[deprecated("use X")]]` | Funciona pero con warning |

### Ciclo de vida de una funcion publica

```
Experimental --> Provisional --> Stable --> Deprecated --> Removed
    (0.x)          (1.x)         (1.x+)      (N.x)       (N+1.0)
```

### Reglas de deprecacion

1. **Marcar con `[[deprecated]]`** y mensaje indicando el reemplazo.
2. **Documentar en CHANGELOG** bajo `Deprecated`.
3. **Mantener al menos 1 version MINOR** antes de eliminar.
4. **Documentar en migration guide** (`docs/guides/migration_guide.md`).

```cpp
// CORRECTO - deprecar con reemplazo claro
[[deprecated("Use to_string() instead. Will be removed in v3.0.0")]]
constexpr const char* to_cstr() const noexcept;
```

---

## 24. SESSION_STATE -- Contexto Persistente

### Proposito

Archivo volatil que mantiene un **snapshot del estado actual** del proyecto.
Permite que cualquier agente IA o desarrollador que entre "en frio" sepa
exactamente donde esta todo sin leer el CHANGELOG completo.

### Contenido obligatorio

```markdown
# SESSION_STATE - [Nombre del Proyecto]

**Ultima actualizacion:** YYYY-MM-DD HH:MM

## Estado Actual
- **Fase:** X.YZ (branch: phase-X.YZ)
- **Ultimo cambio:** [descripcion breve]
- **Compiladores verificados:** GCC 15 [OK], Clang 19 [OK], MSVC [OK], Intel [PENDING]

## Lo que funciona
- [Feature A]: completo, tests pasan en todos los compiladores
- [Feature B]: completo excepto Intel

## Lo que esta roto / pendiente
- [Bug X]: division signed falla con -O3 en GCC 14
- [Feature C]: implementacion al 60%, faltan tests

## Proximos pasos (1-3 acciones inmediatas)
1. Corregir bug X
2. Completar tests de Feature C
3. Actualizar documentacion API

## Decisiones recientes
- Se decidio usar Karatsuba para multiplicacion (ver ADR-005)
```

### Diferencia con CHANGELOG

| CHANGELOG | SESSION_STATE |
|---|---|
| Historico y acumulativo | Snapshot volatil del presente |
| Nunca se borra contenido | Se sobrescribe en cada sesion |
| Para usuarios y releases | Para onboarding rapido de agentes/devs |
| Formato rigido (Keep a Changelog) | Formato libre pero con secciones fijas |

### Reglas

- **Actualizar al inicio y final** de cada sesion de desarrollo.
- **Sobrescribir** — no acumular. Solo refleja el estado actual.
- **Versionado en git** — pero el historial no importa (el CHANGELOG tiene eso).

---

## 25. ROADMAP -- Linea de Tiempo del Proyecto

### Proposito

Mapa cronologico con tres zonas: completado, en curso, y futuro.
Permite ver de un vistazo el progreso del proyecto y las metas pendientes.

### Formato

```markdown
# ROADMAP - [Nombre del Proyecto]

## Completado

### Phase 1.0 - Tipo base (completado YYYY-MM)
- [x] Tipo base con almacenamiento uint64_t[2]
- [x] Operadores aritmeticos basicos (+, -, *, /)
- [x] Operadores de comparacion
- [x] Conversion a/desde string

### Phase 1.5 - Unificacion template (completado YYYY-MM)
- [x] Template parametrizado por signedness
- [x] Type traits y conceptos

## En Curso

### Phase 1.75 - Representaciones parametrizadas (inicio YYYY-MM)
- [x] Two's Complement (TC)
- [ ] Magnitude-Sign (MS) - 80% completado
- [ ] Excess-K (EK) - pendiente
- [ ] Safe arithmetic wrappers

## Futuro

### Phase 2.0 - Fixed-point (estimado YYYY)
- [ ] Tipos fixed-point parametrizados
- [ ] Operadores aritmeticos fixed-point

### Phase 3.0 - Big integers (estimado YYYY)
- [ ] Enteros de precision arbitraria
- [ ] Algoritmos de Karatsuba, Toom-Cook
```

### Reglas

- **Cada fase tiene:** nombre, estado, fecha de inicio/cierre.
- **Criterios de completitud** explicitos (checkboxes).
- **Actualizar al completar cada milestone.**
- **No eliminar fases completadas** — son registro historico.

---

## 26. Architecture Decision Records (ADRs)

### Proposito

Documentar decisiones de diseno importantes, su contexto, y las alternativas
consideradas. Permite entender el "por que" detras del codigo.

### Ubicacion

```
docs/decisions/
+-- README.md    # indice, estado de cada ADR y nota sobre los 001-005
+-- TEMPLATE.md
+-- ADR-001-constructores-y-conversiones-explicitos.md
+-- ADR-002-almacenamiento-little-endian-de-limbos.md
+-- ADR-003-std-byte-para-buffers.md
+-- ADR-004-sin-excepciones-en-el-nucleo.md
+-- ADR-005-representacion-como-parametro-de-plantilla.md
+-- ADR-006 .. ADR-010
```

### Formato (plantilla)

```markdown
# ADR-NNN: Titulo de la Decision

**Estado:** Aceptado | Propuesto | Deprecado | Reemplazado por ADR-XXX
**Fecha:** YYYY-MM-DD
**Autor:** Nombre

## Contexto
Que problema o necesidad motiva esta decision.

## Decision
Que se decidio hacer.

## Consecuencias
### Positivas
- Beneficio 1
- Beneficio 2

### Negativas
- Coste/tradeoff 1

## Alternativas Consideradas
### Alternativa A
- Descripcion y por que se descarto

### Alternativa B
- Descripcion y por que se descarto
```

### Reglas

- **Numeracion secuencial:** ADR-001, ADR-002, ADR-003...
- **Inmutables una vez aceptados** — si cambia la decision, crear un nuevo ADR
  que referencie al anterior.
- **Referenciar desde el codigo** con `// See ADR-NNN`.
- **No requiere consenso formal** — documentar la decision tomada es suficiente.

---

## 27. Known Issues por Plataforma

### Proposito

Tabla centralizada de problemas conocidos por compilador/plataforma con su
estado y workaround.

### Formato

| ID | Problema | Compilador | Version | Plataforma | Estado | Workaround |
|---|---|---|---|---|---|---|
| KI-001 | constexpr-if bug at -O2 | GCC | 14.x | Linux | Reportado | Usar -O1 o -O3 |
| KI-002 | Intel linking en MSYS2 | ICX | all | Windows | Permanente | `source setup_intel.bash` |
| KI-003 | TSan false positive en atomic | Clang | 18 | Linux | Upstream bug | Suprimir con `__tsan_annotate` |
| KI-004 | `/std:c++20` vs `/std:c++latest` | MSVC | 2026 | Windows | Documentado | Usar `/std:c++latest` |

### Reglas

- **ID unico** por issue: `KI-NNN`.
- **Estados:** `Reportado`, `Workaround`, `Permanente`, `Resuelto en vX.Y`.
- **Actualizar** cuando se resuelva o cambie el workaround.
- **Referenciar desde tests** que se saltan: `// Skip: see KI-003`.

---

## 28. Metricas de Calidad Minimas

### Umbrales obligatorios

| Metrica | Umbral minimo | Herramienta | Cuando |
|---|---|---|---|
| **Compilacion limpia** | 0 warnings (con `-Wall -Wextra -Wpedantic`) | Compilador | Cada build |
| **Tests passing** | 100% en todos los compiladores | CTest | Cada PR |
| **Sanitizers** | 0 errores ASan + UBSan | Clang | Cada PR |
| **Cobertura de tests** | >= 90% lineas en modulos core | GCov/llvm-cov | Semanal |
| **Analisis estatico** | 0 errores nivel error/warning | cppcheck | Cada PR |
| **Formato** | 100% conforme a .clang-format | clang-format | Cada PR |
| **Documentacion** | 0 warnings Doxygen en API publica | Doxygen | Cada PR |

### Benchmarks (regresiones)

| Metrica | Umbral | Accion |
|---|---|---|
| Regresion > 5% en operacion critica | Warning en PR | Investigar causa |
| Regresion > 15% | Bloquea merge | Requiere justificacion o fix |
| Mejora > 10% | Informativo | Actualizar baseline |

### Baselines de rendimiento

```
benchs/baselines/
+-- baseline_gcc-15_release-O2.json
+-- baseline_clang-19_release-O2.json
+-- baseline_msvc_release.json
```

- **Formato JSON** con ns/operacion para cada benchmark.
- **Actualizar baselines** solo en releases tag (nunca automaticamente).
- **Comparar en CI** contra el baseline del compilador correspondiente.

---

## 29. Flujo de Trabajo del Desarrollador

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

## 30. Reglas para Agentes IA

### Reglas Críticas

> **Referencia completa:** [`STYLE_CONVENTIONS.md`](STYLE_CONVENTIONS.md)
> contiene todas las reglas con ejemplos detallados.

1. **Nunca compilar manualmente.** Usar siempre `python make.py ...` o el workflow establecido.
2. **Nunca crear archivos en raíz** salvo los listados en la sección 12.
3. **Nunca crear ejecutables (.exe, .obj) fuera de `build/`.**
4. **Nunca modificar `legacy-code/`.** Es inmutable y de solo lectura.
5. **Reportar estado cada ~3 minutos** en sesiones activas de desarrollo.
6. **ASCII-only en output** de consola C++ — sin Unicode.
7. **Actualizar CHANGELOG.md** tras cada cambio significativo.

### Modo de Trabajo Autónomo

El agente IA tiene **autonomía completa** para ejecutar Python y compilar:

- **Ejecutar Python sin supervisión:** `python make.py ...`, scripts de utilidad, etc.
- **Compilar sin supervisión:** build, check, test, demo, benchmarks — todo via `make.py`.
- **Ejecutar tests y reportar resultados** sin esperar confirmación del usuario.
- **Iterar autónomamente:** editar código → compilar → ejecutar tests → corregir errores.

El agente **NO necesita pedir permiso** para:

- Ejecutar `python make.py build/check/test/demo/run ...`
- Ejecutar scripts Python auxiliares del proyecto
- Compilar con cualquier compilador/modo disponible
- Ejecutar la suite completa de tests

El agente **SÍ debe pedir confirmación** para:

- Operaciones destructivas (`git push --force`, `git reset --hard`, borrar archivos)
- Cambios arquitecturales mayores que afecten múltiples módulos
- Operaciones que afecten repositorios remotos o sistemas compartidos

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
| **ai-instructions.md** | [`STYLE_CONVENTIONS.md`](STYLE_CONVENTIONS.md) | Reglas detalladas para agentes IA: rutas de compiladores, jerarquía de build, std::byte, byteswap, ASCII, licencias. Complementa §13, §18, §20. |
| **Explicación del Proyecto** | [`AI_PROMPT/GENERAL_GUIDES/Explicación_del_Proyecto.md`](AI_PROMPT/GENERAL_GUIDES/Explicaci%C3%B3n_del_Proyecto.md) | Visión general del proyecto: 12 objetivos, 9 etapas (TC, MS, EK, fixed-point, big integers, etc.). Complementa §1. |
| **CI Workflow** | [`AI_PROMPT/workflow/ci.yml`](AI_PROMPT/workflow/ci.yml) | Definición completa de CI multi-compilador (~400 líneas). Complementa §16. |
| **Benchmarks Workflow** | [`AI_PROMPT/workflow/benchmarks.yml`](AI_PROMPT/workflow/benchmarks.yml) | Suite semanal de benchmarks (~170 líneas). Complementa §16. |
| **Release Workflow** | [`AI_PROMPT/workflow/release.yml`](AI_PROMPT/workflow/release.yml) | Empaquetado automático de releases (~150 líneas). Complementa §16. |
| **CHANGELOG.md** | [`CHANGELOG.md`](CHANGELOG.md) | Registro de cambios del proyecto. |
| **README.md** | [`README.md`](README.md) | Presentación pública del proyecto. |

---

## 31. Comandos Interactivos para la IA

Adaptación al dominio C++ de la sección homónima de las guías de Lean 4
(`lean4-project-template/AI-GUIDE.md`). La IA debe obedecer estos comandos
exactos cuando el usuario los invoque en el chat. Cada uno tiene además su
*slash command* en `.claude/commands/`, de modo que `/proyecta`, `/documenta`,
`/actualiza_doc` y `/guarda_y_sube` hacen lo mismo que escribir el nombre.

Los tres se apoyan en dos scripts que son la parte automatizable del trabajo:

| Script | Qué comprueba |
|---|---|
| `scripts/check_docs_consistency.py` | Enlaces rotos, cifras de tests desactualizadas, correspondencia `API_*.md` ↔ headers, cabeceras SPDX, `LICENSE.txt`, avisos de Doxygen, coherencia de fechas |
| `scripts/check_headers_selfcontained.py` | Que cada header de `include/` compile aislado y que sus guardas sean idempotentes |

---

### `PROYECTA`

**Propósito:** actualización *local* de la documentación de API para los headers
que se han tocado en la sesión. Es el equivalente C++ del `proyecta` de Lean 4:
allí se proyecta el bloque `export` de un módulo al árbol `REFERENCE`; aquí se
proyecta la API pública de un header a su `docs/API_*.md`.

**Pasos (en orden):**

1. Identificar los `.hpp` de `include/` modificados en la sesión
   (`git diff --name-only`).
2. Por cada uno, extraer su **API pública**: clases, funciones libres, aliases,
   traits y concepts que no estén en `detail::` ni en una sección `private:`.
3. Comprobar que cada símbolo público tiene su comentario Doxygen con `@brief`,
   `@param` y `@return` donde apliquen.
4. Actualizar el `docs/API_*.md` correspondiente: sinopsis, tabla de funciones
   con su firma, semántica y complejidad, y al menos un ejemplo compilable.
5. Verificar que **no se ha filtrado nada privado**: ningún símbolo de
   `detail::`, ningún miembro privado, ningún helper con sufijo `_`.
6. Verificar lo contrario: que **nada público se ha quedado sin proyectar**.
7. Ejecutar `python scripts/check_docs_consistency.py` y dejarlo en verde.

**No hace:** tocar README, CHANGELOG ni NEXT_STEPS. Para eso está
`ACTUALIZA_DOC`.

---

### `DOCUMENTA`

**Propósito:** generación y control de calidad de la documentación Doxygen.

**Pasos (en orden):**

1. `doxygen Doxyfile` y anotar el número de avisos.
2. **Criterio duro: cero avisos atribuibles a `include/`.** Los avisos que no
   son culpa del código (la traducción incompleta de Doxygen al español, los
   enlaces del README que sí funcionan al navegar por GitHub pero que Doxygen no
   resuelve como referencia interna) están en la lista blanca de
   `check_docs_consistency.py`, cada uno con su motivo escrito. Cualquier aviso
   nuevo hace fallar la comprobación: la lista blanca no crece sin justificación.
3. Medir la cobertura de comentarios de los headers tocados y anotarla.
4. Regenerar los `docs/API_*.md` que falten.
5. `python scripts/check_docs_consistency.py --doxygen`.
6. Reportar en el chat: avisos antes y después, cobertura y ficheros generados.

**Errores típicos que este comando ha destapado:**

- `@example` usado para introducir un ejemplo en línea. En Doxygen ese comando
  significa «este comentario documenta un fichero de ejemplo», así que hacía que
  el header entero se tratase como ejemplo. Lo correcto es `@par Ejemplo:`.
- Nombres de cabeceras estándar escritos como `<atomic>` dentro de un
  comentario: Doxygen los lee como etiquetas HTML. Van entre acentos graves.
- `@param` con un nombre que no está en la firma, casi siempre porque el
  parámetro no tiene nombre en la declaración.
- Comandos inventados (`@complexity`) que hay que declarar en `ALIASES`.

---

### `ACTUALIZA_DOC`

**Propósito:** pasada completa al terminar una sesión de trabajo. Sincroniza los
documentos vivos con el estado real del código.

**Pasos (en orden):**

1. `python make.py test gcc release-O2` — anotar el resultado real, no el
   esperado.
2. Leer el estado previo de `NEXT_STEPS.md`, `CHANGELOG.md` y
   `PROJECT_STATUS.md`.
3. Identificar qué ha cambiado: tareas cerradas, ficheros nuevos, APIs añadidas,
   comportamientos modificados.
4. `CHANGELOG.md`: entrada nueva con fecha y los cambios de la sesión.
5. `NEXT_STEPS.md`: mover a completado lo que se ha cerrado, con el hash del
   commit. Si una tarea reveló trabajo nuevo, añadirlo.
6. `PROJECT_STATUS.md`: instantánea del estado de compilación y de la suite.
7. `README.md`: métricas y cifras de la cabecera.
8. Invocar `PROYECTA` para los headers tocados.
9. **Verificar la coherencia**: `python scripts/check_docs_consistency.py`.
   Tiene que quedar en verde antes de dar la sesión por cerrada.
10. Reportar en el chat: qué se cerró, qué ficheros se tocaron y qué queda.

**Regla que motivó este comando:** la auditoría del 23 ago 2026 encontró el
README diciendo «42/42 tests» en la cabecera, «106/106» en una sección y
«197/197» en otra, y enlazando a cuatro ficheros inexistentes. Nada de eso
rompe una compilación, así que había sobrevivido meses. El paso 9 existe
justamente para que eso no vuelva a pasar en silencio.

---

### `GUARDA_Y_SUBE`

**Propósito:** flujo de git seguro. Equivalente C++ del `guarda_y_sube` de las
guías de Lean 4: allí el flujo gira en torno a los bloqueos de fichero
(`git-lock.bash`), aquí no hay bloqueos y su sitio lo ocupan los verificadores.

**Nada se sube sin pasar los cuatro verificadores.** Si alguno falla, se arregla
o se dice que no se sube; no se sube «con eso pendiente».

**Pasos (en orden):**

1. `python make.py test gcc release-O2`. Anotar el resultado real. Si falla, parar.
2. Verificadores, los mismos que exige el job `format-and-docs` del CI:
   - `python scripts/check_headers_selfcontained.py`
   - `python scripts/check_docs_consistency.py --doxygen`
   - `clang-format --dry-run --Werror` sobre los ficheros tocados
3. `git status --short` y `git diff --stat`. **Nunca `git add -A` a ciegas**: se
   añaden ficheros concretos y solo tras mirar la lista. Si aparece algo
   inesperado (binarios, `.orig`, `.bak`), se investiga antes de seguir.
4. Commit con mensaje descriptivo: qué cambia, **por qué**, y qué se verificó.
   Conventional commits para el prefijo, `!` si rompe compatibilidad. Si el
   trabajo son varias cosas separables, varios commits, cada uno compilable y con
   la suite en verde, para que `git bisect` sirva de algo.
5. `git push origin <rama>`, y confirmar con `git log --oneline origin/<rama> -1`.
6. Reportar en el chat: commits que han entrado, qué se verificó y qué queda.

---

### Relación con los comandos de las guías de Lean 4

| Lean 4 | C++ | Diferencia |
|---|---|---|
| `proyecta` | `PROYECTA` | Allí se proyecta el bloque `export` al árbol `REFERENCE`; aquí, la API pública de un header a su `docs/API_*.md` |
| `actualiza doc` | `ACTUALIZA_DOC` | Allí se cuentan `sorry`; aquí, tests que pasan y tareas cerradas |
| `repasa_y_proyecta` | `scripts/check_docs_consistency.py` | Automatizado: en C++ la coherencia es comprobable con un script, no hace falta que la revise la IA a mano |
| `compila_y_comprueba` | `python make.py test` | Ya existía |
| `guarda_y_sube` | `GUARDA_Y_SUBE` | Allí el flujo gira en torno a los bloqueos de fichero; aquí, en torno a los cuatro verificadores |
| — | `DOCUMENTA` | No tiene equivalente en Lean 4: allí la documentación se genera del propio código con `lake`; aquí hace falta Doxygen y su control de avisos |

---

*Fin de AI-GUIDE.md*
