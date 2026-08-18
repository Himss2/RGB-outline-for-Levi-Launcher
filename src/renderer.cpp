#include "outline/renderer.hpp"
#include "outline/resolver.hpp"

#include <android/log.h>
#include <cmath>
#include <cstdint>

#define LOG_TAG "OutlineRGB"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace outline::renderer {

namespace {
bool g_initialized = false;
} // namespace

bool initialize() {
    g_initialized = true;
    return true;
}

void shutdown() {
    g_initialized = false;
}

bool initialized() {
    return g_initialized;
}

void buildBoxEdges(const AABB& box, Vec3 (&edges)[24]) {
    const Vec3& a = box.min;
    const Vec3& b = box.max;

    edges[0] = {a.x, a.y, a.z}; edges[1] = {b.x, a.y, a.z};
    edges[2] = {b.x, a.y, a.z}; edges[3] = {b.x, a.y, b.z};
    edges[4] = {b.x, a.y, b.z}; edges[5] = {a.x, a.y, b.z};
    edges[6] = {a.x, a.y, b.z}; edges[7] = {a.x, a.y, a.z};

    edges[8]  = {a.x, b.y, a.z}; edges[9]  = {b.x, b.y, a.z};
    edges[10] = {b.x, b.y, a.z}; edges[11] = {b.x, b.y, b.z};
    edges[12] = {b.x, b.y, b.z}; edges[13] = {a.x, b.y, b.z};
    edges[14] = {a.x, b.y, b.z}; edges[15] = {a.x, b.y, a.z};

    edges[16] = {a.x, a.y, a.z}; edges[17] = {a.x, b.y, a.z};
    edges[18] = {b.x, a.y, a.z}; edges[19] = {b.x, b.y, a.z};
    edges[20] = {b.x, a.y, b.z}; edges[21] = {b.x, b.y, b.z};
    edges[22] = {a.x, a.y, b.z}; edges[23] = {a.x, b.y, b.z};
}

void renderSelectionBox(
    void* screenContext,
    const SelectionBox& box,
    const Color& color
) {
    if (!g_initialized || !box.valid) return;

    // --- PROTEKSI 1: Cek Validitas ScreenContext ---
    std::uintptr_t scAddr = reinterpret_cast<std::uintptr_t>(screenContext);
    if (scAddr < 0x10000) return;

    if (!std::isfinite(color.r) || !std::isfinite(color.g) ||
        !std::isfinite(color.b) || !std::isfinite(color.a)) return;

    // --- PROTEKSI 2: Cek Validitas Function Pointer ---
    using FnTessBegin  = void (*)(void* _this, int topology);
    using FnTessVertex = void (*)(void* _this, float x, float y, float z);
    using FnTessColor  = void (*)(void* _this, float r, float g, float b, float a);
    using FnRenderMesh = void (*)(void* screenCtx, void* tess, void* mat);

    auto tBegin  = reinterpret_cast<FnTessBegin>(resolver::tessellatorBegin());
    auto tVertex = reinterpret_cast<FnTessVertex>(resolver::tessellatorVertex());
    auto tColor  = reinterpret_cast<FnTessColor>(resolver::tessellatorColor());
    
    std::uintptr_t renderMeshAddr = resolver::meshRenderImmediately() ? 
        resolver::meshRenderImmediately() : resolver::meshRenderImmediately2();

    auto rMesh = reinterpret_cast<FnRenderMesh>(renderMeshAddr);

    // Batal tanpa crash jika ada pointer fungsi yang bernilai NULL/0
    if (!tBegin || !tVertex || !tColor || !rMesh) return;

    // --- PROTEKSI 3: Ekstrak dan Cek Tessellator ---
    void* tessellatorInstance = *reinterpret_cast<void**>(scAddr + 0x30);
    std::uintptr_t tessAddr = reinterpret_cast<std::uintptr_t>(tessellatorInstance);

    if (tessAddr < 0x10000) return; // Mencegah Crash 0x31

    // --- PROTEKSI 4: Resolving Dynamic Material ---
    std::uintptr_t dynamicMat = resolver::fetchDynamicMaterialPtr(renderMeshAddr);
    void* materialPtr = reinterpret_cast<void*>(dynamicMat);

    if (reinterpret_cast<std::uintptr_t>(materialPtr) < 0x10000) {
        std::uintptr_t baseAddress = resolver::libraryBase();
        if (!baseAddress) return;
        materialPtr = reinterpret_cast<void*>(baseAddress + 0x125A3290);
    }

    // --- LOG DIAGNOSTIK (Cek Alamat Tanpa Crash) ---
    static int logTimer = 0;
    if (++logTimer % 300 == 0) {
        LOGI("=== DIAGNOSTIK RENDERER (AMANI/NO CRASH) ===");
        LOGI("ScreenContext    : %p", screenContext);
        LOGI("Tessellator (+30): %p", tessellatorInstance);
        LOGI("MaterialPtr      : %p", materialPtr);
        LOGI("RenderMeshFn     : %p", reinterpret_cast<void*>(renderMeshAddr));
    }

    // --- EKSEKUSI DRAW ---
    Vec3 edges[24]{};
    buildBoxEdges(box.bounds, edges);

    tBegin(tessellatorInstance, 1);
    tColor(tessellatorInstance, color.r, color.g, color.b, color.a);

    for (int i = 0; i < 24; ++i) {
        tVertex(tessellatorInstance, edges[i].x, edges[i].y, edges[i].z);
    }

    rMesh(screenContext, tessellatorInstance, materialPtr);
}

} // namespace outline::renderer
