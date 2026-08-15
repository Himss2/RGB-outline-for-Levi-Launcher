#include "outline/hook.hpp"

#include <android/log.h>

#define LOG_TAG "OutlineRGB"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace {

OutlineHook::Address gTarget = 0;
bool gInstalled = false;

}

namespace OutlineHook {

bool initialize() {
    gTarget = 0;
    gInstalled = false;

    LOGI("Hook subsystem initialized");

    return true;
}

bool install(Address target) {
    if (!target) {
        LOGI("Hook rejected: target is null");
        return false;
    }

    /*
     * IMPORTANT:
     *
     * We intentionally do NOT install a native hook yet.
     *
     * The callback ABI of renderOutlineSelection has not
     * been proven sufficiently.
     */

    LOGI(
        "Diagnostic target received: %p",
        reinterpret_cast<void*>(target)
    );

    LOGI(
        "Native hook installation is intentionally deferred"
    );

    gTarget = target;

    return false;
}

void uninstall() {
    gTarget = 0;
    gInstalled = false;

    LOGI("Hook subsystem reset");
}

bool installed() {
    return gInstalled;
}

Address target() {
    return gTarget;
}

}
