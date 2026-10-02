#!/usr/bin/env python3
# =============================================================================
# bench_history.py - ejecuta los benchmarks y guarda sus cifras con contexto
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Por que existe
# --------------
# `docs/PERFORMANCE.md` exige que toda cifra lleve **fecha, compilador y
# maquina**, porque sin eso no se puede comparar con otra. Hasta ahora eso se
# cumplia a mano, y las tablas heredadas de la fase 1.75 no lo cumplen.
#
# Y hay un matiz que conviene tener presente: **mas frecuente no es mejor si
# las medidas no son comparables**. Una cifra tomada en un runner compartido y
# otra tomada aqui NO van en la misma serie; una serie con medidas
# incomparables es peor que no tener serie, porque invita a leer tendencias que
# no existen. Por eso el historico se indexa POR MAQUINA y las comparaciones
# solo se hacen dentro de la misma.
#
# Como funciona
# -------------
# Los benchmarks escriben sus cifras en el fichero que indique la variable de
# entorno BENCH_OUT (ver `bench_record` en benchs/bench_common.hpp). Este guion
# los ejecuta con esa variable puesta, recoge lo que dejan, le añade el
# contexto que el binario no puede saber -- commit, compilador, maquina -- y lo
# guarda en benchs/history/<maquina>/<fecha>-<commit>.json
#
# Uso
# ---
#   python scripts/bench_history.py                    # todos, con gcc
#   python scripts/bench_history.py --compiler clang
#   python scripts/bench_history.py --only karatsuba
#   python scripts/bench_history.py --compare          # frente a la anterior
#   python scripts/bench_history.py --list             # que hay guardado
# =============================================================================

import argparse
import io
import json
import os
import platform
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

# P3.10: UN solo sitio decide donde se compila y donde se busca el binario.
# Antes lo decidian ocho sitios con reglas distintas, y Windows y WSL se
# pisaban los binarios. Ver scripts/env_setup/rutas.py.
import sys as _sys_rutas
from pathlib import Path as _Path_rutas
_sys_rutas.path.insert(0, str(_Path_rutas(__file__).resolve().parent))
from env_setup import rutas  # noqa: E402

RAIZ = Path(__file__).resolve().parent.parent
BENCHS = RAIZ / "benchs"
HISTORIA = BENCHS / "history"

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent / "env_setup"))


def entorno_de(compilador: str) -> dict:
    """Entorno con el que hay que LANZAR los binarios de ese compilador.

    No basta con `os.environ`. Un binario de ucrt64 lanzado con el PATH de una
    shell cualquiera puede cargar el `libstdc++-6.dll` viejo que trae Git y morir
    con 0xC0000005; uno de clang64 necesita su `libc++`. Es el mismo entorno
    aislado que usa `build_generic` para compilar, y hay que usarlo tambien para
    EJECUTAR.

    Sin esto, los binarios morian al arrancar, no escribian el TSV, y este guion
    lo achacaba al fuente: "no registra (le falta bench_record)". Los ocho
    benchmarks salian con ese mensaje el 6 sep 2026, y todos tienen
    `bench_record`.
    """
    try:
        from compiler_env import CompilerEnvironment  # noqa: PLC0415

        return CompilerEnvironment(compilador).get_env()
    except Exception:
        return os.environ.copy()

# CUANTO SE TIENE QUE MOVER UNA MEDIDA PARA QUE SIGNIFIQUE ALGO. Hay dos
# criterios, y el primero es el bueno.
#
# 1. EL RUIDO DE LA PROPIA MEDIDA. Desde el 10 sep el arnes adaptativo mide cada
#    casilla diez veces con rondas entrelazadas y publica su recorrido. En esta
#    maquina los recorridos van del 9 % al 58 % SEGUN LA CASILLA, asi que un
#    umbral unico marca de mas en unas y de menos en otras. Cuando las dos tomas
#    traen su recorrido, la barra es la SUMA DE LOS DOS: es la comprobacion que
#    pide el @note de `RECORRIDO_RUIDOSO` en benchs/bench_adaptativo.hpp, que
#    hasta ahora era imposible de hacer aqui porque el dato no llegaba al
#    fichero.
#
# 2. EL UMBRAL PLANO, solo para las medidas que no traen recorrido: las tomas
#    guardadas antes de esto y los benchmarks que no usan el arnes adaptativo.
#    MEDIDO el 5 sep 2026 con el arnes VIEJO: dos ejecuciones seguidas sin tocar
#    nada dieron una mediana del 5,1 %, un p90 del 15,6 % y un peor caso del
#    25,2 %. De ahi el 25 %: por debajo serian todo falsos positivos, y un aviso
#    que salta siempre no se mira.
UMBRAL_PLANO = 0.25

# Suelo de la barra «propia». MEDIDO el 27 sep 2026, no elegido a ojo.
#
# El recorrido es la dispersion DENTRO de una ejecucion. Entre dos ejecuciones
# hay ademas deriva --temperatura, colocacion del binario, lo que hiciera la
# maquina-- que ninguna de las dos ve. La pregunta era si esa deriva se come la
# barra en las casillas muy quietas.
#
# LA CALIBRACION: dos tomas seguidas del mismo codigo, 975 medidas comparables.
# Todo lo que se mueva ahi es ruido por definicion.
#
#                           mediana    p90     p99    peor
#     con ruido propio        1,3 %   3,3 %   4,5 %   8,9 %
#     sin ruido propio        3,5 %  14,0 %  42,0 %  95,5 %
#
#     falsos positivos, umbral plano 25 %       26 de 975
#     falsos positivos, suma de recorridos       0 de 158  (con CUALQUIER
#                                                           suelo de 0 a 20 %)
#
# O sea que con suelo CERO ya salen cero falsos positivos: la suma de recorridos
# se basta. El 5 % se deja como seguro barato --esta justo por encima del p99--
# porque por debajo de esa cifra ningun cambio real se distingue del ruido en
# esta maquina, asi que no tapa nada que se pudiera ver.
#
# Y un hallazgo que no se buscaba: el arnes adaptativo no solo MIDE el ruido,
# lo REDUCE. Sus casillas se mueven diez veces menos entre tomas que las del
# arnes viejo (p90 3,3 % frente a 14,0 %). Por eso conviene migrar los que
# quedan.
SUELO_ENTRE_TOMAS = 0.05


def barra_de(antes, ahora):
    """Barra de esta medida, y con que criterio se decidio. TRES, en orden.

    1. **COLA BAJA**, el bueno, activo desde el 27 sep. La cifra que se publica es
       el minimo, o sea un estimador de la cola de ABAJO, asi que su
       incertidumbre es la de esa cola. `dispersion_baja` la mide.

    2. **RECORRIDO**, para las medidas que traen dispersion pero no cola baja
       (tomas guardadas entre el 26 y el 27 sep). Es `max - min`, dominado por la
       contaminacion de ARRIBA -- que no toca al minimo --, asi que sale enorme:
       mediana del 52 % frente al 5 % de la cola baja. Detecta poco, pero no
       miente.

    3. **PLANO**, para las que no traen nada de ruido: el arnes viejo.

    MEDIDO con el par limpio del 27 sep --dos tomas en las mismas condiciones y
    con el mismo codigo, 216 casillas--:

        criterio                        falsos positivos
        recorrido                             0 de 216
        cola baja, suelo 0 %                165 de 216
        cola baja, suelo 5 %                  0 de 216

    O sea que la cola baja SIN SUELO no sirve: mide la estabilidad DENTRO de una
    tanda (0,8 % de mediana) y lo que hace falta es la reproducibilidad ENTRE
    tandas (1,28 % de mediana, 4,31 % en el peor caso). Se diferencian en un
    factor de cuatro o cinco, y el suelo es lo que cubre esa diferencia.
    """
    ba, bb = antes.get("dispersion_baja"), ahora.get("dispersion_baja")
    if ba is not None and bb is not None:
        return max(SUELO_ENTRE_TOMAS, ba + bb), "cola baja"
    ra, rb = antes.get("recorrido"), ahora.get("recorrido")
    if ra is not None and rb is not None:
        return max(SUELO_ENTRE_TOMAS, ra + rb), "recorrido"
    return UMBRAL_PLANO, "plano"


