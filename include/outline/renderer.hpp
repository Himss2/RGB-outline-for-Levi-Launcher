#pragma once

#include "types.hpp"

namespace outline::renderer {

using Tessellator = void;
using ScreenContext = void;

using TessellatorBeginFn =
    void (*)(Tessellator*, void*, int, int, int);

using TessellatorColorFn =
    void (*)(Tessellator*, float, float, float, float);

using TessellatorVertexFn =
    void (*)(Tessellator*, float, float, float);

using RenderMeshImmediatelyFn =
    void (*)(ScreenContext*, Tessellator*, void*, char*);

bool initialize();

bool ready();

void renderSelection(
    void* levelRenderer,
    void* screenContext
);

}
