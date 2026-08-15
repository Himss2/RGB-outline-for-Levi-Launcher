#include "outline/resolver.hpp"

#include <android/log.h>
#include <link.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <algorithm>

#define LOG_TAG "OutlineRGB"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

uintptr_t gBase = 0;
uintptr_t gTextBegin = 0;
uintptr_t gTextEnd = 0;

std::vector<OutlineResolver::Candidate> gCandidates;

bool isReadable(uintptr_t p) {
    return p != 0;
}

bool scanPattern(
    uintptr_t begin,
    uintptr_t end,
    const uint8_t* pattern,
    const char* mask,
    size_t size,
    std::vector<uintptr_t>& result
) {
    if (!begin || !end || end <= begin || size == 0)
        return false;

    for (uintptr_t p = begin; p + size <= end; ++p) {
        bool match = true;

        for (size_t i = 0; i < size; ++i) {
            if (mask[i] == 'x' && *(uint8_t*)(p + i) != pattern[i]) {
                match = false;
                break;
            }
        }

        if (match)
            result.push_back(p);
    }

    return !result.empty();
}

int validateCandidate(uintptr_t address) {
    if (!isReadable(address))
        return 0;

    int score = 0;

    /*
     * IMPORTANT:
     * This is deliberately only a structural validator.
     *
     * We do NOT yet declare a candidate to be
     * renderOutlineSelection solely from its prologue.
     */

    const uint32_t* code =
        reinterpret_cast<const uint32_t*>(address);

    // ARM64 function must contain valid-looking instructions.
    if (code[0] != 0)
        score += 1;

    if (code[1] != 0)
        score += 1;

    return score;
}

}

namespace OutlineResolver {

bool initialize() {
    LOGI("Initializing resolver");

    /*
     * The actual libminecraftpe.so mapping resolver will be
     * connected to the target loader/hook API used by the
     * standalone mod.
     *
     * We intentionally don't hardcode 0xAE2CAE4.
     */

    LOGI("Resolver initialized");

    return true;
}

uintptr_t findOutlineCandidate() {
    gCandidates.clear();

    /*
     * Candidate signatures from ThickBaddie are retained only
     * as discovery signatures.
     *
     * They MUST NOT be interpreted as proof of function identity.
     */

    static const uint8_t pattern[] = {
        0xFD, 0x7B, 0xBA, 0xA9,
        0xFC, 0x6F, 0x01, 0xA9,
        0xFA, 0x67, 0x02, 0xA9,
        0xF8, 0x5F, 0x03, 0xA9,
        0xF6, 0x57, 0x04, 0xA9
    };

    static const char mask[] =
        "xxxxxxxxxxxxxxxxxxxx";

    std::vector<uintptr_t> matches;

    if (!scanPattern(
        gTextBegin,
        gTextEnd,
        pattern,
        mask,
        sizeof(pattern),
        matches
    )) {
        LOGE("No candidate found");
        return 0;
    }

    LOGI(
        "Discovery signature produced %zu candidates",
        matches.size()
    );

    for (uintptr_t address : matches) {
        Candidate c;
        c.address = address;
        c.score = validateCandidate(address);

        gCandidates.push_back(c);

        LOGI(
            "candidate = %p score=%d",
            reinterpret_cast<void*>(address),
            c.score
        );
    }

    /*
     * NEVER automatically hook the first match.
     *
     * Until semantic validation is implemented, return 0.
     */
    return 0;
}

const std::vector<Candidate>& candidates() {
    return gCandidates;
}

}
