# 0002 - Manage JUCE warning fixes via configure-time patching

## Status
Accepted

## Context
Visual Studio's C++ Core Guidelines analyzer reports warnings C26495, C26439, and C26819 when building JUCE 8. These originate from third-party headers (`juce_FixedSizeFunction.h`, `juce_ReferenceCountedObject.h`, and `juce_CharacterFunctions.h`) that we do not control directly. The warnings obscure Dustbox-specific issues when `DUSTBOX_ENABLE_WARNINGS` is enabled and can become errors when `DUSTBOX_STRICT_BUILD` is on.

Directly committing upstream JUCE sources is undesirable because we consume JUCE as a git submodule and want to stay close to the upstream history.

## Decision
Add a configure-time patch step (`cmake/JuceWarningPatches.cmake`) that gently rewrites the affected JUCE headers after the submodule is present. The patch:

- zero-initialises `juce::FixedSizeFunction::storage` to silence C26495,
- default-noexcepts the copy/move operations in `juce::SingleThreadedReferenceCountedObject` to address C26439, and
- annotates intentional `switch` fallthroughs in `juce_CharacterFunctions` to stop C26819.

The patch script only writes files when the original snippet is found, so it is safe to re-run across configurations and future upstream updates.

## Consequences
- JUCE builds cleanly with MSVC's /analyze warnings without mutating the submodule history manually.
- Upstream JUCE updates that change the targeted snippets will cause the patch step to no-op, making divergences easy to detect during configuration.
- Developers must ensure the JUCE submodule is checked out before configuring so the patch step can run.
