#pragma once

#include <atomic>

namespace outline {

struct Config {
    std::atomic_bool enabled{true};

    std::atomic_bool rgb{true};

    std::atomic<float> red{1.0f};
    std::atomic<float> green{0.0f};
    std::atomic<float> blue{0.0f};
    std::atomic<float> alpha{1.0f};

    std::atomic<float> rgbSpeed{1.0f};

    std::atomic<float> thickness{0.025f};
};

inline Config& config() {
    static Config value;
    return value;
}

}
