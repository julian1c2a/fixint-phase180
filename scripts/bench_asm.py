#!/usr/bin/env python3
# =============================================================================
# bench_asm.py - cuenta el codigo que emite cada compilador para una funcion
# =============================================================================
#
# Es la tercera pieza del desguace de benchmarks (P2.7 en NEXT_STEPS.md).
#
# POR QUE EXISTE. Tres veces ha mentido la comparacion Karatsuba/escolar, y las
# tres se destaparon MIRANDO, no por una alarma. La que mas cara salio fue esta:
#
#     GCC 16.2, N=4, ensamblador emitido
#       Karatsuba   119 instrucciones,  9 `mul`   -> desenrollado
#       escolar      61 instrucciones,  1 `mul`   -> BUCLE
#
# Un `mul` ejecutado diez veces contra nueve en linea recta. El cronometro decia
# "Karatsuba gana 1,65x" y tenia razon: lo que no decia es que estaba comparando
# un desenrollado con un bucle. La cuenta de instrucciones lo dice de un vistazo.
#
# LA SENAL NO ES LA CUENTA, ES LA DISCORDANCIA. Si el tiempo dice 1,7x y el
# numero de multiplicaciones dice 0,9x, hay una tercera variable en juego y hay
# que nombrarla ANTES de publicar el numero. Este guion no decide nada: pone las
# dos cifras una al lado de la otra para que la discordancia se vea.
#
# USO
#   python scripts/bench_asm.py --fuente <probe.cpp> --funcion PATRON [...]
#   python scripts/bench_asm.py --fuente probe.cpp --funcion mul_ --funcion escolar
#   python scripts/bench_asm.py ... --compilador gcc --compilador msvc
#
# El patron de --funcion se busca dentro del nombre decorado, asi que basta con
# un trozo: `mul_sin_marca` encuentra el simbolo entero en los cuatro.
# =============================================================================

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(RAIZ / "scripts"))
sys.path.insert(0, str(RAIZ / "scripts" / "env_setup"))


def echo(msg=""):
    print(msg, flush=True)


# Los mnemonicos llevan sufijo en la sintaxis de GNU (movq, adcq, mulq) y no lo
# llevan en la de MSVC. Los patrones aceptan las dos.
FAMILIAS = {
    "mul": r"(mulx|imul|mul)[a-z]?\b",
    "adc/sbb": r"(adc|sbb)[a-z]?\b",
    "add/sub": r"(add|sub)[a-z]?\b",
    "mov": r"mov[a-z]*\b",
    "call": r"call[a-z]?\b",
}


def entorno(compilador):
    from compiler_env import CompilerEnvironment  # noqa: PLC0415

    ce = CompilerEnvironment(compilador)
    return ce.get_compiler_cmd(), ce.get_env()


def emitir_asm(compilador, fuente, salida, extra):
    """Compila a ensamblador. Devuelve (ok, mensaje)."""
    cc, env = entorno(compilador)
    incluye = [str(RAIZ / "include"), str(RAIZ / "benchs")]
    if compilador == "msvc":
        cmd = [cc, "/std:c++20", "/O2", "/EHsc", "/DNDEBUG"]
        for i in incluye:
            cmd += ["/I", i]
        cmd += list(extra) + ["/c", str(fuente), "/FA", "/Fa" + str(salida),
                              "/Fo" + str(salida) + ".obj"]
    else:
        cmd = [cc, "-std=c++20", "-O2", "-DNDEBUG"]
        cmd += ["-I" + i for i in incluye]
        cmd += list(extra) + ["-S", str(fuente), "-o", str(salida)]
    r = subprocess.run(cmd, capture_output=True, text=True, env=env, cwd=str(RAIZ))
    if r.returncode != 0:
        errores = [l for l in (r.stdout + r.stderr).splitlines() if "error" in l.lower()]
        return False, (errores[0][:110] if errores else "sin diagnostico")
    return True, cc


def cuerpos_gnu(texto):
    """Trocea ensamblador de GNU: de `etiqueta:` a `.seh_endproc` o `.size`."""
    out = {}
    for m in re.finditer(r"^([A-Za-z_$.][\w$.]*):$", texto, re.M):
        nombre = m.group(1)
        resto = texto[m.end():]
        fin = re.search(r"^\s*\.(seh_endproc|size|cfi_endproc)\b", resto, re.M)
        cuerpo = resto[:fin.start()] if fin else resto[:60000]
        lineas = [l.strip() for l in cuerpo.splitlines()]
        lineas = [l for l in lineas
                  if l and not l.startswith((".", "#", "//")) and not l.endswith(":")]
        if lineas:
            out[nombre] = lineas
    return out


