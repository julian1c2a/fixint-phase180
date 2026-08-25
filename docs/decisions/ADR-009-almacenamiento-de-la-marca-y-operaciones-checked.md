# ADR-009: Dónde vive la marca de inválido, y las `checked_*` se completan

**Estado:** ✅ Aceptado
**Fecha:** 25 August 2026
**Autor:** Julián Calderón Almendros

**Cierra:** la cuestión abierta que dejó
[ADR-008](ADR-008-diseno-de-la-politica-de-desbordamiento.md), y añade una
decisión sobre las funciones libres `checked_*`.

---

## Contexto

ADR-008 estableció que la política `checked` informa mediante **una marca de
inválido pegajosa dentro del valor** —el equivalente entero de un NaN— porque
la aritmética debe seguir siendo `constexpr`, `noexcept` y encadenable. Y dejó
una única cuestión abierta, de implementación: **dónde se guarda esa marca**.

El dato que la condiciona, medido hoy sobre el árbol (GCC 15, UCRT64):

```
sizeof(uint_fixed_t<1>)  =  8        alignof = 8
sizeof(uint_fixed_t<4>)  = 32        trivially_copyable = 1
sizeof(uint_fixed_t<8>)  = 64        standard_layout    = 1
```

Exactamente `8N` bytes, sin relleno. No es casualidad: es contrato. De ahí
dependen la conversión a y desde `std::array<std::byte, N*8>`, `std::bit_cast`,
y que un `uint256_fixed_t[1000]` ocupe 32 000 bytes clavados.

Un `struct { uint_fixed_t<4> v; bool ok; }` mide **40 bytes**: el `bool` cuesta
8 por el alineamiento. Un **25 % más de memoria y de ancho de banda**.

## Decisiones

### 1. La marca vive en un miembro extra, solo cuando `Policy == checked`

Vía clase base con `[[no_unique_address]]`, o especialización del
almacenamiento. El resultado:

| Tipo | Tamaño | Contrato de hoy |
|---|---|---|
| `fixed_int_t<4, ..., wrap>` | **32 bytes** | intacto |
| `fixed_int_t<4, ..., checked>` | 40 bytes | tamaño distinto, deliberado |

**Quien no pide la marca no la paga.** Como `wrap` es la política por defecto
(ADR-008, decisión 2), todo el código que existe hoy conserva tamaño, disposición
y `bit_cast`.

Alternativas descartadas:

- **Un bit robado al valor.** Imposible, no cara: en un entero de anchura fija
  **todos los patrones de bits son valores válidos**. No hay ninguno libre que
  reservar. Con `double` sí se puede porque IEEE-754 apartó un rango de
  exponentes para eso; aquí no existe ese rango.
- **Un miembro extra siempre.** Cobra un 25 % a quien usa `wrap` y no quiere la
  marca para nada, y rompe el contrato del tamaño para todo el mundo.
- **Un `N+1`-ésimo limbo de estado.** Más simple de implementar, pero cambia el
  tamaño siempre, con el mismo problema que la anterior y desperdiciando 63 bits
  de los 64 que añade.

Lo que esta decisión cuesta, y se asume a sabiendas: **`sizeof(fixed_int_t<N,
...>)` deja de tener una respuesta única**, y el almacenamiento pasa a depender
de un parámetro de plantilla. Es una capa de indirección más en la clase.

### 2. Las funciones libres `checked_*` se completan además de la política

No es «política **o** funciones libres»: son **las dos**, y responden a
necesidades distintas.

- La **política** sirve para una cadena: `a * b + c`, comprobada una vez al
  final. Ergonómica, pero obliga a elegir el tipo antes.
- Las **funciones libres** sirven para una comprobación puntual sobre un tipo
  `wrap` corriente, sin cambiar el tipo de la variable.

Y hay un motivo que las hace obligatorias de todos modos: **ADR-006**. Para
retirar `int128_param_t` hay que replicar en `fixed_int_t` todo lo que ofrece, y
ahí el hueco es real:

| | `int128_param_safe.hpp` | `fixed_width_int_t.hpp` |
|---|---|---|
| `checked_add` / `sub` / `mul` | ✅ | ✅ |
| `checked_div` | ✅ | ❌ **falta** |
| `saturating_add` / `sub` / `mul` | ✅ | ❌ **faltan** |

Mientras falten, `int128_param_t` no puede desaparecer. Completarlas no es un
extra: es un requisito de la migración.

## Consecuencias

### Positivas

- Con `wrap` no cambia **nada**: ni el tamaño, ni `bit_cast`, ni la conversión a
  bytes, ni el rendimiento.
- Las dos vías comparten la misma noción de desbordamiento por representación
  que fija ADR-008 decisión 6: se implementa una vez.
- Se desbloquea ADR-006.

### Negativas

- `sizeof` depende de la política. Hay que documentarlo donde se documente el
  contrato de tamaño, y no solo en este ADR.
- El almacenamiento parametrizado complica la clase, que ya tiene tres
  parámetros.

### Riesgo que esta decisión destapa

La marca pegajosa con semántica de NaN —comparaciones `false` salvo `!=`,
ADR-008 decisión 3— **rompe el orden débil estricto**. Un valor inválido dentro
de un `std::map`, un `std::set` o un `std::sort` es comportamiento
indefinido, no un resultado raro. Y en `std::unordered_map` un valor inválido
nunca se encuentra a sí mismo.

Esto importa porque la biblioteca **sí** integra con la STL:
`include/fixed_int_hash.hpp` y 95 asertos en
`tests/test_fixed_stl_integration.cpp`. Queda anotado aquí para decidirlo al
implementar, no para descubrirlo entonces.

## Cuestión abierta que queda

**Qué forma devuelven las `checked_*`.** Hoy conviven dos convenciones para la
misma idea dentro de la misma biblioteca:

| Sitio | Forma | Qué pasa con el valor al desbordar |
|---|---|---|
| `fixed_width_int_t.hpp` | `std::optional<fixed_int_t<N>>` | **se pierde** |
| `int128_param_safe.hpp` | `checked_result{value, bool overflow}` | se conserva |

Y la política de ADR-008 introduce una tercera. Al completarlas hay que elegir
**una**, y la tercera opción merece considerarse: que `checked_add(a, b)`
devuelva el propio tipo con política `checked`. Unificaría los dos mecanismos en
un único concepto con dos puertas de entrada, y `valid()` sería la consulta en
ambos casos.

## Referencias

- [ADR-008](ADR-008-diseno-de-la-politica-de-desbordamiento.md): la decisión que
  este ADR cierra.
- [ADR-007](ADR-007-politica-de-desbordamiento-como-parametro.md): la política
  como parámetro de plantilla.
- [ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md): la migración que la
  decisión 2 desbloquea.
- `include/int128_param_safe.hpp`: las `checked_*` y `saturating_*` que hay que
  replicar.
