include_guard(GLOBAL)

set(_dustbox_juce_root "${CMAKE_CURRENT_SOURCE_DIR}/extern/JUCE")

if(NOT EXISTS "${_dustbox_juce_root}/modules")
    message(STATUS "Dustbox: JUCE sources not present; skipping JUCE warning patches.")
    return()
endif()

# Helper macro to replace a snippet in a file and track whether the file changed.
macro(_dustbox_apply_patch target_path old_text new_text)
    if(NOT EXISTS "${target_path}")
        message(STATUS "Dustbox: JUCE file ${target_path} not found; skipping patch.")
    else()
        file(READ "${target_path}" _dustbox_patch_original)
        set(_dustbox_patch_updated "${_dustbox_patch_original}")
        string(REPLACE "${old_text}" "${new_text}" _dustbox_patch_updated "${_dustbox_patch_updated}")
        if(NOT _dustbox_patch_original STREQUAL _dustbox_patch_updated)
            file(WRITE "${target_path}" "${_dustbox_patch_updated}")
            message(STATUS "Dustbox: Applied warning fix to ${target_path}")
        endif()
        unset(_dustbox_patch_original)
        unset(_dustbox_patch_updated)
    endif()
endmacro()

set(_dustbox_fixed_size_fn "${_dustbox_juce_root}/modules/juce_core/containers/juce_FixedSizeFunction.h")
_dustbox_apply_patch(
    "${_dustbox_fixed_size_fn}"
    "    alignas (std::max_align_t) mutable std::byte storage[len];"
    "    alignas (std::max_align_t) mutable std::byte storage[len]{};")

set(_dustbox_refcounted_fn "${_dustbox_juce_root}/modules/juce_core/memory/juce_ReferenceCountedObject.h")
_dustbox_apply_patch(
    "${_dustbox_refcounted_fn}"
    "    SingleThreadedReferenceCountedObject (const SingleThreadedReferenceCountedObject&) {}"
    "    SingleThreadedReferenceCountedObject (const SingleThreadedReferenceCountedObject&) noexcept = default;")
_dustbox_apply_patch(
    "${_dustbox_refcounted_fn}"
    "    SingleThreadedReferenceCountedObject (SingleThreadedReferenceCountedObject&&) {}"
    "    SingleThreadedReferenceCountedObject (SingleThreadedReferenceCountedObject&&) noexcept = default;")
_dustbox_apply_patch(
    "${_dustbox_refcounted_fn}"
    "    SingleThreadedReferenceCountedObject& operator= (const SingleThreadedReferenceCountedObject&) { return *this; }"
    "    SingleThreadedReferenceCountedObject& operator= (const SingleThreadedReferenceCountedObject&) noexcept = default;")
_dustbox_apply_patch(
    "${_dustbox_refcounted_fn}"
    "    SingleThreadedReferenceCountedObject& operator= (SingleThreadedReferenceCountedObject&&) { return *this; }"
    "    SingleThreadedReferenceCountedObject& operator= (SingleThreadedReferenceCountedObject&&) noexcept = default;")

set(_dustbox_character_fn "${_dustbox_juce_root}/modules/juce_core/text/juce_CharacterFunctions.h")
_dustbox_apply_patch(
    "${_dustbox_character_fn}"
    "                JUCE_FALLTHROUGH\n            case '+':"
    "                JUCE_FALLTHROUGH; // fallthrough\n            case '+':")
_dustbox_apply_patch(
    "${_dustbox_character_fn}"
    "                    JUCE_FALLTHROUGH\n                case '+':"
    "                    JUCE_FALLTHROUGH; // fallthrough\n                case '+':")

unset(_dustbox_fixed_size_fn)
unset(_dustbox_refcounted_fn)
unset(_dustbox_character_fn)
unset(_dustbox_juce_root)
