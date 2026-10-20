# -*- coding: utf-8 -*-
# =============================================================================
# comun.py - lo que comparten E1, E2 y E3 de la plataforma de medida dedicada
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Los tres experimentos validan las CONDICIONES de medida antes de la toma de
# referencia (NEXT_STEPS, «La plataforma de medida dedicada»). Lo que tienen en
# comun:
#
#   - fijar la CPU con `cpu_de_medida` y DEVOLVERLA al salir, pase lo que pase;
#   - quemadores: procesos que gastan una CPU entera cada uno, COMPROBADOS
#     (un arnes mio dio veredicto una vez con los quemadores muertos);
#   - compilar y leer `benchs/experimento_carga.cpp`.
#
# PARA PROBAR SIN LA MAQUINA: `CPU_DE_MEDIDA=<orden>` sustituye a
# `sudo -n /usr/local/sbin/cpu_de_medida` y se ejecuta SIN sudo. Con un guion
# falso permite comprobar la fontaneria en un contenedor o en WSL; los
# resultados de un experimento corrido asi no dicen nada de la CPU.
# =============================================================================

import os
import shlex
import subprocess
import sys
import time
from pathlib import Path

RAIZ = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(RAIZ / "scripts"))
import bench_history as bh  # noqa: E402  -- el de PRODUCCION, no una copia

# Donde lo deja `prepara_ubuntu.sh`, propiedad de root. NO el del repositorio:
# ese lo puede editar el usuario `bench`, y con `sudo` seria una puerta a root.
CPU_DE_MEDIDA = "/usr/local/sbin/cpu_de_medida"

BUCLE = "import time\nt=time.time()+%f\nwhile time.time()<t: pass\n"


def _orden_cpu(modo):
    falso = os.environ.get("CPU_DE_MEDIDA")
    if falso:
        return shlex.split(falso), True
    # `estado` no necesita root, y el sudoers de `prepara_ubuntu.sh` NO lo
    # permite: solo `fija`, `fija-con-turbo` y `restaura`.
    if modo == "estado":
        return [CPU_DE_MEDIDA], False
    return ["sudo", "-n", CPU_DE_MEDIDA], False


def cpu(modo):
    """`fija`, `fija-con-turbo`, `restaura` o `estado`. Devuelve la salida.

    Si `fija` no coge, sale con error y LO DICE: un turbo que no se pudo apagar
    no puede pasar por apagado. Por eso aqui se levanta excepcion y no se sigue.
    """
    orden, falso = _orden_cpu(modo)
    r = subprocess.run(orden + [modo], capture_output=True, text=True)
    salida = (r.stdout + r.stderr).strip()
    if r.returncode != 0:
        raise RuntimeError("cpu_de_medida %s fallo (rc=%d)%s:\n%s"
                           % (modo, r.returncode, " [FALSO]" if falso else "", salida))
    return salida


class CpuFijada:
    """`with CpuFijada("fija"):` -- y al salir, `restaura`, aunque algo falle."""

    def __init__(self, modo="fija"):
        self.modo = modo

    def __enter__(self):
        try:
            print(cpu(self.modo))
        except Exception:
            # Si `fija` fallo a medias, lo que si cogio hay que devolverlo.
            cpu("restaura")
            raise
        return self

    def __exit__(self, *exc):
        print(cpu("restaura"))
        return False


def turbo_ahora():
    """`apagado`, `encendido` o lo que diga `estado`."""
    for linea in cpu("estado").splitlines():
        partes = linea.split()
        if len(partes) >= 2 and partes[0] == "turbo":
            return partes[1]
    return "?"


def quemadores(k, segundos):
    """Lanza `k` procesos que gastan una CPU entera durante `segundos`."""
    return [subprocess.Popen([sys.executable, "-c", BUCLE % segundos]) for _ in range(k)]


