# =============================================================================
# Multi-Compiler Test Suite: GCC, Clang, MSVC, Intel (PowerShell Version)
# =============================================================================
# Propósito: Compilar y testear division algorithm con todos los compiladores
# Uso: .\compile_all_compilers.ps1
# =============================================================================

$ErrorActionPreference = "Continue"

$ProjectRoot = (Get-Location).Path
$TestFile = "tests\test_divmod_final.cpp"
$BuildDir = "build"
$IncludeDir = "include"

# Colors
$Red = "`e[31m"
$Green = "`e[32m"
$Yellow = "`e[33m"
$Blue = "`e[34m"
$Reset = "`e[0m"

# Ensure build directory exists
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

Write-Host "`n$Blue==========================================================================$Reset"
Write-Host "$Blue  Multi-Compiler Division Test Suite$Reset"
Write-Host "$Blue==========================================================================$Reset`n"

# =============================================================================
# GCC -O0 (Baseline)
# =============================================================================
Write-Host "$Yellow[1/4] GCC -O0 (Baseline - No Optimization)$Reset"
if (Get-Command g++ -ErrorAction SilentlyContinue) {
    & g++ -std=c++20 -O0 -I"$IncludeDir" "$TestFile" -o "$BuildDir\test_gcc_o0.exe" 2>&1 | Select-Object -First 5
    if (Test-Path "$BuildDir\test_gcc_o0.exe") {
        Write-Host "$Green[OK]$Reset Compilation successful"
        Write-Host "[RUNNING]"
        & ".\$BuildDir\test_gcc_o0.exe"
        Write-Host ""
    }
} else {
    Write-Host "$Red[SKIP]$Reset g++ not found in PATH`n"
}

# =============================================================================
# Clang -O2 (Optimized)
# =============================================================================
Write-Host "$Yellow[2/4] Clang -O2 (Optimized)$Reset"
if (Get-Command clang++ -ErrorAction SilentlyContinue) {
    & clang++ -std=c++20 -O2 -I"$IncludeDir" "$TestFile" -o "$BuildDir\test_clang_o2.exe" 2>&1 | Select-Object -First 5
    if (Test-Path "$BuildDir\test_clang_o2.exe") {
        Write-Host "$Green[OK]$Reset Compilation successful"
        Write-Host "[RUNNING]"
        & ".\$BuildDir\test_clang_o2.exe"
        Write-Host ""
    }
} else {
    Write-Host "$Red[SKIP]$Reset clang++ not found in PATH`n"
}

# =============================================================================
# MSVC 2026
# =============================================================================
Write-Host "$Yellow[3/4] MSVC 2026 (Windows Native Compiler)$Reset"
$CLPath = "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
if (Test-Path $CLPath) {
    & $CLPath /std:c++latest /O2 /I"$IncludeDir" /Fe:"$BuildDir\test_msvc.exe" "$TestFile" 2>&1 | Select-Object -First 10
    if (Test-Path "$BuildDir\test_msvc.exe") {
        Write-Host "$Green[OK]$Reset Compilation successful"
        Write-Host "[RUNNING]"
        & ".\$BuildDir\test_msvc.exe"
        Write-Host ""
    } else {
        Write-Host "$Red[FAIL]$Reset Compilation failed`n"
    }
} elseif (Get-Command cl.exe -ErrorAction SilentlyContinue) {
    & cl.exe /std:c++latest /O2 /I"$IncludeDir" /Fe:"$BuildDir\test_msvc.exe" "$TestFile" 2>&1 | Select-Object -First 10
    if (Test-Path "$BuildDir\test_msvc.exe") {
        Write-Host "$Green[OK]$Reset Compilation successful"
        Write-Host "[RUNNING]"
        & ".\$BuildDir\test_msvc.exe"
        Write-Host ""
    } else {
        Write-Host "$Red[FAIL]$Reset Compilation failed`n"
    }
} else {
    Write-Host "$Red[SKIP]$Reset cl.exe (MSVC) not found in PATH"
    Write-Host "         Try: source legacy-code/int128-phase166/scripts/env_setup/setup_msvc_env.bash`n"
}

# =============================================================================
# Intel ICX (Intel oneAPI Compiler)
# =============================================================================
Write-Host "$Yellow[4/4] Intel ICX (Intel oneAPI Compiler)$Reset"
if (Get-Command icx -ErrorAction SilentlyContinue) {
    & icx /std:c++20 /O2 /I"$IncludeDir" /Fe:"$BuildDir\test_intel.exe" "$TestFile" 2>&1 | Select-Object -First 10
    if (Test-Path "$BuildDir\test_intel.exe") {
        Write-Host "$Green[OK]$Reset Compilation successful"
        Write-Host "[RUNNING]"
        & ".\$BuildDir\test_intel.exe"
        Write-Host ""
    } else {
        Write-Host "$Red[FAIL]$Reset Compilation failed`n"
    }
} else {
    Write-Host "$Red[SKIP]$Reset icx (Intel oneAPI) not found in PATH"
    Write-Host "         Try: source legacy-code/int128-phase166/scripts/env_setup/setup_intel_env.bash`n"
}

# =============================================================================
# Summary
# =============================================================================
Write-Host "$Blue==========================================================================$Reset"
Write-Host "$Blue  Test Summary$Reset"
Write-Host "$Blue==========================================================================$Reset`n"
Write-Host "Test file: $TestFile"
Write-Host "Build directory: $BuildDir"
Write-Host "Include directory: $IncludeDir`n"
Write-Host "Generated executables:"
Get-Item "$BuildDir\test_*.exe" -ErrorAction SilentlyContinue | ForEach-Object { Write-Host "  $_" }
Write-Host ""
Write-Host "$Green✓ All available compilers tested$Reset`n"
