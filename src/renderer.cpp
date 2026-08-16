#include "outline/renderer.hpp"
#include "outline/resolver.hpp"
#include "outline/runtime.hpp"

#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

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
RenderMeshImmediatelyFn gRenderMesh{};

constexpr std::uintptr_t kScreenContextColorHolder = 0x30;
constexpr std::uintptr_t kScreenContextTessellator = 0xB8;
constexpr std::uintptr_t kLevelRendererPlayer = 0x420;
constexpr std::uintptr_t kLevelRendererPlayerCamera = 0x61C;
constexpr std::uintptr_t kSelectionOverlayMaterial = 0x1030;
constexpr std::uintptr_t kActorLevel = 464;
constexpr std::uintptr_t kLevelHitResultWrapper = 456;
constexpr std::uintptr_t kHitResultType = 24;
constexpr std::uintptr_t kHitResultPos = 44;

using ClientInstanceGetLocalPlayerFn = void* (*)(void*);
using LevelGetHitResultFn = void* (*)(void*);

ClientInstanceGetLocalPlayerFn gGetLocalPlayer{};
LevelGetHitResultFn gGetHitResult{};

bool validPtr(const void* ptr) {
    return ptr && reinterpret_cast<std::uintptr_t>(ptr) >= 0x1000;
}

void emitBox(
    Tessellator* tessellator,
    const Vec3& min,
    const Vec3& max,
    float camX,
    float camY,
    float camZ,
    float r,
    float g,
    float b,
    float a
) {
    Vec3 p[8] = {
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
        {max.x, min.y, max.z},
        {min.x, min.y, max.z},
        {min.x, max.y, min.z},
        {max.x, max.y, min.z},
        {max.x, max.y, max.z},
        {min.x, max.y, max.z}
    };

    static constexpr int edges[24] = {
        0,1, 1,2, 2,3, 3,0,
        4,5, 5,6, 6,7, 7,4,
        0,4, 1,5, 2,6, 3,7
    };

    gBegin(
        tessellator,
        nullptr,
        4,
        24,
        0
    );

    gColor(
        tessellator,
        r, g, b, a
    );

    for (int i = 0; i < 24; i += 2) {
        const Vec3 a0 = {
            p[edges[i]].x - camX,
            p[edges[i]].y - camY,
            p[edges[i]].z - camZ
        };

        const Vec3 a1 = {
            p[edges[i + 1]].x - camX,
            p[edges[i + 1]].y - camY,
            p[edges[i + 1]].z - camZ
        };

        gVertex(tessellator, a0.x, a0.y, a0.z);
        gVertex(tessellator, a1.x, a1.y, a1.z);
    }
}

}

bool initialize() {
    gBegin = reinterpret_cast<TessellatorBeginFn>(
        resolver::tessellatorBegin()
    );

    gColor = reinterpret_cast<TessellatorColorFn>(
        resolver::tessellatorColor()
    );

    gVertex = reinterpret_cast<TessellatorVertexFn>(
        resolver::tessellatorVertex()
    );

    gRenderMesh = reinterpret_cast<RenderMeshImmediatelyFn>(
        resolver::meshRenderImmediately()
    );

    gGetLocalPlayer = reinterpret_cast<ClientInstanceGetLocalPlayerFn>(
        resolver::clientInstanceGetLocalPlayer()
    );

    gGetHitResult = reinterpret_cast<LevelGetHitResultFn>(
        resolver::levelGetHitResult()
    );

    const bool ok =
        gBegin &&
        gColor &&
        gVertex &&
        gRenderMesh &&
        gGetLocalPlayer &&
        gGetHitResult;

    if (ok)
        LOGI("Renderer API ready");
    else
        LOGE("Renderer API incomplete");

    return ok;
}

bool ready() {
    return
        gBegin &&
        gColor &&
        gVertex &&
        gRenderMesh;
}

void renderSelection(void* levelRenderer, void* screenContext) {
    static std::uint64_t frameCounter = 0;
    ++frameCounter;

    if (!ready() || !validPtr(levelRenderer) || !validPtr(screenContext))
        return;

    const auto screen = reinterpret_cast<std::uintptr_t>(screenContext);
    const auto tessPtr = *reinterpret_cast<std::uintptr_t*>(
        screen + kScreenContextTessellator
    );

    if (tessPtr < 0x1000)
        return;

    auto* tessellator = reinterpret_cast<Tessellator*>(tessPtr);

    const auto renderer = reinterpret_cast<std::uintptr_t>(levelRenderer);
    const auto lrpPtr = *reinterpret_cast<std::uintptr_t*>(
        renderer + kLevelRendererPlayer
    );

    if (lrpPtr < 0x1000)
        return;

    const float camX = *reinterpret_cast<float*>(
        lrpPtr + kLevelRendererPlayerCamera
    );
    const float camY = *reinterpret_cast<float*>(
        lrpPtr + kLevelRendererPlayerCamera + 4
    );
    const float camZ = *reinterpret_cast<float*>(
        lrpPtr + kLevelRendererPlayerCamera + 8
    );

    void* material = *reinterpret_cast<void**>(
        lrpPtr + kSelectionOverlayMaterial
    );

    if (!validPtr(material))
        return;

    void* client = runtime::clientInstance();
    if (!validPtr(client) || !gGetLocalPlayer || !gGetHitResult)
        return;

    void* player = gGetLocalPlayer(client);
    if (!validPtr(player))
        return;

    const auto playerAddr = reinterpret_cast<std::uintptr_t>(player);
    void* level = *reinterpret_cast<void**>(
        playerAddr + kActorLevel
    );

    if (!validPtr(level))
        return;

    void* hit = gGetHitResult(level);
    if (!validPtr(hit))
        return;

    const auto hitAddr = reinterpret_cast<std::uintptr_t>(hit);
    const int hitType = *reinterpret_cast<int*>(
        hitAddr + kHitResultType
    );

    // BedrockTools' HitResult handling treats type 0/1 as valid hits;
    // for the block selection box we only accept type 0.
    if (hitType != 0)
        return;

    const Vec3 hitPos = *reinterpret_cast<Vec3*>(
        hitAddr + kHitResultPos
    );

    if (!std::isfinite(hitPos.x) ||
        !std::isfinite(hitPos.y) ||
        !std::isfinite(hitPos.z)) {
        return;
    }

    const float bx = std::floor(hitPos.x);
    const float by = std::floor(hitPos.y);
    const float bz = std::floor(hitPos.z);

    // First runtime milestone: a vanilla-sized 1x1x1 block selection box.
    // The exact Block::getOutline/AABB path will replace this once the
    // verified 26.44 block-shape function is wired in.
    const Vec3 min = {
        bx + 0.002f,
        by + 0.002f,
        bz + 0.002f
    };

    const Vec3 max = {
        bx + 0.998f,
        by + 0.998f,
        bz + 0.998f
    };

    float r = 1.0f;
    float g = 0.0f;
    float b = 0.0f;

    // Temporary RGB proof-of-life. Once the hook/render path is proven,
    // this becomes the configurable animation from Config.
    if ((frameCounter / 2) % 3 == 1) {
        r = 0.0f;
        g = 1.0f;
    } else if ((frameCounter / 2) % 3 == 2) {
        r = 0.0f;
        b = 1.0f;
    }

    emitBox(
        tessellator,
        min,
        max,
        camX,
        camY,
        camZ,
        r,
        g,
        b,
        1.0f
    );

    char pad[0x58]{};

    gRenderMesh(
        screenContext,
        tessellator,
        material,
        pad
    );
}

}
