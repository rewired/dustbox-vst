# ADR 0003: Safe install-based deploy flow for Dustbox builds

## Status
Accepted

## Context
The previous CMake setup copied the generated `Dustbox.vst3` bundle directly into a `deploy/` folder under the build tree via a
`POST_BUILD` command. Visual Studio re-triggers such copies when the destination sits inside the binary directory, leading to
recursive copy attempts, runaway disk usage, and perpetual "out-of-date" states. Contributors also needed to remember to adjust
`CMAKE_INSTALL_PREFIX` manually to avoid clobbering the build output.

## Decision
- Default the install prefix to `${CMAKE_SOURCE_DIR}/_deploy` and guard against any configuration that resolves inside the build
  tree, overriding unsafe values automatically while printing a warning.
- Disable the post-build deploy option by default and rely on the standard `INSTALL` target for repeatable deployment. A hardened
  copy script remains available behind `DUSTBOX_ENABLE_POST_BUILD_DEPLOY` for advanced workflows.
- Extend the CMake presets with workflow entries that run configure → build → install for Windows, and add a PowerShell helper
  (`scripts/clean-build.ps1`) that clears stale build/deploy directories before driving a fresh Debug build + install.

## Consequences
- Running `cmake --build --preset windows-msvc-debug --config Debug --target INSTALL` always produces a single VST3 bundle under
  `_deploy/VST3/Debug/Dustbox.vst3`, eliminating recursive copies into the build tree.
- Contributors get an immediate warning (and automatic fix) if they attempt to install inside the binary directory, reducing the
  chance of disk bloat on Windows.
- The clean-build script and workflow presets make it easy to reset the environment without manual file deletion or ad-hoc copy
  commands, keeping deployments deterministic across machines.
