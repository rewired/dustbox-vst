# Changelog

## [Unreleased]
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

