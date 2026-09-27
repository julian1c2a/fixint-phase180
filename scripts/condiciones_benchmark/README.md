# Condiciones de medida

**Última actualización:** 27 September 2026

Tres guiones para que «la máquina tiene que estar ociosa» deje de ser una frase
en un comentario. Son de **esta máquina** (MSI, con la suite Killer, Nahimic,
Intel DSA y National Instruments instalados); en otra hay que rehacer las listas.

| | qué hace | admin | cuándo |
|---|---|---|---|
| [`comprueba_condiciones.ps1`](comprueba_condiciones.ps1) | **sólo lee**: qué sondeadores están vivos, qué procesos ruidosos, si Defender excluye `build/`, cuántas tareas programadas disparan en 2 h | no | automático, en cada toma |
| [`silencia_maquina.ps1`](silencia_maquina.ps1) | para los sondeadores y pone las exclusiones de Defender | **sí** | a mano, antes de una sesión |
| [`restaura_maquina.ps1`](restaura_maquina.ps1) | los vuelve a arrancar y quita las exclusiones | **sí** | a mano, después |

## Por qué esto no es paranoia

Está medido, y dos veces:

- **El mismo código, con el equipo compilando en paralelo, dio diferencias de
  hasta un 52 %** ([PERFORMANCE.md](../../docs/PERFORMANCE.md), § «la máquina
  tiene que estar ociosa»). No hay umbral que valga contra eso: la medida
  simplemente no sirve.
- El 27 sep, antes de una toma profunda, la carga de fondo era del **49–60 %**.
  Dos procesos —un juego y un `grep` desbocado con 16 h de CPU acumuladas— se
  comían 2,2 núcleos. **Ningún servicio aparecía en el top 10**, y la lista
  completa de `services.msc` tenía 160 en ejecución. Mirar la lista de servicios
  antes de mirar qué consume de verdad habría sido perder la tarde.

De ahí el orden de las tres herramientas: primero **medir** (la carga, con
`bench_history.py --espera-ocioso`), luego **ver quién sobra**
(`comprueba_condiciones.ps1`) y sólo entonces **apagar cosas**
(`silencia_maquina.ps1`).

## Lo que no tocan, y por qué

- **Defender no se apaga: se le ponen exclusiones** para `build/` y `C:\msys64`.
  Lo que cuesta caro no es que exista, es que escanee cada fichero que el
  compilador escribe y borra mil veces.
- **BitLocker, el firewall, RPC/DCOM, el registro de eventos y Hyper-V/WSL no se
  tocan.** Los dos últimos los necesita el proyecto para los tests cruzados de
  arm64/arm32/riscv64.
- **Ni la frecuencia ni la prioridad ni la afinidad.** Fijar el reloj —quitar el
  turbo, o el `Intel Dynamic Tuning`— es lo más efectivo que existe para
  reproducibilidad, y por eso mismo **cambia el régimen de medida**: obliga a
  empezar una serie nueva de cifras, porque las viejas no serían comparables.
  Eso se decide con su toma antes y después, no de paso.
- **Sólo paran servicios; no cambian el tipo de inicio.** Un reinicio lo deja
  todo como estaba, así que el peor caso de olvidarse de restaurar es que la
  máquina vuelva sola.

## El fallo que tuvo `silencia_maquina.ps1` antes de ejecutarse nunca

La primera versión buscaba los servicios por su nombre visible **escrito sin
tildes** —para que el fichero sobreviviera a cualquier codificación— y lo
comparaba tal cual contra el nombre real. Resultado: `Optimización de
distribución`, `Cola de impresión` y `Administración de autenticación de Xbox
Live` **no coincidían nunca**. El guion habría dicho `[no está]` tres veces y
habría terminado en verde con tres servicios sin parar.

Arreglado quitando las tildes de **los dos lados** antes de comparar, y
comprobado: los 28 patrones resuelven a 30 servicios reales, 0 sin resolver. Y
los dos guiones distinguen ahora **`[NO RESUELVE]`** de **`[ya estaba parado]`**,
porque un patrón que no encuentra nada no dice que el servicio no exista: dice
que **esta lista está desfasada**.

## Detalle de orden que no es obvio

`comprueba_condiciones.ps1` se ejecuta **antes** de la espera de máquina ociosa,
nunca después. Arrancar PowerShell cuesta uno o dos segundos de CPU, así que una
comprobación hecha al final **rompe la condición que acaba de verificar**. Con la
espera detrás, ese pico se absorbe.

Es el mismo efecto que se vio en directo el 27 sep: la espera de la toma se
reinició **30 veces** porque cada sonda que se lanzaba para ver si ya había
arrancado creaba el pico que lo impedía.
