# ADR-012: Un tag publicado no se mueve; los arreglos van en una versión nueva

**Estado:** ✅ Aceptado
**Fecha:** 25 August 2026
**Autor:** Julián Calderón Almendros

---

## Contexto

El workflow `Release` falló sobre el tag `v1.90.1` y **no llegó a publicarse
ninguna release**. La causa: el paso de construcción llamaba a cuatro scripts de
los que **tres no existían** en el repositorio (`scripts/vcvarsall.py`,
`scripts/setvarsall_intel.py`, `scripts/build_benchmarks.bash`).

El arreglo está en el commit `03283af`, que construye con `make.py`. Pero el tag
`v1.90.1` apunta a `ce71d5d`, **anterior al arreglo**, así que el arreglo no se
ha ejercitado nunca.

De ahí la pregunta: ¿se mueve el tag `v1.90.1` al commit arreglado, o se publica
una versión nueva?

## Decisión

**Un tag ya empujado a `origin` no se mueve. Nunca.** El arreglo va en una
versión nueva: **`v1.90.2`**.

### Por qué

Un tag es una promesa: «este nombre designa este código». Moverlo la rompe, y la
rompe **en silencio**, que es lo peor:

- Quien ya tenga `v1.90.1` no se entera de nada. Un `git fetch` normal **no
  actualiza los tags que ya existen en local**, así que dos personas con
  «la misma versión» tendrían código distinto y ninguna forma de notarlo.
- Un `git fetch --tags --force` sí lo actualiza, y entonces el árbol de trabajo
  de alguien cambia bajo sus pies sin que haya pedido nada.
- Cualquier cosa que haya fijado ese tag —un submódulo, un `FetchContent`, un
  paquete, un enlace en un correo— pasa a apuntar a otro código.

El coste de la alternativa es una entrada más en el `CHANGELOG`. El coste de
mover el tag es que la historia deje de ser fiable.

### La regla, para no volver a plantearlo

| Situación | Qué se hace |
|---|---|
| El tag aún **no** está en `origin` | Se puede rehacer libremente |
| El tag ya está en `origin` | **No se toca.** El arreglo va en una versión nueva |
| La release salió mal y no hay artefactos | Igual: versión nueva. El tag roto se queda como testimonio |

Un tag que apunta a una publicación fallida no es basura que haya que limpiar:
es el registro de que aquello falló, y el `CHANGELOG` explica por qué.

## Consecuencias

### Positivas

- `v1.90.1` sigue significando exactamente lo que significaba.
- La v1.90.2 **ejercita el arreglo de verdad**: es la primera vez que
  `release.yml` se ejecuta con el paso de construcción corregido. Si vuelve a
  fallar, se sabrá enseguida y sobre un tag que no promete nada todavía.

### Negativas

- Queda en el repositorio un tag cuya release nunca se publicó. Hay que
  explicarlo en el `CHANGELOG` para que no parezca un descuido.
- El número de versión avanza por un fallo de infraestructura, no por un cambio
  de la biblioteca. Es el precio de que los tags signifiquen algo.

## Referencias

- `.github/workflows/release.yml`, arreglado en `03283af`.
- `CHANGELOG.md`, entrada de v1.90.2.
