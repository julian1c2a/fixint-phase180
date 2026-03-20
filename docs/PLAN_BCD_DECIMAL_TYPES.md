# Plan: BCD Decimal Types (Base-10 Parameterized)

**Status:** Planned  
**Created:** 26 June 2026  
**Priority:** Medium-High — Nuevo tipo fundamental para la biblioteca  
**Branch estimado:** `phase-1.80` o sub-branch de `phase-1.75`

---

## 1. Visión General

Crear un **tipo parametrizado en base 10** utilizando codificación BCD (Binary-Coded Decimal)
dentro de 128 bits. Cada nibble (4 bits) almacena un dígito decimal, permitiendo
**32 dígitos decimales** en 128 bits.

### Dos Variantes por Codificación BCD

| Codificación | Pesos | Representación | Uso |
|-------------|-------|----------------|-----|
| **BCD Natural** | 8-4-2-1 | Unsigned / Excess-K | Aritmética sin signo, offset |
| **BCD Aiken** | 2-4-2-1 | Magnitude-Sign / Two's Complement | Aritmética con signo |

### Por qué BCD Aiken para Signed

BCD Aiken (2-4-2-1) es **auto-complementario**: el complemento bit-a-bit de un dígito `d`
produce `9 - d`. Esto hace que la negación y la sustracción sean operaciones naturales:

```
Dígito d   BCD Aiken   Complemento   Valor complemento
   0        0000        1111          9
   1        0001        1110          8
   2        0010        1101          7
   3        0011        1100          6
   4        0100        1011          5
   5        1011        0100          4
   6        1100        0011          3
   7        1101        0010          2
   8        1110        0001          1
   9        1111        0000          0
```

**Ventaja clave:** Para obtener el complemento a 9 (paso previo al complemento a 10,
análogo al complemento a 1 en binario), basta con invertir todos los bits. Esto
permite implementar negación y sustracción con operaciones bitwise eficientes.

---

## 2. Diseño del Template

### Parámetros del Template

```cpp
// Enum para codificación BCD
enum class bcd_encoding : uint8_t {
    natural,   // 8-4-2-1 (estándar)
    aiken      // 2-4-2-1 (auto-complementario)
};

// Template principal
template <signedness Sign, bcd_encoding Encoding>
class bcd128_t {
    uint64_t data[2];  // 128 bits = 32 nibbles = 32 dígitos decimales
    // data[0] = dígitos 0-15 (menos significativos)
    // data[1] = dígitos 16-31 (más significativos)
};
```

### Combinaciones Válidas

| Sign | Encoding | Alias Propuesto | Justificación |
|------|----------|-----------------|---------------|
| `unsigned_type` | `natural` | `ubcd128_t` | Aritmética BCD sin signo estándar |
| `unsigned_type` | `aiken` | — | ❌ No válida (Aiken es para signed) |
| `signed_type` (TC) | `aiken` | `bcd128_tc_t` | Two's complement en BCD Aiken |
| `signed_type` (MS) | `aiken` | `bcd128_ms_t` | Magnitude-sign en BCD Aiken |
| `signed_type` (EK) | `natural` | `bcd128_ek_t` | Excess-K con offset en BCD Natural |

```cpp
// static_assert enforcing valid combinations
static_assert(
    (Sign == unsigned_type && Encoding == bcd_encoding::natural) ||
    (Sign == signed_type   && Encoding == bcd_encoding::aiken) ||
    (is_excess_k           && Encoding == bcd_encoding::natural),
    "Invalid BCD sign/encoding combination"
);
```

### Relación con el Template Existente

```
int128_param_t<Sign, Form>             ← Binario (existente, phase 1.75)
nstd::decimal::bcd128_t<Sign, Enc>     ← Decimal BCD (nuevo, phase 1.80)
```

Ambos comparten `uint64_t data[2]` como almacenamiento, pero la **semántica de los bits
es completamente diferente** (posicional binario vs. nibble-decimal).

### Sub-Namespace: `nstd::decimal`

Los tipos BCD vivirán en `nstd::decimal::` para separación semántica clara:

```cpp
namespace nstd::decimal {
    template <signedness Sign, bcd_encoding Encoding>
    class bcd128_t;

    // Aliases
    using ubcd128_t    = bcd128_t<unsigned_type, bcd_encoding::natural>;
    using bcd128_tc_t  = bcd128_t<signed_type,   bcd_encoding::aiken>;  // TC
    using bcd128_ms_t  = bcd128_t<signed_type,   bcd_encoding::aiken>;  // MS form
    using bcd128_ek_t  = bcd128_t<signed_type,   bcd_encoding::natural>; // EK form
}
```

