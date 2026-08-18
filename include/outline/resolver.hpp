#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace outline::resolver {

struct Candidate {
    std::uintptr_t address;
    int score;
};

enum class Target : std::uint8_t {
    Unknown = 0,
    SelectionBox,
    Tessellator,
    RenderLevel,
};

bool initialize(std::string_view libraryName);
void shutdown();
bool initialized();
bool ready();

std::uintptr_t libraryBase();
std::uintptr_t renderLevel();
std::uintptr_t clientInstanceUpdate();
std::uintptr_t clientInstanceGetLocalPlayer();
std::uintptr_t levelGetHitResult();
std::uintptr_t tessellatorBegin();
std::uintptr_t tessellatorColor();
std::uintptr_t tessellatorVertex();
std::uintptr_t meshRenderImmediately();
std::uintptr_t meshRenderImmediately2();
std::uintptr_t blockGetOutline();

const std::vector<Candidate>& blockOutlineCandidates();

std::uintptr_t resolve(Target target);
const char* targetName(Target target);

// --- DEKLARASI DYNAMIC ARM64 INSTRUCTION RESOLVER ---
std::uintptr_t resolveAdrlTarget(
    std::uintptr_t pcAddress,
    std::uint32_t adrpInst,
    std::uint32_t addInst
);

std::uintptr_t fetchDynamicMaterialPtr(
    std::uintptr_t callSiteAddress
);

} // namespace outline::resolver
