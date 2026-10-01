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
    "1.9.8":  286,   # ubuntu-24.04, la que usa el CI      — bajada 23 sep 2026 (P3.7; leida del log del CI sobre 7f37017, que es la unica forma: aqui no hay 1.9.8)
    # La distancia entre las dos versiones NO es ruido que se pueda ignorar: 286
    # contra 257, veintinueve avisos sobre el MISMO arbol. Por eso hay una cifra
    # por version desde el 26 ago, cuando un unico numero absoluto dejo el CI en
    # rojo con el arbol correcto.
    "1.18.0": 257,   # MSYS2, la de la maquina de trabajo  — bajada 23 sep 2026 (P3.7: los 209 de fixed_width_int_t y fixed_int_limits; 466-209=257)
}

# Para una version que no este en la tabla no se puede afinar, asi que se usa la
# mas alta conocida y se avisa: es preferible no detectar una subida pequena a
# dejar el CI en rojo por un desfase de version que no dice nada del codigo.
DOXYGEN_BASELINE_POR_DEFECTO = max(DOXYGEN_BASELINE.values())


# Cuantos ficheros declaran `NSTD_QUIERO_INT128_PARAM` para poder incluir la
# familia que ADR-006 retira.
#
# OTRO TECHO: la comprobacion falla si SUBE. Se baja a mano cuando un fichero
# deja de necesitarlo -- migrado al tipo nuevo, o borrado con la familia en el
# tramo 3.
#
# POR QUE HACE FALTA CONTARLO. Hasta el 29 sep la valvula era invisible: un
# `#define NSTD_SILENCIA_INT128_PARAM_DEPRECADO` por fichero, sin contar, y
# habia llegado a 45 sin que nadie lo decidiera. Un fichero nuevo que usara el
# tipo viejo anadia su linea y no pasaba nada. El nivel 3 convirtio esa valvula
# en una DECLARACION explicita; este techo la convierte en un numero que no
# puede crecer sin que alguien lo mire.
#
# Al bajarlo: ejecutar el armonizador, leer la cifra y apuntarla aqui con fecha.
QUIERO_INT128_PARAM_TECHO = 52   # 37 tests + 9 bancos + 6 demos — 29 sep 2026, al poner la puerta


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

    # Headers sin su API_*.md. SON TRES GRUPOS, NO UNO.
    #
    # Esto listaba los ocho juntos como «sin API_*.md propio», y los ocho lo
    # estaban A PROPOSITO. Leerlo como deuda costo abrir una tarea (P3.3) y
    # estar a punto de escribir ocho documentos que habrian dicho que
    # `div_kernels.hpp` es API publica -- justo lo contrario de lo que fijo
    # ADR-014: «entra lo que un usuario puede incluir, o sea la raiz de
    # include/; intrinsics/ y algorithms/ no».
    #
    #   1. INTERNOS (intrinsics/, algorithms/): fuera del ambito publico. No
    #      llevan API_*.md, pero **si** tienen que estar en el mapa de capas,
    #      `docs/ARQUITECTURA_INTERNA.md`. Si uno falta, ESTO FALLA: es un
    #      trinquete, como el techo de doxygen. Un fichero nuevo en
    #      `intrinsics/` que nadie anote al mapa lo deja desfasado en silencio.
    #   2. LEGADO (int128_param_*): deprecados, se borran en la 1.90 (ADR-006).
    #      Documentar lo que se retira es trabajo tirado.
    #   3. PUBLICOS sin documentar: **eso si** es deuda, y es lo unico que
    #      debe leerse como tal.
    all_api_text = chr(10).join(
        d.read_text(encoding="utf-8", errors="replace") for d in api_docs)
    mapa = PROJECT_ROOT / "docs" / "ARQUITECTURA_INTERNA.md"
    texto_mapa = mapa.read_text(encoding="utf-8", errors="replace") if mapa.exists() else ""

    internos, legado, publicos = [], [], []
    for p in sorted((PROJECT_ROOT / "include").rglob("*.hpp")):
        if p.name in all_api_text:
            continue
        if p.parent.name in ("intrinsics", "algorithms"):
            internos.append(p.name)
        elif p.name.startswith("int128_param"):
            legado.append(p.name)
        else:
            publicos.append(p.name)

    fuera_del_mapa = [n for n in internos if n not in texto_mapa]
    if not mapa.exists():
        rep.fail("falta docs/ARQUITECTURA_INTERNA.md, el mapa de las capas internas",
                 f"{len(internos)} headers internos se quedan sin documentar")
    elif fuera_del_mapa:
        rep.fail(f"{len(fuera_del_mapa)} header(s) interno(s) fuera del mapa de capas",
                 ", ".join(fuera_del_mapa) +
                 " -- anyadelos a docs/ARQUITECTURA_INTERNA.md")
    else:
        rep.ok(f"los {len(internos)} headers internos estan en el mapa de capas "
               f"(ARQUITECTURA_INTERNA.md); no llevan API_*.md por ADR-014")

    if not rep.quiet:
        if legado:
            print(f"  [nota] sin API_*.md por estar deprecados, se van en 1.90: "
                  f"{', '.join(legado)}")
        if publicos:
            print(f"  [nota] headers PUBLICOS sin API_*.md -- esto si es deuda: "
                  f"{', '.join(publicos)}")


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

    # P3.1 (ADR-014, enmienda del 10 sep 2026): el ambito PUBLICO es lo que un
    # usuario puede incluir, o sea la raiz de `include/`. Los subdirectorios
    # `intrinsics/` y `algorithms/` existen precisamente para separar la
    # implementacion de la API que se instala, y quedan fuera.
    #
    # No se ignoran: se cuentan aparte y se informan, para que no desaparezcan
    # de la vista. Lo que no hacen es contar contra el techo, que es lo que
    # bloqueaba WARN_AS_ERROR (P3.2).
    INTERNOS = ("include/intrinsics/", "include/algorithms/")

    def es_interno(aviso):
        return any(d in aviso.replace("\\", "/") for d in INTERNOS)

    de_include = [a for a in reales if "include/" in a and not es_interno(a)]
    de_internos = [a for a in reales if "include/" in a and es_interno(a)]

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

    if de_internos:
        print(f"  [nota] {len(de_internos)} avisos en headers INTERNOS "
              f"(intrinsics/, algorithms/): fuera del ambito publico por la "
              f"enmienda de ADR-014, no cuentan contra el techo")

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
# 8. La superficie de la familia que se retira
# =============================================================================
#
# Dos invariantes de distinta naturaleza: un techo que solo baja, y un cero duro.


