#pragma once

#include <atomic>

namespace outline {

struct Config {
    std::atomic<bool> enabled{true};

    std::atomic<float> red{1.0f};
    std::atomic<float> green{0.0f};
    std::atomic<float> blue{0.0f};
    std::atomic<float> alpha{1.0f};

    std::atomic<float> lineWidth{2.0f};
};

Config& config();

}
