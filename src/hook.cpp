#include <outline/hook.hpp>
#include <outline/config.hpp>
#include <outline/resolver.hpp>

#include <android/log.h>

#define LOG_TAG "SelectionOutline"

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace outline::hook {

namespace {

bool gInstalled = false;

/*
 * IMPORTANT:
 *
 * Hook prototype harus disesuaikan dengan ABI hasil RE
 * selectionGeometry().
 *
 * Fungsi ini menerima context vanilla yang kemudian
 * membangun geometry dari selected block/AABB.
 */
using SelectionGeometryFn = void(*)(
    void*,
    void*
);

SelectionGeometryFn gOriginalSelectionGeometry = nullptr;

/*
 * Wrapper sementara.
 *
 * Jangan mengubah AABB.
 * Jangan membangun geometry baru.
 *
 * Tujuan hook:
 *   vanilla -> build selection geometry
 *
 * sehingga kita bisa mempertahankan geometry vanilla
 * dan hanya mengganti rendering appearance.
 */
void selectionGeometryHook(
    void* a,
    void* b
) {
    if (!gOriginalSelectionGeometry) {
        return;
    }

    /*
     * Saat disabled:
     * langsung teruskan vanilla.
     */
    if (!config().enabled.load()) {
        gOriginalSelectionGeometry(a, b);
        return;
    }

    /*
     * Untuk tahap pertama, jangan melakukan modifikasi
     * terhadap argumen.
     *
     * Ini penting untuk membuktikan bahwa hook memang
     * berada di fungsi yang benar.
     */
    gOriginalSelectionGeometry(a, b);
}

}

bool install() {
    if (gInstalled)
        return true;

    if (!resolver::ready()) {
        LOGE("resolver is not ready");
        return false;
    }

    const auto target = resolver::selectionGeometry();

    if (!target) {
        LOGE("selection geometry address not found");
        return false;
    }

    LOGI(
        "selection geometry = %p",
        reinterpret_cast<void*>(target)
    );

    /*
     * TODO:
     *
     * Hubungkan ke hook backend yang sudah ada di repo.
     *
     * Contoh konsep:
     *
     * hook_backend::install(
     *     reinterpret_cast<void*>(target),
     *     reinterpret_cast<void*>(&selectionGeometryHook),
     *     reinterpret_cast<void**>(&gOriginalSelectionGeometry)
     * );
     */

    gInstalled = true;

    LOGI("selection geometry hook installed");

    return true;
}

void uninstall() {
    if (!gInstalled)
        return;

    /*
     * Remove hook using the same backend used by install().
     */

    gInstalled = false;
}

bool installed() {
    return gInstalled;
}

}
