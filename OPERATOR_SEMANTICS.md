# Operator Semantics by Representation Form

## Overview

Este documento clarifica el comportamiento **semántico vs sintáctico** de los operadores aritméticos para cada forma de representación en `int128_param_t`.

---

## Definiciones

### Comportamiento SEMÁNTICO

El operador produce el **resultado matemático correcto** independientemente de cómo se almacenan los valores internamente.

- **Ejemplo:** `int128_ek_t{10} + int128_ek_t{20} == int128_ek_t{30}` ✅ (correcto matemáticamente)

### Comportamiento SINTÁCTICO

El operador opera sobre los **valores almacenados** sin considerar la semántica de la representación.

- **Ejemplo:** `int128_ek_t{10} * int128_ek_t{20}` produce `(10+K) * (20+K)` ≠ `200 + K` ❌ (incorrecto matemáticamente)

---

## Tabla de Comportamiento por Operador y Representación

| Operador | Two's Complement (TC) | Magnitude-Sign (MS) | Excess-K (EK) | Notas |
|----------|----------------------|---------------------|---------------|-------|
| **`++`** | ✅ SEMÁNTICO | ✅ SEMÁNTICO | ✅ SEMÁNTICO | EK: incremento binario simple `(x+K)+1 = (x+1)+K` |
| **`--`** | ✅ SEMÁNTICO | ✅ SEMÁNTICO | ✅ SEMÁNTICO | EK: decremento binario simple `(x+K)-1 = (x-1)+K` |
| **`+=`** | ✅ SEMÁNTICO | ✅ SEMÁNTICO | ✅ SEMÁNTICO | EK: `(x+K) + (y+K) = (x+y) + 2K` → resta K |
| **`-=`** | ✅ SEMÁNTICO | ✅ SEMÁNTICO | ✅ SEMÁNTICO | EK: `(x+K) - (y+K) = (x-y)` → suma K |
| **`*=`** | ✅ SEMÁNTICO | ✅ SEMÁNTICO | ❌ **SINTÁCTICO** | EK: `(x+K) * (y+K) = xy + K(x+y) + K²` ≠ `xy + K` |
| **`/=`** | ✅ SEMÁNTICO | ✅ SEMÁNTICO | ❌ **SINTÁCTICO** | Similar a multiplicación, bias contamina resultado |
| **`%=`** | ✅ SEMÁNTICO | ✅ SEMÁNTICO | ❌ **SINTÁCTICO** | Similar a división |
| **`<<`** | ✅ SEMÁNTICO | ⚠️ ESPECIAL | ⚠️ SINTÁCTICO | MS preserva bit de signo, EK desplaza valor con bias |
| **`>>`** | ✅ SEMÁNTICO | ⚠️ ESPECIAL | ⚠️ SINTÁCTICO | MS: aritmético en magnitud, EK: desplaza valor con bias |
| **`&`** | ✅ SEMÁNTICO | ⚠️ SINTÁCTICO | ⚠️ SINTÁCTICO | MS/EK: opera sobre bits almacenados |
| **`|`** | ✅ SEMÁNTICO | ⚠️ SINTÁCTICO | ⚠️ SINTÁCTICO | MS/EK: opera sobre bits almacenados |
| **`^`** | ✅ SEMÁNTICO | ⚠️ SINTÁCTICO | ⚠️ SINTÁCTICO | MS/EK: opera sobre bits almacenados |
| **`~`** | ✅ SEMÁNTICO | ⚠️ SINTÁCTICO | ⚠️ SINTÁCTICO | MS/EK: opera sobre bits almacenados |
| **`==`** | ✅ SEMÁNTICO | ✅ SEMÁNTICO | ✅ SEMÁNTICO | Todas las formas comparan correctamente |
| **`<`** | ✅ SEMÁNTICO | ✅ SEMÁNTICO | ✅ SEMÁNTICO | EK: orden de valores almacenados = orden real |

---

## Detalles por Representación

### Two's Complement (TC)

**Almacenamiento:** Valor real en complemento a 2  
**Características:**

- ✅ Todos los operadores son **SEMÁNTICOS**
- Aritmética binaria estándar funciona directamente
- Hardware optimizado (nativo en CPU)

**Ejemplos:**

```cpp
int128_tc_t x{10};
int128_tc_t y{20};
auto sum = x + y;      // ✅ sum == 30 (SEMÁNTICO)
auto product = x * y;  // ✅ product == 200 (SEMÁNTICO)
```

---

### Magnitude-Sign (MS)

**Almacenamiento:** `data[1]` MSB = bit de signo, resto = magnitud  
**Características:**

- ✅ Aritmética (`++`, `--`, `+=`, `-=`, `*=`) es **SEMÁNTICA**
- ⚠️ Operadores bit a bit son **SINTÁCTICOS** (operan sobre magnitud + signo por separado)
- ⚠️ Desplazamientos preservan bit de signo pero desplazan magnitud

**Ejemplos:**

```cpp
int128_ms_t x{-10};
int128_ms_t y{20};
auto sum = x + y;       // ✅ sum == 10 (SEMÁNTICO)
auto product = x * y;   // ✅ product == -200 (SEMÁNTICO)
auto shifted = x << 1;  // ⚠️ Desplaza magnitud, preserva signo (comportamiento MS-específico)
```

