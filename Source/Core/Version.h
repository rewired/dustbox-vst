/*
  ==============================================================================
  File: Version.h
  Responsibility: Provide project-wide version and company metadata constants.
  Assumptions: Updated manually when product metadata changes.
  TODO: Revisit before shipping to align with marketing/legal requirements.
  ==============================================================================
*/

#pragma once

namespace dustbox
{
struct Version
{
    static constexpr const char* projectName   = "Dustbox";
    static constexpr const char* companyName   = "Communit Labs";
    static constexpr const char* companyDomain = "commun.it";
    static constexpr int majorVersion          = 0;
    static constexpr int minorVersion          = 1;
    static constexpr int patchVersion          = 0;
};
} // namespace dustbox

