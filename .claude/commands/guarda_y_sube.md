---
description: Flujo de git seguro — verifica, revisa, commitea y sube
---

Ejecuta el comando `GUARDA_Y_SUBE` tal como está definido en la sección 31 de
[AI-GUIDE.md](../../AI-GUIDE.md).

Equivalente C++ del `guarda_y_sube` de las guías de Lean 4. Allí el flujo gira
en torno a los bloqueos de fichero (`git-lock.bash`); aquí no hay bloqueos, así
que su sitio lo ocupan los verificadores.

**Nada se sube sin pasar los cuatro verificadores.** Si alguno falla, se arregla
o se dice en el chat que no se sube; no se sube «con eso pendiente».

Pasos, en orden:

1. **Compilación y suite**

   ```
   python make.py test gcc release-O2
   ```

   Anota el resultado real. Si algo falla, para aquí.

2. **Verificadores**

   ```
   python scripts/check_headers_selfcontained.py
   python scripts/check_docs_consistency.py --doxygen
   clang-format --dry-run --Werror <ficheros tocados>
   ```

   Los tres en verde. Son exactamente los que exige el job `format-and-docs`
   del CI, así que pasarlos aquí evita descubrirlo en GitHub.

3. **Revisar qué se va a subir**

   ```
   git status --short
   git diff --stat
   ```

   **Nunca `git add -A` a ciegas.** Se añaden ficheros concretos, y solo tras
   mirar la lista. Si aparece algo inesperado (binarios, ficheros temporales,
   `.orig`, `.bak`), se investiga antes de seguir.

4. **Commit**

   Mensaje descriptivo de verdad, en el estilo del historial del proyecto: qué
   cambia, **por qué**, y qué se verificó. Conventional commits para el prefijo
   (`feat:`, `fix:`, `perf:`, `docs:`, `test:`, `build:`, `ci:`, `refactor:`).
   Un `!` tras el tipo si rompe compatibilidad, y el detalle en el cuerpo.

   Si el trabajo son varias cosas separables, **varios commits**, cada uno
   compilable y con la suite en verde, para que `git bisect` sirva de algo.

5. **Subir**

   ```
   git push origin <rama>
   ```

   Confirma con `git log --oneline origin/<rama> -1` que ha llegado.

6. **Reportar en el chat**

   Qué commits han entrado, qué se verificó y con qué resultado, y qué queda
   pendiente.

Si el usuario ha pasado argumentos en $ARGUMENTS, úsalos como pista del mensaje
de commit o de qué ficheros incluir.
