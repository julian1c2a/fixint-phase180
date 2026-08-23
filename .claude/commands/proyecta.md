---
description: Proyecta la API pública de los headers tocados a sus docs/API_*.md
---

Ejecuta el comando `PROYECTA` tal como está definido en la sección 31 de
[AI-GUIDE.md](../../AI-GUIDE.md).

Actualización **local** de la documentación de API: solo los headers que se han
tocado en esta sesión, y solo sus `docs/API_*.md`. No toques README, CHANGELOG
ni NEXT_STEPS — eso es trabajo de `/actualiza_doc`.

Pasos, en orden:

1. `git diff --name-only HEAD` y `git status --short` para saber qué `.hpp` de
   `include/` se han tocado en la sesión. Si el usuario ha nombrado headers
   concretos en $ARGUMENTS, usa esos en su lugar.

2. Por cada header, extrae su **API pública**: clases, funciones libres, aliases,
   traits y concepts que no estén en `detail::`, en una sección `private:`, ni
   lleven sufijo `_`.

3. Comprueba que cada símbolo público tiene su comentario Doxygen con `@brief` y,
   donde apliquen, `@param` y `@return`. Añade los que falten, en el estilo del
   header (mira `int128_parameterized.hpp` como referencia: es el que mejor
   documentado está).

4. Actualiza el `docs/API_*.md` correspondiente siguiendo la plantilla de la
   sección 7 de AI-GUIDE.md: sinopsis, tabla de funciones con firma, semántica y
   complejidad, y al menos un ejemplo que compile.

5. Verifica en las dos direcciones:
   - que **no se ha filtrado nada privado** (nada de `detail::`, ningún miembro
     privado, ningún helper con sufijo `_`);
   - que **nada público se ha quedado sin proyectar**.

6. `python scripts/check_docs_consistency.py` — tiene que quedar en verde.

Al terminar, resume en el chat: headers proyectados, símbolos nuevos
documentados, y cualquier símbolo público que hayas decidido no documentar y por
qué.
