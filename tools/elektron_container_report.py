#!/usr/bin/env python3
"""Inspect an Elektron .syx transport and ELE3 section table.

This is metadata-only: it decodes the MIDI 8-in-7 transport and reads section
headers, but never decompresses, patches, executes, or writes firmware bytes.
The input stays where the caller supplied it.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from collections import Counter
from pathlib import Path
from typing import Any


MFR = b"\x00\x20\x3c"
COUNT_OFFSET = 0x1C
TABLE_OFFSET = 0x20
ENTRY_SIZE = 16


def decode_transport(data: bytes) -> tuple[bytes, Counter[int], int]:
    """Return decoded bytes, SysEx body-length counts, and frame count."""
    decoded = bytearray()
    body_lengths: Counter[int] = Counter()
    frame_count = 0
    cursor = 0

    while cursor < len(data):
        if data[cursor] != 0xF0:
            raise ValueError(f"expected SysEx F0 at byte offset {cursor}")
        try:
            end = data.index(0xF7, cursor + 1)
        except ValueError as exc:
            raise ValueError(f"unterminated SysEx frame at byte offset {cursor}") from exc

        body = data[cursor + 1 : end]
        body_lengths[len(body)] += 1
        frame_count += 1
        cursor = end + 1

        # Short framing messages carry no payload. A normal frame has a
        # 9-byte header, 116 encoded bytes, and one checksum byte.
        if len(body) != 126:
            continue
        if body[:3] != MFR:
            continue
        payload = body[9:125]
        high_bits = 0
        while high_bits < len(payload):
            mask = payload[high_bits]
            high_bits += 1
            for bit in range(7):
                if high_bits >= len(payload):
                    break
                decoded.append(
                    payload[high_bits] |
                    (0x80 if (mask >> (6 - bit)) & 1 else 0)
                )
                high_bits += 1

    return bytes(decoded), body_lengths, frame_count


def section_table(container: bytes) -> list[dict[str, Any]]:
    if len(container) < TABLE_OFFSET or container[:4] != b"ELE3":
        raise ValueError("decoded transport does not contain an ELE3 container")

    count = struct.unpack_from(">I", container, COUNT_OFFSET)[0]
    table_end = TABLE_OFFSET + ENTRY_SIZE * count
    if table_end > len(container):
        raise ValueError("ELE3 section table extends beyond the container")

    sections: list[dict[str, Any]] = []
    for index in range(count):
        section_id, offset, length, destination = struct.unpack_from(
            ">IIII", container, TABLE_OFFSET + ENTRY_SIZE * index
        )
        end = offset + length
        if end > len(container):
            raise ValueError(f"section {section_id} extends beyond the container")
        payload = container[offset:end]
        kind = "raw"
        declared_length = None
        declared_sum = None
        if len(payload) >= 8:
            declared_length, declared_sum = struct.unpack_from(">II", payload, 0)
            compressed = (
                declared_length + 8 <= len(payload)
                and sum(payload[8 : 8 + declared_length]) & 0xFFFFFFFF
                == declared_sum
            )
            if compressed:
                kind = "packed"
            elif section_id == 4 and declared_sum == 0:
                kind = "raw-with-header"

        sections.append(
            {
                "id": section_id,
                "offset": offset,
                "length": length,
                "destination_or_version": destination,
                "kind": kind,
                "payload_sha256": hashlib.sha256(payload).hexdigest(),
                "declared_length": declared_length,
                "declared_sum_hex": (
                    f"0x{declared_sum:08x}" if declared_sum is not None else None
                ),
            }
        )
    return sections


def make_report(path: Path) -> dict[str, Any]:
    source = path.read_bytes()
    decoded, body_lengths, frame_count = decode_transport(source)
    magic_offset = decoded.find(b"ELE3")
    if magic_offset < 0:
        raise ValueError("ELE3 magic not found in decoded transport")
    container = decoded[magic_offset:]

    return {
        "file_name": path.name,
        "file_size": len(source),
        "sha256": hashlib.sha256(source).hexdigest(),
        "sysex_frame_count": frame_count,
        "sysex_body_length_counts": {
            str(length): count for length, count in sorted(body_lengths.items())
        },
        "decoded_transport_bytes": len(decoded),
        "ele3_offset": magic_offset,
        "ele3_container_bytes": len(container),
        "ele3_sha256": hashlib.sha256(container).hexdigest(),
        "sections": section_table(container),
        "firmware_bytes_included": False,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("firmware", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--redact-path", action="store_true")
    args = parser.parse_args()

    path = args.firmware.expanduser().resolve()
    if not path.is_file():
        parser.error(f"firmware file does not exist: {path}")

    report = make_report(path)
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
