"""
Compiler environment setup for int128 build system.
Provides isolated environments for MSVC and Intel compilers.
"""

import os
import subprocess
from pathlib import Path


# =============================================================================
# Localizacion de Visual Studio
#
# T7.1 (24 ago 2026): esta ruta estaba CABLEADA a "Visual Studio\18\Community",
# que es la instalacion de la maquina del autor. En el runner de GitHub hay una
# VS 2022 Enterprise, asi que vcvarsall.bat no existia, la funcion avisaba por
# stderr y seguia adelante SIN entorno de MSVC: cl.exe no estaba en el PATH y
# fallaban los 55 tests. El job de MSVC llevaba meses en rojo por esto.
#
# Ahora se busca en cascada:
#   1. VSINSTALLDIR / VCINSTALLDIR  -- las pone cualquier shell de VS ya
#      inicializada, incluida la del `call vcvars64.bat` del CI
#   2. vswhere.exe                  -- el localizador oficial, siempre en la
#      misma ruta desde VS 2017
#   3. rutas conocidas              -- ultima red, incluida la de esta maquina
# =============================================================================


def _vs_candidates():
    """Rutas de vcvarsall.bat a probar, en orden de preferencia."""
    out = []

    # 1. Entorno de una shell de VS ya inicializada.
    for var in ("VSINSTALLDIR", "VCINSTALLDIR"):
        root = os.environ.get(var)
        if root:
            base = Path(root)
            if var == "VCINSTALLDIR":
                base = base.parent
            out.append(base / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat")

    # 2. vswhere: el localizador oficial de Microsoft.
    vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / \
        "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if vswhere.exists():
        try:
            res = subprocess.run(
                [str(vswhere), "-latest", "-products", "*",
                 "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                 "-property", "installationPath"],
                capture_output=True, text=True, encoding="utf-8", errors="replace",
                timeout=60, check=False)
            for line in res.stdout.splitlines():
                line = line.strip()
                if line:
                    out.append(Path(line) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat")
        except (OSError, subprocess.SubprocessError):
            pass

    # 3. Rutas conocidas, por si las dos anteriores fallan.
    for root in (r"C:\Program Files\Microsoft Visual Studio",
                 r"C:\Program Files (x86)\Microsoft Visual Studio"):
        base = Path(root)
        if not base.exists():
            continue
        for version in sorted((d for d in base.iterdir() if d.is_dir()), reverse=True):
            for edition in ("Enterprise", "Professional", "Community", "BuildTools"):
                out.append(version / edition / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat")

    return out


def _find_vcvarsall() -> Path:
    for cand in _vs_candidates():
        if cand.exists():
            return cand
    # Se devuelve la primera candidata aunque no exista, para que el mensaje de
    # error diga algo util.
    cands = _vs_candidates()
    return cands[0] if cands else Path("vcvarsall.bat")


VCVARSALL = _find_vcvarsall()


def _find_msvc_cl() -> Path:
    """Localiza el cl.exe mas reciente de la instalacion encontrada."""
    # VCVARSALL = <vs>/VC/Auxiliary/Build/vcvarsall.bat  ->  <vs>/VC/Tools/MSVC
    base = VCVARSALL.parent.parent.parent / "Tools" / "MSVC"
    if not base.exists():
        return Path("cl.exe")
    candidates = sorted(base.glob("*/bin/Hostx64/x64/cl.exe"), reverse=True)
    return candidates[0] if candidates else Path("cl.exe")


MSVC_CL = _find_msvc_cl()

# Intel oneAPI
INTEL_ROOT = Path(r"C:\Program Files (x86)\Intel\oneAPI")
INTEL_SETVARS = INTEL_ROOT / "setvars.bat"

# Version de Intel que se usa. "auto" coge la mas alta instalada; una cadena
# como "2025.3" la fija.
#
# ANTES ESTABA CABLEADA A "2025.3" y no era una decision, era un descuido: en
# esta maquina hay 2025.3, 2026.0 y 2026.1, y se estaba cogiendo la mas vieja de
# las tres sin que nadie lo hubiera elegido.
INTEL_VERSION = "auto"

# Directorio temporal para el entorno de Intel.
#
# NO ES UN CAPRICHO. Medido el 26 ago 2026: con TMP apuntando a C:\msys64\tmp
# --que es lo que hay en esta maquina, por MSYS2-- los compiladores 2026.0 y
# 2026.1 fallan hasta para responder a `--version`:
#
#     icpx: error #10026: error generating temporary file
#
# Con %LOCALAPPDATA%\Temp o con cualquier otro directorio funcionan. El 2025.3
# no se ve afectado, que es por lo que el problema paso desapercibido mientras
# la version estuvo cableada a la vieja. Se le da a Intel un TMP que si acepta.
INTEL_TMP = Path(os.environ.get("LOCALAPPDATA", r"C:\Windows\Temp")) / "Temp"


def _intel_versions() -> list:
    """Versiones de Intel instaladas, de la mas nueva a la mas vieja.

    Solo las que tienen `bin/icpx.exe`; se ignora `latest`, que es un enlace y
    duplicaria una de ellas.
    """
    base = INTEL_ROOT / "compiler"
    if not base.exists():
        return []
    vers = []
    for d in base.iterdir():
        if not d.is_dir() or d.name == "latest":
            continue
        if (d / "bin" / "icpx.exe").exists():
            vers.append(d.name)

    def clave(v: str):
        try:
            return tuple(int(x) for x in v.split("."))
        except ValueError:
            return (0,)

    return sorted(vers, key=clave, reverse=True)


def _intel_version_elegida() -> str:
    """La version que se va a usar, segun INTEL_VERSION."""
    disponibles = _intel_versions()
    if INTEL_VERSION != "auto":
        return INTEL_VERSION
    return disponibles[0] if disponibles else "latest"


# Cache: avoid running vcvarsall.bat multiple times per session
_msvc_env_cache = None
_intel_env_cache = None


def _capture_env_from_bat(bat_path: str, args: str = "") -> dict:
    """Run a .bat file via cmd.exe and capture the resulting environment variables."""
    cmd = f'cmd.exe /c ""{bat_path}" {args} >nul 2>&1 && set"'
    try:
        result = subprocess.run(
            cmd,
            shell=True,
            capture_output=True,
            text=True,
            timeout=60
        )
        if result.returncode != 0:
            return {}

        env = {}
        for line in result.stdout.splitlines():
            if '=' in line:
                key, _, value = line.partition('=')
                if key:
                    env[key] = value
        return env
    except Exception:
        return {}


class CompilerEnvironment:
    """Provides isolated compiler environments for build scripts."""

    def __init__(self, compiler_name: str):
        self.compiler_name = compiler_name
        self._env = None

    def get_env(self) -> dict:
        """Get the environment dictionary for this compiler."""
        if self._env is not None:
            return self._env

        if self.compiler_name == "msvc":
            self._env = self._get_msvc_env()
        elif self.compiler_name == "intel":
            self._env = self._get_intel_env()
        elif self.compiler_name in ("gcc", "clang"):
            # Both GCC and Clang live in ucrt64/bin and link against its DLLs.
            # Git's mingw64/bin ships older libstdc++/libunwind that miss C++20
            # entry points — ucrt64/bin must come first in PATH for both.
            self._env = self._get_gcc_env()
        else:
            self._env = os.environ.copy()

        return self._env

    def get_compiler_cmd(self) -> str:
        """Get the full path to the compiler executable."""
        if self.compiler_name == "msvc":
            if MSVC_CL.exists():
                return str(MSVC_CL)
            return "cl.exe"
        elif self.compiler_name == "intel":
            icpx = INTEL_ROOT / "compiler" / _intel_version_elegida() / "bin" / "icpx.exe"
            if icpx.exists():
                return str(icpx)
            icpx_latest = INTEL_ROOT / "compiler" / "latest" / "bin" / "icpx.exe"
            if icpx_latest.exists():
                return str(icpx_latest)
            return "icpx"
        elif self.compiler_name == "gcc":
            ucrt64_gpp = Path(r"C:\msys64\ucrt64\bin\g++.exe")
            if ucrt64_gpp.exists():
                return str(ucrt64_gpp)
            return "g++"
        elif self.compiler_name == "clang":
            return "clang++"
        return ""

    def _get_gcc_env(self) -> dict:
        """Get GCC environment with ucrt64 bin prepended to PATH.

        Git's mingw64/bin ships an older libstdc++-6.dll that lacks std::format
        entry points. Without this fix, Windows picks that DLL first and GCC
        binaries exit with 0xC0000139 (STATUS_ENTRYPOINT_NOT_FOUND).
        """
        env = os.environ.copy()
        ucrt64_bin = r"C:\msys64\ucrt64\bin"
        if Path(ucrt64_bin).exists():
            env["PATH"] = ucrt64_bin + os.pathsep + env.get("PATH", "")
        return env

    def _get_msvc_env(self) -> dict:
        """Get MSVC environment by running vcvarsall.bat x64."""
        global _msvc_env_cache
        if _msvc_env_cache is not None:
            return _msvc_env_cache.copy()

        if not VCVARSALL.exists():
            # Antes esto era un [WARN] y se seguia adelante sin entorno de MSVC,
            # de modo que fallaban los 55 tests con un error incomprensible. Es
            # un fallo de configuracion y debe decirlo claro.
            print(f"[ERROR] No se encuentra vcvarsall.bat. Probado: {VCVARSALL}")
            print("[ERROR] Instala las VC++ Build Tools o abre una shell de "
                  "Visual Studio antes de compilar con msvc.")
            return os.environ.copy()

        env = _capture_env_from_bat(str(VCVARSALL), "x64")
        if not env:
            print("[WARN] Failed to capture MSVC environment from vcvarsall.bat")
            return os.environ.copy()

        _msvc_env_cache = env
        return env.copy()

    def _get_intel_env(self) -> dict:
        """Get Intel ICX environment on Windows.

        ICX (icpx.exe) uses MSVC's standard library headers and needs:
          1. The full MSVC environment (vcvarsall.bat x64) for system headers
          2. Intel compiler/<ver>/bin in PATH for the icpx.exe itself
          3. Intel compiler/<ver>/include in INCLUDE for Intel-specific headers

        setvars.bat is NOT used here because it often returns exit 1 in
        non-interactive contexts and captures nothing.  The manual approach
        below is more reliable.
        """
        global _intel_env_cache
        if _intel_env_cache is not None:
            return _intel_env_cache.copy()

        # Start from MSVC environment (provides system headers, LIB, etc.)
        env = self._get_msvc_env()

        # Un TMP que Intel acepte; ver la nota de INTEL_TMP.
        if INTEL_TMP.exists():
            env["TMP"] = str(INTEL_TMP)
            env["TEMP"] = str(INTEL_TMP)

        # Locate the Intel compiler bin and include directories
        compiler_versions = [_intel_version_elegida(), "latest"]
        for ver in compiler_versions:
            compiler_bin = INTEL_ROOT / "compiler" / ver / "bin"
            compiler_inc = INTEL_ROOT / "compiler" / ver / "include"
            compiler_lib = INTEL_ROOT / "compiler" / ver / "lib"
            if compiler_bin.exists():
                env["PATH"] = str(compiler_bin) + os.pathsep + env.get("PATH", "")
                if compiler_inc.exists():
                    env["INCLUDE"] = str(compiler_inc) + ";" + env.get("INCLUDE", "")
                if compiler_lib.exists():
                    env["LIB"] = str(compiler_lib) + ";" + env.get("LIB", "")
                break

        _intel_env_cache = env
        return env.copy()
