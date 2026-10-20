#!/usr/bin/env python3
# =============================================================================
# test_experimentos.py - las cuentas de E1, E2 y E3, sin la maquina
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Los experimentos de `scripts/condiciones_benchmark/experimentos/` deciden si la
# plataforma de medida dedicada vale para la toma de referencia. Lo que ocurre en
# la maquina --quemadores, turbo, la toma-- no se puede probar aqui; lo que se
# hace con lo medido SI, y es donde un error pasaria sin que nadie lo viera:
# clasificar mal una ventana, no apartar una perturbada, dar por buena una fase.
# =============================================================================

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "condiciones_benchmark" / "experimentos"))
import comun  # noqa: E402
import e1_muestreador as e1  # noqa: E402
import e2_turbo as e2  # noqa: E402
import e3_orden as e3  # noqa: E402


class Cobertura(unittest.TestCase):

    TRAMOS = [(100.0, 200.0, "k=1"), (300.0, 400.0, "calor")]

    def test_dentro_fuera_y_a_caballo(self):
        self.assertEqual(comun.clasifica(120, 130, self.TRAMOS), "k=1")
        self.assertEqual(comun.clasifica(10, 20, self.TRAMOS), "limpia")
        self.assertEqual(comun.clasifica(195, 205, self.TRAMOS), "borde")

    def test_casi_entera_no_es_entera(self):
        # El 98 % es el corte: una ventana que pisa un 5 % fuera es de borde.
        self.assertEqual(comun.clasifica(99.5, 109.5, self.TRAMOS), "borde")


class Quemadores(unittest.TestCase):

    def test_mientras_caben_cada_uno_gasta_una_cpu(self):
        self.assertTrue(comun.quema_lo_esperado([99.0, 98.0, 99.0], 8))
        self.assertFalse(comun.quema_lo_esperado([99.0, 40.0, 99.0], 8), "uno no quema")

    def test_sobresuscritos_cuenta_lo_que_ocupan_entre_todos(self):
        # El calor de E2: 8 quemadores y el banco en 8 CPU. A ninguno le toca una
        # CPU entera (lo destapo un contenedor el 20 oct 2026), pero entre todos
        # tienen que ocupar las 7 que deja el banco.
        self.assertTrue(comun.quema_lo_esperado([72.0] * 8, 8))
        self.assertFalse(comun.quema_lo_esperado([30.0] * 8, 8), "medio muertos")


class E1Veredicto(unittest.TestCase):

    def test_el_plan_deja_cpu_al_banco_y_al_sistema(self):
        self.assertEqual([f[3] for f in e1.plan_de_fases(8)], [0, 0, 3, 6])
        self.assertEqual([f[3] for f in e1.plan_de_fases(16)], [0, 0, 7, 14])

    def test_todo_cuadra(self):
        _, fallos = e1.veredicto({"A": (0.20, 0), "B": (0.25, 0), "C": (3.1, 3), "D": (6.3, 6)})
        self.assertEqual(fallos, 0)

    def test_si_no_resta_el_banco_lo_dice(self):
        # LA QUE MAS IMPORTA: si `otros` incluyera al propio benchmark, la fase B
        # daria fondo + 1, y toda ventana saldria «perturbada» por si misma.
        lineas, fallos = e1.veredicto({"A": (0.2, 0), "B": (1.2, 0), "C": (4.2, 3), "D": (7.2, 6)})
        self.assertGreaterEqual(fallos, 1)
        self.assertIn("NO SE RESTA", dict((l[0], l[3]) for l in lineas)["B"])

    def test_se_mide_por_diferencias_no_contra_cero(self):
        # Con un fondo de una CPU entera, todo cuadra igual: se compara contra A.
        _, fallos = e1.veredicto({"A": (1.0, 0), "B": (1.1, 0), "C": (4.0, 3), "D": (7.1, 6)})
        self.assertEqual(fallos, 0)


def _medida(valor, otros=None):
    d = {"valor": valor, "suelo": valor}
    if otros is not None:
        d["otros_suelo"] = otros
    return d


class E2Resumen(unittest.TestCase):

    def test_clasifica_y_compara_con_las_limpias_del_principio(self):
        t0 = 1000.0
        tramos = [(t0 + 60, t0 + 150, "k=1")]
        ventanas = {
            "v00": {"t": (t0 + 5, t0 + 15), "mul": _medida(100.0), "add": _medida(10.0)},
            "v01": {"t": (t0 + 20, t0 + 30), "mul": _medida(100.0), "add": _medida(10.0)},
            "v06": {"t": (t0 + 70, t0 + 80), "mul": _medida(117.0), "add": _medida(11.7)},
            "v40": {"t": (t0 + 400, t0 + 410), "mul": _medida(105.0), "add": _medida(10.0)},
            "v46": {"t": (t0 + 450, t0 + 460), "mul": _medida(100.0), "add": _medida(10.0)},
        }
        r = e2.resume(ventanas, tramos, t0, 1.0)
        self.assertAlmostEqual(r["referencia"]["mul"], 0.0)
        self.assertAlmostEqual(r["k=1"]["mul"], 0.17)
        self.assertAlmostEqual(r["cola 1a mitad"]["mul"], 0.05)
        self.assertAlmostEqual(r["cola 2a mitad"]["mul"], 0.0)

    def test_sin_referencia_no_inventa(self):
        self.assertEqual(e2.resume({}, [], 0.0, 1.0), {})


class E3Compara(unittest.TestCase):

    def test_r_es_rotando_entre_azar_y_se_aparta_lo_perturbado(self):
        tomas = {
            "al_azar": [{"a": _medida(100.0, 0.1), "b": _medida(50.0, 0.1)},
                        {"a": _medida(101.0, 0.1), "b": _medida(50.0, 0.1)}],
            "rotando": [{"a": _medida(110.0, 0.1), "b": _medida(50.0, 0.9)},   # b perturbada
                        {"a": _medida(111.0, 0.1), "b": _medida(50.0, 0.1)}],
        }
        filas = {f["caso"]: f for f in e3.compara(tomas)}
        self.assertNotIn("b", filas, "una casilla con una ventana perturbada no se compara")
        self.assertAlmostEqual(filas["a"]["r"], 1.1, places=2)
        self.assertTrue(filas["a"]["fuera"], "un 10 % con ~0,5 % de ruido esta fuera")

    def test_con_un_solo_par_no_hay_ruido_y_nada_esta_fuera(self):
        # Lo destapo un contenedor el 20 oct 2026: con un par, cada modo tiene un
        # valor, el «ruido» salia 0 y TODAS las casillas quedaban fuera de el.
        tomas = {"al_azar": [{"a": _medida(100.0)}], "rotando": [{"a": _medida(110.0)}]}
        f = e3.compara(tomas)[0]
        self.assertIsNone(f["ruido"])
        self.assertFalse(f["fuera"])
        self.assertAlmostEqual(f["r"], 1.1)

    def test_sin_diferencia_no_esta_fuera(self):
        tomas = {"al_azar": [{"a": _medida(100.0)}, {"a": _medida(102.0)}],
                 "rotando": [{"a": _medida(101.0)}, {"a": _medida(100.0)}]}
        f = e3.compara(tomas)[0]
        self.assertFalse(f["fuera"])


if __name__ == "__main__":
    unittest.main()
