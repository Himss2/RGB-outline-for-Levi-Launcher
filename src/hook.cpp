#include <android/log.h>

#define LOG_TAG "OutlineRGB"
#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace OutlineHook {

bool install() {
    LOGI(
        "Hook installation deferred until "
        "renderOutlineSelection ABI is verified"
    );

    return false;
}

}
