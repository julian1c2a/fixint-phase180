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
int128_param_t<Sign, Form>     ← Binario (existente, phase 1.75)
bcd128_t<Sign, Encoding>       ← Decimal BCD (nuevo, phase 1.80)
```

Ambos comparten `uint64_t data[2]` como almacenamiento, pero la **semántica de los bits
es completamente diferente** (posicional binario vs. nibble-decimal).

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

**Nota:** 32 dígitos decimales < 39 dígitos de `uint128_t` (2^128 ≈ 3.4×10^38).
El tipo BCD sacrifica rango a cambio de representación decimal exacta.

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

## 9. Consideraciones de Diseño Abiertas

1. **¿Compartir template con `int128_param_t`?** Probablemente NO — la semántica de bits
   es fundamentalmente diferente. Mejor un template separado `bcd128_t`.

2. **¿Usar `std::byte` para nibble storage?** El almacenamiento sigue siendo `uint64_t data[2]`
   para alineación y eficiencia. Los nibbles se acceden via masks y shifts.

3. **¿Soporte para BCD de diferentes tamaños?** Posible generalización futura:
   `bcd_t<N_bits, Sign, Encoding>` donde N = 64, 128, 256.

4. **¿DPD vs BID para el futuro decimal128?** BID es más simple de implementar
   en software; DPD es más eficiente en densidad. Decisión aplazada.

5. **¿Interop con `int128_param_t`?** Conversiones explícitas entre binario y BCD.
   No conversiones implícitas (diferentes semánticas de bits).

---

**Documentación relacionada:**

- `docs/PLAN_BENCHMARK_AND_TESTING_METHODOLOGY.md` — Metodología de testing aplicable
- `OPERATOR_SEMANTICS.md` — Semántica de operadores por representación
- `include/int128_parameterized.hpp` — Template binario de referencia
