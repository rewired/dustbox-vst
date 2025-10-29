# Dustbox

Dustbox is a JUCE-based audio effect plugin scaffold targeting macOS and Windows. The current deliverable builds a zero-latency VST3 that wires an `AudioProcessorValueTreeState` parameter model to three DSP module stubs (Tape, Dirt, Pump) and a minimal grouped UI. No creative DSP is implemented yet; this repository establishes the production-ready structure for further development.

## Prerequisites

- [CMake 3.22+](https://cmake.org/) with a C++17 toolchain.
- JUCE as a Git submodule at `extern/JUCE` (`git submodule update --init --recursive`).
- Platform toolchains:
  - **Windows:** Visual Studio 2022 (Desktop development with C++) or recent MSVC with the VST3 SDK runtime.
  - **macOS:** Xcode 14+ with command line tools installed.

## Configure & Build

The project uses the supplied CMake presets. Replace the preset name with the configuration you need (`debug`/`release`).

For manual generator selection, use VS: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`, Ninja: `cmake -S . -B build -G Ninja`.

```bash
cmake --preset windows-msvc-release   # or macos-xcode-release / macos-xcode-debug / windows-msvc-debug
cmake --build --preset windows-msvc-release
```

VST3 bundles are copied after build to `<build-dir>/out/<Config>/Dustbox.vst3`. Load the plugin in any VST3-compatible host to inspect the scaffolding UI and parameter wiring.

## Project Highlights

- **Zero-latency** VST3 with realtime-safe audio thread (no allocations, locks, or file I/O in `processBlock`).
- **Parameter model** via `AudioProcessorValueTreeState` with stable IDs and automation-ready ranges.
- **Module stubs** for Tape, Dirt, and Pump processing, including tempo sync and noise routing placeholders.
- **Grouped generic UI** built with JUCE controls and attachments—ready for a custom skin later.

Refer to the [CHANGELOG](./CHANGELOG.md) and ADRs under [`docs/adr`](./docs/adr) for design notes and future decision tracking.

## Licensing

- Project source: [MIT License](./LICENSE)
- JUCE: subject to the [JUCE 7 license](https://juce.com/juce-7-licence)
- Building VST3 binaries implies acceptance of Steinberg's VST3 license terms.

