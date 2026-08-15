#include "outline/renderer.hpp"
#include "outline/resolver.hpp"

#include <android/log.h>

#define LOG_TAG "OutlineRGB"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace outline::renderer {

namespace {

TessellatorBeginFn gBegin{};
TessellatorColorFn gColor{};
TessellatorVertexFn gVertex{};

Tessellator* gTessellator{};

}

bool initialize() {
    gBegin =
        reinterpret_cast<TessellatorBeginFn>(
            resolver::tessellatorBegin()
        );

    gColor =
        reinterpret_cast<TessellatorColorFn>(
            resolver::tessellatorColor()
        );

    gVertex =
        reinterpret_cast<TessellatorVertexFn>(
            resolver::tessellatorVertex()
        );

    const bool ok =
        gBegin &&
        gColor &&
        gVertex;

    if (ok)
        LOGI("Tessellator API ready");
    else
        LOGE("Tessellator API incomplete");

    return ok;
}

bool ready() {
    return
        gBegin &&
        gColor &&
        gVertex &&
        gTessellator;
}

void setTessellator(
    Tessellator* tessellator
) {
    gTessellator = tessellator;
}

void begin(
    int mode
) {
    if (!gBegin || !gTessellator)
        return;

    gBegin(
        gTessellator,
        mode
    );
}

void color(
    const Color& value
) {
    if (!gColor || !gTessellator)
        return;

    gColor(
        gTessellator,
        value.r,
        value.g,
        value.b,
        value.a
    );
}

void vertex(
    const Vec3& position
) {
    if (!gVertex || !gTessellator)
        return;

    gVertex(
        gTessellator,
        position.x,
        position.y,
        position.z
    );
}

void end() {
    /*
     * There is intentionally no guessed Tessellator::end()
     * signature here.
     *
     * MeshHelpersRenderMeshImmediately is the vanilla
     * submission path we resolved separately.
     */
}

}