# Una declaracion de verdad empieza la linea; lo demas es un ejemplo en un
# comentario.
DECLARACION = re.compile(r"^[ \t]*#[ \t]*define[ \t]+NSTD_QUIERO_INT128_PARAM",
                         re.MULTILINE)


def _familia(ruta) -> bool:
    """Es una cabecera de la familia `int128_param_*`?

    `karatsuba.hpp` y `div_by_const.hpp` cuentan como familia aunque vivan en
    `algorithms/`: solo las incluye ella y mueren con ella (ver el mapa de capas).
    """
    return (ruta.name.startswith("int128_param")
            or ruta.name in ("karatsuba.hpp", "div_by_const.hpp"))


def check_bloque_estado(rep: Report):
    """10. El bloque de estado de NEXT_STEPS no se queda atras en silencio.

    POR QUE EXISTE. Ha envejecido DOS VECES igual, y la segunda con el aviso ya
    escrito dentro: primero citaba un hash **58 commits atras** --y en ese hueco
    el CI estuvo cuatro dias en rojo sin que nadie mirara-- y al arreglarlo se
    escribio otro que el 1 oct 2026 estaba **46 commits atras**, con el recuento
    de ADR tambien mal. El parrafo que explica el problema no evita el problema.

    Se vigilan las dos cifras que se pueden comprobar en local sin red: cuantos
    ADR hay, y a cuantos commits esta el hash de CI que se cita.
    """
    rep.section("10. Bloque de estado de NEXT_STEPS")

    doc = PROJECT_ROOT / "NEXT_STEPS.md"
    if not doc.exists():
        rep.fail("no existe NEXT_STEPS.md")
        return
    texto = doc.read_text(encoding="utf-8", errors="replace")

    problemas = []

    # --- 1. el recuento de ADR -------------------------------------------
    adr_disco = len([f for f in (PROJECT_ROOT / "docs" / "decisions").glob("ADR-*.md")
                     if re.match(r"ADR-\d{3}-", f.name)])
    m = re.search(r"\|\s*\*\*ADR\*\*\s*\|\s*\*\*(\d+)\*\*\s+registros", texto)
    if not m:
        problemas.append("no encuentro la fila «| **ADR** | **N** registros» en el bloque")
    elif int(m.group(1)) != adr_disco:
        problemas.append("dice %s ADR y en docs/decisions/ hay %d"
                         % (m.group(1), adr_disco))

    # --- 2. el hash de CI que cita ---------------------------------------
    # LIMITE, y por que este. Una sesion de trabajo deja ~8 commits; las dos veces
    # que esto pico iban por 46 y 58. 25 esta claramente fuera de lo normal y
    # claramente por debajo de donde hizo dano, asi que no da rojos de rutina.
    LIMITE = 25
    # LA FILA, Y SOLO LA FILA. El primer intento buscaba el hash con `re.DOTALL`
    # desde «| **CI** |», asi que cruzaba lineas y pescaba el hash de cualquier
    # otra fila del documento -- se vio al falsificar: con la fila del CI vacia
    # informaba de `05ba169`, que vive en una fila de P0.1. Salio rojo por suerte,
    # porque ese hash es viejo; con uno reciente habria dado VERDE con la fila
    # vacia, que es justo lo que esto existe para impedir.
    fila = next((l for l in texto.splitlines()
                 if re.match(r"\|\s*\*\*CI\*\*\s*\|", l)), None)
    m = re.search(r"`([0-9a-f]{7,40})`", fila) if fila else None
    if fila is None:
        problemas.append("no encuentro la fila «| **CI** |» en el bloque de estado")
    elif not m:
        problemas.append("la fila «| **CI** |» no cita ningun hash")
    else:
        hash_citado = m.group(1)
        r = subprocess.run(["git", "rev-list", "--count", "%s..HEAD" % hash_citado],
                           cwd=str(PROJECT_ROOT), capture_output=True, text=True,
                           encoding="utf-8", errors="replace")
        # «NO PUEDO MEDIRLO AQUI» NO ES «ESTA MAL». La primera version los
        # confundia y rompio el CI el 1 oct 2026: alli `actions/checkout` trae un
        # solo commit, ningun hash antiguo esta, y esto declaraba «no esta en esta
        # rama». En local, con el historial entero, pasaba -- y los cuatro casos
        # con que se falsifico eran todos locales.
        superficial = subprocess.run(
            ["git", "rev-parse", "--is-shallow-repository"], cwd=str(PROJECT_ROOT),
            capture_output=True, text=True, encoding="utf-8",
            errors="replace").stdout.strip() == "true"
        if r.returncode != 0 and superficial:
            if not rep.quiet:
                print("         [nota] clon superficial: no se puede medir a cuantos commits "
                      "queda `%s` (el job del CI necesita `fetch-depth: 0`)" % hash_citado)
        elif r.returncode != 0:
            problemas.append("el hash de CI `%s` no esta en esta rama, y el clon es completo: "
                             "esta mal escrito" % hash_citado)
        else:
            detras = int((r.stdout or "0").strip() or 0)
            # Se imprime SIEMPRE, verde o rojo: un numero delante de los ojos en
            # cada ejecucion es lo que rompe la costumbre de no mirarlo.
            if not rep.quiet:
                print("         el CI citado (`%s`) esta %d commit(s) por detras de HEAD"
                      % (hash_citado, detras))
            if detras > LIMITE:
                problemas.append(
                    "el hash de CI citado esta %d commits por detras (limite %d): esa cifra "
                    "ya no describe el estado, hay que volver a contarla con `gh run view`"
                    % (detras, LIMITE))

    if problemas:
        rep.fail("el bloque de estado no cuadra con el repositorio",
                 "\n".join(problemas))
    else:
        rep.ok("el bloque de estado cuadra: %d ADR, y el CI citado esta al dia" % adr_disco)


