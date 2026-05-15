"""
Compiler environment setup for int128 build system.
Provides isolated environments for MSVC and Intel compilers.
"""

import os
import subprocess
from pathlib import Path


# Visual Studio 2026 (version 18)
VCVARSALL = Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat")

def _find_msvc_cl() -> Path:
    """Find the latest cl.exe under VS 18 Community (Hostx64/x64)."""
    base = Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC")
    if not base.exists():
        return Path("cl.exe")
    candidates = sorted(base.glob("*/bin/Hostx64/x64/cl.exe"), reverse=True)
    return candidates[0] if candidates else Path("cl.exe")

MSVC_CL = _find_msvc_cl()

# Intel oneAPI
INTEL_ROOT = Path(r"C:\Program Files (x86)\Intel\oneAPI")
INTEL_SETVARS = INTEL_ROOT / "setvars.bat"


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
            icpx = INTEL_ROOT / "compiler" / "2025.3" / "bin" / "icpx.exe"
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
            print(f"[WARN] vcvarsall.bat not found at {VCVARSALL}")
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
          2. Intel compiler\<ver>\bin in PATH for the icpx.exe itself
          3. Intel compiler\<ver>\include in INCLUDE for Intel-specific headers

        setvars.bat is NOT used here because it often returns exit 1 in
        non-interactive contexts and captures nothing.  The manual approach
        below is more reliable.
        """
        global _intel_env_cache
        if _intel_env_cache is not None:
            return _intel_env_cache.copy()

        # Start from MSVC environment (provides system headers, LIB, etc.)
        env = self._get_msvc_env()

        # Locate the Intel compiler bin and include directories
        compiler_versions = ["2025.3", "latest"]
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
