# Cómo contribuir

Gracias por el interés. Este documento dice qué hace falta para que un cambio
entre, y qué se comprueba antes de aceptarlo.

El proyecto se distribuye bajo la [Boost Software License 1.0](LICENSE.txt).
Al contribuir aceptas que tu aportación se publique bajo esa misma licencia.

---

## Antes de escribir código

**Abre un issue primero** si el cambio es algo más que un arreglo evidente. En
particular si toca:

- la semántica de alguna operación (redondeo, desbordamiento, signo);
- la superficie pública de `fixed_int_t`;
- el sistema de construcción o el CI.

Para decisiones de diseño el proyecto usa
[ADRs](docs/decisions/README.md); si tu propuesta es una de esas, lo que hace
falta primero es un ADR, no un parche.

Si vas a tocar rendimiento, trae **medidas**. Este proyecto compara contra GMP,
TomMath y Boost.Multiprecision, y una afirmación de velocidad sin números no se
puede evaluar.

---

## Preparar el entorno

Se necesita un compilador con **C++20** completo. Versiones validadas:

| Plataforma | Compiladores |
|---|---|
| Linux (CI) | GCC 13–16, Clang 18–22, Intel ICX |
| Windows | GCC 16.2 (MSYS2 UCRT64), Clang 22.1 (MSYS2 CLANG64), MSVC 19.5x, Intel ICX |
| Docker | GCC 14, Clang 19 (Ubuntu 24.04) |

Las rutas de los compiladores están en [`toolchains.json`](toolchains.json). Para
ver cuál se va a usar de verdad:

```bash
python scripts/toolchains.py
```

En Windows con MSYS2 esto importa más de lo que parece: `g++` y `clang++` **a
secas** resuelven al toolchain MSYS, que no es el del proyecto.

La biblioteca es *header-only*: para usarla basta con `-I include`. No hace falta
ningún flag no estándar; si algún día hiciera falta, es un bug (lo vigila el job
`clang-no-flags` del CI).

---

## Compilar y probar

```bash
python make.py test gcc release-O2     # suite completa con un compilador
python make.py test all all            # todos los compiladores y modos
python make.py list                    # qué objetivos hay
```

La suite son 55 ficheros y tarda unos 3 minutos con GCC en release.

---

## Qué se comprueba antes de aceptar un cambio

Son los mismos cuatro verificadores que ejecuta el CI. Pasarlos en local evita
descubrirlo en GitHub:

```bash
python make.py test gcc release-O2              # 1. la suite, en verde
python scripts/check_headers_selfcontained.py   # 2. cada header compila aislado
python scripts/check_docs_consistency.py --doxygen   # 3. documentación coherente
clang-format-21 --dry-run --Werror <ficheros>   # 4. formato
```

**La versión de clang-format importa**: es la **21**. clang-format no es estable
entre versiones mayores — la 19 y la 22 reformatean este árbol de maneras
distintas. Está explicado en la cabecera de [`.clang-format`](.clang-format).

Y además:

- **Ningún warning nuevo** con `-Wall -Wextra`.
- **Documentación Doxygen** de todo símbolo público nuevo (`@brief`, y `@param`
  / `@return` donde apliquen).
- **Cabecera de licencia** en cada fichero nuevo de `include/`: SPDX, copyright
  y bloque `@file`. Lo comprueba el verificador 3.

---

## Tests

Todo cambio de comportamiento llega con su test. La suite usa un framework
propio, sin dependencias externas: mira cualquier `tests/test_fixed_*.cpp` como
plantilla.

Tres cosas que se valoran especialmente:

1. **Casos frontera**, no solo el camino feliz: `0`, `1`, `max()`, `min()`,
   `min()+1`, potencias de dos, y los desbordamientos.
2. **`static_assert`** cuando la operación es `constexpr`. Son los tests que no
   se pueden saltar.
3. **No cementar un bug.** Si encuentras comportamiento incorrecto y no lo vas a
   arreglar en ese cambio, no escribas un test que lo dé por bueno: anótalo como
   hueco conocido y abre un issue.

Para aritmética, el test más severo que hay es
`tests/test_fixed_differential.cpp`: compara contra un oráculo independiente
—enteros grandes en base 2^32, algoritmos de libro— que no comparte nada con el
código optimizado. Si tocas multiplicación o división, ejecútalo.

---

## Estilo

- C++20. Llaves Allman, 4 espacios, sin tabuladores. Lo fija `.clang-format`.
- **Salida de consola en ASCII.** Sin acentos ni símbolos Unicode en lo que se
  imprime: hay plataformas en la matriz de CI donde eso se ve mal.
- `std::byte` para buffers de bytes, no `unsigned char` ni `char`.
- Nada de `#pragma once`: guardas de inclusión `#ifndef` / `#define` / `#endif`.

---

## Commits y pull requests

- [Conventional commits](https://www.conventionalcommits.org/) para el prefijo:
  `feat:`, `fix:`, `perf:`, `docs:`, `test:`, `build:`, `ci:`, `refactor:`.
  Un `!` tras el tipo si rompe compatibilidad, y el detalle en el cuerpo.
- El mensaje explica **por qué**, no solo qué. Y qué se verificó, con cifras.
- Varios cambios separables → varios commits, **cada uno compilable y con la
  suite en verde**, para que `git bisect` sirva de algo.
- La PR describe qué se probó y en qué compiladores.

---

## Informar de un fallo

Con esto se puede reproducir; sin esto, normalmente no:

- Compilador y versión exacta (`g++ --version`), y sistema operativo.
- El tipo concreto: `uint_fixed_t<4>`, `int_fixed_t<2>`…
- Un programa mínimo que lo reproduzca.
- Qué esperabas y qué salió.

Si es un fallo de aritmética, indica los valores exactos de entrada, mejor en
hexadecimal o limbo a limbo: en decimal es fácil perder un dígito por el camino.
