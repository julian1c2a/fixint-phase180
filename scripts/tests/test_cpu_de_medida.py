#!/usr/bin/env python3
# =============================================================================
# test_cpu_de_medida.py - pruebas del guion que fija la CPU para medir
# =============================================================================
#
# SPDX-License-Identifier: BSL-1.0
#
# `scripts/condiciones_benchmark/cpu_de_medida.sh` es la unica pieza del arnes
# que corre como root, en la plataforma de medida dedicada. Se prueba contra un
# sysfs FALSO (un arbol de ficheros), con `BENCH_SYSFS` y `BENCH_ESTADO`.
#
# Solo corre donde hay bash POSIX, y NUNCA como root: como root el guion ignora
# la raiz falsa a proposito y escribiria en el /sys de verdad de la maquina que
# ejecuta las pruebas.
# =============================================================================

import os
import shutil
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path

GUION = Path(__file__).resolve().parent.parent / "condiciones_benchmark" / "cpu_de_medida.sh"
CPU = "sys/devices/system/cpu/"

PUEDE = (os.name == "posix" and shutil.which("bash") is not None
         and hasattr(os, "geteuid") and os.geteuid() != 0)


@unittest.skipUnless(PUEDE, "hace falta bash POSIX y no ser root")
class CpuDeMedida(unittest.TestCase):

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.raiz = Path(self._tmp.name)
        self.estado = self.raiz / "estado"

    def tearDown(self):
        # Por si una prueba dejo algun fichero de solo lectura.
        for p in self.raiz.rglob("*"):
            if p.is_file():
                p.chmod(stat.S_IRUSR | stat.S_IWUSR)
        self._tmp.cleanup()

    def _pon(self, ficheros):
        for rel, contenido in ficheros.items():
            p = self.raiz / rel
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(contenido + "\n", encoding="ascii")

    def _lee(self, rel):
        return (self.raiz / rel).read_text(encoding="ascii").strip()

    def _corre(self, orden):
        env = dict(os.environ, BENCH_SYSFS=str(self.raiz / "sys"), BENCH_ESTADO=str(self.estado))
        r = subprocess.run(["bash", str(GUION), orden], capture_output=True, text=True, env=env)
        return r.returncode, r.stdout + r.stderr

    def _intel(self, cpus=4, smt="on", no_turbo="0", gobernador="powersave"):
        f = {CPU + "intel_pstate/no_turbo": no_turbo, CPU + "smt/control": smt,
             CPU + "online": "0-%d" % (cpus - 1)}
        for n in range(cpus):
            f[CPU + "cpu%d/cpufreq/scaling_driver" % n] = "intel_pstate"
            f[CPU + "cpu%d/cpufreq/scaling_governor" % n] = gobernador
            f[CPU + "cpu%d/cpufreq/energy_performance_preference" % n] = "balance_performance"
        self._pon(f)

    # ------------------------------------------------------------------------

    def test_fija_deja_la_serie_de_referencia(self):
        self._intel()
        rc, salida = self._corre("fija")
        self.assertEqual(rc, 0, salida)
        self.assertEqual(self._lee(CPU + "intel_pstate/no_turbo"), "1")
        self.assertEqual(self._lee(CPU + "smt/control"), "off")
        for n in range(4):
            self.assertEqual(self._lee(CPU + "cpu%d/cpufreq/scaling_governor" % n), "performance")
        self.assertIn("lista para medir", salida)

    def test_restaura_devuelve_como_estaba(self):
        self._intel()
        self._corre("fija")
        # LO QUE HACE EL KERNEL Y ESTE SYSFS FALSO NO: con el gobernador en
        # `performance`, intel_pstate pone la EPP en `performance` por su cuenta.
        # Sin simularlo, esta prueba no podia ver si `restaura` devuelve la EPP:
        # `fija` no la toca y en el arbol falso nadie la cambiaba. Se vio rompiendo
        # el guion a proposito el 2 oct 2026 -- la averia pasaba en verde.
        for n in range(4):
            (self.raiz / CPU / ("cpu%d/cpufreq/energy_performance_preference" % n)).write_text(
                "performance\n", encoding="ascii")
        rc, salida = self._corre("restaura")
        self.assertEqual(rc, 0, salida)
        self.assertEqual(self._lee(CPU + "intel_pstate/no_turbo"), "0")
        self.assertEqual(self._lee(CPU + "smt/control"), "on")
        self.assertEqual(self._lee(CPU + "cpu2/cpufreq/scaling_governor"), "powersave")
        self.assertEqual(self._lee(CPU + "cpu2/cpufreq/energy_performance_preference"),
                         "balance_performance")
        self.assertFalse(self.estado.exists(), "el estado guardado tiene que borrarse")

    def test_fija_dos_veces_conserva_el_original(self):
        # LA QUE MAS IMPORTA. Si el segundo `fija` guardara el estado ya fijado,
        # `restaura` dejaria la maquina en la configuracion de medida para siempre.
        self._intel()
        self._corre("fija")
        self._corre("fija")
        self._corre("restaura")
        self.assertEqual(self._lee(CPU + "intel_pstate/no_turbo"), "0")
        self.assertEqual(self._lee(CPU + "cpu0/cpufreq/scaling_governor"), "powersave")

    def test_amd_apaga_el_turbo_por_boost(self):
        f = {CPU + "cpufreq/boost": "1", CPU + "cpu0/cpufreq/scaling_driver": "acpi-cpufreq",
             CPU + "cpu0/cpufreq/scaling_governor": "schedutil"}
        self._pon(f)
        rc, salida = self._corre("fija")
        self.assertEqual(rc, 0, salida)
        self.assertEqual(self._lee(CPU + "cpufreq/boost"), "0")
        self._corre("restaura")
        self.assertEqual(self._lee(CPU + "cpufreq/boost"), "1")

    def test_sin_smt_no_toca_nada_y_vale(self):
        # El i5-8500T: 6 nucleos, 6 hilos.
        self._intel(cpus=6, smt="notsupported")
        rc, salida = self._corre("fija")
        self.assertEqual(rc, 0, salida)
        self.assertEqual(self._lee(CPU + "smt/control"), "notsupported")

    def test_un_ajuste_que_no_coge_hace_fallar(self):
        # Un gobernador que no se deja escribir: el guion tiene que verificarlo
        # leyendo de vuelta, no fiarse de que la escritura funciono.
        self._intel()
        (self.raiz / CPU / "cpu1/cpufreq/scaling_governor").chmod(stat.S_IRUSR)
        rc, salida = self._corre("fija")
        self.assertEqual(rc, 1, salida)
        self.assertIn("NO ESTA LISTA", salida)
        self.assertIn("cpu1", salida)

    def test_sin_forma_de_apagar_el_turbo_falla(self):
        self._pon({CPU + "cpu0/cpufreq/scaling_governor": "powersave"})
        rc, salida = self._corre("fija")
        self.assertEqual(rc, 1, salida)
        self.assertIn("turbo", salida)

    def test_restaura_sin_estado_no_rompe(self):
        self._intel()
        rc, salida = self._corre("restaura")
        self.assertEqual(rc, 0, salida)
        self.assertIn("nada que restaurar", salida)

    def test_orden_desconocida(self):
        rc, _ = self._corre("borra_todo")
        self.assertEqual(rc, 2)


if __name__ == "__main__":
    unittest.main()
