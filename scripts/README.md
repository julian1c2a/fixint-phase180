# `scripts/` — inventario

**Última revisión:** 23 August 2026 (T7.6 del plan de auditoría)

En este directorio conviven 47 ficheros de varias generaciones del proyecto. La
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

## Superados — candidatos a archivar

Ninguno de estos lo referencia nadie salvo, como mucho, otro script igual de
muerto. Son la generación anterior a `build_generic.py` / `run_generic.py` /
`check_generic.py`, o herramientas de refactores que ya se hicieron.

| Script | Sustituido por |
|---|---|
| `build_generic.bash` | `build_generic.py` |
| `run_generic.bash` | `run_generic.py` |
| `check_generic.bash` | `check_generic.py` |
| `build_tests_generic.bash` | `build_generic.py` |
| `build_benchs_generic.bash` | `build_generic.py` |
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
| `build_extracted_tests.bash` | — (los «extracted tests» ya no existen) |
| `run_uint128_extracted_tests.bash` | — (ídem) |
| `fix_structure.bash` | — (refactor ya aplicado) |
| `fix_benchmark_api.py` | — (refactor ya aplicado) |
| `refactor_data_access.py` | — (refactor ya aplicado) |
| `update_benchmarks_includes.py` | — (refactor ya aplicado) |
| `verify_refactor.bash` | — (refactor ya aplicado) |
| `verify_stdbyte_migration.bash` | — (migración a `std::byte` ya aplicada) |

**No se han borrado.** Son 32 ficheros y borrarlos es una decisión del autor, no
de una auditoría: alguno puede seguir siendo útil como referencia de cómo se
hacía algo. La recomendación es moverlos a `scripts/archive/` en un commit
propio, para que el directorio deje de dar a entender que hay 47 herramientas
vivas cuando en realidad son unas 15.

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