def echo(msg):
    print(msg)


def commit_actual() -> str:
    try:
        r = subprocess.run(["git", "rev-parse", "--short", "HEAD"],
                           cwd=RAIZ, capture_output=True, text=True)
        return r.stdout.strip() or "sin-commit"
    except Exception:
        return "sin-commit"


def arbol_limpio() -> bool:
    """Si el arbol tiene cambios, la medida no se puede atribuir al commit.

    SALVO EL PROPIO HISTORICO. Este guion escribe en `benchs/history/`, asi que
    la toma anterior deja un fichero sin commitear y la siguiente se declaraba
    «sucia» por culpa de su predecesora. Paso en la calibracion del 27 sep: la
    segunda toma se guardo con `arbol_limpio: false` teniendo el codigo intacto,
    que es justo la clase de metadato que luego se lee mal.
    """
    try:
        r = subprocess.run(["git", "status", "--porcelain"],
                           cwd=RAIZ, capture_output=True, text=True)
        sucios = [l for l in r.stdout.splitlines()
                  if l.strip() and "benchs/history/" not in l.replace(chr(92), "/")]
        return not sucios
    except Exception:
        return False


def sistema(plataforma: str = None, version: str = None) -> str:
    """`windows`, `wsl` o `linux`: el sistema que corre en la maquina.

    WSL se separa de Linux porque es OTRA plataforma de medida aunque corra en el
    mismo hierro: una maquina virtual, con su planificador y sus relojes. Se
    reconoce por `/proc/version`. Los dos argumentos son para las pruebas.
    """
    plataforma = plataforma or rutas.plataforma()
    if plataforma == "windows":
        return "windows"
    if version is None:
        try:
            with open("/proc/version", encoding="ascii", errors="replace") as f:
                version = f.read()
        except OSError:
            version = ""
    v = version.lower()
    return "wsl" if ("microsoft" in v or "wsl" in v) else "linux"


def nombre_maquina(nodo: str = None, sist: str = None) -> str:
    """Identifica la SERIE: la maquina, y el sistema que corre en ella.

    POR QUE NO BASTA EL NOMBRE DE HOST. Hasta el 2 oct 2026 se usaba solo
    `platform.node()`, y una instalacion de Ubuntu en la misma maquina con el
    mismo nombre --lo natural-- habria ido a la misma carpeta que Windows. Como
    las dos piden `gcc`, `--compare` habria comparado un sistema contra el otro
    sin avisar (P2.24).

    Windows conserva el nombre a secas para no romper la carpeta que ya existe; los
    demas llevan el sistema detras: `MSI-linux`, `MSI-wsl`.
    """
    nodo = nodo if nodo is not None else (platform.node() or "desconocida")
    sist = sist or sistema()
    return nodo if sist == "windows" else "%s-%s" % (nodo, sist)


def version_compilador(compilador: str) -> str:
    """Version del compilador, preguntandosela EN SU ENTORNO.

    ANTES resolvia el comando con `toolchains.resolve` y lo lanzaba con el
    entorno del proceso. Para intel eso da `icx` --un nombre pelado que no esta
    en el PATH mientras no se haya corrido `setvars.bat`-- y para msvc `cl.exe`,
    que necesita `vcvars64.bat`. Los dos caian en "desconocida", y la medida
    quedaba guardada sin decir con que se habia compilado, que es justo el dato
    que la hace comparable. Ahora se usa el entorno aislado, igual que para
    compilar y para ejecutar.
    """
    try:
        from compiler_env import CompilerEnvironment  # noqa: PLC0415

        ce = CompilerEnvironment(compilador)
        cmd, env = ce.get_compiler_cmd(), ce.get_env()
    except Exception:
        cmd, env = compilador, None

    for flag in ("--version", "/?"):
        try:
            r = subprocess.run([cmd, flag], capture_output=True, text=True,
                               timeout=20, env=env)
            salida = (r.stdout or r.stderr).strip().splitlines()
            if salida:
                return salida[0].strip()
        except Exception:
            continue
    return "desconocida"


def _lee(ruta):
    """El contenido de un fichero de sysfs, sin espacios. None si no se puede leer.

    None es «no lo se», y no se sustituye nunca por un valor por omision: un turbo
    que no se pudo leer NO es un turbo apagado.
    """
    try:
        with open(ruta, encoding="ascii", errors="replace") as f:
            return f.read().strip()
    except OSError:
        return None


def _todas_las_cpu(raiz, relativo):
    """Los valores distintos de `cpuN/<relativo>` en todas las CPU, ordenados."""
    base = Path(raiz) / "sys" / "devices" / "system" / "cpu"
    vistos = set()
    for d in sorted(base.glob("cpu[0-9]*")):
        v = _lee(d / relativo)
        if v is not None:
            vistos.add(v)
    return sorted(vistos)


def configuracion_cpu_linux(raiz: str = "/") -> dict:
    """Con que configuracion de CPU se esta midiendo, leida de sysfs. Linux.

    Es lo que define que significan las cifras: con el turbo apagado y el
    gobernador en `performance`, el nucleo va a frecuencia fija como el TSC y los
    «ciclos» son ciclos de verdad; con el turbo encendido, una carga ajena los
    infla (P2.22). Lo que no se puede leer va a None. `raiz` es para las pruebas.
    """
    cpu = Path(raiz) / "sys" / "devices" / "system" / "cpu"
    driver = _lee(cpu / "cpu0" / "cpufreq" / "scaling_driver")

    # EL TURBO SE APAGA POR SITIOS DISTINTOS segun el driver: intel_pstate tiene
    # su `no_turbo` (1 = apagado); acpi-cpufreq y amd-pstate usan `boost`
    # (0 = apagado), global o por CPU.
    turbo = None
    no_turbo = _lee(cpu / "intel_pstate" / "no_turbo")
    if no_turbo in ("0", "1"):
        turbo = "apagado" if no_turbo == "1" else "encendido"
    else:
        boost = _lee(cpu / "cpufreq" / "boost")
        por_cpu = _todas_las_cpu(raiz, "cpufreq/boost")
        valores = [boost] if boost in ("0", "1") else por_cpu
        if valores and set(valores) <= {"0", "1"}:
            turbo = ("apagado" if valores == ["0"] else
                     "encendido" if valores == ["1"] else "mixto")

    def unico(valores):
        if not valores:
            return None
        return valores[0] if len(valores) == 1 else "mixto: " + ",".join(valores)

    modelo = None
    for linea in (_lee(Path(raiz) / "proc" / "cpuinfo") or "").splitlines():
        if linea.startswith("model name"):
            modelo = linea.split(":", 1)[1].strip()
            break

    return {
        "modelo": modelo,
        "driver": driver,
        "turbo": turbo,
        "gobernador": unico(_todas_las_cpu(raiz, "cpufreq/scaling_governor")),
        "epp": unico(_todas_las_cpu(raiz, "cpufreq/energy_performance_preference")),
        "smt": _lee(cpu / "smt" / "control"),
        "en_linea": _lee(cpu / "online"),
        "aisladas": _lee(cpu / "isolated") or None,
        "khz_min": unico(_todas_las_cpu(raiz, "cpufreq/scaling_min_freq")),
        "khz_max": unico(_todas_las_cpu(raiz, "cpufreq/scaling_max_freq")),
    }


def huella_cpu(toma: dict):
    """Lo que tiene que coincidir para que dos tomas sean comparables, o None.

    Driver, turbo, gobernador y SMT: lo que cambia que significan las cifras. None
    en las tomas que no lo registraron (las de Windows, y todas las de antes del
    2 oct 2026) -- y None no es comparable con nada distinto de None.
    """
    c = (toma.get("condiciones") or {}).get("cpu")
    if not c:
        return None
    return (c.get("driver"), c.get("turbo"), c.get("gobernador"), c.get("smt"))


