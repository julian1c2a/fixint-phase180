# Docker Build System para int128/uint128

## Descripción General

Este sistema containeriza **todas las compilaciones** del proyecto int128/uint128, automatizando completamente:

- **Compilación** de tests, benchmarks y demos
- **Ejecución** de tests y benchmarks en múltiples compiladores (GCC, Clang, Intel)
- **Análisis estático** con cppcheck y clang-tidy
- **Sanitizers** (AddressSanitizer, UndefinedBehaviorSanitizer, ThreadSanitizer)
- **Cross-compilation** via QEMU en Docker (arm64, arm32, riscv64)

## Quick Start

### 1. Construir la imagen Docker

```bash
docker build -t fixint-build:latest -f docker/Dockerfile .
```

### 2. Ejecutar tests (default)

```bash
docker run --rm fixint-build:latest
```

### 3. Ejecutar benchmarks

```bash
docker run --rm fixint-build:latest bench
```

## Comandos Disponibles

### Pipelines Completos

```bash
# Compilar y ejecutar todos los tests
docker run --rm fixint-build:latest test

# Compilar y ejecutar todos los benchmarks
docker run --rm fixint-build:latest bench

# Pipeline completo: build + test + bench
docker run --rm fixint-build:latest all

# Pipeline rápido: solo GCC release
docker run --rm fixint-build:latest quick
```

### Compilación Granular

```bash
# Compilar tests de una feature específica
docker run --rm fixint-build:latest build uint128 bits tests gcc release

# Compilar benchmarks con Clang
docker run --rm fixint-build:latest build uint128 cmath benchs clang release

# Compilar un demo específico
docker run --rm fixint-build:latest build demos tutorials 01_basic_operations gcc release
```

### Ejecución Selectiva

```bash
# Ejecutar tests de una feature
docker run --rm fixint-build:latest check uint128 bits gcc debug

# Ejecutar benchmarks con todos los compiladores
docker run --rm fixint-build:latest run uint128 cmath all release

# Ejecutar un demo específico
docker run --rm fixint-build:latest run demos tutorials 01_basic_operations gcc release
```

### Utilidades

```bash
# Listar todas las combinaciones disponibles
docker run --rm fixint-build:latest list

# Análisis estático
docker run --rm fixint-build:latest analyze

# Limpiar directorios de build
docker run --rm fixint-build:latest clean

# Shell interactivo
docker run --rm -it fixint-build:latest /bin/bash
```

## Uso con docker-compose

### Setup

```bash
docker build -t fixint-build:latest -f docker/Dockerfile .
docker compose up build-tests       # Compilar tests
docker compose up run-tests         # Ejecutar tests
docker compose up run-benchmarks    # Ejecutar benchmarks
docker compose up all-pipeline      # Pipeline completo
```

### Servicios Disponibles

```bash
docker compose up build-all         # Compilar todo
docker compose up build-tests       # Compilar solo tests
docker compose up build-benchmarks  # Compilar solo benchmarks

docker compose up run-tests         # Ejecutar todos los tests
docker compose up run-tests-gcc     # Tests con GCC
docker compose up run-tests-clang   # Tests con Clang

docker compose up run-benchmarks    # Ejecutar benchmarks
docker compose up run-benchmarks-gcc    # Benchmarks con GCC
docker compose up run-benchmarks-clang  # Benchmarks con Clang

docker compose up static-analysis   # Análisis estático
docker compose up all-pipeline      # Build + test + bench (secuencial)
docker compose up quick-pipeline    # GCC release (rápido)
```

### Shell Interactivo

```bash
docker compose run --rm bash        # Shell con volúmenes montados
docker compose run --rm demo        # Para ejecutar demos
```

## Persistencia de Builds

Por defecto, los builds se descartan después de la ejecución. Para persistirlos:

```bash
# Montar volumen local
docker run --rm -v $(pwd)/build:/project/build fixint-build:latest test

# Con docker-compose (automático)
docker compose up run-tests         # Los builds se almacenan en build-cache
```

## Volúmenes

El sistema usa dos tipos de volúmenes:

1. **build-cache**: Volumen nombrado tmpfs para builds (4GB, más rápido)
2. **Volumen local**: Opcional, para persistencia de resultados

```bash
# Ver volúmenes creados
docker volume ls | grep fixint

# Inspeccionar volumen
docker volume inspect fixint-build-cache

# Limpiar volúmenes
docker volume rm fixint-build-cache
```

## Variables de Entorno

Configurables en docker-compose.yml o via `-e`:

```bash
docker run -e CMAKE_BUILD_TYPE=Debug fixint-build:latest build uint128 bits tests gcc debug

docker run -e MAKEFLAGS="-j2" fixint-build:latest test

docker run -e CXXFLAGS="-O3 -march=native" fixint-build:latest bench
```

## Análisis Estático

### cppcheck

