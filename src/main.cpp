#include "outline/hook.hpp"
#include "outline/renderer.hpp"
#include "outline/resolver.hpp"

#include <android/log.h>

#include <chrono>
#include <thread>

#define LOG_TAG "OutlineRGB"

#define LOGI(...) \
    __android_log_print( \
        ANDROID_LOG_INFO, \
        LOG_TAG, \
        __VA_ARGS__ \
    )

#define LOGE(...) \
    __android_log_print( \
        ANDROID_LOG_ERROR, \
        LOG_TAG, \
        __VA_ARGS__ \
    )

namespace {

void initializeRuntime() {
    constexpr int maxAttempts = 100;

    for (
        int attempt = 1;
        attempt <= maxAttempts;
        ++attempt
    ) {
        LOGI(
            "Initialization attempt %d/%d",
            attempt,
            maxAttempts
        );

        if (!outline::resolver::initialize(
                "libminecraftpe.so"
            )) {

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100)
            );

            continue;
        }

        LOGI(
            "Minecraft resolver initialized"
        );

        if (!outline::renderer::initialize()) {
            LOGE(
                "Renderer initialization failed"
            );

            return;
        }

        if (!outline::hook::install()) {
            LOGE(
                "Runtime hook installation failed"
            );

            return;
        }

        LOGI(
            "================================"
        );

        LOGI(
            "OutlineRGB initialized"
        );

        LOGI(
            "Minecraft Bedrock target: 26.44"
        );

        LOGI(
            "================================"
        );

        return;
    }

    LOGE(
        "Minecraft was not available after "
        "10 seconds"
    );
}

}

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

    /*
     * LeviLauncher may load the native library
     * before libminecraftpe.so exists.
     */
    std::thread(
        initializeRuntime
    ).detach();
}

__attribute__((destructor))
static void onUnload() {
    outline::hook::uninstall();

    LOGI(
        "OutlineRGB unloaded"
    );
}
