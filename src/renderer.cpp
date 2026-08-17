#include "outline/renderer.hpp"
#include "outline/resolver.hpp"

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
    using FnRenderMesh = void (*)(void* screenContext, void* tessellator, void* material);

    auto tBegin  = reinterpret_cast<FnTessBegin>(resolver::tessellatorBegin());
    auto tVertex = reinterpret_cast<FnTessVertex>(resolver::tessellatorVertex());
    auto tColor  = reinterpret_cast<FnTessColor>(resolver::tessellatorColor());
    
    // Pilih render immediately yang valid
    auto rMesh = reinterpret_cast<FnRenderMesh>(
        resolver::meshRenderImmediately() ? resolver::meshRenderImmediately() : resolver::meshRenderImmediately2()
    );

    // Keamanan: Jika ada 1 saja alamat yang gagal di-resolve, batalkan render agar tidak crash
    if (!tBegin || !tVertex || !tColor || !rMesh) {
        return; 
    }

    Vec3 edges[24]{};

    buildBoxEdges(
        box.bounds,
        edges
    );

    // 2. Dapatkan Instance Tessellator dan Material
    // SEMENTARA KITA GUNAKAN NULL/ALAMAT DUMMY JIKA BELUM ADA POINTERNYA
    void* tessellatorInstance = reinterpret_cast<void*>(0x0); // Ganti dengan pointer Tessellator asli nanti
    void* materialPtr = reinterpret_cast<void*>(0x0); // Ganti dengan material vanilla

    // PERHATIAN: Jika tessellatorInstance masih NULL, baris di bawah ini akan memblokir
    // proses render agar game tidak crash. Hapus return ini setelah Anda memiliki instance yang valid!
    if (!tessellatorInstance) {
        return; 
    }

    // 3. Mulai Menggambar (Topology 1 = Lines, 3 = LineStrip)
    tBegin(tessellatorInstance, 1); 

    // 4. Setel Warna (Format RGBA)
    tColor(tessellatorInstance, color.r, color.g, color.b, color.a);

    // 5. Masukkan ke-24 kordinat sudut sebagai Vertex
    for (int i = 0; i < 24; ++i) {
        tVertex(tessellatorInstance, edges[i].x, edges[i].y, edges[i].z);
    }

    // 6. Eksekusi ke Layar
    rMesh(screenContext, tessellatorInstance, materialPtr);
}

} // namespace outline::renderer
