# Plan: Division/Modulo por Constantes en Tiempo de Compilacion

## 1. Estado Actual (legacy-code/divmod_by_constexpr/)

### Que existe

- **13 divisores implementados manualmente**: 2, 3, 5, 6, 7, 9, 10, 11, 13, 17, 19, 23, 29, 31
- **Tecnica**: Multiplicacion por inverso modular (Barrett reduction simplificada)
  - Para 8/16/32 bits: `(n * M) >> S` donde M y S son constantes magicas
  - Para 64 bits: usa `unsigned __int128` para el producto intermedio, con fallback a division HW
- **Patron comun en cada `divrem_D.hpp`**:
  - `mult_D<N>(n)` — multiplicacion por constante via shifts+adds (siempre rapida)
  - `div_D<N>(n)`  — division via inverso multiplicativo
  - `mod_D<N>(n)`  — `n - D * div_D(n)` (derivado)
  - `divrem_D<N>(n)` — retorna `{quot, rem}`
  - Potencias: `div_pow3<N,K>()` — aplica div_3 K veces (recursivo)
- **Resultados benchmark**: speedup ~0.94x-1.11x para uint32/uint64
  - El compilador ya usa la misma tecnica para constantes conocidas en compilacion
  - La paridad se alcanza, pero no se supera, porque GCC/Clang ya optimizan `n / 3` igualmente
- **Scripts Python** (`solve_pow*.py`): Analizan formulas booleanas de bits de p^n usando sympy (SOP/POS/ANF)
- **Tipos soportados**: solo uint8_t, uint16_t, uint32_t, uint64_t (NO int128)

### Que falta

1. **Generador automatico de inversos en constexpr** — las constantes magicas (M, S) estan hardcodeadas
2. **Extension a 128 bits** — el tipo objetivo es `int128_param_t`
3. **Sistema de expresiones** — informar al compilador que ciertos patrones son optimizables
4. **Soporte signed** — solo hay unsigned, falta complemento a 2, M-S, EK

---

## 2. Fundamento Teorico

### 2.1 Division por Inverso Multiplicativo (Granlund-Montgomery)

Para un divisor constante `d`, se puede reemplazar `n / d` por:

```
q = mulhi(n, M) >> S
```

Donde:

- `M = ceil(2^(W+S) / d)` — el "numero magico" (inverso multiplicativo)
- `S` — shift post-multiplicacion
- `W` — ancho del tipo (32, 64, 128 bits)
- `mulhi(a, b)` — parte alta de la multiplicacion a*b

**Para 128 bits**: necesitamos `mulhi_128x128 -> 128 bits`, que ya tenemos en la
libreria int128 (o podemos construir con `mul_full` de 64x64 -> 128).

### 2.2 Tres Metodos Segun el Divisor

| Metnica | Condicion | Formula | Ejemplo |
|---------|-----------|---------|---------|
| **Shift directo** | d = 2^k | `n >> k` | d=2,4,8,16... |
| **Multiply-shift** | M cabe en W bits | `mulhi(n, M) >> S` | d=3 (W=32) |
| **Add-shift** | M necesita W+1 bits | `(n + mulhi(n, M')) >> S` | d=7 (W=64) |

### 2.3 Calculo de las Constantes Magicas

