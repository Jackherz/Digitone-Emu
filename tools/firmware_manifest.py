#!/usr/bin/env python3
"""Create a metadata-only manifest for a user-supplied Elektron .syx file.

This tool never copies, extracts, patches, or executes firmware. It reports a
SHA-256, file size, generic SysEx framing, and a few printable markers so a
local analysis can be referred to without committing the firmware itself.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
from typing import Any


ASCII_MARKER_RE = re.compile(rb"(?:Digitone|OS\s*\d+\.\d+)", re.IGNORECASE)


def inspect_file(path: Path) -> dict[str, Any]:
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()

    messages: list[dict[str, Any]] = []
    in_sysex = False
    current_size = 0
    manufacturer = bytearray()
    manufacturer_done = False

    for byte in data:
        if not in_sysex:
            if byte == 0xF0:
                in_sysex = True
                current_size = 1
                manufacturer.clear()
                manufacturer_done = False
            continue

        current_size += 1
        if byte == 0xF7:
            messages.append(
                {
                    "size": current_size,
                    "manufacturer_id_hex": manufacturer.hex(),
                    "terminated": True,
                }
            )
            in_sysex = False
            continue

        if not manufacturer_done:
            manufacturer.append(byte)
            # SysEx manufacturer IDs are either one byte or the three-byte
            # extended form. This is only framing metadata, not a decoder.
            if len(manufacturer) == 1 and manufacturer[0] != 0x00:
                manufacturer_done = True
            elif len(manufacturer) == 3:
                manufacturer_done = True

    if in_sysex:
        messages.append(
            {
                "size": current_size,
                "manufacturer_id_hex": manufacturer.hex(),
                "terminated": False,
            }
        )

    markers = sorted(
        {match.group(0).decode("ascii", errors="replace") for match in ASCII_MARKER_RE.finditer(data)}
    )
    return {
        "file_name": path.name,
        "file_size": len(data),
        "sha256": digest,
        "sysex_message_count": len(messages),
        "sysex_messages": messages,
        "printable_markers": markers,
        "firmware_bytes_included": False,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("firmware", type=Path, help="path to your local .syx file")
    parser.add_argument(
        "--output",
        type=Path,
        help="optional JSON report path; the firmware is never written there",
    )
    parser.add_argument(
        "--redact-path",
        action="store_true",
        help="omit the input path from the human-readable report",
    )
    args = parser.parse_args()

    path = args.firmware.expanduser().resolve()
    if not path.is_file():
        parser.error(f"firmware file does not exist: {path}")

    report = inspect_file(path)
    if not args.redact_path:
        report["input_path"] = str(path)

    rendered = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
