#include "outline/renderer.hpp"
#include "outline/config.hpp"

#include <android/log.h>

#define LOG_TAG "OutlineRGB"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

using BeginFn =
    void (*)(void*, int);

using ColorFn =
    void (*)(void*, float, float, float, float);

using VertexFn =
    void (*)(void*, float, float, float);

using EndFn =
    void (*)(void*);

BeginFn gBegin = nullptr;
ColorFn gColor = nullptr;
VertexFn gVertex = nullptr;
EndFn gEnd = nullptr;

}

namespace OutlineRenderer {

bool initialize() {
    /*
     * These function pointers will be connected to the
     * already-resolved Bedrock rendering functions.
     *
     * We don't invent their addresses here.
     */

    LOGI("Renderer initialized");

    return true;
}

void drawAABB(
    void* screenContext,
    const AABB& box,
    const Color& color,
    float thickness
) {
    if (!screenContext)
        return;

    if (!gBegin || !gColor || !gVertex || !gEnd) {
        LOGE("Renderer functions aren't resolved");
        return;
    }

    /*
     * MVP geometry.
     *
     * Once the exact Tessellator ABI is connected,
     * these 12 edges become thin quads.
     */

    gBegin(screenContext, 1);

    gColor(
        screenContext,
        color.r,
        color.g,
        color.b,
        color.a
    );

    const float x0 = box.min.x;
    const float y0 = box.min.y;
    const float z0 = box.min.z;

    const float x1 = box.max.x;
    const float y1 = box.max.y;
    const float z1 = box.max.z;

    // Bottom
    gVertex(screenContext, x0, y0, z0);
    gVertex(screenContext, x1, y0, z0);

    gVertex(screenContext, x1, y0, z0);
    gVertex(screenContext, x1, y0, z1);

    gVertex(screenContext, x1, y0, z1);
    gVertex(screenContext, x0, y0, z1);

    gVertex(screenContext, x0, y0, z1);
    gVertex(screenContext, x0, y0, z0);

    // Top
    gVertex(screenContext, x0, y1, z0);
    gVertex(screenContext, x1, y1, z0);

    gVertex(screenContext, x1, y1, z0);
    gVertex(screenContext, x1, y1, z1);

    gVertex(screenContext, x1, y1, z1);
    gVertex(screenContext, x0, y1, z1);

    gVertex(screenContext, x0, y1, z1);
    gVertex(screenContext, x0, y1, z0);

    // Vertical
    gVertex(screenContext, x0, y0, z0);
    gVertex(screenContext, x0, y1, z0);

    gVertex(screenContext, x1, y0, z0);
    gVertex(screenContext, x1, y1, z0);

    gVertex(screenContext, x1, y0, z1);
    gVertex(screenContext, x1, y1, z1);

    gVertex(screenContext, x0, y0, z1);
    gVertex(screenContext, x0, y1, z1);

    gEnd(screenContext);
}

}
