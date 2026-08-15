#include "outline/hook.hpp"
#include "outline/renderer.hpp"
#include "outline/resolver.hpp"

#include <android/log.h>

#define LOG_TAG "OutlineRGB"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

__attribute__((constructor))
static void onLoad() {
    LOGI(
        "================================"
    );

    LOGI(
        "OutlineRGB loading"
    );

    LOGI(
        "Target: Minecraft Bedrock 26.44"
    );

    LOGI(
        "================================"
    );

    if (!outline::resolver::initialize(
            "libminecraftpe.so")) {

        LOGE(
            "Minecraft resolver failed"
        );

        return;
    }

    LOGI(
        "Minecraft resolver ready"
    );

    if (!outline::renderer::initialize()) {
        LOGE(
            "Renderer initialization failed"
        );

        return;
    }

    /*
     * At this point all known 26.44 rendering
     * addresses are resolved.
     *
     * The selection hook remains disabled until
     * the Android hook backend is actually linked.
     */
    if (!outline::hook::install()) {
        LOGE(
            "Selection hook unavailable"
        );

        LOGI(
            "Resolver/ABI probe completed successfully"
        );

        return;
    }

    LOGI(
        "OutlineRGB initialized"
    );
}

__attribute__((destructor))
static void onUnload() {
    outline::hook::uninstall();

    LOGI(
        "OutlineRGB unloaded"
    );
}
