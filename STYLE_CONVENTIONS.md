# Convenciones de estilo de código

**Última actualización:** 25 August 2026

Fuente de verdad de cómo se escribe el código en este proyecto. Antes vivía
dentro de `AI_PROMPT/ai-instructions.md`, en una sección «Coding Standards» de
catorce reglas que nadie encontraba.

Para los nombres, [NAMING_CONVENTIONS.md](NAMING_CONVENTIONS.md). Para el resto
de la guía de desarrollo, [AI-GUIDE.md](AI-GUIDE.md).

> **Lo que decide el formato es [`.clang-format`](.clang-format), no este
> documento.** Aquí se explica el *porqué* de cada regla y lo que un formateador
> no puede comprobar. Si los dos se contradicen, manda el fichero, y este
> documento está mal y hay que arreglarlo.
>
> **La versión de clang-format es la 21.** No es estable entre versiones
> mayores: la 19 y la 22 reformatean este árbol de maneras distintas. Está
> explicado en la cabecera de `.clang-format`.

---

## Formato — lo que fija `.clang-format`

| Regla | Valor |
|---|---|
| Llaves | **Allman** — la llave de apertura va en su propia línea |
| Indentación | 4 espacios, nunca tabuladores |
| Ancho de línea | 110 columnas |
| Contenido de `namespace` | indentado |
| `public:` / `private:` | a 4 espacios menos que los miembros |
| Orden de `#include` | **no se reordena**: el orden del proyecto es deliberado |
| Reflujo de comentarios | desactivado |

```cpp
// Correcto: Allman
if (b.is_zero())
{
    throw std::domain_error("division by zero");
}

template <std::size_t N>
constexpr fixed_int_t<N> operator+(const fixed_int_t<N> &a, const fixed_int_t<N> &b) noexcept
{
    return a += b;
}
```

`template<...>` va **en su propia línea**, y después la firma. Es lo que hace
legible una declaración con tres parámetros de plantilla y una cláusula
`requires`.

## Reglas que un formateador no puede comprobar

### `const` en todo lo que pueda serlo

Cada variable que no se modifica se declara `const`. No es cosmética: es lo que
permite leer una función de 40 líneas sabiendo qué cambia y qué no.

```cpp
const std::uint64_t mask = 0xFFFFFFFFFFFFFFFFull;
const auto [q, r] = divmod(a, b);
for (const auto &limb : data) { /* ... */ }
```

### Inicializar en la declaración, con llaves

```cpp
std::uint64_t carry{0};        // sí
fixed_int_t<4> result{};       // sí
std::uint64_t carry = 0;       // no
std::uint64_t carry;           // no: sin inicializar
```

Las llaves impiden estrechamientos silenciosos. `int x{3.5};` no compila;
`int x = 3.5;` sí, y pierde el valor.

### `constexpr` y `noexcept` por defecto

Toda operación que pueda evaluarse en compilación se marca `constexpr`, y toda
la que no pueda lanzar, `noexcept`. En esta biblioteca no es una aspiración: la
aritmética entera **completa** es `constexpr`, división y módulo incluidos, y
los tests lo comprueban con `static_assert`.

Cuando una operación necesita un intrínseco, el patrón es:

```cpp
if (!std::is_constant_evaluated())
{
    // camino con intrinseco
}
// camino portable, evaluable en compilacion
```

Nunca dejar un intrínseco sin esa guarda: rompe el `constexpr` en la plataforma
donde ese intrínseco sea el único camino compilado. El job `clang-no-flags` del
CI vigila la consecuencia.

### Errores sin excepciones en el camino normal

Las excepciones se reservan para lo que es un error de programación: división
por cero (`std::domain_error`), cadena inválida (`std::invalid_argument`), valor
fuera de rango (`std::out_of_range`).

Lo que puede fallar de forma esperable devuelve un resultado, no lanza:
`try_from_string()` devuelve `parse_result`, `checked_add()` devuelve
`std::optional`. La regla la fija
[ADR-008](docs/decisions/ADR-008-diseno-de-la-politica-de-desbordamiento.md):
la aritmética sigue siendo `noexcept`.

Un `throw` dentro de una función `constexpr` es correcto y deliberado: en
contexto constante convierte la expresión en no-constante, es decir, en un error
de compilación. Es lo que hace `1/0` con un `int`.

### Conversiones explícitas

Todos los constructores desde otro tipo son `explicit`, y todas las conversiones
de salida también. Los enteros built-in de C++ tienen conversiones implícitas
que son una fuente clásica de fallos; esta biblioteca imita su *aritmética*, no
sus accidentes.

### `std::` explícito

Siempre `std::uint64_t`, nunca `uint64_t` a pelo, y nunca `using namespace std;`
en un header.

### Inmutabilidad y casts

Preferir devolver un valor nuevo a mutar un parámetro. Los casts, siempre
explícitos y con la forma de C++ (`static_cast`, `bit_cast`); nunca la forma de
C.

---

## Divergencias resueltas — gana el código

**Decidido el 25 ago 2026.** Al extraer este documento aparecieron dos reglas que
la versión anterior de las «Coding Standards» pedía y que el código nunca
cumplió. La decisión es que **gana el código**, en ambos casos por el mismo
motivo: una está impuesta por herramienta y la otra por volumen.

| La convención vieja pedía | El código hace | Quién gana y por qué |
|---|---|---|
| **Llaves K&R** — apertura en la misma línea | **Allman** — apertura en su propia línea | El código. Lo impone `.clang-format` y el CI lo comprueba en los 103 ficheros |
| **Llaves siempre**, incluso en `if` de una sentencia | `fixed_width_int_t.hpp` tiene **98 `if` sin llaves y 0 con llaves** | El código. Cambiarlo sería reescribir el árbol entero por una regla que nadie ha seguido nunca |

Ambas están ya corregidas en la tabla de arriba. Se dejan anotadas para que
quede claro que fue una decisión tomada, no un descuido, y para que nadie
"arregle" el código hacia la regla vieja.
