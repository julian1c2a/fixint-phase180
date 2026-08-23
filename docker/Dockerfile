# =============================================================================
# Dockerfile - Sistema de compilación centralizado para int128 project
# =============================================================================
# Compila y ejecuta tests, benchmarks y demos con múltiples compiladores.
# Arquitectura: linux/amd64 (compiladores: GCC, Clang, Intel ICX)
#
# Build:
#   docker build -t fixint-build:latest -f docker/Dockerfile .
#
# Run:
#   docker run --rm fixint-build:latest python3 make.py test
#   docker run --rm fixint-build:latest python3 make.py bench
#   docker run --rm fixint-build:latest python3 make.py all
#
# Con volúmenes:
#   docker run --rm -v $(pwd)/build:/project/build fixint-build:latest python3 make.py test
# =============================================================================

FROM ubuntu:24.04 AS base

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# Instalar herramientas de compilación base
RUN apt-get update && apt-get install -y \
    # Utilidades esenciales
    curl \
    wget \
    git \
    ca-certificates \
    # Python 3 y pip
    python3 \
    python3-pip \
    python3-dev \
    # CMake y Ninja
    cmake \
    ninja-build \
    # Compiladores estándar
    build-essential \
    gcc-13 \
    g++-13 \
    gcc-14 \
    g++-14 \
    clang-18 \
    clang-19 \
    # Herramientas de análisis
    cppcheck \
    clang-tools-19 \
    # Herramientas de debugging
    gdb \
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
# Stage 2: Compiladores adicionales (Intel ICX)
# =============================================================================
FROM base AS compilers

# Instalar Intel OneAPI (GCC, Clang, Intel Compiler)
RUN apt-get update && apt-get install -y \
    # Dependencias para Intel oneapi-compiler-dpcpp-cpp
    libstdc++-14-dev \
    # Instalador de Intel
    wget \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Descargar e instalar Intel oneAPI compiler package (opcional - comentado por tamaño)
# RUN wget https://apt.repos.intel.com/oneapi/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB && \
#     apt-key add GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB && \
#     echo "deb https://apt.repos.intel.com/oneapi all main" > /etc/apt/sources.list.d/oneAPI.list && \
#     apt-get update && \
#     apt-get install -y intel-oneapi-compiler-dpcpp-cpp-2024.0 && \
#     rm -rf /var/lib/apt/lists/*

# Crear symlinks de los compiladores
# T7.2 (auditoria 23 ago 2026): la imagen traia GCC 12 y Clang 14 mientras el
# CI validaba GCC 13-16 y Clang 18-22. Comprobado en contenedor real: GCC 12
# compilaba y pasaba la suite, pero Clang 14 fallaba al compilar
# test_param_format. Ahora la imagen usa GCC 14 y Clang 19, dentro del rango
# que el CI valida de verdad.
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-14 100 && \
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-19 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-19 100

# Instalar herramientas Python adicionales
# Ubuntu 24.04 aplica PEP 668 (entorno gestionado): pip necesita permiso
# explicito para instalar en el Python del sistema, aunque sea un contenedor.
RUN pip3 install --no-cache-dir --break-system-packages \
    colorama \
    setuptools \
    wheel

# =============================================================================
# Stage 3: Código del proyecto
# =============================================================================
FROM compilers AS project

WORKDIR /project

# Copiar archivos de configuración
COPY CMakeLists.txt CMakePresets.json Makefile make.py ./
COPY .clang-tidy ./

# Copiar directorio de headers (sin compilar aún)
COPY include/ ./include/

# Copiar scripts de compilación
COPY scripts/ ./scripts/
COPY cmake/ ./cmake/

# Copiar tests, benchmarks y demos
COPY tests/ ./tests/
COPY benchs/ ./benchs/
COPY demos/ ./demos/

# Copiar docker/ para que make.py tenga acceso
COPY docker/ ./docker/

# =============================================================================
# Stage 4: Ambiente de ejecución y compilación
# =============================================================================
FROM project AS runtime

WORKDIR /project

# Variables de entorno para compilación
ENV CMAKE_BUILD_TYPE=Release
# OJO: `ENV` no ejecuta un shell, asi que `ENV MAKEFLAGS="-j$(nproc)"` dejaba
# el literal `-j$(nproc)` en la variable y make lo rechazaba. El paralelismo se
# fija en tiempo de ejecucion desde el entrypoint (T7.2).
ENV CFLAGS="-O2"
ENV CXXFLAGS="-O2"

# Script de entrada para ejecución flexible
COPY docker/entrypoint.sh /project/entrypoint.sh
RUN chmod +x /project/entrypoint.sh

# Crear volumen para outputs de build
VOLUME ["/project/build"]

# Default command: compilar y ejecutar todos los tests
ENTRYPOINT ["/project/entrypoint.sh"]
CMD ["test"]

# =============================================================================
# Información útil
# =============================================================================

LABEL maintainer="Julian Calderón Almendros <julian1c2a@gmail.com>"
LABEL description="int128/uint128 - Automated Build & Test System"
LABEL version="1.0"

# =============================================================================
# EOF
# =============================================================================
