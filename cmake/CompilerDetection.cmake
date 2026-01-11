# =============================================================================
# CompilerDetection.cmake - Detección de compiladores para int128 Project
# =============================================================================
#
# Detecta y configura: GCC, Clang, MSVC, Intel oneAPI (icx/icpx)
# Soporta: Windows (MSYS2), Linux, WSL, macOS
# Establece flags específicos de compilador y warnings
#
# Variables definidas:
#   INT128_COMPILER_ID       : gcc | clang | msvc | intel
#   INT128_COMPILER_VERSION  : Versión del compilador (X.Y.Z)
#   INT128_PLATFORM          : windows | wsl | linux | macos | unknown
#   INT128_COMPILER_FULL_ID  : gcc-15-wsl, clang-19-windows, etc.
#   INT128_CXX_FLAGS         : Flags de compilación comunes
#   INT128_WARNING_FLAGS     : Flags de warnings
#
# =============================================================================

# =============================================================================
# Detección de plataforma
# =============================================================================

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(INT128_PLATFORM "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Distinguir entre WSL y Linux nativo
    if(EXISTS "/proc/version")
        file(READ "/proc/version" PROC_VERSION)
        if(PROC_VERSION MATCHES "Microsoft|WSL")
            set(INT128_PLATFORM "wsl")
        else()
            set(INT128_PLATFORM "linux")
        endif()
    else()
        set(INT128_PLATFORM "linux")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(INT128_PLATFORM "macos")
else()
    set(INT128_PLATFORM "unknown")
endif()

message(STATUS "Platform: ${INT128_PLATFORM} (${CMAKE_SYSTEM_NAME})")

# =============================================================================
# Detección de compilador principal
# =============================================================================

set(INT128_COMPILER_ID "unknown")
set(INT128_COMPILER_VERSION "")
set(INT128_COMPILER_MAJOR "")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(INT128_COMPILER_ID "gcc")
    set(INT128_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    string(REGEX MATCH "^[0-9]+" INT128_COMPILER_MAJOR ${INT128_COMPILER_VERSION})
    message(STATUS "Detected GCC ${INT128_COMPILER_VERSION}")
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(INT128_COMPILER_ID "clang")
    set(INT128_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    string(REGEX MATCH "^[0-9]+" INT128_COMPILER_MAJOR ${INT128_COMPILER_VERSION})
    message(STATUS "Detected Clang ${INT128_COMPILER_VERSION}")
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(INT128_COMPILER_ID "msvc")
    set(INT128_COMPILER_VERSION ${MSVC_VERSION})
    string(REGEX MATCH "^[0-9]+" INT128_COMPILER_MAJOR ${INT128_COMPILER_VERSION})
    message(STATUS "Detected MSVC ${INT128_COMPILER_VERSION}")
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    set(INT128_COMPILER_ID "intel")
    set(INT128_COMPILER_VERSION ${CMAKE_CXX_COMPILER_VERSION})
    string(REGEX MATCH "^[0-9]+" INT128_COMPILER_MAJOR ${INT128_COMPILER_VERSION})
    message(STATUS "Detected Intel oneAPI ${INT128_COMPILER_VERSION}")
    
else()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# Crear identificador completo (ej: gcc-15-wsl, clang-19-windows)
set(INT128_COMPILER_FULL_ID "${INT128_COMPILER_ID}-${INT128_COMPILER_MAJOR}-${INT128_PLATFORM}")
message(STATUS "Compiler full ID: ${INT128_COMPILER_FULL_ID}")

# =============================================================================
# Flags de compilación comunes (C++20)
# =============================================================================

set(INT128_CXX_FLAGS "")
set(INT128_WARNING_FLAGS "")

# Requerir C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================================
# Flags específicos por compilador
# =============================================================================

if(INT128_COMPILER_ID STREQUAL "gcc" OR INT128_COMPILER_ID STREQUAL "clang")
    # GCC y Clang usan flags similares
    list(APPEND INT128_WARNING_FLAGS
        -Wall
        -Wextra
        # -Wpedantic      # Disabled: __int128 triggers ISO C++ warnings
        # -Wconversion    # Disabled: benchmark macros have many conversions
        # -Wsign-conversion
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wformat=2
    )
    
    # Flags de optimización por modo
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND INT128_CXX_FLAGS -O0 -g)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        list(APPEND INT128_CXX_FLAGS -O2)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        list(APPEND INT128_CXX_FLAGS -O2 -g)
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        list(APPEND INT128_CXX_FLAGS -Os)
    endif()
    
    # GCC-specific flags
    if(INT128_COMPILER_ID STREQUAL "gcc")
        list(APPEND INT128_WARNING_FLAGS
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wnull-dereference
        )
    endif()
    
elseif(INT128_COMPILER_ID STREQUAL "msvc")
    # MSVC warnings
    list(APPEND INT128_WARNING_FLAGS
        /W4                 # Nivel máximo de warnings
        /permissive-        # Modo de conformidad estricta
        /Zc:__cplusplus     # Macro __cplusplus correcta
        /utf-8              # Encoding UTF-8
    )
    
    # MSVC optimization flags
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND INT128_CXX_FLAGS /Od /Zi)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        list(APPEND INT128_CXX_FLAGS /O2)
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        list(APPEND INT128_CXX_FLAGS /O2 /Zi)
    elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        list(APPEND INT128_CXX_FLAGS /O1)
    endif()
    
    # Suprimir warnings específicos de MSVC
    list(APPEND INT128_CXX_FLAGS
        /wd4244  # Conversión implícita con posible pérdida de datos
        /wd4267  # Conversión de size_t a tipo más pequeño
    )
    
elseif(INT128_COMPILER_ID STREQUAL "intel")
    # Intel oneAPI (similar a GCC/Clang)
    list(APPEND INT128_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
    )
    
    # Intel optimization flags
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(WIN32)
            list(APPEND INT128_CXX_FLAGS /Od /Zi)
        else()
            list(APPEND INT128_CXX_FLAGS -O0 -g)
        endif()
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(WIN32)
            list(APPEND INT128_CXX_FLAGS /O2)
        else()
            list(APPEND INT128_CXX_FLAGS -O2)
        endif()
    endif()
endif()

# =============================================================================
# Flags de hardening (seguridad)
# =============================================================================

if(INT128_COMPILER_ID STREQUAL "gcc" OR INT128_COMPILER_ID STREQUAL "clang")
    list(APPEND INT128_CXX_FLAGS
        -fstack-protector-strong    # Protección de stack
        -D_FORTIFY_SOURCE=2         # Protección funciones libc
    )
    
    # Position Independent Executable (PIE)
    if(NOT WIN32)
        list(APPEND INT128_CXX_FLAGS -fPIE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
    endif()
    
    # Control Flow Integrity (CFI) - solo en arquitecturas soportadas
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        list(APPEND INT128_CXX_FLAGS -fcf-protection=full)
    endif()
endif()

# =============================================================================
# Threading support (requerido para thread_safety feature)
# =============================================================================

find_package(Threads REQUIRED)

if(INT128_COMPILER_ID STREQUAL "gcc" OR INT128_COMPILER_ID STREQUAL "clang")
    # GCC/Clang requieren -pthread y -latomic
    list(APPEND INT128_CXX_FLAGS -pthread)
    set(INT128_THREAD_LIBS Threads::Threads atomic)
elseif(INT128_COMPILER_ID STREQUAL "msvc" OR INT128_COMPILER_ID STREQUAL "intel")
    # MSVC e Intel manejan threading automáticamente
    set(INT128_THREAD_LIBS Threads::Threads)
endif()

# =============================================================================
# Exportar variables para uso en otros CMakeLists.txt
# =============================================================================

# Convertir listas a strings para add_compile_options
string(REPLACE ";" " " INT128_CXX_FLAGS_STR "${INT128_CXX_FLAGS}")
string(REPLACE ";" " " INT128_WARNING_FLAGS_STR "${INT128_WARNING_FLAGS}")

message(STATUS "INT128_COMPILER_ID: ${INT128_COMPILER_ID}")
message(STATUS "INT128_COMPILER_VERSION: ${INT128_COMPILER_VERSION}")
message(STATUS "INT128_CXX_FLAGS: ${INT128_CXX_FLAGS_STR}")
message(STATUS "INT128_WARNING_FLAGS: ${INT128_WARNING_FLAGS_STR}")
message(STATUS "INT128_THREAD_LIBS: ${INT128_THREAD_LIBS}")

# =============================================================================
# Helper function: Configurar target con flags de compilador
# =============================================================================

function(int128_configure_target target_name)
    target_compile_options(${target_name} PRIVATE ${INT128_CXX_FLAGS} ${INT128_WARNING_FLAGS})
    target_link_libraries(${target_name} PRIVATE ${INT128_THREAD_LIBS})
    
    # Incluir directorio de headers
    target_include_directories(${target_name} PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
    )
endfunction()

# =============================================================================
# EOF
# =============================================================================
