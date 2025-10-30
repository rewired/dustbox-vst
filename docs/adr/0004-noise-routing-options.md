# ADR 0004: Noise Routing Parameter and Module Split

## Status
Accepted

## Context
Dustbox's initial scaffold embedded tape hiss generation inside the Tape module with a tape-scoped routing enum. The product direction now requires a host-automatable `noiseRouting` choice parameter (`pre_tape`, `post_tape`, `parallel`) that governs how the dedicated noise generator blends with the signal chain. We must support automation-safe state recall, minimise CPU overhead, and keep realtime processing allocation free while honouring existing bypass and mix behaviour.

## Decision
- Introduce a standalone `dsp::NoiseModule` responsible for generating broadband hiss buffers without touching the audio path directly.
- Add a new APVTS `noiseRouting` choice parameter (default `post_tape`) and expose it in the generic editor via a combo box.
- Route noise in the processor according to the parameter:
  - `pre_tape`: sum noise into the wet buffer before Tape processing.
  - `post_tape`: add noise immediately after Tape before Dirt and Pump stages.
  - `parallel`: inject the noise buffer after the equal-power wet/dry mix, scaling with the smoothed output gain.
- Preserve existing tape tone, wet/dry mix, and bypass ramp semantics while keeping noise buffers allocated during `prepareToPlay`.

## Consequences
- Noise routing is now a host-visible control with deterministic indices for automation and preset storage.
- The Tape module focuses solely on wow/flutter and tone shaping, simplifying maintenance and testing.
- Parallel routing keeps noise out of the wet chain yet still respects the output gain smoother, avoiding level jumps during automation.
- Future DSP iterations can expand noise behaviour (colour, filtering) inside the dedicated module without touching the tape algorithm.

