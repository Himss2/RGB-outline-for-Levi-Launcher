#include "outline/renderer.hpp"
#include "outline/resolver.hpp"

#include <cmath>
#include <cstdint>

namespace outline::renderer {

namespace {

bool g_initialized = false;

} // namespace

bool initialize() {
    /*
     * Rendering is intentionally passive for now.
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
    void* screenContext,
    const SelectionBox& box,
    const Color& color
) {
    if (!g_initialized) {
        return;
    }

    if (!box.valid || !screenContext) {
        return;
    }

    if (!std::isfinite(color.r) ||
        !std::isfinite(color.g) ||
        !std::isfinite(color.b) ||
        !std::isfinite(color.a)) {
        return;
    }

    // 1. Cast alamat memori dari resolver ke Function Pointer AArch64
    using FnTessBegin = void (*)(void* _this, int topology);
    using FnTessVertex = void (*)(void* _this, float x, float y, float z);
    using FnTessColor = void (*)(void* _this, float r, float g, float b, float a);
    using FnRenderMesh = void (*)(void* screenCtx, void* tess, void* mat);

    auto tBegin  = reinterpret_cast<FnTessBegin>(resolver::tessellatorBegin());
    auto tVertex = reinterpret_cast<FnTessVertex>(resolver::tessellatorVertex());
    auto tColor  = reinterpret_cast<FnTessColor>(resolver::tessellatorColor());
    
    auto rMesh = reinterpret_cast<FnRenderMesh>(
        resolver::meshRenderImmediately() ? resolver::meshRenderImmediately() : resolver::meshRenderImmediately2()
    );

    if (!tBegin || !tVertex || !tColor || !rMesh) {
        return; 
    }

    // 2. Ekstrak Instance Tessellator dari ScreenContext (Kandidat kuat: Offset +0x30)
    void* tessellatorInstance = *reinterpret_cast<void**>(
        reinterpret_cast<std::uintptr_t>(screenContext) + 0x30
    );

    if (!tessellatorInstance) {
        return; // Mencegah crash jika struktur ScreenContext berubah
    }

    // 3. Kalkulasi Alamat Material UI Dinamis
    // Menggunakan UI Material Table RVA 0x125A3280, Index 1 (ui_fill_color) = +0x10 -> 0x125A3290
    std::uintptr_t baseAddress = resolver::libraryBase();
    if (!baseAddress) {
        return;
    }
    void* materialPtr = reinterpret_cast<void*>(baseAddress + 0x125A3290);

    // 4. Bangun kordinat sudut kotak (AABB)
    Vec3 edges[24]{};
    buildBoxEdges(box.bounds, edges);

    // 5. Eksekusi Mesin Gambar Minecraft (Tessellator)
    tBegin(tessellatorInstance, 1); // Topology 1 = Lines

    tColor(tessellatorInstance, color.r, color.g, color.b, color.a);

    for (int i = 0; i < 24; ++i) {
        tVertex(tessellatorInstance, edges[i].x, edges[i].y, edges[i].z);
    }

    // 6. Gambar ke Layar (X0 = screenContext, X1 = tessellator, X2 = materialPtr)
    rMesh(screenContext, tessellatorInstance, materialPtr);
}

} // namespace outline::renderer
