#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace outline::resolver {

// Deklarasi struct Candidate yang digunakan oleh resolver.cpp
struct Candidate {
    std::uintptr_t address;
    int score;
};

enum class Target : std::uint8_t {
    Unknown = 0,

    // Vanilla selection-box rendering target.
    SelectionBox,

    // Rendering/tessellation target used as fallback.
    Tessellator,

    // Render-level target.
    RenderLevel,
};

bool initialize(std::string_view libraryName);

void shutdown();

bool initialized();

// --- PENAMBAHAN DEKLARASI FUNGSI YANG HILANG ---

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

// -----------------------------------------------

std::uintptr_t resolve(Target target);

const char* targetName(Target target);

} // namespace outline::resolver
