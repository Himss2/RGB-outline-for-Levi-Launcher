#include "outline/resolver.hpp"
#include "outline/types.hpp"

#include <android/log.h>
#include <link.h>

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdio>
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
std::uintptr_t gClientInstanceUpdate{};
std::uintptr_t gClientInstanceGetLocalPlayer{};
std::uintptr_t gLevelGetHitResult{};

std::uintptr_t gTessellatorBegin{};
std::uintptr_t gTessellatorColor{};
std::uintptr_t gTessellatorVertex{};

std::uintptr_t gMeshRenderImmediately{};
std::uintptr_t gMeshRenderImmediately2{};

std::uintptr_t gBlockGetOutline{};

std::vector<Candidate> gBlockCandidates;

/*
 * Only memory ranges that are actually readable are placed here.
 *
 * This is important on Android because an ELF executable segment
 * described by PT_LOAD/PF_X is not by itself a guarantee that every
 * address in the calculated range can safely be read.
 */
struct ScanRange {
    std::uintptr_t begin{};
    std::uintptr_t end{};

    bool valid() const {
        return begin != 0 && end > begin;
    }
};

std::vector<ScanRange> gScanRanges;

struct PatternByte {
    std::uint8_t value{};
    bool wildcard{};
};

using Pattern = std::vector<PatternByte>;

/* ------------------------------------------------------------- */
/* Pattern parser                                                 */
/* ------------------------------------------------------------- */

Pattern parsePattern(std::string_view text) {
    Pattern result;

    std::size_t i = 0;

    while (i < text.size()) {
        while (i < text.size() && text[i] == ' ')
            ++i;

        if (i >= text.size())
            break;

        if (text[i] == '?') {
            result.push_back({
                0,
                true
            });

            ++i;

            if (i < text.size() && text[i] == '?')
                ++i;

            continue;
        }

        if (i + 1 >= text.size())
            break;

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
            static_cast<std::uint8_t>(
                (hi << 4) | lo
            ),
            false
        });

        i += 2;
    }

    return result;
}

/* ------------------------------------------------------------- */
/* Safe memory-range helpers                                      */
/* ------------------------------------------------------------- */

bool rangeContains(
    const ScanRange& range,
    std::uintptr_t address,
    std::size_t size
) {
    if (!range.valid())
        return false;

    if (address < range.begin)
        return false;

    if (size == 0)
        return address <= range.end;

    if (address > range.end)
        return false;

    const std::uintptr_t remaining =
        range.end - address;

    return remaining >= size;
}

/*
 * Find whether an address range is completely contained inside one
 * readable mapping.
 *
 * We deliberately require the entire pattern to fit inside a single
 * mapping. This prevents a signature from crossing a mapping boundary.
 */
bool readableRangeContains(
    std::uintptr_t address,
    std::size_t size
) {
    for (const auto& range : gScanRanges) {
        if (rangeContains(range, address, size))
            return true;
    }

    return false;
}

/* ------------------------------------------------------------- */
/* Safe pattern matching                                          */
/* ------------------------------------------------------------- */

bool matchPattern(
    std::uintptr_t address,
    const Pattern& pattern
) {
    if (!address || pattern.empty())
        return false;

    if (!readableRangeContains(
            address,
            pattern.size()
        )) {
        return false;
    }

    const auto* bytes =
        reinterpret_cast<const std::uint8_t*>(
            address
        );

    for (std::size_t i = 0; i < pattern.size(); ++i) {
        if (!pattern[i].wildcard &&
            bytes[i] != pattern[i].value) {
            return false;
        }
    }

    return true;
}

/* ------------------------------------------------------------- */
/* Scan one readable mapping                                      */
/* ------------------------------------------------------------- */

std::vector<std::uintptr_t> scanRange(
    const ScanRange& range,
    const Pattern& pattern
) {
    std::vector<std::uintptr_t> result;

    if (!range.valid() ||
        pattern.empty() ||
        range.end - range.begin < pattern.size()) {
        return result;
    }

    const std::uintptr_t last =
        range.end - pattern.size();

    /*
     * AArch64 instructions are 4-byte aligned.
     */
    for (
        std::uintptr_t address = range.begin;
        address <= last;
        address += 4
    ) {
        if (matchPattern(address, pattern))
            result.push_back(address);
    }

    return result;
}

/* ------------------------------------------------------------- */
/* Scan all safe ranges                                           */
/* ------------------------------------------------------------- */

