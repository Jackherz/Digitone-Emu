#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
BUILD_DIR=${DN1_BUILD_DIR:-"${ROOT_DIR}/build-macos-arm64"}
INSTALL_DIR=${DN1_VST3_INSTALL_DIR:-"${HOME}/Library/Audio/Plug-Ins/VST3"}
SDK_DIR=${VST3_SDK_DIR:-"${ROOT_DIR}/third_party/vst3sdk"}

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script is for macOS. Use the portable engine test on other hosts." >&2
    exit 1
fi
if [[ "$(uname -m)" != "arm64" ]]; then
    echo "This first build targets Apple silicon (arm64); detected $(uname -m)." >&2
    exit 1
fi

# The VST3 SDK checks xcodebuild itself. Command Line Tools alone provide a
# compiler, but this SDK release also requires the full Xcode application.
if ! command -v xcodebuild >/dev/null 2>&1; then
    echo "Full Xcode is required; xcodebuild was not found." >&2
    echo "Install Xcode from the App Store, then select it with:" >&2
    echo "  sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer" >&2
    exit 1
fi
XCODE_VERSION=$(xcodebuild -version 2>/dev/null | awk '/^Xcode / { print $2; exit }')
if [[ -z "${XCODE_VERSION}" ]]; then
    echo "Full Xcode is required, but xcodebuild is not using an Xcode installation." >&2
    echo "Run: sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer" >&2
    exit 1
fi
XCODE_MAJOR=${XCODE_VERSION%%.*}
if [[ ! "${XCODE_MAJOR}" =~ ^[0-9]+$ ]] || (( XCODE_MAJOR < 9 )); then
    echo "Xcode 9 or newer is required; detected ${XCODE_VERSION}." >&2
    exit 1
fi

echo "Using Xcode ${XCODE_VERSION}"

"${ROOT_DIR}/scripts/bootstrap_vst3sdk.sh"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DDN1_BUILD_PLUGIN=ON \
    -DDN1_BUILD_TESTS=ON \
    -Dvst3sdk_SOURCE_DIR="${SDK_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build "${BUILD_DIR}" --config Release --parallel

PLUGIN_PATH=$(find "${BUILD_DIR}" -type d -name 'DN1FM.vst3' -print -quit)
if [[ -z "${PLUGIN_PATH}" ]]; then
    echo "Build completed but DN1FM.vst3 was not found under ${BUILD_DIR}." >&2
    exit 1
fi

mkdir -p "${INSTALL_DIR}"
DESTINATION="${INSTALL_DIR}/DN1FM.vst3"
rm -rf "${DESTINATION}"
ditto "${PLUGIN_PATH}" "${DESTINATION}"
# An ad-hoc signature is enough for a plug-in used only on this Mac. A real
# distribution build should use the owner's Developer ID and notarization.
codesign --force --deep --sign - "${DESTINATION}" >/dev/null

echo
echo "Installed: ${DESTINATION}"
echo "Restart your DAW and rescan VST3 plug-ins if it was already running."
