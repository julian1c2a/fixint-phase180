# 🔮 NEXT STEPS - v1.90: fixed_int_t\<N\> generalización

**Status:** v1.80 ✅ | v1.81 MS-INTEROP ✅ | `fixed_int_t<N>` unificado ✅ | fast paths N=2 ✅ | single-limb fast path ✅ | **Knuth D ✅** | **Karatsuba N=4/8 ✅** | **Auditoría 23 ago 2026 ✅**
**Last Updated:** 23 August 2026
**Focus:** plan integrado post-auditoría (v1.90.1), sección siguiente. Objetivo de rama: `div`/`mod` constexpr + `fixed_int_t` sustituyendo a los tipos de 256 bits anteriores.

---

## 🔍 AUDITORÍA 23 ago 2026 — Plan integrado (v1.90.1)

> Auditoría completa del proyecto + comentarios del autor (18 puntos). Este bloque
> es el **plan vigente**: sustituye a la tabla "Pendiente (prioridad)" de v1.90, cuyos
> items 1-3 (`to_string`/`from_string` N>2, `checked_*`) están **ya implementados y
> verificados**.

### Evidencia recogida en la auditoría

| Comprobación | Resultado |
|---|---|
| `python make.py test gcc release-O2` | **52/52 ficheros OK** (227 s) |
| Warnings `-Wall -Wextra -Wshadow -Wconversion -Wsign-conversion` | 2 sitios cosméticos en librería |
| Suite tras Fase 0 + T1.1 + T1.4 | **53/53** (GCC 16.2.0 ucrt64, 216 s); `test_fixed_string_io` nuevo, 74/74 |
| **Fuzz diferencial vs enteros grandes de Python** (N=2/4/8, signed+unsigned, `+ - * / % & << >>`, `to_string`) | **17.600 ops, 0 discrepancias** |
| Headers auto-contenidos | 27/28 |
| `TODO`/`FIXME` en `include/` | 0 |

**Conclusión:** el núcleo matemático (Knuth D, Karatsuba, string, comparaciones) es
sólido. La deuda está en empaquetado, higiene, puertas de calidad del CI e integración STL.

---

### Respuestas a las preguntas abiertas del autor

#### [P3] ¿Cómo se arregla la inyección de `-fconstexpr-steps=100000000`?

