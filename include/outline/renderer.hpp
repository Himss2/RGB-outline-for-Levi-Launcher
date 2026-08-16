#pragma once

#include "types.hpp"

#include <cstdint>

namespace outline::renderer {

bool initialize();

void shutdown();

bool initialized();

/*
 * Called only after the Minecraft rendering target
 * has been successfully resolved.
 */
void renderSelectionBox(
    const SelectionBox& box,
    const Color& color
);

/*
 * Utility used by the implementation to generate
 * the twelve edges of an AABB.
 */
void buildBoxEdges(
    const AABB& box,
    Vec3 (&edges)[24]
);

} // namespace outline::renderer