def quema_lo_esperado(usos, n_cpu):
    """¿Gastan lo que deben? `usos`: % de una CPU de cada quemador. Funcion pura.

    Mientras quepan --uno por CPU, dejando una al banco--, cada uno tiene que
    gastar mas del 80 % de una CPU. SOBRESUSCRITOS, como en el calor de E2, a
    ninguno le toca una CPU entera, y exigirselo los daria por muertos sin
    estarlo: lo que se exige es que ENTRE TODOS ocupen lo que hay. Lo destapo E2
    el 20 oct 2026, en un contenedor y en WSL, en la fase de calor.
    """
    k = len(usos)
    if k == 0:
        return True
    libres = max(1, n_cpu - 1)
    if k <= libres:
        return all(u > 80.0 for u in usos)
    return sum(usos) >= 80.0 * libres


def queman(procs, n_cpu=None):
    """¿Estan vivos y gastando de verdad? Se mira durante un segundo."""
    import psutil  # noqa: PLC0415

    ps = [psutil.Process(p.pid) for p in procs]
    for q in ps:
        q.cpu_percent(None)
    time.sleep(1.0)
    usos = [q.cpu_percent(None) for q in ps]
    vivos = all(p.poll() is None for p in procs)
    return vivos and quema_lo_esperado(usos, n_cpu or cpus_en_linea()), usos


def carpeta_de_resultados():
    """`~/resultados/experimentos` (o `$RESULTADOS/experimentos`): FUERA del
    repositorio. Dentro, en la maquina de medida ensuciaria el clon de `bench`, y
    aqui acabaria en un commit."""
    base = Path(os.environ.get("RESULTADOS") or (Path.home() / "resultados"))
    carpeta = base / "experimentos"
    carpeta.mkdir(parents=True, exist_ok=True)
    return carpeta


def cpus_en_linea():
    """Las CPU que ve el sistema AHORA: con el SMT apagado, la mitad."""
    try:
        return len(os.sched_getaffinity(0))
    except AttributeError:
        return os.cpu_count() or 1


def compilador():
    """g++-14 en la plataforma (la serie de referencia); `GCC_CXX` manda."""
    return os.environ.get("GCC_CXX") or "g++-14"


def compila_carga(destino, repeticiones=None):
    """Compila `benchs/experimento_carga.cpp`. `repeticiones` solo para pruebas
    rapidas: cambia el regimen de medida y los resultados no son comparables."""
    orden = [compilador(), "-std=c++20", "-O2", "-I" + str(RAIZ / "include"), "-I" + str(RAIZ / "benchs"),
             str(RAIZ / "benchs" / "experimento_carga.cpp"), "-o", str(destino)]
    if repeticiones:
        orden.insert(1, "-DBENCH_REPETICIONES=%d" % repeticiones)
    r = subprocess.run(orden, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError("experimento_carga no compila:\n" + r.stderr[-2000:])


def lee_ventanas(tsv):
    """{ventana: {"t": (t0, t1), "mul": medida, "add": medida}} desde el TSV."""
    with open(tsv, encoding="utf-8") as f:
        medidas = bh.lee_medidas(f)
    ventanas = {}
    for caso, dato in medidas.items():
        if " / " not in caso or "t_inicio" not in dato:
            continue
        w, variante = caso.split(" / ", 1)
        v = ventanas.setdefault(w, {"t": (dato["t_inicio"], dato["t_fin"])})
        v[variante] = dato
    return ventanas


def cobertura(t0, t1, tramos):
    """{etiqueta: fraccion de [t0, t1] cubierta por cada tramo (desde, hasta, etiqueta)}."""
    out = {}
    for desde, hasta, etiqueta in tramos:
        s = min(t1, hasta) - max(t0, desde)
        if s > 0 and t1 > t0:
            out[etiqueta] = out.get(etiqueta, 0.0) + s / (t1 - t0)
    return out


def clasifica(t0, t1, tramos, lleno=0.98):
    """`limpia`, la etiqueta del tramo que la cubre entera, o `borde`."""
    c = cobertura(t0, t1, tramos)
    if not c:
        return "limpia"
    if len(c) == 1 and max(c.values()) >= lleno:
        return next(iter(c))
    return "borde"
