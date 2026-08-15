#include <outline/hook.hpp>
#include <outline/resolver.hpp>

#include <android/log.h>

#define LOG_TAG "SelectionOutline"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

__attribute__((constructor))
static void onLoad() {
    LOGI("SelectionOutline loading");

    /*
     * Minecraft native library.
     */
    if (!outline::resolver::initialize("libminecraftpe.so")) {
        LOGE("failed to resolve libminecraftpe.so");
        return;
    }

    LOGI("Minecraft symbols resolved");

    if (!outline::hook::install()) {
        LOGE("failed to install selection hook");
        return;
    }

    LOGI("SelectionOutline initialized");
}

__attribute__((destructor))
static void onUnload() {
    outline::hook::uninstall();

    LOGI("SelectionOutline unloaded");
}
