#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Genera la tabla oraculo de Q64.64 para el test del redondeo del punto fijo.

Es el SEGUNDO oraculo. El primero vive dentro del test y usa `__int128` sobre
los tipos de UN limbo, donde el producto de dos crudos de 64 bits cabe en 128.
Para Q64.64 eso no vale --el producto son 256 bits-- asi que aqui se calcula
con los enteros exactos de Python y se emite como tabla.

**Dos oraculos sobre DOS anchuras.** Con uno solo no habria forma de separar
«el redondeo esta bien» de «el redondeo esta bien en la anchura que mire», que
es el eje sin cruzar que este proyecto ya se ha encontrado cuatro veces.

Python encaja con la formula de ADR-020 sin traducir nada:

  - `>>` sobre negativos da el SUELO, igual que el desplazamiento aritmetico
  - `&` sobre negativos da los bits bajos en complemento a dos, no negativos
  - `//` es division entera HACIA EL SUELO, que es justo la `q` de la formula
  - `%` acompana a `//`, asi que el resto lleva el signo del divisor y la
    fraccion `r/den` queda en `[0,1)`

La tabla de decision se reescribe aqui **a mano**. Importarla del C++ no seria
un oraculo, seria un espejo: coincidiria con el codigo aunque los dos
estuvieran mal.

Uso:  python scripts/genera_tabla_redondeo.py
      (reescribe el bloque marcado dentro de tests/test_fixed_point.cpp)