def condiciones_de_medida() -> dict:
    """Estado de la maquina segun `scripts/condiciones_benchmark/`.

    SOLO LEE: no para nada y no necesita elevacion.

    **Se llama ANTES de la espera de maquina ociosa, nunca despues.** Arrancar
    PowerShell cuesta uno o dos segundos de CPU, asi que una comprobacion hecha
    al final rompe la condicion que acaba de verificar. Con la espera detras, el
    pico se absorbe. (Visto en directo el 27 sep: la espera de una toma se
    reinicio 30 veces porque cada sonda que se lanzaba para ver si ya habia
    arrancado creaba el pico que lo impedia.)

    Nunca aborta ni bloquea: si no se puede comprobar, lo dice y sigue. Una toma
    sin este dato vale menos, pero vale.
    """
    if rutas.plataforma() != "windows":
        # EN LINUX LO QUE IMPORTA ES LA CPU, no los servicios (P2.24): con que
        # turbo, gobernador y SMT se esta midiendo, que es lo que define que
        # significan las cifras.
        cpu = configuracion_cpu_linux()
        return {"verdicto": "turbo %s, gobernador %s, SMT %s" % (
                    cpu["turbo"] or "¿?", cpu["gobernador"] or "¿?", cpu["smt"] or "¿?"),
                "cpu": cpu}

    guion = RAIZ / "scripts" / "condiciones_benchmark" / "comprueba_condiciones.ps1"
    if not guion.exists():
        return {"verdicto": "no se pudo comprobar: falta %s" % guion.name}

    raiz_win = os.environ.get("SystemRoot", "C:/Windows")
    exe = os.path.join(raiz_win, "System32", "WindowsPowerShell", "v1.0", "powershell.exe")
    try:
        r = subprocess.run([exe, "-NoProfile", "-ExecutionPolicy", "Bypass",
                            "-File", str(guion)],
                           capture_output=True, text=True, timeout=180,
                           encoding="utf-8", errors="replace")
    except Exception as e:  # noqa: BLE001
        return {"verdicto": "no se pudo comprobar: %s" % e}

    salida = {}
    for linea in (r.stdout or "").splitlines():
        if "=" in linea:
            k, v = linea.split("=", 1)
            salida[k.strip()] = v.strip()
    if not salida:
        pista = (r.stderr or r.stdout or "").strip().splitlines()
        return {"verdicto": "no se pudo comprobar: sin salida%s"
                % ((": " + pista[-1][:70]) if pista else "")}
    return salida


def _tiempos_cpu():
    """(ocupado, total) acumulados desde el arranque. None si no se puede.

    Sin dependencias a proposito: en esta maquina `wmic` esta deprecado y
    `typeperf` no siempre esta, y una sonda que falla en silencio devolveria
    «maquina ociosa» justo cuando no lo esta.
    """
    if rutas.plataforma() == "windows":
        import ctypes  # noqa: PLC0415
        from ctypes import wintypes  # noqa: PLC0415

        class FT(ctypes.Structure):
            _fields_ = [("lo", wintypes.DWORD), ("hi", wintypes.DWORD)]

        def val(f):
            return (f.hi << 32) | f.lo

        idle, kern, user = FT(), FT(), FT()
        try:
            ok = ctypes.windll.kernel32.GetSystemTimes(
                ctypes.byref(idle), ctypes.byref(kern), ctypes.byref(user))
        except Exception:
            return None
        if not ok:
            return None
        # OJO: el tiempo de KERNEL **incluye** el de idle. Restarlo aparte es el
        # error clasico de esta API, y da cargas negativas en reposo.
        total = val(kern) + val(user)
        return total - val(idle), total

    try:
        with open("/proc/stat", encoding="ascii") as f:
            campos = [int(x) for x in f.readline().split()[1:]]
    except (OSError, ValueError):
        return None
    if len(campos) < 4:
        return None
    total = sum(campos)
    parado = campos[3] + (campos[4] if len(campos) > 4 else 0)  # idle + iowait
    return total - parado, total


def carga_ahora(intervalo: float = 1.0):
    """Fraccion de CPU ocupada durante `intervalo` segundos. None si no se sabe."""
    a = _tiempos_cpu()
    if a is None:
        return None
    time.sleep(intervalo)
    b = _tiempos_cpu()
    if b is None:
        return None
    d_total = b[1] - a[1]
    if d_total <= 0:
        return None
    return max(0.0, min(1.0, (b[0] - a[0]) / d_total))


def espera_ocioso(umbral: float, quieto: float, limite: float):
    """Espera a que la maquina lleve `quieto` segundos por debajo de `umbral`.

    Devuelve (carga_final, esperado_segundos, se_consiguio).

    POR QUE ESPERAR Y NO SOLO AVISAR: el aviso lo lee quien lanza, pero la
    medida la hace la maquina. Entre lanzar y empezar a medir puede haber un
    antivirus, un indexador o el cierre de otro editor. Esperar mueve la
    condicion de «acuerdate» a «comprobado».

    NO se aborta si no se consigue: se sigue y se ANOTA. Una toma con carga
    conocida vale mas que ninguna toma, siempre que la carga quede escrita.
    """
    # LA MEDIA DE LA VENTANA, NO TODAS LAS MUESTRAS.
    #
    # Antes exigia que NINGUNA muestra de un segundo pasara del umbral durante
    # `quieto` segundos seguidos, y eso es imposible de cumplir en una maquina
    # real: la toma 5 agoto los 45 minutos de espera **con la maquina al 2 %**,
    # porque basta un pico de un segundo --el propio Windows, o alguien mirando
    # el log-- para volver el contador a cero. Se conto 35 reinicios en otra.
    #
    # Lo que se quiere saber es si la maquina esta tranquila, no si hubo un
    # segundo agitado. Con la media de la ventana, un pico aislado se diluye y
    # una carga sostenida no.
    t0 = time.time()
    ventana = []
    n_ventana = max(3, int(quieto))
    ultima = None
    avisado = False
    while time.time() - t0 < limite:
        c = carga_ahora(1.0)
        if c is None:
            return None, time.time() - t0, False
        ultima = c
        ventana.append(c)
        if len(ventana) > n_ventana:
            ventana.pop(0)
        if len(ventana) >= n_ventana:
            media = sum(ventana) / len(ventana)
            if media <= umbral:
                return media, time.time() - t0, True
            if not avisado:
                echo("  [espera] la media de los ultimos %d s va al %.0f %%; sigo esperando"
                     % (n_ventana, media * 100))
                avisado = True
    return ultima, time.time() - t0, False


# ============================================================================
# LO QUE HACE LA MAQUINA DURANTE LA TOMA (P2.22)
# ============================================================================
#
# POR QUE EXISTE. `espera_ocioso` comprueba la calma ANTES de empezar y nunca
# durante. Dos tomas del mismo codigo, el mismo dia, ambas tranquilas al empezar,
# salieron con 7 y con 46 ventanas sucias de 172: lo que paso durante no lo vio
# nadie, y la sucia se guardo igual que la limpia.
#
# QUE SE MIDE: `otros`, la CPU que ocupan LOS DEMAS procesos mientras corre el
# benchmark, en unidades de CPU logica. Se resta la del propio benchmark porque
# la maquina tiene muchas CPU y el benchmark ocupa una entera: a lo bruto se
# mediria a si mismo.
#
# Y SE CRUZA CON CADA VENTANA gracias a los sellos `t_inicio`/`t_fin` que
# escribe el arnes C++ con el reloj de pared -- validado que es el mismo que el
# de `time.time()` aqui.

