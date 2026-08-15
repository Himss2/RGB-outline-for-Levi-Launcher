#include "outline/resolver.hpp"
#include "outline/renderer.hpp"

#include <android/log.h>

#define LOG_TAG "OutlineRGB"
#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C"
__attribute__((visibility("default")))
void outline_rgb_init() {

    LOGI("================================");
    LOGI(" OutlineRGB standalone");
    LOGI(" initialization");
    LOGI("================================");

    if (!OutlineResolver::initialize()) {
        LOGI("Resolver initialization failed");
        return;
    }

    if (!OutlineRenderer::initialize()) {
        LOGI("Renderer initialization failed");
        return;
    }

    LOGI("Subsystems initialized");

    /*
     * Deliberately no hook yet.
     *
     * The next verified step is:
     *
     * renderOutlineSelection ABI
     *        ↓
     * callback
     *        ↓
     * diagnostic logging
     */
}
