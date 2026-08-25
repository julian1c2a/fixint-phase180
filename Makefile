#!/usr/bin/make -f
# =============================================================================
# Makefile — shim sobre make.py
# =============================================================================
#
# Este fichero NO implementa nada. Traduce `make <objetivo>` a la llamada
# equivalente de `make.py`, que es la capa canónica del sistema de construcción
# (ver §13 de AI-GUIDE.md y scripts/README.md).
#
# Antes tenía 472 líneas con su propia lógica de validación y llamaba a scripts
# .bash que ya estaban superados por sus equivalentes en Python. Eso lo
# convertía en una segunda implementación del build que había que mantener en
# paralelo, y que se quedaba atrás. Reconvertido a shim el 25 ago 2026 (T7.6).
#
# Uso:
#   make test                                  suite completa
#   make test COMPILER=gcc MODE=release-O2     con compilador y modo
#   make build FEATURE=bits                    compila una característica
#   make check FEATURE=bits COMPILER=clang
#   make bench
#   make clean
#   make list
#   make help
#
# Cualquier cosa que no esté aquí se hace directamente con make.py, que tiene
# más opciones:  python make.py --help
# =============================================================================

PY      ?= python
MAKEPY  := $(PY) make.py

# Los tres parámetros que aceptaban los objetivos del Makefile viejo.
TYPE     ?= uint128
FEATURE  ?= all
COMPILER ?= all
MODE     ?= all

.DEFAULT_GOAL := help
.PHONY: help init build build_tests build_benchs check run test bench all \
        clean list sanitize static-analysis analyze benchmark-compare compare \
        demo wsl docker

# =============================================================================
# Objetivos
# =============================================================================

help:
	@$(MAKEPY) --help

init:
	@$(MAKEPY) init

build:
	@$(MAKEPY) build $(TYPE) $(FEATURE) tests $(COMPILER) $(MODE)

build_tests: build

build_benchs:
	@$(MAKEPY) build $(TYPE) $(FEATURE) benchs $(COMPILER) $(MODE)

check:
	@$(MAKEPY) check $(TYPE) $(FEATURE) $(COMPILER) $(MODE)

run:
	@$(MAKEPY) run $(TYPE) $(FEATURE) $(COMPILER) $(MODE)

test:
	@$(MAKEPY) test $(COMPILER) $(MODE)

bench:
	@$(MAKEPY) bench $(COMPILER) $(MODE)

all:
	@$(MAKEPY) all

clean:
	@$(MAKEPY) clean

list:
	@$(MAKEPY) list

sanitize:
	@$(MAKEPY) sanitize $(TYPE) $(FEATURE) $(SANITIZER) $(COMPILER) tests

# Los dos nombres con guion del Makefile viejo se conservan como alias, para no
# romper a quien los tuviera en la memoria de los dedos.
static-analysis: analyze
analyze:
	@$(MAKEPY) analyze

benchmark-compare: compare
compare:
	@$(MAKEPY) compare $(COMPILER) $(MODE)

demo:
	@$(MAKEPY) demo $(CATEGORY) $(DEMO) $(COMPILER) $(MODE)

wsl:
	@$(MAKEPY) wsl

docker:
	@$(MAKEPY) docker $(ARCH)
