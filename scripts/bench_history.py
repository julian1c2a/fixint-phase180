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
    """Barra de esta medida, y con que criterio se decidio."""
    ra, rb = antes.get("recorrido"), ahora.get("recorrido")
    if ra is None or rb is None:
        return UMBRAL_PLANO, "plano"
    return max(SUELO_ENTRE_TOMAS, ra + rb), "propio"


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


def nombre_maquina() -> str:
    """Identifica la maquina. Dos maquinas distintas no comparten serie."""
    return platform.node() or "desconocida"


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
        return {"verdicto": "no aplica: no es Windows"}

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
    t0 = time.time()
    seguidos = 0.0
    ultima = None
    while time.time() - t0 < limite:
        c = carga_ahora(1.0)
        if c is None:
            return None, time.time() - t0, False
        ultima = c
        if c <= umbral:
            seguidos += 1.0
            if seguidos >= quieto:
                return c, time.time() - t0, True
        else:
            if seguidos > 0:
                echo("  [espera] la carga subio al %.0f %%; el contador vuelve a cero" % (c * 100))
            seguidos = 0.0
    return ultima, time.time() - t0, False


def benchmarks_disponibles():
    return sorted(f.stem[len("benchmark_"):]
                  for f in BENCHS.glob("benchmark_*.cpp"))


def ejecutar(nombre: str, compilador: str, modo: str, tmp: Path):
    """Compila y ejecuta un benchmark, devolviendo sus medidas."""
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
    try:
        r = subprocess.run([str(exe)], cwd=RAIZ, capture_output=True, text=True,
                           env=env, timeout=1800)
    except subprocess.TimeoutExpired:
        return None, "timeout"

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

    medidas = {}
    for linea in io.open(salida_tsv, encoding="utf-8", errors="replace"):
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
        medidas[partes[0]] = dato
    return medidas, None


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


def comparar(actual: dict, previo_path: Path):
    previo = json.loads(io.open(previo_path, encoding="utf-8").read())
    echo("")
    echo("=" * 74)
    echo("  Comparacion con %s" % previo_path.name)
    echo("  (misma maquina: %s)" % actual["maquina"])
    echo("=" * 74)

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

    avisos = 0
    todos = []      # |delta| de TODAS las comparables, para la distribucion
    con_propio = 0
    # Para decidir --midiendo-- si la barra debe salir de la cola baja en vez
    # del recorrido. Cada entrada: (|delta|, barra_recorrido, barra_cola_baja).
    ensayo_baja = []
    for suite, medidas in sorted(actual["suites"].items()):
        antes = previo.get("suites", {}).get(suite, {})
        filas = []
        for caso, dato in sorted(medidas.items()):
            v_ahora = dato["valor"]
            if caso not in antes:
                continue
            v_antes = antes[caso]["valor"]
            if v_antes == 0:
                continue
            delta = (v_ahora - v_antes) / v_antes
            barra, criterio = barra_de(antes[caso], dato)
            todos.append(abs(delta))
            if criterio == "propio":
                con_propio += 1
            db_a, db_b = antes[caso].get("dispersion_baja"), dato.get("dispersion_baja")
            if db_a is not None and db_b is not None:
                ensayo_baja.append((abs(delta), barra, db_a + db_b))
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

    # LA DISTRIBUCION, QUE ES LO QUE PERMITE CALIBRAR. Sin ella solo se ven las
    # que saltan, y no se sabe si saltan porque hay algo o porque la barra esta
    # mal puesta. Con dos tomas del mismo commit, todo lo de aqui es ruido.
    echo("")
    if todos:
        todos.sort()
        def pct(q):
            return todos[min(len(todos) - 1, int(q * len(todos)))] * 100
        echo("  %d medidas comparables (%d con su propio ruido, %d con el umbral plano)"
             % (len(todos), con_propio, len(todos) - con_propio))
        echo("  cuanto se mueven:  mediana %.1f %%   p90 %.1f %%   peor %.1f %%"
             % (pct(0.5), pct(0.9), todos[-1] * 100))

    # LA BARRA DE COLA BAJA, MEDIDA Y TODAVIA SIN ACTIVAR.
    #
    # No se cambia el criterio a fe. Aqui se cuenta que pasaria con cada uno, y
    # con dos tomas del MISMO codigo todo lo que salte es falso positivo por
    # definicion. Cuando haya numeros, se decide -- igual que con el suelo.
    if ensayo_baja:
        n = len(ensayo_baja)
        salta_rec = sum(1 for d, br, _ in ensayo_baja if d >= br)
        salta_baja = sum(1 for d, _, bb in ensayo_baja if d >= bb)
        bar_rec = sorted(br for _, br, _ in ensayo_baja)
        bar_baja = sorted(bb for _, _, bb in ensayo_baja)
        echo("")
        echo("  ENSAYO de la barra de cola baja (no esta activa; esto solo mide)")
        echo("    %d medidas traen dispersion de cola baja" % n)
        echo("    barra actual (suma de recorridos):   mediana %5.1f %%   saltan %d"
             % (bar_rec[n // 2] * 100, salta_rec))
        echo("    barra de cola baja:                  mediana %5.1f %%   saltan %d"
             % (bar_baja[n // 2] * 100, salta_baja))
        echo("    Con dos tomas del mismo codigo, lo que salte es falso positivo.")

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
            echo("               servicio: la lista de condiciones_benchmark/ esta desfasada")
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
            echo("  Maquina tranquila (%.0f %%) tras %.0f s de espera. Empezamos."
                 % (carga * 100, esperado))
        else:
            echo("  [OJO] se agoto la espera con la carga al %.0f %%. SE MIDE IGUAL,"
                 % (carga * 100))
            echo("        pero la cifra queda anotada en el JSON: esta toma es")
            echo("        sospechosa y se puede descartar por ese dato.")
        datos["carga_al_empezar"] = carga
        datos["espera_ocioso_s"] = round(esperado, 1)
        datos["maquina_tranquila"] = bool(logrado)
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
        medidas, error = ejecutar(nombre, args.compiler, args.mode, tmp)
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
            comparar(datos, previas[-1])
        else:
            echo("")
            echo("  No hay ejecucion anterior de %s en esta maquina con la que"
                 % datos["compilador_pedido"])
            echo("  comparar. Esta es la base.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
