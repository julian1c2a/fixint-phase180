# ADR-015: Intel oneAPI se usa en su versión más alta instalada, no fijada

**Estado:** ✅ Aceptado
**Fecha:** 26 August 2026
**Autor:** Julián Calderón Almendros

---

## Contexto

`scripts/env_setup/compiler_env.py` tenía la versión de Intel **cableada**:

```python
icpx = INTEL_ROOT / "compiler" / "2025.3" / "bin" / "icpx.exe"
...
compiler_versions = ["2025.3", "latest"]
```

En la máquina de desarrollo hay **tres** instaladas —2025.3, 2026.0 y 2026.1—,
así que el proyecto llevaba tiempo compilando con **la más vieja de las tres**.
Y no porque alguien lo hubiera decidido: ese `"2025.3"` se escribió cuando era
la única, y se quedó.

### Los dos problemas se tapaban mutuamente

Al quitar el cableado apareció lo que estaba debajo. Las dos versiones de 2026
**no arrancan siquiera para responder a `--version`**:

```
icpx: error #10026: error generating temporary file
```

Es el directorio temporal. Medido el 26 ago 2026:

| `TMP` | 2025.3 | 2026.0 y 2026.1 |
|---|---|---|
| `C:\msys64\tmp` (el de MSYS2, el que hay aquí) | ✅ | ❌ `error #10026` |
| `%LOCALAPPDATA%\Temp` | ✅ | ✅ |
| cualquier otro directorio corriente | ✅ | ✅ |

**La 2025.3 no se ve afectada.** De modo que mientras la versión estuvo cableada
a la vieja, el fallo del `TMP` no podía manifestarse; y mientras el `TMP` fuera
el de MSYS2, subir de versión parecía imposible. Cada uno escondía al otro.

## Decisiones

### 1. Se usa la versión **más alta instalada**, descubriéndola

`INTEL_VERSION = "auto"` enumera `oneAPI/compiler/*/bin/icpx.exe`, ignora el
enlace `latest` —que duplicaría una de ellas— y ordena numéricamente. Hoy
resuelve **2026.1**, con la que la suite pasa **55/55**.

La constante admite una cadena concreta (`"2025.3"`) para fijarla cuando haga
falta, pero **el valor por defecto es descubrir**.

### 2. El entorno de Intel lleva su propio `TMP`

`INTEL_TMP` apunta a `%LOCALAPPDATA%\Temp`. Es parte del entorno aislado que
`compiler_env.py` ya construye para Intel y para MSVC, así que no altera el
`TMP` de nadie más.

Es un **rodeo a un fallo ajeno**, y como tal lleva fecha: si Intel arregla el
`#10026`, esto sobra. Mientras tanto, sin él no se puede usar ninguna versión
posterior a la 2025.3 en esta máquina.

## Por qué aquí «la más nueva» y en clang-format una versión fija

Es la pregunta obvia mirando a
[ADR-013](ADR-013-clang-format-local-22-ci-21.md), que hace justo lo contrario.
La diferencia no es de criterio, es de **qué produce cada herramienta**:

| | clang-format | el compilador |
|---|---|---|
| Qué produce | **el árbol de fuentes** | binarios, que no se versionan |
| Si cambia la versión | aparecen diffs de reformateo | compila igual, o avisa de algo nuevo |
| Qué se quiere | **un resultado idéntico** | **compatibilidad amplia** |

Con clang-format, dos versiones distintas producen dos árboles distintos: hay
que fijarla o el repositorio se llena de ruido. Con el compilador pasa lo
contrario: **el objetivo es que la biblioteca funcione con todos**, y usar el
más nuevo es lo que descubre antes lo que va a romperse.

Y hay un argumento de este mismo caso: **fijar la versión es exactamente lo que
causó el problema**. Un número escrito a mano que nadie revisa no es una
decisión, es una fecha de caducidad silenciosa. El proyecto se pasó meses sin
tocar Intel 2026 porque el `"2025.3"` seguía ahí.

## Alternativas descartadas

- **Fijar 2026.1.** Repite el error dentro de un año, cuando salga la 2027 y
  nadie se acuerde de tocar el fichero.
- **Usar `compiler/latest/`**, que es el enlace que instala Intel. Parece lo
  mismo y no lo es: depende de qué haya decidido apuntar el instalador, que no
  siempre es lo más nuevo ni es estable entre instalaciones. Descubrir y ordenar
  no depende de nadie.
- **Arreglar el `TMP` global de la máquina.** Cambiaría el entorno de MSYS2 y de
  todo lo demás para rodear un fallo de una herramienta. El entorno aislado
  existe justamente para no hacer eso.

## Consecuencias

### Positivas

- Se compila con **el Intel más nuevo que haya**, que es donde antes aparecen
  los avisos y los cambios de conformidad.
- Instalar una versión nueva basta para que se use; no hay que acordarse de
  editar nada.
- Queda medido y escrito por qué las 2026 no arrancaban, que costó encontrar.

### Negativas

- **La compilación deja de ser idéntica entre máquinas.** Dos equipos con
  instalaciones distintas usan compiladores distintos. Se asume: para eso está
  la matriz del CI, que es donde la cobertura sí tiene que ser fija y explícita.
- Una versión nueva **entra sin avisar**. Si rompe algo, se ve al ejecutar la
  suite —que es lo que tiene que pasar—, pero el cambio no queda registrado en
  ningún sitio salvo en lo que haya instalado.

### Lo que no cubre

**El runner del CI, donde Intel sigue sin funcionar** y por un motivo distinto:
`ONEAPI_ROOT` viene vacío y la acción de terceros deja un `CMAKE_PREFIX_PATH`
apuntando a `/opt/intel/oneapi/...`, una ruta de Linux en un runner de Windows.
Esta decisión es de la máquina de desarrollo; el runner es otro problema.

## Referencias

- `scripts/env_setup/compiler_env.py`: `INTEL_VERSION`, `INTEL_TMP`,
  `_intel_versions()`.
- [ADR-013](ADR-013-clang-format-local-22-ci-21.md): el caso contrario, y por
  qué lo es.
- [`PROJECT_STATUS.md`](../../PROJECT_STATUS.md): la deuda del runner.
