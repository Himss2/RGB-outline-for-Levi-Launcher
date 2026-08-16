#pragma once

#include <cstdint>
#include <string_view>

namespace outline::resolver {

enum class Target : std::uint8_t {
    Unknown = 0,

    // Vanilla selection-box rendering target.
    SelectionBox,

    // Rendering/tessellation target used as fallback.
    Tessellator,

    // Render-level target.
    RenderLevel,
};

bool initialize(std::string_view libraryName);

void shutdown();

bool initialized();

std::uintptr_t resolve(Target target);

const char* targetName(Target target);

} // namespace outline::resolver
