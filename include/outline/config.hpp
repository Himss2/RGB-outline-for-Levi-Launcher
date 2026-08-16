#pragma once

#include <cstdint>

namespace outline {

struct Config {
    bool enabled = true;

    // RGB animation
    float saturation = 1.0f;
    float brightness = 1.0f;

    // Animation speed.
    // Higher = faster color transition.
    float speed = 1.0f;

    // Selection box line width.
    float lineWidth = 2.0f;

    // Prevent accidental rendering before
    // the Minecraft target has been resolved.
    bool requireResolvedTarget = true;
};

inline Config& config() {
    static Config instance;
    return instance;
}

} // namespace outline
