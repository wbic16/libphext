#!/usr/bin/env bash
# libphext (C) test step: builds phextio + phesql and runs the Makefile
# regression fetches against the sample phexts. Prerequisite: gcc + make.
set -euo pipefail
cd "$(dirname "$0")/.."
make test
