#include <outline/resolver.hpp>

#include <cstdint>
#include <cstring>
#include <link.h>
#include <string>
#include <vector>

namespace outline::resolver {

namespace {

std::uintptr_t gBase = 0;

std::uintptr_t gSelectionGeometry = 0;
std::uintptr_t gRenderLevel = 0;

std::uintptr_t gTessellatorBegin = 0;
std::uintptr_t gTessellatorColor = 0;
std::uintptr_t gTessellatorVertex = 0;

struct ModuleInfo {
    std::uintptr_t base{};
    std::string name;
};

int callback(struct dl_phdr_info* info, size_t, void* data) {
    auto* wanted = static_cast<std::string*>(data);

    if (!info->dlpi_name)
        return 0;

    const char* slash = std::strrchr(info->dlpi_name, '/');
    const char* filename = slash ? slash + 1 : info->dlpi_name;

    if (*wanted == filename) {
        gBase = static_cast<std::uintptr_t>(info->dlpi_addr);
        return 1;
    }

    return 0;
}

std::uintptr_t findLibraryBase(std::string_view name) {
    std::string wanted{name};

    gBase = 0;

    dl_iterate_phdr(
        callback,
        &wanted
    );

    return gBase;
}

/*
 * These are deliberately kept as RVAs only for the current
 * 26.44 verification target.
 *
 * Production resolver should replace these with the project's
 * existing signature scanner once its scanner API is wired here.
 */
constexpr std::uintptr_t kSelectionGeometryRva = 0xAE2E06C;
constexpr std::uintptr_t kRenderLevelRva        = 0xAE0CAA8;

constexpr std::uintptr_t kTessBeginRva         = 0xA3FA640;
constexpr std::uintptr_t kTessColorRva         = 0xA3FABF4;
constexpr std::uintptr_t kTessVertexRva        = 0xA3FAE90;

}

bool initialize(std::string_view library) {
    const auto base = findLibraryBase(library);

    if (!base)
        return false;

    gSelectionGeometry = base + kSelectionGeometryRva;
    gRenderLevel       = base + kRenderLevelRva;

    gTessellatorBegin  = base + kTessBeginRva;
    gTessellatorColor  = base + kTessColorRva;
    gTessellatorVertex = base + kTessVertexRva;

    return ready();
}

std::uintptr_t selectionGeometry() {
    return gSelectionGeometry;
}

std::uintptr_t renderLevel() {
    return gRenderLevel;
}

std::uintptr_t tessellatorBegin() {
    return gTessellatorBegin;
}

std::uintptr_t tessellatorColor() {
    return gTessellatorColor;
}

std::uintptr_t tessellatorVertex() {
    return gTessellatorVertex;
}

bool ready() {
    return
        gSelectionGeometry != 0 &&
        gRenderLevel != 0 &&
        gTessellatorBegin != 0 &&
        gTessellatorColor != 0 &&
        gTessellatorVertex != 0;
}

}
