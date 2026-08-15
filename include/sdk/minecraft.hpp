#pragma once

#include <cstdint>

namespace MC {

struct ScreenContext;
struct Tessellator;

using TessellatorBeginFn =
    void (*)(Tessellator*, int);

using TessellatorColorFn =
    void (*)(Tessellator*, float, float, float, float);

using TessellatorVertexFn =
    void (*)(Tessellator*, float, float, float);

using RenderMeshImmediatelyFn =
    void (*)(ScreenContext*, void*);

struct RenderApi {
    TessellatorBeginFn begin = nullptr;
    TessellatorColorFn color = nullptr;
    TessellatorVertexFn vertex = nullptr;
    RenderMeshImmediatelyFn renderMeshImmediately = nullptr;

    bool valid() const {
        return begin &&
               color &&
               vertex &&
               renderMeshImmediately;
    }
};

}
