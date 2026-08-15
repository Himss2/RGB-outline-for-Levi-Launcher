#pragma once

#include <cstdint>

namespace outline {

struct Vec3 {
    float x;
    float y;
    float z;
};

struct AABB {
    Vec3 min;
    Vec3 max;
};

struct Color {
    float r;
    float g;
    float b;
    float a;
};

}
