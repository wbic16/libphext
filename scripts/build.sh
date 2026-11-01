#!/usr/bin/env bash
# libphext (C) build step: clean rebuild of phextio (and libphext.o).
# The Makefile's default target builds phextio; phesql is built by test.sh.
# Prerequisite: gcc + make.
set -euo pipefail
cd "$(dirname "$0")/.."
make clean
make
