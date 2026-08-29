#!/usr/bin/env python3
# =============================================================================
# check_docs_consistency.py - Armonizador de documentacion
# =============================================================================
#
# Part of int128 Library
# SPDX-License-Identifier: BSL-1.0
# Copyright (c) 2024-2026 Julian Calderon Almendros
#
# T6.5 del plan de auditoria (23 ago 2026).
#
# La auditoria encontro documentacion que se contradecia a si misma: el README
# decia "42/42 tests" en la cabecera y "106/106" en una seccion mientras otra
# decia "197/197", y enlazaba a cuatro ficheros que no existen. Nada de eso
# rompe una compilacion, asi que puede vivir en el repositorio durante meses.
#
# Este script es el equivalente C++ del `repasa_y_proyecta` de las guias de
# Lean 4: recorre la documentacion y comprueba que dice la verdad.
#
# Comprobaciones:
#   1. ENLACES     todo enlace markdown a un fichero del repo apunta a algo que existe
#   2. TESTS       el numero de ficheros de test citado en la documentacion
#                  coincide con los que hay en tests/
#   3. API_DOCS    cada docs/API_*.md corresponde a un header de include/
#   4. SPDX        todo .hpp de include/ lleva su cabecera de licencia
#   5. LICENSE     existe el fichero de licencia que las cabeceras citan
#   6. DOXYGEN     0 avisos de doxygen atribuibles a include/  (--doxygen)
#   7. FECHAS      los "Last Updated" de los documentos vivos no se contradicen
#
# Uso:
#   python scripts/check_docs_consistency.py            # todo menos doxygen
#   python scripts/check_docs_consistency.py --doxygen  # incluye doxygen (lento)
#   python scripts/check_docs_consistency.py --quiet    # solo el resumen
#
# Salida: 0 si todo cuadra, 1 si hay alguna incoherencia.
# =============================================================================

import argparse
import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote

PROJECT_ROOT = Path(__file__).resolve().parent.parent

# Documentos "vivos": los que describen el estado actual y deben estar al dia.
LIVE_DOCS = ["README.md", "PROJECT_STATUS.md", "NEXT_STEPS.md", "CHANGELOG.md"]

# Cifra de referencia de avisos de cobertura de doxygen en include/.
#
# NO ES UN OBJETIVO, ES UN TECHO: la comprobacion falla si SUBE. Se baja a mano
# cada vez que el armonizador diga que ha bajado. La mayoria son de la familia
# int128_param_*, que ADR-006 retira, asi que esta cifra es tambien el medidor
# de progreso de esa migracion: cuanto mas cerca de cero, mas cerca la paridad.
#
# HAY UNA CIFRA POR VERSION DE DOXYGEN, y no es un capricho. El primer intento
# (25 ago 2026) uso un unico numero absoluto, 505, medido en local con doxygen
# 1.18.0. El CI, que usa la 1.9.8 de ubuntu-24.04, conto 518 y el job siguio en
# rojo: MISMO ARBOL, MISMA CONFIGURACION, TRECE AVISOS DE DIFERENCIA, solo por
# la version. El proyecto ya sabia que doxygen no es estable entre versiones
# --de ahi existe DOXYGEN_ALLOWED-- y aun asi el trinquete se escribio sin
# tenerlo en cuenta.
#
# Al anadir una version nueva: ejecutar `check_docs_consistency.py --doxygen`,
# leer la cifra y apuntarla aqui con su fecha.
DOXYGEN_BASELINE = {
    "1.9.8":  518,   # ubuntu-24.04, la que usa el CI      — medido 26 ago 2026
    "1.18.0": 505,   # MSYS2, la de la maquina de trabajo  — medido 26 ago 2026
}

# Para una version que no este en la tabla no se puede afinar, asi que se usa la
# mas alta conocida y se avisa: es preferible no detectar una subida pequena a
# dejar el CI en rojo por un desfase de version que no dice nada del codigo.
DOXYGEN_BASELINE_POR_DEFECTO = max(DOXYGEN_BASELINE.values())


