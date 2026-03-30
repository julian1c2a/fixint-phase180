# Plan de Decisión y Futuro del Proyecto

**Última actualización:** 30 de marzo de 2026
**Estado:** Hoja de ruta estratégica definida post-v1.75, consolidando las decisiones clave para las próximas fases.

## I. Filosofía de Versionado hacia la v2.0

Se ha propuesto una reflexión sobre el esquema de versionado que guiará el progreso hasta la versión 2.0. Más que un cálculo estricto, representa una filosofía de avance incremental basado en la serie armónica y factoriales, simbolizando que cada paso, aunque más pequeño, añade valor fundamental.

- **v1.50:** `1 + 1/2` (Base funcional)
- **v1.66:** `1 + 1/2 + 1/6` (Refinamiento inicial)
- **v1.75 (Actual):** Debería interpretarse conceptualmente como `~1 + 1/2 + 1/5`.
- **Próximas fases conceptuales:**
    - `~1.75`: `1 + 1/2 + 1/4`
    - `~1.83`: `1 + 1/2 + 1/3`
    - Y así sucesivamente, con incrementos cada vez más especializados (`... + 1/4!`, `... + 1/5!`, etc.) hasta la consolidación en la v2.0.

Esta guía filosófica se mapeará a un esquema de versionado pragmático (v1.80, v1.90, etc.) para la planificación concreta de la hoja de ruta.

## II. Decisiones Estratégicas y Hoja de Ruta

Esta es la secuencia de desarrollo consolidada. Las prioridades se han establecido para maximizar la robustez y el potencial a largo plazo de la biblioteca antes de añadir funcionalidades de alto nivel.

### **Prioridad 1: Infraestructura y Portabilidad (Decisión 5.6)**

-   **Acción:** **Soporte prioritario para arquitecturas ARM y RISC-V.** Esta es la tarea más crítica y precede a todas las demás extensiones de características. La robustez y la amplitud de soporte son primordiales.
-   **Implica:**
    -   Adaptar los módulos de intrínsecos de hardware.
    -   Configurar la Integración Continua (CI) para compilar y testear en estas plataformas.
    -   Documentar cualquier especificidad o limitación.

### **Prioridad 2: Migración y Fortalecimiento de Tests (Decisión 3)**

-   **Acción:** **Mejorar y migrar los tests al nuevo prototipo más robusto.** Esto es de vital importancia para asegurar la fiabilidad a medida que la base de código se expande.
-   **Implica:**
    -   Planificar y ejecutar la migración sistemática de los tests existentes al nuevo framework de pruebas.
    -   Establecer objetivos claros de cobertura para cada ciclo de versión.

### **Prioridad 3: Tipo de Entero de Longitud Fija (Decisión 5.7)**

-   **Acción:** Implementar un tipo `int_fixed_t` parametrizado por el número de `uint64_t` (limbs). Esta generalización debe realizarse **antes** de BCD y punto flotante para resolver el problema de la "torre infinita de tipos".
-   **Implica:**
    -   Refactorizar los algoritmos existentes para operar sobre una estructura de datos de longitud variable.
    -   El tipo `int128_t` actual se convertirá en una especialización (`int_fixed_t<2>`).
    -   El diseño se detallará en un **Architecture Decision Record (ADR)**, pero el principio clave es la distinción entre tipos "completos" y "auxiliares". Un `int_fixed_t<N>` tendrá un conjunto completo de funcionalidades. Sin embargo, para operaciones que requieran un espacio intermedio mayor (p. ej., una multiplicación de `N x N` que produce `2N`), se usará un `int_fixed_t<2N>` auxiliar con un **conjunto de funcionalidades truncado**, limitado solo a las operaciones necesarias para el algoritmo (sumas, movimientos de datos, etc.). Esta estrategia evita tener que implementar todas las características para todas las posibles longitudes, resolviendo el problema de la complejidad exponencial.

### **Prioridad 4: Tipos de Punto Fijo (Decisiones 5.2, 5.3)**

-   **Acción:** Desarrollar la aritmética de punto fijo sobre la nueva infraestructura `int_fixed_t`.
-   **Implica:**
    -   **4.1 (Binario):** Implementar punto fijo para representación sin signo y en complemento a 2.
    -   **4.2 (Decimal):** Implementar punto fijo para BCD natural (unsigned) y BCD Aiken (signed, TC), lo que requerirá una base de BCD (ver Prioridad 5).

### **Prioridad 5: Tipos Decimales BCD Completos (Decisión 5.1)**

-   **Acción:** Extender la biblioteca con tipos BCD que sean paralelos a los tipos binarios existentes, ofreciendo una aritmética decimal precisa.
-   **Implica:**
    -   Desarrollar la representación y la aritmética completa para BCD.

### **Prioridad 6: Aritmética de Punto Flotante (Decisiones 5.4, 5.5)**

-   **Acción:** Implementar tipos de punto flotante de alta precisión.
-   **Implica:**
    -   **6.1 (Binario):** Un tipo de punto flotante generalizado y eficiente, siguiendo el estándar IEEE 754.
    -   **6.2 (Decimal):** Un tipo de punto flotante en base 10, conforme a las especificaciones del estándar IEEE 754 para `decimal64/decimal128`.

## III. Funcionalidades Posdatadas a v1.80 o Posterior

Las siguientes decisiones han sido explícitamente pospuestas para después del ciclo de desarrollo inmediato, que se centrará en la infraestructura y la generalización.

-   **Decisión 2:** Funcionalidad no especificada, pospuesta.
-   **Decisión 4:** Funcionalidad no especificada, pospuesta.
-   **Empaquetado y Distribución:** La creación de recetas para Conan/vcpkg se abordará una vez que la base de la biblioteca sea más estable y general.

## IV. Puntos Resueltos y Próximos Pasos

Esta sección documenta las decisiones tomadas sobre las ambigüedades anteriores.

1.  **Decisión 1 (Confirmada):** Se confirma que la interpretación es correcta. La tarea inmediata es la unificación de `to_string` con `divmod_const` y la optimización del módulo GM.

2.  **Conflicto de Nomenclatura (Resuelto):** La numeración de la Decisión 5 ha sido corregida.

3.  **Mapeo de Versionado (Confirmado):** Se confirma la sugerencia de mapeo pragmático:
    -   **v1.80:** Completar Prioridad 1 (Infraestructura ARM/RISC-V) y Prioridad 2 (Tests).
    -   **v1.90:** Completar Prioridad 3 (Tipo `int_fixed_t`).
    -   **v1.9x en adelante:** Abordar las prioridades 4, 5 y 6.

4.  **Diseño de `int_fixed_t` (Principios Definidos):** Se ha establecido el principio de diseño clave para el ADR: el tipo se llamará `int_fixed_t<N>` y la "torre infinita de tipos" se evitará mediante el uso de tipos `int_fixed_t<2N>` auxiliares con funcionalidades muy limitadas, en contraposición a los tipos `int_fixed_t<N>` completos. El ADR deberá detallar esta estrategia.
