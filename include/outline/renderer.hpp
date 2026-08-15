#pragma once

#include "types.hpp"

namespace OutlineRenderer {

bool initialize();

void drawAABB(
    void* screenContext,
    const AABB& box,
    const Color& color,
    float thickness
);

}
