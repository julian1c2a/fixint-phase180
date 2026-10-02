#!/usr/bin/env python3
# =============================================================================
# test_bench_history.py - pruebas de la infraestructura que decide si una toma
#                         de benchmarks es de fiar
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# Por que existe
# --------------
# Hasta el 1 oct 2026 ningun guion de Python del proyecto tenia pruebas, y el CI
# no corria ninguna. Y es justo esta infraestructura la que decide que cifras se
# guardan y contra cuales se compara: si miente, la biblioteca parece regresar o
# mejorar sin haber cambiado. La norma del autor es que **toda infraestructura es
# primaria**, con el mismo rigor que el codigo de la biblioteca.
#
# Se prueban las funciones PURAS. Lo que depende de psutil y de la maquina --el
# Muestreador de verdad-- se valida aparte contra una carga conocida (ver P2.22
# en NEXT_STEPS), y aqui solo se comprueba que, si psutil no esta, lo dice en vez
# de fingir que no hubo carga.
#
# Uso:  python -m unittest discover -s scripts/tests
# =============================================================================

import sys
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import bench_history as bh  # noqa: E402


class LeeMedidas(unittest.TestCase):
    """El lector del TSV, que ha crecido de 3 columnas a 7, 11 y 13."""

    def test_tres_columnas_el_arnes_viejo(self):
        m = bh.lee_medidas(["suma\t2.5000\tcyc/op\n"])
        self.assertEqual(m["suma"], {"valor": 2.5, "unidad": "cyc/op"})

    def test_ausente_no_es_cero(self):
        # LA REGLA QUE MAS IMPORTA. Una linea sin las columnas del ruido no puede
        # salir con dispersion 0: con cero, la barra de comparacion seria cero y
        # cualquier movimiento saltaria como regresion.
        m = bh.lee_medidas(["suma\t2.5\tcyc/op\n"])["suma"]
        for campo in ("dispersion", "recorrido", "suelo", "dispersion_baja", "t_inicio"):
            self.assertNotIn(campo, m, "%s tiene que estar AUSENTE, no a cero" % campo)

    def test_siete_columnas(self):
        m = bh.lee_medidas(["x\t3.0\tcyc/op\t0.01\t0.05\t1000\t25\n"])["x"]
        self.assertEqual((m["dispersion"], m["recorrido"], m["iteraciones"], m["repeticiones"]),
                         (0.01, 0.05, 1000, 25))
        self.assertNotIn("suelo", m)

    def test_once_columnas_la_cola_baja(self):
        m = bh.lee_medidas(["x\t3.0\tcyc/op\t0.01\t0.05\t1000\t25\t3.02\t0.007\t0.6\t5\n"])["x"]
        self.assertEqual((m["suelo"], m["dispersion_baja"], m["limpias"], m["k_suelo"]),
                         (3.02, 0.007, 0.6, 5))
        self.assertNotIn("t_inicio", m)

    def test_trece_columnas_los_sellos(self):
        linea = "x\t3.0\tcyc/op\t0.01\t0.05\t1000\t25\t3.02\t0.007\t0.6\t5\t1759312345.1234\t1759312355.5678\n"
        m = bh.lee_medidas([linea])["x"]
        self.assertAlmostEqual(m["t_inicio"], 1759312345.1234, places=4)
        self.assertAlmostEqual(m["t_fin"], 1759312355.5678, places=4)
        self.assertEqual(m["k_suelo"], 5)  # y lo de antes sigue en su sitio

    def test_un_sello_roto_no_se_lleva_lo_demas(self):
        linea = "x\t3.0\tcyc/op\t0.01\t0.05\t1000\t25\t3.02\t0.007\t0.6\t5\tbasura\t1759312355.0\n"
        m = bh.lee_medidas([linea])["x"]
        self.assertNotIn("t_inicio", m)
        self.assertEqual(m["dispersion_baja"], 0.007)

    def test_lineas_que_no_son_medidas(self):
        m = bh.lee_medidas(["\n", "solo\tdos\n", "x\tno_es_numero\tcyc/op\n", "y\t1.0\tx\n"])
        self.assertEqual(list(m), ["y"])

    def test_los_cuatro_formatos_conviven_en_un_fichero(self):
        # Un mismo benchmark puede mezclar medidas del arnes adaptativo (13) con
        # cocientes derivados (3): las dos tienen que leerse en el mismo pase.
        m = bh.lee_medidas([
            "a\t1.0\tcyc/op\n",
            "b\t1.0\tcyc/op\t0.01\t0.05\t1000\t25\n",
            "c\t1.0\tcyc/op\t0.01\t0.05\t1000\t25\t1.0\t0.0\t1.0\t5\n",
            "d\t1.0\tcyc/op\t0.01\t0.05\t1000\t25\t1.0\t0.0\t1.0\t5\t100.0\t110.0\n",
        ])
        self.assertEqual([len(m[k]) for k in "abcd"], [2, 6, 10, 12])


