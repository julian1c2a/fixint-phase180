# ADR-013: clang-format 22.1.8 en local, 21 en el CI; el árbol satisface las dos

**Estado:** ✅ Aceptado
**Fecha:** 25 August 2026
**Autor:** Julián Calderón Almendros

---

## Contexto

`clang-format` **no es estable entre versiones mayores**: la misma configuración
produce árboles distintos según la versión. El proyecto ya lo sufrió — el
artefacto `R{a} ^ R { b }` que apareció en el árbol de trabajo al empezar la
auditoría del 23 ago 2026 venía de un `clang-format` de la serie 19, que lee
`^ T{` como una declaración y separa la llave.

Desde entonces la versión de referencia declarada era la **21**, y la nota del
proyecto decía que la **22 rompía `std::bitset<64 * N>`** convirtiéndolo en
`<64 *N>` al leer el `*` como puntero.

La máquina de desarrollo tiene las dos: MSYS2 trae la **22.1.8** en `clang64` y
`ucrt64`, y la **21.1.8** en `usr`. Es decir, la versión que un editor va a coger
por defecto es la 22.

## La medida

Sobre los **103 ficheros** de `include/`, `tests/`, `benchs/` y `demos/`, con
`--dry-run --Werror`, el 25 ago 2026:

| Versión | Ficheros que querría cambiar |
|---|---|
| clang-format **21.1.8** | **0** |
| clang-format **22.1.8** | **0** |

**El árbol es hoy punto fijo de las dos.** La nota que decía que la 22 rompía el
`bitset` ya no es cierta sobre este árbol: la línea en cuestión está escrita
`std::bitset<64 *N>`, la forma que las dos aceptan.

## Decisión

1. **En local se usa clang-format 22.1.8**, que es la que hay instalada y la que
   coge el editor. No tiene sentido pedir a nadie que instale una versión más
   vieja para poder guardar un fichero.

2. **El CI sigue comprobando con la 21**, que es la que ofrece `apt.llvm.org`
   para Ubuntu Noble.

3. **La regla es que el árbol satisfaga las dos.** No es un accidente que sean
   distintas: es una **segunda opinión gratis**. Si alguna vez discrepan sobre
   una construcción nueva, el CI se pone en rojo y esa construcción se escribe de
   una forma que valga para ambas.

4. **La serie 19 no se usa**, y queda dicho por qué.

## Alternativas descartadas

- **Fijar las dos a la misma versión.** Es lo obvio, y es peor: pierde el
  contraste. Con una sola versión, una construcción que solo esa versión formatea
  de cierta manera entra en el árbol sin que nadie se entere, y el día que haya
  que actualizar aparecen cientos de líneas de diferencia de golpe. Es
  exactamente cómo se llegó al problema del 23 de agosto.
- **Subir el CI a la 22.** Requiere que `apt.llvm.org` publique el repositorio de
  LLVM 22 para Noble, que hoy no está garantizado, y además colapsaría las dos
  versiones en una, con el problema de arriba.
- **Dejar de comprobar el formato en el CI.** Es lo que había antes de la
  auditoría, y el resultado fue un árbol con reformateos de cientos de líneas
  mezclados con cambios reales, hasta el punto de que un cambio funcional de
  cinco líneas estuvo a punto de perderse dentro de uno de esos diffs.

## Consecuencias

### Positivas

- Quien desarrolla usa la versión que ya tiene, sin instalar nada.
- Dos versiones vigilando en vez de una: la discrepancia se detecta el día que
  aparece, sobre una sola construcción, y no acumulada meses después.

### Negativas

- **Si las dos discrepan, el CI se pone en rojo y el arreglo no se puede
  reproducir en local con la versión de local.** Hay que instalar la otra o
  escribir la construcción a mano de forma que ambas la acepten. Es el coste
  directo de la ventaja de arriba.
- La regla hay que mantenerla medida: si alguien sube la versión de cualquiera de
  los dos lados, hay que volver a comprobar que el árbol sigue siendo punto fijo
  de ambas. La medida de este ADR caduca en cuanto cambie una de las dos.

## Referencias

- Cabecera de `.clang-format`: la nota de versión y el uso.
- `toolchains.json`, sección `herramientas`.
- `.github/workflows/ci.yml`, job `format-and-docs`.
- [STYLE_CONVENTIONS.md](../../STYLE_CONVENTIONS.md).
