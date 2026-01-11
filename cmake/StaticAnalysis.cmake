# =============================================================================
# StaticAnalysis.cmake - Configuración de herramientas de análisis estático
# =============================================================================
#
# Integra herramientas de análisis estático para detección de bugs y code smell:
#   - cppcheck: Análisis estático multiplataforma
#   - clang-tidy: Linter y modernizador de código Clang
#   - infer: Análisis estático de Facebook (OSS)
#   - PVS-Studio: Análisis comercial (gratis para OSS)
#
# Uso:
#   cmake -DINT128_ENABLE_CPPCHECK=ON ..
#   cmake --build . --target cppcheck
#
# =============================================================================

option(INT128_ENABLE_CPPCHECK "Enable cppcheck static analysis" OFF)
option(INT128_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)
option(INT128_ENABLE_INFER "Enable infer static analysis" OFF)
option(INT128_ENABLE_PVS_STUDIO "Enable PVS-Studio static analysis" OFF)

# =============================================================================
# cppcheck
# =============================================================================

if(INT128_ENABLE_CPPCHECK)
    find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)
    
    if(CPPCHECK_EXECUTABLE)
        message(STATUS "Found cppcheck: ${CPPCHECK_EXECUTABLE}")
        
        # Configurar argumentos de cppcheck
        set(CPPCHECK_ARGS
            --enable=all
            --std=c++20
            --error-exitcode=1
            --suppress=missingIncludeSystem
            --suppress=unmatchedSuppression
            --inline-suppr
            --quiet
            --verbose
            -I${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/tests
            ${CMAKE_SOURCE_DIR}/benchs
            ${CMAKE_SOURCE_DIR}/demos
        )
        
        # Crear target personalizado
        add_custom_target(cppcheck
            COMMAND ${CPPCHECK_EXECUTABLE} ${CPPCHECK_ARGS}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running cppcheck static analysis..."
            VERBATIM
        )
        
        message(STATUS "Target 'cppcheck' created (run with: cmake --build . --target cppcheck)")
    else()
        message(WARNING "cppcheck not found - target 'cppcheck' will not be available")
        message(STATUS "Install: pacman -S cppcheck (MSYS2) or apt install cppcheck (Linux)")
    endif()
endif()

# =============================================================================
# clang-tidy
# =============================================================================

if(INT128_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy clang-tidy-19 clang-tidy-18)
    
    if(CLANG_TIDY_EXECUTABLE)
        message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXECUTABLE}")
        
        # Buscar archivo .clang-tidy en raíz del proyecto
        set(CLANG_TIDY_CONFIG "${CMAKE_SOURCE_DIR}/.clang-tidy")
        
        if(NOT EXISTS ${CLANG_TIDY_CONFIG})
            message(STATUS "Creating default .clang-tidy configuration")
            file(WRITE ${CLANG_TIDY_CONFIG}
                "---\n"
                "Checks: 'clang-diagnostic-*,clang-analyzer-*,bugprone-*,performance-*,modernize-*,readability-*,-modernize-use-trailing-return-type'\n"
                "WarningsAsErrors: ''\n"
                "HeaderFilterRegex: '.*'\n"
                "FormatStyle: none\n"
            )
        endif()
        
        # Configurar clang-tidy para todos los targets
        set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXECUTABLE}
            -header-filter=.*
            -checks=-*,clang-diagnostic-*,clang-analyzer-*,bugprone-*,performance-*,modernize-*,readability-*
        )
        
        message(STATUS "clang-tidy enabled for all targets")
    else()
        message(WARNING "clang-tidy not found")
        message(STATUS "Install: pacman -S clang (MSYS2) or apt install clang-tidy (Linux)")
    endif()
endif()

# =============================================================================
# infer (Facebook Static Analyzer)
# =============================================================================

if(INT128_ENABLE_INFER)
    find_program(INFER_EXECUTABLE NAMES infer)
    
    if(INFER_EXECUTABLE)
        message(STATUS "Found infer: ${INFER_EXECUTABLE}")
        
        # infer requiere ejecutar el proceso de compilación completo
        add_custom_target(infer
            COMMAND ${INFER_EXECUTABLE} run -- ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running infer static analysis..."
            VERBATIM
        )
        
        message(STATUS "Target 'infer' created (run with: cmake --build . --target infer)")
    else()
        message(WARNING "infer not found - target 'infer' will not be available")
        message(STATUS "Install: See https://fbinfer.com/docs/getting-started")
    endif()
endif()

# =============================================================================
# PVS-Studio
# =============================================================================