def check_indice_adr(rep: Report):
    """9. Todo ADR escrito esta en el indice, y el indice no inventa ninguno.

    POR QUE EXISTE. El 30 sep 2026 habia TRES ADR escritos --020, 021 y 022-- que
    no aparecian en `docs/decisions/README.md`. Se vio por casualidad, buscando el
    siguiente numero libre para el 023: el indice decia que el ultimo era el 019,
    y por poco se reutiliza un numero. La numeracion secuencial sin reutilizar es
    una de las tres reglas que el propio README declara, y no habia nada que la
    vigilara.
    """
    rep.section("9. Indice de ADR")

    carpeta = PROJECT_ROOT / "docs" / "decisions"
    indice = carpeta / "README.md"
    if not indice.exists():
        rep.fail("no existe docs/decisions/README.md, que es el indice de ADR")
        return

    en_disco = {}
    for f in sorted(carpeta.glob("ADR-*.md")):
        m = re.match(r"ADR-(\d{3})-", f.name)
        if m:
            en_disco[m.group(1)] = f.name

    texto = indice.read_text(encoding="utf-8", errors="replace")
    en_indice = dict(
        re.findall(r"^\| \[(\d{3})\]\((ADR-\d{3}-[^)]+\.md)\)", texto, re.MULTILINE))

    problemas = []
    faltan = sorted(set(en_disco) - set(en_indice))
    if faltan:
        problemas.append("escritos y NO indexados: " + ", ".join(faltan))
        problemas.append("  sin indice nadie sabe cual es el siguiente numero libre,")
        problemas.append("  y no reutilizarlos es regla del propio README")
    sobran = sorted(set(en_indice) - set(en_disco))
    if sobran:
        problemas.append("indexados y sin fichero en disco: " + ", ".join(sobran))
    for n, ruta in sorted(en_indice.items()):
        if n in en_disco and ruta != en_disco[n]:
            problemas.append("ADR-%s: el indice enlaza `%s` y el fichero es `%s`"
                             % (n, ruta, en_disco[n]))

    if problemas:
        rep.fail("el indice de ADR no cuadra con docs/decisions/",
                 "\n".join(problemas))
    else:
        rep.ok("los %d ADR de disco estan en el indice, y ninguno de mas" % len(en_disco))


