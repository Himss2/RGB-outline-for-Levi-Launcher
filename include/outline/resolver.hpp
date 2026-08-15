#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace outline::resolver {

struct Candidate {
    std::uintptr_t address{};
    int score{};
};

bool initialize(std::string_view libraryName);

bool ready();

std::uintptr_t libraryBase();

std::uintptr_t renderLevel();

std::uintptr_t tessellatorBegin();

std::uintptr_t tessellatorColor();

std::uintptr_t tessellatorVertex();

std::uintptr_t meshRenderImmediately();

std::uintptr_t blockGetOutline();

const std::vector<Candidate>& blockOutlineCandidates();

}