# Avisos de doxygen que NO son culpa nuestra ni del codigo, con su motivo.
# Cualquier otro aviso hace fallar la comprobacion.
# El criterio DURO es: cero avisos procedentes de include/. Esos vienen del
# codigo y son estables entre versiones de doxygen.
#
# Fuera de include/ la cosa cambia: el conjunto de avisos depende de la VERSION
# de doxygen, y la del runner no tiene por que ser la del desarrollo. Medido el
# 24 ago 2026: doxygen 1.14 (local) da 4 avisos y doxygen 1.9.8 (ubuntu-24.04)
# da 21 sobre el MISMO arbol. Por eso esta lista contempla clases de aviso que
# son ruido de version, cada una con su motivo.
DOXYGEN_ALLOWED = [
    # La traduccion al espanyol de doxygen no esta completa. No afecta al
    # contenido generado, solo a las cadenas de la interfaz.
    ('The selected output language "spanish" has not been updated',
     "limitacion de doxygen, no del proyecto"),
    # Enlaces del README a documentos que SI existen en el repositorio y
    # funcionan al navegar por GitHub, pero que doxygen no resuelve como
    # referencia interna de la documentacion generada.
    ("unable to resolve reference to '",
     "enlace valido en el repo, no en el sitio generado"),
    # El Doxyfile se actualizo con `doxygen -u` desde una version mas nueva que
    # la del runner, asi que este ignora tags que no conoce. Es puro desfase de
    # version: no cambia lo que se genera.
    ("ignoring unsupported tag",
     "tag del Doxyfile que la version del runner no conoce"),
    # El reverso del anterior: una version MAS nueva que la que genero el
    # Doxyfile marca tags como obsoletos. Mismo desfase, otra direccion.
    ("has become obsolete",
     "tag del Doxyfile marcado obsoleto por una version mas nueva"),
    # doxygen 1.9.x intenta autoenlazar cosas como `::max()` incluso dentro de
    # spans de codigo; 1.14 ya no lo hace. Reescribir documentacion correcta
    # para contentar a una version concreta seria peor que ignorar el aviso.
    ("could not be resolved",
     "autolink de doxygen 1.9.x dentro de spans de codigo"),
]

# Ficheros a los que no se les exige cabecera SPDX.
SPDX_EXEMPT: set = set()


class Report:
    def __init__(self, quiet: bool):
        self.quiet = quiet
        self.problems = []
        self.checks = 0

    def ok(self, name):
        self.checks += 1
        if not self.quiet:
            print(f"  [OK]   {name}")

    def fail(self, name, detail=""):
        self.checks += 1
        self.problems.append((name, detail))
        print(f"  [FALLO] {name}")
        if detail:
            for line in detail.splitlines():
                print(f"          {line}")

    def section(self, title):
        if not self.quiet:
            print(f"\n--- {title} ---")


# =============================================================================
# 1. Enlaces markdown
# =============================================================================

LINK_RE = re.compile(r'\[[^\]]*\]\(([^)]+)\)')


def check_links(rep: Report):
    rep.section("1. Enlaces markdown")

    md_files = [PROJECT_ROOT / d for d in LIVE_DOCS]
    md_files += sorted((PROJECT_ROOT / "docs").glob("*.md"))
    md_files += sorted((PROJECT_ROOT / "docs" / "decisions").glob("*.md"))
    # Los documentos de comunidad tambien: sus enlaces se rompen igual que los
    # demas, y son los primeros que lee alguien de fuera.
    # THOUGHTS.md NO esta aqui a proposito: es el cuaderno personal del autor y
    # queda fuera de todo flujo de documentacion.
    for extra in ("AI-GUIDE.md", "CONTRIBUTING.md", "SECURITY.md", "ROADMAP.md",
                  "NAMING_CONVENTIONS.md", "STYLE_CONVENTIONS.md",
                  "QUICK_REFERENCE.md"):
        md_files.append(PROJECT_ROOT / extra)

    broken = []
    total = 0
    for md in md_files:
        if not md.exists():
            continue
        text = md.read_text(encoding="utf-8", errors="replace")
        for m in LINK_RE.finditer(text):
            target = m.group(1).strip()
            # Se ignoran URLs, anclas y correo.
            if target.startswith(("http://", "https://", "#", "mailto:")):
                continue
            # En markdown tecnico abundan cosas como `[foo](const uint128_t& v)`
            # que parecen enlaces pero son firmas de C++. Se descartan por sus
            # caracteres: ningun fichero del repositorio lleva espacios, '&'
            # ni '*' en el nombre.
            if any(c in target for c in " &*<>"):
                continue
            path_part = unquote(target.split("#", 1)[0])
            if not path_part:
                continue
            total += 1
            resolved = (md.parent / path_part).resolve()
            if not resolved.exists():
                broken.append(f"{md.relative_to(PROJECT_ROOT).as_posix()} -> {path_part}")

    if broken:
        rep.fail(f"enlaces rotos: {len(broken)} de {total}", "\n".join(broken[:20]))
    else:
        rep.ok(f"los {total} enlaces a ficheros del repo existen")


