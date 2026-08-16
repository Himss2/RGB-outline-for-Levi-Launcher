#include "outline/renderer.hpp"
#include "outline/resolver.hpp"
#include "outline/runtime.hpp"

#include <android/log.h>

#include <cmath>
#include <cstdint>

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
RenderMeshImmediatelyFn gRenderMesh2{};

using ClientInstanceGetLocalPlayerFn =
    void* (*)(void*);

using LevelGetHitResultFn =
    void* (*)(void*);

ClientInstanceGetLocalPlayerFn
    gGetLocalPlayer{};

LevelGetHitResultFn
    gGetHitResult{};

constexpr std::uintptr_t
    kScreenContextColorHolder = 0x30;

constexpr std::uintptr_t
    kScreenContextTessellator = 0xB8;

constexpr std::uintptr_t
    kLevelRendererPlayer = 0x420;

constexpr std::uintptr_t
    kLevelRendererPlayerCamera = 0x61C;

constexpr std::uintptr_t
    kSelectionOverlayMaterial = 0x1030;

constexpr std::uintptr_t
    kActorLevel = 464;

constexpr std::uintptr_t
    kHitResultType = 24;

constexpr std::uintptr_t
    kHitResultStartPos = 0;

bool validPtr(const void* ptr) {
    const auto value =
        reinterpret_cast<std::uintptr_t>(ptr);

    return value >= 0x1000;
}

bool finiteVec(const Vec3& value) {
    return
        std::isfinite(value.x) &&
        std::isfinite(value.y) &&
        std::isfinite(value.z);
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
    Vec3 points[8] = {
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
        0, 1,
        1, 2,
        2, 3,
        3, 0,

        4, 5,
        5, 6,
        6, 7,
        7, 4,

        0, 4,
        1, 5,
        2, 6,
        3, 7
    };

    /*
     * Reference BedrockTools renderer uses:
     *
     * Tessellator::begin(..., 4, vertexCount, ...)
     */
    gBegin(
        tessellator,
        nullptr,
        4,
        24,
        0
    );

    gColor(
        tessellator,
        r,
        g,
        b,
        a
    );

    for (int i = 0; i < 24; i += 2) {
        const Vec3& p0 =
            points[edges[i]];

        const Vec3& p1 =
            points[edges[i + 1]];

        gVertex(
            tessellator,
            p0.x - camX,
            p0.y - camY,
            p0.z - camZ
        );

        gVertex(
            tessellator,
            p1.x - camX,
            p1.y - camY,
            p1.z - camZ
        );
    }
}

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

    gRenderMesh =
        reinterpret_cast<RenderMeshImmediatelyFn>(
            resolver::meshRenderImmediately()
        );

    gRenderMesh2 =
        reinterpret_cast<RenderMeshImmediatelyFn>(
            resolver::meshRenderImmediately2()
        );

    gGetLocalPlayer =
        reinterpret_cast<
            ClientInstanceGetLocalPlayerFn
        >(
            resolver::clientInstanceGetLocalPlayer()
        );

    gGetHitResult =
        reinterpret_cast<
            LevelGetHitResultFn
        >(
            resolver::levelGetHitResult()
        );

    LOGI(
        "RenderLevel = %p",
        reinterpret_cast<void*>(
            resolver::renderLevel()
        )
    );

    LOGI(
        "TessellatorBegin = %p",
        reinterpret_cast<void*>(
            resolver::tessellatorBegin()
        )
    );

    LOGI(
        "TessellatorColor = %p",
        reinterpret_cast<void*>(
            resolver::tessellatorColor()
        )
    );

    LOGI(
        "TessellatorVertex = %p",
        reinterpret_cast<void*>(
            resolver::tessellatorVertex()
        )
    );

    LOGI(
        "RenderMeshImmediately = %p",
        reinterpret_cast<void*>(
            resolver::meshRenderImmediately()
        )
    );

    LOGI(
        "RenderMeshImmediately2 = %p",
        reinterpret_cast<void*>(
            resolver::meshRenderImmediately2()
        )
    );

    const bool ok =
        gBegin &&
        gColor &&
        gVertex &&
        (gRenderMesh2 || gRenderMesh) &&
        gGetLocalPlayer &&
        gGetHitResult;

    if (!ok) {
        LOGE(
            "Renderer initialization FAILED"
        );

        return false;
    }

    LOGI(
        "Renderer initialization OK"
    );

    return true;
}