```bash
docker run --rm fixint-build:latest analyze cppcheck headers
docker run --rm fixint-build:latest analyze cppcheck tests
docker run --rm fixint-build:latest analyze cppcheck all
```

### clang-tidy

```bash
docker run --rm fixint-build:latest analyze clang-tidy headers
docker run --rm fixint-build:latest analyze clang-tidy tests
```

## Cross-Compilation (QEMU)

Compilar y ejecutar tests en otras arquitecturas:

```bash
# ARM64
python3 make.py docker arm64

# ARM32
python3 make.py docker arm32

# RISC-V64
python3 make.py docker riscv64

# Todas las arquitecturas
python3 make.py docker all
```

Nota: Requiere Docker Desktop con soporte para QEMU multi-arch.

## Ejemplos Completos

### Desarrollo Local Rápido

```bash
# Test rápido en GCC release
docker run --rm fixint-build:latest quick
```

### Validación Completa

```bash
# Build + Test + Bench con todos los compiladores
docker run --rm fixint-build:latest all
```

### Desarrollo Iterativo

```bash
# Shell interactivo con volumen persistente
docker run --rm -it -v $(pwd)/build:/project/build fixint-build:latest /bin/bash

# Dentro del contenedor:
python3 make.py build uint128 bits tests gcc debug
python3 make.py check uint128 bits gcc debug
```

### CI/CD Pipeline

```bash
# En GitHub Actions / GitLab CI / Jenkins:
docker build -t fixint-build:latest -f docker/Dockerfile .
docker run --rm fixint-build:latest test
docker run --rm fixint-build:latest bench
docker run --rm fixint-build:latest analyze cppcheck all
```

## Arquitectura del Sistema

### Dockerfile

Multi-stage build:

1. **base**: Ubuntu 22.04 + herramientas base
2. **compilers**: GCC 12, Clang 14, Intel ICX (opcional)
3. **project**: Copia código fuente
4. **runtime**: Entrypoint y configuración final

### entrypoint.sh

Script de entrada flexible que:

- Detecta compiladores disponibles
- Delega a `make.py` según comandos
- Proporciona atajos para pipelines comunes
- Ofrece shell interactivo

### docker-compose.yml

Orquestación de servicios:

- Herencia de configuración (`extends`)
- Dependencias entre servicios
- Volúmenes nombrados y locales
- Red interna compartida

## Optimización de Builds

### Cachés de Docker

El Dockerfile aprovecha cachés multinivel:

```dockerfile
# Esto está cacheado entre builds
FROM ubuntu:22.04
RUN apt-get install ...
COPY CMakeLists.txt ...

# Esto se reconstruye si cambia el código
COPY tests/ ./tests/
```

### Volúmenes tmpfs

Los builds se almacenan en tmpfs (en RAM) para mayor velocidad:

```yaml
volumes:
  build-cache:
    driver_opts:
      type: tmpfs
      device: tmpfs
      o: "size=4g"
```

## Troubleshooting

### Error: "Docker daemon not running"

```bash
# Iniciar Docker Desktop
# O en Linux: sudo systemctl start docker
```

### Build tarda mucho

```bash
# Aumentar memoria y CPUs en Docker Desktop
# Settings > Resources > Memory/CPUs

# O usar `quick` en lugar de `all`
docker run --rm fixint-build:latest quick
```

### Tests fallan en el contenedor pero no localmente

```bash
# Verificar variables de entorno
docker run --rm fixint-build:latest bash
env | grep CMAKE

# Reconstruir imagen
docker build --no-cache -t fixint-build:latest -f docker/Dockerfile .
```

### Volumen cacheado sin actualizar

```bash
# Limpiar volumen
docker volume rm fixint-build-cache

# O con compose
docker compose down -v
```

## Especificaciones

- **Imagen base**: Ubuntu 22.04
- **Compiladores**: GCC 12, Clang 14, Intel ICX (opcional)
- **CMake**: 3.22.1+
- **Python**: 3.10+
- **Tamaño de imagen**: ~2.5GB (GCC + Clang)

## Comandos Útiles

```bash
# Inspeccionar imagen
docker inspect fixint-build:latest

# Ver historial de build
docker history fixint-build:latest

# Limpiar imágenes no usadas
docker image prune -a

# Ver logs de un contenedor
docker logs <container_id>

# Ejecutar contenedor sin ser removido
docker run -it fixint-build:latest bash

# Guardar imagen como tarball
docker save fixint-build:latest | gzip > fixint-build.tar.gz

# Cargar imagen desde tarball
docker load < fixint-build.tar.gz
```

## Notas Finales

- El sistema es **completamente automatizado** y requiere solo comandos simples
- Todos los builds están **aislados** en el contenedor, no contaminan el host
- Los resultados son **reproducibles** en cualquier máquina con Docker
- Soporta **CI/CD directo** sin configuración adicional

Para más detalles, ver `docker/Dockerfile`, `docker/entrypoint.sh` y `docker-compose.yml`.