# =============================================================================
# 2. Recuento de tests
# =============================================================================

COUNT_RE = re.compile(r'\b(\d{1,3})\s*/\s*(\d{1,3})\b')


def check_test_counts(rep: Report):
    rep.section("2. Recuento de ficheros de test")

    actual = len(list((PROJECT_ROOT / "tests").glob("test_*.cpp")))
    rep.ok(f"tests/ contiene {actual} ficheros test_*.cpp")

    # Solo se revisan las lineas de ESTADO ACTUAL (las que dicen "suite"), no
    # las historicas: un CHANGELOG cita a proposito cifras de versiones viejas.
    stale = []
    for name in LIVE_DOCS:
        path = PROJECT_ROOT / name
        if not path.exists():
            continue
        for i, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            low = line.lower()
            if "suite actual" not in low and "suite completa" not in low:
                continue
            for m in COUNT_RE.finditer(line):
                a, b = int(m.group(1)), int(m.group(2))
                if a == b and b != actual and 10 <= b <= 200:
                    stale.append(f"{name}:{i}: dice {b}/{b}, la suite tiene {actual}")

    if stale:
        rep.fail("cifras de suite desactualizadas", "\n".join(stale))
    else:
        rep.ok("las cifras de 'suite actual/completa' coinciden con tests/")


# =============================================================================
# 3. docs/API_*.md <-> headers
# =============================================================================

def check_api_docs(rep: Report):
    rep.section("3. docs/API_*.md frente a include/")

    headers = {p.stem for p in (PROJECT_ROOT / "include").rglob("*.hpp")}
    api_docs = sorted((PROJECT_ROOT / "docs").glob("API_*.md"))

    # Un API_*.md documenta uno o varios headers. La regla no puede ser que el
    # nombre del doc contenga el del header (API_fixed_int_stl.md cubre tres),
    # asi que se considera huerfano el que no MENCIONE ningun header existente
    # en su texto.
    header_files = {p.name for p in (PROJECT_ROOT / "include").rglob("*.hpp")}
    huerfanos = []
    for doc in api_docs:
        text = doc.read_text(encoding="utf-8", errors="replace")
        if not any(h in text for h in header_files):
            huerfanos.append(doc.name)

    if huerfanos:
        rep.fail(f"{len(huerfanos)} API_*.md sin header correspondiente",
                 ", ".join(huerfanos))
    else:
        rep.ok(f"los {len(api_docs)} ficheros API_*.md corresponden a headers")

    # Headers publicos sin su API_*.md: informativo, no bloquea. Misma regla que
    # arriba, en el otro sentido: un header esta documentado si algun API_*.md lo
    # menciona por su nombre de fichero.
    all_api_text = chr(10).join(
        d.read_text(encoding="utf-8", errors="replace") for d in api_docs)
    sin_doc = sorted(p.name for p in (PROJECT_ROOT / "include").rglob("*.hpp")
                     if p.name not in all_api_text)
    if sin_doc and not rep.quiet:
        print(f"  [nota] headers sin API_*.md propio: {', '.join(sin_doc)}")


# =============================================================================
# 4. Cabeceras SPDX
# =============================================================================

