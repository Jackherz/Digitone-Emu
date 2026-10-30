# Digitone mk1 OS 1.44: local firmware metadata

This note records metadata and static observations from the owner-supplied
`Digitone_and_Digitone_Keys_OS1.44.syx`. The firmware file itself is not in
this branch. The local copy used for analysis lived under the ignored
`firmware/` directory and was not added to Git.

## Identity

| Field | Value |
|---|---|
| File size | 1,673,760 bytes |
| SHA-256 | `d4f200d04484333d82822db7744e6484d0def8f2db8ddf55ee2b780cc13c9659` |
| SysEx frames | 13,078 |
| Normal frame bodies | 13,076 × 126 bytes |
| Short framing bodies | 2 × 14 bytes |
| Decoded ELE3 container | 1,320,668 bytes |

The SHA-256 matches the OS 1.44 release already documented by the public
[digiemu Digitone mk1 notes](https://github.com/irpina/digiemu/blob/main/DIGITONE-MK1.md).

## Section table

The offsets and lengths below refer to the decoded ELE3 container, not the
original SysEx file. `payload_sha256` hashes the section bytes as stored in the
container. No section payload is reproduced here.

| ID | Offset | Stored bytes | Kind | Destination/version |
|---:|---:|---:|---|---:|
| 5 | `0x0000a0` | 15 | raw metadata | `0x00000000` |
| 2 | `0x0000b0` | 16,052 | packed | `0x02000802` |
| 3 | `0x003f70` | 1,040,348 | packed | `0x40000400` |
| 4 | `0x101f50` | 32,776 | raw with header | `0x80000400` |
| 6 | `0x109f60` | 1,492 | raw | `0x00000000` |
| 7 | `0x10a540` | 126,352 | raw | `0x00000000` |
| 8 | `0x1292d0` | 103,416 | packed | `0x00000000` |

The reproducible metadata command is:

```sh
python3 tools/elektron_container_report.py \
  "$HOME/path/to/Digitone_and_Digitone_Keys_OS1.44.syx" \
  --redact-path
```

The report tool reads transport and section metadata only. It does not
execute, patch, or extract the firmware.

## Static observations relevant to the clean-room engine

A local big-endian 68000-family disassembly of section 7, using its documented
load location, confirms the DSP image begins at `0x40000400` and contains the
boot entry at `0x40000b92`. The image also contains the idle branch at
`0x40000b90` and a render entry at `0x400008fa`. The public reference notes
identify that render as the per-block voice renderer and describe the shared
parameter/output regions and 32-frame audio blocks.

The main OS section contains readable C++ symbol-name strings related to the
Digitone FM UI, including the FM synth and envelope pages and an active
algorithm selector. These names are useful for forming behavior hypotheses;
they are not copied into the plug-in implementation and do not establish exact
parameter offsets or DSP equations.

The current DN1FM engine therefore remains an independent 8-voice, 4-operator
FM implementation. The next calibration work should compare its output against
hardware/reference recordings rather than importing firmware routines or raw
tables.

## REA status

REA was invoked against the local `.syx` with `inspect-firmware-regions`, but
that operation was unavailable in this environment because REA requires a
caller-supplied Binwalk 3.1.x executable. The local metadata and static
observations above were obtained without executing firmware. On a Mac with
Binwalk configured, the corresponding REA command is:

```sh
rea inspect-firmware-regions \
  "$HOME/path/to/Digitone_and_Digitone_Keys_OS1.44.syx" \
  --format json
```

## Provenance warning

The uploaded `.syx` is currently present in the remote repository's `main`
history. Because this repository is hosted on GitHub, remove the firmware from
the remote and rewrite its history if it was not intended to be public. The
working branch intentionally does not contain it.