std::vector<std::uintptr_t> scan(
    const Pattern& pattern
) {
    std::vector<std::uintptr_t> result;

    if (pattern.empty())
        return result;

    for (const auto& range : gScanRanges) {
        const auto matches =
            scanRange(
                range,
                pattern
            );

        result.insert(
            result.end(),
            matches.begin(),
            matches.end()
        );
    }

    return result;
}

/* ------------------------------------------------------------- */
/* Candidate scoring                                              */
/* ------------------------------------------------------------- */

int scoreFunction(
    std::uintptr_t address
) {
    if (!address)
        return 0;

    /*
     * We read exactly 8 bytes, therefore verify that the address is
     * inside a readable mapping first.
     */
    if (!readableRangeContains(
            address,
            sizeof(std::uint32_t) * 2
        )) {
        return 0;
    }

    const auto* code =
        reinterpret_cast<const std::uint32_t*>(
            address
        );

    int score = 0;

    if ((address & 3u) == 0)
        score += 2;

    if (code[0] != 0)
        ++score;

    if (code[1] != 0)
        ++score;

    return score;
}

/* ------------------------------------------------------------- */
/* Module discovery via dl_iterate_phdr                           */
/* ------------------------------------------------------------- */

int phdrCallback(
    dl_phdr_info* info,
    std::size_t,
    void* user
) {
    auto* wanted =
        static_cast<std::string*>(user);

    if (!info || !wanted)
        return 0;

    if (!info->dlpi_name)
        return 0;

    const char* slash =
        std::strrchr(
            info->dlpi_name,
            '/'
        );

    const char* filename =
        slash
            ? slash + 1
            : info->dlpi_name;

    if (*wanted != filename)
        return 0;

    gModule.base =
        static_cast<std::uintptr_t>(
            info->dlpi_addr
        );

    /*
     * Determine the ELF executable range as a reference only.
     *
     * We DO NOT scan this range directly.
     *
     * Actual scanning ranges are populated from /proc/self/maps
     * below, where memory permissions are explicitly available.
     */
    for (
        std::size_t i = 0;
        i < info->dlpi_phnum;
        ++i
    ) {
        const auto& phdr =
            info->dlpi_phdr[i];

        if (phdr.p_type != PT_LOAD)
            continue;

        if ((phdr.p_flags & PF_X) == 0)
            continue;

        const auto begin =
            gModule.base +
            static_cast<std::uintptr_t>(
                phdr.p_vaddr
            );

        const auto end =
            begin +
            static_cast<std::uintptr_t>(
                phdr.p_memsz
            );

        if (!gModule.textBegin ||
            begin < gModule.textBegin) {
            gModule.textBegin = begin;
        }

        if (end > gModule.textEnd)
            gModule.textEnd = end;
    }

    return 1;
}

/* ------------------------------------------------------------- */
/* /proc/self/maps discovery                                      */
/* ------------------------------------------------------------- */

