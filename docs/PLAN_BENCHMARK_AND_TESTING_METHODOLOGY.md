# Plan: Benchmark & Testing Methodology

**Status:** Planned  
**Created:** 26 June 2026  
**Priority:** High — Affects all future test/benchmark development  
**Scope:** Mandatory methodology for all tests and benchmarks in the int128 library

---

## 1. Benchmark Measurement: CPU Cycles (RDTSC)

### Motivación

Los benchmarks deben medir **ciclos de CPU directamente** mediante `RDTSC`, no tiempo
dividido por frecuencia del reloj. Las razones son:

1. **Determinismo:** Los ciclos de CPU son una medida directa del trabajo del procesador,
   independiente de frecuencia dinámica (turbo boost, throttling, power saving).
2. **Precisión:** Elimina la capa de abstracción de `std::chrono` que introduce overhead
   de conversión y resolución limitada.
3. **Reproducibilidad:** Los resultados en ciclos/op son comparables entre ejecuciones,
   incluso si la frecuencia del procesador varía.
4. **Estándar de la industria:** Google Benchmark, nanobench, y otros frameworks
   profesionales reportan ciclos como métrica primaria.

### Implementación de Referencia

La infraestructura RDTSC ya existe en `benchs/benchmark_vs_builtin.cpp`:

```cpp
// Función rdtsc() multiplataforma
inline uint64_t rdtsc() {
#if defined(_MSC_VER)
    return __rdtsc();                    // MSVC: <intrin.h>
#elif defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER)
    return __rdtsc();                    // Intel: <x86intrin.h>
#else
    return __builtin_ia32_rdtsc();       // GCC/Clang
#endif
}

// Clase CycleTimer
class CycleTimer {
    uint64_t start_cycles;
public:
    void start() { start_cycles = rdtsc(); }
    uint64_t elapsed_cycles() const { return rdtsc() - start_cycles; }
};

// doNotOptimize<T>() — evita que el compilador elimine cálculos
template <typename T>
void doNotOptimize(const T& value);
```

### Protocolo de Medición

1. **Warmup:** 10,000 iteraciones de calentamiento (estabiliza caché, branch predictor).
2. **Medición:** 5,000,000 iteraciones (configurable vía `BENCH_ITERATIONS`).
3. **Cálculo:** `cycles_per_op = total_cycles / BENCH_ITERATIONS`.
4. **AntiOptimize:** Toda operación medida DEBE pasar por `doNotOptimize()`.
5. **Baseline:** Siempre medir `uint64_t` como referencia, reportar ratio `nstd/u64`.

### Migración Pendiente

**`benchmark_divmod_algorithms.cpp`** actualmente usa `std::chrono::high_resolution_clock`.
Debe migrarse a RDTSC siguiendo el patrón de `benchmark_vs_builtin.cpp`.

---

## 2. Test & Benchmark Coverage Methodology

### Principio Fundamental

Para tipos de 128 bits, es imposible probar el espacio completo de 2^128 valores.
La estrategia es cubrir **tres regiones críticas** de forma sistemática:

### 2.1. Funciones de Un Argumento: f(x)

Para toda función `f(x)` que recibe un valor de 128 bits, se DEBEN probar:

| Región | Rango | Cantidad | Justificación |
|--------|-------|----------|---------------|
| **Primeros** | `[0, 2^21 - 1]` | 2,097,152 | Valores pequeños, casos base, interacción con limbs bajos |
| **Últimos** | `[MAX - 2^21 + 1, MAX]` | 2,097,152 | Overflow proximity, all-ones patterns, wrap-around |
| **Aleatorios** | Distribución uniforme en `(2^21, MAX - 2^21)` | 2,097,152 | Cobertura del rango medio, ambos limbs activos |
| **Edge cases** | Valores conocidos críticos | Variable | Potencias de 2, patrones de bits específicos, MAX/2, etc. |

**Total mínimo:** ~6.3M valores + edge cases por función.

**Edge cases obligatorios:**

- `0`, `1`, `MAX`, `MAX - 1`
- `2^64 - 1` (max del limb bajo), `2^64` (primer valor que usa limb alto)
- `2^63` (bit de signo en signed), `2^127` (MSB)
- Potencias de 2: `2^k` para k ∈ {0, 1, 7, 8, 15, 16, 31, 32, 63, 64, 127}
- Patrones repetitivos: `0x5555...5555`, `0xAAAA...AAAA`, `0xFF00FF00...`

### 2.2. Funciones de Dos Argumentos: g(x, y)

Para funciones `g(x, y)` de dos argumentos de 128 bits, se necesitan las **6 combinaciones**
de las 3 regiones (combinaciones con repetición de 3 tomadas de 2):

