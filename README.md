# DN1FM

A private, clean-room Digitone mk1-inspired FM instrument for Apple-silicon
macOS, exposed as a VST3 plug-in.

This repository does **not** contain Elektron firmware, extracted firmware,
factory sounds, or code copied from a firmware image. The clean-room engine is a
new 8-voice, 4-operator FM implementation with a VST3 adapter. It is intended
as the first milestone for a personal travel instrument, not as a claim of
cycle-accurate Digitone compatibility yet.

## Current milestone

- Apple-silicon macOS target (`arm64`)
- VST3 instrument with a stereo output and MIDI input
- Eight voices, four FM operators, eight signal routings
- Per-operator ratio, fine tune, level, ADSR; amplifier envelope
- The portable engine has pitch bend, sustain, mod wheel, poly pressure, and
  basic MIDI CC handling; the first VST3 adapter currently routes note and
  poly-pressure events
- Filter, pan, master level, parameter automation, and plug-in state saving
- No firmware is required to build or run the current engine
- No custom editor yet; hosts use their generic VST3 parameter editor

The parameter names and signal flow are deliberately an independent starting
point. They should be calibrated against recordings and measurements from the
owned instrument before calling the result a Digitone emulator.

## Build on an Apple-silicon Mac

Install Xcode Command Line Tools, CMake, Git, and a C++17 compiler. From the
repository root:

```sh
./scripts/test_engine.sh
./scripts/build_macos.sh
```

The build script fetches the Steinberg VST3 SDK into the ignored
`third_party/vst3sdk/` directory, builds an arm64 plug-in with a VSTGUI custom
editor, ad-hoc signs it, and installs it at:

```text
~/Library/Audio/Plug-Ins/VST3/DN1FM.vst3
```

Restart the DAW and rescan VST3 plug-ins if it was already open. Open the
plug-in's editor from the DAW to use the custom FM panel; its controls are
bound to the same automatable parameters as the generic editor.

VS Code is fine as the editor, but it does not replace the compiler, SDK, or
code-signing tools. If you only have Apple's Command Line Tools and want to
try the Makefile/Ninja path without full Xcode, opt in explicitly:

```sh
DN1_ALLOW_COMMAND_LINE_TOOLS=1 ./scripts/build_macos.sh
```

This uses `clang`, `xcrun`, the macOS SDK, and `codesign` from the Command Line
Tools while bypassing the Steinberg SDK's `xcodebuild` version check. It is an
unsupported fallback; install full Xcode if it fails or if you later need AU,
iOS, Xcode project generation, or notarized distribution.

To use another SDK location or install location:

```sh
VST3_SDK_DIR="$HOME/src/vst3sdk" \
DN1_VST3_INSTALL_DIR="$HOME/Library/Audio/Plug-Ins/VST3" \
./scripts/build_macos.sh
```

The Steinberg SDK is fetched from its upstream repository and remains outside
this project’s source tree from Git’s point of view. Review its license before
sharing any build.

## Supplying the firmware for local research

Keep your own official `.syx` file outside this repository. The current plug-in
does not load or execute it. The metadata-only helper can make a report without
copying the firmware bytes:

```sh
python3 tools/firmware_manifest.py \
  "$HOME/Downloads/Digitone_and_Digitone_Keys_OS1.44.syx" \
  --redact-path --output reports/digitone-os.json
```

For a compact section-table report, use the standard-library-only helper:

```sh
python3 tools/elektron_container_report.py \
  "$HOME/Downloads/Digitone_and_Digitone_Keys_OS1.44.syx" \
  --redact-path --output reports/digitone-os1.44-container.json
```

`.syx`, `.bin`, `.img`, `firmware/`, and `reports/` are ignored by Git. Do not
commit the firmware, extracted sections, factory content, or reverse-engineered
binary dumps. The metadata recorded from the uploaded OS 1.44 file is in
[`docs/DIGITONE_OS1_44.md`](docs/DIGITONE_OS1_44.md).

For the firmware-backed standalone route, the existing public
[digiemu](https://github.com/irpina/digiemu) project is the relevant reference:
it already documents a Digitone mk1 firmware runner for Apple-silicon macOS and
uses a bring-your-own-firmware workflow. This project does not copy its source
or bundle its firmware. A firmware runner and a native VST3 clean-room synth are
different milestones; the latter is what this branch currently implements.

## REA-assisted research

[REA](https://github.com/morluto/rea) can be used locally as an optional
analysis aid. Keep its workspace outside this repository and record only
high-level, reproducible observations in `docs/`:

- file identity and checksums;
- confirmed container/section boundaries;
- documented CPU and memory facts;
- externally observable parameter and MIDI behavior;
- audio recordings or numeric measurements made on hardware.

REA is not a runtime dependency of DN1FM. See
[`docs/CLEAN_ROOM.md`](docs/CLEAN_ROOM.md) for the evidence boundary and a
suggested workflow.

## Layout

```text
src/engine/       portable clean-room FM engine
src/plugin/       Steinberg VST3 processor, controller, and factory
scripts/          SDK bootstrap, macOS build, and engine test helpers
tests/            portable engine tests
tools/            metadata-only firmware helpers
docs/             scope and research notes
```

## Next milestones

1. Add a small custom editor with the four operator pages and a preset browser.
2. Establish a hardware/reference-recording test set for envelopes, ratios,
   algorithms, pitch bend, and filter behavior.
3. Add the Digitone-style modulation, effects, and parameter locks only when
   their behavior is supported by observations.
4. Add sample-accurate automation and float64 processing for broader host
   compatibility.
5. Decide whether a separate standalone firmware-backed app is wanted; it
   should remain separate from this clean-room plug-in and continue to use a
   user-supplied firmware file.
