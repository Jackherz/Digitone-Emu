# Clean-room scope and firmware research workflow

## Boundary

DN1FM is a new implementation. It does not link to, embed, patch, execute, or
disassemble Elektron firmware at run time. It does not copy implementation
code or tables from `digiemu`; that project is used only as a public reference
for the broad problem shape and for the fact that Digitone mk1 has a main
processor, a second voice processor, and eight FM voices.

The `.syx` supplied by the owner can be inspected locally to establish identity
and to guide a behavior specification. The source tree should contain only the
specification and independently written code. A firmware-derived constant is
not added just because it makes a test pass; the source of every behavior added
to the engine should be noted as one of:

- `hardware`: an observation made on the physical instrument;
- `public`: a fact in public documentation or an independently licensed
  reference implementation;
- `measurement`: a repeatable audio/MIDI measurement;
- `hypothesis`: an explicit approximation awaiting measurement.

## Suggested evidence loop

1. Keep the official `.syx` and any extracted material in a directory outside
   this Git checkout.
2. Record its SHA-256 with `tools/firmware_manifest.py`. The script emits
   metadata only and never writes firmware bytes to the report.
3. Use REA or another local tool to inspect container/section boundaries. Do
   not put raw dumps, extracted code, factory sounds, or screenshots containing
   serials in this repository.
4. When observing a behavior, make a minimal test recording: MIDI input,
   parameter values, sample rate, note, velocity, and output WAV checksum.
5. Add a short observation to a research note, implement the smallest
   independent change, and add a portable test where possible.
6. Compare output by measurements such as onset time, pitch, RMS, spectral
   peaks, and release duration. Listening alone is useful but not a sufficient
   regression test.

## Optional REA setup

Follow REA's own installation documentation and keep its workspaces outside
this checkout. The public workflow is conceptually:

```sh
# inspect a local firmware container; output stays outside this repository
rea inspect-firmware-regions "$HOME/Downloads/Digitone_OS.syx"
rea extract-firmware "$HOME/Downloads/Digitone_OS.syx" \
  "$HOME/tmp/dn1-firmware-extracted"
```

Only select a native child for static analysis after its format, architecture,
load address, and entry point have been independently established. Do not run
unknown extracted programs, and do not treat a decompiler's guesses as a
behavioral specification. REA's firmware documentation describes the exact
caller-supplied raw-binary options for supported analysis providers; use those
rather than guessing an address from a filename.

## What this repository claims today

The current engine claims only:

- eight simultaneous voices;
- four sine-based FM operators per voice;
- eight useful operator routings;
- parameterized envelopes and MIDI expression;
- deterministic float32 rendering for the same event stream.

It does **not** yet claim Digitone patch compatibility, factory preset
compatibility, exact filter/effect behavior, sequencer behavior, parameter-lock
behavior, or firmware equivalence. Those claims require measurements and tests.