def _cpu_ocupada_sistema(psutil):
    """Segundos de CPU ocupada en toda la maquina, sumados sobre todas las CPU.

    EN WINDOWS NO SE COPIA LA DEFINICION DE PSUTIL, porque cuenta doble. psutil
    toma `system` de `GetSystemTimes`, que ya es el tiempo de nucleo menos el
    ocioso -- e INCLUYE interrupciones y DPC --, y luego suma `interrupt` y `dpc`
    aparte. Medido el 1 oct 2026 contra la suma proceso a proceso: con su
    definicion el Muestreador leia +0,11 a +0,40 CPU de mas, creciendo con la
    carga (mas procesos, mas interrupciones). Aqui: `user + system`.

    En Linux si vale la de psutil: el total menos lo ocioso y `iowait`, y sin
    `guest`, que ya va dentro de `user`.
    """
    t = psutil.cpu_times()
    # POR LA FORMA DEL DATO, NO POR EL NOMBRE DEL SISTEMA. Hasta el 2 oct 2026 se
    # decidia con `sys.platform == "win32"`, y eso depende del interprete: con la
    # Python de MSYS dice 'cygwin' y el arreglo se saltaba en silencio (lo
    # documenta `rutas.plataforma`). La cuenta doble pasa exactamente cuando
    # psutil devuelve `dpc` aparte -- su backend de Windows --, asi que se mira eso.
    if hasattr(t, "dpc"):
        return t.user + t.system
    total = sum(t)
    total -= getattr(t, "guest", 0) + getattr(t, "guest_nice", 0)  # ya van en user/nice
    return total - t.idle - getattr(t, "iowait", 0)


def _cpu_del_proceso(proc):
    t = proc.cpu_times()
    return t.user + t.system


class Muestreador:
    """Mide `otros` cada `periodo` segundos mientras vive el proceso `pid`.

    Se usa como gestor de contexto alrededor de la ejecucion del benchmark. Las
    muestras quedan en `self.muestras` como tuplas (t0, t1, otros), con `t0` y
    `t1` en segundos desde la epoca: el mismo reloj que los sellos del arnes.

    `self.disponible` es False si no se pudo medir -- sin psutil, o porque el
    proceso murio antes de la primera muestra --, y entonces `muestras` va vacia.
    Eso es «no lo se», y quien lo lea tiene que tratarlo asi, no como cero.
    """

    def __init__(self, pid: int, periodo: float = 0.5):
        self.pid = pid
        self.periodo = periodo
        self.muestras = []
        self.disponible = False
        self.motivo = ""
        self._parar = None
        self._hilo = None

    def __enter__(self):
        import threading  # noqa: PLC0415
        try:
            import psutil  # noqa: PLC0415
        except ImportError:
            self.motivo = "psutil no esta instalado"
            return self
        self._psutil = psutil
        self._parar = threading.Event()
        self._hilo = threading.Thread(target=self._bucle, daemon=True)
        self._hilo.start()
        return self

    def __exit__(self, *exc):
        if self._parar is not None:
            self._parar.set()
            self._hilo.join(timeout=5)
        return False

    def _bucle(self):
        psutil = self._psutil
        try:
            hijo = psutil.Process(self.pid)
            t_prev, s_prev, h_prev = time.time(), _cpu_ocupada_sistema(psutil), _cpu_del_proceso(hijo)
        except Exception as e:  # el hijo ya no esta, o no se puede leer
            self.motivo = "no se pudo leer el proceso: %s" % type(e).__name__
            return
        while not self._parar.wait(self.periodo):
            try:
                t, s, h = time.time(), _cpu_ocupada_sistema(psutil), _cpu_del_proceso(hijo)
            except Exception:
                break  # el hijo ha terminado: lo normal al final
            dt = t - t_prev
            if dt > 0:
                # Negativo solo por redondeo de los contadores: se recorta a cero.
                self.muestras.append((t_prev, t, max(0.0, ((s - s_prev) - (h - h_prev)) / dt)))
                self.disponible = True
            t_prev, s_prev, h_prev = t, s, h
        if not self.disponible and not self.motivo:
            self.motivo = "el benchmark termino antes de la primera muestra"


def carga_en_tramo(muestras, t0: float, t1: float):
    """(media, maximo) de `otros` en el tramo [t0, t1]. None si no hay muestras.

    La media va PONDERADA por cuanto solapa cada muestra con el tramo: una muestra
    que lo toca una decima no pesa como una que lo cubre entera. El maximo es el
    de las muestras que solapan algo, porque una rafaga corta es justo lo que se
    busca y una media la diluiria.

    Funcion pura, sin psutil: es la que se prueba sola.
    """
    peso, suma, maximo = 0.0, 0.0, None
    for a, b, otros in muestras:
        solape = min(b, t1) - max(a, t0)
        if solape <= 0:
            continue
        peso += solape
        suma += solape * otros
        maximo = otros if maximo is None else max(maximo, otros)
    if peso <= 0:
        return None
    return suma / peso, maximo


def suelo_en_tramo(muestras, t0: float, t1: float, q: float = 0.10):
    """Percentil `q` de `otros` en las muestras que solapan [t0, t1]. None si no hay.

    ES LA CIFRA QUE DECIDE, y no la media. Medido el 1 oct 2026 (E3, P2.22): una
    carga SOSTENIDA mantiene alto el suelo durante toda la ventana, y la de a
    RAFAGAS --el editor, Windows-- deja huecos y el suelo baja al fondo. La media
    no las separa: una ventana limpia con el editor activo dio 1,59 y una con un
    proceso sostenido, 1,41. El suelo si: inocuas <= 0,30, danina >= 1,10.

    Funcion pura, sin psutil.
    """
    xs = sorted(o for a, b, o in muestras if min(b, t1) - max(a, t0) > 0)
    if not xs:
        return None
    i = q * (len(xs) - 1)
    lo = int(i)
    hi = min(lo + 1, len(xs) - 1)
    return xs[lo] + (xs[hi] - xs[lo]) * (i - lo)


# CUANTO SUELO DE `otros` HACE FALTA PARA LLAMAR PERTURBADA A UNA VENTANA, en CPU
# logicas. CALIBRADO, no elegido, el 1 oct 2026 en esta maquina (8 nucleos, 16
# hilos) y con el editor abierto:
#
#     ventanas inocuas (limpias, a rafagas, bordes)   suelo <= 0,30
#     un solo proceso sostenido                       suelo >= 1,10   (+15 % en el minimo)
#
# 0,6 deja ~0,3 de margen por abajo y ~0,5 por arriba. Elegido en E3 y
# comprobado en E2-bis, que no lo genero. En otra maquina se vuelve a medir: un
# umbral publicado es relativo a la maquina donde se midio.
SUELO_PERTURBADA = 0.6


def ventana_perturbada(dato):
    """True si la ventana tuvo carga SOSTENIDA de otros procesos.

    None si no se sabe --toma de antes del 1 oct 2026, o sin psutil--. Es una
    tercera respuesta, no un False: quien la lea tiene que tratarla asi.
    """
    suelo = dato.get("otros_suelo")
    if suelo is None:
        return None
    return suelo > SUELO_PERTURBADA


def veredicto_durante(toma: dict) -> dict:
    """Que se sabe de lo que paso DURANTE una toma. Funcion pura.

    Cuenta VENTANAS, no casillas: las variantes de una ventana comparten sello y
    se agrupan por el (leccion de P2.18). Devuelve:

        medido        si alguna ventana trae medida de durante
        ventanas      cuantas la traen
        sin_dato      cuantas ventanas con sello NO la traen
        perturbadas   cuantas superan SUELO_PERTURBADA
        por_suite     {suite: perturbadas} solo de las que tienen alguna
        certificable  medido, sin huecos y sin ninguna perturbada
    """
    vistas = {}   # (suite, t_inicio, t_fin) -> perturbada (True/False/None)
    for suite, medidas in toma.get("suites", {}).items():
        for dato in medidas.values():
            if "t_inicio" not in dato:
                continue
            clave = (suite, dato["t_inicio"], dato["t_fin"])
            p = ventana_perturbada(dato)
            # Si alguna variante de la ventana lo sabe, la ventana lo sabe.
            if clave not in vistas or vistas[clave] is None:
                vistas[clave] = p
            elif p:
                vistas[clave] = True
    con_dato = [v for v in vistas.values() if v is not None]
    por_suite = {}
    for (suite, _, _), v in vistas.items():
        if v:
            por_suite[suite] = por_suite.get(suite, 0) + 1
    perturbadas = sum(1 for v in con_dato if v)
    sin_dato = sum(1 for v in vistas.values() if v is None)
    return {
        "medido": bool(con_dato),
        "ventanas": len(con_dato),
        "sin_dato": sin_dato,
        "perturbadas": perturbadas,
        "por_suite": por_suite,
        "certificable": bool(con_dato) and sin_dato == 0 and perturbadas == 0,
    }


