/*
  ==============================================================================
  File: BuildConfig.h
  Responsibility: Centralise compile-time configuration flags for Dustbox.
  Assumptions: Flags are consumed throughout the codebase for conditional logic.
  TODO: Extend with feature toggles once advanced DSP is implemented.
  ==============================================================================
*/

#pragma once

namespace dustbox
{
struct BuildConfig
{
    static constexpr bool enableDenormalGuard = true;
};
} // namespace dustbox

