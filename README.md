# Dustbox

Minimal JUCE-based scaffold for the Dustbox audio plug-in (VST3 + Standalone) targeting Windows 10 / Visual Studio 2022 first, with cross-platform readiness.

## Prerequisites (Windows 10)

1. **Visual Studio 2022** with the *Desktop development with C++* workload.
2. **Windows 10 SDK** (installed via the Visual Studio installer).
3. **CMake 3.22+** (ships with VS2022 or install separately).
4. Git with submodule support.

## Repository Setup

```bash
# Clone the repository
git clone <repo-url> dustbox
cd dustbox

# Fetch JUCE as a submodule
git submodule add https://github.com/juce-framework/JUCE.git extern/JUCE
git submodule update --init --recursive
```

> **Note:** JUCE ships with the VST3 SDK integration. By building a VST3 you agree to Steinberg's licensing terms for distribution.

## Configure & Build (Visual Studio 2022)

```powershell
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release
```

### Build Outputs

* Visual Studio will emit the VST3 bundle under: `build/windows-msvc-release/VST3/Release/Dustbox.vst3`
* CMake additionally copies the bundle to `build/windows-msvc-release/out/Release/Dustbox.vst3`
* The standalone executable lives under the Visual Studio configuration directory: `build/windows-msvc-release/Release/Dustbox.exe`

Load the `.vst3` into any VST3-compatible host, or launch the standalone executable directly to verify the skeleton UI.

## Development Notes

* C++17 is enforced for all targets.
* Warning levels are high on all toolchains; enable strict (warnings-as-errors) builds with `-D DUSTBOX_STRICT_BUILD=ON` or the `ci-strict` preset.
* No DSP processing is implemented yet; the processor passes audio through unchanged.

## Licensing

* Project source: [MIT License](./LICENSE)
* JUCE is provided as a git submodule and subject to the [JUCE license](https://juce.com/juce-7-licence).
* Building VST3 binaries implies acceptance of the Steinberg VST3 license.