"""

import pathlib

K = 64          # bits de escala (F = 1 limbo)
N_BITS = 128    # anchura del crudo (N = 2 limbos)

MODOS = ["to_nearest_even", "to_nearest_away", "toward_zero",
         "toward_neg_inf", "toward_pos_inf"]


def con_signo(x, bits):
    """Reduce a complemento a dos de `bits` bits. Es lo que hace `wrap`."""
    x &= (1 << bits) - 1
    return x - (1 << bits) if x >> (bits - 1) else x


def sube(modo, resto_cero, pasa_mitad, empate, q_impar, q_negativo):
    """La tabla de ADR-020, decision 3."""
    if resto_cero:
        return False
    if modo == "to_nearest_even":
        return pasa_mitad or (empate and q_impar)
    if modo == "to_nearest_away":
        return pasa_mitad or (empate and not q_negativo)
    if modo == "toward_zero":
        return q_negativo
    if modo == "toward_neg_inf":
        return False
    if modo == "toward_pos_inf":
        return True
    raise AssertionError(modo)


def producto(a, b, modo):
    """Crudo del producto de dos crudos con signo."""
    p = a * b
    q = p >> K                    # suelo
    r = p & ((1 << K) - 1)        # no negativo
    d = 1 << K
    inc = sube(modo, r == 0, 2 * r > d, 2 * r == d, bool(q & 1), q < 0)
    return con_signo(q + (1 if inc else 0), N_BITS)


def division(a, b, modo):
    """Crudo de la division de dos crudos con signo."""
    num = a << K
    q = num // b                  # Python ya divide hacia el suelo
    r_bruto = num - q * b         # lleva el signo de `b`
    r, d = abs(r_bruto), abs(b)
    inc = sube(modo, r == 0, 2 * r > d, 2 * r == d, bool(q & 1), q < 0)
    return con_signo(q + (1 if inc else 0), N_BITS)


def resto(a, b):
    """Crudo del `%`: el de C++, con el signo del dividendo. EXACTO."""
    q_trunc = abs(a) // abs(b)
    if (a < 0) != (b < 0):
        q_trunc = -q_trunc
    return con_signo(a - q_trunc * b, N_BITS)


def cadena(a, d, modo):
    """La cadena que tiene que devolver `to_string(d)` para el crudo `a`.

    Se redondea el VALOR --no la magnitud-- que es lo unico que hace salir bien
    los modos dirigidos: `toward_pos_inf` sobre -2,55 con una cifra da -2,5, no
    -2,6.

    El signo se decide por el crudo ORIGINAL y no por el resultado, igual que
    `printf`: un negativo que redondea a cero sale `-0.00`.
    """
    num = a * 10 ** d
    q = num >> K                    # suelo
    r = num & ((1 << K) - 1)        # no negativo
    dd = 1 << K
    inc = sube(modo, r == 0, 2 * r > dd, 2 * r == dd, bool(q & 1), q < 0)
    D = q + (1 if inc else 0)

    t = str(abs(D)).rjust(d + 1, "0")
    if d:
        t = t[:-d] + "." + t[-d:]
    return ("-" if a < 0 else "") + t


def limbos(x):
    u = x & ((1 << N_BITS) - 1)
    return u & ((1 << 64) - 1), u >> 64


# =========================================================================
# Los casos. Las esquinas PRIMERO, y elegidas para que el redondeo tenga algo
# que decidir: si todos los productos salieran exactos, los cinco modos
# darian lo mismo y la tabla no distinguiria nada.
# =========================================================================
UNO = 1 << K
MEDIO = 1 << (K - 1)
CUARTO = 1 << (K - 2)


def crudo(entero, frac=0):
    return entero * UNO + frac


PRODUCTOS = [
    ("uno por uno", crudo(1), crudo(1)),
    ("dos por tres", crudo(2), crudo(3)),
    ("medio por medio", MEDIO, MEDIO),
    ("menos medio por medio", -MEDIO, MEDIO),
    ("cuarto por cuarto", CUARTO, CUARTO),

    # EMPATES EXACTOS: el producto cae justo en la mitad de un epsilon, que es
    # el unico sitio donde al-par y alejarse-del-cero se separan.
    ("empate: epsilon por medio", 1, MEDIO),
    ("empate: 3 epsilon por medio", 3, MEDIO),
    ("empate negativo: -epsilon por medio", -1, MEDIO),
    ("empate negativo: -3 eps por medio", -3, MEDIO),
    ("empate: (uno+eps) por medio", UNO + 1, MEDIO),

    # Justo por debajo y por encima del empate.
    ("bajo la mitad", 1, MEDIO - 1),
    ("sobre la mitad", 1, MEDIO + 1),
    ("negativo bajo la mitad", -1, MEDIO - 1),
    ("negativo sobre la mitad", -1, MEDIO + 1),

    # Paridad del suelo: lo unico que separa al-par de alejarse.
    ("empate con suelo par", 2, MEDIO),
    ("empate con suelo impar", 3, MEDIO),
    ("empate suelo par negativo", -2, MEDIO),
    ("empate suelo impar negativo", -3, MEDIO),

    # Valores corrientes con fraccion, sin empate.
    ("1.5 por 1.5", crudo(1, MEDIO), crudo(1, MEDIO)),
    ("-1.5 por 2.25", -crudo(1, MEDIO), crudo(2, CUARTO)),
    ("7.25 por -3.75", crudo(7, CUARTO), -crudo(3, 3 * CUARTO)),
    ("grande por pequeno", crudo(1000000), 3),
]

DIVISIONES = [
    ("uno entre tres", crudo(1), crudo(3)),
    ("menos uno entre tres", -crudo(1), crudo(3)),
    ("uno entre menos tres", crudo(1), -crudo(3)),
    ("menos uno entre menos tres", -crudo(1), -crudo(3)),
    ("dos entre uno", crudo(2), crudo(1)),
    ("uno entre dos", crudo(1), crudo(2)),
    ("epsilon entre dos", 1, crudo(2)),
    ("menos epsilon entre dos", -1, crudo(2)),
    ("tres epsilon entre dos", 3, crudo(2)),
    ("siete entre dos", crudo(7), crudo(2)),
    ("menos siete entre dos", -crudo(7), crudo(2)),
    ("10 entre 4", crudo(10), crudo(4)),
    ("-10 entre 4", -crudo(10), crudo(4)),
    ("1.5 entre 0.25", crudo(1, MEDIO), CUARTO),
    ("cien entre siete", crudo(100), crudo(7)),
    ("-cien entre siete", -crudo(100), crudo(7)),
    ("cien entre menos siete", crudo(100), -crudo(7)),
]

# `to_string`. Lo que hay que ejercitar no son numeros bonitos sino las tres
# cosas que rompen un decimal de longitud fija: el EMPATE en la ultima cifra, el
# ACARREO que se sale de la parte fraccionaria, y los negativos diminutos que
# redondean a cero.
CADENAS = [
    # (nombre, crudo, decimales)
    ("cero, dos cifras", 0, 2),
    ("siete sin cifras", crudo(7), 0),
    ("menos siete sin cifras", -crudo(7), 0),
    ("un medio, una cifra", MEDIO, 1),
    ("dos y medio", crudo(2, MEDIO), 2),
    ("menos dos y medio", -crudo(2, MEDIO), 2),

    # EMPATES en la ultima cifra: 2,5 y 3,5 sin decimales separan al-par de
    # alejarse-del-cero, y en los negativos van al reves.
    ("empate 2,5 sin cifras", crudo(2, MEDIO), 0),
    ("empate 3,5 sin cifras", crudo(3, MEDIO), 0),
    ("empate -2,5 sin cifras", -crudo(2, MEDIO), 0),
    ("empate -3,5 sin cifras", -crudo(3, MEDIO), 0),
    ("empate 0,5 sin cifras", MEDIO, 0),
    ("empate -0,5 sin cifras", -MEDIO, 0),
    ("empate 0,25 con una cifra", CUARTO, 1),
    ("empate -0,25 con una cifra", -CUARTO, 1),

    # ACARREO: la cifra que se lleva NO se queda en la parte fraccionaria.
    ("casi uno, dos cifras", UNO - 1, 2),
    ("casi diez, dos cifras", crudo(9) + UNO - 1, 2),
    ("casi menos diez, dos cifras", -(crudo(9) + UNO - 1), 2),
    ("casi cien, una cifra", crudo(99) + UNO - 1, 1),

    # Negativos diminutos: redondean a cero y salen CON signo, como printf.
    ("un epsilon negativo, dos cifras", -1, 2),
    ("un epsilon positivo, dos cifras", 1, 2),
    ("menos tres cuartos, una cifra", -(3 * CUARTO), 1),

    # Unos cuantos corrientes, y con muchas cifras.
    ("un tercio aproximado, 8 cifras", (UNO // 3), 8),
    ("menos un tercio, 8 cifras", -(UNO // 3), 8),
    ("mil y pico, 4 cifras", crudo(1000, CUARTO), 4),
    ("epsilon, 20 cifras", 1, 20),
]

RESTOS = [
    ("7 mod 2", crudo(7), crudo(2)),
    ("-7 mod 2", -crudo(7), crudo(2)),
    ("7 mod -2", crudo(7), -crudo(2)),
    ("-7 mod -2", -crudo(7), -crudo(2)),
    ("5.5 mod 2", crudo(5, MEDIO), crudo(2)),
    ("-5.5 mod 2", -crudo(5, MEDIO), crudo(2)),
    ("0.75 mod 0.5", 3 * CUARTO, MEDIO),
    ("epsilon mod uno", 1, crudo(1)),
]


def par(x):
    lo, hi = limbos(x)
    return "{0x%016XULL, 0x%016XULL}" % (lo, hi)


def fila_bin(nombre, a, b, valores):
    vs = ", ".join(par(v) for v in valores)
    return ('    {"%s", %s, %s,\n     {%s}},' % (nombre, par(a), par(b), vs))


def main():
    lineas = [
        # El bloque es generado, asi que no se formatea a mano. Sin esto,
        # clang-format lo reacomodaria y la siguiente pasada del generador
        # volveria a moverlo: un vaiven perpetuo en el diff.
        "// clang-format off",
        "// GENERADO por scripts/genera_tabla_redondeo.py -- no editar a mano.",
        "//",
        "// Oraculo de Q64.64 en aritmetica exacta de Python. El otro oraculo, el de",
        "// `__int128` sobre los tipos de un limbo, esta escrito a mano mas abajo: dos",
        "// caminos independientes sobre dos anchuras distintas.",
        "",
        "constexpr int kModos = 5;  // los del enum `rounding_mode`, en su orden",
        "",
        "struct CasoBin",
        "{",
        "    const char *nombre;",
        "    std::uint64_t a[2];",
        "    std::uint64_t b[2];",
        "    std::uint64_t esperado[kModos][2];",
        "};",
        "",
        "constexpr CasoBin kProductos[] = {",
    ]
    for nombre, a, b in PRODUCTOS:
        lineas.append(fila_bin(nombre, a, b, [producto(a, b, m) for m in MODOS]))
    lineas += ["};", "", "constexpr CasoBin kDivisiones[] = {"]
    for nombre, a, b in DIVISIONES:
        lineas.append(fila_bin(nombre, a, b, [division(a, b, m) for m in MODOS]))
    lineas += [
        "};",
        "",
        "// El resto es EXACTO, asi que lleva UN valor y no cinco (ADR-020).",
        "struct CasoResto",
        "{",
        "    const char *nombre;",
        "    std::uint64_t a[2];",
        "    std::uint64_t b[2];",
        "    std::uint64_t esperado[2];",
        "};",
        "",
        "constexpr CasoResto kRestos[] = {",
    ]
    for nombre, a, b in RESTOS:
        lineas.append('    {"%s", %s, %s,\n     %s},'
                      % (nombre, par(a), par(b), par(resto(a, b))))
    lineas += [
        "};",
        "",
        "struct CasoCadena",
        "{",
        "    const char *nombre;",
        "    std::uint64_t a[2];",
        "    unsigned decimales;",
        "    const char *esperado[kModos];",
        "};",
        "",
        "constexpr CasoCadena kCadenas[] = {",
    ]
    for nombre, a, d in CADENAS:
        esp = ", ".join('"%s"' % cadena(a, d, m) for m in MODOS)
        lineas.append('    {"%s", %s, %d,\n     {%s}},' % (nombre, par(a), d, esp))
    lineas.append("};")
    lineas.append("// clang-format on")

    salida = "\n".join(lineas)

    # Se empalma DENTRO del test, entre marcas, igual que
    # `check_matriz_paridad.py` hace con docs/MATRIZ_DE_PARIDAD.md. Asi no queda
    # un fichero generado suelto en tests/ que los verificadores tengan que
    # aprender a ignorar.
    ini = "    // ---- TABLA:INICIO -- generada por scripts/genera_tabla_redondeo.py ----"
    fin = "    // ---- TABLA:FIN ----"

    destino = pathlib.Path(__file__).resolve().parent.parent / "tests" / "test_fixed_point.cpp"
    t = destino.read_text(encoding="utf-8")
    if ini not in t or fin not in t:
        raise SystemExit("no encuentro las marcas TABLA:INICIO/TABLA:FIN en %s" % destino)
    i, j = t.index(ini), t.index(fin)
    sangrado = "\n".join(("    " + l) if l.strip() else l for l in salida.splitlines())
    destino.write_text(t[:i] + ini + "\n" + sangrado + "\n" + t[j:], encoding="utf-8")

    print("generados %d productos, %d divisiones, %d restos"
          % (len(PRODUCTOS), len(DIVISIONES), len(RESTOS)))
    print("-> %s" % destino)

    # Cordura del propio generador. Si los cinco modos coincidieran en todos los
    # casos, la tabla pasaria igual con cuatro de ellos mal escritos: seria la
    # misma clase de test vacuo que aparecio el 22 sep con las representaciones.
    problemas = 0
    for titulo, casos, f in (("productos", PRODUCTOS, producto),
                             ("divisiones", DIVISIONES, division)):
        n = sum(1 for _, a, b in casos if len({f(a, b, m) for m in MODOS}) > 1)
        print("%s donde los modos NO coinciden: %d de %d" % (titulo, n, len(casos)))
        if n == 0:
            problemas += 1

    n = sum(1 for _, a, d in CADENAS if len({cadena(a, d, m) for m in MODOS}) > 1)
    print("cadenas donde los modos NO coinciden: %d de %d" % (n, len(CADENAS)))
    if n == 0:
        problemas += 1

    return 1 if problemas else 0


if __name__ == "__main__":
    raise SystemExit(main())
