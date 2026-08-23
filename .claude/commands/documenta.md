---
description: Genera la documentación Doxygen y exige cero avisos desde include/
---

Ejecuta el comando `DOCUMENTA` tal como está definido en la sección 31 de
[AI-GUIDE.md](../../AI-GUIDE.md).

**Criterio duro: cero avisos de Doxygen atribuibles a `include/`.**

Los avisos que no son culpa del código están en la lista blanca
`DOXYGEN_ALLOWED` de `scripts/check_docs_consistency.py`, cada uno con su motivo
escrito. Esa lista **no crece sin justificación**: si aparece un aviso nuevo, se
arregla, no se añade a la lista. Si de verdad no es arreglable, se añade con su
motivo explicado y se dice en el chat.

Pasos, en orden:

1. `mkdir -p documentation/generated documentation/doxygen/pages` (Doxygen falla
   si el directorio de salida no existe).
2. `doxygen Doxyfile` y cuenta los avisos: totales, los que vienen de
   `include/`, y los que están en la lista blanca.
3. Arregla los que vengan de `include/`. Los errores típicos que ya ha destapado
   este comando:
   - `@example` para introducir un ejemplo en línea. En Doxygen ese comando
     significa «este comentario documenta un fichero de ejemplo», así que hace
     que el header entero se trate como ejemplo y avisa de documentación
     duplicada. Lo correcto es `@par Ejemplo:`.
   - Cabeceras estándar como `<atomic>` dentro de un comentario: Doxygen las lee
     como etiquetas HTML. Van entre acentos graves.
   - `@param` con un nombre que no está en la firma, casi siempre porque el
     parámetro está sin nombrar en la declaración. Nombrar el parámetro suele ser
     mejor arreglo que borrar el `@param`.
   - Comandos inventados (`@complexity`): se declaran en `ALIASES` del Doxyfile.
   - Ángulos sueltos en markdown (`divmod<D>`): entre acentos graves o dentro de
     un bloque de código.
4. Mide la cobertura de comentarios Doxygen de los headers tocados
   (`grep -c '///\|/\*\*'` frente a las líneas del fichero) y anótala.
5. Regenera los `docs/API_*.md` que falten.
6. `python scripts/check_docs_consistency.py --doxygen` — en verde.

Al terminar, reporta en el chat: avisos antes y después, cobertura de los
headers tocados, y si has añadido algo a la lista blanca, cuál y por qué.
