#include "outline/resolver.hpp"
#include "outline/types.hpp"

#include <android/log.h>
#include <link.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#define LOG_TAG "OutlineRGB"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace outline::resolver {

namespace {

ModuleRange gModule{};

std::uintptr_t gRenderLevel{};
std::uintptr_t gTessellatorBegin{};
std::uintptr_t gTessellatorColor{};
std::uintptr_t gTessellatorVertex{};
std::uintptr_t gMeshRenderImmediately{};
std::uintptr_t gBlockGetOutline{};

std::vector<Candidate> gBlockCandidates;

struct PatternByte {
    std::uint8_t value{};
    bool wildcard{};
};

using Pattern = std::vector<PatternByte>;

Pattern parsePattern(std::string_view text) {
    Pattern result;

    std::size_t i = 0;

    while (i < text.size()) {
        while (i < text.size() && text[i] == ' ')
            ++i;

        if (i >= text.size())
            break;

        if (i + 1 >= text.size())
            break;

        if (text[i] == '?' && text[i + 1] == '?') {
            result.push_back({
                0,
                true
            });

            i += 2;
            continue;
        }

        auto hex = [](char c) -> int {
            if (c >= '0' && c <= '9')
                return c - '0';

            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;

            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;

            return -1;
        };

        const int hi = hex(text[i]);
        const int lo = hex(text[i + 1]);

        if (hi < 0 || lo < 0)
            break;

        result.push_back({
            static_cast<std::uint8_t>((hi << 4) | lo),
            false
        });

        i += 2;
    }

    return result;
}

bool matchPattern(
    std::uintptr_t address,
    const Pattern& pattern
) {
    if (!address || pattern.empty())
        return false;

    const auto* bytes =
        reinterpret_cast<const std::uint8_t*>(address);

    for (std::size_t i = 0; i < pattern.size(); ++i) {
        if (!pattern[i].wildcard &&
            bytes[i] != pattern[i].value) {
            return false;
        }
    }

    return true;
}

std::vector<std::uintptr_t> scan(
    std::uintptr_t begin,
    std::uintptr_t end,
    const Pattern& pattern
) {
    std::vector<std::uintptr_t> result;

    if (!begin ||
        end <= begin ||
        pattern.empty() ||
        end - begin < pattern.size()) {
        return result;
    }

    const std::uintptr_t last =
        end - pattern.size();

    for (std::uintptr_t address = begin;
         address <= last;
         address += 4) {

        if (matchPattern(address, pattern))
            result.push_back(address);
    }

    return result;
}

int scoreFunction(
    std::uintptr_t address
) {
    if (!address)
        return 0;

    const auto* code =
        reinterpret_cast<const std::uint32_t*>(address);

    int score = 0;

    /*
     * AArch64 instructions are 4-byte aligned.
     *
     * We deliberately keep this validator conservative.
     */
    if ((address & 3u) == 0)
        score += 2;

    if (code[0] != 0)
        ++score;

    if (code[1] != 0)
        ++score;

    return score;
}

int phdrCallback(
    dl_phdr_info* info,
    std::size_t,
    void* user
) {
    auto* wanted =
        static_cast<std::string*>(user);

    if (!info->dlpi_name)
        return 0;

    const char* slash =
        std::strrchr(info->dlpi_name, '/');

    const char* filename =
        slash ? slash + 1 : info->dlpi_name;

    if (*wanted != filename)
        return 0;

    gModule.base =
        static_cast<std::uintptr_t>(info->dlpi_addr);

    for (std::size_t i = 0;
         i < info->dlpi_phnum;
         ++i) {

        const auto& phdr =
            info->dlpi_phdr[i];

        if (phdr.p_type != PT_LOAD)
            continue;

        if ((phdr.p_flags & PF_X) == 0)
            continue;

        const auto begin =
            gModule.base +
            static_cast<std::uintptr_t>(phdr.p_vaddr);

        const auto end =
            begin +
            static_cast<std::uintptr_t>(phdr.p_memsz);

        if (!gModule.textBegin ||
            begin < gModule.textBegin) {
            gModule.textBegin = begin;
        }

        if (end > gModule.textEnd)
            gModule.textEnd = end;
    }

    return 1;
}

bool findModule(
    std::string_view libraryName
) {
    std::string wanted(libraryName);

    gModule = {};

    dl_iterate_phdr(
        phdrCallback,
        &wanted
    );

    return gModule.valid();
}

/*
 * These signatures are directly derived from the
 * 26.44 BedrockTools signature table supplied for
 * this project.
 *
 * They are discovery signatures, not RVAs.
 */
const Pattern& patternRenderLevel() {
    static const Pattern pattern =
        parsePattern(
            "? ? ? FC "
            "? ? ? 6D "
            "? ? ? 6D "
            "? ? ? 6D "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? 91 "
            "? ? ? D1 "
            "57 D0 3B D5"
        );

    return pattern;
}

const Pattern& patternTessellatorBegin() {
    static const Pattern pattern =
        parsePattern(
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "FD 03 00 91 "
            "? ? ? 39 "
            "? ? ? 39 "
            "08 01 09 2A"
        );

    return pattern;
}

const Pattern& patternTessellatorColor() {
    static const Pattern pattern =
        parsePattern(
            "? ? ? 52 "
            "? ? ? 39 "
            "04 01 27 1E"
        );

    return pattern;
}

const Pattern& patternTessellatorVertex() {
    static const Pattern pattern =
        parsePattern(
            "? ? ? D1 "
            "? ? ? FD "
            "? ? ? 6D "
            "? ? ? A9 "
            "? ? ? F9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? 91 "
            "58 D0 3B D5 "
            "? ? ? F9"
        );

    return pattern;
}

const Pattern& patternMeshRenderImmediately() {
    static const Pattern pattern =
        parsePattern(
            "? ? ? A9 "
            "? ? ? F9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "FD 03 00 91 "
            "? ? ? D1 "
            "58 D0 3B D5 "
            "F7 03 00 AA "
            "E0 03 01 AA "
            "? ? ? F9 "
            "F4 03 04 AA"
        );

    return pattern;
}

/*
 * block_getOutline is intentionally NOT assigned an RVA.
 *
 * The current repo does not contain a verified 26.44
 * signature for this function.
 *
 * Therefore the resolver leaves this address zero
 * instead of risking a crash by hooking an unrelated
 * function.
 */
const Pattern& patternBlockGetOutlineDiscovery() {
    static const Pattern pattern =
        parsePattern(
            "? ? ? D1 "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? A9 "
            "? ? ? 91"
        );

    return pattern;
}

std::uintptr_t resolveUnique(
    const Pattern& pattern,
    const char* name
) {
    const auto matches =
        scan(
            gModule.textBegin,
            gModule.textEnd,
            pattern
        );

    if (matches.empty()) {
        LOGE(
            "%s: no matches",
            name
        );

        return 0;
    }

    if (matches.size() > 1) {
        LOGI(
            "%s: %zu matches",
            name,
            matches.size()
        );
    }

    return matches.front();
}

}

