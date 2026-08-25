# ADR-010: Los valores inválidos se ordenan, no se vuelven incomparables

**Estado:** ✅ Aceptado
**Fecha:** 25 August 2026
**Autor:** Julián Calderón Almendros

**Revoca en parte:** la decisión 3 de
[ADR-008](ADR-008-diseno-de-la-politica-de-desbordamiento.md), en lo que se
refiere a las comparaciones. Todo lo demás de aquella decisión sigue en pie.

---

## Contexto

ADR-008 decidió que la marca de inválido imitase al NaN también al comparar:

> «Las comparaciones con un operando inválido devuelven `false` salvo `!=`,
> igual que con NaN.»

Al escribir [ADR-009](ADR-009-almacenamiento-de-la-marca-y-operaciones-checked.md)
se vio que esa parte es la que rompe la integración con la STL, y que **no
aporta nada a la propiedad que se quería**.

### Lo que la STL exige

Dos requisitos, y la semántica de NaN incumple los dos:

- **Contenedores ordenados y algoritmos** (`std::map`, `std::set`, `std::sort`,
  `lower_bound`) exigen un **orden débil estricto**. La condición que se rompe
  no es la obvia, sino la **transitividad de la incomparabilidad**: con
  `equiv(a,b)` = «ni `a<b` ni `b<a`», resulta `equiv(1, inválido)` y
  `equiv(inválido, 2)` pero no `equiv(1, 2)`.
- **Contenedores desordenados** exigen que la igualdad sea una **relación de
  equivalencia**. Con `x != x` no es ni reflexiva.

El precio no es un resultado extraño: es **comportamiento indefinido**. En las
implementaciones reales el modo de fallo típico de `std::sort` es salirse del
array, porque un `equiv` no transitivo destruye el centinela del que depende la
inserción sin guarda.

Y la biblioteca sí integra con la STL: `include/fixed_int_hash.hpp` y 95 asertos
en `tests/test_fixed_stl_integration.cpp`.

### Lo que hace el propio estándar

El coma flotante es el único tipo del lenguaje con orden parcial, y C++20 le dio
una salida explícita: **`std::strong_order`** (`<compare>`) implementa el
predicado `totalOrder` de IEEE-754, que **sí ordena los NaN** —los coloca en los
extremos— y distingue `-0.0` de `+0.0`. Es la puerta oficial para meter un
`double` en un `std::map`.

Es decir: cuando el estándar necesita un orden total sobre un tipo con valores
excepcionales, **los ordena**, en vez de dejarlos incomparables.

### Lo que hacen los enteros built-in

Nada: no tienen estado inválido. No hay precedente que seguir por ese lado. Hoy
`fixed_int_t::operator<=>` devuelve `std::strong_ordering`.

## Decisiones

### 1. El orden es total, con cualquier política

`operator<=>` **sigue devolviendo `std::strong_ordering`**, también con
`Policy == checked`. No se introduce `partial_ordering` en ninguna parte, y no
hace falta ofrecer un comparador aparte para los contenedores.

### 2. La comparación es lexicográfica sobre `(válido?, valor)`

```
si a.valid() != b.valid()  ->  el inválido es el mayor
si no                      ->  se comparan los valores
```

De ahí salen las tres propiedades que hacían falta:

- **`x == x` es siempre cierto**, esté marcado o no. La reflexividad vuelve, y
  con ella la relación de equivalencia que exigen `unordered_map` y
  `unordered_set`.
- **El orden es total**, así que `std::map`, `std::set` y `std::sort` son
  correctos.
- `std::hash` no necesita nada especial: dispersa la marca junto con el valor.

### 3. El inválido es mayor que todo valor válido

Es una elección libre entre dos extremos; se decide arriba para que el veneno
**salga a la superficie**: al ordenar queda al final, y `std::max_element` sobre
un rango que contiene un inválido devuelve el inválido en vez de esconderlo.

### 4. Dos inválidos distintos no son iguales

Porque ADR-009 decidió que el valor **se conserva** al desbordar: el resultado
envuelto sigue ahí, y dos desbordamientos con resultados distintos son valores
distintos. Decir que son iguales sería tan raro como decir que `3 == 5`.

Es además lo que hace el precedente elegido: **`totalOrder` de IEEE-754 ordena
los NaN entre sí por su carga útil**, no los declara equivalentes.

Esto es lo que mantiene honesto el `strong_ordering`: dos valores iguales son
indistinguibles por cualquier observación, incluida la conversión a bytes. Si se
hubiera decidido que todos los inválidos son equivalentes, el tipo correcto
sería `std::weak_ordering`.

### 5. Lo que no cambia de ADR-008

- La marca se **propaga**: cualquier operación con un operando inválido produce
  un resultado inválido.
- `to_string()` de un valor inválido lo dice, no devuelve basura.
- La aritmética sigue siendo `constexpr` y `noexcept`.
- `valid()` sigue siendo la consulta del final.

## Consecuencias

### Positivas

- `std::map`, `std::set`, `std::sort`, `std::unordered_map` y
  `std::unordered_set` son **correctos** con valores inválidos dentro. No hay UB
  que documentar, ni comparador aparte que recordar, ni contenedores prohibidos.
- El código de comparación es **más simple** que con semántica de NaN: comparar
  la marca y después el valor, sin casos especiales.
- `operator<=>` conserva el tipo de retorno que ya tiene, así que nada del
  código actual cambia de significado.

### Negativas

- **`inválido == inválido` sorprende a quien venga del coma flotante**, y hay
  que documentarlo donde se documente la política. Es la divergencia deliberada
  con el NaN y conviene que se lea como tal, no como un descuido.
- Un inválido puede colarse en un `std::set` sin que nada chille. Es preferible
  a que el `std::set` se rompa, pero sigue siendo el usuario quien debe llamar a
  `valid()`.

### Lo que se descartó

**Imitar al NaN también en las comparaciones**, que era la decisión de ADR-008.
La propiedad que se quería —que el desbordamiento envenene la cadena y no se
pierda— **no depende de que las comparaciones sean falsas**; la da la
propagación por la aritmética, que se conserva intacta. La parte de la imitación
que se retira no aportaba nada y costaba la corrección de media STL.

`x != x` fue una decisión de señalización del IEEE-754 de 1985, tomada para
hardware, y es de las más discutidas de aquella norma. No es un modelo que
copiar por defecto en una biblioteca de 2026.

## Referencias

- [ADR-008](ADR-008-diseno-de-la-politica-de-desbordamiento.md): la decisión que
  este ADR revoca en parte.
- [ADR-009](ADR-009-almacenamiento-de-la-marca-y-operaciones-checked.md): donde
  se detectó el problema, y donde se decide que el valor se conserva.
- `[cmp.alg]` del estándar: `std::strong_order`, `std::weak_order`.
- `[alg.sorting]`: el orden débil estricto y la transitividad de la
  incomparabilidad.
- `[unord.req.general]`: la igualdad como relación de equivalencia.
- `include/fixed_width_int_t.hpp:589`: el `operator<=>` actual.