**Ventajas:**

- Evita colisiones de nombre con tipos binarios en `nstd::`
- Permite `using namespace nstd::decimal;` selectivo
- Clarifica en el código si se trabaja con tipos binarios o decimales
- Facilita la organización de headers (`include/decimal/bcd128_*.hpp`)

---

## 3. Operaciones Requeridas

### 3.1. Aritmética BCD Natural (8-4-2-1)

La suma BCD requiere un paso de corrección (ajuste decimal):

```cpp
// Suma BCD: resultado + 6 si nibble > 9
constexpr bcd128_t operator+(const bcd128_t& a, const bcd128_t& b) noexcept {
    // 1. Suma binaria normal
    // 2. Detectar nibbles > 9 (carry decimal)
    // 3. Sumar 0x6 a cada nibble que exceda 9
    // 4. Propagar carries entre nibbles
}
```

**Operaciones completas:**

| Operación | Complejidad | Notas |
|-----------|-------------|-------|
| `+`, `-` | O(1) con corrección | Requiere ajuste decimal por nibble |
| `*` | O(n^2) nibble | Multiplicación dígito a dígito |
| `/`, `%` | O(n^2) | División BCD (más compleja que binaria) |
| Comparación | O(1) | Comparación directa (orden lexicográfico = numérico) |
| Shifts decimales | O(1) | Shift de nibbles (×10, ÷10) |
| `to_string()` | O(n) | Trivial: cada nibble → ASCII |
| `from_string()` | O(n) | Trivial: cada ASCII → nibble |

### 3.2. Aritmética BCD Aiken (2-4-2-1)

**Suma Aiken:** Diferente al Natural porque el ajuste es +6 para carry-out
y -6 para carry-in, debido a la codificación no estándar de 5-9.

**Negación (complemento a 10):**

```cpp
constexpr bcd128_t operator-() const noexcept {
    // 1. Invertir todos los bits (complemento a 9, gratis con Aiken)
    // 2. Sumar 1 (convierte complemento a 9 → complemento a 10)
}
```

### 3.3. Conversiones

```cpp
// BCD ↔ Binario
constexpr explicit operator uint128_t() const noexcept;
constexpr explicit bcd128_t(uint128_t binary_value) noexcept;

// BCD ↔ String (trivial para BCD)
constexpr std::optional<bcd128_t> from_string(const char* str) noexcept;
const char* to_cstr() const noexcept;  // Buffer rotativo como int128

// BCD Natural ↔ BCD Aiken
constexpr explicit bcd128_t<S, aiken>(bcd128_t<S, natural> src) noexcept;
```

---

## 4. Rango de Valores

| Tipo | Rango | Dígitos | Comparación |
|------|-------|---------|-------------|
| `ubcd128_t` | `[0, 10^32 - 1]` | 32 | `uint128_t` cubre hasta ~3.4×10^38 |
| `bcd128_tc_t` | `[-(10^31), 10^31 - 1]` | 31 + signo | Similar a `int128_t` |
| `bcd128_ms_t` | `[-(10^31 - 1), 10^31 - 1]` | 31 + signo | Doble cero posible |
| `bcd128_ek_t` | Depende del offset K | 32 | Rango desplazado |

**⚠️ IMPORTANTE:** 32 dígitos decimales < 39 dígitos de `uint128_t` (2^128 ≈ 3.4×10^38).
El tipo BCD sacrifica rango (~10^6 veces menor que `uint128_t::max()`) a cambio de
representación decimal exacta. Esto DEBE documentarse prominentemente en la API.

### 4.1. `std::numeric_limits` para Tipos BCD

Los tipos BCD DEBEN tener `std::numeric_limits` completo y consistente:

```cpp
template <>
struct std::numeric_limits<nstd::decimal::ubcd128_t> {
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr int  radix = 10;              // ← Base 10, no 2

    static constexpr int  digits = 32;             // 32 dígitos decimales
    static constexpr int  digits10 = 32;           // Exacto para BCD
    static constexpr int  max_digits10 = 32;

    static constexpr ubcd128_t min()     { return ubcd128_t{0}; }
    static constexpr ubcd128_t max()     { /* 10^32 - 1 = 99999999999999999999999999999999 */ }
    static constexpr ubcd128_t lowest()  { return min(); }
};
```

