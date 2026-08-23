---
description: Pasada completa de documentación al cerrar una sesión de trabajo
---

Ejecuta el comando `ACTUALIZA_DOC` tal como está definido en la sección 31 de
[AI-GUIDE.md](../../AI-GUIDE.md).

Sincroniza los documentos vivos con el estado **real** del código. La regla que
manda aquí: se documenta lo que la suite dice, no lo que se esperaba que dijera.

Pasos, en orden:

1. `python make.py test gcc release-O2`. Anota el resultado real. Si algo falla,
   dilo en el chat y **no** escribas en la documentación que todo pasa.

2. Lee el estado previo de `NEXT_STEPS.md`, `CHANGELOG.md` y
   `PROJECT_STATUS.md`. Mira también `git log --oneline` desde el último commit
   de documentación, para saber qué ha entrado.

3. Identifica qué ha cambiado: tareas cerradas, ficheros nuevos, APIs añadidas,
   comportamientos modificados, decisiones tomadas.

4. **CHANGELOG.md**: entrada nueva con fecha y los cambios de la sesión, en el
   formato que ya usa el fichero.

5. **NEXT_STEPS.md**: mueve a completado lo cerrado, con el hash del commit. Si
   una tarea reveló trabajo nuevo, añádelo al plan en vez de dejarlo suelto.

6. **PROJECT_STATUS.md**: instantánea del estado de compilación y de la suite.

7. **README.md**: métricas y cifras de la cabecera (número de ficheros de test,
   fecha, estado de la versión).

8. Invoca `/proyecta` para los headers tocados en la sesión.

9. **Verifica la coherencia**: `python scripts/check_docs_consistency.py`. Tiene
   que quedar en verde antes de dar la sesión por cerrada. Comprueba enlaces
   rotos, cifras de tests desactualizadas, correspondencia `API_*.md` ↔ headers,
   cabeceras SPDX, `LICENSE.txt` y coherencia de fechas.

10. Reporta en el chat: qué se cerró, qué ficheros se tocaron, qué queda
    pendiente y dónde está anotado.

**Por qué existe el paso 9:** la auditoría del 23 ago 2026 encontró el README
diciendo «42/42 tests» en la cabecera, «106/106» en una sección y «197/197» en
otra, y enlazando a cuatro ficheros que no existen. Nada de eso rompe una
compilación, así que llevaba meses ahí.
