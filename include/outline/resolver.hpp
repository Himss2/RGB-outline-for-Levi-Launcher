#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace OutlineResolver {

struct Candidate {
    uintptr_t address = 0;
    int score = 0;
};

bool initialize();

uintptr_t findOutlineCandidate();

const std::vector<Candidate>& candidates();

}
