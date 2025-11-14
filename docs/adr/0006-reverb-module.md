# ADR 0006: Reverb Module Integration

## Status
Accepted

## Context
User feedback and product vision both highlighted the need for a controllable ambience stage that could sit alongside the tape, dirt, and pump processing already scaffolded in Dustbox. Without a reverb block the signal chain could not deliver spatial glue or roomy textures, and preset authors were forced to approximate ambience with external plugins. Introducing a native module also enables consistent automation support and snapshot serialisation through the existing APVTS model.

## Decision
- Add a dedicated `ReverbModule` class that lives alongside Tape, Dirt, and Pump and processes after the distortion stage but before the pump envelope to preserve rhythmic ducking options.
- Expose four automatable parameters through APVTS with stable IDs: `reverbPreDelayMs`, `reverbDecaySeconds`, `reverbDamping`, and `reverbMix`, each with musically scoped ranges and smoothing handled by the processor.
- Extend the generic JUCE editor with a REVERB control group that maps sliders to the new parameters and respects the layout minimums used by other modules.
- Update the factory preset snapshots so every program explicitly sets the four reverb parameters, keeping ambience bypassed (`reverbMix = 0.0`) where the sound design should remain dry.

## Consequences
- The processor prepares and resets a fourth DSP module without altering realtime safety guarantees, and the signal chain now reads Tape → Dirt → Reverb → Pump.
- Hosts gain consistent access to reverb automation and preset recall, aligning Dustbox with modern lo-fi workflows.
- Future DSP work can iterate on the reverb algorithm independently because the parameters, UI hooks, and preset schema are locked into the architecture history.
