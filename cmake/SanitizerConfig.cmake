# =============================================================================
# SanitizerConfig.cmake - Configuración de sanitizers para int128 Project
# =============================================================================
#
# Configura AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer
# Solo disponible en GCC/Clang (MSVC tiene soporte limitado)
#
# Opciones de configuración:
#   INT128_ENABLE_ASAN   : ON/OFF (AddressSanitizer)
#   INT128_ENABLE_UBSAN  : ON/OFF (UndefinedBehaviorSanitizer)
#   INT128_ENABLE_TSAN   : ON/OFF (ThreadSanitizer)
#   INT128_ENABLE_MSAN   : ON/OFF (MemorySanitizer, solo Clang)
#
# Nota: TSan es incompatible con ASan/MSan
#
# =============================================================================

option(INT128_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(INT128_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(INT128_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(INT128_ENABLE_MSAN "Enable MemorySanitizer (Clang only)" OFF)

set(INT128_SANITIZER_FLAGS "")
set(INT128_SANITIZER_LINK_FLAGS "")

# =============================================================================
# Validar compatibilidad de sanitizers
# =============================================================================

if(INT128_ENABLE_TSAN AND (INT128_ENABLE_ASAN OR INT128_ENABLE_MSAN))
    message(FATAL_ERROR "ThreadSanitizer is incompatible with AddressSanitizer and MemorySanitizer")
endif()

if(INT128_ENABLE_ASAN AND INT128_ENABLE_MSAN)
    message(FATAL_ERROR "AddressSanitizer and MemorySanitizer are incompatible")
endif()

# =============================================================================
# AddressSanitizer (ASan)
# =============================================================================

if(INT128_ENABLE_ASAN)
    if(INT128_COMPILER_ID STREQUAL "gcc" OR INT128_COMPILER_ID STREQUAL "clang")
        message(STATUS "Enabling AddressSanitizer (ASan)")
        list(APPEND INT128_SANITIZER_FLAGS -fsanitize=address)
        list(APPEND INT128_SANITIZER_FLAGS -fno-omit-frame-pointer)
        list(APPEND INT128_SANITIZER_FLAGS -g)
        list(APPEND INT128_SANITIZER_LINK_FLAGS -fsanitize=address)
        
    elseif(INT128_COMPILER_ID STREQUAL "msvc")
        message(STATUS "Enabling AddressSanitizer (ASan) - MSVC")
        list(APPEND INT128_SANITIZER_FLAGS /fsanitize=address)
        
    elseif(INT128_COMPILER_ID STREQUAL "intel")
        message(STATUS "Enabling AddressSanitizer (ASan) - Intel")
        list(APPEND INT128_SANITIZER_FLAGS -fsanitize=address)
        list(APPEND INT128_SANITIZER_LINK_FLAGS -fsanitize=address)
        
    else()
        message(WARNING "AddressSanitizer not supported for ${INT128_COMPILER_ID}")
    endif()
endif()

# =============================================================================
# UndefinedBehaviorSanitizer (UBSan)
# =============================================================================

if(INT128_ENABLE_UBSAN)
    if(INT128_COMPILER_ID STREQUAL "gcc" OR INT128_COMPILER_ID STREQUAL "clang" OR INT128_COMPILER_ID STREQUAL "intel")
        message(STATUS "Enabling UndefinedBehaviorSanitizer (UBSan)")
        list(APPEND INT128_SANITIZER_FLAGS -fsanitize=undefined)
        list(APPEND INT128_SANITIZER_FLAGS -fno-omit-frame-pointer)
        list(APPEND INT128_SANITIZER_FLAGS -g)
        list(APPEND INT128_SANITIZER_LINK_FLAGS -fsanitize=undefined)
        
        # Opciones adicionales de UBSan
        list(APPEND INT128_SANITIZER_FLAGS -fno-sanitize-recover=all)
        
    else()
        message(WARNING "UndefinedBehaviorSanitizer not supported for ${INT128_COMPILER_ID}")
    endif()
endif()

# =============================================================================
# ThreadSanitizer (TSan)
# =============================================================================

if(INT128_ENABLE_TSAN)
    if(INT128_COMPILER_ID STREQUAL "gcc" OR INT128_COMPILER_ID STREQUAL "clang")
        message(STATUS "Enabling ThreadSanitizer (TSan)")
        list(APPEND INT128_SANITIZER_FLAGS -fsanitize=thread)
        list(APPEND INT128_SANITIZER_FLAGS -g)
        list(APPEND INT128_SANITIZER_LINK_FLAGS -fsanitize=thread)
        
    else()
        message(WARNING "ThreadSanitizer not supported for ${INT128_COMPILER_ID}")
    endif()
endif()

# =============================================================================
# MemorySanitizer (MSan) - Solo Clang
# =============================================================================

if(INT128_ENABLE_MSAN)
    if(INT128_COMPILER_ID STREQUAL "clang")
        message(STATUS "Enabling MemorySanitizer (MSan) - Clang only")
        list(APPEND INT128_SANITIZER_FLAGS -fsanitize=memory)
        list(APPEND INT128_SANITIZER_FLAGS -fno-omit-frame-pointer)
        list(APPEND INT128_SANITIZER_FLAGS -g)
        list(APPEND INT128_SANITIZER_LINK_FLAGS -fsanitize=memory)
        
        # MSan requiere recompilar toda la librería estándar
        list(APPEND INT128_SANITIZER_FLAGS -fsanitize-memory-track-origins)
        
    else()
        message(WARNING "MemorySanitizer only supported for Clang")
    endif()
endif()

# =============================================================================
# Aplicar flags de sanitizer si están habilitados
# =============================================================================

if(INT128_SANITIZER_FLAGS)
    # Convertir lista a string
    string(REPLACE ";" " " INT128_SANITIZER_FLAGS_STR "${INT128_SANITIZER_FLAGS}")
    string(REPLACE ";" " " INT128_SANITIZER_LINK_FLAGS_STR "${INT128_SANITIZER_LINK_FLAGS}")
    
    message(STATUS "Sanitizer compile flags: ${INT128_SANITIZER_FLAGS_STR}")
    message(STATUS "Sanitizer link flags: ${INT128_SANITIZER_LINK_FLAGS_STR}")
    
    # Añadir a flags globales
    add_compile_options(${INT128_SANITIZER_FLAGS})
    link_libraries(${INT128_SANITIZER_LINK_FLAGS})
else()
    message(STATUS "No sanitizers enabled")
endif()

# =============================================================================
# Helper function: Configurar target con sanitizers
# =============================================================================

function(int128_add_sanitizers target_name)
    if(INT128_SANITIZER_FLAGS)
        target_compile_options(${target_name} PRIVATE ${INT128_SANITIZER_FLAGS})
        target_link_options(${target_name} PRIVATE ${INT128_SANITIZER_LINK_FLAGS})
    endif()
endfunction()

# =============================================================================
# Modos de build predefinidos con sanitizers
# =============================================================================

# Crear tipos de build personalizados
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;DebugASan;DebugUBSan;DebugTSan" 
    CACHE STRING "Available build types" FORCE)

# DebugASan: Debug + AddressSanitizer
set(CMAKE_CXX_FLAGS_DEBUGASAN "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer"
    CACHE STRING "Flags for DebugASan configuration" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUGASAN "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address"
    CACHE STRING "Linker flags for DebugASan configuration" FORCE)

# DebugUBSan: Debug + UndefinedBehaviorSanitizer
set(CMAKE_CXX_FLAGS_DEBUGUBSAN "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined -fno-omit-frame-pointer"
    CACHE STRING "Flags for DebugUBSan configuration" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUGUBSAN "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=undefined"
    CACHE STRING "Linker flags for DebugUBSan configuration" FORCE)

# DebugTSan: Debug + ThreadSanitizer
set(CMAKE_CXX_FLAGS_DEBUGTSAN "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread"
    CACHE STRING "Flags for DebugTSan configuration" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUGTSAN "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=thread"
    CACHE STRING "Linker flags for DebugTSan configuration" FORCE)

# =============================================================================
# EOF
# =============================================================================
