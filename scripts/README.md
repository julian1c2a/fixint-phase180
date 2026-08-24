# `scripts/` — inventario

**Última revisión:** 24 August 2026 (T7.6 del plan de auditoría)

En este directorio convivían 47 ficheros de varias generaciones del proyecto. La
auditoría del 23 ago 2026 encontró que la mitad no los referencia nadie: son
ancestros de los tres scripts genéricos que hoy hacen el trabajo. Este
documento dice cuál es cuál, para que nadie tenga que averiguarlo leyéndolos.

El criterio de la columna «Referencias» es cuántos ficheros del repositorio
(`.py`, `.bash`, `.md`, `.yml`, `Makefile`) mencionan el script.

---

## Capa canónica

Lo que usa `make.py` hoy. Tocar aquí es tocar el build de verdad.

| Script | Papel |
|---|---|
| `build_generic.py` | Compila cualquier objetivo (tests, benchs, demos) con cualquier compilador y modo. Es el corazón del sistema |
| `run_generic.py` | Ejecuta binarios ya compilados y mide tiempos |
| `check_generic.py` | Compila **y** ejecuta, y da el resumen de resultados |
| `toolchains.py` | Fuente única de verdad de qué compilador se usa (T7.5). Lee `toolchains.json` |
| `build_demos.py` | Compilación de los demos |
| `vcvars.py` | Localiza y aplica el entorno de MSVC |
| `init_project.py` | Prepara el árbol de directorios de build |

## Verificación (nuevos, de la auditoría)

| Script | Papel |
|---|---|
| `check_headers_selfcontained.py` | Cada header de `include/` compila aislado, y por duplicado para comprobar las guardas (T2.5) |
| `check_docs_consistency.py` | Armonizador de documentación: enlaces, cifras de tests, SPDX, licencia, Doxygen, fechas (T6.5) |

## Utilidad puntual

| Script | Papel |
|---|---|
| `benchmark_comparison.bash` | Comparativa contra GMP, TomMath y Boost.Multiprecision |
| `run_static_analysis.bash` | Pasada de cppcheck y clang-tidy en local |
| `build_with_sanitizers.bash` | Compila con ASan/UBSan |
| `generate_docs.bash` / `generate_docs.bat` | Envoltorio de Doxygen. Hoy lo cubre `/documenta` |
| `cleanup_unicode.py` | Convierte a ASCII la salida de consola (regla del proyecto) |
| `wsl_build_and_test.bash` | Compilación y tests en WSL |
| `compile_wsl.bash` | Compilación puntual en WSL |

## Todavía vivos, pese a estar superados

Estos cinco **siguen siendo llamados desde código vivo**, así que no se han
archivado aunque su sustituto en Python exista:

| Script | Quién lo llama |
|---|---|
| `build_generic.bash` | `Makefile` |
| `run_generic.bash` | `Makefile` |
| `check_generic.bash` | `Makefile` |
| `build_benchs_generic.bash` | `Makefile` |
| `build_extracted_tests.bash` | `.github/workflows/release.yml` |

Desengancharlos es una tarea aparte y con riesgo: significa convertir el
`Makefile` en el shim sobre `make.py` que propone la sección de más abajo, y
tocar el workflow de release, que no se puede probar en local. Hasta entonces se
quedan donde están.

> El primer intento de archivarlos, el 24 ago 2026, los daba por muertos según
> un recuento de referencias mal filtrado. Lo pilló la comprobación que se hace
> al mover: **ningún script se archiva sin verificar antes que nada vivo lo
> referencia.**

## Archivados en `archive/`

Movidos a [`archive/`](archive/) el 24 ago 2026: 27 ficheros que no referencia
nadie. Son la generación anterior a `build_generic.py` / `run_generic.py` /
`check_generic.py`, los de la fase 1.5, o herramientas de refactores ya
aplicados. Se conservan porque alguno sigue valiendo como referencia de cómo se
hacía algo; recuperarlos es un `git mv`.

| Script | Sustituido por |
|---|---|
| `build_tests_generic.bash` | `build_generic.py` |
| `build_phase15_tests.bash` | `build_generic.py` (fase 1.5, ya cerrada) |
| `build_phase15_benchs.bash` | `build_generic.py` (fase 1.5, ya cerrada) |
| `check_phase15_tests.bash` | `check_generic.py` (fase 1.5, ya cerrada) |
| `run_phase15_benchs.bash` | `run_generic.py` (fase 1.5, ya cerrada) |
| `check_direct.bash` | `check_generic.py` |
| `run_direct.bash` | `run_generic.py` |
| `run_direct_check.bash` | `check_generic.py` |
| `run_all_tests.bash` | `python make.py test` |
| `run_all_compilers.bash` | `python make.py test all` |
| `run_priority_tests_multicomp.bash` | `python make.py test` (la suite ya no se divide por prioridades) |
| `build_all_demos.bash` | `build_demos.py` |
| `build_demo.bash` | `build_demos.py` |
| `build_demos.bash` | `build_demos.py` |
| `catalog_demos.bash` | — (catálogo de demos que ya no se mantiene) |
| `test_demos.bash` | `check_generic.py` |
| `run_demo.bash` | `run_generic.py` |
| `wsl_build_system.bash` | `wsl_build_and_test.bash` |
| `wsl_build_and_test.py` | `wsl_build_and_test.bash` |
| `run_wsl_tests.py` | `wsl_build_and_test.bash` |
| `run_uint128_extracted_tests.bash` | — (ídem) |
| `fix_structure.bash` | — (refactor ya aplicado) |
| `fix_benchmark_api.py` | — (refactor ya aplicado) |
| `refactor_data_access.py` | — (refactor ya aplicado) |
| `update_benchmarks_includes.py` | — (refactor ya aplicado) |
| `verify_refactor.bash` | — (refactor ya aplicado) |
| `verify_stdbyte_migration.bash` | — (migración a `std::byte` ya aplicada) |

**No se han borrado**, solo movidos: alguno puede seguir siendo útil como
referencia. Con esto el directorio pasa de aparentar 47 herramientas vivas a las
20 que hay de verdad.

---

## Jerarquía del sistema de construcción

Tal como está pensado (§13 de `AI-GUIDE.md`), y no hay motivo para cambiarlo:

```
python make.py            <- interfaz de usuario, task runner
    |
    v
scripts/*_generic.py      <- entornos por compilador, flags, rutas
    |
    v
CMake / CMakePresets      <- compilación estructurada, CTest
```

Y en paralelo, para probar en otras plataformas:

- **WSL** — `wsl_build_and_test.bash`, para Linux desde Windows
- **Docker** — `docker/Dockerfile` (amd64) y `Dockerfile.crosstest` (arm64,
  arm32, riscv64 vía QEMU), para otras arquitecturas
