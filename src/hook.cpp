#include "outline/hook.hpp"

#include "outline/renderer.hpp"
#include "outline/resolver.hpp"
#include "outline/runtime.hpp"

#include <android/log.h>

#include <pl/memory/Hook.hpp>

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

namespace outline::hook {

namespace {

bool gInstalled = false;

void* gRenderLevelOriginal = nullptr;

void* gClientUpdateOriginal = nullptr;

void clientInstanceUpdateHook(
    void* clientInstance,
    bool value
) {
    if (clientInstance) {
        runtime::setClientInstance(
            clientInstance
        );
    }

    using Fn =
        void* (*)(void*, bool);

    const auto original =
        reinterpret_cast<Fn>(
            gClientUpdateOriginal
        );

    if (original) {
        original(
            clientInstance,
            value
        );
    }
}

void renderLevelHook(
    void* levelRenderer,
    void* screenContext
) {
    using Fn =
        void (*)(void*, void*);

    const auto original =
        reinterpret_cast<Fn>(
            gRenderLevelOriginal
        );

    /*
     * Always let vanilla render first.
     */
    if (original) {
        original(
            levelRenderer,
            screenContext
        );
    }

    /*
     * Buat Dummy Box untuk memastikan mod merender warna merah.
     * Jika mod berjalan, akan ada kotak merah raksasa di koordinat X:0, Y:100, Z:0
     */
    outline::SelectionBox dummyBox;
    dummyBox.valid = true;
    dummyBox.bounds.min = {0.0f, 100.0f, 0.0f};
    dummyBox.bounds.max = {1.0f, 101.0f, 1.0f};

    outline::Color redOutline = {1.0f, 0.0f, 0.0f, 1.0f};

    /*
     * Then draw our overlay.
     */
    renderer::renderSelectionBox(
        screenContext,
        dummyBox,
        redOutline
    );
}

bool installOne(
    std::uintptr_t address,
    void* detour,
    void** original,
    const char* name
) {
    if (!address) {
        LOGE(
            "%s: address is null",
            name
        );

        return false;
    }

    const int rc =
        pl::memory::hook(
            reinterpret_cast<void*>(
                address
            ),
            detour,
            original
        );

    if (rc != 0) {
        LOGE(
            "%s: hook failed rc=%d address=%p",
            name,
            rc,
            reinterpret_cast<void*>(
                address
            )
        );

        return false;
    }

    LOGI(
        "%s: hook installed address=%p original=%p",
        name,
        reinterpret_cast<void*>(
            address
        ),
        *original
    );

    return true;
}

}

bool install() {
    if (gInstalled)
        return true;

    if (!resolver::ready()) {
        LOGE(
            "Resolver is not ready"
        );

        return false;
    }

    bool clientHook =
        installOne(
            resolver::clientInstanceUpdate(),
            reinterpret_cast<void*>(
                clientInstanceUpdateHook
            ),
            &gClientUpdateOriginal,
            "ClientInstanceUpdate"
        );

    bool renderHook =
        installOne(
            resolver::renderLevel(),
            reinterpret_cast<void*>(
                renderLevelHook
            ),
            &gRenderLevelOriginal,
            "RenderLevel"
        );

    if (!clientHook ||
        !renderHook) {

        LOGE(
            "Runtime hook installation failed"
        );

        return false;
    }

    gInstalled = true;

    LOGI(
        "All runtime hooks installed"
    );

    return true;
}

void uninstall() {
    if (!gInstalled)
        return;

    if (gRenderLevelOriginal) {
        pl::memory::unhook(
            reinterpret_cast<void*>(
                resolver::renderLevel()
            ),
            reinterpret_cast<void*>(
                renderLevelHook
            )
        );
    }

    if (gClientUpdateOriginal) {
        pl::memory::unhook(
            reinterpret_cast<void*>(
                resolver::clientInstanceUpdate()
            ),
            reinterpret_cast<void*>(
                clientInstanceUpdateHook
            )
        );
    }

    gRenderLevelOriginal = nullptr;

    gClientUpdateOriginal = nullptr;

    runtime::setClientInstance(
        nullptr
    );

    gInstalled = false;

    LOGI(
        "Runtime hooks removed"
    );
}

bool installed() {
    return gInstalled;
}

}
