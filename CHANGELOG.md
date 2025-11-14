# Changelog

## [Unreleased]
- Introduced a dedicated ReverbModule with APVTS-controlled pre-delay, decay,
  damping, and mix parameters, surfaced through a REVERB control group in the
  generic editor and captured in factory preset refreshes that audition musical
  voicings while keeping ambience bypassed where required.
- Added five deterministic factory presets (Subtle Glue, Lo-Fi Hiss, Chorus Pump, Warm Crunch, Noisy Parallel) exposed via the
  AudioProcessor program interface with APVTS snapshot recall.
- Ensured post-build deploy commands set `CMAKE_GENERATOR` alongside `CMAKE_GENERATOR_PLATFORM` so Visual Studio builds no longer emit spurious generator warnings.
- Added a `noiseRouting` choice parameter with pre/post/parallel options, routed through a dedicated noise module and exposed in the generic UI for automation-safe state recall.
- Raised the CMake preset schema to v6 and fixed workflow presets to stay compatible with CMake 3.27 while retaining INSTALL automation.
- Reworked CMake deploy configuration: default install prefix now points to `_deploy`, unsafe in-build installs are auto-corrected, and post-build copying is disabled by default in favour of the `INSTALL` target.
- Added Windows workflow presets and a `scripts/clean-build.ps1` helper to streamline clean configure/build/install cycles without ballooning the build tree.
- Fixed factory preset creation to use `ValueTree::createCopy` for JUCE 8 const-correct state duplication.
- Updated editor bold fonts to `FontOptions::withStyle("Bold")` for JUCE 8 API compatibility and silence MSVC warnings.
- Adjusted JUCE state serialisation, UTF-8 literals, and font construction for JUCE 8 compatibility and warning-free builds.
- Added a JUCE patch step during configure to eliminate MSVC static analysis warnings from upstream headers.
- Updated HostTempo to use juce::AudioPlayHead::getPosition for modern host compatibility.
- Removed obsolete JUCE splash-screen define and improved JUCE warning patch annotations.

## [0.1.0] - 2024-XX-XX
- Initial JUCE/CMake scaffold for the Dustbox VST3 effect.
- Implemented APVTS parameter layout with full parameter set and smoothing hooks.
- Added Tape, Dirt, and Pump DSP module stubs with tempo/ noise routing support.
- Created grouped generic editor and realtime-safe processor flow.
- Documented build instructions and architecture decisions.