def elige_referencia(tomas, suites=None, huella="sin filtro"):
    """De una lista de tomas (de la mas vieja a la mas nueva), cual usar de
    referencia para comparar `suites`, y por que si no es certificable. Pura.

    Devuelve (indice, motivo). `motivo` es None si la elegida es certificable; si
    no lo es ninguna, se devuelve la mas reciente y `motivo` dice POR QUE no se
    puede fiar de ella. Antes del 1 oct 2026 se cogia siempre la mas reciente, y
    una toma con rafagas valia de referencia igual que una limpia (P2.22).

    LA REFERENCIA TIENE QUE TENER LO QUE SE COMPARA. Si se pasa `suites`, solo
    cuentan las tomas que las tienen TODAS; si ninguna, las que tienen ALGUNA. Se
    vio en la prueba de extremo a extremo: con `--only bases` se escogio una toma
    sin `bases` y la comparacion salio vacia sin avisar.
    """
    if not tomas:
        return None, "no hay tomas anteriores"
    indices = list(range(len(tomas)))
    if suites:
        quiero = set(suites)
        todas = [i for i in indices if quiero <= set(tomas[i].get("suites", {}))]
        alguna = [i for i in indices if quiero & set(tomas[i].get("suites", {}))]
        indices = todas or alguna
        if not indices:
            return None, "ninguna toma anterior tiene estas suites"
    # LA MISMA CONFIGURACION DE CPU, si se pide (P2.24). Una toma con el turbo
    # encendido no sirve de referencia para una con el turbo apagado. Si ninguna
    # coincide, se sigue con las que hay y el aviso de `comparar` lo dira.
    if huella != "sin filtro":
        iguales = [i for i in indices if huella_cpu(tomas[i]) == huella]
        indices = iguales or indices
    for i in reversed(indices):
        if veredicto_durante(tomas[i])["certificable"]:
            return i, None
    ultima = indices[-1]
    v = veredicto_durante(tomas[ultima])
    if not v["medido"]:
        return ultima, ("no tiene medida de lo que paso DURANTE la toma (es de antes "
                        "del 1 oct 2026): no se puede saber si alguna casilla estaba "
                        "inflada por carga de otros procesos")
    return ultima, ("tiene %d ventana(s) perturbada(s) por carga sostenida de otros "
                    "procesos%s" % (v["perturbadas"], (" y %d sin dato" % v["sin_dato"])
                                    if v["sin_dato"] else ""))


def benchmarks_disponibles():
    return sorted(f.stem[len("benchmark_"):]
                  for f in BENCHS.glob("benchmark_*.cpp"))


def lee_medidas(lineas):
    """Las medidas de un TSV de `bench_record`, linea a linea. Funcion pura.

    Acepta las cuatro formas que han existido, y todas siguen vivas:

        3 columnas   caso, valor, unidad                  (arnes viejo)
        7            + dispersion, recorrido, iters, reps
        11           + suelo, disp_baja, limpias, k_suelo (27 sep 2026)
        13           + t_inicio, t_fin                    (1 oct 2026, P2.22)

    Se saco de `ejecutar` el 1 oct 2026 para poder probarla: ha crecido tres
    veces y cada ampliacion arriesgaba romper los formatos anteriores sin que
    nada lo comprobara. Ver scripts/tests/test_bench_history.py.
    """
    medidas = {}
    for linea in lineas:
        partes = linea.rstrip("\n").split("\t")
        if len(partes) < 3:
            continue
        try:
            dato = {"valor": float(partes[1]), "unidad": partes[2]}
        except ValueError:
            continue
        # LAS COLUMNAS DEL RUIDO SON OPCIONALES. Las escribe `bench::registra`,
        # que es quien las sabe; un benchmark del arnes viejo no las tiene y su
        # linea sigue siendo valida. Aqui «ausente» y «cero» tienen que quedar
        # distintos: con cero, la barra de comparacion seria cero y saltaria
        # todo.
        if len(partes) >= 7:
            try:
                dato["dispersion"] = float(partes[3])
                dato["recorrido"] = float(partes[4])
                dato["iteraciones"] = int(partes[5])
                dato["repeticiones"] = int(partes[6])
            except ValueError:
                pass
        # LA COLA BAJA (desde el 27 sep). `suelo` es la media del 20 % mas bajo:
        # a diferencia del minimo, apunta al mismo cuantil con 10 vueltas y con
        # 25, asi que es lo comparable entre regimenes. `dispersion_baja` es lo
        # que se mueve ESA cola, que es la incertidumbre de la cifra que se
        # publica -- el recorrido mide la de arriba, que no le afecta.
        if len(partes) >= 11:
            try:
                dato["suelo"] = float(partes[7])
                dato["dispersion_baja"] = float(partes[8])
                dato["limpias"] = float(partes[9])
                dato["k_suelo"] = int(partes[10])
            except ValueError:
                pass
        # EL CUANDO (desde el 1 oct 2026, P2.22): el tramo de las vueltas
        # cronometradas, en segundos desde la epoca. Es lo que permite cruzar
        # cada ventana con lo que hizo la maquina mientras se media.
        if len(partes) >= 13:
            try:
                dato["t_inicio"] = float(partes[11])
                dato["t_fin"] = float(partes[12])
            except ValueError:
                pass
        medidas[partes[0]] = dato
    return medidas


