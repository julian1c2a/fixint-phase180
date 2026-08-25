# Instrucciones para GitHub Copilot

> **Este fichero es un puntero, no una fuente.**
>
> Hasta el 25 ago 2026 era una copia de `AI_PROMPT/ai-instructions.md`: **1.079
> de sus 1.101 líneas eran idénticas**. Dos ficheros de mil líneas que decían lo
> mismo y que había que actualizar a la vez, cosa que no pasaba.

Las reglas del proyecto viven ahora en un solo sitio cada una:

| Qué buscas | Dónde está |
|---|---|
| Guía de desarrollo completa | [`AI-GUIDE.md`](../AI-GUIDE.md) |
| Cómo se nombran las cosas | [`NAMING_CONVENTIONS.md`](../NAMING_CONVENTIONS.md) |
| Cómo se escribe el código | [`STYLE_CONVENTIONS.md`](../STYLE_CONVENTIONS.md) |
| Cómo se compila y se prueba | [`CONTRIBUTING.md`](../CONTRIBUTING.md) |
| Decisiones de diseño y su porqué | [`docs/decisions/`](../docs/decisions/README.md) |
| Estado actual | [`PROJECT_STATUS.md`](../PROJECT_STATUS.md) |
| Qué viene | [`ROADMAP.md`](../ROADMAP.md) y [`NEXT_STEPS.md`](../NEXT_STEPS.md) |

## Lo mínimo que hay que saber antes de tocar nada

1. **Compiladores.** En Windows, `g++` y `clang++` **a secas no son los del
   proyecto**: resuelven al toolchain MSYS. Usar `python scripts/toolchains.py`
   para ver cuál se va a usar de verdad.
2. **clang-format es la versión 21.** No es estable entre versiones mayores.
3. **Salida de consola en ASCII.** Sin acentos ni símbolos Unicode en lo que se
   imprime.
4. **`std::byte` para buffers de bytes**, no `unsigned char` ni `char`.
5. **El directorio raíz se mantiene limpio**: nada de `.exe`, `.obj`, `.pdb`.
6. **Antes de cerrar sesión**, los cuatro verificadores:
   `python make.py test gcc release-O2`,
   `python scripts/check_headers_selfcontained.py`,
   `python scripts/check_docs_consistency.py --doxygen`,
   y `clang-format-21 --dry-run --Werror` sobre lo tocado.

<!-- mermaid-ai-skills:start -->
## Mermaid Diagrams

When the user asks to create, edit, or visualize a diagram, follow the
instructions in `.github/instructions/mermaid.instructions.md`.
<!-- mermaid-ai-skills:end -->
