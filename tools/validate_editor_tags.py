#!/usr/bin/env python3
"""Validate that the VSTGUI editor tags match exposed VST3 parameter IDs.

The editor resource uses symbolic control-tag names, but the numeric values
must remain identical to the VST3 parameter IDs exposed by the controller.
This check prevents a visual control from silently editing a different engine
parameter or program-list entry when the catalog or UI layout changes.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path
from xml.etree import ElementTree


ENUM_RE = re.compile(
    r"enum\s+class\s+ParameterId\s*:\s*std::uint32_t\s*\{(?P<body>.*?)\};",
    re.DOTALL,
)
IDENTIFIER_RE = re.compile(r"^\s*([A-Za-z_]\w*)\s*(?:=\s*[^,]+)?\s*,")


def parameter_ids(parameter_header: Path, first_vst_id: int) -> dict[str, int]:
    text = parameter_header.read_text(encoding="utf-8")
    match = ENUM_RE.search(text)
    if not match:
        raise ValueError(f"could not find ParameterId enum in {parameter_header}")

    names: list[str] = []
    for line in match.group("body").splitlines():
        item = IDENTIFIER_RE.match(line)
        if not item:
            continue
        name = item.group(1)
        if name != "Count":
            names.append(name)
    if not names:
        raise ValueError(f"ParameterId enum in {parameter_header} is empty")
    return {name: first_vst_id + index for index, name in enumerate(names)}


def validate(parameter_header: Path, resource: Path, first_vst_id: int) -> list[str]:
    expected = parameter_ids(parameter_header, first_vst_id)
    # The named preset program list lives just after the engine catalog. It is
    # a VST3 controller parameter, not an engine ParameterId, but it is still
    # intentionally bound by the custom editor.
    expected["Preset"] = first_vst_id + len(expected)
    root = ElementTree.parse(resource).getroot()
    actual: dict[str, int] = {}
    errors: list[str] = []

    for element in root.findall("control-tags/control-tag"):
        name = element.attrib.get("name")
        tag_text = element.attrib.get("tag")
        if not name or tag_text is None:
            errors.append("every control-tag must have a name and tag")
            continue
        if name in actual:
            errors.append(f"duplicate control-tag name: {name}")
            continue
        try:
            actual[name] = int(tag_text)
        except ValueError:
            errors.append(f"control-tag {name} has a non-integer tag: {tag_text}")

    for name, tag in expected.items():
        if actual.get(name) != tag:
            errors.append(
                f"{name}: expected VST3 tag {tag}, found {actual.get(name, 'missing')}"
            )
    for name in sorted(set(actual) - set(expected)):
        errors.append(f"resource contains unknown control-tag: {name}")

    referenced = {
        element.attrib["control-tag"]
        for element in root.iter("view")
        if "control-tag" in element.attrib
    }
    for name in sorted(referenced - set(actual)):
        errors.append(f"view references undefined control-tag: {name}")
    for name in sorted(set(expected) - referenced):
        errors.append(f"parameter has no editor control: {name}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="repository root (default: inferred from this file)",
    )
    parser.add_argument("--first-vst-id", type=int, default=1000)
    args = parser.parse_args()

    root = args.root.resolve()
    errors = validate(
        root / "src/engine/dn1_parameters.hpp",
        root / "src/plugin/editor.uidesc",
        args.first_vst_id,
    )
    if errors:
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1

    print("editor control tags match the engine parameter catalog")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
