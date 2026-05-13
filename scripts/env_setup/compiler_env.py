"""
Compiler environment setup for int128 build system.
Provides isolated environments for MSVC and Intel compilers.
"""

import os
import subprocess
from pathlib import Path


# Visual Studio 2026 (version 18)
VCVARSALL = Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat")
MSVC_CL = Path(r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe")

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
        """Get Intel oneAPI environment (includes MSVC backend)."""
        global _intel_env_cache
        if _intel_env_cache is not None:
            return _intel_env_cache.copy()

        if INTEL_SETVARS.exists():
            env = _capture_env_from_bat(str(INTEL_SETVARS))
            if env:
                _intel_env_cache = env
                return env.copy()

        # Fallback: start with MSVC env and add Intel paths
        env = self._get_msvc_env()
        compiler_bin = INTEL_ROOT / "compiler" / "2025.3" / "bin"
        if compiler_bin.exists():
            env["PATH"] = str(compiler_bin) + os.pathsep + env.get("PATH", "")
        _intel_env_cache = env
        return env.copy()
