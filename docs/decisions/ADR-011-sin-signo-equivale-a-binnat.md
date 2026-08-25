# ADR-011: Sin signo equivale a `binnat`; las combinaciones válidas son cuatro, no ocho

**Estado:** ✅ Aceptado
**Fecha:** 25 August 2026
**Autor:** Julián Calderón Almendros

**Cierra:** el primero de los dos cabos sueltos que dejó anotados
[ADR-005](ADR-005-representacion-como-parametro-de-plantilla.md).

---

## Contexto

Al reconstruir ADR-005 apareció que los dos tipos de la biblioteca no admiten las
mismas combinaciones de `Sign` y `Form`:

- `int128_param_t` documenta un alias `uint128_tc_t`: **sin signo + complemento a
  dos**.
- `fixed_int_t` lo prohíbe con un `static_assert`
  ([fixed_width_int_t.hpp:207-209](../../include/fixed_width_int_t.hpp#L207-L209)).

Y quedaba por decidir cuál de las dos reglas sobrevive a la migración de
[ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md).

### La ambigüedad de fondo: «sin signo» significa dos cosas

Antes de decidir hay que separarlas, porque es de confundirlas de donde sale la
duda:

1. **Sin signo en construcción, o semántico.** Es el significado que se le quiere
   dar al tipo: no hay signo que codificar, el rango es `[0, 2^(64N) − 1]`, la
   aritmética es modular sin más. Esto es `signedness::unsigned_type`, y es una
   **decisión de diseño** de quien usa la biblioteca.

2. **Sin signo en los componentes internos.** Los limbos son
   `std::array<std::uint64_t, N>` **siempre**, con cualquier `Sign` y con
   cualquier `Form`. Un `int_fixed_t<4>` guarda cuatro `uint64_t`, no cuatro
   `int64_t`. Esto es un **hecho de almacenamiento**, invariante, y no depende de
   ninguno de los dos parámetros: la interpretación con signo vive en cómo se lee
   el limbo más alto, no en el tipo del limbo.

El sentido 2 no interviene en esta decisión. La toma el sentido 1.

## Decisión

**`signedness::unsigned_type` y `representation_form::binnat` son equivalentes.**
Es un bicondicional, en los dos sentidos:

- Todo tipo sin signo es `binnat`, y
- todo tipo `binnat` es sin signo.

La razón es que las otras tres formas **son codificaciones del signo**:
complemento a dos, magnitud-signo y exceso-K existen para representar valores
negativos. Un tipo sin signo no tiene signo que codificar, así que su forma es el
binario natural y no puede ser otra. «Sin signo en complemento a dos» no es una
combinación restrictiva: es una combinación **sin significado**.

### Las combinaciones válidas, en total

| `Sign` | `Form` | |
|---|---|---|
| `unsigned_type` | `binnat` | ✅ la única sin signo |
| `signed_type` | `twos_complement` | ✅ |
| `signed_type` | `magnitude_sign` | ✅ tras ADR-006 |
| `signed_type` | `excess_k` | ✅ tras ADR-006 |

**Cuatro, no ocho.** Y de las cuatro, dos están implementadas hoy en
`fixed_int_t`; las otras dos llegan con ADR-006.

### La divergencia se resuelve a favor de `fixed_int_t`

`uint128_tc_t` es la anomalía, no la regla que hay que conservar. No se porta:
desaparece con `int128_param_t`.

### `Form` sigue siendo un parámetro aparte

Aunque para el caso sin signo esté determinado. Colapsarlo rompería la forma
uniforme `fixed_int_t<N, Sign, Form>` y la maquinaria de traits, y obligaría a
especializar la plantilla entera. El código ya lo resuelve como debe: **`Form` se
deduce de `Sign` por defecto**
([fixed_width_int_t.hpp:175-177](../../include/fixed_width_int_t.hpp#L175-L177)),
de modo que nadie tiene que escribirlo, y el `static_assert` impide la
combinación sin sentido si alguien lo escribe a mano.

Al portar MS y EK, ese `static_assert` pasa de enumerar las dos combinaciones
implementadas a expresar el bicondicional: sin signo ⟺ `binnat`, y con signo una
de las tres codificaciones de signo.

## Consecuencias

### Positivas

- **La superficie de pruebas se reduce a la mitad.** ADR-005 la anotaba como su
  principal consecuencia negativa: «cuatro formas por dos signos son ocho
  combinaciones». Son cuatro. Las otras cuatro no dejan de probarse: no existen.
- **Queda definida la especialización de `representation_traits<binnat>`** que
  falta —el segundo cabo suelto de ADR-005—, porque ahora se sabe qué es
  `binnat`: sin bit de signo implícito, sin inversión, sin dos ceros, y
  optimizable por hardware igual que el complemento a dos, que es la misma
  aritmética modular.
- El usuario no escribe `Form` casi nunca: los alias y la deducción por defecto
  lo resuelven.

### Negativas

- El parámetro `Form` es **redundante** en el caso sin signo, y eso hay que
  explicarlo: existe por uniformidad de la plantilla, no porque haya algo que
  elegir.
- Un `static_assert` es la única barrera. Alguien que escriba la plantilla
  completa a mano puede intentar la combinación sin sentido; fallará al compilar,
  con un mensaje que debe decir por qué.

## Referencias

- [ADR-005](ADR-005-representacion-como-parametro-de-plantilla.md): la decisión
  que dejó este cabo suelto.
- [ADR-006](ADR-006-migracion-int128-param-a-fixed-int.md): la migración que debe
  generalizar el `static_assert` al portar MS y EK.
- `include/representation.hpp`: el enumerado y las traits.
