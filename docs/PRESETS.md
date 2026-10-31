# Preset direction

The engine now contains a small original starter bank in
`src/engine/dn1_presets.hpp`:

- Init
- Solid Bass
- Glass Bell
- Soft Pad
- Metal Pluck
- Organ Stack

These sounds are parameter recipes authored for this project. They are not
extracted from Digitone firmware or copied from factory sound files. The bank
is kept at the portable engine boundary so it can eventually drive the VST3
preset browser, a standalone test harness, or another clean-room front end.

## Later Digitone assessment

A later preset study can use user-supplied Digitone material, such as patches
exported from the owner's hardware, as a reference for high-level categories
and parameter relationships. The implementation should retain original names,
values, and assets unless the user explicitly has the rights to redistribute a
particular source. The useful output of that study is a set of original
categories and recipes, for example bass, metallic percussion, glassy keys,
organ-like stacks, and evolving pads—not a copied factory bank.

The bank is exposed through the VST3 program-list parameter named `Presets`.
Hosts that support `IUnitInfo` can display the six named programs and their
original high-level category/character metadata. The custom editor also
provides a six-position selector using short labels. Either path expands the
selected program into the engine's normalized parameters in the processor.
Processor/controller state version 2 stores both the selected preset index and
the complete parameter array; version 1 state remains readable. Normal
parameter automation remains authoritative after a preset is selected.

The six-position selector is intentionally a compact first step, not the final
browser. A more robust browser is planned for later, with full names, category
filtering, descriptions, scrolling/search, and eventually user-created preset
management, while keeping all sounds inside the single DN1FM plug-in.
