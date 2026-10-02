# Condiciones de medida

**Última actualización:** 27 September 2026

Tres guiones para que «la máquina tiene que estar ociosa» deje de ser una frase
en un comentario. Son de **esta máquina** (MSI, con la suite Killer, Nahimic,
Intel DSA y National Instruments instalados); en otra hay que rehacer las listas.

| | qué hace | admin | cuándo |
|---|---|---|---|
| [`sesion_medicion.ps1`](sesion_medicion.ps1) | **el que se usa**: comprueba, silencia, vuelve a comprobar, y lo deja **todo en un log**. Se eleva solo | se eleva él | a mano, antes de una sesión |
| [`comprueba_condiciones.ps1`](comprueba_condiciones.ps1) | **sólo lee**: sondeadores vivos, procesos ruidosos, exclusiones de Defender, tareas programadas y **mantenimiento de Windows en marcha** | no | automático, en cada toma |
| [`silencia_maquina.ps1`](silencia_maquina.ps1) | para los sondeadores y pone las exclusiones de Defender | **sí** | lo llama el de arriba |
| [`restaura_maquina.ps1`](restaura_maquina.ps1) | los vuelve a arrancar y quita las exclusiones | **sí** | `-Accion restaurar` |

```powershell
.\sesion_medicion.ps1                    # comprobar -> silenciar -> comprobar
.\sesion_medicion.ps1 -Accion comprobar  # sólo mirar
.\sesion_medicion.ps1 -Accion restaurar  # devolverlo todo
```

Todo va a `%USERPROFILE%\condiciones_benchmark.log`, **añadiendo**, con fecha y
máquina en cada bloque. Se comprueba **antes y después** a propósito: un guion que
dice «he parado 24 servicios» no demuestra que la máquina esté preparada; dos
comprobaciones con el estado a los dos lados, sí.

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

- El 1 oct 2026 se midió **cuánto** infla una carga ajena, y **lo que no se ve**:
  **un solo proceso** compitiendo durante una ventana sube su cifra un **17 %**,
  y quince la duplican — y la dispersión de la ventana **no lo delata**, porque
  todas sus vueltas se frenan por igual
  ([PERFORMANCE.md](../../docs/PERFORMANCE.md), § «Lo que publica el arnés son
  ticks del TSC»). Ese mismo día una carga ajena de ~4,5 CPU se coló en un
  experimento entero y lo dejó todo un **34 %** más lento.

De ahí el orden de las tres herramientas: primero **medir** (la carga, con
`bench_history.py --espera-ocioso`), luego **ver quién sobra**
(`comprueba_condiciones.ps1`) y sólo entonces **apagar cosas**
(`silencia_maquina.ps1`).

**Y desde el 1 oct 2026 se mide también DURANTE la toma.** `bench_history.py`
anota, para cada ventana, cuánta CPU usaron los demás procesos mientras se medía,
y al terminar dice si alguna se **perturbó** con carga sostenida. Una toma con
alguna ventana perturbada **no vale de referencia**, y `--compare` no la usa. Que
la máquina esté tranquila al empezar ya no se da por bueno para toda la toma: el
campo se llama `tranquila_al_empezar` precisamente para no afirmar más que eso.

## En Linux: `cpu_de_medida.sh` (la plataforma de medida dedicada)

Desde el 2 oct 2026 la serie de referencia se mide en una máquina aparte con
Ubuntu Server, **con el turbo apagado** (ver NEXT_STEPS, «La plataforma de medida
dedicada»). Allí no hay servicios que silenciar; lo que hay que fijar es **la
CPU**, y lo hace este guion:

    sudo cpu_de_medida.sh fija       turbo apagado, gobernador performance,
                                     SMT apagado (si lo hay)
    sudo cpu_de_medida.sh restaura   vuelve a como estaba antes de `fija`
         cpu_de_medida.sh estado     lo que hay ahora

Es **la única pieza del arnés que corre como root**, y por eso:

- **Verifica cada ajuste leyéndolo de vuelta**. Si alguno no cogió, sale con
  error y dice «ESTA MÁQUINA NO ESTÁ LISTA PARA MEDIR».
- Si `fija` se llama dos veces, **conserva el estado original**, no el ya fijado.
- **Restaura en orden**: primero el SMT, porque al encenderlo reaparecen las CPU
  gemelas y después hay que devolverles el gobernador.
- **Como root ignora cualquier raíz alternativa**: `BENCH_SYSFS` y
  `BENCH_ESTADO` son sólo para las pruebas, para que el guion no sea una vía de
  escritura como root en rutas arbitrarias.

Probado contra un `sysfs` falso en `scripts/tests/test_cpu_de_medida.py` y
falsificado con seis averías a propósito. Dos cosas **no se pueden probar con un
`sysfs` falso** y quedan para la primera sesión en la máquina de verdad: el orden
al restaurar el SMT —en el `/sys` real, apagarlo hace desaparecer los ficheros de
las CPU gemelas— y que la EPP la cambie el kernel al poner `performance` (la
prueba lo simula, pero es una simulación).

`bench_history.py` registra en cada toma con qué configuración se midió, y
`--compare` no compara en silencio tomas con distinta configuración.

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

## Dos formas de mentir que se arreglaron el 27 sep, las dos el mismo día

**Un guion que necesita permisos y no los comprueba.** `silencia_maquina.ps1`,
lanzado desde el terminal de VS Code —que no está elevado— falló **26 veces** con
`Cannot open 'DoSvc' service`, no pudo poner las exclusiones, y terminó con un
tranquilizador «Lo parado queda apuntado en...». Para saber que no había
funcionado había que leerse las 26 líneas. Ahora comprueba la elevación al
principio, aborta con `rc=1`, y `sesion_medicion.ps1` se eleva él solo.

**Un `false` que significaba «no me dejan ver».** `Get-MpPreference` devuelve la
lista de exclusiones **vacía** a quien no es administrador, sin dar ningún error,
así que el comprobador decía `defender_excluye_build=false` con las exclusiones ya
puestas. Ahora hay tres valores —`true`, `false` y `desconocido`— y el veredicto
**no afirma lo que no sabe**.

La misma familia que el `texto.count("")` del parcheador y que el `2>/dev/null`
del CI: el fallo no es equivocarse, es equivocarse **en el sentido tranquilizador**.

## El riesgo que no está en ninguna lista de servicios

Las tareas programadas con hora no son las peligrosas: son pequeñas y se las ve
venir. Las gordas de Windows **no tienen hora** —se disparan cuando la máquina
está ociosa—, y ahí hay una ironía que conviene tener presente: **la espera de
`--espera-ocioso` crea a propósito noventa segundos de inactividad, que es
exactamente la invitación que esas tareas esperan.**

Las que pueden mover una medida de verdad, sacadas del volcado del 27 sep:
`StartComponentCleanup` (limpieza de WinSxS), `SilentCleanup`, los dos `NGEN` de
.NET, **`WinSAT` —que es literalmente un banco de pruebas del sistema—**,
`RegIdleBackup`, el indexador, `ProactiveScan` de chkdsk, el escaneo de integridad
de datos y el escaneo programado de Defender.

El comprobador publica `mantenimiento_corriendo=` con las que estén **en marcha**,
y si hay alguna el veredicto lo pone **primero**: es el único punto que invalida la
toma *ahora mismo* y no «a lo mejor».
