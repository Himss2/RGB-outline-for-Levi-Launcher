#include <outline/config.hpp>
#include <outline/renderer.hpp>
#include <outline/resolver.hpp>
#include <outline/types.hpp>

#include <cstdint>

namespace outline {

namespace {

constexpr std::uintptr_t TESSELLATOR_COLOR_OFFSET = 0x1A0;

/*
 * Layout minimum yang kita butuhkan dari hasil RE.
 *
 * Jangan memperlakukan ini sebagai full Minecraft class.
 */
struct TessellatorView {
    std::byte padding[TESSELLATOR_COLOR_OFFSET];

    std::uint32_t packedColor;
};

std::uint32_t packColor(
    float r,
    float g,
    float b,
    float a
) {
    auto clamp = [](float v) {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    };

    const auto R =
        static_cast<std::uint32_t>(clamp(r) * 255.0f);

    const auto G =
        static_cast<std::uint32_t>(clamp(g) * 255.0f);

    const auto B =
        static_cast<std::uint32_t>(clamp(b) * 255.0f);

    const auto A =
        static_cast<std::uint32_t>(clamp(a) * 255.0f);

    return
        (R << 24) |
        (G << 16) |
        (B << 8) |
        A;
}

std::uint32_t currentColor() {
    auto& cfg = config();

    return packColor(
        cfg.red.load(),
        cfg.green.load(),
        cfg.blue.load(),
        cfg.alpha.load()
    );
}

}

Config& config() {
    static Config instance;
    return instance;
}

}
