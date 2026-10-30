#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
SDK_DIR=${VST3_SDK_DIR:-"${ROOT_DIR}/third_party/vst3sdk"}
SDK_TAG=${VST3_SDK_TAG:-v3.8.1_build_84}

if [[ -f "${SDK_DIR}/CMakeLists.txt" ]]; then
    echo "VST3 SDK already present: ${SDK_DIR}"
    exit 0
fi

if [[ -e "${SDK_DIR}" ]]; then
    echo "Refusing to use a non-SDK path: ${SDK_DIR}" >&2
    exit 1
fi

mkdir -p "$(dirname "${SDK_DIR}")"
echo "Cloning Steinberg VST3 SDK ${SDK_TAG} into ${SDK_DIR}"
git clone --depth 1 --branch "${SDK_TAG}" --recurse-submodules \
    https://github.com/steinbergmedia/vst3sdk.git "${SDK_DIR}"

echo "VST3 SDK ready. The SDK is fetched locally and is not committed by this project."