def check_spdx(rep: Report):
    rep.section("4. Cabeceras de licencia (SPDX)")

    faltan = []
    total = 0
    for hpp in sorted((PROJECT_ROOT / "include").rglob("*.hpp")):
        if hpp.name in SPDX_EXEMPT:
            continue
        total += 1
        head = "\n".join(hpp.read_text(encoding="utf-8", errors="replace").splitlines()[:30])
        if "SPDX-License-Identifier" not in head:
            faltan.append(hpp.relative_to(PROJECT_ROOT).as_posix())

    if faltan:
        rep.fail(f"{len(faltan)} de {total} headers sin SPDX", "\n".join(faltan))
    else:
        rep.ok(f"los {total} headers de include/ llevan SPDX")


# =============================================================================
# 5. Fichero de licencia
# =============================================================================

def check_license(rep: Report):
    rep.section("5. Fichero de licencia")

    candidatos = ["LICENSE.txt", "LICENSE", "LICENSE.md"]
    encontrado = next((c for c in candidatos if (PROJECT_ROOT / c).exists()), None)

    if encontrado is None:
        rep.fail("no existe el fichero de licencia",
                 "Las cabeceras citan 'LICENSE.txt' y AI-GUIDE.md lo declara obligatorio.")
        return

    texto = (PROJECT_ROOT / encontrado).read_text(encoding="utf-8", errors="replace")
    if "Boost Software License" not in texto:
        rep.fail(f"{encontrado} no parece la Boost Software License")
    else:
        rep.ok(f"{encontrado} presente y es la BSL-1.0")


# =============================================================================
# 6. Doxygen
# =============================================================================

def check_doxygen(rep: Report):
    rep.section("6. Doxygen")

    doxyfile = PROJECT_ROOT / "Doxyfile"
    if not doxyfile.exists():
        rep.fail("no hay Doxyfile")
        return

    for d in ("documentation/generated", "documentation/doxygen/pages"):
        (PROJECT_ROOT / d).mkdir(parents=True, exist_ok=True)

    try:
        ver = subprocess.run(["doxygen", "--version"], capture_output=True, text=True,
                             encoding="utf-8", errors="replace", timeout=60, check=False)
        version = ver.stdout.strip().splitlines()[0] if ver.stdout.strip() else "?"
    except (OSError, subprocess.SubprocessError, IndexError):
        version = "?"

    try:
        proc = subprocess.run(["doxygen", str(doxyfile)], cwd=str(PROJECT_ROOT),
                              capture_output=True, text=True, encoding="utf-8",
                              errors="replace", timeout=900, check=False)
    except (OSError, subprocess.SubprocessError) as exc:
        rep.fail("no se pudo ejecutar doxygen", str(exc))
        return

    if not rep.quiet:
        print(f"  [info] doxygen {version}")

    avisos = [l for l in proc.stderr.splitlines() if "warning:" in l]
    permitidos, reales = [], []
    for a in avisos:
        if any(pat in a for pat, _ in DOXYGEN_ALLOWED):
            permitidos.append(a)
        else:
            reales.append(a)

    de_include = [a for a in reales if "include/" in a]

    # TRINQUETE, no puerta cerrada.
    #
    # Hasta el 25 ago 2026 esta comprobacion exigia CERO avisos, y daba cero
    # siempre... porque el Doxyfile tenia EXTRACT_ALL = YES y
    # WARN_IF_UNDOCUMENTED = NO: era imposible que apareciera un aviso de
    # cobertura. Al encenderla de verdad (ADR-014) salieron mas de quinientos.
    #
    # Exigir cero de golpe dejaria el CI en rojo hasta terminar toda la
    # documentacion, y un CI que siempre esta rojo no lo mira nadie. Exigir
    # cero mintiendo era lo de antes. La salida es un trinquete: se guarda la
    # cifra de referencia y **solo se falla si sube**.
    #
    # Ademas la cifra sirve de medidor: la mayor parte son de int128_param_*,
    # que ADR-006 va a retirar, de modo que baja sola conforme se alcanza la
    # paridad. Cuando llegue a cero, esto pasa a exigir cero de verdad.
    n = len(de_include)

    # La referencia depende de la version de doxygen; ver la nota de
    # DOXYGEN_BASELINE. `version` viene como "1.9.8" o similar.
    clave = version.strip()
    if clave in DOXYGEN_BASELINE:
        techo = DOXYGEN_BASELINE[clave]
        de_donde = f"referencia de doxygen {clave}"
    else:
        techo = DOXYGEN_BASELINE_POR_DEFECTO
        de_donde = (f"doxygen {clave} no esta en la tabla; se usa la referencia mas "
                    f"alta conocida ({techo}). Apunta la cifra de esta version "
                    f"en DOXYGEN_BASELINE")

    if n > techo:
        rep.fail(f"{n} avisos de doxygen en include/, y el techo es {techo}: "
                 f"han SUBIDO en {n - techo} ({de_donde})",
                 "\n".join(de_include[:15]) +
                 "\n(documenta lo nuevo, o baja la referencia si has borrado codigo)")
    elif n < techo:
        rep.ok(f"{n} avisos de doxygen en include/ — {techo - n} por debajo del "
               f"techo ({de_donde}). Si has documentado, baja la referencia a {n}.")
    elif n:
        rep.ok(f"{n} avisos de doxygen en include/, igual que el techo "
               f"({de_donde}) — deuda conocida, ver ADR-014")
    else:
        rep.ok("0 avisos de doxygen atribuibles a include/")

    otros = [a for a in reales if "include/" not in a]
    if otros:
        rep.fail(f"{len(otros)} avisos de doxygen fuera de include/ (doxygen {version})",
                 "\n".join(otros[:15]) +
                 "\n(si son ruido de esta version de doxygen, van a DOXYGEN_ALLOWED "
                 "CON SU MOTIVO; si no, se arreglan)")
    else:
        rep.ok("0 avisos de doxygen fuera de include/ (aparte de los permitidos)")

    if permitidos and not rep.quiet:
        print(f"  [nota] {len(permitidos)} avisos permitidos:")
        for pat, motivo in DOXYGEN_ALLOWED:
            if any(pat in a for a in permitidos):
                print(f"         - {pat[:60]}... ({motivo})")