**Diagnóstico:** `GM_TABLE` ([int128_param_divmod.hpp:308](include/int128_param_divmod.hpp#L308))
son 1024 entradas × ~2000 pasos constexpr ≈ 2M pasos, sobre un límite Clang por defecto de 1M.
Verificado: falla en **Clang 14 y Clang 22** por igual. Hoy lo tapa
[build_generic.py:205](scripts/build_generic.py#L205) y el job de sanitizers.
Cualquier consumidor externo que haga `#include` con Clang **no compila**.

**Solución recomendada (de fondo):** que la tabla deje de ser `constexpr` obligatoria.

1. `inline const std::array<...> GM_TABLE` inicializada en runtime → **0 pasos constexpr**,
   y de paso baja el tiempo de compilación de todo el que incluya el header.
2. Para el camino constexpr, `template <std::uint64_t D> inline constexpr gm_entry gm_for_v = compute_magic_128(D);`
   — se calcula **solo el divisor que se use**, no los 1024. Coste: ~2000 pasos, tres
   órdenes de magnitud bajo el límite.
3. `div<D>`/`mod<D>`/`divmod_const<D>` eligen: en contexto constante → `gm_for_v<D>`;
   en runtime → `GM_TABLE[D]`.

**Red de seguridad (aplicar ya, aunque se haga lo anterior):** exponer el flag como
propiedad del target CMake INTERFACE (`target_compile_options(... INTERFACE $<$<CXX_COMPILER_ID:Clang>:-fconstexpr-steps=...>)`)
para que **viaje con el paquete** en vez de ser un secreto de los scripts.

**Test de no-regresión:** un job de CI que compile un TU con Clang **sin** el flag.
Mientras ese job pase, el problema no puede volver a colarse.

#### [P4] Los 3 jobs de CI que no pueden fallar, ¿eran temporales sin documentar?

**No hay rastro de que fueran temporales.** Nacieron así en `630ae18` (20 mar 2026,
"feat(ci): multi-arch CI/CD"), el commit que introdujo el multi-arch. Sin `TODO`, sin
comentario, sin issue. El mensaje de ese commit dice "49/49 tests pass GCC + Clang",
es decir, los jobs de arquitectura eran exploratorios y la tolerancia se quedó congelada.

- `cross-arm32` → `continue-on-error: true` explícito ([ci.yml:237](.github/workflows/ci.yml#L237))
- `cross-x86-32` ([ci.yml:310](.github/workflows/ci.yml#L310)) e `intel-icx` ([ci.yml:485](.github/workflows/ci.yml#L485)) → cuentan fallos, imprimen resumen y **nunca hacen `exit 1`**
- aarch64/riscv64 → toleran hasta 10% de tests rotos ([líneas 161, 182, 273, 292](.github/workflows/ci.yml#L182)) y mandan errores de compilación a `/dev/null`

**Propuesta:** ponerlos estrictos de golpe y ver qué cae de verdad. Si algo está roto
en una arquitectura concreta, allowlist **explícita y nominal** (`ALLOWED_FAIL="test_x test_y"`)
con comentario del porqué — nunca una tolerancia porcentual anónima.

#### [P6] ¿Seguimos trabajando en la integración STL de `fixed_int_t`?

**El objetivo de esta rama es que `fixed_int_t<N>` ocupe el sitio de los tipos de 256 bits
anteriores**, así que la integración STL no es un "nice to have": es **requisito de paridad**.
El predecesor `int128_param_t` ya tiene `std::hash`, `std::formatter` e iostreams
(`int128_param_format.hpp`, `int128_param_iostreams.hpp`). `fixed_int_t` **no tiene ninguno
de los tres** — no se puede imprimir, ni meter en `unordered_map`, ni usar con `std::format`.
El fichero suelto `include/test_fixed_string_io.cpp` (sin trackear, escrito con gtest, que
no es dependencia del proyecto) es evidencia de que esto se empezó y quedó a medias.

#### [P12b] ¿`int128_param_traits_specializations.hpp` debe ser autocontenido?

**Sí**, porque **es API pública de facto**: lo incluyen directamente
`tests/test_param_traits.cpp:10` y `tests/test_traits_specializations.cpp:13`. La regla
estándar (headers auto-contenidos / IWYU) dice que todo header instalable debe compilar solo.

Dos caminos, y hay que elegir uno explícitamente:
- **(a) Hacerlo autocontenido** — que incluya `int128_parameterized.hpp` en su cabecera.
  Es lo correcto si es API pública. Ojo al ciclo: si aparece, romperlo con forward declarations.
- **(b) Declararlo header de detalle** — moverlo a `include/detail/`, documentar que no se
  incluye directamente, y arreglar los dos tests.

Recomiendo **(a)**. Argumento extra: `fixed_int_traits_specializations.hpp` ya documenta en
sus líneas 24-101 un **conflicto por orden de inclusión** con este header. Un header que
depende de qué se incluyó antes es una fragilidad que hay que cerrar, no documentar.

#### [P13] ¿`data` debería ser público? — comprobado en versiones anteriores

**Comprobado: en `int128-phase175` era `private`.** `int128_param_t` declara
`std::uint64_t data[2]` bajo `private:` (línea 247-248) y expone `high()` / `low()`
(líneas 701-704). Es decir: **`fixed_int_t` con `data` público es una regresión de
encapsulación**, no una convención histórica del proyecto.

Coste medido de revertirlo: **107 accesos `.data[` en `tests/`, 4 en `benchs/`, 0 en
`demos/` y 0 en otros headers**. Además `fixed_width_int_t.hpp` **no tiene ni una
declaración `friend`**: el constructor cross-tipo y los operadores libres funcionan
únicamente porque `data` es público.

**DECIDIDO (23 ago 2026):** se hace `data` privado y **se deja decaer el NTTP**, volviendo
al comportamiento de phase-1.75. No hay ningún uso como parámetro no-tipo de plantilla hoy.
Sigue siendo trivialmente copiable, así que `std::bit_cast` y `memcpy` no se ven afectados.

#### [P17] Sobre los sistemas de construcción

La jerarquía que describes (make.py → CMake/Presets local; WSL para Linux; Docker para otras
arquitecturas) **es correcta y no hay que cambiarla**. Los problemas son de duplicación,
no de arquitectura:

1. **Una sola fuente de verdad para toolchains.** Hoy las rutas de compilador viven en
   `build_generic.py`, `check_generic.py`, `CMakePresets.json`, `ci.yml`, los Dockerfiles y
   `AI_PROMPT/ai-instructions.md`. Por eso pasa lo del clang equivocado ([T1.1]).
   Propuesta: `toolchains.json` en la raíz, leído por make.py y generador de los presets.
2. **`Makefile` (20 KB) vs `make.py` (39 KB):** si make.py es la capa canónica, el Makefile
   debería quedarse en un shim de 5 líneas que delegue, o desaparecer.
3. **45 scripts en `scripts/`** (33 bash + 12 python), la mayoría ancestros de
   `build_generic.py` / `run_generic.py` / `check_generic.py`. Auditar y archivar.
4. **3 Dockerfiles** (`Dockerfile`, `Dockerfile.crosstest`, `Dockerfile.riscv32`) → uno solo
   parametrizado con `ARG COMPILER_VERSION` / `ARG TARGET_ARCH`.

---

### Plan de acción — orden de ejecución

#### 🧹 Fase 0 — Higiene y desbloqueo (rápido, sin riesgo)

| id | Tarea | Detalle | Ref |
|----|-------|---------|-----|
| ~~T0.1~~ ✅ | ~~Destrackear ficheros muertos, **también en remoto**~~ (hecho 23 ago 2026, commit `2573ed6`, pusheado) | `git rm --cached .aider.chat.history.md .aider.input.history .aider.conf.yml .aider.tags.cache.v4/cache.db` + los 5 `.old`/`.bak` (`tests/test_param_{bits,cmath,limits,numeric}.cpp.old`, `include/int128_param_limits.hpp.old`, `benchs/benchmark_vs_builtin.cpp.bak`) → commit → push. **Nota:** esto los quita del árbol, no del historial; purgarlos de verdad exige `git filter-repo` y reescribir historia — no recomendado salvo petición expresa. | #10 |
| ~~T0.2~~ ✅ | ~~Mover `include/test_fixed_string_io.cpp`~~ (hecho: `tests/test_fixed_string_io.cpp`, 74/74, commit `d6575cf`) | Un `.cpp` no vive en `include/`. Va a `tests/`, reescrito con el framework propio del proyecto (hoy usa gtest, que no es dependencia). Su contenido es la base de [T5.1]. | #6, #11 |
| ~~T0.3~~ ✅ | ~~Limpieza de disco~~ (5,5 GB → 0; resultados de benchmark preservados) | `build/` ocupa **5,4 GB**; `vc140.pdb` (2,1 MB) y `CRASH` en la raíz. | #10 |
| ~~T0.4~~ ✅ | ~~Erratas menores~~ (hecho; guardas de inclusión unificadas en el commit de estilo) | `.dockerignore`: `DOCISION_AND_FUTURE.md` → `DECISION_`. README: "42/42" → 52/52; `test_cross_operators 106/106` → 197. Unificar `#pragma once` (3 headers) vs include guards (25). | #18 |

#### 🔧 Fase 1 — Toolchain y licencia

| id | Tarea | Detalle | Ref |
|----|-------|---------|-----|
| ~~T1.1~~ ✅ | ~~**Fijar el compilador correcto**~~ (hecho: `scripts/toolchains.py`, commit `bcdb26e`) | Verificado: en este equipo `clang++` y `g++` **a secas** resuelven a `C:\msys64\usr\bin\` (target `x86_64-pc-windows-cygnus`), **no** a `clang64`/`ucrt64`. No hay clang de Lean 4 en el PATH, pero el riesgo es real igualmente. [build_generic.py:397](scripts/build_generic.py#L397) y [check_generic.py:134](scripts/check_generic.py#L134) usan `os.environ.get("CLANG_CXX", "clang++")`. Poner por defecto en Windows `C:/msys64/clang64/bin/clang++.exe` y `C:/msys64/ucrt64/bin/g++.exe`, y que el script **imprima la ruta y el target** del compilador que va a usar. | #1 |
| T1.2 | **LICENSE + SPDX completo** | Crear `LICENSE.txt` (BSL-1.0) — hoy **no existe**, pese a que `AI-GUIDE.md:674` lo declara obligatorio y las cabeceras lo citan. Cumplimiento medido: **17/28 headers con SPDX** (faltan `fixed_width_int_t.hpp`, los 3 `fixed_int_*` y los 5 de `intrinsics/`), **1/52 tests**, **0/9 benchs**. Aplicar la cabecera que ya manda `AI_PROMPT/ai-instructions.md` §License Header. Añadir `check_license_headers.py` + job de CI que falle si falta alguna. | #2 |
| T1.3 | **Empaquetado CMake** | `add_library(int128 INTERFACE)` + `target_include_directories` + `install()` + `export()` + `int128Config.cmake` → consumible por `find_package` y `FetchContent`. Instalar también `LICENSE.txt`. Rellenar o borrar `conanfile.txt` (hoy 1 byte). | #2 |
| ~~T1.4~~ ✅ | ~~`.clang-format`~~ (hecho: commit aislado `46b9cb8` + `.clangd`; el diff pendiente era formateo salvo el signo '+', rescatado en `06eac8f`) | **Este es el problema del formateo (#7b):** no existe `.clang-format` en el repo (sí `.clang-tidy`), así que el formateador del editor aplica su estilo por defecto. El diff sin commitear de `fixed_width_int_t.hpp` (396+/157-) era reformateo puro **salvo un cambio real de 5 líneas** (aceptar el signo `+` en `from_string` de tipos con signo), que estaba enterrado ahí dentro y se rescató en el commit `06eac8f`. El reformateo introdujo además el artefacto `return R{a} ^ R { b };`. Fijar `.clang-format` con el estilo real del proyecto (Allman, 4 espacios, alineación de columnas), decidir si se revierte el diff o se commitea aislado, y añadir `--dry-run --Werror` en CI. | #7b |

#### 🐛 Fase 2 — Correctitud ✅ COMPLETADA (23 ago 2026)

| id | Tarea | Detalle | Ref |
|----|-------|---------|-----|
| ~~T2.1~~ ✅ | ~~**`from_string` sin detección de overflow**~~ (hecho: `try_from_string` + `parse_result`, commit `a01f47a`) | `u256::from_string("2^256")` devuelve **`0`** en silencio. `parse_error::overflow` y `parse_result<T>` están declarados en [fixed_width_int_t.hpp:61-99](include/fixed_width_int_t.hpp#L61) y **no se usan en ningún sitio**. Cablearlos: `try_from_string` → `parse_result<fixed_int_t>`, y `from_string` lanza a partir de él. | auditoría |
| ~~T2.2~~ ✅ | ~~**Constructor desde float con infinito**~~ (hecho: NaN→0, +inf→max(), −inf→min(), commit `a01f47a`) | [línea 218](include/fixed_width_int_t.hpp#L218): `std::fmod(inf, 2^64)` → NaN → `static_cast<uint64_t>(NaN)` = **UB**. Medido: `u256{inf}` da basura (~2^255). NaN sí está protegido (da 0). Fix: `if (!std::isfinite(v))` → NaN→0, `+inf`→`max()`, `-inf`→`min()` (unsigned: 0). Documentarlo y testearlo. | #5 |
| ~~T2.3~~ ✅ | ~~**Shift con contador `fixed_int_t`**~~ (hecho: `shift_count_of` satura a 64N, commit `a01f47a`) | [líneas 612-640](include/fixed_width_int_t.hpp#L612) truncan el contador a `data[0]`: `x << u256{2^64}` devuelve `x` en vez de `0`, incoherente con la sobrecarga `unsigned` (que sí da 0 para contadores ≥ 64N). Fix: si algún limbo alto ≠ 0, o el contador con signo es negativo, aplicar el mismo camino de saturación que la sobrecarga `unsigned`. | #15 |
| ~~T2.4~~ ✅ | ~~**`data` privado**~~ (hecho: `limb`/`set_limb`/`limbs`/`limbs_ref` + friend, 116 accesos migrados, commit `d163e5a`) | Añadir `limb(i)`, `set_limb(i,v)`, `limbs()` + `template <...> friend class fixed_int_t;` (hoy **no hay ni un `friend`**), y migrar los 111 accesos externos. Ver [P13] para el trade-off del *structural type*. | #13 |
| ~~T2.5~~ ✅ | ~~Header autocontenido~~ (hecho: 28/28 vía `scripts/check_headers_selfcontained.py`, commit `d163e5a`) | `int128_param_traits_specializations.hpp` según [P12b], y añadir a CI un test que compile **cada** header aislado. | #12b |

#### ⚙️ Fase 3 — `constexpr` en división y módulo (objetivo de rama)

| id | Tarea | Detalle | Ref |
|----|-------|---------|-----|
| T3.1 | **`divmod` / `operator/` / `operator%` constexpr** | Es viable y no requiere reescribir el algoritmo: el camino portable de división larga **ya existe** en el código. Lo único no-constexpr son los bloques de intrínsecos: `_udiv128`/`_umul128` (MSVC) y `__asm__("divq")` (ICX Windows). `unsigned __int128` de GCC/Clang **sí** es constexpr. Plan: envolver cada bloque de intrínseco en `if (!std::is_constant_evaluated()) { ... } else { portable }` (patrón que ya usa `intrinsics::addcarry_u64`), y marcar `constexpr` la cadena completa: `divmod`, `/`, `%`, `/=`, `%=` y sus 20+ sobrecargas libres. | #7 |
| T3.2 | División por cero en contexto constante | El `throw std::domain_error` de [línea 916](include/fixed_width_int_t.hpp#L916) dentro de una función `constexpr` es **exactamente el comportamiento estándar deseado**: en tiempo de compilación hace que la expresión no sea constante → error de compilación, igual que `1/0` con `int`. No hay que quitarlo. | #7 |
| T3.3 | Arrastre | `sqrt`, `lcm` y todo lo que dependa de `/` pasan a `constexpr` en cascada. Tests: `static_assert` de división en las 4 combinaciones N × signo. | #7 |
| T3.4 | Ver también | `docs/PLAN_DIVMOD_CONSTEXPR.md` (24 KB) ya tiene análisis previo — revisar y reconciliar con este plan antes de implementar. | #7 |

#### 📦 Fase 4 — Integración STL de `fixed_int_t` (paridad con los tipos que sustituye)

| id | Tarea | Detalle | Ref |
|----|-------|---------|-----|
| T4.1 | `operator<<` / `operator>>` de iostreams | Con soporte de `std::hex`/`oct`/`dec`, `showbase`, `width`, `fill`. Espejo de `int128_param_iostreams.hpp`. | #6 |
| T4.2 | `std::formatter<fixed_int_t<...>>` | Spec completa de `std::format` (relleno, signo, `#`, `0`, ancho, tipos `b/B/d/o/x/X`). Espejo de `int128_param_format.hpp`. | #6 |
| T4.3 | `std::hash<fixed_int_t<...>>` | Para `unordered_map`/`unordered_set`. | #6 |
| T4.4 | `to_string(base)` / `from_string(base)` | Bases 2/8/10/16 como mínimo; hoy solo base 10. Reutilizar `parse_result` de [T2.1]. | #6, #11 |

#### 🧪 Fase 5 — Cobertura de IO

| id | Tarea | Detalle | Ref |
|----|-------|---------|-----|
| T5.1 | **`test_fixed_string_io.cpp`** | Hoy la cobertura es ~13 llamadas a `to_string`/`from_string` en toda la suite y **cero** tests de rutas de error. Cubrir: round-trip N=1..8 × signed/unsigned × {0, 1, max, min, min+1, max-1, potencias de 2, 10^k}; rutas de error (cadena vacía, `nullptr`, carácter inválido, solo signo, **overflow**, separadores); signo `+`/`-` (hoy `from_string` de unsigned rechaza ambos, asimétrico con el de signed); espacios en blanco. | #11 |
| T5.2 | **Fuzz diferencial permanente en la suite** | Portar el harness de esta auditoría a `tests/`: semilla fija, N=2/4/8, signed+unsigned, `+ - * / % & \| ^ << >>` + round-trip de string, contra oráculo de referencia. Ya demostró 17.600 ops sin discrepancias; convertirlo en red de regresión fija. | #11 |
| T5.3 | Tests de iostreams/format/hash | Acompañan a [T4.1-T4.3]. | #6, #11 |

#### 📚 Fase 6 — Documentación: comandos y armonización

| id | Tarea | Detalle | Ref |
|----|-------|---------|-----|
| T6.1 | **Sección "Comandos Interactivos para la IA" en `AI-GUIDE.md`** | El `AI-GUIDE.md` de este proyecto (1704 líneas, 24 secciones) es ya una buena adaptación C++ del de Lean 4, pero **le falta justo la sección de comandos**. Traducir del `lean4-project-template/AI-GUIDE.md` (§"Comandos Interactivos", líneas 401-556) al dominio C++. | #2, #12 |
| T6.2 | **`PROYECTA`** | Equivalente C++ de `proyecta`: para los headers tocados en la sesión, extraer la API pública (Doxygen `@brief`, firmas, plantillas) y proyectarla al `docs/API_*.md` correspondiente; verificar que no se filtra nada `private`/`detail` y que nada público queda sin proyectar. | #12 |
| T6.3 | **`DOCUMENTA`** | Generación: ejecutar Doxygen, exigir **0 warnings**, regenerar los `docs/API_*.md` faltantes y medir cobertura de comentarios. **Dato de partida:** `fixed_width_int_t.hpp` tiene **12 comentarios Doxygen en 3259 líneas**, frente a 251 en `int128_parameterized.hpp` — la doc generada del tipo estrella está prácticamente vacía. | #12 |
| T6.4 | **`ACTUALIZA_DOC`** | Pasada completa post-sesión: `make.py test` → CHANGELOG (entrada nueva con fecha) → NEXT_STEPS (mover lo completado) → PROJECT_STATUS (snapshot) → README (métricas) → verificar consistencia de recuentos entre ficheros. | #12 |
| T6.5 | **Armonizador de documentación** | El "sistema que revise y armonice" (#16): script `check_docs_consistency.py` + job de CI que verifique: (1) los recuentos de tests citados en README/PROJECT_STATUS/CHANGELOG/NEXT_STEPS coinciden con la suite real — hoy conviven "42/42", "52/52", "106/106" y "197/197"; (2) todo `API_*.md` corresponde a un header existente y viceversa; (3) todo símbolo público exportado aparece documentado; (4) 0 warnings de Doxygen; (5) cabeceras SPDX presentes ([T1.2]); (6) fechas `Last Updated` coherentes. Equivalente al `repasa_y_proyecta` de Lean 4. | #16 |
| T6.6 | Implementar los comandos como slash commands | `.claude/commands/proyecta.md`, `documenta.md`, `actualiza_doc.md` (hoy `.claude/` solo tiene `settings.json`), para que sean invocables de verdad y no solo prosa en la guía. | #12 |
| T6.7 | Consolidar la doc de raíz | 9.300 líneas en 10 ficheros con solapamiento fuerte (CHANGELOG 4.573, AI-GUIDE 1.704, NEXT_STEPS 1.003). Definir qué fichero es fuente de verdad de qué. | #18 |

#### 🏗️ Fase 7 — CI, Docker y sistemas de build

| id | Tarea | Detalle | Ref |
|----|-------|---------|-----|
| T7.1 | **Cerrar las puertas del CI** | `exit 1` en `cross-x86-32` e `intel-icx`; quitar `continue-on-error` de `cross-arm32`; tolerancia 10% → 0 en aarch64/riscv64; dejar de mandar errores de compilación a `/dev/null`. Ver [P4]. | #4 |
| T7.2 | **Docker al día** | `docker/Dockerfile` instala **GCC 12 / Clang 14** mientras el CI valida GCC 13-16 / Clang 18-22. Verificado en contenedor real: GCC 12 compila y pasa; **Clang 14 falla `test_param_format`** (el asunto de `GM_TABLE`, [P3]). Subir a Ubuntu 24.04 + GCC 13/14 + Clang 18/19. Además [Dockerfile:121](docker/Dockerfile#L121): `ENV MAKEFLAGS="-j$(nproc)"` **no expande el shell** — queda como literal. Unificar los 3 Dockerfiles en uno parametrizado por `ARG`. | #8 |
| T7.3 | **`GM_TABLE` sin flag** | Implementar [P3] + job de CI que compile con Clang **sin** `-fconstexpr-steps`. | #3 |
| T7.4 | **`CONFIGURE_DEPENDS`** | [tests/CMakeLists.txt:5](tests/CMakeLists.txt#L5): `file(GLOB TEST_SOURCES "*.cpp")` sin `CONFIGURE_DEPENDS` → añadir un test no regenera el build. Añadirlo (y lo mismo en `benchs/` y `demos/`). | #14 |
| T7.5 | **`toolchains.json`** | Fuente única de rutas/versiones de compilador, consumida por make.py, generación de presets, Docker y CI. Es la causa raíz de [T1.1]. | #17 |
| T7.6 | Adelgazar la capa de scripts | `Makefile` → shim sobre make.py o eliminarlo; auditar los 45 scripts de `scripts/` y archivar los superados por `build_generic.py`/`run_generic.py`/`check_generic.py`. | #17 |

---

### Orden recomendado

1. **Fase 0** entera (una tarde, cero riesgo, deja el árbol limpio para todo lo demás).
2. **T1.4** (`.clang-format`) **antes de tocar código**, o el próximo guardado vuelve a generar ruido.
3. **T1.1** (compilador correcto) antes de medir o comparar nada.
4. ~~**Fase 2** (correctitud)~~ ✅ completada 23 ago 2026. Los tres fallos corrompían valores en silencio; el `+` de `from_string` y el `data` privado entraron en el mismo bloque.
5. **Fase 3** (constexpr div/mod) — es el objetivo declarado de la rama. **← SIGUIENTE**
6. **Fase 4 + Fase 5** juntas: cada pieza STL entra con sus tests.
7. **Fase 6** al final de cada bloque, invocando `ACTUALIZA_DOC`.
8. **Fase 7** en paralelo, no bloquea a nadie.

---


## 🚧 v1.90 — fixed_int_t\<N\>: estado actual (22 May 2026)

### Completado ✅

| Item | Commits | Tests |
|------|---------|-------|
| `fixed_int_t<N,Sign,Form>` unificado | 6c53a8e | test_cross_operators 106/106 |
| Fast paths `operator*`/`*=` N=2 (schoolbook) | 97e27ab | test_fixed_vs_param 804/804 |
| Fast paths `operator+=`/`-=` in-place | 97e27ab | test_fixed_vs_param 804/804 |
| `divmod` fast paths N=2 (`__uint128_t`/`divq`/`_udiv128`) | 97e27ab | test_fixed_divmod 218/218 |
| Single-limb divisor fast path O(N) | 72497d5 | test_fixed_divmod Section 7 |
| **Knuth Algorithm D** N-limb ÷ M-limb (M ≥ 2) | 6fba207 | test_fixed_divmod Section 8 |
| **Karatsuba `operator*`/`*=` N=4/8** | 89aa9b7 | test_fixed_karatsuba 49/49 |

### Pendiente (prioridad) — ⚠️ SUPERADA por el plan de la auditoría 23 ago 2026

> Los tres items siguientes están **completados y verificados** en la auditoría del
> 23 ago 2026. El plan vigente es la sección **🔍 AUDITORÍA 23 ago 2026** al principio
> de este fichero.

| # | Item | Estado |
|---|------|--------|
| ~~1~~ | ~~**Fase MS-INTEROP** — cross-sign interop completa~~ | ✅ **COMPLETADA v1.81 (22 May 2026)** |
| ~~1~~ | ~~**`to_string` para N>2**~~ | ✅ Verificado hasta N=8 (fuzz diferencial, 0 discrepancias) |
| ~~2~~ | ~~**`from_string` para N>2**~~ | ✅ Funciona — pero **sin detección de overflow**, ver [T2.1] |
| ~~3~~ | ~~**Aritmética segura** (`checked_add`/`checked_sub` → `optional`)~~ | ✅ `checked_add`/`sub`/`mul` para signed y unsigned. Pendiente aparte: aritmética **no modular** completa |

---

## 🎯 Fase MS-INTEROP — Cross-sign interop al estilo built-in (planificada 22 May 2026)

### Contexto y alcance

**Decisión:** `int128_param_t` se deja tal cual (no se añadirán operadores cross-sign entre `uint128_t` e `int128_t`). El esfuerzo de paridad con built-ins se concentra **únicamente en `fixed_int_t<N, Sign, Form>`**.

**Motivación:** la auditoría 22 May 2026 confirmó dos asimetrías concretas:

| Caso | int128_param_t | fixed_int_t<N> | Built-in equivalente |
|------|----------------|----------------|----------------------|
| `unsigned op signed` (mismo ancho) | ❌ no compila / ambiguous | ✅ funciona (mixed_iu_t) | ✅ unsigned wins |
| `op<<` con count cross-sign | n/a (count siempre unsigned built-in) | ❌ no compila | ✅ cualquier integral |
| `std::common_type<u,s>` | n/a | ❌ ausente | ✅ → unsigned |
| `std::is_signed<T>` | ❌ ausente (probable) | ❌ ausente | ✅ built-in |
| `std::numeric_limits<T>::is_signed` | n/a (instancia única) | ❌ ausente | ✅ |
| `operator<=>` (C++20) | ❌ ausente | ❌ ausente | ✅ |

### Estado actual de cross-sign en fixed_int_t (lo que YA funciona)

| Categoría | Operadores | Líneas | Tests |
|-----------|-----------|--------|-------|
| Binarios free | `+ - * / % & \| ^` (28 overloads, ambas orientaciones) | 2588-2648 | test_cross_operators §3-5 |
| Comparaciones free | `== != < <= > >=` (12 overloads) | 2650-2690 | test_cross_operators §3-5 |
| Compound aritmético | `+= -= *= /= %=` (`enable_if<S2 != Sign>`) | 1397-1487 | test_cross_operators §3-4 |
| Compound bitwise | `&= \|= ^=` (`enable_if<S2 != Sign>`) | 1490-1546 | test_cross_operators §3 |
| Trait UAC | `detail::mixed_iu_t<N,M>` | 1973-1977 | implícito |

**Regla UAC implementada:** `mixed_iu_t<N,M> = (N > M) ? int_fixed_t<N> : uint_fixed_t<M>`
(signed gana solo si es estrictamente más ancho; en empate gana unsigned — idéntico a C++ built-in).

### Gaps identificados (a cerrar en esta fase)

| # | Gap | Severidad | Detalle |
|---|-----|-----------|---------|
| **G1** | Shifts con RHS `fixed_int_t<M>` (cualquier Sign) | 🔴 crítico | `operator<<`/`>>` solo aceptan `unsigned` (líneas 520, 545). `x << uint_fixed_t<2>{3}` no compila. Compound `<<= >>=` mismo problema. |
| **G2** | `operator<=>` C++20 (same-sign y cross-sign) | 🟡 importante | 12 comparadores a mano. `<=>` los reemplaza con 2 (same-sign + cross-sign). Habilita interop con `std::sort`, `std::set` y código que usa spaceship. |
| **G3** | `std::common_type<int_fixed_t<N>, uint_fixed_t<M>>` | 🔴 crítico | Sin esto, código genérico tipo `template<class A, class B> common_type_t<A,B> f(A,B)` no funciona con mezcla signed/unsigned. |
| **G4** | `std::is_signed` / `is_unsigned` / `make_signed` / `make_unsigned` | 🔴 crítico | Conceptos C++20 (`std::signed_integral`) y SFINAE clásica rompen. |
| **G5** | `std::numeric_limits<fixed_int_t<N,Sign,Form>>` | 🟡 importante | `min()`, `max()`, `is_signed`, `digits`, `radix`, etc. Actualmente solo hay métodos estáticos en la clase. |
| **G6** | Unary `operator+()` | 🟢 trivial | Solo existe unary `-`. Built-in tienen ambos. Línea ~636. |
| **G7** | Cobertura de tests: edge cases cross-sign | 🟡 importante | INT_MIN ± UINT_MAX, overflow boundary, sign-extension al promover, `int_fixed_t<1>{-1} == uint_fixed_t<2>{2^128-1}` (extensión de signo a unsigned wider), etc. |
| **G8** | Documentación API_*.md y README para `fixed_int_t` | 🟢 estética | El header carece de la familia `API_*.md` que sí tiene `int128_param_t`. |

### Tareas — Fase MS-INTEROP

#### T1 — Shift operators cross-sign (G1) 🔴

**Objetivo:** que `x << y` y `x >> y` acepten cualquier `fixed_int_t<M, S2, F2>` como count, además de `unsigned`.

**Diseño propuesto:**
- LHS conserva su tipo: el resultado de `int_fixed_t<N> << anything` es `int_fixed_t<N>` (igual que built-in `int << unsigned == int`).
- Count se convierte a `unsigned` (o `std::size_t`) tras chequear que cabe; si no cabe → UB equivalente a built-in (sin chequeo runtime, igual que `int << 64u`).
- Free-function overload: `operator<<(const fixed_int_t<N,Sign,Form>&, const fixed_int_t<M,S2,F2>&)`.
- Compound: `operator<<=(const fixed_int_t<M,S2,F2>&)`.
- Mantener el overload `unsigned` existente como fast path.

**Archivos:** `include/fixed_width_int_t.hpp` (insertar tras línea 545).

**Tests:** ampliar `test_cross_operators.cpp` con sección 8 (shifts cross-sign) — ~16 casos: 4 combos sign × {<<,>>,<<=,>>=}.

**Riesgo:** bajo. Trivial wrapper que llama al overload `unsigned` existente.

---

#### T2 — `operator<=>` three-way cross-sign (G2) 🟡 — COEXISTE con comparadores manuales

**Objetivo:** añadir `operator<=>` (same-sign y cross-sign) **sin eliminar** los 12 comparadores manuales existentes.

**Diseño propuesto:**
- Dentro de la clase: `constexpr std::strong_ordering operator<=>(const fixed_int_t&) const noexcept` para same-type.
- Free function: `operator<=>(const fixed_int_t<N,Sign>&, const fixed_int_t<M,S2>&)` para cross-N y/o cross-sign — usa `mixed_iu_t` para promocionar y compara.
- Los `operator==`, `!=`, `<`, `<=`, `>`, `>=` actuales **se mantienen intactos**. El compilador prefiere overloads explícitos sobre síntesis desde `<=>`, así que no hay conflicto.

**Archivos:** `include/fixed_width_int_t.hpp` (insertar `<=>` cerca de la zona de comparadores existentes, ~línea 2650).

**Tests:** `test_cross_operators.cpp` debe seguir pasando 106/106 sin cambios. Añadir 4-6 casos con `<=>` explícito (verificar `strong_ordering::less`, etc.).

**Riesgo:** bajo. Sin eliminación, no hay regresión posible — solo adición de capacidad.

**Tipo de orden:** `strong_ordering` (enteros nuestros, todo total y exacto).

---

#### T3 — `std::common_type<int_fixed_t<N>, uint_fixed_t<M>>` (G3) 🔴

**Objetivo:** que `std::common_type_t<int_fixed_t<2>, uint_fixed_t<4>>` resuelva a `uint_fixed_t<4>` (= `mixed_iu_t<2,4>`).

**Diseño propuesto:**

```cpp
namespace std {
    template <size_t N, size_t M>
    struct common_type<nstd::int_fixed_t<N>, nstd::uint_fixed_t<M>> {
        using type = nstd::detail::mixed_iu_t<N, M>;
    };
    // simétrico para uint_fixed_t<M>, int_fixed_t<N>
    // Y: common_type<int_fixed_t<N>, int_fixed_t<M>> = int_fixed_t<max(N,M)>
    // Y: common_type<uint_fixed_t<N>, uint_fixed_t<M>> = uint_fixed_t<max(N,M)>
    // Y: common_type con built-in integral T (8 casos: int_fixed_t × {char,short,int,long,…})
}
```

**Archivos:** nuevo header `include/fixed_int_traits_specializations.hpp` (paralelo a `int128_param_traits_specializations.hpp`).

**Tests:** sección nueva en `test_cross_operators.cpp` o archivo nuevo `test_fixed_traits.cpp`, ~12-16 `static_assert`.

**Riesgo:** bajo. Es declarativo.

---

#### T4 — `std::is_signed` / `is_unsigned` / `make_signed` / `make_unsigned` (G4) 🔴

**Objetivo:** que `std::is_signed_v<int_fixed_t<4>> == true`, `std::is_unsigned_v<uint_fixed_t<4>> == true`, `std::make_signed_t<uint_fixed_t<4>> == int_fixed_t<4>`, etc.

**Diseño:** specializations en `namespace std` dentro del mismo `fixed_int_traits_specializations.hpp` de T3.

**Cuidado:** `std::is_integral` y `std::is_arithmetic` son **non-specializable** en el estándar (UB modificarlas). Para conceptos C++20 (`std::integral`, `std::signed_integral`, `std::unsigned_integral`) que dependen de `is_integral`, **no podemos hacer que `fixed_int_t` los satisfaga**.

**Solución elegida:** definir nuestros conceptos en `include/fixed_int_concepts.hpp`:
- `nstd::integral<T>` = `std::integral<T>` ‖ `is_fixed_int_v<T>`
- `nstd::signed_integral<T>` = `std::signed_integral<T>` ‖ (`is_fixed_int_v<T>` ∧ `T::is_signed`)
- `nstd::unsigned_integral<T>` = `std::unsigned_integral<T>` ‖ (`is_fixed_int_v<T>` ∧ `!T::is_signed`)

Código del usuario que quiera aceptar built-ins y `fixed_int_t` por igual usará `nstd::integral` en lugar de `std::integral`.

**Archivos:** `fixed_int_traits_specializations.hpp` (T3) + `fixed_int_concepts.hpp` (nuevo).

**Tests:** ~10 `static_assert` por trait + ~6 por concepto nuestro.

**Riesgo:** bajo. UB-free porque `is_signed`, `make_signed`, etc. SÍ son specializables (lo dice el estándar: "If T is a (possibly cv-qualified) user-defined type..."). Confirmar antes con la regla 17.6.4.2.1.

---

#### T5 — `std::numeric_limits<fixed_int_t<N,Sign,Form>>` (G5) 🟡

**Objetivo:** specialization completa con todos los miembros relevantes (`min`, `max`, `lowest`, `digits`, `digits10`, `is_signed`, `is_integer`, `is_exact`, `radix`, `is_modulo`, `is_specialized`, etc.).

**Diseño:** copiar/adaptar `int128_param_limits.hpp` (que ya hace esto para `int128_param_t`). Probablemente se puede generalizar a N limbs.

**Archivos:** nuevo `include/fixed_int_limits.hpp`.

**Tests:** archivo nuevo `test_fixed_limits.cpp` (~20 asserts) o sección de `test_cross_operators.cpp`.

**Riesgo:** bajo. Patrón ya establecido.

---

#### T6 — Unary `operator+()` (G6) 🟢

**Objetivo:** `+x` devuelve copia (igual que built-in).

**Diseño:**

```cpp
constexpr fixed_int_t operator+() const noexcept { return *this; }
```

**Archivos:** `include/fixed_width_int_t.hpp` (junto a operator- línea 636).

**Tests:** 2-3 asserts.

**Riesgo:** ninguno.

---

#### T7 — Cobertura tests cross-sign edge cases (G7) 🟡

**Objetivo:** ampliar `test_cross_operators.cpp` con casos de frontera que el suite actual no cubre:

- `int_fixed_t<1>::min() + uint_fixed_t<2>::max()` — overflow al promocionar
- `int_fixed_t<1>{-1} == uint_fixed_t<2>{2^128 - 1}` — extensión de signo
- `uint_fixed_t<1>{0} - int_fixed_t<1>{1}` — wraparound unsigned
- `(int_fixed_t<2>::min() / uint_fixed_t<2>::max()) == 0` — división cross-sign con extremos
- Mismas comprobaciones con shifts cross-sign (T1)
- `<=>` returns correctos en frontera (T2)

**Archivos:** `tests/test_cross_operators.cpp` — sección nueva 8 y 9.

**Tests añadidos:** ~30-40 casos. Pasar de 106 a ~140 tests.

**Riesgo:** medio. Estos edge cases pueden revelar bugs en la implementación actual de `mixed_iu_t` que el suite "happy path" no destapa.

---

#### T8 — Documentación (G8) 🟢

**Objetivo:** documentar `fixed_int_t<N>` al nivel de detalle que tiene `int128_param_t`.

**Entregables:**
- `docs/API_fixed_int.md` — clase + operadores + cross-sign semantics
- `docs/API_fixed_int_traits.md` — common_type, is_signed, numeric_limits
- Sección nueva en README con tabla "interop signed/unsigned"
- CHANGELOG entry "v1.90 — Fase MS-INTEROP completa"

**Archivos:** `docs/API_fixed_int*.md`, `README.md`, `CHANGELOG.md`.

**Riesgo:** ninguno.

---

### Resumen de archivos tocados

| Archivo | T1 | T2 | T3 | T4 | T5 | T6 | T7 | T8 |
|---------|----|----|----|----|----|----|----|----|
| `include/fixed_width_int_t.hpp` | ✏️ | ✏️ |  |  |  | ✏️ |  |  |
| `include/fixed_int_traits_specializations.hpp` (nuevo) |  |  | 🆕 | 🆕 |  |  |  |  |
| `include/fixed_int_concepts.hpp` (nuevo, opcional) |  |  |  | 🆕 |  |  |  |  |
| `include/fixed_int_limits.hpp` (nuevo) |  |  |  |  | 🆕 |  |  |  |
| `tests/test_cross_operators.cpp` | ✏️ | ✏️ |  |  |  | ✏️ | ✏️ |  |
| `tests/test_fixed_traits.cpp` (nuevo) |  |  | 🆕 | 🆕 |  |  |  |  |
| `tests/test_fixed_limits.cpp` (nuevo) |  |  |  |  | 🆕 |  |  |  |
| `docs/API_fixed_int*.md` (nuevo) |  |  |  |  |  |  |  | 🆕 |
| `README.md`, `CHANGELOG.md` |  |  |  |  |  |  |  | ✏️ |

### Orden de ejecución sugerido

1. **T6** (5 minutos, trivial, calienta motores) — unary `+`
2. **T3 + T4** (juntos: traits + common_type, son co-dependientes) — desbloquea código genérico
3. **T1** (shifts cross-sign) — el gap "duro" más visible para usuario
4. **T5** (numeric_limits) — necesita T4 para coherencia de `is_signed`
5. **T2** (`<=>`) — refactor delicado, mejor con todo lo demás verde
6. **T7** (edge cases tests) — al final, una vez todo lo nuevo existe
7. **T8** (docs) — cierre de fase

### Criterios de aceptación (Fase MS-INTEROP completa)

- ✅ `test_cross_operators.cpp` pasa en los 4 compiladores Windows + WSL (140+ tests)
- ✅ `test_fixed_traits.cpp` y `test_fixed_limits.cpp` pasan en los 4 compiladores
- ✅ `static_assert(std::is_same_v<std::common_type_t<int_fixed_t<2>, uint_fixed_t<4>>, uint_fixed_t<4>>)` compila
- ✅ `static_assert(std::is_signed_v<int_fixed_t<4>>)` compila y es `true`
- ✅ `x << uint_fixed_t<2>{3}` compila y produce el resultado correcto para todo `x` de cualquier `fixed_int_t<N,Sign>`
- ✅ `(int_fixed_t<4>{1} <=> int_fixed_t<4>{2}) == std::strong_ordering::less` compila
- ✅ La interop con built-ins (`fixed_int_t<N> + int`) sigue funcionando como antes (sin regresión)
- ✅ `int128_param_t` permanece intacto

### Decisiones resueltas (22 May 2026)

1. **T2 (`<=>`): COEXISTENCIA.** Se añade `operator<=>` sin eliminar los 12 comparadores manuales existentes. Más seguro frente a regresiones; el compilador usará `<=>` cuando no haya match directo más específico.

2. **T4: definimos `nstd::integral` / `nstd::signed_integral` / `nstd::unsigned_integral`** que aglutinen built-ins y `fixed_int_t`. Vive en `include/fixed_int_concepts.hpp`. El usuario puede usar nuestros conceptos en código genérico que mezcle ambos.

3. **T5 (`numeric_limits`): cubre TODAS las Forms** (`binnat`, `twos_complement`, y por extensión MS/EK cuando lleguen). La specialization se escribe genérica sobre `<N, Sign, Form>`, no por instancia.

4. **`mixed_iu_t` se promueve de `detail::` a `nstd::`.** Pasa a formar parte de la API pública para que código del usuario pueda usarlo. Se mantiene un alias en `detail::` para compatibilidad interna.

5. **Cross-sign con built-ins:** sin cambios. `u2{5} + (-3)` ya funciona y wrappea como built-in — no se toca. Solo se verifica en T7.

6. **Versión que cierra esta fase: v1.81.** (Decisión del usuario, no v1.90.) La rama sigue siendo `phase-1.80`.

---

## ✅ Fase A — Deuda Técnica Representaciones (16 May 2026, branch fixint/core)

| Item | Estado | Detalle |
|------|--------|---------|
| Pragma GCC scope | ✅ HECHO | `#pragma GCC optimize("O0")` reemplazado por helper `ek_store_bias()` con `[[gnu::optimize("O0"), gnu::noinline]]`. Usa `std::is_constant_evaluated()` para dispatch constexpr/runtime. |
| test_sweep_ms.cpp | ✅ HECHO | 15 tests, ~133M verificaciones. 3 regiones × 6 combos binarias. Oráculos adaptados a semántica MS (no roundtrip, sino antisimetría y sub==add(-b)). |
| Nota stale MENSAJES_IA_TEMPORALES | ✅ HECHO | representation.hpp confirmado completo — nota corregida. |

**Resultado:** 41/41 tests GCC release. Commit: `1e17464`.

---

## ✅ Fase B — ARM64 portabilidad: YA COMPLETA (verificado 16 May 2026)

La capa de intrinsics ya cubre ARM64 correctamente. No hace falta NEON manual:

| Path | Mecanismo | Instrucciones ARM64 generadas |
|------|-----------|-------------------------------|
| `add128`/`sub128` | `__SIZEOF_INT128__` → `__uint128_t` | `adds`/`adcs` (nativo) |
| `addcarry_u64`/`subborrow_u64` | `INTRINSICS_USES_GNU_ABI` → `__builtin_uaddll_overflow` | `adds`+`cset` (correcto) |
| `popcount64` | `__builtin_popcountll` | `cnt` (NEON, generado por GCC/Clang) |
| `clz64`/`ctz64` | `__builtin_clzll`/`__builtin_ctzll` | `clz`/`rbit+clz` |
| `umulh`/`mulx_u64` | `__SIZEOF_INT128__` | `mul`+`umulh` |

`<arm_neon.h>` se incluye en `compiler_detection.hpp` (linea 289) pero no es necesario
porque `__uint128_t` + `__builtin_*` ya produce código ARM64 optimal en GCC/Clang.

---

## 🔲 Fase C — Fortalecimiento de Tests (M3, después de Fase B)

| Subtarea | Impacto |
|----------|---------|
| C1: test_sweep_binnat.cpp | Cobertura sistemática para representación unsigned (actualmente sin sweep propio) |
| C2: Sweep de conversiones cruzadas | TC↔MS↔EK round-trip con los 6 combos de regiones |
| C3: Edge cases MS -0 y EK bias | Casos frontera en test_param_ms.cpp / test_param_ek.cpp |

---

## 🗺️ Hoja de Ruta branch → milestone

```text
fixint/core
  Fase A: pragma fix + test_sweep_ms      COMPLETO
  Fase B: ARM64 portabilidad              COMPLETO (ya estaba: __uint128_t + __builtin_*)
  Fase C: test strengthening (M3)         PENDIENTE
     -> merge -> phase-1.80  (v1.80)
phase-1.80 -> v1.90: int_fixed_t<N>
```

---

## 🎯 COMPLETED PRIORITIES (session 7 — 22 March 2026)

### ✅ A1: Optimize Subtraction/Addition (GCC) — COMPLETE

**Root cause:** `subborrow_u64` used `__builtin_usubll_overflow` which generated two separate overflow-checked ops instead of native `sub+sbb` chain.

**Solution:** New `sub128()`/`add128()` intrinsics in `arithmetic_operations.hpp` using `__uint128_t` on GCC/Clang (single native op), `_subborrow_u64`/`_addcarry_u64` chain on MSVC.

**Codegen verified:** nstd generates identical `subq+sbbq` / `addq+adcq` as `__int128`.

**Benchmark fix:** Removed `"memory"` clobber from `doNotOptimize` for 16-byte GCC types (was causing stack spills for nstd structs but not `__int128`).

### ✅ A2: Benchmarks on All 4 Compilers — COMPLETE

| Compiler | SUB ratio | ADD ratio | Verdict |
|----------|-----------|-----------|---------|
| GCC 15.2.0 | **0.962x** | **0.959x** | Both FASTER than __int128 |
| Clang 21.1.8 | 1.058x | 1.002x | Within 1.10x target |
| Intel ICX 2025.3.0 | 0.987x | 1.048x | Within 1.10x target |
| MSVC 19.50.35726 | 0.975x vs u64 | 0.977x vs u64 | No __int128 baseline |

### ✅ A4: Migrate Tests to Sweep Framework — COMPLETE (5 new files, 60/60 PASS)

| File | Sweep Tests | Values Checked | Properties |
|------|-------------|----------------|------------|
| `test_sweep_shift.cpp` | 16 | ~100M+ | Identity, arithmetic equiv, roundtrip, composition, distributivity |
| `test_sweep_comparison.cpp` | 11 | ~130M+ | Reflexivity, complements, trichotomy, arithmetic consistency, antisymmetry |
| `test_sweep_division.cpp` | 13 | ~100M+ | q*d+r=n, r<d, div-by-1, self-div, zero-dividend, pow2 equiv, quotient bound |
| `test_sweep_unary_ops.cpp` | 12 | ~75M+ | Inc/dec roundtrip, ++ vs +1, post-inc semantics, negation, bool conversion |
| `test_sweep_string.cpp` | 8 | ~50M+ | Decimal/hex/octal/binary roundtrip, no leading zeros, length bound |

Total sweep test suite: 8 existing + 5 new = **13 sweep files**, 60 new sweep tests, ~455M+ value checks.

---

## 🎯 COMPLETED PRIORITIES (session 6 — 22 March 2026)

### ✅ Granlund-Montgomery Constexpr Division — COMPLETE (Phases A-F)

Full implementation of Hacker's Delight §10-9 algorithm for division by compile-time constants.

**Created: `include/int128_param_divmod.hpp` (~500 lines)**

- `ce_uint128` — lightweight constexpr 128-bit type (no circular deps)
- `compute_magic_128(d)` — optimal (minimal-shift) magic constant finder
- `GM_TABLE[3..1023]` — constexpr lambda-initialized precomputed table
- `ce_mulhi_128()` — schoolbook 128×128→upper128 (pure C++, constexpr)
- `gm_div_limbs()` — simple path + overflow correction dispatch

**Member templates in `int128_parameterized.hpp`:**

- `div<D>()` — power-of-2→shift, D∈[3,1023]→table, D>1023→runtime compute
- `mod<D>()` — `*this - div<D>() * D`
- `divmod_const<D>()` — returns `{quotient, remainder}`
- `mul<K>()` — binary shift-add decomposition

**Performance (GCC -O2, RDTSC cycles/op):**

| Method | div by 3 | div by 10 | div by 10^19 |
|--------|----------|-----------|-------------|
| `n.div<D>()` (GM generic) | 21 | 22 | 25 |
| `operator/` (Knuth D) | 141 | 134 | 136 |
| Handcoded `fast_divN()` | 15 | 20 | 17 |
| **GM speedup vs Knuth D** | **6.7x** | **6.2x** | **5.5x** |

**Validation:**

- `tests/test_divmod_const.cpp` — 71/71 PASS on GCC, Clang, MSVC, Intel
- Each sweep test: ~6.29M values (3 regions × 2^21 + 20 edge cases)
- Total: ~400M+ individual value checks
- Constexpr step limits: Clang/ICX `-fconstexpr-steps=100000000`, MSVC `/constexpr:steps100000000`
- Regression: `python make.py test gcc release` → 60/60 PASS

**Architectural observations:**

- `compute_magic_128` finds solutions BETTER than old hardcoded constants (d=5: shift=2 vs old shift=3)
- For some divisors (d=7) overflow correction IS required (no non-overflow solution exists)
- `mul<K>()` ~30-70% slower than `operator*` (hardware mul inherently faster than shift-add)
- GM generic (~21-28 cyc/op) ~30-50% slower than handcoded intrinsic versions (~15-20 cyc/op) due to pure C++ `ce_mulhi_128` vs hardware `mulhi128`

---

## 🎯 COMPLETED PRIORITIES (session 5)

### ✅ Karatsuba API — COMPLETE (12/12 tests, ~57M verifications)

- `nstd::widening_mul(a,b)` → Full 128×128→256 Karatsuba (3 multiplications)
- `nstd::mulhi(a,b)` → Upper 128 bits of 256-bit product
- `nstd::mullo(a,b)` → Lower 128 bits (operator* alias)
- `nstd::uint256_t` → 256-bit result type (4 LE limbs)
- div_by_const.hpp: mulhi_128 migrated from schoolbook (4 muls) to Karatsuba (3 muls)

### ✅ std::format Full Standard Spec — COMPLETE (24/24 tests)

- Complete `[[fill]align][sign][#][0][width][type]` per C++20 standard
- Types: d, x/X, b/B, o | Fill/align/sign/alt (#)/zero-pad (0)/width

### ✅ std::hash STL Integration — COMPLETE (14 new assertions)

- All 4 types hashable in `std::unordered_map`, `std::unordered_set`
- Both `nstd::hash<T>` and `std::hash<T>` specialized
- Fixed: nstd::hash was invisible on Clang/libc++ (inside wrong preprocessor guard)

### ✅ Multicompiler Benchmarks — COMPLETE (5 benchmarks × 2 compilers)

**Headline:** nstd::uint128_t 19.8x faster than __int128 for division (GCC -O2)

- Full RDTSC cycle measurements across GCC 15 and Clang 21
- Results: `build/benchmark_results_multicompiler.md`

---

## 🎯 COMPLETED PRIORITIES

### ✅ Float/Double Assignment Operators - COMPLETE (25/25 tests)

### ✅ Type Traits Specializations - COMPLETE (35/35 tests)

### ✅ Phase 3: Knuth Algorithm D - COMPLETE (17 March 2026)

**Achievement:** D_knuth_divrem() with `__uint128_t` native division.

- **6.24x faster** than binary long division (7.17 → 1.15 ns/op)
- GCC-O2: **0.47x vs uint64_t** (faster than native 64-bit!)
- nstd **20x faster** than `__int128` for division
- 55/55 division tests passing (GCC + Clang)

---

## ✅ COMPLETED: Intrinsics Audit (18 March 2026)

**7 critical issues fixed:**

- `int128_param_bits.hpp`: 6 `__builtin_*` calls → `intrinsics::popcount64/clz64/ctz64`
- `int128_param_numeric.hpp`: Removed `detail::portable_clzll()`, replaced with `intrinsics::clz64()`
- All 12 feature headers validated on 11 compilers post-fix

---

## ✅ COMPLETED: Cross-Representation Operators (June 2026)

- Cross-repr copy/move constructors (binnat/TC/MS/EK ↔ binnat/TC/MS/EK)
- Cross-repr assignment operators
- Explicit conversion methods between all forms
- Built-in integral interop with all representation forms

## ✅ COMPLETED: API Reference Documentation (June 2026)

- 14 cppreference-style API docs covering ~280 public symbols
- Main class doc: `API_parameterized.md`
- 13 feature module docs (concepts, traits, limits, algorithm, bits, cmath, numeric, ranges, safe, thread_safety, iostreams, format, representation)

## ✅ COMPLETED: Granlund-Montgomery Division Plan (June 2026)

- `docs/PLAN_DIVMOD_CONSTEXPR.md` — Comprehensive plan for constexpr division by compile-time constants
- Analysis of 17 legacy headers in `legacy-code/divmod_by_constexpr/`

---

## ⏳ CURRENT PRIORITIES

### Implementation

Added 3 explicit assignment operators that delegate to existing constructors:

```cpp
int128_param_t& operator=(float value) noexcept {
    *this = int128_param_t{value};  // Reuses constructor with EK support
    return *this;
}

int128_param_t& operator=(double value) noexcept {
    *this = int128_param_t{value};
    return *this;
}

int128_param_t& operator=(long double value) noexcept {
    *this = int128_param_t{value};
    return *this;
}
```

### Test Results

**25/25 tests passing (100%)** ✅

- Float assignment: 6/6 ✅
- Double assignment: 6/6 ✅
- Long double assignment: 4/4 ✅
- Special values: 3/3 ✅
- Multiple assignments: 3/3 ✅
- Post-construction: 3/3 ✅

### Files Modified

- `include/int128_parameterized.hpp` (+39 lines)
- `tests/test_float_assignment.cpp` (new, 200 lines)

---

## ✅ PRIORITY 2: Type Traits Specializations - COMPLETE

### Implementation

Created comprehensive STL type traits integration:

**int128_param_traits_specializations.hpp** (~474 lines):

- Variadic macros for template types with commas
- 3 code generation macros: `NSTD_DEFINE_INT128_TRAITS`, `NSTD_DEFINE_INT128_ASSIGNABLE`, `NSTD_DEFINE_INT128_HASH`
- Specializations for **4 valid types**: `binnat` (unsigned), `TC`/`MS`/`EK` (signed)

**Traits implemented:**

- One-parameter: `is_integral`, `is_arithmetic`, `is_signed`, `is_unsigned`, 9 trivial traits
- Two-parameter: `is_trivially_assignable` (16 specializations)
- Conversions: `make_signed` / `make_unsigned` (binnat ↔ TC conversions)
- Hash: `nstd::hash<T>` for `std::unordered_map` support
- Helper variables: `_v` suffixes (C++17)
- Type aliases: `_t` suffixes

### Test Results

**35/35 tests passing (100%)** ✅

- Group 1: is_integral (4/4) ✅
- Group 2: is_signed/unsigned (4/4) ✅
- Group 3: is_arithmetic (4/4) ✅
- Group 4: Trivial properties (4/4) ✅
- Group 5: make_signed/unsigned (4/4) ✅
- Group 6: Hash (5/5) ✅ - includes std::unordered_map integration
- Group 7: Backward compatibility (6/6) ✅ - uint128_t/int128_t aliases
- Group 8: Builtin types (4/4) ✅ - nstd:: delegates to std::

### Files Created/Modified

- `include/int128_param_traits_specializations.hpp` (new, ~474 lines)
- `tests/test_traits_specializations.cpp` (rewritten, 215 lines, 35 tests)

### Design Clarification (CRITICAL)

**4 valid type combinations** (not 8 as initially assumed):

- `binnat` - unsigned only (binary natural, no sign encoding)
- `twos_complement` - signed only
- `magnitude_sign` - signed only
- `excess_k` - signed only

**Constraint:** `static_assert((Sign == unsigned_type) == (Form == binnat))` enforces this design

---

## ✅ PRIORITY A: Phase 5 - Additional Operators — COMPLETE

- Increment/decrement (++/--), unary minus for all representations
- 55/55 tests passing on 4 Windows compilers
- Multi-compiler validation complete

---

## ✅ PRIORITY B: Phase166 Feature Parity - 12/12 COMPLETE ✅

### Already Ported ✅

1. Constructors (integral, string, pair)
2. Arithmetic operators (+, -, *, /, %)
3. Comparison operators (<, >, <=, >=, ==, !=)
4. Bitwise operators (&, |, ^, ~)
5. Shift operators (<<, >>)
6. Friend operators (symmetric operations)
7. Helper methods (divmod, abs, swap)
8. Bit manipulation (trailing_zeros, leading_zeros, popcount, rotate)
9. Math functions (int128_param_cmath.hpp)
10. ✅ **Float/double/long double assignment operators** - COMPLETE
11. ✅ **Type traits specializations (nstd::)** - COMPLETE
12. Numeric limits (int128_param_limits.hpp)
13. Numeric algorithms (int128_param_numeric.hpp)
14. I/O streams (int128_param_iostreams.hpp)

### Header 1: ✅ int128_param_safe.hpp - COMPLETE (34/34 tests)

**Status:** ✅ **PRODUCTION READY**  
**Completion Date:** February 4, 2026  
**Time Spent:** ~3.5 hours (estimated 4h)

**Implementation:**

- 3 API styles: `checked_*`, `saturating_*`, `try_*`
- Overflow detection for +, -, *, /
- `checked_result<Sign, Form>` struct with `{value, overflow}`
- Full TC and unsigned support
- MS addition/subtraction working (multiplication blocked by operator*= issue)
- C++20 constexpr throughout

**Test Results:** 34/34 passing (100%) ✅

**Files Created:**

- `include/int128_param_safe.hpp` (380 lines)
- `tests/test_param_safe.cpp` (398 lines, 34 tests)
- `docs/archive/PRIORITY_3_HEADER_1_COMPLETION.md` (~780 lines)

**Files Modified:**

- `include/int128_parameterized.hpp` (+152 lines: max/min methods, divmod fix)

**Bugs Fixed:**

1. Missing max()/min() static methods (added +152 lines)
2. divmod() negates unsigned values (fixed with `if constexpr`)
3. checked_mul() infinite loop (division-based → sign-based)
4. Unsigned overflow detection too strict (AND → OR logic)

**Known Issues:**

- ✅ MS operator*= — FIXED (20 March 2026): zero×neg=-0 bug and magnitude overflow into sign bit
- ✅ EK arithmetic — VERIFIED: `*`, `/`, `%` and compound variants are `= delete` (compile-time error)

**Documentation:** See `docs/archive/PRIORITY_3_HEADER_1_COMPLETION.md` for full report

---

### Remaining Headers ✅ ALL COMPLETE

| Phase166 Header | Phase175 Target | Status |
|-----------------|-----------------|--------|
| `int128_base_safe.hpp` | `int128_param_safe.hpp` | ✅ 34/34 tests, 11 compilers |
| `int128_base_limits.hpp` | `int128_param_limits.hpp` | ✅ 34/34 tests, 11 compilers |
| `int128_base_format.hpp` | `int128_param_format.hpp` | ✅ 10/10 tests, 11 compilers |
| `int128_base_numeric.hpp` | `int128_param_numeric.hpp` | ✅ 11/11 tests, 11 compilers |
| `int128_base_algorithm.hpp` | `int128_param_algorithm.hpp` | ✅ 9/9 tests, 11 compilers |
| `int128_base_thread_safety.hpp` | `int128_param_thread_safety.hpp` | ✅ 43/43 tests, 9 compilers |
| `int128_base_concepts.hpp` | `int128_param_concepts.hpp` | ✅ 13/13 tests, 11 compilers |
| `int128_base_ranges.hpp` | `int128_param_ranges.hpp` | ✅ 13/13 tests, 11 compilers |
| `int128_base_bits.hpp` | `int128_param_bits.hpp` | ✅ 8/8 tests, 11 compilers |
| `int128_base_iostreams.hpp` | `int128_param_iostreams.hpp` | ✅ 28+OK tests, 11 compilers |
| `int128_base_traits.hpp` | `int128_param_traits.hpp` | ✅ 27/27 tests, 11 compilers |
| `int128_base_cmath.hpp` | `int128_param_cmath.hpp` | ✅ 8/8 tests, 11 compilers |

**Progress:** 12/12 complete (100%) ✅  
**Bugs fixed:** iostreams `uint128_tc_t` → `uint128_t`, numeric MSVC `portable_clzll()`

### Recommended Order

1. ✅ **safe** - DONE (overflow-checked arithmetic)
2. ✅ **limits** - DONE (`std::numeric_limits` specialization)
3. ✅ **format** - DONE (Modern C++20 formatting)
4. ✅ **numeric** - DONE (Additional numeric algorithms)
5. ✅ **algorithm** - DONE (STL integration)
6. ✅ **thread_safety** - DONE (Concurrent programming support)
7. ✅ **concepts** - DONE (C++20 concepts)
8. ✅ **ranges** - DONE (C++20 ranges support)

---

## ✅ PRIORITY C: Intrinsics Transplant — COMPLETE

- 5 intrinsics headers already ported and integrated
- Audit completed 18 March 2026: all `__builtin_*` calls unified
- 33 public functions across compiler_detection, arithmetic_operations, bit_operations, byte_operations, fallback_portable

---

## 📅 FUTURE WORK ITEMS

### NEW: Benchmark & Testing Methodology Overhaul

**Status:** ✅ Core Complete | **Priority:** High | **Docs:** `docs/PLAN_BENCHMARK_AND_TESTING_METHODOLOGY.md`

1. **RDTSC-only benchmarks:** ✅ Todos los benchmarks migrados a ciclos CPU directos (RDTSC).
   - ✅ `benchmark_vs_builtin.cpp` — usa RDTSC via `bench_common.hpp`.
   - ✅ `benchmark_divmod_algorithms.cpp` — migrado a `bench_common.hpp` (sesión 21 Mar 2026).
   - ✅ `bench_common.hpp` validado con 4 compiladores: GCC, Clang, MSVC, Intel (Crítica 5 resuelta).
2. **Cobertura sistemática 3-regiones:** ✅ Implementada.
   - ✅ `tests/test_sweep_framework.hpp` — Framework completo (SplitMix64, regiones, sweep_unary/binary).
   - ✅ `tests/test_sweep_framework_validation.cpp` — 20/20 PASS en 4 compiladores (Crítica 3 resuelta).
   - ⬜ Migrar tests existentes al framework de 3-regiones (trabajo futuro, incremental).
3. **Regla formalizada** en `AI_PROMPT/ai-instructions.md` como Regla 8 (Test & Benchmark Methodology).

### NEW: BCD Decimal Types (Base-10 Parameterized)

**Status:** Prototype Complete, Design Planned | **Priority:** Medium-High | **Docs:** `docs/PLAN_BCD_DECIMAL_TYPES.md`

Tipo parametrizado en base 10 con codificación BCD dentro de 128 bits (32 dígitos decimales):

- ✅ **Prototipo validado:** `build_temp/prototype_bcd_conversion.cpp` — Double-Dabble (bin→BCD)
  y Horner (BCD→bin) en 29/29 tests, 4 compiladores.
- **BCD Natural (8-4-2-1):** Para unsigned y Excess-K — aritmética decimal estándar.
- **BCD Aiken (2-4-2-1):** Para signed (TC, MS) — auto-complementario: `~d = 9-d`.
- **Proyección futura:** Base para punto flotante decimal (IEEE 754-2008 decimal128).
- **Casos de uso:** Aritmética financiera, protocolos telecom/banking, conversión string O(n).
- **Limitación descubierta:** BCD128 (32 nibbles) soporta max 10^32-1; uint128_t max (39 dígitos)
  desborda. Considerar BCD160 (40 nibbles) o packed BCD para cobertura completa.
- **Bug Clang:** Funciones BCD no deben ser `constexpr` — el evaluador constexpr de Clang
  produce resultados incorrectos en comparaciones de uint128_t ≥2^64. Usar `inline`.

### NEW: Granlund-Montgomery Division Optimization

**Status:** ✅ COMPLETE (22 March 2026) | **Docs:** `docs/PLAN_DIVMOD_CONSTEXPR.md`

División constexpr por constantes en tiempo de compilación usando multiplicación recíproca.
17 headers legacy analizados en `legacy-code/divmod_by_constexpr/`.
Implementación completa: `int128_param_divmod.hpp`, `div<D>`, `mod<D>`, `divmod_const<D>`, `mul<K>`.
71/71 tests en 4 compiladores. 4-7x speedup sobre Knuth D.

### NEW: Multiplicación Karatsuba

**Status:** Planned | **Priority:** Medium | **Docs:** `docs/PLAN_BCD_DECIMAL_TYPES.md` §11

Algoritmo Karatsuba para multiplicación sub-cuadrática O(n^1.585). Necesario para:

- Tipos BCD más grandes (bcd256_t, bcd512_t) donde schoolbook O(n²) es costoso.
- Futuro `uint256_t` binario.
- Umbral schoolbook/Karatsuba a determinar empíricamente con benchmarks RDTSC.
- Plan detallado: header genérico `include/algorithms/karatsuba.hpp`.

---

## ~~🔴 CRÍTICAS ATACABLES~~ ✅ TODAS RESUELTAS (5/5)

Puntos débiles identificados — **todos resueltos en sesiones del 21-22 Jul 2025**:

### ~~Crítica 1: benchmark_divmod_algorithms.cpp aún usa std::chrono~~ ✅ RESUELTA

- **Problema:** El benchmark de algoritmos de división (`benchs/benchmark_divmod_algorithms.cpp`)
  medía con `std::chrono::high_resolution_clock`, no con RDTSC, incumpliendo la Regla 8.
- **Resolución (sesión 2025-07-21):** Migrado completamente a `#include "bench_common.hpp"`.
  Ahora usa `CycleTimer` + `doNotOptimize()`, 5M iteraciones, 10K warmup,
  reporte tabular con cyc/op + ratio vs baseline. Compilado limpio con GCC 15 y Clang 21.
  Resultado: Knuth D 1.92x más rápido que big_bin en promedio.

### ~~Crítica 2: No existe un test runner unificado~~ ✅ RESUELTA

- **Problema:** Los tests se ejecutan individualmente; no hay un único ejecutable
  o script que corra todos los tests y reporte pass/fail global.
- **Resolución (sesión 2025-07-21):** `python make.py test` ahora funciona como runner
  unificado. Compila y ejecuta todos los 49 test files automáticamente.
  Validado: 49/49 PASS con GCC 15, 49/49 PASS con Clang 21.

### ~~Crítica 3: Cobertura de tests no sigue 3-regiones sistemáticamente~~ ✅ RESUELTA

- **Problema:** Los tests existentes usan valores ad-hoc, no la cobertura de
  3 regiones × 2^21 valores descrita en la metodología.
- **Resolución (sesión 2025-07-22):** Creado `tests/test_sweep_framework.hpp` (~250 líneas)
  con SplitMix64 PRNG determinista, regiones first/last/random de 2^21 valores,
  `sweep_unary()` (6.3M valores) y `sweep_binary()` (12.6M pares + edge cases).
  Validación: `tests/test_sweep_framework_validation.cpp` — 20/20 PASS en los
  4 compiladores (GCC, Clang, MSVC, Intel ICX).

### ~~Crítica 4: Conversiones BCD ↔ binario no implementadas~~ ✅ RESUELTA

- **Problema:** El plan BCD documenta double-dabble e inverso multiplicativo,
  pero no hay implementación ni prototipo que valide la viabilidad constexpr.
- **Resolución (sesión 2025-07-22):** Creado `build_temp/prototype_bcd_conversion.cpp`
  (~370 líneas) con `bcd128_raw` struct, `double_dabble()` (bin→BCD) y
  `horner_bcd_to_binary()` (BCD→bin via Horner mul×10). 29/29 PASS en los
  4 compiladores. Funciones declaradas `inline` (no `constexpr`) debido a bug
  en evaluador constexpr de Clang con operaciones uint128_t ≥2^64.
  **Nota:** BCD128 (32 nibbles) soporta hasta 10^32-1 (32 dígitos); uint128_t max
  (39 dígitos) desborda el rango BCD.

### ~~Crítica 5: bench_common.hpp no se ha compilado con MSVC ni Intel~~ ✅ RESUELTA

- **Problema:** El header compartido de benchmarks se creó y refactorizó
  `benchmark_vs_builtin.cpp`, pero no se ha validado la compilación con los
  4 compiladores (GCC, Clang, MSVC, Intel).
- **Resolución (sesión 2025-07-21):** `bench_common.hpp` compilado y ejecutado
  exitosamente con los 4 compiladores: GCC 15.2.0 ✅, Clang 21.1.8 ✅,
  MSVC 19.50 ✅ (3 pragma warnings suprimidos), Intel ICX 2025.3 ✅.
  El header es totalmente cross-compiler.

---

### 1. Comparative Benchmarking — ✅ COMPLETE (19 March 2026, session 2)

All 9 compiler/mode combinations benchmarked:

- ✅ Win GCC 15.2.0 -O2/-O3: nstd division **0.49x/0.47x vs u64** (2x faster than native!)
- ✅ Win Clang 21.1.8 -O2/-O3: 2.26x/2.16x vs u64 (3x faster than `__int128`)
- ✅ Win MSVC /O2: **1.60x** vs u64 (improved from 1.74x — Knuth D fast paths via `_udiv128`)
- ✅ Win Intel ICX /O2: 3.26x vs u64
- ✅ WSL GCC 14.2.0 -O2/-O3: 2.06x/-O2 (GCC-O3 has pre-existing anomaly)
- ✅ WSL Clang 20.1.8 -O2/-O3: 2.30x/2.24x vs u64
- ✅ WSL Intel ICX 2025.3.2 -O2: **0.96x vs u64** (faster than native! 3.68x faster than `__int128`)
- Results documented: `docs/archive/COMPARATIVE_BENCHMARK_RESULTS.md`
- Phase166 vs phase175 regression analysis: `docs/archive/PHASE166_VS_PHASE175_REGRESSION_ANALYSIS.md`
- GCC -O3 WSL anomaly root cause: `docs/archive/GCC_O3_DIVISION_ANOMALY.md`

**Knuth D refactoring (session 2):** Exposed fast paths [0–3] for MSVC; added `_udiv128` for path [3].
All tests passing: GCC ✅ 30/30 + 25/25, Clang ✅ 30/30 + 25/25, MSVC ✅ 30/30 + 25/25.

### 2. Test Suite Strengthening

- Review test_priority*.cpp for weak assertions
- Add exact value checks instead of non-zero checks
- Add more edge cases for MS and EK representations

### 3. Known Issues: MS/EK

- **MS operator*=**: ✅ FIXED (20 March 2026) — Two bugs corrected:
  1. Zero × negative produced -0 instead of +0 (sign bit set on zero magnitude)
  2. Magnitude overflow into sign bit (bit 63) corrupted sign for large products
  - Fix: Clear sign bit after magnitude multiplication, only set if result non-zero
  - Tests: 13 MS multiplication tests + safe.hpp `ms_mul_mixed_signs` enabled
- **EK arithmetic**: ✅ VERIFIED (20 March 2026) — `*`, `*=`, `/`, `/=`, `%`, `%=` are `= delete` with `requires(is_excess_k)`. All 8 operator forms (including friend `operator*` with builtins) produce compile-time errors. Validated GCC + Clang.
- **Cross-representation casts (MS↔TC↔EK↔binnat)**: ✅ IMPLEMENTED (20 March 2026) — Explicit copy/move constructors between all valid `int128_param_t` instantiations. Uses `representation.hpp` conversion functions (TC as pivot). 72 signed round-trips + 15 unsigned round-trips validated. `test_casts_between_representations` enabled in `test_ms_storage.cpp`.
- See [OPERATOR_SEMANTICS.md](OPERATOR_SEMANTICS.md) for details

### 4. MSYS2 ucrt64 GCC Platform Issue (documented)

- `std::ofstream` inside a non-main function at -O1+ causes segfault on MSYS2 ucrt64 GCC 15.2.0
- Root cause: Windows C++ runtime initialization order with MSYS2 ucrt64 runtime
- Workaround: avoid `std::ofstream` in non-main functions in test files
- `-fno-inline` suppresses the crash but is not a fix

### 4. Phase 5 WSL Validation — ✅ COMPLETE

- 14 test files × 8 WSL compilers (GCC 13/14/15, Clang 18/19/20/21, Intel icpx 2025.3.2) — ALL PASS
- icpx WSL -O2 inlining bug in multi-arg `std::format` — workaround applied (split format calls)

---

## 🔄 REPLANTEAMIENTO — Sesión 5 (22 Julio 2025)

### Estado Actual del Proyecto

| Métrica | Valor |
|---------|-------|
| Headers (include/) | 24 (.hpp): 17 raíz + 5 intrinsics/ + 2 algorithms/ |
| Feature headers operativos | 13/13 + divmod GM |
| Tests (.cpp) | 65 archivos, 65/65 PASS (GCC release) |
| Sweep tests | 13 archivos, ~455M+ additional value checks |
| API docs (docs/) | 15 archivos |
| Benchmarks (benchs/) | 7 archivos: vs_builtin, divmod_algorithms, to_string, from_string, divmod_const, granlund_montgomery, addsub |
| Compiladores validados | 12 (4 Windows MSYS2 + 8 WSL) |
| Representaciones | 4/4 (TC, MS, EK, binnat) |

### Análisis de Benchmarks — Fortalezas y Debilidades

**Fortalezas demostradas (nstd vs __int128):**

| Operación | GCC -O2 | Clang -O2 | Veredicto |
|-----------|---------|-----------|-----------|
| División (/) | **19.8x** más rápido | **1.4x** más rápido | Corona de la librería — Knuth D + fast paths |
| Comparación | **2.3x** más rápido | **3.6x** más rápido | Excelente en ambos compiladores |
| Suma (+) | **1.9x** más rápido | Paridad | GCC genera código más tight |
| Multiplicación (*) | Paridad | Paridad | Esperado — misma instrucción `mul` subyacente |
| to_string() | **1.3x** más rápido | **5.8x** más rápido | Algoritmo de pares de dígitos funciona bien |
| Shift | **1.7x** más rápido | Paridad | — |

**Debilidades identificadas (post-A1 optimization):**

| Operación | GCC -O2 | Clang -O2 | Causa raíz |
|-----------|---------|-----------|------------|
| ~~Resta (-)~~ | ~~1.3x más lento~~ → **0.96x FASTER** | Paridad | ✅ FIXED via sub128() |
| XOR | Paridad | 1.4x más lento | Clang optimiza __int128 XOR nativamente |

**Observaciones clave de compilador:**

- GCC produce aritmética simple ~5x más rápida que Clang (add: 1.19 vs 5.69 cyc/op)
- Clang produce string parsing ~40% más rápido que GCC
- Granlund-Montgomery es **esencial** en Clang: `operator/` es 10-60x más lento que en GCC
- Boost es 3-90x más lento que nstd en todas las operaciones básicas

### Inventario de Trabajo Futuro

Consolidación de todos los items pendientes de NEXT_STEPS y el plan original:

#### Prioridad ALTA (impacto directo en calidad/rendimiento)

| # | Item | Estado | Impacto |
|---|------|--------|---------|
| A1 | ~~**Optimizar resta (GCC)**~~ | ✅ COMPLETE | `sub128()`/`add128()` via `__uint128_t` → SUB 0.96x, ADD 0.96x (faster than __int128) |
| A2 | ~~**Benchmarks MSVC + Intel**~~ | ✅ COMPLETE | All 4 compilers: GCC 0.96x, Clang 1.06x, ICX 0.99x, MSVC 0.98x vs baseline |
| A3 | ~~**Granlund-Montgomery constexpr completo**~~ | ✅ COMPLETE | Fases A-F implementadas, 71/71 tests, 4-7x speedup |
| A4 | ~~**Migrar tests existentes a sweep framework**~~ | ✅ COMPLETE | 5 new sweep files: shift(16), comparison(11), division(13), unary_ops(12), string(8) = 60/60 PASS |

#### Prioridad MEDIA (extensión de funcionalidad)

| # | Item | Estado | Impacto |
|---|------|--------|---------|
| M1 | **BCD Decimal Types completos** | ⏳ Aplazado → Phase 1.80 | Prototipo OK; tipo BCD128 completo aplazado a fases finales |
| M2 | ~~**benchmark_comparison.bash**~~ | ✅ FIXED (15 May 2026) | Reescrito con bench_common.hpp (RDTSC + doNotOptimize); nstd div **6.4x faster** que __int128 |
| M3 | **Test Suite Strengthening** | ⏳ Pendiente | Revisar test_priority*.cpp para assertions débiles; agregar más edge cases |
| M4 | **Clang constexpr bug workaround** | ⏳ Aplazado | Funciones con uint128_t ops ≥2^64 incorrectas si constexpr en Clang; aplazado hasta que sea necesario |

#### Prioridad BAJA (futuro/investigación)

| # | Item | Estado | Impacto |
|---|------|--------|---------|
| B1 | **ARM64/ARM32/RISC-V ports** | ✅ CI hecho | ARM64 nativo en CI; bench_common.hpp con cntvct_el0/rdtime. Intrinsics: x86-only paths already guarded |
| B2 | **Decimal128 (IEEE 754-2008)** | ⏳ Aplazado → Phase 1.85 | Aplazado después de M1 BCD |
| B3 | **int256_t / int512_t extensión** | ⏳ Aplazado → fix_int | Serán casos especiales del tipo fix_int genérico |
| B4 | **Conan/vcpkg packaging** | ⏳ No necesario ahora | conanfile.txt existe; publicación postergada indefinidamente |

### ~~Propuesta de Siguiente Sesión (Sesión 8)~~ — Completado en v1.76-v1.77

1. ~~**Integrar GM en to_string()**~~ ✅ v1.76 — `divmod_const<10>()`/`divmod_const<10^19>()` reemplaza ~105 líneas; `write_u64_digits`, `write_19_padded_digits`, `to_string()` decimal path unificados
2. ~~**Añadir mulhi128 con intrinsics a `gm_div_limbs`**~~ ✅ v1.76 — `rt_mulhi_128()` con `__uint128_t`/`_umul128`; speedup 1.8-2.2x medido en GCC/Clang
3. **BCD Decimal Types completos** (M1) — Tipo BCD128 completo con aritmética
4. **Fix benchmark_comparison.bash** (M2) — `python make.py compare` falla
5. ~~**Commit + tag v1.76**~~ ✅ — Consolidado; v1.77 incluye test suite consolidation

### Decisiones Arquitectónicas Pendientes

1. ~~**¿Merece la resta una instrucción inline ASM?**~~ — ✅ RESUELTO: No fue necesario. `sub128()`/`add128()` via `__uint128_t` genera codegen idéntico a `__int128`.
2. ~~**¿Granlund-Montgomery debe ser siempre constexpr?**~~ — ✅ RESUELTO: Sí, `ce_mulhi_128` es pure C++ constexpr; ruta intrinsics se añadirá como optimization path
3. **¿BCD como phase 1.80 o postergar a 2.0?** — El prototipo funciona; ¿vale la inversión antes de ARM ports?
4. **¿uint256_t completo o solo como soporte interno?** — Actualmente solo struct de 4 limbs para Karatsuba
5. ~~**¿Integrar GM en to_string o mantener fast_divN handcoded?**~~ — ✅ RESUELTO (v1.76): GM integrado + `rt_mulhi_128` cierra la brecha; codebase unificado

---

## 📈 SUCCESS METRICS

Phase 1.75 has achieved:

- ✅ **100% feature parity** with phase166 (12/12 headers)
- ✅ **Equal or superior performance** (Knuth D: 6.24x faster division)
- ✅ **4 complete representations** (TC, MS, EK, binnat)
- ✅ **Full STL integration** (traits, concepts, algorithms)
- ✅ **Production-ready** (safe arithmetic, thread safety)
- ✅ **Modern C++20** (format, ranges, concepts)
- ✅ **12-compiler validation** (4 Windows + 8 WSL)
- ✅ **Intrinsics unified** (cross-compiler abstraction layer)
- ✅ **14 API reference docs** (~280 public symbols documented)

## 📊 PROJECT PROJECTIONS

### Roadmap Estimado

| Phase | Contenido | Estado |
|-------|-----------|--------|
| **1.75** | Parameterized type (4 reprs), 13 feature headers, Knuth D, intrinsics, cross-repr, API docs, Karatsuba, format, hash, **GM constexpr division** | ✅ **COMPLETE** |
| **1.76** | GM integration in to_string, intrinsic mulhi path, subtraction optimization, sweep migration | ✅ **COMPLETE** |
| **1.77** | Test suite consolidation (wave 1-5), CI/CD pipeline, ARM/RISC-V CI, cppcheck+clang-tidy, MS 107 tests, EK 100 tests, M2 compare fix | ✅ **COMPLETE** |
| **1.80** | BCD base-10 types (Natural + Aiken) | 📋 PLANNED (aplazado) |
| **1.85** | Decimal128 floating point (IEEE 754-2008), DPD/BID encoding | 📋 FUTURE |
| **2.0** | Production release: full numeric tower (binary + decimal, integer + float) | 📋 FUTURE |

### Objetivos Pendientes (del plan original de 12)

- **Objetivo 9 (Etapa 9):** Diseño e implementación de tipos BCD → Phase 1.80
- **Objetivo 12:** Punto flotante decimal → Phase 1.85
- **Objetivo 10:** Benchmark methodology sistemática → Phase 1.80

### Métricas de Progreso Global

| Métrica | Actual | Objetivo 2.0 |
|---------|--------|-------------|
| Representaciones binarias | 4/4 ✅ | 4/4 |
| Feature headers (binario) | 12/12 ✅ | 12/12 |
| Tipos BCD | 0/3 (aplazado Phase 1.80) | 3/3 (Natural, Aiken-TC, Aiken-MS) |
| Decimal float | 0/1 (aplazado Phase 1.85) | 1/1 (decimal128) |
| Compiladores CI validados | 12+ (GCC 13-16, Clang 18-22, ARM64, RISC-V QEMU) | 12+ |
| API docs coverage | ~280 symbols | 400+ |

---

## 📋 ARCHIVED CONTENT

Previous session plans, execution plans, and recommendations have been archived.
See `docs/archive/` for historical session documentation.

---

**Report generated:** 22 March 2026
