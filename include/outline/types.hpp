#pragma once

#include <cstdint>

namespace outline {

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

struct AABB {
    Vec3 min{};
    Vec3 max{};
};

struct Color {
    float r{};
    float g{};
    float b{};
    float a{};
};

struct ModuleRange {
    std::uintptr_t base{};
    std::uintptr_t textBegin{};
    std::uintptr_t textEnd{};

    bool valid() const {
        return base != 0 &&
               textBegin != 0 &&
               textEnd > textBegin;
    }
};

}