**Diferencia clave vs binario:** `radix = 10` (no 2), y `digits` = número exacto de
dígitos decimales. Esto permite que código genérico consulte `digits` de forma estándar
y obtenga el resultado correcto independientemente de si el tipo es binario o decimal.

### 4.2. Métodos `max()` y `min()` como Miembros Estáticos

Además de `std::numeric_limits`, los tipos BCD tendrán métodos estáticos `constexpr`:

```cpp
namespace nstd::decimal {
    template <signedness S, bcd_encoding E>
    class bcd128_t {
    public:
        static consteval bcd128_t max() noexcept;
        static consteval bcd128_t min() noexcept;
        static constexpr int decimal_digits() noexcept;  // 32 para unsigned, 31 para signed
    };
}
```

---

## 5. Ventajas y Casos de Uso

### Ventajas de BCD sobre Binario

1. **Conversión a/desde string O(n)** — No requiere divisiones sucesivas
2. **Aritmética decimal exacta** — Sin errores de redondeo en base 10
3. **Aplicaciones financieras** — Cálculos monetarios sin pérdida de precisión
4. **Hardware BCD** — x86 tiene instrucciones AAA, AAS, AAM, AAD (legacy, con pendientes en 64-bit)
5. **Depuración visual** — Los valores son legibles directamente en hex dump

### Casos de Uso

- Cálculos financieros (contabilidad, facturación, impuestos)
- Empaquetado de datos para protocolos que usan BCD (telecomunicaciones, banking)
- Representación intermedia para conversión numérica de alta precisión
- Base para futura aritmética de punto flotante decimal (IEEE 754-2008 decimal128)

---

## 6. Proyección Futura: Punto Flotante Decimal

El tipo BCD de 128 bits sirve como mantisa para un futuro tipo **decimal128**
compatible con IEEE 754-2008:

```
Total: 128 bits
├── Sign:     1 bit
├── Exponent: 14 bits (combination field)
└── Mantissa: 113 bits → 34 dígitos decimales (DPD o BID encoding)
```

**Dos posibles codificaciones:**

- **DPD (Densely Packed Decimal):** 3 dígitos en 10 bits (eficiente en espacio)
- **BID (Binary Integer Decimal):** Mantisa como entero binario (eficiente en cálculo)

La experiencia con `bcd128_t` informará la elección de codificación para el tipo flotante.

---

## 7. Plan de Implementación

### Fase 1: Infraestructura

1. Crear `include/bcd128_base.hpp` — Almacenamiento, constantes, helper nibble access
2. Definir enums `bcd_encoding`, type aliases
3. Implementar `get_nibble(index)` / `set_nibble(index, value)` — Acceso individual a dígitos
4. Implementar validación BCD (todos los nibbles ∈ [0,9])

### Fase 2: BCD Natural (unsigned)

1. Suma BCD con ajuste decimal (`+`, `+=`)
2. Resta BCD con borrow (`-`, `-=`)
3. Comparación
4. Conversión `to_string()` / `from_string()`
5. Conversión BCD ↔ binario
6. Shifts decimales (×10, ÷10 como shift de nibbles)

### Fase 3: BCD Aiken (signed)

1. Negación via complemento bitwise + 1
2. Adaptadores de suma/resta para codificación Aiken
3. Conversiones Natural ↔ Aiken
4. Implementar TC, MS, EK sobre BCD Aiken

### Fase 4: Multiplicación y División

1. Multiplicación dígito × dígito con acumulación
2. División BCD (algoritmo de lápiz y papel adaptado)
3. Módulo
4. Optimizaciones para ×10 y ÷10 (triviales en BCD)

### Fase 5: Integración STL

1. `std::numeric_limits<bcd128_t<...>>`
2. Traits, concepts, hash
3. I/O streams
4. Format (`std::format`)

### Fase 6: Tests y Benchmarks

1. Tests con metodología de 3 regiones (ver PLAN_BENCHMARK_AND_TESTING_METHODOLOGY.md)
2. Benchmarks RDTSC comparando BCD vs binario para conversión string
3. Benchmarks de aritmética BCD vs aritmética binaria + conversión

---

## 8. Archivos Esperados

