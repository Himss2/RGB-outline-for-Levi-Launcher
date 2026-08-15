#include "outline/hook.hpp"
#include "outline/resolver.hpp"

#include <android/log.h>

#define LOG_TAG "OutlineRGB"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace outline::hook {

namespace {

bool gInstalled = false;

}

bool install() {
    if (gInstalled)
        return true;

    if (!resolver::ready()) {
        LOGE("Resolver is not ready");
        return false;
    }

    /*
     * Do NOT hook a guessed address.
     *
     * The actual hook backend must be connected here once
     * the project's Android hook dependency is present.
     */
    LOGE(
        "No Android hook backend is configured"
    );

    LOGE(
        "Selection hook was NOT installed"
    );

    return false;
}

void uninstall() {
    if (!gInstalled)
        return;

    /*
     * Backend-specific unhook goes here.
     */

    gInstalled = false;
}

bool installed() {
    return gInstalled;
}

}
