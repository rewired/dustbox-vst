# ADR 0001: Initial Dustbox Architecture Scaffold

## Status
Accepted

## Context
The first delivery of Dustbox requires a production-ready JUCE project skeleton that supports a zero-latency VST3 effect on macOS and Windows. The scaffold must expose a complete parameter model via `AudioProcessorValueTreeState`, own three DSP module stubs (Tape, Dirt, Pump), and provide a minimal grouped editor while respecting realtime safety constraints.

## Decision
- Use CMake with `juce_add_plugin` to target VST3 only, keeping the build portable across Windows and macOS.
- Structure the source tree into `Core`, `Parameters`, `Dsp`, `Plugin`, and `Ui` folders to isolate responsibilities and ease future DSP iteration.
- Drive all parameters through APVTS with stable string IDs and smoothing hooks managed by the processor.
- Implement Tape, Dirt, and Pump modules as realtime-safe stubs exposing `prepare`, `reset`, and `processBlock`, including tempo syncing and noise routing placeholders.
- Provide a generic grouped editor using JUCE attachments so the entire parameter set is host-automatable before the bespoke UI arrives.

## Consequences
- Future DSP work can focus on filling in module implementations without restructuring the project.
- Adding AU or additional formats later will only require extending the CMake configuration.
- The generic UI keeps development velocity high while enabling QA to validate automation, tempo sync, and parameter behaviour.
- Realtime safety is enforced from day one, reducing refactor risk when advanced DSP is introduced.