def ejecutar(nombre: str, compilador: str, modo: str, tmp: Path, durante_out: dict = None):
    """Compila y ejecuta un benchmark, devolviendo sus medidas.

    Si se pasa `durante_out`, se rellena con lo que se pudo medir de la maquina
    mientras corria (P2.22): si estuvo disponible, por que no, cuantas muestras.
    """
    salida_tsv = tmp / ("%s.tsv" % nombre)
    if salida_tsv.exists():
        salida_tsv.unlink()

    r = subprocess.run([sys.executable, str(RAIZ / "make.py"), "build", "uint128",
                        nombre, "benchs", compilador, modo],
                       cwd=RAIZ, capture_output=True, text=True)
    if r.returncode != 0:
        return None, "no compila"

    exe = (RAIZ / "build" / "build_benchs" / rutas.plataforma() / compilador / modo /
           ("benchmark_%s_%s" % (nombre, compilador)))
    if not exe.exists():
        exe = Path(str(exe) + ".exe")
    if not exe.exists():
        return None, "sin binario"

    env = entorno_de(compilador)
    env["BENCH_OUT"] = str(salida_tsv)
    # `Popen` en vez de `run`: hace falta el PID para medir lo que hacen LOS DEMAS
    # mientras este corre (P2.22). El tiempo maximo y la captura de la salida son
    # los mismos que antes.
    proc = subprocess.Popen([str(exe)], cwd=RAIZ, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True, env=env)
    with Muestreador(proc.pid) as durante:
        try:
            salida, error = proc.communicate(timeout=1800)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.communicate()
            return None, "timeout"
    r = subprocess.CompletedProcess(proc.args, proc.returncode, salida, error)

    # ANTES no se miraba el codigo de salida. Un binario que moria al arrancar
    # --por una DLL equivocada, por ejemplo-- no escribia el TSV, y la unica
    # explicacion que daba este guion era "le falta bench_record": culpaba al
    # fuente de un fallo del entorno. Ahora se distingue.
    if r.returncode != 0:
        detalle = "0x%08X" % (r.returncode & 0xFFFFFFFF) if r.returncode < 0 or r.returncode > 255 \
            else str(r.returncode)
        pista = (r.stderr or r.stdout or "").strip().splitlines()
        return None, "el binario termino con %s%s" % (
            detalle, (": " + pista[-1][:60]) if pista else "")

    if not salida_tsv.exists():
        return None, "no registra (le falta bench_record)"

    with io.open(salida_tsv, encoding="utf-8", errors="replace") as f:
        medidas = lee_medidas(f)

    # CADA VENTANA, CON LO QUE HIZO LA MAQUINA MIENTRAS SE MEDIA (P2.22). Solo
    # las que traen sello; y si no se pudo medir durante, se dice en la suite en
    # vez de poner ceros, que serian una afirmacion falsa.
    for dato in medidas.values():
        if "t_inicio" in dato and durante.disponible:
            c = carga_en_tramo(durante.muestras, dato["t_inicio"], dato["t_fin"])
            if c is not None:
                dato["otros_media"], dato["otros_max"] = round(c[0], 3), round(c[1], 3)
                # EL SUELO es la cifra que decide (ver `suelo_en_tramo`); la media
                # y el maximo se guardan para poder diagnosticar, no para decidir.
                dato["otros_suelo"] = round(suelo_en_tramo(durante.muestras, dato["t_inicio"],
                                                           dato["t_fin"]), 3)
    # FUERA del diccionario de medidas, a proposito: quien lo recorre --`comparar`,
    # las sondas-- espera que cada valor sea una medida con su `valor`, y una
    # entrada de otra forma ahi dentro lo haria reventar.
    if durante_out is not None:
        durante_out.update({
            "disponible": durante.disponible,
            "motivo": durante.motivo,
            "periodo_s": durante.periodo,
            "muestras": len(durante.muestras),
        })
    return medidas, None


# ============================================================================
# LA VERSION DEL PROTOCOLO DE MEDIDA
# ============================================================================
#
# Dos tomas solo se pueden comparar si se midieron igual. El guion ya vigila el
# compilador y el numero de repeticiones; esto cubre el tercer caso, que es que
# cambie el ARNES.
#
# Paso el 30 sep 2026: `karatsuba`, `bases` y `curva_n` salieron del arnes viejo.
# El viejo media las variantes en ORDEN FIJO y con un numero fijo de iteraciones;
# el nuevo rota el orden y fija el TIEMPO por vuelta. Sin tocar una linea del
# codigo medido, `karatsuba N=3` paso de 9,86 a 19,13 cyc/op y seis casillas
# cruzaron el 1,00x. Comparar a traves de esa frontera no dice nada.
#
# Se sube este numero cada vez que cambie COMO se mide. Las tomas anteriores no
# traen el campo, y por eso `_protocolo` las nombra por omision.
PROTOCOLO = "2026-09-30/orden-rotando"

PROTOCOLO_VIEJO = "anterior al 30 sep 2026 (arnes viejo, orden fijo)"


def _protocolo(d: dict) -> str:
    return d.get("protocolo") or PROTOCOLO_VIEJO


def guardar(datos: dict) -> Path:
    carpeta = HISTORIA / datos["maquina"]
    carpeta.mkdir(parents=True, exist_ok=True)
    # EL COMPILADOR VA EN EL NOMBRE. Sin el, cuatro tomas del mismo dia y el
    # mismo commit --una por compilador, que es justo lo que pide P2.2-- se
    # pisaban unas a otras: el guion decia cuatro veces "70 medidas -> fichero"
    # y al final solo quedaban las del ultimo. Visto el 6 sep 2026.
    nombre = "%s-%s-%s.json" % (datos["fecha"][:10], datos["commit"],
                                datos["compilador_pedido"])
    destino = carpeta / nombre
    io.open(destino, "w", encoding="utf-8", newline="\n").write(
        json.dumps(datos, indent=2, ensure_ascii=False) + "\n")
    return destino


def anteriores(maquina: str):
    carpeta = HISTORIA / maquina
    if not carpeta.exists():
        return []
    return sorted(carpeta.glob("*.json"))


