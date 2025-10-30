# Dustbox

Dustbox is a JUCE-based audio effect plugin scaffold targeting macOS and Windows. The current deliverable builds a zero-latency VST3 that wires an `AudioProcessorValueTreeState` parameter model to three DSP module stubs (Tape, Dirt, Pump) and a minimal grouped UI. No creative DSP is implemented yet; this repository establishes the production-ready structure for further development.

### What is Dustbox?

- **Creative lo-fi coloration toolkit** focused on tape wobble (AGE), digital grit (DIRT), and tempo-synced pumping (PUMP).
- **JUCE 8 scaffold** with realtime-safe processor flow, APVTS parameter layout, and grouped generic UI ready for skinning.
- **Cross-platform VST3 project** for Windows and macOS today, with AU support planned once DSP is implemented.

## Prerequisites

- [CMake 3.22+](https://cmake.org/) with a C++17 toolchain.
- JUCE as a Git submodule at `extern/JUCE` (`git submodule update --init --recursive`).
- Platform toolchains:
  - **Windows:** Visual Studio 2022 (Desktop development with C++) or recent MSVC with the VST3 SDK runtime.
  - **macOS:** Xcode 14+ with command line tools installed.

## Configure & Build

The project ships with CMake presets for Visual Studio, Xcode, and Ninja. Builds deploy via the standard `INSTALL` target into
`_deploy/VST3/<Config>/Dustbox.vst3`, keeping large bundles outside of the build tree. A pair of workflow presets (`windows-msvc-*-install`)
drive the configure → build → install sequence in one command if desired.

```powershell
# Debug example on Windows
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug --config Debug --target INSTALL

# Release build
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --config Release --target INSTALL
```

For manual generator selection, use VS: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`, Ninja: `cmake -S . -B build -G Ninja`.
After generating a Visual Studio project manually, build and deploy with:

```
cmake --build build --config Release --target INSTALL
```

> The configure step guards against unsafe install prefixes. If `CMAKE_INSTALL_PREFIX` resolves inside the build tree it is overridden to
> `${CMAKE_SOURCE_DIR}/_deploy` automatically and a warning is printed.

### Troubleshooting & Clean Builds

- If Visual Studio reports perpetual rebuilds or disk usage spikes, delete the `build/` folder(s) and `_deploy/`, or run
  `scripts/clean-build.ps1` (PowerShell) to remove them and perform a fresh Debug configure/build/install cycle.
- The optional `DUSTBOX_ENABLE_POST_BUILD_DEPLOY` toggle is OFF by default; rely on `--target INSTALL` for repeatable deployments.

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