bool findReadableExecutableMappings(
    std::string_view libraryName
) {
    gScanRanges.clear();

    FILE* maps =
        std::fopen(
            "/proc/self/maps",
            "re"
        );

    if (!maps) {
        LOGE(
            "Unable to open /proc/self/maps"
        );

        return false;
    }

    char line[1024]{};

    while (std::fgets(
        line,
        sizeof(line),
        maps
    )) {
        unsigned long long beginRaw{};
        unsigned long long endRaw{};
        unsigned long long offsetRaw{};

        char permissions[5]{};
        char path[768]{};

        const int parsed =
            std::sscanf(
                line,
                "%llx-%llx %4s %llx %*s %*s %767[^\n]",
                &beginRaw,
                &endRaw,
                permissions,
                &offsetRaw,
                path
            );

        if (parsed < 4)
            continue;

        /*
         * We need READ + EXECUTE.
         *
         * "r-xp" is the normal executable file mapping.
         *
         * We intentionally do not accept:
         *
         *   -r--p
         *   rw-p
         *   --xp
         *
         * because our scanner dereferences the memory directly.
         */
        if (permissions[0] != 'r' ||
            permissions[2] != 'x') {
            continue;
        }

        if (beginRaw >= endRaw)
            continue;

        if (parsed < 5)
            continue;

        std::string mappedPath(path);

        /*
         * Strip a possible leading space.
         */
        while (
            !mappedPath.empty() &&
            mappedPath.front() == ' '
        ) {
            mappedPath.erase(
                mappedPath.begin()
            );
        }

        /*
         * Strip " (deleted)".
         *
         * Android may expose a deleted/shared-object path this way.
         */
        constexpr std::string_view deletedSuffix =
            " (deleted)";

        if (
            mappedPath.size() >=
            deletedSuffix.size() &&
            mappedPath.compare(
                mappedPath.size() -
                    deletedSuffix.size(),
                deletedSuffix.size(),
                deletedSuffix
            ) == 0
        ) {
            mappedPath.erase(
                mappedPath.size() -
                    deletedSuffix.size()
            );
        }

        const char* slash =
            std::strrchr(
                mappedPath.c_str(),
                '/'
            );

        const char* filename =
            slash
                ? slash + 1
                : mappedPath.c_str();

        if (
            std::string_view(filename) !=
            libraryName
        ) {
            continue;
        }

        const ScanRange range{
            static_cast<std::uintptr_t>(
                beginRaw
            ),
            static_cast<std::uintptr_t>(
                endRaw
            )
        };

        if (!range.valid())
            continue;

        gScanRanges.push_back(range);

        LOGI(
            "Readable executable mapping: "
            "%p - %p perms=%s offset=0x%llx",
            reinterpret_cast<void*>(
                range.begin
            ),
            reinterpret_cast<void*>(
                range.end
            ),
            permissions,
            offsetRaw
        );
    }

    std::fclose(maps);

    /*
     * Sort ranges so diagnostic output is deterministic.
     */
    std::sort(
        gScanRanges.begin(),
        gScanRanges.end(),
        [](const ScanRange& a,
           const ScanRange& b) {
            return a.begin < b.begin;
        }
    );

    /*
     * Remove exact duplicates.
     */
    gScanRanges.erase(
        std::unique(
            gScanRanges.begin(),
            gScanRanges.end(),
            [](const ScanRange& a,
               const ScanRange& b) {
                return
                    a.begin == b.begin &&
                    a.end == b.end;
            }
        ),
        gScanRanges.end()
    );

    return !gScanRanges.empty();
}

/* ------------------------------------------------------------- */
/* Module discovery                                               */
/* ------------------------------------------------------------- */

bool findModule(
    std::string_view libraryName
) {
    std::string wanted(libraryName);

    gModule = {};
    gScanRanges.clear();

    /*
     * First ask the dynamic linker whether the library is loaded.
     */
    dl_iterate_phdr(
        phdrCallback,
        &wanted
    );

    if (!gModule.valid()) {
        return false;
    }

    /*
     * Then ask the kernel's memory-map view which parts are
     * actually readable + executable.
     */
    if (!findReadableExecutableMappings(
            libraryName
        )) {
        LOGE(
            "No readable executable mapping "
            "found for %.*s",
            static_cast<int>(
                libraryName.size()
            ),
            libraryName.data()
        );

        gModule = {};
        return false;
    }

    return true;
}

/* ------------------------------------------------------------- */
/* Signatures                                                     */
/* ------------------------------------------------------------- */

const Pattern& patternRenderLevel() {
    static const Pattern pattern =
        parsePattern(
            "EE 0F 16 FC ED 33 01 6D "
            "EB 2B 02 6D E9 23 03 6D "
            "FD 7B 04 A9 FC 6F 05 A9 "
            "FA 67 06 A9 F8 5F 07 A9 "
            "F6 57 08 A9 F4 4F 09 A9 "
            "FD 03 01 91 FF 03 06 D1 "
            "57 D0 3B D5 F8 03 00 AA "
            "F4 03 02 AA E8 16 40 F9"
        );

    return pattern;
}

const Pattern& patternClientInstanceUpdate() {
    static const Pattern pattern =
        parsePattern(
            "FD 7B BA A9 FC 6F 01 A9 "
            "FA 67 02 A9 F8 5F 03 A9 "
            "F6 57 04 A9 F4 4F 05 A9 "
            "FD 03 00 91 FF C3 12 D1 "
            "59 D0 3B D5 F3 03 00 AA "
            "F4 03 01 2A 28 17 40 F9 "
            "A8 83 1F F8 08 00 40 F9 "
            "09 35 46 F9 E8 E3 01 91"
        );

    return pattern;
}