class CargaEnTramo(unittest.TestCase):
    """Cruza las muestras de `otros` con el tramo de una ventana."""

    def test_sin_muestras_es_no_lo_se(self):
        self.assertIsNone(bh.carga_en_tramo([], 0.0, 10.0))

    def test_muestras_fuera_del_tramo(self):
        self.assertIsNone(bh.carga_en_tramo([(20.0, 21.0, 5.0)], 0.0, 10.0))

    def test_tocar_el_borde_no_es_solapar(self):
        # Una muestra que acaba justo donde empieza el tramo no estuvo en el.
        self.assertIsNone(bh.carga_en_tramo([(9.0, 10.0, 5.0)], 10.0, 20.0))

    def test_la_carga_del_borde_no_se_cuela_en_la_ventana_vecina(self):
        # LA CONSECUENCIA DE VERDAD, y la prueba de arriba no la veia. Si una
        # muestra que solo toca el borde no se descartara, su carga entraria en el
        # MAXIMO de la ventana de al lado. Con una sola muestra el guardia final
        # (`peso <= 0`) lo tapaba y daba None igual: se vio rompiendo el codigo a
        # proposito el 1 oct 2026. Hace falta una muestra de verdad dentro.
        muestras = [(9.0, 10.0, 8.0), (10.0, 11.0, 1.0)]
        self.assertEqual(bh.carga_en_tramo(muestras, 10.0, 20.0), (1.0, 1.0))

    def test_una_muestra_dentro(self):
        self.assertEqual(bh.carga_en_tramo([(2.0, 3.0, 4.0)], 0.0, 10.0), (4.0, 4.0))

    def test_la_media_pondera_por_solape(self):
        # 9 s sin nadie y 1 s con 10 CPU ocupadas: la media es 1, no 5.
        muestras = [(0.0, 9.0, 0.0), (9.0, 10.0, 10.0)]
        media, maximo = bh.carga_en_tramo(muestras, 0.0, 10.0)
        self.assertAlmostEqual(media, 1.0)
        self.assertEqual(maximo, 10.0)

    def test_la_rafaga_la_ve_el_maximo_aunque_la_media_la_diluya(self):
        # LO QUE JUSTIFICA GUARDAR LOS DOS. Una rafaga corta en una ventana larga
        # casi no mueve la media; el maximo la ve entera.
        muestras = [(float(t), float(t + 1), 0.0) for t in range(20)]
        muestras[7] = (7.0, 8.0, 8.0)
        media, maximo = bh.carga_en_tramo(muestras, 0.0, 20.0)
        self.assertLess(media, 0.5)
        self.assertEqual(maximo, 8.0)

    def test_solape_parcial_en_los_dos_bordes(self):
        # Medio segundo de cada muestra cae dentro del tramo.
        muestras = [(-0.5, 0.5, 2.0), (9.5, 10.5, 6.0)]
        media, _ = bh.carga_en_tramo(muestras, 0.0, 10.0)
        self.assertAlmostEqual(media, 4.0)


class SueloEnTramo(unittest.TestCase):
    """El percentil bajo de `otros`: la cifra que decide si una ventana se perturbo."""

    def test_sin_muestras_es_no_lo_se(self):
        self.assertIsNone(bh.suelo_en_tramo([], 0.0, 10.0))

    def test_carga_sostenida_tiene_el_suelo_alto(self):
        muestras = [(float(t), float(t + 1), 1.2) for t in range(10)]
        self.assertAlmostEqual(bh.suelo_en_tramo(muestras, 0.0, 10.0), 1.2)

    def test_las_rafagas_no_levantan_el_suelo(self):
        # LO QUE SEPARA LO QUE DANA DE LO QUE NO (E3, 1 oct 2026). Dos muestras de
        # cada diez con carga: la media sube a 1,6 pero el suelo se queda en el
        # fondo -- y las rafagas no movieron el minimo.
        muestras = [(float(t), float(t + 1), 0.1) for t in range(10)]
        muestras[3] = (3.0, 4.0, 8.0)
        muestras[7] = (7.0, 8.0, 8.0)
        media, _ = bh.carga_en_tramo(muestras, 0.0, 10.0)
        self.assertGreater(media, 1.5)
        self.assertLess(bh.suelo_en_tramo(muestras, 0.0, 10.0), 0.2)

    def test_solo_cuentan_las_que_solapan(self):
        muestras = [(0.0, 1.0, 9.0), (10.0, 11.0, 0.1), (11.0, 12.0, 0.1)]
        self.assertAlmostEqual(bh.suelo_en_tramo(muestras, 10.0, 12.0), 0.1)


