#include "outline/renderer.hpp"

#include <cmath>

namespace outline::renderer {

namespace {

bool g_initialized = false;

} // namespace

bool initialize() {
    /*
     * Rendering is intentionally passive for now.
     *
     * We first need a confirmed vanilla selection-box
     * rendering path for Minecraft 26.44 before installing
     * a hook into it.
     */

    g_initialized = true;
    return true;
}

void shutdown() {
    g_initialized = false;
}

bool initialized() {
    return g_initialized;
}

void buildBoxEdges(
    const AABB& box,
    Vec3 (&edges)[24]
) {
    const Vec3& a = box.min;
    const Vec3& b = box.max;

    /*
     * Bottom:
     *
     * 0 ---- 1
     * |      |
     * 3 ---- 2
     */

    edges[0] = {a.x, a.y, a.z};
    edges[1] = {b.x, a.y, a.z};

    edges[2] = {b.x, a.y, a.z};
    edges[3] = {b.x, a.y, b.z};

    edges[4] = {b.x, a.y, b.z};
    edges[5] = {a.x, a.y, b.z};

    edges[6] = {a.x, a.y, b.z};
    edges[7] = {a.x, a.y, a.z};

    /*
     * Top.
     */

    edges[8]  = {a.x, b.y, a.z};
    edges[9]  = {b.x, b.y, a.z};

    edges[10] = {b.x, b.y, a.z};
    edges[11] = {b.x, b.y, b.z};

    edges[12] = {b.x, b.y, b.z};
    edges[13] = {a.x, b.y, b.z};

    edges[14] = {a.x, b.y, b.z};
    edges[15] = {a.x, b.y, a.z};

    /*
     * Vertical edges.
     */

    edges[16] = {a.x, a.y, a.z};
    edges[17] = {a.x, b.y, a.z};

    edges[18] = {b.x, a.y, a.z};
    edges[19] = {b.x, b.y, a.z};

    edges[20] = {b.x, a.y, b.z};
    edges[21] = {b.x, b.y, b.z};

    edges[22] = {a.x, a.y, b.z};
    edges[23] = {a.x, b.y, b.z};
}

void renderSelectionBox(
    const SelectionBox& box,
    const Color& color
) {
    if (!g_initialized) {
        return;
    }

    if (!box.valid) {
        return;
    }

    if (!std::isfinite(color.r) ||
        !std::isfinite(color.g) ||
        !std::isfinite(color.b) ||
        !std::isfinite(color.a)) {
        return;
    }

    Vec3 edges[24]{};

    buildBoxEdges(
        box.bounds,
        edges
    );

    /*
     * IMPORTANT:
     *
     * No Minecraft renderer call is made here yet.
     *
     * The geometry generation is isolated so that the
     * confirmed vanilla rendering function can be attached
     * later without changing the AABB logic.
     *
     * The edges array is intentionally local. This function
     * currently acts as a safe rendering boundary.
     */

    (void)edges;
}

} // namespace outline::renderer
