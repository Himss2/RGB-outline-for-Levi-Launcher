#pragma once

#include <cstdint>

namespace outline {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct AABB {
    Vec3 min;
    Vec3 max;
};

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

struct SelectionBox {
    AABB bounds{};
    bool valid = false;
};

} // namespace outline
