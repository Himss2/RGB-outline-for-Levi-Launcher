#include "outline/hook.hpp"
#include "outline/resolver.hpp"
#include "outline/runtime.hpp"
#include "outline/renderer.hpp"

#include <android/log.h>
#include <pl/memory/Hook.hpp>

#define LOG_TAG "OutlineRGB"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace outline::hook {

namespace {

bool gInstalled = false;
void* gRenderLevelOriginal = nullptr;
void* gClientUpdateOriginal = nullptr;

void clientInstanceUpdateHook(void* clientInstance, bool value) {
    if (clientInstance)
        runtime::setClientInstance(clientInstance);

    using Fn = void* (*)(void*, bool);
    auto original = reinterpret_cast<Fn>(gClientUpdateOriginal);

    if (original)
        original(clientInstance, value);
}

void renderLevelHook(void* levelRenderer, void* screenContext, void* a3) {
    using Fn = void (*)(void*, void*, void*);
    auto original = reinterpret_cast<Fn>(gRenderLevelOriginal);

    if (original)
        original(levelRenderer, screenContext, a3);

    renderer::renderSelection(levelRenderer, screenContext);
}

bool installOne(
    std::uintptr_t address,
    void* detour,
    void** original,
    const char* name
) {
    if (!address) {
        LOGE("%s: address is null", name);
        return false;
    }

    const int rc = pl::memory::hook(
        reinterpret_cast<void*>(address),
        detour,
        original
    );

    if (rc != 0) {
        LOGE(
            "%s: hook failed rc=%d address=%p",
            name,
            rc,
            reinterpret_cast<void*>(address)
        );
        return false;
    }

    LOGI(
        "%s: hook installed address=%p original=%p",
        name,
        reinterpret_cast<void*>(address),
        *original
    );

    return true;
}

}

bool install() {
    if (gInstalled)
        return true;

    if (!resolver::ready()) {
        LOGE("Resolver is not ready");
        return false;
    }

    bool ok = true;

    ok &= installOne(
        resolver::clientInstanceUpdate(),
        reinterpret_cast<void*>(clientInstanceUpdateHook),
        &gClientUpdateOriginal,
        "ClientInstanceUpdate"
    );

    ok &= installOne(
        resolver::renderLevel(),
        reinterpret_cast<void*>(renderLevelHook),
        &gRenderLevelOriginal,
        "RenderLevel"
    );

    if (!ok) {
        LOGE("One or more runtime hooks failed");
        return false;
    }

    gInstalled = true;
    LOGI("Runtime hooks installed");
    return true;
}

void uninstall() {
    if (!gInstalled)
        return;

    if (gRenderLevelOriginal) {
        pl::memory::unhook(
            reinterpret_cast<void*>(resolver::renderLevel()),
            reinterpret_cast<void*>(renderLevelHook)
        );
    }

    if (gClientUpdateOriginal) {
        pl::memory::unhook(
            reinterpret_cast<void*>(resolver::clientInstanceUpdate()),
            reinterpret_cast<void*>(clientInstanceUpdateHook)
        );
    }

    gRenderLevelOriginal = nullptr;
    gClientUpdateOriginal = nullptr;
    runtime::setClientInstance(nullptr);
    gInstalled = false;
}

bool installed() {
    return gInstalled;
}

}