# =============================================================================
# 7. Fechas de los documentos vivos
# =============================================================================

DATE_RE = re.compile(r'\*\*Last Updated:\*\*\s*(.+)')


def check_dates(rep: Report):
    rep.section("7. Fechas de los documentos vivos")

    fechas = {}
    for name in LIVE_DOCS:
        path = PROJECT_ROOT / name
        if not path.exists():
            continue
        head = "\n".join(path.read_text(encoding="utf-8", errors="replace").splitlines()[:20])
        m = DATE_RE.search(head)
        if m:
            fechas[name] = m.group(1).strip()

    if not fechas:
        rep.ok("ningun documento declara 'Last Updated' (nada que comparar)")
        return

    distintas = set(fechas.values())
    if len(distintas) > 1:
        detalle = "\n".join(f"{k}: {v}" for k, v in sorted(fechas.items()))
        rep.fail("los documentos vivos declaran fechas distintas", detalle)
    else:
        rep.ok(f"todos los documentos vivos dicen '{next(iter(distintas))}'")


# =============================================================================
# main
# =============================================================================

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--doxygen", action="store_true",
                        help="incluir la pasada de doxygen (tarda ~1 min)")
    parser.add_argument("--quiet", action="store_true", help="solo el resumen")
    args = parser.parse_args()

    print("=" * 78)
    print("  Armonizador de documentacion (T6.5)")
    print("=" * 78)

    rep = Report(args.quiet)
    check_links(rep)
    check_test_counts(rep)
    check_api_docs(rep)
    check_spdx(rep)
    check_license(rep)
    check_dates(rep)
    if args.doxygen:
        check_doxygen(rep)
    else:
        print("\n  [nota] doxygen omitido; usa --doxygen para incluirlo")

    print()
    print("=" * 78)
    if rep.problems:
        print(f"  {len(rep.problems)} incoherencias sobre {rep.checks} comprobaciones")
        for name, _ in rep.problems:
            print(f"    - {name}")
    else:
        print(f"  {rep.checks}/{rep.checks} comprobaciones OK")
    print("=" * 78)
    return 1 if rep.problems else 0


if __name__ == "__main__":
    sys.exit(main())