---

### Excess-K (EK)

**Almacenamiento:** `stored_value = real_value + K` donde `K = 2^126`  
**Características:**

- ✅ `++`, `--` son **SEMÁNTICOS** (incremento/decremento binario simple)
- ✅ `+=`, `-=` son **SEMÁNTICOS** (con compensación de bias para suma de dos valores EK)
- ❌ `*=`, `/=`, `%=` son **SINTÁCTICOS** (bias contamina resultado)
- ⚠️ Para multiplicación/división semántica: **convertir a TC, operar, reconvertir**

**Matemática del Bias:**

```
Incremento: (x+K) + 1 = (x+1) + K                  → no requiere compensación ✅
Decremento: (x+K) - 1 = (x-1) + K                  → no requiere compensación ✅
Suma:       (x+K) + (y+K) = (x+y) + 2K  → restar K → (x+y) + K               ✅
Resta:      (x+K) - (y+K) = (x-y)       → sumar K  → (x-y) + K               ✅
Mult:       (x+K) * (y+K) = xy + K(x+y) + K²       ≠ xy + K                  ❌
División:   (x+K) / (y+K) ≠ (x/y) + K                                         ❌
```

**Nota clave:** El incremento/decremento no requiere compensación de bias porque sumamos/restamos una **constante** al valor real, no sumamos dos valores con bias.

**Ejemplos:**

```cpp
int128_ek_t x{10};   // stored: 10 + 2^126
int128_ek_t y{20};   // stored: 20 + 2^126

// ✅ SEMÁNTICOS (incremento binario simple)
++x;                 // ✅ x == 11: stored (10+K)+1 = (11+K), no compensation needed
--y;                 // ✅ y == 19: stored (20+K)-1 = (19+K), no compensation needed

// ✅ SEMÁNTICOS (con compensación de bias para suma de dos EK)
auto sum = x + y;    // ✅ sum == 30: (10+K) + (20+K) = 30+2K → subtract K → 30+K

// ❌ SINTÁCTICOS (operan sobre valores con bias)
auto product = x * y;  // ❌ product ≠ 200 (INCORRECTO)
                       // Resultado: (10+K) * (20+K) = 200 + K(10+20) + K²

// ✅ Multiplicación SEMÁNTICA (requiere conversión):
int128_tc_t x_tc = convert_to_tc(x);
int128_tc_t y_tc = convert_to_tc(y);
int128_tc_t product_tc = x_tc * y_tc;
int128_ek_t product_ek = convert_to_ek(product_tc);  // ✅ product_ek == 200
```

---

## Recomendaciones de Uso

### Para Two's Complement (TC)

✅ **Recomendado para:** Todo tipo de operaciones  
✅ **Ventajas:** Todos los operadores semánticos, hardware optimizado  
❌ **Limitaciones:** Ninguna significativa

### Para Magnitude-Sign (MS)

✅ **Recomendado para:** Algoritmos que requieren fácil acceso a magnitud y signo por separado  
✅ **Ventajas:** Aritmética semántica, distinción ±0, extracción directa de magnitud  
⚠️ **Cuidado con:** Operadores bit a bit (no preservan semántica MS)

### Para Excess-K (EK)

✅ **Recomendado para:** Comparaciones y sumas/restas frecuentes  
✅ **Ventajas:** Comparaciones muy eficientes (orden de almacenamiento = orden real)  
❌ **NO usar para:** Multiplicación, división, operadores bit a bit  
⚠️ **Alternativa:** Convertir a TC para operaciones complejas

---

## Matriz de Decisión Rápida

| Necesitas... | Usa TC | Usa MS | Usa EK |
|-------------|--------|--------|--------|
| Aritmética general | ✅ | ✅ | ⚠️ |
| Multiplicación/división | ✅ | ✅ | ❌ |
| Comparaciones frecuentes | ✅ | ✅ | ✅✅ |
| Manipulación de bits | ✅ | ❌ | ❌ |
| Acceso a magnitud/signo por separado | ⚠️ | ✅✅ | ❌ |
| Hardware optimizado | ✅✅ | ❌ | ❌ |
| Distinción ±0 | ❌ | ✅ | ❌ |

---

## Conversión Entre Representaciones

Para operaciones no semánticas en MS o EK, **convierte a TC:**

```cpp
// Multiplicación semántica en EK
int128_ek_t x{10}, y{20};

// ❌ INCORRECTO:
auto wrong = x * y;  // Sintáctico, resultado incorrecto

// ✅ CORRECTO:
int128_tc_t x_tc{x.to_tc()};        // Conversión a TC
int128_tc_t y_tc{y.to_tc()};
int128_tc_t product_tc = x_tc * y_tc;
int128_ek_t product = int128_ek_t::from_tc(product_tc);  // ✅ product == 200
```

---

## Referencias

- **Código fuente:** `include/int128_parameterized.hpp`
- **Tests de aritmética nativa:** `tests/test_native_arithmetic.cpp` (47 tests)
- **Tests de cambios de representación:** `tests/test_priority3_representations_ms_ek.cpp`

---

**Última actualización:** 3 February 2026  
**Autor:** Phase 1.75 Development Team
