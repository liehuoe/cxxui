#pragma once
#include <cstdint>

namespace cxxui {

struct Color {
    Color() = default;
    Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255)
        : red(r),
          green(g),
          blue(b),
          alpha(a) {}
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
    std::uint8_t alpha = 0;
};

}  // namespace cxxui