bool initialize(
    std::string_view libraryName
) {
    if (!findModule(libraryName)) {
        LOGE(
            "Unable to locate %.*s",
            static_cast<int>(libraryName.size()),
            libraryName.data()
        );

        return false;
    }

    LOGI(
        "Minecraft base = %p",
        reinterpret_cast<void*>(gModule.base)
    );

    LOGI(
        "Executable range = %p - %p",
        reinterpret_cast<void*>(gModule.textBegin),
        reinterpret_cast<void*>(gModule.textEnd)
    );

    gRenderLevel =
        resolveUnique(
            patternRenderLevel(),
            "RenderLevel"
        );

    gTessellatorBegin =
        resolveUnique(
            patternTessellatorBegin(),
            "TessellatorBegin"
        );

    gTessellatorColor =
        resolveUnique(
            patternTessellatorColor(),
            "TessellatorColor"
        );

    gTessellatorVertex =
        resolveUnique(
            patternTessellatorVertex(),
            "TessellatorVertex"
        );

    gMeshRenderImmediately =
        resolveUnique(
            patternMeshRenderImmediately(),
            "MeshHelpersRenderMeshImmediately"
        );

    /*
     * Discovery only.
     *
     * Never automatically install this address.
     */
    gBlockCandidates.clear();

    const auto blockMatches =
        scan(
            gModule.textBegin,
            gModule.textEnd,
            patternBlockGetOutlineDiscovery()
        );

    for (const auto address : blockMatches) {
        gBlockCandidates.push_back({
            address,
            scoreFunction(address)
        });
    }

    std::sort(
        gBlockCandidates.begin(),
        gBlockCandidates.end(),
        [](const Candidate& a, const Candidate& b) {
            return a.score > b.score;
        }
    );

    LOGI(
        "block_getOutline discovery candidates = %zu",
        gBlockCandidates.size()
    );

    /*
     * IMPORTANT:
     *
     * We deliberately don't assign gBlockGetOutline
     * from an unverified candidate.
     */
    gBlockGetOutline = 0;

    return ready();
}

bool ready() {
    return
        gModule.valid() &&
        gRenderLevel != 0 &&
        gTessellatorBegin != 0 &&
        gTessellatorColor != 0 &&
        gTessellatorVertex != 0 &&
        gMeshRenderImmediately != 0;
}

std::uintptr_t libraryBase() {
    return gModule.base;
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

std::uintptr_t meshRenderImmediately() {
    return gMeshRenderImmediately;
}

std::uintptr_t blockGetOutline() {
    return gBlockGetOutline;
}

const std::vector<Candidate>&
blockOutlineCandidates() {
    return gBlockCandidates;
}

}