if(INT128_ENABLE_PVS_STUDIO)
    find_program(PVS_STUDIO_ANALYZER NAMES pvs-studio-analyzer)
    find_program(PLOG_CONVERTER NAMES plog-converter)
    
    if(PVS_STUDIO_ANALYZER AND PLOG_CONVERTER)
        message(STATUS "Found PVS-Studio: ${PVS_STUDIO_ANALYZER}")
        
        # Target para análisis PVS-Studio
        add_custom_target(pvs-studio
            COMMAND ${PVS_STUDIO_ANALYZER} analyze -o ${CMAKE_BINARY_DIR}/pvs-report.log -j4
            COMMAND ${PLOG_CONVERTER} -a GA:1,2 -t fullhtml ${CMAKE_BINARY_DIR}/pvs-report.log -o ${CMAKE_BINARY_DIR}/pvs-report
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running PVS-Studio static analysis..."
            VERBATIM
        )
        
        message(STATUS "Target 'pvs-studio' created")
        message(STATUS "View report: ${CMAKE_BINARY_DIR}/pvs-report/index.html")
    else()
        message(WARNING "PVS-Studio not found")
        message(STATUS "Install: See https://pvs-studio.com/en/pvs-studio/download/")
        message(STATUS "Free OSS license available at pvs-studio.com")
    endif()
endif()

# =============================================================================
# Helper function: Configurar análisis estático para un target
# =============================================================================

function(int128_enable_static_analysis target_name)
    # Si clang-tidy está habilitado, ya se aplica automáticamente
    # Esta función está disponible para configuración adicional futura
    
    if(INT128_ENABLE_CLANG_TIDY AND CLANG_TIDY_EXECUTABLE)
        set_target_properties(${target_name} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}"
        )
    endif()
endfunction()

# =============================================================================
# Target 'analyze' - Ejecuta todos los análisis habilitados
# =============================================================================

set(ANALYZE_TARGETS "")

if(TARGET cppcheck)
    list(APPEND ANALYZE_TARGETS cppcheck)
endif()

if(TARGET infer)
    list(APPEND ANALYZE_TARGETS infer)
endif()

if(TARGET pvs-studio)
    list(APPEND ANALYZE_TARGETS pvs-studio)
endif()

if(ANALYZE_TARGETS)
    add_custom_target(analyze
        DEPENDS ${ANALYZE_TARGETS}
        COMMENT "Running all enabled static analysis tools..."
    )
    
    message(STATUS "Target 'analyze' created - runs all enabled analysis tools")
endif()

# =============================================================================
# Información de configuración
# =============================================================================

if(INT128_ENABLE_CPPCHECK OR INT128_ENABLE_CLANG_TIDY OR INT128_ENABLE_INFER OR INT128_ENABLE_PVS_STUDIO)
    message(STATUS "")
    message(STATUS "═══════════════════════════════════════════════════════════")
    message(STATUS "  STATIC ANALYSIS CONFIGURATION")
    message(STATUS "═══════════════════════════════════════════════════════════")
    
    if(INT128_ENABLE_CPPCHECK)
        if(CPPCHECK_EXECUTABLE)
            message(STATUS "  cppcheck:    ✅ ENABLED (${CPPCHECK_EXECUTABLE})")
        else()
            message(STATUS "  cppcheck:    ❌ NOT FOUND")
        endif()
    endif()
    
    if(INT128_ENABLE_CLANG_TIDY)
        if(CLANG_TIDY_EXECUTABLE)
            message(STATUS "  clang-tidy:  ✅ ENABLED (${CLANG_TIDY_EXECUTABLE})")
        else()
            message(STATUS "  clang-tidy:  ❌ NOT FOUND")
        endif()
    endif()
    
    if(INT128_ENABLE_INFER)
        if(INFER_EXECUTABLE)
            message(STATUS "  infer:       ✅ ENABLED (${INFER_EXECUTABLE})")
        else()
            message(STATUS "  infer:       ❌ NOT FOUND")
        endif()
    endif()
    
    if(INT128_ENABLE_PVS_STUDIO)
        if(PVS_STUDIO_ANALYZER)
            message(STATUS "  PVS-Studio:  ✅ ENABLED (${PVS_STUDIO_ANALYZER})")
        else()
            message(STATUS "  PVS-Studio:  ❌ NOT FOUND")
        endif()
    endif()
    
    message(STATUS "═══════════════════════════════════════════════════════════")
    message(STATUS "")
else()
    message(STATUS "No static analysis tools enabled")
    message(STATUS "Enable with: -DINT128_ENABLE_CPPCHECK=ON, etc.")
endif()

# =============================================================================
# EOF
# =============================================================================
