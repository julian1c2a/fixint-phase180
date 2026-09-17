#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Comprueba A MAQUINA la precondicion `hi < d` de los nucleos de division.

POR QUE EXISTE
--------------
`detail::div_128_64_hi_menor_que_d` exige `hi < d`. Desde que en x86-64 usa una
instruccion `divq` en vez de una llamada a `__udivti3`, esa precondicion dejo de
ser decorativa: si se rompiera, el procesador lanza `#DE` y mata el proceso.

Los tres llamantes la cumplen por construccion --`div_un_limbo` la tiene como
invariante del bucle, el estimador de Knuth comprueba `u0 < v1` antes de llamar,
y el camino de N=2 obtiene `r_hi = a[1] % d`--. Pero eso es un RAZONAMIENTO, y
este proyecto ya ha visto varias veces que un razonamiento correcto sobre una
lista escrita de memoria se queda corto.

Asi que se comprueba: se compila la suite ENTERA con
`NSTD_DIV_COMPRUEBA_PRECONDICIONES`, que hace que la funcion verifique `hi < d`
en cada llamada y aborte diciendo cual se rompio. Si algun test la viola, lo
dice; si ninguno lo hace, la precondicion queda ejercitada en millones de
llamadas en vez de argumentada.

Es la misma filosofia que `check_matriz_paridad.py`: comprobar compilando y
ejecutando, no fiarse de una lista.

USO
---
    python scripts/check_precondiciones_div.py [compilador]

Por defecto clang. Devuelve 0 si ningun test rompe la precondicion.
"""
import os
import subprocess
import sys
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(RAIZ / 'scripts'))
from env_setup.compiler_env import CompilerEnvironment  # noqa: E402

MACRO = 'NSTD_DIV_COMPRUEBA_PRECONDICIONES'


# La sonda que ROMPE la precondicion a proposito. Ver `autoprueba()`.
SONDA_ROJA = '''#include "algorithms/div_kernels.hpp"
#include <cstdint>
#include <cstdio>
int main()
{
    volatile std::uint64_t d = 1000;
    volatile std::uint64_t hi = 1000; // hi >= d: el cociente NO cabe en 64 bits
    volatile std::uint64_t lo = 7;
    std::uint64_t rem = 0;
    const std::uint64_t q = nstd::algorithms::detail::div_128_64_hi_menor_que_d(
        static_cast<std::uint64_t>(hi), static_cast<std::uint64_t>(lo),
        static_cast<std::uint64_t>(d), rem);
    std::printf("NO ABORTO: q=%llu rem=%llu\\n", (unsigned long long)q, (unsigned long long)rem);
    return 0;
}
'''


def compila(cc, env, estilo_cl, fuente, exe, tmp, raiz):
    """Compila `fuente` con la macro encendida. Devuelve el codigo de salida."""
    if estilo_cl:
        std = '/Qstd:c++20' if 'icx-cl' in cc else '/std:c++20'
        cmd = [cc, std, '/O2', '/EHsc', '/bigobj', '/D' + MACRO,
               '/I' + str(raiz / 'include'), '/I' + str(raiz / 'tests'), str(fuente),
               '/Fe:' + exe, '/Fo:' + str(tmp) + os.sep, '/Fd:' + str(tmp) + os.sep]
    else:
        cmd = [cc, '-std=c++20', '-O2', '-D' + MACRO,
               '-I' + str(raiz / 'include'), '-I' + str(raiz / 'tests'), str(fuente), '-o', exe]
    return subprocess.run(cmd, capture_output=True, text=True, env=env, cwd=str(raiz))


def autoprueba(cc, env, estilo_cl, tmp) -> bool:
    """¿Sabe esta comprobacion ponerse en ROJO?

    Un verificador que no puede fallar no vale nada, y este proyecto ya ha pagado
    varias luces verdes que no podian apagarse --la peor, «0 avisos de Doxygen»
    cuando la comprobacion estaba apagada--. Asi que antes de dar por buena la
    suite se compila una sonda que ROMPE la precondicion a proposito y se exige
    que aborte diciendolo.
    """
    f = tmp / 'sonda_roja.cpp'
    f.write_text(SONDA_ROJA, encoding='utf-8')
    exe = str(tmp / 'sonda_roja.exe')
    p = compila(cc, env, estilo_cl, f, exe, tmp, RAIZ)
    if p.returncode != 0:
        print('  [AUTOPRUEBA] la sonda no compila: no se puede validar la comprobacion')
        return False
    r = subprocess.run([exe], capture_output=True, text=True, env=env, cwd=str(RAIZ))
    salida = (r.stdout or '') + (r.stderr or '')
    if 'PRECONDICION ROTA' in salida and r.returncode != 0:
        print('  [AUTOPRUEBA] la comprobacion SABE fallar: aborta y lo dice')
        return True
    print('  [AUTOPRUEBA] ROTA: se violo la precondicion y la comprobacion no salto')
    print('               salida: %s' % salida.strip()[:160])
    return False


def main() -> int:
    nombre = sys.argv[1] if len(sys.argv) > 1 else 'clang'
    ce = CompilerEnvironment(nombre)
    env = ce.get_env()
    cc = ce.get_compiler_cmd()
    estilo_cl = (nombre == 'msvc') or ('icx-cl' in cc)

    tmp = Path(os.environ.get('TEMP', '/tmp')) / 'nstd_precond_div'
    tmp.mkdir(parents=True, exist_ok=True)

    fuentes = sorted((RAIZ / 'tests').glob('test_*.cpp'))
    print('=' * 78)
    print('  La precondicion `hi < d`, comprobada a maquina')
    print('=' * 78)
    print('  compilador: %s   ficheros: %d   macro: %s' % (nombre, len(fuentes), MACRO))
    print()

    # Lo PRIMERO: comprobar que la comprobacion sabe fallar. Si no, lo que venga
    # despues no significa nada, y mas vale decirlo que dar un verde falso.
    if not autoprueba(cc, env, estilo_cl, tmp):
        print()
        print('=' * 78)
        print('  LA COMPROBACION NO ES FIABLE: no se ejecuta la suite')
        print('=' * 78)
        return 2
    print()

    rotos, no_compilan, ok = [], [], 0
    for f in fuentes:
        exe = str(tmp / (f.stem + '.exe'))
        p = compila(cc, env, estilo_cl, f, exe, tmp, RAIZ)
        if p.returncode != 0:
            no_compilan.append(f.name)
            continue

        r = subprocess.run([exe], capture_output=True, text=True, env=env, cwd=str(RAIZ))
        salida = (r.stdout or '') + (r.stderr or '')
        if 'PRECONDICION ROTA' in salida:
            rotos.append((f.name, [l for l in salida.splitlines() if 'PRECONDICION' in l][:1]))
            print('  [ROTA]  %s' % f.name)
        else:
            ok += 1

    print()
    print('=' * 78)
    if rotos:
        print('  %d fichero(s) ROMPEN la precondicion:' % len(rotos))
        for n, ls in rotos:
            print('    - %s' % n)
            for l in ls:
                print('        %s' % l.strip()[:150])
    else:
        print('  %d ficheros ejercitan la precondicion y NINGUNO la rompe' % ok)
    if no_compilan:
        print('  (%d no compilaron con la macro; se ignoran: %s)'
              % (len(no_compilan), ', '.join(no_compilan[:5])))
    print('=' * 78)
    return 1 if rotos else 0


if __name__ == '__main__':
    sys.exit(main())
