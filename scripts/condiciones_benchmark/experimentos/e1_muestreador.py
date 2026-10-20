#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
# E1 - ¿el muestreador mide lo que dice? Contra cantidades CONOCIDAS
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# El detector de P2.22 decide si una ventana esta perturbada con `otros`: la CPU
# de los DEMAS procesos, restando la del propio benchmark. En Windows se valido el
# 1 oct 2026 (±0,4 CPU frente a una suma independiente). En Linux la cuenta es
# otra --psutil sin `dpc`, la formula de `_cpu_ocupada_sistema`-- y solo tiene
# pruebas unitarias. Esto la valida en la maquina, antes de la toma de referencia.
#
# TODO SE MIDE POR DIFERENCIAS contra el fondo B de la propia prueba: comparar
# contra cero no vale, porque cualquier fondo lo confundiria con «no se resta al
# benchmark», que es JUSTO lo que hay que probar.
#
#     fase   el «benchmark»   quemadores   `otros` esperado
#     A      dormido          0            B          (el fondo)
#     B      quemando 1 CPU   0            B          <- si diera B+1, NO SE RESTA
#     C      quemando         c            B + c
#     D      quemando         d            B + d
#
# con d = CPU en linea - 2 (una para el benchmark y otra para el sistema) y c =
# d / 2. Con el SMT apagado en el Ryzen 7 5825U: 8 CPU, c = 3, d = 6.
#
# Uso, en la maquina de medida y como `bench`:
#
#     python3 e1_muestreador.py              con la CPU fijada (`fija`)
#     python3 e1_muestreador.py --sin-cpu    sin tocar la CPU (WSL, contenedores)
#
# Dura ~50 s. Sale con 0 si las tres fases cuadran, 1 si no, 2 si los
# quemadores no quemaban (sin cantidad conocida no hay prueba).
# =============================================================================

import subprocess
import sys
import threading
import time

import comun

DORMIDO, QUEMANDO = 10.0, 36.0
TOLERANCIA = {"B": 0.4, "C": 0.6, "D": 0.8}   # las de Windows, 1 oct 2026


def plan_de_fases(n_cpu):
    d = max(2, n_cpu - 2)
    c = max(1, d // 2)
    return [("A", 0.0, DORMIDO, 0), ("B", DORMIDO, 22.0, 0), ("C", 22.0, 34.0, c), ("D", 34.0, 46.0, d)]


def veredicto(medias):
    """{fase: (otros_medio, k)} -> (lineas, fallos). Funcion pura, para probarla."""
    fondo = medias["A"][0]
    lineas, fallos = [], 0
    for fase in "ABCD":
        media, k = medias[fase]
        dif = media - fondo
        if fase == "A":
            lineas.append((fase, media, dif, "(referencia: el fondo)"))
            continue
        ok = abs(dif - k) <= TOLERANCIA[fase]
        nota = "OK" if ok else "*** MAL ***"
        if fase == "B" and not ok and abs(dif - 1.0) < 0.4:
            nota += "  <- da B+1: NO SE RESTA AL BENCHMARK"
        fallos += 0 if ok else 1
        lineas.append((fase, media, dif, nota))
    return lineas, fallos


def main():
    sin_cpu = "--sin-cpu" in sys.argv
    print("=== E1: el muestreador, contra cantidades conocidas ===")

    def experimento():
        n_ahora = comun.cpus_en_linea()   # con el SMT ya apagado, si se fijo
        plan = plan_de_fases(n_ahora)
        print("  %d CPU en linea; quemadores: C=%d, D=%d" % (n_ahora, plan[2][3], plan[3][3]))
        falso = subprocess.Popen([sys.executable, "-c",
                                  "import time\ntime.sleep(%f)\nt=time.time()+%f\nwhile time.time()<t: pass\n"
                                  % (DORMIDO, QUEMANDO)])
        fases, bien = [], [True]
        with comun.bh.Muestreador(falso.pid, periodo=0.5) as m:
            t_ini = time.time()

            def lanza():
                for nombre, desde, hasta, k in plan:
                    while time.time() < t_ini + desde:
                        time.sleep(0.02)
                    t0 = time.time()
                    procs = comun.quemadores(k, hasta - desde)
                    if procs:
                        ok, usos = comun.queman(procs)
                        print("    fase %s: %d quemadores, uso %s %s"
                              % (nombre, k, ["%.0f%%" % u for u in usos], "OK" if ok else "*** NO QUEMAN ***"))
                        bien[0] &= ok
                    while time.time() < t_ini + hasta:
                        time.sleep(0.02)
                    for p in procs:
                        p.wait()
                    fases.append((nombre, t0, time.time(), k))

            h = threading.Thread(target=lanza)
            h.start()
            h.join()
            falso.wait()
        return m, fases, bien[0]

    if sin_cpu:
        m, fases, bien = experimento()
    else:
        with comun.CpuFijada("fija"):
            m, fases, bien = experimento()

    print("  muestras: %d, disponible=%s %s" % (len(m.muestras), m.disponible, m.motivo))
    if not m.disponible:
        return 1
    if not bien:
        print("  ABORTADO: algun quemador no quemaba; sin cantidad conocida no hay prueba.")
        return 2

    medias = {}
    for nombre, t0, t1, k in fases:
        c = comun.bh.carga_en_tramo(m.muestras, t0 + 1.5, t1 - 1.5)   # fuera los bordes
        medias[nombre] = (c[0] if c else float("nan"), k)

    lineas, fallos = veredicto(medias)
    print()
    print("  %-5s %8s  %10s   %s" % ("fase", "otros", "frente a A", "veredicto"))
    print("  " + "-" * 60)
    for fase, media, dif, nota in lineas:
        print("  %-5s %8.2f  %+10.2f   %s" % (fase, media, dif, nota))
    print()
    print("  %s" % ("CORRECTO: resta al benchmark y mide la competencia, por diferencias"
                    if not fallos else "%d fases fuera de lo esperado" % fallos))
    return 1 if fallos else 0


if __name__ == "__main__":
    sys.exit(main())
