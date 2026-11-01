#!/usr/bin/env bash
# libphext (C) publish step.
# There is NO package registry for this library: distribution is a git tag
# on the GitHub repository. This script NEVER pushes. With
# PUBLISH_CONFIRM=1 it creates the local tag v0.4.0 and prints the exact
# push command for a human to run.
set -euo pipefail
cd "$(dirname "$0")/.."
VERSION="0.4.0"
echo "== libphext (C) publish: version ${VERSION} =="
echo "-- step 1/2: tests"
"$(dirname "$0")/test.sh"
echo "-- step 2/2: build"
"$(dirname "$0")/build.sh"
echo "Will publish: git tag v${VERSION} on this repository (no registry package exists for the C library)."
if [ "${PUBLISH_CONFIRM:-}" != "1" ]; then
  echo "STOP: dry run only. Re-run with PUBLISH_CONFIRM=1 to create the local tag (the push stays manual)."
  exit 0
fi
if git rev-parse "v${VERSION}" >/dev/null 2>&1; then
  echo "ERROR: tag v${VERSION} already exists locally; not overwriting it." >&2
  exit 1
fi
git tag "v${VERSION}"
echo "Created local tag v${VERSION}. To publish it, a human runs exactly:"
echo "  git push origin v${VERSION}"
