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
from datetime import datetime, timezone
from pathlib import Path

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

# Umbral para avisar en --compare, MEDIDO el 5 sep 2026: el mismo binario en la
# misma maquina, dos ejecuciones seguidas sin tocar nada, da una mediana de
# 5,1 % de diferencia, un p90 de 15,6 % y un peor caso de 25,2 %.
#
# Por eso el umbral es 25 % y no el 5 % que parecia razonable a ojo: por debajo
# de eso serian todo falsos positivos. Un aviso que salta siempre no se mira.
#
# Sigue siendo provisional: sale de DOS ejecuciones, que es el minimo para tener
# un rango y muy poco para fiarse. Ver docs/PERFORMANCE.md, "Cuanto se mueven
# estas cifras".
UMBRAL_AVISO = 0.25


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
    try:
        r = subprocess.run(["git", "status", "--porcelain"],
                           cwd=RAIZ, capture_output=True, text=True)
        return r.stdout.strip() == ""
    except Exception:
        return False


def nombre_maquina() -> str:
    """Identifica la maquina. Dos maquinas distintas no comparten serie."""
    return platform.node() or "desconocida"


def version_compilador(compilador: str) -> str:
    sys.path.insert(0, str(RAIZ / "scripts"))
    try:
        import toolchains  # type: ignore
        cmd = toolchains.resolve(compilador)
    except Exception:
        cmd = compilador
    for flag in ("--version", "/?"):
        try:
            r = subprocess.run([cmd, flag], capture_output=True, text=True, timeout=20)
            salida = (r.stdout or r.stderr).strip().splitlines()
            if salida:
                return salida[0].strip()
        except Exception:
            continue
    return "desconocida"


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

    exe = RAIZ / "build" / "build_benchs" / compilador / modo / ("benchmark_%s_%s" % (nombre, compilador))
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
        if len(partes) >= 3:
            try:
                medidas[partes[0]] = {"valor": float(partes[1]), "unidad": partes[2]}
            except ValueError:
                pass
    return medidas, None


def guardar(datos: dict) -> Path:
    carpeta = HISTORIA / datos["maquina"]
    carpeta.mkdir(parents=True, exist_ok=True)
    nombre = "%s-%s.json" % (datos["fecha"][:10], datos["commit"])
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

    avisos = 0
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
            if abs(delta) >= UMBRAL_AVISO:
                filas.append((caso, v_antes, v_ahora, delta))
        if filas:
            echo("")
            echo("  %s" % suite)
            for caso, va, vn, d in filas:
                signo = "+" if d > 0 else ""
                echo("    %-38s %10.2f -> %10.2f  %s%.1f %%" % (caso[:38], va, vn, signo, d * 100))
                avisos += 1

    echo("")
    if avisos:
        echo("  %d medida(s) se mueven mas de un %.0f %%." % (avisos, UMBRAL_AVISO * 100))
        echo("  OJO: ese umbral NO esta calibrado. Antes de concluir que hay una")
        echo("  regresion, repetir la medida: el ruido de maquina puede dar mas.")
    else:
        echo("  Ninguna medida se mueve mas de un %.0f %%." % (UMBRAL_AVISO * 100))


def main():
    ap = argparse.ArgumentParser(description="Historico de benchmarks")
    ap.add_argument("--compiler", default="gcc", help="gcc | clang | msvc | intel")
    ap.add_argument("--mode", default="release-O2")
    ap.add_argument("--only", action="append", help="solo estos benchmarks")
    ap.add_argument("--compare", action="store_true", help="comparar con la ejecucion anterior")
    ap.add_argument("--list", action="store_true", help="listar lo guardado")
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
        previas = [p for p in anteriores(maquina) if p != destino]
        if previas:
            comparar(datos, previas[-1])
        else:
            echo("")
            echo("  No hay ejecucion anterior en esta maquina con la que comparar.")
            echo("  Esta es la base.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