const Pattern& patternClientInstanceGetLocalPlayer() {
    static const Pattern pattern =
        parsePattern(
            "FF 43 01 D1 FD 7B 03 A9 "
            "F3 23 00 F9 FD C3 00 91 "
            "53 D0 3B D5 E8 03 00 AA "
            "E0 23 00 91 69 16 40 F9 "
            "01 61 08 91 A9 83 1F F8 "
            "? ? ? ? E0 23 00 91 "
            "? ? ? ? ? ? ? ? "
            "E0 23 00 91 21 00 80 52 "
            "? ? ? ? ? ? ? ? "
            "E0 03 1F AA 68 16 40 F9 "
            "A9 83 5F F8 1F 01 09 EB "
            "? ? ? ? FD 7B 43 A9"
        );

    return pattern;
}

const Pattern& patternLevelGetHitResult() {
    static const Pattern pattern =
        parsePattern(
            "00 E8 40 F9 C0 03 5F D6 "
            "00 E8 40 F9 ? ? ? ? "
            "FD 7B BD A9 F6 57 01 A9 "
            "F4 4F 02 A9 FD 03 00 91 "
            "09 E4 40 F9 1F 05 00 F9 "
            "? ? ? ? 35 D9 40 A9 "
            "F3 03 00 AA F4 03 08 AA "
            "? ? ? ? C1 22 00 91"
        );

    return pattern;
}

const Pattern& patternTessellatorBegin() {
    static const Pattern pattern =
        parsePattern(
            "FD 7B BC A9 F8 5F 01 A9 "
            "F6 57 02 A9 F4 4F 03 A9 "
            "FD 03 00 91 08 20 4A 39 "
            "09 14 49 39 08 01 09 2A "
            "? ? ? ? F3 03 00 AA "
            "1F 80 02 B9 F6 03 04 2A "
            "1F 14 09 39 F4 03 03 2A "
            "F5 03 02 2A 1F 20 0A 39"
        );

    return pattern;
}

const Pattern& patternTessellatorColor() {
    static const Pattern pattern =
        parsePattern(
            "E8 6F A8 52 0C 10 49 39 "
            "04 01 27 1E 00 08 24 1E "
            "21 08 24 1E 42 08 24 1E "
            "63 08 24 1E 08 00 38 1E "
            "29 00 38 1E 4A 00 38 1E "
            "6B 00 38 1E ? ? ? ? "
            "6B 7D AB 0A EC 1F 80 52 "
            "4A 7D AA 0A 29 7D A9 0A"
        );

    return pattern;
}

const Pattern& patternTessellatorVertex() {
    static const Pattern pattern =
        parsePattern(
            "FF 43 02 D1 EA 13 00 FD "
            "E9 A3 02 6D FD FB 03 A9 "
            "FB 27 00 F9 FA 67 05 A9 "
            "F8 5F 06 A9 F6 57 07 A9 "
            "F4 4F 08 A9 FD E3 00 91 "
            "58 D0 3B D5 08 17 40 F9 "
            "E8 0F 00 F9 08 80 42 B9 "
            "09 84 42 B9 1F 01 09 6B"
        );

    return pattern;
}

const Pattern& patternMeshRenderImmediately() {
    static const Pattern pattern =
        parsePattern(
            "FD 7B BB A9 FC 0B 00 F9 "
            "F8 5F 02 A9 F6 57 03 A9 "
            "F4 4F 04 A9 FD 03 00 91 "
            "FF C3 09 D1 58 D0 3B D5 "
            "F7 03 00 AA E0 03 01 AA "
            "08 17 40 F9 F4 03 04 AA "
            "F5 03 03 AA F6 03 02 AA "
            "F3 03 01 AA A8 83 1F F8"
        );

    return pattern;
}

const Pattern& patternMeshRenderImmediately2() {
    static const Pattern pattern =
        parsePattern(
            "FD 7B BC A9 FC 5F 01 A9 "
            "F6 57 02 A9 F4 4F 03 A9 "
            "FD 03 00 91 FF C3 09 D1 "
            "57 D0 3B D5 F6 03 00 AA "
            "E0 03 01 AA E8 16 40 F9 "
            "F4 03 03 AA F5 03 02 AA "
            "F3 03 01 AA A8 83 1F F8 "
            "? ? ? ? ? ? ? ?"
        );

    return pattern;
}

const Pattern& patternBlockGetOutlineDiscovery() {
    static const Pattern pattern =
        parsePattern(
            "? ? ? D1 ? ? ? A9 "
            "? ? ? A9 ? ? ? A9 "
            "? ? ? 91"
        );

    return pattern;
}

