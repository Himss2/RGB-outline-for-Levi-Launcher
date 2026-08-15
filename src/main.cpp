#include "outline/resolver.hpp"
#include "outline/hook.hpp"

#include <android/log.h>

#define LOG_TAG "OutlineRGB"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C"
__attribute__((visibility("default")))
void outline_rgb_init() {

    LOGI("--------------------------------");
    LOGI("OutlineRGB ABI Probe");
    LOGI("--------------------------------");

    if (!OutlineResolver::initialize()) {
        LOGI("Resolver initialization failed");
        return;
    }

    if (!OutlineHook::initialize()) {
        LOGI("Hook subsystem initialization failed");
        return;
    }

    OutlineResolver::MemoryRange library;

    if (!OutlineResolver::findMinecraftLibrary(library)) {
        LOGI("Minecraft library unavailable");
        return;
    }

    OutlineResolver::MemoryRange text;

    if (!OutlineResolver::findTextRange(
            library.begin,
            text)) {
        LOGI("Minecraft executable range unavailable");
        return;
    }

    /*
     * ThickBaddie discovery signature.
     *
     * IMPORTANT:
     *
     * This is only a discovery signature.
     * It is NOT treated as renderOutlineSelection.
     */

    static const uint8_t thickBaddiePattern[] = {
        0xFD, 0x7B, 0xBA, 0xA9,
        0xFC, 0x6F, 0x01, 0xA9,
        0xFA, 0x67, 0x02, 0xA9,
        0xF8, 0x5F, 0x03, 0xA9,
        0xF6, 0x57, 0x04, 0xA9
    };

    static const char thickBaddieMask[] =
        "xxxxxxxxxxxxxxxxxxxx";

    const auto matches =
        OutlineResolver::scan(
            text,
            thickBaddiePattern,
            thickBaddieMask,
            sizeof(thickBaddiePattern)
        );

    LOGI(
        "Discovery signature matches: %zu",
        matches.size()
    );

    for (size_t i = 0; i < matches.size(); ++i) {

        const auto address = matches[i];

        const int score =
            OutlineResolver::scoreCandidate(address);

        LOGI(
            "candidate[%zu] = %p score=%d",
            i,
            reinterpret_cast<void*>(address),
            score
        );
    }

    LOGI("--------------------------------");
    LOGI("ABI probe initialization done");
    LOGI("--------------------------------");
}
