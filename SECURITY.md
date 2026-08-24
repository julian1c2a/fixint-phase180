# Política de seguridad

## Qué es esta biblioteca, y qué no

`int128` / `fixed_int_t` es una biblioteca de **aritmética de enteros de
precisión extendida**. No es una biblioteca criptográfica.

En particular, **no ofrece garantías de tiempo constante**. Las rutas de
ejecución dependen de los datos por diseño y por rendimiento:

- La división tiene caminos rápidos que se eligen según cuántos limbos
  significativos tenga el divisor.
- El paso D3 del algoritmo D de Knuth refina el cociente en un bucle cuyo número
  de vueltas depende de los operandos.
- La multiplicación usa Karatsuba a partir de N=4, con recursión.
- `to_string` itera tantas veces como dígitos tenga el número.

**No la uses para operar con material criptográfico secreto** (claves privadas,
nonces, exponentes de RSA o de curvas elípticas) si tu modelo de amenaza incluye
un atacante capaz de medir tiempos. Para eso hacen falta primitivas diseñadas en
tiempo constante, y esta no lo es.

Si en algún momento se añade un subconjunto en tiempo constante, se documentará
de forma explícita aquí. Mientras este párrafo siga en pie, asume que no existe.

## Qué sí se considera un problema de seguridad

Dentro del alcance de esta biblioteca:

- **Comportamiento indefinido** alcanzable desde la API pública con entradas
  válidas: lectura o escritura fuera de rango, desbordamiento de entero con
  signo, desplazamiento fuera de rango, uso de valores no inicializados.
- **Corrupción de memoria** en cualquier operación.
- **Resultados incorrectos en silencio** que puedan usarse para saltarse una
  comprobación aguas arriba. El caso típico: un desbordamiento que no se detecta
  y convierte un valor grande en uno pequeño.
- **Bucle infinito o agotamiento de memoria** provocable con una entrada
  concreta, en particular al parsear cadenas.

Fuera de alcance:

- Ataques por canal lateral de tiempo o de caché (ver arriba).
- Desbordamiento **modular** en las operaciones aritméticas: es el
  comportamiento documentado del tipo, igual que en los enteros built-in de C++.
  Si necesitas detección, ahí están `checked_add` / `checked_sub` /
  `checked_mul` y `try_from_string`.
- Fallos que solo aparecen tras violar una precondición documentada, como
  llamar a `limb(i)` con `i >= N`.

## Cómo informar

Manda un correo a **julian.calderon.almendros@gmail.com** con el asunto
empezando por `[SECURITY]`.

Incluye:

- versión o commit afectado;
- compilador, versión y plataforma;
- un programa mínimo que lo reproduzca;
- qué impacto crees que tiene.

**No abras un issue público** para un fallo que creas explotable. Para todo lo
demás —incluidos los resultados incorrectos que no ves cómo aprovechar— un issue
normal está bien y es más rápido.

Este es un proyecto de un solo autor, sin equipo de seguridad detrás: no hay un
compromiso de tiempo de respuesta. Se contesta en cuanto se puede.

## Versiones con soporte

Se corrige sobre la rama de desarrollo en curso. No hay ramas de mantenimiento
de versiones anteriores.

| Versión | Soporte |
|---|---|
| rama de desarrollo actual (`phase-1.80`) | ✅ |
| versiones publicadas anteriores | ❌ actualiza |

## Qué se hace para prevenir

Para que sepas qué red hay puesta, y dónde están sus agujeros:

- **CI multiplataforma**: GCC 13–16, Clang 18–22, MSVC, Intel ICX; x86-64,
  x86-32, ARM64, ARM32 y RISC-V 64 (estos por QEMU).
- **ASan + UBSan** sobre la suite entera en cada push.
- **cppcheck** y **clang-tidy** en cada push, hoy en modo informativo.
- **Fuzz diferencial** contra un oráculo independiente que no comparte código
  con la implementación optimizada: unas 47.000 comprobaciones por ejecución
  sobre `+ - * / % & | ^ << >>` y el round-trip de cadena.
- Todo header compila aislado y **sin flags no estándar**.

Lo que **no** hay: fuzzing continuo tipo OSS-Fuzz, ni análisis formal, ni
auditoría externa.
