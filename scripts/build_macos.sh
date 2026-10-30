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

# The VST3 SDK has a hard xcodebuild-version check. Full Xcode is the
# supported path. An opt-in Command Line Tools fallback is also available for
# Makefile/Ninja builds: CLT supplies clang, xcrun, the macOS SDK, and codesign,
# but it does not provide xcodebuild. The fallback supplies the minimum version
# variable the SDK uses for that check; it is intentionally not the default.
CMAKE_EXTRA_ARGS=()
XCODE_VERSION=""
XCODE_BUILD_OUTPUT=""
if command -v xcodebuild >/dev/null 2>&1; then
    if XCODE_BUILD_OUTPUT=$(xcodebuild -version 2>/dev/null); then
        XCODE_VERSION=$(awk '/^Xcode / { print $2; exit }' <<<"${XCODE_BUILD_OUTPUT}")
    fi
fi

if [[ -n "${XCODE_VERSION}" ]]; then
    XCODE_MAJOR=${XCODE_VERSION%%.*}
    if [[ ! "${XCODE_MAJOR}" =~ ^[0-9]+$ ]] || (( XCODE_MAJOR < 9 )); then
        echo "Xcode 9 or newer is required; detected ${XCODE_VERSION}." >&2
        exit 1
    fi
    echo "Using Xcode ${XCODE_VERSION}"
else
    if [[ "${DN1_ALLOW_COMMAND_LINE_TOOLS:-0}" != "1" ]]; then
        echo "Full Xcode is required by the default build; xcodebuild is unavailable." >&2
        echo "Install Xcode and select it with:" >&2
        echo "  sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer" >&2
        echo "Or opt into the unsupported Command Line Tools fallback:" >&2
        echo "  DN1_ALLOW_COMMAND_LINE_TOOLS=1 ./scripts/build_macos.sh" >&2
        exit 1
    fi
    if ! command -v xcrun >/dev/null 2>&1; then
        echo "The Command Line Tools fallback requires xcrun." >&2
        exit 1
    fi
    MACOS_SDK_PATH=$(xcrun --sdk macosx --show-sdk-path)
    if [[ -z "${MACOS_SDK_PATH}" || ! -d "${MACOS_SDK_PATH}" ]]; then
        echo "Could not locate the macOS SDK through xcrun." >&2
        exit 1
    fi
    XCODE_VERSION=${DN1_CLT_XCODE_VERSION:-9.0}
    export XCODE_VERSION
    CMAKE_EXTRA_ARGS+=("-DXCODE_VERSION=${XCODE_VERSION}")
    CMAKE_EXTRA_ARGS+=("-DCMAKE_OSX_SYSROOT=${MACOS_SDK_PATH}")
    echo "Using Command Line Tools and macOS SDK ${MACOS_SDK_PATH}"
    echo "Warning: this bypasses the VST3 SDK's full-Xcode check."
fi

"${ROOT_DIR}/scripts/bootstrap_vst3sdk.sh"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
    -DDN1_BUILD_PLUGIN=ON \
    -DDN1_BUILD_TESTS=ON \
    -Dvst3sdk_SOURCE_DIR="${SDK_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
    "${CMAKE_EXTRA_ARGS[@]}"
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
