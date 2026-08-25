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
| [001](ADR-001-constructores-y-conversiones-explicitos.md) | Todos los constructores y conversiones son `explicit` | ✅ Aceptado · documentado a posteriori |
| [002](ADR-002-almacenamiento-little-endian-de-limbos.md) | Los limbos se almacenan en orden little-endian | ✅ Aceptado · documentado a posteriori |
| [003](ADR-003-std-byte-para-buffers.md) | Los buffers de bytes son `std::byte` | ✅ Aceptado · documentado a posteriori |
| [004](ADR-004-sin-excepciones-en-el-nucleo.md) | La aritmética no lanza; las excepciones son para errores de programación | ✅ Aceptado · documentado a posteriori |
| [005](ADR-005-representacion-como-parametro-de-plantilla.md) | La representación es un parámetro de plantilla (binnat, TC, MS, EK) | ✅ Aceptado · documentado a posteriori · su cabo suelto 1 lo cierra [011](ADR-011-sin-signo-equivale-a-binnat.md) |
| [006](ADR-006-migracion-int128-param-a-fixed-int.md) | Replicar `int128_param_t` en `fixed_int_t` hasta retirarlo | ✅ Aceptado |
| [007](ADR-007-politica-de-desbordamiento-como-parametro.md) | La política de desbordamiento pasa a ser parámetro de plantilla | ✅ Aceptado |
| [008](ADR-008-diseno-de-la-politica-de-desbordamiento.md) | Diseño de la política: `wrap` por defecto, `checked` con marca pegajosa | ✅ Aceptado · decisión 3 revocada en parte por [010](ADR-010-orden-total-con-valores-invalidos.md) |
| [009](ADR-009-almacenamiento-de-la-marca-y-operaciones-checked.md) | La marca de inválido vive en un miembro extra solo con `checked`; las `checked_*` se completan | ✅ Aceptado |
| [010](ADR-010-orden-total-con-valores-invalidos.md) | Los valores inválidos se ordenan (orden total) en vez de volverse incomparables como el NaN | ✅ Aceptado |
| [011](ADR-011-sin-signo-equivale-a-binnat.md) | Sin signo equivale a `binnat`; las combinaciones válidas son cuatro, no ocho | ✅ Aceptado |

## Sobre los ADR 001–005

Las cinco decisiones **son anteriores a este repositorio**. Su commit inicial
—`f257cf8`, 11 ene 2026, «Phase 1.75: Infrastructure complete»— ya las trae todas
puestas, y el propio código remite a una fase anterior: *«extends the unified
template from Phase 1.66»*. Aquí no está, por tanto, el momento en que se
tomaron.

Se documentaron el **25 ago 2026**, reconstruyéndolas a partir de tres fuentes,
en este orden de fiabilidad:

1. **El estado del código en el commit inicial**, que es evidencia directa de qué
   se decidió, aunque no de por qué.
2. **Los comentarios de aquel código**, que en varios casos sí dan el motivo. El
   propósito del proyecto que recoge ADR-005 —investigar codificaciones para coma
   flotante— está citado literalmente de ahí.
3. **`CHANGELOG.md`**, para fechar y para el contexto de cada fase.

Donde la razón no consta se dice que se reconstruye, y no se inventa una. Un ADR
que racionaliza a posteriori vale poco; uno que distingue lo documentado de lo
deducido, sí vale.

Se conserva la numeración 001–005 en vez de renumerar desde 001 para no romper
las referencias de `AI-GUIDE.md`, y porque el orden refleja el orden real de
dependencia: ADR-005 es la decisión que da forma al proyecto y la que da contexto
a ADR-006.

### Lo que salió de escribirlas

Reconstruir estas cinco destapó dos cabos sueltos concretos, anotados al final de
ADR-005: los dos tipos **no admitían las mismas combinaciones** de signo y
representación, y `binnat` **no tiene especialización de
`representation_traits`**.

El primero lo cierra [ADR-011](ADR-011-sin-signo-equivale-a-binnat.md), que al
hacerlo deja definido el contenido del segundo. Es el argumento a favor de
escribir los ADR viejos: no son arqueología, destapan trabajo real.