def cuerpos_msvc(texto):
    """Trocea ensamblador de MASM: de `<nombre> PROC` a ` ENDP`."""
    out = {}
    for m in re.finditer(r"^(\S+)\s+PROC\b", texto, re.M):
        nombre = m.group(1)
        corte = texto.find(" ENDP", m.end())
        cuerpo = texto[m.end():corte if corte > 0 else m.end() + 60000]
        lineas = [l.split(";")[0].strip() for l in cuerpo.splitlines()]
        lineas = [l for l in lineas
                  if l and not l.startswith(("$", "PUBLIC", "EXTRN", ";")) and not l.endswith(":")]
        if lineas:
            out[nombre] = lineas
    return out


def contar(lineas):
    d = {"instr": len(lineas)}
    for nombre, patron in FAMILIAS.items():
        d[nombre] = sum(1 for l in lineas if re.match(patron, l, re.I))
    return d


def main():
    ap = argparse.ArgumentParser(description="Cuenta el codigo emitido por funcion")
    ap.add_argument("--fuente", required=True, help="fichero .cpp con las funciones a mirar")
    ap.add_argument("--funcion", action="append", required=True,
                    help="trozo del nombre a buscar; se puede repetir")
    ap.add_argument("--compilador", action="append",
                    help="gcc | clang | msvc | intel; por defecto los cuatro")
    ap.add_argument("--flag", action="append", default=[],
                    help="bandera extra para el compilador; se puede repetir")
    args = ap.parse_args()

    compiladores = args.compilador or ["gcc", "clang", "msvc", "intel"]
    fuente = Path(args.fuente)
    if not fuente.is_absolute():
        fuente = (RAIZ / fuente).resolve()
    if not fuente.exists():
        echo("no existe: %s" % fuente)
        return 2

    tmp = RAIZ / "build" / "asm_tmp"
    tmp.mkdir(parents=True, exist_ok=True)

    echo("=" * 78)
    echo("  Codigo emitido por funcion   (%s)" % fuente.name)
    echo("=" * 78)
    echo("  La senal no es la cuenta: es su DISCORDANCIA con el cronometro.")
    echo("  Un `call` dentro de un nucleo numerico ya es aviso por si solo.")
    echo()

    algun_fallo = False
    for comp in compiladores:
        asm = tmp / ("%s.asm" % comp if comp == "msvc" else "%s.s" % comp)
        ok, detalle = emitir_asm(comp, fuente, asm, args.flag)
        if not ok:
            echo("--- %-6s NO COMPILA: %s" % (comp, detalle))
            algun_fallo = True
            continue

        texto = asm.read_text(encoding="utf-8", errors="replace")
        cuerpos = cuerpos_msvc(texto) if comp == "msvc" else cuerpos_gnu(texto)

        echo("--- %s" % comp)
        echo("    %-46s %6s %5s %8s %8s %5s %5s"
             % ("funcion", "instr", "mul", "adc/sbb", "add/sub", "mov", "call"))
        for patron in args.funcion:
            hallados = [(n, c) for n, c in cuerpos.items() if patron in n]
            if not hallados:
                echo("    %-46s  no encontrada (se inlineo del todo?)" % patron[:46])
                algun_fallo = True
                continue
            # TODAS las coincidencias, de mayor a menor. No solo la mas grande:
            # que una funcion aparezca partida en un envoltorio de seis
            # instrucciones y un cuerpo de cien ES el dato -- asi se vio que MSVC
            # dejaba `kmul_full` fuera de linea. Quedarse con la mayor lo habria
            # escondido.
            hallados.sort(key=lambda p: len(p[1]), reverse=True)
            for orden, (nombre, lineas) in enumerate(hallados[:3]):
                d = contar(lineas)
                avisos = []
                if d["call"]:
                    avisos.append("CALL en un nucleo numerico")
                if orden > 0:
                    avisos.append("otra copia del mismo patron")
                etiqueta = nombre if len(nombre) <= 46 else nombre[:43] + "..."
                echo("    %-46s %6d %5d %8d %8d %5d %5d%s"
                     % (etiqueta, d["instr"], d["mul"], d["adc/sbb"], d["add/sub"],
                        d["mov"], d["call"], ("  <- " + "; ".join(avisos)) if avisos else ""))
            if len(hallados) > 3:
                echo("    %-46s  (y %d coincidencia(s) mas)" % ("", len(hallados) - 3))
        echo()

    return 1 if algun_fallo else 0


if __name__ == "__main__":
    sys.exit(main())