class VentanaPerturbada(unittest.TestCase):

    def test_tres_respuestas_y_la_tercera_no_es_false(self):
        self.assertTrue(bh.ventana_perturbada({"otros_suelo": 1.1}))
        self.assertFalse(bh.ventana_perturbada({"otros_suelo": 0.3}))
        # Una toma de antes del 1 oct 2026 no lo sabe. Eso NO es «limpia».
        self.assertIsNone(bh.ventana_perturbada({"valor": 3.0}))

    def test_el_umbral_esta_entre_lo_inocuo_y_lo_que_dana(self):
        # Lo medido: inocuas <= 0,30, un proceso sostenido >= 1,10.
        self.assertGreater(bh.SUELO_PERTURBADA, 0.30)
        self.assertLess(bh.SUELO_PERTURBADA, 1.10)


def _toma(*ventanas):
    """Una toma minima: cada argumento es (suite, t_inicio, [suelos de sus variantes])."""
    suites = {}
    for i, (suite, t0, suelos) in enumerate(ventanas):
        for j, suelo in enumerate(suelos):
            dato = {"valor": 1.0, "t_inicio": t0, "t_fin": t0 + 10.0}
            if suelo is not None:
                dato["otros_suelo"] = suelo
            suites.setdefault(suite, {})["w%d / v%d" % (i, j)] = dato
    return {"suites": suites}


class VeredictoDurante(unittest.TestCase):

    def test_cuenta_ventanas_no_casillas(self):
        # LECCION DE P2.18: las variantes de una ventana comparten sello y son UN
        # suceso. Una ventana perturbada con tres variantes es UNA, no tres.
        v = bh.veredicto_durante(_toma(("s", 0.0, [1.5, 1.5, 1.5]), ("s", 20.0, [0.1, 0.1, 0.1])))
        self.assertEqual((v["ventanas"], v["perturbadas"]), (2, 1))

    def test_limpia_y_medida_es_certificable(self):
        v = bh.veredicto_durante(_toma(("a", 0.0, [0.1, 0.2]), ("b", 20.0, [0.3])))
        self.assertTrue(v["certificable"])

    def test_una_sola_perturbada_basta_para_no_certificar(self):
        v = bh.veredicto_durante(_toma(("a", 0.0, [0.1]), ("b", 20.0, [0.9])))
        self.assertFalse(v["certificable"])
        self.assertEqual(v["por_suite"], {"b": 1})

    def test_sin_medida_de_durante_no_es_certificable(self):
        # Las tomas de antes del 1 oct 2026. «No lo se» no puede valer de «limpia».
        v = bh.veredicto_durante(_toma(("a", 0.0, [None, None])))
        self.assertFalse(v["medido"])
        self.assertFalse(v["certificable"])

    def test_un_hueco_sin_dato_tampoco_certifica(self):
        v = bh.veredicto_durante(_toma(("a", 0.0, [0.1]), ("b", 20.0, [None])))
        self.assertEqual(v["sin_dato"], 1)
        self.assertFalse(v["certificable"])

    def test_los_cocientes_sin_sello_no_cuentan(self):
        toma = _toma(("a", 0.0, [0.1]))
        toma["suites"]["a"]["razon"] = {"valor": 1.2, "unidad": "x"}
        self.assertEqual(bh.veredicto_durante(toma)["ventanas"], 1)