```
include/
├── bcd128_base.hpp                    # Core BCD type
├── bcd128_arithmetic.hpp              # Suma, resta, mul, div
├── bcd128_conversions.hpp             # BCD ↔ binario, BCD ↔ string
├── bcd128_traits_specializations.hpp  # STL integration
└── bcd128_aiken.hpp                   # Aiken-specific operations

tests/
├── bcd128_natural_tests.cpp
├── bcd128_aiken_tests.cpp
├── bcd128_conversion_tests.cpp
└── bcd128_arithmetic_tests.cpp

benchs/
├── bcd128_vs_binary_benchs.cpp
└── bcd128_string_conversion_benchs.cpp
```

---

## 9. Validación de Nibbles BCD Aiken

### Códigos Válidos e Inválidos

En la codificación Aiken (2-4-2-1), solo 10 de los 16 patrones de 4 bits son válidos:

```
Válidos:   0000 0001 0010 0011 0100  1011 1100 1101 1110 1111
Decimal:     0    1    2    3    4     5    6    7    8    9
Inválidos: 0101 0110 0111 1000 1001 1010
Binario:   0x5  0x6  0x7  0x8  0x9  0xA
```

**Hay un "salto" entre 4 (0100) y 5 (1011).** Los patrones 0101-1010 (0x5-0xA)
no representan ningún dígito decimal en Aiken.

### Validación Obligatoria

```cpp
/// Verifica que todos los 32 nibbles sean códigos Aiken válidos
static constexpr bool is_valid_aiken(uint64_t data[2]) noexcept {
    // Para cada nibble: válido si nibble <= 4 || nibble >= 11
    // Inválido si 5 <= nibble <= 10 (0x5 a 0xA)
    for (int i{0}; i < 32; ++i) {
        const uint8_t nibble{get_nibble(data, i)};
        if (nibble >= 5 && nibble <= 10) {
            return false;
        }
    }
    return true;
}
```

**Puntos de validación:**

- En `from_string()`: rechazar si la conversión produce nibbles inválidos
- En aritmética: la corrección post-operación debe producir solo nibbles válidos
- En debug builds: `assert(is_valid_aiken(data))` después de cada operación
- La validación puede desactivarse en release builds para rendimiento

---

## 10. Conversiones Binario ↔ BCD: Algoritmos Eficientes

### 10.1. Binario → BCD: Double-Dabble

El algoritmo **Double-Dabble** (shift-and-add-3) convierte un número binario a BCD
sin usar divisiones, operando bit a bit:

```
Algoritmo Double-Dabble:
1. Inicializar resultado BCD = 0
2. Para cada bit del número binario (MSB primero):
   a. Para cada nibble BCD: si nibble >= 5, sumar 3
   b. Shift left todo el registro BCD + bit actual entra por la derecha
3. El resultado BCD contiene la representación decimal
```

**Complejidad:** O(n²) donde n = número de bits, pero con constante baja
(operaciones simples por iteración: comparación, suma de 3, shift).

Para 128 bits: 128 iteraciones × 32 nibbles = ~4096 operaciones elementales.

**Ventaja:** Solo usa shifts y sumas condicionales de 3. No requiere
división ni multiplicación. Ideal para implementación constexpr.

### 10.2. BCD → Binario: Horner con Inverso Multiplicativo

La conversión BCD → binario requiere evaluar:

```
valor = d[31]×10^31 + d[30]×10^30 + ... + d[1]×10 + d[0]
```

Usando **esquema de Horner** (evaluación de polinomio):

```
valor = (((...((d[31]×10 + d[30])×10 + d[29])×10 + d[28])...)×10 + d[0])
```

Las **multiplicaciones por 10** se pueden optimizar como:

```cpp
// Multiplicación por 10 eficiente:
constexpr uint128_t mul10(uint128_t x) noexcept {
    return (x << 3) + (x << 1);  // x*8 + x*2 = x*10
}
```

Las **divisiones por 10** (necesarias en bin→BCD via método clásico) pueden usar
el **inverso multiplicativo** planificado en `PLAN_DIVMOD_CONSTEXPR.md`
(Granlund-Montgomery). Al ser 10 una constante conocida en tiempo de compilación,
el inverso multiplicativo es:

```
Para uint128_t, div por 10:
  magic = ceil(2^131 / 10) ← constante precalculable
  q = mulhi(x, magic) >> 3
```

