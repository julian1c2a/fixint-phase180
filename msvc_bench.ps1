$cl = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe'
$msvc = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717'
$sdk = 'C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0'
$sdklib = 'C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0'
$vcpkg = 'c:\vcpkg\installed\x64-windows'

$env:INCLUDE = "$msvc\include;$sdk\ucrt;$sdk\um;$sdk\shared;$vcpkg\include"
$env:LIB = "$msvc\lib\x64;$sdklib\ucrt\x64;$sdklib\um\x64;$vcpkg\lib"

Set-Location 'C:\msys64\ucrt64\home\julian\CppProjects\int128-phase175'

$exe = 'C:\Temp\bench_msvc_O2.exe'

Write-Output "Compiling benchmark with /O2 ..."
& $cl /O2 /std:c++20 /EHsc /W3 `
    /I include `
    /DFORCE_GMP_TOMMATH `
    /Fo:C:\Temp\ `
    /Fe:$exe `
    benchs\benchmark_vs_builtin.cpp `
    $vcpkg\lib\gmp.lib `
    $vcpkg\lib\tommath.lib
$build_exit = $LASTEXITCODE
Write-Output "BUILD_EXIT=$build_exit"

if ($build_exit -eq 0) {
    Write-Output "Running benchmark..."
    & $exe
}
