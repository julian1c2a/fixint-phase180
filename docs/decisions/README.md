# Architecture Decision Records

Registro de las decisiones de diseño importantes del proyecto: qué se decidió,
en qué contexto, y qué alternativas se descartaron y por qué.

El formato y las reglas están en la sección 26 de [AI-GUIDE.md](../../AI-GUIDE.md).
Lo esencial:

- **Numeración secuencial**, sin reutilizar números.
- **Inmutables una vez aceptados.** Si una decisión cambia, se escribe un ADR
  nuevo que referencia al anterior; no se edita el viejo.
- Se referencian desde el código con `// See ADR-NNN`.

## Índice

| ADR | Título | Estado |
|---|---|---|
| 001 | Constructores explícitos | ⬜ pendiente de escribir |
| 002 | Almacenamiento little-endian de limbos | ⬜ pendiente de escribir |
| 003 | `std::byte` para buffers | ⬜ pendiente de escribir |
| 004 | Sin excepciones en el núcleo | ⬜ pendiente de escribir |
| 005 | Representación parametrizada (TC, MS, EK, binnat) | ⬜ pendiente de escribir |
| [006](ADR-006-migracion-int128-param-a-fixed-int.md) | Replicar `int128_param_t` en `fixed_int_t` hasta retirarlo | ✅ Aceptado |

## Sobre los ADR 001–005

`AI-GUIDE.md` §26 los lista desde hace tiempo como ejemplo de la estructura del
directorio, pero **nunca se escribieron**: este directorio no existía hasta el
24 ago 2026. Las cinco decisiones sí se tomaron y están vivas en el código; lo
que falta es documentarlas.

Se conserva su numeración en vez de renumerar desde 001 por dos motivos: no
romper las referencias de la guía, y dejar visible que hay una deuda concreta de
documentación en vez de esconderla bajo una numeración limpia.

Al escribirlas, reconstruir el contexto de la época a partir de `CHANGELOG.md` y
del historial de git, no de cómo se ve el código hoy: un ADR que racionaliza a
posteriori vale poco.