/* ------------------------------------------------------------- */
/* Resolver                                                       */
/* ------------------------------------------------------------- */

std::uintptr_t resolveUnique(
    const Pattern& pattern,
    const char* name
) {
    const auto matches =
        scan(pattern);

    if (matches.empty()) {
        LOGE(
            "%s: no matches (pattern bytes=%zu)",
            name,
            pattern.size()
        );

        return 0;
    }

    if (matches.size() > 1) {
        LOGI(
            "%s: %zu matches; using first",
            name,
            matches.size()
        );
    }

    const auto address =
        matches.front();

    LOGI(
        "%s = %p",
        name,
        reinterpret_cast<void*>(address)
    );

    return address;
}

}

/* ------------------------------------------------------------- */
/* Public API                                                     */
/* ------------------------------------------------------------- */

bool initialize(
    std::string_view libraryName
) {
    if (!findModule(libraryName)) {
        LOGE(
            "Unable to locate %.*s",
            static_cast<int>(
                libraryName.size()
            ),
            libraryName.data()
        );

        return false;
    }

    LOGI(
        "Minecraft base = %p",
        reinterpret_cast<void*>(gModule.base)
    );

    LOGI(
        "ELF executable range = %p - %p",
        reinterpret_cast<void*>(
            gModule.textBegin
        ),
        reinterpret_cast<void*>(
            gModule.textEnd
        )
    );

    LOGI(
        "Readable executable mappings = %zu",
        gScanRanges.size()
    );

    for (
        std::size_t i = 0;
        i < gScanRanges.size();
        ++i
    ) {
        LOGI(
            "ScanRange[%zu] = %p - %p",
            i,
            reinterpret_cast<void*>(
                gScanRanges[i].begin
            ),
            reinterpret_cast<void*>(
                gScanRanges[i].end
            )
        );
    }

    gRenderLevel =
        resolveUnique(
            patternRenderLevel(),
            "RenderLevel"
        );

    gClientInstanceUpdate =
        resolveUnique(
            patternClientInstanceUpdate(),
            "ClientInstanceUpdate"
        );

    gClientInstanceGetLocalPlayer =
        resolveUnique(
            patternClientInstanceGetLocalPlayer(),
            "ClientInstanceGetLocalPlayer"
        );

    gLevelGetHitResult =
        resolveUnique(
            patternLevelGetHitResult(),
            "LevelGetHitResult"
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

    gMeshRenderImmediately2 =
        resolveUnique(
            patternMeshRenderImmediately2(),
            "MeshHelpersRenderMeshImmediately2"
        );

    gMeshRenderImmediately =
        resolveUnique(
            patternMeshRenderImmediately(),
            "MeshHelpersRenderMeshImmediately"
        );

    gBlockCandidates.clear();

    const auto blockMatches =
        scan(
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
        [](const Candidate& a,
           const Candidate& b) {
            return a.score > b.score;
        }
    );

    LOGI(
        "block_getOutline discovery candidates = %zu",
        gBlockCandidates.size()
    );

    /*
     * Do not treat the discovery candidates as a verified
     * Block::getOutline implementation.
     *
     * We still keep this disabled until the ABI/function identity
     * is proven.
     */
    gBlockGetOutline = 0;

    return ready();
}

bool ready() {
    return
        gModule.valid() &&
        gRenderLevel != 0 &&
        gClientInstanceUpdate != 0 &&
        gClientInstanceGetLocalPlayer != 0 &&
        gLevelGetHitResult != 0 &&
        gTessellatorBegin != 0 &&
        gTessellatorColor != 0 &&
        gTessellatorVertex != 0 &&
        (
            gMeshRenderImmediately2 != 0 ||
            gMeshRenderImmediately != 0
        );
}

std::uintptr_t libraryBase() {
    return gModule.base;
}

std::uintptr_t renderLevel() {
    return gRenderLevel;
}

std::uintptr_t clientInstanceUpdate() {
    return gClientInstanceUpdate;
}

std::uintptr_t clientInstanceGetLocalPlayer() {
    return gClientInstanceGetLocalPlayer;
}

std::uintptr_t levelGetHitResult() {
    return gLevelGetHitResult;
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

std::uintptr_t meshRenderImmediately2() {
    return gMeshRenderImmediately2;
}

std::uintptr_t blockGetOutline() {
    return gBlockGetOutline;
}

const std::vector<Candidate>&
blockOutlineCandidates() {
    return gBlockCandidates;
}

}
