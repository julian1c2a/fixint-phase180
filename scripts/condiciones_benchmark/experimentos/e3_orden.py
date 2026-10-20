#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
# E3 - ¿cuanto pesaba el orden ROTANDO de las variantes?
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Hasta el 2 oct 2026 `mide_entrelazado` rotaba el orden una posicion por ronda:
# equilibraba el puesto, pero la variante j iba siempre detras de la j-1. Ahora
# es al azar en cada ronda (ver «El orden de las variantes» en
# benchs/bench_adaptativo.hpp). Que el azar es mas seguro se sabe por diseno;
# CUANTO SESGABA la rotacion no se sabe, y es lo que dice si las series de antes
# del 2 oct estan tocadas.
#
# EL METODO: el MISMO banco con los dos ordenes, ALTERNANDO y el mismo rato
# --memoria «A/B del mismo minuto»--. Cada par es una ejecucion con cada orden,
# y el que va primero se alterna de par en par. Para cada casilla:
#
#     r = suelo con el orden rotando / suelo con el orden al azar
#
# Si el vecino fijo no importa, r ~ 1 en todas. El ruido de referencia es lo que
# se mueve la MISMA casilla entre pares con el MISMO orden: una r que no lo supera
# no dice nada.
#
# QUE ORDEN SE APLICO NO SE SUPONE: cada toma deja su linea `#orden`, y si no es
# la pedida, el experimento se para.
#
# Uso, en la maquina de medida y como `bench`, con la CPU fijada:
#
#     python3 e3_orden.py                       vs_builtin, 3 pares (~80 min)
#     python3 e3_orden.py --suite karatsuba --pares 4
#     python3 e3_orden.py --sin-cpu             sin tocar la CPU (pruebas)
#
# Deja el detalle en `~/resultados/experimentos/e3_<suite>_<fecha>.json`, fuera del repositorio.
# =============================================================================

import argparse
import json
import os
import statistics
import sys
import tempfile
import time
from pathlib import Path

import comun


MODOS = ("al_azar", "rotando")


def una_toma(suite, modo, carpeta):
    """El banco con ese orden, por el camino de PRODUCCION (`ejecutar`)."""
    if modo == "rotando":
        os.environ["BENCH_ORDEN"] = "rotando"
    else:
        os.environ.pop("BENCH_ORDEN", None)
    durante, meta = {}, {}
    medidas, error = comun.bh.ejecutar(suite, "gcc", "release-O2", carpeta, durante, meta)
    if error:
        raise RuntimeError("%s (%s): %s" % (suite, modo, error))
    aplicado = (meta.get("orden") or "?").split(" ")[0]
    if aplicado != modo:
        raise RuntimeError("se pidio el orden %s y la toma dice %r" % (modo, meta.get("orden")))
    return medidas


def compara(tomas):
    """{modo: [medidas por par]} -> filas por casilla. Funcion pura.

    Se aparta toda casilla con alguna ventana perturbada (P2.22): una cifra
    inflada por carga ajena no sirve de ninguno de los dos lados.
    """
    filas = []
    for caso in sorted(tomas["al_azar"][0]):
        por_modo = {}
        for modo in MODOS:
            vals = []
            for medidas in tomas[modo]:
                d = medidas.get(caso)
                if not d or "suelo" not in d or comun.bh.ventana_perturbada(d):
                    vals = None
                    break
                vals.append(d["suelo"])
            por_modo[modo] = vals
        if not por_modo["al_azar"] or not por_modo["rotando"]:
            continue
        azar, rot = por_modo["al_azar"], por_modo["rotando"]
        r = [x / y for x, y in zip(rot, azar)]
        # El ruido: cuanto se aparta cada toma de la mediana de SU modo. CON UN
        # SOLO PAR NO HAY: cada modo tiene un valor y se aparta 0 de si mismo, y
        # entonces cualquier r saldria «fuera del ruido». Lo destapo la prueba
        # en un contenedor el 20 oct 2026: las ocho casillas de `nucleos`,
        # fuera, con «ruido 0,0 %».
        if len(azar) < 2:
            filas.append({"caso": caso, "r": statistics.median(r), "ruido": None, "fuera": False})
            continue
        ruido = max(abs(v / statistics.median(vs) - 1) for vs in (azar, rot) for v in vs)
        filas.append({"caso": caso, "r": statistics.median(r), "ruido": ruido,
                      "fuera": abs(statistics.median(r) - 1) > 2 * ruido})
    return filas


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--suite", default="vs_builtin")
    ap.add_argument("--pares", type=int, default=3)
    ap.add_argument("--sin-cpu", action="store_true")
    args = ap.parse_args()

    print("=== E3: el orden rotando frente al orden al azar, suite %s ===" % args.suite)
    if args.pares < 2:
        print("  [OJO] con un solo par no hay ruido con que comparar: las r se listan, pero")
        print("        ninguna se puede dar por fuera del ruido. Hacen falta 2 pares o mas.")
    os.environ.setdefault("GCC_CXX", comun.compilador())
    tomas = {m: [] for m in MODOS}

    def corre():
        with tempfile.TemporaryDirectory() as d:
            for par in range(args.pares):
                orden = MODOS if par % 2 == 0 else MODOS[::-1]
                for modo in orden:
                    print("  par %d, orden %s ..." % (par + 1, modo), flush=True)
                    tomas[modo].append(una_toma(args.suite, modo, Path(d)))

    if args.sin_cpu:
        corre()
    else:
        with comun.CpuFijada("fija"):
            corre()

    filas = compara(tomas)
    if not filas:
        print("  ninguna casilla comparable (todas perturbadas o sin suelo)")
        return 1
    rs = sorted(f["r"] for f in filas)
    fuera = [f for f in filas if f["fuera"]]
    print()
    print("  %d casillas comparables; r = rotando / al azar (suelo)" % len(filas))
    print("  r: minimo %.3f   mediana %.3f   maximo %.3f" % (rs[0], statistics.median(rs), rs[-1]))
    print("  fuera del ruido (|r-1| > 2 x ruido): %d" % len(fuera))
    for f in sorted(fuera, key=lambda f: -abs(f["r"] - 1))[:20]:
        print("    %-58s r=%.3f  ruido %.1f%%" % (f["caso"][:58], f["r"], 100 * f["ruido"]))
    salida = comun.carpeta_de_resultados() / ("e3_%s_%s.json" % (args.suite, time.strftime("%Y%m%d-%H%M%S")))
    salida.write_text(json.dumps({"suite": args.suite, "filas": filas}, indent=1),
                                            encoding="utf-8")
    print("\n  (detalle en %s)" % salida)
    return 0


if __name__ == "__main__":
    sys.exit(main())