class EligeReferencia(unittest.TestCase):

    def test_prefiere_la_certificada_aunque_no_sea_la_ultima(self):
        limpia = _toma(("a", 0.0, [0.1]))
        sucia = _toma(("a", 0.0, [2.0]))
        self.assertEqual(bh.elige_referencia([limpia, sucia]), (0, None))

    def test_la_ultima_certificada_si_hay_varias(self):
        limpia = _toma(("a", 0.0, [0.1]))
        self.assertEqual(bh.elige_referencia([limpia, limpia, limpia])[0], 2)

    def test_sin_ninguna_certificada_dice_por_que(self):
        vieja = _toma(("a", 0.0, [None]))
        i, motivo = bh.elige_referencia([vieja])
        self.assertEqual(i, 0)
        self.assertIn("DURANTE", motivo)
        i, motivo = bh.elige_referencia([_toma(("a", 0.0, [3.0]))])
        self.assertIn("perturbada", motivo)

    def test_sin_tomas(self):
        self.assertEqual(bh.elige_referencia([])[0], None)

    def test_la_referencia_tiene_que_tener_la_suite(self):
        # LO QUE DESTAPO LA PRUEBA DE EXTREMO A EXTREMO: con `--only bases` se
        # cogio una toma sin `bases` y la comparacion salio vacia.
        con_bases = {"suites": {"bases": {"x": {"valor": 1.0, "t_inicio": 0.0, "t_fin": 10.0,
                                                "otros_suelo": 0.1}}}}
        sin_bases = _toma(("karatsuba", 0.0, [0.1]))      # certificada, pero sin bases
        self.assertEqual(bh.elige_referencia([con_bases, sin_bases], {"bases"}), (0, None))

    def test_prefiere_la_que_tiene_todas_las_suites(self):
        dos = {"suites": {"a": {}, "b": {}}}
        una = {"suites": {"a": {}}}
        self.assertEqual(bh.elige_referencia([dos, una], {"a", "b"})[0], 0)
        # y si ninguna las tiene todas, vale la que tenga alguna
        self.assertEqual(bh.elige_referencia([una], {"a", "b"})[0], 0)

    def test_ninguna_con_la_suite(self):
        i, motivo = bh.elige_referencia([_toma(("a", 0.0, [0.1]))], {"zzz"})
        self.assertIsNone(i)
        self.assertIn("suites", motivo)


class IdentidadDeLaSerie(unittest.TestCase):
    """Que Windows, WSL y un Linux nativo en la MISMA maquina no compartan serie."""

    def test_windows_conserva_el_nombre_a_secas(self):
        # La carpeta `MSI/` ya existe con todo el historico: no se puede romper.
        self.assertEqual(bh.nombre_maquina("MSI", "windows"), "MSI")

    def test_linux_y_wsl_llevan_el_sistema_detras(self):
        self.assertEqual(bh.nombre_maquina("MSI", "linux"), "MSI-linux")
        self.assertEqual(bh.nombre_maquina("MSI", "wsl"), "MSI-wsl")

    def test_los_tres_son_series_distintas_con_el_mismo_host(self):
        nombres = {bh.nombre_maquina("MSI", s) for s in ("windows", "wsl", "linux")}
        self.assertEqual(len(nombres), 3)

    def test_wsl_se_reconoce_por_proc_version(self):
        wsl = "Linux version 6.6.87.2-microsoft-standard-WSL2 (gcc 11.2.0)"
        nativo = "Linux version 6.8.0-45-generic (buildd@lcy02-amd64-075) (gcc-13)"
        self.assertEqual(bh.sistema("linux", wsl), "wsl")
        self.assertEqual(bh.sistema("linux", nativo), "linux")
        self.assertEqual(bh.sistema("windows", "lo que sea"), "windows")


class CuentaDobleDeWindows(unittest.TestCase):
    """La cuenta doble de psutil se decide por la forma del dato, no por el sistema."""

    def test_con_dpc_no_se_suma_aparte(self):
        # El backend de Windows: `system` ya incluye interrupciones y DPC.
        t = type("T", (), {"user": 1.0, "system": 2.0, "idle": 9.0, "interrupt": 0.5, "dpc": 0.5})()
        falso = type("P", (), {"cpu_times": staticmethod(lambda: t)})
        self.assertAlmostEqual(bh._cpu_ocupada_sistema(falso), 3.0)

    def test_sin_dpc_vale_la_formula_de_psutil(self):
        # El de Linux: total menos ocioso y iowait, sin `guest`, que va en `user`.
        import collections
        T = collections.namedtuple("T", "user nice system idle iowait irq softirq steal guest guest_nice")
        t = T(1.0, 0.0, 2.0, 9.0, 1.0, 0.25, 0.25, 0.0, 0.5, 0.0)
        falso = type("P", (), {"cpu_times": staticmethod(lambda: t)})
        # total 14,0 - guest 0,5 = 13,5; menos idle 9 y iowait 1 = 3,5
        self.assertAlmostEqual(bh._cpu_ocupada_sistema(falso), 3.5)


class MuestreadorSinPsutil(unittest.TestCase):
    """Si no se puede medir, lo dice. «No lo se» no es «no hubo carga»."""

    def test_sin_psutil_dice_que_no_lo_sabe(self):
        with mock.patch.dict(sys.modules, {"psutil": None}):
            with bh.Muestreador(pid=1) as m:
                pass
        self.assertFalse(m.disponible)
        self.assertEqual(m.muestras, [])
        self.assertIn("psutil", m.motivo)


if __name__ == "__main__":
    unittest.main()