| # | Región x | Región y | Escenario |
|---|----------|----------|-----------|
| 1 | Primeros | Primeros | Ambos operandos pequeños |
| 2 | Primeros | Últimos  | Operando pequeño + grande |
| 3 | Primeros | Aleatorio | Operando pequeño + medio |
| 4 | Últimos  | Últimos  | Ambos operandos cerca de MAX |
| 5 | Últimos  | Aleatorio | Operando grande + medio |
| 6 | Aleatorio | Aleatorio | Ambos en rango intermedio |

**Total mínimo:** 6 × 2^21 ≈ 12.6M pares + edge cases combinatorios.

**Edge cases combinatorios adicionales:**

- `(0, 0)`, `(0, MAX)`, `(MAX, MAX)`, `(1, MAX)`, `(MAX, 1)`
- `(2^64, 2^64)` — ambos en frontera de limbs
- `(x, 0)` y `(0, y)` — elementos neutros/absorbentes
- `(x, x)` — operandos iguales (diagonal)

### 2.3. Funciones con Argumento de Shift (casos especiales)

Para operaciones de shift `f(x, k)` donde `k ∈ [0, 127]`:

- Probar x con las 3 regiones
- Probar k con **todos los valores** 0-127 (solo 128 valores)
- Total: 3 × 2^21 × 128 si se quiere exhaustividad, o 3 × 2^21 con k muestreado

### 2.4. Implementación Práctica

```cpp
// Generador de regiones de test
struct TestRegion {
    static constexpr uint64_t REGION_SIZE{1ULL << 21};  // 2^21 = 2,097,152

    // Región 1: primeros 2^21 valores
    static uint128_t first_region(uint64_t index) {
        return uint128_t{index};
    }

    // Región 2: últimos 2^21 valores
    static uint128_t last_region(uint64_t index) {
        return uint128_t::max() - uint128_t{REGION_SIZE - 1 - index};
    }

    // Región 3: aleatorios intermedios (PRNG determinista para reproducibilidad)
    static uint128_t random_region(uint64_t index, uint64_t seed);
};
```

**PRNG determinista:** Usar un generador con semilla fija para que los tests sean
reproducibles. Recomendado: `xoshiro256**` o `splitmix64` para generar ambos limbs.

---

## 3. Requisitos para Nuevos Benchmarks

### Estructura Obligatoria

Todo nuevo archivo de benchmark DEBE seguir esta estructura:

```
benchs/[type]_[feature]_extracted_benchs.cpp
```

1. Incluir infraestructura RDTSC (CycleTimer, doNotOptimize, rdtsc).
2. Medir en **ciclos/operación**, no nanosegundos.
3. Incluir baseline `uint64_t` para comparación.
4. Incluir `__int128` y Boost como puntos de referencia (cuando aplique).
5. Reportar ratio `nstd/baseline` para cada operación.
6. Warmup de 10K iteraciones mínimo.
7. Medición de 5M iteraciones mínimo.

### Estructura Obligatoria para Tests

Todo nuevo archivo de test DEBE cubrir:

1. Las 3 regiones (primeros, últimos, aleatorios) con 2^21 valores cada una.
2. Edge cases documentados arriba.
3. Para 2-arg: las 6 combinaciones de regiones.
4. Assertions exactas (no `!= 0`, sino valor esperado concreto).

---

## 4. Tabla de Prioridades de Migración

| Archivo | Estado Actual | Acción Requerida |
|---------|--------------|------------------|
| `benchmark_vs_builtin.cpp` | ✅ RDTSC | Añadir cobertura 3-regiones |
| `benchmark_divmod_algorithms.cpp` | ❌ chrono | Migrar a RDTSC + 6 combinaciones |
| Tests existentes | ⚠️ Variable | Auditar y ampliar a 3 regiones + edge cases |
| Nuevos benchmarks (futuro) | — | Crear con RDTSC desde el inicio |

---

## 5. Métricas de Reporte

Cada benchmark debe producir una tabla como:

```
+---------------------------+-----------+-------+-----------+
| Operation                 | cycles/op | ratio | status    |
+---------------------------+-----------+-------+-----------+
| uint64_t baseline         |      2.34 |  1.00 | reference |
| nstd::uint128_t           |      4.68 |  2.00 |    [OK]   |
| __int128 (builtin)        |      5.10 |  2.18 |    [OK]   |
| boost::uint128_t          |     12.30 |  5.26 |    [OK]   |
+---------------------------+-----------+-------+-----------+
```

---

**Documentación relacionada:**

- `benchs/benchmark_vs_builtin.cpp` — Referencia de RDTSC
- `AI_PROMPT/ai-instructions.md` — Regla 15 (Test & Benchmark Methodology)
- `NEXT_STEPS.md` — Sección de trabajo futuro
