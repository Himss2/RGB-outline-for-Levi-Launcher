#include "outline/resolver.hpp"

#include <android/log.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>

#define LOG_TAG "OutlineRGB"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGW(...) \
    __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace {

OutlineResolver::MemoryRange gLibrary;
OutlineResolver::MemoryRange gText;

std::vector<OutlineResolver::Candidate> gCandidates;

bool containsLibraryName(
    const char* line,
    const char* libraryName
) {
    if (!line || !libraryName)
        return false;

    return std::strstr(line, libraryName) != nullptr;
}

}

namespace OutlineResolver {

bool initialize() {
    gLibrary = {};
    gText = {};
    gCandidates.clear();

    LOGI("Resolver initialized");

    return true;
}

bool findMinecraftLibrary(MemoryRange& library) {
    std::ifstream maps("/proc/self/maps");

    if (!maps.is_open()) {
        LOGW("Unable to open /proc/self/maps");
        return false;
    }

    std::string line;

    uintptr_t lowest = 0;
    uintptr_t highest = 0;

    while (std::getline(maps, line)) {
        if (!containsLibraryName(
                line.c_str(),
                "libminecraftpe.so")) {
            continue;
        }

        uintptr_t start = 0;
        uintptr_t end = 0;

        if (std::sscanf(
                line.c_str(),
                "%lx-%lx",
                &start,
                &end
            ) != 2) {
            continue;
        }

        if (start == 0 || end <= start)
            continue;

        if (lowest == 0 || start < lowest)
            lowest = start;

        if (end > highest)
            highest = end;
    }

    if (lowest == 0 || highest <= lowest) {
        LOGW("libminecraftpe.so not found");

        return false;
    }

    library.begin = lowest;
    library.end = highest;

    gLibrary = library;

    LOGI(
        "Minecraft library: %p - %p",
        reinterpret_cast<void*>(library.begin),
        reinterpret_cast<void*>(library.end)
    );

    return true;
}

bool findTextRange(
    uintptr_t libraryBase,
    MemoryRange& text
) {
    (void)libraryBase;

    /*
     * For the first probe we use executable mappings of
     * libminecraftpe.so from /proc/self/maps.
     *
     * The final resolver will obtain the exact ELF .text
     * section instead of treating every executable mapping
     * as text.
     */

    std::ifstream maps("/proc/self/maps");

    if (!maps.is_open())
        return false;

    std::string line;

    uintptr_t bestStart = 0;
    uintptr_t bestEnd = 0;

    while (std::getline(maps, line)) {
        if (!containsLibraryName(
                line.c_str(),
                "libminecraftpe.so")) {
            continue;
        }

        /*
         * Format:
         *
         * start-end perms offset ...
         */

        uintptr_t start = 0;
        uintptr_t end = 0;
        uintptr_t offset = 0;

        char permissions[5] = {};

        if (std::sscanf(
                line.c_str(),
                "%lx-%lx %4s %lx",
                &start,
                &end,
                permissions,
                &offset
            ) != 4) {
            continue;
        }

        /*
         * Executable mapping.
         */
        if (permissions[2] != 'x')
            continue;

        if (end <= start)
            continue;

        if (bestStart == 0 || start < bestStart) {
            bestStart = start;
            bestEnd = end;
        }
    }

    if (bestStart == 0 || bestEnd <= bestStart) {
        LOGW("Executable libminecraftpe mapping not found");
        return false;
    }

    text.begin = bestStart;
    text.end = bestEnd;

    gText = text;

    LOGI(
        "Minecraft executable range: %p - %p (%zu bytes)",
        reinterpret_cast<void*>(text.begin),
        reinterpret_cast<void*>(text.end),
        text.size()
    );

    return true;
}

std::vector<uintptr_t> scan(
    const MemoryRange& range,
    const uint8_t* pattern,
    const char* mask,
    size_t patternSize
) {
    std::vector<uintptr_t> result;

    if (!range.valid())
        return result;

    if (!pattern || !mask || patternSize == 0)
        return result;

    if (range.size() < patternSize)
        return result;

    for (
        uintptr_t address = range.begin;
        address + patternSize <= range.end;
        ++address
    ) {
        bool match = true;

        for (size_t i = 0; i < patternSize; ++i) {
            if (mask[i] != 'x')
                continue;

            const auto value =
                *reinterpret_cast<const uint8_t*>(
                    address + i
                );

            if (value != pattern[i]) {
                match = false;
                break;
            }
        }

        if (match)
            result.push_back(address);
    }

    return result;
}

int scoreCandidate(uintptr_t address) {
    if (address == 0)
        return 0;

    int score = 0;

    /*
     * ARM64 instructions are 4-byte aligned.
     */
    if ((address & 3) == 0)
        score += 1;

    /*
     * We only perform very conservative structural checks
     * here. This is NOT semantic identification.
     */
    const uint32_t i0 =
        *reinterpret_cast<const uint32_t*>(address);

    const uint32_t i1 =
        *reinterpret_cast<const uint32_t*>(address + 4);

    if (i0 != 0)
        score++;

    if (i1 != 0)
        score++;

    return score;
}

const std::vector<Candidate>& getCandidates() {
    return gCandidates;
}

}
