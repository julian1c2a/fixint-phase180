$icpx = 'C:\Program Files (x86)\Intel\oneAPI\compiler\2025.3\bin\icpx.exe'
$msvc = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717'
$sdk = 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0'
$sdklib = 'C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0'
$vcpkg = 'c:\vcpkg\installed\x64-windows'

$env:INCLUDE = "$msvc\include;$sdk\ucrt;$sdk\um;$sdk\shared"
$intel_lib = 'C:\Program Files (x86)\Intel\oneAPI\compiler\2025.3\lib'
$env:LIB = "$msvc\lib\x64;$sdklib\ucrt\x64;$sdklib\um\x64;$vcpkg\lib;$intel_lib"
$env:PATH = 'C:\Program Files (x86)\Intel\oneAPI\compiler\2025.3\bin;C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64;c:\vcpkg\installed\x64-windows\bin;' + $env:PATH

Set-Location 'C:\msys64\ucrt64\home\julian\CppProjects\int128-phase175'

$exe = 'C:\Temp\bench_icpx_O2.exe'

Write-Output "Compiling with ICX -O2 (GCC-style flags) ..."
& $icpx -std=c++20 -O2 -Wall `
    -I include `
    -I "$vcpkg\include" `
    -DFORCE_GMP_TOMMATH `
    benchs/benchmark_vs_builtin.cpp `
    -o $exe `
    "$vcpkg\lib\gmp.lib" `
    "$vcpkg\lib\tommath.lib"
$build_exit = $LASTEXITCODE
Write-Output "BUILD_EXIT=$build_exit"

if ($build_exit -eq 0) {
    Write-Output "Running..."
    & $exe
}
