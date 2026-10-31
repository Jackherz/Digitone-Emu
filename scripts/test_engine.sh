#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${DN1_TEST_BUILD_DIR:-"${ROOT_DIR}/build-engine-test"}
CXX=${CXX:-c++}

python3 "${ROOT_DIR}/tools/validate_editor_tags.py"

mkdir -p "${BUILD_DIR}"
"${CXX}" -std=c++17 -Wall -Wextra -Wpedantic -O2 \
    -I"${ROOT_DIR}/src" \
    "${ROOT_DIR}/src/engine/dn1_engine.cpp" \
    "${ROOT_DIR}/tests/test_engine.cpp" \
    -o "${BUILD_DIR}/dn1_engine_tests"
"${BUILD_DIR}/dn1_engine_tests"
