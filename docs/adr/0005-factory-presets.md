# ADR 0005: Built-in Factory Presets via APVTS Snapshots

## Status
Accepted

## Context
Hosts expect a small set of curated programs for quick auditioning, but Dustbox
previously shipped ad-hoc factory presets that were hard-coded in the processor,
used inconsistent parameter coverage, and were not aligned with the new product
sound targets (Subtle Glue, Lo-Fi Hiss, Chorus Pump, Warm Crunch, Noisy
Parallel). The old implementation also risked diverging from the APVTS state
schema, making preset recalls non-deterministic and jeopardising session
compatibility.

## Decision
- Define a dedicated `Source/Presets/FactoryPresets` module that maps explicit
  `{ parameterID -> value }` pairs into full APVTS `ValueTree` snapshots.
- Populate five factory presets using the agreed names and parameter intents,
  ensuring every Dustbox parameter is set explicitly for deterministic state
  serialisation.
- Expose presets through the standard `AudioProcessor` program interface so all
  VST3 hosts discover them without custom UI work.
- Suspend audio processing while swapping presets and refresh cached parameter
  snapshots to avoid inconsistent realtime state.

## Consequences
- Preset state now round-trips identically between host program selection and
  project save/restore, improving QA confidence.
- The processor constructor no longer owns preset data directly, simplifying
  future additions and making the preset list reusable for UI controls.
- Hosts and users receive musically scoped starting points covering glue, hiss,
  pump, crunch, and parallel-noise workflows.
- Additional factory or user preset features can build on the centralised
  helper without touching the audio engine.