Algoritmo (Hacker's Delight, cap.10 / Granlund-Montgomery):

```
Input: d (divisor), W (ancho en bits)
Output: M (multiplicador), S (shift), metodo (multiply o add)

1. p = W  (empezar con shift = 0)
2. Mientras 2^p < d * (2^W - rem(2^p, d)):
       p += 1
3. M = (2^p + d - rem(2^p, d)) / d
4. Si M < 2^W:
       metodo = multiply, S = p - W
   Sino:
       M' = M - 2^W  (quitar el bit extra)
       metodo = add, S = p - W
```

---

## 3. Diseno Propuesto

### Fase A: Generador Constexpr de Constantes Magicas

```cpp
namespace nstd::divmod {

/// Resultado del calculo de constantes magicas
template <typename UIntType>
struct magic_constants {
    UIntType multiplier;     // M (o M' para metodo "add")
    unsigned shift;          // S (post-shift)
    bool needs_add;          // true = metodo add-shift, false = multiply-shift
};

/// Calcula las constantes magicas en constexpr para divisor d
/// @tparam UIntType tipo sin signo del dividendo (uint64_t, uint128_t, etc.)
/// @param d divisor constante (d > 0, d != potencia de 2)
/// @return magic_constants con M, S y metodo
template <typename UIntType>
consteval magic_constants<UIntType> compute_magic(UIntType d) noexcept;

}  // namespace nstd::divmod
```

**Dependencias**: Necesita `mulhi()` constexpr para 128 bits.
Ya existe `mul_full()` en intrinsics o se calcula con los 4 productos de 64x64.

### Fase B: `constexpr_divisor<D>` — Divisor Tipado

```cpp
namespace nstd::divmod {

/// Envuelve un divisor constante con sus constantes magicas precalculadas
/// @tparam D el divisor (constexpr)
/// @tparam UIntType tipo del dividendo
template <auto D, typename UIntType = uint128_tc_t>
    requires (D > 0)
struct constexpr_divisor {
    static constexpr auto magic = compute_magic<UIntType>(UIntType{D});

    /// Division optimizada: n / D
    [[nodiscard]] static constexpr UIntType div(UIntType n) noexcept;

    /// Modulo optimizado: n % D
    [[nodiscard]] static constexpr UIntType mod(UIntType n) noexcept;

    /// Division + Modulo simultaneos
    [[nodiscard]] static constexpr std::pair<UIntType, UIntType>
        divmod(UIntType n) noexcept;

    /// Multiplicacion optimizada: n * D (via shifts+adds)
    [[nodiscard]] static constexpr UIntType mul(UIntType n) noexcept;
};

// Alias para uso directo
template <auto D>
using div_by = constexpr_divisor<D, uint128_tc_t>;

}  // namespace nstd::divmod
```

### Fase C: Operadores con Expresiones Marcadas

```cpp
namespace nstd {

/// Marca de que un valor es divisor constexpr conocido
template <auto D>
struct const_div_tag {
    static constexpr auto value = D;
};

// Uso:
//   uint128_tc_t x = ...;
//   auto q = x / const_div<10>{};   // Usa inverso multiplicativo
//   auto r = x % const_div<10>{};   // Usa inverso multiplicativo
//   auto [q, r] = divmod(x, const_div<10>{});

template <auto D>
inline constexpr const_div_tag<D> const_div{};

// Overloads de operadores
template <signedness S, representation_form F, auto D>
[[nodiscard]] constexpr int128_param_t<S, F>
operator/(const int128_param_t<S, F>& n, const_div_tag<D>) noexcept;

template <signedness S, representation_form F, auto D>
[[nodiscard]] constexpr int128_param_t<S, F>
operator%(const int128_param_t<S, F>& n, const_div_tag<D>) noexcept;

template <signedness S, representation_form F, auto D>
[[nodiscard]] constexpr std::pair<int128_param_t<S, F>, int128_param_t<S, F>>
divmod(const int128_param_t<S, F>& n, const_div_tag<D>) noexcept;

}  // namespace nstd
```

### Fase D: Extension a Signed (TC, MS, EK)

Para signed:

1. **TC**: `abs(n) / d`, luego ajustar signo del cociente y resto
2. **MS**: extraer magnitud, dividir, reconstruir con signo
3. **EK**: restar bias, operar en TC, re-biasar
4. **binnat**: identico a unsigned puro

---

## 4. Fases de Implementacion

### Fase A — `compute_magic<T>(d)` consteval (Base)

- [ ] Implementar `mulhi_128()` constexpr (producto alto 128x128 -> 128)
- [ ] Implementar algoritmo Granlund-Montgomery en consteval
- [ ] Tests: verificar constantes para d=3,5,7,...,31 contra las hardcodeadas del legacy
- [ ] Tests: verificar correctitud para todo uint64_t (subset aleatorio grande)
- **Fichero**: `include/int128_param_divmod.hpp`

### Fase B — `constexpr_divisor<D>` (Estructura Tipada)

- [ ] Implementar `div()`, `mod()`, `divmod()`, `mul()` para unsigned 128 bits
- [ ] Especializar potencias de 2: `div()` = shift, `mod()` = mask
- [ ] Tests: exhaustivos para divisores 2..31 con uint128
- [ ] Benchmarks: comparar vs `operator/` estandar de int128_param_t
- **Fichero**: mismo header, seccion nueva

### Fase C — Operadores con `const_div<D>` (Ergonomia)

- [ ] `operator/`, `operator%`, `divmod()` free functions
- [ ] operator/= y operator%= con const_div_tag
- [ ] Tests: ergonomia de uso y correctitud
- **Fichero**: mismo header + integracion en `int128_parameterized.hpp`

### Fase D — Soporte Signed (TC, MS, EK)

- [ ] TC: `sign = sign(n) ^ sign(d_virtual)`, dividir magnitudes
- [ ] MS: extraer mag, dividir, reconstruir sign bit
- [ ] EK: n_real = stored - bias, dividir, re-biasar
- [ ] Tests: divisores positivos y negativos, edge cases (min, max, 0)
- **Fichero**: extension en el mismo header

### Fase E — Optimizaciones Avanzadas (Futuro)

- [ ] Lookup table constexpr para divisores primos <= 256
- [ ] Factorizacion automatica: `div(n, 12)` -> `div(n, 4) then div(n, 3)`
- [ ] Division por potencias de primos: `div(n, 9)` = `div_3(div_3(n))`
- [ ] Integracion con `to_string()` (div/mod por 10 en bucle -> optimizado)

---

## 5. Integracion con `to_string()`

La conversion `int128 -> string decimal` es un hot path que hace:

```cpp
while (n != 0) {
    auto [q, r] = divmod(n, 10);
    buffer[i++] = '0' + r;
    n = q;
}
```

Con `constexpr_divisor<10>`, el `divmod(n, 10)` usara inverso multiplicativo
en vez de division hardware de 128 bits. Esto puede dar **2-5x speedup** en
`to_string()` si la division actual es software (que lo es para 128 bits).

---

## 6. Funcion `mulhi_128()` — Requisito Previo

```cpp
/// Parte alta de la multiplicacion 128x128 -> 256, tomando los 128 bits altos
constexpr uint128_tc_t mulhi_128(uint128_tc_t a, uint128_tc_t b) noexcept
{
    // Descomponer en 4 limbs de 64 bits:
    // a = a_hi * 2^64 + a_lo
    // b = b_hi * 2^64 + b_lo
    // a*b = (a_hi*b_hi)*2^128 + (a_hi*b_lo + a_lo*b_hi)*2^64 + a_lo*b_lo
    //
    // Necesitamos los 128 bits altos del resultado de 256 bits.
    // Esto se calcula con 4 multiplicaciones de 64x64->128.
}
```

Esto ya existe conceptualmente en `intrinsics/arithmetic_operations.hpp`
(la multiplicacion int128 x int128 ya funciona, solo falta exponer los
bits altos como funcion independiente).

---

## 7. Dependencias y Orden de Trabajo

```
mulhi_128() constexpr          [REQUISITO PREVIO]
    |
    v
compute_magic<T>(d) consteval  [FASE A]
    |
    v
constexpr_divisor<D>           [FASE B]
    |
    v
const_div<D> operators         [FASE C]
    |
    v
Signed support (TC/MS/EK)     [FASE D]
    |
    v
to_string() optimization       [FASE E - integracion]
```

---

## 8. Archivos a Crear/Modificar

| Archivo | Accion | Descripcion |
|---------|--------|-------------|
| `include/int128_param_divmod.hpp` | **CREAR** | Header principal: magic, constexpr_divisor, operators |
| `include/int128_parameterized.hpp` | Modificar | Incluir divmod + friend operators para const_div |
| `tests/int128_param_divmod_tests.cpp` | **CREAR** | Tests de correctitud |
| `benchs/int128_param_divmod_benchs.cpp` | **CREAR** | Benchmarks vs division estandar |
| `docs/API_divmod.md` | **CREAR** | Referencia API del modulo |

---

## 9. Metricas de Exito

| Metrica | Objetivo |
|---------|----------|
| Correctitud | 100% para todos los divisores 2..255 en rango completo uint128 (subset aleatorio > 10M) |
| Speedup div_128 | >= 2x vs division software actual para divisores constantes |
| Speedup to_string | >= 1.5x mejora en conversion a decimal |
| Constexpr | `compute_magic()` debe ser consteval, `div()/mod()` deben ser constexpr |
| Compiladores | GCC, Clang, MSVC, Intel — todos pasan tests |
| Zero overhead | Sin penalizacion cuando se usa division normal (no constexpr_divisor) |
