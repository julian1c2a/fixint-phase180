#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
# E2 - ¿apagar el turbo quita el efecto de la carga ajena? Y la cola termica
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Es el experimento que VALIDA LA PLATAFORMA (NEXT_STEPS, «La plataforma de
# medida dedicada»). En Windows, el 1 oct 2026 (P2.22), un solo proceso
# compitiendo inflo todas las cifras un 17 %: le quita turbo al nucleo y el TSC
# va a frecuencia fija. Con el turbo APAGADO el nucleo deberia ir a la frecuencia
# del TSC pase lo que pase, y la carga ajena dejaria de mover la medida.
#
# Y LA SEGUNDA MITAD DE P2.23: tras una carga fuerte, el chip tarda en recuperar
# la frecuencia (un +12 % en Windows, un solo dato). Con el turbo apagado tampoco
# deberia haber cola.
#
# PREDICCIONES, ESCRITAS ANTES DE CORRER (20 oct 2026):
#
#   P1  turbo ENCENDIDO: con 1 quemador las cifras suben (en Windows, +17 %).
#   P2  turbo APAGADO: con 1 quemador, dentro del ruido (|cambio| < 2 %).
#   P3  turbo ENCENDIDO: justo despues del calor, cola visible; apagado, no.
#   P4  con TODOS los nucleos ocupados menos el del banco, ni el turbo apagado
#       lo arregla del todo si comparten algo (cache L3, memoria). Puede que si.
#
# EL PLAN, igual en cada ejecucion, en segundos desde que arranca el banco:
#
#      0- 60  limpia            la referencia
#     60-150  1 quemador        P1/P2
#    150-210  limpia
#    210-300  n-1 quemadores    P4
#    300-330  limpia
#    330-390  CALOR: n quemadores (todas las CPU)
#    390-480  limpia            P3: la cola, si la hay
#
# Y las ejecuciones se ALTERNAN --apagado, encendido, apagado, encendido--, para
# que la deriva de la maquina caiga por igual en los dos modos.
#
# Uso, en la maquina de medida y como `bench` (unos 35 minutos):
#
#     python3 e2_turbo.py                 2 pares de ejecuciones
#     python3 e2_turbo.py --pares 3
#     python3 e2_turbo.py --rapido        solo fontaneria: plan / 10, 3 vueltas
#
# Deja el detalle en `~/resultados/experimentos/e2_<fecha>.json`, fuera del repositorio.
# =============================================================================

import argparse
import json
import os
import statistics
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

import comun


# (desde, hasta, etiqueta, cuantos) -- `cuantos` en funcion de n, las CPU en linea
PLAN = [(60, 150, "k=1", lambda n: 1),
        (210, 300, "k=n-1", lambda n: max(1, n - 1)),
        (330, 390, "calor", lambda n: n)]
DURACION = 480
COLA = (390, 480)          # tras el calor
REF = (0, 60)              # antes de nada


def resume(ventanas, tramos, t_ini, escala):
    """Cambio relativo de cada tipo de ventana frente a las limpias del principio.

    Funcion pura: recibe lo leido y los tramos REALES (con su hora), y devuelve
    {tipo: {"n", "mul", "add"}} con medianas. La cola se separa en dos mitades
    para ver si decae.
    """
    def tipo(t0, t1):
        clase = comun.clasifica(t0, t1, tramos)
        if clase != "limpia":
            return clase
        rel0, rel1 = (t0 - t_ini) / escala, (t1 - t_ini) / escala
        if rel1 <= REF[1]:
            return "referencia"
        if COLA[0] <= rel0:
            return "cola 1a mitad" if rel1 <= (COLA[0] + COLA[1]) / 2 else "cola 2a mitad"
        return "limpia"

    por_tipo = {}
    for d in ventanas.values():
        if "mul" in d and "add" in d:
            por_tipo.setdefault(tipo(*d["t"]), []).append(d)
    if "referencia" not in por_tipo:
        return {}
    ref = {v: statistics.median(d[v]["valor"] for d in por_tipo["referencia"]) for v in ("mul", "add")}
    out = {}
    for t, ds in por_tipo.items():
        out[t] = {"n": len(ds),
                  "mul": statistics.median(d["mul"]["valor"] for d in ds) / ref["mul"] - 1,
                  "add": statistics.median(d["add"]["valor"] for d in ds) / ref["add"] - 1}
    return out


def una_ejecucion(exe, modo, escala, carpeta, s_por_ventana):
    """Una pasada del plan con la CPU en `modo`. Devuelve el resumen."""
    tsv = carpeta / ("e2_%s_%d.tsv" % (modo, int(time.time())))
    ventanas_pedidas = int(DURACION * escala / s_por_ventana) + 1
    env = dict(os.environ, BENCH_OUT=str(tsv), VENTANAS=str(ventanas_pedidas))
    with comun.CpuFijada(modo):
        turbo = comun.turbo_ahora()
        n = comun.cpus_en_linea()
        proc = subprocess.Popen([str(exe)], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, env=env)
        t_ini = time.time()
        tramos, quemaron = [], []

        def plan():
            for desde, hasta, etiqueta, cuantos in PLAN:
                while time.time() < t_ini + desde * escala:
                    if proc.poll() is not None:
                        return
                    time.sleep(0.05)
                procs = comun.quemadores(cuantos(n), (hasta - desde) * escala)
                t_on = time.time()
                ok, _ = comun.queman(procs, n)
                quemaron.append((etiqueta, ok))
                for p in procs:
                    p.wait()
                tramos.append((t_on, time.time(), etiqueta))

        with comun.bh.Muestreador(proc.pid, periodo=0.5) as m:
            h = threading.Thread(target=plan, daemon=True)
            h.start()
            _, error = proc.communicate()
            h.join(timeout=5)
    if proc.returncode != 0:
        raise RuntimeError("experimento_carga termino con %d: %s" % (proc.returncode, error[-500:]))
    ventanas = comun.lee_ventanas(tsv)
    r = resume(ventanas, tramos, t_ini, escala)
    return {"modo": modo, "turbo": turbo, "cpus": n, "quemaron": quemaron, "resumen": r,
            "muestras": len(m.muestras), "tramos": tramos}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pares", type=int, default=2)
    ap.add_argument("--rapido", action="store_true", help="solo fontaneria: plan / 10 y 3 vueltas")
    args = ap.parse_args()
    escala = 0.1 if args.rapido else 1.0
    # Lo que dura una ventana: 2 variantes x 25 vueltas x 200 ms, mas la
    # calibracion. Con 3 vueltas (--rapido), poco mas de un segundo y medio.
    s_por_ventana = 1.6 if args.rapido else 10.5

    print("=== E2: turbo encendido y apagado, con carga conocida ===")
    with tempfile.TemporaryDirectory() as d:
        carpeta = Path(d)
        exe = carpeta / "experimento_carga"
        comun.compila_carga(exe, repeticiones=3 if args.rapido else None)
        ejecuciones = []
        for par in range(args.pares):
            for modo in ("fija", "fija-con-turbo"):
                print("\n  par %d, %s ..." % (par + 1, modo), flush=True)
                e = una_ejecucion(exe, modo, escala, carpeta, s_por_ventana)
                ejecuciones.append(e)
                print("    turbo %s, %d CPU, quemadores bien: %s"
                      % (e["turbo"], e["cpus"], all(ok for _, ok in e["quemaron"])))

    malos = [e for e in ejecuciones if not all(ok for _, ok in e["quemaron"])]
    esperado = {"fija": "apagado", "fija-con-turbo": "encendido"}
    mal_turbo = [e for e in ejecuciones if e["turbo"] != esperado[e["modo"]]]

    tipos = ["referencia", "k=1", "limpia", "k=n-1", "calor", "cola 1a mitad", "cola 2a mitad"]
    print()
    print("  %-14s %22s   %22s" % ("", "turbo APAGADO (mul / add)", "turbo ENCENDIDO (mul / add)"))
    print("  " + "-" * 64)
    for t in tipos:
        fila = []
        for modo in ("fija", "fija-con-turbo"):
            vals = [e["resumen"][t] for e in ejecuciones if e["modo"] == modo and t in e["resumen"]]
            if vals:
                fila.append("%+6.1f%% / %+6.1f%%" % (100 * statistics.median(v["mul"] for v in vals),
                                                    100 * statistics.median(v["add"] for v in vals)))
            else:
                fila.append("-")
        print("  %-14s %22s   %22s" % (t, fila[0], fila[1]))

    salida = comun.carpeta_de_resultados() / ("e2_%s.json" % time.strftime("%Y%m%d-%H%M%S"))
    salida.write_text(json.dumps(ejecuciones, indent=1, default=str), encoding="utf-8")
    print("\n  (detalle en %s)" % salida)
    if args.rapido:
        print("  --rapido: solo comprueba la fontaneria; estas cifras NO dicen nada.")
    if malos:
        print("  [OJO] en %d ejecuciones algun quemador no quemaba." % len(malos))
    if mal_turbo:
        print("  [OJO] en %d ejecuciones el turbo no estaba como se pidio." % len(mal_turbo))
    return 1 if (malos or mal_turbo) else 0


if __name__ == "__main__":
    sys.exit(main())
