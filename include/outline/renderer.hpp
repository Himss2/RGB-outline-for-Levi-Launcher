#pragma once

#include "types.hpp"

#include <cstdint>

namespace outline::renderer {

using Tessellator = void;
using ScreenContext = void;

using TessellatorBeginFn =
    void (*)(Tessellator*, int);

using TessellatorColorFn =
    void (*)(Tessellator*, float, float, float, float);

using TessellatorVertexFn =
    void (*)(Tessellator*, float, float, float);

using RenderMeshImmediatelyFn =
    void (*)(ScreenContext*, void*);

bool initialize();

bool ready();

void setTessellator(
    Tessellator* tessellator
);

void begin(
    int mode
);

void color(
    const Color& color
);

void vertex(
    const Vec3& position
);

void end();

}
