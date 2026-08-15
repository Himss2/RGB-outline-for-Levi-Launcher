#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace OutlineResolver {

struct MemoryRange {
    uintptr_t begin = 0;
    uintptr_t end = 0;

    size_t size() const {
        if (end <= begin)
            return 0;

        return static_cast<size_t>(end - begin);
    }

    bool valid() const {
        return begin != 0 && end > begin;
    }
};

struct Candidate {
    uintptr_t address = 0;
    int score = 0;
};

bool initialize();

bool findMinecraftLibrary(MemoryRange& library);

bool findTextRange(
    uintptr_t libraryBase,
    MemoryRange& text
);

std::vector<uintptr_t> scan(
    const MemoryRange& range,
    const uint8_t* pattern,
    const char* mask,
    size_t patternSize
);

int scoreCandidate(uintptr_t address);

const std::vector<Candidate>& getCandidates();

}