bool ready() {
    return
        gBegin &&
        gColor &&
        gVertex &&
        (gRenderMesh2 || gRenderMesh);
}

void renderSelection(
    void* levelRenderer,
    void* screenContext
) {
    static std::uint64_t frameCounter = 0;

    ++frameCounter;

    if (!ready())
        return;

    if (!validPtr(levelRenderer))
        return;

    if (!validPtr(screenContext))
        return;

    /*
     * ScreenContext::mTessellator = 0xB8
     */
    const auto screen =
        reinterpret_cast<std::uintptr_t>(
            screenContext
        );

    const auto tessellatorPtr =
        *reinterpret_cast<std::uintptr_t*>(
            screen +
            kScreenContextTessellator
        );

    if (tessellatorPtr < 0x1000)
        return;

    auto* tessellator =
        reinterpret_cast<Tessellator*>(
            tessellatorPtr
        );

    /*
     * LevelRenderer::mLevelRendererPlayer = 0x420
     */
    const auto renderer =
        reinterpret_cast<std::uintptr_t>(
            levelRenderer
        );

    const auto lrpPtr =
        *reinterpret_cast<std::uintptr_t*>(
            renderer +
            kLevelRendererPlayer
        );

    if (lrpPtr < 0x1000)
        return;

    /*
     * LevelRendererPlayer::mCamPos = 0x61C
     */
    const float camX =
        *reinterpret_cast<float*>(
            lrpPtr +
            kLevelRendererPlayerCamera
        );

    const float camY =
        *reinterpret_cast<float*>(
            lrpPtr +
            kLevelRendererPlayerCamera +
            4
        );

    const float camZ =
        *reinterpret_cast<float*>(
            lrpPtr +
            kLevelRendererPlayerCamera +
            8
        );

    if (!std::isfinite(camX) ||
        !std::isfinite(camY) ||
        !std::isfinite(camZ)) {
        return;
    }

    /*
     * IMPORTANT:
     *
     * mSelectionOverlayMaterial is a MaterialPtr
     * object embedded at +0x1030.
     *
     * Do NOT dereference it into another pointer.
     *
     * BedrockTools uses the address of the field
     * when no separately resolved MaterialPtr is used.
     */
    void* material =
        reinterpret_cast<void*>(
            lrpPtr +
            kSelectionOverlayMaterial
        );

    if (!validPtr(material))
        return;

    /*
     * Obtain ClientInstance captured by
     * ClientInstanceUpdate.
     */
    void* client =
        runtime::clientInstance();

    if (!validPtr(client))
        return;

    if (!gGetLocalPlayer)
        return;

    if (!gGetHitResult)
        return;

    void* player =
        gGetLocalPlayer(client);

    if (!validPtr(player))
        return;

    /*
     * Actor::mLevel = 464
     */
    const auto playerAddr =
        reinterpret_cast<std::uintptr_t>(
            player
        );

    void* level =
        *reinterpret_cast<void**>(
            playerAddr +
            kActorLevel
        );

    if (!validPtr(level))
        return;

    /*
     * LevelGetHitResult(Level*)
     */
    void* hit =
        gGetHitResult(level);

    if (!validPtr(hit))
        return;

    const auto hitAddr =
        reinterpret_cast<std::uintptr_t>(
            hit
        );

    /*
     * HitResult::mType = 24
     *
     * BedrockTools treats type 0 and type 1
     * as valid hit results.
     *
     * For a block selection box we require type 0.
     */
    const int hitType =
        *reinterpret_cast<int*>(
            hitAddr +
            kHitResultType
        );

    if (hitType != 0)
        return;

    /*
     * HitResult::mStartPos = 0.
     *
     * The stored hit result contains the selected
     * block position data beginning here.
     */
    const Vec3 hitPos =
        *reinterpret_cast<const Vec3*>(
            hitAddr +
            kHitResultStartPos
        );

    if (!finiteVec(hitPos))
        return;

    const float blockX =
        std::floor(hitPos.x);

    const float blockY =
        std::floor(hitPos.y);

    const float blockZ =
        std::floor(hitPos.z);

    /*
     * Current verified runtime milestone:
     *
     * selected block -> world position -> AABB
     * -> Tessellator geometry -> render mesh.
     *
     * This deliberately uses the vanilla full-block
     * selection volume first.
     *
     * The unverified Block::getOutline candidate is
     * NOT called because doing so without its exact
     * ABI would be unsafe.
     */
    constexpr float epsilon = 0.002f;

    const Vec3 min{
        blockX + epsilon,
        blockY + epsilon,
        blockZ + epsilon
    };

    const Vec3 max{
        blockX + 1.0f - epsilon,
        blockY + 1.0f - epsilon,
        blockZ + 1.0f - epsilon
    };

    /*
     * Simple RGB proof-of-life.
     *
     * This is intentionally deterministic and does
     * not depend on external configuration yet.
     */
    const std::uint64_t phase =
        (frameCounter / 8) % 6;

    float r = 1.0f;
    float g = 0.0f;
    float b = 0.0f;

    switch (phase) {
        case 0:
            r = 1.0f;
            g = 0.0f;
            b = 0.0f;
            break;

        case 1:
            r = 1.0f;
            g = 0.5f;
            b = 0.0f;
            break;

        case 2:
            r = 1.0f;
            g = 1.0f;
            b = 0.0f;
            break;

        case 3:
            r = 0.0f;
            g = 1.0f;
            b = 0.0f;
            break;

        case 4:
            r = 0.0f;
            g = 0.5f;
            b = 1.0f;
            break;

        case 5:
            r = 0.5f;
            g = 0.0f;
            b = 1.0f;
            break;
    }

    /*
     * Reference BedrockTools temporarily forces
     * ScreenContext color holder to white before
     * submitting the selection mesh.
     */
    const auto colorHolderPtr =
        *reinterpret_cast<std::uintptr_t*>(
            screen +
            0x30
        );

    float savedColor[4]{};

    bool restoreColor = false;

    if (colorHolderPtr >= 0x1000) {
        auto* colorHolder =
            reinterpret_cast<float*>(
                colorHolderPtr
            );

        savedColor[0] = colorHolder[0];
        savedColor[1] = colorHolder[1];
        savedColor[2] = colorHolder[2];
        savedColor[3] = colorHolder[3];

        colorHolder[0] = 1.0f;
        colorHolder[1] = 1.0f;
        colorHolder[2] = 1.0f;
        colorHolder[3] = 1.0f;

        restoreColor = true;
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

    /*
     * RenderMeshImmediately2 is preferred by the
     * BedrockTools 26.44 implementation.
     */
    char pad[0x58]{};

    if (gRenderMesh2) {
        gRenderMesh2(
            screenContext,
            tessellator,
            material,
            pad
        );
    } else if (gRenderMesh) {
        gRenderMesh(
            screenContext,
            tessellator,
            material,
            pad
        );
    }

    if (restoreColor) {
        auto* colorHolder =
            reinterpret_cast<float*>(
                colorHolderPtr
            );

        colorHolder[0] = savedColor[0];
        colorHolder[1] = savedColor[1];
        colorHolder[2] = savedColor[2];
        colorHolder[3] = savedColor[3];
    }
}

}
