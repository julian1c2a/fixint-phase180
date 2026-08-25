# Roadmap

**Última actualización:** 25 August 2026

Dónde está el proyecto y hacia dónde va. Las nueve etapas vienen de
[`AI_PROMPT/GENERAL_GUIDES/Explicación_del_Proyecto.md`](AI_PROMPT/GENERAL_GUIDES/Explicaci%C3%B3n_del_Proyecto.md),
que es el documento fundacional; aquí se traducen a estado real.

Para el detalle de la tarea concreta que hay en marcha, [`NEXT_STEPS.md`](NEXT_STEPS.md).
Para lo ya publicado, [`CHANGELOG.md`](CHANGELOG.md).

---

## Dónde estamos

**v1.90.1** — auditoría completa. El tipo `fixed_int_t<N, Sign, Form>` está
terminado como entero de N × 64 bits: aritmética modular completa y `constexpr`
—división y módulo incluidos—, interoperabilidad signed/unsigned al estilo de
los enteros built-in, integración con iostreams, `std::format` y `std::hash`, y
conversión a y desde cadena en bases 2..36.

Suite: 55 ficheros, en verde con GCC 13–16, Clang 18–22, MSVC 19.5x e Intel ICX,
sobre x86-64, x86-32, ARM64, ARM32 y RISC-V 64.

---

## Las nueve etapas

| # | Etapa | Estado |
|---|---|---|
| 1 | Entero de 128 bits: aritmética básica, conversiones, tests y benchmarks | ✅ completada |
| 2 | Unificación de las implementaciones en un solo template con representación parametrizada | ✅ completada |
| 3 | Representaciones con signo Magnitud-Signo y Exceso-K | ✅ completada en `int128_param_t`<br>🔶 pendiente de portar a `fixed_int_t` (ver [ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md)) |
| 4 | Generalización a N × 64 bits | ✅ completada (v1.90) |
| 5 | Punto fijo: parte entera E y fraccionaria F configurables por plantilla | ⬜ siguiente |
| 6 | Punto flotante IEEE 754 generalizado: mantisa en M&S, exponente en Exceso-K | ⬜ no iniciada |
| 7 | Enteros de longitud arbitraria (*big integers*) | ⬜ no iniciada |
| 8 | Racionales exactos sobre los enteros de longitud arbitraria | ⬜ no iniciada |
| 9 | Decimales de 128 bits y superiores (BCD natural y BCD Aiken) | ⬜ no iniciada |

La etapa 6 depende de la 3 y de la 5: el punto flotante generalizado necesita
Magnitud-Signo para la mantisa y Exceso-K para el exponente, y ambas tienen que
estar disponibles en `fixed_int_t` para cualquier N, no solo en 128 bits.

---

## Antes de la etapa 5: dos decisiones tomadas, ninguna empezada

Ninguna de las dos es trabajo de mantenimiento; las dos condicionan lo que venga
después, así que van antes del punto fijo. **El orden entre ellas está fijado por
ADR-007: primero la política de desbordamiento, después la unificación.**

### 1.º — Política de desbordamiento

[ADR-007](docs/decisions/ADR-007-politica-de-desbordamiento-como-parametro.md):
la política de desbordamiento pasa a ser **parámetro de plantilla**,
`fixed_int_t<N, Sign, Form, Policy>`. Con ella el usuario escribe `a + b` de
siempre y es el tipo quien decide qué pasa al desbordar, sin tener que reescribir
el código llamante — que es lo que hoy hace que las `checked_*` casi no se usen.

**Va primero por una razón concreta:** introducir la política después de replicar
los diez headers de ADR-006 significaría portar esa API dos veces.

El diseño está cerrado en
[ADR-008](docs/decisions/ADR-008-diseno-de-la-politica-de-desbordamiento.md):
`wrap` por defecto (el comportamiento de hoy y el de los built-in), `checked`
sin excepciones y con **marca pegajosa** dentro del valor —el equivalente entero
de un NaN—, cuarto parámetro de plantilla, y **prohibido mezclar políticas**:
error de compilación, no conversión silenciosa. Toda la aritmética sigue siendo
`constexpr` y `noexcept`.

[ADR-009](docs/decisions/ADR-009-almacenamiento-de-la-marca-y-operaciones-checked.md)
cierra la última cuestión de implementación que quedaba: **la marca vive en un
miembro extra que solo existe cuando `Policy == checked`**, de modo que
`fixed_int_t<4, ..., wrap>` conserva sus 32 bytes exactos y quien no pide la
marca no la paga. Y decide que las funciones libres `checked_*` **se completan
además de la política**: falta `checked_div` y faltan las tres `saturating_*`,
y mientras falten `int128_param_t` no puede retirarse.

Y devuelven **el propio tipo con política `checked`**: la función libre y la
política dejan de ser dos mecanismos y pasan a ser uno con dos puertas de
entrada, con `valid()` como consulta en ambos casos. Se retira el
`std::optional`, que era la única de las tres formas candidatas que tiraba el
valor al desbordar.

[ADR-010](docs/decisions/ADR-010-orden-total-con-valores-invalidos.md) revoca la
parte de ADR-008 que imitaba al NaN al comparar: **los valores inválidos se
ordenan** en vez de volverse incomparables, así que `operator<=>` conserva su
`std::strong_ordering` y `std::map`, `std::set`, `std::sort` y los contenedores
desordenados siguen siendo correctos. La propagación por la aritmética —lo que
de verdad se quería— no dependía de aquello.

Es un cambio mayor: marca la **2.0**.

### 2.º — Unificación de tipos

[ADR-006](docs/decisions/ADR-006-migracion-int128-param-a-fixed-int.md): toda la
funcionalidad de `int128_param_t` se replica en `fixed_int_t` y el tipo viejo se
retira. Diez headers por portar, y el de más peso son las representaciones MS y
EK, que tocan el núcleo del tipo. Es además lo que desbloquea la etapa 3 para
cualquier N y, con ella, la etapa 6.

---

## Qué no está en el plan

Para que quede dicho, y no haya que deducirlo:

- **Primitivas en tiempo constante.** Esta biblioteca no es criptográfica y no
  ofrece esa garantía. Ver [SECURITY.md](SECURITY.md).
- **Aritmética de precisión arbitraria dinámica** antes de la etapa 7. Hasta
  entonces, todas las anchuras son fijas y conocidas en compilación.
- **Compatibilidad con C++17.** El proyecto usa conceptos, `<bit>`, `<format>` y
  `std::is_constant_evaluated`; C++20 es el suelo.

---

## Deuda anotada

Cosas conocidas, con su sitio apuntado, para que no se pierdan:

- **T6.7** — consolidar los ~10.000 renglones de documentación de raíz
  repartidos en 11 ficheros solapados.
- **T7.6** — mover a `scripts/archive/` los 32 scripts superados que identifica
  [`scripts/README.md`](scripts/README.md).
- **ADR 001–005** — cinco decisiones tomadas hace tiempo y nunca escritas. Ver
  [`docs/decisions/README.md`](docs/decisions/README.md).
- **Cobertura Doxygen** de `fixed_width_int_t.hpp` a nivel de miembro: hoy está
  documentado el fichero, la clase y los miembros con semántica no evidente.