def comparar(actual: dict, previo_path: Path, motivo_referencia: str = None):
    previo = json.loads(io.open(previo_path, encoding="utf-8").read())
    echo("")
    echo("=" * 74)
    echo("  Comparacion con %s" % previo_path.name)
    echo("  (misma maquina: %s)" % actual["maquina"])
    echo("=" * 74)

    # LA REFERENCIA PUEDE NO SER DE FIAR, y antes no se decia (P2.22).
    if motivo_referencia:
        echo("  [OJO] esta referencia NO esta certificada: %s." % motivo_referencia)
        echo("        Medido el 1 oct 2026: un solo proceso compitiendo infla las cifras un")
        echo("        ~17 % sin que la dispersion lo vea. Si una casilla «mejora» aqui, puede")
        echo("        ser que la referencia estuviera inflada.")

    if previo.get("compilador") != actual.get("compilador"):
        echo("  [OJO] compilador distinto:")
        echo("        antes: %s" % previo.get("compilador"))
        echo("        ahora: %s" % actual.get("compilador"))
        echo("        las cifras NO son comparables; se muestran igual, pero no")
        echo("        se debe concluir nada de ellas.")

    # EL NUMERO DE REPETICIONES TAMBIEN ROMPE LA COMPARABILIDAD, y esto no lo
    # miraba nadie. Lo que se publica es el MINIMO: el de 25 muestras es
    # sistematicamente algo menor que el de 10, asi que subir las repeticiones
    # aparenta una mejora en TODAS las casillas a la vez. Se saca de las medidas,
    # que lo traen una por una, y no de ningun metadato.
    def _reps(d):
        return sorted({m.get("repeticiones") for s in d.get("suites", {}).values()
                       for m in s.values() if m.get("repeticiones")})

    r_antes, r_ahora = _reps(previo), _reps(actual)
    if r_antes and r_ahora and r_antes != r_ahora:
        echo("  [OJO] repeticiones por casilla distintas: %s -> %s"
             % (r_antes, r_ahora))
        echo("        Lo que se publica es el MINIMO, y el minimo de mas muestras")
        echo("        es menor por construccion. Una bajada general aqui NO es una")
        echo("        mejora: es el cambio de regimen. Ver REPETICIONES en")
        echo("        benchs/bench_adaptativo.hpp.")

    # LA CONFIGURACION DE LA CPU TAMBIEN (P2.24). Con el turbo encendido y con el
    # apagado las cifras no miden lo mismo: unas son ticks del TSC sensibles a la
    # carga y las otras ciclos de verdad.
    if huella_cpu(previo) != huella_cpu(actual):
        echo("  [OJO] configuracion de CPU distinta (driver, turbo, gobernador, SMT):")
        echo("        antes: %s" % (huella_cpu(previo) or "no registrada"))
        echo("        ahora: %s" % (huella_cpu(actual) or "no registrada"))
        echo("        Con el turbo encendido las cifras son ticks del TSC que una carga")
        echo("        ajena infla (P2.22); con el apagado, ciclos de verdad. No se")
        echo("        comparan sin decirlo.")

    # EL ARNES TAMBIEN ROMPE LA COMPARABILIDAD, y es el caso mas traicionero de
    # los tres: el compilador y las repeticiones se ven en los metadatos, pero un
    # cambio de arnes se parece a una regresion de verdad. El 30 sep movio
    # casillas un 94 % sin tocar el codigo medido.
    if _protocolo(previo) != _protocolo(actual):
        echo("  [OJO] protocolo de medida distinto:")
        echo("        antes: %s" % _protocolo(previo))
        echo("        ahora: %s" % _protocolo(actual))
        echo("        Las suites que cambiaron de arnes NO son comparables a traves")
        echo("        de esta frontera: lo que salte en ellas es cambio de protocolo,")
        echo("        no del codigo. Ver CHANGELOG, «el arnes viejo no medía lo que")
        echo("        decía» (P2.17).")

    avisos = 0
    todos = []      # |delta| de TODAS las comparables, para la distribucion
    por_criterio = {}
    # Para seguir viendo el CONTRASTE con el criterio viejo: cada entrada es
    # (|delta|, barra_en_uso, barra_del_recorrido).
    ensayo_baja = []
    # LAS PERTURBADAS SE APARTAN, de los dos lados: una cifra inflada por carga de
    # otros procesos no sirve ni de antes ni de despues, y su delta contaminaria
    # tambien la distribucion de abajo.
    apartadas = []
    for suite, medidas in sorted(actual["suites"].items()):
        antes = previo.get("suites", {}).get(suite, {})
        filas = []
        for caso, dato in sorted(medidas.items()):
            v_ahora = dato["valor"]
            if caso not in antes:
                continue
            if ventana_perturbada(dato) or ventana_perturbada(antes[caso]):
                apartadas.append((suite, caso, "ahora" if ventana_perturbada(dato) else "antes"))
                continue
            v_antes = antes[caso]["valor"]
            if v_antes == 0:
                continue
            delta = (v_ahora - v_antes) / v_antes
            barra, criterio = barra_de(antes[caso], dato)
            todos.append(abs(delta))
            por_criterio[criterio] = por_criterio.get(criterio, 0) + 1
            ra, rb = antes[caso].get("recorrido"), dato.get("recorrido")
            if criterio == "cola baja" and ra is not None and rb is not None:
                ensayo_baja.append((abs(delta), barra, max(SUELO_ENTRE_TOMAS, ra + rb)))
            if abs(delta) >= barra:
                filas.append((caso, v_antes, v_ahora, delta, barra, criterio))
        if filas:
            echo("")
            echo("  %s" % suite)
            for caso, va, vn, d, barra, criterio in filas:
                signo = "+" if d > 0 else ""
                echo("    %-34s %9.2f -> %9.2f  %s%.1f %%   (barra %.0f %%, %s)"
                     % (caso[:34], va, vn, signo, d * 100, barra * 100, criterio))
                avisos += 1

    if apartadas:
        echo("")
        echo("  %d casilla(s) APARTADAS por carga sostenida de otros procesos durante la"
             % len(apartadas))
        echo("  medida (suelo de `otros` > %.1f CPU). No se comparan:" % SUELO_PERTURBADA)
        for suite, caso, lado in apartadas[:12]:
            echo("    %-24s %-34s (%s)" % (suite[:24], caso[:34], lado))
        if len(apartadas) > 12:
            echo("    ... y %d mas" % (len(apartadas) - 12))

    # LA DISTRIBUCION, QUE ES LO QUE PERMITE CALIBRAR. Sin ella solo se ven las
    # que saltan, y no se sabe si saltan porque hay algo o porque la barra esta
    # mal puesta. Con dos tomas del mismo commit, todo lo de aqui es ruido.
    echo("")
    if todos:
        todos.sort()
        def pct(q):
            return todos[min(len(todos) - 1, int(q * len(todos)))] * 100
        detalle = ", ".join("%d por %s" % (n, c) for c, n in sorted(por_criterio.items()))
        echo("  %d medidas comparables (%s)" % (len(todos), detalle))
        echo("  cuanto se mueven:  mediana %.1f %%   p90 %.1f %%   peor %.1f %%"
             % (pct(0.5), pct(0.9), todos[-1] * 100))

    # LA BARRA DE COLA BAJA, MEDIDA Y TODAVIA SIN ACTIVAR.
    #
    # No se cambia el criterio a fe. Aqui se cuenta que pasaria con cada uno, y
    # con dos tomas del MISMO codigo todo lo que salte es falso positivo por
    # definicion. Cuando haya numeros, se decide -- igual que con el suelo.
    # EL CONTRASTE CON EL CRITERIO VIEJO. Se conserva porque es lo que justifica
    # el cambio cada vez que alguien lee una comparacion: no basta con que la
    # barra nueva sea mas estrecha, hay que ver CUANTO se dejaba pasar antes.
    if ensayo_baja:
        n = len(ensayo_baja)
        salta_ahora = sum(1 for d, ba, _ in ensayo_baja if d >= ba)
        salta_antes = sum(1 for d, _, br in ensayo_baja if d >= br)
        b_ahora = sorted(ba for _, ba, _ in ensayo_baja)
        b_antes = sorted(br for _, _, br in ensayo_baja)
        echo("")
        echo("  CONTRASTE con el criterio viejo, en las %d medidas con cola baja:" % n)
        echo("    barra de cola baja (en uso):   mediana %5.1f %%   marca %d"
             % (b_ahora[n // 2] * 100, salta_ahora))
        echo("    barra del recorrido (vieja):   mediana %5.1f %%   marca %d"
             % (b_antes[n // 2] * 100, salta_antes))

    echo("")
    if avisos:
        echo("  %d medida(s) pasan su barra." % avisos)
        echo("  Antes de concluir que hay una regresion, REPETIR LA MEDIDA: si el")
        echo("  mismo commit medido dos veces las mueve igual, es la maquina.")
    else:
        echo("  Ninguna medida pasa su barra.")


def main():
    ap = argparse.ArgumentParser(description="Historico de benchmarks")
    ap.add_argument("--compiler", default="gcc", help="gcc | clang | msvc | intel")
    ap.add_argument("--mode", default="release-O2")
    ap.add_argument("--only", action="append", help="solo estos benchmarks")
    ap.add_argument("--compare", action="store_true", help="comparar con la ejecucion anterior")
    ap.add_argument("--list", action="store_true", help="listar lo guardado")
    ap.add_argument("--espera-ocioso", nargs="?", type=float, const=60.0, default=None,
                    metavar="SEG",
                    help="esperar a que la maquina lleve SEG segundos tranquila antes de "
                         "medir (por defecto 60 si se pone el flag sin valor)")
    ap.add_argument("--umbral-carga", type=float, default=0.10,
                    help="que se considera tranquila, en tanto por uno (0,10)")
    ap.add_argument("--espera-max", type=float, default=1800.0,
                    help="cuanto esperar como maximo, en segundos (1800)")
    ap.add_argument("--sin-condiciones", action="store_true",
                    help="no comprobar el estado de la maquina (sondeadores vivos, "
                         "exclusiones de Defender, tareas programadas)")
    args = ap.parse_args()

    maquina = nombre_maquina()

    if args.list:
        for m in sorted(p.name for p in HISTORIA.glob("*") if p.is_dir()):
            echo("%s%s" % (m, "   <- esta maquina" if m == maquina else ""))
            for f in anteriores(m):
                d = json.loads(io.open(f, encoding="utf-8").read())
                n = sum(len(v) for v in d.get("suites", {}).values())
                echo("    %-30s %s  %d medidas" % (f.name, d.get("compilador", "?")[:40], n))
        return 0

    quiero = args.only or benchmarks_disponibles()
    tmp = RAIZ / "build" / "bench_tmp"
    tmp.mkdir(parents=True, exist_ok=True)

    datos = {
        "fecha": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "commit": commit_actual(),
        "arbol_limpio": arbol_limpio(),
        "maquina": maquina,
        "so": "%s %s" % (platform.system(), platform.release()),
        "cpu": platform.processor() or "desconocida",
        "compilador_pedido": args.compiler,
        "compilador": version_compilador(args.compiler),
        "modo": args.mode,
        "protocolo": PROTOCOLO,
        "suites": {},
    }

    echo("=" * 74)
    echo("  Historico de benchmarks")
    echo("=" * 74)
    echo("  maquina    : %s" % datos["maquina"])
    echo("  compilador : %s" % datos["compilador"])
    echo("  modo       : %s" % datos["modo"])
    echo("  commit     : %s%s" % (datos["commit"],
                                  "" if datos["arbol_limpio"] else "  (ARBOL SUCIO)"))
    if not datos["arbol_limpio"]:
        echo("  [OJO] el arbol tiene cambios sin commitear: esta medida no se puede")
        echo("        atribuir al commit de arriba.")
    echo("")
    # EN QUE ESTADO ESTABA LA MAQUINA, no solo cuanta carga tenia. Va ANTES de
    # la espera a proposito: ver `condiciones_de_medida`.
    if not args.sin_condiciones:
        cond = condiciones_de_medida()
        datos["condiciones"] = cond
        echo("  condiciones: %s" % cond.get("verdicto", "?"))
        vivos = cond.get("sondeadores_cuales")
        if vivos:
            echo("               sondeadores vivos: %s" % vivos[:96])
        if cond.get("sondeadores_sin_resolver") not in (None, "0"):
            echo("               [OJO] %s patron(es) de la lista no resuelven a ningun"
                 % cond["sondeadores_sin_resolver"])
            echo("               servicio: o ese software no esta instalado, o la lista de")
            echo("               condiciones_benchmark/ esta desfasada")
        echo("")

    # LA CONDICION, COMPROBADA EN VEZ DE RECORDADA. Y la carga queda anotada
    # en el JSON: sin ella, una toma sospechosa no se puede descartar despues
    # con ningun argumento.
    if args.espera_ocioso:
        echo("  Esperando a que la maquina lleve %.0f s por debajo del %.0f %% "
             "(maximo %.0f min)..." % (args.espera_ocioso, args.umbral_carga * 100,
                                       args.espera_max / 60.0))
        carga, esperado, logrado = espera_ocioso(args.umbral_carga, args.espera_ocioso,
                                                 args.espera_max)
        if carga is None:
            echo("  [OJO] no se pudo medir la carga; se sigue sin esperar.")
        elif logrado:
            echo("  Maquina tranquila AL EMPEZAR (%.0f %%) tras %.0f s de espera."
                 % (carga * 100, esperado))
            echo("  (Eso dice como esta ahora, no como va a estar: lo de durante se mide aparte.)")
        else:
            echo("  [OJO] se agoto la espera con la carga al %.0f %%. SE MIDE IGUAL,"
                 % (carga * 100))
            echo("        pero la cifra queda anotada en el JSON: esta toma es")
            echo("        sospechosa y se puede descartar por ese dato.")
        datos["carga_al_empezar"] = carga
        datos["espera_ocioso_s"] = round(esperado, 1)
        # «AL EMPEZAR», Y EL NOMBRE LO DICE. Hasta el 1 oct 2026 este campo se
        # llamaba `maquina_tranquila`, y afirmaba mas de lo que se comprobaba: lo
        # que mide es la media de carga ANTES de la primera suite, no durante la
        # toma. Dos tomas del mismo codigo, ambas con `True`, salieron con 7 y con
        # 46 ventanas sucias de 172 (P2.22). Lo que paso DURANTE la toma va en
        # `durante`, que se mide de verdad. Las tomas viejas conservan el nombre
        # viejo: el historico no se reescribe, pero su `True` significa esto.
        datos["tranquila_al_empezar"] = bool(logrado)
    else:
        c = carga_ahora(1.0)
        datos["carga_al_empezar"] = c
        if c is not None:
            echo("  carga ahora  : %.0f %%%s" % (c * 100,
                 "   <-- DEMASIADO ALTA, esta toma va a salir movida"
                 if c > 0.10 else ""))

    echo("  [OJO] LA MAQUINA TIENE QUE ESTAR OCIOSA mientras esto corre.")
    echo("        Compilar otra cosa a la vez, o cualquier carga de fondo, mueve")
    echo("        las cifras mas que casi cualquier cambio de codigo. Medido el")
    echo("        5 sep 2026: dos tomas del MISMO codigo, una de ellas con el")
    echo("        equipo compilando en paralelo, dieron diferencias de hasta un")
    echo("        52 %. Si has hecho algo mientras, esta medida no vale.")
    echo("")

    for nombre in quiero:
        print("  %-26s " % nombre, end="", flush=True)
        durante = datos.setdefault("durante", {}).setdefault(nombre, {})
        medidas, error = ejecutar(nombre, args.compiler, args.mode, tmp, durante)
        if error:
            echo("-- %s" % error)
            continue
        datos["suites"][nombre] = medidas
        echo("%d medidas" % len(medidas))

    total = sum(len(v) for v in datos["suites"].values())
    if total == 0:
        echo("")
        echo("  No se recogio ninguna medida. Nada que guardar.")
        return 1

    destino = guardar(datos)
    echo("")
    echo("  %d medidas de %d suites -> %s" % (total, len(datos["suites"]),
                                              destino.relative_to(RAIZ)))

    # EL VEREDICTO DE LO QUE PASO DURANTE, en voz alta (P2.22). Antes se escribia
    # `maquina_tranquila: True` mirando solo la espera previa, y dos tomas asi
    # marcadas salieron con 7 y con 46 ventanas sucias.
    v = veredicto_durante(datos)
    echo("")
    if not v["medido"]:
        echo("  [OJO] no se pudo medir lo que paso DURANTE la toma (%s)."
             % ", ".join(sorted({d.get("motivo") or "?" for d in datos.get("durante", {}).values()})))
        echo("        Esta toma NO se puede certificar como referencia.")
    elif v["certificable"]:
        echo("  Durante la toma: %d ventanas medidas, NINGUNA perturbada. Sirve de referencia."
             % v["ventanas"])
    else:
        echo("  [OJO] durante la toma: %d de %d ventanas PERTURBADAS por carga sostenida"
             % (v["perturbadas"], v["ventanas"]))
        echo("        de otros procesos%s:" % (" (y %d sin dato)" % v["sin_dato"] if v["sin_dato"] else ""))
        for suite, n in sorted(v["por_suite"].items()):
            echo("          %-26s %d" % (suite, n))
        echo("        Sus cifras pueden estar infladas (un solo proceso: ~+17 %, medido),")
        echo("        y la dispersion no lo delata. Esta toma no vale de referencia.")
        echo("        Y las ventanas justo DESPUES pueden arrastrar el efecto: tras una")
        echo("        carga fuerte el procesador tarda en recuperar la frecuencia.")

    if args.compare:
        # SOLO CONTRA EL MISMO COMPILADOR. Comparar la toma de gcc con la de
        # intel no dice nada de si el codigo ha cambiado: dice que son dos
        # compiladores distintos, que ya se sabia. Antes se cogia el fichero
        # anterior sin mirar, porque el nombre no llevaba el compilador y las
        # tomas de un mismo dia se pisaban entre si.
        previas = []
        for p in anteriores(maquina):
            if p == destino:
                continue
            try:
                d = json.loads(io.open(p, encoding="utf-8").read())
            except Exception:
                continue
            # los ficheros viejos, de antes de que el nombre llevara compilador,
            # llevan el dato dentro igualmente
            if d.get("compilador_pedido") == datos["compilador_pedido"]:
                previas.append(p)
        if previas:
            # LA REFERENCIA CERTIFICADA MAS RECIENTE, no la ultima sin mirar
            # (P2.22). Si ninguna lo es, la ultima, y se dice por que no se fia.
            tomas = [json.loads(io.open(q, encoding="utf-8").read()) for q in previas]
            i, motivo = elige_referencia(tomas, set(datos["suites"]), huella_cpu(datos))
            if i is None:
                echo("")
                echo("  No hay toma anterior de %s con estas suites (%s): nada con que"
                     % (datos["compilador_pedido"], ", ".join(sorted(datos["suites"]))))
                echo("  comparar. Esta es la base.")
            else:
                comparar(datos, previas[i], motivo)
        else:
            echo("")
            echo("  No hay ejecucion anterior de %s en esta maquina con la que"
                 % datos["compilador_pedido"])
            echo("  comparar. Esta es la base.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