Esto convierte cada `x / 10` en una multiplicación + shift, eliminando
completamente la división hardware.

### 10.3. Estrategia de Conversión Recomendada

| Dirección | Algoritmo Primario | Alternativa | Notas |
|-----------|-------------------|-------------|-------|
| bin → BCD | Double-Dabble | Divisiones con inv. mult. | Constexpr-friendly |
| BCD → bin | Horner (mul×10) | — | Siempre eficiente |
| BCD → string | Nibble → ASCII directo | — | O(n), trivial |
| string → BCD | ASCII → Nibble directo | — | O(n), trivial |

---

## 11. Multiplicación Karatsuba

### Motivación

La multiplicación schoolbook (O(n²) en dígitos) es aceptable para 32 dígitos BCD,
pero para tipos más grandes (bcd256_t, bcd512_t) o para la multiplicación
interna de componentes, **Karatsuba** (O(n^1.585)) será necesaria.

### Algoritmo

Para multiplicar dos números de n dígitos x e y:

```
x = x_H × B + x_L       (split en mitades, B = 10^(n/2))
y = y_H × B + y_L

z0 = x_L × y_L
z2 = x_H × y_H
z1 = (x_L + x_H) × (y_L + y_H) - z0 - z2

x × y = z2 × B² + z1 × B + z0
```

**3 multiplicaciones** en lugar de 4 (schoolbook), con overhead de sumas.

### Aplicabilidad en int128

- **BCD 128 bits (32 dígitos):** Karatsuba da beneficio marginal (~32 dígitos es el
  punto de cruce para algunos análisis). Medir con benchmarks RDTSC.
- **BCD 256+ bits:** Karatsuba será claramente superior.
- **Multiplicación binaria 256-bit:** También se beneficiará cuando se implemente
  `uint256_t` en el futuro.

### Plan de Implementación

```cpp
// En include/decimal/bcd128_arithmetic.hpp (y futuro include/algorithms/karatsuba.hpp)

/// Multiplicación Karatsuba genérica para enteros grandes
/// @tparam T tipo entero (bcd128_t, uint256_t, etc.)
template <typename T>
constexpr T karatsuba_mul(const T& x, const T& y) noexcept;

/// Umbral: usar schoolbook si n_digits <= KARATSUBA_THRESHOLD
static constexpr int KARATSUBA_THRESHOLD{16};  // Determinar empíricamente
```

**El umbral exacto se determinará mediante benchmarks RDTSC** comparando
schoolbook vs Karatsuba para diferentes tamaños.

---

## 12. Consideraciones de Diseño (Decisiones Tomadas y Abiertas)

### Decisiones Tomadas ✅

1. **Template separado de `int128_param_t`:** La semántica de bits es fundamentalmente
   diferente. `bcd128_t` es un template independiente.

2. **Sub-namespace `nstd::decimal::`**: Separación clara de tipos binarios y decimales.
   Headers en `include/decimal/`.

3. **`std::numeric_limits` con `radix = 10`:** Los tipos BCD reportan `digits` como
   número de dígitos decimales, no bits. `radix = 10` es la forma estándar.

4. **Almacenamiento `uint64_t data[2]`:** Para alineación y eficiencia. Los nibbles
   se acceden via masks y shifts.

5. **Conversiones explícitas BCD ↔ binario:** No conversiones implícitas.

### Cuestiones Abiertas

1. **¿DPD vs BID para el futuro decimal128?** BID es más simple de implementar
   en software; DPD es más eficiente en densidad. Decisión aplazada.

2. **¿Soporte para BCD de diferentes tamaños?** Posible generalización futura:
   `bcd_t<N_bits, Sign, Encoding>` donde N = 64, 128, 256.

3. **¿BCD empaquetado vs desempaquetado?** El plan actual asume packed BCD
   (2 dígitos por byte). Para operaciones SIMD, unpacked BCD (1 dígito por byte,
   16 dígitos por uint128_t) podría ser más eficiente. Posible tercer encoding.

4. **Umbral Karatsuba vs schoolbook:** Se determinará empíricamente con benchmarks.

---

**Documentación relacionada:**

- `docs/PLAN_BENCHMARK_AND_TESTING_METHODOLOGY.md` — Metodología de testing aplicable
- `OPERATOR_SEMANTICS.md` — Semántica de operadores por representación
- `include/int128_parameterized.hpp` — Template binario de referencia