def check_superficie_legado(rep: Report):
    rep.section("8. Superficie de la familia que se retira (ADR-006)")

    fuentes = []
    for carpeta in ("include", "tests", "benchs", "demos"):
        d = PROJECT_ROOT / carpeta
        if d.exists():
            fuentes += [f for f in d.rglob("*") if f.suffix in (".hpp", ".cpp")]

    declaran, internos_fuera = [], []
    for f in fuentes:
        try:
            texto = f.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        # La linea tiene que EMPEZAR por #define. Sin esto se cuentan los
        # ejemplos del comentario de la propia puerta, que escribe
        # `//     #define NSTD_QUIERO_INT128_PARAM` como parte de la
        # explicacion: 52 declaraciones reales salian como 54.
        #
        # Y las cabeceras de la familia no cuentan aunque lo declararan: la
        # puerta esta en su frontera, y una familia que se autorizara a si
        # misma no tendria puerta.
        if not _familia(f) and DECLARACION.search(texto):
            declaran.append(f.relative_to(PROJECT_ROOT).as_posix())
        # Los alias internos son asunto de la familia. Que los use alguien de
        # fuera seria abrir una tercera valvula justo despues de cerrar dos.
        if not _familia(f) and "_interno_t" in texto:
            internos_fuera.append(f.relative_to(PROJECT_ROOT).as_posix())

    n = len(declaran)
    if n > QUIERO_INT128_PARAM_TECHO:
        nuevos = "\n".join("  " + d for d in sorted(declaran)[-6:])
        rep.fail(
            f"{n} ficheros declaran NSTD_QUIERO_INT128_PARAM, y el techo es "
            f"{QUIERO_INT128_PARAM_TECHO}",
            "La familia se retira (ADR-006): la lista puede bajar, no crecer.\n"
            "Si el fichero nuevo puede usar el tipo nuevo, usalo; si de verdad "
            "necesita el viejo,\nsube el techo A MANO y di por que en el commit."
            f"\nAlgunos de los que declaran:\n{nuevos}")
    elif n < QUIERO_INT128_PARAM_TECHO:
        rep.ok(f"{n} ficheros declaran NSTD_QUIERO_INT128_PARAM "
               f"(el techo son {QUIERO_INT128_PARAM_TECHO}: BAJALO)")
    else:
        por = {}
        for d in declaran:
            por[d.split("/")[0]] = por.get(d.split("/")[0], 0) + 1
        detalle = ", ".join(f"{v} en {k}" for k, v in sorted(por.items()))
        rep.ok(f"{n} ficheros declaran NSTD_QUIERO_INT128_PARAM, igual que el "
               f"techo ({detalle})")

    if internos_fuera:
        rep.fail(
            f"{len(internos_fuera)} fichero(s) de FUERA de la familia usan los "
            "alias internos",
            "`uint128_interno_t` y sus hermanos no llevan [[deprecated]] a "
            "proposito: son\nel apano interno de la familia, no una puerta "
            "trasera para el resto.\n" + "\n".join("  " + x for x in internos_fuera))
    else:
        rep.ok("nadie de fuera de la familia usa los alias internos sin marcar")


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
    check_superficie_legado(rep)
    check_indice_adr(rep)
    check_bloque_estado(rep)
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
